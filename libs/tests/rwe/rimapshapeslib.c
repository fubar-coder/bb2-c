/*
 * rwe rimapshapeslib - rimapshapeslib (userlib 6): the C build or
 * the reference build of rwelibsrc/MapEd/MapShape2.bb (blitz2 2.10),
 * one set as LIBSET says, observed as cases (rwecmp.h; the
 * expected data libs/tests/expected/rwe_rimapshapeslib.txt).
 *
 *   rimapshapeslib DIR     (DIR with rimapshapeslib .obj or .orig)
 *
 * The set has its Blitz shapes (32-byte objects, the data in chip
 * memory, random), a window object whose Window/RastPort/BitMap are built
 * here (no layer: BltBitMapRastPort blits straight into the bitmap), and
 * the map shapes/group the library allocates.  Observed: the map shapes
 * (all fields; the plane pointers relative to the shape's data), the
 * group (member addresses relative to map shape 0, the offsets, the end
 * mark), MapShapeAddr/ResetGroup relative to the blocks, the
 * bitmap after every blit, the kept registers.
 * Covered: init; SetNumMapShapes (allocate, reallocate: cleared);
 * MapShapeAddr with garbage above the word; ShapeToMapShape of shapes of
 * depth 1-3, widths 16-48, odd heights; BlitMapShape of these and of an
 * empty map shape (nothing), at several positions; BlitMapShapeSize with
 * smaller and larger (clamped) heights; RemapMapShape (nothing);
 * ResetGroup;
 * AddGroupMember (also an empty map shape), BlitGroup with a6 =
 * graphics.library (BUG(orig) #902: the caller's a6), twice with another
 * ResetGroup; FreeMapShapes (a range, last < first, one shape, first
 * -1: the word compare passes, the address compare not); finit.
 * Not covered: map shapes before SetNumMapShapes (address 0), depth 0,
 * more than 63 group members, BlitGroup with another a6 (all BUG(orig): #906 #903 #904 #902);
 * blits that leave the bitmap or have a size <= 0 (without a layer
 * nothing clips them).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
static struct Library *msgfx;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME msgfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "rimapshapeslib"
enum { TOMAP, BLIT, REMAP, BLITSIZE, SETNUM, ADDR, FREE, ADDMEMBER, BLITGROUP, RESETGROUP,
       FINIT, INIT };

#define NSH 6
#define NMS 20
#define BMW 320
#define BMH 128
#define DEPTH 3

static UBYTE shp[NSH][32];
static UBYTE *shdata[NSH];
static ULONG shsize[NSH];
static struct BitMap bm;
static struct RastPort rp;
static struct Window win;
static struct Window *winobj[2];
static UBYTE *base;                     /* map shape 0 */
static ULONG *group;

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* the shape i: w x h pixels, d planes, random data in chip memory */
static void makeshape(int i, UWORD w, UWORD h, UWORD d)
{
    UWORD eb = ((w + 15) >> 4) << 1;
    ULONG n = (ULONG)eb * h * d, k;

    shsize[i] = n;
    shdata[i] = AllocMem(n, MEMF_CHIP);
    memset(shp[i], 0, 32);
    SW(shp[i], 0) = w;
    SW(shp[i], 2) = h;
    SW(shp[i], 4) = d;
    SW(shp[i], 6) = eb;
    SL(shp[i], 14) = (ULONG)shdata[i];
    SW(shp[i], 22) = eb * h;
    SW(shp[i], 26) = n;
    for (k = 0; k < n; k++)
        shdata[i][k] = (UBYTE)(rnd() >> 8);
}

/* ------------------------------------------------------------ observe */

static void samemapshapes(void)
{
    int n, p;
    char w[60];

    for (n = 0; n < NMS; n++) {
        const UBYTE *a = base + 64 * n;
        sprintf(w, "map shape %d fields", n);
        OBSBYTES(w, a, 10);
        for (p = 0; p < 8; p++) {
            int i;
            ULONG v = SL(a, 10 + 4 * p);
            /* relative to the data of the shape it lies in */
            for (i = 0; i < NSH; i++)
                if (v >= (ULONG)shdata[i] && v < (ULONG)shdata[i] + shsize[i]) {
                    v = (v - (ULONG)shdata[i]) | (i + 1) << 24;
                    break;
                }
            sprintf(w, "map shape %d plane %d", n, p);
            OBS(w, v);
        }
        sprintf(w, "map shape %d rest", n);
        OBSBYTES(w, a + 42, 22);
    }
}

static void samebitmap(void)
{
    int p;
    char w[30];

    for (p = 0; p < DEPTH; p++) {
        sprintf(w, "bitmap plane %d", p);
        OBSBYTES(w, bm.Planes[p], (ULONG)bm.BytesPerRow * BMH);
    }
}

static void samegroup(void)
{
    int i;
    char w[40];

    for (i = 0; i < 64; i++) {
        sprintf(w, "group member %d", i);
        OBS(w, group[2 * i] ? group[2 * i] - (ULONG)base : 0xffffffffUL);
        if (!group[2 * i])
            break;
        sprintf(w, "group member %d offsets", i);
        OBS(w, group[2 * i + 1]);
    }
}

/* ------------------------------------------------------------ calls */

static void setnum(UWORD n)
{
    ROUT = "SetNumMapShapes";
    sprintf(VEC, "%u", n);
    DO1(ENT(LIB, SETNUM, 0), R.d[0] = WA(n));
    ROUT = "MapShapeAddr";
    DO1(ENT(LIB, ADDR, 0), R.d[0] = WA(0));
    base = (UBYTE *)R.d[0];
    BOTH("allocated", base != NULL);
    sprintf(VEC, "7");
    DO1(ENT(LIB, ADDR, 0), R.d[0] = WA(7));
    OBS("d0", R.d[0] - (ULONG)base);
}

static void tomap(int sh, UWORD ms)
{
    ROUT = "ShapeToMapShape";
    sprintf(VEC, "shape %d map shape %u", sh, ms);
    DO1(ENT(LIB, TOMAP, 0), R.d[0] = sh << 5; R.d[1] = WA(ms);
        R.a[0] = (ULONG)shp[sh]);
}

static void clearbitmap(void)
{
    int p;
    for (p = 0; p < DEPTH; p++)
        memset(bm.Planes[p], 0x5a ^ p, (ULONG)bm.BytesPerRow * BMH);
}

/* a checksum of the bitmap (to see that a blit changed it) */
static ULONG bmsum(void)
{
    ULONG sum = 0, k, n = (ULONG)bm.BytesPerRow * BMH;
    int p;
    for (p = 0; p < DEPTH; p++)
        for (k = 0; k < n; k++)
            sum = sum * 31 + bm.Planes[p][k];
    return sum;
}

static void blitms(UWORD ms, WORD x, WORD y)
{
    ULONG sum = bmsum();
    ROUT = "BlitMapShape";
    sprintf(VEC, "map shape %u at %d,%d", ms, x, y);
    DO1(ENT(LIB, BLIT, 0), R.d[0] = 0; R.d[1] = WA(ms);
        R.d[2] = (UWORD)x; R.d[3] = (UWORD)y;
        R.d[4] = 0; R.d[5] = 0; R.d[6] = 0;
        R.a[1] = (ULONG)winobj; R.a[6] = (ULONG)msgfx);
    samebitmap();
    BOTH(ms ? "blitted" : "nothing blitted", (bmsum() != sum) == (ms != 0));
}

static void blitsize(UWORD ms, WORD x, WORD y, WORD w, WORD h)
{
    ROUT = "BlitMapShapeSize";
    sprintf(VEC, "map shape %u at %d,%d size %d,%d", ms, x, y, w, h);
    DO1(ENT(LIB, BLITSIZE, 0), R.d[0] = 0; R.d[1] = WA(ms);
        R.d[2] = (UWORD)x; R.d[3] = (UWORD)y;
        R.d[4] = (UWORD)w; R.d[5] = (UWORD)h; R.d[6] = 0;
        R.a[1] = (ULONG)winobj; R.a[6] = (ULONG)msgfx);
    samebitmap();
}

static void resetgroup(UWORD bw, UWORD bh)
{
    ROUT = "ResetGroup";
    sprintf(VEC, "%u,%u", bw, bh);
    DO1(ENT(LIB, RESETGROUP, 0), R.d[0] = WA(bw); R.d[1] = WA(bh));
    group = (ULONG *)R.d[0];
    BOTH("group", group != NULL);
    samegroup();
}

static void member(UWORD ms, UWORD dx, UWORD dy)
{
    ROUT = "AddGroupMember";
    sprintf(VEC, "%u,%u,%u", ms, dx, dy);
    DO1(ENT(LIB, ADDMEMBER, 0), R.d[0] = WA(ms); R.d[1] = WA(dx);
        R.d[2] = WA(dy));
    samegroup();
}

static void blitgroup(WORD x, WORD y)
{
    ROUT = "BlitGroup";
    sprintf(VEC, "at %d,%d", x, y);
    DO1(ENT(LIB, BLITGROUP, 0), R.d[0] = 0; R.d[1] = (UWORD)x;
        R.d[2] = (UWORD)y; R.d[3] = 0; R.d[4] = 0; R.d[6] = 0;
        R.a[1] = (ULONG)winobj; R.a[6] = (ULONG)msgfx);
    samebitmap();
}

static void freems(UWORD first, UWORD last)
{
    ROUT = "FreeMapShapes";
    sprintf(VEC, "%u,%u", first, last);
    DO1(ENT(LIB, FREE, 0), R.d[0] = WA(first); R.d[1] = WA(last));
    samemapshapes();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int p, i;

    if (argc < 2) {
        printf("usage: rimapshapeslib DIR\n");
        return 20;
    }
    msgfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!msgfx || !load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("SetNumMapShapes", d2-d6|a2|a3);
    KEEPS("MapShapeAddr", d1-d7|a1-a3|a6);
    KEEPS("ShapeToMapShape", d2-d7|a2|a3|a6);
    KEEPS("RemapMapShape", d1-d7|a1-a3|a6);
    KEEPS("BlitMapShape", d2|d3|d7|a2|a3|a6);
    KEEPS("BlitMapShapeSize", d2-d4|d7|a2|a3|a6);
    KEEPS("ResetGroup", d1-d7|a1-a3|a6);
    KEEPS("AddGroupMember", d3-d7|a2|a3|a6);
    KEEPS("BlitGroup", d7|a2|a3|a6);
    KEEPS("FreeMapShapes", d2-d7|a2|a3|a6);
    InitBitMap(&bm, DEPTH, BMW, BMH);
    for (p = 0; p < DEPTH; p++)
        bm.Planes[p] = AllocRaster(BMW, BMH);
    InitRastPort(&rp);
    rp.BitMap = &bm;
    win.RPort = &rp;
    winobj[0] = &win;
    makeshape(0, 16, 16, 3);
    makeshape(1, 24, 11, 2);
    makeshape(2, 32, 7, 1);
    makeshape(3, 48, 21, 3);
    makeshape(4, 17, 5, 2);
    makeshape(5, 40, 33, 3);
    clearbitmap();

    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, INIT, 0), (void)0);

    setnum(NMS);
    samemapshapes();
    for (i = 0; i < NSH; i++)
        tomap(i, i + 1);
    tomap(3, 12);
    samemapshapes();

    ROUT = "RemapMapShape";
    strcpy(VEC, "");
    DO1(ENT(LIB, REMAP, 0), R.d[0] = WA(1); R.d[1] = WA(2); R.d[2] = WA(5));
    samemapshapes();

    blitms(0, 10, 10);                  /* empty */
    for (i = 1; i <= NSH; i++)
        blitms(i, 3 + 17 * i, 2 + 3 * i);
    blitms(12, 100, 30);
    blitms(2, 0, 0);
    blitms(6, BMW - 40, BMH - 33);

    clearbitmap();
    blitsize(1, 5, 5, 16, 8);
    blitsize(1, 40, 5, 9, 40);          /* clamped to 16 */
    blitsize(4, 60, 10, 17, 5);
    blitsize(6, 80, 20, 33, 12);
    blitsize(0, 10, 10, 16, 16);        /* empty */

    clearbitmap();
    resetgroup(16, 8);
    member(1, 0, 0);
    member(2, 1, 0);
    member(0, 2, 0);                    /* empty: skipped */
    member(3, 0, 1);
    member(5, 2, 2);
    member(6, 5, 3);
    blitgroup(4, 2);
    blitgroup(1, 20);
    resetgroup(3, 5);
    for (i = 0; i < 12; i++)
        member(i % 7, i, i / 2);
    blitgroup(10, 1);
    resetgroup(0, 0);
    blitgroup(0, 0);                    /* nothing */

    freems(2, 4);
    freems(9, 8);                       /* nothing */
    freems(12, 12);
    freems(0xffff, 3);                  /* -1 <= 3, but map shape 65535 lies behind 3: nothing */
    setnum(NMS);                        /* new block, cleared */
    samemapshapes();

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), (void)0);

    for (p = 0; p < DEPTH; p++)
        FreeRaster(bm.Planes[p], BMW, BMH);
    for (i = 0; i < NSH; i++)
        FreeMem(shdata[i], shsize[i]);
    CloseLibrary(msgfx);
    return rwe_done(LIB);
}
