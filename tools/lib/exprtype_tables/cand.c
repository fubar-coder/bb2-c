/*
 * cand.c - a candidate rule set of the decision round of
 * expr-tree-5-operand-typing (open question 1) as a rule set of the
 * tables (exprtype_tables.h, ett_rules_cand): the operations from the
 * expression matrix's model (tools/lib/exprmatrix, em_cand_cell).
 */
#include <string.h>
#include "exprtype_tables.h"
#include "exprmatrix.h"
#include "exprtyping.h"

/* the matrix's operator of an ETO_* (-1: the model has none) */
static int emop(unsigned op)
{
    switch (op) {
    case ETO_ADD: return EM_ADD;
    case ETO_SUB: return EM_SUB;
    case ETO_MUL: return EM_MUL;
    case ETO_DIV: return EM_DIV;
    case ETO_AMP:
    case ETO_AND: return EM_AND;
    case ETO_BAR:
    case ETO_OR: return EM_OR;
    case ETO_EOR: return EM_EOR;
    case ETO_LSL: return EM_LSL;
    case ETO_ASL: return EM_ASL;
    case ETO_LSR: return EM_LSR;
    case ETO_ASR: return EM_ASR;
    case ETO_MOD: return EM_MOD;
    case ETO_POW: return EM_POW;
    case ETO_EQ: return EM_EQ;
    case ETO_NE: return EM_NE;
    case ETO_LT: return EM_LT;
    case ETO_LE: return EM_LE;
    case ETO_GT: return EM_GT;
    case ETO_GE: return EM_GE;
    default: return -1;
    }
}

static int c_cell(void *ctx, unsigned op, unsigned tsf, unsigned l, unsigned r, const struct ett_facts *x,
                  struct ett_cell *out)
{
    const struct ett_cand *c = ctx;
    struct em_optype t;
    int o = emop(op);

    /* by rule P+ a comparison's result is a whole number, a Word (EM_R_PCMPW):
       the operation with it is that of a Word operand */
    if (x && x->rcmp && !x->words && !x->byte978 && (c->rules & EM_R_PCMPW))
        x = NULL, r = ET_WORD;
    if (o < 0 || (x && (x->rcmp || x->words || x->byte978)))
        return c->base->cell(c->base->ctx, op, tsf, l, r, x, out);
    memset(out, 0, sizeof *out);
    em_cand_cell(c->rules, (enum em_op)o, (int)tsf, (int)l, (int)r, &t);
    if (t.op < 0 || !t.result) {
        out->result = 0;
        return 0;
    }
    out->result = (unsigned char)t.result;
    out->l.as = (unsigned char)t.las;
    out->l.how = t.lraw ? ETT_RAW : ETT_KEEP;
    out->r.as = (unsigned char)t.ras;
    out->r.how = t.rraw ? ETT_RAW : ETT_KEEP;
    out->exact = t.path == 'x';
    out->wide = t.path == '3';
    return 0;
}

/* a value keeps its own type until its operation converts it */
static int c_element(void *ctx, unsigned tsf, unsigned own, struct ett_elem *out)
{
    (void)ctx;
    (void)tsf;
    memset(out, 0, sizeof *out);
    out->result = (unsigned char)own;
    return 0;
}

/* a constant alone: as today (consttype2 under the wanted type; the
   typing pass of rule P+ types operations, a constant alone keeps the
   compiler's flow) */
static int c_constant(void *ctx, unsigned tsf, const struct exprtype_constfacts *k, struct ett_elem *out)
{
    const struct ett_cand *c = ctx;

    return c->base->constant(c->base->ctx, tsf, k, out);
}

void ett_rules_cand(struct ett_rules *r, struct ett_cand *c)
{
    r->name = em_cand_name((int)((c->rules & EM_R_CANDMASK) >> 9));
    r->ctx = c;
    r->cell = c_cell;
    r->element = c_element;
    r->constant = c_constant;
}

int ett_pplus_beside(unsigned want, unsigned partner, const char *text)
{
    struct exprtyping_node k, v, b;
    struct exprtyping_result res;
    int64_t num = 0, den = 1;
    int point = 0, frac = 0;
    const char *p;

    for (p = text; *p; p++) {
        if (*p == '.') {
            point = frac = 1;
            continue;
        }
        if (*p == 'e' || *p == 'E') {
            int e = 0;

            point = 1;
            for (p++; *p >= '0' && *p <= '9'; p++)
                e = e * 10 + (*p - '0');
            while (e-- > 0)
                num *= 10;
            break;
        }
        if (*p < '0' || *p > '9')
            return 0;
        num = num * 10 + (*p - '0');
        if (frac)
            den *= 10;
    }
    memset(&k, 0, sizeof k);
    memset(&v, 0, sizeof v);
    memset(&b, 0, sizeof b);
    k.kind = ETN_CONST;
    k.num = num;
    k.den = den;
    k.point = (unsigned char)point;
    v.kind = ETN_VAR;
    v.type = (unsigned char)partner;
    b.kind = ETN_BIN;
    b.op = ETO_ADD;
    b.l = &v;
    b.r = &k;
    if (exprtyping_type(&b, want, &res))
        return 0;
    return b.o.ras;
}
