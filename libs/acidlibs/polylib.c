/*
 * polylib - command library 121 (#chunkylib in its source, "3d rendering
 * lib"): ChunkyCLS ChunkyPlot ChunkyVlin ChunkyPoly BitMapPoly
 * RenderToBitmap RenderToChunky RenderPoly.
 *
 * C translation of bblibsrc/polylib (the same file with #chunkylib=87 is
 * renderlib); the CD binary matches the source.
 *
 * Translated at register level like 2dlib/medlib: d0-d7 and a0-a6 are
 * static variables, every instruction keeps its operand size.
 *
 * Chunky commands work on the copper chunky area of the used CopList
 * (displaylib CustomChunky): cchunky (96) = first MOVE of the area,
 * chw/chh/chr/chc (102..108) = width, height, resolution, bytes per line;
 * a pixel is the data word of a colour MOVE: x -> (x & 31) * 4 + (x >> 5)
 * * 132 (32 colour MOVEs and one bank switch per 132 bytes), y -> y * chc.
 * BitMapPoly fills a polygon on a Blitz bitmap with the blitter (one blit
 * per line and plane; the dither form one blit per line over all planes,
 * B = two alternating lines of the dither bitmap at colour * 2).  The
 * polygon is clipped against bitmap 56/58 (chunky: chh/chw).  Neither
 * routine owns the blitter (no Blitz/Amiga variants), as the original.
 * RenderToBitmap/RenderToChunky choose what RenderPoly draws.
 *
 * BUG(orig), kept:
 *   - no origin, no orientation test: the points must run clockwise. (#419)
 *   - RenderPoly after RenderToBitmap b,d draws on d with b as dither
 *     bitmap (the two are swapped, #421); RenderPoly before any RenderTo...
 *     draws on a3 = the string pointer (no !libs) (#420).
 *   - RenderToChunky colours > 14 read behind the 15 entry colour table. (#422)
 *   - more than 16 points overflow polypts, more than 10 edges one side
 *     the edge lists l0/l1. (#395)
 *   - the dither form assumes evenly spaced bitplanes (plane 1 - plane 0). (#418)
 *   - a depth of 0 runs the plane loop 65536 times. (#393)
 * DIFF (memory layout, unavoidable): what such an overflow hits.
 */
#include "blitzlib.h"

void _chunkycls(void), _chunkyplot(void), _chunkyplotclip(void);
void _chunkyvlin(void), _chunkyvlinclip(void), _chunkypoly(void), _chunkypoly2(void);
void _bitmappoly(void), _bitmappoly2(void), _rendertobitmap(void), _rendertobitmap2(void);
void _rendertochunky(void), _rendertochunky2(void), _renderpoly(void);


/* !libheader {#chunkylib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_polylib, 0, BL_NORET, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkycls, 0, 0);
BLITZ_NAME("ChunkyCLS", "color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkyplot, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkyplotclip, 0, 0);
BLITZ_NAME("ChunkyPlot", "x,y,color,[,CLIP]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkyvlin, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkyvlinclip, 0, 0);
BLITZ_NAME("ChunkyVlin", "x,y1,y2,color,[,CLIP]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkypoly, BLITZ_BLITZONLY, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1302);
BLITZ_SUBS(_chunkypoly2, BLITZ_BLITZONLY, 0);
BLITZ_NAME("ChunkyPoly", "numpts,*coords.w,color[,noblitter]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302);
BLITZ_SUBS(_bitmappoly, 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1083);
BLITZ_SUBS(_bitmappoly2, 0, 0);
BLITZ_NAME("BitMapPoly", "numpts,*coords.w,color[,ditherbitmap]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080);
BLITZ_SUBS(_rendertobitmap, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_bitmaplib, 0x1181);
BLITZ_SUBS(_rendertobitmap2, 0, 0);
BLITZ_NAME("RenderToBitmap", "BitMap#[,DitherBitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1080);
BLITZ_SUBS(_rendertochunky, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1080);
BLITZ_SUBS(_rendertochunky2, 0, 0);
BLITZ_NAME("RenderToChunky", "CopList#[,split]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_renderpoly, BLITZ_BLITZONLY, 0);
BLITZ_NAME("RenderPoly", "numpts,*coords.w,color");

BLITZ_LIBFIN();

/* ------------------------------------------------- register emulation */

static ULONG d0, d1, d2, d3, d4, d5, d6, d7;
static UBYTE *a0, *a1, *a2, *a3, *a4, *a5, *a6;

#define B(r)       ((UBYTE)(r))
#define W(r)       ((UWORD)(r))
#define SW(r)      ((WORD)(r))
#define SETW(r, v) ((r) = ((r) & 0xffff0000UL) | (UWORD)(v))
#define EXTL(r)    ((ULONG)(LONG)(WORD)(r))
#define MULU(a, b) bl_mulu_w((UWORD)(a), (UWORD)(b))
#define MULS(a, b) ((ULONG)bl_muls_w((WORD)(a), (WORD)(b)))
#define SWAP(r)    ((ULONG)(r) << 16 | (ULONG)(r) >> 16)
#define MW(p, o)   (*(UWORD *)((UBYTE *)(p) + (o)))
#define ML(p, o)   (*(ULONG *)((UBYTE *)(p) + (o)))
#define MP(p, o)   (*(UBYTE **)((UBYTE *)(p) + (o)))
#define MOVEMW(p, o) EXTL(MW(p, o))        /* MOVEM.w to a data register */
#define DIVS(r, s) ((r) = bl_divs_w((r), (WORD)(s)))
#define EXG(x, y)  do { ULONG t_ = (x); (x) = (y); (y) = t_; } while (0)
#define EXGA(x, y) do { UBYTE *t_ = (x); (x) = (y); (y) = t_; } while (0)

/* ADD.l src,dst:BCC:ADDQ #2,dst (ADDQ.w) */
#define ADDC2(r, s) do { ULONG o_ = (r); (r) += (s); if ((r) < o_) SETW(r, (r) + 2); } while (0)
/* MOVEQ #0,d7:ADD.l s,r:ADDX d7,r (word ADDX) */
#define ADDX1(r, s) do { ULONG o_ = (r); (r) += (s); if ((r) < o_) SETW(r, (r) + 1); } while (0)

#define CW(cb, o) (*(volatile UWORD *)((UBYTE *)(cb) + (o)))
#define CL(cb, o) (*(volatile ULONG *)((UBYTE *)(cb) + (o)))
#define BUSY(cb)  (*(volatile UBYTE *)((UBYTE *)(cb) + 2) & 0x40)
#define BLITWAIT(cb) do { (void)BUSY(cb); while (BUSY(cb)) ; } while (0)

/* CopList fields (SizeOf.mycop\x) */
#define M_cchunky 96
#define M_chw     102
#define M_chh     104
#define M_chc     108

/* ------------------------------------------------------------ variables */

static const UWORD cols[15] = {
    0, 0xf00, 0x0f0, 0x00f, 0xa40, 0x028, 0xfff, 0xf80,
    0x840, 0x8f4, 0x84f, 0x248, 0x488, 0x888, 0xf88,
};

static void rpoly0(void), rpoly1(void), rpoly2(void), bitmappoly(void);
static void (*polyvector)(void) = bitmappoly;
static UBYTE *bmap0, *bmap1, *chunk0;
static ULONG loflo;
static UWORD pcol, chc;
static UWORD srcmod, negmod, bltsize;
static ULONG bmwid, bmoff;
static UWORD rtclip;
static ULONG polypts[32];
/* in the order of the original: an overflow of l0 runs into l1 */
static struct {
    UWORD width, height;
    UWORD xyx[3];
    UWORD l0[32], l1[32];
} pv;
#define width  pv.width
#define height pv.height
#define xyx    pv.xyx
#define l0     pv.l0
#define l1     pv.l1

static const UWORD lup1[16] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};
static const UWORD lup2[16] = {
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff,
};
#define dup1 lup1
#define dup2 lup2

/* ---------------------------------------------------- outline lists */

/* tops..rside: d7 = number of points in polypts (twice); returns with
   d0 = 0 if nothing is visible (Z set in the original), else -1 */
static void tops(void)
{
    a0 = (UBYTE *)polypts;
    SETW(d6, d7);
    SETW(d7, d7 - 1);
    goto fud;
fhi:
    if (SW(d1 - MW(a0, 2)) < 0)
        goto flo;
    if (W(d1) != MW(a0, 2))
        goto fud;
    if (SW(d0 - MW(a0, 0)) >= 0) {
        SETW(d0, MW(a0, 0));
        a1 = a0;
        goto flo;
    }
    if (SW(d2 - MW(a0, 0)) >= 0)                /* fox */
        goto flo;
    SETW(d2, MW(a0, 0));
    a2 = a0;
    goto flo;
fud:
    d0 = MOVEMW(a0, 0);
    d1 = MOVEMW(a0, 2);
    SETW(d2, d0);
    a1 = a0;
    a2 = a0;
flo:
    a0 += 4;
    SETW(d7, d7 - 1);
    if (W(d7) != 0xffff)
        goto fhi;
    SETW(d6, d6 << 2);
    a1 += SW(d6);
    a1 -= 4;
    a2 += 4;

    /* starts */
    if (SW(d1) < 0) {                           /* cliptop */
        d5 = 0;
        for (;;) {
            if (SW(d5) < SW(MW(a1, 2)))
                break;
            d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
            a1 -= 4;
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a1, 0); d4 = MOVEMW(a1, 2);     /* clipl */
        SETW(d5, d5 - d1);
        SETW(d3, d3 - d0);
        SETW(d4, d4 - d1);
        d3 = MULS(d5, d3);
        DIVS(d3, d4);
        SETW(d0, d0 + d3);
        d5 = 0;
        SETW(d1, MW(a2, -2));
        for (;;) {
            if (SW(d5) < SW(MW(a2, 2)))
                break;
            d1 = MOVEMW(a2, 0); d2 = MOVEMW(a2, 2);
            a2 += 4;
            EXG(d1, d2);
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a2, 0); d4 = MOVEMW(a2, 2);     /* clipr */
        SETW(d5, d5 - d1);
        SETW(d3, d3 - d2);
        SETW(d4, d4 - d1);
        d3 = MULS(d5, d3);
        DIVS(d3, d4);
        SETW(d2, d2 + d3);
        d1 = 0;
    }
    /* duntop */
    if (!(SW(d0 - width) < 0)) {                /* cliprstart */
        SETW(d6, d1);
        SETW(d5, width);
        SETW(d5, d5 - 1);
        for (;;) {
            if (SW(d5) > SW(MW(a1, 0)))
                break;
            d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
            a1 -= 4;
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a1, 0); d4 = MOVEMW(a1, 2);     /* clipm */
        SETW(d5, d5 - d0);
        SETW(d3, d3 - d0);
        SETW(d4, d4 - d1);
        d4 = MULS(d5, d4);
        DIVS(d4, d3);
        SETW(d1, d1 + d4);
        EXG(d1, d6);
        SETW(d0, width);
        SETW(d0, d0 - 1);
        for (;;) {
            if (SW(d6 - MW(a2, 2)) < 0)
                break;
            d1 = MOVEMW(a2, 0); d2 = MOVEMW(a2, 2);
            a2 += 4;
            EXG(d1, d2);
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a2, 0); d4 = MOVEMW(a2, 2);     /* mom */
        SETW(d4, d4 - d1);
        SETW(d3, d3 - d2);
        EXG(d1, d6);
        SETW(d6, d6 - d1);
        d6 = MULS(d3, d6);
        DIVS(d6, d4);
        SETW(d2, d2 - d6);
    } else if (SW(d2) < 0) {                    /* cliplstart */
        SETW(d6, d1);
        d5 = 0;
        for (;;) {
            if (SW(d5) < SW(MW(a2, 0)))
                break;
            d1 = MOVEMW(a2, 0); d2 = MOVEMW(a2, 2);
            a2 += 4;
            EXG(d1, d2);
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a2, 0); d4 = MOVEMW(a2, 2);     /* clipn */
        SETW(d5, d5 - d2);
        SETW(d3, d3 - d2);
        SETW(d4, d4 - d1);
        d4 = MULS(d5, d4);
        DIVS(d4, d3);
        SETW(d1, d1 + d4);
        d2 = 0;
        EXG(d1, d6);
        for (;;) {
            if (SW(d6 - MW(a1, 2)) < 0)
                break;
            d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
            a1 -= 4;
            if (!((LONG)(a1 - a2) >= 0))
                goto dun;
        }
        d3 = MOVEMW(a1, 0); d4 = MOVEMW(a1, 2);     /* mon */
        SETW(d4, d4 - d1);
        SETW(d3, d3 - d0);
        EXG(d1, d6);
        SETW(d6, d6 - d1);
        d6 = MULS(d3, d6);
        DIVS(d6, d4);
        SETW(d0, d0 - d6);
    }
    /* dunstart: still inside window? */
    if (SW(d1 - height) >= 0)
        goto dun;
    xyx[0] = d0;
    xyx[1] = d1;
    xyx[2] = d2;
    SETW(d5, height);
    SETW(d6, width);
    SETW(d6, d6 - 1);
    d7 = 0;

    /* lside */
    a3 = (UBYTE *)l0;
    d2 = MOVEMW(a1, 0); d3 = MOVEMW(a1, 2);
    for (;;) {                                  /* lline */
        if (SW(d3) <= SW(d1)) {                 /* SUB d1,d3:BLE dunl */
            SETW(d3, d3 - d1);
            break;
        }
        SETW(d3, d3 - d1);
        SETW(d1, d3);
        SETW(d3, d3 - 1);
        MW(a3, 0) = d3;
        a3 += 2;
        SETW(d2, d2 - d0);
        d2 = EXTL(d2);
        d4 = 15;
        SETW(d3, d3 + 1);
        do {                                    /* lls */
            SETW(d4, d4 - 3);
            d2 <<= 3;
            SETW(d3, W(d3) >> 3);
        } while (W(d3));
        DIVS(d2, d1);
        d2 = EXTL(d2);
        d2 = (d4 & 63) >= 32 ? 0 : d2 << (d4 & 63);
        d2 = SWAP(d2);
        SETW(d2, d2 + d2);
        ML(a3, 0) = d2;
        a3 += 4;
        d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
        if (SW(d1 - d5) >= 0) {                 /* lbot */
            SETW(d1, d1 - d5);
            MW(a3, -6) -= W(d1);
            break;
        }
        if (SW(d0) > SW(d6)) {                  /* loff */
            d2 = MOVEMW(a1, 4); d3 = MOVEMW(a1, 6);
            SETW(d2, d2 - d0);
            SETW(d3, d3 - d1);
            SETW(d0, d0 - d6);
            d3 = MULS(d0, d3);
            DIVS(d3, d2);
            MW(a3, -6) -= W(d3);
            break;
        }
        a1 -= 4;
        d2 = MOVEMW(a1, 0); d3 = MOVEMW(a1, 2);
    }
    d0 = 0xffffffff;                            /* dunl */
    ML(a3, 0) = d0;

    /* rside */
    d0 = MOVEMW(xyx, 2); d1 = MOVEMW(xyx, 4);
    EXG(d0, d1);
    a3 = (UBYTE *)l1;
    d2 = MOVEMW(a2, 0); d3 = MOVEMW(a2, 2);
    for (;;) {                                  /* rline */
        if (SW(d3) <= SW(d1)) {
            SETW(d3, d3 - d1);
            break;
        }
        SETW(d3, d3 - d1);
        SETW(d1, d3);
        SETW(d3, d3 - 1);
        MW(a3, 0) = d3;
        a3 += 2;
        SETW(d2, d2 - d0);
        d2 = EXTL(d2);
        d4 = 15;
        SETW(d3, d3 + 1);
        do {                                    /* rls */
            SETW(d4, d4 - 3);
            d2 <<= 3;
            SETW(d3, W(d3) >> 3);
        } while (W(d3));
        DIVS(d2, d1);
        d2 = EXTL(d2);
        d2 = (d4 & 63) >= 32 ? 0 : d2 << (d4 & 63);
        d2 = SWAP(d2);
        SETW(d2, d2 + d2);
        ML(a3, 0) = d2;
        a3 += 4;
        d0 = MOVEMW(a2, 0); d1 = MOVEMW(a2, 2); d2 = MOVEMW(a2, 4); d3 = MOVEMW(a2, 6);
        if (SW(d1 - d5) >= 0) {                 /* rbot */
            SETW(d1, d1 - d5);
            MW(a3, -6) -= W(d1);
            break;
        }
        if (SW(d0 - d7) < 0) {                  /* roff */
            d2 = MOVEMW(a2, -4); d3 = MOVEMW(a2, -2);
            SETW(d2, d2 - d0);
            SETW(d3, d3 - d1);
            SETW(d0, d0 - d7);
            d3 = MULS(d0, d3);
            DIVS(d3, d2);
            MW(a3, -6) -= W(d3);
            break;
        }
        a2 += 4;
    }
    d0 = 0xffffffff;                            /* dunr */
    ML(a3, 0) = d0;
    return;
dun:
    d0 = 0;
}

/* _calcpoly (chunky: x/y swapped) / _calcpoly2; a0 = points, d0 = number
   (negative: backwards) */
static void calcpoly(int swap)
{
    UWORD k;

    if (SW(d0) >= 0) {
        SETW(d7, d0);
        SETW(d7, d7 << 2);
        a1 = (UBYTE *)polypts;
        a2 = a1 + SW(d7);
        SETW(d7, d0);
        SETW(d0, d0 - 1);
        k = d0;
        do {
            d1 = ML(a0, 0);
            a0 += 4;
            if (swap)
                d1 = SWAP(d1);
            ML(a1, 0) = d1;
            a1 += 4;
            ML(a2, 0) = d1;
            a2 += 4;
        } while (k-- != 0);
    } else {                                    /* anti */
        SETW(d0, -SW(d0));
        SETW(d7, d0);
        SETW(d7, d7 << 2);
        a1 = (UBYTE *)polypts + SW(d7);
        a2 = a1 + SW(d7);
        SETW(d7, d0);
        SETW(d0, d0 - 1);
        k = d0;
        do {
            d1 = ML(a0, 0);
            a0 += 4;
            if (swap)
                d1 = SWAP(d1);
            a1 -= 4;
            ML(a1, 0) = d1;
            a2 -= 4;
            ML(a2, 0) = d1;
        } while (k-- != 0);
    }
    SETW(d0, 0xffff);                           /* DBRA end */
    tops();
}

/* -------------------------------------------------------------- chunky */

#undef width
#undef height

/* the chunky area of a CopList (displaylib CustomChunky, SizeOf.mycop) */
struct coplist {
    UBYTE pad[96];
    UBYTE *chunky;              /* +96 cchunky: the first colour MOVE */
    UWORD chy;                  /* +100 */
    UWORD width, height;        /* +102/+104 chw/chh */
    UWORD res;                  /* +106 chr */
    UWORD modulo;               /* +108 chc: bytes per chunky line */
} BL_PACKED;

/* (WORD)(a - b) < 0: the original's CMP/SUB with BMI - the sign of the 16
   bit difference, which wraps on overflow (unlike a signed compare) */
#define BMI_W(a, b) ((WORD)((a) - (b)) < 0)

static void BLITZ_ROUTINE _chunkycls_c(UWORD c BL_REG(d0))
{
    struct coplist *cl = (struct coplist *)blitz_string;
    UBYTE *p = cl->chunky, *q;
    UWORD x = cl->width - 1, h = cl->height - 1, n = 31, y;
    WORD mod = cl->modulo;

    do {                                        /* nxx */
        q = p;
        p += 4;
        if (n-- == 0) {
            p += 4;
            n = 31;
        }
        y = h;
        do {
            *(UWORD *)q = c;
            q += mod;
        } while (y-- != 0);
    } while (x-- != 0);
}
BLITZ_THUNK(_chunkycls, _chunkycls_c, "d1/a0-a1");

/* the word of chunky pixel x, y: 32 colour MOVEs (4 bytes) and a bank
   switch per 132 bytes, y * bytes per line */
static UWORD *chunkypixel(const struct coplist *cl, UWORD x, UWORD y)
{
    return (UWORD *)(cl->chunky + ((x & 31) << 2) + bl_mulu_w(x >> 5, 132)
                     + bl_mulu_w(y, cl->modulo));
}

static void BLITZ_ROUTINE _chunkyplot_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                        UWORD colour BL_REG(d2))
{
    *chunkypixel((struct coplist *)blitz_string, x, y) = colour;
}
BLITZ_THUNK(_chunkyplot, _chunkyplot_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkyplotclip_c(WORD x BL_REG(d0), WORD y BL_REG(d1),
                                            UWORD colour BL_REG(d2))
{
    struct coplist *cl = (struct coplist *)blitz_string;

    if (x < 0 || y < 0 || !BMI_W(x, cl->width) || !BMI_W(y, cl->height))
        return;
    *chunkypixel(cl, x, y) = colour;
}
BLITZ_THUNK(_chunkyplotclip, _chunkyplotclip_c, "d1/a0-a1");

/* y2 - y1 + 1 words down from x, y1 (DBRA: y1 > y2 gives about 65536, #417) */
static void chunkyvlin(const struct coplist *cl, UWORD x, UWORD y1, UWORD y2, UWORD colour)
{
    UWORD *p = chunkypixel(cl, x, y1), n = y2 - y1;

    do {
        *p = colour;
        p = (UWORD *)((UBYTE *)p + (WORD)cl->modulo);
    } while (n-- != 0);
}

static void BLITZ_ROUTINE _chunkyvlin_c(UWORD x BL_REG(d0), UWORD y1 BL_REG(d1),
                                        UWORD y2 BL_REG(d2), UWORD colour BL_REG(d3))
{
    chunkyvlin((struct coplist *)blitz_string, x, y1, y2, colour);
}
BLITZ_THUNK(_chunkyvlin, _chunkyvlin_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkyvlinclip_c(WORD x BL_REG(d0), WORD y1 BL_REG(d1),
                                            WORD y2 BL_REG(d2), UWORD colour BL_REG(d3))
{
    struct coplist *cl = (struct coplist *)blitz_string;
    WORD h = cl->height;

    if (x < 0)
        return;
    if (BMI_W(y2, y1)) {                       /* CMP d1,d2:BPL */
        WORD t = y1;

        y1 = y2;
        y2 = t;
    }
    if (y1 < 0)
        y1 = 0;
    if (y2 < 0 || !BMI_W(y1, h))
        return;
    if (!BMI_W(y2, h))
        y2 = h - 1;
    chunkyvlin(cl, x, y1, y2, colour);
}
BLITZ_THUNK(_chunkyvlinclip, _chunkyvlinclip_c, "d1/a0-a1");

#define width  pv.width
#define height pv.height

#define CREGS ULONG r0 BL_REG(d0), ULONG r1 BL_REG(d1), ULONG r2 BL_REG(d2), \
              ULONG r3 BL_REG(d3)
#define CLOAD() (d0 = r0, d1 = r1, d2 = r2, d3 = r3, a3 = blitz_string)


/* ChunkyPoly - with the blitter (vertical lines, one blit each) or with
   the CPU (noblitter form); a3 = CopList */
static void chunkypoly(int cpu)
{
    UBYTE *sa4 = a4, *sa5 = a5, *sa6 = a6, *sa3;

    sa3 = a3;
    pcol = d2;
    a0 = (UBYTE *)d1;
    height = MW(a3, M_chw);
    width = MW(a3, M_chh);
    SETW(d0, -SW(d0));
    calcpoly(1);
    a3 = sa3;                                   /* MOVE.l (a7)+,a3:BEQ */
    if (!d0)
        goto pdun;
    a0 = MP(a3, M_cchunky);
    a1 = (UBYTE *)xyx;
    d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2); d2 = MOVEMW(a1, 4);
    a1 += 6;
    d0 ^= 0x80000000;
    d2 ^= 0x80000000;
    a2 = a1 + 64;
    a4 = a0 + SW(MW(a3, M_chc));
    a4 -= 8;
    loflo = (ULONG)a4;
    SETW(d7, d1);
    SETW(d7, W(d7) >> 5);
    d7 = MULU(d7, 132);
    a0 += SW(d7);
    d3 = 31;
    SETW(d7, d1);
    SETW(d7, d7 & d3);
    SETW(d7, d7 << 2);
    a0 += SW(d7);
    SETW(d1, ~d1);
    SETW(d1, d1 & d3);
    SETW(d3, MW(a1, 0));
    a1 += 2;
    if (SW(d3) < 0)
        goto pdun;
    d5 = ML(a1, 0);
    a1 += 4;
    SETW(d5, SW(d5) >> 1);                 /* ASR #1 (.w) */
    SETW(d4, MW(a2, 0));
    a2 += 2;
    if (SW(d4) < 0)
        goto pdun;
    d6 = ML(a2, 0);
    a2 += 4;
    SETW(d6, SW(d6) >> 1);                 /* ASR #1 (.w) */
    if (!cpu) {
        a5 = (UBYTE *)0xdff000;
        BLITWAIT(a5);
        CW(a5, 0x74) = pcol;
        CL(a5, 0x44) = 0xffffffff;
        SETW(d7, MW(a3, M_chc));
        SETW(d7, d7 - 2);
        CW(a5, 0x66) = d7;
        CL(a5, 0x40) = 0x01f00000;
        a6 = (UBYTE *)EXTL(MW(a3, M_chh));
        chc = MW(a3, M_chc);
    } else {
        a5 = (UBYTE *)EXTL(MW(a3, M_chh));
        a6 = (UBYTE *)EXTL(MW(a3, M_chc));
        chc = (UWORD)(ULONG)a6;
    }
    for (;;) {                                  /* ploop */
        UBYTE *lim = cpu ? a5 : a6;

        ADDX1(d0, d5);
        ADDX1(d2, d6);
        if (SW(d0 - (UWORD)(ULONG)lim) >= 0)
            goto pdun;
        SETW(d7, d0);
        if (SW(d7) < 0)
            d7 = 0;
        a3 = (UBYTE *)EXTL(d7);                 /* MOVE d7,a3 */
        d7 = MULU(d7, chc);
        a4 = a0 + d7;
        SETW(d7, d2);
        if (!(SW(d7 - (UWORD)(ULONG)lim) < 0)) {
            SETW(d7, (UWORD)(ULONG)lim);
            SETW(d7, d7 - 1);
        }
        SETW(d7, d7 - (UWORD)(ULONG)a3);
        if (SW(d7) < 0)
            goto pdun;
        if (!cpu) {
            SETW(d7, d7 + 1);
            BLITWAIT(a5);
            CL(a5, 0x54) = (ULONG)a4;
            CW(a5, 0x5c) = d7;
            CW(a5, 0x5e) = 1;
        } else {
            do {                                /* vloop */
                *(UWORD *)a4 = pcol;
                a4 += SW((UWORD)(ULONG)a6);
                SETW(d7, d7 - 1);
            } while (W(d7) != 0xffff);
        }
        a0 += 4;
        SETW(d1, d1 - 1);
        if (W(d1) == 0xffff) {
            a0 += 4;
            d1 = 31;
        }
        if ((ULONG)a0 == loflo)
            goto pdun;
        ADDX1(d0, d5);
        ADDX1(d2, d6);
        SETW(d3, d3 - 1);
        if (W(d3) == 0xffff) {
            SETW(d3, MW(a1, 0));
            a1 += 2;
            if (SW(d3) < 0)
                goto pdun;
            d5 = ML(a1, 0);
            a1 += 4;
            SETW(d5, SW(d5) >> 1);                 /* ASR #1 (.w) */
        }
        SETW(d4, d4 - 1);
        if (W(d4) == 0xffff) {
            SETW(d4, MW(a2, 0));
            a2 += 2;
            if (SW(d4) < 0)
                goto pdun;
            d6 = ML(a2, 0);
            a2 += 4;
            SETW(d6, SW(d6) >> 1);                 /* ASR #1 (.w) */
        }
    }
pdun:
    a4 = sa4;
    a5 = sa5;
    a6 = sa6;
}

/* ChunkyPoly: d0 = n, d1 = points, d2 = colour (d3 = noblitter, ignored:
   the 4 argument form is always the CPU version) */
static void BLITZ_ROUTINE _chunkypoly_c(CREGS)
{
    CLOAD();
    chunkypoly(0);
    blitz_string = a3;
}
BLITZ_THUNK(_chunkypoly, _chunkypoly_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkypoly2_c(CREGS)
{
    CLOAD();
    chunkypoly(1);
    blitz_string = a3;
}
BLITZ_THUNK(_chunkypoly2, _chunkypoly2_c, "d1/a0-a1");

/* ---------------------------------------------------------- BitMapPoly */

static void bitmappoly(void)
{
    UBYTE *sa4 = a4, *sa5 = a5, *sa6 = a6, *sa3;

    pcol = d2;
    sa3 = a3;
    a0 = (UBYTE *)d1;
    width = MW(a3, 56);
    height = MW(a3, 58);
    calcpoly(0);
    a6 = sa3;
    if (!d0)
        goto bdun;
    a5 = (UBYTE *)0xdff000;
    d0 = MOVEMW(xyx, 0); d1 = MOVEMW(xyx, 2); d2 = MOVEMW(xyx, 4);
    d1 = MULU(d1, MW(a6, 0));
    bmoff = d1;
    SETW(bmwid, MW(a6, 0));
    SETW(d1, MW(a6, 56));
    SETW(d1, d1 - 1);
    SETW(d1, d1 + d1);
    rtclip = d1;
    d1 = d2;
    SETW(d0, d0 + d0);
    SETW(d1, d1 + d1);
    d0 ^= 0x80000000;
    d1 ^= 0x80000000;
    a0 = (UBYTE *)l0;
    SETW(d2, MW(a0, 0));
    a0 += 2;
    if (SW(d2) < 0)
        goto bdun;
    d4 = ML(a0, 0);
    a0 += 4;
    a1 = (UBYTE *)l1;
    SETW(d3, MW(a1, 0));
    a1 += 2;
    if (SW(d3) < 0)
        goto bdun;
    d5 = ML(a1, 0);
    a1 += 4;
    ADDC2(d0, d4);
    ADDC2(d1, d5);
    BLITWAIT(a5);
    CW(a5, 0x74) = 0xffff;
    CW(a5, 0x42) = 0;
    for (;;) {                                  /* pploop */
        SETW(d6, d0);
        if (SW(d6) > SW(rtclip))
            goto bdun;
        d6 = SWAP(d6);
        SETW(d6, d0);
        if (SW(d6) < 0)
            d6 = 0;
        if (SW(d1) < 0)
            goto bdun;
        SETW(d7, rtclip);
        if (!(SW(d7 - d1) < 0))
            SETW(d7, d1);
        BLITWAIT(a5);
        SETW(d6, d6 & 30);
        CW(a5, 0x44) = lup1[W(d6) >> 1];
        SETW(d6, d7);
        SETW(d6, d6 & 30);
        CW(a5, 0x46) = lup2[W(d6) >> 1];
        d6 = SWAP(d6);
        SETW(d6, W(d6) >> 5);
        SETW(d7, W(d7) >> 5);
        SETW(d7, d7 - d6);
        if (SW(d7) < 0)
            goto bdun;
        SETW(d7, d7 + 65);
        SETW(d6, d6 + d6);
        a2 = (UBYTE *)bmoff + SW(d6);
        d6 = 0;
        a4 = a6 + 8;
        do {                                    /* nxpolyplane */
            BLITWAIT(a5);
            CW(a5, 0x40) = 0x3fa;
            if (!((UBYTE)pcol & (1 << (d6 & 7))))   /* BTST d6,pcol+1 */
                CW(a5, 0x40) = 0x30a;
            a3 = MP(a4, 0) + (ULONG)a2;
            a4 += 4;
            CL(a5, 0x48) = (ULONG)a3;
            CL(a5, 0x54) = (ULONG)a3;
            CW(a5, 0x58) = d7;
            SETW(d6, d6 + 1);
        } while (W(d6) != MW(a6, 4));
        d7 = bmwid;
        bmoff += d7;
        ADDC2(d0, d4);
        ADDC2(d1, d5);
        SETW(d2, d2 - 1);
        if (W(d2) == 0xffff) {
            SETW(d2, MW(a0, 0));
            a0 += 2;
            if (SW(d2) < 0)
                goto bdun;
            d4 = ML(a0, 0);
            a0 += 4;
        }
        SETW(d3, d3 - 1);
        if (W(d3) == 0xffff) {
            SETW(d3, MW(a1, 0));
            a1 += 2;
            if (SW(d3) < 0)
                goto bdun;
            d5 = ML(a1, 0);
            a1 += 4;
        }
        ADDC2(d0, d4);
        ADDC2(d1, d5);
    }
bdun:
    a4 = sa4;
    a5 = sa5;
    a6 = sa6;
}

/* dither form: a0 = dither bitmap, a3 = bitmap */
static void bitmappoly2(void)
{
    UBYTE *sa4 = a4, *sa5 = a5, *sa6 = a6, *sa0 = a0, *sa3 = a3;

    pcol = d2;
    a0 = (UBYTE *)d1;
    width = MW(a3, 56);
    height = MW(a3, 58);
    calcpoly(0);
    a4 = sa0;
    a6 = sa3;
    if (!d0)
        goto ddun;
    a5 = (UBYTE *)0xdff000;
    d3 = ML(a6, 12);
    d3 -= ML(a6, 8);
    d3 -= 2;
    srcmod = d3;
    d3 -= ML(a4, 12);
    d3 += ML(a4, 8);
    d3 += 2;
    negmod = d3;
    SETW(d3, MW(a6, 4));
    SETW(d3, d3 << 6);
    SETW(d3, d3 + 1);
    bltsize = d3;
    SETW(d2, pcol);
    SETW(d2, d2 + d2);
    SETW(d3, MW(a4, 0));
    d2 = MULU(d2, d3);
    d2 += ML(a4, 8);
    a3 = (UBYTE *)d2;
    a4 = a3 + SW(d3);
    d0 = MOVEMW(xyx, 0); d1 = MOVEMW(xyx, 2); d2 = MOVEMW(xyx, 4);
    if (d1 & 1)
        EXGA(a3, a4);
    SETW(bmwid, MW(a6, 0));
    SETW(d3, MW(a6, 56));
    SETW(d3, d3 - 1);
    SETW(d3, d3 + d3);
    rtclip = d3;
    d1 = MULU(d1, MW(a6, 0));
    a6 = MP(a6, 8);
    a6 += d1;
    d1 = d2;
    SETW(d0, d0 + d0);
    SETW(d1, d1 + d1);
    d0 ^= 0x80000000;
    d1 ^= 0x80000000;
    a0 = (UBYTE *)l0;
    SETW(d2, MW(a0, 0));
    a0 += 2;
    if (SW(d2) < 0)
        goto ddun;
    d4 = ML(a0, 0);
    a0 += 4;
    a1 = (UBYTE *)l1;
    SETW(d3, MW(a1, 0));
    a1 += 2;
    if (SW(d3) < 0)
        goto ddun;
    d5 = ML(a1, 0);
    a1 += 4;
    ADDC2(d0, d4);
    ADDC2(d1, d5);
    BLITWAIT(a5);
    CL(a5, 0x40) = 0x07ca0000;
    CW(a5, 0x74) = 0xffff;
    for (;;) {                                  /* dploop */
        SETW(d6, d0);
        if (SW(d6) > SW(rtclip))
            goto ddun;
        d6 = SWAP(d6);
        SETW(d6, d0);
        if (SW(d6) < 0)
            d6 = 0;
        if (SW(d1) < 0)
            goto ddun;
        SETW(d7, rtclip);
        if (!(SW(d7 - d1) < 0))
            SETW(d7, d1);
        BLITWAIT(a5);
        SETW(d6, d6 & 30);
        CW(a5, 0x44) = dup1[W(d6) >> 1];
        SETW(d6, d7);
        SETW(d6, d6 & 30);
        CW(a5, 0x46) = dup2[W(d6) >> 1];
        d6 = SWAP(d6);
        SETW(d6, W(d6) >> 5);
        SETW(d7, W(d7) >> 5);
        SETW(d7, d7 - d6);
        if (SW(d7) < 0)
            goto ddun;
        SETW(d6, d6 + d6);
        a2 = a6 + SW(d6);
        SETW(d6, srcmod);
        SETW(d6, d6 - d7);
        SETW(d6, d6 - d7);
        SETW(d7, d7 + bltsize);
        CW(a5, 0x60) = d6;
        CW(a5, 0x66) = d6;
        SETW(d6, d6 + negmod);
        CW(a5, 0x62) = d6;
        CL(a5, 0x48) = (ULONG)a2;               /* MOVEM.l a2-a3,$48 */
        CL(a5, 0x4c) = (ULONG)a3;
        CL(a5, 0x54) = (ULONG)a2;
        CW(a5, 0x58) = d7;
        a6 += SW(W(bmwid));
        EXGA(a3, a4);
        ADDC2(d0, d4);
        ADDC2(d1, d5);
        SETW(d2, d2 - 1);
        if (W(d2) == 0xffff) {
            SETW(d2, MW(a0, 0));
            a0 += 2;
            if (SW(d2) < 0)
                goto ddun;
            d4 = ML(a0, 0);
            a0 += 4;
        }
        SETW(d3, d3 - 1);
        if (W(d3) == 0xffff) {
            SETW(d3, MW(a1, 0));
            a1 += 2;
            if (SW(d3) < 0)
                goto ddun;
            d5 = ML(a1, 0);
            a1 += 4;
        }
        ADDC2(d0, d4);
        ADDC2(d1, d5);
    }
ddun:
    a4 = sa4;
    a5 = sa5;
    a6 = sa6;
}

#define PREGS ULONG r0 BL_REG(d0), ULONG r1 BL_REG(d1), ULONG r2 BL_REG(d2)

static void BLITZ_ROUTINE _bitmappoly_c(PREGS)
{
    d0 = r0; d1 = r1; d2 = r2;
    a3 = blitz_string;
    bitmappoly();
    blitz_string = a3;
}
BLITZ_THUNK(_bitmappoly, _bitmappoly_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitmappoly2_c(PREGS, UBYTE *dither BL_REG(a0))
{
    d0 = r0; d1 = r1; d2 = r2;
    a0 = dither;
    a3 = blitz_string;
    bitmappoly2();
    blitz_string = a3;
}
BLITZ_THUNK(_bitmappoly2, _bitmappoly2_c, "d1/a0-a1");

/* ------------------------------------------------------------- Render */

static void rpoly0(void)
{
    a3 = bmap0;
    bitmappoly();
}

static void rpoly1(void)
{
    a0 = bmap0;                                 /* BUG(orig) (#421): swapped */
    a3 = bmap1;
    bitmappoly2();
}

static void rpoly2(void)
{
    a3 = chunk0;
    SETW(d2, d2 + d2);
    SETW(d2, cols[SW(d2) >> 1]);
    chunkypoly(0);
}

static void BLITZ_ROUTINE _rendertobitmap_c(UBYTE *bm BL_REG(a0))
{
    bmap0 = bm;
    polyvector = rpoly0;
}
BLITZ_THUNK(_rendertobitmap, _rendertobitmap_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _rendertobitmap2_c(UBYTE *bm BL_REG(a0), UBYTE *dither BL_REG(a1))
{
    bmap0 = bm;
    bmap1 = dither;
    polyvector = rpoly1;
}
BLITZ_THUNK(_rendertobitmap2, _rendertobitmap2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _rendertochunky_c(UBYTE *cl BL_REG(a0))
{
    chunk0 = cl;
    polyvector = rpoly2;
}
BLITZ_THUNK(_rendertochunky, _rendertochunky_c, "d0-d1/a0-a1");

/* split: ASL #4,d1:ADD d1,a0 - the CopList 16 bytes per split further */
static void BLITZ_ROUTINE _rendertochunky2_c(UWORD split BL_REG(d1), UBYTE *cl BL_REG(a0))
{
    chunk0 = cl + (WORD)(split << 4);
    polyvector = rpoly2;
}
BLITZ_THUNK(_rendertochunky2, _rendertochunky2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _renderpoly_c(PREGS)
{
    d0 = r0; d1 = r1; d2 = r2;
    a3 = blitz_string;                          /* default: _bitmappoly on a3 */
    polyvector();
    blitz_string = a3;
}
BLITZ_THUNK(_renderpoly, _renderpoly_c, "d1/a0-a1");
