/*
 * bb_blitlib - RWE userlib 19 (Reflective Images, Stephen McNamara): the
 * custom blitter code of Blitz Bombers.  Blitz mode routines that program
 * the blitter registers directly (no OwnBlitter), on chip memory:
 *
 *   SpecialBlock top,bottom,x,y | offset   16x16 block: top shape over
 *       bottom shape (through top's cookie) into the used bitmap
 *   SpecialBlockBitmap top,bitmap,x,y | offset   a shape of any size over
 *       the same area of another bitmap into the used bitmap
 *   TBlock shape,x,y                       16x16 shape through its cookie
 *   PredatorBlit(shape,x,y)                the "predator" effect: the
 *       bitmap shifted one line up and one pixel (B = the line below)
 *       inside the cookie; returns the cookie address
 *   BPredatorBlit buffer,shape,x,y         the same, buffered (the
 *       background saved like bblitlib's BBlit), one line less
 *   SetupMapTwo ebwidth                    the bytes per line of the
 *       bitmaps (default 40) the routines above assume
 *   CreateShape start,count,chipmem,like   count shape objects like
 *       `like`, their data in chipmem one after another ("cL", no cookie)
 *   SpecialBlockToShape top,bottom,dest    SpecialBlock into a shape
 *   BlockToShape shape,dest                copies a shape's data (16 wide)
 *   BColourBlit buffer,shape,x,y,colour    buffered: the cookie in one
 *       colour
 *   TBlockImage image_addr,image,x,y       16x16 image of 6 planes (mask
 *       and planes, 32 bytes each) through its mask
 *   BTBlockImage buffer,image_addr,x,y,image   the same, buffered and
 *       shifted
 *
 * C translation of rwelibsrc/blitzbomberslib/BB_BlitLib4.bb (30/1/96);
 * reference: the reference build of the source (no binary exists,
 * docs/rwelibs/README.md 3.3).  Notes: docs/rwelibs/port/bb_blitlib.md.
 *
 * Objects: shape (shapeslib, 32 bytes: +0 width, +2 height, +4 depth, +6
 * bytes per row, +8 blit size, +10/+12 handles, +14 data, +18 cookie,
 * +22/+24 bytes per plane (+24 with the shift word), +26 all planes, +30
 * "cL"), bitmap (bitmaplib: +0 bytes per row, +4 depth, +8 planes),
 * buffer (bblitlib: +0 write position, +4 last record; a record as
 * bblitlib's BBlit writes it).
 *
 * The register writes are the original's, in its order: the blitter
 * pointers that are not reloaded per plane go on where the last blit
 * left them.  init and finit are RTS.  No assembler besides the thunks.
 *
 * BUG(orig), kept:
 *   - Nothing is checked but the two error checks; a shape or bitmap of
 *     depth 0 loops 65536 times (DBRA) (#429); BlockToShape copies one
 *     word per line (#425).
 *   - BPredatorBlit's B channel reads the line SetupMapTwo's width below,
 *     not the bitmap's (#426).
 *   - BTBlockImage adds image*224 as a word (images from 147 on are
 *     before image_addr) (#427).
 *   - CreateShape with count 0 creates 65536 shapes (#428).
 * Original behaviour, not defects: SpecialBlock, TBlock and PredatorBlit
 * assume 16x16 shapes and bitmaps of SetupMapTwo's width (the map format
 * they are written for); BPredatorBlit stores a blit size one line less
 * in the record (deliberate, ";!!! BB Cludge!" in the source); BColourBlit
 * sets the modulos of the B channel it does not use (B is off) and shifts
 * its colour as a byte (LSR.b; at most 8 planes).
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _specialblock(void), _specialblock2(void), _specialblockbitmap(void);
void _specialblockbitmap2(void), _tblock(void), _predatorblit(void), _bpredatorblit(void);
void _MapInfo(void), _createshape(void), _specialblocktoshape(void), _blocktoshape(void);
void docolblit(void), _tblockimage(void), _btblockimage(void);
void _errSBlockToShape(void), _errBlockToShape(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define UA0 (BL_LA(0) | BL_USED)                    /* #ua0 */
#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */
/* the !libs of the buffered blits (as bblitlib's BBlit) */
#define BUFLIBS LIB_bblitlib, 0x1080, LIB_bitmaplib, 0x1202, LIB_chipbaselib, 0x1100, \
                LIB_graphicslib, 0x5600

/* !libheader {19,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_bb_blitlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0), LIB_shapeslib, IA(2, 1));
BLITZ_SUBS(_specialblock, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0), LIB_shapeslib, IA(2, 1));
BLITZ_SUBS(_specialblock2, 0, 0);
BLITZ_NAME("SpecialBlock", "topshape,bottomshape,(x,y)|offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0), LIB_bitmaplib, IA(2, 1));
BLITZ_SUBS(_specialblockbitmap, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0), LIB_bitmaplib, IA(2, 1));
BLITZ_SUBS(_specialblockbitmap2, 0, 0);
BLITZ_NAME("SpecialBlockBitmap", "topshape,bitmap,(x,y)|offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0));
BLITZ_SUBS(_tblock, 0, 0);
BLITZ_NAME("TBlock", "shape,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0, LIB_shapeslib, IA(1, 0));
BLITZ_SUBS(_predatorblit, 0, 0);
BLITZ_NAME("PredatorBlit", "shape,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1381, BUFLIBS);
BLITZ_SUBS(_bpredatorblit, 0, 0);
BLITZ_NAME("BPredatorBlit", "buffer,shape,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_MapInfo, 0, 0);
BLITZ_NAME("SetupMapTwo", "ebwidth");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1280, LIB_shapeslib, 0x1383);
BLITZ_SUBS(_createshape, 0, 0);
BLITZ_NAME("CreateShape", "start_shape,count,chipmemblock,like_shape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(1, 0), LIB_shapeslib, IA(2, 1), LIB_shapeslib, IA(0, 2));
BLITZ_SUBS(_specialblocktoshape, _errSBlockToShape, 0);
BLITZ_NAME("SpecialBlockToShape", "topshape,bottomshape,destshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(1, 0), LIB_shapeslib, IA(2, 1));
BLITZ_SUBS(_blocktoshape, _errBlockToShape, 0);
BLITZ_NAME("BlockToShape", "shape,destshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1381, BUFLIBS);
BLITZ_SUBS(docolblit, 0, 0);
BLITZ_NAME("BColourBlit", "buffer,shape,x,y,colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, UA0);
BLITZ_SUBS(_tblockimage, 0, 0);
BLITZ_NAME("TBlockImage", "image_addr,image,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(BUFLIBS);
BLITZ_SUBS(_btblockimage, 0, 0);
BLITZ_NAME("BTBlockImage", "buffer,image_addr,x,y,image");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* init, finit: RTS */
void BLITZ_ROUTINE _libinit(void)
{
}

void BLITZ_ROUTINE _libfinit(void)
{
}

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))

/* the blitter registers: absolute ($DFF0xx) or through the chip base the
   buffered routines get in a1 (#chipbaselib) */
#define CUSTOM ((UBYTE *)0xdff000)
#define W(o) BL_CUSTOMW(CUSTOM, o)
#define L(o) BL_CUSTOML(CUSTOM, o)
#define CW(o) BL_CUSTOMW(cb, o)
#define CL(o) BL_CUSTOML(cb, o)
#define BLITWAIT() BL_BLITWAIT(CUSTOM)
enum {
    BLTCON0 = 0x40, BLTCON1 = 0x42, BLTAFWM = 0x44, BLTALWM = 0x46, BLTCPT = 0x48,
    BLTBPT = 0x4c, BLTAPT = 0x50, BLTDPT = 0x54, BLTSIZE = 0x58, BLTCMOD = 0x60,
    BLTBMOD = 0x62, BLTAMOD = 0x64, BLTDMOD = 0x66
};

#define MYBLITSIZE (1 + (16 << 6))          /* 16 lines of one word */
#define MYBLITSIZESCROLL (2 + (16 << 6))    /* 16 lines of two words */
#define DEPTH 6                             /* TBlockImage */
#define IMAGEPLANESIZE 32
#define IMAGESIZE (IMAGEPLANESIZE * (DEPTH + 1))
#define SHAPE_DEPTH 6                       /* BTBlockImage */
#define SHAPE_EBWIDTH 2
#define SHAPE_BLITSIZE ((16 * 64) + SHAPE_EBWIDTH)
#define SHAPE_PLANEBYTESIZE (SHAPE_EBWIDTH * 16)
#define SHAPE_ONEBPMEMX ((SHAPE_EBWIDTH + 2) * 16)
#define SHAPE_BYTESIZE (SHAPE_PLANEBYTESIZE * (SHAPE_DEPTH + 1))

/* pad: Dc.w 0 in front of ebwidth - ADD.l ebwidth-2(pc) adds the long
   pad:ebwidth, i.e. ebwidth */
static UWORD ebwidth = 40, modwidth = 38, modwidth2 = 36;
static UWORD coltoblit;

/* SetupMapTwo ebwidth */
static void BLITZ_ROUTINE mapinfo_c(UWORD w BL_REG(d0))
{
    ebwidth = w;
    modwidth = w - 2;
    modwidth2 = w - 4;
}

/* (x,y) to an offset in the bitmap: MULU.w by the width, x ASR 3, a word
   sum */
static UWORD xyoff(UWORD x, UWORD y, UWORD width)
{
    return (UWORD)((ULONG)y * width) + (UWORD)((WORD)x >> 3);
}

/* ------------------------------------------------------- SpecialBlock */

/* the 16x16 top shape (B) over the bottom shape (C) through top's cookie
   (A) into each plane of the used bitmap at off; B and C go on from plane
   to plane */
static void BLITZ_ROUTINE specialblock2_c(UWORD off BL_REG(d2), UBYTE *bm BL_REG(a0),
                                          UBYTE *top BL_REG(a1), UBYTE *bot BL_REG(a2))
{
    UWORD n = SW(top, 4) - 1;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;                    /* 4 channels, D = A ? B : C */
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = 0;
    W(BLTDMOD) = modwidth;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = SL(top, 14);
    L(BLTCPT) = SL(bot, 14);
    cookie = SL(top, 18);
    goto first;
    do {
        BLITWAIT();
first:
        {
            ULONG d = (ULONG)*plane++ + off;
            L(BLTAPT) = cookie;
            L(BLTDPT) = d;
            W(BLTSIZE) = MYBLITSIZE;
        }
    } while (n-- != 0);
}

static void BLITZ_ROUTINE specialblock_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                         UBYTE *bm BL_REG(a0), UBYTE *top BL_REG(a1),
                                         UBYTE *bot BL_REG(a2))
{
    specialblock2_c(xyoff(x, y, ebwidth), bm, top, bot);
}

/* ------------------------------------------------- SpecialBlockBitmap */

/* the shape (B, any size) over the source bitmap (C) through its cookie
   into the used bitmap, both at off */
static void BLITZ_ROUTINE specialblockbitmap2_c(UWORD off BL_REG(d2), UBYTE *bm BL_REG(a0),
                                                UBYTE *shape BL_REG(a1), UBYTE *src BL_REG(a2))
{
    UWORD n = SW(shape, 4) - 1;
    UWORD mod = SW(bm, 0) - SW(shape, 6);
    UWORD size = (UWORD)(SW(shape, 6) >> 1) + (UWORD)(SW(shape, 2) << 6);
    UBYTE **plane = (UBYTE **)(bm + 8), **splane = (UBYTE **)(src + 8);
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = mod;
    W(BLTDMOD) = mod;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = SL(shape, 14);
    cookie = SL(shape, 18);
    goto first;
    do {
        BLITWAIT();
first:
        L(BLTCPT) = (ULONG)*splane++ + off;
        L(BLTDPT) = (ULONG)*plane++ + off;
        L(BLTAPT) = cookie;
        W(BLTSIZE) = size;
    } while (n-- != 0);
}

static void BLITZ_ROUTINE specialblockbitmap_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                               UBYTE *bm BL_REG(a0), UBYTE *shape BL_REG(a1),
                                               UBYTE *src BL_REG(a2))
{
    specialblockbitmap2_c(xyoff(x, y, SW(bm, 0)), bm, shape, src);
}

/* ------------------------------------------------------------- TBlock */

/* a 16x16 shape (B) through its cookie (A) onto the bitmap (C = D) */
static void BLITZ_ROUTINE tblock_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2), UBYTE *bm BL_REG(a0),
                                   UBYTE *shape BL_REG(a1))
{
    ULONG off = xyoff(x, y, ebwidth);
    UWORD n = SW(shape, 4) - 1;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = modwidth;
    W(BLTDMOD) = modwidth;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = SL(shape, 14);
    cookie = SL(shape, 18);
    goto first;
    do {
        BLITWAIT();
first:
        {
            ULONG d = (ULONG)*plane++ + off;
            L(BLTCPT) = d;
            L(BLTDPT) = d;
            L(BLTAPT) = cookie;
            W(BLTSIZE) = MYBLITSIZE;
        }
    } while (n-- != 0);
}

/* ------------------------------------------------------- PredatorBlit */

/* B = the bitmap one line further, shifted by x and one pixel more, C =
   D = the bitmap, 2 words wide at the even word, through the cookie (A,
   shifted, last word masked); d0 = the cookie */
static ULONG BLITZ_ROUTINE predatorblit_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                          UBYTE *bm BL_REG(a0), UBYTE *shape BL_REG(a1))
{
    ULONG con = 0x0fca1000UL | (ULONG)(x & 15) << 28;
    ULONG off = (UWORD)((UWORD)((ULONG)y * ebwidth) + (UWORD)(((WORD)x >> 3) & 0xfffe));
    UWORD n = SW(shape, 4) - 1;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = con;
    W(BLTAMOD) = (UWORD)-2;
    W(BLTBMOD) = modwidth2;
    W(BLTCMOD) = modwidth2;
    W(BLTDMOD) = modwidth2;
    L(BLTAFWM) = 0xffff0000;
    cookie = SL(shape, 18);
    goto first;
    do {
        BLITWAIT();
first:
        {
            ULONG d = (ULONG)*plane++ + off;
            L(BLTCPT) = d;
            L(BLTDPT) = d;
            L(BLTAPT) = cookie;
            L(BLTBPT) = d + ebwidth;
            W(BLTSIZE) = MYBLITSIZESCROLL;
        }
    } while (n-- != 0);
    return cookie;
}

/* ---------------------------------------------------- buffered blits */

/* the save of the background into a record of the buffer (bblitlib's
   BBlit): +0 previous record, +4.w modulo, +6.w blit size, +8.w bitmap
   planes-1, +10.w bytes per plane, +12 offset, per plane {address,
   data}.  The values the blit goes on with. */
struct bsave {
    ULONG cbmod, admod;                         /* d4-d5: the modulos, MOVEM.l d4-d5 */
    ULONG off;
    UWORD size;
};

static void bsave(struct bsave *s, UBYTE *buf, UBYTE *bm, UBYTE *cb, WORD x, WORD y,
                  UWORD mod, UWORD size, UWORD save)
{
    UBYTE *rec = *(UBYTE **)buf;
    UWORD n;
    UBYTE **plane;

    *(ULONG *)rec = SL(buf, 4);                 /* link the record */
    SL(buf, 4) = (ULONG)rec;
    rec += 4;
    s->admod = 0xfffe0000UL | mod;
    *(UWORD *)rec = mod;
    rec += 2;
    s->cbmod = s->admod << 16 | s->admod >> 16;
    s->off = (ULONG)bl_muls_w(y, SW(bm, 0)) + (LONG)(WORD)(x >> 3);
    s->size = size;
    *(UWORD *)rec = size;
    rec += 2;
    n = SW(bm, 4) - 1;
    *(UWORD *)rec = n;
    rec += 2;
    *(UWORD *)rec = save;
    rec += 2;
    *(ULONG *)rec = s->off;
    rec += 4;
    BL_BLITWAIT(cb);
    CW(BLTAMOD) = mod;
    CW(BLTDMOD) = 0;
    CL(BLTCON0) = 0x09f00000;                   /* A -> D */
    CL(BLTAFWM) = 0xffffffff;
    plane = (UBYTE **)(bm + 8);
    goto first;
    do {
        BL_BLITWAIT(cb);
first:
        {
            ULONG a = (ULONG)*plane++ + s->off;
            CL(BLTAPT) = a;
            *(ULONG *)rec = a;
            rec += 4;
            CL(BLTDPT) = (ULONG)rec;
            CW(BLTSIZE) = size;
            rec += (WORD)save;
            __asm__("" : "+a"(rec));            /* no IV multiplication */
        }
    } while (n-- != 0);
    *(UBYTE **)buf = rec;
}

/* BPredatorBlit: the shape's area saved (one line less: "BB Cludge"),
   then PredatorBlit's blit through the shifted cookie (B: the line
   ebwidth below, one pixel more) */
static void BLITZ_ROUTINE bpredatorblit_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                          UBYTE *buf BL_REG(a0), UBYTE *cb BL_REG(a1),
                                          UBYTE *bm BL_REG(a2), UBYTE *shape BL_REG(a3))
{
    struct bsave s;
    UWORD shift, n;
    UBYTE **plane;
    ULONG cookie;

    x -= SW(shape, 10);
    y -= SW(shape, 12);
    bsave(&s, buf, bm, cb, x, y, (UWORD)(SW(bm, 0) - SW(shape, 6) - 2),
          (UWORD)(SW(shape, 8) - 64), SW(shape, 24));
    shift = (UWORD)((x & 15) << 12);
    n = SW(shape, 4) - 1;
    plane = (UBYTE **)(bm + 8);
    BL_BLITWAIT(cb);
    L(BLTCMOD) = s.cbmod;                       /* MOVEM.l d4-d5,BLTCMOD */
    L(BLTAMOD) = s.admod;
    W(BLTBMOD) = (UWORD)s.admod;
    W(BLTCON1) = 0x1000;
    W(BLTCON0) = shift | 0xfca;
    CW(BLTALWM) = 0;
    cookie = SL(shape, 18);
    goto first;
    do {
        BL_BLITWAIT(cb);
first:
        {
            ULONG d = (ULONG)*plane++ + s.off;
            L(BLTAPT) = cookie;
            L(BLTCPT) = d;
            L(BLTDPT) = d;
            L(BLTBPT) = d + ebwidth;
            W(BLTSIZE) = s.size;
        }
    } while (n-- != 0);
}

/* BColourBlit: the shape's area saved, then the cookie (shifted) in
   `colour`: per plane D = A | C (bit set) or ~A & C */
static void BLITZ_ROUTINE docolblit_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                      UWORD colour BL_REG(d4), UBYTE *buf BL_REG(a0),
                                      UBYTE *cb BL_REG(a1), UBYTE *bm BL_REG(a2),
                                      UBYTE *shape BL_REG(a3))
{
    struct bsave s;
    UWORD con, n;
    UBYTE bits;
    UBYTE **plane;
    ULONG cookie;

    coltoblit = colour;
    x -= SW(shape, 10);
    y -= SW(shape, 12);
    bsave(&s, buf, bm, cb, x, y, (UWORD)(SW(bm, 0) - SW(shape, 6) - 2), SW(shape, 8),
          SW(shape, 24));
    con = (UWORD)((x & 15) << 12) | 0xb00;      /* A, C, D */
    n = SW(shape, 4) - 1;
    plane = (UBYTE **)(bm + 8);
    BL_BLITWAIT(cb);
    L(BLTCMOD) = s.cbmod;
    L(BLTAMOD) = s.admod;
    W(BLTCON1) = 0;
    CW(BLTALWM) = 0;
    cookie = SL(shape, 18);
    bits = (UBYTE)coltoblit;
    goto first;
    do {
        BL_BLITWAIT(cb);
first:
        {
            ULONG d;
            W(BLTCON0) = con | ((bits & 1) ? 0xfa : 0x0a);
            bits >>= 1;
            L(BLTAPT) = cookie;
            d = (ULONG)*plane++ + s.off;
            L(BLTDPT) = d;
            L(BLTCPT) = d;
            W(BLTSIZE) = s.size;
        }
    } while (n-- != 0);
}

/* ------------------------------------------------------------ images */

/* TBlockImage: image `image` of image_addr (224 bytes: the mask, then 6
   planes of 16x16) through its mask onto 6 planes of the used bitmap */
static void BLITZ_ROUTINE tblockimage_c(ULONG addr BL_REG(d0), UWORD image BL_REG(d1),
                                        UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                        UBYTE *bm BL_REG(a0))
{
    UWORD width = SW(bm, 0);
    ULONG off = xyoff(x, y, width);
    UWORD n = DEPTH - 1;
    UBYTE **plane = (UBYTE **)(bm + 8);

    addr += (ULONG)image * IMAGESIZE;
    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = width - 2;
    W(BLTDMOD) = width - 2;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = addr + IMAGEPLANESIZE;
    goto first;
    do {
        BLITWAIT();
first:
        {
            ULONG d = (ULONG)*plane++ + off;
            L(BLTCPT) = d;
            L(BLTDPT) = d;
            L(BLTAPT) = addr;
            W(BLTSIZE) = MYBLITSIZE;
        }
    } while (n-- != 0);
}

/* BTBlockImage: the area saved, then the image shifted through its mask */
static void BLITZ_ROUTINE btblockimage_c(ULONG addr BL_REG(d1), UWORD x BL_REG(d2),
                                         UWORD y BL_REG(d3), UWORD image BL_REG(d4),
                                         UBYTE *buf BL_REG(a0), UBYTE *cb BL_REG(a1),
                                         UBYTE *bm BL_REG(a2))
{
    struct bsave s;
    UWORD shift, n = SHAPE_DEPTH - 1;
    UBYTE **plane;
    ULONG mask;

    addr += (WORD)(UWORD)((ULONG)image * SHAPE_BYTESIZE);     /* ADD.w */
    bsave(&s, buf, bm, cb, x, y, (UWORD)(SW(bm, 0) - SHAPE_EBWIDTH - 2), SHAPE_BLITSIZE,
          SHAPE_ONEBPMEMX);
    shift = (UWORD)((x & 15) << 12);
    plane = (UBYTE **)(bm + 8);
    BL_BLITWAIT(cb);
    CL(BLTCMOD) = s.cbmod;
    CL(BLTAMOD) = s.admod;
    CW(BLTCON1) = shift;
    CW(BLTCON0) = shift | 0xfca;
    CW(BLTALWM) = 0;
    mask = addr;
    CL(BLTBPT) = addr + SHAPE_PLANEBYTESIZE;
    goto first;
    do {
        BL_BLITWAIT(cb);
first:
        {
            ULONG d = (ULONG)*plane++ + s.off;
            CL(BLTAPT) = mask;
            CL(BLTCPT) = d;
            CL(BLTDPT) = d;
            CW(BLTSIZE) = SHAPE_BLITSIZE;
        }
    } while (n-- != 0);
}

/* ------------------------------------------------------------ shapes */

/* CreateShape: count shape objects from a2 like a3, the data at mem one
   after another (+26 of the like shape each), no cookie, "cL" */
static void BLITZ_ROUTINE createshape_c(UWORD count BL_REG(d1), ULONG mem BL_REG(d2),
                                        UBYTE *shape BL_REG(a2), UBYTE *like BL_REG(a3))
{
    UWORD n = count - 1;
    ULONG all = SW(like, 26);

    do {
        SL(shape, 0) = SL(like, 0);
        SL(shape, 4) = SL(like, 4);
        SL(shape, 8) = SL(like, 8);
        SW(shape, 12) = SW(like, 12);
        SL(shape, 14) = mem;
        SL(shape, 18) = 0;
        SL(shape, 22) = SL(like, 22);
        SL(shape, 26) = SL(like, 26);
        SW(shape, 30) = 0x634c;                 /* "cL" */
        shape += 32;
        mem += all;
    } while (n-- != 0);
}

/* SpecialBlockToShape: SpecialBlock into the data of dest (16x16) */
static void BLITZ_ROUTINE specialblocktoshape_c(UBYTE *dest BL_REG(a0), UBYTE *top BL_REG(a1),
                                                UBYTE *bot BL_REG(a2))
{
    UWORD n = SW(top, 4) - 1;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = 0;
    W(BLTDMOD) = 0;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = SL(top, 14);
    L(BLTCPT) = SL(bot, 14);
    L(BLTDPT) = SL(dest, 14);
    cookie = SL(top, 18);
    goto first;
    do {
        BLITWAIT();
first:
        L(BLTAPT) = cookie;
        W(BLTSIZE) = MYBLITSIZE;
    } while (n-- != 0);
}

/* BlockToShape: height*depth words of the shape's data to dest's */
static void BLITZ_ROUTINE blocktoshape_c(UBYTE *shape BL_REG(a1), UBYTE *dest BL_REG(a2))
{
    BLITWAIT();
    L(BLTCON0) = 0x09f00000;                    /* D = A */
    W(BLTAMOD) = 0;
    W(BLTDMOD) = 0;
    L(BLTAFWM) = 0xffffffff;
    L(BLTAPT) = SL(shape, 14);
    L(BLTDPT) = SL(dest, 14);
    W(BLTSIZE) = (UWORD)((UWORD)(SW(shape, 2) * SW(shape, 4)) << 6) + 1;
}

/* Registers: the original changes d0-d7/a0-a3 (the routines differ) and
   keeps a4/a5; the thunks keep d0/d1/a0/a1 (PredatorBlit: d1/a0/a1), the
   C code d2-d7/a2-a6. */
BLITZ_THUNK(_specialblock, specialblock_c, "d0-d1/a0-a1");
BLITZ_THUNK(_specialblock2, specialblock2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_specialblockbitmap, specialblockbitmap_c, "d0-d1/a0-a1");
BLITZ_THUNK(_specialblockbitmap2, specialblockbitmap2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_tblock, tblock_c, "d0-d1/a0-a1");
BLITZ_THUNK(_predatorblit, predatorblit_c, "d1/a0-a1");
BLITZ_THUNK(_bpredatorblit, bpredatorblit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_MapInfo, mapinfo_c, "d0-d1/a0-a1");
BLITZ_THUNK(_createshape, createshape_c, "d0-d1/a0-a1");
BLITZ_THUNK(_specialblocktoshape, specialblocktoshape_c, "d0-d1/a0-a1");
BLITZ_THUNK(_blocktoshape, blocktoshape_c, "d0-d1/a0-a1");
BLITZ_THUNK(docolblit, docolblit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_tblockimage, tblockimage_c, "d0-d1/a0-a1");
BLITZ_THUNK(_btblockimage, btblockimage_c, "d0-d1/a0-a1");

/* --------------------------------------------- error checks (errors) */

BLITZ_ERRCHK_MESSAGE(diffsizes, "Shapes must be the same size!");
BLITZ_ERRCHK_MESSAGE(noshape, "Destination shape MUST be pre-reserved!");

/* BlockToShape: dest has data, both shapes the same size (+26) */
static void BLITZ_ERRCHK errblocktoshape_c(UBYTE *shape BL_REG(a1), UBYTE *dest BL_REG(a2))
{
    if (!SL(dest, 14))
        BLITZ_RUNERR(noshape);
    if (SW(shape, 26) != SW(dest, 26))
        BLITZ_RUNERR(diffsizes);
}

/* SpecialBlockToShape: the same for dest (a0) and the top shape */
static void BLITZ_ERRCHK errsblocktoshape_c(UBYTE *dest BL_REG(a0), UBYTE *top BL_REG(a1))
{
    if (!SL(dest, 14))
        BLITZ_RUNERR(noshape);
    if (SW(top, 26) != SW(dest, 26))
        BLITZ_RUNERR(diffsizes);
}

/* the original changes d0 (the low word: +26 of the shape); kept here */
BLITZ_ERRCHK_THUNK(_errBlockToShape, errblocktoshape_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errSBlockToShape, errsblocktoshape_c, "d0-d1/a0-a1");
