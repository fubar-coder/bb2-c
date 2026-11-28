/*
 * modlib - system library 64070 ($D400-$D404): the MOD operator for every
 * numeric type (tokens modbyte..modfloat, blitz2.s equates 140-144;
 * called from the expression evaluator, blitz2.s domodb..domodf).
 *
 * C translation of bblibsrc/modlib.  In: d0 = left, d1 = right; out: d0.
 * The result is the floor Mod a - b*floor(a/b): the remainder of the
 * division towards zero gets the divisor added (once) when it is not 0 and
 * its sign is not the divisor's.  Fixed (#1197, #1198; type-system-rework
 * Q2 "Floor"): the original added the divisor to every negative remainder
 * (-7 MOD -3 was -4, 7 MOD -3 was 1); with a positive divisor both rules
 * give 0..b-1.  A divisor 0: the 68000 division trap for Byte, Word and
 * Quick (DIVS), the dividend for Long and Float (ldivide's 'infinity
 * times 0), as in the original; the compiler tests the divisor of a Long
 * and a Float Mod before the call (type-system-rework D1/D7).
 *
 *   byte/word: EXT.l d0:DIVS d1,d0:SWAP d0, the remainder fixed up in the
 *              low word.  The high word of d0 is the quotient.  d1 is kept.
 *   quick:     SWAP d0:SWAP d1 first, then the word code on the integer
 *              parts.  The result is quotient<<16 | remainder, but the
 *              compiler types it as a Word (blitz2.s:9459 domodq "now a
 *              word"), so only the remainder is used - not a visible bug (#221)
 *              (the fractions of the inputs are ignored).  Kept bit for
 *              bit.
 *   long:      a - (a/b)*b with ldivlib ldivide ($CB01) and lmullib
 *              lmultiply ($CA01), the floor fix-up; for a divisor
 *              below $10000 in magnitude the same remainder from DIVU.
 *   float:     SPFix both (mathffp, a6 = ffplib base), then the long code;
 *              The result is a Long, not an FFP number - the compiler
 *              types it as a Long (blitz2.s:9463 domodf "now a long"), so
 *              this is correct (only the fractions are lost, #222).
 *
 * Registers: the contract of an operator (perf-analysis thunks.md 1.4:
 * the compiler saves only d0/d1 around it and keeps intermediates in
 * d2-d7) is d2-d7 and a2-a6; no third-party library calls these tokens.
 * The entries are the C functions (no thunk) and domodlong's assembler;
 * d1, a0 and a1 are not kept (the original keeps d1/a0/a1 for byte and
 * word, a0/a1 for long and float, and leaves d1 swapped for Quick).
 */
#include "blitzlib.h"

/* entry points (the C functions, domodlong and domodfloat assembler;
   defined below) */
void domodbyte(void), domodword(void), domodlong(void);
void domodquick(void), domodfloat(void);

/* !syslibheader{#modlib,$d400,$d405,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_modlib, 0xd400, 0xd405, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(domodbyte, 0, 0);                         /* $D400 modbyte */
BLITZ_NULLSUB(domodword, 0, 0);                         /* $D401 modword */
BLITZ_NULLSUB(domodlong, 0, 0);                         /* $D402 modlong */
BLITZ_NULLSUB(domodquick, 0, 0);                        /* $D403 modquick */
BLITZ_NULLSUB(domodfloat, 0, 0, LIB_ffplib, BL_LA6);    /* $D404 modfloat */
BLITZ_LIBFIN();

/* domodword - EXT.l d0:DIVS d1,d0:SWAP d0, the floor fix-up (Fixed
   (#1197): the original's TST d0:BPL 'skip:ADD d1,d0 added the divisor to
   every negative remainder).  DIVS as an instruction (bl_divs_w): C has
   nothing with the same behaviour - on overflow (quotient outside
   -32768..32767) d0 stays UNCHANGED, and a divisor of 0 raises the 68000
   divide-by-zero exception (vector 5) like the original. */
static inline __attribute__((always_inline)) ULONG mod_w(ULONG acc, ULONG divisor)
{
    acc = BL_EXTL(acc);                             /* EXT.l d0 */
    acc = bl_divs_w(acc, divisor);
    acc = BL_SWAP(acc);                             /* SWAP d0 */
    if ((WORD)acc && ((WORD)acc ^ (WORD)divisor) < 0)  /* not 0, not the divisor's sign */
        BL_SETW(acc, (UWORD)acc + (UWORD)divisor);  /* ADD d1,d0 */
    return acc;
}

/**
 * @brief a MOD b for Word and Byte (tokens modword $D401, modbyte $D400).
 *
 * EXT.l d0:DIVS d1,d0:SWAP d0; a remainder that is not 0 and whose sign
 * is not the divisor's gets the divisor added (once), the floor Mod
 * (#1197).  domodbyte is the same code (the byte is sign extended by
 * the compiler; only EXT.l on the word).  Keeps d2-d7/a2-a6 (the
 * contract of an operator); d1, a0 and a1 are free.
 *
 * Errors: b = 0 raises the 68000 divide-by-zero exception (vector 5) like
 * the original; on a DIVS overflow d0 stays unchanged.
 *
 * BUG(orig) (issue to be filed): the one DIVS overflow of two Words,
 * -32768 MOD -1 (quotient 32768), leaves d0 as the dividend extended
 * ($FFFF8000), so the low word after the SWAP is -1 instead of the
 * remainder 0: the original gives -2 (the divisor added), the floor rule
 * -1 (test `modlib-word-min-mod`).  domodquick has it for the whole parts
 * -32768 and -1.
 *
 * @param a d0: the left operand.
 * @param b d1: the right operand.
 * @return d0: the remainder in the low word, the quotient in the high
 *         word.
 */
static ULONG BLITZ_ROUTINE domodword_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1))
{
    return mod_w(a, b);
}
BLITZ_ALIAS(domodword, domodword_c);

/* domodbyte - same code as domodword (the byte is sign extended by the
   compiler; only EXT.l on the word) */
BLITZ_ALIAS(domodbyte, domodword_c);

/**
 * @brief a MOD b for Quick (token modquick $D403).
 *
 * SWAP d0:SWAP d1, then the word code on the integer parts (the fractions
 * of the inputs are ignored).  The result is quotient<<16 | remainder,
 * but the compiler types it as a Word (blitz2.s:9459 domodq "now a
 * word"), so only the remainder is used - not a visible bug (#221).
 * Keeps d2-d7/a2-a6 like the word code (the original leaves d1 swapped).
 *
 * Errors as for Word.
 *
 * @param a d0: the left operand (Quick).
 * @param b d1: the right operand (Quick).
 * @return d0: the remainder in the low word, the quotient in the high
 *         word.
 */
static ULONG BLITZ_ROUTINE domodquick_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1))
{
    return mod_w(BL_SWAP(a), BL_SWAP(b));
}
BLITZ_ALIAS(domodquick, domodquick_c);

/**
 * @brief a MOD b for Long (token modlong $D402).
 *
 * a - (a/b)*b with ldivlib ldivide ($CB01) and lmullib lmultiply ($CA01),
 * the remainder towards zero, then the floor fix-up: plus b when it is not
 * 0 and its sign is not b's (Fixed (#1197): the original added b to every
 * negative remainder).  Keeps d2-d7 and a0-a6; d1 is changed (the
 * original leaves (a/b)*b in d1).
 *
 * In assembler for speed (a hot path, every Mod on Long).  For a divisor
 * of 1 to $FFFF in magnitude the remainder comes from two DIVU instead
 * of the two calls, with the same result: ldivide's quotient is the
 * exact one of the magnitudes with the sign applied, lmultiply's product
 * is the one modulo 2^32, so a - (a/b)*b is the remainder of the
 * magnitudes with the dividend's sign.  A divisor from $10000 on takes
 * the calls, also a divisor 0 (ldivide's 'infinity; the result is a).
 *
 * @arg d0 a, the left operand.
 * @arg d1 b, the right operand.
 * @par Result
 * d0: the remainder.
 */
BLITZ_ASM_ROUTINE(domodlong,
        "   move.l  d1,-(sp)\n"             /* b */
        "   beq.s   .Lcalls\n"
        "   bpl.s   1f\n"
        "   neg.l   d1\n"
        "1: cmp.l   #0xffff,d1\n"
        "   bhi.s   .Lcalls\n"
        "   move.l  d0,-(sp)\n"             /* a, for its sign */
        "   bpl.s   2f\n"
        "   neg.l   d0\n"
        "2: move.l  d2,-(sp)\n"
        "   moveq   #0,d2\n"                /* |a| MOD |b| with two DIVU */
        "   swap    d0\n"
        "   move.w  d0,d2\n"
        "   divu    d1,d2\n"
        "   swap    d0\n"
        "   move.w  d0,d2\n"
        "   divu    d1,d2\n"
        "   clr.w   d2\n"
        "   swap    d2\n"
        "   move.l  d2,d0\n"
        "   move.l  (sp)+,d2\n"
        "   tst.l   (sp)+\n"
        "   bpl.s   .Lfloor\n"
        "   neg.l   d0\n"                   /* the dividend's sign */
        /* the remainder towards zero in d0, b at (sp): plus b when it is
           not 0 and has the other sign */
        ".Lfloor:\n"
        "   tst.l   d0\n"
        "   beq.s   3f\n"
        "   move.l  (sp),d1\n"
        "   eor.l   d0,d1\n"
        "   bpl.s   3f\n"
        "   add.l   (sp),d0\n"
        "3: addq.l  #4,sp\n"
        "   rts\n"
        ".Lcalls:\n"
        "   move.l  (sp),d1\n"
        "   movem.l d0/a0-a1,-(sp)\n"       /* a; lmultiply may change a0/a1 */
        "   jsr     _blitz_tok+0x80000000+" BL_STR(TOK_ldivide) "\n"
        "   move.l  12(sp),d1\n"
        "   jsr     _blitz_tok+0x80000000+" BL_STR(TOK_lmultiply) "\n"
        "   move.l  d0,d1\n"
        "   move.l  (sp),d0\n"
        "   sub.l   d1,d0\n"                /* the remainder towards zero */
        "   movem.l 4(sp),a0-a1\n"
        "   lea     12(sp),sp\n"
        "   bra.s   .Lfloor");

/**
 * @brief a MOD b for Float (token modfloat $D404).
 *
 * SPFix(a), SPFix(b) (d0 in, d0 out; the original passes the other
 * operand in d1, mathffp ignores it), then the long code.  The result is
 * a Long, not an FFP number - the compiler types it as a Long
 * (blitz2.s:9463 domodf "now a long"), so this is correct (only the
 * fractions are lost, #222).  Keeps d2-d7/a2-a6 (mathffp keeps them);
 * d1, a0 and a1 are free.
 *
 * In assembler, the original's code: in C the two mathffp calls made GCC
 * save d2-d7/a2/a6 (BLITZ_BASEJSR assumes the call may change them), 150
 * cycles of a routine of about 1300.
 *
 * @arg d0 a, the left operand (FFP).
 * @arg d1 b, the right operand (FFP).
 * @arg a6 ffp, the mathffp base (!libs #ffplib,#la6).
 * @par Result
 * d0: the remainder (Long).
 */
BLITZ_ASM_ROUTINE(domodfloat,
        "   move.l  d1,-(sp)\n"
        "   jsr     " BL_STR(LVO_SPFix) "(a6)\n"
        "   move.l  (sp)+,d1\n"
        "   move.l  d0,-(sp)\n"
        "   move.l  d1,d0\n"
        "   jsr     " BL_STR(LVO_SPFix) "(a6)\n"
        "   move.l  d0,d1\n"
        "   move.l  (sp)+,d0\n"
        "   bra.w   _domodlong");
