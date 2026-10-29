/*
 * ritooltypeslib - RWE userlib 59 (#ritooltypes): the icon of a file and
 * its tool types through icon.library - GetIconObject, FindToolValue,
 * SetToolValue, PutIconObject, FreeIconObject, MatchToolValue,
 * NewToolType, FindToolNumber, ClearToolTypes, SetIconHit, ShapeToIcon,
 * SetIconType, IconRender, IconDefTool, FindToolType, IconToShape.
 *
 * C translation of rwelibsrc/tooltypeslib/ToolTypes4.bb (Reflective
 * Images, Stephen McNamara, 5/4/95); the declarations equal the CD's BSS2
 * RITOOLTYPESLIB.OBJ (ctest rwe_declarations_ritooltypeslib), the
 * behaviour is compared with it in libs/tests/rwe/ritooltypeslib.c.
 * Notes: docs/rwelibs/port/ritooltypeslib.md.  All commands are Amiga
 * mode only (odd code address in !subs).
 *   init:  OpenLibrary("icon.library",0) (iconlibbase, 0 if missing:
 *          then the commands do nothing and return 0)
 *   finit: FreeIconObject if an icon is open, CloseLibrary
 *
 * One icon at a time: GetIconObject keeps the DiskObject (diskobject)
 * and points its do_ToolTypes to the library's own array of 100 entries
 * (tooltype), copied from the icon's.  SetToolValue/NewToolType/
 * IconDefTool allocate the new strings (AllocMem) and hang them on the
 * icon's FreeList (AddFreeList at DiskObject+78, freed with the icon).
 * ShapeToIcon/IconToShape exchange images with shapeslib's shape objects
 * (32 bytes, see libs/acidlibs/shapeslib.c); IconToShape allocates the
 * shape's data with memlib globalloc and makes the cookie with
 * shapeslib's MakeCookie.
 *
 * Everything is C; icon.library and exec through the NDK inlines (the
 * base: iconlibbase), memlib and shapeslib through BLITZ_ALIBJSR.
 * String arguments: d0 = the string, its length on the argument stack
 * below a2 (read through a copy, a2 stays unchanged: DIFF as in all C
 * ports, docs/libs/cmdlibs-port.md).
 *
 * BUG(orig):
 *   - GetIconObject copies the icon's tool types without a limit into the
 *     100 entries (and reads from address 0 if the icon has none);
 *     NewToolType appends without a limit either; nothing checks for the
 *     last entry (a full array runs into memory behind it).
 *   - SetToolValue matches a name that is a prefix of the entry ("FL"
 *     replaces "FLAG"); the entry's own "(" is skipped, so the name
 *     "OFF" replaces "(OFF)=1".  The new string is built with
 *     do-while loops: an empty name copies its 0 and then whatever
 *     follows the name up to the next 0.
 *   - FindToolType tt$,1 sets the bracket check flag, which only a
 *     bracket search clears: after a hit without brackets, a name with
 *     "(", a FindToolType before GetIconObject, the next FindToolType
 *     tt$ also searches "(tt$)".  The bracket hit returns 1, not -1.
 *     A name over 253 characters overflows the 256 byte buffer.
 *   - FindToolValue stops at ")" (the end of "(name=value)") but also at
 *     a ")" inside the value; FindToolValue/FindToolNumber write no
 *     terminating 0 (invisible, see docs/libs/cmdlibs-port.md).
 *   - IconDefTool rewinds a3 by the length of its argument + 1 (like
 *     FindToolValue does for its result) and leaves it there.
 *   - SetIconType without an icon returns its argument.
 *   - PutIconObject writes the icon (PutDiskObject's result ignored),
 *     then forgets it without FreeDiskObject (lost).
 *   - ShapeToIcon shape,shape uses the library's own Image for the
 *     selected image with PlanePick/PlaneOnOff 0 and sets the gadget
 *     flags to 6 (GADGHIMAGE), dropping all others.
 *   - IconToShape: the shape is filled before the data is allocated (no
 *     memory: the shape keeps its size and has no data); the copy takes
 *     (width >> 4) + 1 words per row, one word more than the shape's
 *     data has when the width is a multiple of 16 (writes behind it);
 *     MakeCookie is called with a3 = the shape only if the shape was in
 *     use before (freeashape), otherwise with the caller's a3 - a cookie
 *     for whatever a3 points to; IconToShape shape,shape gives the second
 *     shape the cookie of the first (a3 is still the first shape).
 * DIFF: ShapeToIcon shape,shape and IconToShape leave a3 unchanged (the
 * original uses it as a work register and leaves it on its own Image or
 * the shape); a2 unchanged.
 * Registers: the thunks keep every one of d0/d1/a0/a1 that is not the
 * result, GCC keeps d2-d7/a2-a6 (the original changes d7 and a6 nearly
 * everywhere, a2 in several routines).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#define __NOLIBBASE__
#define ICON_BASE_NAME iconlibbase
#include <proto/icon.h>
#undef __NOLIBBASE__
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <stddef.h>

/* entries (thunks below) */
void libinit(void), libfinit(void);
void getstructure(void), gettooltype(void), PutTool2(void), PutTool(void);
void Put_icon2(void), Put_icon(void), Close_icon(void), Matchtoolvalue(void);
void NewTool2(void), NewTool(void), gettoolnumber(void), cleartooltypes(void);
void seticonhit(void), shapetoicon(void), shapetoicon2(void), icontype(void);
void selectrender(void), defaulttool(void), istooltype(void), istooltype2(void);
void icontoshape(void), icontoshape2(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ritooltypes,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ritooltypes, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(getstructure), 0, 0);
BLITZ_NAME("GetIconObject", "(filename$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(gettooltype), 0, 0);
BLITZ_NAME("FindToolValue", "(tooltype$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(PutTool2), 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(PutTool), 0, 0);
BLITZ_NAME("SetToolValue", "(tooltype$,value$[,status])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(Put_icon2), 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(Put_icon), 0, 0);
BLITZ_NAME("PutIconObject", "(filename$[,icontype])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(Close_icon), 0, 0);
BLITZ_NAME("FreeIconObject", " - free the disk object of a icon");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(Matchtoolvalue), 0, 0);
BLITZ_NAME("MatchToolValue", "(tooltype$,value$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(NewTool2), 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(NewTool), 0, 0);
BLITZ_NAME("NewToolType", "(tooltype$,value$[,status])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(gettoolnumber), 0, 0);
BLITZ_NAME("FindToolNumber", "(tooltype#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cleartooltypes), 0, 0);
BLITZ_NAME("ClearToolTypes", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(seticonhit), 0, 0);
BLITZ_NAME("SetIconHit", "width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_LA(0) | BL_ITEM | BL_PD(0));          /* #ia0|#pd0 */
BLITZ_SUBS(BLITZ_AMIGAONLY(shapetoicon), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_LA(0) | BL_ITEM | BL_PD(0),
           LIB_shapeslib, BL_LA(1) | BL_ITEM | BL_PD(1));          /* #ia1|#pd1 */
BLITZ_SUBS(BLITZ_AMIGAONLY(shapetoicon2), 0, 0);
BLITZ_NAME("ShapeToIcon", "shape[,shape]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(icontype), 0, 0);
BLITZ_NAME("SetIconType", "object_type");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(selectrender), 0, 0);
BLITZ_NAME("IconRender", "render_flags");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(defaulttool), 0, 0);
BLITZ_NAME("IconDefTool", "tool$");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(istooltype), 0, 0);
BLITZ_ARGS(BL_STRING, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(istooltype2), 0, 0);
BLITZ_NAME("FindToolType", "(tooltype$[,bracket_check])");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_LA(0) | BL_ITEM | BL_PD(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(icontoshape), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_LA(0) | BL_ITEM | BL_PD(0),
           LIB_shapeslib, BL_LA(1) | BL_ITEM | BL_PD(1));
BLITZ_SUBS(BLITZ_AMIGAONLY(icontoshape2), 0, 0);
BLITZ_NAME("IconToShape", "shape[,shape]");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

/* shapeslib MakeCookie (a3 = the shape) */
#define TOK_makecookie ((LIB_shapeslib << 7) + 2)

#define DO_FREELIST 78                  /* the FreeList behind the DiskObject */
#define NTOOLTYPES 100

static struct Library *iconlibbase;
static struct DiskObject *diskobject;
static STRPTR *tooltype;                /* = tooltypearray when an icon is open */
static ULONG toolbuffersize;
static LONG status_tool;
static STRPTR *array_address;
static const char *tooltype_address;
static const char *string_address;
static UWORD bracket_check;
static UWORD shapenumber;
/* SelectRender: the Image of ShapeToIcon shape,shape */
static struct Image SelectRender;
static STRPTR tooltypearray[NTOOLTYPES];

/* shape objects (shapeslib) */
#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define CLUDGE 0x634c                   /* "cL" */

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* the address FreeMem gets after a failed AddFreeList: 0 (through an asm,
   GCC would warn about FreeMem(NULL)) */
static APTR nofree(void)
{
    APTR p;

    __asm__("" : "=a"(p) : "0"(0));
    return p;
}

void BLITZ_ROUTINE c_libinit(void)
{
    iconlibbase = OpenLibrary((CONST_STRPTR)"icon.library", 0);
    tooltype = NULL;
}

/* FreeIconObject: -1 if an icon was open */
ULONG BLITZ_ROUTINE c_Close_icon(void)
{
    if (!iconlibbase || !diskobject)
        return 0;
    FreeDiskObject(diskobject);
    diskobject = NULL;
    tooltype = NULL;
    return (ULONG)-1;
}

void BLITZ_ROUTINE c_libfinit(void)
{
    if (!iconlibbase)
        return;
    if (diskobject)
        c_Close_icon();
    CloseLibrary(iconlibbase);
}

/* GetIconObject filename$: the DiskObject, 0 if an icon is open already
   or there is none */
ULONG BLITZ_ROUTINE c_getstructure(CONST_STRPTR name BL_REG(d0))
{
    STRPTR *from;
    STRPTR *to = tooltypearray;
    int i;

    if (tooltype || !iconlibbase)
        return 0;
    diskobject = GetDiskObjectNew(name);
    if (!diskobject)
        return 0;
    from = diskobject->do_ToolTypes;
    tooltype = tooltypearray;
    diskobject->do_ToolTypes = tooltypearray;
    for (i = 0; i < NTOOLTYPES; i++)
        tooltypearray[i] = NULL;
    while (*from)                       /* BUG(orig): no limit, no NULL check */
        *to++ = *from++;
    return (ULONG)diskobject;
}

/* ClearToolTypes */
void BLITZ_ROUTINE c_cleartooltypes(void)
{
    STRPTR *p = tooltype;

    if (!p)
        return;
    while (*p)
        *p++ = NULL;
}

/* FindToolNumber(n): the n-th tool type to a3 (no 0) */
ULONG BLITZ_ROUTINE c_gettoolnumber(ULONG n BL_REG(d0))
{
    STRPTR *p = tooltype, s;
    ULONG i = 0, len = 0;

    if (!p)
        return 0;
    for (;; i++, p++) {
        if (!*p)
            return 0;
        if (i == n)
            break;
    }
    for (s = *p; *s; len++)
        *blitz_string++ = *s++;
    return len;
}

/* FindToolType(tt$[,bracket_check]): -1 = found, 1 = found as "(tt$)" */
ULONG BLITZ_ROUTINE c_istooltype(CONST_STRPTR name BL_REG(d0))
{
    UBYTE *temp;
    const char *s;
    char *d;
    ULONG r;

    if (!tooltype)
        return 0;
    string_address = (const char *)name;
    if (FindToolType((CONST_STRPTR *)tooltype, name))
        return (ULONG)-1;
    if (!bracket_check)
        return 0;
    if (*string_address == '(')
        return 0;
    temp = AllocMem(256, MEMF_PUBLIC);
    if (!temp)
        return 0;
    s = string_address;
    d = (char *)temp;
    *d++ = '(';
    while ((*d++ = *s++) != 0)
        ;
    d[-1] = ')';
    *d = 0;
    r = FindToolType((CONST_STRPTR *)tooltype, (CONST_STRPTR)temp) ? 1 : 0;
    FreeMem(temp, 256);
    bracket_check = 0;
    return r;
}

ULONG BLITZ_ROUTINE c_istooltype2(CONST_STRPTR name BL_REG(d0))
{
    bracket_check = 0xffff;
    return c_istooltype(name);
}

/* FindToolValue(tt$): the value after "=" up to ")" or the end, over
   the argument at a3 (a3 - length - 1) */
ULONG BLITZ_ROUTINE c_gettooltype(CONST_STRPTR name BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    STRPTR v;
    UWORD len = 0;

    if (!tooltype)
        return 0;
    v = FindToolType((CONST_STRPTR *)tooltype, name);
    if (!v)
        return 0;
    blitz_string -= lens[-1] + 1;       /* "Blitz is wierd..." */
    while (*v != ')' && *v)
        *blitz_string++ = *v++, len++;
    return len;
}

/* MatchToolValue(tt$,value$): -1 if the tool type has the value */
ULONG BLITZ_ROUTINE c_Matchtoolvalue(CONST_STRPTR name BL_REG(d0),
                                     CONST_STRPTR value BL_REG(d1))
{
    STRPTR v;

    string_address = (const char *)value;
    if (!tooltype)
        return 0;
    v = FindToolType((CONST_STRPTR *)tooltype, name);
    if (!v)
        return 0;
    return MatchToolValue(v, (CONST_STRPTR)string_address) ? (ULONG)-1 : 0;
}

/* PutIconObject filename$[,icontype] */
ULONG BLITZ_ROUTINE c_Put_icon(CONST_STRPTR name BL_REG(d0), ULONG type BL_REG(d1))
{
    if (!iconlibbase || !diskobject)
        return 0;
    if ((UBYTE)type)
        diskobject->do_Type = (UBYTE)type;
    PutDiskObject(name, diskobject);    /* BUG(orig): result ignored */
    diskobject = NULL;                  /* BUG(orig): not freed */
    tooltype = NULL;
    return (ULONG)-1;
}

ULONG BLITZ_ROUTINE c_Put_icon2(CONST_STRPTR name BL_REG(d0))
{
    return c_Put_icon(name, 0);
}

/* _stringcompare: -1 if name (a0) is a prefix of the entry (a1, its own
   "(" skipped), case insensitive for a-z */
static int stringcompare(const UBYTE *name, const UBYTE *entry)
{
    UBYTE c, e;

    if (*entry == '(')
        entry++;
    while ((c = *name++) != 0) {
        e = *entry++;
        if (c >= 'a' && c <= 'z')
            c -= 0x20;
        if (e >= 'a' && e <= 'z')
            e -= 0x20;
        if (c != e)
            return 0;
    }
    return -1;
}

/* gotit: the new string "name=value" ("(name=value)" for status 0) into
   *array_address, on the icon's FreeList */
static ULONG gotit(STRPTR *entry)
{
    UBYTE *mem, *d;
    const char *s;

    array_address = entry;
    mem = AllocMem(toolbuffersize, MEMF_CLEAR | MEMF_PUBLIC);
    if (!mem)
        return 0;
    if (!AddFreeList((struct FreeList *)((UBYTE *)diskobject + DO_FREELIST), mem,
                     toolbuffersize)) {
        /* BUG(orig): MOVE.l d0,a1 with d0 = AddFreeList's 0 - frees
           address 0, the block is lost */
        FreeMem(nofree(), toolbuffersize);
        return 0;
    }
    *array_address = (STRPTR)mem;
    d = mem;
    s = tooltype_address;
    if (!status_tool)
        *d++ = '(';
    do                                  /* BUG(orig): do-while (see above) */
        *d++ = *s++;
    while (*s);
    s = string_address;
    if (*s) {
        *d++ = '=';
        do
            *d++ = *s++;
        while (*s);
    }
    if (!status_tool)
        *d = ')';
    return (ULONG)-1;
}

/* the arguments of SetToolValue/NewToolType */
static void toolargs(const char *name, const char *value, LONG status, const ULONG *lens)
{
    tooltype_address = name;
    string_address = value;
    status_tool = status;
    toolbuffersize = lens[-1] + lens[-2] + 8;
}

/* SetToolValue(tt$,value$[,status]): replace the entry that starts with
   tt$; 0 if there is none */
ULONG BLITZ_ROUTINE c_PutTool(const char *name BL_REG(d0), const char *value BL_REG(d1),
                              LONG status BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    STRPTR *p;

    toolargs(name, value, status, lens);
    if (!(p = tooltype))
        return 0;
    for (; *p; p++)
        if (stringcompare((const UBYTE *)tooltype_address, (const UBYTE *)*p))
            return gotit(p);
    return 0;
}

ULONG BLITZ_ROUTINE c_PutTool2(const char *name BL_REG(d0), const char *value BL_REG(d1),
                               const ULONG *lens BL_REG(a2))
{
    return c_PutTool(name, value, -1, lens);
}

/* NewToolType(tt$,value$[,status]): a new entry at the end */
ULONG BLITZ_ROUTINE c_NewTool(const char *name BL_REG(d0), const char *value BL_REG(d1),
                              LONG status BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    STRPTR *p;

    toolargs(name, value, status, lens);
    if (!(p = tooltype))
        return 0;
    while (*p)                          /* BUG(orig): no limit */
        p++;
    p[1] = NULL;
    return gotit(p);
}

ULONG BLITZ_ROUTINE c_NewTool2(const char *name BL_REG(d0), const char *value BL_REG(d1),
                               const ULONG *lens BL_REG(a2))
{
    return c_NewTool(name, value, -1, lens);
}

/* SetIconHit width,height: the gadget's size */
void BLITZ_ROUTINE c_seticonhit(UWORD w BL_REG(d0), UWORD h BL_REG(d1))
{
    if (!diskobject)
        return;
    diskobject->do_Gadget.Width = w;
    diskobject->do_Gadget.Height = h;
}

/* ShapeToIcon shape: the gadget's size and image from the shape */
void BLITZ_ROUTINE c_shapetoicon(const UBYTE *shape BL_REG(a0))
{
    struct Image *im;

    if (!diskobject)
        return;
    diskobject->do_Gadget.Width = SW(shape, 0);
    diskobject->do_Gadget.Height = SW(shape, 2);
    diskobject->do_Gadget.Height += 1;
    im = (struct Image *)diskobject->do_Gadget.GadgetRender;
    im->Width = SW(shape, 0);
    im->Height = SW(shape, 2);
    im->Depth = SW(shape, 4);
    im->ImageData = (UWORD *)SL(shape, 14);
}

/* ShapeToIcon shape,shape: the second shape as selected image */
void BLITZ_ROUTINE c_shapetoicon2(const UBYTE *shape BL_REG(a0), const UBYTE *shape2 BL_REG(a1))
{
    if (!diskobject)
        return;
    SelectRender.Width = SW(shape2, 0);
    SelectRender.Height = SW(shape2, 2);
    SelectRender.Depth = SW(shape2, 4);
    SelectRender.ImageData = (UWORD *)SL(shape2, 14);
    diskobject->do_Gadget.SelectRender = &SelectRender;
    diskobject->do_Gadget.Flags = 6;    /* GFLG_GADGHIMAGE | GFLG_GADGIMAGE */
    c_shapetoicon(shape);
}

/* SetIconType object_type (1-5): -1; BUG(orig) without icon d0 = type */
ULONG BLITZ_ROUTINE c_icontype(ULONG type BL_REG(d0))
{
    if (!diskobject)
        return type;
    if (!type || type > 5)
        return 0;
    diskobject->do_Type = (UBYTE)type;
    return (ULONG)-1;
}

/* IconRender render_flags: the gadget flags */
void BLITZ_ROUTINE c_selectrender(UWORD flags BL_REG(d0))
{
    if (diskobject)
        diskobject->do_Gadget.Flags = flags;
}

/* IconDefTool tool$: the default tool (a copy on the FreeList) */
ULONG BLITZ_ROUTINE c_defaulttool(const char *tool BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    UBYTE *mem, *d;
    const char *s;
    ULONG len;

    if (!diskobject)
        return 0;
    string_address = tool;
    len = lens[-1];
    if (!len)
        return 0;
    toolbuffersize = len + 1;
    blitz_string -= toolbuffersize;     /* BUG(orig): a3 rewound */
    mem = AllocMem(toolbuffersize, MEMF_CLEAR | MEMF_PUBLIC);
    if (!mem)
        return 0;
    if (!AddFreeList((struct FreeList *)((UBYTE *)diskobject + DO_FREELIST), mem,
                     toolbuffersize)) {
        /* BUG(orig): MOVE.l d0,a1 with d0 = AddFreeList's 0 - frees
           address 0, the block is lost */
        FreeMem(nofree(), toolbuffersize);
        return 0;
    }
    d = mem;
    s = string_address;
    do
        *d++ = *s++;
    while (*s);
    diskobject->do_DefaultTool = (STRPTR)mem;
    return (ULONG)-1;
}

/* freeashape (a3 = shape): free data and cookie of a shape in use */
static void freeashape(UBYTE *s)
{
    ULONG c;

    if (!SW(s, 0))
        return;
    SW(s, 0) = 0;
    if (SW(s, 30) != CLUDGE) {
        SW(s, 30) = 0;
        globfree(SL(s, 14), SW(s, 26));
    }
    c = SL(s, 18);
    if (c) {
        SL(s, 18) = 0;
        globfree(c, SW(s, 22));
    }
}

/* icontoshape_jump: shape (a0) from the image at do_Gadget+render
   (GadgetRender 22, SelectRender 26); `a3` = the a3 the original has at
   this point (the shape after freeashape, otherwise unchanged) */
static void icontoshape_jump(ULONG d0, UBYTE *shape, UWORD render, UBYTE **a3)
{
    UBYTE *im;
    UWORD w, h, words, bytes, d;
    const UWORD *from;
    UWORD *to;
    ULONG mem;

    if (SW(shape, 0)) {
        *a3 = shape;
        freeashape(shape);
    }
    shapenumber = (UWORD)((WORD)d0 >> 5);
    im = *(UBYTE **)((UBYTE *)diskobject + render);
    if (!im)
        return;
    im += 4;                            /* Image.Width */
    SL(shape, 0) = SL(im, 0);           /* width and height */
    SW(shape, 4) = SW(im, 4);           /* depth */
    w = SW(im, 0);
    h = SW(im, 2);
    words = (UWORD)((UWORD)(w + 15) >> 4);
    bytes = (UWORD)(words + words);
    SW(shape, 6) = bytes;               /* ebwidth */
    SW(shape, 8) = (UWORD)(((UWORD)(h << 6) | words) + 1);  /* blitsize */
    SL(shape, 10) = 0;                  /* handles */
    d = (UWORD)bl_mulu_w(bytes, h);
    SW(shape, 22) = d;                  /* onebpmem */
    SW(shape, 24) = (UWORD)(d + (UWORD)(h << 1));               /* onebpmemx */
    SW(shape, 26) = (UWORD)bl_mulu_w(d, SW(im, 4));             /* allbpmem */
    SW(shape, 28) = (UWORD)bl_mulu_w(SW(shape, 24), SW(im, 4)); /* allbpmemx */
    mem = globalloc(SW(shape, 26), 0x10001 | MEMF_CHIP);
    if (!mem)
        return;
    SL(shape, 14) = mem;
    from = *(const UWORD **)(im + 6);   /* ImageData */
    to = (UWORD *)mem;
    /* BUG(orig): (width >> 4) + 1 words per row (MULU.w: low words) */
    d = (UWORD)bl_mulu_w((UWORD)bl_mulu_w((UWORD)(((WORD)w >> 4) + 1), h), SW(im, 4));
    d--;
    do
        *to++ = *from++;
    while (d-- != 0);                   /* DBRA */
    SW(shape, 30) = 0;
    {
        /* ALibJsr MakeCookie with d0 = shapenumber and the original's a3 */
        UBYTE *keep = blitz_string;
        struct bl_regs r = { .d0 = shapenumber };

        blitz_string = *a3;
        BLITZ_ALIBJSR(TOK_makecookie, r);
        blitz_string = keep;
    }
}

/* IconToShape shape: the icon's image as shape (DIFF: a3 kept) */
void BLITZ_ROUTINE c_icontoshape(ULONG d0 BL_REG(d0), UBYTE *shape BL_REG(a0))
{
    UBYTE *a3 = blitz_string;

    if (!diskobject)
        return;
    icontoshape_jump(d0, shape, 22, &a3);
}

/* IconToShape shape,shape: the image and the selected image */
void BLITZ_ROUTINE c_icontoshape2(ULONG d0 BL_REG(d0), ULONG d1 BL_REG(d1),
                                  UBYTE *shape BL_REG(a0), UBYTE *shape2 BL_REG(a1))
{
    UBYTE *a3 = blitz_string;

    if (!diskobject)
        return;
    icontoshape_jump(d0, shape, 22, &a3);
    icontoshape_jump(d1, shape2, 26, &a3);
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(getstructure, c_getstructure, "d1/a0-a1");
BLITZ_THUNK(gettooltype, c_gettooltype, "d1/a0-a1");
BLITZ_THUNK(PutTool2, c_PutTool2, "d1/a0-a1");
BLITZ_THUNK(PutTool, c_PutTool, "d1/a0-a1");
BLITZ_THUNK(Put_icon2, c_Put_icon2, "d1/a0-a1");
BLITZ_THUNK(Put_icon, c_Put_icon, "d1/a0-a1");
BLITZ_THUNK(Close_icon, c_Close_icon, "d1/a0-a1");
BLITZ_THUNK(Matchtoolvalue, c_Matchtoolvalue, "d1/a0-a1");
BLITZ_THUNK(NewTool2, c_NewTool2, "d1/a0-a1");
BLITZ_THUNK(NewTool, c_NewTool, "d1/a0-a1");
BLITZ_THUNK(gettoolnumber, c_gettoolnumber, "d1/a0-a1");
BLITZ_THUNK(cleartooltypes, c_cleartooltypes, "d0-d1/a0-a1");
BLITZ_THUNK(seticonhit, c_seticonhit, "d0-d1/a0-a1");
BLITZ_THUNK(shapetoicon, c_shapetoicon, "d0-d1/a0-a1");
BLITZ_THUNK(shapetoicon2, c_shapetoicon2, "d0-d1/a0-a1");
BLITZ_THUNK(icontype, c_icontype, "d1/a0-a1");
BLITZ_THUNK(selectrender, c_selectrender, "d0-d1/a0-a1");
BLITZ_THUNK(defaulttool, c_defaulttool, "d1/a0-a1");
BLITZ_THUNK(istooltype, c_istooltype, "d1/a0-a1");
BLITZ_THUNK(istooltype2, c_istooltype2, "d1/a0-a1");
BLITZ_THUNK(icontoshape, c_icontoshape, "d0-d1/a0-a1");
BLITZ_THUNK(icontoshape2, c_icontoshape2, "d0-d1/a0-a1");
