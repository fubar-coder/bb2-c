/*
 * richunkylib - RWE userlib 24 (#richunkylib, Reflective Images, S.
 * Matty, 14/8/95): chunky (one byte per pixel) bitmaps, fixed to
 * 320x256x8 -
 *   ChunkyBitmap chunky#,width,height   an object: width*height bytes
 *   ChunkyToPlanar chunky#,bitmap#      chunky to planar (C2P) into a
 *                                       320x256 bitmap of depth >= 8
 *   PlanarToChunky bitmap#,chunky#      no code
 *   ChunkyLoadBitMap filename$,chunky#  no code
 *   ChunkyData(chunky#)                 the address of the chunky data
 *   RaycastSky chunky#,x,height,column,&texture   a column copy
 *
 * C translation of rwelibsrc/chunkylib.bb; reference: the reference
 * build of that source with blitz2 2.10 (REFERENCE refbuild,
 * ref/richunkylib.obj) - no binary of the library exists (chunkylib2
 * does not compile, docs/rwelibs/README.md 2).  Notes:
 * docs/rwelibs/port/richunkylib.md.
 *
 * The object (16 bytes, max 8 by default): width.w, height.w, depth.w
 * (always 8), 2 bytes unused, the data pointer (memlib's globalloc,
 * MEMF_CLEAR), 4 bytes unused.  init and finit are empty; the objects
 * are freed by the object manager's free routine (_free).
 *
 * C2P: the 8-pixel merge network of the original (a well-known 68000
 * C2P routine: 4 pixels per long, $0F0F0F0F/$3333CCCC/$55555555 masks,
 * 32 pixels buffered then written as longs), in assembler (c2pcore, for
 * speed) with the same steps, four of them unrolled per long; the longs
 * go to planes 7 down to 0 (the original: 1, 7, 6, 5, 4, 3, 2, 0, which
 * differs only for planes that overlap), row by row with the bitmap's
 * BytesPerRow, a 16-pixel step at the end of a row whose width is an odd
 * multiple of 16.  RaycastSky is an assembler entry (speed), the rest C.
 *
 * BUG(orig):
 * Fixed (intended deviation, #884): ChunkyToPlanar converts the chunky
 * bitmap's width x height into the bitmap's top left (BytesPerRow, the
 * plane stride from the plane pointers) and refuses a width that is not
 * a multiple of 16, a chunky bitmap wider or higher than the bitmap and
 * planes that are not equally spaced (c2pchk's error with runtime error
 * checking); the original converted 320 x 256 into planes 10240 bytes
 * apart from plane 0 whatever the sizes.
 * Fixed (intended deviation, #885): ChunkyBitmap frees the data the
 * object holds when its data pointer is not 0; the original tested the
 * long at offset 6 (TST.l 6(a0): the unused word and the pointer's upper
 * word), so data below $10000 was never freed and, when the new
 * allocation failed, the object kept the old pointer with width, height
 * and depth 0.
 * Fixed (intended deviation, #887): RaycastSky copies nothing for a
 * height below 1; the original took height - 1 as a DBRA count, so a
 * height of 0 copied 65536 rows (20 MB of addresses).  With runtime error
 * checking it also refuses an x whose column leaves the chunky data and
 * a negative texture column (raycastchk; the original has no error check
 * area); without it x and column are not checked.
 * Fixed (intended deviation, #886): the free routine leaves CIA-A PRA
 * bit 1 (the power LED / audio filter) as it is; the original toggled it
 * (BCHG #1,$bfe001, a debug flash left in).
 * Fixed (intended deviation, #883): ChunkyToPlanar tests the byte Depth
 * (below 8: nothing); the original compared the word Flags * 256 + Depth
 * signed, so a Flags byte of 1-127 let any depth pass and one with bit 7
 * set refused every depth.
 * Not a defect: the free routine extends the width twice (EXT.l d0
 * twice) and not the height, so the size it computes has the caller's
 * upper word of d1 - but the size is unused (globfree needs none).
 */
#include "blitzlib.h"
#include <graphics/gfx.h>

/* entry points (thunks, below) */
void _chunkybitmap(void), _c2p(void), _p2c(void), _chunkyloadbitmap(void);
void _chunkydata(void), _raycastsky(void), _dofreechunkybitmap(void);
void richunkylib_libinit(void), richunkylib_libfinit(void);
void raycastchk(void), c2pchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader {#richunkylib,init,0,finit,0}; error field: the error check
   area of raycastchk (fixed, #887) and c2pchk (fixed, #884) */
BLITZ_LIBHEADER(LIB_richunkylib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, BL_IA0|BL_PD0);
BLITZ_SUBS((char *)_chunkybitmap + 1, 0, 0);    /* +1: Amiga mode only */
BLITZ_NAME_AT(_toke, "ChunkyBitmap", "bitmap#,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, BL_IA0|BL_PD0, LIB_bitmaplib, BL_IA1|BL_PD1);
BLITZ_SUBS(_c2p, c2pchk, 0);                              /* check: fixed #884 */
BLITZ_NAME("ChunkyToPlanar", "chunkybitmap,bitmap    --320x256x8 bitmap");  /* the original's help text: other shapes too (#884) */

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, BL_IA1|BL_PD1, LIB_bitmaplib, BL_IA0|BL_PD0);
BLITZ_SUBS(_p2c, 0, 0);
BLITZ_NAME("PlanarToChunky", "bitmap,chunkybitmap    --320x256x8 bitmap");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, BL_IA0|BL_PD1);
BLITZ_SUBS((char *)_chunkyloadbitmap + 1, 0, 0);
BLITZ_NAME("ChunkyLoadBitMap", "filename$,chunkybitmap#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_richunkylib, BL_IA0|BL_PD0);
BLITZ_SUBS(_chunkydata, 0, 0);
BLITZ_NAME("ChunkyData", "chunkybitmap#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_richunkylib, BL_IA0|BL_PD0);
BLITZ_SUBS(_raycastsky, raycastchk, 0);                   /* check: fixed #887 */
BLITZ_NAME("RaycastSky", "chunkybitmap#,x,height,column,&texture");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _dofreechunkybitmap, 0, 0);

BLITZ_NULLSUB_AT(finit, richunkylib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, richunkylib_libinit, 0, 0);

/* !libfin {_toke,_load,_save,_use,_free,8,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 8, 4);

/* ------------------------------------------------------------ the object */

/* the chunky object (!libfin: 16 bytes) */
struct chunky {
    WORD width;                                 /* +0 */
    WORD height;                                /* +2 */
    WORD depth;                                 /* +4: always 8 */
    UWORD unused6;                              /* +6 */
    ULONG data;                                 /* +8: the data (globalloc, MEMF_CLEAR);
                                                   an address as a long: as a
                                                   pointer GCC builds other loops */
    ULONG unused12;                             /* +12 */
};
static_assert(sizeof(struct chunky) == 16);

/* the long the original addresses across members: at +0 (width and
   height) */
#define WIDTHHEIGHT(o) (*(ULONG *)&(o)->width)

/**
 * @brief init, finit, PlanarToChunky bitmap#,chunky# and ChunkyLoadBitMap
 * filename$,chunky#: no code (RTS).
 *
 * The two commands are declared but do nothing (their arguments are
 * ignored).
 */
static void BLITZ_ROUTINE nothing_c(void)
{
}

/* _freechunkybitmap (a0 = the object): the size width*height through
   lmullib (the width extended twice, d1's upper word the caller's - no
   defect, the product is unused), globfree, the fields cleared (fixed,
   #886: the original toggles the LED here); callerd1 = the caller's d1 */
static void freechunky(struct chunky *o, ULONG callerd1)
{
    struct bl_regs r;

    if (!o->data)
        return;
    r.d0 = (ULONG)(LONG)o->width;
    r.d1 = (callerd1 & 0xffff0000UL) | (UWORD)o->height;
    r.a0 = (ULONG)o;
    r.a1 = 0;
    BLITZ_ALIBJSR(TOK_lmultiply, r);
    r.a1 = o->data;
    BLITZ_ALIBJSR(TOK_globfree, r);
    WIDTHHEIGHT(o) = 0;
    o->depth = 0;
    o->data = 0;
}

/**
 * @brief The object manager's free routine (_free): frees a chunky
 * object's data.
 *
 * As freechunky: nothing without data, else globfree and the fields
 * cleared (fixed, #886: the original also toggles CIA-A PRA bit 1).
 * @param callerd1  d1: the caller's d1 (its upper word goes to lmullib)
 * @param o         a3: the object
 */
static void BLITZ_ROUTINE dofreechunkybitmap_c(ULONG callerd1 BL_REG(d1), struct chunky *o BL_REG(a3))
{
    freechunky(o, callerd1);
}

/**
 * @brief ChunkyBitmap chunky#,width,height: a chunky object of
 * width*height bytes (memlib globalloc, MEMF_CLEAR), depth 8.
 *
 * Amiga mode only.  An object with data is freed first, decided by the
 * data pointer: fixed (#885), the original tested the long at offset 6
 * (the unused word and the pointer's upper word), so data below $10000
 * was never freed and a failing allocation left the old pointer in the
 * object.  On failure ('cantalloc) width, height and depth are 0.
 * @param w  d1: the width (a word)
 * @param h  d2: the height (a word)
 * @param o  a0: the object (!libs #richunkylib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE chunkybitmap_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2),
                                         struct chunky *o BL_REG(a0))
{
    struct bl_regs r;

    /* the data pointer; fixed (#885): the original's TST.l 6(a0) tested
       the unused word and the pointer's upper word */
    if (o->data)
        freechunky(o, w);
    o->width = (WORD)w;
    o->height = (WORD)h;
    o->depth = 8;                               /* depth: fixed to 8 */
    r.d0 = (ULONG)(LONG)(WORD)w;
    r.d1 = (ULONG)(LONG)(WORD)h;
    r.a0 = (ULONG)o;
    r.a1 = 0;
    BLITZ_ALIBJSR(TOK_lmultiply, r);
    r.d1 = 0x10000;                             /* MEMF_CLEAR */
    if (!BLITZ_ALIBJSR(TOK_globalloc, r)) {
        WIDTHHEIGHT(o) = 0;                     /* 'cantalloc */
        o->depth = 0;
        return;
    }
    o->data = r.d0;
}

/**
 * @brief ChunkyData(chunky#): the address of the chunky data.
 * @param o  a0: the object
 * @return d0: the data, 0 if the object has none
 */
static ULONG BLITZ_ROUTINE chunkydata_c(struct chunky *o BL_REG(a0))
{
    return o->data;
}

/**
 * @brief RaycastSky chunky#,x,height,column,&texture: height bytes of a
 * texture column to a chunky column, both 320 bytes apart.
 *
 * x and column are added as signed words (offsets: x may hold rows of
 * 320 bytes); with runtime error checking raycastchk refuses a column
 * that leaves the chunky data and a negative texture column (fixed,
 * #887), the release code checks nothing.  A height below 1 copies
 * nothing: fixed (#887), the original's DBRA count height - 1 copied
 * 65536 rows for a height of 0 (BLT after the SUBQ: 8 cycles per call).
 * An assembler entry in the original's form, without a thunk (a
 * user-level statement: it changes d2, a0 and a1, as the original, which
 * the register rule allows): the per-column call of a ray caster, at the
 * original's speed (bchunky.raycast*).
 * In: d1 x (the offset in the chunky data, a signed word), d2 the height
 * (a word), d3 the column (the offset in the texture, a signed word), d4
 * the texture, a0 the object.
 */
BLITZ_ASM_ROUTINE(_raycastsky,
"   move.l  8(a0),a0\n"                /* the chunky data */
"   move.l  d4,a1\n"                   /* the texture */
"   add.w   d1,a0\n"                   /* ADD.w: x a signed word */
"   add.w   d3,a1\n"                   /* ADD.w: the column a signed word */
"   subq.w  #1,d2\n"                   /* the DBRA count */
"   blt.s   1f\n"                     /* fixed (#887): a height below 1, nothing */
"0: move.b  (a1),(a0)\n"
"   lea     320(a1),a1\n"
"   lea     320(a0),a0\n"
"   dbf     d2,0b\n"
"1: rts");

/* ------------------------------------------------------------ C2P */

#define PLSIZ 10240                             /* 320*256/8 */

/* What c2pcore converts (a0 = this block): `rows` rows of `steps` 32
   pixel steps and, if `tail`, one 16 pixel step behind them; the chunky
   rows lie one behind the other from `src`.  `dst` is the row's position
   in plane 7, the planes are `ps` bytes apart (written 7 down to 0),
   `back` = 7 * ps + 4 returns from plane 0 to plane 7's next long, `skip`
   is added at the end of each row. */
struct c2pargs {
    ULONG src;                                  /* +0 */
    ULONG dst;                                  /* +4 */
    LONG ps;                                    /* +8 */
    LONG back;                                  /* +12 */
    LONG skip;                                  /* +16 */
    UWORD rows;                                 /* +20: 1 or more */
    UWORD steps;                                /* +22 */
    UWORD tail;                                 /* +24: 0 or 1 */
};
static_assert(sizeof(struct c2pargs) == 26);

/* 8 chunky pixels (two longs from a0) through the original's merge
   network into the stack buffers: plane 0/1 bytes at (a3)/4(a3), 2/3 at
   (a4)/4(a4), 4/5 at (a5)/4(a5), 6/7 at (a6)/4(a6), a3-a6 advanced by
   one; d5 = $0F0F0F0F, d6 = $55555555, d7 = $3333CCCC (_chunky2planar of
   chunkylib.bb, instruction for instruction) */
#define C2P8                                                                \
"   move.l  (a0)+,d0\n"                                                     \
"   move.l  (a0)+,d1\n"                                                     \
"   move.l  d0,d2\n"                                                        \
"   and.l   d5,d2\n"                                                        \
"   eor.l   d2,d0\n"                                                        \
"   move.l  d1,d3\n"                                                        \
"   and.l   d5,d3\n"                                                        \
"   eor.l   d3,d1\n"                                                        \
"   lsl.l   #4,d2\n"                                                        \
"   or.l    d3,d2\n"                                                        \
"   lsr.l   #4,d1\n"                                                        \
"   or.l    d1,d0\n"                                                        \
"   move.l  d2,d3\n"                                                        \
"   and.l   d7,d3\n"                                                        \
"   move.w  d3,d1\n"                                                        \
"   clr.w   d3\n"                                                           \
"   lsl.l   #2,d3\n"                                                        \
"   lsr.w   #2,d1\n"                                                        \
"   or.w    d1,d3\n"                                                        \
"   swap    d2\n"                                                           \
"   and.l   d7,d2\n"                                                        \
"   or.l    d2,d3\n"                                                        \
"   move.l  d0,d1\n"                                                        \
"   and.l   d7,d1\n"                                                        \
"   move.w  d1,d2\n"                                                        \
"   clr.w   d1\n"                                                           \
"   lsl.l   #2,d1\n"                                                        \
"   lsr.w   #2,d2\n"                                                        \
"   or.w    d2,d1\n"                                                        \
"   swap    d0\n"                                                           \
"   and.l   d7,d0\n"                                                        \
"   or.l    d0,d1\n"                                                        \
"   move.l  d1,d2\n"                                                        \
"   lsr.l   #7,d2\n"                                                        \
"   move.l  d1,d0\n"                                                        \
"   and.l   d6,d0\n"                                                        \
"   eor.l   d0,d1\n"                                                        \
"   move.l  d2,d4\n"                                                        \
"   and.l   d6,d4\n"                                                        \
"   eor.l   d4,d2\n"                                                        \
"   or.l    d4,d1\n"                                                        \
"   lsr.l   #1,d1\n"                                                        \
"   move.b  d1,4(a6)\n"                 /* plane 7 */                       \
"   swap    d1\n"                                                           \
"   move.b  d1,4(a5)\n"                 /* plane 5 */                       \
"   or.l    d0,d2\n"                                                        \
"   move.b  d2,(a6)+\n"                 /* plane 6 */                       \
"   swap    d2\n"                                                           \
"   move.b  d2,(a5)+\n"                 /* plane 4 */                       \
"   move.l  d3,d2\n"                                                        \
"   lsr.l   #7,d2\n"                                                        \
"   move.l  d3,d0\n"                                                        \
"   and.l   d6,d0\n"                                                        \
"   eor.l   d0,d3\n"                                                        \
"   move.l  d2,d4\n"                                                        \
"   and.l   d6,d4\n"                                                        \
"   eor.l   d4,d2\n"                                                        \
"   or.l    d4,d3\n"                                                        \
"   lsr.l   #1,d3\n"                                                        \
"   move.b  d3,4(a4)\n"                 /* plane 3 */                       \
"   swap    d3\n"                                                           \
"   move.b  d3,4(a3)\n"                 /* plane 1 */                       \
"   or.l    d0,d2\n"                                                        \
"   move.b  d2,(a4)+\n"                 /* plane 2 */                       \
"   swap    d2\n"                                                           \
"   move.b  d2,(a3)+\n"                 /* plane 0 */

/* the buffers of planes 7 down to 0 to the planes (SZ "l" after four
   C2P8, "w" after two; OFF the offset of the odd plane's bytes from the
   advanced pointer: 0 or 2), a1 from plane 7 down by a2 (the plane
   stride); the pointers back at the buffers' start */
#define C2PFLUSH(SZ, OFF)                                                   \
"   move." SZ "  " OFF "(a6),(a1)\n"    /* plane 7 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  -(a6),(a1)\n"          /* plane 6 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  " OFF "(a5),(a1)\n"    /* plane 5 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  -(a5),(a1)\n"          /* plane 4 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  " OFF "(a4),(a1)\n"    /* plane 3 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  -(a4),(a1)\n"          /* plane 2 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  " OFF "(a3),(a1)\n"    /* plane 1 */                       \
"   suba.l  a2,a1\n"                                                        \
"   move." SZ "  -(a3),(a1)\n"          /* plane 0 */                       \
"   adda.l  8(sp),a1\n"                 /* back: plane 7, the next long */

/* c2pcore (a0 = struct c2pargs): the conversion; keeps d2-d7/a2-a6.
   Assembler for speed (bchunky.c2p320): the original's 8 pixel step,
   four of them unrolled per long (no end test of the buffer, so a2 holds
   the plane stride), the planes written through one pointer.  Stack:
   0 the steps left in the row, 2 the rows left, 4 steps, 6 tail, 8 back,
   12 skip, 16 the size of the aligned 32 byte buffer above them. */
BLITZ_ASM_BLOCK(c2pcore,
"   .globl  _c2pcore\n"
"_c2pcore:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"
"   move.w  sp,d0\n"                    /* a long aligned 32 byte buffer */
"   and.w   #2,d0\n"
"   add.w   #32,d0\n"
"   suba.w  d0,sp\n"
"   movea.l sp,a3\n"
"   move.w  d0,-(sp)\n"
"   move.l  16(a0),-(sp)\n"             /* skip */
"   move.l  12(a0),-(sp)\n"             /* back */
"   move.l  22(a0),-(sp)\n"             /* steps, tail */
"   move.w  20(a0),-(sp)\n"             /* rows */
"   subq.l  #2,sp\n"                    /* the steps left */
"   movea.l 8(a0),a2\n"
"   movea.l 4(a0),a1\n"
"   movea.l (a0),a0\n"
"   move.l  #0x0f0f0f0f,d5\n"
"   move.l  #0x55555555,d6\n"
"   move.l  #0x3333cccc,d7\n"
"   lea     8(a3),a4\n"
"   lea     8(a4),a5\n"
"   lea     8(a5),a6\n"
"1: move.w  4(sp),(sp)\n"               /* a row */
"   beq.w   3f\n"
"2:\n"
C2P8 C2P8 C2P8 C2P8
C2PFLUSH("l", "")
"   subq.w  #1,(sp)\n"
"   bne.w   2b\n"
"3: tst.w   6(sp)\n"                    /* the 16 pixel tail */
"   beq.w   4f\n"
C2P8 C2P8
C2PFLUSH("w", "2")
"   subq.l  #2,a1\n"                    /* a word, not a long */
"4: adda.l  12(sp),a1\n"                /* the row's end */
"   subq.w  #1,2(sp)\n"
"   bne.w   1b\n"
"   lea     16(sp),sp\n"
"   adda.w  (sp)+,sp\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n");

/* the conversion of a (a struct c2pargs) */
static void c2p_run(const struct c2pargs *a)
{
    register const struct c2pargs *p __asm("a0") = a;

    __asm__ volatile("jsr     _c2pcore" : "+a"(p) : : "d0", "d1", "a1", "cc", "memory");
}

/* the plane stride of bm (plane n = plane 0 + n * stride for all 8
   planes) in *ps; 0 if the planes are not equally spaced */
static int planestride(const struct BitMap *bm, LONG *ps)
{
    const ULONG *pl = (const ULONG *)bm->Planes;
    LONG d = (LONG)(pl[1] - pl[0]);
    int n;

    for (n = 2; n < 8; n++)
        if ((LONG)(pl[n] - pl[n - 1]) != d)
            return 0;
    *ps = d;
    return 1;
}

/**
 * @brief ChunkyToPlanar chunky#,bitmap#: chunky to planar (C2P), the
 * chunky bitmap to the top left of the bitmap's 8 planes.
 *
 * Nothing without chunky data, nothing for a bitmap of depth below 8.
 * Fixed (#884): the chunky bitmap's width and height are converted, row
 * by row with the bitmap's BytesPerRow, into planes that are equally
 * spaced (the stride from the plane pointers, checked once per call);
 * nothing (with runtime error checking c2pchk's error) for a width that
 * is not a multiple of 16, a chunky bitmap wider or higher than the
 * bitmap, or planes that are not equally spaced.  The original always
 * converted 320 x 256 pixels into planes 10240 bytes apart from plane 0,
 * reading behind smaller chunky data and writing behind smaller bitmaps.
 * Fixed (#883): the depth is the byte Depth; the original compared Flags
 * and Depth as one signed word.  The conversion is c2pcore, the
 * original's merge network in assembler (speed).
 * @param o   a0: the chunky object
 * @param bm  a1: the bitmap (a struct BitMap)
 */
static void BLITZ_ROUTINE c2p_c(struct chunky *o BL_REG(a0), struct BitMap *bm BL_REG(a1))
{
    struct c2pargs a;
    WORD w = o->width, h = o->height;
    LONG ps;

    /* the byte Depth; fixed (#883): the original's CMPI.w #8,4(a1): BLT
       compared Flags and Depth as one signed word */
    if (bm->Depth < 8)
        return;
    if (!o->data)
        return;
    /* the shapes (fixed, #884) */
    if (w <= 0 || h <= 0 || (w & 15) || (UWORD)w > bm->BytesPerRow * 8U
        || (UWORD)h > bm->Rows || !planestride(bm, &ps))
        return;
    a.src = o->data;
    a.dst = *(ULONG *)&bm->Planes[0] + 7 * ps;  /* read as a long, as the data */
    a.ps = ps;
    a.back = 7 * ps + 4;
    a.skip = bm->BytesPerRow - (w >> 3);
    a.rows = h;
    a.steps = w >> 5;
    a.tail = (w >> 4) & 1;
    c2p_run(&a);
}

/* Registers: the original changes d0, d1, d7, a0, a1 (ChunkyBitmap,
   the free routine), d2/a0/a1 (RaycastSky, an assembler entry above),
   d0/d1/a0/a1 (ChunkyToPlanar), and nothing in its empty routines; the
   thunks keep d1/a0/a1 (and d0 for the empty ones), the C code
   d2-d7/a2-a6. */
BLITZ_THUNK(richunkylib_libinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(richunkylib_libfinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_p2c, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkyloadbitmap, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkybitmap, chunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_dofreechunkybitmap, dofreechunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_chunkydata, chunkydata_c, "d1/a0-a1");
BLITZ_THUNK(_c2p, c2p_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(outside, "RaycastSky outside the chunky bitmap");  /* fixed, #887 */
BLITZ_ERRCHK_MESSAGE(badcolumn, "RaycastSky column below 0");           /* fixed, #887 */

/**
 * @brief The error check of RaycastSky (debugger only).
 *
 * Fixed (#887), not in the original (richunkylib had no error check
 * area): RaycastSky adds x and column to the chunky data and the texture
 * as signed words and copies height bytes 320 apart without a look at
 * the chunky bitmap: a negative x wrote before the chunky data, an x
 * whose column runs past the last row behind it, a negative column read
 * before the texture.  The release code is unchanged (invalid arguments,
 * D-005).  A height below 1 is not checked: RaycastSky copies nothing
 * for it (fixed, #887).
 *
 * Errors: "RaycastSky outside the chunky bitmap" (x below 0, or x + 320 *
 * (rows - 1) not below width * height: also an object without data),
 * "RaycastSky column below 0" (column, a word, below 0).
 * @param x    d1: the offset in the chunky data (a word)
 * @param h    d2: the number of rows (a word)
 * @param col  d3: the offset in the texture (a word)
 * @param o    a0: the object
 */
static void BLITZ_ERRCHK raycastchk_c(ULONG x BL_REG(d1), ULONG h BL_REG(d2),
                                      ULONG col BL_REG(d3), const struct chunky *o BL_REG(a0))
{
    LONG last = 320L * ((WORD)h - 1);           /* the offset of the last row */

    if ((WORD)h < 1)
        return;
    if ((WORD)x < 0 || (WORD)x + last >= (LONG)o->width * o->height)
        BLITZ_RUNERR(outside);
    if ((WORD)col < 0)
        BLITZ_RUNERR(badcolumn);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(raycastchk, raycastchk_c, "d0-d1/a0-a1");

BLITZ_ERRCHK_MESSAGE(badwidth, "Chunky bitmap width not a multiple of 16");  /* fixed, #884 */
BLITZ_ERRCHK_MESSAGE(toolarge, "Chunky bitmap larger than the bitmap");      /* fixed, #884 */
BLITZ_ERRCHK_MESSAGE(badplanes, "Bitmap planes not equally spaced");         /* fixed, #884 */

/**
 * @brief The error check of ChunkyToPlanar (debugger only).
 *
 * Fixed (#884), not in the original: the shapes ChunkyToPlanar refuses
 * without a word in release code.  Nothing for a bitmap of depth below 8
 * or a chunky bitmap without data (the release code does nothing there,
 * as the original).
 *
 * Errors: "Chunky bitmap width not a multiple of 16", "Chunky bitmap
 * larger than the bitmap" (wider than BytesPerRow * 8 or higher than
 * Rows), "Bitmap planes not equally spaced" (the 8 plane pointers).
 * @param o   a0: the chunky object
 * @param bm  a1: the bitmap
 */
static void BLITZ_ERRCHK c2pchk_c(const struct chunky *o BL_REG(a0),
                                  const struct BitMap *bm BL_REG(a1))
{
    const ULONG *pl = (const ULONG *)bm->Planes;
    WORD w = o->width, h = o->height;
    int n;

    if (bm->Depth < 8 || !o->data || w <= 0 || h <= 0)
        return;
    if (w & 15)
        BLITZ_RUNERR(badwidth);
    if ((UWORD)w > bm->BytesPerRow * 8U || (UWORD)h > bm->Rows)
        BLITZ_RUNERR(toolarge);
    for (n = 2; n < 8; n++)
        if (pl[n] - pl[n - 1] != pl[1] - pl[0])
            BLITZ_RUNERR(badplanes);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(c2pchk, c2pchk_c, "d0-d1/a0-a1");
