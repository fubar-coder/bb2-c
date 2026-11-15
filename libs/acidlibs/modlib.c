/*
 * modlib - system library 64070 ($D400-$D404): the MOD operator for every
 * numeric type (tokens modbyte..modfloat, blitz2.s equates 140-144;
 * called from the expression evaluator, blitz2.s domodb..domodf).
 *
 * C translation of bblibsrc/modlib (binary checked: same as the source).
 * In: d0 = left, d1 = right; out: d0.  The result takes the sign of the
 * divisor's side the way the original does it: a negative remainder gets
 * the divisor added (once).
 *
 *   byte/word: EXT.l d0:DIVS d1,d0:SWAP d0, remainder fixed up in the low
 *              word.  The high word of d0 is the quotient.  d1 is kept.
 *   quick:     SWAP d0:SWAP d1 first, then the word code on the integer
 *              parts.  The result is quotient<<16 | remainder, but the
 *              compiler types it as a Word (blitz2.s:9459 domodq "now a
 *              word"), so only the remainder is used - not a visible bug (#221)
 *              (the fractions of the inputs are ignored).  Kept bit for
 *              bit.
 *   long:      a - (a/b)*b with ldivlib ldivide ($CB01) and lmullib
 *              lmultiply ($CA01), plus b if negative; for a divisor
 *              below $10000 in magnitude the same remainder from DIVU.
 *   float:     SPFix both (mathffp, a6 = ffplib base), then the long code;
 *              The result is a Long, not an FFP number - the compiler
 *              types it as a Long (blitz2.s:9463 domodf "now a long"), so
 *              this is correct (only the fractions are lost, #222).
 *
 * Registers: byte/word keep d1/a0/a1 (and everything else); long/float
 * keep a0/a1 (ldivlib/lmullib/mathffp do), d1 is changed (the original
 * leaves (a/b)*b in d1).
 * DIFF: domodquick keeps d1 (the original leaves it swapped).
 */
#include "blitzlib.h"

/* entry points (thunks, domodlong assembler; defined below) */
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

/* domodword - EXT.l d0:DIVS d1,d0:SWAP d0:TST d0:BPL 'skip:ADD d1,d0
   DIVS as an instruction (bl_divs_w): C has nothing with the same
   behaviour - on overflow (quotient outside -32768..32767) d0 stays
   UNCHANGED, and a divisor of 0 raises the 68000 divide-by-zero exception
   (vector 5) like the original. */
static ULONG mod_w(ULONG acc, ULONG divisor)
{
    acc = BL_EXTL(acc);                             /* EXT.l d0 */
    acc = bl_divs_w(acc, divisor);
    acc = BL_SWAP(acc);                             /* SWAP d0 */
    if ((WORD)acc < 0)                              /* TST d0:BPL 'skip */
        BL_SETW(acc, (UWORD)acc + (UWORD)divisor);  /* ADD d1,d0 */
    return acc;
}

/**
 * @brief a MOD b for Word and Byte (tokens modword $D401, modbyte $D400).
 *
 * EXT.l d0:DIVS d1,d0:SWAP d0, a negative remainder gets the divisor
 * added (once).  domodbyte is the same code (the byte is sign extended by
 * the compiler; only EXT.l on the word).  Keeps d1/a0/a1 (and everything
 * else).
 *
 * Errors: b = 0 raises the 68000 divide-by-zero exception (vector 5) like
 * the original; on a DIVS overflow d0 stays unchanged.
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
BLITZ_THUNK(domodword, domodword_c, "d1/a0-a1");

/* domodbyte - same code as domodword (the byte is sign extended by the
   compiler; only EXT.l on the word) */
BLITZ_THUNK(domodbyte, domodword_c, "d1/a0-a1");

/**
 * @brief a MOD b for Quick (token modquick $D403).
 *
 * SWAP d0:SWAP d1, then the word code on the integer parts (the fractions
 * of the inputs are ignored).  The result is quotient<<16 | remainder,
 * but the compiler types it as a Word (blitz2.s:9459 domodq "now a
 * word"), so only the remainder is used - not a visible bug (#221).
 * DIFF: keeps d1 (the original leaves it swapped).
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
BLITZ_THUNK(domodquick, domodquick_c, "d1/a0-a1");

/**
 * @brief a MOD b for Long (token modlong $D402).
 *
 * a - (a/b)*b with ldivlib ldivide ($CB01) and lmullib lmultiply ($CA01),
 * plus b if negative (the original's code).  Keeps d2-d7 and a0-a6; d1
 * is changed (the original leaves (a/b)*b in d1).
 *
 * In assembler for speed (a hot path, every Mod on Long).  For a divisor
 * of 1 to $FFFF in magnitude the remainder comes from two DIVU instead
 * of the two calls, with the same result: ldivide's quotient is the
 * exact one of the magnitudes with the sign applied, lmultiply's product
 * is the one modulo 2^32, so a - (a/b)*b is the remainder of the
 * magnitudes with the dividend's sign.  A divisor 0 (ldivide's
 * 'infinity; the result is a) or one from $10000 on takes the calls.
 *
 * @param a d0: the left operand.
 * @param b d1: the right operand.
 * @return d0: the remainder.
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
        "   bpl.s   3f\n"
        "   neg.l   d0\n"                   /* the dividend's sign */
        "   bpl.s   3f\n"
        "   add.l   (sp),d0\n"              /* negative: + b */
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
        "   sub.l   d1,d0\n"
        "   bpl.s   4f\n"
        "   add.l   12(sp),d0\n"
        "4: movem.l 4(sp),a0-a1\n"          /* the CCR stays */
        "   lea     16(sp),sp\n"
        "   rts");

/**
 * @brief a MOD b for Float (token modfloat $D404).
 *
 * SPFix(a), SPFix(b) (d0 in, d0 out; the original passes the other
 * operand in d1, mathffp ignores it), then the long code.  The result is
 * a Long, not an FFP number - the compiler types it as a Long
 * (blitz2.s:9463 domodf "now a long"), so this is correct (only the
 * fractions are lost, #222).  Keeps a0/a1; d1 is changed.
 *
 * @param a d0: the left operand (FFP).
 * @param b d1: the right operand (FFP).
 * @param ffp a6: the mathffp base (!libs #ffplib,#la6).
 * @return d0: the remainder (Long).
 */
static ULONG BLITZ_ROUTINE domodfloat_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1),
                                        const void *ffp BL_REG(a6))
{
    struct bl_regs r = { .d0 = a, .d1 = b };
    ULONG ia;

    ia = BLITZ_BASEJSR(ffp, LVO_SPFix, r);
    r.d0 = b;
    r.d1 = b;                                       /* MOVE.l (a7)+,d1 */
    {
        register ULONG d1 __asm("d1") = BLITZ_BASEJSR(ffp, LVO_SPFix, r);
        register ULONG d0 __asm("d0") = ia;

        /* the Long code: domodlong, which keeps all but d0/d1 */
        __asm volatile("jsr     _domodlong(pc)"
                       : "+d"(d0), "+d"(d1) : : "cc", "memory");
        return d0;
    }
}
BLITZ_THUNK(domodfloat, domodfloat_c, "a0-a1");
