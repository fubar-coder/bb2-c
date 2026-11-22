/*
 * compilerlib - system library 64030 ($D600-$D602), the port's, not in
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
 */
#include "blitzlib.h"

void mixmult(void), qqmult(void), lqdivide(void);
void lqdivchk(void);

/* !syslibheader{64030,$d600,$d603,0,0,0,errors,$4000} */
BLITZ_SYSLIBHEADER(64030, 0xd600, 0xd603, 0, 0, 0, BLITZ_ERROR_AREA, 0x4000);
BLITZ_NULLSUB(mixmult, 0, 0);           /* $D600 mixmult */
BLITZ_NULLSUB(qqmult, 0, 0);            /* $D601 qqmult */
BLITZ_NULLSUB(lqdivide, lqdivchk, 0);   /* $D602 lqdiv */
BLITZ_LIBFIN();

/**
 * @brief mixmult ($D600) and qqmult ($D601): the whole-number products of
 * a Quick under a Long type so far.
 *
 * mixmult: d0.l = floor(d0 * d1 / 65536), one factor a Long and the other
 * a Quick (16.16, either order): bits 16..47 of the signed 64 bit product;
 * qqmult: d0.l = floor(d0 * d1 / 2^32), both Quicks: bits 32..63.
 *
 * The 64 bit product of the magnitudes from four MULU (lo*lo, the two
 * cross products added as 33 bits, hi*hi), negated in 64 bits when the
 * signs differ, so that the bits taken are those of the floor.
 * $80000000 counts as the magnitude 2^31.
 *
 * @arg d0 a, the first factor
 * @arg d1 b, the second factor
 * @par Result
 * d0: the product rounded down
 */
BLITZ_ASM_BLOCK(mixmult,
        /* d3:d2 = a * b (signed 64 bits), d4/d5 used */
        ".Lmul64:\n"
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
        "4: rts\n"
        "   .globl  _mixmult\n"
        "_mixmult:\n"
        "   movem.l d2-d5,-(sp)\n"
        "   bsr.s   .Lmul64\n"
        "   swap    d2\n"
        "   swap    d3\n"
        "   move.w  d2,d3\n"                /* bits 16..47 */
        "   move.l  d3,d0\n"
        "   movem.l (sp)+,d2-d5\n"
        "   rts\n"
        "   .globl  _qqmult\n"
        "_qqmult:\n"
        "   movem.l d2-d5,-(sp)\n"
        "   bsr.s   .Lmul64\n"
        "   move.l  d3,d0\n"                /* bits 32..63 */
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
