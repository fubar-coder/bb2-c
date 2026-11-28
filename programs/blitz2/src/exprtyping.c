/*
 * exprtyping.c - the typing pass of rule P+ (exprtyping.h; OpenSpec change
 * expr-tree-5-operand-typing, design decisions 1 and 6-10).  No asm
 * counterpart: the original typed an expression while it read it (the type
 * so far, blitz2.s:8983 eval5); this pass types the finished tree.
 *
 * Its parts, as the design names them: the fractional world (frac_*: an
 * expression or subtree with a Quick and no Float in 16.16 or 48.16, parts
 * 1, 3, 7, 8), the operand typing of everything else (typed: today's
 * operations with every operand typed by its own operands, a whole-number
 * operation in a comparison at least a Long, parts 5, 6, 8), the constants
 * (const_*: part 2 and the bit-operator and shift rules), and the
 * predicates that decide between them (has_quick, is_floaty, has_long).
 */
#include "exprtyping.h"

#include <stddef.h>
#include <string.h>

/* ------------------------------------------------------------ rationals */

/* an exact value num/den (den > 0, lowest terms); ok 0: beyond int64 */
struct rq {
    int64_t n, d;
    int ok;
};

static int64_t gcd64(int64_t a, int64_t b)
{
    if (a < 0)
        a = -a;
    while (b) {
        int64_t t = a % b;

        a = b;
        b = t;
    }
    return a ? a : 1;
}

static struct rq rq_make(int64_t n, int64_t d, int ok)
{
    struct rq r;
    int64_t g;

    r.ok = ok && d != 0 && n != INT64_MIN && d != INT64_MIN;
    if (!r.ok) {
        r.n = 0;
        r.d = 1;
        return r;
    }
    if (d < 0) {
        n = -n;
        d = -d;
    }
    g = gcd64(n, d);
    r.n = n / g;
    r.d = d / g;
    return r;
}

static struct rq rq_int(int64_t v)
{
    return rq_make(v, 1, 1);
}

static int mul_ovf(int64_t a, int64_t b, int64_t *r)
{
    return __builtin_mul_overflow(a, b, r);
}

static struct rq rq_op(struct rq a, struct rq b, char op)
{
    int64_t x, y, n, d;
    int ok = a.ok && b.ok;

    if (!ok)
        return rq_make(0, 1, 0);
    switch (op) {
    case '+':
    case '-':
        if (mul_ovf(a.n, b.d, &x) || mul_ovf(b.n, a.d, &y) || mul_ovf(a.d, b.d, &d))
            return rq_make(0, 1, 0);
        if (op == '+' ? __builtin_add_overflow(x, y, &n) : __builtin_sub_overflow(x, y, &n))
            return rq_make(0, 1, 0);
        return rq_make(n, d, 1);
    case '*':
        if (mul_ovf(a.n, b.n, &n) || mul_ovf(a.d, b.d, &d))
            return rq_make(0, 1, 0);
        return rq_make(n, d, 1);
    default:                    /* '/', b not 0 */
        if (mul_ovf(a.n, b.d, &n) || mul_ovf(a.d, b.n, &d))
            return rq_make(0, 1, 0);
        return rq_make(n, d, 1);
    }
}

/* a wanted Byte, Word or Long */
static int is_wholewant(unsigned want)
{
    return want == ET_BYTE || want == ET_WORD || want == ET_LONG;
}

static int rq_isint(struct rq a)
{
    return a.ok && a.d == 1;
}

static int64_t rq_floor(struct rq a)
{
    int64_t q = a.n / a.d;

    if (a.n % a.d && a.n < 0)
        q--;
    return q;
}

static int64_t rq_ceil(struct rq a)
{
    int64_t q = a.n / a.d;

    if (a.n % a.d && a.n > 0)
        q++;
    return q;
}

/* the value fits a constant of type t (a Quick's whole range -32768..32767) */
static int rq_fits(struct rq a, unsigned t)
{
    int64_t lo, hi;

    if (!a.ok)
        return t == ET_FLOAT;
    switch (t) {
    case ET_BYTE: lo = -128; hi = 127; break;
    case ET_WORD:
    case ET_QUICK: lo = -32768; hi = 32767; break;
    case ET_LONG: lo = INT32_MIN; hi = INT32_MAX; break;
    default: return 1;
    }
    return rq_floor(a) >= lo && rq_ceil(a) <= hi;
}

/* a 16.16 value holds it exactly (den a power of two up to 65536) */
static int rq_exact16(struct rq a)
{
    return a.ok && a.d <= 65536 && !(65536 % a.d);
}

/* ------------------------------------------------------------ the tree */

static int is_arith(unsigned op)
{
    return op == ETO_ADD || op == ETO_SUB || op == ETO_MUL || op == ETO_DIV || op == ETO_MOD;
}

static int is_shift(unsigned op)
{
    return op == ETO_LSL || op == ETO_ASL || op == ETO_LSR || op == ETO_ASR;
}

/* the logic and bit operators (whole parts) */
static int is_bit(unsigned op)
{
    return op == ETO_AND || op == ETO_OR || op == ETO_EOR || op == ETO_AMP || op == ETO_BAR
           || (op >= ETO_BITTST && op <= ETO_BITCHG);
}

static int is_cmp(const struct exprtyping_node *n)
{
    return n->kind == ETN_BIN && exprtype_iscompare(n->op);
}

int exprtyping_isconst(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_CONST: return 1;
    case ETN_NEG: return exprtyping_isconst(n->l);
    case ETN_BIN: return n->op != ETO_POW && exprtyping_isconst(n->l) && exprtyping_isconst(n->r);
    default: return 0;
    }
}

#define isconst exprtyping_isconst

static int has_quick(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_VAR: return n->type == ET_QUICK;
    case ETN_CONST: return 0;
    case ETN_BIN: return has_quick(n->l) || has_quick(n->r);
    default: return has_quick(n->l);
    }
}

static int has_type(const struct exprtyping_node *n, unsigned t)
{
    switch (n->kind) {
    case ETN_VAR: return n->type == t;
    case ETN_CONST: return 0;
    case ETN_BIN: return has_type(n->l, t) || has_type(n->r, t);
    default: return has_type(n->l, t);
    }
}

/* a constant written with a point somewhere in the subtree */
static int has_point(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_VAR: return 0;
    case ETN_CONST: return n->point;
    case ETN_BIN: return has_point(n->l) || has_point(n->r);
    default: return has_point(n->l);
    }
}

/* a constant subtree with a shift of a constant with a point: not folded,
   the shifted constant is a Quick (EM_R_PBITS: "Wie eine Quick-Variable") */
static int shifts_point(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN:
        if (is_shift(n->op) && has_point(n->l))
            return 1;
        return shifts_point(n->l) || shifts_point(n->r);
    case ETN_NEG:
    case ETN_NOT: return shifts_point(n->l);
    default: return 0;
    }
}

static int folded(const struct exprtyping_node *n)
{
    return isconst(n) && !shifts_point(n);
}

/* the exact value of a constant subtree (a whole-number operator on the
   whole parts; a division by 0: 0); a Mod by today's sign rule, the
   value the code computes (the divisor added to a negative remainder:
   -6 Mod 4 is 2) */
static struct rq const_value(const struct exprtyping_node *n)
{
    struct rq a, b;
    int64_t x, y;

    if (n->kind == ETN_CONST)
        return rq_make(n->num, n->den, 1);
    if (n->kind == ETN_NEG)
        return rq_op(rq_int(0), const_value(n->l), '-');
    a = const_value(n->l);
    b = const_value(n->r);
    if (exprtype_iscompare(n->op)) {
        /* a comparison of constants evaluated ("expr-tree-5 group 3: three
           readings of the four rule points": "Auch nach Regel falten") */
        struct rq d = rq_op(a, b, '-');
        int s, t;

        if (!d.ok)
            return rq_make(0, 1, 0);
        s = d.n > 0 ? 1 : d.n < 0 ? -1 : 0;
        switch (n->op) {
        case ETO_EQ: t = s == 0; break;
        case ETO_NE: t = s != 0; break;
        case ETO_LT: t = s < 0; break;
        case ETO_LE: t = s <= 0; break;
        case ETO_GT: t = s > 0; break;
        default: t = s >= 0; break;
        }
        return rq_int(t ? -1 : 0);
    }
    switch (n->op) {
    case ETO_ADD: return rq_op(a, b, '+');
    case ETO_SUB: return rq_op(a, b, '-');
    case ETO_MUL: return rq_op(a, b, '*');
    case ETO_DIV: return b.ok && b.n ? rq_op(a, b, '/') : rq_int(0);
    default: break;
    }
    if (!a.ok || !b.ok)
        return rq_make(0, 1, 0);
    x = rq_floor(a);
    y = rq_floor(b);
    switch (n->op) {
    case ETO_AND:
    case ETO_AMP: return rq_int(x & y);
    case ETO_OR:
    case ETO_BAR: return rq_int(x | y);
    case ETO_EOR: return rq_int(x ^ y);
    case ETO_MOD:
        if (b.n && (!rq_isint(a) || !rq_isint(b))) {
            /* Fixed (#1219, #1223): a Mod with a fraction in an operand is exact
               (as the pass's code does, today's sign rule): a - trunc(a/b)*b,
               plus b when negative */
            struct rq t;

            t = rq_op(a, b, '/');
            if (!t.ok)
                return t;
            t = rq_op(a, rq_op(b, rq_int(t.n / t.d), '*'), '-');
            return t.ok && t.n < 0 ? rq_op(t, b, '+') : t;
        }
        if (!y)
            return rq_int(0);
        /* #1228: today's sign rule (domodword, domodlong), not C's % */
        return rq_int(x % y < 0 ? x % y + y : x % y);
    case ETO_LSL:
    case ETO_ASL: return rq_int(y >= 0 && y < 32 ? (int64_t)((uint64_t)x << y) : 0);
    case ETO_LSR:
    case ETO_ASR: return rq_int(y >= 0 && y < 32 ? x >> y : 0);
    default: return rq_int(-1);         /* no other operator */
    }
}

int exprtyping_constvalue(const struct exprtyping_node *n, int64_t *num, int64_t *den)
{
    struct rq a = const_value(n);

    *num = a.n;
    *den = a.d;
    return !a.ok;
}

/* ------------------------------------------------------------ the pass */

struct ctx {
    int dry;                    /* typing a subtree only for its type: nothing written */
    int refused;
    int quickhere;              /* a Quick variable is in the expression or comparison side typed */
    int cmplong;                /* in a comparison's operands: whole-number operations at least Long */
    int reachedf;               /* the expression is typed under Float as a reached format (part 7) */
    int fmtfloat;               /* the Float typed under is a reached format, no wanted type */
    int bittaker;               /* the operand typed is taken by a bit operator, shift or NOT */
    int modtree;                /* const_flow of a const_quick expression (#1224) */
    int tsr;                    /* the tree is classed (exprtyping_classify): the whole-width rule */
    const struct exprtyping_node *top;  /* the root (the classes: a shift at the top, frac_shift) */
    unsigned wrap;              /* the next node typed is reached from a destination that wraps at
                                   wrap bits (8, 16, 32) through operators that keep the low bits
                                   only (wb_modular); 0 none.  typed() takes it at its entry */
};

static unsigned tmax(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

static void mark(struct ctx *c, struct exprtyping_node *n, unsigned t, unsigned fmt)
{
    if (c->dry)
        return;
    n->typed = 1;
    n->t = (unsigned char)t;
    n->own = (unsigned char)t;
    n->fmt = (unsigned char)fmt;
}

static void record(struct ctx *c, struct exprtyping_node *n, unsigned path, unsigned lfrom, unsigned las,
                   unsigned lwide, unsigned rfrom, unsigned ras, unsigned rwide, unsigned result)
{
    if (c->dry)
        return;
    n->o.path = (unsigned char)path;
    n->o.lfrom = (unsigned char)lfrom;
    n->o.las = (unsigned char)las;
    n->o.lwide = (unsigned char)lwide;
    n->o.rfrom = (unsigned char)rfrom;
    n->o.ras = (unsigned char)ras;
    n->o.rwide = (unsigned char)rwide;
    n->o.result = (unsigned char)result;
    n->o.fast = 0;
}

static unsigned refuse(struct ctx *c)
{
    c->refused = 1;
    return 0;
}

/* --------------------------------------------------- the constants */

/* the type today's constant fetch (consttype2) gives a literal of value a
   (point: written with a point) under the type so far t.
   BUG(orig) (#1231): a constant with a point and no fraction (2.0) stays
   a whole number here, so 3/2.0 is the whole division 1 and 7/2.0+0.5 is
   3, as in the original; a constant with a point is never whole. */
static unsigned const_fetch(int point, struct rq a, unsigned t)
{
    int64_t iv;

    if (!a.ok)
        return ET_FLOAT;
    switch (t) {
    case ET_NONE:
        if (point)
            return ET_FLOAT;
        t = ET_BYTE;
        /* fall through */
    case ET_BYTE:
        /* a whole value with a point stays the whole Byte (#1186's litcut,
           #1188: a fraction keeps its fraction as a Float) */
        if (point && (!rq_isint(a) || !rq_fits(a, ET_LONG)))
            return ET_FLOAT;
        iv = point ? (a.n / a.d) : a.n;
        return iv >= -128 && iv <= 127 ? ET_BYTE : iv >= -32768 && iv <= 32767 ? ET_WORD : ET_LONG;
    case ET_WORD:
        if (point)
            return ET_FLOAT;
        return a.n >= -32768 && a.n <= 32767 ? ET_WORD : ET_LONG;
    case ET_LONG: return point ? ET_FLOAT : ET_LONG;
    case ET_QUICK:
        if (point) {
            iv = a.n / a.d;
            return iv > 32767 || iv < -32768 ? ET_FLOAT : ET_QUICK;
        }
        return a.n > 32767 || a.n < -32768 ? ET_LONG : ET_QUICK;
    default: return ET_FLOAT;
    }
}

static unsigned handler_result(unsigned op, unsigned t);

static int has_div(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN: return n->op == ETO_DIV || has_div(n->l) || has_div(n->r);
    case ETN_NEG:
    case ETN_NOT: return has_div(n->l);
    default: return 0;
    }
}

/* an operation of constants without a point and without a / whose value
   is whole (7 Mod 3, 2*3, 7 AND 3): in the variable form of a #1224
   expression a constant of its value beside the Quick */
static int const_wholesub(const struct exprtyping_node *n)
{
    struct rq a;

    if (n->kind != ETN_BIN || has_point(n) || has_div(n))
        return 0;
    a = const_value(n);
    return a.ok && rq_isint(a);
}

/* #1227: the operation n of a constant subtree, its left operand of type
   lt and its right one of the whole type rt below it, is computed in lt:
   +, -, *, / or Mod with a Float on the left, or a Quick and a Byte or
   Word (a Mod's Word), or a Quick and a Long where the right operand and
   the result are in the Quick range ((6 Mod 4)*2, 40000-39999); not a /
   or Mod by 0, which stays as before */
static int const_raise(const struct exprtyping_node *n, unsigned lt, unsigned rt)
{
    if (!is_arith(n->op) || !rt || rt >= lt)
        return 0;
    if ((n->op == ETO_DIV || n->op == ETO_MOD) && !const_value(n->r).n)
        return 0;
    if (lt == ET_FLOAT || (lt == ET_QUICK && rt <= ET_WORD))
        return 1;
    return lt == ET_QUICK && rt == ET_LONG && rq_fits(const_value(n->r), ET_QUICK)
           && rq_fits(const_value(n), ET_QUICK);
}

/* a constant subtree typed as today's flow types it under the type so
   far t (a constant alone, or one not beside an operand): each operation
   in the type so far after its right operand; a bit operator or shift in
   Float is refused, as today (1.5 AND 3) */
static unsigned const_flow(struct ctx *c, const struct exprtyping_node *n, unsigned t)
{
    unsigned lt, rt;

    if (c->modtree && const_wholesub(n))
        return const_fetch(0, const_value(n), t);   /* #1224: as in the variable form, a constant */
    switch (n->kind) {
    case ETN_CONST: return const_fetch(n->point, const_value(n), t);
    case ETN_NEG:
        if (n->l->kind == ETN_CONST)
            return const_fetch(n->l->point, const_value(n), t);
        return const_flow(c, n->l, t);
    default: break;
    }
    lt = const_flow(c, n->l, t);
    if (exprtype_iscompare(n->op)) {
        rt = const_flow(c, n->r, 0);
        if ((lt == ET_LONG && rt == ET_QUICK) || (lt == ET_QUICK && rt == ET_LONG))
            return ET_WORD;
        return tmax(lt, rt) > ET_FLOAT ? refuse(c) : ET_WORD;
    }
    rt = const_flow(c, n->r, lt);
    if (is_cmp(n->r) && lt > rt && rt)
        rt = lt;                /* #1182: a comparison's Word up to the left operand's type */
    else if (const_raise(n, lt, rt))
        /* Fixed (#1227): a whole-number result of the right operation (a
           Mod's Word in a Quick, its Long in a Float) up to the left
           operand's Quick or Float, not the left operand down to it
           (0.5+6 Mod 4 is 2.5, as with variables) */
        rt = lt;
    if (rt == ET_FLOAT && (is_bit(n->op) || is_shift(n->op)))
        return refuse(c);
    if (n->op == ETO_MOD && (rt == ET_QUICK || rt == ET_FLOAT)) {
        struct rq a = const_value(n->l), b = const_value(n->r);

        /* Fixed (#1219, #1223): the exact Mod with a fraction in an operand,
           where it has a fraction, in the Quick or Float it is computed in
           (a whole result keeps the handler's type) */
        if (a.ok && b.ok && b.n && (!rq_isint(a) || !rq_isint(b)) && !rq_isint(const_value(n)))
            return rt;
    }
    return handler_result(n->op, rt);
}

/* part 2 (EM_R_PCONST): a constant with a point is a Float here, as in
   the original: under a wanted Float, beside a Float or beyond the Quick
   range always; under a wanted Quick never; under a wanted Byte, Word or
   Long always (#1222: the user's decision of 2026-11-26, "P+ part 3 under
   Byte/Word (A1) and F2 under whole destinations (option 1)"); else
   unless a Quick is beside it or in its expression */
static int const_isfloat(const struct ctx *c, struct rq a, unsigned want, unsigned beside)
{
    if (want == ET_FLOAT || beside == ET_FLOAT || !rq_fits(a, ET_QUICK))
        return 1;
    if (want == ET_QUICK)
        return 0;
    if (is_wholewant(want))
        return 1;
    return !(beside == ET_QUICK || c->quickhere);
}

enum { BIT_NONE, BIT_OPERAND, BIT_COUNT, BIT_SHIFTED };

/* AND, OR or EOR in a constant subtree on an operand with a point or a
   fraction (a comparison in it evaluated: const_value) */
static int bit_fraction(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN:
        if ((n->op == ETO_AND || n->op == ETO_OR || n->op == ETO_EOR || n->op == ETO_AMP || n->op == ETO_BAR)
            && (has_point(n->l) || has_point(n->r) || !rq_isint(const_value(n->l))
                || !rq_isint(const_value(n->r))))
            return 1;
        return bit_fraction(n->l) || bit_fraction(n->r);
    case ETN_NEG: return bit_fraction(n->l);
    default: return 0;
    }
}

/* a Mod of two constants with a fraction or a point in an operand (#1223:
   the user's decision of 2026-11-26, "B: like the variable form"): a Quick
   under a whole destination, rounded down as the variable form, an
   exception to the Float of a constant with a point there (#1222) */
static int const_modq(const struct exprtyping_node *n)
{
    struct rq a, b;

    if (n->kind == ETN_NEG)
        return const_modq(n->l);
    if (n->kind != ETN_BIN || n->op != ETO_MOD)
        return 0;
    a = const_value(n->l);
    b = const_value(n->r);
    return a.ok && b.ok && b.n && (has_point(n) || !rq_isint(a) || !rq_isint(b));
}

/* +, - or * (or a negation) over whole constants without a point, whole
   operations of them (const_wholesub: 7 Mod 3) and Mods of const_modq, one
   of them at least (*mod) */
static int const_modtree(const struct exprtyping_node *n, int *mod)
{
    switch (n->kind) {
    case ETN_CONST: return !n->point && n->den == 1;
    case ETN_NEG: return const_modtree(n->l, mod);
    case ETN_BIN:
        if (const_modq(n))
            return *mod = 1;
        if (const_wholesub(n))
            return 1;
        return (n->op == ETO_ADD || n->op == ETO_SUB || n->op == ETO_MUL) && const_modtree(n->l, mod)
               && const_modtree(n->r, mod);
    default: return 0;
    }
}

/* the constant subtree n is a Quick under a whole destination, rounded
   down as its variable form (each Mod of const_modq one of Quick
   variables), an expression of a Quick without a Float: a Mod of
   const_modq (#1223) or a larger expression of const_modtree (#1224: the
   user's decision of 2026-11-26, the rule of #1223 also in a larger
   constant expression) */
static int const_quick(const struct exprtyping_node *n)
{
    int mod = 0;

    return const_modq(n) || (const_modtree(n, &mod) && mod);
}

/* the type of the constant subtree n beside an operand of type beside (0:
   none) under the wanted type want; role: BIT_* (an operand of AND, OR,
   EOR or NOT, a shift count, the shifted operand) */
static unsigned const_type(struct ctx *c, const struct exprtyping_node *n, unsigned want, unsigned beside,
                           int role)
{
    struct rq a = const_value(n);
    int whole = rq_isint(a), point = has_point(n), bitwhole = 0;
    unsigned tt;
    int modtree = is_wholewant(want) && const_quick(n);

    if (modtree)
        want = ET_QUICK;

    if (!beside)
        tt = want;
    else if (whole)
        tt = 0;                 /* a whole constant by its smallest type */
    else
        tt = beside == ET_FLOAT || want == ET_FLOAT ? ET_FLOAT : rq_fits(a, ET_QUICK) ? ET_QUICK : ET_FLOAT;
    /* a fraction a 16.16 value cannot hold exactly stays a Float beside a
       whole number */
    if (tt == ET_QUICK && beside && beside != ET_QUICK && !whole && !rq_exact16(a))
        tt = ET_FLOAT;
    if (beside && a.ok && (!whole || point))
        tt = const_isfloat(c, a, want, beside) ? ET_FLOAT : ET_QUICK;
    if (role == BIT_SHIFTED) {
        if (a.ok && (point || !whole) && rq_fits(a, ET_QUICK))
            tt = ET_QUICK;
    } else if (role != BIT_NONE && a.ok && (point || !whole)) {
        if (role == BIT_COUNT)
            return refuse(c);   /* a constant with a point as a shift count */
        a = rq_int(rq_floor(a));
        if (rq_fits(a, ET_LONG)) {
            tt = rq_fits(a, ET_BYTE) ? ET_BYTE : rq_fits(a, ET_WORD) ? ET_WORD : ET_LONG;
            bitwhole = 1;
        }
    }
    if (!bitwhole && role != BIT_SHIFTED && bit_fraction(n)) {
        /* "expr-tree-5 group 3 findings: four rule points of P+", 3: a bit
           operator of constants with a fraction folds as b AND 1.5 takes
           it, the fraction cut (const_value: the whole parts); refused
           under a wanted Float, as the bit operator with a variable is */
        if (want == ET_FLOAT && !c->fmtfloat)
            return refuse(c);
        a = const_value(n);
        tt = !beside ? want : rq_isint(a) ? 0 : const_isfloat(c, a, want, beside) ? ET_FLOAT : ET_QUICK;
        bitwhole = 1;
    }
    if (!bitwhole && (n->kind == ETN_CONST || !beside || !a.ok || (!whole && tt != ET_QUICK && tt != ET_FLOAT))) {
        c->modtree = modtree;
        tt = const_flow(c, n, tt);
        c->modtree = 0;
        return tt;
    }
    /* the folded value as a constant of its own: its magnitude, negated */
    if (a.n < 0)
        a = rq_make(-a.n, a.d, 1);
    return const_fetch(!rq_isint(a), a, tt);
}

/* ------------------------------------------------- the predicates */

static unsigned typed(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned beside, int role);
static unsigned frac_operand(struct ctx *c, struct exprtyping_node *n, unsigned want, int whole);

static int is_floaty(struct ctx *c, const struct exprtyping_node *n, unsigned want);
static int q48_class(const struct exprtyping_node *n);
static int q_shift(const struct exprtyping_node *n);

/* is_floaty of an operand: a comparison's result is a whole number, also
   when it compares in Float ("expr-tree-5 group 3: three readings of the
   four rule points": "Vergleichsergebnis ganzzahlig") */
static int floaty_operand(struct ctx *c, const struct exprtyping_node *n, unsigned want)
{
    return is_cmp(n) ? 0 : is_floaty(c, n, want);
}

/* part 2, 5: the subtree under the wanted type want holds a Float value (a
   Float variable, a ^, a constant with a point that is a Float there) */
static int is_floaty(struct ctx *c, const struct exprtyping_node *n, unsigned want)
{
    switch (n->kind) {
    case ETN_VAR: return n->type == ET_FLOAT;
    case ETN_CONST: {
        struct rq a = const_value(n);

        if (!a.ok || (rq_isint(a) && !n->point))
            return 0;
        return const_isfloat(c, a, want, 0);
    }
    case ETN_BIN:
        if (n->op == ETO_POW)
            return 1;
        if (exprtype_iscompare(n->op)) {
            int save = c->quickhere, k;

            c->quickhere = has_quick(n->l);
            k = floaty_operand(c, n->l, 0);
            c->quickhere = has_quick(n->r);
            k = k || floaty_operand(c, n->r, 0);
            c->quickhere = save;
            /* a constant alone beside the other operand takes its class */
            if (isconst(n->l) && !has_type(n->r, ET_FLOAT) && has_quick(n->r))
                k = floaty_operand(c, n->r, 0);
            if (isconst(n->r) && !has_type(n->l, ET_FLOAT) && has_quick(n->l)) {
                c->quickhere = 1;
                k = floaty_operand(c, n->l, 0);
                c->quickhere = save;
            }
            return k;
        }
        return floaty_operand(c, n->l, want) || floaty_operand(c, n->r, want);
    default: return floaty_operand(c, n->l, want);
    }
}

/* the type of a subtree by the operand typing, nothing written */
static unsigned dry_type(struct ctx *c, struct exprtyping_node *n, unsigned want)
{
    struct ctx d = *c;
    unsigned t;

    d.dry = 1;
    d.refused = 0;
    t = typed(&d, n, want, 0, BIT_NONE);
    return d.refused ? 0 : t;
}

/* a whole constant beyond a Word somewhere in the subtree */
static int has_bigconst(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_VAR: return 0;
    case ETN_CONST: {
        struct rq a = const_value(n);

        return rq_isint(a) && !rq_fits(a, ET_WORD);
    }
    case ETN_BIN: return has_bigconst(n->l) || has_bigconst(n->r);
    default: return has_bigconst(n->l);
    }
}

/* part 1: a Long takes part in the arithmetic of the subtree under the
   wanted type want (a Long variable, a whole constant beyond a Word, a
   whole-number or bit-operator subtree of type Long there) */
static int has_long(struct ctx *c, struct exprtyping_node *n, unsigned want)
{
    if (isconst(n)) {
        struct rq a = const_value(n);

        return rq_isint(a) && !rq_fits(a, ET_WORD);
    }
    if (c->tsr && q48_class(n))
        /* the classes: an operation of Quick class by its operands, as one
           with a Quick in it (its typing is the fractional world's) */
        return has_long(c, n->l, want) || (n->kind == ETN_BIN && has_long(c, n->r, want));
    if (c->tsr && q_shift(n))
        return has_long(c, n->l, want);     /* a shift of Quick class: its left operand */
    if (!has_quick(n)) {
        if (n->kind == ETN_VAR)
            return n->type == ET_LONG;
        if (n->kind == ETN_NEG)
            return has_long(c, n->l, want);
        /* Word+Word is a Long only by its handler (add.w): a Long only in
           a comparison, where whole-number operations are at least Long */
        if (n->kind == ETN_BIN && (n->op == ETO_ADD || n->op == ETO_SUB) && !c->cmplong)
            return has_long(c, n->l, want) || has_long(c, n->r, want);
        return has_type(n, ET_LONG) || has_bigconst(n) || dry_type(c, n, want) == ET_LONG;
    }
    switch (n->kind) {
    case ETN_VAR: return 0;
    case ETN_NEG: return has_long(c, n->l, want);
    case ETN_BIN:
        if (exprtype_iscompare(n->op))
            return 0;
        if (is_arith(n->op))
            return has_long(c, n->l, want) || has_long(c, n->r, want);
        return dry_type(c, n, want) == ET_LONG;
    default: return dry_type(c, n, want) == ET_LONG;
    }
}

/* -------------------------------------------- the fractional world */

/* an operation of Quick class (exprtyping_classify) that the classes
   compute in 48.16 (task 4.2): + - * / and Mod (Mod's format still rule
   P+'s, task 4.7) and the unary minus, not a constant subtree */
static int q48_class(const struct exprtyping_node *n)
{
    return n->cls == ET_QUICK && !folded(n) && (n->kind == ETN_NEG || (n->kind == ETN_BIN && is_arith(n->op)));
}

/* a shift of Quick class (task 4.4): a Quick left operand, or the shift
   exception (E) of a whole one; typed by frac_shift */
static int q_shift(const struct exprtyping_node *n)
{
    return n->cls == ET_QUICK && n->kind == ETN_BIN && is_shift(n->op) && !folded(n);
}

/* the format of a fractional value (ETF_*) as the bits of the model's
   formats: 0 whole, 16 (16.16), 32 (48.16; a Float side 32 too) */
enum { FW_WHOLE = 0, FW_16 = 16, FW_48 = 32 };

static unsigned frac_value(struct ctx *c, struct exprtyping_node *n, unsigned want, int force, int whole,
                           unsigned *from);

/* a bit operator, shift, NOT or ^ with a Quick inside: by the operand
   typing (its operands typed by their own), its result in the fractional
   format of its type */
static unsigned frac_other(struct ctx *c, struct exprtyping_node *n, unsigned want)
{
    unsigned t = typed(c, n, want, 0, BIT_NONE);

    return t == ET_QUICK ? FW_16 : t == ET_FLOAT ? FW_48 : FW_WHOLE;
}

/* an operand of a fractional operation: a fractional subtree (a Quick in
   it, or with whole set an arithmetic one: part 7) in its format (from
   ET_QUICK), else a whole number by the operand typing (from its type;
   ETY_CONST a constant) */
static unsigned frac_side(struct ctx *c, struct exprtyping_node *n, unsigned want, int force, int whole,
                          unsigned *from)
{
    if (c->tsr) {
        /* the classes (task 4.2): a node of Quick class in the fractional
           world, a whole one as the whole number its class is (not in a
           fractional format: P+'s part 7 goes) */
        if (folded(n)) {
            *from = ETY_CONST;
            return FW_WHOLE;
        }
        if (n->cls == ET_QUICK) {
            *from = ET_QUICK;
            return frac_value(c, n, want, force, whole, from);
        }
        *from = typed(c, n, 0, 0, BIT_NONE);   /* no destination reaches it */
        return FW_WHOLE;
    }
    if (has_quick(n)
        || (whole && !isconst(n) && (n->kind == ETN_NEG || (n->kind == ETN_BIN && is_arith(n->op))))) {
        *from = ET_QUICK;
        return frac_value(c, n, want, force, whole, from);
    }
    if (folded(n)) {
        *from = ETY_CONST;
        return FW_WHOLE;
    }
    *from = typed(c, n, want, 0, BIT_NONE);
    return FW_WHOLE;
}

/* a whole type a shift count is taken in: a Quick rounded down (quicktoword),
   a fractional value as frac_operand gives it; an address a Long */
static unsigned shift_count(unsigned t)
{
    return t == ET_QUICK ? ET_WORD : t >= ET_BYTE && t <= ET_LONG ? t : ET_LONG;
}

/* A shift of Quick class under the classes (task 4.4 of type-system-rework;
   spec delta compiler/expression-types, "A shift keeps the type of its left
   operand", "A shift at the top under a Quick destination", "A left shift
   at the top under a Quick destination wraps"): a Quick left operand's LSR
   on its 16.16 bits (the low 32 bits of a 48.16 one), its ASR on the 16.16
   bits when it is held in them (exact) and in 48.16 when it is a 48.16
   pair, its LSL and ASL in 48.16; the shift exception (E) of a whole left
   operand with 16 fraction bits: of a Byte or Word class on the 16.16 bits
   (in the Quick range), of a Long class in 48.16 (on the 16.16 bits inside
   the Quick range, in 48.16 beyond it: the code decides at run time).  A
   left shift of a Quick at the top under a Quick destination (J) or under
   a Byte or Word one is the 16.16 shift: only the low 32 bits of the
   48.16 value reach the result.  The 16.16 shifts are the build's code
   (ETY_PLAIN in Quick, the count converted to a Quick and rounded down by
   the handler); the 48.16 ones the pass's (ETY_SHIFT).  The count is
   whole: a Quick rounded down, a 48.16 value as frac_operand takes it under
   a shift (its whole part). */
static unsigned frac_shift(struct ctx *c, struct exprtyping_node *n, unsigned want)
{
    unsigned op = n->op, lt, rt, lf = ET_QUICK, fl = FW_WHOLE;
    int lc = isconst(n->l), rc = isconst(n->r), wide;

    if (n->l->cls == ET_QUICK) {
        fl = frac_side(c, n->l, want, 0, 0, &lf);
        lt = ET_QUICK;
    } else {
        /* the shift exception (E): the whole left operand as rule P+ types it */
        c->bittaker = 1;
        c->wrap = 0;
        lt = typed(c, n->l, want, 0, lc ? BIT_SHIFTED : BIT_NONE);
        lf = lt;
    }
    if (c->refused)
        return FW_WHOLE;
    c->bittaker = 1;
    c->wrap = 0;
    rt = typed(c, n->r, want, rc ? lt : 0, rc ? BIT_COUNT : BIT_NONE);
    if (c->refused)
        return FW_WHOLE;
    if (lt == ET_STRING || rt == ET_STRING || rt == ET_FLOAT) {
        refuse(c);
        return FW_WHOLE;
    }
    if (n->l->cls != ET_QUICK)
        wide = n->l->cls == ET_LONG;
    else if (op == ETO_LSR)
        wide = 0;
    else if (op == ETO_ASR)
        wide = fl == FW_48;
    else
        wide = !((want == ET_QUICK && (n->exc & ETX_SHIFTWRAP))
                 || (n == c->top && (want == ET_BYTE || want == ET_WORD)));
    if (!wide) {
        record(c, n, ETY_PLAIN, lc ? ETY_CONST : lf, ET_QUICK, 0, rc ? ETY_CONST : rt, ET_QUICK, 0, ET_QUICK);
        mark(c, n, ET_QUICK, ETF_TYPE);
        return FW_16;
    }
    record(c, n, ETY_SHIFT, lc ? ETY_CONST : lf, n->l->cls == ET_QUICK ? ET_QUICK : ET_LONG, fl == FW_48,
           rc ? ETY_CONST : rt, shift_count(rt), 0, ET_LONG);
    mark(c, n, ET_LONG, ETF_Q48);
    return FW_48;
}

/* n in the fractional world under want; force: every operation 48.16;
   whole: a whole-number arithmetic subtree in the fractional format */
static unsigned frac_value(struct ctx *c, struct exprtyping_node *n, unsigned want, int force, int whole,
                           unsigned *from)
{
    unsigned fl, fr, lf, rf, f;

    *from = ET_QUICK;
    switch (n->kind) {
    case ETN_VAR:
        mark(c, n, ET_QUICK, ETF_Q16);
        return FW_16;
    case ETN_NEG:
        f = frac_side(c, n->l, want, force, whole, &lf);
        if (c->tsr && n->cls == ET_QUICK) {
            /* the classes: a minus sign of Quick class in 48.16 (its operand
               widened first: -q of -32768 is 32768) */
            if (!c->dry)
                n->wide = (unsigned char)(f != FW_48);
            f = FW_48;
        }
        mark(c, n, f ? ET_QUICK : lf, f == FW_48 ? ETF_Q48 : f ? ETF_Q16 : ETF_TYPE);
        return f;
    case ETN_NOT: return frac_other(c, n, want);
    default: break;
    }
    if (c->tsr && q_shift(n))
        return frac_shift(c, n, want);
    if (!exprtype_iscompare(n->op) && !is_arith(n->op))
        return frac_other(c, n, want);
    if (exprtype_iscompare(n->op)) {
        /* part 8: both sides with no wanted type, each in its own format,
           compared exactly; a Long in either side makes both 48.16 */
        int save = c->cmplong, sq = c->quickhere, fc;

        c->cmplong = 1;
        /* the classes: each side by its class (a Quick operation 48.16) */
        fc = !c->tsr && (has_long(c, n->l, 0) || has_long(c, n->r, 0));
        c->quickhere = has_quick(n->l);
        fl = frac_side(c, n->l, 0, fc, 1, &lf);
        c->quickhere = has_quick(n->r);
        fr = frac_side(c, n->r, 0, fc, 1, &rf);
        c->quickhere = sq;
        c->cmplong = save;
        record(c, n, ETY_EXACT, isconst(n->l) ? ETY_CONST : lf, lf, fl == FW_48, isconst(n->r) ? ETY_CONST : rf, rf,
               fr == FW_48, ET_WORD);
        mark(c, n, ET_WORD, ETF_TYPE);
        return FW_WHOLE;
    }
    /* part 1: under a wanted Byte, Word or Quick an operation with a Long
       in it is 48.16, and so is everything below it */
    if (c->tsr && n->op != ETO_MOD) {
        /* the classes (task 4.2): every + - * / of Quick class in 48.16,
           its operands by their classes */
        frac_side(c, n->l, want, 0, 0, &lf);
        frac_side(c, n->r, want, 0, 0, &rf);
        f = FW_48;
    } else {
        if (want >= ET_BYTE && want <= ET_QUICK && has_long(c, n, want))
            force = 1;
        if (c->tsr && has_long(c, n, want))
            force = 1;          /* the classes: a Mod with a Long in it in 48.16, also in a
                                   comparison and without a destination (rule P+'s force) */
        fl = frac_side(c, n->l, want, force, whole, &lf);
        fr = frac_side(c, n->r, want, force, whole, &rf);
        f = force || want == ET_LONG ? FW_48 : FW_16;
        if (c->tsr && (fl == FW_48 || fr == FW_48))
            f = FW_48;          /* Mod (task 4.7) of a 48.16 operand */
    }
    record(c, n, f == FW_48 ? ETY_Q48 : ETY_Q16, isconst(n->l) ? ETY_CONST : lf, ET_QUICK, f == FW_48,
           isconst(n->r) ? ETY_CONST : rf, ET_QUICK, f == FW_48, f == FW_48 ? ET_LONG : ET_QUICK);
    mark(c, n, f == FW_48 ? ET_LONG : ET_QUICK, f == FW_48 ? ETF_Q48 : ETF_Q16);
    return f;
}

/* a fractional subtree inside an operation of the operand typing (a bit
   operator, a shift, NOT, an expression with a Float: part 6) or the
   operand of a Float comparison (whole set, part 8): computed by the
   fractional world under want, then taken by the operation ("expr-tree-5
   group 3 findings: four rule points of P+", 1): a 16.16 value as the
   Quick it is; a 48.16 value by a bit operator, shift or NOT as its whole
   part, a Long (rounded down; the classes: also where rule P+ took its low
   32 bits, task 4.4), anywhere else straight as a Float; a comparison as
   its Word */
static unsigned frac_operand(struct ctx *c, struct exprtyping_node *n, unsigned want, int whole)
{
    unsigned from, f, t;
    int force = !want && !is_cmp(n) && !(c->tsr && q_shift(n)) ? has_long(c, n, 0) : 0, bit = c->bittaker;

    f = frac_value(c, n, want, force, whole || (want != ET_LONG && !is_cmp(n)), &from);
    if (is_cmp(n))
        return ET_WORD;
    t = f != FW_48 ? ET_QUICK : bit ? ET_LONG : ET_FLOAT;
    if (!c->dry) {
        n->t = (unsigned char)t;
        n->fmt = f == FW_48 ? ETF_Q48 : ETF_Q16;
    }
    return t;
}

/* ---------------------------------------------- the operand typing */

/* the type today's handler of op gives for operands of type t */
static unsigned handler_result(unsigned op, unsigned t)
{
    switch (op) {
    case ETO_ADD:
    case ETO_MUL: return t == ET_BYTE ? ET_WORD : t == ET_WORD ? ET_LONG : t;
    case ETO_MOD: return t == ET_QUICK ? ET_WORD : t == ET_FLOAT ? ET_LONG : t;
    case ETO_POW: return ET_FLOAT;
    default:
        if (is_shift(op))
            return t == ET_QUICK ? ET_QUICK : ET_LONG;
        return t;
    }
}

static int wb_modular(unsigned kind, unsigned op);

/* the operands of the binary n (no comparison): a constant after the
   other operand, which it is typed beside; wrap: ctx.wrap at n (passed on
   to the operands of + - * and the bit operators and to the left one of a
   left shift, the whole-width rule) */
static int operands(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned wrap, unsigned *lt,
                    unsigned *rt)
{
    int lc = isconst(n->l), rc = isconst(n->r), shift = is_shift(n->op), bit = is_bit(n->op) && !shift;
    unsigned lw = wb_modular(ETN_BIN, n->op) || n->op == ETO_LSL || n->op == ETO_ASL ? wrap : 0;
    unsigned rw = wb_modular(ETN_BIN, n->op) ? wrap : 0;

    if (lc && !rc) {
        c->bittaker = bit || shift;
        c->wrap = rw;
        *rt = typed(c, n->r, want, 0, BIT_NONE);
        if (c->refused)
            return 1;
        c->bittaker = bit || shift;
        c->wrap = lw;
        *lt = typed(c, n->l, want, shift ? 0 : *rt, shift ? BIT_SHIFTED : bit ? BIT_OPERAND : BIT_NONE);
        return c->refused;
    }
    c->bittaker = bit || shift;
    c->wrap = lw;
    *lt = typed(c, n->l, want, 0, shift && lc ? BIT_SHIFTED : BIT_NONE);
    if (c->refused)
        return 1;
    c->bittaker = bit || shift;
    c->wrap = rw;
    *rt = typed(c, n->r, want, rc ? *lt : 0, rc ? (shift ? BIT_COUNT : bit ? BIT_OPERAND : BIT_NONE) : BIT_NONE);
    return c->refused;
}

static unsigned typed_compare(struct ctx *c, struct exprtyping_node *n)
{
    unsigned lt, rt, T;
    int save = c->cmplong, sq = c->quickhere;

    c->cmplong = 1;
    if (is_floaty(c, n, 0)) {
        /* part 8: the comparison's common format is Float: it reaches both
           operands (a fractional side computed and taken as a Float) */
        struct exprtyping_node *o[2];
        unsigned *t[2];
        int i;

        o[0] = n->l;
        o[1] = n->r;
        t[0] = &lt;
        t[1] = &rt;
        for (i = 0; i < 2 && !c->refused; i++) {
            c->quickhere = has_quick(o[i]);
            c->bittaker = 0;
            if (!isconst(o[i]) && has_quick(o[i]) && !is_floaty(c, o[i], 0))
                *t[i] = frac_operand(c, o[i], 0, 1);
            else {
                int sf = c->fmtfloat;

                c->fmtfloat = 1;
                *t[i] = typed(c, o[i], ET_FLOAT, isconst(o[i]) ? ET_FLOAT : 0, BIT_NONE);
                c->fmtfloat = sf;
            }
            c->quickhere = sq;
        }
    } else if (isconst(n->l) && !isconst(n->r)) {
        c->bittaker = 0;
        c->quickhere = has_quick(n->r);
        rt = typed(c, n->r, 0, 0, BIT_NONE);
        if (!c->refused) {
            c->quickhere = 0;
            c->bittaker = 0;
            lt = typed(c, n->l, 0, rt, BIT_NONE);
        }
    } else {
        c->bittaker = 0;
        c->quickhere = has_quick(n->l);
        lt = typed(c, n->l, 0, 0, BIT_NONE);
        if (!c->refused) {
            c->quickhere = has_quick(n->r);
            c->bittaker = 0;
            rt = typed(c, n->r, 0, isconst(n->r) ? lt : 0, BIT_NONE);
        }
    }
    c->quickhere = sq;
    c->cmplong = save;
    if (c->refused)
        return 0;
    if (lt == ET_STRING || rt == ET_STRING)
        return refuse(c);
    if ((lt == ET_LONG && rt == ET_QUICK) || (lt == ET_QUICK && rt == ET_LONG)) {
        record(c, n, ETY_EXACT, isconst(n->l) ? ETY_CONST : lt, lt, 0, isconst(n->r) ? ETY_CONST : rt, rt, 0,
               ET_WORD);
    } else {
        T = tmax(lt, rt);
        if (T > ET_FLOAT)
            return refuse(c);
        record(c, n, ETY_COMPARE, isconst(n->l) ? ETY_CONST : lt, T, 0, isconst(n->r) ? ETY_CONST : rt, T, 0,
               ET_WORD);
    }
    mark(c, n, ET_WORD, ETF_TYPE);
    return ET_WORD;
}

/* ------------------------------------------- the whole-width rule */

/* The whole-number operations of type-system-rework (task 4.1; spec delta
   compiler/expression-types, "A whole-number operation is one step wider
   than its operands", "Optimizations keep the value"; design.md
   "Decisions" 2 and 3): a node of whole class (exprtyping_classify: cls a
   Byte, Word or Long, no exception) gives the value of its class, the
   exact value of + - * and the unary minus up to a Word operand, modulo
   2^32 for a Long.  The type the operand typing chose (today's, the
   wanted one among them) stays where its code gives that value for every
   operand value: under a destination that wraps at n bits through + - *,
   the unary minus, NOT and the bit operators (their low n bits depend on
   the operands' low n bits only, design "Narrow whole code"; a / of two
   Byte or Word operands in its Byte or Word too: DIVS.W keeps the
   dividend on its one overflow, -32768 / -1, whose low 16 bits are the
   quotient's), or where the operands' value range fits the code (a range
   bound: a Word add of values of at most 15 bits, a 16.16 or Float
   operation within its exact range, MULS.W of two Words).  Elsewhere the
   operation is computed in the narrowest whole type whose code holds the
   value: the operands extended (ext) and the Long code, a / of two values
   in the Word range by #1179's inline DIVS.W with the fall-back for
   -32768 / -1 (o.fast).  Mod keeps its type (task 4.7: its sign rule and
   width), its operands get their exact values. */

/* the signed width of v: the least k with -2^(k-1) <= v < 2^(k-1) */
static unsigned wb_vbits(int64_t v)
{
    unsigned k = 1;

    while (k < 63 && (v < -((int64_t)1 << (k - 1)) || v >= ((int64_t)1 << (k - 1))))
        k++;
    return k;
}

static unsigned wb_need(const struct exprtyping_node *n);

/* the signed width of n's value as an operand: at most 32 (a Long wraps) */
static unsigned wb_bits(const struct exprtyping_node *n)
{
    unsigned b = wb_need(n);

    return b > 32 ? 32 : b;
}

/* the signed width the exact value of n's operation needs (beyond 32: a
   Long that wraps); a value with a fraction as its whole part, the
   classes that are no whole number 32 */
static unsigned wb_need(const struct exprtyping_node *n)
{
    unsigned l, r;

    if (folded(n)) {
        struct rq a = const_value(n);

        return a.ok ? wb_vbits(rq_floor(a)) : 32;
    }
    if (n->cls == ET_QUICK && n->kind != ETN_VAR)
        /* a 16.16 value (rule P+ until task 4.2): its whole part */
        return n->typed && n->fmt != ETF_Q48 && n->t == ET_QUICK ? 16 : 32;
    switch (n->kind) {
    case ETN_VAR:
        switch (n->type) {
        case ET_BYTE: return 8;
        case ET_WORD:
        case ET_QUICK: return 16;  /* a Quick as a whole number: rounded down */
        default: return 32;
        }
    case ETN_NEG: return wb_bits(n->l) + 1;
    case ETN_NOT: return wb_bits(n->l);
    case ETN_BIN: break;
    default: return 32;
    }
    if (exprtype_iscompare(n->op) || n->op == ETO_BITTST)
        return 1;           /* -1 or 0 */
    if (is_shift(n->op) && (n->cls == ET_BYTE || n->cls == ET_WORD))
        return n->cls == ET_BYTE ? 8 : 16;  /* LSR, ASR in the left operand's width (task 4.4) */
    if (n->cls == ET_FLOAT || is_shift(n->op) || n->op == ETO_POW || n->op > ETO_BITTST)
        return 32;
    l = wb_bits(n->l);
    r = wb_bits(n->r);
    switch (n->op) {
    case ETO_ADD:
    case ETO_SUB: return tmax(l, r) + 1;
    case ETO_MUL: return l + r;
    case ETO_DIV:
        if (folded(n->r)) {
            struct rq d = const_value(n->r);

            if (d.ok && rq_isint(d) && (d.n >= 2 || d.n <= -2))
                return l;   /* a divisor of at least 2: no wider than the dividend */
        }
        return l + 1;       /* -32768 / -1 */
    case ETO_MOD: return tmax(l, r) + 1;    /* today's sign rule adds the divisor once */
    default: return tmax(l, r);             /* AND, OR, EOR */
    }
}

/* the operation keeps the low n bits of its operands' low n bits (+ - *,
   the unary minus, NOT, the bit operators; kind ETN_NEG, ETN_NOT or a
   BIN's op) */
static int wb_modular(unsigned kind, unsigned op)
{
    if (kind != ETN_BIN)
        return 1;
    return op == ETO_ADD || op == ETO_SUB || op == ETO_MUL || op == ETO_AND || op == ETO_OR || op == ETO_EOR
           || op == ETO_AMP || op == ETO_BAR;
}

static unsigned wb_width(unsigned t)
{
    return t == ET_BYTE ? 8 : t == ET_WORD ? 16 : t == ET_LONG || t == ET_ADDR ? 32 : 0;
}

/* the code of the operation in type t (today's handler; kind and op as
   wb_modular) takes operands of up to *in bits exactly and holds its
   result exactly up to the bits returned (64: modulo 2^32, as the class of
   a Long; 0: never, a / with its fraction) */
static unsigned wb_cap(unsigned kind, unsigned op, unsigned t, unsigned *in)
{
    int bin = kind == ETN_BIN;

    switch (t) {
    case ET_BYTE:
        *in = 8;
        return bin && (op == ETO_ADD || op == ETO_MUL) ? 16 : 8;   /* add.w, muls.w: now a Word */
    case ET_WORD:
        *in = 16;
        return bin && op == ETO_MUL ? 32 : 16;                      /* muls.w: now a Long */
    case ET_LONG:
    case ET_ADDR: *in = 32; return 64;
    case ET_QUICK: *in = 16; return bin && op == ETO_DIV ? 0 : 16;
    case ET_FLOAT:
        *in = 24;           /* the FFP mantissa */
        return bin && op == ETO_DIV ? 0 : 24;
    default: *in = 0; return 0;
    }
}

static int wb_exact(unsigned kind, unsigned op, unsigned t, unsigned lb, unsigned rb, unsigned need)
{
    unsigned in, cap = wb_cap(kind, op, t, &in);

    return lb <= in && rb <= in && need <= cap;
}

/* the type the whole-class node n (kind, op) is computed in: T, the
   operand typing's, where its code gives the class value, else the
   narrowest whole type at least its operands' (lt, rt; rt 0 for a unary
   one) that does; wrap as ctx.wrap at n; *fast: a / of two values in the
   Word range by the inline DIVS.W */
static unsigned wb_type(const struct ctx *c, const struct exprtyping_node *n, unsigned T, unsigned lt,
                        unsigned rt, unsigned wrap, int *fast)
{
    unsigned kind = n->kind, op = kind == ETN_BIN ? n->op : 0, lb, rb = 0, need, t, base;

    *fast = 0;
    if (!c->tsr || c->dry || n->exc || n->cls < ET_BYTE || n->cls > ET_LONG || op == ETO_MOD
        || is_shift(op) || (op >= ETO_BITTST && kind == ETN_BIN))
        return T;
    lb = wb_bits(n->l);
    if (kind == ETN_BIN)
        rb = wb_bits(n->r);
    need = wb_need(n);
    if (wrap && wb_width(T) >= wrap) {
        unsigned in;

        if (wb_modular(kind, op))
            return T;
        if (op == ETO_DIV && T <= ET_WORD && (wb_cap(kind, op, T, &in), lb <= in && rb <= in))
            return T;
    }
    if (wb_exact(kind, op, T, lb, rb, need))
        return T;
    /* the narrowest whole type of the operands (a Quick's whole part a
       Word, a Float's a Long) that holds the value */
    base = tmax(lt, rt);
    base = base == ET_QUICK ? ET_WORD : base >= ET_FLOAT || base == 0 ? ET_LONG : base;
    for (t = base; t < ET_LONG && !wb_exact(kind, op, t, lb, rb, need); t++)
        ;
    if (t == ET_LONG && op == ETO_DIV && lb <= 16 && rb <= 16)
        *fast = 1;
    return t;
}

/* a whole type of the build's (a Quick or Float as the Long of its whole
   part; 0 none) */
static unsigned whole_of(unsigned t)
{
    return (t >= ET_BYTE && t <= ET_LONG) || t == ET_ADDR ? t : t ? ET_LONG : 0;
}

/* A shift of whole class under the classes (task 4.4; spec delta "A shift
   keeps the type of its left operand"), its operands typed (lt, rt): LSR of
   a Byte or Word class in its own width (ETY_SHIFT, the pass's lsr.b or
   lsr.w), every other one as a Long (the build's handler shifts a Long:
   LSR of a Long class, ASR, whose low bits are those of the shift in the
   operand's width, LSL and ASL); the type of the build's code (the wanted
   one among them when it is whole), a Quick count rounded down by its
   conversion to that type.  The left shift that is the whole expression
   under a Quick destination (J) is the build's 16.16 shift: its low 32
   bits are those of the Long shifted and wrapped to a Quick. */
static unsigned whole_shift(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned lt, unsigned rt)
{
    unsigned op = n->op, T, lw = n->l->cls;
    int lc = isconst(n->l), rc = isconst(n->r);

    if (want == ET_FLOAT || lt == ET_FLOAT || rt == ET_FLOAT || lt == ET_STRING || rt == ET_STRING)
        return refuse(c);
    if (op == ETO_LSR && (lw == ET_BYTE || lw == ET_WORD)) {
        record(c, n, ETY_SHIFT, lc ? ETY_CONST : lt, lw, 0, rc ? ETY_CONST : rt, shift_count(rt), 0, lw);
        mark(c, n, lw, ETF_TYPE);
        return lw;
    }
    if (want == ET_QUICK && (n->exc & ETX_SHIFTWRAP))
        T = ET_QUICK;
    else {
        /* the build's type: the wanted whole one and the left operand's (not
           the count's); a Quick count at least a Word (quicktoword: its
           whole part without the overflow check of a Byte) */
        T = tmax(want >= ET_BYTE && want <= ET_LONG ? want : 0, whole_of(lt));
        if (rt == ET_QUICK && T < ET_WORD)
            T = ET_WORD;
    }
    record(c, n, ETY_PLAIN, lc ? ETY_CONST : lt, T, 0, rc ? ETY_CONST : rt, T, 0, handler_result(op, T));
    mark(c, n, handler_result(op, T), ETF_TYPE);
    return handler_result(op, T);
}

/* n by the operand typing under want, beside an operand of type beside (a
   constant subtree only); role: the constant's BIT_* */
static unsigned typed(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned beside, int role)
{
    unsigned lt, rt, T, op, wrap = c->wrap;
    int lc, rc, fast;

    c->wrap = 0;
    if (c->refused)
        return 0;
    if (folded(n))
        return const_type(c, n, want, beside, role);
    if (want == ET_FLOAT
        && (n->kind == ETN_NOT
            || (n->kind == ETN_BIN && (is_bit(n->op) || (c->fmtfloat && is_shift(n->op)))))) {
        /* parts 5-8: a Float format reached from a comparison or an
           expression with a Float is no wanted Float for a bit operator,
           shift or NOT: typed by its own operands, its result taken as a
           Float.  A bit operator or NOT under a wanted Float (rf.f=b AND
           1) is typed so too: its whole result converts at the end (a
           Float operand is refused) */
        int save = c->fmtfloat;

        /* a constant with a point as an operand stays refused here */
        if (!save && (has_point(n->l) || (n->kind == ETN_BIN && has_point(n->r))))
            return refuse(c);
        c->fmtfloat = 0;
        T = typed(c, n, 0, beside, role);
        c->fmtfloat = save;
        if (c->refused || T == ET_STRING)
            return refuse(c);
        if (!save)
            return T;           /* a wanted Float: the whole value, converted by its consumer */
        if (!c->dry)
            n->t = ET_FLOAT;
        return ET_FLOAT;
    }
    if (c->tsr && q_shift(n)) {
        /* the classes (task 4.4): a shift of Quick class by frac_shift,
           taken as frac_operand says; under a wanted Float refused, as
           rule P+ (a Float format reached is typed without it, above) */
        if (want == ET_FLOAT)
            return refuse(c);
        return frac_operand(c, n, want, 0);
    }
    if (c->tsr && q48_class(n)) {
        /* the classes (task 4.2): an operation of Quick class in 48.16
           wherever it is (beside a Float taken as a Float, by a bit
           operator, shift or NOT as frac_operand says), unless rule P+
           reads a constant with a point in it as a Float (part 2; the
           reading of constants is task 3.3) */
        unsigned w = want == ET_FLOAT ? 0 : want;

        if (want != ET_STRING && !is_floaty(c, n, c->reachedf && want == ET_FLOAT ? 0 : want))
            return frac_operand(c, n, w, 0);
    } else if (has_quick(n) && (is_cmp(n) || (!c->tsr && (n->kind == ETN_NEG || (n->kind == ETN_BIN && is_arith(n->op)))))) {
        /* part 6, 7: a fractional subtree inside the operand typing */
        if (c->reachedf && want == ET_FLOAT && !is_floaty(c, n, 0))
            return frac_operand(c, n, 0, 1);
        if (want != ET_FLOAT && want != ET_STRING && !is_floaty(c, n, want))
            return frac_operand(c, n, want, 0);
    }
    switch (n->kind) {
    case ETN_VAR:
        mark(c, n, n->type, ETF_TYPE);
        return n->type;
    case ETN_NEG:
        c->wrap = wrap;
        lt = typed(c, n->l, want, beside, BIT_NONE);
        if (c->refused || lt == ET_STRING)
            return refuse(c);
        /* the whole-width rule: one step wider than its operand, the
           operand converted first (wide) */
        T = wb_type(c, n, lt, lt, 0, wrap, &fast);
        if (!c->dry)
            n->wide = (unsigned char)(T != lt);
        mark(c, n, T, ETF_TYPE);
        return T;
    case ETN_NOT:
        c->bittaker = 1;
        c->wrap = wrap;
        lt = typed(c, n->l, want, 0, isconst(n->l) ? BIT_OPERAND : BIT_NONE);
        if (c->refused)
            return 0;
        /* NOT in the higher of want and its operand's type; a Quick under a
           wanted Long rounded down, as today */
        T = lt;
        if (T == ET_QUICK && want == ET_LONG)
            T = ET_LONG;
        else if (want > T && want != ET_STRING)
            T = want;
        if (T >= ET_FLOAT)
            return refuse(c);
        T = wb_type(c, n, T, lt, 0, wrap, &fast);
        mark(c, n, T, ETF_TYPE);
        return T;
    default: break;
    }
    op = n->op;
    if (exprtype_iscompare(op))
        return typed_compare(c, n);
    if (operands(c, n, want, wrap, &lt, &rt))
        return 0;
    if (lt == ET_STRING || rt == ET_STRING)
        return refuse(c);
    lc = isconst(n->l);
    rc = isconst(n->r);
    if (c->tsr && is_shift(op))
        return whole_shift(c, n, want, lt, rt);
    /* a constant too large for a Quick under a wanted Quick is a Long, as
       today, and the operation one under a Long */
    if (want == ET_QUICK
        && ((lc && !rq_fits(const_value(n->l), ET_QUICK)) || (rc && !rq_fits(const_value(n->r), ET_QUICK))))
        want = ET_LONG;
    if (op == ETO_ADD || op == ETO_SUB || op == ETO_MUL || op == ETO_DIV) {
        if (want == ET_FLOAT || lt == ET_FLOAT || rt == ET_FLOAT)
            T = ET_FLOAT;
        else if (want == ET_QUICK)
            T = ET_QUICK;
        else if (lt == ET_QUICK || rt == ET_QUICK) {
            /* a fractional operand of an operation that is no fractional
               one (beside a Float comparison): a Quick under a wanted Long
               or beside a Long by today's routines of #1178 (mixedop:
               mixmult, lqdiv, longdiv), the result a Long */
            if (want == ET_LONG || lt == ET_LONG || rt == ET_LONG) {
                record(c, n, ETY_MIXED, lc ? ETY_CONST : lt, lt == ET_QUICK ? ET_QUICK : ET_LONG, lt == ET_QUICK,
                       rc ? ETY_CONST : rt, rt == ET_QUICK ? ET_QUICK : ET_LONG, rt == ET_QUICK, ET_LONG);
                mark(c, n, ET_LONG, ETF_TYPE);
                return ET_LONG;
            }
            T = ET_QUICK;
        } else {
            T = tmax(want, tmax(lt, rt));
            if (c->cmplong && T < ET_LONG)
                T = ET_LONG;    /* a whole-number operation in a comparison */
        }
    } else if (op == ETO_POW) {
        /* dopow: both to Float, through a higher wanted Quick or whole
           number first, as today */
        unsigned tt = tmax(want, tmax(lt, rt));

        if (tt == ET_QUICK && want != ET_QUICK && (lt == ET_LONG || rt == ET_LONG))
            tt = ET_FLOAT;
        record(c, n, ETY_PLAIN, lc ? ETY_CONST : lt, tmax(lt, tt), 0, rc ? ETY_CONST : rt, tmax(rt, tt), 0,
               ET_FLOAT);
        mark(c, n, ET_FLOAT, ETF_TYPE);
        return ET_FLOAT;
    } else {
        /* the bit operators, Mod and the shifts (a Quick count makes the
           shift a Quick one, as today; under a Long it is rounded down) */
        unsigned rr = rt;

        if (is_shift(op))
            rr = rt == ET_FLOAT ? ET_FLOAT : rt == ET_QUICK && tmax(want, lt) < ET_LONG ? ET_QUICK : 0;
        if (want == ET_FLOAT)
            T = ET_FLOAT;
        else if (want != ET_QUICK && !is_shift(op)
                 && ((lt == ET_LONG && rr == ET_QUICK) || (lt == ET_QUICK && rr == ET_LONG)))
            T = ET_LONG;        /* the Quick rounded down, as a Long and a Quick add */
        else
            T = tmax(want, tmax(lt, rr));
        if (T == ET_QUICK && want == ET_LONG)
            T = ET_LONG;        /* the Quick rounded down, as under today's Long */
        if ((is_bit(op) || is_shift(op)) && T == ET_FLOAT)
            return refuse(c);
        if (op == ETO_MOD && (T == ET_QUICK || T == ET_FLOAT)
            && ((lt >= ET_QUICK && lt <= ET_FLOAT) || (rt >= ET_QUICK && rt <= ET_FLOAT))) {
            /* "expr-tree-5 group 3 findings: four rule points of P+", 2:
               an operand with a fraction makes Mod exact with today's sign
               rule, in the Quick or Float it is computed in (result that
               type, not today's handler's Word or Long) */
            record(c, n, ETY_PLAIN, lc ? ETY_CONST : lt, T, 0, rc ? ETY_CONST : rt, T, 0, T);
            mark(c, n, T, ETF_TYPE);
            return T;
        }
    }
    T = wb_type(c, n, T, lt, rt, wrap, &fast);
    record(c, n, ETY_PLAIN, lc ? ETY_CONST : lt, T, 0, rc ? ETY_CONST : rt, T, 0, handler_result(op, T));
    if (!c->dry)
        n->o.fast = (unsigned char)fast;
    mark(c, n, handler_result(op, T), ETF_TYPE);
    return handler_result(op, T);
}

/* ------------------------------------------------------------ the root */

/* the fast path of a 48.16 operation: both operands narrow, their 48.16
   value the sign extension of a 16.16 one (a Quick, a Byte or Word value,
   a comparison's Word, a constant in the Quick range, a 16.16 result) */
static int narrow(const struct exprtyping_node *n)
{
    if (isconst(n)) {
        struct rq a = const_value(n);

        return a.ok && rq_fits(a, ET_QUICK);
    }
    if (!n->typed)
        return 0;
    if (n->fmt == ETF_Q16)
        return 1;
    if (n->fmt == ETF_Q48)
        return 0;
    return n->t == ET_BYTE || n->t == ET_WORD || n->t == ET_QUICK;
}

static void fastpaths(struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN:
        fastpaths(n->l);
        fastpaths(n->r);
        if (n->typed && n->o.path == ETY_Q48)
            n->o.fast = (unsigned char)(narrow(n->l) && narrow(n->r));
        break;
    case ETN_NEG:
    case ETN_NOT: fastpaths(n->l); break;
    default: break;
    }
}

static void clear(struct exprtyping_node *n)
{
    n->typed = 0;
    n->t = n->fmt = n->own = n->wide = 0;
    n->o.path = 0;
    n->o.fast = 0;
    if (n->kind == ETN_BIN) {
        clear(n->l);
        clear(n->r);
    } else if (n->kind == ETN_NEG || n->kind == ETN_NOT)
        clear(n->l);
}

int exprtyping_type(struct exprtyping_node *root, unsigned want, struct exprtyping_result *res)
{
    struct ctx c = { 0 };
    unsigned t = 0;

    clear(root);
    res->refused = 0;
    res->fmt = ETF_TYPE;
    res->consumer = ETC_TYPE;
    res->conv = 0;
    c.quickhere = has_quick(root);
    c.tsr = root->cls != 0;
    c.top = root;
    if (c.tsr && want != ET_STRING
        && ((q48_class(root) && !is_floaty(&c, root, want)) || (q_shift(root) && want != ET_FLOAT))) {
        /* the classes (task 4.2): an operation of Quick class at the top,
           computed in 48.16 (Mod as rule P+ decides, task 4.7), its value
           taken by the destination; a shift of Quick class (task 4.4) in
           16.16 or 48.16 as frac_shift decides */
        unsigned from, f = frac_value(&c, root, want == ET_FLOAT ? 0 : want, 0, 0, &from);

        res->fmt = f == FW_48 ? ETF_Q48 : ETF_Q16;
        if (c.refused)
            ;
        else if (want == ET_QUICK || !want) {
            t = ET_QUICK;
            res->consumer = f != FW_48 ? ETC_KEEP : want ? ETC_SAT : ETC_FIT;
        } else if (want == ET_FLOAT) {
            t = f == FW_48 ? ET_FLOAT : ET_QUICK;
            res->consumer = f == FW_48 ? ETC_TOFLOAT : ETC_TYPE;
        } else {
            t = ET_LONG;
            res->consumer = ETC_FLOOR;  /* rounded down (spec: the conversion to a whole destination) */
        }
    } else if (has_quick(root) && want != ET_FLOAT && !is_floaty(&c, root, want)
               && !(c.tsr && root->cls != ET_QUICK
                    && (root->kind == ETN_NEG || (root->kind == ETN_BIN && is_arith(root->op))))) {
        /* (the classes: a whole operation at the top is no fractional one) */
        /* an expression with a Quick and no Float: the fractional world,
           rounded once at the consumer */
        int cmp = is_cmp(root);
        int force = !want && !cmp ? has_long(&c, root, 0) : 0;
        unsigned from, f = frac_value(&c, root, want, force, want != ET_LONG && !cmp, &from);

        res->fmt = f == FW_48 ? ETF_Q48 : f == FW_16 ? ETF_Q16 : ETF_TYPE;
        if (c.refused)
            ;
        else if (cmp) {
            t = ET_WORD;
            res->consumer = ETC_BOOL;
        } else if (want == ET_QUICK || (!want && f == FW_16)) {
            t = ET_QUICK;
            res->consumer = ETC_KEEP;
        } else if (!want && f == FW_WHOLE) {
            /* a bit operator, shift or NOT at the top: its whole value (a
               Long beyond the Quick range stays a Long) */
            t = root->t;
            res->consumer = t == ET_LONG ? ETC_FIT : ETC_KEEP;
        } else if (!want) {
            t = ET_QUICK;       /* or a Long, at run time */
            res->consumer = ETC_FIT;
        } else {
            t = ET_LONG;
            /* part 3: a / at the top under a wanted Byte, Word or Long
               towards zero (#1222) */
            res->consumer = root->kind == ETN_BIN && root->op == ETO_DIV && is_wholewant(want) ? ETC_TRUNC : ETC_FLOOR;
        }
    } else if (has_quick(root) && !is_wholewant(want) && want != ET_FLOAT && want != ET_STRING
               && !is_cmp(root) && is_floaty(&c, root, want)) {
        /* part 7: an expression with a Quick and a Float under Float */
        c.reachedf = 1;
        c.fmtfloat = 1;
        t = typed(&c, root, ET_FLOAT, 0, BIT_NONE);
    } else {
        c.wrap = wb_width(want);    /* a whole destination wraps */
        t = typed(&c, root, want, 0, BIT_NONE);
    }
    if (!c.refused && want && t != want) {
        if (t == ET_STRING || want == ET_STRING)
            c.refused = 1;
        else
            res->conv = (unsigned char)want;
    }
    res->refused = (unsigned char)c.refused;
    res->t = (unsigned char)(c.refused ? 0 : t);
    if (!c.refused)
        fastpaths(root);
    return c.refused;
}

/* ===================================== the operand-only classes (TSR) */

/* The classes of the rules of the OpenSpec change type-system-rework (spec
   delta compiler/expression-types, design.md "Decisions" 1 and 2; tasks
   3.1, 3.2): every node typed from its operator and its operands only, a
   whole number one step wider than its operands (the unary minus and NOT
   too, NOT of a Quick a Long), an operation with a Quick 48.16, with a
   Float Float; the destination only at the root and for the named
   exceptions, which reach a node from the root: the whole-division
   exception (C1: at the top or through + - * / and the unary minus under
   a Quick or Float destination), a whole / beside a Quick-class operand
   (C3: at the arithmetic operators, Mod, ^ and the comparisons, also
   through a unary minus; not at the bit operators and shifts, not beside a
   Float), the top operation under a Float destination (D: an arithmetic
   one with a Quick operand, a constant with a point there read as a
   Float), the shift exception (E: a right shift of a whole operand at the
   top or through + - under a Quick destination, with 16 fraction bits) and
   the left-shift wrap (J).  A whole constant is its smallest type (no
   DEFTYPE; a minus before it the unary minus); a constant with a point a
   Quick when it is exact in 16.16 or a Quick is the other operand of its
   operator (also under a unary minus), else a Float.  The oracle is the
   matrix model's rule set EM_RULES_TSR (tools/lib/exprmatrix/model.c,
   tsr_class; programs/blitz2/tests/host/test_exprtyping.cpp compares
   every node). */

enum { CK_W = 1, CK_Q = ET_QUICK, CK_F = ET_FLOAT, CK_P = 6 };   /* CK_P: a constant with a point, not decided */

/* how a node is reached from the root */
enum { CR_ROOT = 1, CR_C1 = 2, CR_E = 4 };

/* what a node is told about the other operand of its operator (else CK_W,
   CK_Q, CK_F: the other operand's class) */
enum { CS_NONE = 0, CS_PROBE = 8, CS_SHIFTL = 9, CS_FTOP = 10 };

struct cls {
    unsigned char k, n;         /* the class; a whole number's width in bits */
    unsigned char refused;
    unsigned char lsib, rsib, lreach, rreach;   /* what the operands are told */
    unsigned char ftop;         /* D applies */
    unsigned char div;          /* a whole / that keeps its fraction beside a Quick when told so */
    unsigned char exc;          /* ETX_*: the exception that gives another type here */
};

static int cls_arith(unsigned op)
{
    return op == ETO_ADD || op == ETO_SUB || op == ETO_MUL || op == ETO_DIV || op == ETO_MOD;
}

static unsigned cls_wider(unsigned n)
{
    return n == 8 ? 16 : 32;
}

/* a constant with a point, also under unary minuses; else NULL */
static const struct exprtyping_node *cls_pc(const struct exprtyping_node *n)
{
    while (n->kind == ETN_NEG)
        n = n->l;
    return n->kind == ETN_CONST && n->point ? n : NULL;
}

/* the literal's value (no sign) is below 32768: it fits a Quick */
static int cls_pcfits(const struct exprtyping_node *n)
{
    int64_t lim;

    return n->num >= 0 && n->den > 0 && !__builtin_mul_overflow(n->den, (int64_t)32768, &lim) && n->num < lim;
}

/* exact in 16.16 and in the Quick range */
static int cls_pcexact(const struct exprtyping_node *n)
{
    int64_t s;

    return cls_pcfits(n) && !__builtin_mul_overflow(n->num, (int64_t)65536, &s) && !(s % n->den);
}

/* the width of a whole constant: the smallest type of its 32 bits */
static unsigned cls_width(const struct exprtyping_node *n)
{
    int64_t v = (int32_t)(uint32_t)(uint64_t)(n->den > 1 ? n->num / n->den : n->num);

    return v >= -128 && v <= 127 ? 8 : v >= -32768 && v <= 32767 ? 16 : 32;
}

static void cls_of(unsigned dest, const struct exprtyping_node *n, unsigned reach, unsigned sib, struct cls *o)
{
    struct cls a, b;
    unsigned op;

    memset(o, 0, sizeof *o);
    switch (n->kind) {
    case ETN_VAR:
        if (n->type == ET_QUICK || n->type == ET_FLOAT)
            o->k = n->type;
        else if (n->type == ET_BYTE || n->type == ET_WORD || n->type == ET_LONG) {
            o->k = CK_W;
            o->n = n->type == ET_BYTE ? 8 : n->type == ET_WORD ? 16 : 32;
        } else
            o->refused = 1;
        return;
    case ETN_CONST:
        if (!n->point) {
            o->k = CK_W;
            o->n = (unsigned char)cls_width(n);
        } else if (sib == CS_FTOP)
            o->k = CK_F;        /* D: the Float of the written value */
        else if (cls_pcexact(n) || (sib == CK_Q && cls_pcfits(n)))
            o->k = CK_Q;
        else
            o->k = sib == CS_PROBE ? CK_P : CK_F;
        return;
    case ETN_NEG:
        /* C1 reaches through it, D and E do not; a constant with a point
           and a whole / under it are read as without it (C3 too) */
        o->lreach = (unsigned char)(reach & CR_C1);
        o->lsib = (unsigned char)(cls_pc(n->l) || sib == CK_Q ? sib : CS_NONE);
        cls_of(dest, n->l, o->lreach, o->lsib, &a);
        o->refused = a.refused;
        o->k = a.k;
        o->div = a.div;
        if (a.k == CK_W)
            o->n = (unsigned char)cls_wider(a.n);
        return;
    case ETN_NOT:
        /* a whole operand, a Quick rounded down; a Float refused; one step
           wider, of a Quick a Long */
        cls_of(dest, n->l, 0, CS_NONE, &a);
        o->refused = a.refused || a.k == CK_F;
        o->k = CK_W;
        o->n = (unsigned char)(a.k == CK_W ? cls_wider(a.n) : 32);
        return;
    default: break;
    }
    op = n->op;
    if (is_shift(op)) {
        const struct exprtyping_node *c = n->r;
        int left = op == ETO_LSL || op == ETO_ASL;

        /* the type of its left operand; the count whole, a constant with a
           point refused */
        o->lsib = CS_SHIFTL;
        cls_of(dest, n->l, 0, CS_SHIFTL, &a);
        cls_of(dest, c, 0, CS_NONE, &b);
        while (c->kind == ETN_NEG)
            c = c->l;
        o->refused = a.refused || b.refused || a.k == CK_F || b.k == CK_F || (c->kind == ETN_CONST && c->point);
        if (a.k == CK_Q) {
            o->k = CK_Q;
            if (left && dest == ET_QUICK && (reach & CR_ROOT))
                o->exc = ETX_SHIFTWRAP;         /* J of a Quick (I6 B) */
        } else if (!left && dest == ET_QUICK && (reach & CR_E)) {
            o->k = CK_Q;                        /* the shift exception (E) */
            o->exc = ETX_SHIFTFRAC;
        } else if (left) {
            o->k = CK_W;
            o->n = 32;
            if (dest == ET_QUICK && (reach & CR_ROOT))
                o->exc = ETX_SHIFTWRAP;         /* J (I5 A) */
        } else {
            o->k = CK_W;
            o->n = a.n;
        }
        return;
    }
    {
        unsigned r = 0, ka, kb;
        int bit = is_bit(op);

        if ((reach & CR_C1) && cls_arith(op) && op != ETO_MOD)
            r |= CR_C1;
        if ((reach & CR_E) && (op == ETO_ADD || op == ETO_SUB))
            r |= CR_E;
        o->lreach = o->rreach = (unsigned char)r;
        cls_of(dest, n->l, r, CS_PROBE, &a);
        cls_of(dest, n->r, r, CS_PROBE, &b);
        o->refused = a.refused || b.refused;
        ka = a.k;
        kb = b.k;
        /* a constant with a point beside a Quick-class operand a Quick */
        if (ka == CK_P)
            ka = kb == CK_Q && cls_pcfits(cls_pc(n->l)) ? CK_Q : CK_F;
        if (kb == CK_P)
            kb = ka == CK_Q && cls_pcfits(cls_pc(n->r)) ? CK_Q : CK_F;
        /* C3: a whole / beside a Quick-class operand, not at a bit operator */
        if (!bit) {
            if (a.div && kb == CK_Q)
                ka = CK_Q;
            if (b.div && ka == CK_Q)
                kb = CK_Q;
        }
        o->lsib = (unsigned char)kb;
        o->rsib = (unsigned char)ka;
        if (a.div && ka == CK_W)
            o->lsib = CS_NONE;
        if (b.div && kb == CK_W)
            o->rsib = CS_NONE;
        /* D: the arithmetic top operation with a Quick under a Float */
        if (dest == ET_FLOAT && (reach & CR_ROOT) && (cls_arith(op) || op == ETO_POW) && (ka == CK_Q || kb == CK_Q)) {
            o->ftop = 1;
            if (op == ETO_POW ? (cls_pc(n->l) && ka == CK_Q) || (cls_pc(n->r) && kb == CK_Q)
                              : ka != CK_F && kb != CK_F)
                o->exc = ETX_FLOATTOP;
            if (cls_pc(n->l))
                o->lsib = CS_FTOP;
            if (cls_pc(n->r))
                o->rsib = CS_FTOP;
        }
        if (op == ETO_POW) {
            o->k = CK_F;
            return;
        }
        if (exprtype_iscompare(op)) {
            o->k = CK_W;        /* the boolean a Word */
            o->n = 16;
            return;
        }
        if (bit) {
            o->refused |= ka == CK_F || kb == CK_F;
            o->k = CK_W;
            o->n = (unsigned char)cls_wider(tmax(ka == CK_W ? a.n : 32, kb == CK_W ? b.n : 32));
            return;
        }
        if (o->ftop || ka == CK_F || kb == CK_F)
            o->k = CK_F;
        else if (ka == CK_Q || kb == CK_Q)
            o->k = CK_Q;
        else if (op == ETO_DIV && (reach & CR_C1) && (dest == ET_QUICK || dest == ET_FLOAT)) {
            o->k = (unsigned char)dest; /* the whole-division exception (C1) */
            o->exc = ETX_DIVFRAC;
        } else if (op == ETO_DIV && sib == CK_Q) {
            o->k = CK_Q;                /* C3, told by the parent */
            o->exc = ETX_DIVQUICK;
        } else {
            o->k = CK_W;
            o->n = (unsigned char)cls_wider(tmax(a.n, b.n));
            o->div = op == ETO_DIV;
        }
    }
}

/* the class of every node of n into n->cls, n->exc; *warn the exceptions */
static int cls_mark(unsigned dest, struct exprtyping_node *n, unsigned reach, unsigned sib, unsigned *warn)
{
    struct cls c;

    cls_of(dest, n, reach, sib, &c);
    if (c.refused)
        return 1;
    switch (n->kind) {
    case ETN_NEG:
        if (cls_mark(dest, n->l, c.lreach, c.lsib, warn))
            return 1;
        break;
    case ETN_NOT:
        if (cls_mark(dest, n->l, 0, CS_NONE, warn))
            return 1;
        break;
    case ETN_BIN:
        if (is_shift(n->op)) {
            if (cls_mark(dest, n->l, 0, CS_SHIFTL, warn) || cls_mark(dest, n->r, 0, CS_NONE, warn))
                return 1;
        } else if (cls_mark(dest, n->l, c.lreach, c.lsib, warn) || cls_mark(dest, n->r, c.rreach, c.rsib, warn))
            return 1;
        break;
    default: break;
    }
    n->cls = (unsigned char)(c.k == CK_W ? (c.n == 8 ? ET_BYTE : c.n == 16 ? ET_WORD : ET_LONG) : c.k);
    n->exc = c.exc;
    *warn |= c.exc;
    return 0;
}

int exprtyping_classify(struct exprtyping_node *root, unsigned want, unsigned *warn)
{
    *warn = 0;
    if (cls_mark(want, root, CR_ROOT | CR_C1 | CR_E, CS_NONE, warn)) {
        *warn = 0;
        return 1;
    }
    return 0;
}
