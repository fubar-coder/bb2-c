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

/**
 * @brief longmult ($CA01): Long multiplication, d0.l = d0.l * d1.l.
 *
 * On the magnitudes, the sign applied last.
 * BUG(orig) (#199): the cross products are added SWAPped, not shifted:
 * their high word also lands in the low word of the result.  That only
 * happens when a cross product is >= $10000, i.e. when the true product
 * does not fit in 32 bits anyway.
 *
 * @param a  d0: the first factor
 * @param b  d1: the second factor
 * @return d0: the product
 */
LONG lmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG mag_a = a, mag_b = b;         /* d0 d1 */
    ULONG acc;                          /* d0: the product */
    ULONG cross_hilo, cross_lohi;       /* d3 d4 */
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)mag_a < 0) {
        mag_a = -mag_a;
        neg = 0xffff;
    }
    if ((LONG)mag_b < 0) {              /* 'skip */
        mag_b = -mag_b;
        neg = ~neg;
    }
    acc = mulu(mag_a, mag_b);           /* 'skip2: lo(a) * lo(b) */
    cross_hilo = mulu(swap(mag_a), mag_b); /* hi(a) * lo(b) */
    acc += swap(cross_hilo);
    cross_lohi = mulu(mag_a, swap(mag_b)); /* lo(a) * hi(b) */
    acc += swap(cross_lohi);
    if (neg)
        acc = -acc;                     /* 'skip3 */
    return (LONG)acc;
}

/**
 * @brief quickmult ($CA00): Quick multiplication (16.16 fixed point),
 * d0.q = d0.q * d1.q.
 *
 * The four partial products shifted to keep bits 16..47 of the 64 bit
 * product.  The low product contributes only its high word (CLR/SWAP:
 * truncation, no rounding).
 * BUG(orig) (#199): the high product is added SWAPped like in longmult
 * (its high word lands in the low word when hi*hi >= $10000).
 *
 * @param a  d0: the first factor
 * @param b  d1: the second factor
 * @return d0: the product
 */
LONG qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG mag_a = a, mag_b = b;         /* d0 d1 (d3 d4: copies of |a|) */
    ULONG acc;                          /* d0: the product */
    ULONG swap_b;                       /* d1: |b| SWAPped */
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)mag_a < 0) {
        mag_a = -mag_a;
        neg = 0xffff;
    }
    if ((LONG)mag_b < 0) {              /* 'skip */
        mag_b = -mag_b;
        neg = ~neg;
    }
    acc = mulu(mag_a, mag_b) >> 16;     /* 'skip2: lo*lo: CLR d0:SWAP d0 */
    acc += mulu(swap(mag_a), mag_b);    /* + hi(a) * lo(b) */
    swap_b = swap(mag_b);
    acc += mulu(mag_a, swap_b);         /* + lo(a) * hi(b) */
    acc += swap(mulu(swap(mag_a), swap_b)); /* + hi(a) * hi(b) << 16 */
    if (neg)
        acc = -acc;                     /* 'skip3 */
    return (LONG)acc;
}
