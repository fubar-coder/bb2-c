/*
 * lmullib - system library 64835 ($CA00-$CA01): Quick and 32 bit
 * multiplication for 68000 generated code (the compiler calls it for `*`
 * on Quick and Long, tokens quickmult/longmult, blitz2.s equates 170-171).
 *
 * C translation of bblibsrc/lmullib (Acid Software).  The 68000 has only a
 * 16x16->32 bit MULU, so both routines combine partial products.  The way
 * they are added is kept exactly; it differs from a true 32 bit product
 * when the result overflows (see the notes at the routines).
 */
#include "blitzlib.h"

LONG BLITZ_ROUTINE qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1));
LONG BLITZ_ROUTINE lmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1));

/* !syslibheader{#lmullib,$ca00,$ca02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_lmullib, 0xca00, 0xca02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qmultiply, 0, 0);         /* $CA00 quickmult */
BLITZ_NULLSUB(lmultiply, 0, 0);         /* $CA01 longmult */
BLITZ_LIBFIN();

/* MULU: low word times low word, 32 bit result */
static inline ULONG mulu(ULONG a, ULONG b)
{
    return (ULONG)(UWORD)a * (UWORD)b;
}

/* SWAP: exchange the words of a long */
static inline ULONG swap(ULONG a)
{
    return (a << 16) | (a >> 16);
}

/* lmultiply - d0.l = d0.l * d1.l, on the magnitudes, sign applied last.
   BUG(orig) (#199): the cross products are added SWAPped, not shifted: their
   high word also lands in the low word of the result.  That only happens when a cross product is
   >= $10000, i.e. when the true product does not fit in 32 bits anyway. */
LONG lmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG d0 = a, d1 = b, d3, d4;
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)d0 < 0) {
        d0 = -d0;
        neg = 0xffff;
    }
    if ((LONG)d1 < 0) {                 /* 'skip */
        d1 = -d1;
        neg = ~neg;
    }
    d3 = d4 = d0;                       /* 'skip2 */
    d0 = mulu(d0, d1);                  /* lo(a) * lo(b) */
    d3 = mulu(swap(d3), d1);            /* hi(a) * lo(b) */
    d0 += swap(d3);
    d4 = mulu(d4, swap(d1));            /* lo(a) * hi(b) */
    d0 += swap(d4);
    if (neg)
        d0 = -d0;                       /* 'skip3 */
    return (LONG)d0;
}

/* qmultiply - d0.q = d0.q * d1.q (16.16 fixed point): the four partial
   products shifted to keep bits 16..47 of the 64 bit product.  The low
   product contributes only its high word (CLR/SWAP: truncation, no
   rounding); BUG(orig) (#199): the high product is added SWAPped like in
   lmultiply (its high word lands in the low word when hi*hi >= $10000). */
LONG qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG d0 = a, d1 = b, d3, d4;
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)d0 < 0) {
        d0 = -d0;
        neg = 0xffff;
    }
    if ((LONG)d1 < 0) {                 /* 'skip */
        d1 = -d1;
        neg = ~neg;
    }
    d3 = d4 = d0;                       /* 'skip2 */
    d0 = mulu(d0, d1) >> 16;            /* lo*lo: CLR d0:SWAP d0 */
    d0 += mulu(swap(d3), d1);           /* + hi(a) * lo(b) */
    d1 = swap(d1);
    d3 = d4;
    d0 += mulu(d4, d1);                 /* + lo(a) * hi(b) */
    d0 += swap(mulu(swap(d3), d1));     /* + hi(a) * hi(b) << 16 */
    if (neg)
        d0 = -d0;                       /* 'skip3 */
    return (LONG)d0;
}
