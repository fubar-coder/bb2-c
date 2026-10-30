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
 * 32 pixels buffered then written as longs), in C with the same steps
 * and the same accesses to the planes.  The planes must be contiguous:
 * plane n is plane 0 + n*10240, the bitmap's other plane pointers are
 * not read.  No assembler besides the thunks.
 *
 * BUG(orig):
 *   - ChunkyBitmap decides whether the object holds data by TST.l 6(a0),
 *     the unused word and the upper word of the data pointer: data below
 *     $10000 is not freed (and never is) (#885).
 *   - The free routine toggles CIA-A PRA bit 1 (BCHG #1,$bfe001: the
 *     power LED / audio filter), a debug flash left in (#886).
 *   - ChunkyBitmap on an object whose data was not freed (the TST.l above)
 *     overwrites width and height and, if the allocation fails, clears
 *     them but keeps the old data pointer (#885).
 *   - ChunkyToPlanar ignores the sizes (the chunky data must have 81920
 *     bytes, the bitmap 8 contiguous planes of 10240 bytes) (#884); a
 *     depth word below 8 (signed) does nothing (#883).
 *   - RaycastSky: no checks; the offsets x and column are added as signed
 *     words, the height is a DBRA count (height 0 copies 65536 bytes)
 *     (#887).
 * Not a defect: the free routine extends the width twice (EXT.l d0
 * twice) and not the height, so the size it computes has the caller's
 * upper word of d1 - but the size is unused (globfree needs none).
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _chunkybitmap(void), _c2p(void), _p2c(void), _chunkyloadbitmap(void);
void _chunkydata(void), _raycastsky(void), _dofreechunkybitmap(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {#richunkylib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_richunkylib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, IA(0, 0));
BLITZ_SUBS((char *)_chunkybitmap + 1, 0, 0);    /* +1: Amiga mode only */
BLITZ_NAME_AT(_toke, "ChunkyBitmap", "bitmap#,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, IA(0, 0), LIB_bitmaplib, IA(1, 1));
BLITZ_SUBS(_c2p, 0, 0);
BLITZ_NAME("ChunkyToPlanar", "chunkybitmap,bitmap    --320x256x8 bitmap");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, IA(1, 1), LIB_bitmaplib, IA(0, 0));
BLITZ_SUBS(_p2c, 0, 0);
BLITZ_NAME("PlanarToChunky", "bitmap,chunkybitmap    --320x256x8 bitmap");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_richunkylib, IA(0, 1));
BLITZ_SUBS((char *)_chunkyloadbitmap + 1, 0, 0);
BLITZ_NAME("ChunkyLoadBitMap", "filename$,chunkybitmap#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_richunkylib, IA(0, 0));
BLITZ_SUBS(_chunkydata, 0, 0);
BLITZ_NAME("ChunkyData", "chunkybitmap#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_richunkylib, IA(0, 0));
BLITZ_SUBS(_raycastsky, 0, 0);
BLITZ_NAME("RaycastSky", "chunkybitmap#,x,height,column,&texture");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _dofreechunkybitmap, 0, 0);

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

/* !libfin {_toke,_load,_save,_use,_free,8,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 8, 4);

/* ------------------------------------------------------------ the object */

#define OW(o, off) (*(WORD *)((UBYTE *)(o) + (off)))
#define OL(o, off) (*(ULONG *)((UBYTE *)(o) + (off)))
#define CW 0                                    /* width.w */
#define CH 2                                    /* height.w */
#define CD 4                                    /* depth.w */
#define CDATA 8                                 /* the data */

#define CIAA_PRA (*(volatile UBYTE *)0xbfe001)

/* init, finit, PlanarToChunky, ChunkyLoadBitMap: RTS */
static void BLITZ_ROUTINE nothing_c(void)
{
}

/* _freechunkybitmap (a0 = the object): the size width*height through
   lmullib (the width extended twice, d1's upper word the caller's - no
   defect, the product is unused), globfree, the fields cleared, the LED
   toggled */
static void freechunky(UBYTE *o, ULONG d1)
{
    struct bl_regs r;

    if (!OL(o, CDATA))
        return;
    r.d0 = (ULONG)(LONG)OW(o, CW);
    r.d1 = (d1 & 0xffff0000UL) | (UWORD)OW(o, CH);
    r.a0 = (ULONG)o;
    r.a1 = 0;
    BLITZ_ALIBJSR(TOK_lmultiply, r);
    r.a1 = OL(o, CDATA);
    BLITZ_ALIBJSR(TOK_globfree, r);
    OL(o, CW) = 0;
    OW(o, CD) = 0;
    OL(o, CDATA) = 0;
    CIAA_PRA ^= 2;                              /* BCHG #1,$bfe001: BUG(orig) (#886) */
}

/* the object manager's free routine: a3 = the object */
static void BLITZ_ROUTINE dofreechunkybitmap_c(ULONG d1 BL_REG(d1), UBYTE *o BL_REG(a3))
{
    freechunky(o, d1);
}

/* ChunkyBitmap n,width,height (d1, d2; a0 = the object) */
static void BLITZ_ROUTINE chunkybitmap_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2),
                                         UBYTE *o BL_REG(a0))
{
    struct bl_regs r;

    /* TST.l 6(a0): the unused word and the pointer's upper word,
       BUG(orig) (#885) */
    if (*(ULONG *)(o + 6))
        freechunky(o, w);
    OW(o, CW) = (WORD)w;
    OW(o, CH) = (WORD)h;
    OW(o, CD) = 8;                              /* depth: fixed to 8 */
    r.d0 = (ULONG)(LONG)(WORD)w;
    r.d1 = (ULONG)(LONG)(WORD)h;
    r.a0 = (ULONG)o;
    r.a1 = 0;
    BLITZ_ALIBJSR(TOK_lmultiply, r);
    r.d1 = 0x10000;                             /* MEMF_CLEAR */
    if (!BLITZ_ALIBJSR(TOK_globalloc, r)) {
        OL(o, CW) = 0;                          /* 'cantalloc */
        OW(o, CD) = 0;
        return;
    }
    OL(o, CDATA) = r.d0;
}

/* ChunkyData(n) (a0 = the object) */
static ULONG BLITZ_ROUTINE chunkydata_c(UBYTE *o BL_REG(a0))
{
    return OL(o, CDATA);
}

/* RaycastSky n,x,height,column,&texture (d1-d4; a0 = the object):
   height bytes of the texture column to the chunky column, 320 apart */
static void BLITZ_ROUTINE raycastsky_c(ULONG x BL_REG(d1), ULONG h BL_REG(d2),
                                       ULONG col BL_REG(d3), ULONG tex BL_REG(d4),
                                       UBYTE *o BL_REG(a0))
{
    UBYTE *dst = (UBYTE *)OL(o, CDATA) + (WORD)x;       /* ADD.w d1,a0 */
    const UBYTE *src = (const UBYTE *)tex + (WORD)col;  /* ADD.w d3,a1 */
    UWORD n = (UWORD)(h - 1);                           /* SUBQ.w #1: DBRA */

    do {
        *dst = *src;
        src += 320;
        dst += 320;
    } while (n-- != 0);
}

/* ------------------------------------------------------------ C2P */

#define PLSIZ 10240                             /* 320*256/8 */

/* ((x & $33330000) << 2) | (swap(x) & $3333CCCC) | ((x & $0000CCCC) >> 2) */
static ULONG merge2(ULONG x)
{
    ULONG hi = (x & 0x33330000UL) << 2;
    UWORD lo = (UWORD)((x & 0x0000ccccUL) >> 2);
    ULONG sw = (x << 16 | x >> 16) & 0x3333ccccUL;

    return (hi | lo) | sw;
}

/* ChunkyToPlanar chunky,bitmap (a0 = the chunky object, a1 = the
   bitmap): 81920 pixels to the 8 planes from the bitmap's first plane */
static void BLITZ_ROUTINE c2p_c(UBYTE *o BL_REG(a0), UBYTE *bm BL_REG(a1))
{
    const ULONG *src;
    ULONG *pl;
    UBYTE buf[8][4];                            /* 4 bytes per plane */
    UWORD outer;

    if (OW(bm, 4) < 8)                          /* CMPI.w #8,4(a1): BLT */
        return;
    pl = (ULONG *)OL(bm, 8);
    if (!OL(o, CDATA))
        return;
    src = (const ULONG *)OL(o, CDATA);
    for (outer = PLSIZ / 4; outer; outer--) {
        int k;

        for (k = 0; k < 4; k++) {
            ULONG d0 = *src++, d1 = *src++, d2, d3, d4;

            d2 = d0 & 0x0f0f0f0fUL;
            d0 ^= d2;
            d3 = d1 & 0x0f0f0f0fUL;
            d1 ^= d3;
            d2 = (d2 << 4) | d3;
            d0 |= d1 >> 4;
            d3 = merge2(d2);
            d1 = merge2(d0);

            d2 = d1 >> 7;
            d0 = d1 & 0x55555555UL;
            d1 ^= d0;
            d4 = d2 & 0x55555555UL;
            d2 ^= d4;
            d1 = (d1 | d4) >> 1;
            buf[7][k] = (UBYTE)d1;
            buf[5][k] = (UBYTE)(d1 >> 16);
            d2 |= d0;
            buf[6][k] = (UBYTE)d2;
            buf[4][k] = (UBYTE)(d2 >> 16);

            d2 = d3 >> 7;
            d0 = d3 & 0x55555555UL;
            d3 ^= d0;
            d4 = d2 & 0x55555555UL;
            d2 ^= d4;
            d3 = (d3 | d4) >> 1;
            buf[3][k] = (UBYTE)d3;
            buf[1][k] = (UBYTE)(d3 >> 16);
            d2 |= d0;
            buf[2][k] = (UBYTE)d2;
            buf[0][k] = (UBYTE)(d2 >> 16);
        }
        /* the buffers to the planes as longs, in the original's order */
        pl[PLSIZ / 4 * 1] = *(ULONG *)buf[1];
        pl[PLSIZ / 4 * 7] = *(ULONG *)buf[7];
        pl[PLSIZ / 4 * 6] = *(ULONG *)buf[6];
        pl[PLSIZ / 4 * 5] = *(ULONG *)buf[5];
        pl[PLSIZ / 4 * 4] = *(ULONG *)buf[4];
        pl[PLSIZ / 4 * 3] = *(ULONG *)buf[3];
        pl[PLSIZ / 4 * 2] = *(ULONG *)buf[2];
        pl[0] = *(ULONG *)buf[0];
        pl++;
    }
}

/* Registers: the original changes d0, d1, d7, a0, a1 (ChunkyBitmap,
   the free routine), d2 (RaycastSky), d0/d1/a0/a1 (ChunkyToPlanar), and
   nothing in its empty routines; the thunks keep d1/a0/a1 (and d0 for
   the empty ones), the C code d2-d7/a2-a6. */
BLITZ_THUNK(_libinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_p2c, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkyloadbitmap, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkybitmap, chunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_dofreechunkybitmap, dofreechunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_chunkydata, chunkydata_c, "d1/a0-a1");
BLITZ_THUNK(_raycastsky, raycastsky_c, "d1/a0-a1");
BLITZ_THUNK(_c2p, c2p_c, "d1/a0-a1");
