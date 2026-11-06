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
#include <graphics/gfx.h>

/* entry points (thunks, below) */
void _chunkybitmap(void), _c2p(void), _p2c(void), _chunkyloadbitmap(void);
void _chunkydata(void), _raycastsky(void), _dofreechunkybitmap(void);
void richunkylib_libinit(void), richunkylib_libfinit(void);

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

/* the longs the original addresses across members: at +0 (width and
   height) and at +6 (the unused word and the data pointer's upper word) */
#define WIDTHHEIGHT(o) (*(ULONG *)&(o)->width)
#define AT6(o) (*(ULONG *)&(o)->unused6)

#define CIAA_PRA (*(volatile UBYTE *)0xbfe001)

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
   defect, the product is unused), globfree, the fields cleared, the LED
   toggled; callerd1 = the caller's d1 */
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
    CIAA_PRA ^= 2;                              /* BCHG #1,$bfe001: BUG(orig) (#886) */
}

/**
 * @brief The object manager's free routine (_free): frees a chunky
 * object's data.
 *
 * As freechunky: nothing without data, else globfree, the fields
 * cleared and BUG(orig) (#886) the CIA-A PRA bit 1 toggled.
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
 * Amiga mode only.  An object with data is freed first, decided by
 * TST.l 6(a0): BUG(orig) (#885) the unused word and the pointer's upper
 * word - data below $10000 is never freed, and then width and height
 * are overwritten and, if the allocation fails, cleared with the old
 * data pointer kept.  On failure ('cantalloc) width, height and depth
 * are 0.
 * @param w  d1: the width (a word)
 * @param h  d2: the height (a word)
 * @param o  a0: the object (!libs #richunkylib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE chunkybitmap_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2),
                                         struct chunky *o BL_REG(a0))
{
    struct bl_regs r;

    /* TST.l 6(a0): the unused word and the pointer's upper word,
       BUG(orig) (#885) */
    if (AT6(o))
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
 * BUG(orig) (#887): no checks; x and column are added as signed words,
 * the height is a DBRA count (height 0 copies 65536 bytes).
 * @param x    d1: the column in the chunky data (a signed word)
 * @param h    d2: the number of bytes
 * @param col  d3: the column in the texture (a signed word)
 * @param tex  d4: the texture
 * @param o    a0: the object
 */
static void BLITZ_ROUTINE raycastsky_c(ULONG x BL_REG(d1), ULONG h BL_REG(d2),
                                       ULONG col BL_REG(d3), ULONG tex BL_REG(d4),
                                       struct chunky *o BL_REG(a0))
{
    UBYTE *dst = (UBYTE *)o->data + (WORD)x;           /* ADD.w d1,a0 */
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

/**
 * @brief ChunkyToPlanar chunky#,bitmap#: chunky to planar (C2P), 81920
 * pixels to the 8 planes from the bitmap's first plane.
 *
 * The planes must be contiguous (plane n = plane 0 + n*10240); nothing
 * without chunky data.  BUG(orig): the sizes are ignored (#884), and the
 * depth test compares Flags and Depth as one signed word (#883).
 * @param o   a0: the chunky object
 * @param bm  a1: the bitmap (a struct BitMap)
 */
static void BLITZ_ROUTINE c2p_c(struct chunky *o BL_REG(a0), struct BitMap *bm BL_REG(a1))
{
    const ULONG *src;
    ULONG *pl;
    UBYTE buf[8][4];                            /* 4 bytes per plane */
    UWORD outer;

    /* CMPI.w #8,4(a1): BLT - Flags and Depth as one signed word,
       BUG(orig) (#883) */
    if (*(WORD *)&bm->Flags < 8)
        return;
    pl = (ULONG *)*(ULONG *)&bm->Planes[0];     /* read as a long, as the data */
    if (!o->data)
        return;
    src = (const ULONG *)o->data;
    for (outer = PLSIZ / 4; outer; outer--) {
        int k;

        for (k = 0; k < 4; k++) {
            /* d0-d4: the pixels, then nibble and bit groups */
            ULONG px0 = *src++, px1 = *src++;   /* d0, d1: the input longs */
            ULONG lo0, lo1, hi0, hi1;           /* d2, d3, d0, d1: the nibble halves */
            ULONG lonib, hinib;                 /* d2, d0: the merged nibbles */
            ULONG lomerge, himerge;             /* d3, d1: merge2 of them */

            lo0 = px0 & 0x0f0f0f0fUL;
            hi0 = px0 ^ lo0;
            lo1 = px1 & 0x0f0f0f0fUL;
            hi1 = px1 ^ lo1;
            lonib = (lo0 << 4) | lo1;
            hinib = hi0 | hi1 >> 4;
            lomerge = merge2(lonib);
            himerge = merge2(hinib);

            {                                   /* planes 7, 5, 6, 4 */
                ULONG shifted = himerge >> 7;               /* d2 */
                ULONG even = himerge & 0x55555555UL;        /* d0 */
                ULONG odd = himerge ^ even;                 /* d1 */
                ULONG mask = shifted & 0x55555555UL;        /* d4 */
                ULONG oddpl = (odd | mask) >> 1;            /* d1 */
                ULONG evenpl = (shifted ^ mask) | even;     /* d2 */

                buf[7][k] = (UBYTE)oddpl;
                buf[5][k] = (UBYTE)(oddpl >> 16);
                buf[6][k] = (UBYTE)evenpl;
                buf[4][k] = (UBYTE)(evenpl >> 16);
            }
            {                                   /* planes 3, 1, 2, 0 */
                ULONG shifted = lomerge >> 7;               /* d2 */
                ULONG even = lomerge & 0x55555555UL;        /* d0 */
                ULONG odd = lomerge ^ even;                 /* d3 */
                ULONG mask = shifted & 0x55555555UL;        /* d4 */
                ULONG oddpl = (odd | mask) >> 1;            /* d3 */
                ULONG evenpl = (shifted ^ mask) | even;     /* d2 */

                buf[3][k] = (UBYTE)oddpl;
                buf[1][k] = (UBYTE)(oddpl >> 16);
                buf[2][k] = (UBYTE)evenpl;
                buf[0][k] = (UBYTE)(evenpl >> 16);
            }
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
BLITZ_THUNK(richunkylib_libinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(richunkylib_libfinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_p2c, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkyloadbitmap, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_chunkybitmap, chunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_dofreechunkybitmap, dofreechunkybitmap_c, "d1/a0-a1");
BLITZ_THUNK(_chunkydata, chunkydata_c, "d1/a0-a1");
BLITZ_THUNK(_raycastsky, raycastsky_c, "d1/a0-a1");
BLITZ_THUNK(_c2p, c2p_c, "d1/a0-a1");
