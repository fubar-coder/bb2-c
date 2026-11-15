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
 * Line, Box, Boxf, Poly, Circle, Ellipse, Circlef, Ellipsef and the line
 * loop of Polyf are the original's code in assembler (OpenSpec change
 * libs-performance, task 10.2: the C stayed 1.2 to 2.1 times slower),
 * with the fixes of the C (#391, #392); the rest is C.
 *
 * Blitz bitmap object: 0 bytes per row, 2 rows, 4 depth, 8 planes[8],
 * 48/50 origin x/y, 52 flags, 54 bytes per row of the window, 56/58
 * clip width/height, 60 colours, 62 "is real".  Shape object (ClipBlit):
 * 0 width, 2 height, 4 depth, 6 bytes per row, 8 bltsize, 10/12 handle,
 * 14 data, 18 cookie (mask).
 *
 * BUG(orig), kept (selection):
 *   - Fixed (#391): Box added the origin twice (once itself, once in the
 *     line clipper); only the line clipper adds it now.
 *   - Fixed (#392): Circle/Circlef with points above the top edge: the
 *     MULU of the negative y gave a large offset that was not rejected -
 *     the CPU plot wrote far outside the bitmap / fill buffer.  The
 *     height compare is unsigned now (the same instructions).
 *   - Fixed (#396): Poly with 1 point loops 65535 times (0 points: 65536);
 *     such a count is an invalid argument: with runtime error checking
 *     _polychk (error check area only, the original had none and no error
 *     check area) raises "Poly needs at least 2 Points"; the loop stays.
 *   - Fixed (#395): more than 10 edges on one side overflowed the edge
 *     lists l0/l1 (here ledges/redges, Dcb.w 32 in the original); they
 *     have room for the 15 edges of a polygon of 16 points now.  More
 *     than 16 points overflow the 32-long point buffer: an invalid
 *     argument; with runtime error checking _polyfchk (error check area
 *     only) raises "Too Many Points", the copy stays.
 *   - Fixed (#390): AllocFill (statement) took the bitmap from a3 = the
 *     string pointer: it was declared without a used bitmap.  It is
 *     declared with the used bitmap now (!libs {#bitmaplib,#ua3}).
 *   - Fixed (#393): a depth of 0 makes the plane loops run 65536 (CMP.w)
 *     / 256 (CMP.b) times; such a bitmap is an invalid argument,
 *     refused in the error check area by BitMap and CludgeBitMap (the
 *     loops stay).
 *   - FreeFill does not clear the buffer pointer. (#394)
 * Fixed (intended deviation, docs/libs/deviations.md):
 *   - Polyf clips and steps the edges with word arithmetic (the slope is
 *     one DIVS, an edge of 16384 lines gets the slope 0, the clipping
 *     DIVS overflow, the orientation test MULS of wrapped differences):
 *     a polygon with vertices far outside the bitmap is drawn wrongly or
 *     not at all.  Here a polygon of 3 to 16 points that leaves the
 *     range in which the word arithmetic is exact (polyinrange) is first
 *     clipped to the bitmap plus one pixel with long arithmetic
 *     (clippoly), and the orientation test compares exact products; the
 *     polygons in that range are drawn as before. (#1000)
 *   - Polyf divides by zero when a side starts beyond the clip edge it is
 *     cut at and its first edge is vertical (the cut at the side divides
 *     by the edge's dx); here such an edge is not cut.  The clipping of
 *     #1000 can make such an edge (#1034). (#1033)
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

/* entry points (C routines / aliases / thunks / prefixes, defined below) */
void _ellipse(void), _circle(void), _ellipsef(void), _circlef(void);
void _box(void), _boxf(void), _line(void), _lineto(void), _linew(void);
void _freefill(void), _allocfill(void), _remap(void), _remap2(void);
void _floodfill(void), _floodfill2(void), _poly(void), _polyf(void), _polyf2(void);
void _bitplanesbitmap(void), _doclipblit(void), _doclipblitx(void), _cbchk(void);
void _doclipblitmode(void), _bitmapwindow(void), _bitmaporigin(void), _init(void);
void _polychk(void), _polyfchk(void);


/* !libheader {#sis2dlib,0,0,0,errors}: the original has no error check
   area (0); _polychk (#396) is its first check */
BLITZ_LIBHEADER(LIB_sis2dlib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3);
BLITZ_SUBS(_ellipse, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3);
BLITZ_SUBS(_circle, 0, 0);
BLITZ_NAME("Circle", "x,y,rx[,ry],color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_graphicslib, BL_IFAMIGA|BL_LA6, LIB_chipbaselib, BL_LA2);
BLITZ_SUBS(_ellipsef, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_graphicslib, BL_IFAMIGA|BL_LA6, LIB_chipbaselib, BL_LA2);
BLITZ_SUBS(_circlef, 0, 0);
BLITZ_NAME("Circlef", "x,y,rx[,ry],color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_box, 0, 0);
BLITZ_NAME("Box", "x1,y1,x2,y2,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_boxf, 0, 0);
BLITZ_NAME("Boxf", "x1,y1,x2,y2,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_line, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_lineto, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_linew, 0, 0);
BLITZ_NAME("Line", "[x1,y1,]x2,y2,color[,width]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_freefill, 0, 0);
BLITZ_NAME("FreeFill", "frees memory allocated for 2d fill routines");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_bitmaplib, BL_UA3);         /* fixed (#390): the used bitmap in a3 */
BLITZ_SUBS(_allocfill, 0, 0);
BLITZ_NAME("AllocFill", "for internal use only (polylib.obj)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_remap, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_bitmaplib, BL_IA0|BL_PD2, LIB_chipbaselib, BL_LA2,
           LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_remap2, 0, 0);
BLITZ_NAME("ReMap", "colour# to replace,destination colour# [,BitMap destintion");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_floodfill, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_floodfill2, 0, 0);
BLITZ_NAME("FloodFill", "x,y,colour [,border colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_poly, _polychk, 0);
BLITZ_NAME("Poly", "numpts,*coords.w,color");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_polyf, _polyfchk, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA3, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_polyf2, _polyfchk, 0);
BLITZ_NAME("Polyf", "numpts,*coords.w,color[,color2]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0, LIB_bitmaplib, BL_IA1|BL_PD1);
BLITZ_SUBS(_bitplanesbitmap, 0, 0);
BLITZ_NAME("BitPlanesBitMap", "srcbitmap,destbitmap,planepick");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA1, LIB_bitmaplib, BL_UA2,
           LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_doclipblit, _cbchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA1, LIB_bitmaplib, BL_UA2,
           LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_doclipblitx, _cbchk, 0);
BLITZ_NAME("ClipBlit", "Shape#,X,Y,[Excess]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_doclipblitmode, 0, 0);
BLITZ_NAME("ClipBlitMode", "BPLCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0, LIB_bitmaplib, BL_IA1|BL_PD1);
BLITZ_SUBS(_bitmapwindow, 0, 0);
BLITZ_NAME("BitMapWindow", "srcbitmap,destbitmap,x,y,w,h");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0);
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
static_assert(sizeof(struct point) == 4);     /* Polyf copies it as a long */

/* an edge of one side of a filled polygon: the lines it covers - 1 and
   the x step per half line (edge_step); the long -1 ends a list */
struct edge {
    WORD lines;
    ULONG step;
} BL_PACKED;

/* (WORD)(a - b) < 0: the original's CMP/SUB with BMI - the sign of the 16
   bit difference, which wraps on overflow (unlike a signed compare) */
#define BMI_W(a, b) ((WORD)((a) - (b)) < 0)

/* the most points of a polygon (the n points twice fill polypts) and the
   most edges of a side between the top and the bottom point */
#define POINTMAX 16
#define EDGEMAX (POINTMAX - 1)

/* ------------------------------------------------------------ variables */

static struct point polypts[2 * POINTMAX];  /* Polyf: the n points twice */
/* in the order of the original, so that an overflow of ledges runs into
   redges */
static struct {
    struct {
        WORD left, y, right;            /* the first line of the outline */
    } top;
    /* struct edge lists (l0, l1); fixed (#395): room for EDGEMAX edges, as
       many as a side of 16 points has (the original: Dcb.w 32, 10 edges) */
    UWORD ledges[EDGEMAX * 3 + 2], redges[EDGEMAX * 3 + 2];
} pv;
static ULONG fbsize;
/* used by the assembler part (yofill): not static */
ULONG fbloc;                            /* the fill buffer */
UWORD coords[2];                        /* FloodFill: start point */
UWORD fcol;                             /* FloodFill: colour */
UWORD widbytes;
UBYTE flags[2];
static UWORD bigsize[2] __attribute__((used)); /* Circlef: BLTSIZV, BLTSIZH */
static UWORD bcon0 = 0xfca;             /* ClipBlitMode */

static const UWORD lup1[16] __attribute__((used)) = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};
static const UWORD lup2[16] __attribute__((used)) = {
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff,
};

static const char mess1[] = "SHAPE UNKNOWNST TO ME";

/* ---------------------------------------------------------- arithmetic */

/* MULS a,b : DIVS c - the quotient word (on overflow DIVS leaves the
   product, then its low word) */
BL_ALWAYS_INLINE WORD muldiv(WORD a, WORD b, WORD c)
{
    return (WORD)bl_divs_w((ULONG)bl_muls_w(a, b), c);
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
BLITZ_ALIAS(_bitmaporigin, _bitmaporigin_c);

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
BLITZ_ALIAS(_bitplanesbitmap, _bitplanesbitmap_c);

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
BLITZ_ALIAS(_bitmapwindow, _bitmapwindow_c);

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
    allocfill((struct bb_bitmap *)blitz_string);   /* the used bitmap (fixed, #390) */
}
BLITZ_ALIAS(_allocfill, _allocfill_c);

static ULONG BLITZ_ROUTINE _freefill_c(void)
{
    if (fbsize) {
        memfree(fbloc, fbsize);
        fbsize = 0;
    }
    return 0;
}
BLITZ_ALIAS(_freefill, _freefill_c);

/* -------------------------------------------------------------- lines */

/* The line drawing (Line, Box, Poly), the filled box (Boxf) and the
   outline of an ellipse (Circle, Ellipse) are the original's code in
   assembler (OpenSpec change libs-performance, task 10.2; decision 15:
   the C stayed 1.2 to 1.6 times slower than the original - its line
   clipper took its state from the stack in the plane loop, the plot of
   the ellipse built every bit with shifts where the original has BSET,
   BCLR and BCHG).  The fixes of the C are in it: Box does not add the
   origin itself (#391), the plot of the ellipse compares y with the
   clip height unsigned (#392); #393 (depth 0) and #396 (Poly with fewer
   than 2 points) stay.

   lv: the original's center (4 words), color, cpos (LineTo), dpth and
   pfc, in this order (MOVEM.w d0-d4,center sets the colour too); ev: dx,
   dy, asq, bsq, asq2, bsq2 (MOVEM.l dx,d0-d3), flag. */
static struct {
    WORD center[4];             /* +0 x, y, x2/rx, y2/ry */
    UWORD color;                /* +8 */
    WORD cpos[2];               /* +10 the end of the last Line (LineTo) */
    UWORD dpth;                 /* +14 the depth of the bitmap */
    UWORD pfc;                  /* +16 Poly: the lines left */
} lv __attribute__((used));
static struct {
    ULONG dx, dy, asq, bsq, asq2, bsq2;
    UWORD flag;
} ev __attribute__((used));
static_assert(offsetof(__typeof__(lv), color) == 8 && offsetof(__typeof__(lv), cpos) == 10
              && offsetof(__typeof__(lv), dpth) == 14 && offsetof(__typeof__(lv), pfc) == 16);
static_assert(offsetof(__typeof__(ev), asq) == 8 && offsetof(__typeof__(ev), bsq) == 12
              && offsetof(__typeof__(ev), asq2) == 16 && offsetof(__typeof__(ev), bsq2) == 20
              && offsetof(__typeof__(ev), flag) == 24);

/* clipme: the line d0,d1 - d2,d3 (plus the origin) in lv.color, clipped
   to the bitmap a3, drawn with the blitter (a2 = the chip base) in line
   mode plane by plane: colour bit 1 sets, 0 clears, a negative colour
   complements; leaves a3 behind the plane pointers it used (the bitmap
   if nothing is drawn).  Changes d0-d7, a0, a1, a3.
   l2_bline (Line x1,y1,x2,y2,c: d4 = colour, the end into cpos),
   l2_blineto (Line x,y,c: from the end of the last line), l2_blinew
   (Line with width d5: for w = width .. 0 four lines with one of the
   coordinates + w, a3 kept), l2_boxo (Box: four lines, a4/a5 saved),
   l2_poly (Poly: n = d0, the points at d1, colour d2; a4-a6 saved) */
BLITZ_ASM_BLOCK(lines,
"   .globl  _l2_bline, _l2_blineto, _l2_blinew, _l2_boxo, _l2_poly\n"
"_l2_blinew:\n"
"   movem.w d0-d5,-(sp)\n"
"   add.w   d5,d0\n"
"   move.l  a3,-(sp)\n"
"   bsr.s   _l2_bline\n"
"   move.l  (sp)+,a3\n"
"   movem.w (sp),d0-d5\n"
"   add.w   d5,d1\n"
"   move.l  a3,-(sp)\n"
"   bsr.s   _l2_bline\n"
"   move.l  (sp)+,a3\n"
"   movem.w (sp),d0-d5\n"
"   add.w   d5,d2\n"
"   move.l  a3,-(sp)\n"
"   bsr.s   _l2_bline\n"
"   move.l  (sp)+,a3\n"
"   movem.w (sp),d0-d5\n"
"   add.w   d5,d3\n"
"   move.l  a3,-(sp)\n"
"   bsr.s   _l2_bline\n"
"   move.l  (sp)+,a3\n"
"   movem.w (sp)+,d0-d5\n"
"   dbf     d5,_l2_blinew\n"
"   rts\n"
"_l2_blineto:\n"
"   move.w  d2,_lv+8\n"
"   movem.w _lv+10,d2-d3\n"
"   movem.w d0-d1,_lv+10\n"
"   bra.s   l2_clipme\n"
"_l2_bline:\n"
"   move.w  d4,_lv+8\n"
"   movem.w d2-d3,_lv+10\n"
"l2_clipme:\n"
"   movem.w 48(a3),d4-d5\n"             /* the origin */
"   add.w   d4,d0\n"
"   add.w   d4,d2\n"
"   add.w   d5,d1\n"
"   add.w   d5,d3\n"
"   movem.w 56(a3),d4-d5\n"             /* the clip width and height */
"   cmp.w   d0,d2\n"
"   bpl.s   1f\n"
"   exg     d0,d2\n"
"   exg     d1,d3\n"
"1: tst.w   d2\n"
"   bmi     9f\n"                       /* noline */
"   cmp.w   d4,d0\n"
"   bpl     9f\n"
"   cmp.w   d4,d2\n"
"   bmi.s   2f\n"
"   move.w  d2,d7\n"                    /* clip right */
"   sub.w   d4,d7\n"
"   addq.w  #1,d7\n"
"   move.w  d3,d6\n"
"   sub.w   d1,d6\n"
"   muls.w  d6,d7\n"
"   move.w  d2,d6\n"
"   sub.w   d0,d6\n"
"   divs.w  d6,d7\n"
"   sub.w   d7,d3\n"
"   move.w  d4,d2\n"
"   subq.w  #1,d2\n"
"2: tst.w   d0\n"
"   bpl.s   3f\n"
"   move.w  d0,d7\n"                    /* clip left */
"   move.w  d3,d6\n"
"   sub.w   d1,d6\n"
"   muls.w  d6,d7\n"
"   move.w  d2,d6\n"
"   sub.w   d0,d6\n"
"   divs.w  d6,d7\n"
"   sub.w   d7,d1\n"
"   clr.w   d0\n"
"3: cmp.w   d1,d3\n"
"   bpl.s   4f\n"
"   exg     d0,d2\n"
"   exg     d1,d3\n"
"4: tst.w   d3\n"
"   bmi     9f\n"
"   cmp.w   d5,d1\n"
"   bpl     9f\n"
"   cmp.w   d5,d3\n"
"   bmi.s   5f\n"
"   move.w  d3,d7\n"                    /* clip bottom */
"   sub.w   d5,d7\n"
"   addq.w  #1,d7\n"
"   move.w  d2,d6\n"
"   sub.w   d0,d6\n"
"   muls.w  d6,d7\n"
"   move.w  d3,d6\n"
"   sub.w   d1,d6\n"
"   divs.w  d6,d7\n"
"   sub.w   d7,d2\n"
"   move.w  d5,d3\n"
"   subq.w  #1,d3\n"
"5: tst.w   d1\n"
"   bpl.s   6f\n"
"   move.w  d1,d7\n"                    /* clip top */
"   move.w  d2,d6\n"
"   sub.w   d0,d6\n"
"   muls.w  d6,d7\n"
"   move.w  d3,d6\n"
"   sub.w   d1,d6\n"
"   divs.w  d6,d7\n"
"   sub.w   d7,d0\n"
"   clr.w   d1\n"
/* CalcOctant: d2 = the major, d3 = the minor distance, d5 = the octant
   (+1: LINE) */
"6: sub.w   d0,d2\n"
"   bmi.s   20f\n"
"   sub.w   d1,d3\n"
"   move.w  d3,d7\n"
"   bmi.s   22f\n"
"   cmp.w   d3,d2\n"
"   bmi.s   21f\n"
"   moveq   #16+1,d5\n"
"   bra.s   29f\n"
"21:exg     d2,d3\n"
"   moveq   #0+1,d5\n"
"   bra.s   29f\n"
"22:neg.w   d3\n"
"   cmp.w   d3,d2\n"
"   bmi.s   23f\n"
"   moveq   #24+1,d5\n"
"   bra.s   29f\n"
"23:exg     d2,d3\n"
"   moveq   #4+1,d5\n"
"   bra.s   29f\n"
"20:neg.w   d2\n"
"   sub.w   d1,d3\n"
"   move.w  d3,d7\n"
"   bmi.s   25f\n"
"   cmp.w   d3,d2\n"
"   bmi.s   24f\n"
"   moveq   #20+1,d5\n"
"   bra.s   29f\n"
"24:exg     d2,d3\n"
"   moveq   #8+1,d5\n"
"   bra.s   29f\n"
"25:neg.w   d3\n"
"   cmp.w   d3,d2\n"
"   bmi.s   26f\n"
"   moveq   #28+1,d5\n"
"   bra.s   29f\n"
"26:exg     d2,d3\n"
"   moveq   #12+1,d5\n"
"29:mulu.w  (a3),d1\n"                  /* the word of the start point */
"   move.l  d1,a1\n"
"   ext.l   d0\n"
"   ror.l   #4,d0\n"
"   add.w   d0,a1\n"
"   add.w   d0,a1\n"
"   swap    d0\n"                       /* the shift in bits 12-15 */
"   lsl.w   #2,d3\n"
"   add.w   d2,d2\n"
"   move.w  d2,d4\n"
"   ext.l   d4\n"
"   lsl.l   #5,d4\n"
"   move.l  d4,d1\n"
"   swap    d1\n"                       /* blits of 1024 pixels first */
"   add.w   #0x42,d4\n"                 /* the size */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.w  d3,0x62(a2)\n"              /* BLTBMOD */
"   sub.w   d2,d3\n"
"   ext.l   d3\n"
"   move.l  d3,d7\n"                    /* BLTAPT */
"   bpl.s   30f\n"
"   or.w    #64,d5\n"                   /* SIGN */
"30:sub.w   d2,d3\n"
"   move.w  d3,0x64(a2)\n"              /* BLTAMOD */
"   move.w  (a3),0x60(a2)\n"
"   move.w  (a3),0x66(a2)\n"
"   move.l  #-1,0x44(a2)\n"
"   move.w  4(a3),_lv+14\n"             /* dpth */
"   lea     8(a3),a3\n"
"   moveq   #0,d6\n"
"   tst.w   _lv+8\n"
"   bmi.s   40f\n"
"31:move.l  (a3)+,a0\n"                 /* nxplane: set or clear */
"   add.l   a1,a0\n"
"   or.w    #0xbfa,d0\n"
"   btst    d6,_lv+9\n"
"   bne.s   32f\n"
"   eor.b   #0xf0,d0\n"
"32:\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.w  #0x8000,0x74(a2)\n"
"   move.l  d7,0x50(a2)\n"
"   move.l  a0,0x48(a2)\n"
"   move.l  a0,0x54(a2)\n"
"   move.w  d0,0x40(a2)\n"
"   move.w  d5,0x42(a2)\n"
"   tst.w   d1\n"
"   bne.s   34f\n"
"33:move.w  d4,0x58(a2)\n"
"   addq.b  #1,d6\n"
"   cmp.b   _lv+15,d6\n"                /* CMP.b: depth 0 gives 256 (#393) */
"   bne.s   31b\n"
"9: rts\n"
"34:move.w  d1,-(sp)\n"                 /* doextend1 */
"   subq.w  #1,d1\n"
"35:move.w  #2,0x58(a2)\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   dbf     d1,35b\n"
"   move.w  (sp)+,d1\n"
"   bra.s   33b\n"
"40:move.l  (a3)+,a0\n"                 /* nxplanus: complement */
"   add.l   a1,a0\n"
"   or.w    #0xb5a,d0\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.w  #0x8000,0x74(a2)\n"
"   move.l  d7,0x50(a2)\n"
"   move.l  a0,0x48(a2)\n"
"   move.l  a0,0x54(a2)\n"
"   move.w  d0,0x40(a2)\n"
"   move.w  d5,0x42(a2)\n"
"   tst.w   d1\n"
"   bne.s   42f\n"
"41:move.w  d4,0x58(a2)\n"
"   addq.b  #1,d6\n"
"   cmp.b   _lv+15,d6\n"
"   bne.s   40b\n"
"   rts\n"
"42:move.w  d1,-(sp)\n"                 /* doextend2 */
"   subq.w  #1,d1\n"
"43:move.w  #2,0x58(a2)\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   dbf     d1,43b\n"
"   move.w  (sp)+,d1\n"
"   bra.s   41b\n"
/* Box: fixed (#391), the line clipper adds the origin; the original
   added it here too, so the box was displaced by twice the origin */
"_l2_boxo:\n"
"   movem.l a4-a5,-(sp)\n"
"   cmp.w   d0,d2\n"
"   bpl.s   1f\n"
"   exg     d0,d2\n"
"1: cmp.w   d1,d3\n"
"   bpl.s   2f\n"
"   exg     d1,d3\n"
"2: movem.w d0-d4,_lv\n"                /* center and color */
"   move.l  a3,a4\n"
"   move.w  d0,d2\n"
"   subq.w  #1,d3\n"
"   bsr     l2_clipme\n"
"   movem.w _lv,d0-d3\n"
"   move.w  d2,d0\n"
"   addq.w  #1,d1\n"
"   move.l  a4,a3\n"
"   bsr     l2_clipme\n"
"   movem.w _lv,d0-d3\n"
"   move.w  d1,d3\n"
"   addq.w  #1,d0\n"
"   move.l  a4,a3\n"
"   bsr     l2_clipme\n"
"   movem.w _lv,d0-d3\n"
"   move.w  d3,d1\n"
"   subq.w  #1,d2\n"
"   move.l  a4,a3\n"
"   bsr     l2_clipme\n"
"   movem.l (sp)+,a4-a5\n"
"   rts\n"
/* Poly: n - 1 lines from point to point (#396: n = 1 gives 65535), then
   the last point to the first */
"_l2_poly:\n"
"   movem.l a4-a6,-(sp)\n"
"   move.l  a3,a4\n"
"   move.l  d1,a5\n"
"   move.l  d1,a6\n"
"   subq.w  #1,d0\n"
"   move.w  d0,_lv+16\n"
"   move.w  d2,_lv+8\n"
"1: movem.w (a5),d0-d3\n"
"   addq.l  #4,a5\n"
"   move.l  a4,a3\n"
"   bsr     l2_clipme\n"
"   subq.w  #1,_lv+16\n"
"   bne.s   1b\n"
"   movem.w (a5),d0-d1\n"
"   movem.w (a6),d2-d3\n"
"   move.l  a4,a3\n"
"   bsr     l2_clipme\n"
"   movem.l (sp)+,a4-a6\n"
"   rts");

/* the Amiga mode code (the original's BlitLabel): the Blitz mode code
   between graphics OwnBlitter and DisownBlitter, the blitter finished;
   d0/d1 kept around OwnBlitter */
#define AMIGALINE(code)                                                     \
    "   movem.l d0-d1,-(sp)\n"                                              \
    "   jsr     -456(a6)\n"                 /* OwnBlitter */                \
    "   movem.l (sp)+,d0-d1\n"                                              \
    "   bsr     _" #code "\n"                                               \
    "   btst    #6,2(a2)\n"                                                 \
    "0: btst    #6,2(a2)\n"                                                 \
    "   bne.s   0b\n"                                                       \
    "   jmp     -462(a6)"                   /* DisownBlitter */
BLITZ_AB_ROUTINE(_line, l2_bline, AMIGALINE(l2_bline));
BLITZ_AB_ROUTINE(_lineto, l2_blineto, AMIGALINE(l2_blineto));
BLITZ_AB_ROUTINE(_linew, l2_blinew, AMIGALINE(l2_blinew));
BLITZ_AB_ROUTINE(_box, l2_boxo, AMIGALINE(l2_boxo));
BLITZ_AB_ROUTINE(_poly, l2_poly, AMIGALINE(l2_poly));

/* ----------------------------------------------------------- Boxf */

/* Boxf: the rectangle (plus the origin) clipped (a box starting in the
   last column or line is not drawn), in parts of 990 pixels; dobof: one
   blit per plane (more than 1023 lines: parts of 1024 first), colour bit
   1 sets, 0 clears, a negative colour (its low word) only complements
   the planes of its 1 bits.  Leaves a3 behind the plane pointers (the
   bitmap if nothing is drawn). */
BLITZ_ASM_BLOCK(boxf,
"   .globl  _l2_boxfo\n"
"_l2_boxfo:\n"
"   movem.w 48(a3),d5-d6\n"
"   add.w   d5,d0\n"
"   add.w   d5,d2\n"
"   add.w   d6,d1\n"
"   add.w   d6,d3\n"
"   cmp.w   d0,d2\n"
"   bpl.s   1f\n"
"   exg     d0,d2\n"
"1: cmp.w   d1,d3\n"
"   bpl.s   2f\n"
"   exg     d1,d3\n"
"2: movem.w 56(a3),d6-d7\n"
"   subq.w  #1,d6\n"
"   subq.w  #1,d7\n"
"   tst.w   d0\n"
"   bpl.s   3f\n"
"   clr.w   d0\n"
"3: tst.w   d1\n"
"   bpl.s   4f\n"
"   clr.w   d1\n"
"4: tst.w   d2\n"
"   bmi     9f\n"
"   tst.w   d3\n"
"   bmi     9f\n"
"   cmp.w   d6,d0\n"
"   bge     9f\n"
"   cmp.w   d7,d1\n"
"   bge     9f\n"
"   cmp.w   d6,d2\n"
"   blt.s   5f\n"
"   move.w  d6,d2\n"
"5: cmp.w   d7,d3\n"
"   blt.s   6f\n"
"   move.w  d7,d3\n"
"6: move.w  d2,d6\n"                    /* noclip2: parts of 990 */
"   sub.w   d0,d6\n"
"   cmp.w   #991,d6\n"
"   bmi.s   10f\n"
"   movem.l d0-d4/a3,-(sp)\n"
"   move.w  d0,d2\n"
"   add.w   #989,d2\n"
"   bsr.s   10f\n"
"   movem.l (sp)+,d0-d4/a3\n"
"   add.w   #990,d0\n"
"   bra.s   6b\n"
"10:\n"                                 /* dobof */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   lea     _lup1,a0\n"
"   move.w  d0,d5\n"
"   and.w   #15,d5\n"
"   add.w   d5,d5\n"
"   move.w  0(a0,d5.w),0x44(a2)\n"
"   lea     _lup2,a0\n"
"   move.w  d2,d5\n"
"   and.w   #15,d5\n"
"   add.w   d5,d5\n"
"   move.w  0(a0,d5.w),0x46(a2)\n"
"   move.w  #0xffff,0x74(a2)\n"
"   move.w  #0,0x42(a2)\n"
"   sub.w   d1,d3\n"
"   addq.w  #1,d3\n"
"   ext.l   d3\n"
"   asl.l   #6,d3\n"
"   mulu.w  (a3),d1\n"                  /* the offset */
"   lsr.w   #4,d0\n"
"   ext.l   d0\n"
"   add.l   d0,d1\n"
"   add.l   d0,d1\n"
"   lsr.w   #4,d2\n"
"   sub.w   d0,d2\n"
"   addq.w  #1,d2\n"
"   or.w    d2,d3\n"                    /* the size */
"   move.w  (a3),d6\n"
"   sub.w   d2,d6\n"
"   sub.w   d2,d6\n"
"   move.w  d6,0x66(a2)\n"
"   move.w  d6,0x60(a2)\n"
"   moveq   #0,d7\n"
"   move.w  4(a3),d2\n"
"   lea     8(a3),a3\n"
"   move.l  d3,d5\n"
"   swap    d5\n"
"   tst.w   d5\n"
"   bne     20f\n"                      /* longun */
"   tst.w   d4\n"
"   bmi.s   12f\n"
"11:\n"                                 /* nxp1 */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  (a3)+,d0\n"
"   add.l   d1,d0\n"
"   move.l  d0,0x48(a2)\n"
"   move.l  d0,0x54(a2)\n"
"   move.w  #0x3fa,0x40(a2)\n"
"   btst    d7,d4\n"
"   bne.s   13f\n"
"   move.w  #0x30a,0x40(a2)\n"
"13:move.w  d3,0x58(a2)\n"
"   addq.w  #1,d7\n"
"   cmp.w   d7,d2\n"
"   bne.s   11b\n"
"   rts\n"
"12:\n"                                 /* nxp2 */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  (a3)+,d0\n"
"   btst    d7,d4\n"
"   beq.s   14f\n"
"   add.l   d1,d0\n"
"   move.l  d0,0x48(a2)\n"
"   move.l  d0,0x54(a2)\n"
"   move.w  #0x35a,0x40(a2)\n"
"   move.w  d3,0x58(a2)\n"
"14:addq.w  #1,d7\n"
"   cmp.w   d7,d2\n"
"   bne.s   12b\n"
"9: rts\n"
"20:tst.w   d4\n"                       /* longun */
"   bmi.s   24f\n"
"21:\n"                                 /* lnxp1 */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  (a3)+,d0\n"
"   add.l   d1,d0\n"
"   move.l  d0,0x48(a2)\n"
"   move.l  d0,0x54(a2)\n"
"   move.w  #0x3fa,0x40(a2)\n"
"   btst    d7,d4\n"
"   bne.s   22f\n"
"   move.w  #0x30a,0x40(a2)\n"
"22:movem.w d3/d5,-(sp)\n"
"   and.w   #63,d3\n"
"   subq.w  #1,d5\n"
"23:move.w  d3,0x58(a2)\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   dbf     d5,23b\n"
"   movem.w (sp)+,d3/d5\n"
"   move.w  d3,0x58(a2)\n"
"   addq.w  #1,d7\n"
"   cmp.w   d7,d2\n"
"   bne.s   21b\n"
"   rts\n"
"24:\n"                                 /* lnxp2 */
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  (a3)+,d0\n"
"   btst    d7,d4\n"
"   beq.s   26f\n"
"   add.l   d1,d0\n"
"   move.l  d0,0x48(a2)\n"
"   move.l  d0,0x54(a2)\n"
"   move.w  #0x35a,0x40(a2)\n"
"   movem.w d3/d5,-(sp)\n"
"   and.w   #63,d3\n"
"   subq.w  #1,d5\n"
"25:move.w  d3,0x58(a2)\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   dbf     d5,25b\n"
"   movem.w (sp)+,d3/d5\n"
"   move.w  d3,0x58(a2)\n"
"26:addq.w  #1,d7\n"
"   cmp.w   d7,d2\n"
"   bne.s   24b\n"
"   rts");
BLITZ_AB_ROUTINE(_boxf, l2_boxfo, AMIGALINE(l2_boxfo));

/* -------------------------------------------------------------- ellipses */

/* cplot: the point d0,d1 in d4 (complement if negative) with the CPU
   (the original's macro); fixed (#392): y is compared with the clip
   height unsigned (BCC instead of BPL), so a point above the top edge is
   clipped; the original's signed compare let it pass, and the MULU of its
   unsigned word wrote it up to 65535 rows behind the plane.  The plane
   loops count with CMP.b on the depth byte (#393).  Changes d0, d1, d3,
   a0, a2. */
#define CPLOT(n)                                                            \
"   cmp.w   58(a3),d1\n"                                                    \
"   bcc     " #n "9f\n"                                                     \
"   mulu.w  (a3),d1\n"                                                      \
"   bmi     " #n "9f\n"                                                     \
"   move.w  d0,d3\n"                                                        \
"   bmi     " #n "9f\n"                                                     \
"   cmp.w   56(a3),d3\n"                                                    \
"   bpl     " #n "9f\n"                                                     \
"   lsr.w   #3,d3\n"                                                        \
"   ext.l   d3\n"                                                           \
"   add.l   d3,d1\n"                                                        \
"   and.w   #7,d0\n"                                                        \
"   eor.w   #7,d0\n"                                                        \
"   lea     8(a3),a2\n"                                                     \
"   moveq   #0,d3\n"                                                        \
"   tst.w   d4\n"                                                           \
"   bpl.s   " #n "2f\n"                                                     \
#n "1: move.l  (a2)+,a0\n"                  /* complement */                \
"   bchg.b  d0,0(a0,d1.l)\n"                                                \
"   addq.w  #1,d3\n"                                                        \
"   cmp.b   5(a3),d3\n"                                                     \
"   bne.s   " #n "1b\n"                                                     \
"   bra.s   " #n "9f\n"                                                     \
#n "2: move.l  (a2)+,a0\n"                                                  \
"   btst    d3,d4\n"                                                        \
"   bne.s   " #n "3f\n"                                                     \
"   bclr.b  d0,0(a0,d1.l)\n"                                                \
"   bra.s   " #n "4f\n"                                                     \
#n "3: bset.b  d0,0(a0,d1.l)\n"                                             \
#n "4: addq.w  #1,d3\n"                                                     \
"   cmp.b   5(a3),d3\n"                                                     \
"   bne.s   " #n "2b\n"                                                     \
#n "9:\n"

/* Circle x,y,r,colour / Ellipse x,y,rx,ry,colour: the midpoint ellipse
   (calcellipse: x in d6, y in d7, the decision in d5, ev the rest), four
   points per step with cplot; a3 = the bitmap; changes d0-d7, a0, a2 */
BLITZ_ASM_BLOCK(ellipse,
"   .globl  __circle, __ellipse\n"
"__circle:\n"
"   move.w  d3,d4\n"
"   move.w  d2,d3\n"
"__ellipse:\n"
"   clr.w   _ev+24\n"                   /* flag */
"   add.w   48(a3),d0\n"
"   add.w   50(a3),d1\n"
"   movem.w d0-d4,_lv\n"                /* center, radii, color */
"   bsr     l2_calcellipse\n"
"1: movem.w _lv,d0-d1\n"                /* nxelp */
"   add.w   d6,d0\n"
"   add.w   d7,d1\n"
CPLOT(1)
"   movem.w _lv,d0-d1\n"
"   add.w   d6,d0\n"
"   sub.w   d7,d1\n"
CPLOT(2)
"   movem.w _lv,d0-d1\n"
"   sub.w   d6,d0\n"
"   add.w   d7,d1\n"
CPLOT(3)
"   movem.w _lv,d0-d1\n"
"   sub.w   d6,d0\n"
"   sub.w   d7,d1\n"
CPLOT(4)
"   movem.l _ev,d0-d3\n"                /* dx, dy, asq, bsq */
"   tst.w   _ev+24\n"
"   bne.s   6f\n"
"   cmp.l   d1,d0\n"
"   bpl.s   5f\n"                       /* dxgtdy */
"   tst.l   d5\n"
"   ble.s   4f\n"
"   subq.w  #1,d7\n"
"   sub.l   _ev+16,d1\n"
"   sub.l   d1,d5\n"
"4: addq.w  #1,d6\n"                    /* nodab */
"   add.l   _ev+20,d0\n"
"   add.l   d0,d5\n"
"   add.l   _ev+12,d5\n"
"   movem.l d0-d1,_ev\n"
"   bra     1b\n"
"5: not.w   _ev+24\n"                   /* dxgtdy */
"   sub.l   d3,d2\n"
"   move.l  d2,d3\n"
"   asl.l   #1,d2\n"
"   add.l   d3,d2\n"
"   asr.l   #1,d2\n"
"   sub.l   d0,d2\n"
"   sub.l   d1,d2\n"
"   asr.l   #1,d2\n"
"   add.l   d2,d5\n"
"6: tst.w   d7\n"                       /* dooly */
"   ble.s   9f\n"
"   tst.l   d5\n"
"   bpl.s   7f\n"
"   addq.w  #1,d6\n"
"   add.l   _ev+20,d0\n"
"   add.l   d0,d5\n"
"7: subq.w  #1,d7\n"                    /* dodab */
"   sub.l   _ev+16,d1\n"
"   add.l   _ev+8,d5\n"
"   sub.l   d1,d5\n"
"   movem.l d0-d1,_ev\n"
"   bra     1b\n"
"9: rts\n"
/* calcellipse: d2 = a, d3 = b: y = b (d7), x = 0 (d6), d (d5), ev */
"l2_calcellipse:\n"
"   move.w  d3,d7\n"
"   mulu.w  d2,d2\n"
"   move.l  d2,_ev+8\n"                 /* asq */
"   asl.l   #1,d2\n"
"   move.l  d2,_ev+16\n"                /* asq2 */
"   mulu.w  d3,d3\n"
"   move.l  d3,_ev+12\n"                /* bsq */
"   asl.l   #1,d3\n"
"   move.l  d3,_ev+20\n"                /* bsq2 */
"   move.l  _ev+12,d5\n"
"   move.l  _ev+8,d3\n"
"   move.w  d3,d6\n"                    /* asq * b, two MULUs */
"   mulu.w  d7,d6\n"
"   swap    d3\n"
"   mulu.w  d7,d3\n"
"   swap    d3\n"
"   clr.w   d3\n"
"   add.l   d6,d3\n"
"   sub.l   d3,d5\n"
"   move.l  _ev+8,d3\n"
"   asr.l   #2,d3\n"
"   add.l   d3,d5\n"
"   moveq   #0,d0\n"
"   move.l  d0,_ev\n"                   /* dx */
"   move.l  _ev+16,d3\n"
"   move.w  d3,d6\n"                    /* asq2 * b */
"   mulu.w  d7,d6\n"
"   swap    d3\n"
"   mulu.w  d7,d3\n"
"   swap    d3\n"
"   clr.w   d3\n"
"   add.l   d6,d3\n"
"   move.l  d3,_ev+4\n"                 /* dy */
"   moveq   #0,d6\n"
"   rts");

/* cplot2: toggle the bit of d0,d1 in the fill buffer a0, x clipped at
   d4 (the original's macro; y as in cplot, fixed #392: unsigned, a point
   above the top edge is clipped and no longer toggled behind the fill
   buffer).  Changes d0, d1, d3. */
#define CPLOT2(n)                                                           \
"   cmp.w   58(a3),d1\n"                                                    \
"   bcc.s   " #n "9f\n"                                                     \
"   mulu.w  (a3),d1\n"                                                      \
"   bmi.s   " #n "9f\n"                                                     \
"   move.w  d0,d3\n"                                                        \
"   bmi.s   " #n "9f\n"                                                     \
"   cmp.w   d4,d3\n"                                                        \
"   bmi.s   " #n "1f\n"                                                     \
"   move.w  d4,d3\n"                                                        \
"   move.w  d4,d0\n"                                                        \
#n "1: lsr.w   #3,d3\n"                                                     \
"   ext.l   d3\n"                                                           \
"   add.l   d3,d1\n"                                                        \
"   and.w   #7,d0\n"                                                        \
"   eor.w   #7,d0\n"                                                        \
"   bchg.b  d0,0(a0,d1.l)\n"                                                \
#n "9:\n"

/* the state of Ellipsef: the modulo and size of the blits (0: bigsize),
   the offset of the rectangle in a plane, its address in the fill
   buffer */
static struct {
    UWORD mod, size;            /* modsize */
    ULONG tempadd, blitpos;
} fv __attribute__((used));
static_assert(offsetof(__typeof__(fv), tempadd) == 4 && offsetof(__typeof__(fv), blitpos) == 8);

/* Circlef x,y,r,colour / Ellipsef x,y,rx,ry,colour (no origin): the
   outline as one bit per line into the fill buffer (the rectangle of the
   ellipse cleared first), filled with the blitter (descending, exclusive
   fill), then through it into each plane: colour bit 1 sets, 0 clears, a
   negative colour complements (DBRA over depth - 1: 0 gives 65536).  The
   original's code (assembler, see the lines above: the C stayed 1.27
   times slower), with the fix of #392 in cplot2.  a3 = the bitmap, a2 =
   the chip base; leaves a3 behind the plane pointers (the bitmap if
   nothing is drawn); changes d0-d7, a0, a1. */
BLITZ_ASM_BLOCK(ellipsef,
"   .globl  _l2_circlefo, _l2_ellipsefo\n"
"_l2_circlefo:\n"
"   move.w  d3,d4\n"
"   move.w  d2,d3\n"
"_l2_ellipsefo:\n"
"   movem.l d0-d1,-(sp)\n"
"   jsr     __allocfill\n"              /* the fill buffer: fbloc */
"   movem.l (sp)+,d0-d1\n"
"   clr.w   _ev+24\n"
"   movem.w d0-d4,_lv\n"                /* center, radii, color */
"   movem.l a4-a5,-(sp)\n"
"   move.w  d0,d6\n"
"   move.w  d1,d7\n"
"   moveq   #1,d4\n"                    /* the radii + 1 */
"   add.w   d4,d2\n"
"   add.w   d4,d3\n"
"   sub.w   d2,d0\n"
"   bpl.s   1f\n"
"   clr.w   d0\n"
"1: sub.w   d3,d1\n"
"   bpl.s   2f\n"
"   clr.w   d1\n"
"2: add.w   d2,d6\n"
"   bmi     40f\n"                      /* pa */
"   add.w   d3,d7\n"
"   bmi     40f\n"
"   moveq   #4,d4\n"                    /* in words */
"   lsr.w   d4,d0\n"
"   lsr.w   d4,d6\n"
"   movem.w 56(a3),d4-d5\n"
"   lsr.w   #4,d4\n"
"   cmp.w   d4,d0\n"
"   bpl     40f\n"
"   cmp.w   d5,d1\n"
"   bpl     40f\n"
"   cmp.w   d4,d6\n"
"   bmi.s   3f\n"
"   move.w  d4,d6\n"
"   subq.w  #1,d6\n"
"3: cmp.w   d5,d7\n"
"   bmi.s   4f\n"
"   move.w  d5,d7\n"
"   subq.w  #1,d7\n"
"4: sub.w   d7,d1\n"                    /* the lines */
"   neg.w   d1\n"
"   addq.w  #1,d1\n"
"   sub.w   d6,d0\n"                    /* the words */
"   neg.w   d0\n"
"   addq.w  #1,d0\n"
"   cmp.w   #1024,d1\n"
"   bpl.s   5f\n"
"   cmp.w   #64,d0\n"
"   bmi.s   6f\n"
"5: move.w  d1,_bigsize\n"              /* bigo */
"   move.w  d0,_bigsize+2\n"
"   moveq   #0,d1\n"
"   bra.s   7f\n"
"6: asl.w   #6,d1\n"
"   or.w    d0,d1\n"
"7: asl.w   #1,d0\n"                    /* domod */
"   sub.w   (a3),d0\n"
"   neg.w   d0\n"
"   mulu.w  (a3),d7\n"                  /* descending: the last word */
"   move.l  d7,a0\n"
"   add.w   d6,a0\n"
"   add.w   d6,a0\n"
"   move.l  a0,_fv+4\n"                 /* tempadd */
"   add.l   _fbloc,a0\n"
"   move.l  a0,_fv+8\n"                 /* blitpos */
"   movem.w d0-d1,_fv\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  #-1,0x44(a2)\n"             /* clear the rectangle */
"   move.l  a0,0x54(a2)\n"
"   move.w  d0,0x66(a2)\n"
"   move.l  #0x01000002,0x40(a2)\n"
"   tst.w   d1\n"
"   bne.s   8f\n"
"   move.l  _bigsize,0x5c(a2)\n"
"   bra.s   9f\n"
"8: move.w  d1,0x58(a2)\n"
"9:\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   movem.w _lv,d0-d3\n"
"   bsr     l2_calcellipse\n"
"   move.l  _fbloc,a0\n"
"   move.w  56(a3),d4\n"
"   subq.w  #1,d4\n"
"10:tst.w   d7\n"                       /* nxelp2 */
"   beq     11f\n"
"   movem.w _lv,d0-d1\n"
"   add.w   d6,d0\n"
"   add.w   d7,d1\n"
CPLOT2(5)
"   movem.w _lv,d0-d1\n"
"   sub.w   d6,d0\n"
"   add.w   d7,d1\n"
CPLOT2(6)
"11:movem.w _lv,d0-d1\n"                /* j1e */
"   add.w   d6,d0\n"
"   sub.w   d7,d1\n"
CPLOT2(7)
"   movem.w _lv,d0-d1\n"
"   sub.w   d6,d0\n"
"   sub.w   d7,d1\n"
CPLOT2(8)
"12:movem.l _ev,d0-d3\n"                /* flop9 */
"   tst.w   _ev+24\n"
"   bne.s   14f\n"
"   cmp.l   d1,d0\n"
"   bpl.s   13f\n"
"   tst.l   d5\n"
"   ble.s   15f\n"
"   subq.w  #1,d7\n"
"   sub.l   _ev+16,d1\n"
"   sub.l   d1,d5\n"
"   addq.w  #1,d6\n"
"   add.l   _ev+20,d0\n"
"   add.l   d0,d5\n"
"   add.l   _ev+12,d5\n"
"   movem.l d0-d1,_ev\n"
"   bra     10b\n"
"15:addq.w  #1,d6\n"                    /* nodab2: no plot on the same line */
"   add.l   _ev+20,d0\n"
"   add.l   d0,d5\n"
"   add.l   _ev+12,d5\n"
"   movem.l d0-d1,_ev\n"
"   bra.s   12b\n"
"13:not.w   _ev+24\n"                   /* dxgtdy2 */
"   sub.l   d3,d2\n"
"   move.l  d2,d3\n"
"   asl.l   #1,d2\n"
"   add.l   d3,d2\n"
"   asr.l   #1,d2\n"
"   sub.l   d0,d2\n"
"   sub.l   d1,d2\n"
"   asr.l   #1,d2\n"
"   add.l   d2,d5\n"
"14:tst.w   d7\n"                       /* dooly2 */
"   beq.s   20f\n"
"   tst.l   d5\n"
"   bpl.s   16f\n"
"   addq.w  #1,d6\n"
"   add.l   _ev+20,d0\n"
"   add.l   d0,d5\n"
"16:subq.w  #1,d7\n"                    /* dodab2 */
"   sub.l   _ev+16,d1\n"
"   add.l   _ev+8,d5\n"
"   sub.l   d1,d5\n"
"   movem.l d0-d1,_ev\n"
"   bra     10b\n"
"20:move.l  _fv+8,a0\n"                 /* DoFill */
"   movem.w _fv,d0-d1\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  a0,0x50(a2)\n"
"   move.l  a0,0x54(a2)\n"
"   move.w  d0,0x64(a2)\n"
"   move.w  d0,0x66(a2)\n"
"   move.l  #0x09f0000a,0x40(a2)\n"
"   tst.w   d1\n"
"   bne.s   21f\n"
"   move.l  _bigsize,0x5c(a2)\n"
"   bra.s   22f\n"
"21:move.w  d1,0x58(a2)\n"
"22:move.l  _fv+8,a0\n"                 /* DoTransfer */
"   movem.w _fv,d0-d1\n"
"   moveq   #-1,d3\n"
"   add.w   4(a3),d3\n"
"   lea     8(a3),a3\n"
"   move.l  _fv+4,d6\n"
"   move.w  _lv+8,d4\n"
"   bmi.s   30f\n"
"23:move.l  (a3)+,a1\n"                 /* nxplaner */
"   add.l   d6,a1\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  a0,0x50(a2)\n"
"   move.l  a1,0x4c(a2)\n"
"   move.l  a1,0x54(a2)\n"
"   move.w  d0,0x64(a2)\n"
"   move.w  d0,0x62(a2)\n"
"   move.w  d0,0x66(a2)\n"
"   move.l  #0x0dfc0002,0x40(a2)\n"
"   lsr.w   #1,d4\n"
"   bcs.s   24f\n"
"   move.w  #0x0d0c,0x40(a2)\n"
"24:tst.w   d1\n"
"   bne.s   25f\n"
"   move.l  _bigsize,0x5c(a2)\n"
"   bra.s   26f\n"
"25:move.w  d1,0x58(a2)\n"
"26:dbf     d3,23b\n"
"   movem.l (sp)+,a4-a5\n"
"   rts\n"
"30:move.l  (a3)+,a1\n"                 /* xort */
"   add.l   d6,a1\n"
"   btst    #6,2(a2)\n"
"0: btst    #6,2(a2)\n"
"   bne.s   0b\n"
"   move.l  a0,0x50(a2)\n"
"   move.l  a1,0x4c(a2)\n"
"   move.l  a1,0x54(a2)\n"
"   move.w  d0,0x64(a2)\n"
"   move.w  d0,0x62(a2)\n"
"   move.w  d0,0x66(a2)\n"
"   move.l  #0x0d3c0002,0x40(a2)\n"
"   tst.w   d1\n"
"   bne.s   31f\n"
"   move.l  _bigsize,0x5c(a2)\n"
"   bra.s   32f\n"
"31:move.w  d1,0x58(a2)\n"
"32:dbf     d3,30b\n"
"40:movem.l (sp)+,a4-a5\n"              /* pa */
"   rts");
BLITZ_AB_ROUTINE(_circlef, l2_circlefo, AMIGALINE(l2_circlefo));
BLITZ_AB_ROUTINE(_ellipsef, l2_ellipsefo, AMIGALINE(l2_ellipsefo));

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
   BLTSIZE in *size (the original's FloodFill with border inverted only
   there, fixed #986: invertmask). */
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

/* invertmask: D = NOT A over the mask that mono fetched into the fill
   buffer, in its columns of 64 words and parts of 1024 lines (fixed,
   #986: the original inverted only with the pointer and the size mono
   left, those of the last column and of its last part).  A bitmap of at
   most 1024 x 1024 pixels gets the one blit of the original, with the
   modulos mono left. */
static void invertmask(UBYTE *chip, const struct bb_bitmap *bm, UBYTE *last)
{
    struct columns c;
    UBYTE *mask;
    UWORD m;

    columns(bm, &c);
    mask = last - c.last;
    BLITWAIT(chip);
    CW(chip, BLTCON0) = 0x090f;
    if (c.last) {
        CW(chip, BLTAMOD) = bm->bpr - 128;
        CW(chip, BLTDMOD) = bm->bpr - 128;
        do {                                    /* the full columns */
            CL(chip, BLTAPT) = (ULONG)mask;
            CL(chip, BLTDPT) = (ULONG)mask;
            blitsize3(chip, c.parts, c.size & 0xffc0);
            BLITWAIT(chip);
            mask += 128;
        } while (mask != last);
        m = bm->bpr & 127;
        if (!m)
            m = 128;
        CW(chip, BLTAMOD) = bm->bpr - m;
        CW(chip, BLTDMOD) = bm->bpr - m;
    }
    CL(chip, BLTAPT) = (ULONG)last;
    CL(chip, BLTDPT) = (ULONG)last;
    blitsize3(chip, c.parts, c.size);
}

/* FloodFill x,y,colour,border: fill up to the border colour (the mask of
   the border colour inverted).  The border colour is a word in the
   register of x (SETW(d0, d3)). */
static UBYTE *floodfill2(struct bb_bitmap *bm, UBYTE *chip, ULONG x, WORD y, UWORD colour,
                         UWORD border)
{
    UBYTE *mask;
    UWORD size;

    coords[0] = x;
    coords[1] = y;
    fcol = colour;
    mask = mono(chip, bm, (x & 0xffff0000UL) | border, 0, &size);
    invertmask(chip, bm, mask);
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
   65536 lines.  Fixed (#396): fewer than 2 points are an invalid
   argument, refused by _polychk with runtime error checking (the loop
   unchanged).  Returns the a3 of the last line. */
/* the steps along an edge (as polylib's): an x position is a fixed point
   number with the fraction in the upper word and twice the integer in the
   lower word; ADD.l adds a step, its carry adds 2 to the lower word */
BL_ALWAYS_INLINE ULONG edge_step(ULONG pos, ULONG step)
{
    ULONG sum = pos + step;

    if (sum < pos)
        sum = (sum & 0xffff0000UL) | (UWORD)(sum + 2);
    return sum;
}

/* the start: x (sign extended) with the lower word 2 * x, the fraction
   word EORed with $8000 (one half; $7FFF for a negative x) */
BL_ALWAYS_INLINE ULONG edge_start(WORD x)
{
    return (((ULONG)(LONG)x & 0xffff0000UL) | (UWORD)(x + x)) ^ 0x80000000UL;
}

/* the next edge of a list: its lines - 1 and its step; 0 at the end mark */
BL_ALWAYS_INLINE int next_edge(const struct edge **list, UWORD *lines, ULONG *step)
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
BL_ALWAYS_INLINE UWORD *side(UWORD *list, WORD x, WORD y, const struct point *p, int dir,
                              int right, WORD width, WORD height)
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
            WORD dx = from->x - x;

            /* Fixed (#1033): a vertical edge beyond the clip width has no
               crossing to cut at (the side starts on it: the start handling
               moves in one start only, and the clipping of #1000 can put
               such an edge on x = -1 or x = width, #1034); the original
               divides by its dx of 0.  Its lines stay: the drawing loop
               stops at the first, whose x is beyond the clip width. */
            if (dx)
                e[-1].lines -= muldiv(right ? x : x - last, from->y - y, dx);
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
   pts (polypts or the clipped points; n points, twice in a row) - the top
   line, clipped at the top edge and at the sides, and the edge lists of
   the left side (backwards through the points, into ledges) and the right
   side (redges).  NULL if nothing is visible, else the address of the
   right list's end mark (where the original leaves a3). */
BL_ALWAYS_INLINE UWORD *outline(const struct point *pts, UWORD n, WORD width, WORD height,
                                UWORD *ledges, UWORD *redges)
{
    const struct point *p = pts, *lp, *rp;
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
    side(ledges, lx, y, lp, -1, 0, width, height);
    return side(redges, rx, y, rp, 1, 1, width, height);
}

/* Fixed (#1000): the clipping of a polygon outside the range of the word
   arithmetic.  CLIPMAX: the most points of a clipped polygon (16 + 4 for
   a convex one; a concave one may get more, then nothing is drawn) */
#define CLIPMAX 20

/* POLYQUICK(any): every point (any = the points ORed together as longs)
   at 0..8191 in x and y - then polyinrange is true (the points within
   +-16383, every edge within +-8191), without its loop */
#define POLYQUICK(any) (!((any) & 0xe000e000UL))

/* the range in which outline, side and drawpoly compute exactly: every
   edge (the points are twice in pts, so p[1] of the last is the first)
   at most 8191 in x and y (the slope DIVS of side fits in a word, the
   shift count stays positive), every point within +-16383 (twice x is a
   word in edge_start).  The differences in word arithmetic: they wrap
   only where a point is out of range, and then the answer is 0 anyway
   (every point is tested). */
static int polyinrange(const struct point *p, UWORD n)
{
    do {
        UWORD dx = p[1].x - p->x, dy = p[1].y - p->y;

        if ((UWORD)(p->x + 16383) > 32766 || (UWORD)(p->y + 16383) > 32766
            || (UWORD)(dx + 8191) > 16382 || (UWORD)(dy + 8191) > 16382)
            return 0;
        p++;
    } while (--n);
    return 1;
}

static WORD coord(const struct point *p, int yaxis)
{
    return yaxis ? p->y : p->x;
}

/* one pass of the clipping (Sutherland-Hodgman): the points of src (n)
   on the kept side of the line x = c (yaxis: y = c; high: keep >= c, else
   <= c), with the crossings of the edges, into dst.  A crossing is the
   other coordinate of the edge at c, from its first point a, rounded:
   the difference times the part of the edge up to c over the edge's
   length with MULU/DIVU (all below 65536, the quotient at most the
   difference).
   Returns the number of points, 0 if they are more than CLIPMAX. */
static UWORD clipline(struct point *dst, const struct point *src, UWORD n, int yaxis, WORD c,
                      int high)
{
    const struct point *a = src + n - 1;
    UWORD m = 0;

    for (; n; n--, a = src++) {
        LONG ua = coord(a, yaxis), ub = coord(src, yaxis);
        int ain = high ? ua >= c : ua <= c, bin = high ? ub >= c : ub <= c;

        if (ain != bin) {
            LONG va = coord(a, !yaxis), dv = (LONG)coord(src, !yaxis) - va;
            ULONG len = ub > ua ? ub - ua : ua - ub, part = c > ua ? c - ua : ua - c;
            UWORD q = (UWORD)bl_divu_w(bl_mulu_w(dv < 0 ? -dv : dv, part) + (len >> 1), len);

            if (m == CLIPMAX)
                return 0;
            va += dv < 0 ? -(LONG)q : (LONG)q;
            dst[m].x = yaxis ? (WORD)va : c;
            dst[m].y = yaxis ? c : (WORD)va;
            m++;
        }
        if (bin) {
            if (m == CLIPMAX)
                return 0;
            dst[m++] = *src;
        }
    }
    return m;
}

/* the polygon of polypts (n points) clipped to the bitmap plus one pixel
   (x -1..width, y -1..height; both at most 8190) into dst (2 * CLIPMAX
   points: the passes alternate between its halves), its points twice in
   a row as outline wants them.  Returns the number of points. */
static UWORD clippoly(struct point *dst, UWORD n, WORD width, WORD height)
{
    struct point *b = dst + CLIPMAX;
    UWORD i;

    n = clipline(b, polypts, n, 0, -1, 1);
    n = clipline(dst, b, n, 0, width, 0);
    n = clipline(b, dst, n, 1, -1, 1);
    n = clipline(dst, b, n, 1, height, 0);
    for (i = 0; i < n; i++)
        dst[n + i] = dst[i];
    return n;
}

/* the state of the line loop of drawpoly (the original's bmoff, bmwid,
   rtclip, pcolor) */
static struct {
    ULONG bmoff;                /* the offset of the line in a plane */
    ULONG bmwid;                /* bytes per row */
    UWORD rtclip;               /* 2 * (clip width - 1) */
    UWORD pcolor;               /* the colour (bits of the low byte) */
} pq __attribute__((used));
static_assert(offsetof(__typeof__(pq), bmwid) == 4 && offsetof(__typeof__(pq), rtclip) == 8
              && offsetof(__typeof__(pq), pcolor) == 10);

/* ploop: the lines of the polygon, one blit per plane (the original's
   loop: the C kept its state on the stack and was 1.8 times slower).
   In: d0/d1 the left/right x position (edge_start), d2/d3 the lines - 1
   of the current left/right edge, d4/d5 their steps, a0/a1 the next left
   and right edge (struct edge lists), a2 the a3 to leave if nothing is
   drawn, a6 the bitmap, pq.  Out: d0 the a3 the original leaves (the
   last plane pointer).  Changes d0-d7, a0-a2; keeps a3-a6. */
BLITZ_ASM_BLOCK(ploop,
"   .globl  _ploop\n"
"_ploop:\n"
"   movem.l a3-a5,-(sp)\n"
"   move.l  a2,a3\n"
"   lea     0xdff000,a4\n"
"   btst    #6,2(a4)\n"                 /* bltsetup */
"1: btst    #6,2(a4)\n"
"   bne.s   1b\n"
"   move.w  #-1,0x74(a4)\n"
"   move.w  #0,0x42(a4)\n"
"2: add.l   d4,d0\n"                    /* ploop */
"   bcc.s   3f\n"
"   addq.w  #2,d0\n"
"3: add.l   d5,d1\n"
"   bcc.s   4f\n"
"   addq.w  #2,d1\n"
"4: move.w  d0,d6\n"
"   cmp.w   _pq+8(pc),d6\n"
"   bgt     20f\n"
"   swap    d6\n"                       /* the start word in the upper word */
"   move.w  d0,d6\n"
"   bpl.s   5f\n"
"   moveq   #0,d6\n"
"5: tst.w   d1\n"
"   bmi     20f\n"
"   move.w  _pq+8(pc),d7\n"
"   cmp.w   d1,d7\n"
"   bmi.s   6f\n"
"   move.w  d1,d7\n"
"6: btst    #6,2(a4)\n"
"7: btst    #6,2(a4)\n"
"   bne.s   7b\n"
"   lea     _lup1(pc),a2\n"
"   and.w   #30,d6\n"
"   move.w  0(a2,d6.w),0x44(a4)\n"
"   move.w  d7,d6\n"
"   and.w   #30,d6\n"
"   lea     _lup2(pc),a2\n"
"   move.w  0(a2,d6.w),0x46(a4)\n"
"   swap    d6\n"
"   lsr.w   #5,d6\n"
"   lsr.w   #5,d7\n"
"   sub.w   d6,d7\n"
"   bmi     20f\n"
"   add.w   #65,d7\n"
"   add.w   d6,d6\n"
"   move.l  _pq(pc),a2\n"               /* bmoff */
"   add.w   d6,a2\n"
"   moveq   #0,d6\n"
"   lea     8(a6),a5\n"
"8: btst    #6,2(a4)\n"                 /* nxpolyplane */
"9: btst    #6,2(a4)\n"
"   bne.s   9b\n"
"   move.w  #0x3fa,0x40(a4)\n"
"   btst    d6,_pq+11(pc)\n"            /* the colour bit (byte) */
"   bne.s   10f\n"
"   move.w  #0x30a,0x40(a4)\n"
"10:move.l  (a5)+,a3\n"
"   add.l   a2,a3\n"
"   move.l  a3,0x48(a4)\n"
"   move.l  a3,0x54(a4)\n"
"   move.w  d7,0x58(a4)\n"
"   addq.w  #1,d6\n"
"   cmp.w   4(a6),d6\n"
"   bne.s   8b\n"
"   move.l  _pq+4(pc),d7\n"             /* the next line */
"   add.l   d7,_pq\n"
"   add.l   d4,d0\n"
"   bcc.s   11f\n"
"   addq.w  #2,d0\n"
"11:add.l   d5,d1\n"
"   bcc.s   12f\n"
"   addq.w  #2,d1\n"
"12:dbf     d2,13f\n"
"   move.w  (a0)+,d2\n"
"   bmi.s   20f\n"
"   move.l  (a0)+,d4\n"
"13:dbf     d3,2b\n"
"   move.w  (a1)+,d3\n"
"   bmi.s   20f\n"
"   move.l  (a1)+,d5\n"
"   bra     2b\n"
"20:move.l  a3,d0\n"
"   movem.l (sp)+,a3-a5\n"
"   rts");

/* drawpoly: the polygon in polypts (plus the origin, added to both copies
   of the points) filled with the blitter, one blit per line and plane: A
   = $FFFF in the span's masks, D = C = the plane, set or cleared by the
   colour bit; a negative colour draws nothing.  a3 = what the copy loop
   left; returns the a3 the original leaves (or that one).  any: the
   points ORed together (as longs), for the quick range test. */
BL_ALWAYS_INLINE UBYTE *drawpoly(struct bb_bitmap *bm, UWORD n, UWORD colour, UBYTE *a3,
                                 ULONG any)
{
    struct point clip[2 * CLIPMAX];
    UWORD cledges[CLIPMAX * 3 + 2], credges[CLIPMAX * 3 + 2];
    const struct point *pts = polypts;
    UWORD *ledges = pv.ledges, *redges = pv.redges;
    const struct edge *le, *re;
    ULONG left, right, lstep, rstep, rowoff;
    UWORD llines, rlines, rtclip;
    UBYTE *end;

    if ((WORD)colour < 0)                       /* TST pcolor */
        return a3;
    if (bm->xorigin || bm->yorigin) {           /* porigin */
        struct point *p = polypts;
        UWORD k = n - 1;

        any = 0xffffffffUL;                     /* the full range test */

        do {
            p[0].x += bm->xorigin;
            p[0].y += bm->yorigin;
            p[1].x += bm->xorigin;
            p[1].y += bm->yorigin;
            p += 2;
        } while (k-- != 0);
    }
    /* Fixed (#1000): outside the range of the word arithmetic, clip with
       longs first (into the edge lists of this frame: the clipped polygon
       may have more edges per side than the lists of pv hold); fewer than
       3 points are no polygon, more than 16 have already overflowed
       polypts (#395), and a bitmap wider or higher than 8190 would leave
       the range again: those go on as in the original */
    if (n >= 3 && n <= 16 && bm->width <= 8190 && bm->height <= 8190
        && !POLYQUICK(any) && !polyinrange(polypts, n)) {
        n = clippoly(clip, n, bm->width, bm->height);
        if (n < 3)
            return a3;
        pts = clip;
        ledges = cledges;
        redges = credges;
    }
    le = (const struct edge *)ledges;
    re = (const struct edge *)redges;
    end = (UBYTE *)outline(pts, n, bm->width, bm->height, ledges, redges);
    if (!end)
        return a3;
    /* the end mark of the right list, where the original leaves a3; for
       a clipped polygon that list is in this frame, and the a3 left is
       only an address (nothing reads through it): the empty asm keeps
       GCC from tracing it to the frame (-Wreturn-local-addr) */
    __asm__("" : "+r"(end));
    a3 = end;
    rowoff = bl_mulu_w(pv.top.y, bm->bpr);
    rtclip = (bm->width - 1) * 2;
    left = edge_start(pv.top.left);
    right = edge_start(pv.top.right);
    if (!next_edge(&le, &llines, &lstep) || !next_edge(&re, &rlines, &rstep))
        return a3;
    pq.bmoff = rowoff;
    pq.bmwid = bm->bpr;
    pq.rtclip = rtclip;
    pq.pcolor = colour;
    {
        register ULONG d0 __asm("d0") = left, d1 __asm("d1") = right;
        register ULONG d2 __asm("d2") = llines, d3 __asm("d3") = rlines;
        register ULONG d4 __asm("d4") = lstep, d5 __asm("d5") = rstep;
        register const struct edge *a0 __asm("a0") = le, *a1 __asm("a1") = re;
        register UBYTE *a2 __asm("a2") = a3;
        register struct bb_bitmap *a6 __asm("a6") = bm;

        __asm__ volatile("jsr     _ploop"
                         : "+d"(d0), "+d"(d1), "+d"(d2), "+d"(d3), "+d"(d4), "+d"(d5),
                           "+a"(a0), "+a"(a1), "+a"(a2)
                         : "a"(a6)
                         : "d6", "d7", "cc", "memory");
        return (UBYTE *)d0;
    }
}

/* MULU.w of the low words of a and b: MULU reads only the low word of
   its destination, so a and b need no zero extension */
BL_ALWAYS_INLINE ULONG mulu_lo(ULONG a, ULONG b)
{
    __asm__("mulu.w %1,%0" : "+d"(a) : "d"(b) : "cc");
    return a;
}

/* a * b > c * d for |a|, |b|, |c|, |d| below 65536 (differences of two
   words): the magnitudes with MULU (each fits in the low word), then the
   signs */
BL_ALWAYS_INLINE int prodgt(LONG a, LONG b, LONG c, LONG d)
{
    ULONG p = mulu_lo(a < 0 ? -a : a, b < 0 ? -b : b);
    ULONG q = mulu_lo(c < 0 ? -c : c, d < 0 ? -d : d);
    int np = p && (a < 0) != (b < 0), nq = q && (c < 0) != (d < 0);

    if (np != nq)
        return nq;
    return np ? p < q : p > q;
}

/* Polyf: the points twice into polypts - backwards if they run anti-
   clockwise (by the first three points), then with colour2 (#395: more
   than 16 points overflow it; _polyfchk refuses them) */
static UBYTE *polyf(struct bb_bitmap *bm, UWORD n, const struct point *pts, UWORD colour,
                    UWORD colour2)
{
    const ULONG *s = (const ULONG *)pts;
    ULONG *p, *q, any = 0;
    UWORD k = n - 1;

    /* Fixed (#1000): the differences as longs, the products compared
       exactly (the original: MULS of the word differences) */
    if (prodgt((LONG)pts[1].x - pts[0].x, (LONG)pts[2].y - pts[1].y,
               (LONG)pts[2].x - pts[1].x, (LONG)pts[1].y - pts[0].y)) {  /* clck */
        p = (ULONG *)polypts;
        q = p + n;
        do {                                    /* a point as one long */
            ULONG v = *s++;

            any |= v;
            *p++ = v;
            *q++ = v;
        } while (k-- != 0);
    } else {                                    /* anti */
        p = (ULONG *)polypts + n;
        q = p + n;
        do {
            ULONG v = *s++;

            any |= v;
            *--p = v;
            *--q = v;
        } while (k-- != 0);
        colour = colour2;
    }
    return drawpoly(bm, n, colour, (UBYTE *)q, any);
}

/* ------------------------------------------------------------- ClipBlit */

/* Speed: the clip and blit code is inline (BL_ALWAYS_INLINE), and the
   Blitz mode code of ClipBlit saves no register (BLITZ_ENTRYPOINT, a6
   kept by BLITZ_EP_KEEP_A6; the register rule keeps a3-a7). */

static void BLITZ_ROUTINE _doclipblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_ALIAS(_doclipblitmode, _doclipblitmode_c);

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
BL_ALWAYS_INLINE int docblit(UBYTE *chip, struct bb_shape *sh, struct bb_bitmap *bm,
                             WORD x, WORD y, ULONG r6, ULONG r7, struct cblit *cb)
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
BL_ALWAYS_INLINE void docblitx(UBYTE *chip, struct bb_shape *sh, struct bb_bitmap *bm,
                               WORD x, WORD y, UWORD excess, ULONG r6, ULONG r7)
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
    static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_##name##_c(__VA_ARGS__,   \
                                                           UBYTE *chip BL_REG(a2)) \
    {                                                                       \
        BLITZ_EP_KEEP_A6();                                                 \
        struct bb_bitmap *bm = (struct bb_bitmap *)blitz_string;            \
                                                                            \
        blitz_string = call;                                                \
        BLITZ_EP_KEEP_A6_END();                                             \
    }                                                                       \
    BLITZ_EP_MARK(b_##name##_c);                                            \
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

BLITPAIR(floodfill, floodfill(bm, chip, x, y, c),
         WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(floodfill2, floodfill2(bm, chip, x, y, c, border),
         ULONG x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2), UWORD border BL_REG(d3))
BLITPAIR(remap, remap(bm, chip, from, to, bm), ULONG from BL_REG(d0), ULONG to BL_REG(d1))
BLITPAIR(remap2, remap(bm, chip, from, to, dst), ULONG from BL_REG(d0), ULONG to BL_REG(d1),
         struct bb_bitmap *dst BL_REG(a0))
BLITPAIR(polyf, ((void)chip, polyf(bm, n, (const struct point *)pts, c, c)),
         UWORD n BL_REG(d0), ULONG pts BL_REG(d1), UWORD c BL_REG(d2))
BLITPAIR(polyf2, ((void)chip, polyf(bm, n, (const struct point *)pts, c, c2)),
         UWORD n BL_REG(d0), ULONG pts BL_REG(d1), UWORD c BL_REG(d2), UWORD c2 BL_REG(d3))

#define ABENTRY(e, n) \
    BLITZ_AB_ROUTINE(e, b_##n##_c, "   jmp     _a_" #n "_c")

ABENTRY(_floodfill, floodfill);
ABENTRY(_floodfill2, floodfill2);
ABENTRY(_remap, remap);
ABENTRY(_remap2, remap2);
ABENTRY(_polyf, polyf);
ABENTRY(_polyf2, polyf2);

/* ClipBlit: a3 = shape, a2 = bitmap, a1 = chip, d1/d2 = x/y, d3 = planes */
#define CBREGS WORD x BL_REG(d1), WORD y BL_REG(d2), UWORD excess BL_REG(d3), \
               ULONG r6 BL_REG(d6), ULONG r7 BL_REG(d7), UBYTE *chip BL_REG(a1), \
               struct bb_bitmap *bm BL_REG(a2)
#define SHAPE ((struct bb_shape *)blitz_string)

static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_docblit_c(CBREGS)
{
    BLITZ_EP_KEEP_A6();
    struct cblit cb;

    (void)excess;
    docblit(chip, SHAPE, bm, x, y, r6, r7, &cb);
    BLITZ_EP_KEEP_A6_END();
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

static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_docblitx_c(CBREGS)
{
    BLITZ_EP_KEEP_A6();

    docblitx(chip, SHAPE, bm, x, y, excess, r6, r7);
    BLITZ_EP_KEEP_A6_END();
}

static void BLITZ_ROUTINE a_docblitx_c(CBREGS, struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    docblitx(chip, SHAPE, bm, x, y, excess, r6, r7);
    BLITWAIT(chip);
    DisownBlitter();
}

void b_docblit(void), b_docblitx(void);
BLITZ_ALIAS_EP(b_docblit, b_docblit_c);
BLITZ_AB_ROUTINE(_doclipblit, b_docblit, "   jmp     _a_docblit_c");
BLITZ_ALIAS_EP(b_docblitx, b_docblitx_c);
BLITZ_AB_ROUTINE(_doclipblitx, b_docblitx, "   jmp     _a_docblitx_c");

/* _cbchk - TST.w (a3):BEQ err2 (shape not initialised); in the normal
   code (the library has no error area) */
static void BLITZ_ROUTINE _cbchk_c(void)
{
    if (!SHAPE->width)
        BLITZ_RUNERR(mess1);
}
BLITZ_THUNK(_cbchk, _cbchk_c, "d0-d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(polymess, "Poly needs at least 2 Points");
BLITZ_ERRCHK_MESSAGE(pointsmess, "Too Many Points");

/* _polychk - not in the original (#396): Poly with fewer than 2 points
   (d0), which would draw 65535 or 65536 lines from the words behind the
   coordinates */
static void BLITZ_ERRCHK _polychk_c(WORD n BL_REG(d0))
{
    if (n < 2)
        BLITZ_RUNERR(polymess);
}

/* _polyfchk - not in the original (#395): Polyf with more than POINTMAX
   points (d0), which overflow polypts */
static void BLITZ_ERRCHK _polyfchk_c(UWORD n BL_REG(d0))
{
    if (n > POINTMAX)
        BLITZ_RUNERR(pointsmess);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(_polychk, _polychk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_polyfchk, _polyfchk_c, "d0-d1/a0-a1");
