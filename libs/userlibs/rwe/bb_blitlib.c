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
 * left them.  init and finit are RTS.
 *
 * Speed (at or below the original's cycles per call, perf bbblit): the
 * entries save no register (BLITZ_ENTRYPOINT, a6 kept by
 * BLITZ_EP_KEEP_A6), write the blitter registers through the chip base
 * in an address register, and their plane loops are the original's
 * assembler loops (asm blocks); the buffered blits (BPredatorBlit,
 * BColourBlit, BTBlockImage) are assembler routines.
 *
 * Fixed (intended deviations):
 *   - BlockToShape blits the shape's real width; the original copied one
 *     word per line (#425).  Its error check refuses a shape larger than
 *     one blit.
 *   - The blits that take a shape and had no error check (SpecialBlock,
 *     SpecialBlockBitmap, TBlock, PredatorBlit, BPredatorBlit,
 *     BColourBlit) and SpecialBlockToShape's check refuse a shape of
 *     depth 0 (never defined) with "Uninitialized Shape"; their plane
 *     loops (DBRA on depth-1) run 65536 times for it, in the release
 *     code as in the original (#429).  A bitmap of depth 0 cannot be
 *     made with runtime error checking (bitmaplib's checks).
 *   - CreateShape has an error check: count 0, which makes the release
 *     code create 65536 shapes, is the runtime error "CreateShape count
 *     must be at least 1!"; the original had no check (#428).  So is a
 *     count that runs past the shape table (start_shape + count above
 *     Maximum Shapes): "CreateShape start+count above Maximum Shapes!"
 *     (#1077).  The check gets the table and the maximum in a0 and d4.
 *   - BTBlockImage adds image*224 as a long, as TBlockImage does; the
 *     original added it as a word (ADD.w), images from 147 on were read
 *     before image_addr (#427).
 *   - BPredatorBlit's B channel reads the line below with the bitmap's
 *     bytes per row, as the rest of the routine; the original added
 *     SetupMapTwo's width (#426).
 * Original behaviour, not defects: SpecialBlock, TBlock and PredatorBlit
 * assume 16x16 shapes and bitmaps of SetupMapTwo's width (the map format
 * they are written for); BPredatorBlit stores a blit size one line less
 * in the record (deliberate, ";!!! BB Cludge!" in the source); BColourBlit
 * sets the modulos of the B channel it does not use (B is off) and shifts
 * its colour as a byte (LSR.b; at most 8 planes).
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (aliases of the C functions, below) */
void _specialblock(void), _specialblock2(void), _specialblockbitmap(void);
void _specialblockbitmap2(void), _tblock(void), _predatorblit(void), _bpredatorblit(void);
void _MapInfo(void), _createshape(void), _specialblocktoshape(void), _blocktoshape(void);
void docolblit(void), _tblockimage(void), _btblockimage(void);
void _errSBlockToShape(void), _errBlockToShape(void), _errCreateShape(void);
void _errShapeA1(void), _errShapeA3(void);
static void _libinit(void);
static void _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* the !libs of the buffered blits (as bblitlib's BBlit) */
#define BUFLIBS LIB_bblitlib, BL_IA0|BL_PD0, LIB_bitmaplib, BL_UA2, LIB_chipbaselib, BL_LA1, \
                LIB_graphicslib, BL_IFAMIGA|BL_LA6

/* !libheader {19,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_bb_blitlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1);
BLITZ_SUBS(_specialblock, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1);
BLITZ_SUBS(_specialblock2, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("SpecialBlock", "topshape,bottomshape,(x,y)|offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0, LIB_bitmaplib, BL_IA2|BL_PD1);
BLITZ_SUBS(_specialblockbitmap, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0, LIB_bitmaplib, BL_IA2|BL_PD1);
BLITZ_SUBS(_specialblockbitmap2, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("SpecialBlockBitmap", "topshape,bitmap,(x,y)|offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0);
BLITZ_SUBS(_tblock, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("TBlock", "shape,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0, LIB_shapeslib, BL_IA1|BL_PD0);
BLITZ_SUBS(_predatorblit, _errShapeA1, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("PredatorBlit", "shape,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD1, BUFLIBS);
BLITZ_SUBS(_bpredatorblit, _errShapeA3, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("BPredatorBlit", "buffer,shape,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_MapInfo, 0, 0);
BLITZ_NAME("SetupMapTwo", "ebwidth");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_WORD);
/* Fixed (#1077): + a0 = the shape table, d4 = Maximum Shapes, for the check */
BLITZ_LIBS(LIB_shapeslib, BL_IA2|BL_PD0, LIB_shapeslib, BL_IA3|BL_PD3, LIB_shapeslib, BL_BA0,
           LIB_shapeslib, BL_MD4);
BLITZ_SUBS(_createshape, _errCreateShape, 0);  /* Fixed (#428, #1077): the original has no check */
BLITZ_NAME("CreateShape", "start_shape,count,chipmemblock,like_shape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA1|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1, LIB_shapeslib, BL_IA0|BL_PD2);
BLITZ_SUBS(_specialblocktoshape, _errSBlockToShape, 0);
BLITZ_NAME("SpecialBlockToShape", "topshape,bottomshape,destshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA1|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1);
BLITZ_SUBS(_blocktoshape, _errBlockToShape, 0);
BLITZ_NAME("BlockToShape", "shape,destshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD1, BUFLIBS);
BLITZ_SUBS(docolblit, _errShapeA3, 0);  /* Fixed (#429): the original has no check */
BLITZ_NAME("BColourBlit", "buffer,shape,x,y,colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0);
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
static void BLITZ_ROUTINE _libinit(void)
{
}

/**
 * @brief finit: does nothing (RTS).
 */
static void BLITZ_ROUTINE _libfinit(void)
{
}

/* the shape (shapeslib) and the bitmap (bitmaplib) objects: struct bb_shape,
   struct bb_bitmap (blitzobj.h); the original reads the bitmap's flags and
   depth (+4, +5) as one word (flagsdepth); the shape's data and cookie
   addresses are taken as the longs the blitter registers get.  A buffer
   object of bblitlib (the buffered blits, assembler): +0 the write
   position, +4 the last record (0: none). */

/* the blitter registers through the chip base: the buffered routines get
   it in a1 (#chipbaselib), the others load $DFF000 into an address
   register (HIDE_A: GCC would fold it into absolute addresses again); a
   register write as d16(An) takes 4 cycles less than the original's
   absolute address */
#define CUSTOM ((UBYTE *)0xdff000)
#define CW(o) BL_CUSTOMW(cb, o)
#define CL(o) BL_CUSTOML(cb, o)
#define HIDE_A(p) __asm__("" : "+a"(p))
/* The plane loops are the original's assembler loops (in C, GCC tested
   the counter and branched twice per plane instead of one DBF): the
   first plane enters behind the blitter wait, every further plane waits
   (BTST once for the Agnus bug, then while busy) and ends with DBF on
   %[n].  The body between them is the routine's. */
#define PLANELOOP_START                                                     \
    "   bra.s   1f\n"                                                       \
    "0: tst.b   2(%[cb])\n"                                                 \
    "2: btst    #6,2(%[cb])\n"                                              \
    "   bne.s   2b\n"                                                       \
    "1:\n"
#define PLANELOOP_END "   dbf     %[n],0b"
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

/* SetupMapTwo's values, one after another as in the original (there a
   pad word in front: ADD.l ebwidth-2(pc) adds the long pad:ebwidth, i.e.
   ebwidth) */
static struct {
    UWORD ebwidth, modwidth, modwidth2;
} map = { 40, 38, 36 };
static UWORD coltoblit __attribute__((used));   /* BColourBlit (asm) */

/**
 * @brief SetupMapTwo ebwidth: the bytes per line of the bitmaps.
 *
 * SpecialBlock, TBlock and the predator blits assume bitmaps of this
 * width (default 40); also sets the modulos ebwidth-2 and ebwidth-4.
 * The three words through an address register (MOVE.w (a0)+, 8 cycles
 * against the original's 16 for an absolute address).
 * @param w  d0: the bytes per line
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE mapinfo_c(UWORD w BL_REG(d0))
{
    UWORD *p = &map.ebwidth;

#define PUT(v) do { *p++ = (v); HIDE_A(p); } while (0)
    HIDE_A(p);
    PUT(w);
    w -= 2;
    PUT(w);
    w -= 2;
    PUT(w);
#undef PUT
}

/* (x,y) to an offset in the bitmap: MULU.w by the width, x ASR 3, a word
   sum */
BL_ALWAYS_INLINE UWORD xyoff(UWORD x, UWORD y, UWORD width)
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
 * SetupMapTwo's width.
 * Depth 0 loops 65536 times: Fixed (#429), errshapea1_c refuses it.
 * @param off  d2: the byte offset in the bitmap
 * @param bm   a0: the used bitmap object
 * @param top  a1: the top shape object (parameter 0)
 * @param bot  a2: the bottom shape object (parameter 1)
 */
BL_ALWAYS_INLINE void specialblock(UWORD off, struct bb_bitmap *bm, struct bb_shape *top,
                                   struct bb_shape *bot)
{
    UWORD n = top->depth - 1;
    UBYTE **plane = bm->planes;
    ULONG tdata = (ULONG)top->data, bdata = (ULONG)bot->data, cookie = (ULONG)top->cookie, d;
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x0fca0000;                   /* 4 channels, D = A ? B : C */
    CW(BLTAMOD) = 0;
    CW(BLTBMOD) = 0;
    CW(BLTCMOD) = 0;
    CW(BLTDMOD) = map.modwidth;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTBPT) = tdata;
    CL(BLTCPT) = bdata;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  (%[pl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT */
        "   move.l  %[d],0x54(%[cb])\n"             /* BLTDPT */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [pl] "+a"(plane), [n] "+d"(n), [d] "=&d"(d)
        : [cb] "a"(cb), [off] "d"((ULONG)off), [ck] "d"(cookie), [sz] "i"(MYBLITSIZE)
        : "cc", "memory");
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
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE specialblock_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                         struct bb_bitmap *bm BL_REG(a0), struct bb_shape *top BL_REG(a1),
                                         struct bb_shape *bot BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    specialblock(xyoff(x, y, map.ebwidth), bm, top, bot);
    BLITZ_EP_KEEP_A6_END();
}

/**
 * @brief SpecialBlock topshape,bottomshape,offset: the entry of the
 * offset form (specialblock() above; d2 the offset, a0-a2 as there).
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE specialblock2_c(UWORD off BL_REG(d2), struct bb_bitmap *bm BL_REG(a0),
                                          struct bb_shape *top BL_REG(a1),
                                          struct bb_shape *bot BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    specialblock(off, bm, top, bot);
    BLITZ_EP_KEEP_A6_END();
}

/* ------------------------------------------------- SpecialBlockBitmap */

/**
 * @brief SpecialBlockBitmap topshape,bitmap,offset: a shape over another
 * bitmap into the used bitmap.
 *
 * The shape (B, any size) over the same area of the source bitmap (C)
 * through its cookie (A) into the used bitmap, both at off; B goes on
 * from plane to plane.  Depth 0 loops 65536 times: Fixed (#429),
 * errshapea1_c refuses it.
 * @param off    d2: the byte offset in both bitmaps
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object (parameter 0)
 * @param src    a2: the source bitmap object (parameter 1)
 */
BL_ALWAYS_INLINE void specialblockbitmap(UWORD off, struct bb_bitmap *bm, struct bb_shape *shape,
                                         struct bb_bitmap *src)
{
    UWORD n = shape->depth - 1;
    UWORD mod = bm->bpr - shape->ebwidth;
    UWORD size = (UWORD)(shape->ebwidth >> 1) + (UWORD)(shape->height << 6);
    UBYTE **plane = bm->planes, **splane = src->planes;
    ULONG data = (ULONG)shape->data, cookie = (ULONG)shape->cookie, d;
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x0fca0000;
    CW(BLTAMOD) = 0;
    CW(BLTBMOD) = 0;
    CW(BLTCMOD) = mod;
    CW(BLTDMOD) = mod;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTBPT) = data;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  (%[spl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[d],0x48(%[cb])\n"             /* BLTCPT */
        "   move.l  (%[pl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[d],0x54(%[cb])\n"             /* BLTDPT */
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [pl] "+a"(plane), [spl] "+a"(splane), [n] "+d"(n), [d] "=&d"(d)
        : [cb] "a"(cb), [off] "d"((ULONG)off), [ck] "d"(cookie), [sz] "d"(size)
        : "cc", "memory");
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
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE specialblockbitmap_c(UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                               struct bb_bitmap *bm BL_REG(a0),
                                               struct bb_shape *shape BL_REG(a1),
                                               struct bb_bitmap *src BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    specialblockbitmap(xyoff(x, y, bm->bpr), bm, shape, src);
    BLITZ_EP_KEEP_A6_END();
}

/**
 * @brief SpecialBlockBitmap topshape,bitmap,offset: the entry of the
 * offset form (specialblockbitmap() above; d2 the offset, a0-a2 as there).
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE specialblockbitmap2_c(UWORD off BL_REG(d2),
                                                struct bb_bitmap *bm BL_REG(a0),
                                                struct bb_shape *shape BL_REG(a1),
                                                struct bb_bitmap *src BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    specialblockbitmap(off, bm, shape, src);
    BLITZ_EP_KEEP_A6_END();
}

/* ------------------------------------------------------------- TBlock */

/**
 * @brief TBlock shape,x,y: a 16x16 shape through its cookie onto the used
 * bitmap.
 *
 * B = the shape, A = its cookie, C = D = the bitmap at y * SetupMapTwo's
 * width + x/8; B goes on from plane to plane.  Depth 0
 * loops 65536 times: Fixed (#429), errshapea1_c refuses it.
 * @param x      d1: x (ASR 3)
 * @param y      d2: y
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE tblock_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                   struct bb_bitmap *bm BL_REG(a0), struct bb_shape *shape BL_REG(a1))
{
    BLITZ_EP_KEEP_A6();
    ULONG off = xyoff(x, y, map.ebwidth);
    UWORD n = shape->depth - 1, mod = map.modwidth;
    UBYTE **plane = bm->planes;
    ULONG data = (ULONG)shape->data, cookie = (ULONG)shape->cookie, d;
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x0fca0000;
    CW(BLTAMOD) = 0;
    CW(BLTBMOD) = 0;
    CW(BLTCMOD) = mod;
    CW(BLTDMOD) = mod;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTBPT) = data;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  (%[pl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[d],0x48(%[cb])\n"             /* BLTCPT */
        "   move.l  %[d],0x54(%[cb])\n"             /* BLTDPT */
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [pl] "+a"(plane), [n] "+d"(n), [d] "=&d"(d)
        : [cb] "a"(cb), [off] "d"(off), [ck] "d"(cookie), [sz] "i"(MYBLITSIZE)
        : "cc", "memory");
    BLITZ_EP_KEEP_A6_END();
}

/* ------------------------------------------------------- PredatorBlit */

/**
 * @brief PredatorBlit(shape,x,y): the "predator" effect inside a shape's
 * cookie.
 *
 * B = the bitmap one line (SetupMapTwo's width) further, shifted by x
 * and one pixel more, C = D = the bitmap, 2 words wide at the even word,
 * through the cookie (A, shifted, last word masked).  Assumes 16x16
 * shapes.  Depth 0 loops 65536 times: Fixed (#429), errshapea1_c
 * refuses it.
 * @param x      d1: x
 * @param y      d2: y
 * @param bm     a0: the used bitmap object
 * @param shape  a1: the shape object
 * @return d0: the address of the shape's cookie
 */
static BLITZ_ENTRYPOINT ULONG BLITZ_ROUTINE predatorblit_c(UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                          struct bb_bitmap *bm BL_REG(a0),
                                          struct bb_shape *shape BL_REG(a1))
{
    BLITZ_EP_KEEP_A6();
    ULONG sh = x & 15;
    ULONG con = 0x0fca1000UL | (sh >> 4 | sh << 28);       /* ROR.l #4 */
    ULONG off = (UWORD)((UWORD)((ULONG)y * map.ebwidth) + (UWORD)(((WORD)x >> 3) & 0xfffe));
    UWORD n = shape->depth - 1, mod = map.modwidth2;
    UBYTE **plane = bm->planes;
    ULONG cookie = (ULONG)shape->cookie, w = map.ebwidth, d;
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = con;
    CW(BLTAMOD) = (UWORD)-2;
    CW(BLTBMOD) = mod;
    CW(BLTCMOD) = mod;
    CW(BLTDMOD) = mod;
    CL(BLTAFWM) = 0xffff0000;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  (%[pl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[d],0x48(%[cb])\n"             /* BLTCPT */
        "   move.l  %[d],0x54(%[cb])\n"             /* BLTDPT */
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT */
        "   add.l   %[w],%[d]\n"
        "   move.l  %[d],0x4c(%[cb])\n"             /* BLTBPT: the line below */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [pl] "+a"(plane), [n] "+d"(n), [d] "=&d"(d)
        : [cb] "a"(cb), [off] "d"(off), [ck] "d"(cookie), [w] "d"(w), [sz] "i"(MYBLITSIZESCROLL)
        : "cc", "memory");
    BLITZ_EP_KEEP_A6_END();
    return cookie;
}

/* ---------------------------------------------------- buffered blits */

/* The buffered blits are assembler routines (for speed: in C, GCC had
   too few address registers for the record, the planes, the bitmap, the
   shape and the chip base and kept values on the stack, also without
   saves: 6 to 8 % slower than the original).  They follow the original's code, with the fixes, and
   use the chip base in a1 for every register write (the original wrote
   the blit's registers as absolute addresses); a3 is free in a statement
   (the register rule), a4/a5 stay untouched (the original saved them).

   BSAVE(ebw, size, save): the save of the background into a record of
   the buffer, as bblitlib's BBlit: +0 previous record, +4.w modulo, +6.w
   blit size, +8.w bitmap planes-1, +10.w bytes per plane, +12 offset,
   per plane {address, data}.  In: d2 x, d3 y, a0 the buffer, a1 the
   chip base, a2 the bitmap, a3 kept (in d4 meanwhile); `ebw` the width
   (bytes) of the blit, `size` the code that loads the blit size into d6,
   `save` the bytes per plane in the record (a word, added signed as the
   original's ADD.w).  Out: d2 the shift << 12, d3 the offset, d5
   $FFFE:modulo, d6 the blit size, d0/d1/d4/d7/a0 changed, a2/a3 as
   given; the record linked, the buffer's write position behind it. */
#define BSAVE(ebw, size, save)                                              \
    "   move.l  (a0)+,d1\n"             /* the record: the write position */ \
    "   move.l  (a0),d0\n"              /* the last record */                \
    "   move.l  d1,(a0)\n"              /* the last record: this one */      \
    "   move.l  a0,-(sp)\n"                                                 \
    "   move.l  d1,a0\n"                                                    \
    "   move.l  d0,(a0)+\n"             /* +0 the previous record */         \
    "   moveq   #-2,d5\n"                                                   \
    "   swap    d5\n"                                                       \
    "   move.w  (a2),d5\n"                                                  \
    "   sub.w   " ebw ",d5\n"                                               \
    "   subq.w  #2,d5\n"                                                    \
    "   move.w  d5,(a0)+\n"             /* +4 the modulo */                  \
    "   move.w  d2,d0\n"                                                    \
    "   asr.w   #3,d0\n"                                                    \
    "   ext.l   d0\n"                                                       \
    "   muls.w  (a2),d3\n"                                                  \
    "   add.l   d0,d3\n"                /* the offset */                     \
    "   and.w   #15,d2\n"                                                   \
    "   ror.w   #4,d2\n"                /* the shift << 12 */                \
    size                                                                    \
    "   move.w  d6,(a0)+\n"             /* +6 the blit size */               \
    "   move.w  4(a2),d7\n"                                                 \
    "   subq.w  #1,d7\n"                                                    \
    "   move.w  d7,(a0)+\n"             /* +8 the bitmap's planes-1 */       \
    "   move.w  " save ",d1\n"                                              \
    "   move.w  d1,(a0)+\n"             /* +10 the bytes per plane */        \
    "   move.l  d3,(a0)+\n"             /* +12 the offset */                 \
    "   move.l  a3,d4\n"                                                    \
    "   lea     8(a2),a3\n"             /* the bitmap's planes */            \
    "   tst.b   2(a1)\n"                                                    \
    "0: btst    #6,2(a1)\n"                                                 \
    "   bne.s   0b\n"                                                       \
    "   move.w  d5,0x64(a1)\n"          /* BLTAMOD */                        \
    "   move.w  #0,0x66(a1)\n"          /* BLTDMOD */                        \
    "   move.l  #0x09f00000,0x40(a1)\n" /* BLTCON0/1: A -> D */              \
    "   moveq   #-1,d0\n"                                                   \
    "   move.l  d0,0x44(a1)\n"          /* BLTAFWM/BLTALWM */                \
    "   bra.s   1f\n"                                                       \
    "0: tst.b   2(a1)\n"                                                    \
    "2: btst    #6,2(a1)\n"                                                 \
    "   bne.s   2b\n"                                                       \
    "1: move.l  (a3)+,d0\n"                                                 \
    "   add.l   d3,d0\n"                                                    \
    "   move.l  d0,0x50(a1)\n"          /* BLTAPT: the plane */              \
    "   move.l  d0,(a0)+\n"             /* its address */                    \
    "   move.l  a0,0x54(a1)\n"          /* BLTDPT: the record */             \
    "   move.w  d6,0x58(a1)\n"          /* BLTSIZE */                        \
    "   add.w   d1,a0\n"                                                    \
    "   dbf     d7,0b\n"                                                    \
    "   move.l  (sp)+,a3\n"                                                 \
    "   move.l  a0,-(a3)\n"             /* the write position */             \
    "   move.l  d4,a3\n"
/* the blitter wait of a blit's set-up (BTST once for the Agnus bug) */
#define BWAIT                                                               \
    "   tst.b   2(a1)\n"                                                    \
    "0: btst    #6,2(a1)\n"                                                 \
    "   bne.s   0b\n"
/* the modulos of the blit: BLTCMOD and BLTDMOD the modulo, BLTBMOD and
   BLTAMOD -2 (the original's MOVEM.l d4-d5 with d4 = d5 swapped) */
#define BMODS                                                               \
    "   swap    d5\n"                                                       \
    "   move.l  d5,0x60(a1)\n"          /* BLTCMOD, BLTBMOD */               \
    "   swap    d5\n"                                                       \
    "   move.l  d5,0x64(a1)\n"          /* BLTAMOD, BLTDMOD */

/**
 * @brief BPredatorBlit buffer,shape,x,y: PredatorBlit, buffered.
 *
 * The shape's area (at x,y minus the handle) is saved into a record of
 * the buffer as bblitlib's BBlit does, with a blit size one line less
 * (deliberate: ";!!! BB Cludge!"), then PredatorBlit's blit through the
 * shifted cookie (B: the line below, one pixel more).  Fixed (#426): B
 * reads the line the bitmap's bytes per row below; the original added
 * SetupMapTwo's width.  Depth 0 loops 65536 times: Fixed (#429),
 * errshapea3_c refuses it.
 * In: d2 x, d3 y, a0 the buffer object (bblitlib), a1 the chip base
 * (chipbaselib), a2 the used bitmap object, a3 the shape object.
 */
BLITZ_ASM_ROUTINE(_bpredatorblit,
"   sub.w   10(a3),d2\n"                /* the handle */
"   sub.w   12(a3),d3\n"
BSAVE("6(a3)", "   move.w  8(a3),d6\n   sub.w   #64,d6\n", "24(a3)")
"   move.w  4(a3),d7\n"
"   subq.w  #1,d7\n"
"   moveq   #0,d4\n"
"   move.w  (a2),d4\n"                  /* Fixed (#426): was SetupMapTwo's width */
"   addq.l  #8,a2\n"
"   move.l  18(a3),d0\n"                /* the cookie */
BWAIT
BMODS
"   move.w  d5,0x62(a1)\n"              /* BLTBMOD: the modulo */
"   move.w  #0x1000,0x42(a1)\n"         /* BLTCON1: B one pixel more */
"   or.w    #0xfca,d2\n"
"   move.w  d2,0x40(a1)\n"              /* BLTCON0 */
"   move.w  #0,0x46(a1)\n"              /* BLTALWM */
"   bra.s   1f\n"
"0: tst.b   2(a1)\n"
"2: btst    #6,2(a1)\n"
"   bne.s   2b\n"
"1: move.l  d0,0x50(a1)\n"              /* BLTAPT: the cookie */
"   move.l  (a2)+,d1\n"
"   add.l   d3,d1\n"
"   move.l  d1,0x48(a1)\n"              /* BLTCPT */
"   move.l  d1,0x54(a1)\n"              /* BLTDPT */
"   add.l   d4,d1\n"
"   move.l  d1,0x4c(a1)\n"              /* BLTBPT: the line below */
"   move.w  d6,0x58(a1)\n"              /* BLTSIZE */
"   dbf     d7,0b\n"
"   rts");

/**
 * @brief BColourBlit buffer,shape,x,y,colour: a shape's cookie in one
 * colour, buffered.
 *
 * The area is saved as BBlit does, then the shifted cookie is blitted in
 * `colour`: per plane D = A | C (bit set) or ~A & C.  Sets the B modulos
 * it does not use and shifts the colour as a byte (at most 8 planes):
 * original behaviour.  Depth 0 loops 65536 times: Fixed (#429),
 * errshapea3_c refuses it.  The colour waits in coltoblit while the save
 * uses every data register, as in the original.
 * In: d2 x, d3 y, d4 the colour, a0 the buffer object, a1 the chip base,
 * a2 the used bitmap object, a3 the shape object.
 */
BLITZ_ASM_ROUTINE(docolblit,
"   move.w  d4,_coltoblit\n"
"   sub.w   10(a3),d2\n"                /* the handle */
"   sub.w   12(a3),d3\n"
BSAVE("6(a3)", "   move.w  8(a3),d6\n", "24(a3)")
"   move.w  4(a3),d7\n"
"   subq.w  #1,d7\n"
"   addq.l  #8,a2\n"
"   move.l  18(a3),d0\n"                /* the cookie */
"   or.w    #0xb00,d2\n"                /* A, C, D */
"   move.w  _coltoblit(pc),d4\n"
BWAIT
BMODS
"   move.w  #0,0x42(a1)\n"              /* BLTCON1 */
"   move.w  #0,0x46(a1)\n"              /* BLTALWM */
"   move.w  d2,d5\n"
"   or.w    #0xfa,d5\n"                 /* the bit set: D = A | C */
"   or.w    #0x0a,d2\n"                 /* clear: D = ~A & C */
"   bra.s   1f\n"
"0: tst.b   2(a1)\n"
"2: btst    #6,2(a1)\n"
"   bne.s   2b\n"
"1: lsr.b   #1,d4\n"
"   bcc.s   3f\n"
"   move.w  d5,0x40(a1)\n"              /* BLTCON0 */
"   bra.s   4f\n"
"3: move.w  d2,0x40(a1)\n"
"4: move.l  d0,0x50(a1)\n"              /* BLTAPT: the cookie */
"   move.l  (a2)+,d1\n"
"   add.l   d3,d1\n"
"   move.l  d1,0x54(a1)\n"              /* BLTDPT */
"   move.l  d1,0x48(a1)\n"              /* BLTCPT */
"   move.w  d6,0x58(a1)\n"              /* BLTSIZE */
"   dbf     d7,0b\n"
"   rts");

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
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE tblockimage_c(ULONG addr BL_REG(d0), UWORD image BL_REG(d1),
                                        UWORD x BL_REG(d2), UWORD y BL_REG(d3),
                                        struct bb_bitmap *bm BL_REG(a0))
{
    BLITZ_EP_KEEP_A6();
    UWORD width = bm->bpr;
    ULONG off = xyoff(x, y, width);
    UWORD n = DEPTH - 1, mod = width - 2;
    UBYTE **plane = bm->planes;
    UBYTE *cb = CUSTOM;
    ULONG d;

    addr += (ULONG)image * IMAGESIZE;
    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x0fca0000;
    CW(BLTAMOD) = 0;
    CW(BLTBMOD) = 0;
    CW(BLTCMOD) = mod;
    CW(BLTDMOD) = mod;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTBPT) = addr + IMAGEPLANESIZE;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  (%[pl])+,%[d]\n"
        "   add.l   %[off],%[d]\n"
        "   move.l  %[d],0x48(%[cb])\n"             /* BLTCPT */
        "   move.l  %[d],0x54(%[cb])\n"             /* BLTDPT */
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT: the mask */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [pl] "+a"(plane), [n] "+d"(n), [d] "=&d"(d)
        : [cb] "a"(cb), [off] "d"(off), [ck] "d"(addr), [sz] "i"(MYBLITSIZE)
        : "cc", "memory");
    BLITZ_EP_KEEP_A6_END();
}

/**
 * @brief BTBlockImage buffer,image_addr,x,y,image: TBlockImage, buffered
 * and shifted.
 *
 * The area is saved as BBlit does, then the image shifted through its
 * mask.  Fixed (#427): image*224 is added as a long; the original added
 * it as a word (ADD.w: from image 147 on before image_addr).  Assembler
 * like the other buffered blits.
 * In: d1 image_addr, d2 x, d3 y, d4 the image number, a0 the buffer
 * object, a1 the chip base, a2 the used bitmap object.
 */
BLITZ_ASM_ROUTINE(_btblockimage,
"   mulu.w  #" BL_STR(SHAPE_BYTESIZE) ",d4\n"
"   move.l  d1,a3\n"
"   add.l   d4,a3\n"                    /* Fixed (#427): ADD.l, was ADD.w */
BSAVE("#" BL_STR(SHAPE_EBWIDTH), "   move.w  #" BL_STR(SHAPE_BLITSIZE) ",d6\n",
      "#" BL_STR(SHAPE_ONEBPMEMX))
"   moveq   #" BL_STR(SHAPE_DEPTH) "-1,d7\n"
"   addq.l  #8,a2\n"
BWAIT
BMODS
"   move.w  d2,0x42(a1)\n"              /* BLTCON1: B shifted too */
"   or.w    #0xfca,d2\n"
"   move.w  d2,0x40(a1)\n"              /* BLTCON0 */
"   move.w  #0,0x46(a1)\n"              /* BLTALWM */
"   move.l  a3,d0\n"                    /* the mask */
"   lea     " BL_STR(SHAPE_PLANEBYTESIZE) "(a3),a3\n"
"   move.l  a3,0x4c(a1)\n"              /* BLTBPT: the planes */
"   bra.s   1f\n"
"0: tst.b   2(a1)\n"
"2: btst    #6,2(a1)\n"
"   bne.s   2b\n"
"1: move.l  d0,0x50(a1)\n"              /* BLTAPT: the mask */
"   move.l  (a2)+,d1\n"
"   add.l   d3,d1\n"
"   move.l  d1,0x48(a1)\n"              /* BLTCPT */
"   move.l  d1,0x54(a1)\n"              /* BLTDPT */
"   move.w  d6,0x58(a1)\n"              /* BLTSIZE */
"   dbf     d7,0b\n"
"   rts");

/* ------------------------------------------------------------ shapes */

/**
 * @brief CreateShape start_shape,count,chipmemblock,like_shape: shape
 * objects whose data lie in a given block.
 *
 * count shape objects from start_shape on like like_shape, their data at
 * chipmemblock one after another (size, +26, of the like shape
 * each), no cookie, "cL" (the data is not the shapes').
 * count 0 creates 65536 shapes (DBRA), a count past Maximum Shapes
 * writes behind the shape table; Fixed (#428, #1077): the error check
 * errcreateshape_c refuses both.
 * @param count  d1: the number of shapes
 * @param mem    d2: the chip memory block
 * @param shape  a2: the first shape object
 * @param like   a3: the shape to copy the sizes from
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE createshape_c(UWORD count BL_REG(d1), ULONG mem BL_REG(d2),
                                        struct bb_shape *shape BL_REG(a2),
                                        struct bb_shape *like BL_REG(a3))
{
    UWORD n = count - 1;
    ULONG all = like->size;
    UBYTE *d = (UBYTE *)shape;
    ULONG zero = 0;
    UWORD cludge = BB_CLUDGE;

    /* the fields one after another with MOVE (a2)+ (in C with offsets
       from the object, GCC's moves took 4 cycles more each), 0 and "cL"
       from registers (CLR.l (a2)+ takes 20 cycles, MOVE.l d,(a2)+ 12);
       the like shape is read anew for every shape, it may be one of them */
#define PUT(type, v) do { *(type *)d = (v); d += sizeof(type); HIDE_A(d); } while (0)
    __asm__("" : "+d"(zero), "+d"(cludge));
    do {
        PUT(ULONG, *(const ULONG *)&like->width);       /* width, height */
        PUT(ULONG, *(const ULONG *)&like->depth);       /* depth, ebwidth */
        PUT(ULONG, *(const ULONG *)&like->bltsize);     /* bltsize, handlex */
        PUT(UWORD, like->handley);
        PUT(ULONG, mem);                                /* data */
        PUT(ULONG, zero);                               /* cookie */
        PUT(ULONG, *(const ULONG *)&like->planesize);   /* planesize, xplanesize */
        PUT(ULONG, *(const ULONG *)&like->size);        /* size, xsize */
        PUT(UWORD, cludge);                             /* "cL" */
        mem += all;
    } while (n-- != 0);
#undef PUT
}

/**
 * @brief SpecialBlockToShape topshape,bottomshape,destshape: SpecialBlock
 * into a shape.
 *
 * The 16x16 block (top over bottom through top's cookie) into the data
 * of dest.  Error check: errsblocktoshape_c.  Depth 0 loops 65536
 * times: Fixed (#429), errsblocktoshape_c refuses it.
 * @param dest  a0: the destination shape object
 * @param top   a1: the top shape object
 * @param bot   a2: the bottom shape object
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE specialblocktoshape_c(struct bb_shape *dest BL_REG(a0),
                                                struct bb_shape *top BL_REG(a1),
                                                struct bb_shape *bot BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    UWORD n = top->depth - 1;
    ULONG tdata = (ULONG)top->data, bdata = (ULONG)bot->data, ddata = (ULONG)dest->data;
    ULONG cookie = (ULONG)top->cookie;
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x0fca0000;
    CW(BLTAMOD) = 0;
    CW(BLTBMOD) = 0;
    CW(BLTCMOD) = 0;
    CW(BLTDMOD) = 0;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTBPT) = tdata;
    CL(BLTCPT) = bdata;
    CL(BLTDPT) = ddata;
    __asm__ volatile(
        PLANELOOP_START
        "   move.l  %[ck],0x50(%[cb])\n"            /* BLTAPT */
        "   move.w  %[sz],0x58(%[cb])\n"            /* BLTSIZE */
        PLANELOOP_END
        : [n] "+d"(n)
        : [cb] "a"(cb), [ck] "d"(cookie), [sz] "i"(MYBLITSIZE)
        : "cc", "memory");
    BLITZ_EP_KEEP_A6_END();
}

/**
 * @brief BlockToShape shape,destshape: copies a shape's data into another.
 *
 * One blit of height*depth lines of ebwidth/2 words, modulos 0: the
 * whole data, straight through.  Fixed (#425): the original blitted one
 * word per line (`ADDQ.w #1`), so of a shape wider than 16 pixels only
 * the first 2/ebwidth of the data.  The width field is 6 bits (0 = 64
 * words), the height field 10 bits (0 = 1024 lines); larger shapes are
 * refused by errblocktoshape_c.
 * @param shape  a1: the source shape object
 * @param dest   a2: the destination shape object
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE blocktoshape_c(struct bb_shape *shape BL_REG(a1),
                                         struct bb_shape *dest BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();
    ULONG sdata = (ULONG)shape->data, ddata = (ULONG)dest->data;
    /* ebwidth is always even (words * 2); & 63 encodes 64 words as 0,
       which BLTSIZE reads as 64 (errblocktoshape_c refuses ebwidth > 128) */
    UWORD size = (UWORD)((UWORD)(shape->height * shape->depth) << 6)
                 | ((shape->ebwidth >> 1) & 63);
    UBYTE *cb = CUSTOM;

    HIDE_A(cb);
    BL_BLITWAIT_TST(cb);
    CL(BLTCON0) = 0x09f00000;                   /* D = A */
    CW(BLTAMOD) = 0;
    CW(BLTDMOD) = 0;
    CL(BLTAFWM) = 0xffffffff;
    CL(BLTAPT) = sdata;
    CL(BLTDPT) = ddata;
    CW(BLTSIZE) = size;
    BLITZ_EP_KEEP_A6_END();
}

/* Registers: every entry is a user-level statement (PredatorBlit a
   function), so the register rule (docs/libs/deviations.md, "Register
   preservation") asks it to keep a4-a7 and PredatorBlit also a3: the
   C entries are their C functions without saves (BLITZ_ENTRYPOINT,
   BLITZ_ALIAS_EP), the buffered blits assembler routines (above); they
   change d0-d7/a0-a2 and the assembler routines also a3, as the
   original, which changes d0-d7/a0-a3 (the routines differ) and keeps
   a4/a5/a6. */
BLITZ_ALIAS_EP(_specialblock, specialblock_c);
BLITZ_ALIAS_EP(_specialblock2, specialblock2_c);
BLITZ_ALIAS_EP(_specialblockbitmap, specialblockbitmap_c);
BLITZ_ALIAS_EP(_specialblockbitmap2, specialblockbitmap2_c);
BLITZ_ALIAS_EP(_tblock, tblock_c);
BLITZ_ALIAS_EP(_predatorblit, predatorblit_c);
BLITZ_ALIAS_EP(_MapInfo, mapinfo_c);
BLITZ_ALIAS_EP(_createshape, createshape_c);
BLITZ_ALIAS_EP(_specialblocktoshape, specialblocktoshape_c);
BLITZ_ALIAS_EP(_blocktoshape, blocktoshape_c);
BLITZ_ALIAS_EP(_tblockimage, tblockimage_c);

/* --------------------------------------------- error checks (errors) */

BLITZ_ERRCHK_MESSAGE(diffsizes, "Shapes must be the same size!");
BLITZ_ERRCHK_MESSAGE(noshape, "Destination shape MUST be pre-reserved!");
BLITZ_ERRCHK_MESSAGE(toobig, "Shape too large for BlockToShape!");

/**
 * @brief BlockToShape's error check: dest has data, both shapes the same
 * size (size, +26), and the shape fits one blit.
 *
 * Runtime errors "Destination shape MUST be pre-reserved!", "Shapes must
 * be the same size!" and, Fixed (#425), "Shape too large for
 * BlockToShape!" for more than 1024 lines (height*depth) or a width above
 * 64 words (1024 pixels): the blitter's limits for the one blit.
 * @param shape  a1: the source shape object
 * @param dest   a2: the destination shape object
 */
static void BLITZ_ERRCHK errblocktoshape_c(struct bb_shape *shape BL_REG(a1),
                                           struct bb_shape *dest BL_REG(a2))
{
    if (!dest->data)
        BLITZ_RUNERR(noshape);
    if (shape->size != dest->size)
        BLITZ_RUNERR(diffsizes);
    if ((ULONG)shape->height * shape->depth > 1024 || shape->ebwidth > 128)
        BLITZ_RUNERR(toobig);
}

BLITZ_ERRCHK_MESSAGE(noinit, "Uninitialized Shape");

/**
 * @brief SpecialBlockToShape's error check: the same for dest and the top
 * shape, then the top shape's depth.
 *
 * Fixed (#429): a top shape of depth 0 (a shape never defined, whose size
 * is that of dest) is the runtime error "Uninitialized Shape"; the
 * original does not test it, and the blit loop runs 65536 times.
 * @param dest  a0: the destination shape object
 * @param top   a1: the top shape object
 */
static void BLITZ_ERRCHK errsblocktoshape_c(struct bb_shape *dest BL_REG(a0),
                                            struct bb_shape *top BL_REG(a1))
{
    if (!dest->data)
        BLITZ_RUNERR(noshape);
    if (top->size != dest->size)
        BLITZ_RUNERR(diffsizes);
    else if (!top->depth)
        BLITZ_RUNERR(noinit);
}

/**
 * @brief The error check of SpecialBlock, SpecialBlockBitmap, TBlock and
 * PredatorBlit (not in the original): the shape in a1 has planes.
 *
 * Fixed (#429): a shape of depth 0 (a shape never defined) makes the
 * blit loop (DBRA on depth-1) run 65536 times over the "plane pointers"
 * behind the bitmap object; runtime error "Uninitialized Shape".  The
 * release code is unchanged (an invalid argument).
 * @param shape  a1: the shape object
 */
static void BLITZ_ERRCHK errshapea1_c(struct bb_shape *shape BL_REG(a1))
{
    if (!shape->depth)
        BLITZ_RUNERR(noinit);
}

/**
 * @brief The error check of BPredatorBlit and BColourBlit (not in the
 * original): the shape in a3 has planes.
 *
 * Fixed (#429): as errshapea1_c (the save of the background loops over
 * the bitmap's planes, the blit over the shape's).
 * @param shape  a3: the shape object
 */
static void BLITZ_ERRCHK errshapea3_c(struct bb_shape *shape BL_REG(a3))
{
    if (!shape->depth)
        BLITZ_RUNERR(noinit);
}

BLITZ_ERRCHK_MESSAGE(nocount, "CreateShape count must be at least 1!");
BLITZ_ERRCHK_MESSAGE(pastmax, "CreateShape start+count above Maximum Shapes!");

/**
 * @brief CreateShape's error check (not in the original): a count of at
 * least 1 that stays inside the shape table.
 *
 * Fixed (#428): count 0 makes the routine's DBRA loop create 65536
 * shapes over the memory behind the shape table; runtime error
 * "CreateShape count must be at least 1!".  Fixed (#1077): start_shape +
 * count above Maximum Shapes writes the shapes from Maximum Shapes on
 * behind the table; runtime error "CreateShape start+count above Maximum
 * Shapes!".  start_shape itself is checked by the compiler's item check
 * ("Value out of Maximum Range"), so shape - table is below the maximum.
 * The release code is unchanged (an invalid argument); the declaration
 * loads a0 and d4 for this check.
 * @param count  d1: the number of shapes
 * @param max    d4: Maximum Shapes (BL_MD4)
 * @param table  a0: the shape table, shape 0 (BL_BA0)
 * @param shape  a2: the first shape object
 */
static void BLITZ_ERRCHK errcreateshape_c(UWORD count BL_REG(d1), UWORD max BL_REG(d4),
                                          struct bb_shape *table BL_REG(a0),
                                          struct bb_shape *shape BL_REG(a2))
{
    if (!count)
        BLITZ_RUNERR(nocount);
    else if ((ULONG)(shape - table) + count > max)
        BLITZ_RUNERR(pastmax);
}

/* the original changes d0 (the low word: +26 of the shape); kept here */
BLITZ_ERRCHK_THUNK(_errBlockToShape, errblocktoshape_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errSBlockToShape, errsblocktoshape_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errCreateShape, errcreateshape_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errShapeA1, errshapea1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_errShapeA3, errshapea3_c, "d0-d1/a0-a1");
