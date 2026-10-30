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
 *              lmultiply ($CA01), plus b if negative.
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

/* entry points (thunks, defined below) */
void domodbyte(void), domodword(void), domodlong(void);
void domodquick(void), domodfloat(void);

/* !syslibheader{#modlib,$d400,$d405,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_modlib, 0xd400, 0xd405, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(domodbyte, 0, 0);                         /* $D400 modbyte */
BLITZ_NULLSUB(domodword, 0, 0);                         /* $D401 modword */
BLITZ_NULLSUB(domodlong, 0, 0);                         /* $D402 modlong */
BLITZ_NULLSUB(domodquick, 0, 0);                        /* $D403 modquick */
BLITZ_NULLSUB(domodfloat, 0, 0, LIB_ffplib, BL_LA(6));  /* $D404 modfloat */
BLITZ_LIBFIN();

/* domodword - EXT.l d0:DIVS d1,d0:SWAP d0:TST d0:BPL 'skip:ADD d1,d0
   DIVS as an instruction (bl_divs_w): C has nothing with the same
   behaviour - on overflow (quotient outside -32768..32767) d0 stays
   UNCHANGED, and a divisor of 0 raises the 68000 divide-by-zero exception
   (vector 5) like the original. */
static ULONG mod_w(ULONG d0, ULONG d1)
{
    d0 = BL_EXTL(d0);                               /* EXT.l d0 */
    d0 = bl_divs_w(d0, d1);
    d0 = BL_SWAP(d0);                               /* SWAP d0 */
    if ((WORD)d0 < 0)                               /* TST d0:BPL 'skip */
        BL_SETW(d0, (UWORD)d0 + (UWORD)d1);         /* ADD d1,d0 */
    return d0;
}

static ULONG BLITZ_ROUTINE domodword_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1))
{
    return mod_w(a, b);
}
BLITZ_THUNK(domodword, domodword_c, "d1/a0-a1");

/* domodbyte - same code as domodword (the byte is sign extended by the
   compiler; only EXT.l on the word) */
BLITZ_THUNK(domodbyte, domodword_c, "d1/a0-a1");

/* domodquick - SWAP d0:SWAP d1, then domodword */
static ULONG BLITZ_ROUTINE domodquick_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1))
{
    return mod_w(BL_SWAP(a), BL_SWAP(b));
}
BLITZ_THUNK(domodquick, domodquick_c, "d1/a0-a1");

/* domodlong - d0 = a - ldivide(a,b)*b; if negative, + b */
static ULONG mod_l(ULONG a, ULONG b)
{
    struct bl_regs r = { .d0 = a, .d1 = b };
    ULONG d0;

    BLITZ_ALIBJSR(TOK_ldivide, r);
    r.d1 = b;                                       /* MOVE.l (a7),d1 */
    BLITZ_ALIBJSR(TOK_lmultiply, r);
    d0 = a - r.d0;                                  /* SUB.l d1,d0 */
    if ((LONG)d0 < 0)                               /* BPL 'done */
        d0 += b;                                    /* ADD.l (a7),d0 */
    return d0;
}

static ULONG BLITZ_ROUTINE domodlong_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1))
{
    return mod_l(a, b);
}
BLITZ_THUNK(domodlong, domodlong_c, "a0-a1");

/* domodfloat - SPFix(a), SPFix(b) (d0 in, d0 out; the original passes the
   other operand in d1, mathffp ignores it), then domodlong */
static ULONG BLITZ_ROUTINE domodfloat_c(ULONG a BL_REG(d0), ULONG b BL_REG(d1),
                                        const void *ffp BL_REG(a6))
{
    struct bl_regs r = { .d0 = a, .d1 = b };
    ULONG ia, ib;

    ia = BLITZ_BASEJSR(ffp, LVO_SPFix, r);
    r.d0 = b;
    r.d1 = b;                                       /* MOVE.l (a7)+,d1 */
    ib = BLITZ_BASEJSR(ffp, LVO_SPFix, r);
    return mod_l(ia, ib);
}
BLITZ_THUNK(domodfloat, domodfloat_c, "a0-a1");
