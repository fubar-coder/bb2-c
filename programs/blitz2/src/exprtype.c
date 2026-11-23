/*
 * exprtype.c - the type decisions of an expression (exprtype.h; issue
 * #1180, step 1).  No asm counterpart: the rules the original took in
 * eval5 (blitz2.s:8983), the operator handlers (blitz2.s:9142-9606),
 * varcont (blitz2.s:10479) and consttype2 (blitz2.s:10913) at the moment
 * of emission, with the port's fixes #978, #1177, #1178, #1179, #1182 and
 * #1188, collected in one place; which routine each rule comes from:
 * docs/blitz2/expression-tree.md.  The decisions are those of the code
 * before (every result byte-identical), pinned by the host test's table
 * (programs/blitz2/tests/host/exprtype.tsv).
 */
#include <stddef.h>
#include "exprtype.h"

/* no asm counterpart; replaces the handlers' result types (operators[],
   blitz2.s:14704, and the handlers blitz2.s:9142-9606): the type each
   handler of operators[] returns for operands of type t (index t-1), 0
   where the entry has none.  + of two Bytes is a Word, of two Words a
   Long ("now a word", "now a long"), * the same; a shift gives a Long but
   of a Quick (shpoke "don't fuck with quix"); Mod of a Quick a Word, of a
   Float a Long; ^ a Float; a comparison and BitTst a Word (cpoke);
   BitSet, BitClr, BitChg a Long. */
#define B ET_BYTE
#define W ET_WORD
#define L ET_LONG
#define Q ET_QUICK
#define F ET_FLOAT
#define S ET_STRING
static const unsigned char optypes[ETO_COUNT][ET_NTYPES - 1] = {
    /*            B  W  L  Q  F  A  S */
    [ETO_ADD]  = { W, L, L, Q, F, 0, S },
    [ETO_SUB]  = { B, W, L, Q, F, 0, 0 },
    [ETO_MUL]  = { W, L, L, Q, F, 0, 0 },
    [ETO_DIV]  = { B, W, L, Q, F, 0, 0 },
    [ETO_AMP]  = { B, W, L, Q, 0, 0, 0 },
    [ETO_BAR]  = { B, W, L, Q, 0, 0, 0 },
    [ETO_AND]  = { B, W, L, Q, 0, 0, 0 },
    [ETO_OR]   = { B, W, L, Q, 0, 0, 0 },
    [ETO_EOR]  = { B, W, L, Q, 0, 0, 0 },
    [ETO_LSL]  = { L, L, L, Q, 0, 0, 0 },
    [ETO_ASL]  = { L, L, L, Q, 0, 0, 0 },
    [ETO_LSR]  = { L, L, L, Q, 0, 0, 0 },
    [ETO_ASR]  = { L, L, L, Q, 0, 0, 0 },
    [ETO_MOD]  = { B, W, L, W, L, 0, 0 },
    [ETO_POW]  = { F, F, F, F, F, 0, 0 },
    [ETO_EQ]   = { W, W, W, W, W, W, W },
    [ETO_NE]   = { W, W, W, W, W, W, W },
    [ETO_LT]   = { W, W, W, W, W, W, W },
    [ETO_LE]   = { W, W, W, W, W, W, W },
    [ETO_GT]   = { W, W, W, W, W, W, W },
    [ETO_GE]   = { W, W, W, W, W, W, W },
    [ETO_BITTST] = { W, W, W, W, 0, 0, 0 },
    [ETO_BITSET] = { L, L, L, L, 0, 0, 0 },
    [ETO_BITCLR] = { L, L, L, L, 0, 0, 0 },
    [ETO_BITCHG] = { L, L, L, L, 0, 0, 0 },
};
#undef B
#undef W
#undef L
#undef Q
#undef F
#undef S

/* no asm counterpart; replaces the type check of blitz2.s:9142 doop */
unsigned exprtype_optype(unsigned op, unsigned t)
{
    if (op >= ETO_COUNT || t < ET_BYTE || t >= ET_NTYPES)
        return 0;
    return optypes[op][t - 1];
}

int exprtype_iscompare(unsigned op)
{
    return op >= ETO_EQ && op <= ETO_GE;
}

/* Fixed (#1178), no asm counterpart: the consumer op takes a raw Quick
   rounded down, as the original's cut gave it, whatever its other
   operand: a shift, AND, OR, EOR, a bit operator (not + - * / ^ Mod, not
   a comparison, not the end; Mod takes the fraction when its right
   operand is a Float) */
int exprtype_cutsraw(unsigned op)
{
    if (op >= ETO_COUNT || exprtype_iscompare(op))
        return 0;
    return op != ETO_ADD && op != ETO_SUB && op != ETO_MUL && op != ETO_DIV && op != ETO_POW
           && op != ETO_MOD;
}

/* Fixed (#1177), no asm counterpart: the operands of a comparison are
   compared with their own values, not in the type the expression is
   wanted in.  A raw left operand is a Quick (#1178); a left operand of
   one element gets its own type back (cmpownleft); the right operand
   starts at type 0. */
struct exprtype_left exprtype_leftop(unsigned op, unsigned left, int lraw)
{
    struct exprtype_left r;

    r.left = (unsigned char)left;
    r.lraw = lraw != 0;
    r.ownleft = 0;
    r.compare = exprtype_iscompare(op) != 0;
    if (r.compare && lraw) {
        r.left = ET_QUICK;
        r.lraw = 0;
    } else if (r.compare)
        r.ownleft = 1;
    return r;
}

/* Fixed (#1177), no asm counterpart: the conversions cmpownleft drops.
   Byte/Word to Long is exact and stays; a Quick under a Long is not cut
   any more (#1178). */
int exprtype_ownleft(unsigned from, unsigned to)
{
    return (from >= ET_BYTE && from <= ET_LONG && to == ET_QUICK)
           || (from >= ET_BYTE && from <= ET_QUICK && to == ET_FLOAT);
}

/* no asm counterpart; replaces the type decisions of blitz2.s:8983 eval5
   (.higher: the late convtypef, blitz2.s:8998, doop) and of the port's
   compareop, cmplq (#1177) and mixedop (#1178) */
struct exprtype_bin exprtype_binop(unsigned op, unsigned left, unsigned right, const struct exprtype_binfacts *f)
{
    struct exprtype_bin d = { 0 };
    int cmpop = exprtype_iscompare(op);
    int eqne = op == ETO_EQ || op == ETO_NE;
    int rcmp = f->rcmp;

    /* Fixed (#978): an equality comparison of a byte value with a
       constant from 128 to 255 (each operand one element) takes the
       constant as the byte it stands for, sign-extended as varcont
       extends the byte */
    if (eqne && f->lbyte && f->rconst)
        d.byteext = ETB_RIGHT;
    else if (eqne && f->lconst && f->rbyte)
        d.byteext = ETB_LEFT;
    /* Fixed (#1182): a right operand that a comparison lowered below the
       left operand's type is converted up to the left one's type and the
       operation done in that type (the original converted the left
       operand down, blitz2.s:8998).  Not a Long under a Quick left (the
       late cut of #1178 stays). */
    if (!cmpop && rcmp && left >= ET_LONG && left <= ET_ADDR && right && right < left
        && !(left == ET_QUICK && right >= ET_LONG)) {
        d.rup_from = (unsigned char)right;
        d.rup_to = (unsigned char)left;
        right = left;
        rcmp = 0;
    }
    if (cmpop) {
        /* Fixed (#1177): compareop - a Long and a Quick exactly (cmplq),
           otherwise the lower of the two up; the boolean a Word (cpoke).
           A raw right operand is not reached (it starts at type 0). */
        d.rcut = f->rraw;
        if ((left == ET_LONG && right == ET_QUICK) || (left == ET_QUICK && right == ET_LONG)) {
            d.path = ETP_CMPLQ;
            d.result = ET_WORD;
        } else {
            d.path = ETP_COMPARE;
            if (left && right < left) {
                d.rup_from = (unsigned char)right;
                d.rup_to = (unsigned char)left;
                right = left;
            }
            if (left) {
                d.lconv_from = (unsigned char)left;
                d.lconv_to = (unsigned char)right;
            }
            d.handler = (unsigned char)right;
            d.result = (unsigned char)exprtype_optype(op, right);
        }
    } else if (f->lraw || f->rraw || (left == ET_QUICK && right == ET_LONG && (op == ETO_MUL || op == ETO_DIV))) {
        /* Fixed (#1178): mixedop - one operand a raw Quick under a Long
           type so far, or a Quick that a Long right operand made Long
           under * or / */
        int lq = f->lraw || left == ET_QUICK, rq = f->rraw;

        d.path = ETP_MIXED;
        d.lq = (unsigned char)lq;
        d.rq = (unsigned char)rq;
        if (right != ET_LONG) {
            /* a Float (or another) type so far set by the right side */
            d.rcut = (unsigned char)rq;
            if (lq || left) {
                d.lconv_from = lq ? ET_QUICK : (unsigned char)left;
                d.lconv_to = (unsigned char)right;
            }
            d.handler = (unsigned char)right;
            d.result = (unsigned char)exprtype_optype(op, right);
        } else if (op == ETO_POW) {
            /* both operands with their fractions to Float */
            d.lconv_from = lq ? ET_QUICK : ET_LONG;
            d.lconv_to = ET_FLOAT;
            d.rconv_from = rq ? ET_QUICK : ET_LONG;
            d.rconv_to = ET_FLOAT;
            d.handler = ET_FLOAT;
            d.result = ET_FLOAT;
        } else if (op == ETO_ADD || op == ETO_SUB || op == ETO_MUL || op == ETO_DIV) {
            d.result = ET_LONG;     /* inline, rounded at the operator */
        } else {
            /* Mod with a whole-number divisor, a shift or a logic
               operator: the Quick rounded down, as before */
            d.lcut = (unsigned char)lq;
            d.rcut = (unsigned char)rq;
            d.handler = ET_LONG;
            d.result = (unsigned char)exprtype_optype(op, ET_LONG);
        }
    } else {
        /* blitz2.s:8998: the left operand to the type so far after the
           right one ("could possibly fuck regat+1 !"), its handler */
        d.path = ETP_PLAIN;
        if (left) {
            d.lconv_from = (unsigned char)left;
            d.lconv_to = (unsigned char)right;
        }
        d.handler = (unsigned char)right;
        d.result = (unsigned char)exprtype_optype(op, right);
        d.wordop = f->words;        /* Fixed (#1179) */
    }
    d.cmpres = (unsigned char)(cmpop || rcmp);  /* Fixed (#1182) */
    return d;
}

/* no asm counterpart; replaces the type decisions of blitz2.s:10479
   varcont and blitz2.s:10913 consttype2 */
struct exprtype_elem exprtype_element(unsigned tsf, unsigned own, const struct exprtype_constfacts *c)
{
    struct exprtype_elem e = { 0 };

    if (c) {
        /* consttype2: the constant is made the type it is wanted in or
           the size it needs; no conversion code */
        switch (tsf) {
        case ET_NONE:
            /* Let's set type: the size (the .byte path).  #12: a float
               literal is a float constant.  The asm set type q and jumped
               to .quick2 without makeint (blitz2.s:10920); its range
               check then always failed (the upper word of d0 was that of
               the last FFP result, negative), so 1.95 always ended in
               numdofloat.  The port passed the character with an upper
               word of 0 and made some literals a Quick by chance (a port
               regression); it goes to numdofloat directly now. */
            e.result = c->point ? ET_FLOAT : c->size;
            break;
        case ET_BYTE:
            /* Fixed (#1188): a literal with a non-zero fraction is a Float
               constant, as under .word, so the operation is computed in
               Float and its result converted to the Byte at the end
               (towards zero): rb.b=(1.5+1.5) is 3 and rb.b=(0.5*4) 2, as
               under a Word.  The original cut it to a whole number here
               (makeint) before the operation (2 and 0).  A whole-valued
               literal with a point or an exponent (2.0, 1e2) stays the
               whole number, unlike .word: the value is the same without
               Float code (rb.b=vb AND 2.0 compiles; #1186: litcut).  A
               Data value keeps the cut (Data.b 1.5 is 1; under .word "Bad
               Data").  A whole part beyond a Long (makeint's overflow) is
               a Float. */
            if (c->point && (!c->fix || (!c->whole && !c->data)))
                e.result = ET_FLOAT;
            else {
                e.result = c->size;
                e.litcut = c->point;
            }
            break;
        case ET_WORD:           /* #12: a point literal is a Float */
            e.result = c->point ? ET_FLOAT : c->size < ET_WORD ? ET_WORD : c->size;
            break;
        case ET_LONG:
            e.result = c->point ? ET_FLOAT : ET_LONG;
            break;
        case ET_QUICK:
            /* a whole number beyond a Word is a Long ("quick int out of
               range -> long (sic)"), a point literal a Float where it does
               not fit a Quick */
            if (c->point)
                e.result = c->fix && c->qfit ? ET_QUICK : ET_FLOAT;
            else
                e.result = c->size <= ET_WORD ? ET_QUICK : ET_LONG;
            break;
        case ET_FLOAT:
            e.result = ET_FLOAT;
            break;
        default:                /* "Type Mismatch" */
            e.result = 0;
            break;
        }
        return e;
    }
    /* varcont: ALWAYS CONVERT BYTE TO WORD; bump the type up to the old
       one */
    e.ext = own == ET_BYTE;
    e.wordval = own == ET_BYTE || own == ET_WORD;  /* Fixed (#1179) */
    e.result = (unsigned char)own;
    if (!tsf)
        return e;               /* type not set yet... it is now! */
    if (tsf == ET_LONG && own == ET_QUICK) {
        /* Fixed (#1178): a Quick under a Long is held raw, not cut (the
           original's goto conv2, blitz2.s:10491-10498) */
        e.result = ET_LONG;
        e.raw = 1;
        return e;
    }
    if (tsf <= own)
        return e;
    e.conv_from = (unsigned char)own;
    e.conv_to = (unsigned char)tsf;
    e.result = (unsigned char)tsf;
    return e;
}
