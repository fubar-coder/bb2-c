/*
 * page.c - the page docs/blitz2/expression-type-tables.md from a rule set
 * (exprtype_tables.h), and its comparison with the committed page.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exprtype_tables.h"

/* ------------------------------------------------------------ the text */

struct sb {
    char *p;
    size_t n, cap;
    int oom;
};

static void sb_add(struct sb *s, const char *t, size_t len)
{
    if (s->oom)
        return;
    if (s->n + len + 1 > s->cap) {
        size_t nc = s->cap ? s->cap : 4096;
        char *p;

        while (nc < s->n + len + 1)
            nc *= 2;
        p = realloc(s->p, nc);
        if (!p) {
            s->oom = 1;
            return;
        }
        s->p = p;
        s->cap = nc;
    }
    memcpy(s->p + s->n, t, len);
    s->n += len;
    s->p[s->n] = 0;
}

static void sb_puts(struct sb *s, const char *t)
{
    sb_add(s, t, strlen(t));
}

static void sb_printf(struct sb *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void sb_printf(struct sb *s, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    int k;

    va_start(ap, fmt);
    k = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (k < 0 || (size_t)k >= sizeof buf) {
        s->oom = 1;             /* not reached: the texts are short */
        return;
    }
    sb_add(s, buf, (size_t)k);
}

/* ----------------------------------------------------------- the names */

const char *ett_tname(unsigned t)
{
    static const char *const n[ET_NTYPES] = { "-", "B", "W", "L", "Q", "F", "A", "S" };
    return t < ET_NTYPES ? n[t] : "?";
}

const char *ett_oname(unsigned op)
{
    static const char *const n[ETO_COUNT + 1] = {
        "+",   "-",   "*",   "/",   "&",  "|",  "AND",    "OR",     "EOR",    "LSL",    "ASL",    "LSR",   "ASR", "Mod",
        "^",   "=",   "<>",  "<",   "<=", ">",  ">=",     "BitTst", "BitSet", "BitClr", "BitChg", "the end",
    };
    return op <= ETO_COUNT ? n[op] : "?";
}

/* the operator as code (no table cell holds one) */
static void opcode(struct sb *s, unsigned op)
{
    sb_puts(s, "`");
    sb_puts(s, ett_oname(op));
    sb_puts(s, "`");
}

/* the letter of a type in a cell: - is shown as an en dash */
static const char *tletter(unsigned t)
{
    return t ? ett_tname(t) : "\xe2\x80\x93";
}

/* the types of the operands and of the type so far */
static const unsigned numtypes[] = { ET_BYTE, ET_WORD, ET_LONG, ET_QUICK, ET_FLOAT };
static const unsigned tsftypes[] = { ET_NONE, ET_BYTE, ET_WORD, ET_LONG, ET_QUICK, ET_FLOAT };
#define NNUM (sizeof numtypes / sizeof numtypes[0])
#define NTSF (sizeof tsftypes / sizeof tsftypes[0])

/* ----------------------------------------------------------- the cells */

struct gen {
    const struct ett_rules *r;
    struct sb *s;
    int fail;                   /* a decision the rule set does not have */
    char what[160];             /* which */
    const struct ett_pageopts *o;       /* ett_page_opts: another rule set's page (NULL: today's) */
    struct ett_stats *st;               /* its counts of changed and marked cells */
};

static int cell(struct gen *g, unsigned op, unsigned tsf, unsigned l, unsigned r, const struct ett_facts *x,
                struct ett_cell *c)
{
    if (g->fail)
        return -1;
    if (g->r->cell(g->r->ctx, op, tsf, l, r, x, c)) {
        g->fail = 1;
        snprintf(g->what, sizeof g->what, "no decision for %s %s %s under %s", ett_tname(l), ett_oname(op),
                 ett_tname(r), ett_tname(tsf));
        memset(c, 0, sizeof *c);
        return -1;
    }
    return 0;
}

static void operand(struct sb *s, const struct ett_operand *o, unsigned own)
{
    sb_puts(s, tletter(o->as));
    if (o->how == ETT_RAW)
        sb_puts(s, "~");
    else if (o->as < own && own >= ET_QUICK && o->as <= ET_LONG)
        sb_puts(s, "\xe2\x86\x93");    /* rounded down or truncated: a down arrow */
}

static void celltext(struct sb *s, const struct ett_cell *c, unsigned l, unsigned r)
{
    if (!c->result) {
        sb_puts(s, "illegal");
        return;
    }
    operand(s, &c->l, l);
    sb_puts(s, " ");
    operand(s, &c->r, r);
    sb_puts(s, " \xe2\x86\x92 ");      /* a right arrow */
    sb_puts(s, tletter(c->result));
}

static int samecell(const struct ett_cell *a, const struct ett_cell *b)
{
    return a->result == b->result && a->l.as == b->l.as && a->l.how == b->l.how && a->r.as == b->r.as
           && a->r.how == b->r.how && a->exact == b->exact;
}

/* the operator with the operands swapped (a comparison mirrored); ETO_END:
   the operands are not interchangeable */
static unsigned swapop(unsigned op)
{
    switch (op) {
    case ETO_ADD:
    case ETO_MUL:
    case ETO_AMP:
    case ETO_BAR:
    case ETO_AND:
    case ETO_OR:
    case ETO_EOR:
    case ETO_EQ:
    case ETO_NE:
        return op;
    case ETO_LT: return ETO_GT;
    case ETO_GT: return ETO_LT;
    case ETO_LE: return ETO_GE;
    case ETO_GE: return ETO_LE;
    default: return ETO_END;
    }
}

/* the swapped cell with its operands back in the order of the row */
static int swapped(struct gen *g, unsigned op, unsigned tsf, unsigned l, unsigned r, struct ett_cell *c)
{
    struct ett_cell w;
    struct ett_operand t;

    if (cell(g, swapop(op), tsf, r, l, NULL, &w))
        return -1;
    t = w.l;
    w.l = w.r;
    w.r = t;
    *c = w;
    return 0;
}

/* the swapped operation, as text in the order it is written */
static void swappedtext(struct sb *s, const struct ett_cell *c, unsigned l, unsigned r)
{
    struct ett_cell w = *c;

    w.l = c->r;
    w.r = c->l;
    celltext(s, &w, r, l);
}

/* ----------------------------------------------------- the operator tables */

static const char *const tsfhead =
    "| left | right | \xe2\x80\x93 | B | W | L | Q | F |\n|---|---|---|---|---|---|---|---|\n";

/* the body of the tables of op (without its heading): the operations, the
   cells that differ from the swapped operation, the comparison as the
   right operand (#1182), the Word operations of a later index (#1179) */
static void opbody(struct gen *g, struct sb *s, unsigned op, unsigned group, unsigned *rows, unsigned *cells,
                   unsigned *nswap)
{
    struct ett_cell c, w;
    unsigned sop = swapop(op);
    struct sb list = { 0 };

    sb_puts(s, tsfhead);
    for (size_t i = 0; i < NNUM; i++)
        for (size_t j = 0; j < NNUM; j++) {
            unsigned l = numtypes[i], r = numtypes[j];

            sb_printf(s, "| %s | %s |", ett_tname(l), ett_tname(r));
            for (size_t k = 0; k < NTSF; k++) {
                unsigned t = tsftypes[k];

                if (cell(g, op, t, l, r, NULL, &c))
                    goto out;
                sb_puts(s, " ");
                celltext(s, &c, l, r);
                if (g->o && g->o->base) {
                    struct ett_cell b;

                    if (g->o->base->cell(g->o->base->ctx, op, t, l, r, NULL, &b)) {
                        g->fail = 1;
                        snprintf(g->what, sizeof g->what, "no base decision for %s %s %s under %s",
                                 ett_tname(l), ett_oname(op), ett_tname(r), ett_tname(t));
                        goto out;
                    }
                    if ((c.result || b.result) && !samecell(&c, &b)) {   /* two refusals are the same */
                        sb_puts(s, " \xe2\x97\x86");     /* a diamond: not today's cell */
                        g->st->changed[group]++;
                    }
                }
                if (g->o && g->o->mark && g->o->mark(g->o->markctx, op, t, l, r)) {
                    sb_puts(s, " \xe2\x80\xa1");         /* a double dagger: a CD place */
                    g->st->marked[group]++;
                }
                if (sop != ETO_END) {
                    if (swapped(g, op, t, l, r, &w))
                        goto out;
                    if (!samecell(&c, &w)) {
                        sb_puts(s, " \xe2\x80\xa0");   /* a dagger */
                        (*nswap)++;
                        if (i <= j) {   /* i > j: listed with its pair */
                            sb_printf(&list, "* left %s, right %s, type so far %s: `", ett_tname(l), ett_tname(r),
                                      tletter(t));
                            celltext(&list, &c, l, r);
                            sb_puts(&list, "`; swapped: `");
                            swappedtext(&list, &w, l, r);
                            sb_puts(&list, "`\n");
                        }
                    }
                }
                sb_puts(s, " |");
                (*cells)++;
            }
            sb_puts(s, "\n");
            (*rows)++;
        }
    sb_puts(s, "\n");
    if (sop != ETO_END) {
        const char *how = group == ETG_COMPARE ? "the comparison with its operands swapped and mirrored (`b>a` for "
                                                 "`a<b`, `b=a` for `a=b`)"
                                               : "the operation with its operands swapped (`b OP a` for `a OP b`)";

        if (!list.n)
            sb_printf(s, "Everywhere %s gives the same cell.\n\n", how);
        else {
            sb_printf(s, "The cells marked \xe2\x80\xa0 differ from %s. Each pair, the swapped one written in its "
                         "own order:\n\n", how);
            sb_add(s, list.p, list.n);
            sb_puts(s, "\n");
        }
    }
    if (list.oom)
        s->oom = 1;
    free(list.p);
    list.p = NULL;
    if (group != ETG_COMPARE) {
        struct ett_facts x = { 0 };

        x.rcmp = 1;
        sb_puts(s, "With a comparison as the right operand (its boolean, a Word; #1182):\n\n");
        sb_puts(s, "| left | right | \xe2\x80\x93 | B | W | L | Q | F |\n|---|---|---|---|---|---|---|---|\n");
        for (size_t i = 0; i < NNUM; i++) {
            unsigned l = numtypes[i];

            sb_printf(s, "| %s | a comparison |", ett_tname(l));
            for (size_t k = 0; k < NTSF; k++) {
                if (cell(g, op, tsftypes[k], l, ET_WORD, &x, &c))
                    goto out;
                sb_puts(s, " ");
                celltext(s, &c, l, ET_WORD);
                sb_puts(s, " |");
            }
            sb_puts(s, "\n");
        }
        sb_puts(s, "\n");
    }
    if (op == ETO_MUL || op == ETO_DIV) {
        /* the Long handlers of * and / use the mark (eval.c dotimesl, dodivl) */
        struct ett_facts x = { 0 };
        int any = 0;

        x.words = 1;
        for (size_t i = 0; i < 2; i++)
            for (size_t j = 0; j < 2; j++) {
                unsigned l = numtypes[i], r = numtypes[j];

                if (cell(g, op, ET_LONG, l, r, &x, &c))
                    goto out;
                if (!c.wordop)
                    continue;
                sb_puts(s, any ? ", " : "In an array index after the first (#1179), the operations `");
                if (any)
                    sb_puts(s, "`");
                sb_printf(s, "%s %s", ett_tname(l), ett_tname(r));
                sb_puts(s, "`");
                any = 1;
            }
        if (any)
            sb_printf(s, " are done with the processor's Word instruction (`%s`) instead of the Long library "
                         "routine; the operands and the result are as in the table.\n\n",
                      op == ETO_MUL ? "muls.w" : "divs.w");
    }
out:
    free(list.p);
}

struct opset {
    unsigned ops[ETO_COUNT];
    unsigned nops;
    char *body;
};

/* the tables of the operators of a group: the operators with the same
   body share them */
static void group(struct gen *g, unsigned grp, const unsigned *ops, unsigned nops, struct ett_stats *st)
{
    struct opset sets[ETO_COUNT];
    unsigned nsets = 0;

    for (unsigned i = 0; i < nops && !g->fail; i++) {
        struct sb b = { 0 };
        unsigned rows = 0, cells = 0, nswap = 0, k;

        opbody(g, &b, ops[i], grp, &rows, &cells, &nswap);
        if (b.oom || !b.p) {
            g->s->oom = 1;
            free(b.p);
            break;
        }
        for (k = 0; k < nsets; k++)
            if (!strcmp(sets[k].body, b.p))
                break;
        if (k < nsets) {
            sets[k].ops[sets[k].nops++] = ops[i];
            free(b.p);
            continue;
        }
        sets[nsets].ops[0] = ops[i];
        sets[nsets].nops = 1;
        sets[nsets].body = b.p;
        nsets++;
        st->tables[grp]++;
        st->rows[grp] += rows;
        st->cells[grp] += cells;
        st->swapped[grp] += nswap;
    }
    for (unsigned k = 0; k < nsets; k++) {
        sb_puts(g->s, "### ");
        for (unsigned i = 0; i < sets[k].nops; i++) {
            if (i)
                sb_puts(g->s, ", ");
            opcode(g->s, sets[k].ops[i]);
        }
        sb_puts(g->s, "\n\n");
        sb_puts(g->s, sets[k].body);
        free(sets[k].body);
    }
}

/* ------------------------------------------------------------- the page */

static void intro(struct gen *g, struct sb *s)
{
    int any;

    if (g->o) {
        sb_printf(s, "# %s\n\n", g->o->title);
        sb_puts(s, g->o->about);
        sb_puts(s, "\n");
        return;
    }

    sb_puts(s,
            "# Expression type tables\n"
            "\n"
            "<!-- Generated by tools/exprtables (exprtables page) from the type decisions of\n"
            "     programs/blitz2/src/exprtype.c; do not edit.  After a change of the rules,\n"
            "     build the target expression_type_tables_update and commit this file. -->\n"
            "\n"
            "These tables list the type rules of an expression in `blitz2` as the compiler\n"
            "applies them: for one operation of two values, the type each operand is taken\n"
            "in and the type that results, for every operator, operand type and type so far.\n"
            "The rules and the reasons for them are explained in\n"
            "[The type of an expression in blitz2](expression-types.md); this page only\n"
            "lists them. It is generated from the compiler's type decisions\n"
            "(`programs/blitz2/src/exprtype.c`) by `tools/exprtables`, and the build checks\n"
            "that it is up to date.\n"
            "\n"
            "## How to read the tables\n"
            "\n"
            "An expression is compiled from left to right with one *type so far*. It starts\n"
            "as the type the expression is wanted in: the type of the destination of an\n"
            "assignment or of a parameter, Long for an array index, none for `If`, `While`,\n"
            "`Until` and `Print` and for the right operand of a comparison. Every value of a\n"
            "higher type raises it ([The types and the \"type so\n"
            "far\"](expression-types.md#the-types-and-the-type-so-far)).\n"
            "\n"
            "Each table is for one operator, or for several that behave the same. A row is\n"
            "an operation `a OP b` of two variables or function values of the types *left*\n"
            "and *right*; a column is the type so far when the left operand is met. For the\n"
            "first operation of an expression that is the wanted type; for a later operation\n"
            "of a chain (the second `+` of `a+b+c`) it is the type the chain has reached,\n"
            "and the left operand is the value computed so far. So the same operation can\n"
            "have another type in another place of an expression: that is what makes the\n"
            "result depend on the order of the operands.\n"
            "\n"
            "A cell `L R \xe2\x86\x92 T` says: the left operand is taken as type L, the right one\n"
            "as type R, and the operation leaves the type so far T. The letters are B Byte,\n"
            "W Word, L Long, Q Quick, F Float, \xe2\x80\x93 none.\n"
            "\n"
            "* An operand taken as a higher type than its own is converted up first; `\xe2\x86\x93`\n"
            "  marks a Quick or Float operand converted down to a whole number (a Quick\n"
            "  rounded down, a Float truncated towards zero).\n"
            "* `Q~` is a Quick that keeps its fraction under a Long type so far: the operation\n"
            "  is computed exactly and its result rounded to a whole number at the operator\n"
            "  (#1178).");
    any = 0;
    for (unsigned op = 0; op < ETO_COUNT && !g->fail; op++) {
        struct ett_cell c;

        if (cell(g, op, ET_LONG, ET_QUICK, ET_LONG, NULL, &c))
            return;
        if (!c.result || c.l.how == ETT_RAW || c.l.as != ET_LONG)
            continue;
        sb_puts(s, any ? ", " : "\n  The operators ");
        opcode(s, op);
        any = 1;
    }
    if (any)
        sb_puts(s, " take it rounded\n  down (`L\xe2\x86\x93`), as the original does");
    sb_puts(s, ".\n"
            "* `illegal`: the compiler refuses the operation (\"Illegal Operator for Type\").\n"
            "* \xe2\x80\xa0 marks a cell that differs when the operands are swapped (listed below\n"
            "  each table).\n"
            "\n"
            "The value of the whole expression is converted to the wanted type at the end;\n"
            "that conversion is not part of the tables. A constant is fetched by its own\n"
            "rule (\"Constants\", below), so an operation with a constant operand follows\n"
            "the row of the type the constant takes. String operands (`+` and the\n"
            "comparisons take them) are not shown.\n"
            "\n");
}

static void elements(struct gen *g, struct sb *s)
{
    sb_puts(s, "## Elements\n\n"
               "A variable or a function value of the type *own* under each type so far: the\n"
               "type so far after it. `W\xe2\x86\x92L`: the value is converted to the type so far; a\n"
               "single letter: the value keeps its type, and the type so far becomes it; `Q~ (L)`:\n"
               "a Quick held with its fraction, the type so far stays Long (#1178). A Byte value\n"
               "is always extended to a Word in its register; that is no change of type.\n\n"
               "| own | \xe2\x80\x93 | B | W | L | Q | F |\n|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < NNUM && !g->fail; i++) {
        unsigned own = numtypes[i];

        sb_printf(s, "| %s |", ett_tname(own));
        for (size_t k = 0; k < NTSF; k++) {
            struct ett_elem e;

            if (g->r->element(g->r->ctx, tsftypes[k], own, &e)) {
                g->fail = 1;
                snprintf(g->what, sizeof g->what, "no decision for a %s value under %s", ett_tname(own),
                         ett_tname(tsftypes[k]));
                return;
            }
            if (e.raw)
                sb_printf(s, " Q~ (%s) |", tletter(e.result));
            else if (e.conv)
                sb_printf(s, " %s\xe2\x86\x92%s |", ett_tname(own), tletter(e.result));
            else
                sb_printf(s, " %s |", tletter(e.result));
        }
        sb_puts(s, "\n");
    }
    sb_puts(s, "\n");
}

struct constkind {
    const char *example, *what;
    struct exprtype_constfacts k;
};

/* the kinds of constants: point fix whole data qfit size */
static const struct constkind kinds[] = {
    { "5", "a whole number of Byte size", { 0, 0, 0, 0, 0, ET_BYTE } },
    { "300", "a whole number of Word size", { 0, 0, 0, 0, 0, ET_WORD } },
    { "100000", "a whole number of Long size", { 0, 0, 0, 0, 0, ET_LONG } },
    { "1.5", "a fraction that fits a Quick", { 1, 1, 0, 0, 1, ET_BYTE } },
    { "40000.5", "a fraction beyond the Quick range", { 1, 1, 0, 0, 0, ET_LONG } },
    { "2.0", "a whole value written with a point", { 1, 1, 1, 0, 1, ET_BYTE } },
    { "300.0", "the same of Word size", { 1, 1, 1, 0, 1, ET_WORD } },
    { "1e10", "a whole part beyond a Long", { 1, 0, 0, 0, 0, ET_LONG } },
};
#define NKINDS (sizeof kinds / sizeof kinds[0])

static int constant(struct gen *g, unsigned tsf, const struct exprtype_constfacts *k, const char *ex,
                    struct ett_elem *e)
{
    if (g->fail)
        return -1;
    if (g->r->constant(g->r->ctx, tsf, k, e)) {
        g->fail = 1;
        snprintf(g->what, sizeof g->what, "no decision for the constant %s under %s", ex, ett_tname(tsf));
        return -1;
    }
    return 0;
}

static void consttext(struct sb *s, const struct ett_elem *e)
{
    if (!e->result)
        sb_puts(s, "mismatch");
    else {
        sb_puts(s, tletter(e->result));
        if (e->litcut)
            sb_puts(s, " (whole)");
    }
}

/* the same in a sentence */
static void constprose(struct sb *s, const struct ett_elem *e)
{
    static const char *const names[] = { "none", "Byte", "Word", "Long", "Quick", "Float", "Addr", "String" };

    if (!e->result || e->result >= ET_NTYPES)
        sb_puts(s, "refused (\"Type Mismatch\")");
    else
        sb_printf(s, "%s %s", e->litcut ? "made the whole number, a" : "a", names[e->result]);
}

static void constants(struct gen *g, struct sb *s)
{
    struct ett_elem e;
    struct exprtype_constfacts data = { 1, 1, 0, 1, 1, ET_BYTE };

    sb_puts(s, "## Constants\n\n"
               "A constant takes the type it is wanted in, or the size it needs; it is written in\n"
               "that type, without conversion code. The cell is the type so far after it;\n"
               "\"(whole)\": a constant written with a point that is taken as the whole number it\n"
               "is. A negative constant is the constant with a minus before it.\n\n"
               "| constant | kind | \xe2\x80\x93 | B | W | L | Q | F |\n|---|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < NKINDS && !g->fail; i++) {
        sb_printf(s, "| `%s` | %s |", kinds[i].example, kinds[i].what);
        for (size_t k = 0; k < NTSF; k++) {
            if (constant(g, tsftypes[k], &kinds[i].k, kinds[i].example, &e))
                return;
            sb_puts(s, " ");
            consttext(s, &e);
            sb_puts(s, " |");
        }
        sb_puts(s, "\n");
    }
    sb_puts(s, "\n");
    /* #1188: the Byte column, and a Data value */
    sb_puts(s, "Under a Byte type so far (#1188) a constant with a fraction (`1.5`) is ");
    if (constant(g, ET_BYTE, &kinds[3].k, kinds[3].example, &e))
        return;
    constprose(s, &e);
    sb_puts(s, ",\nso the operation is computed in that type and only its result is converted to\n"
               "the Byte; a whole value written with a point (`2.0`) is ");
    if (constant(g, ET_BYTE, &kinds[5].k, kinds[5].example, &e))
        return;
    constprose(s, &e);
    sb_puts(s, ".\nA `Data.b` value with a fraction (`Data.b 1.5`) is ");
    if (constant(g, ET_BYTE, &data, "Data.b 1.5", &e))
        return;
    constprose(s, &e);
    sb_puts(s, ".\n\n");
}

static void exceptions(struct sb *s)
{
    sb_puts(s,
            "## The exceptions and their issues\n"
            "\n"
            "The rules of the original compiler with the port's fixes of these original\n"
            "bugs; each is described on [The type of an expression in\n"
            "blitz2](expression-types.md):\n"
            "\n"
            "* #978: an equality comparison of a byte value with a constant from 128 to 255\n"
            "  takes the constant as the byte it stands for ([deviations](deviations.md)).\n"
            "* #1177: the operands of a comparison are compared with their own values\n"
            "  ([A comparison starts again](expression-types.md#a-comparison-starts-again)).\n"
            "* #1178: a Quick under a Long type so far keeps its fraction\n"
            "  ([The port: a Quick keeps its\n"
            "  fraction](expression-types.md#the-port-a-quick-keeps-its-fraction-1178)).\n"
            "* #1179: an array index after the first is a Long, as the first\n"
            "  ([Where this is a problem](expression-types.md#where-this-is-a-problem)).\n"
            "* #1182: a comparison as the right operand is converted up to the left\n"
            "  operand's type ([A comparison starts again](expression-types.md#a-comparison-starts-again)).\n"
            "* #1183 (open): a left operand of a comparison that is itself an operation keeps\n"
            "  the type so far ([A comparison starts again](expression-types.md#a-comparison-starts-again)).\n"
            "* #1186, #1188: constants with a point under a Byte type so far ([A constant with\n"
            "  a fraction under a Byte type so\n"
            "  far](expression-types.md#a-constant-with-a-fraction-under-a-byte-type-so-far-1188)).\n"
            "* #1209: `&`, `|`, `AND`, `OR`, `EOR` take a Quick as a whole number ([Bit\n"
            "  operators under a Quick type so\n"
            "  far](expression-types.md#bit-operators-under-a-quick-type-so-far-1209)).\n");
}

/* the constants beside an operand of each type, under no wanted type */
static void constbeside(struct gen *g, struct sb *s)
{
    static const struct {
        const char *text, *what;
    } k[] = {
        { "5", "a whole number of Byte size" },       { "300", "a whole number of Word size" },
        { "100000", "a whole number of Long size" },  { "1.5", "a fraction that fits a Quick" },
        { "40000.5", "a fraction beyond the Quick range" }, { "2.0", "a whole value written with a point" },
    };

    if (!g->o->constbeside)
        return;
    sb_puts(s, "## Constants beside an operand\n\n"
               "A constant beside an operand of the type *other* (the columns), with no wanted\n"
               "type: the type it takes.\n\n"
               "| constant | kind | B | W | L | Q | F |\n|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < sizeof k / sizeof k[0]; i++) {
        sb_printf(s, "| `%s` | %s |", k[i].text, k[i].what);
        for (size_t j = 0; j < NNUM; j++)
            sb_printf(s, " %s |",
                      tletter((unsigned)g->o->constbeside(g->o->constctx, ET_NONE, numtypes[j], k[i].text)));
        sb_puts(s, "\n");
    }
    sb_puts(s, "\n");
}

char *ett_page(const struct ett_rules *r, struct ett_stats *stats, char **error)
{
    return ett_page_opts(r, NULL, stats, error);
}

char *ett_page_opts(const struct ett_rules *r, const struct ett_pageopts *o, struct ett_stats *stats,
                    char **error)
{
    static const unsigned arith[] = { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV };
    static const unsigned bit[] = { ETO_AMP, ETO_BAR, ETO_AND, ETO_OR, ETO_EOR };
    static const unsigned shift[] = { ETO_LSL, ETO_ASL, ETO_LSR, ETO_ASR };
    static const unsigned mod[] = { ETO_MOD };
    static const unsigned pow[] = { ETO_POW };
    static const unsigned cmp[] = { ETO_EQ, ETO_NE, ETO_LT, ETO_LE, ETO_GT, ETO_GE };
    static const unsigned bitt[] = { ETO_BITTST, ETO_BITSET, ETO_BITCLR, ETO_BITCHG };
    struct sb s = { 0 };
    struct ett_stats st;
    struct gen g = { r, &s, 0, "", o, &st };
    struct ett_cell c;
    struct ett_facts x = { 0 };
    int any;

    memset(&st, 0, sizeof st);
    *error = NULL;
    intro(&g, &s);

    sb_puts(&s, "## Arithmetic\n\n");
    group(&g, ETG_ARITH, arith, 4, &st);

    sb_puts(&s, "## Bit operators\n\n"
                "`&` and `AND`, `|` and `OR` are the same operators; `EOR` is the exclusive or.\n\n");
    group(&g, ETG_BIT, bit, 5, &st);
    sb_puts(&s, "Under a Quick type so far (#1209) a Quick operand of these operators is made a\n"
                "whole number first (rounded down, its fraction bits cleared), and the result is\n"
                "a Quick: `q AND p` with `q=1.5` and `p=2.5` is 0. A Float has none of them.\n\n");

    sb_puts(&s, "## Shifts\n\n"
                "The right operand is the count. A shift of a Quick works on its 16.16 value, so\n"
                "the fraction takes part (`x LSR 1` with `x=5` is 2.5).\n\n");
    group(&g, ETG_SHIFT, shift, 4, &st);

    sb_puts(&s, "## Mod\n\n");
    group(&g, ETG_MOD, mod, 1, &st);
    sb_puts(&s, "Under a Long type so far a Quick operand of `Mod` keeps its fraction only when\n"
                "the other operand is a Float; with a whole-number divisor it is rounded down\n"
                "(`L\xe2\x86\x93`, #1178).\n\n");

    sb_puts(&s, "## Power\n\n");
    group(&g, ETG_POW, pow, 1, &st);

    sb_puts(&s, "## Comparisons\n\n"
                "A comparison leaves a Word, its boolean (-1 or 0), as the type so far. Its\n"
                "operands are compared with their own values (#1177): the right operand starts\n"
                "with no type so far, a left operand that is one element (a variable, field,\n"
                "array element or function value) keeps its own type, and the lower of the\n"
                "two is converted up to the other.");
    any = 0;
    for (size_t i = 0; i < NNUM && !any; i++)
        for (size_t j = 0; j < NNUM && !any; j++) {
            if (cell(&g, ETO_LT, ET_NONE, numtypes[i], numtypes[j], NULL, &c))
                break;
            any = c.exact;
        }
    if (any)
        sb_puts(&s, " A Long and a Quick (`L Q`, `Q L`) are compared exactly:\n"
                    "the Long against the whole part of the Quick, on equality the fraction decides;\n"
                    "the Long is not converted to a Quick.");
    sb_puts(&s, "\n\n");
    group(&g, ETG_COMPARE, cmp, 6, &st);
    if (!g.o)
        sb_puts(&s, "A left operand that is itself an operation (`q*2`, `q+q`) is not one element: it\n"
                "is computed in the type so far, as the left operand of any operator, and only\n"
                "then compared (#1183, open): `r.l=(q*2>2.4)` with `q=1.25` is 0, as 2.5 is\n"
                "rounded down to 2 under the Long.\n\n");
    x.byte978 = 1;
    any = 0;
    for (size_t i = 0; i < 6 && !g.fail; i++) {
        if (cell(&g, cmp[i], ET_WORD, ET_BYTE, ET_WORD, &x, &c))
            break;
        if (!c.byteext)
            continue;
        sb_puts(&s, any ? " and " : "An ");
        opcode(&s, cmp[i]);
        any = 1;
    }
    if (any)
        sb_puts(&s, " comparison of one Byte value with one constant from 128 to 255\n"
                    "takes the constant as the byte it stands for, sign-extended as the Byte is\n"
                    "(#978): `a.b=$90 : If a=$90` is true.\n\n");

    sb_puts(&s, "## Bit test and set\n\n"
                "The right operand is the number of the bit.\n\n");
    group(&g, ETG_BITTEST, bitt, 4, &st);

    elements(&g, &s);
    constants(&g, &s);
    if (o) {
        constbeside(&g, &s);
        sb_puts(&s, o->notes);
    } else
        exceptions(&s);

    if (g.fail || s.oom) {
        free(s.p);
        *error = malloc(sizeof g.what + 32);
        if (*error)
            snprintf(*error, sizeof g.what + 32, "%s", g.fail ? g.what : "out of memory");
        return NULL;
    }
    if (stats)
        *stats = st;
    return s.p;
}

/* -------------------------------------------------------- the comparison */

int ett_page_compare(const char *want, size_t wn, const char *got, size_t gn, const char *name, char **report)
{
    struct sb s = { 0 };
    size_t line = 0, ndiff = 0;
    const char *a = want, *b = got, *ae = want + wn, *be = got + gn;

    *report = NULL;
    while (a < ae || b < be) {
        const char *an = a < ae ? memchr(a, '\n', (size_t)(ae - a)) : NULL;
        const char *bn = b < be ? memchr(b, '\n', (size_t)(be - b)) : NULL;
        size_t al = a < ae ? (size_t)((an ? an : ae) - a) : 0, bl = b < be ? (size_t)((bn ? bn : be) - b) : 0;
        int ahas = a < ae, bhas = b < be;

        line++;
        if (ahas != bhas || al != bl || memcmp(a, b, al)) {
            if (++ndiff <= 10) {
                if (!ahas)
                    sb_printf(&s, "%s:%zu: missing: ", name, line);
                else if (!bhas)
                    sb_printf(&s, "%s:%zu: not in the generated page: ", name, line);
                else
                    sb_printf(&s, "%s:%zu: differs from the generated page: ", name, line);
                sb_add(&s, ahas ? a : b, ahas ? al : bl);
                if (ahas && bhas) {
                    sb_puts(&s, "\n  generated: ");
                    sb_add(&s, b, bl);
                }
                sb_puts(&s, "\n");
            }
        }
        a = ahas ? (an ? an + 1 : ae) : a;
        b = bhas ? (bn ? bn + 1 : be) : b;
    }
    if (!ndiff)
        return 0;
    sb_printf(&s, "%s: lines that differ from the generated page: %zu\n", name, ndiff);
    if (s.oom) {
        free(s.p);
        return -1;
    }
    *report = s.p;
    return 1;
}
