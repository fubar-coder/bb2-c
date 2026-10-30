/*
 * ridatatypeslib - RWE userlib 25 (RI DataTypes Lib, Leading Edge
 * Software, S. Matty, 1995/96): one file at a time through
 * datatypes.library - DataTypeOpen, DataTypeClose, DataTypeName,
 * DataTypeBaseName, DataTypeID, DataTypeGroup, DataTypeCreate,
 * AttachDataType, DetachDataType, DataTypeReady, DataTypeSuper,
 * DataTypeBitmap, DataTypesPalette, DataTypeWidth, DataTypeHeight,
 * DataTypeDepth, SetDataTypeAttr, GetDataTypeAttr, DataTypeModeID.
 *
 * C translation of rwelibsrc/datatypeslib6.bb (with #beta=1: without
 * BitmapDataType, as the source builds); the reference is the reference
 * build of that source by blitz2 2.10 (REFERENCE "refbuild",
 * ref/ridatatypeslib.obj; the BSS2 RIDATATYPESLIB.OBJ is an earlier build
 * of the same source, without the `TST.l database` checks).  Declarations
 * equal (ctest rwe_declarations_ridatatypeslib), the behaviour is
 * compared with it in libs/tests/rwe/ridatatypeslib.c.  Notes:
 * docs/rwelibs/port/ridatatypeslib.md.  All commands are Amiga mode only.
 *   init:  OpenLibrary datatypes, dos, intuition, graphics (V37; a missing
 *          one gives 0, the commands that test its base do nothing);
 *          storage = memlib globalloc(1024, MEMF_CLEAR) - the tag list
 *          and GetAttr buffer of every command
 *   finit: DetachDataType, DataTypeClose, CloseLibrary, globfree
 *
 * State: the Lock of the file (DataTypeOpen; its name is copied to fname,
 * the name of NewDTObject), the DataType (ObtainDataType) and its
 * DataTypeHeader, the object (DataTypeCreate), its BitMapHeader
 * (DataTypeBitmap) and the window it is attached to (AttachDataType: the
 * window object of windowslib in a0 while attaching, the Window pointer
 * afterwards).  The string functions write at a3 (blitz_string).
 *
 * Everything is C; the libraries through the NDK inlines with the
 * original's base table (libbases), memlib through BLITZ_ALIBJSR.  The
 * thunks keep d0/d1/a0/a1 except the result; the original changes a6
 * everywhere, a0/a1 nearly everywhere, d1 (the string functions, d1 in
 * DataTypeOpen ...), d2 (DataTypeOpen), d3 (DataTypesPalette), a2
 * (init/finit, AttachDataType) and d7/a2 (SetDataTypeAttr with pairs).
 *
 * BUG(orig):
 *   - DataTypeOpen copies the name into its 384 byte buffer without a
 *     limit and reads the DataTypeHeader from the result of
 *     ObtainDataType without checking it for 0 (#604, #605).
 *   - DataTypeClose disposes of the object without detaching it from the
 *     window; it unlocks the file only if there is a DataType (#599, #605).
 *   - DataTypeCreate without a tag list does not need an open file (the
 *     name is the last one DataTypeOpen copied, at first ""); a second
 *     DataTypeCreate loses the object before (#601, #600).
 *   - AttachDataType window,id reads the size from the BitMapHeader
 *     without checking it (DataTypeBitmap not called: address 0);
 *     AttachDataType window,id,x,y,w,h puts the word arguments into the
 *     tag list as longs (with the upper words the registers have);
 *     AttachDataType window,id,&taglist passes the id as the tag list to
 *     SetAttrsA and ignores the tag list (#595, #597, #596).
 *   - DataTypeBitmap takes whatever is in the buffer if GetAttr fails
 *     (#598).
 *   - DataTypesPalette copies as many colours as the palette has (read
 *     behind the datatype's colours; 0 colours: 65536); without colours
 *     it returns GetAttr's result, otherwise the address of its buffer
 *     (#606, #607).
 *   - SetDataTypeAttr with pairs: 0 pairs are 65536 (DBRA), more than 127
 *     overflow the 1024 byte buffer (#608).
 *   - DataTypeModeID with a tag list writes its own tags into it (#603).
 *   - finit closes the libraries only up to the first one that did not
 *     open (#602).
 * DataTypeHeight, DataTypeDepth, DataTypeModeID, DataTypesPalette,
 * SetDataTypeAttr, GetDataTypeAttr, DetachDataType and AttachDataType
 * (but its second form) do not check datatypes.library, like the
 * original: no defect, without it there is no object nor header, so
 * they do nothing either.
 * DIFF: a2 unchanged by SetDataTypeAttr with pairs (the original leaves
 * it below the pairs), as in all C ports (docs/libs/cmdlibs-port.md).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#define __NOLIBBASE__
#define DATATYPES_BASE_NAME database
#define DOS_BASE_NAME dosbase
#define INTUITION_BASE_NAME intuibase
#define GRAPHICS_BASE_NAME gfxbase
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <graphics/displayinfo.h>

/* entries (thunks below) */
void _datatypeopen(void), _datatypeclose(void), _datatypename(void);
void _datatypebasename(void), _datatypeid(void), _datatypecreate2(void);
void _datatypecreate(void), _attachdatatype(void), _attachdatatype2(void);
void _attachdatatype3(void), _detachdatatype(void), _datatypesuper(void);
void _datatypebitmap(void), _datatypespalette(void), _datatypewidth(void);
void _datatypeheight(void), _datatypedepth(void), _datatypeready(void);
void _setdatatypeattr(void), _setdatatypeattr2(void), _getdatatypeattr(void);
void _getmodeid(void), _getmodeid2(void), _datatypegroup(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

#define IA(n) (BL_LA(n) | BL_ITEM | BL_PD(n))   /* #ia<n>|#pd<n> */

/* !libheader {#ridatatypeslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ridatatypeslib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypeopen), 0, 0);
BLITZ_NAME("DataTypeOpen", "filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypeclose), 0, 0);
BLITZ_NAME("DataTypeClose", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypename), 0, 0);
BLITZ_NAME("DataTypeName", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypebasename), 0, 0);
BLITZ_NAME("DataTypeBaseName", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypeid), 0, 0);
BLITZ_NAME("DataTypeID", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypecreate2), 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypecreate), 0, 0);
BLITZ_NAME("DataTypeCreate", "[&taglist]");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_windowslib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(_attachdatatype), 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(_attachdatatype2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_windowslib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(_attachdatatype3), 0, 0);
BLITZ_NAME("AttachDataType", "window,id[,x,y,width,height] | window,id,&taglist");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_detachdatatype), 0, 0);
BLITZ_NAME("DetachDataType", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypesuper), 0, 0);
BLITZ_NAME("DataTypeSuper", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypebitmap), 0, 0);
BLITZ_NAME("DataTypeBitmap", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypespalette), 0, 0);
BLITZ_NAME("DataTypesPalette", "palette#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypewidth), 0, 0);
BLITZ_NAME("DataTypeWidth", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypeheight), 0, 0);
BLITZ_NAME("DataTypeHeight", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypedepth), 0, 0);
BLITZ_NAME("DataTypeDepth", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypeready), 0, 0);
BLITZ_NAME("DataTypeReady", "");

/* RedrawDataType, commented out in the source: an empty entry */
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_setdatatypeattr), 0, 0);
BLITZ_REPARGS(0, 2, BL_LONG | BL_PUSH, BL_LONG | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_setdatatypeattr2), 0, 0);
BLITZ_NAME("SetDataTypeAttr", "&taglist / attr,data[,attr,data..]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_getdatatypeattr), 0, 0);
BLITZ_NAME("GetDataTypeAttr", "attribute");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_getmodeid), 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_getmodeid2), 0, 0);
BLITZ_NAME("DataTypeModeID", "[taglist]]");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_datatypegroup), 0, 0);
BLITZ_NAME("DataTypeGroup", "");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

/* libbases: datatypes, dos, intuition, graphics, 0 (finit's end) */
static struct Library *libbases[5];
#define database  libbases[0]
#define dosbase   libbases[1]
#define intuibase libbases[2]
#define gfxbase   libbases[3]

static const char *const libnames[] = {
    "datatypes.library", "dos.library", "intuition.library", "graphics.library", NULL
};

static ULONG *storage;
static BPTR lock;
static struct DataType *dtype;
static struct DataTypeHeader *dtypehead;
static Object *object;
static UBYTE fname[384];
static struct BitMapHeader *bitmapheader;
static ULONG attached;                  /* window object, then the Window */
static UWORD attachtobitmap;

void BLITZ_ROUTINE c_libinit(void)
{
    struct bl_regs r = { .d0 = 1024, .d1 = MEMF_CLEAR };
    int i;

    for (i = 0; libnames[i]; i++)
        libbases[i] = OpenLibrary((CONST_STRPTR)libnames[i], 37);
    storage = (ULONG *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

/* DetachDataType */
void BLITZ_ROUTINE c_detachdatatype(void)
{
    if ((LONG)attached <= 0)
        return;
    if (attachtobitmap)
        return;
    RemoveDTObject((struct Window *)attached, object);
    attached = 0;
}

/* DataTypeClose: the object, the DataType, the lock */
void BLITZ_ROUTINE c_datatypeclose(void)
{
    if (!database)
        return;
    if (object) {
        DisposeDTObject(object);        /* BUG(orig) (#599): not detached */
        object = NULL;
    }
    if (!dtype)
        return;
    ReleaseDataType(dtype);
    dtype = NULL;
    UnLock(lock);
    bitmapheader = NULL;
    lock = 0;
    dtypehead = NULL;
}

void BLITZ_ROUTINE c_libfinit(void)
{
    struct Library **p;
    struct bl_regs r;

    c_detachdatatype();
    c_datatypeclose();
    for (p = libbases; *p; p++) {       /* BUG(orig) (#602): up to the first 0 */
        struct Library *l = *p;

        *p = NULL;
        CloseLibrary(l);
    }
    r = (struct bl_regs){ .d0 = 1024, .a1 = (ULONG)storage };
    BLITZ_ALIBJSR(TOK_globfree, r);
    storage = NULL;
}

/* a string with its 0 to a3: the length (a3 behind the 0) */
static ULONG putstring(const UBYTE *s)
{
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    return n;
}

/* up to four characters of an ID (stops at a 0) to a3 */
static ULONG putid(const UBYTE *s)
{
    ULONG n;
    UBYTE c;

    for (n = 0;; n++) {
        c = s[n];
        if (!c || n == 4)
            return n;
        *blitz_string++ = c;
    }
}

/* DataTypeOpen filename$: the DataType of the file (0: no file, or one
   is open) */
ULONG BLITZ_ROUTINE c_datatypeopen(CONST_STRPTR name BL_REG(d0))
{
    const UBYTE *s = (const UBYTE *)name;
    UBYTE *d = fname;

    if (!database || lock)
        return 0;
    while ((*d++ = *s++) != 0)          /* BUG(orig) (#604): no limit */
        ;
    lock = Lock(name, SHARED_LOCK);
    if (!lock)
        return 0;
    dtype = ObtainDataTypeA(DTST_FILE, (APTR)lock, NULL);
    dtypehead = dtype->dtn_Header;      /* BUG(orig) (#605): 0 not checked */
    return (ULONG)dtype;
}

/* DataTypeName / DataTypeBaseName / DataTypeID / DataTypeGroup */
ULONG BLITZ_ROUTINE c_datatypename(void)
{
    if (!database || !dtypehead)
        return 0;
    return putstring((const UBYTE *)dtypehead->dth_Name);
}

ULONG BLITZ_ROUTINE c_datatypebasename(void)
{
    if (!database || !dtype)
        return 0;
    return putstring((const UBYTE *)dtypehead->dth_BaseName);
}

ULONG BLITZ_ROUTINE c_datatypeid(void)
{
    if (!database || !dtype)
        return 0;
    return putid((const UBYTE *)&dtypehead->dth_ID);
}

ULONG BLITZ_ROUTINE c_datatypegroup(void)
{
    if (!database || !dtypehead)
        return 0;
    return putid((const UBYTE *)&dtypehead->dth_GroupID);
}

/* DataTypeCreate [&taglist]: NewDTObject of the file's name */
static ULONG create(struct TagItem *tags)
{
    object = NewDTObjectA(fname, tags); /* BUG(orig) (#600): the old one lost */
    return (ULONG)object;
}

ULONG BLITZ_ROUTINE c_datatypecreate2(void)
{
    if (!database)
        return 0;
    storage[0] = TAG_DONE;
    return create((struct TagItem *)storage);
}

ULONG BLITZ_ROUTINE c_datatypecreate(struct TagItem *tags BL_REG(d0))
{
    if (!database || !lock)
        return 0;
    return create(tags);
}

/* doattach: the window of the window object, AddDTObject */
static ULONG doattach(const ULONG *winobj)
{
    attachtobitmap = 0;
    attached = *winobj;
    AddDTObject((struct Window *)attached, NULL, object, -1);
    return (ULONG)-1;
}

/* the tags every form ends with */
static ULONG *tailtags(ULONG *t)
{
    *t++ = ICA_TARGET;
    *t++ = (ULONG)-1;
    *t++ = PDTA_Remap;
    *t++ = 0;
    *t = TAG_DONE;
    return t;
}

/* AttachDataType window,id: at 0,0 with the size of the BitMapHeader */
ULONG BLITZ_ROUTINE c_attachdatatype(ULONG id BL_REG(d1), ULONG *winobj BL_REG(a0))
{
    ULONG *t = storage;

    if (!lock || attached)
        return 0;
    attached = (ULONG)winobj;
    *t++ = GA_Left;
    *t++ = 0;
    *t++ = GA_Top;
    *t++ = 0;
    *t++ = GA_Width;
    *t++ = bitmapheader->bmh_Width;     /* BUG(orig) (#595): 0 not checked */
    *t++ = GA_Height;
    *t++ = bitmapheader->bmh_Height;
    *t++ = GA_ID;
    *t++ = id;
    tailtags(t);
    SetAttrsA(object, (struct TagItem *)storage);
    return doattach((const ULONG *)attached);
}

/* AttachDataType window,id,x,y,width,height (BUG(orig) (#597): the words as
   longs) */
ULONG BLITZ_ROUTINE c_attachdatatype2(ULONG id BL_REG(d1), ULONG x BL_REG(d2),
                                      ULONG y BL_REG(d3), ULONG w BL_REG(d4),
                                      ULONG h BL_REG(d5), ULONG *winobj BL_REG(a0))
{
    ULONG *t = storage;

    if (!database || !lock || attached)
        return 0;
    attached = (ULONG)winobj;
    *t++ = GA_ID;
    *t++ = id;
    *t++ = GA_Left;
    *t++ = x;
    *t++ = GA_Top;
    *t++ = y;
    *t++ = GA_Width;
    *t++ = w;
    *t++ = GA_Height;
    *t++ = h;
    tailtags(t);
    SetAttrsA(object, (struct TagItem *)storage);
    return doattach((const ULONG *)attached);
}

/* AttachDataType window,id,&taglist (BUG(orig) (#596): the id is the tag list
   of the second SetAttrsA) */
ULONG BLITZ_ROUTINE c_attachdatatype3(ULONG id BL_REG(d1), ULONG *winobj BL_REG(a0))
{
    ULONG *t = storage;

    if (!lock || attached)
        return 0;
    attached = (ULONG)winobj;
    *t++ = GA_ID;
    *t++ = id;
    tailtags(t);
    SetAttrsA(object, (struct TagItem *)storage);
    SetAttrsA(object, (struct TagItem *)id);
    return doattach((const ULONG *)attached);
}

/* DataTypeReady: -1 when the layout is done */
ULONG BLITZ_ROUTINE c_datatypeready(void)
{
    if (!database || !attached)
        return 0;
    storage[0] = 0;
    GetAttr(DTA_LayoutProc, object, storage);
    if (storage[0])
        return 0;
    return (ULONG)-1;
}

/* DataTypeSuper: the ID of the object's class */
ULONG BLITZ_ROUTINE c_datatypesuper(void)
{
    if (!database || !object)
        return 0;
    return putstring((const UBYTE *)OCLASS(object)->cl_ID);
}

/* DataTypeBitmap: the BitMapHeader (BUG(orig) (#598): GetAttr not checked) */
ULONG BLITZ_ROUTINE c_datatypebitmap(void)
{
    if (!database || !object)
        return 0;
    GetAttr(PDTA_BitMapHeader, object, storage);
    bitmapheader = (struct BitMapHeader *)storage[0];
    return (ULONG)bitmapheader;
}

/* DataTypesPalette palette#: the colours into the palette object (count.w,
   first.w, then a long per component; the upper bytes set) */
ULONG BLITZ_ROUTINE c_datatypespalette(UBYTE **palobj BL_REG(a0))
{
    const UBYTE *cr;
    UBYTE *p;
    UWORD n;
    ULONG r;

    if (!object)
        return 0;
    r = GetAttr(PDTA_ColorRegisters, object, storage);
    cr = (const UBYTE *)storage[0];
    if (!cr)
        return r;                       /* BUG(orig) (#607): GetAttr's result */
    p = *palobj;
    n = *(const UWORD *)p;
    p += 4;
    n--;
    do {                                /* BUG(orig) (#606): DBRA, 0 = 65536 */
        p[0] = *cr++;
        p[4] = *cr++;
        p[8] = *cr++;
        p += 12;
    } while (n-- != 0);
    return (ULONG)storage;
}

/* DataTypeWidth / DataTypeHeight / DataTypeDepth */
UWORD BLITZ_ROUTINE c_datatypewidth(void)
{
    if (!database || !bitmapheader)
        return 0;
    return bitmapheader->bmh_Width;
}

UWORD BLITZ_ROUTINE c_datatypeheight(void)
{
    if (!bitmapheader)
        return 0;
    return bitmapheader->bmh_Height;
}

UWORD BLITZ_ROUTINE c_datatypedepth(void)
{
    if (!bitmapheader)
        return 0;
    return (UWORD)(WORD)(BYTE)bitmapheader->bmh_Depth;
}

/* SetDataTypeAttr &taglist */
void BLITZ_ROUTINE c_setdatatypeattr(struct TagItem *tags BL_REG(d0))
{
    if (!object)
        return;
    SetAttrsA(object, tags);
}

/* SetDataTypeAttr attr,data[,attr,data...]: d7 pairs below a2 */
void BLITZ_ROUTINE c_setdatatypeattr2(UWORD n BL_REG(d7), const ULONG *sp BL_REG(a2))
{
    ULONG *t = storage;

    if (!object)
        return;
    n--;
    do {                                /* BUG(orig) (#608): DBRA, no limit */
        *t++ = *--sp;
        *t++ = *--sp;
    } while (n-- != 0);
    *t = TAG_DONE;
    SetAttrsA(object, (struct TagItem *)storage);
}

/* GetDataTypeAttr(attribute): the address of the value */
ULONG BLITZ_ROUTINE c_getdatatypeattr(ULONG attr BL_REG(d0))
{
    if (!object)
        return 0;
    GetAttr(attr, object, storage);
    return (ULONG)storage;
}

/* DataTypeModeID [taglist]: BestModeID for the picture's size and depth
   (the tags are written to the tag list, BUG(orig) (#603)) */
static ULONG domodeid(ULONG *t)
{
    if (!bitmapheader)
        return 0;
    t[0] = BIDTAG_NominalWidth;
    t[1] = bitmapheader->bmh_Width;
    t[2] = BIDTAG_NominalHeight;
    t[3] = bitmapheader->bmh_Height;
    t[4] = BIDTAG_Depth;
    t[5] = bitmapheader->bmh_Depth;
    t[6] = TAG_DONE;
    return BestModeIDA((struct TagItem *)t);
}

ULONG BLITZ_ROUTINE c_getmodeid(void)
{
    return domodeid(storage);
}

ULONG BLITZ_ROUTINE c_getmodeid2(ULONG *tags BL_REG(d0))
{
    return domodeid(tags);
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(_datatypeopen, c_datatypeopen, "d1/a0-a1");
BLITZ_THUNK(_datatypeclose, c_datatypeclose, "d0-d1/a0-a1");
BLITZ_THUNK(_datatypename, c_datatypename, "d1/a0-a1");
BLITZ_THUNK(_datatypebasename, c_datatypebasename, "d1/a0-a1");
BLITZ_THUNK(_datatypeid, c_datatypeid, "d1/a0-a1");
BLITZ_THUNK(_datatypegroup, c_datatypegroup, "d1/a0-a1");
BLITZ_THUNK(_datatypecreate2, c_datatypecreate2, "d1/a0-a1");
BLITZ_THUNK(_datatypecreate, c_datatypecreate, "d1/a0-a1");
BLITZ_THUNK(_attachdatatype, c_attachdatatype, "d1/a0-a1");
BLITZ_THUNK(_attachdatatype2, c_attachdatatype2, "d1/a0-a1");
BLITZ_THUNK(_attachdatatype3, c_attachdatatype3, "d1/a0-a1");
BLITZ_THUNK(_detachdatatype, c_detachdatatype, "d0-d1/a0-a1");
BLITZ_THUNK(_datatypeready, c_datatypeready, "d1/a0-a1");
BLITZ_THUNK(_datatypesuper, c_datatypesuper, "d1/a0-a1");
BLITZ_THUNK(_datatypebitmap, c_datatypebitmap, "d1/a0-a1");
BLITZ_THUNK(_datatypespalette, c_datatypespalette, "d1/a0-a1");
BLITZ_THUNK(_datatypewidth, c_datatypewidth, "d1/a0-a1");
BLITZ_THUNK(_datatypeheight, c_datatypeheight, "d1/a0-a1");
BLITZ_THUNK(_datatypedepth, c_datatypedepth, "d1/a0-a1");
BLITZ_THUNK(_setdatatypeattr, c_setdatatypeattr, "d0-d1/a0-a1");
BLITZ_THUNK(_setdatatypeattr2, c_setdatatypeattr2, "d0-d1/a0-a1");
BLITZ_THUNK(_getdatatypeattr, c_getdatatypeattr, "d1/a0-a1");
BLITZ_THUNK(_getmodeid, c_getmodeid, "d1/a0-a1");
BLITZ_THUNK(_getmodeid2, c_getmodeid2, "d1/a0-a1");
