/*
 * polylib - command library 121 (#chunkylib in its source, "3d rendering
 * lib"): ChunkyCLS ChunkyPlot ChunkyVlin ChunkyPoly BitMapPoly
 * RenderToBitmap RenderToChunky RenderPoly.
 *
 * C translation of bblibsrc/polylib (the same file with #chunkylib=87 is
 * renderlib); the CD binary matches the source.
 *
 * Plain C with the integer semantics of the original: word arithmetic
 * that wraps, MULS/DIVS as instructions (DIVS keeps the dividend on
 * overflow), CMP/BMI (sign of the 16 bit difference) where the original
 * tests that, and the edge positions as its swapped fixed point numbers
 * (edge_step).  Where the original leaves a3 (bitmap, edge list, plane
 * line ...), the routines leave it too: AllocFill and RenderPoly read it
 * (#390, #420).
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
 *     the edge lists l0/l1 (here ledges/redges). (#395)
 *   - ChunkyVlin without CLIP does not swap y1 > y2: about 65536 words. (#417)
 *   - the dither form assumes evenly spaced bitplanes (plane 1 - plane 0). (#418)
 *   - a depth of 0 runs the plane loop 65536 times. (#393)
 * DIFF (memory layout, unavoidable): what such an overflow hits.
 */
#include "blitzlib.h"
#include "blitzobj.h"

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

/* -------------------------------------------------------------- objects */

/* the bitmap (bitmaplib) and the chunky area of a CopList (displaylib
   CustomChunky): struct bb_bitmap, struct bb_coplist (blitzobj.h) */
/* cchunky (a long in displaylib) read as the pointer it is: as a cast
   value GCC would order the address arithmetic differently */
#define CHUNKY(cl) (*(UBYTE *const *)&(cl)->cchunky)

/* blitter registers (a long write sets two of them) */
#define BLTCON0 0x40
#define BLTCON1 0x42
#define BLTAFWM 0x44
#define BLTALWM 0x46
#define BLTCPT  0x48
#define BLTBPT  0x4c
#define BLTDPT  0x54
#define BLTSIZE 0x58
#define BLTSIZV 0x5c
#define BLTSIZH 0x5e
#define BLTCMOD 0x60
#define BLTBMOD 0x62
#define BLTDMOD 0x66
#define BLTADAT 0x74
#define CHIPW(o)   BL_CUSTOMW(BL_CUSTOM, o)
#define CHIPL(o)   BL_CUSTOML(BL_CUSTOM, o)
#define BLITWAIT() BL_BLITWAIT(BL_CUSTOM)

/* a polygon point as the caller gives it (words x, y) */
struct point {
    WORD x, y;
};

/* an edge of one side of the outline: the lines it covers - 1 and the x
   step per half line (edge_step); the long -1 ends a list */
struct edge {
    WORD lines;
    ULONG step;
} BL_PACKED;

/* (WORD)(a - b) < 0: the original's CMP/SUB with BMI - the sign of the 16
   bit difference, which wraps on overflow (unlike a signed compare) */
#define BMI_W(a, b) ((WORD)((a) - (b)) < 0)

/* ------------------------------------------------------------ variables */

static const UWORD cols[15] = {
    0, 0xf00, 0x0f0, 0x00f, 0xa40, 0x028, 0xfff, 0xf80,
    0x840, 0x8f4, 0x84f, 0x248, 0x488, 0x888, 0xf88,
};

/* what RenderPoly draws (polyvector): the routine gets the a3 of the call
   and returns the a3 it leaves */
typedef UBYTE *(*renderfn)(WORD n, const struct point *pts, UWORD colour, UBYTE *a3);
static UBYTE *render_a3(WORD n, const struct point *pts, UWORD colour, UBYTE *a3);
static renderfn polyvector = render_a3;
static struct bb_bitmap *bmap0, *bmap1;
static struct bb_coplist *chunk0;
static struct point polypts[32];        /* the n points twice in a row */
/* in the order of the original: an overflow of ledges runs into redges */
static struct {
    UWORD width, height;                /* clip size (chunky: swapped) */
    struct {
        WORD left, y, right;            /* the first line of the outline */
    } top;
    UWORD ledges[32], redges[32];       /* struct edge lists (l0, l1) */
} pv;

static const UWORD lup1[16] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};
static const UWORD lup2[16] = {
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff,
};

/* ---------------------------------------------------------- arithmetic */

/* MULS a,b : DIVS c - the quotient word (on overflow DIVS leaves the
   product, then its low word) */
static WORD muldiv(WORD a, WORD b, WORD c)
{
    return (WORD)bl_divs_w((ULONG)bl_muls_w(a, b), c);
}

/* An x position along an edge is a fixed point number with the fraction
   in the upper word and the integer, times `unit`, in the lower word (the
   integer can be used as a word directly).  ADD.l adds a step; its carry
   (out of the fraction) adds `unit` to the lower word (ADDQ / ADDX.w). */
static ULONG edge_step(ULONG pos, ULONG step, UWORD unit)
{
    ULONG sum = pos + step;

    if (sum < pos)
        sum = (sum & 0xffff0000UL) | (UWORD)(sum + unit);
    return sum;
}

/* the start: x (sign extended) with the lower word x * unit, the fraction
   word EORed with $8000 (one half; $7FFF for a negative x) */
static ULONG edge_start(WORD x, UWORD unit)
{
    return (((ULONG)(LONG)x & 0xffff0000UL) | (UWORD)(x * unit)) ^ 0x80000000UL;
}

/* the next edge of a list: its lines - 1 and its step; 0 at the end mark */
static int next_edge(const struct edge **list, UWORD *lines, ULONG *step)
{
    const struct edge *e = (*list)++;

    *lines = e->lines;
    if (e->lines < 0)
        return 0;
    *step = e->step;
    return 1;
}

/* ---------------------------------------------------- outline lists */

/* lside/rside: the edges of one side from (x, y) over the points p, p +
   dir, ... down to the bottom; cut where the side leaves the clip width
   (the left side at x > width - 1, the right side at x < 0).  Returns the
   address of the end mark. */
static UWORD *side(UWORD *list, WORD x, WORD y, const struct point *p, int dir, int right)
{
    struct edge *e = (struct edge *)list;
    WORD last = pv.width - 1, tx = p->x, ty = p->y;

    while (ty > y) {                            /* SUB y,ty:BLE: done */
        WORD dy = ty - y;
        UWORD rest = dy, shift = 15;
        ULONG dx = (ULONG)(LONG)(WORD)(tx - x), step;

        e->lines = dy - 1;
        do {                                    /* dx * 8^k with dy < 8^k */
            shift -= 3;
            dx <<= 3;
            rest >>= 3;
        } while (rest);
        step = (ULONG)(LONG)(WORD)bl_divs_w(dx, dy);
        step = (shift & 63) >= 32 ? 0 : step << (shift & 63);     /* LSL.l */
        step = BL_SWAP(step);
        BL_SETW(step, step + step);
        e->step = step;
        e++;
        x = p->x;
        y = p->y;
        if (!BMI_W(y, pv.height)) {            /* lbot/rbot */
            e[-1].lines -= (WORD)(y - pv.height);
            break;
        }
        if (right ? x < 0 : x > last) {         /* roff/loff */
            const struct point *from = p - dir;

            e[-1].lines -= muldiv(right ? x : x - last, from->y - y, from->x - x);
            break;
        }
        p += dir;
        tx = p->x;
        ty = p->y;
    }
    *(ULONG *)e = 0xffffffffUL;
    return (UWORD *)e;
}

/* tops..rside: the outline of the polygon in polypts (n points, twice in a
   row): the top line, clipped at the top edge and at the sides, and the
   edge lists of the left side (backwards through the points) and the
   right side.  NULL if nothing is visible, else the address of the right
   list's end mark (where the original leaves a3). */
static UWORD *outline(UWORD n)
{
    const struct point *p = polypts, *lp, *rp;
    WORD lx, rx, y, ly, ry;
    UWORD k = n - 1;

    /* the top point; for several, lp = the leftmost, rp = the rightmost */
    lx = rx = p->x;
    y = p->y;
    lp = rp = p;
    while (p++, --k != 0xffff) {
        if (BMI_W(y, p->y))                     /* below */
            continue;
        if (y != p->y) {                        /* above: the new top */
            lx = rx = p->x;
            y = p->y;
            lp = rp = p;
        } else if (!BMI_W(lx, p->x)) {
            lx = p->x;
            lp = p;
        } else if (BMI_W(rx, p->x)) {
            rx = p->x;
            rp = p;
        }
    }
    /* the left side starts at the point before lp's copy (ADDA.w n*4),
       the right side at the point after rp */
    lp = (const struct point *)((const UBYTE *)lp + (WORD)(n << 2)) - 1;
    rp++;

    if (y < 0) {                                /* cliptop */
        ly = y;
        while (!(0 < lp->y)) {
            lx = lp->x;
            ly = lp->y;
            if (--lp < rp)
                return NULL;
        }
        lx += muldiv(-ly, lp->x - lx, lp->y - ly);
        ry = rp[-1].y;
        while (!(0 < rp->y)) {
            rx = rp->x;
            ry = rp->y;
            if (lp < ++rp)
                return NULL;
        }
        rx += muldiv(-ry, rp->x - rx, rp->y - ry);
        y = 0;
    }
    if (!BMI_W(lx, pv.width)) {                /* cliprstart */
        WORD last = pv.width - 1;

        ly = y;
        while (!(last > lp->x)) {
            lx = lp->x;
            ly = lp->y;
            if (--lp < rp)
                return NULL;
        }
        ly += muldiv(last - lx, lp->y - ly, lp->x - lx);
        lx = last;
        ry = y;
        while (!BMI_W(ly, rp->y)) {
            rx = rp->x;
            ry = rp->y;
            if (lp < ++rp)
                return NULL;
        }
        rx -= muldiv(rp->x - rx, ry - ly, rp->y - ry);
        y = ly;
    } else if (rx < 0) {                        /* cliplstart */
        ry = y;
        while (!(0 < rp->x)) {
            rx = rp->x;
            ry = rp->y;
            if (lp < ++rp)
                return NULL;
        }
        ry += muldiv(-rx, rp->y - ry, rp->x - rx);
        rx = 0;
        ly = y;
        while (!BMI_W(ry, lp->y)) {
            lx = lp->x;
            ly = lp->y;
            if (--lp < rp)
                return NULL;
        }
        lx -= muldiv(lp->x - lx, ly - ry, lp->y - ly);
        y = ry;
    }
    if (!BMI_W(y, pv.height))                  /* dunstart */
        return NULL;
    pv.top.left = lx;
    pv.top.y = y;
    pv.top.right = rx;
    side(pv.ledges, lx, y, lp, -1, 0);
    return side(pv.redges, rx, y, rp, 1, 1);
}

/* _calcpoly: the n points (backwards for n < 0; x and y swapped for the
   chunky routines) twice into polypts (#395: more than 16 overflow it),
   then the outline */
static UWORD *polygon(WORD n, const struct point *pts, int swap)
{
    int back = n < 0;
    UWORD count = back ? -n : n, k = count - 1;
    struct point *a = back ? polypts + count : polypts, *b = a + count;

    do {
        struct point q = *pts++;

        if (swap) {
            WORD t = q.x;

            q.x = q.y;
            q.y = t;
        }
        if (back) {
            *--a = q;
            *--b = q;
        } else {
            *a++ = q;
            *b++ = q;
        }
    } while (k-- != 0);
    return outline(count);
}

/* -------------------------------------------------------------- chunky */

static void BLITZ_ROUTINE _chunkycls_c(UWORD c BL_REG(d0))
{
    struct bb_coplist *cl = (struct bb_coplist *)blitz_string;
    UBYTE *p = CHUNKY(cl), *q;
    UWORD x = cl->chw - 1, h = cl->chh - 1, n = 31, y;
    WORD mod = cl->chc;

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
static UWORD *chunkypixel(const struct bb_coplist *cl, UWORD x, UWORD y)
{
    return (UWORD *)(CHUNKY(cl) + ((x & 31) << 2) + bl_mulu_w(x >> 5, 132)
                     + bl_mulu_w(y, cl->chc));
}

static void BLITZ_ROUTINE _chunkyplot_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                        UWORD colour BL_REG(d2))
{
    *chunkypixel((struct bb_coplist *)blitz_string, x, y) = colour;
}
BLITZ_THUNK(_chunkyplot, _chunkyplot_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkyplotclip_c(WORD x BL_REG(d0), WORD y BL_REG(d1),
                                            UWORD colour BL_REG(d2))
{
    struct bb_coplist *cl = (struct bb_coplist *)blitz_string;

    if (x < 0 || y < 0 || !BMI_W(x, cl->chw) || !BMI_W(y, cl->chh))
        return;
    *chunkypixel(cl, x, y) = colour;
}
BLITZ_THUNK(_chunkyplotclip, _chunkyplotclip_c, "d1/a0-a1");

/* y2 - y1 + 1 words down from x, y1 (DBRA: y1 > y2 gives about 65536, #417) */
static void chunkyvlin(const struct bb_coplist *cl, UWORD x, UWORD y1, UWORD y2, UWORD colour)
{
    UWORD *p = chunkypixel(cl, x, y1), n = y2 - y1;

    do {
        *p = colour;
        p = (UWORD *)((UBYTE *)p + (WORD)cl->chc);
    } while (n-- != 0);
}

static void BLITZ_ROUTINE _chunkyvlin_c(UWORD x BL_REG(d0), UWORD y1 BL_REG(d1),
                                        UWORD y2 BL_REG(d2), UWORD colour BL_REG(d3))
{
    chunkyvlin((struct bb_coplist *)blitz_string, x, y1, y2, colour);
}
BLITZ_THUNK(_chunkyvlin, _chunkyvlin_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkyvlinclip_c(WORD x BL_REG(d0), WORD y1 BL_REG(d1),
                                            WORD y2 BL_REG(d2), UWORD colour BL_REG(d3))
{
    struct bb_coplist *cl = (struct bb_coplist *)blitz_string;
    WORD h = cl->chh;

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

/* ChunkyPoly: the polygon with x and y swapped, so that a line of the
   outline is a chunky column, filled from its top row down by one blit
   (BLTSIZV/H, D = the colour) or by the CPU (cpu).  The point list runs
   the other way round (count negated).  Returns the a3 the original
   leaves: the CopList, or the top row of the last column (MOVE d7,a3). */
static UBYTE *chunkypoly(WORD n, const struct point *pts, UWORD colour,
                         struct bb_coplist *cl, int cpu)
{
    const struct edge *le = (const struct edge *)pv.ledges;
    const struct edge *re = (const struct edge *)pv.redges;
    UBYTE *a3 = (UBYTE *)cl, *column, *colend, *pixel;
    ULONG top, bottom, tstep, bstep;
    UWORD tlines, blines, bankleft, x, rows = cl->chh, mod = cl->chc;

    pv.height = cl->chw;
    pv.width = cl->chh;
    if (!polygon(-n, pts, 1))
        return a3;
    top = edge_start(pv.top.left, 1);
    x = pv.top.y;
    bottom = edge_start(pv.top.right, 1);
    column = CHUNKY(cl);
    colend = column + (WORD)cl->chc - 8;     /* behind the last column */
    column += (WORD)bl_mulu_w(x >> 5, 132);
    column += (x & 31) << 2;
    bankleft = ~x & 31;
    if (!next_edge(&le, &tlines, &tstep))
        return a3;
    BL_SETW(tstep, (WORD)tstep >> 1);           /* ASR #1 (.w): integer */
    if (!next_edge(&re, &blines, &bstep))
        return a3;
    BL_SETW(bstep, (WORD)bstep >> 1);
    if (!cpu) {
        BLITWAIT();
        CHIPW(BLTADAT) = colour;
        CHIPL(BLTAFWM) = 0xffffffff;
        CHIPW(BLTDMOD) = mod - 2;
        CHIPL(BLTCON0) = 0x01f00000;            /* D = A */
    }
    for (;;) {                                  /* ploop */
        UWORD row, last;

        top = edge_step(top, tstep, 1);
        bottom = edge_step(bottom, bstep, 1);
        if (!BMI_W(top, rows))
            break;
        row = (WORD)top < 0 ? 0 : (UWORD)top;
        a3 = (UBYTE *)(LONG)(WORD)row;
        pixel = column + bl_mulu_w(row, mod);
        last = bottom;
        if (!BMI_W(last, rows))
            last = rows - 1;
        last -= row;
        if ((WORD)last < 0)
            break;
        if (!cpu) {
            BLITWAIT();
            CHIPL(BLTDPT) = (ULONG)pixel;
            CHIPW(BLTSIZV) = last + 1;
            CHIPW(BLTSIZH) = 1;
        } else {
            do {                                /* vloop */
                *(UWORD *)pixel = colour;
                pixel += (WORD)mod;
            } while (last-- != 0);
        }
        column += 4;
        if (bankleft-- == 0) {
            column += 4;
            bankleft = 31;
        }
        if (column == colend)
            break;
        top = edge_step(top, tstep, 1);
        bottom = edge_step(bottom, bstep, 1);
        if (tlines-- == 0) {
            if (!next_edge(&le, &tlines, &tstep))
                break;
            BL_SETW(tstep, (WORD)tstep >> 1);
        }
        if (blines-- == 0) {
            if (!next_edge(&re, &blines, &bstep))
                break;
            BL_SETW(bstep, (WORD)bstep >> 1);
        }
    }
    return a3;
}

/* ChunkyPoly n,points,colour (blitter) / n,points,colour,noblitter (the 4
   argument form is always the CPU version, the argument is ignored) */
static void BLITZ_ROUTINE _chunkypoly_c(WORD n BL_REG(d0), ULONG pts BL_REG(d1),
                                        UWORD colour BL_REG(d2))
{
    blitz_string = chunkypoly(n, (const struct point *)pts, colour,
                              (struct bb_coplist *)blitz_string, 0);
}
BLITZ_THUNK(_chunkypoly, _chunkypoly_c, "d1/a0-a1");

static void BLITZ_ROUTINE _chunkypoly2_c(WORD n BL_REG(d0), ULONG pts BL_REG(d1),
                                         UWORD colour BL_REG(d2))
{
    blitz_string = chunkypoly(n, (const struct point *)pts, colour,
                              (struct bb_coplist *)blitz_string, 1);
}
BLITZ_THUNK(_chunkypoly2, _chunkypoly2_c, "d1/a0-a1");

/* ---------------------------------------------------------- BitMapPoly */

/* the span of a line from the edge positions (twice the x; rtclip = twice
   the last x of the clip): the masks to the blitter, the byte offset of
   its first word and the number of words - 1; 0 if the polygon ends here */
static int span(ULONG left, ULONG right, UWORD rtclip, UWORD *first, UWORD *words)
{
    UWORD xs, xe;

    if ((WORD)left > (WORD)rtclip)
        return 0;
    xs = (WORD)left < 0 ? 0 : (UWORD)left;
    if ((WORD)right < 0)
        return 0;
    xe = rtclip;
    if (!BMI_W(xe, right))
        xe = right;
    BLITWAIT();
    CHIPW(BLTAFWM) = lup1[(xs & 30) >> 1];
    CHIPW(BLTALWM) = lup2[(xe & 30) >> 1];
    xs >>= 5;
    *words = (xe >> 5) - xs;
    *first = xs + xs;
    return (WORD)*words >= 0;
}

/* BitMapPoly (RenderPoly on a bitmap): one blit per line and plane, A =
   $FFFF in the span's masks, D = C = the plane, set or cleared by the
   plane's colour bit.  Returns the a3 the original leaves: the bitmap, the
   end mark of the right edge list, or the last plane line blitted. */
static UBYTE *bitmappoly(WORD n, const struct point *pts, UWORD colour, struct bb_bitmap *bm)
{
    const struct edge *le = (const struct edge *)pv.ledges;
    const struct edge *re = (const struct edge *)pv.redges;
    UBYTE *a3;
    ULONG left, right, lstep, rstep, rowoff;
    UWORD llines, rlines, rtclip, first, words, plane;

    pv.width = bm->width;
    pv.height = bm->height;
    a3 = (UBYTE *)polygon(n, pts, 0);
    if (!a3)
        return (UBYTE *)bm;
    rowoff = bl_mulu_w(pv.top.y, bm->bpr);
    rtclip = (bm->width - 1) * 2;
    left = edge_start(pv.top.left, 2);
    right = edge_start(pv.top.right, 2);
    if (!next_edge(&le, &llines, &lstep) || !next_edge(&re, &rlines, &rstep))
        return a3;
    left = edge_step(left, lstep, 2);
    right = edge_step(right, rstep, 2);
    BLITWAIT();
    CHIPW(BLTADAT) = 0xffff;
    CHIPW(BLTCON1) = 0;
    while (span(left, right, rtclip, &first, &words)) {     /* pploop */
        plane = 0;
        do {                                    /* nxpolyplane */
            BLITWAIT();
            CHIPW(BLTCON0) = 0x3fa;
            if (!((UBYTE)colour & (1 << (plane & 7))))
                CHIPW(BLTCON0) = 0x30a;
            a3 = bm->planes[plane] + rowoff + (WORD)first;
            CHIPL(BLTCPT) = (ULONG)a3;
            CHIPL(BLTDPT) = (ULONG)a3;
            CHIPW(BLTSIZE) = words + 65;
        } while (++plane != bm->flagsdepth);         /* depth 0: 65536 (#393) */
        rowoff += bm->bpr;
        left = edge_step(left, lstep, 2);
        right = edge_step(right, rstep, 2);
        if (llines-- == 0 && !next_edge(&le, &llines, &lstep))
            break;
        if (rlines-- == 0 && !next_edge(&re, &rlines, &rstep))
            break;
        left = edge_step(left, lstep, 2);
        right = edge_step(right, rstep, 2);
    }
    return a3;
}

/* the dither form (also RenderPoly after RenderToBitmap b,d): one blit per
   line over all planes (BLTSIZE height = depth, the modulos step from plane
   to plane: they assume evenly spaced planes, #418), B = two alternating
   lines of the dither bitmap at colour * 2.  Returns the a3 the original
   leaves: the bitmap, the end mark of the right edge list, or the dither
   line for the next line. */
static UBYTE *bitmappoly_dither(WORD n, const struct point *pts, UWORD colour,
                                struct bb_bitmap *bm, struct bb_bitmap *dither)
{
    const struct edge *le = (const struct edge *)pv.ledges;
    const struct edge *re = (const struct edge *)pv.redges;
    UBYTE *a3, *row, *other, *t;
    ULONG left, right, lstep, rstep;
    UWORD llines, rlines, rtclip, first, words, srcmod, negmod, height, mod;

    pv.width = bm->width;
    pv.height = bm->height;
    a3 = (UBYTE *)polygon(n, pts, 0);
    if (!a3)
        return (UBYTE *)bm;
    srcmod = bm->planes[1] - bm->planes[0] - 2;
    negmod = (bm->planes[1] - bm->planes[0]) - (dither->planes[1] - dither->planes[0]);
    height = (bm->flagsdepth << 6) + 1;
    a3 = dither->planes[0] + bl_mulu_w(colour * 2, dither->bpr);
    other = a3 + (WORD)dither->bpr;
    if (pv.top.y & 1) {
        t = a3;
        a3 = other;
        other = t;
    }
    rtclip = (bm->width - 1) * 2;
    row = bm->planes[0] + bl_mulu_w(pv.top.y, bm->bpr);
    left = edge_start(pv.top.left, 2);
    right = edge_start(pv.top.right, 2);
    if (!next_edge(&le, &llines, &lstep) || !next_edge(&re, &rlines, &rstep))
        return a3;
    left = edge_step(left, lstep, 2);
    right = edge_step(right, rstep, 2);
    BLITWAIT();
    CHIPL(BLTCON0) = 0x07ca0000;                /* D = B in A, C outside */
    CHIPW(BLTADAT) = 0xffff;
    while (span(left, right, rtclip, &first, &words)) {     /* dploop */
        mod = srcmod - words - words;
        CHIPW(BLTCMOD) = mod;
        CHIPW(BLTDMOD) = mod;
        CHIPW(BLTBMOD) = mod + negmod;
        CHIPL(BLTCPT) = (ULONG)(row + (WORD)first);
        CHIPL(BLTBPT) = (ULONG)a3;
        CHIPL(BLTDPT) = (ULONG)(row + (WORD)first);
        CHIPW(BLTSIZE) = words + height;
        row += (WORD)bm->bpr;
        t = a3;
        a3 = other;
        other = t;
        left = edge_step(left, lstep, 2);
        right = edge_step(right, rstep, 2);
        if (llines-- == 0 && !next_edge(&le, &llines, &lstep))
            break;
        if (rlines-- == 0 && !next_edge(&re, &rlines, &rstep))
            break;
        left = edge_step(left, lstep, 2);
        right = edge_step(right, rstep, 2);
    }
    return a3;
}

static void BLITZ_ROUTINE _bitmappoly_c(WORD n BL_REG(d0), ULONG pts BL_REG(d1),
                                        UWORD colour BL_REG(d2))
{
    blitz_string = bitmappoly(n, (const struct point *)pts, colour,
                              (struct bb_bitmap *)blitz_string);
}
BLITZ_THUNK(_bitmappoly, _bitmappoly_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitmappoly2_c(WORD n BL_REG(d0), ULONG pts BL_REG(d1),
                                         UWORD colour BL_REG(d2),
                                         struct bb_bitmap *dither BL_REG(a0))
{
    blitz_string = bitmappoly_dither(n, (const struct point *)pts, colour,
                                     (struct bb_bitmap *)blitz_string, dither);
}
BLITZ_THUNK(_bitmappoly2, _bitmappoly2_c, "d1/a0-a1");

/* ------------------------------------------------------------- Render */

/* before any RenderTo...: _bitmappoly on a3 = the string pointer (#420) */
static UBYTE *render_a3(WORD n, const struct point *pts, UWORD colour, UBYTE *a3)
{
    return bitmappoly(n, pts, colour, (struct bb_bitmap *)a3);
}

static UBYTE *render_bitmap(WORD n, const struct point *pts, UWORD colour, UBYTE *a3)
{
    (void)a3;
    return bitmappoly(n, pts, colour, bmap0);
}

/* BUG(orig) (#421): the bitmaps are swapped - draws on the dither bitmap */
static UBYTE *render_dither(WORD n, const struct point *pts, UWORD colour, UBYTE *a3)
{
    (void)a3;
    return bitmappoly_dither(n, pts, colour, bmap1, bmap0);
}

/* the colour through the table, index as a word (ADD d2,d2; #422) */
static UBYTE *render_chunky(WORD n, const struct point *pts, UWORD colour, UBYTE *a3)
{
    (void)a3;
    return chunkypoly(n, pts, cols[(WORD)(colour << 1) >> 1], chunk0, 0);
}

static void BLITZ_ROUTINE _rendertobitmap_c(struct bb_bitmap *bm BL_REG(a0))
{
    bmap0 = bm;
    polyvector = render_bitmap;
}
BLITZ_THUNK(_rendertobitmap, _rendertobitmap_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _rendertobitmap2_c(struct bb_bitmap *bm BL_REG(a0),
                                             struct bb_bitmap *dither BL_REG(a1))
{
    bmap0 = bm;
    bmap1 = dither;
    polyvector = render_dither;
}
BLITZ_THUNK(_rendertobitmap2, _rendertobitmap2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _rendertochunky_c(struct bb_coplist *cl BL_REG(a0))
{
    chunk0 = cl;
    polyvector = render_chunky;
}
BLITZ_THUNK(_rendertochunky, _rendertochunky_c, "d0-d1/a0-a1");

/* split: ASL #4,d1:ADD d1,a0 - the CopList 16 bytes per split further */
static void BLITZ_ROUTINE _rendertochunky2_c(UWORD split BL_REG(d1), UBYTE *cl BL_REG(a0))
{
    chunk0 = (struct bb_coplist *)(cl + (WORD)(split << 4));
    polyvector = render_chunky;
}
BLITZ_THUNK(_rendertochunky2, _rendertochunky2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE _renderpoly_c(WORD n BL_REG(d0), ULONG pts BL_REG(d1),
                                        UWORD colour BL_REG(d2))
{
    blitz_string = polyvector(n, (const struct point *)pts, colour, blitz_string);
}
BLITZ_THUNK(_renderpoly, _renderpoly_c, "d1/a0-a1");
