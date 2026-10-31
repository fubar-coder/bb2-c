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
 * Like medlib, the code is translated at register level: d0-d7 and
 * a0-a6 are static variables of this file, every instruction keeps its
 * operand size (MOVEM.w into data registers sign-extends to the long,
 * byte/word operations change only the low part, MULS/MULU/DIVS/DIVU as
 * on the 68000).  Lines, filled boxes, polygon fill, flood fill masks,
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
 *     side overflow the edge lists l0/l1 (#395).
 *   - AllocFill (statement) takes the bitmap from a3 = the string pointer. (#390)
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

/* ------------------------------------------------- register emulation */

static ULONG d0, d1, d2, d3, d4, d5, d6, d7;
static UBYTE *a0, *a1, *a2, *a3, *a4, *a5, *a6;

#define B(r)       ((UBYTE)(r))
#define SB(r)      ((BYTE)(r))
#define W(r)       ((UWORD)(r))
#define SW(r)      ((WORD)(r))
#define SETB(r, v) ((r) = ((r) & 0xffffff00UL) | (UBYTE)(v))
#define SETW(r, v) ((r) = ((r) & 0xffff0000UL) | (UWORD)(v))
#define EXTL(r)    ((ULONG)(LONG)(WORD)(r))
#define MULU(a, b) bl_mulu_w((UWORD)(a), (UWORD)(b))
#define MULS(a, b) ((ULONG)bl_muls_w((WORD)(a), (WORD)(b)))
#define SWAP(r)    ((ULONG)(r) << 16 | (ULONG)(r) >> 16)
#define MB(p, o)   (*(UBYTE *)((UBYTE *)(p) + (o)))
#define MW(p, o)   (*(UWORD *)((UBYTE *)(p) + (o)))
#define ML(p, o)   (*(ULONG *)((UBYTE *)(p) + (o)))
#define MP(p, o)   (*(UBYTE **)((UBYTE *)(p) + (o)))
#define MOVEMW(p, o) EXTL(MW(p, o))        /* MOVEM.w to a data register */
#define DIVS(r, s) ((r) = bl_divs_w((r), (WORD)(s)))
#define DIVU(r, s) ((r) = bl_divu_w((r), (UWORD)(s)))
#define EXG(x, y)  do { ULONG t_ = (x); (x) = (y); (y) = t_; } while (0)

/* chip registers (base a1 / a2 / a4) */
#define CW(cb, o) (*(volatile UWORD *)((UBYTE *)(cb) + (o)))
#define CL(cb, o) (*(volatile ULONG *)((UBYTE *)(cb) + (o)))
#define BUSY(cb)  (*(volatile UBYTE *)((UBYTE *)(cb) + 2) & 0x40)
#define BLITWAIT(cb) do { (void)BUSY(cb); while (BUSY(cb)) ; } while (0)

/* ------------------------------------------------------------ variables */

static UWORD pfc;
static ULONG polypts[32];
/* in the order of the original, so that an overflow of l0 runs into l1 */
static struct {
    ULONG bmwid, bmoff;
    ULONG pcolor;                       /* two words: colour, colour2 */
    UWORD rtclip;
    UWORD xyx[3];
    UWORD l0[32], l1[32];
} pv;
#define bmwid  pv.bmwid
#define bmoff  pv.bmoff
#define pcolor pv.pcolor
#define rtclip pv.rtclip
#define xyx    pv.xyx
#define l0     pv.l0
#define l1     pv.l1
static ULONG fbsize;
/* used by the assembler part (yofill): not static */
ULONG fbloc;
UWORD coords[2];
UWORD fcol;                             /* coords+4 */
UWORD widbytes;
UBYTE flags[2];
static UWORD finx;
static UWORD dpth;
static ULONG dx, dy, asq, bsq, asq2, bsq2;
static UWORD flag;
static UWORD modsize[2];
static UWORD bigsize[2];
static ULONG tempadd, blitpos;
static UWORD center[5] = { 0, 0, 0, 0, 1 };     /* center[4] = color */
#define color center[4]
static UWORD cpos[2];
static UWORD bcon0 = 0xfca;

static const UWORD lup1[16] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};
static const UWORD lup2[16] = {
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff,
};
#define m1 lup1
#define m2 lup2

static const char mess1[] = "SHAPE UNKNOWNST TO ME";

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
                                          UBYTE *bm BL_REG(a0))
{
    if (!MW(bm, 62))
        return;
    MW(bm, 48) = x;
    MW(bm, 50) = y;
}
BLITZ_THUNK(_bitmaporigin, _bitmaporigin_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitplanesbitmap_c(ULONG pick BL_REG(d2), UBYTE *src BL_REG(a0),
                                             UBYTE *dst BL_REG(a1))
{
    UWORD n = 0, i = 0;
    ULONG *srcplane = (ULONG *)(src + 8), *dstplane = (ULONG *)(dst + 8);

    if (!MW(src, 62) || MW(dst, 62))
        return;
    ML(dst, 0) = ML(src, 0);
    do {
        ULONG p = *srcplane++;

        if (pick & (1UL << (i & 31))) {
            n++;
            *dstplane++ = p;
        }
        i++;
    } while (i != MW(src, 4));
    blitz_string = (UBYTE *)dstplane;           /* a3 left behind as LEA 8(a1),a3 */
    MW(dst, 4) = n;
    if (!n)
        return;
    MW(dst, 52) = 0;
    ML(dst, 56) = ML(src, 56);
    MW(dst, 62) = 1;
}
BLITZ_THUNK(_bitplanesbitmap, _bitplanesbitmap_c, "d1/a0-a1");

static void BLITZ_ROUTINE _bitmapwindow_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                          UWORD w BL_REG(d4), UWORD h BL_REG(d5),
                                          UBYTE *src BL_REG(a0), UBYTE *dst BL_REG(a1))
{
    UWORD bpr, dep, n;
    ULONG off;
    ULONG *srcplane = (ULONG *)(src + 8), *dstplane = (ULONG *)(dst + 8);

    if (!MW(src, 62) || MW(dst, 62))
        return;
    bpr = MW(src, 0);
    dep = MW(src, 4);
    x = (x >> 4) * 2;                           /* round to nearest word */
    w = (w >> 4) * 2;
    off = (ULONG)y * bpr + (LONG)(WORD)x;
    MW(dst, 0) = bpr;
    MW(dst, 2) = h;
    MW(dst, 4) = dep;
    n = dep - 1;
    do
        *dstplane++ = *srcplane++ + off;
    while (n-- != 0);
    blitz_string = (UBYTE *)dstplane;           /* a3 left behind */
    MW(dst, 52) = MW(src, 52);
    MB(dst, 53) |= 1;
    MW(dst, 54) = w;
    MW(dst, 56) = w << 3;
    MW(dst, 58) = h;
    MW(dst, 60) = MW(src, 60);
    MW(dst, 62) = 1;
}
BLITZ_THUNK(_bitmapwindow, _bitmapwindow_c, "d1/a0-a1");

/* ---------------------------------------------------- fill buffer */

static void BLITZ_ROUTINE _init_c(void)
{
    fbloc = 0;
    fbsize = 0;
}
BLITZ_THUNK(_init, _init_c, "d0-d1/a0-a1");

/* _allocfill - a3 = bitmap: buffer of width/8 * height bytes (chip),
   d7 = buffer */
static void allocfill(void)
{
    d7 = MULU((UWORD)(MW(a3, 56) >> 3), MW(a3, 58));
    if (!((LONG)(d7 - fbsize) <= 0)) {          /* BMI/BEQ yokay */
        if (fbsize)
            memfree(fbloc, fbsize);
        fbsize = d7;
        fbloc = memalloc(d7, MEMF_CHIP);
    }
    d7 = fbloc;
}

static void BLITZ_ROUTINE _allocfill_c(void)
{
    a3 = blitz_string;                          /* BUG(orig) (#390): no bitmap here */
    allocfill();
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

/* clipme - line d0,d1 - d2,d3 in colour `color` on a3, a2 = chip */
static void clipme(void)
{
    d4 = MOVEMW(a3, 48);
    d5 = MOVEMW(a3, 50);
    SETW(d0, d0 + d4);
    SETW(d2, d2 + d4);
    SETW(d1, d1 + d5);
    SETW(d3, d3 + d5);
    d4 = MOVEMW(a3, 56);
    d5 = MOVEMW(a3, 58);
    if (SW(d2 - d0) < 0) {
        EXG(d0, d2);
        EXG(d1, d3);
    }
    if (SW(d2) < 0)
        return;
    if (SW(d0 - d4) >= 0)
        return;
    if (SW(d2 - d4) >= 0) {                     /* clip right */
        SETW(d7, d2);
        SETW(d7, d7 - d4);
        SETW(d7, d7 + 1);
        SETW(d6, d3);
        SETW(d6, d6 - d1);
        d7 = MULS(d7, d6);
        SETW(d6, d2);
        SETW(d6, d6 - d0);
        DIVS(d7, d6);
        SETW(d3, d3 - d7);
        SETW(d2, d4);
        SETW(d2, d2 - 1);
    }
    if (SW(d0) < 0) {                           /* clip left */
        SETW(d7, d0);
        SETW(d6, d3);
        SETW(d6, d6 - d1);
        d7 = MULS(d7, d6);
        SETW(d6, d2);
        SETW(d6, d6 - d0);
        DIVS(d7, d6);
        SETW(d1, d1 - d7);
        SETW(d0, 0);
    }
    if (SW(d3 - d1) < 0) {
        EXG(d0, d2);
        EXG(d1, d3);
    }
    if (SW(d3) < 0)
        return;
    if (SW(d1 - d5) >= 0)
        return;
    if (SW(d3 - d5) >= 0) {                     /* clip bottom */
        SETW(d7, d3);
        SETW(d7, d7 - d5);
        SETW(d7, d7 + 1);
        SETW(d6, d2);
        SETW(d6, d6 - d0);
        d7 = MULS(d7, d6);
        SETW(d6, d3);
        SETW(d6, d6 - d1);
        DIVS(d7, d6);
        SETW(d2, d2 - d7);
        SETW(d3, d5);
        SETW(d3, d3 - 1);
    }
    if (SW(d1) < 0) {                           /* clip top */
        SETW(d7, d1);
        SETW(d6, d2);
        SETW(d6, d6 - d0);
        d7 = MULS(d7, d6);
        SETW(d6, d3);
        SETW(d6, d6 - d1);
        DIVS(d7, d6);
        SETW(d0, d0 - d7);
        SETW(d1, 0);
    }
    /* noclip: CalcOctant */
    SETW(d2, d2 - d0);
    if (SW(d2) < 0) {                           /* xneg */
        SETW(d2, -SW(d2));
        SETW(d3, d3 - d1);
        SETW(d7, d3);
        if (SW(d3) < 0) {                       /* xyneg */
            SETW(d3, -SW(d3));
            if (SW(d2 - d3) < 0) {
                EXG(d2, d3);
                d5 = 12 + 1;
            } else
                d5 = 28 + 1;
        } else if (SW(d2 - d3) < 0) {
            EXG(d2, d3);
            d5 = 8 + 1;
        } else
            d5 = 20 + 1;
    } else {
        SETW(d3, d3 - d1);
        SETW(d7, d3);
        if (SW(d3) < 0) {                       /* yneg */
            SETW(d3, -SW(d3));
            if (SW(d2 - d3) < 0) {
                EXG(d2, d3);
                d5 = 4 + 1;
            } else
                d5 = 24 + 1;
        } else if (SW(d2 - d3) < 0) {
            EXG(d2, d3);
            d5 = 0 + 1;
        } else
            d5 = 16 + 1;
    }
    d1 = MULU(d1, MW(a3, 0));
    a1 = (UBYTE *)d1;
    d0 = EXTL(d0);
    d0 = d0 >> 4 | d0 << 28;                    /* ROR.l #4 */
    a1 += SW(d0);
    a1 += SW(d0);
    d0 = SWAP(d0);
    SETW(d3, d3 << 2);
    SETW(d2, d2 + d2);
    SETW(d4, d2);
    d4 = EXTL(d4);
    d4 <<= 5;
    d1 = d4;
    d1 = SWAP(d1);
    SETW(d4, d4 + 0x42);
    BLITWAIT(a2);
    CW(a2, 0x62) = d3;                          /* bmod */
    SETW(d3, d3 - d2);
    d3 = EXTL(d3);
    d7 = d3;
    if ((LONG)d3 < 0)
        SETW(d5, d5 | 64);
    SETW(d3, d3 - d2);                          /* love */
    CW(a2, 0x64) = d3;                          /* amod */
    CW(a2, 0x60) = MW(a3, 0);
    CW(a2, 0x66) = MW(a3, 0);
    CL(a2, 0x44) = 0xffffffff;
    dpth = MW(a3, 4);
    a3 += 8;
    d6 = 0;
    if (SW(color) >= 0) {
        do {                                    /* nxplane */
            a0 = MP(a3, 0) + (ULONG)a1;
            a3 += 4;
            SETW(d0, d0 | 0xbfa);
            if (!((UBYTE)color & (1 << (d6 & 7))))
                SETB(d0, d0 ^ 0xf0);
            BLITWAIT(a2);                       /* colon */
            CW(a2, 0x74) = 0x8000;
            CL(a2, 0x50) = d7;
            CL(a2, 0x48) = (ULONG)a0;
            CL(a2, 0x54) = (ULONG)a0;
            CW(a2, 0x40) = d0;
            CW(a2, 0x42) = d5;
            if (W(d1)) {                        /* doextend1 */
                UWORD n = W(d1) - 1;

                do {
                    CW(a2, 0x58) = 2;
                    BLITWAIT(a2);
                } while (n-- != 0);
            }
            CW(a2, 0x58) = d4;                  /* dunex1 */
            SETB(d6, d6 + 1);
        } while (B(d6) != (UBYTE)dpth);
        return;
    }
    do {                                        /* nxplanus */
        a0 = MP(a3, 0) + (ULONG)a1;
        a3 += 4;
        SETW(d0, d0 | 0xb5a);
        BLITWAIT(a2);
        CW(a2, 0x74) = 0x8000;
        CL(a2, 0x50) = d7;
        CL(a2, 0x48) = (ULONG)a0;
        CL(a2, 0x54) = (ULONG)a0;
        CW(a2, 0x40) = d0;
        CW(a2, 0x42) = d5;
        if (W(d1)) {
            UWORD n = W(d1) - 1;

            do {
                CW(a2, 0x58) = 2;
                BLITWAIT(a2);
            } while (n-- != 0);
        }
        CW(a2, 0x58) = d4;
        SETB(d6, d6 + 1);
    } while (B(d6) != (UBYTE)dpth);
}

static void bline(void)
{
    color = d4;
    cpos[0] = d2;
    cpos[1] = d3;
    clipme();
}

static void blineto(void)
{
    color = d2;
    d2 = MOVEMW(cpos, 0);
    d3 = MOVEMW(cpos, 2);
    cpos[0] = d0;
    cpos[1] = d1;
    clipme();
}

static void blinew(void)
{
    UWORD saved[6];                             /* d0-d5 */
    int i;

    do {
        saved[0] = d0; saved[1] = d1; saved[2] = d2;
        saved[3] = d3; saved[4] = d4; saved[5] = d5;
        for (i = 0; i < 4; i++) {
            UBYTE *sa3 = a3;

            if (i)
                d0 = EXTL(saved[0]), d1 = EXTL(saved[1]), d2 = EXTL(saved[2]),
                d3 = EXTL(saved[3]), d4 = EXTL(saved[4]), d5 = EXTL(saved[5]);
            switch (i) {
            case 0: SETW(d0, d0 + d5); break;
            case 1: SETW(d1, d1 + d5); break;
            case 2: SETW(d2, d2 + d5); break;
            case 3: SETW(d3, d3 + d5); break;
            }
            bline();
            a3 = sa3;
        }
        d0 = EXTL(saved[0]); d1 = EXTL(saved[1]); d2 = EXTL(saved[2]);
        d3 = EXTL(saved[3]); d4 = EXTL(saved[4]); d5 = EXTL(saved[5]);
        SETW(d5, d5 - 1);
    } while (W(d5) != 0xffff);                  /* DBRA d5,_blinew */
}

/* ----------------------------------------------------------- Box, Boxf */

static void boxo(void)
{
    UBYTE *sa4 = a4;

    d5 = MOVEMW(a3, 48);
    d6 = MOVEMW(a3, 50);
    SETW(d0, d0 + d5);
    SETW(d2, d2 + d5);
    SETW(d1, d1 + d6);
    SETW(d3, d3 + d6);
    if (SW(d2 - d0) < 0)
        EXG(d0, d2);
    if (SW(d3 - d1) < 0)
        EXG(d1, d3);
    center[0] = d0; center[1] = d1; center[2] = d2; center[3] = d3; center[4] = d4;
    a4 = a3;
    SETW(d2, d0);
    SETW(d3, d3 - 1);
    a3 = a4;
    clipme();
    d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2); d2 = MOVEMW(center, 4); d3 = MOVEMW(center, 6);
    SETW(d0, d2);
    SETW(d1, d1 + 1);
    a3 = a4;
    clipme();
    d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2); d2 = MOVEMW(center, 4); d3 = MOVEMW(center, 6);
    SETW(d3, d1);
    SETW(d0, d0 + 1);
    a3 = a4;
    clipme();
    d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2); d2 = MOVEMW(center, 4); d3 = MOVEMW(center, 6);
    SETW(d1, d3);
    SETW(d2, d2 - 1);
    a3 = a4;
    clipme();
    a4 = sa4;
}

/* dobof - filled rectangle d0,d1-d2,d3 (clipped), colour d4 (long) */
static void dobof(void)
{
    BLITWAIT(a2);
    SETW(d5, d0);
    SETW(d5, d5 & 15);
    SETW(d5, d5 + d5);
    CW(a2, 0x44) = m1[W(d5) >> 1];
    SETW(d5, d2);
    SETW(d5, d5 & 15);
    SETW(d5, d5 + d5);
    CW(a2, 0x46) = m2[W(d5) >> 1];
    CW(a2, 0x74) = 0xffff;
    CW(a2, 0x42) = 0;
    SETW(d3, d3 - d1);
    SETW(d3, d3 + 1);
    d3 = EXTL(d3);
    d3 <<= 6;
    d1 = MULU(d1, MW(a3, 0));
    SETW(d0, W(d0) >> 4);
    d0 = EXTL(d0);
    d1 += d0;
    d1 += d0;
    SETW(d2, W(d2) >> 4);
    SETW(d2, d2 - d0);
    SETW(d2, d2 + 1);
    SETW(d3, d3 | W(d2));
    SETW(d6, MW(a3, 0));
    SETW(d6, d6 - d2);
    SETW(d6, d6 - d2);
    CW(a2, 0x66) = d6;
    CW(a2, 0x60) = d6;
    d7 = 0;
    SETW(d2, MW(a3, 4));
    a3 += 8;
    d5 = SWAP(d3);
    if (!W(d5)) {
        if (SW(d4) >= 0) {
            do {                                /* nxp1 */
                BLITWAIT(a2);
                d0 = ML(a3, 0) + d1;
                a3 += 4;
                CL(a2, 0x48) = d0;
                CL(a2, 0x54) = d0;
                CW(a2, 0x40) = 0x3fa;
                if (!(d4 & (1UL << (d7 & 31))))
                    CW(a2, 0x40) = 0x30a;
                CW(a2, 0x58) = d3;
                SETW(d7, d7 + 1);
            } while (W(d7) != W(d2));
            return;
        }
        do {                                    /* nxp2 */
            BLITWAIT(a2);
            d0 = ML(a3, 0);
            a3 += 4;
            if (d4 & (1UL << (d7 & 31))) {
                d0 += d1;
                CL(a2, 0x48) = d0;
                CL(a2, 0x54) = d0;
                CW(a2, 0x40) = 0x35a;
                CW(a2, 0x58) = d3;
            }
            SETW(d7, d7 + 1);
        } while (W(d7) != W(d2));
        return;
    }
    /* longun: more than 1023 lines - blit the extra 1024 line parts */
    do {
        UWORD s3 = d3, s5 = d5, n;

        BLITWAIT(a2);
        d0 = ML(a3, 0);
        a3 += 4;
        if (SW(d4) >= 0) {
            d0 += d1;
            CL(a2, 0x48) = d0;
            CL(a2, 0x54) = d0;
            CW(a2, 0x40) = 0x3fa;
            if (!(d4 & (1UL << (d7 & 31))))
                CW(a2, 0x40) = 0x30a;
        } else {
            if (!(d4 & (1UL << (d7 & 31))))
                goto lmissp;
            d0 += d1;
            CL(a2, 0x48) = d0;
            CL(a2, 0x54) = d0;
            CW(a2, 0x40) = 0x35a;
        }
        SETW(d3, d3 & 63);
        n = W(d5) - 1;
        do {
            CW(a2, 0x58) = d3;
            BLITWAIT(a2);
        } while (n-- != 0);
        d3 = EXTL(s3);                          /* MOVEM (a7)+,d3/d5 */
        d5 = EXTL(s5);
        CW(a2, 0x58) = d3;
lmissp:
        SETW(d7, d7 + 1);
    } while (W(d7) != W(d2));
}

static void boxfo(void)
{
    d5 = MOVEMW(a3, 48);
    d6 = MOVEMW(a3, 50);
    SETW(d0, d0 + d5);
    SETW(d2, d2 + d5);
    SETW(d1, d1 + d6);
    SETW(d3, d3 + d6);
    if (SW(d2 - d0) < 0)
        EXG(d0, d2);
    if (SW(d3 - d1) < 0)
        EXG(d1, d3);
    d6 = MOVEMW(a3, 56);
    d7 = MOVEMW(a3, 58);
    SETW(d6, d6 - 1);
    SETW(d7, d7 - 1);
    if (SW(d0) < 0)
        SETW(d0, 0);
    if (SW(d1) < 0)
        SETW(d1, 0);
    if (SW(d2) < 0 || SW(d3) < 0)
        return;
    if (SW(d0) >= SW(d6) || SW(d1) >= SW(d7))   /* BGE nobx */
        return;
    if (!(SW(d2) < SW(d6)))                     /* BLT */
        SETW(d2, d6);
    if (!(SW(d3) < SW(d7)))
        SETW(d3, d7);
    for (;;) {                                  /* noclip2 */
        SETW(d6, d2);
        SETW(d6, d6 - d0);
        if (SW(d6 - 991) < 0)
            break;
        {
            ULONG s0 = d0, s1 = d1, s2 = d2, s3 = d3, s4 = d4;
            UBYTE *sa3 = a3;

            SETW(d2, d0);
            SETW(d2, d2 + 989);
            dobof();
            d0 = s0; d1 = s1; d2 = s2; d3 = s3; d4 = s4;
            a3 = sa3;
        }
        SETW(d0, d0 + 990);
    }
    dobof();
}

/* -------------------------------------------------------------- ellipses */

static void calcellipse(void)
{
    SETW(d7, d3);                               /* d7 = y = b */
    d2 = MULU(d2, d2);
    asq = d2;
    d2 <<= 1;
    asq2 = d2;
    d3 = MULU(d3, d3);
    bsq = d3;
    d3 <<= 1;
    bsq2 = d3;
    d5 = bsq;
    d3 = asq;
    SETW(d6, d3);                               /* 32x16 multiply asq*b */
    d6 = MULU(d6, d7);
    d3 = SWAP(d3);
    d3 = MULU(d3, d7);
    d3 = SWAP(d3);
    SETW(d3, 0);
    d3 += d6;
    d5 -= d3;
    d3 = asq;
    d3 = (ULONG)((LONG)d3 >> 2);
    d5 += d3;
    d0 = 0;
    dx = d0;
    d3 = asq2;
    SETW(d6, d3);
    d6 = MULU(d6, d7);
    d3 = SWAP(d3);
    d3 = MULU(d3, d7);
    d3 = SWAP(d3);
    SETW(d3, 0);
    d3 += d6;
    dy = d3;
    d6 = 0;
}

/* cplot - CPU plot at d0,d1 in colour d4 (complement if negative) */
static void cplot(void)
{
    if (SW(d1 - MW(a3, 58)) >= 0)
        return;
    d1 = MULU(d1, MW(a3, 0));
    if ((LONG)d1 < 0)
        return;
    SETW(d3, d0);
    if (SW(d3) < 0)
        return;
    if (SW(d3 - MW(a3, 56)) >= 0)
        return;
    SETW(d3, W(d3) >> 3);
    d3 = EXTL(d3);
    d1 += d3;
    SETW(d0, d0 & 7);
    SETW(d0, d0 ^ 7);
    a2 = a3 + 8;
    d3 = 0;
    if (SW(d4) < 0) {
        do {                                    /* nxpm */
            a0 = MP(a2, 0);
            a2 += 4;
            MB(a0, d1) ^= 1 << (d0 & 7);
            SETW(d3, d3 + 1);
        } while (B(d3) != MB(a3, 5));
        return;
    }
    do {                                        /* nxpp */
        a0 = MP(a2, 0);
        a2 += 4;
        if (d4 & (1UL << (d3 & 31)))
            MB(a0, d1) |= 1 << (d0 & 7);
        else
            MB(a0, d1) &= ~(1 << (d0 & 7));
        SETW(d3, d3 + 1);
    } while (B(d3) != MB(a3, 5));
}

/* cplot2 - toggle a bit in the fill buffer a0, x clipped at d4 */
static void cplot2(void)
{
    if (SW(d1 - MW(a3, 58)) >= 0)
        return;
    d1 = MULU(d1, MW(a3, 0));
    if ((LONG)d1 < 0)
        return;
    SETW(d3, d0);
    if (SW(d3) < 0)
        return;
    if (!(SW(d3 - d4) < 0)) {
        SETW(d3, d4);
        SETW(d0, d4);
    }
    SETW(d3, W(d3) >> 3);
    d3 = EXTL(d3);
    d1 += d3;
    SETW(d0, d0 & 7);
    SETW(d0, d0 ^ 7);
    MB(a0, d1) ^= 1 << (d0 & 7);
}

static void ellipse(void)
{
    flag = 0;
    SETW(d0, d0 + MW(a3, 48));
    SETW(d1, d1 + MW(a3, 50));
    center[0] = d0; center[1] = d1; center[2] = d2; center[3] = d3; center[4] = d4;
    calcellipse();
    for (;;) {
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);         /* nxelp */
        SETW(d0, d0 + d6);
        SETW(d1, d1 + d7);
        cplot();
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
        SETW(d0, d0 + d6);
        SETW(d1, d1 - d7);
        cplot();
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
        SETW(d0, d0 - d6);
        SETW(d1, d1 + d7);
        cplot();
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
        SETW(d0, d0 - d6);
        SETW(d1, d1 - d7);
        cplot();
        d0 = dx; d1 = dy; d2 = asq; d3 = bsq;
        if (!flag) {
            if (!((LONG)(d0 - d1) >= 0)) {
                if ((LONG)d5 > 0) {
                    SETW(d7, d7 - 1);
                    d1 -= asq2;
                    d5 -= d1;
                }
                SETW(d6, d6 + 1);               /* nodab */
                d0 += bsq2;
                d5 += d0;
                d5 += bsq;
                dx = d0; dy = d1;
                continue;
            }
            flag = ~flag;                       /* dxgtdy */
            d2 -= d3;
            d3 = d2;
            d2 <<= 1;
            d2 += d3;
            d2 = (ULONG)((LONG)d2 >> 1);
            d2 -= d0;
            d2 -= d1;
            d2 = (ULONG)((LONG)d2 >> 1);
            d5 += d2;
        }
        if (SW(d7) <= 0)                        /* dooly */
            return;
        if ((LONG)d5 < 0) {
            SETW(d6, d6 + 1);
            d0 += bsq2;
            d5 += d0;
        }
        SETW(d7, d7 - 1);                       /* dodab */
        d1 -= asq2;
        d5 += asq;
        d5 -= d1;
        dx = d0; dy = d1;
    }
}

static void BLITZ_ROUTINE _ellipse_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1), UWORD rx BL_REG(d2),
                                     UWORD ry BL_REG(d3), UWORD c BL_REG(d4))
{
    a3 = blitz_string;
    d0 = x; d1 = y; d2 = rx; d3 = ry; d4 = c;
    ellipse();
}
BLITZ_THUNK(_ellipse, _ellipse_c, "d1/a0-a1");

static void BLITZ_ROUTINE _circle_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1), UWORD r BL_REG(d2),
                                    UWORD c BL_REG(d3))
{
    a3 = blitz_string;
    d0 = x; d1 = y; d2 = r; d3 = r; d4 = c;     /* MOVE.w d3,d4:MOVE.w d2,d3 */
    ellipse();
}
BLITZ_THUNK(_circle, _circle_c, "d1/a0-a1");

/* bigsize (BLTSIZV/BLTSIZH) or a normal BLTSIZE */
static void bltsize(UWORD size)
{
    if (size)
        CW(a2, 0x58) = size;
    else
        CL(a2, 0x5c) = *(ULONG *)bigsize;
}

static void ellipsefo(void)
{
    UBYTE *sa4 = a4, *sa5 = a5;

    allocfill();
    flag = 0;
    center[0] = d0; center[1] = d1; center[2] = d2; center[3] = d3; center[4] = d4;
    SETW(d6, d0);
    SETW(d7, d1);
    d4 = 1;
    SETW(d2, d2 + d4);                          /* bigger radius */
    SETW(d3, d3 + d4);
    SETW(d0, d0 - d2);
    if (SW(d0) < 0)
        SETW(d0, 0);
    SETW(d1, d1 - d3);
    if (SW(d1) < 0)
        SETW(d1, 0);
    SETW(d6, d6 + d2);
    if (SW(d6) < 0)
        goto pa;
    SETW(d7, d7 + d3);
    if (SW(d7) < 0)
        goto pa;
    d4 = 4;
    SETW(d0, W(d0) >> 4);
    SETW(d6, W(d6) >> 4);
    d4 = MOVEMW(a3, 56);
    d5 = MOVEMW(a3, 58);
    SETW(d4, W(d4) >> 4);
    if (SW(d0 - d4) >= 0 || SW(d1 - d5) >= 0)
        goto pa;
    if (!(SW(d6 - d4) < 0)) {
        SETW(d6, d4);
        SETW(d6, d6 - 1);
    }
    if (!(SW(d7 - d5) < 0)) {
        SETW(d7, d5);
        SETW(d7, d7 - 1);
    }
    SETW(d1, d1 - d7);                          /* bboc2 */
    SETW(d1, -SW(d1));
    SETW(d1, d1 + 1);
    SETW(d0, d0 - d6);
    SETW(d0, -SW(d0));
    SETW(d0, d0 + 1);
    if (SW(d1 - 1024) >= 0 || !(SW(d0 - 64) < 0)) {     /* bigo */
        bigsize[0] = d1;
        bigsize[1] = d0;
        d1 = 0;
    } else {
        SETW(d1, d1 << 6);
        SETW(d1, d1 | W(d0));
    }
    SETW(d0, d0 << 1);                          /* domod */
    SETW(d0, d0 - MW(a3, 0));
    SETW(d0, -SW(d0));
    d7 = MULU(d7, MW(a3, 0));
    a0 = (UBYTE *)d7;
    a0 += SW(d6);
    a0 += SW(d6);
    tempadd = (ULONG)a0;
    a0 += fbloc;
    blitpos = (ULONG)a0;
    modsize[0] = d0;
    modsize[1] = d1;
    BLITWAIT(a2);                               /* clear rectangle */
    CL(a2, 0x44) = 0xffffffff;
    CL(a2, 0x54) = (ULONG)a0;
    CW(a2, 0x66) = d0;
    CL(a2, 0x40) = 0x01000002;
    bltsize(d1);
    BLITWAIT(a2);
    d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2); d2 = MOVEMW(center, 4); d3 = MOVEMW(center, 6);
    calcellipse();
    a0 = (UBYTE *)fbloc;
    SETW(d4, MW(a3, 56));
    SETW(d4, d4 - 1);
    for (;;) {
nxelp2:
        if (W(d7)) {
            d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
            SETW(d0, d0 + d6);
            SETW(d1, d1 + d7);
            cplot2();
            d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
            SETW(d0, d0 - d6);
            SETW(d1, d1 + d7);
            cplot2();
        }
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);         /* j1e */
        SETW(d0, d0 + d6);
        SETW(d1, d1 - d7);
        cplot2();
        d0 = MOVEMW(center, 0); d1 = MOVEMW(center, 2);
        SETW(d0, d0 - d6);
        SETW(d1, d1 - d7);
        cplot2();
        for (;;) {                              /* flop9 */
            d0 = dx; d1 = dy; d2 = asq; d3 = bsq;
            if (flag)
                break;
            if ((LONG)(d0 - d1) >= 0) {         /* dxgtdy2 */
                flag = ~flag;
                d2 -= d3;
                d3 = d2;
                d2 <<= 1;
                d2 += d3;
                d2 = (ULONG)((LONG)d2 >> 1);
                d2 -= d0;
                d2 -= d1;
                d2 = (ULONG)((LONG)d2 >> 1);
                d5 += d2;
                break;
            }
            if ((LONG)d5 > 0) {
                SETW(d7, d7 - 1);
                d1 -= asq2;
                d5 -= d1;
                SETW(d6, d6 + 1);
                d0 += bsq2;
                d5 += d0;
                d5 += bsq;
                dx = d0; dy = d1;
                goto nxelp2;
            }
            SETW(d6, d6 + 1);                   /* nodab2 */
            d0 += bsq2;
            d5 += d0;
            d5 += bsq;
            dx = d0; dy = d1;
        }
        if (!W(d7))                             /* dooly2 */
            break;
        if ((LONG)d5 < 0) {
            SETW(d6, d6 + 1);
            d0 += bsq2;
            d5 += d0;
        }
        SETW(d7, d7 - 1);                       /* dodab2 */
        d1 -= asq2;
        d5 += asq;
        d5 -= d1;
        dx = d0; dy = d1;
    }
    /* DoFill */
    a0 = (UBYTE *)blitpos;
    d0 = MOVEMW(modsize, 0);
    d1 = MOVEMW(modsize, 2);
    BLITWAIT(a2);
    CL(a2, 0x50) = (ULONG)a0;
    CL(a2, 0x54) = (ULONG)a0;
    CW(a2, 0x64) = d0;
    CW(a2, 0x66) = d0;
    CL(a2, 0x40) = 0x09f0000a;
    bltsize(d1);
    /* DoTransfer */
    a0 = (UBYTE *)blitpos;
    d0 = MOVEMW(modsize, 0);
    d1 = MOVEMW(modsize, 2);
    d3 = 0xffffffff;
    SETW(d3, d3 + MW(a3, 4));
    a3 += 8;
    d6 = tempadd;
    SETW(d4, color);
    if (SW(d4) >= 0) {
        do {                                    /* nxplaner */
            a1 = MP(a3, 0) + d6;
            a3 += 4;
            BLITWAIT(a2);
            CL(a2, 0x50) = (ULONG)a0;
            CL(a2, 0x4c) = (ULONG)a1;
            CL(a2, 0x54) = (ULONG)a1;
            CW(a2, 0x64) = d0;
            CW(a2, 0x62) = d0;
            CW(a2, 0x66) = d0;
            CL(a2, 0x40) = 0x0dfc0002;
            {
                int c = d4 & 1;

                SETW(d4, W(d4) >> 1);
                if (!c)
                    CW(a2, 0x40) = 0x0d0c;
            }
            bltsize(d1);
            SETW(d3, d3 - 1);
        } while (W(d3) != 0xffff);
    } else {
        do {                                    /* xort */
            a1 = MP(a3, 0) + d6;
            a3 += 4;
            BLITWAIT(a2);
            CL(a2, 0x50) = (ULONG)a0;
            CL(a2, 0x4c) = (ULONG)a1;
            CL(a2, 0x54) = (ULONG)a1;
            CW(a2, 0x64) = d0;
            CW(a2, 0x62) = d0;
            CW(a2, 0x66) = d0;
            CL(a2, 0x40) = 0x0d3c0002;
            bltsize(d1);
            SETW(d3, d3 - 1);
        } while (W(d3) != 0xffff);
    }
pa:
    a4 = sa4;
    a5 = sa5;
}

/* --------------------------------------------- mask fetch / write (ReMap) */

/* column setup common to monofetch/monowrite */
static void monosetup(void)
{
    SETW(d7, MW(a3, 4));
    d5 = MOVEMW(a3, 56);
    d6 = MOVEMW(a3, 58);
    d6 <<= 6;
    SETW(d5, W(d5) >> 4);
    SETW(d2, d5);
    SETW(d2, d2 & 0x3fc0);
    SETW(d2, d2 + d2);
    d3 = SWAP(d6);
    if (!W(d6))
        SETW(d3, d3 - 1);
    SETW(d5, d5 & 63);
    if (!W(d5))
        SETW(d2, d2 - 128);
    SETW(d6, d6 | W(d5));
    a4 = a3 + 8;
    SETW(d7, d7 - 1);
    d5 = 0;
}

static void setmods(UWORD m)
{
    CW(a2, 0x60) = m;
    CW(a2, 0x62) = m;
    CW(a2, 0x64) = m;
    CW(a2, 0x66) = m;
}

/* the extra 1024 line parts (d3 = count) and the final size */
static void blitsize3(void)
{
    if (W(d3)) {
        UWORD s6 = d6;

        SETW(d4, d3 - 1);
        SETW(d6, d6 & 63);
        do {
            CW(a2, 0x58) = d6;
            BLITWAIT(a2);
            SETW(d4, d4 - 1);
        } while (W(d4) != 0xffff);              /* DBRA d4 */
        SETW(d6, s6);
    }
    CW(a2, 0x58) = d6;
}

/* dunxx1 - all planes of one 64 word column: mask of colour d0 to a5 */
static void dunxx1(void)
{
    CL(a2, 0x40) = 0x0bf00000;
    if (!(d0 & (1UL << (d5 & 31))))
        CW(a2, 0x40) = 0x0b0f;
    a0 = MP(a4, 0) + SW(d2);
    a4 += 4;
    CL(a2, 0x50) = (ULONG)a0;
    CL(a2, 0x48) = (ULONG)a5;
    CL(a2, 0x54) = (ULONG)a5;
    blitsize3();
    goto poo1;
nxpoo1:
    BLITWAIT(a2);
    CW(a2, 0x40) = 0x0ba0;
    if (!(d0 & (1UL << (d5 & 31))))
        CW(a2, 0x40) = 0x0b0a;
    a0 = MP(a4, 0) + SW(d2);
    a4 += 4;
    CL(a2, 0x50) = (ULONG)a0;
    CL(a2, 0x48) = (ULONG)a5;
    CL(a2, 0x54) = (ULONG)a5;
    blitsize3();
poo1:
    SETW(d5, d5 + 1);
    SETW(d7, d7 - 1);
    if (W(d7) != 0xffff)
        goto nxpoo1;
}

static void monofetch(void)
{
    allocfill();
    monosetup();
    a5 = (UBYTE *)fbloc;
    BLITWAIT(a2);
    CL(a2, 0x44) = 0xffffffff;
    CL(a2, 0x60) = 0;
    CL(a2, 0x64) = 0;
    finx = d2;
    if (W(d2)) {
        UWORD s6;

        d2 = 0;
        s6 = d6;
        SETW(d6, d6 & 0xffc0);
        SETW(d4, MW(a3, 0));
        SETW(d4, d4 - 128);
        setmods(d4);
        do {                                    /* nxxx1 */
            ULONG s5 = d5, s7 = d7;
            UBYTE *sa4 = a4;

            dunxx1();
            d5 = s5; d7 = s7; a4 = sa4;
            BLITWAIT(a2);
            SETW(d2, d2 + 128);
            a5 += 128;
        } while (W(d2) != finx);
        SETW(d6, s6);
        SETW(d4, MW(a3, 0));
        SETW(d4, d4 & 127);
        if (!W(d4))
            SETW(d4, 128);
        SETW(d4, d4 - MW(a3, 0));
        SETW(d4, -SW(d4));
        setmods(d4);
    }
    dunxx1();
}

/* dunxx2 - all planes: colour d1 from the mask a5 */
static void dunxx2(void)
{
    CL(a2, 0x40) = 0x0bfa0000;
    if (!(d1 & (1UL << (d5 & 31))))
        CW(a2, 0x40) = 0x0b0a;
    CL(a2, 0x50) = (ULONG)a5;
    a0 = MP(a4, 0) + SW(d2);
    a4 += 4;
    CL(a2, 0x48) = (ULONG)a0;
    CL(a2, 0x54) = (ULONG)a0;
    blitsize3();
    goto poo2;
nxpoo2:
    BLITWAIT(a2);
    CW(a2, 0x40) = 0x0bfa;
    if (!(d1 & (1UL << (d5 & 31))))
        CW(a2, 0x40) = 0x0b0a;
    CL(a2, 0x50) = (ULONG)a5;
    a0 = MP(a4, 0) + SW(d2);
    a4 += 4;
    CL(a2, 0x48) = (ULONG)a0;
    CL(a2, 0x54) = (ULONG)a0;
    blitsize3();
poo2:
    SETW(d5, d5 + 1);
    SETW(d7, d7 - 1);
    if (W(d7) != 0xffff)
        goto nxpoo2;
}

static void monowrite(void)
{
    a5 = (UBYTE *)fbloc;
    monosetup();
    BLITWAIT(a2);
    CL(a2, 0x44) = 0xffffffff;
    CL(a2, 0x60) = 0;
    CL(a2, 0x64) = 0;
    finx = d2;
    if (W(d2)) {
        UWORD s6;

        d2 = 0;
        s6 = d6;
        SETW(d6, d6 & 0xffc0);
        SETW(d4, MW(a3, 0));
        SETW(d4, d4 - 128);
        setmods(d4);
        do {
            ULONG s5 = d5, s7 = d7;
            UBYTE *sa4 = a4;

            dunxx2();
            d5 = s5; d7 = s7; a4 = sa4;
            BLITWAIT(a2);
            SETW(d2, d2 + 128);
            a5 += 128;
        } while (W(d2) != finx);
        SETW(d6, s6);
        SETW(d4, MW(a3, 0));
        SETW(d4, d4 & 127);
        if (!W(d4))
            SETW(d4, 128);
        SETW(d4, d4 - MW(a3, 0));
        SETW(d4, -SW(d4));
        setmods(d4);
    }
    dunxx2();
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

/* yofill with the bitmap in a3 (static) */
static void yofill(void)
{
    register UBYTE *r_a0 __asm("a0") = a3;

    __asm volatile("jsr     _yofill_asm"
                   : "+a"(r_a0) : : "d0", "d1", "a1", "cc", "memory");
}

static void floodfill(void)
{
    UBYTE *sa4 = a4, *sa5 = a5;

    coords[0] = d0;
    coords[1] = d1;
    fcol = d2;
    d1 = MULU(d1, MW(a3, 0));
    SETW(d2, d0);
    SETW(d2, ~d2);
    SETW(d0, W(d0) >> 3);
    d0 = EXTL(d0);
    d1 += d0;
    SETW(d7, MW(a3, 4));
    d6 = 0;
    d0 = 0;
    a4 = a3 + 8;
    do {                                        /* colour under the point */
        a0 = MP(a4, 0);
        a4 += 4;
        if (MB(a0, d1) & (1 << (d2 & 7)))
            d0 |= 1UL << (d6 & 31);
        SETW(d6, d6 + 1);
    } while (W(d6) != W(d7));
    monofetch();
    BLITWAIT(a2);
    yofill();
    a4 = sa4;
    a5 = sa5;
}

static void floodfill2(void)
{
    UBYTE *sa4 = a4, *sa5 = a5;

    coords[0] = d0;
    coords[1] = d1;
    fcol = d2;
    SETW(d0, d3);                               /* border colour */
    monofetch();
    BLITWAIT(a2);
    CW(a2, 0x40) = 0x090f;                      /* invert the mask */
    CL(a2, 0x50) = (ULONG)a5;
    CL(a2, 0x54) = (ULONG)a5;
    CW(a2, 0x58) = d6;
    BLITWAIT(a2);
    yofill();
    a4 = sa4;
    a5 = sa5;
}

/* ------------------------------------------------------------ polygons */

static void polyo(void)
{
    UBYTE *sa4 = a4, *sa5 = a5, *sa6 = a6;

    a4 = a3;
    a5 = (UBYTE *)d1;
    a6 = (UBYTE *)d1;
    SETW(d0, d0 - 1);
    pfc = d0;
    color = d2;
    do {                                        /* nxpfc */
        d0 = MOVEMW(a5, 0); d1 = MOVEMW(a5, 2); d2 = MOVEMW(a5, 4); d3 = MOVEMW(a5, 6);
        a5 += 4;
        a3 = a4;
        clipme();
    } while (--pfc);                            /* SUB #1,pfc:BNE */
    d0 = MOVEMW(a5, 0); d1 = MOVEMW(a5, 2);
    d2 = MOVEMW(a6, 0); d3 = MOVEMW(a6, 2);
    a3 = a4;
    clipme();
    a4 = sa4;
    a5 = sa5;
    a6 = sa6;
}

/* polygon outline lists (shared with polylib's algorithm) */
static void drawpoly(void)
{
    if (SW(pcolor >> 16) < 0)                   /* TST pcolor */
        return;
    d6 = ML(a6, 48);                            /* porigin */
    if (d6) {
        UWORD n;

        d5 = SWAP(d6);
        SETW(d4, d7);
        SETW(d4, d4 - 1);
        a1 = a0;
        n = d4;
        do {
            MW(a1, 0) += d5; MW(a1, 2) += d6;
            MW(a1, 4) += d5; MW(a1, 6) += d6;
            a1 += 8;
        } while (n-- != 0);
    }
    SETW(d6, d7);                               /* tops */
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

    /* cliptotop */
    if (SW(d1) < 0) {
        d5 = 0;
        for (;;) {
            if (SW(d5) < SW(MW(a1, 2)))
                break;
            d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
            a1 -= 4;
            if (!((LONG)(a1 - a2) >= 0))
                return;
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
                return;
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
    /* cliptostrt */
    if (!(SW(d0 - MW(a6, 56)) < 0)) {          /* entering from right */
        SETW(d6, d1);
        SETW(d5, MW(a6, 56));
        SETW(d5, d5 - 1);
        for (;;) {
            if (SW(d5) > SW(MW(a1, 0)))
                break;
            d0 = MOVEMW(a1, 0); d1 = MOVEMW(a1, 2);
            a1 -= 4;
            if (!((LONG)(a1 - a2) >= 0))
                return;
        }
        d3 = MOVEMW(a1, 0); d4 = MOVEMW(a1, 2);     /* clipm */
        SETW(d5, d5 - d0);
        SETW(d3, d3 - d0);
        SETW(d4, d4 - d1);
        d4 = MULS(d5, d4);
        DIVS(d4, d3);
        SETW(d1, d1 + d4);
        EXG(d1, d6);
        SETW(d0, MW(a6, 56));
        SETW(d0, d0 - 1);
        for (;;) {
            if (SW(d6 - MW(a2, 2)) < 0)
                break;
            d1 = MOVEMW(a2, 0); d2 = MOVEMW(a2, 2);
            a2 += 4;
            EXG(d1, d2);
            if (!((LONG)(a1 - a2) >= 0))
                return;
        }
        d3 = MOVEMW(a2, 0); d4 = MOVEMW(a2, 2);     /* mom */
        SETW(d4, d4 - d1);
        SETW(d3, d3 - d2);
        EXG(d1, d6);
        SETW(d6, d6 - d1);
        d6 = MULS(d3, d6);
        DIVS(d6, d4);
        SETW(d2, d2 - d6);
    } else if (SW(d2) < 0) {                    /* entering from left */
        SETW(d6, d1);
        d5 = 0;
        for (;;) {
            if (SW(d5) < SW(MW(a2, 0)))
                break;
            d1 = MOVEMW(a2, 0); d2 = MOVEMW(a2, 2);
            a2 += 4;
            EXG(d1, d2);
            if (!((LONG)(a1 - a2) >= 0))
                return;
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
                return;
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
    /* nonclip: still inside window? */
    if (SW(d1 - MW(a6, 58)) >= 0)
        return;
    xyx[0] = d0;
    xyx[1] = d1;
    xyx[2] = d2;
    SETW(d5, MW(a6, 58));
    SETW(d6, MW(a6, 56));
    SETW(d6, d6 - 1);
    d7 = 0;

    /* lside */
    a3 = (UBYTE *)l0;
    d2 = MOVEMW(a1, 0); d3 = MOVEMW(a1, 2);
    for (;;) {                                  /* lline */
        if (SW(d3) <= SW(d1)) {                 /* SUB d1,d3:BLE */
            SETW(d3, d3 - d1);
            goto dunl;
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
            SETW(d4, d4 - 2);
            d2 <<= 2;
            SETW(d3, W(d3) >> 2);
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
            goto dunl;
        }
        if (SW(d0) > SW(d6)) {                  /* loff */
            d2 = MOVEMW(a1, 4); d3 = MOVEMW(a1, 6);
            SETW(d2, d2 - d0);
            SETW(d3, d3 - d1);
            SETW(d0, d0 - d6);
            d3 = MULS(d0, d3);
            DIVS(d3, d2);
            MW(a3, -6) -= W(d3);
            goto dunl;
        }
        a1 -= 4;
        d2 = MOVEMW(a1, 0); d3 = MOVEMW(a1, 2);
    }
dunl:
    ML(a3, 0) = 0xffffffff;

    /* rside */
    d0 = MOVEMW(xyx, 2); d1 = MOVEMW(xyx, 4);
    EXG(d0, d1);
    a3 = (UBYTE *)l1;
    d2 = MOVEMW(a2, 0); d3 = MOVEMW(a2, 2);
    for (;;) {                                  /* rline */
        if (SW(d3) <= SW(d1)) {                 /* SUB d1,d3:BLE */
            SETW(d3, d3 - d1);
            goto dunr;
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
        d0 = 3;
        do {
            SETW(d4, d4 - 2);
            d2 <<= 2;
            SETW(d3, W(d3) >> 2);
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
            goto dunr;
        }
        if (SW(d0 - d7) < 0) {                  /* roff */
            d2 = MOVEMW(a2, -4); d3 = MOVEMW(a2, -2);
            SETW(d2, d2 - d0);
            SETW(d3, d3 - d1);
            SETW(d0, d0 - d7);
            d3 = MULS(d0, d3);
            DIVS(d3, d2);
            MW(a3, -6) -= W(d3);
            goto dunr;
        }
        a2 += 4;
    }
dunr:
    ML(a3, 0) = 0xffffffff;

    /* fill with the blitter */
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
        return;
    d4 = ML(a0, 0);
    a0 += 4;
    a1 = (UBYTE *)l1;
    SETW(d3, MW(a1, 0));
    a1 += 2;
    if (SW(d3) < 0)
        return;
    d5 = ML(a1, 0);
    a1 += 4;
    a4 = (UBYTE *)0xdff000;
    BLITWAIT(a4);                               /* bltsetup */
    CW(a4, 0x74) = 0xffff;
    CW(a4, 0x42) = 0;
    for (;;) {                                  /* ploop */
        ULONG before;                           /* for the carry of ADD.l */

        before = d0;
        d0 += d4;
        if (d0 < before)
            SETW(d0, d0 + 2);
        before = d1;
        d1 += d5;
        if (d1 < before)
            SETW(d1, d1 + 2);
        SETW(d6, d0);
        if (SW(d6) > SW(rtclip))
            return;
        d6 = SWAP(d6);
        SETW(d6, d0);
        if (SW(d6) < 0)
            d6 = 0;
        if (SW(d1) < 0)
            return;
        SETW(d7, rtclip);
        if (!(SW(d7 - d1) < 0))
            SETW(d7, d1);
        BLITWAIT(a4);
        SETW(d6, d6 & 30);
        CW(a4, 0x44) = lup1[W(d6) >> 1];
        SETW(d6, d7);
        SETW(d6, d6 & 30);
        CW(a4, 0x46) = lup2[W(d6) >> 1];
        d6 = SWAP(d6);
        SETW(d6, W(d6) >> 5);
        SETW(d7, W(d7) >> 5);
        SETW(d7, d7 - d6);
        if (SW(d7) < 0)
            return;
        SETW(d7, d7 + 65);
        SETW(d6, d6 + d6);
        a2 = (UBYTE *)bmoff + SW(d6);
        d6 = 0;
        a5 = a6 + 8;
        do {                                    /* nxpolyplane */
            BLITWAIT(a4);
            CW(a4, 0x40) = 0x3fa;
            if (!((UBYTE)(pcolor >> 16) & (1 << (d6 & 7))))     /* BTST d6,pcolor+1 */
                CW(a4, 0x40) = 0x30a;
            a3 = MP(a5, 0) + (ULONG)a2;
            a5 += 4;
            CL(a4, 0x48) = (ULONG)a3;
            CL(a4, 0x54) = (ULONG)a3;
            CW(a4, 0x58) = d7;
            SETW(d6, d6 + 1);
        } while (W(d6) != MW(a6, 4));
        d7 = bmwid;
        bmoff += d7;
        before = d0;
        d0 += d4;
        if (d0 < before)
            SETW(d0, d0 + 2);
        before = d1;
        d1 += d5;
        if (d1 < before)
            SETW(d1, d1 + 2);
        SETW(d2, d2 - 1);
        if (W(d2) == 0xffff) {
            SETW(d2, MW(a0, 0));
            a0 += 2;
            if (SW(d2) < 0)
                return;
            d4 = ML(a0, 0);
            a0 += 4;
        }
        SETW(d3, d3 - 1);
        if (W(d3) == 0xffff) {
            SETW(d3, MW(a1, 0));
            a1 += 2;
            if (SW(d3) < 0)
                return;
            d5 = ML(a1, 0);
            a1 += 4;
        }
    }
}

static void polyfill(UWORD n, ULONG pts)
{
    UBYTE *sa4 = a4, *sa5 = a5, *sa6 = a6;
    UWORD k;
    int clockwise;

    a2 = (UBYTE *)pts;
    SETW(d7, n);
    a6 = a3;
    d0 = MOVEMW(a2, 0); d1 = MOVEMW(a2, 2); d2 = MOVEMW(a2, 4);
    d3 = MOVEMW(a2, 6); d4 = MOVEMW(a2, 8); d5 = MOVEMW(a2, 10);
    SETW(d4, d4 - d2);
    SETW(d5, d5 - d3);
    SETW(d2, d2 - d0);
    SETW(d3, d3 - d1);
    d5 = MULS(d2, d5);
    d3 = MULS(d4, d3);
    clockwise = !((LONG)d5 <= (LONG)d3);         /* SUB.l d3,d5:BLE anti */
    d5 -= d3;
    if (clockwise) {                            /* clck */
        SETW(d6, d7);
        SETW(d6, d6 << 2);
        a1 = (UBYTE *)polypts;
        a3 = a1 + SW(d6);
        k = W(d7) - 1;
        do {
            d0 = ML(a2, 0);
            a2 += 4;
            ML(a1, 0) = d0;
            a1 += 4;
            ML(a3, 0) = d0;
            a3 += 4;
        } while (k-- != 0);
    } else {                                    /* anti */
        SETW(d6, d7);
        SETW(d6, d6 << 2);
        a1 = (UBYTE *)polypts + SW(d6);
        a3 = a1 + SW(d6);
        k = W(d7) - 1;
        do {
            d0 = ML(a2, 0);
            a2 += 4;
            a1 -= 4;
            ML(a1, 0) = d0;
            a3 -= 4;
            ML(a3, 0) = d0;
        } while (k-- != 0);
        pcolor = (pcolor & 0xffff) | (pcolor << 16);    /* MOVE pcolor+2,pcolor */
    }
    a0 = (UBYTE *)polypts;                      /* dunc */
    drawpoly();
    a4 = sa4;
    a5 = sa5;
    a6 = sa6;
    a2 = (UBYTE *)0xdff000;
}

/* ------------------------------------------------------------- ClipBlit */

static void BLITZ_ROUTINE _doclipblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(_doclipblitmode, _doclipblitmode_c, "d0-d1/a0-a1");

/* b_docblit - a3 = shape, a2 = bitmap, a1 = chip, d1/d2 = x/y;
   d7 = 0 if nothing was blitted */
static void docblit(void)
{
    SETW(d1, d1 - MW(a3, 10));
    SETW(d2, d2 - MW(a3, 12));
    if (SW(d1 - MW(a2, 56)) >= 0 || SW(d2 - MW(a2, 58)) >= 0)
        goto flopme;
    BLITWAIT(a1);
    CL(a1, 0x60) = d6;                          /* MOVEM.l d6-d7,$060(a1) */
    CL(a1, 0x64) = d7;
    SETW(d6, MW(a3, 8));
    d3 = 0;
    SETW(d4, d2);
    if (SW(d2) < 0) {
        SETW(d2, -SW(d2));
        if (SW(d2 - MW(a3, 2)) >= 0)
            goto flopme;
        SETW(d3, d2);
        d3 = MULU(d3, MW(a3, 6));
        SETW(d2, d2 << 6);
        if (W(d6) < W(d2))                      /* SUB.w d2,d6:BCS */
            goto flopme;
        SETW(d6, d6 - d2);
        SETW(d2, 0);
    }
    SETW(d4, d4 + MW(a3, 2));                   /* bolo */
    if (SW(d4) < 0)
        goto flopme;
    SETW(d4, d4 - MW(a2, 58));
    if (SW(d4) >= 0) {
        SETW(d4, d4 << 6);
        if (W(d6) < W(d4))
            goto flopme;
        SETW(d6, d6 - d4);
    }
    CL(a1, 0x44) = 0xffff0000;                  /* abuv */
    d5 = 0xfffffffe;
    SETW(d7, MW(a2, 0));
    SETW(d7, d7 - MW(a3, 6));
    SETW(d7, d7 + d5);
    if (SW(d1) < 0) {
        SETW(d1, -SW(d1));
        if (SW(d1 - MW(a3, 0)) >= 0)
            goto flopme;
        SETW(d1, -SW(d1));
        for (;;) {                              /* g1 */
            SETW(d1, d1 + 16);
            if (SW(d1) >= 0)
                break;
            SETW(d5, d5 + 2);
            SETW(d7, d7 + 2);
            SETW(d6, d6 - 1);
            SETW(d3, d3 + 2);
        }
        d0 = 0xffffffff;                        /* g2 */
        SETW(d0, (d1 & 63) >= 16 ? 0 : (UWORD)(W(d0) << (d1 & 63)));
        SETW(d0, ~d0);
        CW(a1, 0x44) = d0;
        SETW(d1, d1 - 16);
    }
    SETW(d0, d1);                               /* nolo */
    SETW(d0, SW(d0) >> 4);
    SETW(d0, d0 + d0);
    SETW(d0, d0 + MW(a3, 6));
    SETW(d0, d0 + 2);
    SETW(d4, MW(a2, 56));
    SETW(d4, W(d4) >> 3);
    {
        int le = SW(d0) <= SW(d4);              /* SUB.w d4,d0:BLE noro */

        SETW(d0, d0 - d4);
        if (le)
            goto noro;
    }
    {
        SETW(d5, d5 + d0);
        SETW(d7, d7 + d0);
        SETW(d0, W(d0) >> 1);
        SETW(d6, d6 - d0);
        SETW(d4, d1);
        SETW(d4, d4 & 15);
        SETW(d0, 0xffff);
        SETW(d0, (d4 & 63) >= 16 ? 0 : (UWORD)(W(d0) << (d4 & 63)));
        CW(a1, 0x46) = d0;
    }
noro:
    CW(a1, 0x62) = d5;
    CW(a1, 0x64) = d5;
    CW(a1, 0x60) = d7;
    CW(a1, 0x66) = d7;
    SETW(d0, d1);
    SETW(d0, d0 & 15);
    SETW(d0, (UWORD)(W(d0) >> 4 | W(d0) << 12));    /* ROR #4 */
    SETW(d4, d0);
    CW(a1, 0x42) = d0;
    SETW(d0, d0 | bcon0);
    CW(a1, 0x40) = d0;
    d0 = d3;
    d0 += ML(a3, 18);
    d3 += ML(a3, 14);
    d2 = MULS(d2, MW(a2, 0));
    SETW(d1, SW(d1) >> 4);
    d1 = EXTL(d1);
    d2 += d1;
    d2 += d1;
    a0 = a2 + 8;
    d5 = MULU(MW(a3, 2), MW(a3, 6));
    SETW(d7, MW(a3, 4));
    if (!W(d7))
        goto flopme;
    SETW(d7, d7 - 1);
    for (;;) {                                  /* doblit */
        CL(a1, 0x50) = d0;
        CL(a1, 0x4c) = d3;
        d3 += d5;
        d1 = ML(a0, 0) + d2;
        a0 += 4;
        CL(a1, 0x48) = d1;
        CL(a1, 0x54) = d1;
        CW(a1, 0x58) = d6;
        SETW(d7, d7 - 1);
        if (W(d7) == 0xffff)
            return;
        BLITWAIT(a1);
    }
flopme:
    d7 = 0;
}

static void docblitx(void)
{
    UWORD s3 = d3;

    docblit();
    SETW(d3, s3);
    if (!W(d7))
        return;
    SETW(d7, MW(a3, 4));
    for (;;) {
        int c;

        if (W(d7) >= MW(a2, 4))
            return;
        SETW(d5, 0xb0a);
        c = d3 & 1;
        SETW(d3, W(d3) >> 1);
        if (c)
            SETW(d5, 0xbfa);
        SETW(d5, d5 | W(d4));
        d1 = ML(a0, 0) + d2;
        a0 += 4;
        BLITWAIT(a1);
        CW(a1, 0x40) = d5;
        CL(a1, 0x50) = d0;
        CL(a1, 0x48) = d1;
        CL(a1, 0x54) = d1;
        CW(a1, 0x58) = d6;
        SETW(d7, d7 + 1);
    }
}

/* -------------------------------------------- entries (Blitz / Amiga) */

/* load the routine's registers: d0-d7 from the caller, a1/a2/a3 */
#define REGS ULONG r0 BL_REG(d0), ULONG r1 BL_REG(d1), ULONG r2 BL_REG(d2), \
             ULONG r3 BL_REG(d3), ULONG r4 BL_REG(d4), ULONG r5 BL_REG(d5)
#define LOAD() (d0 = r0, d1 = r1, d2 = r2, d3 = r3, d4 = r4, d5 = r5)

/* Blitz version (b_) and Amiga version (a_) of a blitter routine:
   the Amiga version owns the blitter (graphics base in a6) */
#define BLITPAIR(name, body)                                                \
    static void BLITZ_ROUTINE b_##name##_c(REGS, UBYTE *cb BL_REG(a2))      \
    {                                                                       \
        LOAD();                                                             \
        a2 = cb;                                                            \
        a3 = blitz_string;                                                  \
        body;                                                               \
        blitz_string = a3;                                                  \
    }                                                                       \
    static void BLITZ_ROUTINE a_##name##_c(REGS, UBYTE *cb BL_REG(a2),      \
                                           struct GfxBase *gfx BL_REG(a6))  \
    {                                                                       \
        OwnBlitter();                                                       \
        LOAD();                                                             \
        a2 = cb;                                                            \
        a3 = blitz_string;                                                  \
        body;                                                               \
        BLITWAIT(a2);                                                       \
        DisownBlitter();                                                    \
        blitz_string = a3;                                                  \
    }

BLITPAIR(line, bline())
BLITPAIR(lineto, blineto())
BLITPAIR(linew, blinew())
BLITPAIR(box, boxo())
BLITPAIR(boxf, boxfo())
BLITPAIR(circlef, (SETW(d4, d3), SETW(d3, d2), ellipsefo()))
BLITPAIR(ellipsef, ellipsefo())
BLITPAIR(floodfill, floodfill())
BLITPAIR(floodfill2, floodfill2())
BLITPAIR(remap, (monofetch(), monowrite()))
BLITPAIR(poly, polyo())
BLITPAIR(polyf, (pcolor = (ULONG)(UWORD)d2 << 16 | (UWORD)d2, polyfill(d0, d1)))
BLITPAIR(polyf2, (pcolor = (ULONG)(UWORD)d2 << 16 | (UWORD)d3, polyfill(d0, d1)))

/* ReMap 2: destination bitmap in a0 */
static void remap2(UBYTE *dst)
{
    monofetch();
    a3 = dst;
    monowrite();
}

static void BLITZ_ROUTINE b_remap2_c(REGS, UBYTE *dst BL_REG(a0), UBYTE *cb BL_REG(a2))
{
    LOAD();
    a2 = cb;
    a3 = blitz_string;
    remap2(dst);
    blitz_string = a3;
}

static void BLITZ_ROUTINE a_remap2_c(REGS, UBYTE *dst BL_REG(a0), UBYTE *cb BL_REG(a2),
                                     struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    LOAD();
    a2 = cb;
    a3 = blitz_string;
    remap2(dst);
    BLITWAIT(a2);
    DisownBlitter();
    blitz_string = a3;
}

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
#define CBREGS ULONG r1 BL_REG(d1), ULONG r2 BL_REG(d2), ULONG r3 BL_REG(d3), \
               ULONG r6 BL_REG(d6), ULONG r7 BL_REG(d7), UBYTE *cb BL_REG(a1), \
               UBYTE *bm BL_REG(a2)
#define CBLOAD() (d1 = r1, d2 = r2, d3 = r3, d6 = r6, d7 = r7, a1 = cb, a2 = bm, \
                  a3 = blitz_string)

static void BLITZ_ROUTINE b_docblit_c(CBREGS)
{
    CBLOAD();
    docblit();
}

static void BLITZ_ROUTINE a_docblit_c(CBREGS, struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    CBLOAD();
    docblit();
    BLITWAIT(a1);
    DisownBlitter();
}

static void BLITZ_ROUTINE b_docblitx_c(CBREGS)
{
    CBLOAD();
    docblitx();
}

static void BLITZ_ROUTINE a_docblitx_c(CBREGS, struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    CBLOAD();
    docblitx();
    BLITWAIT(a1);
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
    if (!MW(blitz_string, 0))
        BLITZ_RUNERR(mess1);
}
BLITZ_THUNK(_cbchk, _cbchk_c, "d0-d1/a0-a1");
