/*
 * 2dlib - command library 119 (#sis2dlib, "simons 2d library"): drawing
 * on Blitz bitmaps.  Circle Circlef Box Boxf Line FreeFill AllocFill ReMap
 * FloodFill Poly Polyf BitPlanesBitMap ClipBlit ClipBlitMode BitMapWindow
 * BitMapOrigin.
 *
 * C translation of bblibsrc/2dlib (the CD binary matches 2dlib, not the
 * older sis2dlib.bb: it has BitMapOrigin, Line with width, ClipBlit with
 * excess planes); binary of the CD's acidlibs checked against the source.
 *
 * Plain C with the integer semantics of the original: word arithmetic
 * that wraps, MULS/MULU/DIVS as instructions (DIVS keeps the dividend on
 * overflow), CMP/BMI (sign of the 16 bit difference) where the original
 * tests that, plane loops counted as the original counts them (CMP.b on
 * the depth byte, CMP.w, DBRA), colours tested as the register the
 * original tests.  Lines, filled boxes, polygon fill, flood fill masks,
 * filled ellipses and ClipBlit program the blitter (chip registers through
 * a pointer, BTST #6 of DMACONR twice as wait); Circle and the flood fill
 * itself draw with the CPU.  The Blitz/Amiga variants ($A001) own the
 * blitter in Amiga mode (graphics OwnBlitter/DisownBlitter).
 *
 * Blitz bitmap object: 0 bytes per row, 2 rows, 4 depth, 8 planes[8],
 * 48/50 origin x/y, 52 flags, 54 bytes per row of the window, 56/58
 * clip width/height, 60 colours, 62 "is real".  Shape object (ClipBlit):
 * 0 width, 2 height, 4 depth, 6 bytes per row, 8 bltsize, 10/12 handle,
 * 14 data, 18 cookie (mask).
 *
 * BUG(orig), kept (selection):
 *   - Box adds the origin twice (once itself, once in the line clipper). (#391)
 *   - Circle/Circlef with points above the top edge: MULU of the negative
 *     y gives a large offset that is not rejected - the CPU plot writes
 *     far outside the bitmap / fill buffer. (#392)
 *   - Poly with 1 point loops 65535 times (#396); Polyf with more than 16
 *     points overflows its 32-long point buffer, more than 10 edges on one
 *     side overflow the edge lists l0/l1 (here ledges/redges) (#395).
 *   - AllocFill (statement) takes the bitmap from a3 = the string pointer
 *     (#390), so the routines leave a3 where the original leaves it.
 *   - a depth of 0 makes the plane loops run 65536 (CMP.w) / 256 (CMP.b)
 *     times. (#393)
 *   - FreeFill does not clear the buffer pointer. (#394)
 * Not a bug (statements, a3 has no meaning after them): the routines leave
 * a3 where the original leaves it (bitmap, plane pointer, ...), not the
 * string pointer.
 * The flood fill proper (yofill) is the original assembler: it keeps the
 * lines still to be filled on the 68000 stack without a limit, which C
 * cannot do between its own frames.
 * DIFF (memory layout, unavoidable): what such an overflow hits - in the
 * original the code behind polypts / l1, here the data behind them.
 */
#include "blitzlib.h"
#include "blitzobj.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (C routines / thunks / prefixes, defined below) */
void _ellipse(void), _circle(void), _ellipsef(void), _circlef(void);
void _box(void), _boxf(void), _line(void), _lineto(void), _linew(void);
void _freefill(void), _allocfill(void), _remap(void), _remap2(void);
void _floodfill(void), _floodfill2(void), _poly(void), _polyf(void), _polyf2(void);
void _bitplanesbitmap(void), _doclipblit(void), _doclipblitx(void), _cbchk(void);
void _doclipblitmode(void), _bitmapwindow(void), _bitmaporigin(void), _init(void);


/* !libheader {#sis2dlib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_sis2dlib, 0, BL_NORET, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302);
BLITZ_SUBS(_ellipse, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302);
BLITZ_SUBS(_circle, 0, 0);
BLITZ_NAME("Circle", "x,y,rx[,ry],color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_graphicslib, 0x5600, LIB_chipbaselib, BL_LA(2));
BLITZ_SUBS(_ellipsef, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_graphicslib, 0x5600, LIB_chipbaselib, BL_LA(2));
BLITZ_SUBS(_circlef, 0, 0);
BLITZ_NAME("Circlef", "x,y,rx[,ry],color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_box, 0, 0);
BLITZ_NAME("Box", "x1,y1,x2,y2,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_boxf, 0, 0);
BLITZ_NAME("Boxf", "x1,y1,x2,y2,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_line, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_lineto, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_linew, 0, 0);
BLITZ_NAME("Line", "[x1,y1,]x2,y2,color[,width]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_freefill, 0, 0);
BLITZ_NAME("FreeFill", "frees memory allocated for 2d fill routines");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_allocfill, 0, 0);
BLITZ_NAME("AllocFill", "for internal use only (polylib.obj)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_remap, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1082, LIB_chipbaselib, BL_LA(2),
           LIB_graphicslib, 0x5600);
BLITZ_SUBS(_remap2, 0, 0);
BLITZ_NAME("ReMap", "colour# to replace,destination colour# [,BitMap destintion");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_floodfill, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_floodfill2, 0, 0);
BLITZ_NAME("FloodFill", "x,y,colour [,border colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_poly, 0, 0);
BLITZ_NAME("Poly", "numpts,*coords.w,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_polyf, 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_chipbaselib, BL_LA(2), LIB_graphicslib, 0x5600);
BLITZ_SUBS(_polyf2, 0, 0);
BLITZ_NAME("Polyf", "numpts,*coords.w,color[,color2]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_bitmaplib, 0x1181);
BLITZ_SUBS(_bitplanesbitmap, 0, 0);
BLITZ_NAME("BitPlanesBitMap", "srcbitmap,destbitmap,planepick");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_chipbaselib, BL_LA(1), LIB_bitmaplib, 0x1202,
           LIB_graphicslib, 0x5600);
BLITZ_SUBS(_doclipblit, _cbchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_chipbaselib, BL_LA(1), LIB_bitmaplib, 0x1202,
           LIB_graphicslib, 0x5600);
BLITZ_SUBS(_doclipblitx, _cbchk, 0);
BLITZ_NAME("ClipBlit", "Shape#,X,Y,[Excess]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_doclipblitmode, 0, 0);
BLITZ_NAME("ClipBlitMode", "BPLCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_bitmaplib, 0x1181);
BLITZ_SUBS(_bitmapwindow, 0, 0);
BLITZ_NAME("BitMapWindow", "srcbitmap,destbitmap,x,y,w,h");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080);
BLITZ_SUBS(_bitmaporigin, 0, 0);
BLITZ_NAME("BitMapOrigin", "BitMap#,x,y");

BLITZ_NULLSUB(_init, 0, 0);

BLITZ_LIBFIN();


/* -------------------------------------------------------------- objects */

/* the bitmap and the shape: struct bb_bitmap, struct bb_shape (blitzobj.h);
   the bitmap's depth word is flagsdepth, the word at +52 ilword */

/* blitter registers (a long write sets two of them) */
#define BLTCON0 0x40
#define BLTCON1 0x42
#define BLTAFWM 0x44
#define BLTALWM 0x46
#define BLTCPT  0x48
#define BLTBPT  0x4c
#define BLTAPT  0x50
#define BLTDPT  0x54
#define BLTSIZE 0x58
#define BLTSIZV 0x5c
#define BLTCMOD 0x60
#define BLTBMOD 0x62
#define BLTAMOD 0x64
#define BLTDMOD 0x66
#define BLTADAT 0x74
#define CW(chip, o)    BL_CUSTOMW(chip, o)
#define CL(chip, o)    BL_CUSTOML(chip, o)
#define BLITWAIT(chip) BL_BLITWAIT(chip)

/* a polygon point as the caller gives it (words x, y) */
struct point {
    WORD x, y;
};

/* an edge of one side of a filled polygon: the lines it covers - 1 and
   the x step per half line (edge_step); the long -1 ends a list */
struct edge {
    WORD lines;
    ULONG step;
} BL_PACKED;

/* (WORD)(a - b) < 0: the original's CMP/SUB with BMI - the sign of the 16
   bit difference, which wraps on overflow (unlike a signed compare) */
#define BMI_W(a, b) ((WORD)((a) - (b)) < 0)

/* ------------------------------------------------------------ variables */

static struct point polypts[32];        /* Polyf: the n points twice */
/* in the order of the original, so that an overflow of ledges runs into
   redges */
static struct {
    struct {
        WORD left, y, right;            /* the first line of the outline */
    } top;
    UWORD ledges[32], redges[32];       /* struct edge lists (l0, l1) */
} pv;
static ULONG fbsize;
/* used by the assembler part (yofill): not static */
ULONG fbloc;                            /* the fill buffer */
UWORD coords[2];                        /* FloodFill: start point */
UWORD fcol;                             /* FloodFill: colour */
UWORD widbytes;
UBYTE flags[2];
static UWORD bigsize[2];                /* Circlef: BLTSIZV, BLTSIZH */
static WORD cpos[2];                    /* the end of the last Line (LineTo) */
static UWORD bcon0 = 0xfca;             /* ClipBlitMode */

static const UWORD lup1[16] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};
static const UWORD lup2[16] = {
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff,
};

static const char mess1[] = "SHAPE UNKNOWNST TO ME";

/* ---------------------------------------------------------- arithmetic */

/* MULS a,b : DIVS c - the quotient word (on overflow DIVS leaves the
   product, then its low word) */
static WORD muldiv(WORD a, WORD b, WORD c)
{
    return (WORD)bl_divs_w((ULONG)bl_muls_w(a, b), c);
}

/* 32 x 16 bit product, the low long (two MULUs) */
static ULONG mul32x16(ULONG a, UWORD b)
{
    return bl_mulu_w(a, b) + (bl_mulu_w(a >> 16, b) << 16);
}

/* ----------------------------------------------- memlib for the buffers */

static ULONG memalloc(ULONG size, ULONG flags_)
{
    struct bl_regs r = { .d0 = size, .d1 = flags_ };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void memfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* ------------------------------------------------- BitMapOrigin & co */

static void BLITZ_ROUTINE _bitmaporigin_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                          struct bb_bitmap *bm BL_REG(a0))
{
    if (!bm->isreal)
        return;
    bm->xorigin = x;
    bm->yorigin = y;
}
BLITZ_THUNK(_bitmaporigin, _bitmaporigin_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitplanesbitmap_c(ULONG pick BL_REG(d2), struct bb_bitmap *src BL_REG(a0),
                                             struct bb_bitmap *dst BL_REG(a1))
{
    UWORD n = 0, i = 0;
    UBYTE **srcplane = src->planes, **dstplane = dst->planes;

    if (!src->isreal || dst->isreal)
        return;
    dst->bpr = src->bpr;
    dst->rows = src->rows;
    do {
        UBYTE *p = *srcplane++;

        if (pick & (1UL << (i & 31))) {
            n++;
            *dstplane++ = p;
        }
        i++;
    } while (i != src->flagsdepth);
    blitz_string = (UBYTE *)dstplane;           /* a3 left behind as LEA 8(a1),a3 */
    dst->flagsdepth = n;
    if (!n)
        return;
    dst->ilword = 0;
    dst->width = src->width;
    dst->height = src->height;
    dst->isreal = 1;
}
BLITZ_THUNK(_bitplanesbitmap, _bitplanesbitmap_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitmapwindow_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                          UWORD w BL_REG(d4), UWORD h BL_REG(d5),
                                          struct bb_bitmap *src BL_REG(a0),
                                          struct bb_bitmap *dst BL_REG(a1))
{
    UWORD n;
    ULONG off;
    UBYTE **srcplane = src->planes, **dstplane = dst->planes;

    if (!src->isreal || dst->isreal)
        return;
    x = (x >> 4) * 2;                           /* round to nearest word */
    w = (w >> 4) * 2;
    off = (ULONG)y * src->bpr + (LONG)(WORD)x;
    dst->bpr = src->bpr;
    dst->rows = h;
    dst->flagsdepth = src->flagsdepth;
    n = src->flagsdepth - 1;
    do
        *dstplane++ = *srcplane++ + off;
    while (n-- != 0);
    blitz_string = (UBYTE *)dstplane;           /* a3 left behind */
    dst->ilword = src->ilword | 1;
    dst->planedist = w;
    dst->width = w << 3;
    dst->height = h;
    dst->colours = src->colours;
    dst->isreal = 1;
}
BLITZ_THUNK(_bitmapwindow, _bitmapwindow_c, "d1/a0-a1");

/* ---------------------------------------------------- fill buffer */

static void BLITZ_ROUTINE _init_c(void)
{
    fbloc = 0;
    fbsize = 0;
}
BLITZ_THUNK(_init, _init_c, "d0-d1/a0-a1");

/* _allocfill: the fill buffer for bm (clip width / 8 * height bytes,
   chip), grown when needed; returns it */
static ULONG allocfill(const struct bb_bitmap *bm)
{
    ULONG size = bl_mulu_w(bm->width >> 3, bm->height);

    if ((LONG)(size - fbsize) > 0) {           /* BMI/BEQ yokay */
        if (fbsize)
            memfree(fbloc, fbsize);
        fbsize = size;
        fbloc = memalloc(size, MEMF_CHIP);
    }
    return fbloc;
}

static void BLITZ_ROUTINE _allocfill_c(void)
{
    allocfill((struct bb_bitmap *)blitz_string);   /* BUG(orig) (#390): no bitmap here */
}
BLITZ_THUNK(_allocfill, _allocfill_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE _freefill_c(void)
{
    if (fbsize) {
        memfree(fbloc, fbsize);
        fbsize = 0;
    }
    return 0;
}
BLITZ_THUNK(_freefill, _freefill_c, "d1/a0-a1");

/* -------------------------------------------------------------- lines */

/* clipme: the line x1,y1 - x2,y2 (plus the origin), clipped to the
   bitmap, drawn with the blitter in line mode plane by plane: colour bit
   1 sets, 0 clears, a negative colour complements.  Returns the a3 the
   original leaves: the bitmap, or behind the plane pointers it used. */
static UBYTE *drawline(struct bb_bitmap *bm, UBYTE *chip, WORD x1, WORD y1, WORD x2, WORD y2,
                       UWORD colour)
{
    WORD w = bm->width, h = bm->height, t, dx, dy, err;
    UWORD octant, major2, con0, extra, size;
    ULONG rot, start, len;
    UBYTE **plane = bm->planes, i = 0;
    int xneg, yneg;

    x1 += bm->xorigin;
    x2 += bm->xorigin;
    y1 += bm->yorigin;
    y2 += bm->yorigin;
    if (BMI_W(x2, x1)) {
        t = x1, x1 = x2, x2 = t;
        t = y1, y1 = y2, y2 = t;
    }
    if (x2 < 0 || !BMI_W(x1, w))
        return (UBYTE *)bm;
    if (!BMI_W(x2, w)) {                        /* clip right */
        y2 -= muldiv(x2 - w + 1, y2 - y1, x2 - x1);
        x2 = w - 1;
    }
    if (x1 < 0) {                               /* clip left */
        y1 -= muldiv(x1, y2 - y1, x2 - x1);
        x1 = 0;
    }
    if (BMI_W(y2, y1)) {
        t = x1, x1 = x2, x2 = t;
        t = y1, y1 = y2, y2 = t;
    }
    if (y2 < 0 || !BMI_W(y1, h))
        return (UBYTE *)bm;
    if (!BMI_W(y2, h)) {                        /* clip bottom */
        x2 -= muldiv(y2 - h + 1, x2 - x1, y2 - y1);
        y2 = h - 1;
    }
    if (y1 < 0) {                               /* clip top */
        x1 -= muldiv(y1, x2 - x1, y2 - y1);
        y1 = 0;
    }

    /* CalcOctant: dx = the major, dy = the minor distance */
    dx = x2 - x1;
    dy = y2 - y1;
    xneg = dx < 0;
    if (xneg)
        dx = -dx;
    yneg = dy < 0;
    if (yneg)
        dy = -dy;
    if (BMI_W(dx, dy)) {
        t = dx, dx = dy, dy = t;
        octant = xneg ? (yneg ? 12 : 8) : (yneg ? 4 : 0);
    } else
        octant = xneg ? (yneg ? 28 : 20) : (yneg ? 24 : 16);
    octant += 1;                                /* LINE */

    /* the word of the start point: ROR.l #4 of x1 gives the word index in
       the lower and the shift (bits 12-15) in the upper word */
    rot = (ULONG)(LONG)x1;
    rot = rot >> 4 | rot << 28;
    start = bl_mulu_w(y1, bm->bpr) + (WORD)rot + (WORD)rot;
    con0 = rot >> 16;
    major2 = dx + dx;
    len = (ULONG)(LONG)(WORD)major2 << 5;
    extra = len >> 16;                          /* blits of 1024 pixels first */
    size = (UWORD)len + 0x42;                   /* height dx + 1, width 2 */
    BLITWAIT(chip);
    CW(chip, BLTBMOD) = dy << 2;
    err = (WORD)(dy << 2) - (WORD)major2;
    if (err < 0)
        octant |= 64;                           /* SIGN */
    CW(chip, BLTAMOD) = err - major2;
    CW(chip, BLTCMOD) = bm->bpr;
    CW(chip, BLTDMOD) = bm->bpr;
    CL(chip, BLTAFWM) = 0xffffffff;
    /* the plane loop counts with CMP.b on the depth byte: 0 gives 256 (#393) */
    do {
        UBYTE *p = *plane++ + start;
        UWORD con = (UWORD)(con0 | 0xbfa);

        if ((WORD)colour < 0)
            con = con0 | 0xb5a;                 /* complement */
        else if (!((UBYTE)colour & (1 << (i & 7))))
            con ^= 0xf0;                        /* clear */
        BLITWAIT(chip);
        CW(chip, BLTADAT) = 0x8000;
        CL(chip, BLTAPT) = (ULONG)(LONG)err;
        CL(chip, BLTCPT) = (ULONG)p;
        CL(chip, BLTDPT) = (ULONG)p;
        CW(chip, BLTCON0) = con;
        CW(chip, BLTCON1) = octant;
        if (extra) {
            UWORD n = extra - 1;

            do {
                CW(chip, BLTSIZE) = 2;
                BLITWAIT(chip);
            } while (n-- != 0);
        }
        CW(chip, BLTSIZE) = size;
    } while (++i != (UBYTE)bm->flagsdepth);
    return (UBYTE *)plane;
}

static UBYTE *line(struct bb_bitmap *bm, UBYTE *chip, WORD x1, WORD y1, WORD x2, WORD y2,
                   UWORD colour)
{
    cpos[0] = x2;
    cpos[1] = y2;
    return drawline(bm, chip, x1, y1, x2, y2, colour);
}

/* Line x,y,colour: from x,y to the end of the last line */
static UBYTE *lineto(struct bb_bitmap *bm, UBYTE *chip, WORD x, WORD y, UWORD colour)
{
    WORD x2 = cpos[0], y2 = cpos[1];

    cpos[0] = x;
    cpos[1] = y;
    return drawline(bm, chip, x, y, x2, y2, colour);
}

/* Line with width: for w = width .. 0 four lines, each with one of the
   coordinates + w; a3 is restored after each line */
static UBYTE *linew(struct bb_bitmap *bm, UBYTE *chip, WORD x1, WORD y1, WORD x2, WORD y2,
                    UWORD colour, UWORD width)
{
    UWORD w = width;

    do {
        line(bm, chip, x1 + w, y1, x2, y2, colour);
        line(bm, chip, x1, y1 + w, x2, y2, colour);
        line(bm, chip, x1, y1, x2 + w, y2, colour);
        line(bm, chip, x1, y1, x2, y2 + w, colour);
    } while (w-- != 0);
    return (UBYTE *)bm;
}

/* ----------------------------------------------------------- Box, Boxf */

/* Box: four lines; the origin is added here and again in the line
   clipper (BUG(orig) #391) */
static UBYTE *box(struct bb_bitmap *bm, UBYTE *chip, WORD x1, WORD y1, WORD x2, WORD y2,
                  UWORD colour)
{
    WORD t;

    x1 += bm->xorigin;
    x2 += bm->xorigin;
    y1 += bm->yorigin;
    y2 += bm->yorigin;
    if (BMI_W(x2, x1))
        t = x1, x1 = x2, x2 = t;
    if (BMI_W(y2, y1))
        t = y1, y1 = y2, y2 = t;
    drawline(bm, chip, x1, y1, x1, y2 - 1, colour);
    drawline(bm, chip, x2, y1 + 1, x2, y2, colour);
    drawline(bm, chip, x1 + 1, y1, x2, y1, colour);
    return drawline(bm, chip, x1, y2, x2 - 1, y2, colour);
}

/* dobof: the filled rectangle x1,y1 - x2,y2 (clipped, at most 990 pixels
   wide), one blit per plane (more than 1023 lines: parts of 1024 first);
   colour bit 1 sets, 0 clears; a negative colour (its low word) only
   complements the planes of its 1 bits.  Returns the a3 the original
   leaves (behind the plane pointers). */
static UBYTE *dobof(struct bb_bitmap *bm, UBYTE *chip, UWORD x1, UWORD y1, UWORD x2, UWORD y2,
                    ULONG colour)
{
    UBYTE **plane = bm->planes, *p;
    ULONG size, off;
    UWORD words, parts, mod, n, i = 0;

    BLITWAIT(chip);
    CW(chip, BLTAFWM) = lup1[x1 & 15];
    CW(chip, BLTALWM) = lup2[x2 & 15];
    CW(chip, BLTADAT) = 0xffff;
    CW(chip, BLTCON1) = 0;
    size = (ULONG)(LONG)(WORD)(y2 - y1 + 1) << 6;
    off = bl_mulu_w(y1, bm->bpr) + 2 * (x1 >> 4);
    words = (x2 >> 4) - (x1 >> 4) + 1;
    BL_SETW(size, size | words);
    mod = bm->bpr - words - words;
    CW(chip, BLTDMOD) = mod;
    CW(chip, BLTCMOD) = mod;
    parts = size >> 16;                         /* SWAP: 1024 line parts */
    if (!parts) {
        if ((WORD)colour >= 0) {
            do {                                /* nxp1 */
                BLITWAIT(chip);
                p = *plane++ + off;
                CL(chip, BLTCPT) = (ULONG)p;
                CL(chip, BLTDPT) = (ULONG)p;
                CW(chip, BLTCON0) = 0x3fa;
                if (!(colour & (1UL << (i & 31))))
                    CW(chip, BLTCON0) = 0x30a;
                CW(chip, BLTSIZE) = size;
            } while (++i != bm->flagsdepth);         /* depth 0: 65536 (#393) */
            return (UBYTE *)plane;
        }
        do {                                    /* nxp2 */
            BLITWAIT(chip);
            p = *plane++;
            if (colour & (1UL << (i & 31))) {
                p += off;
                CL(chip, BLTCPT) = (ULONG)p;
                CL(chip, BLTDPT) = (ULONG)p;
                CW(chip, BLTCON0) = 0x35a;
                CW(chip, BLTSIZE) = size;
            }
        } while (++i != bm->flagsdepth);
        return (UBYTE *)plane;
    }
    do {                                        /* longun */
        BLITWAIT(chip);
        p = *plane++;
        if ((WORD)colour >= 0) {
            p += off;
            CL(chip, BLTCPT) = (ULONG)p;
            CL(chip, BLTDPT) = (ULONG)p;
            CW(chip, BLTCON0) = 0x3fa;
            if (!(colour & (1UL << (i & 31))))
                CW(chip, BLTCON0) = 0x30a;
        } else {
            if (!(colour & (1UL << (i & 31))))
                continue;                       /* lmissp */
            p += off;
            CL(chip, BLTCPT) = (ULONG)p;
            CL(chip, BLTDPT) = (ULONG)p;
            CW(chip, BLTCON0) = 0x35a;
        }
        n = parts - 1;
        do {
            CW(chip, BLTSIZE) = size & 63;      /* height 0 = 1024 */
            BLITWAIT(chip);
        } while (n-- != 0);
        CW(chip, BLTSIZE) = size;
    } while (++i != bm->flagsdepth);
    return (UBYTE *)plane;
}

/* Boxf: the rectangle (plus the origin) clipped (a box starting in the
   last column or line is not drawn), in parts of 990 pixels */
static UBYTE *boxf(struct bb_bitmap *bm, UBYTE *chip, WORD x1, WORD y1, WORD x2, WORD y2,
                   ULONG colour)
{
    WORD t, last_x, last_y;

    x1 += bm->xorigin;
    x2 += bm->xorigin;
    y1 += bm->yorigin;
    y2 += bm->yorigin;
    if (BMI_W(x2, x1))
        t = x1, x1 = x2, x2 = t;
    if (BMI_W(y2, y1))
        t = y1, y1 = y2, y2 = t;
    last_x = bm->width - 1;
    last_y = bm->height - 1;
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 < 0 || y2 < 0)
        return (UBYTE *)bm;
    if (x1 >= last_x || y1 >= last_y)           /* BGE nobx */
        return (UBYTE *)bm;
    if (!(x2 < last_x))
        x2 = last_x;
    if (!(y2 < last_y))
        y2 = last_y;
    while (!BMI_W(x2 - x1, 991)) {             /* noclip2 */
        dobof(bm, chip, x1, y1, x1 + 989, y2, colour);
        x1 += 990;
    }
    return dobof(bm, chip, x1, y1, x2, y2, colour);
}

/* -------------------------------------------------------------- ellipses */

/* the midpoint ellipse (calcellipse): x, y from the top point, decision
   d, dx = 2 b^2 x, dy = 2 a^2 y */
struct ellipse {
    UWORD x, y;
    ULONG d, dx, dy, asq, bsq, asq2, bsq2;
};

static void calcellipse(struct ellipse *e, UWORD a, UWORD b)
{
    e->asq = bl_mulu_w(a, a);
    e->asq2 = e->asq << 1;
    e->bsq = bl_mulu_w(b, b);
    e->bsq2 = e->bsq << 1;
    e->d = e->bsq - mul32x16(e->asq, b) + (ULONG)((LONG)e->asq >> 2);
    e->dx = 0;
    e->dy = mul32x16(e->asq2, b);
    e->x = 0;
    e->y = b;
}

/* the steps in x and in y */
static void ellipse_x(struct ellipse *e)
{
    e->x++;
    e->dx += e->bsq2;
    e->d += e->dx;
}

static void ellipse_y(struct ellipse *e)
{
    e->y--;
    e->dy -= e->asq2;
    e->d -= e->dy;
}

/* dxgtdy: the decision for the second region */
static void ellipse_region2(struct ellipse *e)
{
    ULONG t = e->asq - e->bsq;

    t += t << 1;
    t = (ULONG)((LONG)t >> 1);
    t -= e->dx;
    t -= e->dy;
    e->d += (ULONG)((LONG)t >> 1);
}

/* a step of the second region (dodab) */
static void ellipse_step2(struct ellipse *e)
{
    if ((LONG)e->d < 0)
        ellipse_x(e);
    e->d += e->asq;
    ellipse_y(e);
}

/* cplot: the point x, y in colour (complement if negative) with the CPU;
   y is checked only with a signed compare, its unsigned word multiplied
   (#392); the plane loop counts with CMP.b on the depth byte (#393) */
static void cplot(struct bb_bitmap *bm, WORD x, WORD y, UWORD colour)
{
    ULONG off;
    UBYTE bit, i = 0;

    if (!BMI_W(y, bm->height))
        return;
    off = bl_mulu_w(y, bm->bpr);
    if ((LONG)off < 0)
        return;
    if (x < 0 || !BMI_W(x, bm->width))
        return;
    off += (UWORD)x >> 3;
    bit = 1 << ((x & 7) ^ 7);
    if ((WORD)colour < 0) {
        do                                      /* nxpm */
            bm->planes[i][off] ^= bit;
        while (++i != (UBYTE)bm->flagsdepth);
        return;
    }
    do {                                        /* nxpp */
        if (colour & (1UL << (i & 31)))
            bm->planes[i][off] |= bit;
        else
            bm->planes[i][off] &= ~bit;
    } while (++i != (UBYTE)bm->flagsdepth);
}

/* Circle/Ellipse: the outline with the CPU */
static void ellipse(struct bb_bitmap *bm, WORD cx, WORD cy, UWORD a, UWORD b, UWORD colour)
{
    struct ellipse e;
    int second = 0;

    cx += bm->xorigin;
    cy += bm->yorigin;
    calcellipse(&e, a, b);
    for (;;) {                                  /* nxelp */
        cplot(bm, cx + e.x, cy + e.y, colour);
        cplot(bm, cx + e.x, cy - e.y, colour);
        cplot(bm, cx - e.x, cy + e.y, colour);
        cplot(bm, cx - e.x, cy - e.y, colour);
        if (!second) {
            if ((LONG)(e.dx - e.dy) < 0) {
                if ((LONG)e.d > 0)
                    ellipse_y(&e);
                ellipse_x(&e);                  /* nodab */
                e.d += e.bsq;
                continue;
            }
            second = 1;
            ellipse_region2(&e);
        }
        if ((WORD)e.y <= 0)                     /* dooly */
            return;
        ellipse_step2(&e);
    }
}

static void BLITZ_ROUTINE _ellipse_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1), UWORD rx BL_REG(d2),
                                     UWORD ry BL_REG(d3), UWORD c BL_REG(d4))
{
    ellipse((struct bb_bitmap *)blitz_string, x, y, rx, ry, c);
}
BLITZ_THUNK(_ellipse, _ellipse_c, "d1/a0-a1");

static void BLITZ_ROUTINE _circle_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1), UWORD r BL_REG(d2),
                                    UWORD c BL_REG(d3))
{
    ellipse((struct bb_bitmap *)blitz_string, x, y, r, r, c);
}
BLITZ_THUNK(_circle, _circle_c, "d1/a0-a1");

/* a blit of `size`, or with BLTSIZV/BLTSIZH (bigsize) for size 0 */
static void bltsize(UBYTE *chip, UWORD size)
{
    if (size)
        CW(chip, BLTSIZE) = size;
    else
        CL(chip, BLTSIZV) = *(ULONG *)bigsize;
}

/* cplot2: toggle the bit of x, y in the fill buffer, x clipped at last
   (y as in cplot: #392) */
static void cplot2(const struct bb_bitmap *bm, WORD x, WORD y, WORD last)
{
    ULONG off;

    if (!BMI_W(y, bm->height))
        return;
    off = bl_mulu_w(y, bm->bpr);
    if ((LONG)off < 0)
        return;
    if (x < 0)
        return;
    if (!BMI_W(x, last))
        x = last;
    off += (UWORD)x >> 3;
    ((UBYTE *)fbloc)[off] ^= 1 << ((x & 7) ^ 7);
}

/* Circlef/Ellipsef (no origin): the outline as one bit per line into the
   fill buffer (the rectangle of the ellipse cleared first), filled with
   the blitter (descending, exclusive fill), then through it into each
   plane: colour bit 1 sets, 0 clears, a negative colour complements.
   Returns the a3 the original leaves: the bitmap, or behind the plane
   pointers. */
static UBYTE *ellipsef(struct bb_bitmap *bm, UBYTE *chip, WORD cx, WORD cy, UWORD a, UWORD b,
                       UWORD colour)
{
    struct ellipse e;
    UBYTE **plane = bm->planes;
    ULONG planeoff, blitpos;
    WORD x0, y0, x1, y1, wwords, h, lines, words, last;
    UWORD size, mod, k, bits;
    int second = 0;

    allocfill(bm);
    x0 = cx - (WORD)(a + 1);                    /* the radii + 1 */
    if (x0 < 0)
        x0 = 0;
    y0 = cy - (WORD)(b + 1);
    if (y0 < 0)
        y0 = 0;
    x1 = cx + (WORD)(a + 1);
    if (x1 < 0)
        return (UBYTE *)bm;
    y1 = cy + (WORD)(b + 1);
    if (y1 < 0)
        return (UBYTE *)bm;
    x0 = (UWORD)x0 >> 4;                        /* in words */
    x1 = (UWORD)x1 >> 4;
    wwords = bm->width >> 4;
    h = bm->height;
    if (!BMI_W(x0, wwords) || !BMI_W(y0, h))
        return (UBYTE *)bm;
    if (!BMI_W(x1, wwords))
        x1 = wwords - 1;
    if (!BMI_W(y1, h))
        y1 = h - 1;
    lines = y1 - y0 + 1;                        /* bboc2 */
    words = x1 - x0 + 1;
    if (!BMI_W(lines, 1024) || !BMI_W(words, 64)) {     /* bigo */
        bigsize[0] = lines;
        bigsize[1] = words;
        size = 0;
    } else
        size = lines << 6 | (UWORD)words;
    mod = bm->bpr - (words << 1);               /* domod */
    /* descending blits: from the last word of the rectangle */
    planeoff = bl_mulu_w(y1, bm->bpr) + x1 + x1;
    blitpos = planeoff + fbloc;
    BLITWAIT(chip);                             /* clear the rectangle */
    CL(chip, BLTAFWM) = 0xffffffff;
    CL(chip, BLTDPT) = blitpos;
    CW(chip, BLTDMOD) = mod;
    CL(chip, BLTCON0) = 0x01000002;
    bltsize(chip, size);
    BLITWAIT(chip);

    calcellipse(&e, a, b);
    last = bm->width - 1;
    for (;;) {                                  /* nxelp2 */
        if (e.y) {
            cplot2(bm, cx + e.x, cy + e.y, last);
            cplot2(bm, cx - e.x, cy + e.y, last);
        }
        cplot2(bm, cx + e.x, cy - e.y, last);   /* j1e */
        cplot2(bm, cx - e.x, cy - e.y, last);
        if (!second) {
            /* flop9: x steps on the same line are not plotted (the fill
               needs one bit per line) */
            for (;;) {
                if ((LONG)(e.dx - e.dy) >= 0) {
                    second = 1;
                    ellipse_region2(&e);
                    break;
                }
                if ((LONG)e.d > 0) {
                    ellipse_y(&e);
                    ellipse_x(&e);
                    e.d += e.bsq;
                    break;
                }
                ellipse_x(&e);                  /* nodab2 */
                e.d += e.bsq;
            }
            if (!second)
                continue;
        }
        if (!e.y)                               /* dooly2 */
            break;
        ellipse_step2(&e);
    }

    BLITWAIT(chip);                             /* DoFill */
    CL(chip, BLTAPT) = blitpos;
    CL(chip, BLTDPT) = blitpos;
    CW(chip, BLTAMOD) = mod;
    CW(chip, BLTDMOD) = mod;
    CL(chip, BLTCON0) = 0x09f0000a;
    bltsize(chip, size);

    k = bm->flagsdepth - 1;                          /* DoTransfer (DBRA: 0 = 65536) */
    bits = colour;
    do {
        UBYTE *p = *plane++ + planeoff;

        BLITWAIT(chip);
        CL(chip, BLTAPT) = blitpos;
        CL(chip, BLTBPT) = (ULONG)p;
        CL(chip, BLTDPT) = (ULONG)p;
        CW(chip, BLTAMOD) = mod;
        CW(chip, BLTBMOD) = mod;
        CW(chip, BLTDMOD) = mod;
        if ((WORD)colour >= 0) {                /* nxplaner */
            CL(chip, BLTCON0) = 0x0dfc0002;
            if (!(bits & 1))
                CW(chip, BLTCON0) = 0x0d0c;
            bits >>= 1;
        } else
            CL(chip, BLTCON0) = 0x0d3c0002;     /* xort */
        bltsize(chip, size);
    } while (k-- != 0);
    return (UBYTE *)plane;
}

/* --------------------------------------------- mask fetch / write (ReMap) */

/* monosetup: the bitmap in columns of 64 words (BLTSIZE width 0), the last
   column with the rest of the width */
struct columns {
    UWORD last;                 /* byte offset of the last column (finx) */
    UWORD parts;                /* 1024 line parts before each column blit */
    UWORD size;                 /* BLTSIZE of the last column */
};

static void columns(const struct bb_bitmap *bm, struct columns *c)
{
    UWORD words = bm->width >> 4;
    ULONG lines = (ULONG)(LONG)(WORD)bm->height << 6;

    c->last = (words & 0x3fc0) * 2;
    c->parts = lines >> 16;
    if (!(UWORD)lines)
        c->parts--;
    words &= 63;
    if (!words)
        c->last -= 128;
    c->size = (UWORD)lines | words;
}

static void setmods(UBYTE *chip, UWORD m)
{
    CW(chip, BLTCMOD) = m;
    CW(chip, BLTBMOD) = m;
    CW(chip, BLTAMOD) = m;
    CW(chip, BLTDMOD) = m;
}

/* blitsize3: the 1024 line parts and the final size */
static void blitsize3(UBYTE *chip, UWORD parts, UWORD size)
{
    if (parts) {
        UWORD n = parts - 1;

        do {
            CW(chip, BLTSIZE) = size & 63;
            BLITWAIT(chip);
        } while (n-- != 0);
    }
    CW(chip, BLTSIZE) = size;
}

/* one column (at byte offset off) of all planes (DBRA over depth - 1: 0
   gives 65536):
     fetch (dunxx1): the mask of the pixels of colour into mask: D = A
       (plane 0, or its complement), then D = A AND C (C = D = the mask)
     write (dunxx2): colour where the mask is set: D = A OR C (set) or
       NOT A AND C (clear), A = the mask, C = D = the plane */
static void column(UBYTE *chip, const struct bb_bitmap *bm, ULONG colour, WORD off, UBYTE *mask,
                   const struct columns *c, UWORD size, int write)
{
    UBYTE *const *plane = bm->planes;
    UWORD k = bm->flagsdepth - 1, i = 0;

    for (;;) {
        UBYTE *p = *plane++ + off;
        int bit = (colour & (1UL << (i & 31))) != 0;

        if (i)
            BLITWAIT(chip);
        if (!i)
            CL(chip, BLTCON0) = write ? 0x0bfa0000 : 0x0bf00000;
        else
            CW(chip, BLTCON0) = write ? 0x0bfa : 0x0ba0;
        if (!bit)
            CW(chip, BLTCON0) = write ? 0x0b0a : (i ? 0x0b0a : 0x0b0f);
        CL(chip, BLTAPT) = (ULONG)(write ? mask : p);
        CL(chip, BLTCPT) = (ULONG)(write ? p : mask);
        CL(chip, BLTDPT) = (ULONG)(write ? p : mask);
        blitsize3(chip, c->parts, size);
        i++;
        if (k-- == 0)
            break;
    }
}

/* monofetch/monowrite: the mask of colour over the whole bitmap into the
   fill buffer (fetch, allocates it) or colour through the mask into the
   bitmap (write).  Returns the buffer position of the last column and its
   BLTSIZE in *size (FloodFill with border inverts only there). */
static UBYTE *mono(UBYTE *chip, struct bb_bitmap *bm, ULONG colour, int write, UWORD *size)
{
    struct columns c;
    UBYTE *mask = write ? (UBYTE *)fbloc : (UBYTE *)allocfill(bm);
    UWORD off = 0, m;

    columns(bm, &c);
    BLITWAIT(chip);
    CL(chip, BLTAFWM) = 0xffffffff;
    CL(chip, BLTCMOD) = 0;
    CL(chip, BLTAMOD) = 0;
    if (c.last) {
        setmods(chip, bm->bpr - 128);
        do {                                    /* nxxx1 */
            column(chip, bm, colour, off, mask, &c, c.size & 0xffc0, write);
            BLITWAIT(chip);
            off += 128;
            mask += 128;
        } while (off != c.last);
        m = bm->bpr & 127;
        if (!m)
            m = 128;
        setmods(chip, bm->bpr - m);
    }
    column(chip, bm, colour, c.last, mask, &c, c.size, write);
    *size = c.size;
    return mask;
}

/* ------------------------------------------------------------ FloodFill */

/* yofill - the flood fill itself.  It keeps the lines still to be
   filled on the 68000 stack (MOVEM d0-d2,-(a7), -1 as end mark) without
   a limit, as the original; C cannot push onto the stack between its own
   frames, so this part is the original assembler (bitmap in a0, fill
   mask fbloc, colour fcol, start coords).  The wrapper saves what C needs
   (a3-a6 are fixed / callee-saved). */
BLITZ_ASM_ROUTINE(yofill_asm,
"   movem.l d2-d7/a2-a6,-(sp)\n"
"   move.l  a0,a3\n"
"   bsr.s   1f\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
"1: movem.w _coords,d0-d1\n"
"   move.w  #-1,-(sp)\n"
"   move.w  d0,d2\n"
"   not.w   d2\n"
"   and.w   #7,d2\n"
"   lsr.w   #3,d0\n"
"   ext.l   d0\n"
"   move.w  56(a3),d4\n"
"   lsr.w   #3,d4\n"
"   move.w  d4,_widbytes\n"
"   move.w  (a3),d4\n"
"   move.w  d4,d5\n"
"   neg.w   d5\n"
"fillline:\n"
"   move.w  d1,d3\n"
"   mulu.w  (a3),d3\n"
"   add.l   d0,d3\n"
"   move.l  _fbloc,a0\n"
"   add.l   d3,a0\n"
"goleft:\n"
"   btst.b  d2,(a0)\n"
"   beq.s   gotleft\n"
"   addq.w  #1,d2\n"
"   btst    #3,d2\n"
"   beq.s   goleft\n"
"   bclr    #3,d2\n"
"   subq.l  #1,a0\n"
"   subq.w  #1,d3\n"
"   subq.w  #1,d0\n"
"   bpl.s   goleft\n"
"gotleft:\n"
"   move.w  #0,_flags\n"
"drawright:\n"
"   subq.w  #1,d2\n"
"   bpl.s   chkr\n"
"   and.w   #7,d2\n"
"   addq.l  #1,a0\n"
"   addq.w  #1,d3\n"
"   addq.w  #1,d0\n"
"   cmp.w   _widbytes,d0\n"
"   beq.s   gotright\n"
"chkr:\n"
"   btst.b  d2,(a0)\n"
"   beq.s   gotright\n"
"   bclr.b  d2,(a0)\n"
"   lea     8(a3),a1\n"
"   move.w  4(a3),d6\n"
"   subq.w  #1,d6\n"
"   move.w  _fcol,d7\n"
"nxps:\n"
"   move.l  (a1)+,a4\n"
"   add.l   d3,a4\n"
"   lsr.w   #1,d7\n"
"   bcs.s   dosc\n"
"   bclr.b  d2,(a4)\n"
"   bra.s   dosc2\n"
"dosc:\n"
"   bset.b  d2,(a4)\n"
"dosc2:\n"
"   dbra    d6,nxps\n"
"   btst.b  d2,0(a0,d4.w)\n"
"   sne     d6\n"
"   cmp.b   _flags,d6\n"
"   beq.s   nodo\n"
"   not.b   _flags\n"
"   beq.s   nodo\n"
"   addq.w  #1,d1\n"
"   cmp.w   58(a3),d1\n"
"   beq.s   oflo\n"
"   movem.w d0-d2,-(sp)\n"
"oflo:\n"
"   subq.w  #1,d1\n"
"nodo:\n"
"   tst.w   d1\n"
"   beq.s   noup\n"
"   btst.b  d2,0(a0,d5.w)\n"
"   sne     d6\n"
"   cmp.b   _flags+1,d6\n"
"   beq.s   noup\n"
"   not.b   _flags+1\n"
"   beq.s   noup\n"
"   subq.w  #1,d1\n"
"   movem.w d0-d2,-(sp)\n"
"   addq.w  #1,d1\n"
"noup:\n"
"   bra     drawright\n"
"gotright:\n"
"   move.w  (sp)+,d0\n"
"   bmi.s   finito\n"
"   ext.l   d0\n"
"   movem.w (sp)+,d1-d2\n"
"   bra     fillline\n"
"finito:\n"
"   rts");

static void yofill(struct bb_bitmap *bm)
{
    register struct bb_bitmap *r_a0 __asm("a0") = bm;

    __asm volatile("jsr     _yofill_asm"
                   : "+a"(r_a0) : : "d0", "d1", "a1", "cc", "memory");
}

/* FloodFill x,y,colour: fill the area of the colour under the point */
static UBYTE *floodfill(struct bb_bitmap *bm, UBYTE *chip, WORD x, WORD y, UWORD colour)
{
    ULONG off, under = 0;
    UWORD i = 0, size;
    UBYTE bit = ~x & 7;

    coords[0] = x;
    coords[1] = y;
    fcol = colour;
    off = bl_mulu_w(y, bm->bpr) + ((UWORD)x >> 3);
    do {                                        /* the colour under the point */
        if (bm->planes[i][off] & (1 << bit))
            under |= 1UL << (i & 31);
    } while (++i != bm->flagsdepth);
    mono(chip, bm, under, 0, &size);
    BLITWAIT(chip);
    yofill(bm);
    return (UBYTE *)bm;
}

/* FloodFill x,y,colour,border: fill up to the border colour (the mask of
   the border colour inverted - only in the last column of 64 words).  The
   border colour is a word in the register of x (SETW(d0, d3)). */
static UBYTE *floodfill2(struct bb_bitmap *bm, UBYTE *chip, ULONG x, WORD y, UWORD colour,
                         UWORD border)
{
    UBYTE *mask;
    UWORD size;

    coords[0] = x;
    coords[1] = y;
    fcol = colour;
    mask = mono(chip, bm, (x & 0xffff0000UL) | border, 0, &size);
    BLITWAIT(chip);
    CW(chip, BLTCON0) = 0x090f;                 /* invert the mask */
    CL(chip, BLTAPT) = (ULONG)mask;
    CL(chip, BLTDPT) = (ULONG)mask;
    CW(chip, BLTSIZE) = size;
    BLITWAIT(chip);
    yofill(bm);
    return (UBYTE *)bm;
}

/* ReMap from,to[,dst]: the pixels of colour from (in bm) get colour to
   (in dst); returns dst (a3) */
static UBYTE *remap(struct bb_bitmap *bm, UBYTE *chip, ULONG from, ULONG to, struct bb_bitmap *dst)
{
    UWORD size;

    mono(chip, bm, from, 0, &size);
    mono(chip, dst, to, 1, &size);
    return (UBYTE *)dst;
}

/* ------------------------------------------------------------ polygons */

/* Poly: the lines between the points and back to the first; the count
   is decremented after the first line (SUB #1,pfc:BNE): 1 point draws
   65536 lines (#396).  Returns the a3 of the last line. */
static UBYTE *poly(struct bb_bitmap *bm, UBYTE *chip, UWORD n, const struct point *pts,
                   UWORD colour)
{
    const struct point *p = pts;
    UWORD count = n - 1;

    do {                                        /* nxpfc */
        drawline(bm, chip, p[0].x, p[0].y, p[1].x, p[1].y, colour);
        p++;
    } while (--count);
    return drawline(bm, chip, p->x, p->y, pts->x, pts->y, colour);
}

/* the steps along an edge (as polylib's): an x position is a fixed point
   number with the fraction in the upper word and twice the integer in the
   lower word; ADD.l adds a step, its carry adds 2 to the lower word */
static ULONG edge_step(ULONG pos, ULONG step)
{
    ULONG sum = pos + step;

    if (sum < pos)
        sum = (sum & 0xffff0000UL) | (UWORD)(sum + 2);
    return sum;
}

/* the start: x (sign extended) with the lower word 2 * x, the fraction
   word EORed with $8000 (one half; $7FFF for a negative x) */
static ULONG edge_start(WORD x)
{
    return (((ULONG)(LONG)x & 0xffff0000UL) | (UWORD)(x + x)) ^ 0x80000000UL;
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

/* lside/rside: the edges of one side from (x, y) over the points p, p +
   dir, ... down to the bottom; cut where the side leaves the clip width
   (the left side at x > width - 1, the right side at x < 0).  Returns the
   address of the end mark. */
static UWORD *side(UWORD *list, WORD x, WORD y, const struct point *p, int dir, int right,
                   WORD width, WORD height)
{
    struct edge *e = (struct edge *)list;
    WORD last = width - 1, tx = p->x, ty = p->y;

    while (ty > y) {                            /* SUB y,ty:BLE: done */
        WORD dy = ty - y;
        UWORD rest = dy, shift = 15;
        ULONG dx = (ULONG)(LONG)(WORD)(tx - x), step;

        e->lines = dy - 1;
        do {                                    /* dx * 4^k with dy < 4^k */
            shift -= 2;
            dx <<= 2;
            rest >>= 2;
        } while (rest);
        step = (ULONG)(LONG)(WORD)bl_divs_w(dx, dy);
        step = (shift & 63) >= 32 ? 0 : step << (shift & 63);     /* LSL.l */
        step = BL_SWAP(step);
        BL_SETW(step, step + step);
        e->step = step;
        e++;
        x = p->x;
        y = p->y;
        if (!BMI_W(y, height)) {               /* lbot/rbot */
            e[-1].lines -= (WORD)(y - height);
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

/* tops..rside (the algorithm of polylib): the outline of the polygon in
   polypts (n points, twice in a row) - the top line, clipped at the top
   edge and at the sides, and the edge lists of the left side (backwards
   through the points) and the right side.  NULL if nothing is visible,
   else the address of the right list's end mark (where the original
   leaves a3). */
static UWORD *outline(UWORD n, WORD width, WORD height)
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

    if (y < 0) {                                /* cliptotop */
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
    if (!BMI_W(lx, width)) {                   /* entering from the right */
        WORD last = width - 1;

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
    } else if (rx < 0) {                        /* entering from the left */
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
    if (!BMI_W(y, height))                     /* nonclip: inside? */
        return NULL;
    pv.top.left = lx;
    pv.top.y = y;
    pv.top.right = rx;
    side(pv.ledges, lx, y, lp, -1, 0, width, height);
    return side(pv.redges, rx, y, rp, 1, 1, width, height);
}

/* drawpoly: the polygon in polypts (plus the origin, added to both copies
   of the points) filled with the blitter, one blit per line and plane: A
   = $FFFF in the span's masks, D = C = the plane, set or cleared by the
   colour bit; a negative colour draws nothing.  a3 = what the copy loop
   left; returns the a3 the original leaves (or that one). */
static UBYTE *drawpoly(struct bb_bitmap *bm, UWORD n, UWORD colour, UBYTE *a3)
{
    UBYTE *chip = BL_CUSTOM;
    const struct edge *le = (const struct edge *)pv.ledges;
    const struct edge *re = (const struct edge *)pv.redges;
    ULONG left, right, lstep, rstep, rowoff;
    UWORD llines, rlines, rtclip, xs, xe, words, plane;
    UBYTE *end;

    if ((WORD)colour < 0)                       /* TST pcolor */
        return a3;
    if (bm->xorigin || bm->yorigin) {           /* porigin */
        struct point *p = polypts;
        UWORD k = n - 1;

        do {
            p[0].x += bm->xorigin;
            p[0].y += bm->yorigin;
            p[1].x += bm->xorigin;
            p[1].y += bm->yorigin;
            p += 2;
        } while (k-- != 0);
    }
    end = (UBYTE *)outline(n, bm->width, bm->height);
    if (!end)
        return a3;
    a3 = end;
    rowoff = bl_mulu_w(pv.top.y, bm->bpr);
    rtclip = (bm->width - 1) * 2;
    left = edge_start(pv.top.left);
    right = edge_start(pv.top.right);
    if (!next_edge(&le, &llines, &lstep) || !next_edge(&re, &rlines, &rstep))
        return a3;
    BLITWAIT(chip);                             /* bltsetup */
    CW(chip, BLTADAT) = 0xffff;
    CW(chip, BLTCON1) = 0;
    for (;;) {                                  /* ploop */
        left = edge_step(left, lstep);
        right = edge_step(right, rstep);
        if ((WORD)left > (WORD)rtclip)
            return a3;
        xs = (WORD)left < 0 ? 0 : (UWORD)left;
        if ((WORD)right < 0)
            return a3;
        xe = rtclip;
        if (!BMI_W(xe, right))
            xe = right;
        BLITWAIT(chip);
        CW(chip, BLTAFWM) = lup1[(xs & 30) >> 1];
        CW(chip, BLTALWM) = lup2[(xe & 30) >> 1];
        xs >>= 5;
        words = (xe >> 5) - xs;
        if ((WORD)words < 0)
            return a3;
        plane = 0;
        do {                                    /* nxpolyplane */
            BLITWAIT(chip);
            CW(chip, BLTCON0) = 0x3fa;
            if (!((UBYTE)colour & (1 << (plane & 7))))
                CW(chip, BLTCON0) = 0x30a;
            a3 = bm->planes[plane] + rowoff + (WORD)(xs + xs);
            CL(chip, BLTCPT) = (ULONG)a3;
            CL(chip, BLTDPT) = (ULONG)a3;
            CW(chip, BLTSIZE) = words + 65;
        } while (++plane != bm->flagsdepth);         /* depth 0: 65536 (#393) */
        rowoff += bm->bpr;
        left = edge_step(left, lstep);
        right = edge_step(right, rstep);
        if (llines-- == 0 && !next_edge(&le, &llines, &lstep))
            return a3;
        if (rlines-- == 0 && !next_edge(&re, &rlines, &rstep))
            return a3;
    }
}

/* Polyf: the points twice into polypts - backwards if they run anti-
   clockwise (by the first three points), then with colour2 (#395: more
   than 16 points overflow it) */
static UBYTE *polyf(struct bb_bitmap *bm, UWORD n, const struct point *pts, UWORD colour,
                    UWORD colour2)
{
    LONG a = bl_muls_w(pts[1].x - pts[0].x, pts[2].y - pts[1].y);
    LONG b = bl_muls_w(pts[2].x - pts[1].x, pts[1].y - pts[0].y);
    struct point *p, *q;
    UWORD k = n - 1;

    if (a > b) {                                /* clck */
        p = polypts;
        q = polypts + n;
        do {
            *p++ = *pts;
            *q++ = *pts++;
        } while (k-- != 0);
    } else {                                    /* anti */
        p = polypts + n;
        q = p + n;
        do {
            *--p = *pts;
            *--q = *pts++;
        } while (k-- != 0);
        colour = colour2;
    }
    return drawpoly(bm, n, colour, (UBYTE *)q);
}

/* ------------------------------------------------------------- ClipBlit */

static void BLITZ_ROUTINE _doclipblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(_doclipblitmode, _doclipblitmode_c, "d0-d1/a0-a1");

/* what b_docblit leaves for the excess planes (b_docblitx) */
struct cblit {
    ULONG mask;                 /* A: the cookie */
    ULONG dest;                 /* the offset in the planes */
    UBYTE **plane;              /* the next plane */
    UWORD shift;                /* BLTCON1 (and the shift of BLTCON0) */
    UWORD size;
};

/* b_docblit: the shape at x, y (minus its handle) through its cookie
   onto the planes of bm (cookie cut, BLTCON0 = shift | ClipBlitMode),
   clipped at all edges.  r6/r7: the caller's d6/d7 (MOVEM.l d6-d7,$60
   (a1), overwritten before the blit).  Returns 0 if nothing was blitted. */
static int docblit(UBYTE *chip, struct bb_shape *sh, struct bb_bitmap *bm, WORD x, WORD y,
                   ULONG r6, ULONG r7, struct cblit *cb)
{
    ULONG src = 0, planesize, p;
    WORD bottom, amod, dmod, over, rowbytes;
    UWORD size, k;

    x -= sh->handlex;
    y -= sh->handley;
    if (!BMI_W(x, bm->width) || !BMI_W(y, bm->height))
        return 0;
    BLITWAIT(chip);
    CL(chip, BLTCMOD) = r6;
    CL(chip, BLTAMOD) = r7;
    size = sh->bltsize;
    bottom = y;
    if (y < 0) {                                /* clip top */
        y = -y;
        if (!BMI_W(y, sh->height))
            return 0;
        src = bl_mulu_w(y, sh->ebwidth);
        y <<= 6;
        if (size < (UWORD)y)                    /* SUB.w d2,d6:BCS */
            return 0;
        size -= y;
        y = 0;
    }
    bottom += sh->height;                       /* bolo */
    if (bottom < 0)
        return 0;
    bottom -= bm->height;
    if (bottom >= 0) {                          /* clip bottom */
        bottom <<= 6;
        if (size < (UWORD)bottom)
            return 0;
        size -= bottom;
    }
    CL(chip, BLTAFWM) = 0xffff0000;             /* abuv */
    amod = -2;
    dmod = bm->bpr - sh->ebwidth - 2;
    if (x < 0) {                                /* clip left: whole words */
        if (!BMI_W(-x, sh->width))
            return 0;
        for (;;) {                              /* g1 */
            x += 16;
            if (x >= 0)
                break;
            amod += 2;
            dmod += 2;
            size -= 1;
            BL_SETW(src, src + 2);
        }
        CW(chip, BLTAFWM) = ~(UWORD)((x & 63) >= 16 ? 0 : 0xffff << (x & 63));      /* g2 */
        x -= 16;
    }
    over = (WORD)(((WORD)x >> 4) * 2 + sh->ebwidth + 2);    /* nolo */
    rowbytes = bm->width >> 3;
    if (!(over <= rowbytes)) {                  /* clip right */
        over -= rowbytes;
        amod += over;
        dmod += over;
        size -= (UWORD)over >> 1;
        CW(chip, BLTALWM) = (UWORD)(0xffff << (x & 15));
    }
    CW(chip, BLTBMOD) = amod;                   /* noro */
    CW(chip, BLTAMOD) = amod;
    CW(chip, BLTCMOD) = dmod;
    CW(chip, BLTDMOD) = dmod;
    cb->shift = (UWORD)((x & 15) << 12);        /* ROR #4 */
    CW(chip, BLTCON1) = cb->shift;
    CW(chip, BLTCON0) = cb->shift | bcon0;
    cb->mask = src + (ULONG)sh->cookie;
    src += (ULONG)sh->data;
    cb->dest = (ULONG)bl_muls_w(y, bm->bpr) + 2 * (LONG)((WORD)x >> 4);
    cb->plane = bm->planes;
    cb->size = size;
    planesize = bl_mulu_w(sh->height, sh->ebwidth);
    k = sh->depth;
    if (!k)
        return 0;
    k--;
    for (;;) {                                  /* doblit */
        CL(chip, BLTAPT) = cb->mask;
        CL(chip, BLTBPT) = src;
        src += planesize;
        p = (ULONG)*cb->plane++ + cb->dest;
        CL(chip, BLTCPT) = p;
        CL(chip, BLTDPT) = p;
        CW(chip, BLTSIZE) = size;
        if (k-- == 0)
            return 1;
        BLITWAIT(chip);
    }
}

/* b_docblitx: and the planes of bm beyond the shape's depth through the
   cookie: bit set in excess = set, else cleared */
static void docblitx(UBYTE *chip, struct bb_shape *sh, struct bb_bitmap *bm, WORD x, WORD y,
                     UWORD excess, ULONG r6, ULONG r7)
{
    struct cblit cb;
    UWORD i;

    if (!docblit(chip, sh, bm, x, y, r6, r7, &cb))
        return;
    for (i = sh->depth; i < bm->flagsdepth; i++) {
        UWORD con = (excess & 1) ? 0xbfa : 0xb0a;
        ULONG p = (ULONG)*cb.plane++ + cb.dest;

        excess >>= 1;
        BLITWAIT(chip);
        CW(chip, BLTCON0) = con | cb.shift;
        CL(chip, BLTAPT) = cb.mask;
        CL(chip, BLTCPT) = p;
        CL(chip, BLTDPT) = p;
        CW(chip, BLTSIZE) = cb.size;
    }
}

/* -------------------------------------------- entries (Blitz / Amiga) */

/* Blitz version (b_) and Amiga version (a_, owns the blitter, graphics
   base in a6) of a blitter routine: a3 = the bitmap, a2 = the chip base;
   `call` gives the a3 the routine leaves */
#define BLITPAIR(name, call, ...)                                           \
    static void BLITZ_ROUTINE b_##name##_c(__VA_ARGS__, UBYTE *chip BL_REG(a2)) \
    {                                                                       \
        struct bb_bitmap *bm = (struct bb_bitmap *)blitz_string;                  \
                                                                            \
        blitz_string = call;                                                \
    }                                                                       \
    static void BLITZ_ROUTINE a_##name##_c(__VA_ARGS__, UBYTE *chip BL_REG(a2), \
                                           struct GfxBase *gfx BL_REG(a6))  \
    {                                                                       \
        struct bb_bitmap *bm = (struct bb_bitmap *)blitz_string;                  \
        UBYTE *a3;                                                          \
                                                                            \
        OwnBlitter();                                                       \
        a3 = call;                                                          \
        BLITWAIT(chip);                                                     \
        DisownBlitter();                                                    \
        blitz_string = a3;                                                  \
    }

#define XY1 WORD x1 BL_REG(d0), WORD y1 BL_REG(d1)
#define XY2 WORD x2 BL_REG(d2), WORD y2 BL_REG(d3)

BLITPAIR(line, line(bm, chip, x1, y1, x2, y2, c), XY1, XY2, UWORD c BL_REG(d4))
BLITPAIR(lineto, lineto(bm, chip, x, y, c),
         WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(linew, linew(bm, chip, x1, y1, x2, y2, c, w), XY1, XY2, UWORD c BL_REG(d4),
         UWORD w BL_REG(d5))
BLITPAIR(box, box(bm, chip, x1, y1, x2, y2, c), XY1, XY2, UWORD c BL_REG(d4))
BLITPAIR(boxf, boxf(bm, chip, x1, y1, x2, y2, c), XY1, XY2, ULONG c BL_REG(d4))
BLITPAIR(circlef, ellipsef(bm, chip, x, y, r, r, c),
         WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD r BL_REG(d2), UWORD c BL_REG(d3))
BLITPAIR(ellipsef, ellipsef(bm, chip, x, y, rx, ry, c),
         WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD rx BL_REG(d2), UWORD ry BL_REG(d3),
         UWORD c BL_REG(d4))
BLITPAIR(floodfill, floodfill(bm, chip, x, y, c),
         WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(floodfill2, floodfill2(bm, chip, x, y, c, border),
         ULONG x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2), UWORD border BL_REG(d3))
BLITPAIR(remap, remap(bm, chip, from, to, bm), ULONG from BL_REG(d0), ULONG to BL_REG(d1))
BLITPAIR(remap2, remap(bm, chip, from, to, dst), ULONG from BL_REG(d0), ULONG to BL_REG(d1),
         struct bb_bitmap *dst BL_REG(a0))
BLITPAIR(poly, poly(bm, chip, n, (const struct point *)pts, c),
         UWORD n BL_REG(d0), ULONG pts BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(polyf, ((void)chip, polyf(bm, n, (const struct point *)pts, c, c)),
         UWORD n BL_REG(d0), ULONG pts BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(polyf2, ((void)chip, polyf(bm, n, (const struct point *)pts, c, c2)),
         UWORD n BL_REG(d0), ULONG pts BL_REG(d1), UWORD c BL_REG(d2), UWORD c2 BL_REG(d3))

#define ABENTRY(e, n) \
    void b_##n(void); \
    BLITZ_THUNK(b_##n, b_##n##_c, "d1/a0-a1"); \
    BLITZ_AB_ROUTINE(e, b_##n, "   jmp     _a_" #n "_c")

ABENTRY(_line, line);
ABENTRY(_lineto, lineto);
ABENTRY(_linew, linew);
ABENTRY(_box, box);
ABENTRY(_boxf, boxf);
ABENTRY(_circlef, circlef);
ABENTRY(_ellipsef, ellipsef);
ABENTRY(_floodfill, floodfill);
ABENTRY(_floodfill2, floodfill2);
ABENTRY(_remap, remap);
ABENTRY(_remap2, remap2);
ABENTRY(_poly, poly);
ABENTRY(_polyf, polyf);
ABENTRY(_polyf2, polyf2);

/* ClipBlit: a3 = shape, a2 = bitmap, a1 = chip, d1/d2 = x/y, d3 = planes */
#define CBREGS WORD x BL_REG(d1), WORD y BL_REG(d2), UWORD excess BL_REG(d3), \
               ULONG r6 BL_REG(d6), ULONG r7 BL_REG(d7), UBYTE *chip BL_REG(a1), \
               struct bb_bitmap *bm BL_REG(a2)
#define SHAPE ((struct bb_shape *)blitz_string)

static void BLITZ_ROUTINE b_docblit_c(CBREGS)
{
    struct cblit cb;

    (void)excess;
    docblit(chip, SHAPE, bm, x, y, r6, r7, &cb);
}

static void BLITZ_ROUTINE a_docblit_c(CBREGS, struct GfxBase *gfx BL_REG(a6))
{
    struct cblit cb;

    (void)excess;
    OwnBlitter();
    docblit(chip, SHAPE, bm, x, y, r6, r7, &cb);
    BLITWAIT(chip);
    DisownBlitter();
}

static void BLITZ_ROUTINE b_docblitx_c(CBREGS)
{
    docblitx(chip, SHAPE, bm, x, y, excess, r6, r7);
}

static void BLITZ_ROUTINE a_docblitx_c(CBREGS, struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    docblitx(chip, SHAPE, bm, x, y, excess, r6, r7);
    BLITWAIT(chip);
    DisownBlitter();
}

void b_docblit(void), b_docblitx(void);
BLITZ_THUNK(b_docblit, b_docblit_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(_doclipblit, b_docblit, "   jmp     _a_docblit_c");
BLITZ_THUNK(b_docblitx, b_docblitx_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(_doclipblitx, b_docblitx, "   jmp     _a_docblitx_c");

/* _cbchk - TST.w (a3):BEQ err2 (shape not initialised); in the normal
   code (the library has no error area) */
static void BLITZ_ROUTINE _cbchk_c(void)
{
    if (!SHAPE->width)
        BLITZ_RUNERR(mess1);
}
BLITZ_THUNK(_cbchk, _cbchk_c, "d0-d1/a0-a1");
