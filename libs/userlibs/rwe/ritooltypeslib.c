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
 * and points its do_ToolTypes to the library's own array (tooltype,
 * AllocMem, ttcap entries with the terminator), copied from the icon's;
 * NewToolType makes it larger when it is full.  SetToolValue/NewToolType/
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
 *   - GetIconObject reads the icon's tool types from address 0 if the
 *     icon has none (#845).
 *   - IconDefTool rewinds a3 by the length of its argument + 1 (like
 *     FindToolValue does for its result) and leaves it there. No defect:
 *     it gives back the space of the string argument, which the compiler
 *     does not take back after a library function with a numeric result
 *     (#846, closed).
 *   - PutIconObject forgets the written icon without FreeDiskObject
 *     (lost) (#850).
 *   - ShapeToIcon shape,shape uses the library's own Image for the
 *     selected image with PlanePick/PlaneOnOff 0 and sets the gadget
 *     flags to 6 (GADGHIMAGE), dropping all others (#854).
 *   - IconToShape: the shape is filled before the data is allocated (no
 *     memory: the shape keeps its size and has no data) (#848; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!"); the copy
 *     takes (width >> 4) + 1 words per row, one word more than the
 *     shape's data has when the width is a multiple of 16 (writes behind
 *     it) (#847); MakeCookie is called with a3 = the shape only if the
 *     shape was in use before (freeashape), otherwise with the caller's
 *     a3 - a cookie for whatever a3 points to; IconToShape shape,shape
 *     gives the second shape the cookie of the first (a3 is still the
 *     first shape) (#849).
 * Fixed (intended deviation, #852): SetIconType without an icon returned
 *   its argument (non-zero: success for a program that tests it); now 0.
 * Fixed (intended deviation, #843): FindToolType tt$,1 set a flag for the
 *   bracket search that only the end of that search cleared; after a hit
 *   without brackets, a name with "(", a call without an icon or no
 *   memory, the next FindToolType tt$ also searched "(tt$)".  The bracket
 *   check is now a parameter of the call.
 * Fixed (intended deviation, #844): FindToolValue stopped at the first
 *   ")" (meant for the end of "(name=value)"), also inside a value
 *   ("a)b" gave "a").  Now only a ")" at the end of the value is dropped,
 *   and only for a name given with "(" (the only name icon.library finds
 *   "(name=value)" with).
 * Fixed (intended deviation, #853): SetToolValue replaced the first entry
 *   that only began with the name ("FL" replaced "FLAG"); after the name
 *   the entry must now end or go on with "=" or ")".
 * Fixed (intended deviation, #841): SetToolValue/NewToolType copied the
 *   name with a do-while loop: an empty name copied its 0 and whatever
 *   followed it, the entry began with 0 (read back as "").  An empty name
 *   now gives the entry "=value".
 * Fixed (intended deviation, #840): when AddFreeList failed,
 *   SetToolValue/NewToolType/IconDefTool freed address 0 (d0, its result)
 *   instead of the new block, which was lost; they free the block now and
 *   return 0 as before.
 * Fixed (intended deviation, #842): the bracket search of FindToolType
 *   tt$,1 built "(tt$)" in 256 bytes; a name over 253 characters wrote
 *   behind them.  The buffer now has the name's length + 3 bytes.
 * Fixed (intended deviation, #855): the tool types lived in a static
 *   array of 100 entries; GetIconObject copied an icon's tool types into
 *   it and NewToolType appended to it without a count, so the 101st entry
 *   and the terminator went into the memory behind it.  The array is now
 *   allocated for the icon's count and the terminator and NewToolType
 *   makes it larger when it is full (the user's decision of 2026-11-17,
 *   w1-decisions.md); without memory GetIconObject/NewToolType return 0
 *   and change nothing.  It is freed with the icon.
 * Fixed (intended deviation, #851): PutIconObject ignored PutDiskObject's
 *   result, returned -1 and closed the icon also when writing failed; it
 *   returns 0 then and the icon stays open (the user's decision of
 *   2026-11-17, w1-decisions.md).
 * Original behaviour that is no defect: SetToolValue skips the entry's
 * own "(", so the name "OFF" replaces "(OFF)=1" (the switched-off form of
 * the same entry).  FindToolType's bracket hit (a switched-off entry)
 * returns 1, not -1 (it tells the two apart).  FindToolValue and
 * FindToolNumber write no terminating 0 (string results need none, see
 * #257 and docs/libs/cmdlibs-port.md).
 * DIFF: ShapeToIcon shape,shape and IconToShape leave a3 unchanged (the
 * original uses it as a work register and leaves it on its own Image or
 * the shape); a2 unchanged.
 * Registers: the thunks keep every one of d0/d1/a0/a1 that is not the
 * result, GCC keeps d2-d7/a2-a6 (the original changes d7 and a6 nearly
 * everywhere, a2 in several routines).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include "blitzexec.h"
#define __NOLIBBASE__
#define ICON_BASE_NAME iconlibbase
#include <proto/icon.h>
#undef __NOLIBBASE__
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <stddef.h>

/* entries (thunks below) */
void ritooltypeslib_libinit(void), ritooltypeslib_libfinit(void);
void getstructure(void), gettooltype(void), PutTool2(void), PutTool(void);
void Put_icon2(void), Put_icon(void), Close_icon(void), Matchtoolvalue(void);
void NewTool2(void), NewTool(void), gettoolnumber(void), cleartooltypes(void);
void seticonhit(void), shapetoicon(void), shapetoicon2(void), icontype(void);
void selectrender(void), defaulttool(void), istooltype(void);
void istooltype2(void), icontoshape(void), icontoshape2(void);

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
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0);                          /* #ia0|#pd0 */
BLITZ_SUBS(BLITZ_AMIGAONLY(shapetoicon), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0,
           LIB_shapeslib, BL_IA1|BL_PD1);                          /* #ia1|#pd1 */
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
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0);
BLITZ_SUBS(BLITZ_AMIGAONLY(icontoshape), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0,
           LIB_shapeslib, BL_IA1|BL_PD1);
BLITZ_SUBS(BLITZ_AMIGAONLY(icontoshape2), 0, 0);
BLITZ_NAME("IconToShape", "shape[,shape]");

BLITZ_NULLSUB_AT(finit, ritooltypeslib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ritooltypeslib_libinit, 0, 0);

BLITZ_LIBFIN();

/* shapeslib MakeCookie (a3 = the shape) */
#define TOK_makecookie ((LIB_shapeslib << 7) + 2)

/* the FreeList behind the DiskObject (DiskObject+78), as icon.library
   allocates it */
struct diskobjfree {
    struct DiskObject dob;
    struct FreeList freelist;
};
static_assert(offsetof(struct diskobjfree, freelist) == 78);
#define DO_FREELIST(dob) (&((struct diskobjfree *)(dob))->freelist)
static struct Library *iconlibbase;
static struct DiskObject *diskobject;
static STRPTR *tooltype;                /* the tool type array when an icon is open */
static ULONG ttcap;                     /* its entries, the terminator included */
static ULONG toolbuffersize;
static BB_BOOL status_tool;           /* BB_FALSE: "(name=value)", switched off */
static STRPTR *array_address;
static const char *tooltype_address;
static const char *string_address;
static UWORD shapenumber;
/* SelectRender: the Image of ShapeToIcon shape,shape */
static struct Image SelectRender;

/* a shape object (shapeslib, 32 bytes): struct bb_shape (blitzobj.h);
   BB_CLUDGE = the data is not ours */

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

/**
 * @brief init: opens icon.library (any version).
 *
 * Without it (iconlibbase 0) the commands do nothing and return 0.
 */
static void BLITZ_ROUTINE c_libinit(void)
{
    iconlibbase = OpenLibrary((CONST_STRPTR)"icon.library", 0);
    tooltype = NULL;
    ttcap = 0;
}

/* a tool type array of n entries (the terminator included), cleared;
   NULL without memory */
static STRPTR *newtooltypes(ULONG n)
{
    return AllocMem(n * sizeof(STRPTR), MEMF_CLEAR | MEMF_PUBLIC);
}

/* frees the tool type array of the icon (fixed #855) */
static void freetooltypes(void)
{
    if (tooltype)
        FreeMem(tooltype, ttcap * sizeof(STRPTR));
    tooltype = NULL;
    ttcap = 0;
}

/**
 * @brief FreeIconObject: frees the open icon (FreeDiskObject) and its tool type array.
 * @return d0: BB_TRUE if an icon was open, BB_FALSE otherwise or without icon.library
 */
BB_BOOL BLITZ_ROUTINE c_Close_icon(void)
{
    if (!iconlibbase || !diskobject)
        return BB_FALSE;
    FreeDiskObject(diskobject);
    diskobject = NULL;
    freetooltypes();
    return BB_TRUE;
}

/**
 * @brief finit: frees the open icon, closes icon.library.
 */
static void BLITZ_ROUTINE c_libfinit(void)
{
    if (!iconlibbase)
        return;
    if (diskobject)
        c_Close_icon();
    CloseLibrary(iconlibbase);
}

/**
 * @brief GetIconObject(filename$): opens the icon of a file.
 *
 * GetDiskObjectNew; the icon's tool types are copied into the library's
 * array (allocated for their count and the terminator), which becomes the
 * icon's do_ToolTypes.  Fixed (#855): the original's array had 100
 * entries and the copy no limit.  BUG(orig): an icon without tool types
 * is read from address 0 (#845).
 * @param name  d0: the file name (without .info)
 * @return d0: the DiskObject, 0 if an icon is open already, there is none,
 *         no memory for the array or no icon.library
 */
ULONG BLITZ_ROUTINE c_getstructure(CONST_STRPTR name BL_REG(d0))
{
    STRPTR *from, *to;
    ULONG n;

    if (tooltype || !iconlibbase)
        return 0;
    diskobject = GetDiskObjectNew(name);
    if (!diskobject)
        return 0;
    from = diskobject->do_ToolTypes;
    for (n = 0; from[n]; n++)           /* BUG(orig): no NULL check (#845) */
        ;
    to = newtooltypes(n + 1);
    if (!to) {
        FreeDiskObject(diskobject);
        diskobject = NULL;
        return 0;
    }
    tooltype = to;
    ttcap = n + 1;
    diskobject->do_ToolTypes = to;
    while (*from)
        *to++ = *from++;
    return (ULONG)diskobject;
}

/**
 * @brief ClearToolTypes: removes all tool types of the open icon.
 *
 * Clears the entries of the array up to the first empty one; nothing
 * without an icon.
 */
void BLITZ_ROUTINE c_cleartooltypes(void)
{
    STRPTR *p = tooltype;

    if (!p)
        return;
    while (*p)
        *p++ = NULL;
}

/**
 * @brief FindToolNumber(tooltype#): the tool type of that number (0 = the first).
 *
 * Copied to a3 without a terminating 0 (string results need none: no
 * defect, #257).
 * @param n  d0: the number
 * @return d0: the length of the string, 0 if there is no such entry or no icon
 */
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

/* _istooltype: icon.library FindToolType; with `bracket` also the
   switched-off form "(tt$)", in an AllocMem buffer of the name's length
   + 3 ("(", ")", 0).  Fixed (#842): the original's buffer had 256 bytes,
   a name over 253 characters overflowed it. */
static ULONG findtooltype(CONST_STRPTR name, BOOL bracket)
{
    UBYTE *bracketed;
    ULONG size;
    const char *src;
    char *dst;
    ULONG r;

    if (!tooltype)
        return 0;
    string_address = (const char *)name;
    if (FindToolType((CONST_STRPTR *)tooltype, name))
        return (ULONG)-1;
    if (!bracket)
        return 0;
    if (*string_address == '(')
        return 0;
    for (size = 3; string_address[size - 3]; size++)
        ;
    bracketed = AllocMem(size, MEMF_PUBLIC);
    if (!bracketed)
        return 0;
    src = string_address;
    dst = (char *)bracketed;
    *dst++ = '(';
    while ((*dst++ = *src++) != 0)
        ;
    dst[-1] = ')';
    *dst = 0;
    r = FindToolType((CONST_STRPTR *)tooltype, (CONST_STRPTR)bracketed) ? 1 : 0;
    FreeMem(bracketed, size);
    return r;
}

/**
 * @brief FindToolType(tooltype$): whether the icon has the tool type.
 *
 * icon.library FindToolType, without the bracket search.  Fixed (#843):
 * the original kept the bracket check in a flag that FindToolType tt$,1
 * set and only the end of the bracket search cleared, so after such a
 * call that returned earlier this form also searched "(tt$)".
 * @param name  d0: the tool type's name
 * @return d0: -1 found, 0 not found or no icon
 */
ULONG BLITZ_ROUTINE c_istooltype(CONST_STRPTR name BL_REG(d0))
{
    return findtooltype(name, FALSE);
}

/**
 * @brief FindToolType(tooltype$,bracket_check): as FindToolType(tt$), with the bracket check.
 *
 * Also looks for the switched-off form "(tt$)" (for any bracket_check
 * value, as in the original), unless tt$ begins with "(".
 * @param name  d0: the tool type's name
 * @return d0: -1 found, 1 found as "(tt$)" (no defect: it tells the two
 *         apart), 0 not found or no icon
 */
ULONG BLITZ_ROUTINE c_istooltype2(CONST_STRPTR name BL_REG(d0))
{
    return findtooltype(name, TRUE);
}

/**
 * @brief FindToolValue(tooltype$): the value of a tool type.
 *
 * The value after "=" (icon.library FindToolType), written over the
 * argument at a3 (a3 - length - 1), without a 0; for a name beginning
 * with "(" (a switched-off entry "(name=value)") without its last ")".
 * Fixed (#844): the original stopped at every ")", also inside the value.
 * @param name  d0: the tool type's name
 * @param lens  a2: the argument stack (lens[-1] = the length of name)
 * @return d0: the length of the value, 0 if not found or no icon
 */
ULONG BLITZ_ROUTINE c_gettooltype(CONST_STRPTR name BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    STRPTR v;
    UWORD len = 0;
    BOOL off;

    if (!tooltype)
        return 0;
    v = FindToolType((CONST_STRPTR *)tooltype, name);
    if (!v)
        return 0;
    blitz_string -= lens[-1] + 1;       /* "Blitz is wierd..." */
    off = *name == '(';                 /* before the copy: it overwrites the name */
    while (*v)
        *blitz_string++ = *v++, len++;
    if (off && len && v[-1] == ')')
        blitz_string--, len--;          /* the end of "(name=value)" */
    return len;
}

/**
 * @brief MatchToolValue(tooltype$,value$): whether the tool type has the value.
 *
 * icon.library FindToolType, then MatchToolValue.
 * @param name   d0: the tool type's name
 * @param value  d1: the value
 * @return d0: BB_TRUE if it matches, BB_FALSE otherwise or no icon
 */
BB_BOOL BLITZ_ROUTINE c_Matchtoolvalue(CONST_STRPTR name BL_REG(d0),
                                       CONST_STRPTR value BL_REG(d1))
{
    STRPTR v;

    string_address = (const char *)value;
    if (!tooltype)
        return BB_FALSE;
    v = FindToolType((CONST_STRPTR *)tooltype, name);
    if (!v)
        return BB_FALSE;
    return MatchToolValue(v, (CONST_STRPTR)string_address) ? BB_TRUE : BB_FALSE;
}

/**
 * @brief PutIconObject(filename$,icontype): writes the open icon and closes it.
 *
 * A non-zero icontype (its low byte) becomes do_Type; PutDiskObject.  If
 * that fails, the icon stays open (it can be written again or freed with
 * FreeIconObject) and the result is 0 (fixed #851, the user's decision
 * of 2026-11-17: the original returned -1 and closed it).  BUG(orig): the
 * written icon is forgotten without FreeDiskObject (lost, #850).
 * @param name  d0: the file name
 * @param type  d1: the icon type (0: unchanged)
 * @return d0: BB_TRUE written, BB_FALSE writing failed or no icon
 */
BB_BOOL BLITZ_ROUTINE c_Put_icon(CONST_STRPTR name BL_REG(d0), ULONG type BL_REG(d1))
{
    if (!iconlibbase || !diskobject)
        return BB_FALSE;
    if ((UBYTE)type)
        diskobject->do_Type = (UBYTE)type;
    if (!PutDiskObject(name, diskobject))
        return BB_FALSE;
    diskobject = NULL;                  /* BUG(orig): not freed (#850) */
    freetooltypes();
    return BB_TRUE;
}

/**
 * @brief PutIconObject(filename$): as PutIconObject with the icon type unchanged.
 * @param name  d0: the file name
 * @return d0: BB_TRUE written, BB_FALSE writing failed or no icon
 */
BB_BOOL BLITZ_ROUTINE c_Put_icon2(CONST_STRPTR name BL_REG(d0))
{
    return c_Put_icon(name, 0);
}

/* _stringcompare: -1 if the entry (a1, its own "(" skipped) is the tool
   type of that name (a0): the name, case insensitive for a-z, then the
   end, "=" or ")".  Fixed (#853): the original took every entry that
   began with the name. */
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
    if (*entry && *entry != '=' && *entry != ')')
        return 0;
    return -1;
}

/* gotit: the new string "name=value" ("(name=value)" for status 0) into
   *array_address, on the icon's FreeList */
static BB_BOOL gotit(STRPTR *entry)
{
    UBYTE *mem, *d;
    const char *s;

    array_address = entry;
    mem = AllocMem(toolbuffersize, MEMF_CLEAR | MEMF_PUBLIC);
    if (!mem)
        return BB_FALSE;
    if (!AddFreeList(DO_FREELIST(diskobject), mem,
                     toolbuffersize)) {
        FreeMem(mem, toolbuffersize);   /* fixed #840: the original freed address 0 */
        return BB_FALSE;
    }
    *array_address = (STRPTR)mem;
    d = mem;
    s = tooltype_address;
    if (!status_tool)
        *d++ = '(';
    while (*s)                          /* fixed #841: the original's do-while */
        *d++ = *s++;
    s = string_address;
    if (*s) {
        *d++ = '=';
        do
            *d++ = *s++;
        while (*s);
    }
    if (!status_tool)
        *d = ')';
    return BB_TRUE;
}

/* the arguments of SetToolValue/NewToolType */
static void toolargs(const char *name, const char *value, BB_BOOL status, const ULONG *lens)
{
    tooltype_address = name;
    string_address = value;
    status_tool = status;
    toolbuffersize = lens[-1] + lens[-2] + 8;
}

/**
 * @brief SetToolValue(tooltype$,value$,status): replaces a tool type.
 *
 * The first entry of tt$ (tt$ case insensitive for a-z, then the end,
 * "=" or ")"; its own "(" skipped: "OFF" also replaces "(OFF)=1", no
 * defect) becomes "tt$=value$" (no "=" for an empty value),
 * "(tt$=value$)" with status 0; the string is allocated (length of both
 * + 8) and hung on the icon's FreeList.  Fixed (#853): the original took
 * the first entry that only began with tt$ ("FL" replaced "FLAG").
 * An empty name gives "=value$" (fixed #841).  A failing AddFreeList
 * frees the block (fixed #840: the original freed address 0).
 * @param name    d0: the tool type's name
 * @param value   d1: the value
 * @param status  d2: BB_FALSE for the switched-off form
 * @param lens    a2: the argument stack (the lengths of name and value)
 * @return d0: BB_TRUE done, BB_FALSE no such entry, no memory or no icon
 */
BB_BOOL BLITZ_ROUTINE c_PutTool(const char *name BL_REG(d0), const char *value BL_REG(d1),
                                BB_BOOL status BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    STRPTR *p;

    toolargs(name, value, status, lens);
    if (!(p = tooltype))
        return BB_FALSE;
    for (; *p; p++)
        if (stringcompare((const UBYTE *)tooltype_address, (const UBYTE *)*p))
            return gotit(p);
    return BB_FALSE;
}

/**
 * @brief SetToolValue(tooltype$,value$): as SetToolValue with status -1.
 * @param name   d0: the tool type's name
 * @param value  d1: the value
 * @param lens   a2: the argument stack
 * @return d0: BB_TRUE done, BB_FALSE otherwise
 */
BB_BOOL BLITZ_ROUTINE c_PutTool2(const char *name BL_REG(d0), const char *value BL_REG(d1),
                                 const ULONG *lens BL_REG(a2))
{
    return c_PutTool(name, value, BB_TRUE, lens);
}

/**
 * @brief NewToolType(tooltype$,value$,status): appends a tool type.
 *
 * A new entry at the end, built as by SetToolValue.  A full array is
 * replaced by one of twice the size (fixed #855: the original's array had
 * 100 entries and NewToolType no limit).
 * @param name    d0: the tool type's name
 * @param value   d1: the value
 * @param status  d2: BB_FALSE for the switched-off form "(tt$=value$)"
 * @param lens    a2: the argument stack (the lengths of name and value)
 * @return d0: BB_TRUE done, BB_FALSE no memory (nothing changed) or no icon
 */
BB_BOOL BLITZ_ROUTINE c_NewTool(const char *name BL_REG(d0), const char *value BL_REG(d1),
                                BB_BOOL status BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    STRPTR *p, *larger;
    ULONG n, i;

    toolargs(name, value, status, lens);
    if (!(p = tooltype))
        return BB_FALSE;
    for (n = 0; p[n]; n++)
        ;
    if (n + 2 > ttcap) {                /* the new entry and the terminator */
        larger = newtooltypes(ttcap * 2 + 2);
        if (!larger)
            return BB_FALSE;
        for (i = 0; i < n; i++)
            larger[i] = p[i];
        FreeMem(p, ttcap * sizeof(STRPTR));
        tooltype = p = larger;
        ttcap = ttcap * 2 + 2;
        diskobject->do_ToolTypes = larger;
    }
    p[n + 1] = NULL;
    return gotit(&p[n]);
}

/**
 * @brief NewToolType(tooltype$,value$): as NewToolType with status -1.
 * @param name   d0: the tool type's name
 * @param value  d1: the value
 * @param lens   a2: the argument stack
 * @return d0: BB_TRUE done, BB_FALSE otherwise
 */
BB_BOOL BLITZ_ROUTINE c_NewTool2(const char *name BL_REG(d0), const char *value BL_REG(d1),
                                 const ULONG *lens BL_REG(a2))
{
    return c_NewTool(name, value, BB_TRUE, lens);
}

/**
 * @brief SetIconHit width,height: the size of the icon's gadget (the hit box).
 * @param w  d0: the width
 * @param h  d1: the height
 * Returns nothing (nothing happens without an icon).
 */
void BLITZ_ROUTINE c_seticonhit(UWORD w BL_REG(d0), UWORD h BL_REG(d1))
{
    if (!diskobject)
        return;
    diskobject->do_Gadget.Width = w;
    diskobject->do_Gadget.Height = h;
}

/**
 * @brief ShapeToIcon shape: the icon's image from a shape.
 *
 * The gadget gets the shape's width and height + 1, its GadgetRender
 * Image the shape's width, height, depth and data.
 * @param shape  a0: the shape object (#ia0|#pd0)
 * Returns nothing (nothing happens without an icon).
 */
void BLITZ_ROUTINE c_shapetoicon(const struct bb_shape *shape BL_REG(a0))
{
    struct Image *im;

    if (!diskobject)
        return;
    diskobject->do_Gadget.Width = shape->width;
    diskobject->do_Gadget.Height = shape->height;
    diskobject->do_Gadget.Height += 1;
    im = (struct Image *)diskobject->do_Gadget.GadgetRender;
    im->Width = shape->width;
    im->Height = shape->height;
    im->Depth = shape->depth;
    im->ImageData = (UWORD *)shape->data;
}

/**
 * @brief ShapeToIcon shape,shape: the icon's image and selected image from two shapes.
 *
 * The second shape goes into the library's own Image (SelectRender),
 * then as ShapeToIcon shape.  BUG(orig) (#854): that Image has PlanePick/
 * PlaneOnOff 0, and the gadget flags become 6 (GADGHIMAGE), all others
 * dropped.  DIFF: a3 unchanged.
 * @param shape   a0: the shape for the image (#ia0|#pd0)
 * @param shape2  a1: the shape for the selected image (#ia1|#pd1)
 * Returns nothing (nothing happens without an icon).
 */
void BLITZ_ROUTINE c_shapetoicon2(const struct bb_shape *shape BL_REG(a0),
                                  const struct bb_shape *shape2 BL_REG(a1))
{
    if (!diskobject)
        return;
    SelectRender.Width = shape2->width;
    SelectRender.Height = shape2->height;
    SelectRender.Depth = shape2->depth;
    SelectRender.ImageData = (UWORD *)shape2->data;
    diskobject->do_Gadget.SelectRender = &SelectRender;
    diskobject->do_Gadget.Flags = 6;    /* GFLG_GADGHIMAGE | GFLG_GADGIMAGE */
    c_shapetoicon(shape);
}

/**
 * @brief SetIconType object_type: the icon's type (do_Type, 1-5).
 *
 * Fixed (#852): without an icon the original returned the argument.
 * @param type  d0: the type
 * @return d0: -1 done, 0 for a type outside 1-5 or without an icon
 */
ULONG BLITZ_ROUTINE c_icontype(ULONG type BL_REG(d0))
{
    if (!diskobject)
        return 0;
    if (!type || type > 5)
        return 0;
    diskobject->do_Type = (UBYTE)type;
    return (ULONG)-1;
}

/**
 * @brief IconRender render_flags: the flags of the icon's gadget.
 * @param flags  d0: the gadget flags (a word)
 * Returns nothing (nothing happens without an icon).
 */
void BLITZ_ROUTINE c_selectrender(UWORD flags BL_REG(d0))
{
    if (diskobject)
        diskobject->do_Gadget.Flags = flags;
}

/**
 * @brief IconDefTool tool$: the icon's default tool.
 *
 * A copy of tool$ (AllocMem, on the icon's FreeList) becomes
 * do_DefaultTool.  a3 is rewound by the length + 1 and stays there: no
 * defect, it gives back the space of the string argument, which the
 * compiler does not take back after a library function with a numeric
 * result (#846, closed; without it "ab"+Str$(IconDefTool("tool"))+"cd"
 * gives "abtool" instead of "ab-1cd").  A failing AddFreeList frees the
 * block (fixed #840: the original freed address 0).
 * @param tool  d0: the default tool
 * @param lens  a2: the argument stack (lens[-1] = the length of tool$)
 * @return d0: BB_TRUE done, BB_FALSE for an empty string, no memory or no icon
 */
BB_BOOL BLITZ_ROUTINE c_defaulttool(const char *tool BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    UBYTE *mem, *d;
    const char *s;
    ULONG len;

    if (!diskobject)
        return BB_FALSE;
    string_address = tool;
    len = lens[-1];
    if (!len)
        return BB_FALSE;
    toolbuffersize = len + 1;
    blitz_string -= toolbuffersize;     /* gives back the string argument's space (no defect, #846) */
    mem = AllocMem(toolbuffersize, MEMF_CLEAR | MEMF_PUBLIC);
    if (!mem)
        return BB_FALSE;
    if (!AddFreeList(DO_FREELIST(diskobject), mem,
                     toolbuffersize)) {
        FreeMem(mem, toolbuffersize);   /* fixed #840: the original freed address 0 */
        return BB_FALSE;
    }
    d = mem;
    s = string_address;
    do
        *d++ = *s++;
    while (*s);
    diskobject->do_DefaultTool = (STRPTR)mem;
    return BB_TRUE;
}

/* freeashape (a3 = shape): free data and cookie of a shape in use */
static void freeashape(struct bb_shape *s)
{
    APTR c;

    if (!s->width)
        return;
    s->width = 0;
    if (s->cludge != BB_CLUDGE) {
        s->cludge = 0;
        globfree((ULONG)s->data, s->size);
    }
    c = s->cookie;
    if (c) {
        s->cookie = NULL;
        globfree((ULONG)c, s->planesize);
    }
}

/* icontoshape_jump: shape (a0) from the image at do_Gadget+render
   (GadgetRender 22, SelectRender 26: RENDER_IMAGE/RENDER_SELECT);
   shapearg = d0, `cookiestr` = the a3 the original has at this point (the
   shape after freeashape, otherwise unchanged) */
#define RENDER_IMAGE offsetof(struct DiskObject, do_Gadget.GadgetRender)
#define RENDER_SELECT offsetof(struct DiskObject, do_Gadget.SelectRender)
static_assert(RENDER_IMAGE == 22 && RENDER_SELECT == 26);

static void icontoshape_jump(ULONG shapearg, struct bb_shape *shape, UWORD render,
                             UBYTE **cookiestr)
{
    const struct Image *im;
    UWORD w, h, words, bytes;
    UWORD planebytes;                   /* the bytes of one plane */
    UWORD count;                        /* the DBRA count of the words to copy */
    const UWORD *from;
    UWORD *to;
    ULONG mem;

    if (shape->width) {
        *cookiestr = (UBYTE *)shape;
        freeashape(shape);
    }
    shapenumber = (UWORD)((WORD)shapearg >> 5);
    im = *(const struct Image **)((UBYTE *)diskobject + render);
    if (!im)
        return;
    *(ULONG *)&shape->width = *(const ULONG *)&im->Width;      /* width and height, a long */
    shape->depth = im->Depth;
    w = im->Width;
    h = im->Height;
    words = (UWORD)((UWORD)(w + 15) >> 4);
    bytes = (UWORD)(words + words);
    shape->ebwidth = bytes;
    shape->bltsize = (UWORD)(((UWORD)(h << 6) | words) + 1);
    *(ULONG *)&shape->handlex = 0;      /* both handles, a long */
    planebytes = (UWORD)bl_mulu_w(bytes, h);
    shape->planesize = planebytes;
    shape->xplanesize = (UWORD)(planebytes + (UWORD)(h << 1));
    shape->size = (UWORD)bl_mulu_w(planebytes, im->Depth);
    shape->xsize = (UWORD)bl_mulu_w(shape->xplanesize, im->Depth);
    mem = globalloc(shape->size, 0x10001 | MEMF_CHIP);
    if (!mem)
        return;
    shape->data = (APTR)mem;
    from = im->ImageData;
    to = (UWORD *)mem;
    /* BUG(orig) #847: (width >> 4) + 1 words per row (MULU.w: low words) */
    count = (UWORD)bl_mulu_w((UWORD)bl_mulu_w((UWORD)(((WORD)w >> 4) + 1), h), im->Depth);
    count--;
    do
        *to++ = *from++;
    while (count-- != 0);               /* DBRA */
    shape->cludge = 0;
    {
        /* ALibJsr MakeCookie with d0 = shapenumber and the original's a3 */
        UBYTE *keep = blitz_string;
        struct bl_regs r = { .d0 = shapenumber };

        blitz_string = *cookiestr;
        BLITZ_ALIBJSR(TOK_makecookie, r);
        blitz_string = keep;
    }
}

/**
 * @brief IconToShape shape: the icon's image (GadgetRender) as a shape.
 *
 * A shape in use is freed first; the data is allocated with memlib
 * globalloc (CHIP) and copied from the Image, the cookie made with
 * shapeslib's MakeCookie.  BUG(orig): the shape is filled before the data
 * is allocated (#848, not a bug in practice); the copy takes one word per
 * row too many for a width that is a multiple of 16 (#847); MakeCookie
 * gets the caller's a3 unless the shape was in use (#849).  DIFF: a3 kept.
 * @param shapearg  d0: the shape's offset (shape# << 5)
 * @param shape     a0: the shape object (#ia0|#pd0)
 * Returns nothing (nothing happens without an icon or image).
 */
void BLITZ_ROUTINE c_icontoshape(ULONG shapearg BL_REG(d0), struct bb_shape *shape BL_REG(a0))
{
    UBYTE *cookiestr = blitz_string;    /* a3 */

    if (!diskobject)
        return;
    icontoshape_jump(shapearg, shape, RENDER_IMAGE, &cookiestr);
}

/**
 * @brief IconToShape shape,shape: the image and the selected image as two shapes.
 *
 * As IconToShape shape for each; BUG(orig) (#849): the second shape gets
 * the cookie of the first (a3 is still the first shape).
 * @param shapearg   d0: the first shape's offset (shape# << 5)
 * @param shapearg2  d1: the second shape's offset
 * @param shape      a0: the shape for the image (#ia0|#pd0)
 * @param shape2     a1: the shape for the selected image (#ia1|#pd1)
 */
void BLITZ_ROUTINE c_icontoshape2(ULONG shapearg BL_REG(d0),
                                  ULONG shapearg2 BL_REG(d1),
                                  struct bb_shape *shape BL_REG(a0),
                                  struct bb_shape *shape2 BL_REG(a1))
{
    UBYTE *cookiestr = blitz_string;    /* a3 */

    if (!diskobject)
        return;
    icontoshape_jump(shapearg, shape, RENDER_IMAGE, &cookiestr);
    icontoshape_jump(shapearg2, shape2, RENDER_SELECT, &cookiestr);
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(ritooltypeslib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(ritooltypeslib_libfinit, c_libfinit, "d0-d1/a0-a1");
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
