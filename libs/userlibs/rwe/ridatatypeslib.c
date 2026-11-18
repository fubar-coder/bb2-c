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
 * a block of its own length, the name of NewDTObject), the DataType (ObtainDataType) and its
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
 *   - SetDataTypeAttr with pairs: 0 pairs are 65536 (DBRA), more than 127
 *     overflow the 1024 byte buffer (#608).
 *
 * Fixed (intended deviations):
 *   - DataTypeBitmap clears the buffer before GetAttr: an object that has
 *     no BitMapHeader (not a picture) gives 0.  The original took
 *     whatever the command before had left in the buffer (#598).
 *   - AttachDataType window,id needs an object and gets the BitMapHeader
 *     itself (as DataTypeBitmap) if DataTypeBitmap was not called; an
 *     object without one (not a picture) is not attached (BB_FALSE).
 *     The original read the size through the unchecked pointer: without
 *     DataTypeBitmap the two words at address 0, an object of 0 x 0
 *     (#595).
 *   - DataTypeClose detaches the object from its window first, as finit
 *     does.  The original disposed of it while still attached: the
 *     window kept the freed gadget, and `attached` stayed set, so the
 *     next AttachDataType returned 0 (#599).
 *   - DataTypeOpen unlocks the file again and returns 0 if
 *     ObtainDataType fails; DataTypeClose unlocks the file whenever it
 *     is locked.  The original read the DataTypeHeader through the
 *     address 28 and kept the lock, which DataTypeClose released only
 *     with a DataType: DataTypeOpen refused every further file (#605).
 *   - DataTypeCreate without a tag list needs an open file, as the form
 *     with one.  The original made an object of the name the last
 *     DataTypeOpen had copied (at first "", later the name of a file
 *     closed since) (#601).
 *   - DataTypeCreate (both forms) returns 0 while an object exists; the
 *     object stays in use, as DataTypeOpen and AttachDataType refuse a
 *     second file or window (user decision W4 Q4).  The original stored
 *     the new object over the old one, which was never disposed of
 *     (#600).
 *   - DataTypeOpen copies the name into a block of its own length
 *     (AllocVec; freed when the file is unlocked again, by DataTypeClose
 *     or finit), out of memory: 0.  The original copied it into a 384
 *     byte buffer without a limit, over the BitMapHeader pointer and the
 *     library's tables behind it (#604).
 *   - finit closes each of the four libraries that opened.  The original
 *     stopped at the first entry 0 of its table: without
 *     datatypes.library (the first) dos, intuition and graphics stayed
 *     open (#602).
 *   - AttachDataType window,id,&taglist passes the program's tag list
 *     (d2) to its second SetAttrsA.  The original passed the id (d1) as
 *     the address of a tag list and ignored the program's (#596).
 *   - AttachDataType window,id,x,y,width,height sign-extends its four
 *     word arguments into the tag list.  The original wrote the
 *     registers as longs, with whatever upper words they had (#597).
 *   - DataTypeModeID taglist builds its three tags in the library's
 *     buffer and chains the program's list behind them (TAG_MORE): the
 *     list is used and stays unchanged; a tag in both is the library's
 *     (BestModeID takes the first).  The original wrote its tags and a
 *     TAG_DONE over the first seven longs of the program's list (#603).
 *   - DataTypesPalette copies the picture's colours (PDTA_NumColors), at
 *     most as many as the palette has; none for a palette of 0 colours.
 *     The original copied as many as the palette has: behind the
 *     picture's colour table for a larger palette, 65536 for 0 (DBRA)
 *     (#606).  A palette smaller than the picture gets its first colours,
 *     as before (RIDataTypesLib.guide asks for 2^depth colours).
 *   - DataTypesPalette clears the buffer before GetAttr and returns 0
 *     without a colour table.  The original returned GetAttr's result
 *     then (1 for a class that knows the attribute), and for an object
 *     that does not know it took the buffer's first long, left by the
 *     command before, as the colour table (#607).
 * DataTypeHeight, DataTypeDepth, DataTypeModeID, DataTypesPalette,
 * SetDataTypeAttr, GetDataTypeAttr, DetachDataType and AttachDataType
 * (but its second form) do not check datatypes.library, like the
 * original: no defect, without it there is no object nor header, so
 * they do nothing either.
 * DIFF: a2 unchanged by SetDataTypeAttr with pairs (the original leaves
 * it below the pairs), as in all C ports (docs/libs/cmdlibs-port.md).
 */
#include "blitzlib.h"
#include "blitzobj.h"
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
void ridatatypeslib_libinit(void), ridatatypeslib_libfinit(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

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
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_attachdatatype), 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_attachdatatype2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
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
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
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

BLITZ_NULLSUB_AT(finit, ridatatypeslib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ridatatypeslib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

/* libbases: datatypes, dos, intuition, graphics, 0 (the original's end
   marker) */
static struct Library *libbases[5];
#define database  libbases[0]
#define dosbase   libbases[1]
#define intuibase libbases[2]
#define gfxbase   libbases[3]

static const char *const libnames[] = {
    "datatypes.library", "dos.library", "intuition.library", "graphics.library", NULL
};

/* storage: the 1024 byte buffer of every command - GetAttr's result in
   its first long, or a tag list */
static union {
    ULONG value;
    struct TagItem tags[128];
} *storage;
static BPTR lock;
static struct DataType *dtype;
static struct DataTypeHeader *dtypehead;
static Object *object;
static UBYTE *fname;                    /* the open file's name (AllocVec) */
static struct BitMapHeader *bitmapheader;
static ULONG attached;                  /* window object, then the Window */
static UWORD attachtobitmap;

/**
 * @brief init: opens datatypes, dos, intuition and graphics.library (V37)
 * and allocates the 1024 byte buffer (memlib globalloc, MEMF_CLEAR).
 *
 * A library that does not open leaves its base 0; the commands that test
 * the base then do nothing.
 */
static void BLITZ_ROUTINE c_libinit(void)
{
    struct bl_regs r = { .d0 = 1024, .d1 = MEMF_CLEAR };
    int i;

    for (i = 0; libnames[i]; i++)
        libbases[i] = OpenLibrary((CONST_STRPTR)libnames[i], 37);
    storage = (void *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

/**
 * @brief DetachDataType: removes the object from the window it is attached
 * to.
 *
 * Nothing if it is not attached, or attached to a bitmap.
 */
void BLITZ_ROUTINE c_detachdatatype(void)
{
    if ((LONG)attached <= 0)
        return;
    if (attachtobitmap)
        return;
    RemoveDTObject((struct Window *)attached, object);
    attached = 0;
}

/* the name of the file DataTypeOpen copied (fixed, #604) */
static void freename(void)
{
    FreeVec(fname);
    fname = NULL;
}

/**
 * @brief DataTypeClose: detaches the object, disposes of it, releases the
 * DataType and unlocks the file.
 *
 * Detached first (fixed, #599: the original disposed of an attached
 * object, the window kept the freed gadget).  The file is unlocked
 * whenever it is locked (fixed, #605: the original unlocked it, and
 * cleared the BitMapHeader, only if there was a DataType).  Nothing
 * without datatypes.library.
 */
void BLITZ_ROUTINE c_datatypeclose(void)
{
    if (!database)
        return;
    c_detachdatatype();                 /* fixed (#599) */
    if (object) {
        DisposeDTObject(object);
        object = NULL;
    }
    bitmapheader = NULL;
    if (dtype) {
        ReleaseDataType(dtype);
        dtype = NULL;
    }
    if (lock) {                         /* fixed (#605): also without a DataType */
        UnLock(lock);
        lock = 0;
        freename();
    }
    dtypehead = NULL;
}

/**
 * @brief finit: DetachDataType, DataTypeClose, closes the libraries and
 * frees the buffer.
 *
 * Each of the four that opened is closed (fixed, #602: the original
 * stopped at the first one that had not opened).
 */
static void BLITZ_ROUTINE c_libfinit(void)
{
    struct Library **p;
    struct bl_regs r;

    c_detachdatatype();
    c_datatypeclose();
    for (p = libbases; p < libbases + 4; p++) {         /* fixed (#602) */
        struct Library *l = *p;

        *p = NULL;
        if (l)
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

/**
 * @brief DataTypeOpen filename$: locks a file and obtains its DataType.
 *
 * The name is copied for DataTypeCreate into a block of its own length
 * (fixed, #604: the original copied it into a 384 byte buffer without a
 * limit); out of memory: 0.  Without a DataType the file is unlocked again (fixed, #605: the
 * original read the DataTypeHeader through address 28 and kept the
 * lock).
 * @param name  d0: the file name
 * @return d0: the DataType, 0 without datatypes.library, if a file is
 *         already open, out of memory, the file does not lock or has no
 *         DataType
 */
ULONG BLITZ_ROUTINE c_datatypeopen(CONST_STRPTR name BL_REG(d0))
{
    const UBYTE *s = (const UBYTE *)name;
    UBYTE *d;
    ULONG n = 1;

    if (!database || lock)
        return 0;
    while (s[n - 1])
        n++;
    fname = AllocVec(n, MEMF_ANY);      /* fixed (#604): its own length */
    if (!fname)
        return 0;
    d = fname;
    while ((*d++ = *s++) != 0)
        ;
    lock = Lock(name, SHARED_LOCK);
    if (!lock) {
        freename();
        return 0;
    }
    dtype = ObtainDataTypeA(DTST_FILE, (APTR)lock, NULL);
    if (!dtype) {                       /* fixed (#605) */
        UnLock(lock);
        lock = 0;
        freename();
        return 0;
    }
    dtypehead = dtype->dtn_Header;
    return (ULONG)dtype;
}

/**
 * @brief DataTypeName: the name of the file's DataType (dth_Name).
 *
 * The string is written at a3 (a3 behind its 0).
 * @return d0: its length, 0 without datatypes.library or DataType
 */
ULONG BLITZ_ROUTINE c_datatypename(void)
{
    if (!database || !dtypehead)
        return 0;
    return putstring((const UBYTE *)dtypehead->dth_Name);
}

/**
 * @brief DataTypeBaseName: the base name of the file's DataType
 * (dth_BaseName), written at a3 like DataTypeName.
 * @return d0: its length, 0 without datatypes.library or DataType
 */
ULONG BLITZ_ROUTINE c_datatypebasename(void)
{
    if (!database || !dtype)
        return 0;
    return putstring((const UBYTE *)dtypehead->dth_BaseName);
}

/**
 * @brief DataTypeID: the ID of the file's DataType (dth_ID), up to four
 * characters (up to a 0) at a3.
 * @return d0: the number of characters, 0 without datatypes.library or
 *         DataType
 */
ULONG BLITZ_ROUTINE c_datatypeid(void)
{
    if (!database || !dtype)
        return 0;
    return putid((const UBYTE *)&dtypehead->dth_ID);
}

/**
 * @brief DataTypeGroup: the group ID of the file's DataType
 * (dth_GroupID), up to four characters at a3.
 * @return d0: the number of characters, 0 without datatypes.library or
 *         DataType
 */
ULONG BLITZ_ROUTINE c_datatypegroup(void)
{
    if (!database || !dtypehead)
        return 0;
    return putid((const UBYTE *)&dtypehead->dth_GroupID);
}

/* DataTypeCreate [&taglist]: NewDTObject of the file's name */
static ULONG create(struct TagItem *tags)
{
    object = NewDTObjectA(fname, tags);
    return (ULONG)object;
}

/**
 * @brief DataTypeCreate: NewDTObject of the file's name without tags.
 *
 * Needs an open file, as the form with a tag list (fixed, #601: the
 * original made an object of the name the last DataTypeOpen had copied,
 * at first "").  While an object exists: 0, the object stays (fixed,
 * #600: the original lost it).
 * @return d0: the object, 0 without datatypes.library or open file, while
 *         an object exists, or on failure
 */
ULONG BLITZ_ROUTINE c_datatypecreate2(void)
{
    if (!database || !lock || object)   /* fixed (#601, #600) */
        return 0;
    storage->tags[0].ti_Tag = TAG_DONE;
    return create(storage->tags);
}

/**
 * @brief DataTypeCreate &taglist: NewDTObject of the file's name with a
 * tag list.
 *
 * While an object exists: 0, the object stays (fixed, #600: the
 * original lost it).
 * @param tags  d0: the tag list
 * @return d0: the object, 0 without datatypes.library or open file, while
 *         an object exists, or on failure
 */
ULONG BLITZ_ROUTINE c_datatypecreate(struct TagItem *tags BL_REG(d0))
{
    if (!database || !lock || object)   /* fixed (#600) */
        return 0;
    return create(tags);
}

ULONG BLITZ_ROUTINE c_datatypebitmap(void);

/* doattach: the window of the window object, AddDTObject */
static BB_BOOL doattach(const ULONG *winobj)
{
    attachtobitmap = 0;
    attached = *winobj;
    AddDTObject((struct Window *)attached, NULL, object, -1);
    return BB_TRUE;
}

/* the tags every form ends with */
static struct TagItem *tailtags(struct TagItem *t)
{
    t->ti_Tag = ICA_TARGET;
    t->ti_Data = (ULONG)-1;                     /* ICTARGET_IDCMP */
    t++;
    t->ti_Tag = PDTA_Remap;
    t->ti_Data = 0;
    t++;
    t->ti_Tag = TAG_DONE;
    return t;
}

/* one tag */
#define TAG(t, tag, data) ((t)->ti_Tag = (tag), (t)->ti_Data = (data), (t)++)

/**
 * @brief AttachDataType window,id: attaches the object to a window at 0,0
 * with the size of the BitMapHeader.
 *
 * SetAttrs (GA_Left/Top/Width/Height/ID, ICA_TARGET ICTARGET_IDCMP,
 * PDTA_Remap 0), then AddDTObject.  Without a BitMapHeader it is got
 * first, as DataTypeBitmap does (fixed, #595: the original read the size
 * through the unchecked pointer, without DataTypeBitmap at address 0).
 * Does not test datatypes.library (an open file needs it).
 * @param id      d1: the gadget id
 * @param winobj  a0: the window object (windowslib, parameter 0)
 * @return d0: BB_TRUE, BB_FALSE without an open file or object, if the
 *         object has no BitMapHeader or if attached already
 */
BB_BOOL BLITZ_ROUTINE c_attachdatatype(ULONG id BL_REG(d1), ULONG *winobj BL_REG(a0))
{
    struct TagItem *t = storage->tags;

    if (!lock || !object || attached)                   /* fixed (#595) */
        return BB_FALSE;
    if (!bitmapheader && !c_datatypebitmap())
        return BB_FALSE;
    attached = (ULONG)winobj;
    TAG(t, GA_Left, 0);
    TAG(t, GA_Top, 0);
    TAG(t, GA_Width, bitmapheader->bmh_Width);
    TAG(t, GA_Height, bitmapheader->bmh_Height);
    TAG(t, GA_ID, id);
    tailtags(t);
    SetAttrsA(object, storage->tags);
    return doattach((const ULONG *)attached);
}

/**
 * @brief AttachDataType window,id,x,y,width,height: attaches the object to
 * a window at a position and size.
 *
 * The word arguments go into the tag list sign-extended (fixed, #597:
 * the original wrote the registers, with the upper words they had).
 * @param id      d1: the gadget id
 * @param x       d2: left (word)
 * @param y       d3: top (word)
 * @param w       d4: width (word)
 * @param h       d5: height (word)
 * @param winobj  a0: the window object
 * @return d0: BB_TRUE, BB_FALSE without datatypes.library or an open file,
 *         or if attached already
 */
BB_BOOL BLITZ_ROUTINE c_attachdatatype2(ULONG id BL_REG(d1), WORD x BL_REG(d2),
                                        WORD y BL_REG(d3), WORD w BL_REG(d4),
                                        WORD h BL_REG(d5), ULONG *winobj BL_REG(a0))
{
    struct TagItem *t = storage->tags;

    if (!database || !lock || attached)
        return BB_FALSE;
    attached = (ULONG)winobj;
    TAG(t, GA_ID, id);
    TAG(t, GA_Left, (LONG)x);                           /* fixed (#597) */
    TAG(t, GA_Top, (LONG)y);
    TAG(t, GA_Width, (LONG)w);
    TAG(t, GA_Height, (LONG)h);
    tailtags(t);
    SetAttrsA(object, storage->tags);
    return doattach((const ULONG *)attached);
}

/**
 * @brief AttachDataType window,id,&taglist: attaches the object to a
 * window with a tag list of the program's.
 *
 * SetAttrs (GA_ID, ICA_TARGET ICTARGET_IDCMP, PDTA_Remap 0), SetAttrs
 * with the program's tag list (fixed, #596: the original passed the id as
 * the tag list and ignored the program's), then AddDTObject.
 * @param id      d1: the gadget id
 * @param tags    d2: the program's tag list
 * @param winobj  a0: the window object
 * @return d0: BB_TRUE, BB_FALSE without an open file or if attached
 *         already
 */
BB_BOOL BLITZ_ROUTINE c_attachdatatype3(ULONG id BL_REG(d1), struct TagItem *tags BL_REG(d2),
                                        ULONG *winobj BL_REG(a0))
{
    struct TagItem *t = storage->tags;

    if (!lock || attached)
        return BB_FALSE;
    attached = (ULONG)winobj;
    TAG(t, GA_ID, id);
    tailtags(t);
    SetAttrsA(object, storage->tags);
    SetAttrsA(object, tags);                            /* fixed (#596) */
    return doattach((const ULONG *)attached);
}

/**
 * @brief DataTypeReady: whether the attached object's layout is done
 * (DTA_LayoutProc 0).
 * @return d0: BB_TRUE when done, BB_FALSE while laying out, without
 *         datatypes.library or when not attached
 */
BB_BOOL BLITZ_ROUTINE c_datatypeready(void)
{
    if (!database || !attached)
        return BB_FALSE;
    storage->value = 0;
    GetAttr(DTA_LayoutProc, object, &storage->value);
    if (storage->value)
        return BB_FALSE;
    return BB_TRUE;
}

/**
 * @brief DataTypeSuper: the ID of the object's class (cl_ID), written at
 * a3 like DataTypeName.
 * @return d0: its length, 0 without datatypes.library or object
 */
ULONG BLITZ_ROUTINE c_datatypesuper(void)
{
    if (!database || !object)
        return 0;
    return putstring((const UBYTE *)OCLASS(object)->cl_ID);
}

/**
 * @brief DataTypeBitmap: gets the object's BitMapHeader
 * (PDTA_BitMapHeader) for DataTypeWidth/Height/Depth/ModeID and
 * AttachDataType.
 *
 * The buffer is cleared first (fixed, #598: the original took whatever
 * the command before had left in it when GetAttr failed).
 * @return d0: the BitMapHeader, 0 without datatypes.library or object, or
 *         if the object has none
 */
ULONG BLITZ_ROUTINE c_datatypebitmap(void)
{
    if (!database || !object)
        return 0;
    storage->value = 0;                 /* fixed (#598) */
    GetAttr(PDTA_BitMapHeader, object, &storage->value);
    bitmapheader = (struct BitMapHeader *)storage->value;
    return (ULONG)bitmapheader;
}

/* the colour block of a palette object (palettelib, struct bb_palette of
   blitzobj.h, its pdata): the number of colours, a word 0, then per
   colour a long per component (the value in the top byte, the bytes
   below as the program set them) */
struct pcolour {
    UBYTE red, red_low[3];
    UBYTE green, green_low[3];
    UBYTE blue, blue_low[3];
};
static_assert(sizeof(struct pcolour) == 12);
struct pdata {
    UWORD count;
    UWORD first;
    struct pcolour colour[];
};
#define PALDATA(o) ((struct pdata *)(o)->pdata)

/**
 * @brief DataTypesPalette palette#: the object's colours
 * (PDTA_ColorRegisters) into a palette object.
 *
 * Into the palette's colour block (count.w, first.w, then a long per
 * component; the top bytes set).  The smaller of the picture's
 * (PDTA_NumColors) and the palette's number of colours (fixed, #606: the
 * original copied the palette's number, behind the picture's table for a
 * larger palette, 65536 for 0).  Does not test datatypes.library.
 * @param palobj  a0: the palette object (palettelib, parameter 0)
 * The buffer is cleared before GetAttr (fixed, #607: the original took
 * what the command before had left in it for an object that does not
 * know the attribute).
 * @return d0: the address of the buffer; 0 without object or colour table
 *         (fixed, #607: the original returned GetAttr's result)
 */
ULONG BLITZ_ROUTINE c_datatypespalette(struct bb_palette *palobj BL_REG(a0))
{
    const UBYTE *cr;
    struct pcolour *p;
    ULONG n, nc = 0;

    if (!object)
        return 0;
    storage->value = 0;                 /* fixed (#607) */
    GetAttr(PDTA_ColorRegisters, object, &storage->value);
    cr = (const UBYTE *)storage->value;
    if (!cr)
        return 0;                       /* fixed (#607) */
    GetAttr(PDTA_NumColors, object, &nc);
    n = PALDATA(palobj)->count;
    if (nc < n)                         /* fixed (#606) */
        n = nc;
    p = PALDATA(palobj)->colour;
    for (; n; n--) {
        p->red = *cr++;
        p->green = *cr++;
        p->blue = *cr++;
        p++;
    }
    return (ULONG)storage;
}

/**
 * @brief DataTypeWidth: the picture's width (bmh_Width).
 * @return d0.w: the width, 0 without datatypes.library or DataTypeBitmap
 */
UWORD BLITZ_ROUTINE c_datatypewidth(void)
{
    if (!database || !bitmapheader)
        return 0;
    return bitmapheader->bmh_Width;
}

/**
 * @brief DataTypeHeight: the picture's height (bmh_Height).
 * @return d0.w: the height, 0 without DataTypeBitmap
 */
UWORD BLITZ_ROUTINE c_datatypeheight(void)
{
    if (!bitmapheader)
        return 0;
    return bitmapheader->bmh_Height;
}

/**
 * @brief DataTypeDepth: the picture's depth (bmh_Depth, sign-extended).
 * @return d0.w: the depth, 0 without DataTypeBitmap
 */
UWORD BLITZ_ROUTINE c_datatypedepth(void)
{
    if (!bitmapheader)
        return 0;
    return (UWORD)(WORD)(BYTE)bitmapheader->bmh_Depth;
}

/**
 * @brief SetDataTypeAttr &taglist: SetAttrs on the object.
 * @param tags  d0: the tag list
 */
void BLITZ_ROUTINE c_setdatatypeattr(struct TagItem *tags BL_REG(d0))
{
    if (!object)
        return;
    SetAttrsA(object, tags);
}

/**
 * @brief SetDataTypeAttr attr,data[,attr,data..]: SetAttrs with the pairs
 * as a tag list in the buffer.
 *
 * BUG(orig) (#608): 0 pairs are 65536 (DBRA), more than 127 overflow the
 * 1024 byte buffer.  DIFF: a2 stays unchanged.
 * @param n   d7: the number of pairs
 * @param sp  a2: the argument stack (the pairs below it)
 */
void BLITZ_ROUTINE c_setdatatypeattr2(UWORD n BL_REG(d7), const ULONG *sp BL_REG(a2))
{
    struct TagItem *t = storage->tags;

    if (!object)
        return;
    n--;
    do {                                /* BUG(orig) (#608): DBRA, no limit */
        t->ti_Tag = *--sp;
        (t++)->ti_Data = *--sp;
    } while (n-- != 0);
    t->ti_Tag = TAG_DONE;
    SetAttrsA(object, storage->tags);
}

/**
 * @brief GetDataTypeAttr(attribute): GetAttr of one attribute into the
 * buffer.
 * @param attr  d0: the attribute
 * @return d0: the address of the value (the buffer), 0 without object
 */
ULONG BLITZ_ROUTINE c_getdatatypeattr(ULONG attr BL_REG(d0))
{
    if (!object)
        return 0;
    GetAttr(attr, object, &storage->value);
    return (ULONG)storage;
}

/* DataTypeModeID [taglist]: BestModeID for the picture's size and depth,
   the tags in the buffer, the program's list (more) behind them (fixed,
   #603: the original wrote the tags over the program's list) */
static ULONG domodeid(struct TagItem *more)
{
    struct TagItem *t = storage->tags;

    if (!bitmapheader)
        return 0;
    t[0].ti_Tag = BIDTAG_NominalWidth;
    t[0].ti_Data = bitmapheader->bmh_Width;
    t[1].ti_Tag = BIDTAG_NominalHeight;
    t[1].ti_Data = bitmapheader->bmh_Height;
    t[2].ti_Tag = BIDTAG_Depth;
    t[2].ti_Data = bitmapheader->bmh_Depth;
    if (more) {
        t[3].ti_Tag = TAG_MORE;
        t[3].ti_Data = (ULONG)more;
    } else {
        t[3].ti_Tag = TAG_DONE;
    }
    return BestModeIDA(t);
}

/**
 * @brief DataTypeModeID: BestModeID for the picture's size and depth
 * (the tags in the buffer).
 * @return d0: the mode id, 0 without DataTypeBitmap
 */
ULONG BLITZ_ROUTINE c_getmodeid(void)
{
    return domodeid(NULL);
}

/**
 * @brief DataTypeModeID taglist: as DataTypeModeID, with the program's
 * tag list behind the library's tags (TAG_MORE; the library's come
 * first).  Fixed (#603): the original wrote its tags over the list.
 * @param tags  d0: the tag list (0: none)
 * @return d0: the mode id, 0 without DataTypeBitmap
 */
ULONG BLITZ_ROUTINE c_getmodeid2(struct TagItem *tags BL_REG(d0))
{
    return domodeid(tags);
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(ridatatypeslib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(ridatatypeslib_libfinit, c_libfinit, "d0-d1/a0-a1");
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
