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

/* entry points (assembler, defined below) */
void qmultiply(void), lmultiply(void);

/* !syslibheader{#lmullib,$ca00,$ca02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_lmullib, 0xca00, 0xca02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qmultiply, 0, 0);         /* $CA00 quickmult */
BLITZ_NULLSUB(lmultiply, 0, 0);         /* $CA01 longmult */
BLITZ_LIBFIN();

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
 * The four partial products of the magnitudes shifted to keep bits 16..47
 * of the 64 bit product (modulo 2^32); the low product contributes only
 * its high word (truncation, no rounding), so the magnitudes are needed
 * and the sign is applied last.  The sign lives in the code path: four
 * entries by the sign bits of a and b lead to two copies of the core, one
 * followed by NEG.l (no flag register, no TST/BEQ at the end).  The sign
 * bits are tested as in the original, so $80000000 (magnitude 2^31 as an
 * unsigned long) counts as negative for both factors.  Negating both
 * factors for a negative b, as longmult does, would be wrong here: NEG
 * leaves $80000000 negative, so -32768 * -1/65536 would give -0.5.  The
 * four MULU
 * multiply by the words of |b| (lo(b), lo(b), hi(b), hi(b)), the
 * original's source words, so the MULU time is the original's for every
 * input (MULU takes 38 + 2 cycles per 1 bit of its source word).
 * Fixed (#199): the original adds the high product hi(a) * hi(b) SWAPped
 * like longmult, so its high word also lands in the low word when hi*hi
 * >= $10000 (300 * 300 gave $5F90.0001).  Here its low word goes to the
 * high word, together with the high word of lo*lo in one long (MOVE.w,
 * SWAP).
 *
 * Assembler because the cost is in the register use: GCC's code saved
 * one register more, branched around the sign of a and took two MULU
 * source words from a (the C version was between 62 cycles faster and 82
 * slower than the original, depending on the 1 bits of a).  This one is
 * 42 to 52 cycles faster than the original for every input (12.5 * 3.25:
 * 308 cycles against 354), counted per instruction (analysis
 * analyse-g13-mul.md).
 *
 * Registers: d0 the product, d1 changed (as in the original), d2-d7 and
 * a0-a6 kept (internal token: the compiler holds d2-d7 across an
 * operator; third-party callers, see longmult); the CCR is no result.
 *
 * @param a  d0: the first factor
 * @param b  d1: the second factor
 * @return d0: the product
 */
#define QMUL_CORE                                                           \
        "   move.l  d0,d3\n"                                                \
        "   swap    d3\n"                   /* d3 = [lo(a) : hi(a)] */      \
        "   move.w  d3,d2\n"                                                \
        "   mulu.w  d1,d2\n"                /* hi(a) * lo(b) */             \
        "   mulu.w  d1,d0\n"                /* lo(a) * lo(b) */             \
        "   move.w  d3,d0\n"                /* d0 = [hi(lo*lo) : hi(a)] */  \
        "   swap    d3\n"                                                   \
        "   swap    d1\n"                                                   \
        "   mulu.w  d1,d3\n"                /* lo(a) * hi(b) */             \
        "   add.l   d3,d2\n"                                                \
        "   move.w  d0,d3\n"                                                \
        "   mulu.w  d1,d3\n"                /* hi(a) * hi(b) */             \
        "   move.w  d3,d0\n"                /* d0 = [hi(lo*lo) : lo(hi*hi)] */ \
        "   swap    d0\n"                                                   \
        "   add.l   d2,d0\n"

BLITZ_ASM_ROUTINE(qmultiply,
        "   movem.l d2-d3,-(sp)\n"
        "   tst.l   d1\n"
        "   bmi.s   2f\n"                   /* b < 0 */
        "   tst.l   d0\n"
        "   bmi.s   4f\n"                   /* a < 0, b >= 0 */
        "1:\n"                              /* the signs alike */
        QMUL_CORE
        "   movem.l (sp)+,d2-d3\n"
        "   rts\n"
        "2: neg.l   d1\n"
        "   tst.l   d0\n"
        "   bmi.s   5f\n"                   /* both < 0 */
        "3:\n"                              /* the signs unlike */
        QMUL_CORE
        "   neg.l   d0\n"
        "   movem.l (sp)+,d2-d3\n"
        "   rts\n"
        "4: neg.l   d0\n"
        "   bra.s   3b\n"
        "5: neg.l   d0\n"
        "   bra.s   1b");
