/*
 * rwe rimapshapeslib - the C build of rimapshapeslib (userlib 6) against
 * the reference build of rwelibsrc/MapEd/MapShape2.bb (blitz2 2.10),
 * method of rwecmp.h.
 *
 *   rimapshapeslib DIR     (DIR with rimapshapeslib .obj/.orig)
 *
 * Each set has its own Blitz shapes (32-byte objects, the data in chip
 * memory, random), a window object whose Window/RastPort/BitMap are built
 * here (no layer: BltBitMapRastPort blits straight into the bitmap), and
 * the map shapes/group the library allocates.  Compared: the map shapes
 * (all fields; the plane pointers relative to the shape's data), the
 * group (member addresses relative to map shape 0, the offsets, the end
 * mark), MapShapeAddr/ResetGroup relative to the set's blocks, the
 * bitmap after every blit, the kept registers.
 * Covered: init; SetNumMapShapes (allocate, reallocate: cleared);
 * MapShapeAddr with garbage above the word; ShapeToMapShape of shapes of
 * depth 1-3, widths 16-48, odd heights; BlitMapShape of these and of an
 * empty map shape (nothing), at several positions; BlitMapShapeSize with
 * smaller and larger (clamped) heights; RemapMapShape (nothing);
 * ResetGroup;
 * AddGroupMember (also an empty map shape), BlitGroup with a6 =
 * graphics.library (BUG(orig): the caller's a6), twice with another
 * ResetGroup; FreeMapShapes (a range, last < first, one shape, first
 * -1: the word compare passes, the address compare not); finit.
 * Not covered: map shapes before SetNumMapShapes (address 0), depth 0,
 * more than 63 group members, BlitGroup with another a6 (all BUG(orig));
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

static UBYTE shp[2][NSH][32];
static UBYTE *shdata[2][NSH];
static ULONG shsize[NSH];
static struct BitMap bm[2];
static struct RastPort rp[2];
static struct Window win[2];
static struct Window *winobj[2][2];
static UBYTE *base[2];                  /* map shape 0 */
static ULONG *group[2];

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* the shape i: w x h pixels, d planes, random data in chip memory */
static void makeshape(int i, UWORD w, UWORD h, UWORD d)
{
    UWORD eb = ((w + 15) >> 4) << 1;
    ULONG n = (ULONG)eb * h * d, k;
    int s;

    shsize[i] = n;
    for (s = 0; s < 2; s++) {
        shdata[s][i] = AllocMem(n, MEMF_CHIP);
        memset(shp[s][i], 0, 32);
        SW(shp[s][i], 0) = w;
        SW(shp[s][i], 2) = h;
        SW(shp[s][i], 4) = d;
        SW(shp[s][i], 6) = eb;
        SL(shp[s][i], 14) = (ULONG)shdata[s][i];
        SW(shp[s][i], 22) = eb * h;
        SW(shp[s][i], 26) = n;
    }
    for (k = 0; k < n; k++)
        shdata[0][i][k] = shdata[1][i][k] = (UBYTE)(rnd() >> 8);
}

/* ------------------------------------------------------------ compare */

static void samemapshapes(void)
{
    int n, p;
    char w[60];

    for (n = 0; n < NMS; n++) {
        const UBYTE *a[2] = { base[0] + 64 * n, base[1] + 64 * n };
        sprintf(w, "map shape %d fields", n);
        SAMEBYTES(w, a, 10);
        for (p = 0; p < 8; p++) {
            int i;
            ULONG v[2];
            for (cur = 0; cur < 2; cur++) {
                v[cur] = SL(base[cur] + 64 * n, 10 + 4 * p);
                /* relative to the data of the shape it lies in */
                for (i = 0; i < NSH; i++)
                    if (v[cur] >= (ULONG)shdata[cur][i]
                        && v[cur] < (ULONG)shdata[cur][i] + shsize[i]) {
                        v[cur] = (v[cur] - (ULONG)shdata[cur][i]) | (i + 1) << 24;
                        break;
                    }
            }
            sprintf(w, "map shape %d plane %d", n, p);
            if (v[0] != v[1])
                fail(w, v[0], v[1]);
            else
                checks++;
        }
        sprintf(w, "map shape %d rest", n);
        a[0] += 42;
        a[1] += 42;
        SAMEBYTES(w, a, 22);
    }
}

static void samebitmap(void)
{
    int p;
    char w[30];

    for (p = 0; p < DEPTH; p++) {
        const UBYTE *a[2] = { bm[0].Planes[p], bm[1].Planes[p] };
        sprintf(w, "bitmap plane %d", p);
        SAMEBYTES(w, a, (ULONG)bm[0].BytesPerRow * BMH);
    }
}

static void samegroup(void)
{
    int i;
    char w[40];

    for (i = 0; i < 64; i++) {
        sprintf(w, "group member %d", i);
        SAME(w, group[cur][2 * i] ? group[cur][2 * i] - (ULONG)base[cur] : 0xffffffffUL);
        if (!group[0][2 * i])
            break;
        sprintf(w, "group member %d offsets", i);
        SAME(w, group[cur][2 * i + 1]);
    }
}

/* ------------------------------------------------------------ calls */

static void setnum(UWORD n)
{
    ROUT = "SetNumMapShapes";
    sprintf(VEC, "%u", n);
    DO2(ENT(LIB, SETNUM, 0), R[cur].d[0] = WA(n));
    ROUT = "MapShapeAddr";
    DO2(ENT(LIB, ADDR, 0), R[cur].d[0] = WA(0));
    for (cur = 0; cur < 2; cur++)
        base[cur] = (UBYTE *)R[cur].d[0];
    BOTH("allocated", base[cur] != NULL);
    sprintf(VEC, "7");
    DO2(ENT(LIB, ADDR, 0), R[cur].d[0] = WA(7));
    SAME("d0", R[cur].d[0] - (ULONG)base[cur]);
}

static void tomap(int sh, UWORD ms)
{
    ROUT = "ShapeToMapShape";
    sprintf(VEC, "shape %d map shape %u", sh, ms);
    DO2(ENT(LIB, TOMAP, 0), R[cur].d[0] = sh << 5; R[cur].d[1] = WA(ms);
        R[cur].a[0] = (ULONG)shp[cur][sh]);
}

static void clearbitmap(void)
{
    int p;
    for (cur = 0; cur < 2; cur++)
        for (p = 0; p < DEPTH; p++)
            memset(bm[cur].Planes[p], 0x5a ^ p, (ULONG)bm[cur].BytesPerRow * BMH);
}

/* a checksum of the set's bitmap (to see that a blit changed it) */
static ULONG bmsum(void)
{
    ULONG sum = 0, k, n = (ULONG)bm[cur].BytesPerRow * BMH;
    int p;
    for (p = 0; p < DEPTH; p++)
        for (k = 0; k < n; k++)
            sum = sum * 31 + bm[cur].Planes[p][k];
    return sum;
}

static void blitms(UWORD ms, WORD x, WORD y)
{
    ULONG sum[2];
    for (cur = 0; cur < 2; cur++)
        sum[cur] = bmsum();
    ROUT = "BlitMapShape";
    sprintf(VEC, "map shape %u at %d,%d", ms, x, y);
    DO2(ENT(LIB, BLIT, 0), R[cur].d[0] = 0; R[cur].d[1] = WA(ms);
        R[cur].d[2] = (UWORD)x; R[cur].d[3] = (UWORD)y;
        R[cur].d[4] = 0; R[cur].d[5] = 0; R[cur].d[6] = 0;
        R[cur].a[1] = (ULONG)winobj[cur]; R[cur].a[6] = (ULONG)msgfx);
    samebitmap();
    BOTH(ms ? "blitted" : "nothing blitted", (bmsum() != sum[cur]) == (ms != 0));
}

static void blitsize(UWORD ms, WORD x, WORD y, WORD w, WORD h)
{
    ROUT = "BlitMapShapeSize";
    sprintf(VEC, "map shape %u at %d,%d size %d,%d", ms, x, y, w, h);
    DO2(ENT(LIB, BLITSIZE, 0), R[cur].d[0] = 0; R[cur].d[1] = WA(ms);
        R[cur].d[2] = (UWORD)x; R[cur].d[3] = (UWORD)y;
        R[cur].d[4] = (UWORD)w; R[cur].d[5] = (UWORD)h; R[cur].d[6] = 0;
        R[cur].a[1] = (ULONG)winobj[cur]; R[cur].a[6] = (ULONG)msgfx);
    samebitmap();
}

static void resetgroup(UWORD bw, UWORD bh)
{
    ROUT = "ResetGroup";
    sprintf(VEC, "%u,%u", bw, bh);
    DO2(ENT(LIB, RESETGROUP, 0), R[cur].d[0] = WA(bw); R[cur].d[1] = WA(bh));
    for (cur = 0; cur < 2; cur++)
        group[cur] = (ULONG *)R[cur].d[0];
    BOTH("group", group[cur] != NULL);
    samegroup();
}

static void member(UWORD ms, UWORD dx, UWORD dy)
{
    ROUT = "AddGroupMember";
    sprintf(VEC, "%u,%u,%u", ms, dx, dy);
    DO2(ENT(LIB, ADDMEMBER, 0), R[cur].d[0] = WA(ms); R[cur].d[1] = WA(dx);
        R[cur].d[2] = WA(dy));
    samegroup();
}

static void blitgroup(WORD x, WORD y)
{
    ROUT = "BlitGroup";
    sprintf(VEC, "at %d,%d", x, y);
    DO2(ENT(LIB, BLITGROUP, 0), R[cur].d[0] = 0; R[cur].d[1] = (UWORD)x;
        R[cur].d[2] = (UWORD)y; R[cur].d[3] = 0; R[cur].d[4] = 0; R[cur].d[6] = 0;
        R[cur].a[1] = (ULONG)winobj[cur]; R[cur].a[6] = (ULONG)msgfx);
    samebitmap();
}

static void freems(UWORD first, UWORD last)
{
    ROUT = "FreeMapShapes";
    sprintf(VEC, "%u,%u", first, last);
    DO2(ENT(LIB, FREE, 0), R[cur].d[0] = WA(first); R[cur].d[1] = WA(last));
    samemapshapes();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, p, i;

    if (argc < 2) {
        printf("usage: rimapshapeslib DIR\n");
        return 20;
    }
    msgfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!msgfx || !load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        InitBitMap(&bm[s], DEPTH, BMW, BMH);
        for (p = 0; p < DEPTH; p++)
            bm[s].Planes[p] = AllocRaster(BMW, BMH);
        InitRastPort(&rp[s]);
        rp[s].BitMap = &bm[s];
        win[s].RPort = &rp[s];
        winobj[s][0] = &win[s];
    }
    makeshape(0, 16, 16, 3);
    makeshape(1, 24, 11, 2);
    makeshape(2, 32, 7, 1);
    makeshape(3, 48, 21, 3);
    makeshape(4, 17, 5, 2);
    makeshape(5, 40, 33, 3);
    clearbitmap();

    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);

    setnum(NMS);
    samemapshapes();
    for (i = 0; i < NSH; i++)
        tomap(i, i + 1);
    tomap(3, 12);
    samemapshapes();

    ROUT = "RemapMapShape";
    strcpy(VEC, "");
    DO2(ENT(LIB, REMAP, 0), R[cur].d[0] = WA(1); R[cur].d[1] = WA(2); R[cur].d[2] = WA(5));
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
    DO2(ENT(LIB, FINIT, 0), (void)0);

    for (s = 0; s < 2; s++) {
        for (p = 0; p < DEPTH; p++)
            FreeRaster(bm[s].Planes[p], BMW, BMH);
        for (i = 0; i < NSH; i++)
            FreeMem(shdata[s][i], shsize[i]);
    }
    CloseLibrary(msgfx);
    return rwe_done(LIB);
}
