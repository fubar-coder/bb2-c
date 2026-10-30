/*
 * floatquicklib - system library 64080 ($D300-$D301): conversion between
 * Quick (16.16 fixed point, signed) and Float (Motorola FFP) (tokens
 * qutofl/fltoqu, blitz2.s equates 135-136, used by the type conversion
 * code, blitz2.s 8638/8679).
 *
 * C translation of bblibsrc/floatquicklib (binary checked: same as the
 * source).  All arithmetic is done by mathffp (a6 = ffplib base, same
 * calls with the same operands as the original); the sign is handled
 * outside, on the magnitude:
 *   quicktofloat: d0.q -> d0.f = float(int part) + float(frac)/65536.0
 *   floattoquick: d0.f -> d0.q = SPFix(x)<<16 | SPFix((x-int)*65536.0)
 *
 * Registers: the original keeps d2-d7/a2-a6 itself and whatever mathffp
 * keeps; the thunks keep d1/a0/a1 in addition.  The CCR is not a result
 * (the original leaves the flags of the last mathffp call / BSET).
 */
#include "blitzlib.h"

/* entry points (thunks, defined below) */
void quicktofloat(void), floattoquick(void);

/* !syslibheader{#floatquicklib,$d300,$d302,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_floatquicklib, 0xd300, 0xd302, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(quicktofloat, 0, 0, LIB_ffplib, BL_LA(6));  /* $D300 qutofl */
BLITZ_NULLSUB(floattoquick, 0, 0, LIB_ffplib, BL_LA(6));  /* $D301 fltoqu */
BLITZ_LIBFIN();

#define f65536 0x80000051UL                     /* 65536.0 in FFP */

/* 'doit2 of quicktofloat: d0 = magnitude (as unsigned) */
static ULONG q2f(const void *ffp, ULONG v)
{
    ULONG frac, i;

    /* SWAP d0:CLR d0:SWAP d0  d0=float of frac (eg 32768 for .5) */
    frac = BLITZ_BASEJSR1(ffp, LVO_SPFlt, v & 0xffff);
    /* MOVE.l #f65536,d1:JSR -84(a6) */
    frac = BLITZ_BASEJSR2(ffp, LVO_SPDiv, frac, f65536);
    /* MOVE.l 4(a7),d0:CLR d0:SWAP d0 */
    i = BLITZ_BASEJSR1(ffp, LVO_SPFlt, v >> 16);
    /* MOVE.l (a7)+,d1:JMP -66(a6) */
    return BLITZ_BASEJSR2(ffp, LVO_SPAdd, i, frac);
}

/* quicktofloat - NEG.l d0:BMI 'doit:BSR 'doit2:BSET #7,d0:RTS
   'doit:NEG.l d0 ('doit2)
   The sign test is on -d0: negative inputs are converted as their
   magnitude and get the FFP sign bit (bit 7) set.
   BUG(orig) (#146): 0 counts as negative (NEG gives 0, not minus): Quick 0
   becomes $00000080, an FFP "-0".  BUG(orig) (#147): $80000000 (-32768.0)
   gives +32768.0 (NEG overflows, the magnitude is $80000000 read as
   unsigned). */
static ULONG BLITZ_ROUTINE quicktofloat_c(ULONG q BL_REG(d0), const void *ffpbase BL_REG(a6))
{
    ULONG n = -q;

    if ((LONG)n < 0)                            /* BMI 'doit */
        return q2f(ffpbase, q);
    return q2f(ffpbase, n) | 0x80;              /* BSET #7,d0 */
}
BLITZ_THUNK(quicktofloat, quicktofloat_c, "d1/a0-a1");

/* 'doit2 of floattoquick: d0 = FFP number with the sign bit clear */
static ULONG f2q(const void *ffp, ULONG x)
{
    ULONG i, d2;

    i = BLITZ_BASEJSR1(ffp, LVO_SPFix, x);          /* JSR -30(a6) */
    d2 = (ULONG)(UWORD)i << 16;                     /* MOVE d0,d2:SWAP d2:CLR d2 ;integer part */
    i = BLITZ_BASEJSR1(ffp, LVO_SPFlt, i);          /* d0=float of INT PART */
    x = BLITZ_BASEJSR2(ffp, LVO_SPSub, x, i);       /* num-int -> d0=float of fraction */
    x = BLITZ_BASEJSR2(ffp, LVO_SPMul, x, f65536);  /* frac * 65536 */
    x = BLITZ_BASEJSR1(ffp, LVO_SPFix, x);          /* back to int. */
    return d2 | (UWORD)x;                           /* MOVE d0,d2:MOVE.l d2,d0 */
}

/* floattoquick - BCHG #7,d0:BEQ 'doit:BSR 'doit2:NEG.l d0:RTS
   'doit:BCHG #7,d0 ('doit2) - convert the magnitude, negate if the sign
   bit was set */
static ULONG BLITZ_ROUTINE floattoquick_c(ULONG f BL_REG(d0), const void *ffpbase BL_REG(a6))
{
    if (!(f & 0x80))                            /* BEQ 'doit */
        return f2q(ffpbase, f);
    return -f2q(ffpbase, f & ~0x80UL);          /* NEG.l d0 */
}
BLITZ_THUNK(floattoquick, floattoquick_c, "d1/a0-a1");
