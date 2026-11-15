/*
 * floatquicklib - system library 64080 ($D300-$D301): conversion between
 * Quick (16.16 fixed point, signed) and Float (Motorola FFP) (tokens
 * qutofl/fltoqu, blitz2.s equates 135-136, used by the type conversion
 * code, blitz2.s 8638/8679).
 *
 * The original's code of bblibsrc/floatquicklib (binary checked: same as
 * the source), in assembler.  All arithmetic is done by mathffp (a6 =
 * ffplib base); the sign is handled outside, on the magnitude:
 *   quicktofloat: d0.q -> d0.f = float(int part) + float(frac)/65536.0
 *   floattoquick: d0.f -> d0.q = SPFix(x)<<16 | SPFix((x-int)*65536.0)
 *
 * Registers: the contract of a conversion the compiler calls (qtof2,
 * perf-analysis thunks.md 1.4) is d2-d7/a2-a6; floattoquick also keeps
 * a0-a1 for RIMiscLib, which calls it with both live
 * (analyse-internal-token-callers.md).  d1 is free.  The CCR is not a
 * result (the original leaves the flags of the last mathffp call / BSET).
 *
 * Both are assembler entries in the original's form (user decision
 * 2026-11-15): only d0, d1 and the stack are written, d2 is saved, and
 * mathffp is called through jsr -xx(a6) with the original's operands, so
 * a0 and a1 stay what the caller had (the ROM's mathffp does not touch
 * them, which the original relies on as well) and no thunk is needed.
 * In C, BLITZ_BASEJSR loaded a0/a1 with 0 for each call and made GCC
 * save d2-d7/a2/a6, a thunk kept a0/a1.
 */
#include "blitzlib.h"

/* entry points (assembler, defined below) */
void quicktofloat(void), floattoquick(void);

/* !syslibheader{#floatquicklib,$d300,$d302,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_floatquicklib, 0xd300, 0xd302, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(quicktofloat, 0, 0, LIB_ffplib, BL_LA6);    /* $D300 qutofl */
BLITZ_NULLSUB(floattoquick, 0, 0, LIB_ffplib, BL_LA6);    /* $D301 fltoqu */
BLITZ_LIBFIN();

#define f65536 0x80000051                       /* 65536.0 in FFP */

/**
 * @brief $D300 qutofl: convert a Quick (16.16 fixed point) to a Float (FFP).
 *
 * The sign is tested on d0 (TST.l d0:BPL 'doit2); negative inputs are
 * converted as their magnitude (float(int part) + float(frac) / 65536.0,
 * by mathffp; the magnitude of $80000000 is 32768.0, read as unsigned)
 * and get the FFP sign bit (bit 7) set.
 * Fixed (#146, #147): the original tests the sign on -d0 (NEG.l d0:BMI
 * 'doit:BSR 'doit2:BSET #7,d0:RTS / 'doit:NEG.l d0): 0 counted as
 * negative (NEG gives 0, not minus), so Quick 0 became $00000080, an FFP
 * "-0" (#146); NEG of $80000000 overflows and stays negative, so -32768.0
 * became +32768.0 (#147).  The test on d0 costs no more than the
 * original's (one TST.l instead of the NEG.l of a copy).
 *
 * @param q  d0: the Quick
 * @param ffpbase  a6: mathffp.library (ffplib)
 * @return d0: the Float; d1 changed, a0/a1 and d2-d7/a2-a6 kept
 */
BLITZ_ASM_ROUTINE(quicktofloat,
        "   tst.l   d0\n"
        "   bpl.s   .Lq2f\n"
        "   neg.l   d0\n"                   /* the magnitude */
        "   bsr.s   .Lq2f\n"
        "   bset    #7,d0\n"                /* the FFP sign */
        "   rts\n"
        ".Lq2f:\n"                          /* 'doit2: d0 = the magnitude */
        "   move.l  d0,-(sp)\n"
        "   swap    d0\n"
        "   clr.w   d0\n"
        "   swap    d0\n"
        "   jsr     " BL_STR(LVO_SPFlt) "(a6)\n" /* the float of the fraction */
        "   move.l  #" BL_STR(f65536) ",d1\n"
        "   jsr     " BL_STR(LVO_SPDiv) "(a6)\n"
        "   move.l  d0,-(sp)\n"
        "   move.l  4(sp),d0\n"
        "   clr.w   d0\n"
        "   swap    d0\n"
        "   jsr     " BL_STR(LVO_SPFlt) "(a6)\n" /* the float of the integer */
        "   move.l  (sp)+,d1\n"
        "   addq.l  #4,sp\n"
        "   jmp     " BL_STR(LVO_SPAdd) "(a6)");

/**
 * @brief $D301 fltoqu: convert a Float (FFP) to a Quick (16.16 fixed point).
 *
 * BCHG #7,d0:BEQ 'doit:BSR 'doit2:NEG.l d0:RTS / 'doit:BCHG #7,d0
 * ('doit2): converts the magnitude (SPFix(x) << 16 | SPFix((x - int) *
 * 65536.0), by mathffp) and negates it if the sign bit was set.
 *
 * @param f  d0: the Float
 * @param ffpbase  a6: mathffp.library (ffplib)
 * @return d0: the Quick; d1 changed, a0/a1 (third party: RIMiscLib) and
 *         d2-d7/a2-a6 kept
 */
BLITZ_ASM_ROUTINE(floattoquick,
        "   bchg    #7,d0\n"
        "   beq.s   1f\n"                   /* the sign bit was clear */
        "   bsr.s   .Lf2q\n"
        "   neg.l   d0\n"
        "   rts\n"
        "1: bchg    #7,d0\n"
        ".Lf2q:\n"                          /* 'doit2: d0 = the magnitude */
        "   move.l  d2,-(sp)\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     " BL_STR(LVO_SPFix) "(a6)\n"
        "   move.w  d0,d2\n"                /* the integer part */
        "   swap    d2\n"
        "   clr.w   d2\n"
        "   jsr     " BL_STR(LVO_SPFlt) "(a6)\n" /* the float of the integer */
        "   move.l  d0,d1\n"
        "   move.l  (sp)+,d0\n"
        "   jsr     " BL_STR(LVO_SPSub) "(a6)\n" /* the fraction */
        "   move.l  #" BL_STR(f65536) ",d1\n"
        "   jsr     " BL_STR(LVO_SPMul) "(a6)\n" /* times 65536 */
        "   jsr     " BL_STR(LVO_SPFix) "(a6)\n"
        "   move.w  d0,d2\n"
        "   move.l  d2,d0\n"
        "   move.l  (sp)+,d2\n"
        "   rts");
