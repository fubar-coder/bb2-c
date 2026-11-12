/*
 * rwe ridatatypeslib - the DataTypes library: the C build or the
 * reference build of rwelibsrc/datatypeslib6.bb (ridatatypeslib.orig,
 * ref/ridatatypeslib.obj of ctest rwe_refbuild), one set as LIBSET says,
 * observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_ridatatypeslib.txt).
 *
 *   ridatatypeslib DIR   (DIR with ridatatypeslib.obj/.orig)
 *
 * The test writes a small ILBM picture (16x8, 2 planes, 4 colours) and a
 * text file to RAM: and lets the library work on them through the
 * system's datatypes.library (ilbm.datatype, ascii.datatype), with a
 * string area (a3), a window (opened on the Workbench screen, the
 * windowslib object in a0), a palette object, tag lists and an argument
 * stack.  memlib is replaced by stubs (sysload_stubs: AllocMem/FreeMem,
 * sizes and flags logged).  Observed: d0 (word results: the low word; addresses:
 * not 0, or the library's buffer), a3 and the string area, the
 * BitMapHeader, the library's tag buffer after every command that fills
 * it, the values GetDataTypeAttr returns, the palette and tag list a
 * command writes, the memlib calls and the kept registers.
 * DataTypeReady is waited for (the layout runs in its own process: not
 * observed call by call).
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

static struct { ULONG op, size, flags; } mlog[8];
static int nlog;

static ULONG __attribute__((used)) st_globalloc_c(ULONG size, ULONG flags)
{
    ULONG *p;

    if (nlog < 8) {
        mlog[nlog].op = 1;
        mlog[nlog].size = size;
        mlog[nlog].flags = flags;
    }
    nlog++;
    p = AllocMem(size + 8, flags);
    if (!p)
        return 0;
    p[0] = size + 8;
    return (ULONG)(p + 2);
}

static void __attribute__((used)) st_globfree_c(ULONG mem, ULONG size)
{
    if (nlog < 8) {
        mlog[nlog].op = 2;
        mlog[nlog].size = size;
        mlog[nlog].flags = mem != 0;
    }
    nlog++;
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

    OBS("memlib calls", nlog);
    for (i = 0; i < nlog && i < 8; i++) {
        sprintf(buf, "memlib call %d", i);
        OBS(buf, mlog[i].op);
        OBS(buf, mlog[i].size);
        OBS(buf, mlog[i].flags);
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
static UBYTE area[AREA];                /* the string area (a3) */
static char nbuf[64];                   /* file names */
static struct Window *win;
static ULONG winobj[2];                 /* windowslib object: the Window */
static UBYTE paldata[4 + 12 * 4];
static ULONG palobj[2];                 /* palettelib object: the data */
static ULONG tags[16];                  /* tag lists of the test */
static ULONG stk[8];                    /* the argument stack (a2) */
static ULONG *store;                    /* the library's tag buffer */

static void fillarea(void)
{
    int i;
    for (i = 0; i < AREA; i++)
        area[i] = (UBYTE)(0xA0 ^ i);
}

/* a call without arguments, d0 compared (mask: the bits that count) */
static void call0(const char *rout, int idx, int form, ULONG mask)
{
    ROUT = rout;
    DO1(ENT(LIB, idx, form), (void)0);
    OBS("d0", R.d[0] & mask);
    same_mlog();
}

/* a string function: a3 = the area, d0, a3 and the area compared */
static void callstr(const char *rout, int idx)
{
    fillarea();
    ROUT = rout;
    DO1(ENT(LIB, idx, 0), R.a[3] = (ULONG)area);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)area);
    OBSBYTES("string area", area, AREA);
}

static void strings(void)
{
    callstr("DataTypeName", E_NAME);
    callstr("DataTypeBaseName", E_BASENAME);
    callstr("DataTypeID", E_ID);
    callstr("DataTypeGroup", E_GROUP);
}

/* the area begins with the string s (and its 0) */
static void areais(const char *s)
{
    BOTH(s, memcmp(area, s, strlen(s) + 1) == 0);
}

/* DataTypeOpen name: `ok` = a DataType expected */
static void open(const char *name, BOOL ok)
{
    strcpy(nbuf, name);
    ROUT = "DataTypeOpen";
    strcpy(VEC, name);
    DO1(ENT(LIB, E_OPEN, 0), R.d[0] = (ULONG)nbuf);
    BOTH(ok ? "d0 not 0" : "d0 0", (R.d[0] != 0) == ok);
    BOTH("d0 a DataType",
         !R.d[0] || ((struct DataType *)R.d[0])->dtn_Header != NULL);
    strcpy(VEC, "");
}

/* the library's tag buffer: n longs */
static void same_store(const char *what, int n)
{
    if (!store) {
        check("no tag buffer", FALSE);
        return;
    }
    OBSBYTES(what, store, (ULONG)n * 4);
}

/* GetDataTypeAttr(attr): the address of the buffer and the value */
static void getattr(ULONG attr)
{
    ROUT = "GetDataTypeAttr";
    sprintf(VEC, "attr $%08lX", attr);
    DO1(ENT(LIB, E_GETATTR, 0), R.d[0] = attr);
    BOTH("d0 not 0", R.d[0] != 0);
    store = (ULONG *)R.d[0];
    OBS("value", store ? store[0] : 0xdeadbeef);
    strcpy(VEC, "");
}

/* the bitmap header the library has */
static void same_bmh(void)
{
    ROUT = "DataTypeBitmap";
    DO1(ENT(LIB, E_BITMAP, 0), (void)0);
    BOTH("d0 not 0", R.d[0] != 0);
    if (!R.d[0])
        return;
    OBSBYTES("BitMapHeader", (UBYTE *)R.d[0], sizeof(struct BitMapHeader));
}

/* wait for the layout (DataTypeReady) */
static void ready(void)
{
    int i;
    ULONG ok = 0;

    ROUT = "DataTypeReady";
    for (i = 0; i < 100 && !ok; i++) {
        struct regs r;
        ULONG popped;

        markregs(&r);
        callstk(ENT(LIB, E_READY, 0), &r, NULL, 0, &popped);
        ok = (r.d[0] & 0xffff) == 0xffff;
        if (!ok)
            Delay(2);
    }
    BOTH("layout done", ok);
    /* once more, observed */
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
    DO1(ENT(LIB, E_MODEID, 1), R.d[0] = (ULONG)tags);
    OBS("d0", R.d[0]);
    call0("GetDataTypeAttr", E_GETATTR, 0, ~0UL);
    ROUT = "DataTypeCreate tags";
    DO1(ENT(LIB, E_CREATE, 1), R.d[0] = (ULONG)tags);
    OBS("d0", R.d[0]);
    ROUT = "DataTypesPalette";
    DO1(ENT(LIB, E_PALETTE, 0), R.d[0] = WA(1); R.a[0] = (ULONG)palobj);
    OBS("d0", R.d[0]);
    ROUT = "SetDataTypeAttr";
    DO1(ENT(LIB, E_SETATTR, 0), R.d[0] = (ULONG)tags);
    for (f = 0; f < 3; f++) {
        ROUT = "AttachDataType";
        sprintf(VEC, "form %d", f);
        DO1(ENT(LIB, E_ATTACH, f), R.d[0] = WA(0); R.d[1] = 7;
            R.a[0] = (ULONG)winobj);
        OBS("d0", R.d[0] & 0xffff);
    }
    strcpy(VEC, "");
    detach();
    call0("DataTypeClose", E_CLOSE, 0, 0);
}

/* the tag list of the test */
static void settags(const ULONG *t, int n)
{
    memcpy(tags, t, (ULONG)n * 4);
}

static void palette(UWORD n)
{
    int i;

    for (i = 0; i < (int)sizeof paldata; i++)
        paldata[i] = (UBYTE)(0x11 * i + 3);
    *(UWORD *)paldata = n;
    *(UWORD *)(paldata + 2) = 0;
    ROUT = "DataTypesPalette";
    sprintf(VEC, "%u colours", n);
    DO1(ENT(LIB, E_PALETTE, 0), R.d[0] = WA(1); R.a[0] = (ULONG)palobj);
    BOTH("d0 = the buffer", R.d[0] == (ULONG)store);
    OBSBYTES("palette", paldata, sizeof paldata);
    BOTH("colour 1 red", paldata[4 + 12] == 0xff && paldata[4 + 12 + 4] == 0);
    strcpy(VEC, "");
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    ULONG t[16];

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
    if (!load_set(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    KEEPS("init", d2-d7|a3);
    KEEPS("DataTypeName", d1-d7|a1|a2|a6);
    KEEPS("DataTypeBaseName", d1-d7|a1|a2|a6);
    KEEPS("DataTypeID", d2-d7|a1|a2|a6);
    KEEPS("DataTypeGroup", d2-d7|a1|a2|a6);
    KEEPS("DataTypeSuper", d1-d7|a1|a2|a6);
    KEEPS("DataTypeBitmap", d2-d7|a2|a3);
    KEEPS("DataTypeWidth", d1-d7|a1-a3|a6);
    KEEPS("DataTypeHeight", d1-d7|a1-a3|a6);
    KEEPS("DataTypeDepth", d1-d7|a1-a3|a6);
    KEEPS("DataTypeReady", d2-d7|a2|a3);
    KEEPS("DataTypeModeID", d2-d7|a2|a3);
    KEEPS("DataTypeModeID tags", d2-d7|a2|a3);
    KEEPS("GetDataTypeAttr", d2-d7|a2|a3);
    KEEPS("DataTypeCreate tags", d2-d7|a2|a3);
    KEEPS("DataTypesPalette", d2|d4-d7|a2|a3);
    KEEPS("SetDataTypeAttr", d2-d7|a2|a3);
    KEEPS("AttachDataType", d2-d7|a3);
    KEEPS("DetachDataType", d2-d7|a2|a3);
    KEEPS("DataTypeClose", d2-d7|a2|a3);
    KEEPS("DataTypeCreate", d2-d7|a2|a3);
    KEEPS("DataTypeOpen", d3-d7|a2|a3);
    KEEPS("SetDataTypeAttr pairs", d2-d6|a3);
    KEEPS("finit", d2-d7|a3);
    {
        struct TagItem wt[] = {
            { WA_Left, 20 }, { WA_Top, 20 }, { WA_Width, 200 }, { WA_Height, 80 },
            { WA_Title, (ULONG)"rwedt" }, { WA_IDCMP, 0 },
            { TAG_DONE, 0 }
        };
        win = OpenWindowTagList(NULL, wt);
        if (!win) {
            check("ridatatypeslib: open the window", FALSE);
            return 20;
        }
        winobj[0] = (ULONG)win;
        palobj[0] = (ULONG)paldata;
    }
    DONTCARE = 1;                       /* d0: the result, compared by the test */
    strcpy(VEC, "");

    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    same_mlog();

    /* nothing open */
    settags((const ULONG[]){ TAG_DONE }, 1);
    nothing();
    /* DataTypeCreate without a file: NewDTObject("") */
    call0("DataTypeCreate", E_CREATE, 0, 0);
    OBS("d0 not 0", R.d[0] != 0);
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
    BOTH("ID", R.d[0] == 4 && memcmp(area, "ilbm", 4) == 0);
    callstr("DataTypeGroup", E_GROUP);
    BOTH("group", R.d[0] == 4 && memcmp(area, "pict", 4) == 0);
    call0("DataTypeCreate", E_CREATE, 0, 0);
    BOTH("d0 not 0", R.d[0] != 0);
    callstr("DataTypeSuper", E_SUPER);
    areais("ilbm.datatype");
    getattr(DTA_NominalHoriz);
    getattr(DTA_NominalVert);
    getattr(PDTA_NumColors);
    getattr(TAG_USER + 0x7777);         /* unknown: the buffer as it was */
    same_bmh();
    call0("DataTypeWidth", E_WIDTH, 0, 0xffff);
    BOTH("width 16", (R.d[0] & 0xffff) == 16);
    call0("DataTypeHeight", E_HEIGHT, 0, 0xffff);
    BOTH("height 8", (R.d[0] & 0xffff) == 8);
    call0("DataTypeDepth", E_DEPTH, 0, 0xffff);
    BOTH("depth 2", (R.d[0] & 0xffff) == 2);
    call0("DataTypeModeID", E_MODEID, 0, ~0UL);
    same_store("tags of DataTypeModeID", 7);
    settags((const ULONG[]){ 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 9);
    ROUT = "DataTypeModeID tags";
    DO1(ENT(LIB, E_MODEID, 1), R.d[0] = (ULONG)tags);
    OBS("d0", R.d[0]);
    OBSBYTES("tag list", tags, 9 * 4);
    palette(4);
    palette(2);
    getattr(DTA_NominalHoriz);

    /* SetDataTypeAttr: a tag list, pairs */
    settags((const ULONG[]){ DTA_TopVert, 1, DTA_TopHoriz, 1, TAG_DONE }, 5);
    ROUT = "SetDataTypeAttr";
    DO1(ENT(LIB, E_SETATTR, 0), R.d[0] = (ULONG)tags);
    getattr(DTA_TopVert);
    getattr(DTA_TopHoriz);
    stk[3] = DTA_TopVert;               /* the first pair: the last pushed */
    stk[2] = 2;
    stk[1] = DTA_TopHoriz;
    stk[0] = 3;
    ROUT = "SetDataTypeAttr pairs";
    DO1(ENT(LIB, E_SETATTR, 1), R.d[7] = WA(1); R.a[2] = (ULONG)&stk[4]);
    same_store("tags of SetDataTypeAttr 1 pair", 3);
    DO1(ENT(LIB, E_SETATTR, 1), R.d[7] = WA(2); R.a[2] = (ULONG)&stk[4]);
    same_store("tags of SetDataTypeAttr 2 pairs", 5);
    getattr(DTA_TopVert);
    getattr(DTA_TopHoriz);
    settags((const ULONG[]){ DTA_TopVert, 0, DTA_TopHoriz, 0, TAG_DONE }, 5);
    ROUT = "SetDataTypeAttr";
    DO1(ENT(LIB, E_SETATTR, 0), R.d[0] = (ULONG)tags);
    getattr(DTA_TopVert);

    /* AttachDataType window,id */
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id");
    DO1(ENT(LIB, E_ATTACH, 0), R.d[0] = WA(0); R.d[1] = 7;
        R.a[0] = (ULONG)winobj);
    OBS("d0", R.d[0] & 0xffff);
    same_store("tags of AttachDataType", 15);
    DO1(ENT(LIB, E_ATTACH, 0), R.d[0] = WA(0); R.d[1] = 8;
        R.a[0] = (ULONG)winobj);
    OBS("d0 attached", R.d[0] & 0xffff);
    strcpy(VEC, "");
    ready();
    detach();
    detach();
    call0("DataTypeReady", E_READY, 0, 0xffff);
    /* AttachDataType window,id,x,y,width,height: the upper words */
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id,x,y,w,h");
    DO1(ENT(LIB, E_ATTACH, 1), R.d[0] = WA(0); R.d[1] = 9;
        R.d[2] = 0x10004; R.d[3] = 0x20006; R.d[4] = 12; R.d[5] = 5;
        R.a[0] = (ULONG)winobj);
    OBS("d0", R.d[0] & 0xffff);
    same_store("tags of AttachDataType", 15);
    strcpy(VEC, "");
    /* (the position outside the window, through the upper words) */
    ready();
    detach();
    /* AttachDataType window,id,&taglist: the id (0) as tag list */
    settags((const ULONG[]){ GA_Left, 2, TAG_DONE }, 3);
    ROUT = "AttachDataType";
    strcpy(VEC, "window,id,&taglist");
    DO1(ENT(LIB, E_ATTACH, 2), R.d[0] = WA(0); R.d[1] = 0;
        R.d[2] = (ULONG)tags; R.a[0] = (ULONG)winobj);
    OBS("d0", R.d[0] & 0xffff);
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
    DO1(ENT(LIB, E_CREATE, 1), R.d[0] = (ULONG)tags);
    OBS("d0 not 0", R.d[0] != 0);
    same_bmh();
    ROUT = "AttachDataType";
    DO1(ENT(LIB, E_ATTACH, 0), R.d[0] = WA(0); R.d[1] = 7;
        R.a[0] = (ULONG)winobj);
    OBS("d0", R.d[0] & 0xffff);
    ready();
    ROUT = "finit";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    same_mlog();
    {
        BPTR l = Lock((STRPTR)"RAM:rwedt.iff", EXCLUSIVE_LOCK);
        check("ridatatypeslib: the file unlocked by finit", l != 0);
        if (l)
            UnLock(l);
    }
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    same_mlog();
    callstr("DataTypeName", E_NAME);
    ROUT = "finit";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    same_mlog();

    CloseWindow(win);
    DeleteFile((STRPTR)"RAM:rwedt.iff");
    DeleteFile((STRPTR)"RAM:rwedt.txt");
    CloseLibrary(TIntui);
    return rwe_done("ridatatypeslib");
}
