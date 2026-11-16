/*
 * lmullib - system library 64835 ($CA00-$CA01): Quick and 32 bit
 * multiplication for 68000 generated code (the compiler calls it for `*`
 * on Quick and Long, tokens quickmult/longmult, blitz2.s equates 170-171).
 *
 * Translation of bblibsrc/lmullib (Acid Software).  The 68000 has only a
 * 16x16->32 bit MULU, so both routines combine partial products.  The
 * result is the product modulo 2^32 (Quick: bits 16..47 of the 64 bit
 * product), also when it overflows.
 * Fixed (#199): the original adds partial products SWAPped instead of
 * shifted by 16, so on an overflow their high word also landed in the low
 * word of the result.  The fixed routines take no more time than the
 * original's (rule #1019; see the notes at the routines).
 */
#include "blitzlib.h"

LONG BLITZ_ROUTINE qmultiply(LONG a BL_REG(d0), LONG b BL_REG(d1));
void lmultiply(void);                   /* assembler, below */

/* !syslibheader{#lmullib,$ca00,$ca02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_lmullib, 0xca00, 0xca02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qmultiply, 0, 0);         /* $CA00 quickmult */
BLITZ_NULLSUB(lmultiply, 0, 0);         /* $CA01 longmult */
BLITZ_LIBFIN();

/* SWAP: exchange the words of a long */
static inline ULONG swap(ULONG a)
{
    return (a << 16) | (a >> 16);
}

/**
 * @brief longmult ($CA01): Long multiplication, d0.l = d0.l * d1.l.
 *
 * The product modulo 2^32 needs no signs: the low 32 bits of the product
 * of the two's complement words are the result, and (-a) * (-b) = a * b.
 * So only the sign of b is tested, and a negative b negates both factors:
 * the three MULU then multiply by the words of |b|, the original's source
 * words (MULU takes 38 + 2 cycles per 1 bit of its source word), and the
 * MULU time is the original's for every input.  Zero words take short
 * paths: hi(b) = 0 needs no lo(a) * hi(b), and hi(a) = 0 as well no
 * hi(a) * lo(b) and no saved register (100 * 20 is one MULU).
 * Fixed (#199): the original adds the cross products SWAPped, not
 * shifted: their high word also lands in the low word of the result when
 * a cross product is >= $10000, i.e. when the true product does not fit
 * in 32 bits anyway (196608 * 30000 gave 1603272705, not 1603272704).
 * Here the low words of the two cross products are added (ADD.w) and
 * land in the high word only.
 *
 * Assembler because the cost is in the register use: GCC's code tested
 * both signs, saved one register more and took the MULU source words
 * from a (the C version was between 38 cycles faster and 46 slower than
 * the original, depending on the 1 bits of a).  These paths are 50 to
 * 222 cycles faster than the original for every input (64 to 84 on three
 * MULU; 123456 * 789: 208 cycles against 304, 100 * 20: 112 against
 * 292), counted per instruction (analysis analyse-g13-mul.md).
 *
 * Registers: d0 the product, d1 changed (as in the original), d2-d7 and
 * a0-a6 kept (internal token: the compiler holds d2-d7 across an
 * operator; LotanConvert holds a0 across it); the CCR is no result.
 *
 * @param a  d0: the first factor
 * @param b  d1: the second factor
 * @return d0: the product
 */
BLITZ_ASM_ROUTINE(lmultiply,
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   neg.l   d1\n"                   /* b < 0: (-a) * (-b) */
        "   neg.l   d0\n"
        "1: swap    d1\n"
        "   tst.w   d1\n"
        "   bne.s   3f\n"                   /* hi(b) <> 0: three MULU */
        "   swap    d1\n"
        "   swap    d0\n"
        "   tst.w   d0\n"
        "   bne.s   2f\n"                   /* hi(a) <> 0: two MULU */
        "   swap    d0\n"
        "   mulu.w  d1,d0\n"                /* lo(a) * lo(b) */
        "   rts\n"
        "2: move.l  d2,-(sp)\n"             /* d0 = [lo(a) : hi(a)] */
        "   move.w  d0,d2\n"
        "   mulu.w  d1,d2\n"                /* hi(a) * lo(b) */
        "   swap    d0\n"
        "   mulu.w  d1,d0\n"                /* lo(a) * lo(b) */
        "   swap    d0\n"
        "   add.w   d2,d0\n"                /* the cross product's low word */
        "   swap    d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        "3: move.l  d2,-(sp)\n"             /* d1 = [lo(b) : hi(b)] */
        "   move.l  d0,d2\n"
        "   mulu.w  d1,d0\n"                /* lo(a) * hi(b) */
        "   swap    d1\n"
        "   swap    d0\n"
        "   move.w  d2,d0\n"                /* d0 = [lo(lo*hi) : lo(a)] */
        "   swap    d2\n"
        "   mulu.w  d1,d2\n"                /* hi(a) * lo(b) */
        "   swap    d0\n"
        "   add.w   d2,d0\n"                /* the cross products' low words */
        "   swap    d0\n"                   /* d0 = [cross : lo(a)] */
        "   move.w  d0,d2\n"
        "   mulu.w  d1,d2\n"                /* lo(a) * lo(b) */
        "   clr.w   d0\n"
        "   add.l   d2,d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts");

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
