/*
 * rwe ridatatypeslib - the DataTypes library: the C build against the
 * reference build of rwelibsrc/datatypeslib6.bb (ridatatypeslib.orig,
 * ref/ridatatypeslib.obj of ctest rwe_refbuild), method of rwecmp.h.
 *
 *   ridatatypeslib DIR   (DIR with ridatatypeslib.obj/.orig)
 *
 * The test writes a small ILBM picture (16x8, 2 planes, 4 colours) and a
 * text file to RAM: and lets both sets work on them through the system's
 * datatypes.library (ilbm.datatype, ascii.datatype), each set with its
 * own string area (a3), window (opened on the Workbench screen, the
 * windowslib object in a0), palette object, tag lists and argument stack.
 * memlib is replaced by stubs (sysload_stubs: AllocMem/FreeMem, sizes and
 * flags logged).  Compared: d0 (word results: the low word; addresses:
 * not 0, or the library's buffer), a3 and the string area, the
 * BitMapHeader, the library's tag buffer after every command that fills
 * it, the values GetDataTypeAttr returns, the palette and tag list a
 * command writes, the memlib calls and every register the original keeps.
 * DataTypeReady is waited for on each set (the layout runs in its own
 * process: not compared call by call).
 *
 * Covered: every command and form before DataTypeOpen (nothing, 0);
 * DataTypeCreate without a file (BUG(orig) (#601): no check, name ""); a
 * missing file; the text file (name, base name, ID, group, a second
 * DataTypeOpen: 0); the picture: the strings, DataTypeCreate both forms,
 * DataTypeSuper, DataTypeBitmap, Width/Height/Depth, DataTypeModeID both
 * forms (BUG(orig) (#603): the tags written to the caller's list),
 * GetDataTypeAttr (known and unknown attributes), SetDataTypeAttr with a
 * tag list and with 1 and 2 pairs (the upper word of d7 a marker),
 * DataTypesPalette with 4 and 2 colours, AttachDataType in all three
 * forms (BUG(orig) (#597, #596): the words of the second as longs - x and y with upper
 * words 1 and 2; the id 0 as tag list of the third), a second AttachDataType (0),
 * DataTypeReady, DetachDataType (twice), DataTypeClose (then the strings
 * and sizes give 0), finit with an attached object (detached, closed, the
 * file unlocked), init again.
 * Not covered: a library missing at init (finit closes only up to it),
 * ObtainDataType failing (BUG(orig) (#605): reads the header from 0),
 * AttachDataType window,id without DataTypeBitmap (reads at 0),
 * DataTypeClose of an attached object, DataTypesPalette with more colours
 * than the picture or 0 (65536), SetDataTypeAttr with 0 or more than 127
 * pairs, names over 383 characters.
 */
#include "rwecmp.h"
#define __NOLIBBASE__
#define INTUITION_BASE_NAME TIntui
#include <proto/intuition.h>
#undef __NOLIBBASE__
#include <intuition/intuition.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <intuition/gadgetclass.h>

static struct Library *TIntui;

#define LIB "ridatatypeslib"

enum { E_OPEN, E_CLOSE, E_NAME, E_BASENAME, E_ID, E_CREATE, E_ATTACH, E_DETACH, E_SUPER,
       E_BITMAP, E_PALETTE,
       E_WIDTH, E_HEIGHT, E_DEPTH, E_READY, E_NULLSUB, E_SETATTR, E_GETATTR, E_MODEID, E_GROUP,
       E_FINIT, E_INIT };

/* word argument with a marker in the upper word */
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* ------------------------------------------------------------ memlib */

static struct { ULONG op, size, flags; } mlog[2][8];
static int nlog[2];

static ULONG __attribute__((used)) st_globalloc_c(ULONG size, ULONG flags)
{
    ULONG *p;

    if (nlog[cur] < 8) {
        mlog[cur][nlog[cur]].op = 1;
        mlog[cur][nlog[cur]].size = size;
        mlog[cur][nlog[cur]].flags = flags;
    }
    nlog[cur]++;
    p = AllocMem(size + 8, flags);
    if (!p)
        return 0;
    p[0] = size + 8;
    return (ULONG)(p + 2);
}

static void __attribute__((used)) st_globfree_c(ULONG mem, ULONG size)
{
    if (nlog[cur] < 8) {
        mlog[cur][nlog[cur]].op = 2;
        mlog[cur][nlog[cur]].size = size;
        mlog[cur][nlog[cur]].flags = mem != 0;
    }
    nlog[cur]++;
    if (mem)
        FreeMem((ULONG *)mem - 2, ((ULONG *)mem)[-2]);
}

/* memlib's register interface: d0 (d1, a1) in, d0 out, all others kept */
void st_globalloc(void), st_globfree(void);
__asm__(
"   .text\n"
"   .even\n"
"_st_globalloc:\n"
"   movem.l d1-d7/a0-a6,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _st_globalloc_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d1-d7/a0-a6\n"
"   rts\n"
"_st_globfree:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _st_globfree_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rts\n");

static const struct stub stubs[] = {
    { 0xc002, (APTR)st_globalloc },
    { 0xc003, (APTR)st_globfree },
    { 0, NULL }
};

static void same_mlog(void)
{
    int i;
    char buf[30];

    SAME("memlib calls", nlog[cur]);
    for (i = 0; i < nlog[0] && i < nlog[1] && i < 8; i++) {
        sprintf(buf, "memlib call %d", i);
        SAME(buf, mlog[cur][i].op);
        SAME(buf, mlog[cur][i].size);
        SAME(buf, mlog[cur][i].flags);
    }
}

/* ------------------------------------------------------------ files */

static const UBYTE ilbm[] = {
    'F', 'O', 'R', 'M', 0, 0, 0, 92, 'I', 'L', 'B', 'M',
    'B', 'M', 'H', 'D', 0, 0, 0, 20,
    0, 16, 0, 8, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 10, 11, 1, 64, 0, 200,
    'C', 'M', 'A', 'P', 0, 0, 0, 12,
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    'B', 'O', 'D', 'Y', 0, 0, 0, 32,
    0xf0, 0x0f, 0xff, 0x00, 0x0f, 0xf0, 0x00, 0xff, 0xaa, 0x55, 0xcc, 0x33,
    0x55, 0xaa, 0x33, 0xcc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,
};
static const char text[] = "Some text for the ascii datatype.\nA second line.\n";

static BOOL writefile(const char *name, const void *data, LONG len)
{
    BPTR fh = Open((STRPTR)name, MODE_NEWFILE);
    LONG n;

    if (!fh)
        return FALSE;
    n = Write(fh, (APTR)data, len);
    Close(fh);
    return n == len;
}

/* ------------------------------------------------------------ helpers */

#define AREA 256
static UBYTE area[2][AREA];             /* the string area (a3) */
static char nbuf[2][64];                /* file names */
static struct Window *win[2];
static ULONG winobj[2][2];              /* windowslib object: the Window */
static UBYTE paldata[2][4 + 12 * 4];
static ULONG palobj[2][2];              /* palettelib object: the data */
static ULONG tags[2][16];               /* tag lists of the test */
static ULONG stk[2][8];                 /* the argument stack (a2) */
static ULONG *store[2];                 /* the library's tag buffer */

static void fillarea(void)
{
    int s, i;
    for (s = 0; s < 2; s++)
        for (i = 0; i < AREA; i++)
            area[s][i] = (UBYTE)(0xA0 ^ i);
}

/* a call without arguments, d0 compared (mask: the bits that count) */
static void call0(const char *rout, int idx, int form, ULONG mask)
{
    ROUT = rout;
    DO2(ENT(LIB, idx, form), (void)0);
    SAME("d0", R[cur].d[0] & mask);
    same_mlog();
}

/* a string function: a3 = the area, d0, a3 and the area compared */
static void callstr(const char *rout, int idx)
{
    fillarea();
    ROUT = rout;
    DO2(ENT(LIB, idx, 0), R[cur].a[3] = (ULONG)area[cur]);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)area[cur]);
    SAMEBYTES("string area", area, AREA);
}

static void strings(void)
{
    callstr("DataTypeName", E_NAME);
    callstr("DataTypeBaseName", E_BASENAME);
    callstr("DataTypeID", E_ID);
    callstr("DataTypeGroup", E_GROUP);
}

/* the area begins with the string s (and its 0) in both sets */
static void areais(const char *s)
{
    BOTH(s, memcmp(area[cur], s, strlen(s) + 1) == 0);
}

/* DataTypeOpen name: `ok` = a DataType expected */
static void open(const char *name, BOOL ok)
{
    int s;

    for (s = 0; s < 2; s++)
        strcpy(nbuf[s], name);
    ROUT = "DataTypeOpen";
    strcpy(VEC, name);
    DO2(ENT(LIB, E_OPEN, 0), R[cur].d[0] = (ULONG)nbuf[cur]);
    BOTH(ok ? "d0 not 0" : "d0 0", (R[cur].d[0] != 0) == ok);
    BOTH("d0 a DataType",
         !R[cur].d[0] || ((struct DataType *)R[cur].d[0])->dtn_Header != NULL);
    strcpy(VEC, "");
}

/* the library's tag buffer: n longs the same */
static void same_store(const char *what, int n)
{
    ULONG *x[2] = { store[0], store[1] };

    if (!store[0] || !store[1]) {
        fail("no tag buffer", 0, 0);
        return;
    }
    SAMEBYTES(what, x, (ULONG)n * 4);
}

/* GetDataTypeAttr(attr): the address of the buffer and the value */
static void getattr(ULONG attr)
{
    ROUT = "GetDataTypeAttr";
    sprintf(VEC, "attr $%08lX", attr);
    DO2(ENT(LIB, E_GETATTR, 0), R[cur].d[0] = attr);
    BOTH("d0 not 0", R[cur].d[0] != 0);
    for (cur = 0; cur < 2; cur++)
        store[cur] = (ULONG *)R[cur].d[0];
    SAME("value", store[cur] ? store[cur][0] : 0xdeadbeef);
    strcpy(VEC, "");
}

/* the bitmap header the library has, the same in both sets */
static void same_bmh(void)
{
    UBYTE *x[2];

    ROUT = "DataTypeBitmap";
    DO2(ENT(LIB, E_BITMAP, 0), (void)0);
    BOTH("d0 not 0", R[cur].d[0] != 0);
    if (!R[0].d[0] || !R[1].d[0])
        return;
    x[0] = (UBYTE *)R[0].d[0];
    x[1] = (UBYTE *)R[1].d[0];
    SAMEBYTES("BitMapHeader", x, sizeof(struct BitMapHeader));
}

/* wait for the layout of each set (DataTypeReady) */
static void ready(void)
{
    int i;
    ULONG ok[2];

    ROUT = "DataTypeReady";
    for (cur = 0; cur < 2; cur++) {
        ok[cur] = 0;
        for (i = 0; i < 100 && !ok[cur]; i++) {
            struct regs r;
            ULONG popped;

            markregs(&r);
            callstk(ENT(LIB, E_READY, 0), &r, NULL, 0, &popped);
            ok[cur] = (r.d[0] & 0xffff) == 0xffff;
            if (!ok[cur])
                Delay(2);
        }
    }
    BOTH("layout done", ok[cur]);
    /* once more, compared */
    call0("DataTypeReady", E_READY, 0, 0xffff);
}

static void detach(void)
{
    call0("DetachDataType", E_DETACH, 0, 0);
}

/* the commands that do nothing without a file or object */
static void nothing(void)
{
    int f;

    strings();
    callstr("DataTypeSuper", E_SUPER);
    call0("DataTypeBitmap", E_BITMAP, 0, ~0UL);
    call0("DataTypeWidth", E_WIDTH, 0, 0xffff);
    call0("DataTypeHeight", E_HEIGHT, 0, 0xffff);
    call0("DataTypeDepth", E_DEPTH, 0, 0xffff);
    call0("DataTypeReady", E_READY, 0, 0xffff);
    call0("DataTypeModeID", E_MODEID, 0, ~0UL);
    ROUT = "DataTypeModeID tags";
    DO2(ENT(LIB, E_MODEID, 1), R[cur].d[0] = (ULONG)tags[cur]);
    SAME("d0", R[cur].d[0]);
    call0("GetDataTypeAttr", E_GETATTR, 0, ~0UL);
    ROUT = "DataTypeCreate tags";
    DO2(ENT(LIB, E_CREATE, 1), R[cur].d[0] = (ULONG)tags[cur]);
    SAME("d0", R[cur].d[0]);
    ROUT = "DataTypesPalette";
    DO2(ENT(LIB, E_PALETTE, 0), R[cur].d[0] = WA(1); R[cur].a[0] = (ULONG)palobj[cur]);
    SAME("d0", R[cur].d[0]);
    ROUT = "SetDataTypeAttr";
    DO2(ENT(LIB, E_SETATTR, 0), R[cur].d[0] = (ULONG)tags[cur]);
    for (f = 0; f < 3; f++) {
        ROUT = "AttachDataType";
        sprintf(VEC, "form %d", f);
        DO2(ENT(LIB, E_ATTACH, f), R[cur].d[0] = WA(0); R[cur].d[1] = 7;
            R[cur].a[0] = (ULONG)winobj[cur]);
        SAME("d0", R[cur].d[0] & 0xffff);
    }
    strcpy(VEC, "");
    detach();
    call0("DataTypeClose", E_CLOSE, 0, 0);
}

/* the tag list of the test in both sets */
static void settags(const ULONG *t, int n)
{
    int s;
    for (s = 0; s < 2; s++)
        memcpy(tags[s], t, (ULONG)n * 4);
}

static void palette(UWORD n)
{
    int s, i;
    UBYTE *x[2] = { paldata[0], paldata[1] };

    for (s = 0; s < 2; s++) {
        for (i = 0; i < (int)sizeof paldata[0]; i++)
            paldata[s][i] = (UBYTE)(0x11 * i + 3);
        *(UWORD *)paldata[s] = n;
        *(UWORD *)(paldata[s] + 2) = 0;
    }
    ROUT = "DataTypesPalette";
    sprintf(VEC, "%u colours", n);
    DO2(ENT(LIB, E_PALETTE, 0), R[cur].d[0] = WA(1); R[cur].a[0] = (ULONG)palobj[cur]);
    BOTH("d0 = the buffer", R[cur].d[0] == (ULONG)store[cur]);
    SAMEBYTES("palette", x, sizeof paldata[0]);
    BOTH("colour 1 red", paldata[cur][4 + 12] == 0xff && paldata[cur][4 + 12 + 4] == 0);
    strcpy(VEC, "");
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    ULONG t[16];
    int s;
    ULONG *x[2];

    if (argc < 2) {
        printf("usage: ridatatypeslib DIR\n");
        return 20;
    }
    TIntui = OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    if (!TIntui)
        return 20;
    if (!writefile("RAM:rwedt.iff", ilbm, sizeof ilbm)
        || !writefile("RAM:rwedt.txt", text, sizeof text - 1)) {
        check("ridatatypeslib: write the files to RAM:", FALSE);
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_sets(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    for (s = 0; s < 2; s++) {
        struct TagItem wt[] = {
            { WA_Left, 20 + 220 * s }, { WA_Top, 20 }, { WA_Width, 200 }, { WA_Height, 80 },
            { WA_Title, (ULONG)(s ? "rwedt C" : "rwedt orig") }, { WA_IDCMP, 0 },
            { TAG_DONE, 0 }
        };
        win[s] = OpenWindowTagList(NULL, wt);
        if (!win[s]) {
            check("ridatatypeslib: open the windows", FALSE);
            return 20;
        }
        winobj[s][0] = (ULONG)win[s];
        palobj[s][0] = (ULONG)paldata[s];
    }
    DONTCARE = 1;                       /* d0: the result, compared by the test */
    strcpy(VEC, "");

    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    same_mlog();

    /* nothing open */
    settags((const ULONG[]){ TAG_DONE }, 1);
    nothing();
    /* DataTypeCreate without a file: NewDTObject("") */
    call0("DataTypeCreate", E_CREATE, 0, 0);
    SAME("d0 not 0", R[cur].d[0] != 0);
    call0("DataTypeClose", E_CLOSE, 0, 0);

    /* a missing file, the text file */
    open("RAM:rwedt.missing", FALSE);
    strings();
    open("RAM:rwedt.txt", TRUE);
    strings();
    open("RAM:rwedt.iff", FALSE);       /* one is open: 0 */
    strings();
    call0("DataTypeClose", E_CLOSE, 0, 0);
    nothing();

    /* the picture */
    open("RAM:rwedt.iff", TRUE);
    callstr("DataTypeName", E_NAME);
    areais("ILBM");
    callstr("DataTypeBaseName", E_BASENAME);
    areais("ilbm");
    callstr("DataTypeID", E_ID);
    BOTH("ID", R[cur].d[0] == 4 && memcmp(area[cur], "ilbm", 4) == 0);
    callstr("DataTypeGroup", E_GROUP);
    BOTH("group", R[cur].d[0] == 4 && memcmp(area[cur], "pict", 4) == 0);
    call0("DataTypeCreate", E_CREATE, 0, 0);
    BOTH("d0 not 0", R[cur].d[0] != 0);
    callstr("DataTypeSuper", E_SUPER);
    areais("ilbm.datatype");
    getattr(DTA_NominalHoriz);
    getattr(DTA_NominalVert);
    getattr(PDTA_NumColors);
    getattr(TAG_USER + 0x7777);         /* unknown: the buffer as it was */
    same_bmh();
    call0("DataTypeWidth", E_WIDTH, 0, 0xffff);
    BOTH("width 16", (R[cur].d[0] & 0xffff) == 16);
    call0("DataTypeHeight", E_HEIGHT, 0, 0xffff);
    BOTH("height 8", (R[cur].d[0] & 0xffff) == 8);
    call0("DataTypeDepth", E_DEPTH, 0, 0xffff);
    BOTH("depth 2", (R[cur].d[0] & 0xffff) == 2);
    call0("DataTypeModeID", E_MODEID, 0, ~0UL);
    same_store("tags of DataTypeModeID", 7);
    settags((const ULONG[]){ 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 9);
    ROUT = "DataTypeModeID tags";
    DO2(ENT(LIB, E_MODEID, 1), R[cur].d[0] = (ULONG)tags[cur]);
    SAME("d0", R[cur].d[0]);
    x[0] = tags[0];
    x[1] = tags[1];
    SAMEBYTES("tag list", x, 9 * 4);
    palette(4);
    palette(2);
    getattr(DTA_NominalHoriz);

    /* SetDataTypeAttr: a tag list, pairs */
    settags((const ULONG[]){ DTA_TopVert, 1, DTA_TopHoriz, 1, TAG_DONE }, 5);
    ROUT = "SetDataTypeAttr";
    DO2(ENT(LIB, E_SETATTR, 0), R[cur].d[0] = (ULONG)tags[cur]);
    getattr(DTA_TopVert);
    getattr(DTA_TopHoriz);
    for (s = 0; s < 2; s++) {
        stk[s][3] = DTA_TopVert;        /* the first pair: the last pushed */
        stk[s][2] = 2;
        stk[s][1] = DTA_TopHoriz;
        stk[s][0] = 3;
    }
    ROUT = "SetDataTypeAttr pairs";
    DO2(ENT(LIB, E_SETATTR, 1), R[cur].d[7] = WA(1); R[cur].a[2] = (ULONG)&stk[cur][4]);
    same_store("tags of SetDataTypeAttr 1 pair", 3);
    DO2(ENT(LIB, E_SETATTR, 1), R[cur].d[7] = WA(2); R[cur].a[2] = (ULONG)&stk[cur][4]);
    same_store("tags of SetDataTypeAttr 2 pairs", 5);
    getattr(DTA_TopVert);
    getattr(DTA_TopHoriz);
    settags((const ULONG[]){ DTA_TopVert, 0, DTA_TopHoriz, 0, TAG_DONE }, 5);
    ROUT = "SetDataTypeAttr";
    DO2(ENT(LIB, E_SETATTR, 0), R[cur].d[0] = (ULONG)tags[cur]);
    getattr(DTA_TopVert);

    /* AttachDataType window,id */
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id");
    DO2(ENT(LIB, E_ATTACH, 0), R[cur].d[0] = WA(0); R[cur].d[1] = 7;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("d0", R[cur].d[0] & 0xffff);
    same_store("tags of AttachDataType", 15);
    DO2(ENT(LIB, E_ATTACH, 0), R[cur].d[0] = WA(0); R[cur].d[1] = 8;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("d0 attached", R[cur].d[0] & 0xffff);
    strcpy(VEC, "");
    ready();
    detach();
    detach();
    call0("DataTypeReady", E_READY, 0, 0xffff);
    /* AttachDataType window,id,x,y,width,height: the upper words */
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id,x,y,w,h");
    DO2(ENT(LIB, E_ATTACH, 1), R[cur].d[0] = WA(0); R[cur].d[1] = 9;
        R[cur].d[2] = 0x10004; R[cur].d[3] = 0x20006; R[cur].d[4] = 12; R[cur].d[5] = 5;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("d0", R[cur].d[0] & 0xffff);
    same_store("tags of AttachDataType", 15);
    strcpy(VEC, "");
    /* (the position outside the window, through the upper words) */
    ready();
    detach();
    /* AttachDataType window,id,&taglist: the id (0) as tag list */
    settags((const ULONG[]){ GA_Left, 2, TAG_DONE }, 3);
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id,&taglist");
    DO2(ENT(LIB, E_ATTACH, 2), R[cur].d[0] = WA(0); R[cur].d[1] = 0;
        R[cur].d[2] = (ULONG)tags[cur]; R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("d0", R[cur].d[0] & 0xffff);
    same_store("tags of AttachDataType", 7);
    strcpy(VEC, "");
    ready();
    detach();
    call0("DataTypeClose", E_CLOSE, 0, 0);
    strings();
    call0("DataTypeWidth", E_WIDTH, 0, 0xffff);
    call0("DataTypeHeight", E_HEIGHT, 0, 0xffff);
    call0("GetDataTypeAttr", E_GETATTR, 0, ~0UL);

    /* DataTypeCreate with a tag list, attached at finit */
    open("RAM:rwedt.iff", TRUE);
    t[0] = PDTA_Remap;
    t[1] = FALSE;
    t[2] = TAG_DONE;
    settags(t, 3);
    ROUT = "DataTypeCreate tags";
    DO2(ENT(LIB, E_CREATE, 1), R[cur].d[0] = (ULONG)tags[cur]);
    SAME("d0 not 0", R[cur].d[0] != 0);
    same_bmh();
    ROUT = "AttachDataType";
    DO2(ENT(LIB, E_ATTACH, 0), R[cur].d[0] = WA(0); R[cur].d[1] = 7;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("d0", R[cur].d[0] & 0xffff);
    ready();
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    same_mlog();
    {
        BPTR l = Lock((STRPTR)"RAM:rwedt.iff", EXCLUSIVE_LOCK);
        check("ridatatypeslib: the file unlocked by finit", l != 0);
        if (l)
            UnLock(l);
    }
    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    same_mlog();
    callstr("DataTypeName", E_NAME);
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    same_mlog();

    for (s = 0; s < 2; s++)
        CloseWindow(win[s]);
    DeleteFile((STRPTR)"RAM:rwedt.iff");
    DeleteFile((STRPTR)"RAM:rwedt.txt");
    CloseLibrary(TIntui);
    return rwe_done("ridatatypeslib");
}
