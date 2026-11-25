/*
 * exprfold.c - the constant folding of a whole-number or Quick operation
 * in C (exprfold.h).  No asm counterpart: the original folded two
 * constants by running the code it had just generated for them
 * (blitz2.s:9007-9083, eval5); each function here is the code of the
 * eval.c routine it names, as the 68000 executes it.
 */
#include "exprfold.h"

#include "exprarith.h"

/* ------------------------------------------------------ the instructions */

void exprfold_setl(struct exprfold_reg *r, uint32_t v)
{
    r->v = v;
    r->known = EXF_ALL;
}

void exprfold_setw(struct exprfold_reg *r, uint16_t v)
{
    r->v = (r->v & 0xffff0000u) | v;
    r->known |= EXF_LO;
}

void exprfold_move(struct exprfold_reg *to, const struct exprfold_reg *from, unsigned size)
{
    if (size == 4) {
        *to = *from;
        return;
    }
    to->v = (to->v & 0xffff0000u) | (from->v & 0xffffu);
    to->known = (unsigned char)((to->known & EXF_HI) | (from->known & EXF_LO));
}

/* the low word changed, the high word kept */
static void setlo(struct exprfold_reg *r, uint32_t lo)
{
    r->v = (r->v & 0xffff0000u) | (lo & 0xffffu);
}

static int has(const struct exprfold_reg *r, unsigned char k)
{
    return (r->known & k) == k;
}

/* ext.l Dn */
static int extl(struct exprfold_reg *r)
{
    if (!has(r, EXF_LO))
        return 0;
    exprfold_setl(r, (uint32_t)(int32_t)(int16_t)(uint16_t)r->v);
    return 1;
}

/* swap Dn */
static void swap(struct exprfold_reg *r)
{
    r->v = (r->v << 16) | (r->v >> 16);
    r->known = (unsigned char)(((r->known & EXF_LO) << 1) | ((r->known & EXF_HI) >> 1));
}

/* clr.w Dn */
static void clrw(struct exprfold_reg *r)
{
    r->v &= 0xffff0000u;
    r->known |= EXF_LO;
}

int exprfold_neg(struct exprfold_reg *r, unsigned t, unsigned n)
{
    if (t >= ET_FLOAT || !has(r, t >= ET_LONG ? EXF_ALL : EXF_LO))
        return 0;
    for (; n; n--) {
        if (t >= ET_LONG)
            r->v = 0u - r->v;                   /* neg.l */
        else
            setlo(r, 0u - r->v);                /* neg.w */
    }
    return 1;
}

int exprfold_not(struct exprfold_reg *r, unsigned t)
{
    if (t >= ET_FLOAT || !has(r, t >= ET_LONG ? EXF_ALL : EXF_LO))
        return 0;
    if (t >= ET_LONG)
        r->v = ~r->v;                           /* not.l */
    else
        setlo(r, ~r->v);                        /* not.w */
    if (t == ET_QUICK)
        clrw(r);                                /* quickwhole */
    return 1;
}

/* ------------------------------------------------------ the conversions */

int exprfold_conv(struct exprfold_reg *r, unsigned from, unsigned to)
{
    /* the whole-number types and an address hold their value as a Long,
       a Byte and a Word in the low word */
    unsigned f = from == ET_ADDR ? ET_LONG : from, t = to == ET_ADDR ? ET_LONG : to;

    if (from < ET_BYTE || from > ET_ADDR || to < ET_BYTE || to > ET_ADDR || f == ET_FLOAT || t == ET_FLOAT)
        return from == to && from != ET_FLOAT && from != ET_STRING && from != ET_NONE;
    if (f == t)
        return 1;
    switch (f) {
    case ET_BYTE:
    case ET_WORD:
        if (t == ET_LONG)
            return extl(r);                     /* bytetolong, wordtolong */
        if (t == ET_QUICK) {                    /* wordtoquick: swap, clr.w */
            if (!has(r, EXF_LO))
                return 0;
            swap(r);
            clrw(r);
            return 1;
        }
        return has(r, EXF_LO);                  /* bytetoword, wordtobyte: no code */
    case ET_LONG:
        if (t == ET_QUICK) {                    /* longtoquick */
            if (!has(r, EXF_LO))
                return 0;
            swap(r);
            clrw(r);
            return 1;
        }
        return has(r, EXF_LO);                  /* longtobyte, longtoword: no code */
    default:                                    /* a Quick: quicktob2 = clr.w, swap */
        if (!has(r, EXF_HI))
            return 0;
        clrw(r);
        swap(r);
        return t == ET_LONG ? extl(r) : 1;      /* quicktolong: ext.l */
    }
}

/* ---------------------------------------------------- the operations */

int exprfold_prepare(unsigned op, const struct exprtype_bin *d, struct exprfold_reg *l, struct exprfold_reg *r)
{
    (void)op;
    /* operate: the right operand up (not for a comparison: compareop) */
    if (d->path != ETP_COMPARE && d->rup_from && !exprfold_conv(r, d->rup_from, d->rup_to))
        return 0;
    switch (d->path) {
    case ETP_COMPARE:
        /* operate's rcut, compareop's rup and lconv */
        if (d->rcut && !exprfold_conv(r, ET_QUICK, ET_LONG))
            return 0;
        if (d->rup_from && !exprfold_conv(r, d->rup_from, d->rup_to))
            return 0;
        return !d->lconv_from || exprfold_conv(l, d->lconv_from, d->lconv_to);
    case ETP_CMPLQ:
        return !d->rcut || exprfold_conv(r, ET_QUICK, ET_LONG);
    case ETP_MIXED:
        if (!d->handler)
            return 1;               /* the operation's own code (mixedop's switch) */
        /* mixedop: lcut, rcut (rawcut), lconv, rconv */
        if (d->lcut && !exprfold_conv(l, ET_QUICK, ET_LONG))
            return 0;
        if (d->rcut && !exprfold_conv(r, ET_QUICK, ET_LONG))
            return 0;
        if (d->lconv_from && !exprfold_conv(l, d->lconv_from, d->lconv_to))
            return 0;
        return !d->rconv_from || exprfold_conv(r, d->rconv_from, d->rconv_to);
    default:
        return !d->lconv_from || exprfold_conv(l, d->lconv_from, d->lconv_to);
    }
}

int exprfold_divzero(unsigned op, const struct exprtype_bin *d, const struct exprfold_reg *r)
{
    unsigned h = d->handler;

    if ((op != ETO_DIV && op != ETO_MOD) || d->path == ETP_COMPARE || d->path == ETP_CMPLQ || h < ET_BYTE
        || h > ET_FLOAT)
        return 0;                   /* divzerochk is not called or does nothing */
    if (h == ET_FLOAT)
        return -1;
    if (h < ET_LONG)
        return has(r, EXF_LO) ? (uint16_t)r->v == 0 : -1;
    return has(r, EXF_ALL) ? r->v == 0 : -1;
}

/* a comparison's result (cpoke: Scc, ext.w): -1 or 0 in the low word */
static void cpoke(struct exprfold_reg *l, int c)
{
    setlo(l, c ? 0xffffu : 0);
    l->known |= EXF_LO;
}

static int compare(unsigned op, int64_t a, int64_t b)
{
    switch (op) {
    case ETO_EQ: return a == b;
    case ETO_NE: return a != b;
    case ETO_LT: return a < b;
    case ETO_LE: return a <= b;
    case ETO_GT: return a > b;
    default: return a >= b;
    }
}

/* cmplq (#1177): the Long and the Quick compared exactly */
static int cmplq(unsigned op, unsigned left, struct exprfold_reg *l, const struct exprfold_reg *r)
{
    int64_t a, b;

    if (!has(l, EXF_ALL) || !has(r, EXF_ALL))
        return 0;
    if (left == ET_LONG) {
        a = (int64_t)(int32_t)l->v * 65536;
        b = (int32_t)r->v;
    } else {
        a = (int32_t)l->v;
        b = (int64_t)(int32_t)r->v * 65536;
    }
    cpoke(l, compare(op, a, b));
    return 1;
}

/* mixedop's own code (#1178): + and - of a raw Quick under a Long */
static int mixed(unsigned op, const struct exprtype_bin *d, struct exprfold_reg *l, const struct exprfold_reg *r)
{
    unsigned f = 0;

    if ((op != ETO_ADD && op != ETO_SUB) || !has(l, EXF_ALL) || !has(r, EXF_ALL))
        return 0;                   /* * and / are not folded (nocando) */
    exprfold_setl(l, (uint32_t)exprarith_mixed(op, (int32_t)l->v, d->lq, (int32_t)r->v, d->rq, &f));
    return 1;
}

/* the shifts (shpoke): the count a Word, the value a Long (a Quick stays
   one), lsl.l / lsr.l / asr.l Dcount,Dn: the count modulo 64 */
static int shift(unsigned op, unsigned h, struct exprfold_reg *l, struct exprfold_reg *r)
{
    unsigned n;

    if (!exprfold_conv(r, h, ET_WORD) || (h != ET_QUICK && !exprfold_conv(l, h, ET_LONG)) || !has(l, EXF_ALL)
        || !has(r, EXF_LO))
        return 0;
    n = r->v & 63;
    if (op == ETO_ASR)
        l->v = n >= 32 ? ((int32_t)l->v < 0 ? 0xffffffffu : 0) : (uint32_t)((int32_t)l->v >> n);
    else if (op == ETO_LSR)
        l->v = n >= 32 ? 0 : l->v >> n;
    else
        l->v = n >= 32 ? 0 : l->v << n;
    return 1;
}

/* the bit operators (dothebit): the value a Long, the bit number a Byte,
   btst / bset / bclr / bchg Dbit,Dn: the bit number modulo 32 */
static int bitop(unsigned op, unsigned h, struct exprfold_reg *l, struct exprfold_reg *r)
{
    uint32_t bit;

    if (!exprfold_conv(l, h, ET_LONG) || !exprfold_conv(r, h, ET_BYTE) || !has(l, EXF_ALL) || !has(r, EXF_LO))
        return 0;
    bit = (uint32_t)1 << (r->v & 31);
    switch (op) {
    case ETO_BITTST: cpoke(l, (l->v & bit) != 0); break;       /* sne, ext.w */
    case ETO_BITSET: l->v |= bit; break;
    case ETO_BITCLR: l->v &= ~bit; break;
    default: l->v ^= bit; break;
    }
    return 1;
}

/* doop: the handler of type h (operators[]) on l and r */
static int handler(unsigned op, const struct exprtype_bin *d, struct exprfold_reg *l, struct exprfold_reg *r)
{
    unsigned h = d->handler, f = 0;
    int words = h < ET_LONG;
    int32_t a, b;
    uint32_t v;

    if (h < ET_BYTE || h > ET_QUICK) {
        if (h == ET_ADDR && exprtype_iscompare(op) && has(l, EXF_ALL) && has(r, EXF_ALL)) {
            cpoke(l, compare(op, (int32_t)l->v, (int32_t)r->v));        /* doeql */
            return 1;
        }
        return 0;                   /* a Float, a string: not here */
    }
    if (op >= ETO_LSL && op <= ETO_ASR)
        return shift(op, h, l, r);
    if (op >= ETO_BITTST && op <= ETO_BITCHG)
        return bitop(op, h, l, r);
    if (!has(l, words ? EXF_LO : EXF_ALL) || !has(r, words ? EXF_LO : EXF_ALL))
        return 0;
    a = words ? (int16_t)(uint16_t)l->v : (int32_t)l->v;
    b = words ? (int16_t)(uint16_t)r->v : (int32_t)r->v;
    if (exprtype_iscompare(op)) {
        /* cmp.b, cmp.w, cmp.l */
        if (h == ET_BYTE) {
            a = (int8_t)(uint8_t)l->v;
            b = (int8_t)(uint8_t)r->v;
        }
        cpoke(l, compare(op, a, b));
        return 1;
    }
    switch (op) {
    case ETO_AMP:
    case ETO_AND:
    case ETO_BAR:
    case ETO_OR:
    case ETO_EOR:
        v = op == ETO_EOR ? l->v ^ r->v : op == ETO_AMP || op == ETO_AND ? l->v & r->v : l->v | r->v;
        if (words)
            setlo(l, v);            /* and.w, or.w, eor.w (#1218, #1217) */
        else {
            l->v = v;               /* and.l, or.l, eor.l */
            if (h == ET_QUICK)
                clrw(l);            /* quickwhole (#1209) */
        }
        return 1;
    case ETO_ADD:
    case ETO_SUB:
    case ETO_MUL:
    case ETO_DIV:
    case ETO_MOD:
        break;
    default:
        return 0;                   /* ^: both to Float (dopow) */
    }
    if (h == ET_QUICK) {
        if (op == ETO_MOD) {
            v = (uint32_t)exprarith_qmod_today(a, b, &f);       /* domodquick: a Word */
            if (f & EXA_TRAP)
                return 0;           /* DIVS.W by a whole part 0: the CPU's trap */
            setlo(l, v);
            return 1;
        }
        exprfold_setl(l, (uint32_t)exprarith_q16(op, a, b, &f));        /* qmul, qdivide */
        return 1;
    }
    if (h == ET_LONG && d->wordop && (op == ETO_MUL || op == ETO_DIV)) {
        /* #1179: two Word values in a later index, muls.w or divs.w */
        a = (int16_t)(uint16_t)l->v;
        b = (int16_t)(uint16_t)r->v;
        if (op == ETO_MUL) {
            exprfold_setl(l, (uint32_t)(a * b));
            return 1;
        }
        if (!b)
            return 0;               /* the CPU's trap */
        v = (uint32_t)exprarith_whole(ETO_DIV, ET_WORD, (int32_t)l->v, b, &f);
        /* divs.w: an overflow (-32768 / -1) keeps the dividend, neg.l */
        exprfold_setl(l, (int32_t)l->v == -32768 && b == -1 ? 32768u : (uint32_t)(int32_t)(int16_t)v);
        return 1;
    }
    if (h == ET_LONG && op == ETO_MOD)
        return 0;                   /* domodl: not folded (nocando) */
    v = (uint32_t)exprarith_whole(op, h, a, b, &f);
    if (f & EXA_TRAP)
        return 0;
    if (h == ET_LONG || (h == ET_WORD && (op == ETO_ADD || op == ETO_MUL)))
        exprfold_setl(l, v);        /* the Long, add.w + ext.l, muls.w */
    else
        setlo(l, v);                /* add.w, sub.w, muls.w, DIVS.W, domodword: the low word */
    return 1;
}

int exprfold_binop(unsigned op, const struct exprtype_bin *d, unsigned left, struct exprfold_reg *l,
                   struct exprfold_reg *r)
{
    if (op == ETO_POW || !d->result || !exprfold_prepare(op, d, l, r))
        return 0;
    if (exprfold_divzero(op, d, r))
        return 0;                   /* "Division by Zero" (or not known here) */
    if (d->path == ETP_CMPLQ)
        return cmplq(op, left, l, r);
    if (d->path == ETP_MIXED && !d->handler)
        return mixed(op, d, l, r);
    return handler(op, d, l, r);
}

int exprfold_libdep(unsigned op, const struct exprtype_bin *d)
{
    return op == ETO_MUL && (d->path == ETP_PLAIN || (d->path == ETP_MIXED && d->handler))
           && (d->handler == ET_QUICK || (d->handler == ET_LONG && !d->wordop));
}

int exprfold_value(const struct exprfold_reg *r, unsigned t, uint32_t *value)
{
    if (!has(r, t < ET_LONG ? EXF_LO : EXF_ALL))
        return 0;
    *value = t < ET_LONG ? (r->v & 0xffffu) : r->v;
    return 1;
}
