/*
 * compilerlib - system library 64030 ($D600-$D614), the port's, not in
 * the original: the routines of the code the compiler generates that no
 * library set has.  The compiler embeds it (programs/blitz2, asmlibs.c
 * loadlibs) and links it first in its library list in both modes,
 * whatever library set it loads (BB2's acidlibs, the CD's, AmiBlitz3's);
 * a program that uses none of its routines gets nothing of it.
 *
 * The routines of a Quick under a Long type so far (#1178,
 * quick-long-arithmetic; user decision of 2026-11-22, "#1178 after the
 * option D spike"): the compiler calls them for `*` and `/` with a Quick
 * operand when the type so far is Long (eval.c, mixedop), where the
 * original cut the Quick to a whole number first.
 *
 *   $D600 mixmult   floor(l * q), one factor a Long, the other a Quick
 *   $D601 qqmult    floor(q * p), both Quicks
 *   $D602 lqdiv     trunc(l / q), a Long divided by a Quick
 *
 * Registers (as longmult and longdiv, the contract of an operator): d0 and
 * d1 the operands, d0 the result, d1 changed, d2-d7 and a0-a6 kept; the
 * CCR is no result.  Results modulo 2^32 (a product or quotient beyond 32
 * bits wraps, as longmult; user decision 7 of quick-long-arithmetic).
 *
 * The 48.16 routines of rule P+ (OpenSpec change expr-tree-5-operand-typing,
 * design decision 9; #1216): a 48.16 value is a register pair, the low 32
 * bits (a 16.16 value) in the first register, the high 32 bits in the
 * second; the value is the pair's 64 bits / 65536.  Their values are
 * programs/blitz2/src/exprarith.c's bit for bit (exprarith_q48 and the
 * casts), the oracle of their emulator test (libs/tests/libtest_q48.c).
 *
 *   $D603 q48add    a + b                 modulo 2^64
 *   $D604 q48sub    a - b                 modulo 2^64
 *   $D605 q48mul    a * b                 cut towards zero to 1/65536
 *   $D606 q48div    a / b                 cut towards zero to 1/65536
 *                                         (q48divw, q48divb: down, #1221);
 *   $D607 q48divq   a / b                 a divisor 0 gives the largest
 *   $D608 q48divw   a / b                 value of the wanted type with
 *   $D609 q48divb   a / b                 the dividend's sign: a Long or
 *                                         none (q48div) +-$7FFFFFFF, a
 *                                         Quick (q48divq) +-$7FFFFFFF as
 *                                         16.16 bits, a Word (q48divw)
 *                                         +-32767, a Byte (q48divb) +-127
 *   $D60A q48mod    a Mod b               the exact floor Mod a -
 *                                         b*floor(a/b) (#1197); a divisor
 *                                         0 gives a
 *   $D60B l2q48     a Long as 48.16       exact
 *   $D60C q2q48     a Quick as 48.16      exact
 *   $D60D q48floor  48.16 to a Long       rounded down, modulo 2^32
 *   $D60E q48trunc  48.16 to a Long       towards zero, modulo 2^32
 *
 * The other casts need no routine: 48.16 to 16.16 is the low register
 * itself (exprarith_q48_q16), 48.16 to a Word or a Byte is the low word of
 * q48floor's or q48trunc's Long, sign-extended (ext.l; exprarith_q48_floor
 * and exprarith_q48_trunc with ET_WORD and ET_BYTE).
 *
 * Registers of the 48.16 routines: a in d0 (low) and d1 (high), b in d2
 * and d3; the result in d0 and d1 (q48floor, q48trunc: d0); d2 and d3
 * changed by the routines with two operands, every other register kept;
 * the CCR is no result.
 *
 * The routines of the speed review of rule P+ (user decision of
 * 2026-11-26, "Speed review of P+: the candidates and the rest"; D-027),
 * with the values of exprarith.c as the 48.16 routines:
 *
 *   $D60F q16mod    a Mod b of two Quicks the exact 16.16 floor Mod
 *                                         (exprarith_q16: a % b, plus b
 *                                         when not 0 and of the other
 *                                         sign); b not 0; d0/d1
 *                                         in, d0 out (longdiv's contract)
 *   $D610 q48mull   a * b of a Long and   exact (q48mul's value); a in
 *                   a Quick (either       d0, b in d2, the product in
 *                   order)                d0/d1, d2 and d3 changed (the
 *                                         48.16 routines' contract)
 *
 * The cut of a 48.16 product or quotient under a wanted Byte, Word or
 * Long rounds down (#1221, user decision of 2026-11-26, "Quick rounding:
 * down stays; the 48.16 cut in the final direction (E)";
 * exprarith_q48_down): q48divw and q48divb (only under a wanted Word or
 * Byte) cut their quotient down, and
 *
 *   $D611 q48mulf   a * b                 cut down to 1/65536 (q48mul's
 *                                         contract)
 *
 * The conversions of OpenSpec change type-system-rework (task 4.3; D-027;
 * the values of exprarith.c's exprarith_q48_quick, exprarith_ffp_whole and
 * exprarith_ffp_quick):
 *
 *   $D612 q48sat    48.16 to a Quick      saturated: $7FFFFFFF above,
 *                                         $80000000 below the Quick range;
 *                                         d0/d1 in, d0 out, d1 kept
 *   $D613 f2l       a Float to a Long     towards zero, beyond the Long
 *                                         range wrapped (I8); d0 in and
 *                                         out, d1 changed
 *   $D614 f2q       a Float to a Quick    rounded down to 1/65536,
 *                                         saturated as q48sat; d0 in and
 *                                         out, d1 changed
 */
#include "blitzlib.h"

void mixmult(void), qqmult(void), lqdivide(void);
void q48add(void), q48sub(void), q48mul(void);
void q48div(void), q48divq(void), q48divw(void), q48divb(void), q48mod(void);
void l2q48(void), q2q48(void), q48floor(void), q48trunc(void);
void q16mod(void), q48mull(void), q48mulf(void);
void q48sat(void), f2l(void), f2q(void);
void lqdivchk(void), q48divchk(void);

/* !syslibheader{64030,$d600,$d615,0,0,0,errors,$4000} */
BLITZ_SYSLIBHEADER(64030, 0xd600, 0xd615, 0, 0, 0, BLITZ_ERROR_AREA, 0x4000);
BLITZ_NULLSUB(mixmult, 0, 0);           /* $D600 mixmult */
BLITZ_NULLSUB(qqmult, 0, 0);            /* $D601 qqmult */
BLITZ_NULLSUB(lqdivide, lqdivchk, 0);   /* $D602 lqdiv */
BLITZ_NULLSUB(q48add, 0, 0);            /* $D603 q48add */
BLITZ_NULLSUB(q48sub, 0, 0);            /* $D604 q48sub */
BLITZ_NULLSUB(q48mul, 0, 0);            /* $D605 q48mul */
BLITZ_NULLSUB(q48div, q48divchk, 0);    /* $D606 q48div */
BLITZ_NULLSUB(q48divq, q48divchk, 0);   /* $D607 q48divq */
BLITZ_NULLSUB(q48divw, q48divchk, 0);   /* $D608 q48divw */
BLITZ_NULLSUB(q48divb, q48divchk, 0);   /* $D609 q48divb */
BLITZ_NULLSUB(q48mod, 0, 0);            /* $D60A q48mod */
BLITZ_NULLSUB(l2q48, 0, 0);             /* $D60B l2q48 */
BLITZ_NULLSUB(q2q48, 0, 0);             /* $D60C q2q48 */
BLITZ_NULLSUB(q48floor, 0, 0);          /* $D60D q48floor */
BLITZ_NULLSUB(q48trunc, 0, 0);          /* $D60E q48trunc */
BLITZ_NULLSUB(q16mod, 0, 0);            /* $D60F q16mod */
BLITZ_NULLSUB(q48mull, 0, 0);           /* $D610 q48mull */
BLITZ_NULLSUB(q48mulf, 0, 0);           /* $D611 q48mulf */
BLITZ_NULLSUB(q48sat, 0, 0);            /* $D612 q48sat */
BLITZ_NULLSUB(f2l, 0, 0);               /* $D613 f2l */
BLITZ_NULLSUB(f2q, 0, 0);               /* $D614 f2q */
BLITZ_LIBFIN();

/**
 * @brief mixmult ($D600) and qqmult ($D601): the whole-number products of
 * a Quick under a Long type so far.
 *
 * mixmult: d0.l = floor(d0 * d1 / 65536), one factor a Long and the other
 * a Quick (16.16, either order): bits 16..47 of the signed 64 bit product;
 * qqmult: d0.l = floor(d0 * d1 / 2^32), both Quicks: bits 32..63.
 *
 * A factor written as h * 65536 + l (h signed, l unsigned 0..65535).  The
 * paths of mixmult (#1191), the first that fits:
 * - one factor a signed word m (a Long -32768..32767, `i*4`), the other
 *   n: m * h(n) + floor(m * l(n) / 65536) with two MULS; with l(n) = 0
 *   (a whole Quick) m * h(n), one MULS.  m * l(n) fits in 32 bits (MULS
 *   takes an l(n) >= $8000 as l(n) - 65536: + m afterwards).
 * - one factor with l = 0 (a whole Quick), the other n: n * h modulo 2^32
 *   with two MULU.
 * - else floor(a*b / 65536) = h(a)*h(b) * 65536 + h(a)*l(b) + l(a)*h(b)
 *   + floor(l(a)*l(b) / 65536) modulo 2^32 with four MULU: a negative
 *   h(a) or h(b) as an unsigned word is 65536 too large, which takes
 *   l(b) or l(a) from the high word of the sum.
 * qqmult: a factor with l = 0, p = h * 65536, makes floor(q * p / 2^32)
 * floor(q * h / 65536), mixmult's signed-word path (both whole: one
 * MULS); else the 64 bit product of the magnitudes from four MULU (lo*lo,
 * the two cross products added as 33 bits, hi*hi), negated in 64 bits
 * when the signs differ, so that the bits taken are those of the floor;
 * $80000000 counts as the magnitude 2^31.
 *
 * @arg d0 a, the first factor
 * @arg d1 b, the second factor
 * @par Result
 * d0: the product rounded down
 */
BLITZ_ASM_BLOCK(mixmult,
        "   .globl  _mixmult\n"
        "_mixmult:\n"
        "   move.l  d2,-(sp)\n"
        "   move.w  d1,d2\n"
        "   ext.l   d2\n"
        "   cmp.l   d1,d2\n"
        "   beq.s   .Lword\n"               /* b a signed word */
        "   move.w  d0,d2\n"
        "   ext.l   d2\n"
        "   cmp.l   d0,d2\n"
        "   bne.s   .Llong\n"
        "   exg     d0,d1\n"                /* a a signed word */
        /* d0 = floor(n * m / 65536), d0 = n, d1.w = m (signed) */
        ".Lword:\n"
        "   tst.w   d0\n"
        "   bne.s   .Lfrac\n"
        "   swap    d0\n"
        "   muls.w  d1,d0\n"                /* l(n) = 0: h(n) * m */
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        ".Lfrac:\n"
        "   move.l  d0,d2\n"
        "   muls.w  d1,d0\n"                /* l(n) * m (l(n) signed) */
        "   swap    d2\n"                   /* N: l(n) >= $8000 */
        "   bmi.s   1f\n"
        "   muls.w  d1,d2\n"                /* h(n) * m */
        "   swap    d0\n"
        "   ext.l   d0\n"                   /* floor(l(n) * m / 65536) */
        "   add.l   d2,d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        "1: muls.w  d1,d2\n"
        "   swap    d0\n"
        "   add.w   d1,d0\n"                /* + m: MULS took l(n) - 65536 */
        "   ext.l   d0\n"
        "   add.l   d2,d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        ".Llong:\n"
        "   tst.w   d1\n"
        "   beq.s   .Lwhole\n"
        "   tst.w   d0\n"
        "   bne.s   .Lfour\n"
        "   exg     d0,d1\n"
        /* d0 = n * h modulo 2^32, d0 = n, d1 = [h : 0] */
        ".Lwhole:\n"
        "   swap    d1\n"
        "   move.l  d0,d2\n"
        "   swap    d2\n"
        "   mulu.w  d1,d2\n"                /* h(n) * h, its low word */
        "   tst.w   d1\n"
        "   bpl.s   1f\n"
        "   sub.w   d0,d2\n"                /* h < 0: - l(n) */
        "1: mulu.w  d1,d0\n"                /* l(n) * h */
        "   swap    d0\n"
        "   add.w   d2,d0\n"
        "   swap    d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        ".Lfour:\n"
        "   move.l  d3,-(sp)\n"
        "   move.l  d0,d2\n"
        "   swap    d2\n"
        "   move.w  d2,d3\n"
        "   mulu.w  d1,d3\n"                /* h(a) * l(b) */
        "   swap    d1\n"
        "   mulu.w  d1,d2\n"                /* h(a) * h(b), its low word */
        "   tst.l   d0\n"
        "   bpl.s   1f\n"
        "   swap    d1\n"
        "   sub.w   d1,d2\n"                /* h(a) < 0: - l(b) */
        "   swap    d1\n"
        "1: tst.w   d1\n"
        "   bpl.s   2f\n"
        "   sub.w   d0,d2\n"                /* h(b) < 0: - l(a) */
        "2: swap    d2\n"
        "   clr.w   d2\n"
        "   add.l   d2,d3\n"
        "   move.w  d0,d2\n"
        "   mulu.w  d1,d2\n"                /* l(a) * h(b) */
        "   add.l   d2,d3\n"
        "   swap    d1\n"
        "   mulu.w  d1,d0\n"                /* l(a) * l(b) */
        "   clr.w   d0\n"
        "   swap    d0\n"
        "   add.l   d3,d0\n"
        "   move.l  (sp)+,d3\n"
        "   move.l  (sp)+,d2\n"
        "   rts\n"
        "   .globl  _qqmult\n"
        "_qqmult:\n"
        "   tst.w   d1\n"
        "   beq.s   1f\n"
        "   tst.w   d0\n"
        "   bne.s   .Lqq64\n"
        "   exg     d0,d1\n"
        "1: swap    d1\n"                   /* p whole: floor(q * h(p) / 65536) */
        "   tst.w   d0\n"
        "   bne.s   2f\n"
        "   swap    d0\n"
        "   muls.w  d1,d0\n"                /* both whole: h(q) * h(p) */
        "   rts\n"
        "2: move.l  d2,-(sp)\n"
        "   bra.w   .Lfrac\n"
        /* d3:d2 = a * b (signed 64 bits), d4/d5 used */
        ".Lqq64:\n"
        "   movem.l d2-d5,-(sp)\n"
        "   move.l  d0,d5\n"
        "   eor.l   d1,d5\n"                /* bit 31: the sign of the product */
        "   tst.l   d0\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"
        "1: tst.l   d1\n"
        "   bpl.s   2f\n"
        "   neg.l   d1\n"
        "2: move.l  d0,d2\n"
        "   move.l  d0,d3\n"
        "   swap    d3\n"                   /* d3.w = hi(a) */
        "   move.w  d3,d4\n"
        "   mulu.w  d1,d4\n"                /* hi(a) * lo(b) */
        "   mulu.w  d1,d2\n"                /* lo(a) * lo(b) */
        "   swap    d1\n"
        "   mulu.w  d1,d0\n"                /* lo(a) * hi(b) */
        "   mulu.w  d1,d3\n"                /* hi(a) * hi(b) */
        "   add.l   d4,d0\n"                /* the cross products, carry */
        "   bcc.s   3f\n"
        "   add.l   #0x10000,d3\n"          /* the carry is bit 48 */
        "3: move.l  d0,d4\n"
        "   clr.w   d4\n"
        "   swap    d4\n"                   /* d4 = cross >> 16 */
        "   swap    d0\n"
        "   clr.w   d0\n"                   /* d0 = cross << 16 */
        "   add.l   d0,d2\n"
        "   addx.l  d4,d3\n"
        "   tst.l   d5\n"
        "   bpl.s   4f\n"
        "   neg.l   d2\n"
        "   negx.l  d3\n"
        "4: move.l  d3,d0\n"                /* bits 32..63 */
        "   movem.l (sp)+,d2-d5\n"
        "   rts");

/**
 * @brief lqdiv ($D602): d0.l = trunc(d0.l / d1.q), a Long divided by a
 * Quick (16.16) under a Long type so far.
 *
 * The whole-number quotient towards zero, as BB2's Long division (user
 * decisions 5 and 8 of quick-long-arithmetic): (|d0| << 16) / |d1|, the
 * sign applied last.  The paths: a divisor without a fraction divides
 * |d0| by its high word with two DIVU (as ldivlib's quickdiv); a dividend
 * up to $FFFF makes |d0| << 16 a long, two DIVU by a divisor below $10000,
 * else 32 restoring steps; a larger dividend 48 steps (the remainder in d3
 * with its 33rd bit in the carry, the quotient in d4).  A divisor 0 gives
 * the largest number with the dividend's sign, as longdiv (#200); with
 * runtime error checking lqdivchk reports it first.
 *
 * @arg d0 the dividend (a Long)
 * @arg d1 the divisor (a Quick)
 * @par Result
 * d0: the quotient, towards zero
 */
BLITZ_ASM_BLOCK(lqdivide,
        ".Linfinity:\n"
        "   tst.l   d0\n"
        "   bmi.s   1f\n"
        "   move.l  #0x7fffffff,d0\n"
        "   rts\n"
        "1: move.l  #0x80000001,d0\n"
        "   rts\n"
        "   .globl  _lqdivide\n"
        "_lqdivide:\n"
        "   tst.l   d1\n"
        "   beq.s   .Linfinity\n"
        "   movem.l d2-d5,-(sp)\n"          /* the CCR stays: d1's sign */
        "   smi     d2\n"                   /* d2.b: odd number of negatives */
        "   bpl.s   1f\n"
        "   neg.l   d1\n"
        "1: tst.l   d0\n"
        "   bpl.s   2f\n"
        "   neg.l   d0\n"
        "   not.b   d2\n"
        "2: tst.w   d1\n"
        "   bne.s   3f\n"
        "   swap    d1\n"                   /* no fraction: |d0| / hi(|d1|) */
        "   bra.s   .Ldivu\n"
        "3: cmp.l   #0xffff,d0\n"
        "   bhi.s   5f\n"                   /* beyond a word: 48 steps */
        "   swap    d0\n"
        "   clr.w   d0\n"                   /* |d0| << 16 in a long */
        "   cmp.l   #0xffff,d1\n"
        "   bhi.s   4f\n"
        /* d0 = d0 / d1.w with two DIVU (no overflow: the high word is
           below $10000, the remainder below d1.w) */
        ".Ldivu:\n"
        "   moveq   #0,d3\n"
        "   swap    d0\n"
        "   move.w  d0,d3\n"
        "   divu    d1,d3\n"
        "   move.w  d3,d0\n"
        "   swap    d0\n"
        "   move.w  d0,d3\n"
        "   divu    d1,d3\n"
        "   move.w  d3,d0\n"
        "   bra.s   .Lsign\n"
        "4: moveq   #31,d5\n"               /* 32 steps */
        "   bra.s   6f\n"
        "5: moveq   #47,d5\n"               /* 48 steps */
        "6: moveq   #0,d3\n"
        "   moveq   #0,d4\n"
        ".Lstep:\n"
        "   add.l   d4,d4\n"
        "   add.l   d0,d0\n"                /* the next dividend bit in X */
        "   addx.l  d3,d3\n"
        "   bcs.s   7f\n"                   /* 2^32 or more: above the divisor */
        "   cmp.l   d1,d3\n"
        "   bcs.s   8f\n"
        "7: sub.l   d1,d3\n"
        "   addq.l  #1,d4\n"
        "8: dbra    d5,.Lstep\n"
        "   move.l  d4,d0\n"
        ".Lsign:\n"
        "   tst.b   d2\n"
        "   beq.s   9f\n"
        "   neg.l   d0\n"
        "9: movem.l (sp)+,d2-d5\n"
        "   rts");

/**
 * @brief q48add ($D603) and q48sub ($D604): d1:d0 = a + b and a - b,
 * modulo 2^64 (exprarith_q48 ETO_ADD, ETO_SUB).
 *
 * @arg d0/d1 a (low, high)
 * @arg d2/d3 b (low, high)
 * @par Result
 * d0/d1: the sum or difference
 */
BLITZ_ASM_BLOCK(q48add,
        "   .globl  _q48add\n"
        "_q48add:\n"
        "   add.l   d2,d0\n"
        "   addx.l  d3,d1\n"
        "   rts\n"
        "   .globl  _q48sub\n"
        "_q48sub:\n"
        "   sub.l   d2,d0\n"
        "   subx.l  d3,d1\n"
        "   rts");

/**
 * @brief q48mul ($D605): d1:d0 = a * b, the product of the magnitudes
 * shifted right by 16 (cut towards zero to 1/65536), the sign applied,
 * modulo 2^64 (exprarith_q48 ETO_MUL).
 *
 * The magnitudes A = Ah * 2^32 + Al and B = Bh * 2^32 + Bl; the result is
 * (Al * Bl >> 16) + (Ah * Bl + Al * Bh) * 2^16 + Ah * Bh * 2^48.  The fast
 * path, both magnitudes below 2^32 (a 16.16 value, a whole number below
 * 65536 as 48.16, any narrow pair: high register the sign extension of the
 * low one): only Al * Bl >> 16, with two MULU when a factor has no
 * fraction (its low word 0: Al * h(B)), else four (as qqmult's 64 bit
 * product, its bits 16..79 taken).  Else (a wide operand) the cross
 * products Ah * Bl and Al * Bh, four MULU each, their low 48 bits needed,
 * and the low word of Ah * Bh: 13 MULU at most.
 *
 * @arg d0/d1 a (low, high)
 * @arg d2/d3 b (low, high)
 * @par Result
 * d0/d1: the product
 */
BLITZ_ASM_BLOCK(q48mul,
        "   .globl  _q48mulf\n"
        "_q48mulf:\n"
        "   tst.l   d1\n"
        "   bmi.s   1f\n"
        "   tst.l   d3\n"
        "   bpl.s   _q48mul\n"              /* a positive product: q48mul's value */
        "   bra.s   2f\n"
        "1: tst.l   d3\n"
        "   bmi.s   _q48mul\n"
        "2: move.l  d4,-(sp)\n"
        "   move.w  d0,d4\n"
        "   mulu.w  d2,d4\n"                /* the low word of the cut bits */
        "   tst.w   d4\n"
        "   movem.l (sp)+,d4\n"             /* the CCR stays */
        "   beq.s   _q48mul\n"              /* none: q48mul's value */
        "   bsr.s   _q48mul\n"
        "   subq.l  #1,d0\n"                /* one 1/65536 down */
        "   bcc.s   3f\n"
        "   subq.l  #1,d1\n"
        "3: rts\n"
        "   .globl  _q48mul\n"
        "_q48mul:\n"
        "   movem.l d4-d6,-(sp)\n"
        "   move.l  d1,d6\n"
        "   eor.l   d3,d6\n"                /* bit 31: the sign of the product */
        "   clr.w   d6\n"                   /* d6.w 0: both magnitudes below 2^32 */
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"
        "   negx.l  d1\n"
        "1: tst.l   d3\n"
        "   bpl.s   2f\n"
        "   neg.l   d2\n"
        "   negx.l  d3\n"
        "2: tst.l   d1\n"
        "   bne.s   3f\n"
        "   tst.l   d3\n"
        "   beq.s   .Lm32\n"
        "3: movem.l d0-d3,-(sp)\n"          /* Al, Ah, Bl, Bh */
        "   addq.w  #1,d6\n"
        /* d1:d0 = Al * Bl >> 16, d0 = Al, d2 = Bl; d2-d5 changed */
        ".Lm32:\n"
        "   tst.w   d2\n"
        "   beq.s   .Lmwhole\n"
        "   tst.w   d0\n"
        "   bne.s   .Lmfour\n"
        "   exg     d0,d2\n"
        /* d2 = [y1 : 0]: d0 * y1, two MULU */
        ".Lmwhole:\n"
        "   swap    d2\n"
        "   move.l  d0,d1\n"
        "   swap    d1\n"
        "   mulu.w  d2,d1\n"                /* x1 * y1 */
        "   mulu.w  d2,d0\n"                /* x0 * y1 */
        "   move.l  d1,d4\n"
        "   swap    d4\n"
        "   clr.w   d4\n"                   /* x1 * y1 << 16 */
        "   clr.w   d1\n"
        "   swap    d1\n"                   /* x1 * y1 >> 16 */
        "   add.l   d4,d0\n"
        "   moveq   #0,d4\n"
        "   addx.l  d4,d1\n"
        "   bra.s   .Lmdone\n"
        ".Lmfour:\n"
        "   move.l  d2,d3\n"
        "   swap    d3\n"                   /* d3.w = y1 */
        "   move.l  d0,d1\n"
        "   swap    d1\n"                   /* d1.w = x1 */
        "   move.l  d1,d4\n"
        "   mulu.w  d3,d4\n"                /* x1 * y1 */
        "   mulu.w  d2,d1\n"                /* x1 * y0 */
        "   mulu.w  d0,d3\n"                /* x0 * y1 */
        "   mulu.w  d2,d0\n"                /* x0 * y0 */
        "   clr.w   d0\n"
        "   swap    d0\n"                   /* x0 * y0 >> 16 */
        "   add.l   d3,d0\n"                /* no carry: below 2^32 */
        "   add.l   d1,d0\n"                /* X: bit 32 */
        "   moveq   #0,d1\n"
        "   addx.l  d1,d1\n"
        "   move.l  d4,d5\n"
        "   clr.w   d5\n"
        "   swap    d5\n"                   /* x1 * y1 >> 16 */
        "   swap    d4\n"
        "   clr.w   d4\n"                   /* x1 * y1 << 16 */
        "   add.l   d4,d0\n"
        "   addx.l  d5,d1\n"
        ".Lmdone:\n"
        "   tst.w   d6\n"
        "   bne.s   .Lmwide\n"
        ".Lmsign:\n"
        "   tst.l   d6\n"
        "   bpl.s   4f\n"
        "   neg.l   d0\n"
        "   negx.l  d1\n"
        "4: movem.l (sp)+,d4-d6\n"
        "   rts\n"
        /* a wide operand: the stack holds Al, Ah, Bl, Bh */
        ".Lmwide:\n"
        "   move.l  4(sp),d4\n"
        "   beq.s   5f\n"
        "   move.l  8(sp),d5\n"
        "   bsr.s   .Lmcross\n"             /* + Ah * Bl << 16 */
        "5: move.l  12(sp),d5\n"
        "   beq.s   6f\n"
        "   move.l  (sp),d4\n"
        "   bsr.s   .Lmcross\n"             /* + Al * Bh << 16 */
        "   move.l  4(sp),d4\n"
        "   beq.s   6f\n"
        "   mulu.w  14(sp),d4\n"            /* the low word of Ah * Bh, at bit 48 */
        "   swap    d4\n"
        "   clr.w   d4\n"
        "   add.l   d4,d1\n"
        "6: lea     16(sp),sp\n"
        "   bra.s   .Lmsign\n"
        /* d1:d0 += (d4 * d5) << 16 modulo 2^64; d2-d5 changed */
        ".Lmcross:\n"
        "   move.l  d4,d3\n"
        "   swap    d3\n"                   /* d3.w = x1 */
        "   move.l  d3,d2\n"
        "   mulu.w  d5,d2\n"                /* x1 * y0 */
        "   swap    d5\n"
        "   mulu.w  d5,d3\n"                /* x1 * y1, its low word at bit 48 */
        "   swap    d3\n"
        "   clr.w   d3\n"
        "   add.l   d2,d3\n"
        "   move.l  d4,d2\n"
        "   mulu.w  d5,d2\n"                /* x0 * y1 */
        "   add.l   d2,d3\n"
        "   swap    d5\n"
        "   mulu.w  d5,d4\n"                /* x0 * y0 */
        "   moveq   #0,d2\n"
        "   swap    d4\n"
        "   move.w  d4,d2\n"
        "   add.l   d2,d3\n"                /* the high long */
        "   clr.w   d4\n"                   /* the low long */
        "   add.l   d4,d0\n"
        "   addx.l  d3,d1\n"
        "   rts");

/**
 * @brief q48div ($D606), q48divq ($D607), q48divw ($D608), q48divb
 * ($D609): d1:d0 = a / b, (|a| << 16) / |b| cut towards zero (q48divw and
 * q48divb, only under a wanted Word or Byte: down, exprarith_q48_down,
 * #1221), the sign applied, modulo 2^64 (exprarith_q48 ETO_DIV); and
 * q48mod ($D60A): d1:d0 = a Mod b (ETO_MOD), the floor Mod: |a| modulo
 * |b| with a's sign, plus b when that is not 0 and of the other sign
 * (#1197).
 *
 * A divisor 0: the division gives the largest value of the wanted type
 * with the dividend's sign (exprarith_div0: q48div a Long or none
 * +-$7FFFFFFF, q48divq a Quick +-$7FFFFFFF as 16.16 bits, q48divw a Word
 * +-32767, q48divb a Byte +-127; design decision 9), with runtime error
 * checking q48divchk reports it first; Mod gives the dividend, as
 * domodlong.
 *
 * The dividend N as 16 bit digits on the stack (|a| and, for the
 * division, a 0 digit for the << 16), the divisor D = |b|.  The division
 * first drops the low digits of D that are 0 and as many of N (the
 * quotient of N / 65536 by D / 65536 is the same), then (.Lqcore) skips
 * N's leading 0 digits and takes the first path that fits:
 * - D below $10000: one DIVU per digit of N;
 * - D below 2^32: the quotient digit by digit (Knuth's algorithm D with
 *   two divisor digits): D and N shifted left until D's top bit is set,
 *   each digit estimated by a DIVU of the remainder's top 32 bits by D's
 *   high word and corrected by the exact test with D's low word (at most
 *   twice), so no digit needs to be added back;
 * - else (a wide divisor with a fraction, at least 65536.0) bit by bit,
 *   the 64 bit remainder preloaded with N's top 32 bits: at most 48 steps.
 *
 * @arg d0/d1 a, the dividend (low, high)
 * @arg d2/d3 b, the divisor (low, high)
 * @par Result
 * d0/d1: the quotient or the remainder
 */
BLITZ_ASM_BLOCK(q48div,
        "   .globl  _q48divb\n"
        "_q48divb:\n"
        "   tst.l   d2\n"
        "   bne.s   .Ldivf\n"
        "   tst.l   d3\n"
        "   bne.s   .Ldivf\n"
        "   move.l  #0x007f0000,d2\n"       /* 127.0 */
        "   bra.s   .Ldiv0\n"
        "   .globl  _q48divw\n"
        "_q48divw:\n"
        "   tst.l   d2\n"
        "   bne.s   .Ldivf\n"
        "   tst.l   d3\n"
        "   bne.s   .Ldivf\n"
        "   move.l  #0x7fff0000,d2\n"       /* 32767.0 */
        "   bra.s   .Ldiv0\n"
        "   .globl  _q48divq\n"
        "_q48divq:\n"
        "   tst.l   d2\n"
        "   bne.s   .Ldiv\n"
        "   tst.l   d3\n"
        "   bne.s   .Ldiv\n"
        "   move.l  #0x7fffffff,d2\n"       /* the largest Quick */
        "   bra.s   .Ldiv0\n"
        "   .globl  _q48div\n"
        "_q48div:\n"
        "   tst.l   d2\n"
        "   bne.s   .Ldiv\n"
        "   tst.l   d3\n"
        "   bne.s   .Ldiv\n"
        "   move.l  #0xffff0000,d2\n"       /* $7FFFFFFF.0 */
        "   move.w  #0x7fff,d3\n"
        /* d3:d2 the largest value, the dividend's sign */
        ".Ldiv0:\n"
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   neg.l   d2\n"
        "   negx.l  d3\n"
        "1: move.l  d2,d0\n"
        "   move.l  d3,d1\n"
        "   rts\n"
        ".Ldivf:\n"
        "   movem.l d4-d7/a0-a1,-(sp)\n"
        "   moveq   #1,d4\n"                /* the cut down (q48divw, q48divb) */
        "   bra.s   5f\n"
        ".Ldiv:\n"
        "   movem.l d4-d7/a0-a1,-(sp)\n"
        "   moveq   #0,d4\n"
        "5: move.l  d1,d5\n"
        "   eor.l   d3,d5\n"                /* bit 31: the sign of the quotient */
        "   clr.w   d5\n"
        "   or.w    d4,d5\n"                /* bit 0: the cut down */
        "   move.l  d5,-(sp)\n"
        "   bsr.w   .Lqmag\n"
        "   tst.l   d4\n"
        "   beq.s   6f\n"
        "   tst.l   d2\n"                   /* the digits of N the drop below leaves out: */
        "   bne.s   6f\n"
        "   tst.w   d0\n"                   /* D's low long 0: |a|'s low word, */
        "   bne.s   7f\n"
        "   tst.w   d3\n"
        "   bne.s   6f\n"
        "   tst.l   d0\n"                   /* D's low 48 bits 0: its low long */
        "   beq.s   6f\n"
        "7: bset    #1,3(sp)\n"             /* bit 1: not 0 */
        "6: clr.w   -(sp)\n"                /* the pad digit of .Lqcore */
        "   clr.w   -(sp)\n"                /* N = |a| << 16 */
        "   bsr.w   .Lqpush\n"
        "   moveq   #5,d6\n"
        "2: tst.w   d2\n"
        "   bne.s   3f\n"
        "   move.w  d3,d2\n"                /* D >> 16, a digit of N less */
        "   swap    d2\n"
        "   clr.w   d3\n"
        "   swap    d3\n"
        "   subq.w  #1,d6\n"
        "   bra.s   2b\n"
        "3: bsr     .Lqcore\n"
        "   lea     14(sp),sp\n"
        "   move.l  (sp)+,d4\n"
        "   bpl.s   4f\n"
        "   btst    #0,d4\n"
        "   beq.s   8f\n"                   /* towards zero */
        "   btst    #1,d4\n"
        "   bne.s   9f\n"
        "   or.l    d3,d2\n"
        "   beq.s   8f\n"                   /* exact */
        "9: addq.l  #1,d0\n"                /* down: one 1/65536 more before the sign */
        "   bcc.s   8f\n"
        "   addq.l  #1,d1\n"
        "8: neg.l   d0\n"
        "   negx.l  d1\n"
        "4: movem.l (sp)+,d4-d7/a0-a1\n"
        "   rts\n"
        "   .globl  _q48mod\n"
        "_q48mod:\n"
        "   tst.l   d2\n"
        "   bne.s   1f\n"
        "   tst.l   d3\n"
        "   bne.s   1f\n"
        "   rts\n"                          /* Mod 0: the dividend */
        "1: movem.l d4-d7/a0-a1,-(sp)\n"
        "   move.l  d3,-(sp)\n"
        "   move.l  d2,-(sp)\n"             /* b */
        "   move.l  d1,-(sp)\n"             /* bit 31: a's sign */
        "   bsr.w   .Lqmag\n"
        "   clr.w   -(sp)\n"                /* the pad digit */
        "   bsr.w   .Lqpush\n"              /* N = |a| */
        "   moveq   #4,d6\n"
        "   bsr     .Lqcore\n"
        "   lea     12(sp),sp\n"
        "   move.l  d2,d0\n"
        "   move.l  d3,d1\n"
        "   move.l  (sp)+,d4\n"             /* bit 31: a's sign */
        "   or.l    d3,d2\n"
        "   beq.s   2f\n"                   /* 0 */
        "   tst.l   d4\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"                   /* a's sign: the remainder towards zero */
        "   negx.l  d1\n"
        "1: move.l  4(sp),d3\n"
        "   eor.l   d3,d4\n"
        "   bpl.s   2f\n"                   /* b's sign */
        "   add.l   (sp),d0\n"              /* the other sign: plus b */
        "   addx.l  d3,d1\n"
        "2: addq.l  #8,sp\n"
        "   movem.l (sp)+,d4-d7/a0-a1\n"
        "   rts\n"
        /* the magnitudes of a (d1:d0) and b (d3:d2) */
        ".Lqmag:\n"
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"
        "   negx.l  d1\n"
        "1: tst.l   d3\n"
        "   bpl.s   2f\n"
        "   neg.l   d2\n"
        "   negx.l  d3\n"
        "2: rts\n"
        /* the four digits of |a| (d1:d0) onto the caller's stack, the
           highest at the lowest address, a 0 digit in front of them;
           a0 = the highest digit */
        ".Lqpush:\n"
        "   move.l  (sp)+,a1\n"
        "   move.w  d0,-(sp)\n"
        "   swap    d0\n"
        "   move.w  d0,-(sp)\n"
        "   move.w  d1,-(sp)\n"
        "   swap    d1\n"
        "   move.w  d1,-(sp)\n"
        "   clr.w   -(sp)\n"
        "   lea     2(sp),a0\n"
        "   jmp     (a1)\n"
        /* d1:d0 = N / D modulo 2^64, d3:d2 = N modulo D: N the d6.w digits
           from a0 (a 0 digit in front of them, one more behind them for
           the pad), D = d3:d2, not 0; d4-d7, a0, a1 changed */
        ".Lqcore:\n"
        "1: tst.w   (a0)\n"
        "   bne.s   2f\n"
        "   addq.l  #2,a0\n"                /* a leading 0 digit */
        "   subq.w  #1,d6\n"
        "   bne.s   1b\n"
        "   moveq   #0,d0\n"                /* N = 0 */
        "   moveq   #0,d1\n"
        "   moveq   #0,d2\n"
        "   moveq   #0,d3\n"
        "   rts\n"
        "2: tst.l   d3\n"
        "   bne     .Lqwide\n"
        "   cmp.l   #0xffff,d2\n"
        "   bhi.s   .Lqknuth\n"
        /* D below $10000: one DIVU per digit (the remainder below D, so
           no overflow) */
        "   moveq   #0,d0\n"
        "   moveq   #0,d1\n"
        "   moveq   #0,d4\n"
        "   subq.w  #1,d6\n"
        "3: move.w  (a0)+,d4\n"             /* [remainder : digit] */
        "   divu.w  d2,d4\n"                /* [remainder : quotient digit] */
        "   swap    d1\n"
        "   swap    d0\n"
        "   move.w  d0,d1\n"
        "   move.w  d4,d0\n"                /* Q = Q << 16 | digit */
        "   dbra    d6,3b\n"
        "   clr.w   d4\n"
        "   swap    d4\n"
        "   move.l  d4,d2\n"
        "   rts\n"
        /* D below 2^32: the digits of N << s by D << s (s: D's top bit
           set), d2 = [v1 : v0], d3 the remainder r (below D), d5 = s,
           a0 the next digit, a1 behind the last one */
        ".Lqknuth:\n"
        "   move.w  d6,d4\n"
        "   add.w   d4,d4\n"
        "   lea     0(a0,d4.w),a1\n"
        "   clr.w   (a1)\n"                 /* the pad: N's shifted last digit */
        "   moveq   #0,d5\n"
        "   cmp.l   #0x00ffffff,d2\n"
        "   bhi.s   4f\n"
        "   lsl.l   #8,d2\n"
        "   moveq   #8,d5\n"
        "4: tst.l   d2\n"
        "   bmi.s   6f\n"
        "5: addq.w  #1,d5\n"
        "   add.l   d2,d2\n"
        "   bpl.s   5b\n"
        "6: subq.l  #2,a0\n"                /* the 0 digit in front */
        "   moveq   #0,d0\n"
        "   moveq   #0,d1\n"
        "   move.l  (a0),d3\n"
        "   addq.l  #2,a0\n"
        "   lsl.l   d5,d3\n"
        "   move.l  (a0),d7\n"
        "   addq.l  #2,a0\n"
        "   lsl.l   d5,d7\n"
        "   swap    d7\n"
        "   move.w  d7,d3\n"                /* r: the top two digits of N << s */
        ".Lqkloop:\n"
        "   cmpa.l  a1,a0\n"
        "   beq.s   .Lqkdone\n"
        "   move.l  (a0),d7\n"
        "   addq.l  #2,a0\n"
        "   lsl.l   d5,d7\n"
        "   swap    d7\n"                   /* d7.w = u, the next digit */
        "   swap    d1\n"
        "   swap    d0\n"
        "   move.w  d0,d1\n"                /* Q << 16, the digit in d0.w */
        "   move.l  d3,d4\n"
        "   swap    d4\n"
        "   swap    d2\n"                   /* d2.w = v1 */
        "   cmp.w   d2,d4\n"
        "   beq.s   .Lqkmax\n"
        "   move.l  d3,d4\n"
        "   divu.w  d2,d4\n"                /* [rhat : qhat] = r / v1 */
        "   swap    d2\n"
        "   move.w  d4,d0\n"
        "   move.w  d0,d6\n"
        "   mulu.w  d2,d6\n"                /* qhat * v0 */
        "   move.w  d7,d4\n"                /* [rhat : u] */
        ".Lqktest:\n"
        "   cmp.l   d6,d4\n"
        "   bcc.s   .Lqkok\n"               /* qhat * v0 <= rhat : u */
        "   subq.w  #1,d0\n"
        "   moveq   #0,d7\n"
        "   move.w  d2,d7\n"
        "   sub.l   d7,d6\n"
        "   move.l  d2,d7\n"
        "   clr.w   d7\n"
        "   add.l   d7,d4\n"                /* rhat + v1 */
        "   bcc.s   .Lqktest\n"             /* rhat 65536 or more: qhat holds */
        ".Lqkok:\n"
        "   sub.l   d6,d4\n"
        "   move.l  d4,d3\n"                /* r = rhat : u - qhat * v0 */
        "   bra.s   .Lqkloop\n"
        /* r's high word v1: qhat = $FFFF, rhat = r's low word + v1 */
        ".Lqkmax:\n"
        "   swap    d2\n"
        "   move.w  #-1,d0\n"
        "   move.w  d3,d4\n"
        "   swap    d4\n"
        "   move.w  d7,d4\n"
        "   move.l  d2,d7\n"
        "   clr.w   d7\n"
        "   add.l   d7,d4\n"
        "   bcs.s   7f\n"
        "   move.w  d0,d6\n"
        "   mulu.w  d2,d6\n"
        "   bra.s   .Lqktest\n"
        "7: move.w  d0,d6\n"
        "   mulu.w  d2,d6\n"
        "   bra.s   .Lqkok\n"
        ".Lqkdone:\n"
        "   move.l  d3,d2\n"
        "   lsr.l   d5,d2\n"                /* the remainder of N by D */
        "   moveq   #0,d3\n"
        "   rts\n"
        /* D 2^32 or more: the remainder d5:d4 preloaded with N's top two
           digits, the others top-aligned in d1:d0, one step per bit of
           them; the quotient bits come into d0 from the right */
        ".Lqwide:\n"
        "   moveq   #0,d4\n"
        "   moveq   #0,d5\n"
        "   moveq   #0,d0\n"
        "   moveq   #0,d1\n"
        "   move.w  (a0)+,d4\n"
        "   subq.w  #1,d6\n"
        "   beq.s   .Lqwdone\n"
        "   swap    d4\n"
        "   move.w  (a0)+,d4\n"
        "   subq.w  #1,d6\n"
        "   beq.s   .Lqwdone\n"
        "   move.w  d6,d7\n"
        "   lsl.w   #4,d7\n"
        "   subq.w  #1,d7\n"                /* 16 steps per digit, dbra */
        "   move.w  (a0)+,d1\n"
        "   swap    d1\n"
        "   subq.w  #1,d6\n"
        "   beq.s   .Lqwloop\n"
        "   move.w  (a0)+,d1\n"
        "   subq.w  #1,d6\n"
        "   beq.s   .Lqwloop\n"
        "   move.w  (a0)+,d0\n"
        "   swap    d0\n"
        ".Lqwloop:\n"
        "   add.l   d0,d0\n"
        "   addx.l  d1,d1\n"
        "   addx.l  d4,d4\n"
        "   addx.l  d5,d5\n"
        "   bcs.s   8f\n"                   /* 2^64 or more: above D */
        "   cmp.l   d3,d5\n"
        "   bhi.s   8f\n"
        "   bcs.s   9f\n"
        "   cmp.l   d2,d4\n"
        "   bcs.s   9f\n"
        "8: sub.l   d2,d4\n"
        "   subx.l  d3,d5\n"
        "   addq.l  #1,d0\n"
        "9: dbra    d7,.Lqwloop\n"
        ".Lqwdone:\n"
        "   move.l  d4,d2\n"
        "   move.l  d5,d3\n"
        "   rts");

/**
 * @brief The casts of a 48.16 value (exprarith_whole_q48, exprarith_q16_q48,
 * exprarith_q48_floor and exprarith_q48_trunc with ET_LONG):
 * l2q48 ($D60B) d1:d0 = the Long d0 as 48.16, q2q48 ($D60C) d1:d0 = the
 * Quick d0 as 48.16, q48floor ($D60D) d0 = d1:d0 rounded down, q48trunc
 * ($D60E) d0 = d1:d0 towards zero, the Long's 32 bits (a whole part beyond
 * a Long wraps).
 *
 * @arg d0 (l2q48, q2q48) or d0/d1 (q48floor, q48trunc) the value
 * @par Result
 * d0/d1 (l2q48, q2q48) or d0 (q48floor, q48trunc)
 */
BLITZ_ASM_BLOCK(q48casts,
        "   .globl  _l2q48\n"
        "_l2q48:\n"
        "   move.l  d0,d1\n"
        "   swap    d1\n"
        "   ext.l   d1\n"                   /* the Long >> 16 */
        "   swap    d0\n"
        "   clr.w   d0\n"                   /* the Long << 16 */
        "   rts\n"
        "   .globl  _q2q48\n"
        "_q2q48:\n"
        "   move.l  d0,d1\n"
        "   add.l   d1,d1\n"
        "   subx.l  d1,d1\n"                /* the sign extension */
        "   rts\n"
        "   .globl  _q48trunc\n"
        "_q48trunc:\n"
        "   tst.l   d1\n"
        "   bpl.s   _q48floor\n"
        "   tst.w   d0\n"
        "   beq.s   _q48floor\n"
        "   move.w  d1,d0\n"                /* negative with a fraction: */
        "   swap    d0\n"
        "   addq.l  #1,d0\n"                /* the floor + 1 */
        "   rts\n"
        "   .globl  _q48floor\n"
        "_q48floor:\n"
        "   move.w  d1,d0\n"
        "   swap    d0\n"                   /* bits 16..47 */
        "   rts");

/**
 * @brief q16mod ($D60F): d0.l = d0 Mod d1, two Quicks (16.16), the exact
 * floor Mod (exprarith_q16 ETO_MOD, also q48mod's low 32 bits): r = |a|
 * Mod |b| with a's sign, plus b when that is not 0 and of the other sign
 * (#1197; modulo 2^32).
 * The caller has checked b against 0 (the trap of a Mod by 0 is its
 * code's).
 *
 * |b| below $10000 (a divisor below 1): two DIVU, the high word of |a|
 * (at most $8000, so its quotient fits) and the remainder above the low
 * word.  Else |a| / |b| is at most 32768: both shifted right by s, the
 * fewest steps that leave b' = |b| >> s below $10000 (b' >= $8000), and
 * q = (|a| >> s) / b' by one DIVU (a word: |a| >> s is at most 2^30).  q
 * is never too small (|a| >= q0 |b| gives |a| >> s >= q0 b') and at most
 * 2 too large ((q0 + 1)(1 + 1/b') < q0 + 3 for q0 <= 32768); its product
 * with |b| (two MULU) is below 2^31 + 2^16 (q |b| <= |a| |b| / (2^s b')
 * < |a| (b' + 1) / b'), so it is exact in 32 bits and |a| - q |b| borrows
 * exactly when it is negative; then |b| is added until the sum carries
 * past 2^32 (at most twice: the remainder is at least -2 |b| > -2^32),
 * which it does first at the remainder in 0..|b|-1.
 *
 * @arg d0 a
 * @arg d1 b (not 0)
 * @par Result
 * d0: a Mod b
 */
BLITZ_ASM_BLOCK(q16mod,
        "   .globl  _q16mod\n"
        "_q16mod:\n"
        "   movem.l d2-d5,-(sp)\n"
        "   move.l  d1,d4\n"
        "   bpl.s   1f\n"
        "   neg.l   d4\n"                   /* |b| */
        "1: move.l  d0,d2\n"
        "   bpl.s   2f\n"
        "   neg.l   d2\n"                   /* |a|; d0: a's sign */
        "2: cmp.l   #0xffff,d4\n"
        "   bhi.s   .Lqmbig\n"
        "   moveq   #0,d3\n"                /* |b| below $10000: two DIVU */
        "   swap    d2\n"
        "   move.w  d2,d3\n"
        "   beq.s   3f\n"
        "   divu.w  d4,d3\n"
        "3: swap    d2\n"
        "   move.w  d2,d3\n"
        "   divu.w  d4,d3\n"
        "   clr.w   d3\n"
        "   swap    d3\n"                   /* the remainder */
        "   bra.s   .Lqmsign\n"
        ".Lqmbig:\n"
        "   move.l  d4,d3\n"
        "   move.l  d2,d5\n"
        "4: lsr.l   #1,d3\n"
        "   lsr.l   #1,d5\n"
        "   cmp.l   #0xffff,d3\n"
        "   bhi.s   4b\n"
        "   divu.w  d3,d5\n"                /* q, at most 2 too large */
        "   move.l  d4,d3\n"
        "   swap    d3\n"
        "   mulu.w  d5,d3\n"
        "   swap    d3\n"
        "   clr.w   d3\n"                   /* q * hi(|b|) << 16 */
        "   mulu.w  d4,d5\n"
        "   add.l   d3,d5\n"                /* q * |b|, below 2^32 */
        "   move.l  d2,d3\n"
        "   sub.l   d5,d3\n"                /* |a| - q * |b| */
        "   bcc.s   .Lqmsign\n"
        "5: add.l   d4,d3\n"
        "   bcc.s   5b\n"                   /* until it carries */
        ".Lqmsign:\n"
        "   tst.l   d3\n"
        "   beq.s   6f\n"                   /* 0 */
        "   tst.l   d0\n"
        "   bpl.s   7f\n"
        "   neg.l   d3\n"                   /* a's sign: the remainder towards zero */
        "7: eor.l   d1,d0\n"
        "   bpl.s   6f\n"                   /* b's sign */
        "   add.l   d1,d3\n"                /* the other sign: plus b */
        "6: move.l  d3,d0\n"
        "   movem.l (sp)+,d2-d5\n"
        "   rts");

/**
 * @brief q48mull ($D610): d1:d0 = d0 * d2, a Long times a Quick (16.16)
 * in either order as 48.16: the signed 64-bit product of the two 32-bit
 * values, which is q48mul's value for the Long widened (l << 16 times q,
 * shifted right by 16: nothing is cut; exprarith_q48 ETO_MUL).
 *
 * The magnitudes x and y: a factor below $10000 (after the swap: y) gives
 * x * y with two MULU, else four (x1 y1 << 32, the cross products << 16:
 * with magnitudes up to 2^31 their sum fits 32 bits, x0 y0); the sign
 * applied in 64 bits.
 *
 * @arg d0 a
 * @arg d2 b
 * @par Result
 * d0/d1: the product (low, high); d2, d3 changed
 */
BLITZ_ASM_BLOCK(q48mull,
        "   .globl  _q48mull\n"
        "_q48mull:\n"
        "   move.l  d4,-(sp)\n"
        "   move.l  d0,d4\n"
        "   eor.l   d2,d4\n"                /* bit 31: the sign of the product */
        "   tst.l   d0\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"
        "1: tst.l   d2\n"
        "   bpl.s   2f\n"
        "   neg.l   d2\n"
        "2: move.l  d2,d3\n"
        "   swap    d3\n"
        "   tst.w   d3\n"
        "   beq.s   .Llword\n"              /* y below $10000 */
        "   move.l  d0,d1\n"
        "   swap    d1\n"
        "   tst.w   d1\n"
        "   bne.s   .Llfour\n"
        "   exg     d0,d2\n"                /* x below $10000: the other way */
        /* d1:d0 = d0 * d2.w */
        ".Llword:\n"
        "   move.l  d0,d1\n"
        "   swap    d1\n"
        "   mulu.w  d2,d1\n"                /* x1 * y */
        "   mulu.w  d2,d0\n"                /* x0 * y */
        "   move.l  d1,d3\n"
        "   swap    d3\n"
        "   clr.w   d3\n"                   /* << 16, its low part */
        "   clr.w   d1\n"
        "   swap    d1\n"                   /* its high part */
        "   add.l   d3,d0\n"
        "   moveq   #0,d3\n"
        "   addx.l  d3,d1\n"
        ".Llsign:\n"
        "   tst.l   d4\n"
        "   bpl.s   3f\n"
        "   neg.l   d0\n"
        "   negx.l  d1\n"
        "3: move.l  (sp)+,d4\n"
        "   rts\n"
        /* d1.w = x1, d3.w = y1, both not 0 */
        ".Llfour:\n"
        "   move.l  d5,-(sp)\n"
        "   move.w  d1,d5\n"
        "   mulu.w  d2,d5\n"                /* x1 * y0 */
        "   mulu.w  d3,d1\n"                /* x1 * y1 */
        "   mulu.w  d0,d3\n"                /* x0 * y1 */
        "   mulu.w  d2,d0\n"                /* x0 * y0 */
        "   add.l   d5,d3\n"                /* the cross products: no carry, each below 2^31 */
        "   move.l  d3,d2\n"
        "   clr.w   d2\n"
        "   swap    d2\n"
        "   add.l   d2,d1\n"                /* + cross >> 16 */
        "   swap    d3\n"
        "   clr.w   d3\n"
        "   add.l   d3,d0\n"                /* + cross << 16 */
        "   moveq   #0,d3\n"
        "   addx.l  d3,d1\n"
        "   move.l  (sp)+,d5\n"
        "   bra.s   .Llsign");

/**
 * @brief The conversions of type-system-rework (task 4.3; D-027), with the
 * values of exprarith.c (exprarith_q48_quick, exprarith_ffp_whole,
 * exprarith_ffp_quick):
 *
 * q48sat ($D612) d0 = the 48.16 value d1:d0 as a Quick: the low 32 bits
 * when the high 32 bits are their sign extension, else $7FFFFFFF above
 * and $80000000 below the Quick range (design.md P2).
 *
 * f2l ($D613) d0 = the Float d0 cut towards zero, the low 32 bits: in the
 * Long range SPFix's value, beyond it the value wraps (I8 A+C) where
 * SPFix saturates; a Word or a Byte is the low word of the Long, ext.l.
 * FFP (exprarith.h): the mantissa M (bits 31-8, the low byte cleared) /
 * 2^32 * 2^e with e = the exponent (bits 6-0) - 64, the sign in bit 7.
 * The magnitude is M >> (32 - e) for 1 <= e <= 32 (below 16 as (M >>
 * 16) >> (16 - e) by SWAP and a word shift: fewer cycles for the common
 * small values) and M << (e - 32) modulo 2^32 above (e - 32 is at most
 * 31), 0 for e <= 0 (and for M 0).
 *
 * f2q ($D614) d0 = the Float d0 as a Quick, rounded down to 1/65536 and
 * saturated as q48sat: the value * 65536 is M / 2^s with s = 16 - e.
 * e >= 16 (s <= 0) is at least 32768 (bit 31 of M set): saturated (M 0
 * gives 0).  Else the magnitude M >> s (s above 32 counts as 32: 0), and
 * for a negative value -(floor((M - 1) / 2^s) + 1) = ~((M - 1) >> s),
 * the magnitude rounded up (M 0, the Float -0, gives 0).
 *
 * The sign is kept in bit 31 of d1 (the byte of sign and exponent,
 * extended), the count in its low byte.
 *
 * @arg d0 (q48sat: d0/d1) the value
 * @par Result
 * d0; q48sat keeps d1, f2l and f2q change it (qtof2's contract of a
 * conversion: d1 free); every other register kept
 */
BLITZ_ASM_BLOCK(convs,
        "   .globl  _q48sat\n"
        "_q48sat:\n"
        "   tst.l   d1\n"
        "   bmi.s   .Lqsneg\n"
        "   bne.s   .Lqsmax\n"              /* the high 32 bits above 0 */
        "   tst.l   d0\n"
        "   bmi.s   .Lqsmax\n"              /* high 0, low 2^31 or more */
        "   rts\n"
        ".Lqsmax:\n"
        "   move.l  #0x7fffffff,d0\n"
        "   rts\n"
        ".Lqsneg:\n"
        "   not.l   d1\n"
        "   beq.s   1f\n"                   /* high -1 */
        "   not.l   d1\n"
        "   bra.s   .Lqsmin\n"
        "1: not.l   d1\n"
        "   tst.l   d0\n"
        "   bmi.s   2f\n"                   /* high -1, low negative: fits */
        ".Lqsmin:\n"
        "   move.l  #0x80000000,d0\n"
        "2: rts\n"
        "   .globl  _f2l\n"
        "_f2l:\n"
        "   move.b  d0,d1\n"
        "   ext.w   d1\n"
        "   ext.l   d1\n"                   /* bit 31: the sign */
        "   andi.b  #0x7f,d1\n"
        "   subi.b  #64+16,d1\n"            /* e - 16 */
        "   bmi.s   .Lflsmall\n"
        "   subi.b  #16,d1\n"               /* e - 32 */
        "   bpl.s   .Lflwide\n"
        "   neg.b   d1\n"                   /* 32 - e, 1..16 */
        "   clr.b   d0\n"
        "   lsr.l   d1,d0\n"
        "   bra.s   .Lflsign\n"
        ".Lflsmall:\n"
        "   cmpi.b  #-16,d1\n"
        "   ble.s   .Lflzero\n"             /* e <= 0 */
        "   neg.b   d1\n"                   /* 16 - e, 1..15 */
        "   clr.w   d0\n"
        "   swap    d0\n"                   /* M >> 16 */
        "   lsr.w   d1,d0\n"
        "   bra.s   .Lflsign\n"
        ".Lflwide:\n"
        "   clr.b   d0\n"
        "   lsl.l   d1,d0\n"                /* e - 32, 0..31: wraps */
        ".Lflsign:\n"
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   neg.l   d0\n"
        "1: rts\n"
        ".Lflzero:\n"
        "   moveq   #0,d0\n"
        "   rts\n"
        "   .globl  _f2q\n"
        "_f2q:\n"
        "   move.b  d0,d1\n"
        "   ext.w   d1\n"
        "   ext.l   d1\n"                   /* bit 31: the sign */
        "   andi.b  #0x7f,d1\n"
        "   subi.b  #64+16,d1\n"            /* e - 16 */
        "   bpl.s   .Lfqsat\n"
        "   neg.b   d1\n"                   /* s = 16 - e, 1..80 */
        "   cmpi.b  #32,d1\n"
        "   bls.s   1f\n"
        "   move.b  #32,d1\n"               /* below 2^-16: as 32 */
        "1: clr.b   d0\n"
        "   tst.l   d1\n"
        "   bmi.s   .Lfqneg\n"
        "   lsr.l   d1,d0\n"
        "   rts\n"
        ".Lfqneg:\n"
        "   subq.l  #1,d0\n"
        "   bcs.s   .Lfqzero\n"             /* M 0: -0 */
        "   lsr.l   d1,d0\n"
        "   not.l   d0\n"
        "   rts\n"
        ".Lfqsat:\n"
        "   clr.b   d0\n"
        "   tst.l   d0\n"
        "   beq.s   .Lfqzero\n"             /* M 0 */
        "   move.l  #0x7fffffff,d0\n"
        "   tst.l   d1\n"
        "   bpl.s   1f\n"
        "   not.l   d0\n"                   /* $80000000 */
        "1: rts\n"
        ".Lfqzero:\n"
        "   moveq   #0,d0\n"
        "   rts");

/* ------------------------------------------- error checks (debugger only) */

/*
 * lqdivchk - the check of lqdiv, d1 the divisor, as ldivlib's divchk: a
 * divisor 0 executes DIVU.W with it, the CPU's own division trap, which
 * the debugger reports as for a Word division ("GURU! - Division by
 * Zero"); d0/d1 stay as they are.
 */
BLITZ_ERRCHK_ASM(lqdivchk,
        "   .globl  _lqdivchk\n"
        "_lqdivchk:\n"
        "   tst.l   d1\n"
        "   bne.s   1f\n"
        "   divu.w  d1,d0\n"
        "1: rts");

/*
 * q48divchk - the check of the 48.16 divisions, d3:d2 the divisor, as
 * lqdivchk: a divisor 0 executes DIVU.W with d2 (0), the CPU's division
 * trap; the registers stay as they are.
 */
BLITZ_ERRCHK_ASM(q48divchk,
        "   .globl  _q48divchk\n"
        "_q48divchk:\n"
        "   tst.l   d2\n"
        "   bne.s   1f\n"
        "   tst.l   d3\n"
        "   bne.s   1f\n"
        "   divu.w  d2,d0\n"
        "1: rts");
