/*
 * rwe rimapeditorlib - the C build of rimapeditorlib (userlib 8) against
 * the reference build of rwelibsrc/MapEd/MapEd_Bit2.bb (blitz2 2.10),
 * method of rwecmp.h.
 *
 *   rimapeditorlib DIR     (DIR with rimapeditorlib, maxslib, memlib
 *                           .obj/.orig)
 *
 * Each set has its own memlib/maxslib (the names), MapObjects, Blitz
 * shapes, bitmap object, RastPort/BitMap (chip memory, the default font),
 * window object and globals (a5, the used map at 8).  Compared: the map
 * objects (all fields; data, name and the addresses at 24/28 by content
 * or relative to the set's blocks), the map data, the used map, a3 after
 * CreateMap and the string functions, the strings, d0, the shapes' data,
 * the bitmap after the text commands and the RastPort's pen/position,
 * the kept registers; memlib's memory back at the end.
 * Covered: every entry; the inline routines (checked inline with the same
 * length, their code called as a subroutine); CreateMap (new, again on
 * the same object: the old one freed, an empty name, AllocMem failing:
 * datasize stays 1, never read); MapDataGrab of bytes (BUG(orig) #898:
 * the carry into the next values) and words; MapCanDraw inside and outside the map, with
 * and without a rub-through map (smaller than the map), stencil hits;
 * ScanShapes (to highest, to an empty shape, start > highest) and the
 * maxima; ShapeTo1616 of heights < 16, 16, > 16, depths 1-4;
 * ShapeMaskFill; FillBitplane; MapObjName$ (no name, a name of 300
 * characters: BUG(orig) #899 the byte length) and SetMapObjName$; the text
 * commands and the pen; SetWindowDraw; M_NextArg$ before M_InitParse,
 * strings with empty arguments; the free routine; init, finit.
 * Not covered: negative coordinates for MapCanDraw, sizes 0 of the DBRA
 * loops, M_InitParse of more than 255 characters (BUG(orig), #897), a map whose
 * data lies at $xxxx0000 (BUG(orig) #894: not freed).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
static struct Library *megfx;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME megfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "rimapeditorlib"
enum { TOKE, TO1616, MASKFILL, NULL1, NULL2, SCAN, MAXW, MAXH, MAXD, CHANGED, FILLBP, CREATE,
       DATAADR, DATASIZE, OBJWIDTH, OBJHEIGHT, GRAB, OBJNAME, SETNAME, VIEWX, VIEWY, OUTTEXT,
       RUB, STENCIL, CANDRAW, CANDRAWADDR, CLEARRUB, OUTPEN, TEXTCENTER, TEXTRIGHT, WINDRAW,
       INITPARSE, NEXTARG, LOAD, SAVE, USE, FREE, FINIT, INIT };

#define NMAP 4
#define NSH 16
#define BMW 320
#define BMH 64
#define USED 8                          /* offset of the used map in the globals */

static UBYTE maps[2][NMAP][32];
static ULONG globals[2][8];
static ULONG stk[2][4];                 /* the a2 stack */
static UBYTE stencil[2][64];
static UBYTE shp[2][NSH][32];
static UBYTE shdata[2][NSH][300];
static UBYTE dest[2][200];
static UBYTE cookie[2][48];
static UBYTE bmobj[2][16];
static UBYTE plane0[2][20 * 24 + 64];
static UBYTE strbuf[2][400];
static UBYTE grabsrc[200];
static struct BitMap bm[2];
static struct RastPort rp[2];
static struct Window win[2];
static struct RastPort wrp[2];
static struct Window *winobj[2][2];

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* ------------------------------------------------------------ compare */

/* map n: the fields, the data (width*height words), the name; the
   addresses at 24/28 relative to the data / to the map objects */
static void samemap(int n)
{
    const UBYTE *a[2] = { maps[0][n], maps[1][n] };
    char w[60];

    sprintf(w, "map %d fields", n);
    SAMEBYTES(w, a, 10);
    a[0] += 14;
    a[1] += 14;
    SAMEBYTES(w, a, 4);
    SAME(w, SW(maps[cur][n], 22));
    sprintf(w, "map %d data set", n);
    SAME(w, SL(maps[cur][n], 10) != 0);
    if (SL(maps[0][n], 10) && SL(maps[1][n], 10)) {
        const UBYTE *d[2] = { (UBYTE *)SL(maps[0][n], 10), (UBYTE *)SL(maps[1][n], 10) };
        sprintf(w, "map %d data", n);
        SAMEBYTES(w, d, 2UL * SW(maps[0][n], 0) * SW(maps[0][n], 2));
    }
    sprintf(w, "map %d name", n);
    SAME(w, SL(maps[cur][n], 18) != 0);
    if (SL(maps[0][n], 18) && SL(maps[1][n], 18))
        BOTH(w, !strcmp((char *)SL(maps[0][n], 18), (char *)SL(maps[1][n], 18)));
    sprintf(w, "map %d last address", n);
    SAME(w, SL(maps[cur][n], 24) ? SL(maps[cur][n], 24) - SL(maps[cur][n], 10) : 0x7fffffffUL);
    sprintf(w, "map %d rub-through map", n);
    SAME(w, SL(maps[cur][n], 28) ? SL(maps[cur][n], 28) - (ULONG)maps[cur] : 0x7fffffffUL);
}

static void samemaps(void)
{
    int n;
    for (n = 0; n < NMAP; n++)
        samemap(n);
}

static void samebitmap(void)
{
    int p;
    char w[30];

    for (p = 0; p < 2; p++) {
        const UBYTE *a[2] = { bm[0].Planes[p], bm[1].Planes[p] };
        sprintf(w, "bitmap plane %d", p);
        SAMEBYTES(w, a, (ULONG)bm[0].BytesPerRow * BMH);
    }
    SAME("cp_x", rp[cur].cp_x);
    SAME("cp_y", rp[cur].cp_y);
    SAME("FgPen", rp[cur].FgPen);
}

/* the string written at a3 (strbuf), d0, a3 */
static void samestring(void)
{
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)strbuf[cur]);
    SAMEBYTES("string", strbuf, sizeof strbuf[0]);
}

/* ------------------------------------------------------------ calls */

/* CreateMap; `nomem`: its AllocMem fails (replaced by rwe_fail0 - a
   real failing AllocMem would run the low memory handlers, which free
   memory of the system at their own pace) */
static void create(int n, UWORD w, UWORD h, UWORD bw, UWORD bh, const char *name, int nomem)
{
    APTR old = NULL;

    ROUT = "CreateMap";
    sprintf(VEC, "map %d %ux%u %u,%u \"%.20s\"%s", n, w, h, bw, bh, name,
            nomem ? " AllocMem fails" : "");
    if (nomem) {
        Forbid();
        old = SetFunction((struct Library *)SysBase, -198, (APTR)rwe_fail0);
        CacheClearU();
    }
    DO2(ENT(LIB, CREATE, 0), R[cur].d[0] = n << 5; R[cur].d[1] = WA(w); R[cur].d[2] = WA(h);
        R[cur].d[3] = WA(bw); R[cur].d[4] = WA(bh); R[cur].d[5] = (ULONG)name;
        R[cur].d[6] = USED; stk[cur][0] = strlen(name); R[cur].a[2] = (ULONG)&stk[cur][1];
        R[cur].a[3] = (ULONG)maps[cur][n]; R[cur].a[5] = (ULONG)globals[cur]);
    if (nomem) {
        SetFunction((struct Library *)SysBase, -198, old);
        CacheClearU();
        Permit();
    }
    SAME("a3", R[cur].a[3] - (ULONG)maps[cur][n]);
    SAME("used map", globals[cur][USED / 4] - (ULONG)maps[cur][n]);
    samemaps();
}

/* an inline routine of map n: its code as a subroutine, d0 compared
   (`rel` 1: relative to the map's data, 2: to the maps) */
static void inl(int e, int form, const char *name, int n, ULONG d0, ULONG d1, int rel)
{
    ROUT = name;
    sprintf(VEC, "map %d d0 $%08lX d1 $%08lX", n, d0, d1);
    BOTH("inline", W(ENT(LIB, e, form)) == 0xa000);
    SAME("inline length", W(ENT(LIB, e, form) + 2));
    DO2(ENT(LIB, e, form) + 4, R[cur].d[0] = d0; R[cur].d[1] = d1;
        R[cur].a[0] = (ULONG)maps[cur][(n + 1) % NMAP]; R[cur].a[3] = (ULONG)maps[cur][n]);
    SAME("d0", rel == 1 ? R[cur].d[0] - SL(maps[cur][n], 10)
               : rel == 2 ? (R[cur].d[0] ? R[cur].d[0] - (ULONG)maps[cur] : 0x7fffffffUL)
               : R[cur].d[0]);
    SAME("d1", R[cur].d[1]);
    samemap(n);
}

static void grab(int n, int words, UWORD null)
{
    ROUT = "MapDataGrab";
    sprintf(VEC, "map %d %s mapnull %u", n, words ? "words" : "bytes", null);
    DO2(ENT(LIB, GRAB, 0), R[cur].d[0] = (ULONG)grabsrc; R[cur].d[1] = WA(words);
        R[cur].d[2] = WA(null); R[cur].a[3] = (ULONG)maps[cur][n]);
    samemap(n);
}

static void candraw(int n, UWORD shape, UWORD x, UWORD y)
{
    ROUT = "MapCanDraw";
    sprintf(VEC, "map %d shape %u at %u,%u", n, shape, x, y);
    DO2(ENT(LIB, CANDRAW, 0), R[cur].d[0] = WA(shape); R[cur].d[1] = WA(x);
        R[cur].d[2] = WA(y); R[cur].a[3] = (ULONG)maps[cur][n]);
    SAME("d0", R[cur].d[0]);
    samemap(n);
}

static void scan(UWORD start, UWORD highest)
{
    static const int e[3] = { MAXW, MAXH, MAXD };
    int i;

    ROUT = "ScanShapes";
    sprintf(VEC, "%u,%u", start, highest);
    DO2(ENT(LIB, SCAN, 0), R[cur].d[0] = 0x5a000000UL | start << 5; R[cur].d[1] = WA(highest);
        R[cur].a[0] = (ULONG)shp[cur][start]);
    SAME("d0", R[cur].d[0]);
    for (i = 0; i < 3; i++) {
        ROUT = i == 0 ? "ScanMaxWidth" : i == 1 ? "ScanMaxheight" : "ScanMaxdepth";
        DO2(ENT(LIB, e[i], 0), R[cur].d[0] = 0x5a5a1234UL);
        SAME("d0", R[cur].d[0]);
    }
}

/* the Blitz shape i: w x h, depth d, random data */
static void makeshape(int i, UWORD w, UWORD h, UWORD d)
{
    UWORD eb = ((w + 15) >> 4) << 1;
    int s, k;

    for (s = 0; s < 2; s++) {
        memset(shp[s][i], 0, 32);
        SW(shp[s][i], 0) = w;
        SW(shp[s][i], 2) = h;
        SW(shp[s][i], 4) = d;
        SW(shp[s][i], 6) = eb;
        SL(shp[s][i], 14) = (ULONG)shdata[s][i];
        SW(shp[s][i], 22) = eb * h;
        SL(shp[s][i], 18) = (ULONG)cookie[s];
    }
    for (k = 0; k < 300; k++)
        shdata[0][i][k] = shdata[1][i][k] = (UBYTE)(rnd() >> 8);
}

static void to1616(int src)
{
    int s;
    for (s = 0; s < 2; s++) {
        memset(dest[s], 0x77, sizeof dest[s]);
        SL(shp[s][NSH - 1], 14) = (ULONG)dest[s];
    }
    ROUT = "ShapeTo1616";
    sprintf(VEC, "shape %d (%u high, depth %u)", src, SW(shp[0][src], 2), SW(shp[0][src], 4));
    DO2(ENT(LIB, TO1616, 0), R[cur].d[0] = src << 5; R[cur].d[1] = (NSH - 1) << 5;
        R[cur].a[0] = (ULONG)shp[cur][src]; R[cur].a[1] = (ULONG)shp[cur][NSH - 1]);
    SAMEBYTES("dest", dest, sizeof dest[0]);
}

static void fillbp(UWORD w, UWORD h)
{
    int s;
    for (s = 0; s < 2; s++)
        memset(plane0[s], 0x33, sizeof plane0[s]);
    ROUT = "FillBitplane";
    sprintf(VEC, "%u,%u", w, h);
    DO2(ENT(LIB, FILLBP, 0), R[cur].d[0] = 0; R[cur].d[1] = WA(w); R[cur].d[2] = WA(h);
        R[cur].a[0] = (ULONG)bmobj[cur]);
    SAMEBYTES("plane", plane0, sizeof plane0[0]);
}

static void objname(int n)
{
    ROUT = "MapObjName$";
    sprintf(VEC, "map %d", n);
    for (cur = 0; cur < 2; cur++)
        memset(strbuf[cur], 0xee, sizeof strbuf[cur]);
    DO2(ENT(LIB, OBJNAME, 0), R[cur].a[1] = (ULONG)maps[cur][n]; R[cur].a[3] = (ULONG)strbuf[cur]);
    samestring();
}

static void setname(int n, const char *name)
{
    ROUT = "SetMapObjName$";
    sprintf(VEC, "map %d \"%.20s\"", n, name);
    DO2(ENT(LIB, SETNAME, 0), R[cur].d[0] = (ULONG)name; stk[cur][0] = strlen(name);
        R[cur].a[2] = (ULONG)&stk[cur][1]; R[cur].a[3] = (ULONG)maps[cur][n]);
    samemap(n);
    objname(n);
}

static void text(int e, const char *name, WORD x, WORD y, const char *s)
{
    ROUT = name;
    sprintf(VEC, "%d,%d \"%s\"", x, y, s);
    DO2(ENT(LIB, e, 0), R[cur].d[0] = (UWORD)x; R[cur].d[1] = (UWORD)y;
        R[cur].d[2] = (ULONG)&rp[cur]; R[cur].d[3] = (ULONG)s);
    samebitmap();
}

static void pen(UWORD p)
{
    ROUT = "MapOutTextPen";
    sprintf(VEC, "%u", p);
    DO2(ENT(LIB, OUTPEN, 0), R[cur].d[0] = (ULONG)&rp[cur]; R[cur].d[1] = WA(p));
    samebitmap();
}

static void windraw(ULONG mode)
{
    ROUT = "SetWindowDraw";
    sprintf(VEC, "mode $%08lX", mode);
    for (cur = 0; cur < 2; cur++) {
        memset(&wrp[cur], 0x77, sizeof wrp[cur]);
        win[cur].RPort = &wrp[cur];
        winobj[cur][0] = &win[cur];
    }
    DO2(ENT(LIB, WINDRAW, 0), R[cur].d[0] = 0x5a000003; R[cur].d[1] = mode;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("AreaPtrn set", wrp[cur].AreaPtrn != NULL);
    if (wrp[0].AreaPtrn && wrp[1].AreaPtrn) {
        SAME("AreaPtrn[0]", wrp[cur].AreaPtrn[0]);
        SAME("AreaPtrn[1]", wrp[cur].AreaPtrn[1]);
    }
    SAME("AreaPtSz", wrp[cur].AreaPtSz);
    SAME("DrawMode", wrp[cur].DrawMode);
}

static void initparse(const char *s)
{
    ROUT = "M_InitParse";
    sprintf(VEC, "\"%s\"", s);
    DO2(ENT(LIB, INITPARSE, 0), R[cur].d[0] = (ULONG)s);
}

static void nextarg(void)
{
    ROUT = "M_NextArg$";
    for (cur = 0; cur < 2; cur++)
        memset(strbuf[cur], 0xee, sizeof strbuf[cur]);
    DO2(ENT(LIB, NEXTARG, 0), R[cur].a[3] = (ULONG)strbuf[cur]);
    samestring();
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "maxslib", "memlib", NULL };
    static char longname[301];
    static struct TextAttr topaz8 = { (STRPTR)"topaz.font", 8, 0, FPF_ROMFONT };
    struct TextFont *topaz;
    static const char *const args[] = { "", ",", "alpha,beta,,gamma delta, x", "one", "a,b,",
                                        ",,lead" };
    ULONG avail;
    int s, p, i, k;

    if (argc < 2) {
        printf("usage: rimapeditorlib DIR\n");
        return 20;
    }
    megfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!megfx || !(topaz = OpenFont(&topaz8)) || !load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        InitBitMap(&bm[s], 2, BMW, BMH);
        for (p = 0; p < 2; p++) {
            bm[s].Planes[p] = AllocRaster(BMW, BMH);
            memset(bm[s].Planes[p], 0, (ULONG)bm[s].BytesPerRow * BMH);
        }
        InitRastPort(&rp[s]);
        rp[s].BitMap = &bm[s];
        SetFont(&rp[s], topaz);
        SW(bmobj[s], 0) = 20;
        SL(bmobj[s], 8) = (ULONG)plane0[s];
    }
    for (k = 0; k < 64; k++)
        stencil[0][k] = stencil[1][k] = (rnd() & 0x300) == 0;
    for (k = 0; k < (int)sizeof grabsrc; k++)
        grabsrc[k] = (UBYTE)(rnd() % 30);
    memset(longname, 'n', 300);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();

    ROUT = "memlib init";
    memlib(7);
    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);

    /* the maps */
    create(0, 10, 8, 16, 16, "level1", 0);
    create(1, 6, 5, 32, 8, "", 0);
    create(2, 4, 5, 1, 2, "nothing", 1);        /* datasize stays (never read) */
    create(3, 7, 3, 8, 8, "first", 0);
    create(3, 9, 4, 8, 16, "second", 0);        /* the old one freed */
    grab(0, 0, 1);
    grab(1, 1, 0);
    grab(3, 0, 0);
    for (k = 0; k < 40; k++)
        grabsrc[k] = 250 + k % 6;
    grab(3, 0, 10);                     /* BUG(orig) (#898): the carry */
    for (k = 0; k < 40; k++)
        grabsrc[k] = (UBYTE)(rnd() % 30);
    grab(3, 1, 0xfff0);

    /* the inline routines */
    for (i = 0; i < NMAP; i++) {
        inl(OBJWIDTH, 0, "MapObjWidth", i, 0x5a5a5a5a, 0, 0);
        inl(OBJHEIGHT, 0, "MapObjheight", i, 0x5a5a5a5a, 0, 0);
        inl(DATASIZE, 0, "MapDataSize", i, 0x5a5a5a5a, 0, 0);
        inl(VIEWX, 0, "MapObjViewX", i, 0x5a5a5a5a, 0, 0);
        inl(VIEWY, 0, "MapObjViewY", i, 0x5a5a5a5a, 0, 0);
        inl(CHANGED, 0, "MapDataChanged", i, 0x5a5a5a5a, 0, 0);
        inl(RUB, 0, "RubThroughMap", i, 0x5a5a5a5a, 0, 2);
        inl(CANDRAWADDR, 0, "MapCanDrawAddr", i, 0x5a5a5a5a, 0, 0);
    }
    inl(DATAADR, 0, "MapDataAdr", 0, 0x5a5a5a5a, 0, 1);
    inl(DATAADR, 1, "MapDataAdr(x,y)", 0, WA(3), WA(2), 1);
    inl(DATAADR, 1, "MapDataAdr(x,y)", 3, 0xfffe8001, 0x00030001, 1);
    for (i = 0; i < 4; i++) {
        ROUT = "MapDataChanged(newstate)";
        sprintf(VEC, "map %d", i);
        DO2(ENT(LIB, CHANGED, 1), R[cur].d[0] = WA(i * 0x7fff - 1); R[cur].a[3] = (ULONG)maps[cur][i]);
        SAME("d0", R[cur].d[0]);
        samemap(i);
    }
    inl(CHANGED, 0, "MapDataChanged", 2, 0x12345678, 0, 0);

    /* MapCanDraw */
    ROUT = "SetStencil";
    strcpy(VEC, "");
    DO2(ENT(LIB, STENCIL, 0), R[cur].d[0] = (ULONG)stencil[cur]);
    for (k = 0; k < 60; k++)
        candraw(0, (UWORD)(rnd() % 32), (UWORD)(rnd() % 12), (UWORD)(rnd() % 10));
    for (k = 0; k < 10; k++)            /* the map's value - 1: -1 */
        candraw(0, SW(SL(maps[0][0], 10), 2 * (k + 10)) - 1, k, 1);
    inl(CANDRAWADDR, 0, "MapCanDrawAddr", 0, 0x5a5a5a5a, 0, 1);
    inl(RUB, 1, "RubThroughMap(map#)", 0, 0x5a5a5a5a, 0, 0);    /* a0 = map 1 */
    for (k = 0; k < 60; k++)
        candraw(0, (UWORD)(rnd() % 32), (UWORD)(rnd() % 12), (UWORD)(rnd() % 10));
    inl(RUB, 0, "RubThroughMap", 0, 0x5a5a5a5a, 0, 2);
    inl(CLEARRUB, 0, "ClearRubThroughMap", 0, 0x5a5a5a5a, 0, 0);
    candraw(0, 3, 4, 4);

    /* shapes */
    makeshape(0, 32, 9, 2);
    makeshape(1, 16, 16, 3);
    makeshape(2, 48, 23, 1);
    makeshape(3, 20, 1, 3);
    makeshape(4, 0x8001, 15, 2);        /* a negative width */
    makeshape(5, 17, 0x8000, 0xffff);
    makeshape(6, 64, 31, 1);
    makeshape(7, 0, 0, 0);              /* empty */
    makeshape(8, 24, 5, 4);
    makeshape(9, 40, 33, 2);
    makeshape(10, 0, 3, 3);             /* empty */
    for (i = 11; i < NSH - 1; i++)
        makeshape(i, 8 * i, i, i - 10);
    makeshape(NSH - 1, 0, 0, 0);        /* empty (also ShapeTo1616's dest) */
    scan(0, 3);
    scan(0, 15);                        /* to the empty shape 7 */
    scan(8, 8);
    scan(4, 6);
    scan(11, 12);
    scan(13, 2);                        /* start > highest: to shape 15 */
    to1616(0);
    to1616(1);
    to1616(2);
    to1616(3);
    to1616(8);
    to1616(9);
    to1616(11);
    ROUT = "ShapeMaskFill";
    strcpy(VEC, "");
    for (cur = 0; cur < 2; cur++)
        memset(cookie[cur], 0x11, sizeof cookie[cur]);
    DO2(ENT(LIB, MASKFILL, 0), R[cur].d[0] = 0; R[cur].a[0] = (ULONG)shp[cur][0]);
    SAMEBYTES("cookie", cookie, sizeof cookie[0]);
    fillbp(16, 1);
    fillbp(64, 7);
    fillbp(160, 24);
    fillbp(100, 3);
    fillbp(31, 2);

    /* names */
    objname(0);
    objname(1);                         /* no name: "" */
    setname(1, "renamed map");
    setname(0, longname);               /* BUG(orig) (#899): the length as a byte */
    setname(0, "");
    setname(3, "third");

    /* text */
    pen(1);
    text(OUTTEXT, "MapOutText", 4, 2, "Hello");
    pen(2);
    text(TEXTCENTER, "MapOutTextCenter", 160, 12, "Centered");
    pen(3);
    text(TEXTRIGHT, "MapOutTextRight", 300, 24, "Right");
    text(TEXTCENTER, "MapOutTextCenter", 100, 30, "x");
    text(TEXTRIGHT, "MapOutTextRight", 200, 30, "");
    text(OUTTEXT, "MapOutText", 250, 2, "");

    windraw(1);
    windraw(0);
    windraw(0x5a010000);

    /* parsing */
    nextarg();                          /* before M_InitParse: "" */
    for (i = 0; i < (int)(sizeof args / sizeof args[0]); i++) {
        initparse(args[i]);
        for (k = 0; k < 6; k++)
            nextarg();
    }

    /* the free routine */
    for (i = 0; i < NMAP; i++) {
        ROUT = "free";
        sprintf(VEC, "map %d", i);
        DO2(ENT(LIB, FREE, 0), R[cur].a[3] = (ULONG)maps[cur][i]);
        samemap(i);
    }
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    ROUT = "memlib finit";
    memlib(8);
    rwe_mem_check(LIB, avail, TRUE);

    for (s = 0; s < 2; s++)
        for (p = 0; p < 2; p++)
            FreeRaster(bm[s].Planes[p], BMW, BMH);
    CloseFont(topaz);
    CloseLibrary(megfx);
    return rwe_done(LIB);
}
