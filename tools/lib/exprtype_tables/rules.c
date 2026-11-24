/*
 * rules.c - today's rules of the type of an expression as a rule set
 * (exprtype_tables.h): the primitives of exprtype.c, and the flow of the
 * type so far through them for one operation of two values.
 */
#include <string.h>
#include "exprtype_tables.h"

static int x_cutsraw(void *ctx, unsigned op, int *out)
{
    (void)ctx;
    *out = exprtype_cutsraw(op);
    return 0;
}

static int x_leftop(void *ctx, unsigned op, unsigned left, int lraw, struct exprtype_left *out)
{
    (void)ctx;
    *out = exprtype_leftop(op, left, lraw);
    return 0;
}

static int x_ownleft(void *ctx, unsigned from, unsigned to, int *out)
{
    (void)ctx;
    *out = exprtype_ownleft(from, to);
    return 0;
}

static int x_binop(void *ctx, unsigned op, unsigned left, unsigned right, const struct exprtype_binfacts *f,
                   struct exprtype_bin *out)
{
    (void)ctx;
    *out = exprtype_binop(op, left, right, f);
    return 0;
}

static int x_element(void *ctx, unsigned tsf, unsigned own, const struct exprtype_constfacts *c,
                     struct exprtype_elem *out)
{
    (void)ctx;
    *out = exprtype_element(tsf, own, c);
    return 0;
}

void ett_prims_exprtype(struct ett_prims *p)
{
    p->ctx = NULL;
    p->cutsraw = x_cutsraw;
    p->leftop = x_leftop;
    p->ownleft = x_ownleft;
    p->binop = x_binop;
    p->element = x_element;
}

/* the operation op as the evaluator builds it (eval.c: contleaf for the
   left value, binopbuild's raw cut, exprtree_pre with exprtype_leftop and
   ownleft, exprtree_enter's type so far of the right operand, contleaf
   for it, binopbuild with exprtype_binop): the left value is met at the
   type so far tsf, the right one at the type so far after the left (none
   under a comparison) */
static int flow_cell(void *ctx, unsigned op, unsigned tsf, unsigned l, unsigned r, const struct ett_facts *x,
                     struct ett_cell *c)
{
    const struct ett_prims *p = ctx;
    struct ett_facts none = { 0 };
    struct exprtype_elem el, er;
    struct exprtype_left pre;
    struct exprtype_binfacts f;
    struct exprtype_bin d;
    int cut, lraw, rraw = 0, own;
    unsigned left, right;

    if (!x)
        x = &none;
    memset(c, 0, sizeof *c);
    memset(&f, 0, sizeof f);
    if (p->element(p->ctx, tsf, l, NULL, &el) || p->cutsraw(p->ctx, op, &cut))
        return -1;
    /* the left value: a raw Quick whose consumer is op is rounded down
       at once (exprtype_cutsraw) */
    c->l.as = el.result;
    lraw = el.raw && !cut;
    if (lraw) {
        c->l.as = ET_QUICK;
        c->l.how = ETT_RAW;
    }
    if (p->leftop(p->ctx, op, el.result, lraw, &pre))
        return -1;
    left = pre.left;
    if (lraw && !pre.lraw) {
        c->l.as = (unsigned char)left;  /* a comparison: the Quick it is */
        c->l.how = ETT_KEEP;
    }
    if (pre.ownleft && el.conv_from) {
        if (p->ownleft(p->ctx, el.conv_from, el.conv_to, &own))
            return -1;
        if (own) {
            left = el.conv_from;        /* ownleft: the conversion dropped */
            c->l.as = (unsigned char)left;
        }
    }
    /* the right operand */
    memset(&er, 0, sizeof er);
    if (x->rcmp)
        right = ET_WORD;                /* the boolean (cpoke) */
    else if (x->byte978) {
        struct exprtype_constfacts k = { 0 };

        k.size = ET_WORD;               /* 128 to 255 */
        if (p->element(p->ctx, pre.compare ? 0 : left, 0, &k, &er))
            return -1;
        right = er.result;
    } else {
        if (p->element(p->ctx, pre.compare ? 0 : left, r, NULL, &er))
            return -1;
        right = er.result;
        rraw = er.raw && !cut;
    }
    c->r.as = (unsigned char)right;
    if (rraw) {
        c->r.as = ET_QUICK;
        c->r.how = ETT_RAW;
    }
    if (!right) {
        c->result = 0;                  /* "Type Mismatch" */
        return 0;
    }
    f.lraw = pre.lraw;
    f.rraw = (unsigned char)rraw;
    f.rcmp = x->rcmp;
    f.words = x->words && el.wordval && er.wordval;
    if (x->byte978) {
        f.lbyte = l == ET_BYTE;
        f.rconst = 1;
    }
    if (p->binop(p->ctx, op, left, right, &f, &d))
        return -1;
    c->result = d.result;
    c->wordop = d.wordop;
    c->byteext = d.byteext;
    if (d.rup_from)
        c->r.as = d.rup_to;
    switch (d.path) {
    case ETP_CMPLQ:
        c->exact = 1;
        break;
    case ETP_PLAIN:
    case ETP_COMPARE:
        if (d.lconv_from)
            c->l.as = d.lconv_to;
        break;
    default:                            /* ETP_MIXED */
        if (d.lconv_from) {
            c->l.as = d.lconv_to;
            c->l.how = ETT_KEEP;
        }
        if (d.rconv_from) {
            c->r.as = d.rconv_to;
            c->r.how = ETT_KEEP;
        }
        if (d.lcut) {
            c->l.as = ET_LONG;
            c->l.how = ETT_KEEP;
        }
        if (d.rcut) {
            c->r.as = ET_LONG;
            c->r.how = ETT_KEEP;
        }
        if (!d.handler && d.result == ET_LONG) {
            /* inline, exact, rounded at the operator */
            c->l.as = d.lq ? ET_QUICK : ET_LONG;
            c->l.how = d.lq ? ETT_RAW : ETT_KEEP;
            c->r.as = d.rq ? ET_QUICK : ET_LONG;
            c->r.how = d.rq ? ETT_RAW : ETT_KEEP;
        }
        break;
    }
    return 0;
}

static int flow_element(void *ctx, unsigned tsf, unsigned own, struct ett_elem *out)
{
    const struct ett_prims *p = ctx;
    struct exprtype_elem e;

    if (p->element(p->ctx, tsf, own, NULL, &e))
        return -1;
    out->result = e.result;
    out->conv = e.conv_from != 0;
    out->raw = e.raw;
    out->litcut = 0;
    return 0;
}

static int flow_constant(void *ctx, unsigned tsf, const struct exprtype_constfacts *k, struct ett_elem *out)
{
    const struct ett_prims *p = ctx;
    struct exprtype_elem e;

    if (p->element(p->ctx, tsf, 0, k, &e))
        return -1;
    out->result = e.result;
    out->conv = 0;
    out->raw = 0;
    out->litcut = e.litcut;
    return 0;
}

void ett_rules_flow(struct ett_rules *r, const struct ett_prims *p)
{
    r->name = "today";
    r->ctx = (void *)p;
    r->cell = flow_cell;
    r->element = flow_element;
    r->constant = flow_constant;
}
