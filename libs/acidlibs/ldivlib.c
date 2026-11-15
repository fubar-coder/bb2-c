/*
 * ldivlib - system library 64735 ($CB00-$CB01): 32 bit and Quick division
 * for 68000 generated code (the compiler calls it for `/` on Long and
 * Quick, tokens quickdiv/longdiv, blitz2.s equates 175-176).
 *
 * Translation of bblibsrc/ldivlib (Acid Software), in assembler for speed
 * (a hot path: every `/` on Long and Quick).  The results are the
 * original's bit for bit, including those for overflow: a restoring
 * shift/subtract division on the magnitudes, NOT the same as C's `/` in
 * all cases (Quick: below).  Where the divisor fits in 16 bits the
 * quotient comes from two DIVU instead of the loop, with the same result.
 * The result of a division by 0 has the dividend's sign (fixed, #200).
 *
 * Errors (debugger, divchk): "Division by Zero" for a divisor 0 (#200;
 * the original has no error check area).  The compiler never runs the
 * check at compile time: it refuses a constant divisor 0 itself (#1044).
 */
#include "blitzlib.h"

void qdivide(void), ldivide(void);
void divchk(void);

/* !syslibheader{#ldivlib,$cb00,$cb02,0,0,0,errors,$4000}: the original
   has no error check area (0); divchk (#200) is its first check */
BLITZ_SYSLIBHEADER(LIB_ldivlib, 0xcb00, 0xcb02, 0, 0, 0, BLITZ_ERROR_AREA,
                   0x4000);
BLITZ_NULLSUB(qdivide, divchk, 0);      /* $CB00 quickdiv */
BLITZ_NULLSUB(ldivide, divchk, 0);      /* $CB01 longdiv */
BLITZ_LIBFIN();

/*
 * quickdiv ($CB00, qdivide): Quick division (16.16 fixed point), d0.q =
 * d0.q / d1.q; longdiv ($CB01, ldivide): Long division, d0.l = d0.l /
 * d1.l.  Parameters: d0 the dividend, d1 the divisor; result: d0 the
 * quotient.  d1 is changed; every other register is kept, a0/a1 too
 * (the compiler keeps intermediates in d2-d7 across an operator; third
 * party: RWEMathLib calls quickdiv with d7 live), so the entries need no
 * thunk.
 *
 * The original's algorithm ('loop): the signs are removed first (d2: odd
 * number of negatives) and applied to the quotient at the end.  The
 * divisor is shifted right out of d1 into d3 from the top; once d1 is
 * empty each step shifts d3 one further right and subtracts it from the
 * dividend (the remainder in the end) where it fits, one quotient bit in
 * d4 per step.  32 steps for Long (dbra count 31) give the exact quotient of
 * the magnitudes.  Quick has 16 more (count 47): the 16 fraction bits,
 * computed with the divisor shifted right past bit 0: its lost low bits
 * make a fraction bit 1 where an exact division gives 0 (1/3.25 is
 * $00004EC5 here, $00004EC4 exact); quotient bits above bit 31 are lost.
 *
 * The fast paths give the same bits as the loop:
 *  - Long, divisor < $10000: two DIVU, the high word, then the remainder
 *    with the low word: the exact quotient of the magnitudes.
 *  - Quick, divisor with a zero fraction (low word 0): the loop divides
 *    exactly then, by $10000 times the divisor's high word, so its 48
 *    bits are the dividend / that high word: the same two DIVU by it.
 *  - Quick, divisor below 1.0 (< $10000) with a fraction: the integer
 *    part and its remainder with the two DIVU, then the loop's 16
 *    fraction steps on words (remainder and shifted divisor are below
 *    $10000).
 * Any other divisor (from $10000 on; for Quick with a fraction) runs the
 * loop, with the same steps but faster: the original's first steps only
 * move the divisor bit by bit into d3 until d1 is empty (one step per bit
 * below its top bit m, no quotient bit), so the divisor is normalised
 * first instead (shifted left until bit 31 is set: by 8, 4 and single
 * bits), and the 32 - m (Long) or 48 - m (Quick) compare steps follow
 * without the original's tests of d1 and of d3 = 0 (for a divisor from
 * $10000 on d3 is never 0 there: its last value is the divisor >> 16).
 * Cycles per call on the A500 (tests/perf, the call included): 123456/789
 * 519 (the original 2661), Quick 12.5/3.25 (the loop) 3648 before the
 * normalisation, 1965 now (the original 3820).
 *
 * A divisor 0 gives the largest number with the dividend's sign, no
 * error without the debugger (with it, divchk reports "Division by Zero"
 * first): $7FFFFFFF for a dividend >= 0, -$7FFFFFFF ($80000001) for a
 * negative one ('infinity).  Fixed (#200): the original returned
 * $7FFFFFFF ("LARGEST POSITIVE NUMBER") for any sign, it tested the
 * divisor before the signs.
 */
BLITZ_ASM_BLOCK(divide,
        ".Linfinity:\n"
        "   tst.l   d0\n"
        "   bmi.s   5f\n"
        "   move.l  #0x7fffffff,d0\n"
        "   rts\n"
        "5: move.l  #0x80000001,d0\n"
        "   rts\n"
        "   .globl  _qdivide\n"
        "_qdivide:\n"
        "   tst.l   d1\n"
        "   beq.s   .Linfinity\n"
        "   movem.l d2-d3,-(sp)\n"          /* the CCR stays: d1's sign */
        "   smi     d2\n"                   /* d2.b: odd number of negatives */
        "   bpl.s   1f\n"
        "   neg.l   d1\n"
        "1: tst.l   d0\n"
        "   bpl.s   2f\n"
        "   neg.l   d0\n"
        "   not.b   d2\n"
        "2: tst.w   d1\n"
        "   bne.s   .Lqfraction\n"
        "   swap    d1\n"                   /* a zero fraction: d0 / d1.w */
        "   bra.s   .Ldivu\n"
        ".Lqfraction:\n"
        "   cmp.l   #0xffff,d1\n"
        "   bhi.s   .Lqloop\n"
        /* below 1.0: the integer part as at .Ldivu */
        "   moveq   #0,d3\n"
        "   swap    d0\n"
        "   move.w  d0,d3\n"
        "   divu    d1,d3\n"
        "   move.w  d3,d0\n"
        "   swap    d0\n"
        "   move.w  d0,d3\n"
        "   divu    d1,d3\n"
        "   move.w  d3,d0\n"
        "   swap    d3\n"                   /* d3.w: its remainder */
        "   move.l  d4,-(sp)\n"
        "   moveq   #15,d4\n"
        ".Lqstep:\n"                        /* the fraction steps of 'loop */
        "   add.l   d0,d0\n"
        "   lsr.w   #1,d1\n"
        "   beq.s   3f\n"
        "   cmp.w   d1,d3\n"
        "   bcs.s   3f\n"
        "   sub.w   d1,d3\n"
        "   addq.w  #1,d0\n"
        "3: dbra    d4,.Lqstep\n"
        "   move.l  (sp)+,d4\n"
        "   bra.s   .Lsign\n"
        ".Lqloop:\n"
        "   move.l  d4,-(sp)\n"
        "   moveq   #16,d4\n"               /* 16 fraction steps more */
        "   bra.s   .Lnorm\n"
        "   .globl  _ldivide\n"
        "_ldivide:\n"
        "   tst.l   d1\n"
        "   beq.s   .Linfinity\n"
        "   movem.l d2-d3,-(sp)\n"          /* the CCR stays: d1's sign */
        "   smi     d2\n"
        "   bpl.s   1f\n"
        "   neg.l   d1\n"
        "1: tst.l   d0\n"
        "   bpl.s   2f\n"
        "   neg.l   d0\n"
        "   not.b   d2\n"
        "2: cmp.l   #0xffff,d1\n"
        "   bhi.s   .Llloop\n"
        /* d0 = d0 / d1.w with two DIVU (no overflow: the high word is
           below $10000, the remainder below d1.w); d3 = remainder:low
           word of the quotient */
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
        ".Lsign:\n"
        "   tst.b   d2\n"
        "   beq.s   4f\n"
        "   neg.l   d0\n"
        "4: movem.l (sp)+,d2-d3\n"
        "   rts\n"
        /* the loop for a divisor from $10000 on: normalised first (d1
           shifted left until bit 31 is set, d4 counts the shifts: k, from
           0 to 15), then the compare steps of 'loop, 32 - (31 - k) for
           Long and 16 more for Quick (dbra count d4 = k, 16 + k) */
        ".Llloop:\n"
        "   move.l  d4,-(sp)\n"
        "   moveq   #0,d4\n"
        ".Lnorm:\n"
        "   cmp.l   #0x00ffffff,d1\n"
        "   bhi.s   1f\n"
        "   lsl.l   #8,d1\n"
        "   addq.w  #8,d4\n"
        "1: cmp.l   #0x0fffffff,d1\n"
        "   bhi.s   2f\n"
        "   lsl.l   #4,d1\n"
        "   addq.w  #4,d4\n"
        "2: tst.l   d1\n"
        "   bmi.s   4f\n"
        "3: addq.w  #1,d4\n"
        "   add.l   d1,d1\n"
        "   bpl.s   3b\n"
        "4: moveq   #0,d3\n"
        ".Lstep:\n"                         /* 'skip6 without the tests */
        "   add.l   d3,d3\n"
        "   cmp.l   d1,d0\n"
        "   bcs.s   5f\n"
        "   sub.l   d1,d0\n"
        "   addq.w  #1,d3\n"                /* bit 0 is free */
        "5: lsr.l   #1,d1\n"
        "   dbra    d4,.Lstep\n"
        "   move.l  d3,d0\n"
        "   move.l  (sp)+,d4\n"
        "   bra.s   .Lsign");

/* ------------------------------------------- error checks (debugger only) */

/*
 * divchk - not in the original (a divisor 0 gave $7FFFFFFF without a
 * message, #200): the check of quickdiv and longdiv, d1 the divisor.  A
 * divisor 0 executes DIVU.W with it, the CPU's own division trap: the
 * debugger reports it as for a Word division, "GURU! - Division by Zero".
 * In assembler for that trap (a C division by 0 is undefined); d0/d1 stay
 * as they are (the trap leaves them unchanged).
 */
BLITZ_ERRCHK_ASM(divchk,
        "   .globl  _divchk\n"
        "_divchk:\n"
        "   tst.l   d1\n"
        "   bne.s   1f\n"
        "   divu.w  d1,d0\n"
        "1: rts");
