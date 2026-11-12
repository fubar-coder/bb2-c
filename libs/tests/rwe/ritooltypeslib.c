/*
 * rwe ritooltypeslib - the C build of the RWE userlib ritooltypeslib
 * (libs/userlibs/rwe/ritooltypeslib.c) or the CD binary BSS2
 * RITOOLTYPESLIB.OBJ (ritooltypeslib.orig), one set as LIBSET says,
 * observed as cases (sysload.h; the expected data
 * libs/tests/expected/rwe_ritooltypeslib.txt).
 *
 *   ritooltypeslib DIR     (DIR: ritooltypeslib.obj, .orig and the
 *                           acidlibs below as .obj/.orig)
 *
 * The test writes an icon RAM:rwett.info (icon.library's default tool
 * icon with its own tool types) and lets the set work on it (other
 * registers = markers).  The set has its own memlib and shapeslib
 * (MakeCookie, ALibJsr from IconToShape; with the libraries these need);
 * the test has the string area (a3), the argument stack (a2), the shapes
 * and the output files (RAM:rwett1, RAM:rwett2).  Observed: d0, a3 and
 * the string area of the string functions, the kept registers, the open
 * icon (tool type array, default tool, type,
 * gadget size and flags, images), the shapes made by IconToShape (fields,
 * data, cookie) and the icons written by PutIconObject (read back with
 * GetDiskObject).
 * Covered: every command and form without an icon (nothing, BUG(orig) #852:
 * SetIconType returns its argument), GetIconObject (twice: the second
 * gives 0; a missing file), FindToolValue (value up to ")", missing
 * name), FindToolNumber (all entries, beyond the end), MatchToolValue,
 * FindToolType with and without bracket check (BUG(orig) #843: the flag
 * stays set), SetToolValue (replace, prefix match "FL" -> "FLAG", the
 * entry's "(" skipped, status 0 = "(name=value)", not found), NewToolType
 * (with and without value and status), ClearToolTypes, SetIconHit,
 * IconRender, SetIconType 0-6, IconDefTool (BUG(orig) #846: a3 rewound; empty
 * string), ShapeToIcon (both forms), IconToShape (both forms, shapes in
 * use and fresh, BUG(orig) #849: MakeCookie on the caller's a3 for a fresh
 * shape - here a3 points to a decoy shape; the second shape of the two
 * argument form gets no cookie), PutIconObject (both forms), FreeIconObject,
 * finit with an icon open, init.
 * Not covered: icon.library missing, AllocMem/AddFreeList failing (the
 * FreeMem(0) of BUG(orig), #840), more than 100 tool types, IconToShape with a
 * width that is a multiple of 16 (writes behind the shape's data,
 * BUG(orig), #847) or into a shape when the icon has no image.
 */
#include "sysload.h"
#define __NOLIBBASE__
#define ICON_BASE_NAME TTIconBase
#include <proto/icon.h>
#undef __NOLIBBASE__
#include <workbench/workbench.h>
#include <intuition/intuition.h>

static struct Library *TTIconBase;

static const char *names[] = { "ritooltypeslib", "shapeslib", "memlib", "bitmaplib",
                               "environlib", "iffiolib", "ilbmifflib", "intlib",
                               "palettelib", "scrolllib", NULL };

enum { GETICON, FINDTOOLVALUE, SETTOOLVALUE, PUTICON, FREEICON, MATCHTOOLVALUE, NEWTOOLTYPE,
       FINDTOOLNUMBER, CLEARTOOLTYPES, SETICONHIT, SHAPETOICON, SETICONTYPE, ICONRENDER,
       ICONDEFTOOL, FINDTOOLTYPE, ICONTOSHAPE, FINIT, INIT };

/* the registers of the call (R after it, IN before it); ROUT and VEC
   (the key of the cases) are sysload.h's */
static struct regs R, IN;


static struct lib *thelib(void)
{
    return getlib(&loaded, names[0]);
}

static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(thelib(), idx, form) & ~1UL);
}

/* the code of a library's init (header +22) or finit (+28) nullsub */
static APTR nullsub(const char *name, int off)
{
    struct lib *l = getlib(&loaded, name);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

/* call entry idx, form `form`: markers in all registers, `setup` sets R;
   then the kept case */
#define DO1(idx, form, setup) do {                                          \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callregs(ent(idx, form), &R, 0, 0);                                 \
        obs_kept(&IN, &R);                                                  \
    } while (0)

/* --------------------------------------------- strings, a2 and a3 */

#define AREA 512
#define ARGAT 64
static UBYTE area[AREA];                /* the string area (a3) */
static char sbuf[2][300];               /* string arguments */
static ULONG lens[4];                   /* the argument stack: a2 = &lens[2] */

static void fillarea(void)
{
    int i;
    for (i = 0; i < AREA; i++)
        area[i] = (UBYTE)(0xA0 ^ i);
}

/* string arguments s1 (d0) and s2 (d1): in the test's buffers, their
   lengths below a2; a3 = behind a copy of s1 in the string area (as the
   compiler leaves it after building the argument there) */
static void strargs(const char *s1, const char *s2)
{
    strcpy(sbuf[0], s1);
    strcpy(sbuf[1], s2 ? s2 : "");
    lens[0] = strlen(s1);
    lens[1] = s2 ? strlen(s2) : strlen(s1);
    memcpy(&area[ARGAT], s1, strlen(s1) + 1);
    R.d[0] = (ULONG)sbuf[0];
    if (s2)
        R.d[1] = (ULONG)sbuf[1];
    R.a[2] = (ULONG)&lens[2];
    R.a[3] = (ULONG)&area[ARGAT + strlen(s1) + 1];
}

static ULONG rela3(void)
{
    return R.a[3] - (ULONG)area;
}

static void same_area(void)
{
    OBS("a3", rela3());
    OBSBYTES("string area", area, AREA);
}

/* the icon the set has open (GetIconObject's d0) */
static struct DiskObject *dobj;

static ULONG strsum(const char *s)
{
    ULONG v = 0;
    if (!s)
        return 0;
    while (*s)
        v = v * 31 + (UBYTE)*s++;
    return v + 1;
}

static ULONG imagesum(const struct Image *im)
{
    ULONG v, n, i;
    if (!im)
        return 0;
    v = im->Width | (ULONG)im->Height << 12 | (ULONG)im->Depth << 24;
    n = ((im->Width + 15) >> 4) * im->Height * im->Depth;
    for (i = 0; i < n && im->ImageData; i++)
        v = v * 31 + im->ImageData[i];
    return v;
}

/* the contents of a DiskObject */
static void same_dobj(const char *what, struct DiskObject *d)
{
    char buf[60];
    int i;

    sprintf(buf, "%s: open", what);
    OBS(buf, d != NULL);
    if (!d)
        return;
    sprintf(buf, "%s: type", what);
    OBS(buf, d->do_Type);
    sprintf(buf, "%s: gadget size", what);
    OBS(buf, (ULONG)d->do_Gadget.Width << 16 | d->do_Gadget.Height);
    sprintf(buf, "%s: gadget flags", what);
    OBS(buf, d->do_Gadget.Flags);
    sprintf(buf, "%s: default tool", what);
    OBS(buf, strsum((const char *)d->do_DefaultTool));
    sprintf(buf, "%s: image", what);
    OBS(buf, imagesum((struct Image *)d->do_Gadget.GadgetRender));
    sprintf(buf, "%s: select image", what);
    OBS(buf, imagesum((struct Image *)d->do_Gadget.SelectRender));
    sprintf(buf, "%s: tool types", what);
    OBS(buf, d->do_ToolTypes != NULL);
    if (!d->do_ToolTypes)
        return;
    for (i = 0; i < 100; i++) {
        sprintf(buf, "%s: tool type %d", what, i);
        OBS(buf, strsum((const char *)d->do_ToolTypes[i]));
        if (!d->do_ToolTypes[i])
            break;
    }
}

static void same_icon(void)
{
    same_dobj("icon", dobj);
}

/* ------------------------------------------------------------ calls */

/* a command with string arguments; result d0 (normalised: the icon = 1) */
static void scall(int idx, int form, const char *name, const char *s1, const char *s2,
                  int extra, ULONG d2)
{
    ROUT = name;
    snprintf(VEC, sizeof VEC, "\"%s\"%s%s%s", s1, s2 ? ",\"" : "", s2 ? s2 : "",
             s2 ? "\"" : "");
    if (extra)
        sprintf(VEC + strlen(VEC), ",%ld", (LONG)d2);
    fillarea();
    DO1(idx, form, strargs(s1, s2); if (extra) { if (s2) R.d[2] = d2; else R.d[1] = d2; });
    /* GetIconObject: the DiskObject as 1 */
    OBS("d0", idx == GETICON && R.d[0] ? 1 : R.d[0]);
    same_area();
    if (idx != GETICON)
        same_icon();
}

/* statements: d0 is no result (the original leaves 0 there on some
   paths, the C build keeps it) */
static int statement(int idx)
{
    return idx == CLEARTOOLTYPES || idx == SETICONHIT || idx == SHAPETOICON
           || idx == ICONRENDER || idx == ICONTOSHAPE;
}

static void ncall(int idx, int form, const char *name, ULONG d0, ULONG d1)
{
    ROUT = name;
    sprintf(VEC, "$%lX,$%lX", d0, d1);
    fillarea();
    DO1(idx, form, R.d[0] = d0; R.d[1] = d1;
        R.a[3] = (ULONG)&area[ARGAT]);
    if (!statement(idx))
        OBS("d0", R.d[0]);
    same_area();
    if (idx != FREEICON)
        same_icon();
}

static void geticon(const char *file)
{
    scall(GETICON, 0, "GetIconObject", file, NULL, 0, 0);
    if (R.d[0])
        dobj = (struct DiskObject *)R.d[0];
    same_icon();
}

static void freeicon(void)
{
    ncall(FREEICON, 0, "FreeIconObject", 0, 0);
    if (R.d[0])
        dobj = NULL;
}

/* ------------------------------------------------------------ shapes */

#define NSHAPES 8
static UBYTE shape[NSHAPES][32];
static UBYTE decoy[32];

static ULONG memcall(UWORD tok, ULONG d0, ULONG d1, ULONG a1)
{
    struct regs r;
    markregs(&r);
    r.d[0] = d0;
    r.d[1] = d1;
    r.a[1] = a1;
    callregs(find_token(&loaded, tok, 0), &r, 0, 0);
    return r.d[0];
}

/* a shape in use, its data from the set's memlib (freeashape frees it) */
static void makeshape(UBYTE *s, UWORD w, UWORD h, UWORD depth, UBYTE seed)
{
    UWORD words = (w + 15) >> 4, ebw = words * 2;
    UWORD one = ebw * h;
    UBYTE *data;
    ULONG i;

    memset(s, 0, 32);
    *(UWORD *)(s + 0) = w;
    *(UWORD *)(s + 2) = h;
    *(UWORD *)(s + 4) = depth;
    *(UWORD *)(s + 6) = ebw;
    *(UWORD *)(s + 8) = ((h << 6) | words) + 1;
    *(UWORD *)(s + 22) = one;
    *(UWORD *)(s + 24) = one + h * 2;
    *(UWORD *)(s + 26) = one * depth;
    *(UWORD *)(s + 28) = (one + h * 2) * depth;
    data = (UBYTE *)memcall(0xc002, one * depth, MEMF_CHIP | MEMF_CLEAR, 0);
    for (i = 0; i < (ULONG)one * depth; i++)
        data[i] = (UBYTE)(i * 7 + seed);
    *(ULONG *)(s + 14) = (ULONG)data;
}

static void same_shape(const char *what, UBYTE *s)
{
    UBYTE fields[24];
    char buf[60];

    /* the fields but the data and cookie pointers: 0-13, 22-31 */
    memcpy(fields, s, 14);
    memcpy(fields + 14, s + 22, 10);
    sprintf(buf, "%s: fields", what);
    OBSBYTES(buf, fields, sizeof fields);
    sprintf(buf, "%s: width/height", what);
    OBS(buf, *(ULONG *)s);
    sprintf(buf, "%s: data", what);
    OBS(buf, *(ULONG *)(s + 14) != 0);
    if (*(ULONG *)(s + 14) && *(UWORD *)s) {
        sprintf(buf, "%s: data bytes", what);
        OBSBYTES(buf, (void *)*(ULONG *)(s + 14), *(UWORD *)(s + 26));
    }
    sprintf(buf, "%s: cookie", what);
    OBS(buf, *(ULONG *)(s + 18) != 0);
    if (*(ULONG *)(s + 18)) {
        sprintf(buf, "%s: cookie bytes", what);
        OBSBYTES(buf, (void *)*(ULONG *)(s + 18), *(UWORD *)(s + 22));
    }
}

static void same_shapes(void)
{
    char buf[20];
    int i;
    for (i = 0; i < NSHAPES; i++) {
        sprintf(buf, "shape %d", i);
        same_shape(buf, shape[i]);
    }
    same_shape("decoy", decoy);
}

static void shapecall(int idx, int form, const char *name, int a, int b, UBYTE *a3)
{
    ROUT = name;
    sprintf(VEC, "%d%s%d", a, form ? "," : "", form ? b : 0);
    DO1(idx, form, R.d[0] = (ULONG)a << 5; R.a[0] = (ULONG)shape[a];
        R.d[1] = (ULONG)b << 5; R.a[1] = (ULONG)shape[b];
        R.a[3] = a3 ? (ULONG)decoy : (ULONG)&area[ARGAT]);
    same_icon();
    same_shapes();
}

/* ------------------------------------------------------ the test icon */

static const char *tooltypes[] = { "FOO=bar", "(OFF)=1", "FLAG", "Mixed=Case|Two",
                                   "(BRACK)", "PAREN=a)b", "lower=x", NULL };

static BOOL writeicon(void)
{
    struct DiskObject *d = GetDefDiskObject(WBTOOL);
    STRPTR *old;
    BOOL ok;

    if (!d)
        return FALSE;
    old = d->do_ToolTypes;
    d->do_ToolTypes = (STRPTR *)tooltypes;
    ok = PutDiskObject((CONST_STRPTR)"RAM:rwett", d);
    d->do_ToolTypes = old;
    FreeDiskObject(d);
    return ok;
}

static void readback(const char *what, const char *file)
{
    struct DiskObject *d = GetDiskObject((CONST_STRPTR)file);
    ROUT = what;
    strcpy(VEC, "");
    same_dobj("written icon", d);
    if (d)
        FreeDiskObject(d);
}

/* PutIconObject to <base> */
static void puticon(const char *base, int form, ULONG type)
{
    ROUT = "PutIconObject";
    sprintf(VEC, "%s,%lu (form %d)", base, type, form);
    DO1(PUTICON, form, strargs(base, NULL); R.d[1] = type);
    OBS("d0", R.d[0]);
    if (R.d[0])
        dobj = NULL;
}

int main(int argc, char **argv)
{
    static const char *names16[] = { "FOO", "OFF", "FLAG", "MIXED", "BRACK", "PAREN", "LOWER",
                                     "nothere", "", NULL };
    int i;

    if (argc < 2) {
        printf("usage: ritooltypeslib DIR\n");
        return 20;
    }
    TTIconBase = OpenLibrary((CONST_STRPTR)"icon.library", 36);
    if (!TTIconBase || !load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("FindToolValue (no icon)", d2-d7|a0-a3|a6);
    KEEPS("FindToolNumber (no icon)", d0-d7|a0-a3|a6);
    KEEPS("FreeIconObject (no icon)", d0-d7|a0-a3);
    KEEPS("PutIconObject (no icon)", d2-d6|a0-a3);
    KEEPS("SetIconType (no icon)", d0-d7|a0-a3|a6);
    KEEPS("IconDefTool (no icon)", d1-d7|a0-a3|a6);
    KEEPS("FindToolType (no icon)", d2-d7|a0-a3|a6);
    KEEPS("MatchToolValue (no icon)", d2-d7|a0-a3|a6);
    KEEPS("SetToolValue (no icon)", d1|d3-d6|a1|a3|a6);
    KEEPS("NewToolType (no icon)", d1-d6|a1|a3|a6);
    KEEPS("ClearToolTypes (no icon)", d0-d7|a0-a3|a6);
    KEEPS("SetIconHit (no icon)", d1|d3-d7|a0-a3|a6);
    KEEPS("IconRender (no icon)", d0-d7|a0-a3|a6);
    KEEPS("ShapeToIcon (no icon)", d0|d1|d3-d7|a0-a3|a6);
    KEEPS("IconToShape (no icon)", d1-d7|a0-a3|a6);
    KEEPS("GetIconObject", d2-d6|a3);
    KEEPS("FindToolValue", d2-d6);
    KEEPS("FindToolNumber", d2-d7|a1|a2|a6);
    KEEPS("MatchToolValue", d2-d7|a2|a3);
    KEEPS("FindToolType", d2-d6|a2|a3);
    KEEPS("SetToolValue", d3-d6|a3);
    KEEPS("NewToolType", d3-d6|a3);
    KEEPS("SetIconHit", d0|d1|d3-d7|a1-a3|a6);
    KEEPS("IconRender", d0-d7|a1-a3|a6);
    KEEPS("SetIconType", d1-d7|a1-a3|a6);
    KEEPS("IconDefTool", d3-d7);
    KEEPS("ShapeToIcon", d0|d1|d3-d7|a6);
    KEEPS("IconToShape (fresh, a3 = decoy)", d6|a3|a6);
    KEEPS("IconToShape (in use)", d6|a0|a6);
    KEEPS("IconToShape (both in use)", d6|a6);
    KEEPS("IconToShape (in use, fresh)", d6|a6);
    KEEPS("PutIconObject", d2-d6|a2|a3);
    KEEPS("FreeIconObject", d2-d7|a2|a3);
    KEEPS("FindToolNumber (written icon)", d2-d7|a1|a2|a6);
    KEEPS("ClearToolTypes", d0|d2-d7|a1-a3|a6);
    KEEPS("FindToolNumber (cleared)", d0-d7|a1-a3|a6);
    KEEPS("NewToolType (cleared)", d3-d6|a3);
    KEEPS("finit", d2-d7|a2|a3);
    if (!writeicon()) {
        check("ritooltypeslib: write RAM:rwett.info", FALSE);
        return 20;
    }
    ROUT = "memlib/shapeslib init";
    strcpy(VEC, "");
    {
        struct regs r;
        markregs(&r);
        callregs(nullsub("memlib", 22), &r, 0, 0);
        markregs(&r);
        callregs(nullsub("shapeslib", 22), &r, 0, 0);
    }
    ROUT = "init";
    DO1(INIT, 0, (void)0);

    /* no icon open */
    scall(FINDTOOLVALUE, 0, "FindToolValue (no icon)", "FOO", NULL, 0, 0);
    ncall(FINDTOOLNUMBER, 0, "FindToolNumber (no icon)", 0, 0);
    ncall(FREEICON, 0, "FreeIconObject (no icon)", 0, 0);
    scall(PUTICON, 0, "PutIconObject (no icon)", "RAM:rwett_x", NULL, 0, 0);
    ncall(SETICONTYPE, 0, "SetIconType (no icon)", 3, 0);
    scall(ICONDEFTOOL, 0, "IconDefTool (no icon)", "C:Dir", NULL, 0, 0);
    scall(FINDTOOLTYPE, 0, "FindToolType (no icon)", "FOO", NULL, 0, 0);
    scall(FINDTOOLTYPE, 1, "FindToolType (no icon)", "FOO", NULL, 1, 1);  /* flag stays */
    scall(MATCHTOOLVALUE, 0, "MatchToolValue (no icon)", "FOO", "bar", 0, 0);
    scall(SETTOOLVALUE, 0, "SetToolValue (no icon)", "FOO", "baz", 0, 0);
    scall(NEWTOOLTYPE, 1, "NewToolType (no icon)", "NEW", "1", 1, 0);
    ncall(CLEARTOOLTYPES, 0, "ClearToolTypes (no icon)", 0, 0);
    ncall(SETICONHIT, 0, "SetIconHit (no icon)", 10, 10);
    ncall(ICONRENDER, 0, "IconRender (no icon)", 5, 0);
    makeshape(shape[0], 20, 10, 2, 1);
    makeshape(shape[1], 25, 7, 1, 50);
    makeshape(decoy, 25, 7, 1, 99);
    shapecall(SHAPETOICON, 0, "ShapeToIcon (no icon)", 0, 0, NULL);
    shapecall(SHAPETOICON, 1, "ShapeToIcon (no icon)", 0, 1, NULL);
    shapecall(ICONTOSHAPE, 0, "IconToShape (no icon)", 2, 0, NULL);
    shapecall(ICONTOSHAPE, 1, "IconToShape (no icon)", 2, 3, NULL);

    /* the icon */
    geticon("RAM:rwett");
    check("ritooltypeslib: GetIconObject RAM:rwett", dobj != NULL);
    if (!dobj)
        return 5;
    geticon("RAM:rwett");                       /* open already: 0 */
    for (i = 0; names16[i]; i++)
        scall(FINDTOOLVALUE, 0, "FindToolValue", names16[i], NULL, 0, 0);
    for (i = 0; i < 9; i++)
        ncall(FINDTOOLNUMBER, 0, "FindToolNumber", i, 0);
    ncall(FINDTOOLNUMBER, 0, "FindToolNumber", 0xffffffffUL, 0);
    scall(MATCHTOOLVALUE, 0, "MatchToolValue", "MIXED", "two", 0, 0);
    scall(MATCHTOOLVALUE, 0, "MatchToolValue", "mixed", "CASE", 0, 0);
    scall(MATCHTOOLVALUE, 0, "MatchToolValue", "MIXED", "three", 0, 0);
    scall(MATCHTOOLVALUE, 0, "MatchToolValue", "nothere", "x", 0, 0);
    /* the bracket check flag of the call without icon is still set */
    scall(FINDTOOLTYPE, 0, "FindToolType", "FLAG", NULL, 0, 0);
    scall(FINDTOOLTYPE, 0, "FindToolType", "BRACK", NULL, 0, 0);
    scall(FINDTOOLTYPE, 0, "FindToolType", "BRACK", NULL, 0, 0);
    scall(FINDTOOLTYPE, 1, "FindToolType", "brack", NULL, 1, 1);
    scall(FINDTOOLTYPE, 1, "FindToolType", "(BRACK", NULL, 1, 1);
    scall(FINDTOOLTYPE, 0, "FindToolType", "OFF", NULL, 0, 0);
    scall(FINDTOOLTYPE, 0, "FindToolType", "nothere", NULL, 0, 0);
    scall(FINDTOOLTYPE, 1, "FindToolType", "FOO", NULL, 1, 0);

    scall(SETTOOLVALUE, 0, "SetToolValue", "foo", "baz", 0, 0);
    scall(SETTOOLVALUE, 1, "SetToolValue", "FL", "x", 1, 0);
    scall(SETTOOLVALUE, 1, "SetToolValue", "OFF", "2", 1, (ULONG)-1);
    scall(SETTOOLVALUE, 1, "SetToolValue", "lower", "", 1, 7);
    scall(SETTOOLVALUE, 0, "SetToolValue", "nothere", "1", 0, 0);
    scall(NEWTOOLTYPE, 0, "NewToolType", "NEW", "val", 0, 0);
    scall(NEWTOOLTYPE, 0, "NewToolType", "ALONE", "", 0, 0);
    scall(NEWTOOLTYPE, 1, "NewToolType", "OFFT", "v", 1, 0);
    for (i = 0; i < 12; i++)
        ncall(FINDTOOLNUMBER, 0, "FindToolNumber", i, 0);
    scall(FINDTOOLVALUE, 0, "FindToolValue", "FOO", NULL, 0, 0);
    scall(FINDTOOLVALUE, 0, "FindToolValue", "OFFT", NULL, 0, 0);

    ncall(SETICONHIT, 0, "SetIconHit", 0xA5A50040UL, 0xA5A50020UL);
    ncall(ICONRENDER, 0, "IconRender", 5, 0);
    for (i = 0; i <= 6; i++)
        ncall(SETICONTYPE, 0, "SetIconType", i, 0);
    ncall(SETICONTYPE, 0, "SetIconType", 0x103, 0);
    ncall(SETICONTYPE, 0, "SetIconType", 3, 0);
    scall(ICONDEFTOOL, 0, "IconDefTool", "SYS:Utilities/MultiView", NULL, 0, 0);
    scall(ICONDEFTOOL, 0, "IconDefTool", "", NULL, 0, 0);

    /* shapes: 0 (20x10x2) and 1 (25x7x1) in use, 2-7 fresh, the decoy in
       use (a3 of a fresh IconToShape) */
    shapecall(SHAPETOICON, 0, "ShapeToIcon", 1, 0, NULL);
    shapecall(SHAPETOICON, 1, "ShapeToIcon", 0, 1, NULL);
    shapecall(ICONTOSHAPE, 1, "IconToShape (fresh, a3 = decoy)", 2, 3, decoy);
    shapecall(ICONTOSHAPE, 0, "IconToShape (in use)", 2, 0, NULL);
    shapecall(ICONTOSHAPE, 1, "IconToShape (both in use)", 2, 3, NULL);
    shapecall(ICONTOSHAPE, 0, "IconToShape (fresh, a3 = decoy)", 4, 0, decoy);
    shapecall(ICONTOSHAPE, 1, "IconToShape (in use, fresh)", 4, 5, NULL);

    puticon("RAM:rwett1", 0, 0);
    same_icon();
    readback("PutIconObject RAM:rwett1", "RAM:rwett1");
    puticon("RAM:rwett1", 0, 0);                /* no icon: 0 */
    freeicon();                                 /* no icon: 0 */

    /* the written icons again: PutIconObject with a type, ClearToolTypes */
    ROUT = "GetIconObject";
    strcpy(VEC, "RAM:rwett1");
    fillarea();
    DO1(GETICON, 0, strargs("RAM:rwett1", NULL));
    dobj = (struct DiskObject *)R.d[0];
    same_icon();
    for (i = 0; i < 12; i++)
        ncall(FINDTOOLNUMBER, 0, "FindToolNumber (written icon)", i, 0);
    puticon("RAM:rwett2", 1, 4);
    readback("PutIconObject RAM:rwett2,4", "RAM:rwett2");
    ROUT = "GetIconObject";
    strcpy(VEC, "RAM:rwett2");
    DO1(GETICON, 0, strargs("RAM:rwett2", NULL));
    dobj = (struct DiskObject *)R.d[0];
    same_icon();
    ncall(CLEARTOOLTYPES, 0, "ClearToolTypes", 0, 0);
    ncall(FINDTOOLNUMBER, 0, "FindToolNumber (cleared)", 0, 0);
    scall(NEWTOOLTYPE, 0, "NewToolType (cleared)", "ONLY", "one", 0, 0);
    ncall(FINDTOOLNUMBER, 0, "FindToolNumber", 0, 0);
    freeicon();
    freeicon();

    geticon("RAM:rwett_nothere");               /* icon.library's default or 0 */
    if (dobj)
        freeicon();

    /* finit with an icon open, init again */
    geticon("RAM:rwett");
    ROUT = "finit";
    strcpy(VEC, "icon open");
    DO1(FINIT, 0, (void)0);
    dobj = NULL;
    ROUT = "init";
    strcpy(VEC, "");
    DO1(INIT, 0, (void)0);
    geticon("RAM:rwett");
    freeicon();
    ROUT = "finit";
    DO1(FINIT, 0, (void)0);

    ROUT = "memlib finit";
    {
        struct regs r;
        markregs(&r);
        callregs(nullsub("memlib", 28), &r, 0, 0);
    }
    DeleteDiskObject((STRPTR)"RAM:rwett");
    DeleteDiskObject((STRPTR)"RAM:rwett1");
    DeleteDiskObject((STRPTR)"RAM:rwett2");
    CloseLibrary(TTIconBase);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
