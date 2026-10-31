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

/**
 * @brief init: does nothing (RTS).
 */
void BLITZ_ROUTINE _libinit(void)
{
}

/**
 * @brief finit: does nothing (RTS).
 */
void BLITZ_ROUTINE _libfinit(void)
{
}

/* a shape object of shapeslib (32 bytes) */
struct shape {
    UWORD width;                /* +0 pixel width */
    UWORD height;               /* +2 */
    UWORD depth;                /* +4 */
    UWORD ebwidth;              /* +6 even byte width */
    UWORD blitsize;             /* +8 height<<6 | words+1 */
    UWORD xhandle, yhandle;     /* +10, +12 */
    ULONG data;                 /* +14 plane after plane, chip memory */
    ULONG cookie;               /* +18 OR of all planes, or 0 */
    UWORD onebpmem;             /* +22 bytes per plane */
    UWORD onebpmemx;            /* +24 per plane + one word per row */
    UWORD allbpmem;             /* +26 the whole shape */
    UWORD allbpmemx;            /* +28 whole + one word per row */
    UWORD cl;                   /* +30 "cL": the data is not the shape's */
} BL_PACKED;
static_assert(sizeof(struct shape) == 32);

/* a bitmap object of bitmaplib, as far as these routines read it: the
   original reads flags and depth (+4, +5) as one word */
struct bmap {
    UWORD bpr;                  /* +0 bytes per row */
    UWORD rows;                 /* +2 */
    UWORD depth;                /* +4.w */
    UWORD pad;                  /* +6 */
    UBYTE *planes[8];           /* +8 */
} BL_PACKED;
static_assert(sizeof(struct bmap) == 40);

/* a buffer object of bblitlib, as far as the buffered blits use it */
struct buffer {
    UBYTE *pos;                 /* +0 write position */
    UBYTE *last;                /* +4 last record (0: none) */
};

/* CreateShape copies two words of a shape as one long */
#define LONGS(d, s, m) (*(ULONG *)&(d)->m = *(const ULONG *)&(s)->m)

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

/**
 * @brief SetupMapTwo ebwidth: the bytes per line of the bitmaps.
 *
 * SpecialBlock, TBlock and the predator blits assume bitmaps of this
 * width (default 40); also sets the modulos ebwidth-2 and ebwidth-4.
 * @param w  d0: the bytes per line
 */
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

/**
 * @brief SpecialBlock topshape,bottomshape,offset: a 16x16 block of two
 * shapes into the used bitmap.
 *
 * The top shape (B) over the bottom shape (C) through top's cookie (A)
 * into each plane of the used bitmap at off (D = A ? B : C); B and C go
 * on from plane to plane.  Assumes 16x16 shapes and bitmaps of
 * SetupMapTwo's width.  BUG(orig) (#429): depth 0 loops 65536 times.
 * @param off  d2: the byte offset in the bitmap
 * @param bm   a0: the used bitmap object
 * @param top  a1: the top shape object (parameter 0)
 * @param bot  a2: the bottom shape object (parameter 1)
 */
static void BLITZ_ROUTINE specialblock2_c(UWORD off BL_REG(d2), struct bmap *bm BL_REG(a0),
                                          struct shape *top BL_REG(a1),
                                          struct shape *bot BL_REG(a2))
{
    UWORD n = top->depth - 1;
    UBYTE **plane = bm->planes;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;                    /* 4 channels, D = A ? B : C */
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = 0;
    W(BLTDMOD) = modwidth;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = top->data;
    L(BLTCPT) = bot->data;
    cookie = top->cookie;
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

/**
 * @brief SpecialBlock topshape,bottomshape,x,y: as the offset form, at
 * y * SetupMapTwo's width + x/8.
 * @param x    d2: x (ASR 3: a byte column)
 * @param y    d3: y
 * @param bm   a0: the used bitmap object
 * @param top  a1: the top shape object
 * @param bot  a2: the bottom shape object
 */
static void BLITZ_ROUTINE specialblock_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                         struct bmap *bm BL_REG(a0), struct shape *top BL_REG(a1),
                                         struct shape *bot BL_REG(a2))
{
    specialblock2_c(xyoff(x, y, ebwidth), bm, top, bot);
}

/* ------------------------------------------------- SpecialBlockBitmap */

/**
 * @brief SpecialBlockBitmap topshape,bitmap,offset: a shape over another
 * bitmap into the used bitmap.
 *
 * The shape (B, any size) over the same area of the source bitmap (C)
 * through its cookie (A) into the used bitmap, both at off; B goes on
 * from plane to plane.  BUG(orig) (#429): depth 0 loops 65536 times.
 * @param off    d2: the byte offset in both bitmaps
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object (parameter 0)
 * @param src    a2: the source bitmap object (parameter 1)
 */
static void BLITZ_ROUTINE specialblockbitmap2_c(UWORD off BL_REG(d2),
                                                struct bmap *bm BL_REG(a0),
                                                struct shape *shape BL_REG(a1),
                                                struct bmap *src BL_REG(a2))
{
    UWORD n = shape->depth - 1;
    UWORD mod = bm->bpr - shape->ebwidth;
    UWORD size = (UWORD)(shape->ebwidth >> 1) + (UWORD)(shape->height << 6);
    UBYTE **plane = bm->planes, **splane = src->planes;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = mod;
    W(BLTDMOD) = mod;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = shape->data;
    cookie = shape->cookie;
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

/**
 * @brief SpecialBlockBitmap topshape,bitmap,x,y: as the offset form, at
 * y * the used bitmap's width + x/8.
 * @param x      d2: x (ASR 3)
 * @param y      d3: y
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object
 * @param src    a2: the source bitmap object
 */
static void BLITZ_ROUTINE specialblockbitmap_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                               struct bmap *bm BL_REG(a0),
                                               struct shape *shape BL_REG(a1),
                                               struct bmap *src BL_REG(a2))
{
    specialblockbitmap2_c(xyoff(x, y, bm->bpr), bm, shape, src);
}

/* ------------------------------------------------------------- TBlock */

/**
 * @brief TBlock shape,x,y: a 16x16 shape through its cookie onto the used
 * bitmap.
 *
 * B = the shape, A = its cookie, C = D = the bitmap at y * SetupMapTwo's
 * width + x/8; B goes on from plane to plane.  BUG(orig) (#429): depth 0
 * loops 65536 times.
 * @param x      d1: x (ASR 3)
 * @param y      d2: y
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object
 */
static void BLITZ_ROUTINE tblock_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                   struct bmap *bm BL_REG(a0), struct shape *shape BL_REG(a1))
{
    ULONG off = xyoff(x, y, ebwidth);
    UWORD n = shape->depth - 1;
    UBYTE **plane = bm->planes;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = modwidth;
    W(BLTDMOD) = modwidth;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = shape->data;
    cookie = shape->cookie;
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

/**
 * @brief PredatorBlit(shape,x,y): the "predator" effect inside a shape's
 * cookie.
 *
 * B = the bitmap one line (SetupMapTwo's width) further, shifted by x
 * and one pixel more, C = D = the bitmap, 2 words wide at the even word,
 * through the cookie (A, shifted, last word masked).  Assumes 16x16
 * shapes.  BUG(orig) (#429): depth 0 loops 65536 times.
 * @param x      d1: x
 * @param y      d2: y
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object
 * @return d0: the address of the shape's cookie
 */
static ULONG BLITZ_ROUTINE predatorblit_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                          struct bmap *bm BL_REG(a0),
                                          struct shape *shape BL_REG(a1))
{
    ULONG con = 0x0fca1000UL | (ULONG)(x & 15) << 28;
    ULONG off = (UWORD)((UWORD)((ULONG)y * ebwidth) + (UWORD)(((WORD)x >> 3) & 0xfffe));
    UWORD n = shape->depth - 1;
    UBYTE **plane = bm->planes;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = con;
    W(BLTAMOD) = (UWORD)-2;
    W(BLTBMOD) = modwidth2;
    W(BLTCMOD) = modwidth2;
    W(BLTDMOD) = modwidth2;
    L(BLTAFWM) = 0xffff0000;
    cookie = shape->cookie;
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

static void bsave(struct bsave *s, struct buffer *buf, struct bmap *bm, UBYTE *cb, WORD x, WORD y,
                  UWORD mod, UWORD size, UWORD save)
{
    UBYTE *rec = buf->pos;
    UWORD n;
    UBYTE **plane;

    *(UBYTE **)rec = buf->last;                /* link the record */
    buf->last = rec;
    rec += 4;
    s->admod = 0xfffe0000UL | mod;
    *(UWORD *)rec = mod;
    rec += 2;
    s->cbmod = s->admod << 16 | s->admod >> 16;
    s->off = (ULONG)bl_muls_w(y, bm->bpr) + (LONG)(WORD)(x >> 3);
    s->size = size;
    *(UWORD *)rec = size;
    rec += 2;
    n = bm->depth - 1;
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
    plane = bm->planes;
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
    buf->pos = rec;
}

/**
 * @brief BPredatorBlit buffer,shape,x,y: PredatorBlit, buffered.
 *
 * The shape's area (at x,y minus the handle) is saved into a record of
 * the buffer as bblitlib's BBlit does, with a blit size one line less
 * (deliberate: ";!!! BB Cludge!"), then PredatorBlit's blit through the
 * shifted cookie (B: the line ebwidth below, one pixel more).
 * BUG(orig) (#426): B reads the line SetupMapTwo's width below, not the
 * bitmap's.  BUG(orig) (#429): depth 0 loops 65536 times.
 * @param x      d2: x
 * @param y      d3: y
 * @param buf    a0: the buffer object (bblitlib)
 * @param cb     a1: the chip base (chipbaselib)
 * @param bm     a2: the used bitmap object
 * @param shape  a3: the shape object
 */
static void BLITZ_ROUTINE bpredatorblit_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                          struct buffer *buf BL_REG(a0), UBYTE *cb BL_REG(a1),
                                          struct bmap *bm BL_REG(a2),
                                          struct shape *shape BL_REG(a3))
{
    struct bsave s;
    UWORD shift, n;
    UBYTE **plane;
    ULONG cookie;

    x -= shape->xhandle;
    y -= shape->yhandle;
    bsave(&s, buf, bm, cb, x, y, (UWORD)(bm->bpr - shape->ebwidth - 2),
          (UWORD)(shape->blitsize - 64), shape->onebpmemx);
    shift = (UWORD)((x & 15) << 12);
    n = shape->depth - 1;
    plane = bm->planes;
    BL_BLITWAIT(cb);
    L(BLTCMOD) = s.cbmod;                       /* MOVEM.l d4-d5,BLTCMOD */
    L(BLTAMOD) = s.admod;
    W(BLTBMOD) = (UWORD)s.admod;
    W(BLTCON1) = 0x1000;
    W(BLTCON0) = shift | 0xfca;
    CW(BLTALWM) = 0;
    cookie = shape->cookie;
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

/**
 * @brief BColourBlit buffer,shape,x,y,colour: a shape's cookie in one
 * colour, buffered.
 *
 * The area is saved as BBlit does, then the shifted cookie is blitted in
 * `colour`: per plane D = A | C (bit set) or ~A & C.  Sets the B modulos
 * it does not use and shifts the colour as a byte (at most 8 planes):
 * original behaviour.  BUG(orig) (#429): depth 0 loops 65536 times.
 * @param x       d2: x
 * @param y       d3: y
 * @param colour  d4: the colour
 * @param buf     a0: the buffer object
 * @param cb      a1: the chip base
 * @param bm      a2: the used bitmap object
 * @param shape   a3: the shape object
 */
static void BLITZ_ROUTINE docolblit_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                      UWORD colour BL_REG(d4), struct buffer *buf BL_REG(a0),
                                      UBYTE *cb BL_REG(a1), struct bmap *bm BL_REG(a2),
                                      struct shape *shape BL_REG(a3))
{
    struct bsave s;
    UWORD con, n;
    UBYTE bits;
    UBYTE **plane;
    ULONG cookie;

    coltoblit = colour;
    x -= shape->xhandle;
    y -= shape->yhandle;
    bsave(&s, buf, bm, cb, x, y, (UWORD)(bm->bpr - shape->ebwidth - 2), shape->blitsize,
          shape->onebpmemx);
    con = (UWORD)((x & 15) << 12) | 0xb00;      /* A, C, D */
    n = shape->depth - 1;
    plane = bm->planes;
    BL_BLITWAIT(cb);
    L(BLTCMOD) = s.cbmod;
    L(BLTAMOD) = s.admod;
    W(BLTCON1) = 0;
    CW(BLTALWM) = 0;
    cookie = shape->cookie;
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

/**
 * @brief TBlockImage image_addr,image,x,y: a 16x16 image of 6 planes
 * through its mask.
 *
 * Image `image` of image_addr (224 bytes: the mask, then 6 planes of
 * 16x16) through its mask onto 6 planes of the used bitmap.
 * @param addr   d0: image_addr
 * @param image  d1: the image number
 * @param x      d2: x (ASR 3)
 * @param y      d3: y
 * @param bm     a0: the used bitmap object
 */
static void BLITZ_ROUTINE tblockimage_c(ULONG addr BL_REG(d0), UWORD image BL_REG(d1),
                                        UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                        struct bmap *bm BL_REG(a0))
{
    UWORD width = bm->bpr;
    ULONG off = xyoff(x, y, width);
    UWORD n = DEPTH - 1;
    UBYTE **plane = bm->planes;

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

/**
 * @brief BTBlockImage buffer,image_addr,x,y,image: TBlockImage, buffered
 * and shifted.
 *
 * The area is saved as BBlit does, then the image shifted through its
 * mask.  BUG(orig) (#427): image*224 is added as a word (from image 147
 * on before image_addr).
 * @param addr   d1: image_addr
 * @param x      d2: x
 * @param y      d3: y
 * @param image  d4: the image number
 * @param buf    a0: the buffer object
 * @param cb     a1: the chip base
 * @param bm     a2: the used bitmap object
 */
static void BLITZ_ROUTINE btblockimage_c(ULONG addr BL_REG(d1), UWORD x BL_REG(d2),
                                         UWORD y BL_REG(d3), UWORD image BL_REG(d4),
                                         struct buffer *buf BL_REG(a0), UBYTE *cb BL_REG(a1),
                                         struct bmap *bm BL_REG(a2))
{
    struct bsave s;
    UWORD shift, n = SHAPE_DEPTH - 1;
    UBYTE **plane;
    ULONG mask;

    addr += (WORD)(UWORD)((ULONG)image * SHAPE_BYTESIZE);     /* ADD.w */
    bsave(&s, buf, bm, cb, x, y, (UWORD)(bm->bpr - SHAPE_EBWIDTH - 2), SHAPE_BLITSIZE,
          SHAPE_ONEBPMEMX);
    shift = (UWORD)((x & 15) << 12);
    plane = bm->planes;
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

/**
 * @brief CreateShape start_shape,count,chipmemblock,like_shape: shape
 * objects whose data lie in a given block.
 *
 * count shape objects from start_shape on like like_shape, their data at
 * chipmemblock one after another (allbpmem, +26, of the like shape
 * each), no cookie, "cL" (the data is not the shapes').
 * BUG(orig) (#428): count 0 creates 65536 shapes.
 * @param count  d1: the number of shapes
 * @param mem    d2: the chip memory block
 * @param shape  a2: the first shape object
 * @param like   a3: the shape to copy the sizes from
 */
static void BLITZ_ROUTINE createshape_c(UWORD count BL_REG(d1), ULONG mem BL_REG(d2),
                                        struct shape *shape BL_REG(a2),
                                        struct shape *like BL_REG(a3))
{
    UWORD n = count - 1;
    ULONG all = like->allbpmem;

    do {
        LONGS(shape, like, width);         /* MOVE.l: width, height */
        LONGS(shape, like, depth);         /* depth, ebwidth */
        LONGS(shape, like, blitsize);      /* blitsize, xhandle */
        shape->yhandle = like->yhandle;
        shape->data = mem;
        shape->cookie = 0;
        LONGS(shape, like, onebpmem);      /* onebpmem, onebpmemx */
        LONGS(shape, like, allbpmem);      /* allbpmem, allbpmemx */
        shape->cl = 0x634c;                 /* "cL" */
        shape++;
        mem += all;
    } while (n-- != 0);
}

/**
 * @brief SpecialBlockToShape topshape,bottomshape,destshape: SpecialBlock
 * into a shape.
 *
 * The 16x16 block (top over bottom through top's cookie) into the data
 * of dest.  Error check: errsblocktoshape_c.  BUG(orig) (#429): depth 0
 * loops 65536 times.
 * @param dest  a0: the destination shape object
 * @param top   a1: the top shape object
 * @param bot   a2: the bottom shape object
 */
static void BLITZ_ROUTINE specialblocktoshape_c(struct shape *dest BL_REG(a0),
                                                struct shape *top BL_REG(a1),
                                                struct shape *bot BL_REG(a2))
{
    UWORD n = top->depth - 1;
    ULONG cookie;

    BLITWAIT();
    L(BLTCON0) = 0x0fca0000;
    W(BLTAMOD) = 0;
    W(BLTBMOD) = 0;
    W(BLTCMOD) = 0;
    W(BLTDMOD) = 0;
    L(BLTAFWM) = 0xffffffff;
    L(BLTBPT) = top->data;
    L(BLTCPT) = bot->data;
    L(BLTDPT) = dest->data;
    cookie = top->cookie;
    goto first;
    do {
        BLITWAIT();
first:
        L(BLTAPT) = cookie;
        W(BLTSIZE) = MYBLITSIZE;
    } while (n-- != 0);
}

/**
 * @brief BlockToShape shape,destshape: copies a shape's data into another.
 *
 * height*depth words of the shape's data to dest's: BUG(orig) (#425),
 * one word per line.  Error check: errblocktoshape_c.
 * @param shape  a1: the source shape object
 * @param dest   a2: the destination shape object
 */
static void BLITZ_ROUTINE blocktoshape_c(struct shape *shape BL_REG(a1),
                                         struct shape *dest BL_REG(a2))
{
    BLITWAIT();
    L(BLTCON0) = 0x09f00000;                    /* D = A */
    W(BLTAMOD) = 0;
    W(BLTDMOD) = 0;
    L(BLTAFWM) = 0xffffffff;
    L(BLTAPT) = shape->data;
    L(BLTDPT) = dest->data;
    W(BLTSIZE) = (UWORD)((UWORD)(shape->height * shape->depth) << 6) + 1;
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

/**
 * @brief BlockToShape's error check: dest has data, both shapes the same
 * size (allbpmem, +26).
 *
 * Runtime errors "Destination shape MUST be pre-reserved!" and "Shapes
 * must be the same size!".
 * @param shape  a1: the source shape object
 * @param dest   a2: the destination shape object
 */
static void BLITZ_ERRCHK errblocktoshape_c(struct shape *shape BL_REG(a1),
                                           struct shape *dest BL_REG(a2))
{
    if (!dest->data)
        BLITZ_RUNERR(noshape);
    if (shape->allbpmem != dest->allbpmem)
        BLITZ_RUNERR(diffsizes);
}

/**
 * @brief SpecialBlockToShape's error check: the same for dest and the top
 * shape.
 * @param dest  a0: the destination shape object
 * @param top   a1: the top shape object
 */
static void BLITZ_ERRCHK errsblocktoshape_c(struct shape *dest BL_REG(a0),
                                            struct shape *top BL_REG(a1))
{
    if (!dest->data)
        BLITZ_RUNERR(noshape);
    if (top->allbpmem != dest->allbpmem)
        BLITZ_RUNERR(diffsizes);
}

/* the original changes d0 (the low word: +26 of the shape); kept here */
BLITZ_ERRCHK_THUNK(_errBlockToShape, errblocktoshape_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errSBlockToShape, errsblocktoshape_c, "d0-d1/a0-a1");
