/*
 * ldivlib - system library 64735 ($CB00-$CB01): 32 bit and Quick division
 * for 68000 generated code (the compiler calls it for `/` on Long and
 * Quick, tokens quickdiv/longdiv, blitz2.s equates 175-176).
 *
 * C translation of bblibsrc/ldivlib (Acid Software).  The algorithm is
 * kept bit for bit, including the results for division by 0 and for
 * overflow; it is a restoring shift/subtract division and NOT the same as
 * C's `/` in all cases.
 */
#include "blitzlib.h"

LONG BLITZ_ROUTINE qdivide(LONG a BL_REG(d0), LONG b BL_REG(d1));
LONG BLITZ_ROUTINE ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1));

/* !syslibheader{#ldivlib,$cb00,$cb02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_ldivlib, 0xcb00, 0xcb02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qdivide, 0, 0);           /* $CB00 quickdiv */
BLITZ_NULLSUB(ldivide, 0, 0);           /* $CB01 longdiv */
BLITZ_LIBFIN();

/* divide - common part.  `steps` is the dbf counter: 31 for Long (32
   quotient bits), 47 for Quick (16 more bits = 16.16 fixed point).
     d1 = 0           -> $7FFFFFFF ("LARGEST POSITIVE NUMBER", any sign;
                         BUG(orig) (#200): tested before the signs, no error)
     the signs are removed first and applied to the quotient at the end */
static LONG divide(ULONG d0, ULONG d1, UWORD steps)
{
    UWORD neg = 0;                      /* d2.w: odd number of negatives */
    ULONG d3 = 0, d4 = 0;               /* shifted divisor, quotient */
    ULONG x;                            /* X flag */

    if (d1 == 0)
        return 0x7fffffff;              /* 'infinity */
    if ((LONG)d1 < 0) {
        d1 = -d1;
        neg = 0xffff;                   /* MOVEQ #-1,d2 */
    }
    if ((LONG)d0 < 0) {
        d0 = -d0;
        neg = ~neg;                     /* NOT d2 */
    }
    do {                                /* 'loop */
        d4 <<= 1;
        x = d1 & 1;                     /* LSR.l #1,d1 */
        d1 >>= 1;
        d3 = (d3 >> 1) | (x << 31);     /* ROXR.l #1,d3 */
        /* while d1 still has bits, the divisor is only moved into d3
           (from the top); afterwards each step compares and subtracts */
        if (d1 == 0 && d3 != 0 && d0 >= d3) {      /* 'skip6 / BCS 'skip3 */
            d0 -= d3;
            d4 += 1;                    /* ADDQ #1,d4 (bit 0 is free) */
        }
    } while (steps-- != 0);             /* DBF d5,'loop */
    if (neg)                            /* 'skip7 */
        d4 = -d4;
    return (LONG)d4;
}

/* qdivide - d0.q = d0.q / d1.q */
LONG qdivide(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    return divide(a, b, 47);
}

/* ldivide - d0.l = d0.l / d1.l */
LONG ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    return divide(a, b, 31);
}
