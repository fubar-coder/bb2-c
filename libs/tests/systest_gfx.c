/*
 * systest_gfx - the drawing commands of 2dlib (Line, Box, Boxf, Circle,
 *   Ellipse, Circlef, Ellipsef, Poly, Polyf), the display and palette
 *   commands of palettelib (PalRGB, AGAPalRGB, FadePalette, ShowPalette,
 *   Use Palette, RGB, Red, Green, Blue), blitzcoplib (Show, ShowF, ShowB),
 *   spriteslib (ShowSprite), displaylib (DisplayBitMap, DisplaySprite,
 *   DisplayPalette) and scrolllib (Scroll) on the Amiga: one set (the
 *   original from acidlibs or the C build, as LIBSET says), observed as
 *   cases (sysload.h; the expected data libs/tests/expected/systest_gfx.txt).
 *
 *   systest_gfx DIR
 *
 * The objects are made here: a bitmap of 64 x 24 pixels and 3 planes in
 * chip memory (and a second one as the source of Scroll), palettes,
 * sprites, a slice and a coplist whose copper moves are buffers of the
 * test.  Every drawing case starts from the same bitmap contents and
 * writes the planes after the call (the blitter finished); the display
 * commands write their copper moves, observed as values and as offsets
 * from the planes or the sprite data (never an address).  The edge cases:
 * clipping at each edge and outside, the origin of BitMapOrigin, every
 * colour and a negative one (complement), swapped corners, the shifts of
 * Show and DisplayBitMap, the clipping of the sprites at the top and the
 * bottom, attached sprites, the second sprite bank, palettes that grow,
 * a brightness above 1.  The blitter commands run in Blitz mode (the
 * test owns the blitter around the call) and in Amiga mode (the routine
 * owns it, a6 = graphics.library); the display commands in Blitz mode.
 */
#include "sysload.h"
#include <exec/memory.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static struct regs R, IN;
static ULONG POP;
static struct GfxBase *gfx;             /* graphics.library (proto: GRAPHICS_BASE_NAME) */

/* the token table of lib: the code of form `form` of the command `name`,
   or (name NULL) of the nullsub number `form` (counted from 0) */
static UBYTE *cmd_named(struct lib *l, const char *name, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    int nulls = 0;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2;
            if (!name && nulls++ == form)
                return (UBYTE *)((ULONG)L(p + 4) & ~1UL);
            p += 12;
        } else {
            int f = 0;
            if (kind != 8) {
                while (W(p) != 0xffff) {        /* forms */
                    p += 2 + (W(p) & 0xff);
                    p = b + ((p - b + 1) & ~1);
                    while (W(p)) p += 4;
                    p += 2;
                    if (f++ == form)
                        code = (UBYTE *)L(p + 4);
                    p += 12;
                }
                p += 2;
            }
            p += 6;                             /* next.l, num.w */
            if (name && !strcmp((const char *)p, name)) {
                check("cmd_named: the form exists", code != NULL);
                return (UBYTE *)((ULONG)code & ~1UL);
            }
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
    }
    check("cmd_named: the command exists", FALSE);
    return NULL;
}

/* the Blitz mode code of a Blitz/Amiga routine (`Dc $a001:Dc.l code`
   in front of the Amiga mode code) and its Amiga mode code */
static UBYTE *blitzcode(UBYTE *e)
{
    return e && W(e) == 0xa001 ? (UBYTE *)L(e + 2) : e;
}

static UBYTE *amigacode(UBYTE *e)
{
    return e && W(e) == 0xa001 ? e + 6 : e;
}

/* call fn with the registers `setup` makes in R (the others markers);
   amiga: in Amiga mode (a6 = graphics.library), else in Blitz mode, the
   test owning the blitter around the call; the blitter has finished when
   the kept case is written */
#define CALL(fn, amiga, setup) do {                                         \
        markregs(&R);                                                       \
        setup;                                                              \
        if (amiga)                                                          \
            R.a[6] = (ULONG)gfx;                                            \
        IN = R;                                                             \
        if (!(amiga))                                                       \
            OwnBlitter();                                                   \
        callstk((fn), &R, NULL, 0, &POP);                                   \
        WaitBlit();                                                         \
        if (!(amiga))                                                       \
            DisownBlitter();                                                \
        obs_kept(&IN, &R);                                                  \
        OBS("popped", POP);                                                 \
    } while (0)

static ULONG seed = 0x2468ace0;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

#define N(lib, name, form) cmd_named(getlib(&loaded, lib), (name), (form))
#define NULLSUB(lib, n) cmd_named(getlib(&loaded, lib), NULL, (n))
#define CHIP 0xdff000UL
#define VECF(...) snprintf(VEC, sizeof VEC, __VA_ARGS__)

/* words and longs of the objects at byte offsets */
typedef UWORD __attribute__((may_alias)) AWORD;
typedef ULONG __attribute__((may_alias)) ALONG;
#define PW(p, o, v) (*(AWORD *)((UBYTE *)(p) + (o)) = (UWORD)(v))
#define PL(p, o, v) (*(ALONG *)((UBYTE *)(p) + (o)) = (ULONG)(v))
#define GW(p, o) (*(AWORD *)((UBYTE *)(p) + (o)))
#define GL(p, o) (*(ALONG *)((UBYTE *)(p) + (o)))

/* x and y with junk in the upper words (only the low words count) */
#define JX(x) ((ULONG)(UWORD)(x) | 0xabcd0000UL)
#define JY(y) ((ULONG)(UWORD)(y) | 0x12340000UL)

/* ------------------------------------------------------------ the objects */

#define BPR 8
#define ROWS 24
#define DEPTH 3
#define PLANE (BPR * ROWS)

static UBYTE *chipmem;
static ULONG chipused;

static UBYTE *chip(ULONG n)
{
    UBYTE *p = chipmem + chipused;
    chipused += (n + 3) & ~3UL;
    return p;
}

/* a struct bb_bitmap (64 bytes) on planes in chip memory */
struct bitmap {
    ULONG obj[16];
    UBYTE *planes;
};
static struct bitmap bm, src;

static void mkbitmap(struct bitmap *b)
{
    UBYTE *o = (UBYTE *)b->obj;
    int i;
    memset(b->obj, 0, sizeof b->obj);
    b->planes = chip(PLANE * DEPTH);
    PW(o, 0, BPR);
    PW(o, 2, ROWS);
    PW(o, 4, DEPTH);                            /* flags 0, depth */
    for (i = 0; i < DEPTH; i++)
        PL(o, 8 + 4 * i, b->planes + i * PLANE);
    PW(o, 56, BPR * 8);                         /* width */
    PW(o, 58, ROWS);                            /* height */
    PW(o, 62, 1);                               /* set up */
}

static void origin(struct bitmap *b, WORD x, WORD y)
{
    PW(b->obj, 48, x);
    PW(b->obj, 50, y);
}

/* the bitmap contents every case starts from: some bits set */
static void fill(struct bitmap *b, ULONG s)
{
    int i;
    seed = s;
    for (i = 0; i < PLANE * DEPTH; i++)
        b->planes[i] = (UBYTE)(rnd() & rnd() & 0xff);
}

#define PLANES() OBSBYTES("planes", bm.planes, PLANE * DEPTH)

/* ------------------------------------------------------------- 2dlib */

/* one drawing call in both modes: `bits` the bitmap contents (seed) */
#define DRAW(code, amode, seedv, setup) do {                                \
        fill(&bm, (seedv));                                                 \
        CALL((code), (amode), R.a[2] = CHIP; R.a[3] = (ULONG)bm.obj; setup); \
        PLANES();                                                           \
    } while (0)

struct seg { WORD x1, y1, x2, y2; };
/* inside, along the edges, clipped at each edge, outside, a point,
   vertical and horizontal, every octant */
static const struct seg segs[] = {
    { 3, 2, 40, 15 }, { 40, 15, 3, 2 }, { 10, 20, 50, 1 }, { 5, 5, 5, 20 },
    { 0, 0, 63, 0 }, { 0, 23, 63, 23 }, { 0, 0, 0, 23 }, { 63, 0, 63, 23 },
    { -10, 5, 30, 12 }, { 30, 12, 80, 20 }, { 20, -8, 30, 10 }, { 25, 10, 35, 40 },
    { -20, -20, 90, 50 }, { -5, -5, -1, -1 }, { 70, 2, 90, 20 }, { 12, 12, 12, 12 },
    { 31, 3, 32, 22 }, { 2, 11, 61, 12 }, { 60, 22, 1, 1 }, { 63, 23, 0, 0 },
};
#define NSEGS (sizeof segs / sizeof segs[0])
static const WORD cols[] = { 0, 1, 2, 5, 7, -1 };
#define NCOLS (sizeof cols / sizeof cols[0])

static void test_lines(void)
{
    unsigned i, k;
    int a;

    for (a = 0; a < 2; a++) {
        UBYTE *ln = N("2dlib", "Line", 0), *lt = N("2dlib", "Line", 1);
        UBYTE *lw = N("2dlib", "Line", 2), *bx = N("2dlib", "Box", 0);
        UBYTE *bf = N("2dlib", "Boxf", 0);
        UBYTE *l0 = a ? amigacode(ln) : blitzcode(ln), *l1 = a ? amigacode(lt) : blitzcode(lt);
        UBYTE *l2 = a ? amigacode(lw) : blitzcode(lw), *b0 = a ? amigacode(bx) : blitzcode(bx);
        UBYTE *f0 = a ? amigacode(bf) : blitzcode(bf);
        int o;

        for (o = 0; o < 2; o++) {
            origin(&bm, o ? 3 : 0, o ? -2 : 0);
            ROUT = a ? "Line (Amiga mode)" : "Line";
            for (i = 0; i < NSEGS; i++) {
                k = i % NCOLS;
                VECF("origin %d %d,%d-%d,%d colour %d", o, segs[i].x1, segs[i].y1, segs[i].x2,
                     segs[i].y2, cols[k]);
                DRAW(l0, a, 1, R.d[0] = JX(segs[i].x1); R.d[1] = JY(segs[i].y1);
                     R.d[2] = JX(segs[i].x2); R.d[3] = JY(segs[i].y2);
                     R.d[4] = 0xfff00000UL | (UWORD)cols[k]);
            }
            /* Line x,y,colour: from the end of the last line */
            ROUT = a ? "Line to (Amiga mode)" : "Line to";
            for (i = 0; i < 6; i++) {
                VECF("origin %d to %d,%d colour %d", o, segs[i + 8].x2, segs[i + 8].y1,
                     cols[i % NCOLS]);
                DRAW(l1, a, 2, R.d[0] = JX(segs[i + 8].x2); R.d[1] = JY(segs[i + 8].y1);
                     R.d[2] = (UWORD)cols[i % NCOLS]);
            }
            ROUT = a ? "Line width (Amiga mode)" : "Line width";
            for (i = 0; i < 4; i++) {
                VECF("origin %d %d,%d-%d,%d width %u", o, segs[i].x1, segs[i].y1, segs[i].x2,
                     segs[i].y2, i);
                DRAW(l2, a, 3, R.d[0] = segs[i].x1; R.d[1] = segs[i].y1;
                     R.d[2] = segs[i].x2; R.d[3] = segs[i].y2;
                     R.d[4] = (UWORD)cols[(i + 1) % NCOLS]; R.d[5] = i);
            }
            ROUT = a ? "Box (Amiga mode)" : "Box";
            for (i = 0; i < NSEGS; i++) {
                k = (i + 2) % NCOLS;
                VECF("origin %d %d,%d-%d,%d colour %d", o, segs[i].x1, segs[i].y1, segs[i].x2,
                     segs[i].y2, cols[k]);
                DRAW(b0, a, 4, R.d[0] = JX(segs[i].x1); R.d[1] = JY(segs[i].y1);
                     R.d[2] = JX(segs[i].x2); R.d[3] = JY(segs[i].y2);
                     R.d[4] = 0xfff00000UL | (UWORD)cols[k]);
            }
            ROUT = a ? "Boxf (Amiga mode)" : "Boxf";
            for (i = 0; i < NSEGS; i++) {
                k = (i + 3) % NCOLS;
                VECF("origin %d %d,%d-%d,%d colour %d", o, segs[i].x1, segs[i].y1, segs[i].x2,
                     segs[i].y2, cols[k]);
                /* a negative colour: its low word complements (a long) */
                DRAW(f0, a, 5, R.d[0] = JX(segs[i].x1); R.d[1] = JY(segs[i].y1);
                     R.d[2] = JX(segs[i].x2); R.d[3] = JY(segs[i].y2);
                     R.d[4] = (ULONG)(LONG)cols[k]);
            }
        }
        origin(&bm, 0, 0);
    }
}

struct ell { WORD x, y; UWORD rx, ry; };
/* never above the top edge (y - ry >= 0, also with the origin -4,3):
   the original wrote such points far behind the bitmap (#392) */
static const struct ell ells[] = {
    { 32, 12, 10, 10 }, { 32, 12, 20, 6 }, { 10, 9, 4, 9 }, { 8, 8, 8, 8 },
    { 63, 23, 12, 5 }, { 60, 9, 9, 9 }, { 5, 20, 7, 3 }, { -5, 12, 9, 4 },
    { 32, 6, 6, 6 }, { 32, 30, 10, 9 }, { 70, 10, 9, 9 }, { 20, 12, 0, 0 },
    { 20, 12, 1, 0 }, { 20, 12, 0, 3 }, { 30, 20, 40, 20 }, { 16, 16, 15, 2 },
};
#define NELLS (sizeof ells / sizeof ells[0])

static void test_ellipses(void)
{
    unsigned i, k;
    int a, o;
    UBYTE *ce = N("2dlib", "Circle", 0), *cc = N("2dlib", "Circle", 1);

    /* Circle / Ellipse: the CPU, no Blitz/Amiga forms */
    for (o = 0; o < 2; o++) {
        origin(&bm, o ? -4 : 0, o ? 3 : 0);
        ROUT = "Circle";
        for (i = 0; i < NELLS; i++) {
            k = i % NCOLS;
            VECF("origin %d %d,%d r %u colour %d", o, ells[i].x, ells[i].y, ells[i].rx, cols[k]);
            DRAW(cc, 0, 6, R.d[0] = JX(ells[i].x); R.d[1] = JY(ells[i].y);
                 R.d[2] = JX(ells[i].rx); R.d[3] = 0xfff00000UL | (UWORD)cols[k]);
        }
        ROUT = "Ellipse";
        for (i = 0; i < NELLS; i++) {
            k = (i + 1) % NCOLS;
            VECF("origin %d %d,%d r %u,%u colour %d", o, ells[i].x, ells[i].y, ells[i].rx,
                 ells[i].ry, cols[k]);
            DRAW(ce, 0, 7, R.d[0] = JX(ells[i].x); R.d[1] = JY(ells[i].y);
                 R.d[2] = JX(ells[i].rx); R.d[3] = JY(ells[i].ry);
                 R.d[4] = 0xfff00000UL | (UWORD)cols[k]);
        }
    }
    origin(&bm, 0, 0);
    /* Circlef / Ellipsef: the fill buffer and the blitter (no origin) */
    for (a = 0; a < 2; a++) {
        UBYTE *fe = N("2dlib", "Circlef", 0), *fc = N("2dlib", "Circlef", 1);
        UBYTE *e0 = a ? amigacode(fe) : blitzcode(fe), *c0 = a ? amigacode(fc) : blitzcode(fc);

        ROUT = a ? "Circlef (Amiga mode)" : "Circlef";
        for (i = 0; i < NELLS; i++) {
            k = (i + 2) % NCOLS;
            VECF("%d,%d r %u colour %d", ells[i].x, ells[i].y, ells[i].rx, cols[k]);
            DRAW(c0, a, 8, R.d[0] = JX(ells[i].x); R.d[1] = JY(ells[i].y);
                 R.d[2] = JX(ells[i].rx); R.d[3] = 0xfff00000UL | (UWORD)cols[k]);
        }
        ROUT = a ? "Ellipsef (Amiga mode)" : "Ellipsef";
        for (i = 0; i < NELLS; i++) {
            k = (i + 3) % NCOLS;
            VECF("%d,%d r %u,%u colour %d", ells[i].x, ells[i].y, ells[i].rx, ells[i].ry,
                 cols[k]);
            DRAW(e0, a, 9, R.d[0] = JX(ells[i].x); R.d[1] = JY(ells[i].y);
                 R.d[2] = JX(ells[i].rx); R.d[3] = JY(ells[i].ry);
                 R.d[4] = 0xfff00000UL | (UWORD)cols[k]);
        }
    }
}

/* polygons: count and points (words x, y) */
struct poly { UWORD n; WORD p[16]; };
static const struct poly polys[] = {
    { 3, { 10, 10, 60, 20, 30, 50 } },
    { 3, { 2, 2, 40, 5, 20, 20 } },
    { 3, { 20, 20, 40, 5, 2, 2 } },
    { 4, { 5, 3, 55, 3, 55, 20, 5, 20 } },
    { 4, { 32, 0, 63, 12, 32, 23, 0, 12 } },
    { 5, { 10, 2, 50, 4, 60, 18, 30, 22, 4, 15 } },
    { 3, { -10, -5, 80, 10, 20, 40 } },
    { 4, { 20, -10, 40, -10, 45, 30, 15, 30 } },   /* no vertical edge cut (#1033) */
    { 3, { 70, 2, 90, 10, 75, 20 } },
    { 6, { 5, 5, 20, 2, 35, 8, 50, 2, 58, 20, 10, 21 } },
    { 3, { 10, 10, 10, 20, 10, 15 } },
    { 2, { 4, 4, 50, 18 } },
};
#define NPOLYS (sizeof polys / sizeof polys[0])
static WORD ptbuf[16];

static void test_polys(void)
{
    unsigned i, k;
    int a;

    for (a = 0; a < 2; a++) {
        UBYTE *po = N("2dlib", "Poly", 0), *pf = N("2dlib", "Polyf", 0);
        UBYTE *pf2 = N("2dlib", "Polyf", 1);
        UBYTE *p0 = a ? amigacode(po) : blitzcode(po), *f0 = a ? amigacode(pf) : blitzcode(pf);
        UBYTE *f1 = a ? amigacode(pf2) : blitzcode(pf2);

        for (i = 0; i < NPOLYS; i++) {
            k = i % NCOLS;
            memcpy(ptbuf, polys[i].p, sizeof ptbuf);
            ROUT = a ? "Poly (Amiga mode)" : "Poly";
            VECF("polygon %u colour %d", i, cols[k]);
            DRAW(p0, a, 10, R.d[0] = 0xfff00000UL | polys[i].n; R.d[1] = (ULONG)ptbuf;
                 R.d[2] = 0xfff00000UL | (UWORD)cols[k]);
            if (polys[i].n < 3)
                continue;
            ROUT = a ? "Polyf (Amiga mode)" : "Polyf";
            VECF("polygon %u colour %d", i, cols[(k + 1) % NCOLS]);
            DRAW(f0, a, 11, R.d[0] = 0xfff00000UL | polys[i].n; R.d[1] = (ULONG)ptbuf;
                 R.d[2] = 0xfff00000UL | (UWORD)cols[(k + 1) % NCOLS]);
            ROUT = a ? "Polyf two colours (Amiga mode)" : "Polyf two colours";
            VECF("polygon %u colours %d %d", i, cols[(k + 2) % NCOLS], cols[(k + 3) % NCOLS]);
            DRAW(f1, a, 12, R.d[0] = 0xfff00000UL | polys[i].n; R.d[1] = (ULONG)ptbuf;
                 R.d[2] = 0xfff00000UL | (UWORD)cols[(k + 2) % NCOLS];
                 R.d[3] = 0xfff00000UL | (UWORD)cols[(k + 3) % NCOLS]);
        }
    }
}

/* ------------------------------------------------------------ palettelib */

/* a palette object (128 bytes) and its colours: the count, the colours
   (12 bytes each) and the object behind the data pointer */
static ULONG pal[4][32];

static void obs_palette(int n)
{
    UBYTE *o = (UBYTE *)pal[n], *d = (UBYTE *)GL(o, 0);
    UWORD c = GW(o, 4);

    OBS("colours", c);
    OBSBYTES("object", o + 4, 124);
    if (d)
        OBSBYTES("data", d, 4 + 12 * (ULONG)c);
    else
        OBS("no data", 0);
}

/* a slice (64 bytes) with its colour moves in buf (32 colours) */
static ULONG slice[16];
static UWORD colbuf[64];
static ULONG sprbuf[8 * 4 + 8];

static void mkslice(UWORD colours)
{
    int i;
    memset(slice, 0, sizeof slice);
    PW(slice, 0, 44);                           /* y */
    PW(slice, 8, colours);
    PL(slice, 14, sprbuf);
    PL(slice, 18, colbuf);
    PW(slice, 32, 200);                         /* height */
    for (i = 0; i < 64; i += 2) {
        colbuf[i] = (UWORD)(0x180 + i);
        colbuf[i + 1] = (UWORD)(0x0f0f ^ (i * 0x111));
    }
}

static void test_palettelib(void)
{
    static const UWORD rgb[][4] = {
        { 0, 15, 0, 0 }, { 1, 0, 15, 0 }, { 2, 0, 0, 15 }, { 3, 1, 2, 3 }, { 7, 15, 15, 15 },
        { 4, 9, 8, 7 }, { 9, 5, 10, 12 }, { 31, 6, 7, 8 }, { 12, 0, 0, 0 },
    };
    static const ULONG brs[] = { 0, 0x8000, 0x10000, 0x18000, 0x40000, 0x2345 };
    UBYTE *palrgb = N("palettelib", "PalRGB", 0), *aga = N("palettelib", "AGAPalRGB", 0);
    UBYTE *fade = N("palettelib", "FadePalette", 0);
    UBYTE *show = blitzcode(N("palettelib", "ShowPalette", 0));
    UBYTE *use = blitzcode(NULLSUB("palettelib", 5));
    UBYTE *mode = N("palettelib", "NewPaletteMode", 0);
    UBYTE *rgbc = blitzcode(N("palettelib", "RGB", 0));
    UBYTE *red = blitzcode(N("palettelib", "Red", 0));
    UBYTE *green = blitzcode(N("palettelib", "Green", 0));
    UBYTE *blue = blitzcode(N("palettelib", "Blue", 0));
    unsigned i, k, s;

    memset(pal, 0, sizeof pal);
    /* PalRGB into an empty palette: it grows (the old colours copied) */
    ROUT = "PalRGB";
    for (i = 0; i < sizeof rgb / sizeof rgb[0]; i++) {
        VECF("palette 0 colour %u %u,%u,%u", rgb[i][0], rgb[i][1], rgb[i][2], rgb[i][3]);
        CALL(palrgb, 0, R.d[1] = 0xfff00000UL | rgb[i][0]; R.d[2] = 0xfff00000UL | rgb[i][1];
             R.d[3] = 0xfff00000UL | rgb[i][2]; R.d[4] = 0xfff00000UL | rgb[i][3];
             R.a[0] = (ULONG)pal[0]);
        obs_palette(0);
    }
    ROUT = "AGAPalRGB";
    for (i = 0; i < sizeof rgb / sizeof rgb[0]; i++) {
        VECF("palette 1 colour %u %u,%u,%u", rgb[i][0] + 1, rgb[i][1] * 17, rgb[i][2] * 13,
             rgb[i][3] * 11);
        CALL(aga, 0, R.d[1] = rgb[i][0] + 1; R.d[2] = rgb[i][1] * 17; R.d[3] = rgb[i][2] * 13;
             R.d[4] = rgb[i][3] * 11; R.a[0] = (ULONG)pal[1]);
        obs_palette(1);
    }
    /* palette 2: 8 colours as the benchmark's */
    for (i = 0; i < 8; i++)
        CALL(palrgb, 0, R.d[1] = 7 - i; R.d[2] = 15 - i; R.d[3] = i; R.d[4] = (i * 3) & 15;
             R.a[0] = (ULONG)pal[2]);
    for (i = 0; i < 3; i++)
        CALL(palrgb, 0, R.d[1] = i; R.d[2] = 1; R.d[3] = 2; R.d[4] = 3; R.a[0] = (ULONG)pal[3]);
    ROUT = "FadePalette";
    for (k = 0; k < sizeof brs / sizeof brs[0]; k++)
        for (s = 0; s < 3; s++) {
            static const int sd[3][2] = { { 0, 1 }, { 1, 0 }, { 2, 0 } };
            VECF("palette %d to %d brightness $%lX", sd[s][0], sd[s][1], brs[k]);
            CALL(fade, 0, R.d[2] = brs[k]; R.a[0] = (ULONG)pal[sd[s][0]];
                 R.a[1] = (ULONG)pal[sd[s][1]]);
            obs_palette(sd[s][1]);
        }
    /* ShowPalette and Use Palette (Blitz mode): the colour moves of the
       slice, as many as both have */
    for (s = 0; s < 4; s++) {
        static const UWORD ncol[4] = { 32, 8, 4, 1 };
        for (k = 0; k < 4; k++) {
            ROUT = "ShowPalette";
            VECF("palette %u slice colours %u", k, ncol[s]);
            mkslice(ncol[s]);
            CALL(show, 0, R.a[0] = (ULONG)slice; R.a[3] = (ULONG)pal[k]);
            OBSBYTES("colour moves", colbuf, sizeof colbuf);
        }
    }
    for (s = 0; s < 2; s++) {
        ROUT = "NewPaletteMode";
        VECF("%u", s);
        CALL(mode, 0, R.d[0] = 0xffff0000UL | s);
        for (k = 0; k < 4; k++) {
            ROUT = "Use Palette";
            VECF("mode %u palette %u", s, k);
            mkslice(16);
            CALL(use, 0, R.a[0] = (ULONG)slice; R.a[3] = (ULONG)pal[k]);
            OBSBYTES("colour moves", colbuf, sizeof colbuf);
        }
    }
    ROUT = "NewPaletteMode";
    strcpy(VEC, "0 again");
    CALL(mode, 0, R.d[0] = 0);
    /* RGB, Red, Green, Blue (Blitz mode): the colour moves */
    mkslice(32);
    for (i = 0; i < sizeof rgb / sizeof rgb[0]; i++) {
        ROUT = "RGB";
        VECF("colour %u %u,%u,%u", rgb[i][0], rgb[i][1], rgb[i][2], rgb[i][3]);
        CALL(rgbc, 0, R.d[0] = 0xfff00000UL | rgb[i][0]; R.d[1] = rgb[i][1];
             R.d[2] = rgb[i][2]; R.d[3] = rgb[i][3]; R.a[0] = (ULONG)slice);
        OBSBYTES("colour moves", colbuf, sizeof colbuf);
    }
    for (i = 0; i < 32; i += 3) {
        VECF("colour %u", i);
        ROUT = "Red";
        CALL(red, 0, R.d[0] = 0xabcd0000UL | i; R.a[0] = (ULONG)slice);
        OBS("d0.w", (UWORD)R.d[0]);
        ROUT = "Green";
        CALL(green, 0, R.d[0] = 0xabcd0000UL | i; R.a[0] = (ULONG)slice);
        OBS("d0.w", (UWORD)R.d[0]);
        ROUT = "Blue";
        CALL(blue, 0, R.d[0] = 0xabcd0000UL | i; R.a[0] = (ULONG)slice);
        OBS("d0.w", (UWORD)R.d[0]);
    }
}

/* -------------------------------------------- blitzcoplib Show, ShowF, ShowB */

/* the bitplane moves of a slice: 8 pairs (high, low) of 8 bytes each */
static UWORD bplbuf[8 * 4];
static UWORD con1;

/* the moves as offsets from the planes of bm, one case; a move the
   command did not write keeps the test's value (no address: the case
   must not depend on where the program is) */
static void obs_moves(const UWORD *m, int n, int step)
{
    ULONG off[8];
    int i;
    for (i = 0; i < n; i++, m += step / 2) {
        ULONG p = (ULONG)m[1] << 16 | m[3];
        LONG d = (LONG)(p - (ULONG)bm.planes);  /* before the planes: Show at x 0 */

        off[i] = d > -0x10000 && d < 0x10000 ? (ULONG)d : p;
    }
    OBSBYTES("plane moves", off, 4 * n);
}

static void mkshowslice(UWORD flags, UWORD planes)
{
    int i;
    memset(slice, 0, sizeof slice);
    PW(slice, 0, 44);
    PW(slice, 2, flags);
    PW(slice, 4, planes);
    PL(slice, 10, bplbuf);
    PL(slice, 22, &con1);
    for (i = 0; i < 8 * 4; i += 2) {
        bplbuf[i] = (UWORD)(0xe0 + i);
        bplbuf[i + 1] = 0x5a5a;
    }
    con1 = 0x1234;
}

static void test_show(void)
{
    static const WORD sx[] = { 0, 1, 7, 15, 16, 17, 31, 40 }, sy[] = { 0, 3 };
    static const UWORD flags[] = { 0, 1, 8, 9 };
    unsigned f, i, j;

    for (f = 0; f < sizeof flags / sizeof flags[0]; f++) {
        ROUT = "Show";
        VECF("flags %u at 0,0", flags[f]);
        mkshowslice(flags[f], DEPTH);
        CALL(N("blitzcoplib", "Show", 0), 0, R.a[0] = (ULONG)bm.obj; R.a[1] = (ULONG)slice);
        obs_moves(bplbuf, DEPTH, 8);
        OBS("BPLCON1", con1);
        OBS("BPLCON1 copy", GW(slice, 30));
        for (i = 0; i < sizeof sx / sizeof sx[0]; i++)
            for (j = 0; j < sizeof sy / sizeof sy[0]; j++) {
                VECF("flags %u at %d,%d", flags[f], sx[i], sy[j]);
                mkshowslice(flags[f], DEPTH);
                CALL(N("blitzcoplib", "Show", 1), 0, R.d[1] = JX(sx[i]); R.d[2] = JY(sy[j]);
                     R.a[0] = (ULONG)bm.obj; R.a[1] = (ULONG)slice);
                obs_moves(bplbuf, DEPTH, 8);
                OBS("BPLCON1", con1);
                OBS("BPLCON1 copy", GW(slice, 30));
            }
    }
    /* dual playfield: ShowF and ShowB in a slice of 6 planes (planeshi 0:
       every second move) and with planeshi set */
    for (f = 0; f < 2; f++)
        for (i = 0; i < 4; i++) {
            UWORD planes = (UWORD)(f ? 0xc006 : 6);
            ROUT = "ShowF";
            VECF("planes $%X at %d,%d back %d", planes, sx[i], sy[i & 1], sx[i + 2]);
            mkshowslice(0, planes);
            CALL(N("blitzcoplib", "ShowF", 2), 0, R.d[1] = JX(sx[i]); R.d[2] = JY(sy[i & 1]);
                 R.d[3] = JX(sx[i + 2]); R.a[0] = (ULONG)bm.obj; R.a[1] = (ULONG)slice);
            obs_moves(bplbuf, 6, 8);
            OBS("BPLCON1", con1);
            ROUT = "ShowB";
            mkshowslice(1, planes);
            CALL(N("blitzcoplib", "ShowB", 2), 0, R.d[1] = JX(sx[i]); R.d[2] = JY(sy[i & 1]);
                 R.d[3] = JX(sx[i + 2]); R.a[0] = (ULONG)bm.obj; R.a[1] = (ULONG)slice);
            obs_moves(bplbuf, 6, 8);
            OBS("BPLCON1", con1);
        }
    ROUT = "ShowF";
    strcpy(VEC, "at 0,0");
    mkshowslice(0, 6);
    CALL(N("blitzcoplib", "ShowF", 0), 0, R.a[0] = (ULONG)bm.obj; R.a[1] = (ULONG)slice);
    obs_moves(bplbuf, 6, 8);
    OBS("BPLCON1", con1);
    ROUT = "ShowB";
    mkshowslice(0, 6);
    CALL(N("blitzcoplib", "ShowB", 1), 0, R.d[1] = 9; R.d[2] = 2; R.a[0] = (ULONG)bm.obj;
         R.a[1] = (ULONG)slice);
    obs_moves(bplbuf, 6, 8);
    OBS("BPLCON1", con1);
}

/* --------------------------------------------------- spriteslib ShowSprite */

/* sprites (16 bytes): data, height + 1, count, flags, bytes per image,
   handle */
static ULONG sprite[2][4];
static UBYTE sprdata[512];

static void mksprite(ULONG *s, UWORD height, UWORD count, UWORD flags, WORD hx, WORD hy)
{
    memset(s, 0, 16);
    PL(s, 0, sprdata);
    PW(s, 4, height);
    PW(s, 6, count);
    PW(s, 8, flags);
    PW(s, 10, height * 4);
    PW(s, 12, hx);
    PW(s, 14, hy);
}

/* the sprite moves of n channels, one case: per channel pos and ctl
   (a long) and the data pointer as an offset from the sprite data
   ($FFFFFFFF: 0; a channel the command did not write keeps the test's
   value: no address in the case) */
static void obs_sprmoves(const char *what, const ULONG *buf, int n)
{
    const UWORD *m = (const UWORD *)buf;
    ULONG rec[16];
    int i;
    for (i = 0; i < n; i++, m += 8) {
        ULONG p = (ULONG)m[5] << 16 | m[7];
        rec[2 * i] = (ULONG)m[1] << 16 | m[3];
        rec[2 * i + 1] = !p ? 0xffffffffUL
                         : p - (ULONG)sprdata < 0x1000 ? p - (ULONG)sprdata : p;
    }
    OBSBYTES(what, rec, 8 * n);
}

static void clrsprbuf(void)
{
    int i;
    for (i = 0; i < 8 * 4; i++)
        sprbuf[i] = 0x01400000UL | (i << 17) | 0x3333;
}

static void test_showsprite(void)
{
    static const WORD xy[][2] = {
        { 100, 100 }, { 101, 50 }, { 0, 0 }, { 300, 10 }, { 100, -3 }, { 100, -15 },
        { 100, -16 }, { 100, -40 }, { 100, 190 }, { 100, 196 }, { 100, 200 }, { 100, 230 },
        { -20, 30 }, { 255, 211 }, { 7, 255 }, { 33, 1 },
    };
    unsigned i, s, f;
    UBYTE *ss = N("spriteslib", "ShowSprite", 0);

    ROUT = "SpriteMode";
    strcpy(VEC, "0");
    CALL(N("spriteslib", "SpriteMode", 0), 0, R.d[0] = 0);
    for (s = 0; s < 3; s++)
        for (f = 0; f < 2; f++) {
            /* 16 high, 1 sprite; 32 high, 2 sprites; attached: 2 pairs */
            static const UWORD h[3] = { 17, 33, 9 }, c[3] = { 1, 2, 4 }, fl[3] = { 16, 32, 0x8020 };
            mksprite(sprite[0], h[s], c[s], fl[s], f ? 3 : 0, f ? -2 : 0);
            for (i = 0; i < sizeof xy / sizeof xy[0]; i++) {
                ROUT = "ShowSprite";
                VECF("sprite %u handle %u at %d,%d channel %u", s, f, xy[i][0], xy[i][1], i & 3);
                mkslice(8);
                PW(slice, 2, (i & 1) ? 0x10 : 0);       /* flagslo bit 4: +2 lines */
                clrsprbuf();
                /* y: the upper word clean (#283: the original added it when
                   clipping at the top) */
                CALL(ss, 0, R.d[1] = JX(xy[i][0]); R.d[2] = (UWORD)xy[i][1];
                     R.d[3] = 0xabcd0000UL | (i & 3); R.a[2] = (ULONG)slice;
                     R.a[3] = (ULONG)sprite[0]);
                obs_sprmoves("sprite moves", sprbuf, 8);
            }
        }
}

/* ---------------------------------------------------------- displaylib */

/* a coplist (128 bytes) with its moves in buffers: the bitplane moves
   (behind 10 bytes of BPLCON1/modulo moves), the sprites, the custom
   sprites and the colours */
static ULONG cop[32];
static UWORD dbpl[8 * 4 + 8];
static ULONG dspr[8 * 4], dcspr[8 * 4];
static ULONG dcol[300];

static void mkcop(UWORD numbp, UWORD res, UWORD ham8, UWORD aga, UWORD numcols)
{
    int i;
    memset(cop, 0, sizeof cop);
    PL(cop, 0, 1);                              /* size: initialised */
    PL(cop, 8, dcol);
    PL(cop, 12, dspr);
    PL(cop, 20, dbpl + 6);
    PL(cop, 32, dcol + 290);                    /* dob: the end of the moves */
    PW(cop, 38, numbp);
    PW(cop, 42, 40);                            /* fetchwid */
    PW(cop, 44, 15);                            /* xand */
    PW(cop, 46, 3);                             /* xshift */
    PW(cop, 48, 44);                            /* ypos */
    PW(cop, 50, 200);                           /* height */
    PW(cop, 52, res);
    PW(cop, 54, 8);                             /* numsprites */
    PW(cop, 56, numcols);
    PW(cop, 70, 0x0c40);                        /* bplcon3 */
    PW(cop, 72, aga);
    PW(cop, 74, (res & 1) ? 1 : 0);             /* resshift */
    PW(cop, 84, 2);                             /* spif */
    PW(cop, 88, 8);                             /* sspwid */
    PW(cop, 90, ham8);
    PW(cop, 126, (res & 1) ? 1 : 0);            /* spri */
    for (i = 0; i < 8 * 4 + 8; i++)
        dbpl[i] = (UWORD)(0x7000 + i);
    for (i = 0; i < 8 * 4; i++) {
        dspr[i] = 0x01200000UL | i;
        dcspr[i] = 0x01300000UL | i;
    }
    for (i = 0; i < 300; i++)
        dcol[i] = 0x01fe0000UL | i;
}

/* a bitmap of 8 planes for HAM8: the planes of bm, then 5 more */
static ULONG bm8[16];

static void test_displaybitmap(void)
{
    static const ULONG qx[] = { 0, 0x10000, 0x70000, 0xf8000, 0x100000, 0x115555, 0x1f0000 };
    static const UWORD dy[] = { 0, 5 };
    static const UWORD res[] = { 0, 1, 0x10, 0x11 };
    UBYTE *d0 = N("displaylib", "DisplayBitMap", 0), *d1 = N("displaylib", "DisplayBitMap", 1);
    unsigned r, h, i, j;

    memcpy(bm8, bm.obj, sizeof bm8);
    PW(bm8, 4, 8);
    for (i = 3; i < 8; i++)
        PL(bm8, 8 + 4 * i, bm.planes + i * 64);
    for (r = 0; r < sizeof res / sizeof res[0]; r++)
        for (h = 0; h < 2; h++) {
            UWORD n = (UWORD)(h ? 8 : DEPTH);
            const ULONG *b = h ? bm8 : bm.obj;
            ROUT = "DisplayBitMap";
            VECF("res $%X ham8 %u", res[r], h);
            mkcop(n, res[r], h ? 0xffff : 0, 0, 8);
            CALL(d0, 0, R.d[2] = 0x11112222UL; R.d[3] = 0x33334444UL; R.a[0] = (ULONG)b;
                 R.a[3] = (ULONG)cop);
            OBSBYTES("modulo moves", dbpl, 12);
            obs_moves(dbpl + 6, n, 8);
            OBS("x0", GL(cop, 62));
            OBS("x1", GL(cop, 66));
            for (i = 0; i < sizeof qx / sizeof qx[0]; i++)
                for (j = 0; j < sizeof dy / sizeof dy[0]; j++) {
                    ROUT = "DisplayBitMap x,y";
                    VECF("res $%X ham8 %u at $%lX,%u", res[r], h, qx[i], dy[j]);
                    mkcop(n, res[r], h ? 0xffff : 0, 0, 8);
                    CALL(d1, 0, R.d[2] = qx[i]; R.d[3] = 0xfff00000UL | dy[j]; R.a[0] = (ULONG)b;
                         R.a[3] = (ULONG)cop);
                    OBSBYTES("modulo moves", dbpl, 12);
                    obs_moves(dbpl + 6, n, 8);
                    OBS("x0", GL(cop, 62));
                    OBS("x1", GL(cop, 66));
                }
        }
    ROUT = "DisplayBitMap";
    strcpy(VEC, "no planes");
    mkcop(0, 0, 0, 0, 8);
    CALL(d0, 0, R.a[0] = (ULONG)bm.obj; R.a[3] = (ULONG)cop);
    OBSBYTES("modulo moves", dbpl, 12);
    OBS("x0", GL(cop, 62));
}

static void test_displaysprite(void)
{
    static const LONG xy[][2] = {
        { 100 << 16, 100 }, { (101 << 16) | 0x8000, 50 }, { 0, 0 }, { 300 << 16, 10 },
        { 100 << 16, -3 }, { 100 << 16, -15 }, { 100 << 16, -40 }, { 100 << 16, 190 },
        { 100 << 16, 200 }, { 100 << 16, 230 }, { -20 << 16, 30 }, { 255 << 16, 211 },
    };
    static const UWORD chans[] = { 0, 3, 7, 8, 9 };
    UBYTE *ds = N("displaylib", "DisplaySprite", 0);
    unsigned s, i, c, r;

    for (r = 0; r < 2; r++)
        for (s = 0; s < 3; s++) {
            static const UWORD h[3] = { 17, 33, 9 }, n[3] = { 1, 2, 4 }, fl[3] = { 16, 32, 0x8020 };
            mksprite(sprite[1], h[s], n[s], fl[s], s ? 2 : 0, s ? -1 : 0);
            for (i = 0; i < sizeof xy / sizeof xy[0]; i++)
                for (c = 0; c < sizeof chans / sizeof chans[0]; c++) {
                    /* a channel from 8 on: the CustomSprites bank (only
                       where the sprite fits; spriteoff only in the normal
                       bank, #1051) */
                    if (chans[c] + n[s] > (chans[c] >= 8 ? 16 : 8))
                        continue;
                    if (chans[c] >= 8) {
                        WORD v = (WORD)(xy[i][1] - (s ? -1 : 0));
                        if (v >= 201 || (WORD)(h[s] + v) <= 0)
                            continue;
                    }
                    ROUT = "DisplaySprite";
                    VECF("res %u sprite %u at $%lX,%ld channel %u", r, s, (ULONG)xy[i][0],
                         (long)xy[i][1], chans[c]);
                    mkcop(DEPTH, r, 0, 0, 8);
                    PL(cop, 92, dcspr);
                    CALL(ds, 0, R.d[2] = (ULONG)xy[i][0]; R.d[3] = JY(xy[i][1]);
                         R.d[4] = 0xfff00000UL | chans[c]; R.a[1] = (ULONG)sprite[1];
                         R.a[3] = (ULONG)cop);
                    obs_sprmoves("sprite moves", dspr, 8);
                    obs_sprmoves("custom sprite moves", dcspr, 8);
                }
        }
}

static void test_displaypalette(void)
{
    static const UWORD ncol[] = { 1, 8, 32 };
    UBYTE *p0 = N("displaylib", "DisplayPalette", 0), *p1 = N("displaylib", "DisplayPalette", 1);
    UBYTE *p2 = N("displaylib", "DisplayPalette", 2);
    unsigned i, k, a;

    /* palette 1 of the palettelib test has 10 colours; one of 40 */
    for (i = 0; i < 40; i++)
        CALL(N("palettelib", "AGAPalRGB", 0), 0, R.d[1] = 39 - i; R.d[2] = i * 6;
             R.d[3] = 255 - i * 5; R.d[4] = (i * 37) & 255; R.a[0] = (ULONG)pal[3]);
    for (a = 0; a < 2; a++)
        for (k = 0; k < sizeof ncol / sizeof ncol[0]; k++)
            for (i = 0; i < 2; i++) {
                UWORD agav = (UWORD)(a ? 0x8000 : 0);
                ULONG *p = i ? pal[3] : pal[1];
                /* only colours the palette has (offset 2 + count) */
                if (!i && ncol[k] > 8)
                    continue;
                ROUT = "DisplayPalette";
                VECF("aga %u colours %u palette %u", a, ncol[k], i);
                mkcop(DEPTH, 0, 0, agav, ncol[k]);
                CALL(p0, 0, R.a[0] = (ULONG)p; R.a[3] = (ULONG)cop);
                OBSBYTES("colour moves", dcol, 4 * 80);
                ROUT = "DisplayPalette offset";
                VECF("aga %u colours %u palette %u from 2", a, ncol[k], i);
                mkcop(DEPTH, 0, 0, agav, ncol[k]);
                CALL(p1, 0, R.d[2] = 0xfff00002UL; R.a[0] = (ULONG)p; R.a[3] = (ULONG)cop);
                OBSBYTES("colour moves", dcol, 4 * 80);
                ROUT = "DisplayPalette offset bank";
                VECF("aga %u colours %u palette %u from 1 bank 3", a, ncol[k], i);
                mkcop(DEPTH, 0, 0, agav, ncol[k]);
                CALL(p2, 0, R.d[2] = 1; R.d[3] = 3; R.a[0] = (ULONG)p; R.a[3] = (ULONG)cop);
                OBSBYTES("colour moves", dcol, 4 * 80);
            }
    /* AGA with 40 colours: two banks */
    ROUT = "DisplayPalette";
    strcpy(VEC, "aga 1 colours 40 palette 1");
    mkcop(DEPTH, 0, 0, 0x8000, 40);
    CALL(p0, 0, R.a[0] = (ULONG)pal[3]; R.a[3] = (ULONG)cop);
    OBSBYTES("colour moves", dcol, sizeof dcol);
}

/* ----------------------------------------------------------- scrolllib */

/* the argument stack of Scroll: -2(a2) width, -4(a2) height */
static UWORD sargs[4];

static void test_scroll(void)
{
    static const WORD sc[][6] = {     /* x1, y1, w, h, x2, y2 */
        { 0, 0, 32, 8, 20, 10 }, { 20, 10, 32, 8, 0, 0 }, { 3, 2, 17, 5, 9, 2 },
        { 9, 2, 17, 5, 3, 2 }, { 1, 1, 40, 12, 2, 3 }, { 15, 7, 1, 1, 48, 16 },
        { 0, 0, 64, 24, 0, 0 }, { 5, 4, 30, 10, 21, 13 }, { 33, 2, 16, 16, 7, 1 },
        { 0, 3, 48, 20, 16, 2 },
    };
    unsigned i, s;
    int a;

    ROUT = "scrolllib init";
    strcpy(VEC, "");
    CALL(NULLSUB("scrolllib", 1), 0, (void)0);
    for (a = 0; a < 2; a++) {
        UBYTE *sc0 = N("scrolllib", "Scroll", 0), *sc1 = N("scrolllib", "Scroll", 1);
        UBYTE *c0 = a ? amigacode(sc0) : blitzcode(sc0), *c1 = a ? amigacode(sc1) : blitzcode(sc1);
        for (s = 0; s < 2; s++)
            for (i = 0; i < sizeof sc / sizeof sc[0]; i++) {
                ROUT = s ? (a ? "Scroll from (Amiga mode)" : "Scroll from")
                         : (a ? "Scroll (Amiga mode)" : "Scroll");
                VECF("%d,%d %dx%d to %d,%d", sc[i][0], sc[i][1], sc[i][2], sc[i][3], sc[i][4],
                     sc[i][5]);
                fill(&bm, 20 + i);
                fill(&src, 40 + i);
                sargs[0] = (UWORD)sc[i][3];
                sargs[1] = (UWORD)sc[i][2];
                /* the source: the used bitmap itself, or another one */
                CALL(s ? c1 : c0, a, R.d[0] = JX(sc[i][0]); R.d[1] = JY(sc[i][1]);
                     R.d[2] = JX(sc[i][4]); R.d[3] = JY(sc[i][5]);
                     R.a[0] = (ULONG)(s ? src.obj : bm.obj); R.a[1] = CHIP;
                     R.a[2] = (ULONG)&sargs[2]; R.a[3] = (ULONG)bm.obj;
                     R.d[6] = (ULONG)gfx);
                PLANES();
            }
    }
}

/* ------------------------------------------------------------------ main */

/* the tokens of the libraries these libraries call and this program does
   not load (intlib, switchlib, iffiolib, ilbmifflib, bitmaplib,
   screenslib, shapeslib, fileiolib: the interrupts and the copper list
   of the display commands, LoadPalette ..., never called here): an RTS
   each */
static const UWORD stubrts[] = { 0x4e75 };
#define NSTUBBASES 8
static struct stub stubs[NSTUBBASES * 32 + 1];

static void mkstubs(void)
{
    static const UWORD bases[NSTUBBASES] = {
        0xc100, 0xc200, 162 << 7, 161 << 7, 160 << 7, 158 << 7, 154 << 7, 124 << 7
    };
    int i, j, n = 0;
    for (i = 0; i < NSTUBBASES; i++)
        for (j = 0; j < 32; j++) {
            stubs[n].tok = (UWORD)(bases[i] + j);
            stubs[n++].code = (APTR)stubrts;
        }
    stubs[n].tok = 0;
    stubs[n].code = NULL;
    sysload_stubs = stubs;
}

static const char *names[] = {
    "memlib", "environlib", "2dlib", "palettelib", "blitzcoplib", "spriteslib", "displaylib",
    "scrolllib", NULL
};

int main(int argc, char **argv)
{
    static const char *const routs[] = {
        "Line", "Line (Amiga mode)", "Line to", "Line to (Amiga mode)", "Line width",
        "Line width (Amiga mode)", "Box", "Box (Amiga mode)", "Boxf", "Boxf (Amiga mode)",
        "Circle", "Ellipse", "Circlef", "Circlef (Amiga mode)", "Ellipsef",
        "Ellipsef (Amiga mode)", "Poly", "Poly (Amiga mode)", "Polyf", "Polyf (Amiga mode)",
        "Polyf two colours", "Polyf two colours (Amiga mode)", "PalRGB", "AGAPalRGB",
        "FadePalette", "ShowPalette", "Use Palette", "NewPaletteMode", "RGB", "Show",
        "ShowF", "ShowB", "SpriteMode", "ShowSprite", "DisplayBitMap", "DisplayBitMap x,y",
        "DisplaySprite", "DisplayPalette", "DisplayPalette offset",
        "DisplayPalette offset bank", "Scroll", "Scroll (Amiga mode)", "Scroll from",
        "Scroll from (Amiga mode)", NULL
    };
    int i;

    if (argc < 2) {
        printf("usage: systest_gfx DIR\n");
        return 20;
    }
    mkstubs();
    if (!load_set(argv[1], names))
        return 20;
    for (i = 0; routs[i]; i++)
        KEEPS_STATEMENT(routs[i], none);
    KEEPS_FUNCTION("Red", none);
    KEEPS_FUNCTION("Green", none);
    KEEPS_FUNCTION("Blue", none);
    KEEPS("scrolllib init", none);
    KEEPS_STATEMENT("FreeFill", none);
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    gfx = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    chipmem = AllocMem(4096, MEMF_CHIP | MEMF_CLEAR);
    if (!gfx || !chipmem) {
        check("graphics.library and the memory", FALSE);
        return 20;
    }
    /* the blits of Scroll read and write back a few words left and right
       of the area: room around the bitmaps */
    chip(64);
    mkbitmap(&bm);
    mkbitmap(&src);
    chip(64);
    check("the chip memory suffices", chipused <= 4096);

    /* the memory of the fill buffer and the palettes: memlib's lists */
    ROUT = "memlib init";
    strcpy(VEC, "");
    CALL(entry_code(getlib(&loaded, "memlib"), 7), 0, (void)0);
    test_lines();
    test_ellipses();
    test_polys();
    ROUT = "FreeFill";
    strcpy(VEC, "");
    CALL(N("2dlib", "FreeFill", 0), 0, (void)0);
    test_palettelib();
    test_show();
    test_showsprite();
    test_displaybitmap();
    test_displaysprite();
    test_displaypalette();
    test_scroll();
    ROUT = "memlib finit";
    strcpy(VEC, "");
    CALL(entry_code(getlib(&loaded, "memlib"), 8), 0, (void)0);

    FreeMem(chipmem, 4096);
    CloseLibrary((struct Library *)gfx);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
