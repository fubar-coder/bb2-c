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
    ULONG acc = a, mag_b = b, cross_hilo, cross_lohi;  /* d0 d1 d3 d4 */
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)acc < 0) {
        acc = -acc;
        neg = 0xffff;
    }
    if ((LONG)mag_b < 0) {              /* 'skip */
        mag_b = -mag_b;
        neg = ~neg;
    }
    cross_hilo = cross_lohi = acc;      /* 'skip2 */
    acc = mulu(acc, mag_b);             /* lo(a) * lo(b) */
    cross_hilo = mulu(swap(cross_hilo), mag_b); /* hi(a) * lo(b) */
    acc += swap(cross_hilo);
    cross_lohi = mulu(cross_lohi, swap(mag_b)); /* lo(a) * hi(b) */
    acc += swap(cross_lohi);
    if (neg)
        acc = -acc;                     /* 'skip3 */
    return (LONG)acc;
}

/* qmultiply - d0.q = d0.q * d1.q (16.16 fixed point): the four partial
   products shifted to keep bits 16..47 of the 64 bit product.  The low
   product contributes only its high word (CLR/SWAP: truncation, no
   rounding); BUG(orig) (#199): the high product is added SWAPped like in
   lmultiply (its high word lands in the low word when hi*hi >= $10000). */
LONG qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG acc = a, mag_b = b, a_hi, a_lo;   /* d0 d1 d3 d4 */
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)acc < 0) {
        acc = -acc;
        neg = 0xffff;
    }
    if ((LONG)mag_b < 0) {              /* 'skip */
        mag_b = -mag_b;
        neg = ~neg;
    }
    a_hi = a_lo = acc;                  /* 'skip2 */
    acc = mulu(acc, mag_b) >> 16;       /* lo*lo: CLR d0:SWAP d0 */
    acc += mulu(swap(a_hi), mag_b);     /* + hi(a) * lo(b) */
    mag_b = swap(mag_b);
    a_hi = a_lo;
    acc += mulu(a_lo, mag_b);           /* + lo(a) * hi(b) */
    acc += swap(mulu(swap(a_hi), mag_b)); /* + hi(a) * hi(b) << 16 */
    if (neg)
        acc = -acc;                     /* 'skip3 */
    return (LONG)acc;
}
