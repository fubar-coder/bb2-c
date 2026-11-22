/*
 * exprmatrix.c - the description of the expression matrix, its cells and
 * the generated sources (exprmatrix.h).
 */
#define _DEFAULT_SOURCE
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "exprmatrix.h"

static const char *const kind_names[EM_NKINDS] = { "b", "w", "l", "q", "f", "s", "cb", "cw", "cl", "cf" };

static const struct {
    const char *name;           /* the key part */
    const char *text;           /* the Blitz text */
} ops[EM_NOPS] = {
    { "add", "+" }, { "sub", "-" }, { "mul", "*" }, { "div", "/" }, { "mod", "MOD" }, { "pow", "^" },
    { "and", "AND" }, { "or", "OR" }, { "eor", "EOR" }, { "lsl", "LSL" }, { "lsr", "LSR" },
    { "asl", "ASL" }, { "asr", "ASR" }, { "eq", "=" }, { "ne", "<>" }, { "lt", "<" }, { "le", "<=" },
    { "gt", ">" }, { "ge", ">=" }, { "neg", "-" }, { "not", "NOT" }
};

static const char *const ctx_names[EM_NCTX] = {
    "db", "dw", "dl", "dq", "df", "pl", "pw", "pq", "pf", "if", "i1", "i2", "print"
};

/* the Blitz type suffix of the variables of a kind, of a destination or parameter */
static const char *const kind_suffix[EM_NKINDS] = { ".b", ".w", ".l", ".q", ".f", "$", "", "", "", "" };

const char *em_kind_name(enum em_kind k)
{
    return (unsigned)k < EM_NKINDS ? kind_names[k] : "?";
}

const char *em_op_name(enum em_op o)
{
    return (unsigned)o < EM_NOPS ? ops[o].name : "?";
}

const char *em_op_text(enum em_op o)
{
    return (unsigned)o < EM_NOPS ? ops[o].text : "?";
}

const char *em_ctx_name(enum em_ctx c)
{
    return (unsigned)c < EM_NCTX ? ctx_names[c] : "?";
}

int em_op_is_unary(enum em_op o)
{
    return o == EM_NEG || o == EM_NOT;
}

int em_op_is_compare(enum em_op o)
{
    return o >= EM_EQ && o <= EM_GE;
}

static int is_variable(enum em_kind k)
{
    return k <= EM_S;
}

/* the right operand of a shift and of ^ is the count of its kind */
static int takes_count(enum em_op o)
{
    return o == EM_POW || (o >= EM_LSL && o <= EM_ASR);
}

/* ---- growing text ---- */

struct buf {
    char *s;
    size_t n, cap;
    int oom;
};

static void put(struct buf *b, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

static void put(struct buf *b, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (b->oom)
        return;
    for (;;) {
        size_t room = b->cap - b->n;

        va_start(ap, fmt);
        n = vsnprintf(b->s ? b->s + b->n : NULL, room, fmt, ap);
        va_end(ap);
        if (n < 0) {
            b->oom = 1;
            return;
        }
        if ((size_t)n < room) {
            b->n += (size_t)n;
            return;
        }
        size_t cap = b->cap ? b->cap * 2 : 256;
        while (cap - b->n <= (size_t)n)
            cap *= 2;
        char *s = realloc(b->s, cap);
        if (!s) {
            b->oom = 1;
            return;
        }
        b->s = s;
        b->cap = cap;
    }
}

/* ---- the description ---- */

enum { MAXWORDS = 64 };

struct pblock {                 /* the block being read */
    int open;
    size_t index;
    int line;
    int ctx[EM_NCTX];
    size_t nctx;
    int bin[EM_NOPS], un[EM_NOPS];
    size_t nbin, nun;
    int kinds[EM_NKINDS];
    size_t nkinds;
    size_t *sets;
    size_t nsets;
    struct em_cell *cells;      /* its cell lines */
    size_t ncells;
};

struct parser {
    const char *file;
    int line;
    struct em_matrix *m;
    struct buf err;
    size_t nerr;
    int oom;
    struct pblock b;
    /* per set and kind: the missing value (1) or count (2) is reported */
    unsigned char (*missing)[EM_NKINDS];
    size_t nmissing;
};

static void perr(struct parser *p, int line, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

static void perr(struct parser *p, int line, const char *fmt, ...)
{
    char msg[300];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    put(&p->err, "%s:%d: %s\n", p->file, line, msg);
    p->nerr++;
}

static int lookup(const char *w, const char *const *names, size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (!strcasecmp(w, names[i]))
            return (int)i;
    return -1;
}

static int find_kind(const char *w)
{
    return lookup(w, kind_names, EM_NKINDS);
}

static int find_ctx(const char *w)
{
    return lookup(w, ctx_names, EM_NCTX);
}

/* an operator by its Blitz text or its key part; unary: "-" is neg */
static int find_op(const char *w, int unary)
{
    if (unary) {
        if (!strcmp(w, "-") || !strcasecmp(w, "neg"))
            return EM_NEG;
        if (!strcasecmp(w, "not"))
            return EM_NOT;
        return -1;
    }
    for (int o = 0; o < EM_NEG; o++)
        if (!strcasecmp(w, ops[o].text) || !strcasecmp(w, ops[o].name))
            return o;
    return -1;
}

static long find_set(const struct em_matrix *m, const char *w)
{
    for (size_t i = 0; i < m->nsets; i++)
        if (!strcmp(m->sets[i].name, w))
            return (long)i;
    return -1;
}

/* a name of a block or a set: letters and digits (a block also '_') */
static int good_name(const char *w, int block)
{
    if (!*w)
        return 0;
    for (const char *c = w; *c; c++)
        if (!isalnum((unsigned char)*c) && !(block && *c == '_'))
            return 0;
    return 1;
}

/* a decimal number: optional '-', digits, optionally '.' and digits; *frac: it has a '.' */
static int number(const char *s, int *frac, long long *whole)
{
    const char *c = s;
    long long v = 0;
    int neg = 0;

    *frac = 0;
    if (*c == '-') {
        neg = 1;
        c++;
    }
    if (!isdigit((unsigned char)*c))
        return 0;
    while (isdigit((unsigned char)*c)) {
        if (v < 100000000000LL)
            v = v * 10 + (*c - '0');
        c++;
    }
    if (*c == '.') {
        c++;
        if (!isdigit((unsigned char)*c))
            return 0;
        while (isdigit((unsigned char)*c))
            c++;
        *frac = 1;
    }
    if (*c)
        return 0;
    *whole = neg ? -v : v;
    return 1;
}

/* NULL: the value is good for the kind; else what is wrong (count: a count) */
static const char *check_value(enum em_kind k, const char *v, int count)
{
    int frac;
    long long n;

    if (k == EM_S) {
        if (count)
            return "a string has no count";
        for (const char *c = v; *c; c++)
            if (*c == '"' || !isprint((unsigned char)*c))
                return "a string value holds no '\"' and only printable characters";
        return NULL;
    }
    if (!number(v, &frac, &n))
        return "not a decimal number";
    switch (k) {
    case EM_Q:
        return n < -32768 || n > 32767 ? "outside the Quick range" : NULL;
    case EM_F:
        return NULL;
    case EM_CF:
        return count || frac ? NULL : "a constant with a fraction needs a '.'";
    default:
        break;
    }
    if (frac)
        return "a whole number is needed";
    if (n < INT32_MIN || n > INT32_MAX)
        return "outside the Long range";
    if (count)
        return NULL;
    switch (k) {
    case EM_B:
        return n < -128 || n > 127 ? "outside the Byte range" : NULL;
    case EM_W:
        return n < -32768 || n > 32767 ? "outside the Word range" : NULL;
    case EM_CB:
        return n < -128 || n > 127 ? "not a constant of Byte size" : NULL;
    case EM_CW:
        return n >= -128 && n <= 127 ? "a constant of Byte size, not of Word size"
               : n < -32768 || n > 32767 ? "not a constant of Word size" : NULL;
    case EM_CL:
        return n >= -32768 && n <= 32767 ? "a constant of Word size, not of Long size" : NULL;
    default:
        return NULL;
    }
}

static void d_values(struct parser *p, char **w, size_t nw, int count)
{
    struct em_matrix *m = p->m;
    const char *what = count ? "counts" : "values";
    long si;

    if (nw < 3) {
        perr(p, p->line, "%s needs a value set and KIND=VALUE", what);
        return;
    }
    if (!good_name(w[1], 0)) {
        perr(p, p->line, "bad value set name \"%s\" (letters and digits)", w[1]);
        return;
    }
    si = find_set(m, w[1]);
    if (si < 0) {
        struct em_set *s = realloc(m->sets, (m->nsets + 1) * sizeof *s);

        if (!s) {
            p->oom = 1;
            return;
        }
        m->sets = s;
        memset(&s[m->nsets], 0, sizeof *s);
        s[m->nsets].line = p->line;
        if (!(s[m->nsets].name = strdup(w[1]))) {
            p->oom = 1;
            return;
        }
        si = (long)m->nsets++;
    }
    for (size_t i = 2; i < nw; i++) {
        char *eq = strchr(w[i], '=');
        int k;
        const char *bad;
        char **slot;

        if (!eq) {
            perr(p, p->line, "\"%s\" is not KIND=VALUE", w[i]);
            continue;
        }
        *eq = 0;
        if ((k = find_kind(w[i])) < 0) {
            perr(p, p->line, "unknown kind \"%s\"", w[i]);
            continue;
        }
        if ((bad = check_value((enum em_kind)k, eq + 1, count))) {
            perr(p, p->line, "%s %s=%s: %s", what, kind_names[k], eq + 1, bad);
            continue;
        }
        slot = count ? &m->sets[si].count[k] : &m->sets[si].value[k];
        if (*slot) {
            perr(p, p->line, "value set %s: %s of kind %s given twice", w[1], count ? "count" : "value",
                 kind_names[k]);
            continue;
        }
        if (!(*slot = strdup(eq + 1)))
            p->oom = 1;
    }
}

static void d_number(struct parser *p, char **w, size_t nw, long *dst, long min)
{
    char *end;
    long v;

    errno = 0;
    v = nw == 2 ? strtol(w[1], &end, 10) : 0;
    if (nw != 2 || errno || *end || end == w[1] || v < min || v > 100000) {
        perr(p, p->line, "%s needs a number from %ld to 100000", w[0], min);
        return;
    }
    *dst = v;
}

/* adds the cell to the matrix with its key */
static void add_cell(struct parser *p, struct em_cell c)
{
    struct em_matrix *m = p->m;
    struct em_cell *v;

    if (c.right < 0)
        snprintf(c.key, sizeof c.key, "%s-%s-%s-%s", ctx_names[c.ctx], ops[c.op].name, kind_names[c.left],
                 m->sets[c.set].name);
    else
        snprintf(c.key, sizeof c.key, "%s-%s-%s-%s-%s", ctx_names[c.ctx], ops[c.op].name, kind_names[c.left],
                 kind_names[c.right], m->sets[c.set].name);
    v = realloc(m->cells, (m->ncells + 1) * sizeof *v);
    if (!v) {
        p->oom = 1;
        return;
    }
    m->cells = v;
    v[m->ncells++] = c;
}

/* NULL: BASIC gives the cell a meaning; else why not */
static const char *meaningless(enum em_ctx ctx, enum em_op op, enum em_kind l, int r)
{
    int ls = l == EM_S, rs = r == EM_S;

    if (r < 0)
        return ls ? "a string under a unary operator" : NULL;
    if (ls != rs)
        return "a string with a number";
    if (ls && op != EM_ADD && !em_op_is_compare(op))
        return "a string under an operator other than + and the comparisons";
    if (ls && op == EM_ADD && ctx != EM_PRINT)
        return "a string + outside print";
    return NULL;
}

/* the set has the values the cell needs; reports a missing one once per set and kind */
static int has_values(struct parser *p, const struct em_cell *c, int line)
{
    const struct em_set *s = &p->m->sets[c->set];
    int ok = 1;

    if (!s->value[c->left]) {
        if (!(p->missing[c->set][c->left] & 1))
            perr(p, line, "value set %s has no value of kind %s", s->name, kind_names[c->left]);
        p->missing[c->set][c->left] |= 1;
        ok = 0;
    }
    if (c->right >= 0) {
        int cnt = takes_count(c->op);
        char *const *v = cnt ? s->count : s->value;

        if (!v[c->right]) {
            if (!(p->missing[c->set][c->right] & (cnt ? 2 : 1)))
                perr(p, line, "value set %s has no %s of kind %s", s->name, cnt ? "count" : "value",
                     kind_names[c->right]);
            p->missing[c->set][c->right] |= cnt ? 2 : 1;
            ok = 0;
        }
    }
    return ok;
}

static void block_close(struct parser *p)
{
    struct pblock *b = &p->b;

    if (!b->open)
        return;
    b->open = 0;
    if (!b->ncells) {
        if (!b->nctx)
            perr(p, b->line, "block %s has no contexts", p->m->blocks[b->index].name);
        if (!b->nkinds)
            perr(p, b->line, "block %s has no kinds", p->m->blocks[b->index].name);
        if (!b->nbin && !b->nun)
            perr(p, b->line, "block %s has no operators", p->m->blocks[b->index].name);
        if (!b->nsets)
            perr(p, b->line, "block %s has no value sets", p->m->blocks[b->index].name);
    }
    if (p->nmissing < p->m->nsets) {
        unsigned char(*v)[EM_NKINDS] = realloc(p->missing, p->m->nsets * sizeof *v);

        if (!v) {
            p->oom = 1;
            return;
        }
        memset(v + p->nmissing, 0, (p->m->nsets - p->nmissing) * sizeof *v);
        p->missing = v;
        p->nmissing = p->m->nsets;
    }
    for (size_t s = 0; s < b->nsets; s++)
        for (size_t x = 0; x < b->nctx; x++) {
            struct em_cell c = { .ctx = (enum em_ctx)b->ctx[x], .set = b->sets[s], .block = b->index,
                                 .line = b->line };

            for (size_t o = 0; o < b->nun; o++)
                for (size_t k = 0; k < b->nkinds; k++) {
                    c.op = (enum em_op)b->un[o];
                    c.left = (enum em_kind)b->kinds[k];
                    c.right = -1;
                    if (!meaningless(c.ctx, c.op, c.left, c.right) && has_values(p, &c, b->line))
                        add_cell(p, c);
                }
            for (size_t o = 0; o < b->nbin; o++)
                for (size_t k = 0; k < b->nkinds; k++)
                    for (size_t r = 0; r < b->nkinds; r++) {
                        c.op = (enum em_op)b->bin[o];
                        c.left = (enum em_kind)b->kinds[k];
                        c.right = b->kinds[r];
                        if (!meaningless(c.ctx, c.op, c.left, c.right) && has_values(p, &c, b->line))
                            add_cell(p, c);
                    }
        }
    for (size_t i = 0; i < b->ncells; i++)
        if (has_values(p, &b->cells[i], b->cells[i].line))
            add_cell(p, b->cells[i]);
    free(b->sets);
    free(b->cells);
    memset(b, 0, sizeof *b);
}

static void d_block(struct parser *p, char **w, size_t nw)
{
    struct em_matrix *m = p->m;
    struct em_block *v;

    block_close(p);
    if (nw != 2 || !good_name(w[1], 1)) {
        perr(p, p->line, "block needs one name (letters, digits, '_')");
        return;
    }
    for (size_t i = 0; i < m->nblocks; i++)
        if (!strcmp(m->blocks[i].name, w[1])) {
            perr(p, p->line, "block %s given twice (line %d)", w[1], m->blocks[i].line);
            return;
        }
    v = realloc(m->blocks, (m->nblocks + 1) * sizeof *v);
    if (!v) {
        p->oom = 1;
        return;
    }
    m->blocks = v;
    v[m->nblocks].line = p->line;
    if (!(v[m->nblocks].name = strdup(w[1]))) {
        p->oom = 1;
        return;
    }
    p->b.open = 1;
    p->b.index = m->nblocks++;
    p->b.line = p->line;
}

/* adds x to the list of the block unless it is there; 0: it was */
static int add_once(int *list, size_t *n, int x)
{
    for (size_t i = 0; i < *n; i++)
        if (list[i] == x)
            return 0;
    list[(*n)++] = x;
    return 1;
}

static void d_list(struct parser *p, char **w, size_t nw)
{
    struct pblock *b = &p->b;
    const char *d = w[0];

    if (nw < 2) {
        perr(p, p->line, "%s needs at least one entry", d);
        return;
    }
    for (size_t i = 1; i < nw; i++) {
        int x;

        if (!strcmp(d, "contexts")) {
            if ((x = find_ctx(w[i])) < 0)
                perr(p, p->line, "unknown context \"%s\"", w[i]);
            else if (!add_once(b->ctx, &b->nctx, x))
                perr(p, p->line, "context %s given twice", ctx_names[x]);
        } else if (!strcmp(d, "binary") || !strcmp(d, "unary")) {
            int un = d[0] == 'u';

            if ((x = find_op(w[i], un)) < 0)
                perr(p, p->line, "unknown operator \"%s\"", w[i]);
            else if (!add_once(un ? b->un : b->bin, un ? &b->nun : &b->nbin, x))
                perr(p, p->line, "operator %s given twice", w[i]);
        } else if (!strcmp(d, "kinds")) {
            if ((x = find_kind(w[i])) < 0)
                perr(p, p->line, "unknown kind \"%s\"", w[i]);
            else if (!add_once(b->kinds, &b->nkinds, x))
                perr(p, p->line, "kind %s given twice", kind_names[x]);
        } else {                /* sets */
            long s = find_set(p->m, w[i]);
            size_t j;

            if (s < 0) {
                perr(p, p->line, "unknown value set \"%s\"", w[i]);
                continue;
            }
            for (j = 0; j < b->nsets && b->sets[j] != (size_t)s; j++)
                ;
            if (j < b->nsets) {
                perr(p, p->line, "value set %s given twice", w[i]);
                continue;
            }
            size_t *v = realloc(b->sets, (b->nsets + 1) * sizeof *v);
            if (!v) {
                p->oom = 1;
                return;
            }
            b->sets = v;
            v[b->nsets++] = (size_t)s;
        }
    }
}

static void d_cell(struct parser *p, char **w, size_t nw)
{
    struct pblock *b = &p->b;
    struct em_cell c = { .block = b->index, .line = p->line, .right = -1 };
    long s;
    int x, op, l, r = -1;
    const char *why;

    if (nw != 5 && nw != 6) {
        perr(p, p->line, "cell needs SET CTX OP KIND [KIND]");
        return;
    }
    if ((s = find_set(p->m, w[1])) < 0) {
        perr(p, p->line, "unknown value set \"%s\"", w[1]);
        return;
    }
    if ((x = find_ctx(w[2])) < 0) {
        perr(p, p->line, "unknown context \"%s\"", w[2]);
        return;
    }
    if ((op = find_op(w[3], nw == 5)) < 0) {
        perr(p, p->line, "unknown operator \"%s\"", w[3]);
        return;
    }
    if ((l = find_kind(w[4])) < 0 || (nw == 6 && (r = find_kind(w[5])) < 0)) {
        perr(p, p->line, "unknown kind \"%s\"", l < 0 ? w[4] : w[5]);
        return;
    }
    if ((why = meaningless((enum em_ctx)x, (enum em_op)op, (enum em_kind)l, r))) {
        perr(p, p->line, "cell without a meaning: %s", why);
        return;
    }
    c.set = (size_t)s;
    c.ctx = (enum em_ctx)x;
    c.op = (enum em_op)op;
    c.left = (enum em_kind)l;
    c.right = r;
    struct em_cell *v = realloc(b->cells, (b->ncells + 1) * sizeof *v);
    if (!v) {
        p->oom = 1;
        return;
    }
    b->cells = v;
    v[b->ncells++] = c;
}

static int by_key(const void *a, const void *b)
{
    const struct em_cell *x = *(const struct em_cell *const *)a, *y = *(const struct em_cell *const *)b;
    int c = strcmp(x->key, y->key);

    return c ? c : (x->line > y->line) - (x->line < y->line);
}

static void check_keys(struct parser *p)
{
    struct em_matrix *m = p->m;
    const struct em_cell **v;

    if (m->ncells < 2)
        return;
    if (!(v = malloc(m->ncells * sizeof *v))) {
        p->oom = 1;
        return;
    }
    for (size_t i = 0; i < m->ncells; i++)
        v[i] = &m->cells[i];
    qsort(v, m->ncells, sizeof *v, by_key);
    for (size_t i = 1; i < m->ncells; i++)
        if (!strcmp(v[i]->key, v[i - 1]->key) && (i < 2 || strcmp(v[i]->key, v[i - 2]->key)))
            perr(p, v[i]->line, "duplicate key %s (also line %d)", v[i]->key, v[i - 1]->line);
    free(v);
}

void em_free(struct em_matrix *m)
{
    for (size_t i = 0; i < m->nsets; i++) {
        free(m->sets[i].name);
        for (int k = 0; k < EM_NKINDS; k++) {
            free(m->sets[i].value[k]);
            free(m->sets[i].count[k]);
        }
    }
    free(m->sets);
    for (size_t i = 0; i < m->nblocks; i++)
        free(m->blocks[i].name);
    free(m->blocks);
    free(m->cells);
    memset(m, 0, sizeof *m);
}

int em_parse(const char *text, size_t len, const char *file, struct em_matrix *m, char **errors)
{
    struct parser p = { .file = file, .m = m };
    const char *s = text, *end = text + len;

    memset(m, 0, sizeof *m);
    m->per_source = EM_PER_SOURCE_DEFAULT;
    m->dim = EM_DIM_DEFAULT;
    *errors = NULL;
    while (s < end && !p.oom && !p.err.oom) {
        const char *eol = memchr(s, '\n', (size_t)(end - s));
        size_t n = eol ? (size_t)(eol - s) : (size_t)(end - s);
        char *line = malloc(n + 1), *w[MAXWORDS], *c, *hash;
        size_t nw = 0;

        p.line++;
        if (!line) {
            p.oom = 1;
            break;
        }
        memcpy(line, s, n);
        line[n] = 0;
        s += n + (eol != NULL);
        if ((hash = strchr(line, '#')))
            *hash = 0;
        for (c = strtok(line, " \t\r"); c; c = strtok(NULL, " \t\r"))
            if (nw < MAXWORDS)
                w[nw++] = c;
        if (!nw) {
            free(line);
            continue;
        }
        if (nw == MAXWORDS)
            perr(&p, p.line, "more than %d words", MAXWORDS - 1);
        else if (!strcmp(w[0], "cells-per-source"))
            d_number(&p, w, nw, &m->per_source, 1);
        else if (!strcmp(w[0], "dim"))
            d_number(&p, w, nw, &m->dim, 0);
        else if (!strcmp(w[0], "values") || !strcmp(w[0], "counts"))
            d_values(&p, w, nw, w[0][0] == 'c');
        else if (!strcmp(w[0], "block"))
            d_block(&p, w, nw);
        else if (!strcmp(w[0], "contexts") || !strcmp(w[0], "binary") || !strcmp(w[0], "unary")
                 || !strcmp(w[0], "kinds") || !strcmp(w[0], "sets") || !strcmp(w[0], "cell")) {
            if (!p.b.open)
                perr(&p, p.line, "%s outside a block", w[0]);
            else if (!strcmp(w[0], "cell"))
                d_cell(&p, w, nw);
            else
                d_list(&p, w, nw);
        } else
            perr(&p, p.line, "unknown directive \"%s\"", w[0]);
        free(line);
    }
    if (!p.oom)
        block_close(&p);
    if (!p.oom && !p.nerr && !m->ncells)
        perr(&p, p.line, "no cells");
    if (!p.oom && !p.nerr)
        check_keys(&p);
    free(p.b.sets);
    free(p.b.cells);
    free(p.missing);
    if (p.oom || p.err.oom) {
        free(p.err.s);
        em_free(m);
        return -1;
    }
    if (p.nerr) {
        *errors = p.err.s;
        em_free(m);
        return 1;
    }
    free(p.err.s);
    return 0;
}

/* ---- the sources ---- */

/* the text of an operand: a variable or a constant */
static void operand(struct buf *t, const struct em_set *s, enum em_kind k, int count)
{
    if (is_variable(k))
        put(t, "%c%s%s", count ? 'n' : 'v', kind_names[k], k == EM_S ? "$" : "");
    else
        put(t, "%s", count ? s->count[k] : s->value[k]);
}

static void expression(struct buf *t, const struct em_set *s, const struct em_cell *c)
{
    if (c->right < 0) {
        const char *v = is_variable(c->left) ? NULL : s->value[c->left];

        /* "- -3", "NOT -3", "NOT vb", "-vb" */
        put(t, "%s%s", ops[c->op].text, c->op == EM_NOT || (v && v[0] == '-') ? " " : "");
        operand(t, s, c->left, 0);
        return;
    }
    operand(t, s, c->left, 0);
    put(t, " %s ", ops[c->op].text);
    operand(t, s, (enum em_kind)c->right, takes_count(c->op));
}

/* the type letter of a destination or parameter context */
static char ctx_type(enum em_ctx x)
{
    static const char t[] = "bwlqflwqf";

    return x <= EM_PF ? t[x] : 0;
}

static int count_lines(const struct buf *t)
{
    int n = 0;

    for (size_t i = 0; i < t->n; i++)
        n += t->s[i] == '\n';
    return n;
}

/* the variables, the context's setup and the cell lines (source_text,
   em_source_text) */
static void source_body(struct buf *t, const struct em_matrix *m, const struct em_cell *const *cells, size_t n,
                        int seterr, int *line_of)
{
    const struct em_cell *c0 = cells[0];
    const struct em_set *s = &m->sets[c0->set];
    enum em_ctx x = c0->ctx;
    char ty = ctx_type(x);
    int used[EM_NKINDS] = { 0 }, cnt[EM_NKINDS] = { 0 };
    const char *sep;

    for (size_t i = 0; i < n; i++) {
        used[cells[i]->left] = 1;
        if (cells[i]->right >= 0)
            (takes_count(cells[i]->op) ? cnt : used)[cells[i]->right] = 1;
    }
    sep = "";
    for (int k = 0; k <= EM_S; k++)
        if (used[k]) {
            if (k == EM_S)
                put(t, "%sv%s$=\"%s\"", sep, kind_names[k], s->value[k]);
            else
                put(t, "%sv%s%s=%s", sep, kind_names[k], kind_suffix[k], s->value[k]);
            sep = " : ";
        }
    if (*sep)
        put(t, "\n");
    sep = "";
    for (int k = 0; k < EM_S; k++)
        if (cnt[k]) {
            put(t, "%sn%s%s=%s", sep, kind_names[k], kind_suffix[k], s->count[k]);
            sep = " : ";
        }
    if (*sep)
        put(t, "\n");
    if (x <= EM_DF)
        put(t, "r%c.%c=0\n", ty, ty);
    else if (x <= EM_PF) {
        if (ty == 'q' || ty == 'f')
            put(t, "t%c.%c=0\n", ty, ty);
        put(t, "Statement p%c{x.%c}\n", ty, ty);
        if (ty == 'q' || ty == 'f')
            put(t, "  SHARED t%c\n  t%c=x : NPrint Hex$(Peek.l(&t%c))\n", ty, ty, ty);
        else
            put(t, "  NPrint x\n");
        put(t, "End Statement\n");
    } else if (x == EM_I1)
        put(t, "Dim ia.l(%ld)\nFor ii.l=0 To %ld : ia(ii)=ii*16+1 : Next\n", m->dim, m->dim);
    else if (x == EM_I2)
        put(t, "Dim ja.l(1,%ld)\nFor ii.l=0 To %ld : ja(1,ii)=ii*16+1 : Next\n", m->dim, m->dim);
    for (size_t i = 0; i < n; i++) {
        const struct em_cell *c = cells[i];
        int cmp = em_op_is_compare(c->op);

        /* a runtime error (checked, outside the array) goes to the
           handler, which prints the key and goes on behind the cell */
        if (seterr)
            put(t, "SetErr : NPrint \"%s !runtime\" : Goto mx%zu : End SetErr\n", c->key, i + 1);
        if (line_of && !t->oom)
            line_of[i] = count_lines(t) + 1;
        switch (x) {
        case EM_DB: case EM_DW: case EM_DL: case EM_DQ: case EM_DF:
            put(t, "r%c=%s", ty, cmp ? "(" : "");
            expression(t, s, c);
            if (ty == 'q' || ty == 'f')
                put(t, "%s : NPrint \"%s \",Hex$(Peek.l(&r%c))\n", cmp ? ")" : "", c->key, ty);
            else
                put(t, "%s : NPrint \"%s \",r%c\n", cmp ? ")" : "", c->key, ty);
            break;
        case EM_PL: case EM_PW: case EM_PQ: case EM_PF:
            put(t, "Print \"%s \" : p%c{", c->key, ty);
            expression(t, s, c);
            put(t, "}\n");
            break;
        case EM_IF:
            put(t, "If ");
            expression(t, s, c);
            put(t, " Then NPrint \"%s T\" Else NPrint \"%s F\"\n", c->key, c->key);
            break;
        case EM_I1:
        case EM_I2:
            put(t, "NPrint \"%s \",%s", c->key, x == EM_I1 ? "ia(" : "ja(1,");
            expression(t, s, c);
            put(t, ")\n");
            break;
        default:
            put(t, "NPrint \"%s \",", c->key);
            expression(t, s, c);
            put(t, "\n");
            break;
        }
        if (seterr)
            put(t, "mx%zu:\n", i + 1);
    }
    put(t, "End\n");
}

static void source_text(struct buf *t, const struct em_matrix *m, const char *name, const char *desc,
                        const struct em_cell *const *cells, size_t n, size_t first, size_t total)
{
    const struct em_cell *c0 = cells[0];

    put(t, "; %s - expression matrix: block %s, context %s, value set %s, cells %zu to %zu of %zu\n", name,
        m->blocks[c0->block].name, ctx_names[c0->ctx], m->sets[c0->set].name, first + 1, first + n, total);
    put(t, "; generated by exprmatrix from %s (tools/exprmatrix/README.md); do not edit\n", desc);
    source_body(t, m, cells, n, 0, NULL);
}

char *em_source_text(const struct em_matrix *m, const struct em_cell *const *cells, size_t n, const char *head,
                     int seterr, int *line_of)
{
    struct buf t = { 0 };

    if (!n)
        return NULL;
    put(&t, "%s", head);
    source_body(&t, m, cells, n, seterr, line_of);
    if (t.oom) {
        free(t.s);
        return NULL;
    }
    return t.s;
}

void em_sources_free(struct em_source *s, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        free(s[i].name);
        free(s[i].text);
    }
    free(s);
}

int em_generate(const struct em_matrix *m, const char *desc, long per_source, struct em_source **out,
                size_t *nout, char **errors)
{
    struct em_source *src = NULL;
    size_t nsrc = 0;
    const struct em_cell **group = NULL;
    unsigned char *done = NULL;
    struct buf err = { 0 };
    size_t nerr = 0;
    int oom = 0;

    *out = NULL;
    *nout = 0;
    *errors = NULL;
    if (per_source <= 0)
        per_source = m->per_source;
    if (!m->ncells)
        return 0;
    group = malloc(m->ncells * sizeof *group);
    done = calloc(m->ncells, 1);
    if (!group || !done)
        oom = 1;
    /* the groups (block, set, context) in the order of their first cell */
    for (size_t i = 0; i < m->ncells && !oom; i++) {
        const struct em_cell *c = &m->cells[i];
        size_t n = 0;

        if (done[i])
            continue;
        for (size_t j = i; j < m->ncells; j++) {
            const struct em_cell *d = &m->cells[j];

            if (!done[j] && d->block == c->block && d->set == c->set && d->ctx == c->ctx) {
                group[n++] = d;
                done[j] = 1;
            }
        }
        for (size_t first = 0, part = 1; first < n && !oom; first += (size_t)per_source, part++) {
            size_t k = n - first < (size_t)per_source ? n - first : (size_t)per_source;
            char name[64];
            struct buf t = { 0 };
            struct em_source *v;

            snprintf(name, sizeof name, "%s-%s-%s-%02zu", m->blocks[c->block].name, ctx_names[c->ctx],
                     m->sets[c->set].name, part);
            if (strlen(name) > EM_NAME_MAX) {
                put(&err, "%s: source name longer than %d characters (block %s, line %d)\n", name,
                    EM_NAME_MAX, m->blocks[c->block].name, m->blocks[c->block].line);
                nerr++;
                continue;
            }
            source_text(&t, m, name, desc, group + first, k, first, n);
            v = realloc(src, (nsrc + 1) * sizeof *v);
            if (t.oom || !v) {
                free(t.s);
                if (v)
                    src = v;
                oom = 1;
                break;
            }
            src = v;
            src[nsrc].text = t.s;
            src[nsrc].ncells = k;
            if (!(src[nsrc].name = strdup(name))) {
                free(t.s);
                oom = 1;
                break;
            }
            nsrc++;
        }
    }
    free(group);
    free(done);
    if (oom || err.oom) {
        em_sources_free(src, nsrc);
        free(err.s);
        return -1;
    }
    if (nerr) {
        em_sources_free(src, nsrc);
        *errors = err.s;
        return 1;
    }
    *out = src;
    *nout = nsrc;
    return 0;
}
