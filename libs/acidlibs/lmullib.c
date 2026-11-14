/*
 * lmullib - system library 64835 ($CA00-$CA01): Quick and 32 bit
 * multiplication for 68000 generated code (the compiler calls it for `*`
 * on Quick and Long, tokens quickmult/longmult, blitz2.s equates 170-171).
 *
 * C translation of bblibsrc/lmullib (Acid Software).  The 68000 has only a
 * 16x16->32 bit MULU, so both routines combine partial products.  The
 * result is the product modulo 2^32 (Quick: bits 16..47 of the 64 bit
 * product), also when it overflows.
 * Fixed (#199): the original adds partial products SWAPped instead of
 * shifted by 16, so on an overflow their high word also landed in the low
 * word of the result.  The fixed routines take no more time than the
 * unfixed ones (rule #1019; see the notes at the routines).
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
 * Fixed (#199): the original adds the cross products SWAPped, not
 * shifted: their high word also lands in the low word of the result when
 * a cross product is >= $10000, i.e. when the true product does not fit
 * in 32 bits anyway (196608 * 30000 gave 1603272705, not 1603272704).
 * Now the low words of the two cross products are added and shifted
 * once; that is 8 cycles fewer than before (ADD.w, SWAP, CLR.w, ADD.l for
 * SWAP, ADD.l, SWAP, ADD.l: 36 instead of 44 besides the MULU), with the
 * three MULU on the same source words.
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
    cross_lohi = mulu(mag_a, swap(mag_b)); /* lo(a) * hi(b) */
    /* Fixed (#199): the cross products shifted by 16, only their low
       words count */
    acc += (ULONG)(UWORD)(cross_hilo + cross_lohi) << 16;
    if (neg)
        acc = -acc;                     /* 'skip3 */
    return (LONG)acc;
}

/**
 * @brief quickmult ($CA00): Quick multiplication (16.16 fixed point),
 * d0.q = d0.q * d1.q.
 *
 * The four partial products shifted to keep bits 16..47 of the 64 bit
 * product (modulo 2^32).  The low product contributes only its high word
 * (truncation, no rounding).
 * Fixed (#199): the original adds the high product hi(a) * hi(b) SWAPped
 * like longmult, so its high word also lands in the low word when hi*hi
 * >= $10000 (300 * 300 gave $5F90.0001).  Now its low word goes to the
 * high word, together with the high word of lo*lo in one long (MOVE.w,
 * SWAP).  The core is inline assembler so that it stays as cheap as the
 * original's (rule #1019, a hot path): the same four MULU with the same
 * source words (lo(b), hi(b), hi(a), hi(a): their 1 bits set the MULU
 * time), the same saved registers, and the instructions besides the
 * MULU take 52 cycles instead of 60 from 'skip2 on (the core: MOVE.w,
 * SWAP instead of CLR.w, SWAP ... SWAP, ADD.l; with gcc's register
 * copies around it, counted in the built routine).
 *
 * @param a  d0: the first factor
 * @param b  d1: the second factor
 * @return d0: the product
 */
LONG qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG mag_a = a, mag_b = b;         /* d0 d1 */
    ULONG swap_a, swap_b;               /* |a| and |b| SWAPped */
    ULONG lolo;                         /* lo*lo, then the two outer words */
    UWORD neg = 0;                      /* d2.w */

    if ((LONG)mag_a < 0) {
        mag_a = -mag_a;
        neg = 0xffff;
    }
    if ((LONG)mag_b < 0) {              /* 'skip */
        mag_b = -mag_b;
        neg = ~neg;
    }
    swap_a = swap(mag_a);               /* 'skip2 */
    swap_b = swap(mag_b);
    __asm__("move.w  %[a],%[lolo]\n\t"
            "mulu.w  %[b],%[lolo]\n\t"  /* lo(a) * lo(b) */
            "mulu.w  %[sb],%[a]\n\t"    /* lo(a) * hi(b) */
            "mulu.w  %[sa],%[b]\n\t"    /* hi(a) * lo(b) */
            "mulu.w  %[sa],%[sb]\n\t"   /* hi(a) * hi(b) */
            "move.w  %[sb],%[lolo]\n\t" /* high word of lo*lo, low word of hi*hi */
            "swap    %[lolo]\n\t"
            "add.l   %[lolo],%[a]\n\t"
            "add.l   %[b],%[a]"
            : [lolo] "=&d"(lolo), [a] "+d"(mag_a), [b] "+d"(mag_b), [sb] "+d"(swap_b)
            : [sa] "d"(swap_a)
            : "cc");
    if (neg)
        mag_a = -mag_a;                 /* 'skip3 */
    return (LONG)mag_a;
}
