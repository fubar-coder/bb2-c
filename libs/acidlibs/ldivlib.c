/*
 * ldivlib - system library 64735 ($CB00-$CB01): 32 bit and Quick division
 * for 68000 generated code (the compiler calls it for `/` on Long and
 * Quick, tokens quickdiv/longdiv, blitz2.s equates 175-176).
 *
 * C translation of bblibsrc/ldivlib (Acid Software).  The algorithm is
 * kept bit for bit, including the results for overflow; it is a restoring
 * shift/subtract division and NOT the same as C's `/` in all cases.  The
 * result of a division by 0 has the dividend's sign (fixed, #200).
 */
#include "blitzlib.h"

LONG BLITZ_ROUTINE qdivide(LONG a BL_REG(d0), LONG b BL_REG(d1));
LONG BLITZ_ROUTINE ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1));

/* !syslibheader{#ldivlib,$cb00,$cb02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_ldivlib, 0xcb00, 0xcb02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qdivide, 0, 0);           /* $CB00 quickdiv */
BLITZ_NULLSUB(ldivide, 0, 0);           /* $CB01 longdiv */
BLITZ_LIBFIN();

/* divide - common part, called by quickdiv and longdiv with a divisor
   that is not 0 (their zero test: below).  `steps` is the dbf counter: 31
   for Long (32 quotient bits), 47 for Quick (16 more bits = 16.16 fixed
   point); dividend = d0 (the remainder in the end), divisor = d1.  The
   signs are removed first and applied to the quotient at the end. */
static LONG __attribute__((used)) divide(ULONG dividend, ULONG divisor,
                                         UWORD steps)
{
    UWORD neg = 0;                      /* d2.w: odd number of negatives */
    ULONG shifted = 0, quotient = 0;    /* d3 shifted divisor, d4 quotient */
    ULONG xbit;                         /* X flag */

    if ((LONG)divisor < 0) {
        divisor = -divisor;
        neg = 0xffff;                   /* MOVEQ #-1,d2 */
    }
    if ((LONG)dividend < 0) {
        dividend = -dividend;
        neg = ~neg;                     /* NOT d2 */
    }
    do {                                /* 'loop */
        quotient <<= 1;
        xbit = divisor & 1;             /* LSR.l #1,d1 */
        divisor >>= 1;
        shifted = (shifted >> 1) | (xbit << 31); /* ROXR.l #1,d3 */
        /* while d1 still has bits, the divisor is only moved into d3
           (from the top); afterwards each step compares and subtracts */
        if (divisor == 0 && shifted != 0 && dividend >= shifted) { /* 'skip6 / BCS 'skip3 */
            dividend -= shifted;
            quotient += 1;              /* ADDQ #1,d4 (bit 0 is free) */
        }
    } while (steps-- != 0);             /* DBF d5,'loop */
    if (neg)                            /* 'skip7 */
        quotient = -quotient;
    return (LONG)quotient;
}

/*
 * quickdiv ($CB00, qdivide): Quick division (16.16 fixed point), d0.q =
 * d0.q / d1.q, with 48 steps of divide; longdiv ($CB01, ldivide): Long
 * division, d0.l = d0.l / d1.l, with 32 steps.  Parameters: d0 the
 * dividend, d1 the divisor; result: d0 the quotient.
 *
 * A divisor 0 gives the largest number with the dividend's sign, no
 * error: $7FFFFFFF for a dividend >= 0, -$7FFFFFFF ($80000001) for a
 * negative one ('infinity).  Fixed (#200): the original returned
 * $7FFFFFFF ("LARGEST POSITIVE NUMBER") for any sign, it tested the
 * divisor before the signs.
 *
 * In assembler because of the fix: the instructions gcc generated for the
 * C entries (push the arguments, call divide) with the zero test on the
 * flags of the divisor's push, so a correct division costs the cycles it
 * cost before (the not taken BEQ.s moved from divide to here).  Every C
 * form of the fix made gcc lay out divide differently and cost the
 * correct divisions 4-14 cycles more.
 */
__asm__("   .text\n"
        "   .even\n"
        "   .globl  _qdivide\n"
        "_qdivide:\n"
        "   pea     47.w\n"
        "   move.l  d1,-(sp)\n"
        "   beq.s   .Linfinity\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     _divide(pc)\n"
        "   lea     12(sp),sp\n"
        "   rts\n"
        "   .globl  _ldivide\n"
        "_ldivide:\n"
        "   pea     31.w\n"
        "   move.l  d1,-(sp)\n"
        "   beq.s   .Linfinity\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     _divide(pc)\n"
        "   lea     12(sp),sp\n"
        "   rts\n"
        ".Linfinity:\n"
        "   addq.l  #8,sp\n"
        "   tst.l   d0\n"
        "   bmi.s   1f\n"
        "   move.l  #0x7fffffff,d0\n"
        "   rts\n"
        "1: move.l  #0x80000001,d0\n"
        "   rts\n");
