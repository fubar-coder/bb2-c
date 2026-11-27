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
   (point: written with a point) under the type so far t */
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
        mark(c, n, f ? ET_QUICK : lf, f == FW_48 ? ETF_Q48 : f ? ETF_Q16 : ETF_TYPE);
        return f;
    case ETN_NOT: return frac_other(c, n, want);
    default: break;
    }
    if (!exprtype_iscompare(n->op) && !is_arith(n->op))
        return frac_other(c, n, want);
    if (exprtype_iscompare(n->op)) {
        /* part 8: both sides with no wanted type, each in its own format,
           compared exactly; a Long in either side makes both 48.16 */
        int save = c->cmplong, sq = c->quickhere, fc;

        c->cmplong = 1;
        fc = has_long(c, n->l, 0) || has_long(c, n->r, 0);
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
    if (want >= ET_BYTE && want <= ET_QUICK && has_long(c, n, want))
        force = 1;
    frac_side(c, n->l, want, force, whole, &lf);
    frac_side(c, n->r, want, force, whole, &rf);
    f = force || want == ET_LONG ? FW_48 : FW_16;
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
   part, a Long (rounded down), anywhere else straight as a Float; a
   comparison as its Word */
static unsigned frac_operand(struct ctx *c, struct exprtyping_node *n, unsigned want, int whole)
{
    unsigned from, f, t;
    int force = !want && !is_cmp(n) ? has_long(c, n, 0) : 0, bit = c->bittaker;

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

/* the operands of the binary n (no comparison): a constant after the
   other operand, which it is typed beside */
static int operands(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned *lt, unsigned *rt)
{
    int lc = isconst(n->l), rc = isconst(n->r), shift = is_shift(n->op), bit = is_bit(n->op) && !shift;

    if (lc && !rc) {
        c->bittaker = bit || shift;
        *rt = typed(c, n->r, want, 0, BIT_NONE);
        if (c->refused)
            return 1;
        c->bittaker = bit || shift;
        *lt = typed(c, n->l, want, shift ? 0 : *rt, shift ? BIT_SHIFTED : bit ? BIT_OPERAND : BIT_NONE);
        return c->refused;
    }
    c->bittaker = bit || shift;
    *lt = typed(c, n->l, want, 0, shift && lc ? BIT_SHIFTED : BIT_NONE);
    if (c->refused)
        return 1;
    c->bittaker = bit || shift;
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

/* n by the operand typing under want, beside an operand of type beside (a
   constant subtree only); role: the constant's BIT_* */
static unsigned typed(struct ctx *c, struct exprtyping_node *n, unsigned want, unsigned beside, int role)
{
    unsigned lt, rt, T, op;
    int lc, rc;

    if (c->refused)
        return 0;
    if (folded(n))
        return const_type(c, n, want, beside, role);
    if (want == ET_FLOAT && c->fmtfloat
        && (n->kind == ETN_NOT || (n->kind == ETN_BIN && (is_bit(n->op) || is_shift(n->op))))) {
        /* parts 5-8: a Float format reached from a comparison or an
           expression with a Float is no wanted Float for a bit operator,
           shift or NOT: typed by its own operands, its result taken as a
           Float (a wanted Float, rf.f=b AND 1, stays refused) */
        int save = c->fmtfloat;

        c->fmtfloat = 0;
        T = typed(c, n, 0, beside, role);
        c->fmtfloat = save;
        if (c->refused || T == ET_STRING)
            return refuse(c);
        if (!c->dry)
            n->t = ET_FLOAT;
        return ET_FLOAT;
    }
    if (has_quick(n) && (n->kind == ETN_NEG || (n->kind == ETN_BIN && (is_arith(n->op) || exprtype_iscompare(n->op))))) {
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
        T = typed(c, n->l, want, beside, BIT_NONE);
        if (c->refused || T == ET_STRING)
            return refuse(c);
        mark(c, n, T, ETF_TYPE);
        return T;
    case ETN_NOT:
        c->bittaker = 1;
        T = typed(c, n->l, want, 0, isconst(n->l) ? BIT_OPERAND : BIT_NONE);
        if (c->refused)
            return 0;
        /* NOT in the higher of want and its operand's type; a Quick under a
           wanted Long rounded down, as today */
        if (T == ET_QUICK && want == ET_LONG)
            T = ET_LONG;
        else if (want > T && want != ET_STRING)
            T = want;
        if (T >= ET_FLOAT)
            return refuse(c);
        mark(c, n, T, ETF_TYPE);
        return T;
    default: break;
    }
    op = n->op;
    if (exprtype_iscompare(op))
        return typed_compare(c, n);
    if (operands(c, n, want, &lt, &rt))
        return 0;
    if (lt == ET_STRING || rt == ET_STRING)
        return refuse(c);
    lc = isconst(n->l);
    rc = isconst(n->r);
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
    record(c, n, ETY_PLAIN, lc ? ETY_CONST : lt, T, 0, rc ? ETY_CONST : rt, T, 0, handler_result(op, T));
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
    n->t = n->fmt = n->own = 0;
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
    if (has_quick(root) && want != ET_FLOAT && !is_floaty(&c, root, want)) {
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
    } else
        t = typed(&c, root, want, 0, BIT_NONE);
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
