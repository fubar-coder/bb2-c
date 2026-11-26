/*
 * tsv.c - the rows of programs/blitz2/tests/host/exprtype.tsv (the
 * committed table of every decision of exprtype.c, written by
 * test_exprtype) as primitives (exprtype_tables.h): each decision is
 * looked up by its key, the fields before the result; a decision without
 * a row is counted as missing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exprtype_tables.h"

struct row {
    char *key;                  /* the key fields, tab-separated */
    char *val;                  /* the result fields, tab-separated */
    int used;
};

struct ett_tsv {
    struct row *rows;
    size_t n, used, missing;
    char *first_missing;
};

/* the number of key fields of a row kind (the kind included); 0: unknown */
static int keyfields(const char *kind, size_t len)
{
    static const struct {
        const char *kind;
        int n;
    } k[] = {
        { "optype", 3 }, { "cutsraw", 2 }, { "left", 4 }, { "ownleft", 3 },
        { "binop", 5 },  { "element", 3 }, { "const", 4 },  { "bypass", 5 }, { "bypassnot", 3 },
    };

    for (size_t i = 0; i < sizeof k / sizeof k[0]; i++)
        if (strlen(k[i].kind) == len && !memcmp(k[i].kind, kind, len))
            return k[i].n;
    return 0;
}

static int rowcmp(const void *a, const void *b)
{
    return strcmp(((const struct row *)a)->key, ((const struct row *)b)->key);
}

static char *dupn(const char *s, size_t n)
{
    char *d = malloc(n + 1);

    if (d) {
        memcpy(d, s, n);
        d[n] = 0;
    }
    return d;
}

/* appends a formatted line to *errors */
static int adderr(char **errors, size_t *elen, const char *name, size_t line, const char *msg)
{
    char buf[512];
    int k = snprintf(buf, sizeof buf, "%s:%zu: %s\n", name, line, msg);
    char *e = realloc(*errors, *elen + (size_t)k + 1);

    if (!e)
        return -1;
    memcpy(e + *elen, buf, (size_t)k + 1);
    *errors = e;
    *elen += (size_t)k;
    return 0;
}

void ett_tsv_free(struct ett_tsv *t)
{
    if (!t)
        return;
    for (size_t i = 0; i < t->n; i++) {
        free(t->rows[i].key);
        free(t->rows[i].val);
    }
    free(t->rows);
    free(t->first_missing);
    free(t);
}

int ett_tsv_load(const char *text, size_t n, const char *name, struct ett_tsv **out, char **errors)
{
    struct ett_tsv *t = calloc(1, sizeof *t);
    size_t cap = 0, elen = 0, line = 0;
    const char *p = text, *end = text + n;

    *out = NULL;
    *errors = NULL;
    if (!t)
        return -1;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *q, *split;
        int nk, fields;

        if (!eol)
            eol = end;
        line++;
        if (p == eol || *p == '#') {
            p = eol + 1;
            continue;
        }
        q = memchr(p, '\t', (size_t)(eol - p));
        nk = keyfields(p, q ? (size_t)(q - p) : (size_t)(eol - p));
        if (!nk || !q) {
            if (adderr(errors, &elen, name, line, "unknown row"))
                goto oom;
            p = eol + 1;
            continue;
        }
        /* the key: nk fields */
        split = p;
        for (fields = 0; fields < nk && split; fields++) {
            split = memchr(split, '\t', (size_t)(eol - split));
            if (split && fields < nk - 1)
                split++;
        }
        if (!split || split >= eol) {
            if (adderr(errors, &elen, name, line, "a row without its result"))
                goto oom;
            p = eol + 1;
            continue;
        }
        if (t->n == cap) {
            size_t nc = cap ? cap * 2 : 8192;
            struct row *r = realloc(t->rows, nc * sizeof *r);

            if (!r)
                goto oom;
            t->rows = r;
            cap = nc;
        }
        t->rows[t->n].key = dupn(p, (size_t)(split - p));
        t->rows[t->n].val = dupn(split + 1, (size_t)(eol - split - 1));
        t->rows[t->n].used = 0;
        if (!t->rows[t->n].key || !t->rows[t->n].val) {
            free(t->rows[t->n].key);
            free(t->rows[t->n].val);
            goto oom;
        }
        t->n++;
        p = eol + 1;
    }
    qsort(t->rows, t->n, sizeof *t->rows, rowcmp);
    for (size_t i = 1; i < t->n; i++)
        if (!strcmp(t->rows[i - 1].key, t->rows[i].key)) {
            if (adderr(errors, &elen, name, 0, "a key twice") || adderr(errors, &elen, name, 0, t->rows[i].key))
                goto oom;
        }
    if (*errors) {
        ett_tsv_free(t);
        return 1;
    }
    *out = t;
    return 0;
oom:
    ett_tsv_free(t);
    free(*errors);
    *errors = NULL;
    return -1;
}

size_t ett_tsv_rows(const struct ett_tsv *t)
{
    return t->n;
}

size_t ett_tsv_used(const struct ett_tsv *t)
{
    return t->used;
}

size_t ett_tsv_missing(const struct ett_tsv *t)
{
    return t->missing;
}

const char *ett_tsv_first_missing(const struct ett_tsv *t)
{
    return t->first_missing;
}

/* the result fields of the row with key, split at the tabs into f (at
   most nf); the number of fields, -1: no row */
static int lookup(struct ett_tsv *t, const char *key, char f[][16], int nf)
{
    struct row k = { (char *)key, NULL, 0 }, *r = bsearch(&k, t->rows, t->n, sizeof *t->rows, rowcmp);
    const char *p;
    int i = 0;

    if (!r) {
        t->missing++;
        if (!t->first_missing)
            t->first_missing = dupn(key, strlen(key));
        return -1;
    }
    if (!r->used) {
        r->used = 1;
        t->used++;
    }
    for (p = r->val; i < nf; i++) {
        const char *e = strchr(p, '\t');
        size_t len = e ? (size_t)(e - p) : strlen(p);

        if (len >= sizeof f[0])
            len = sizeof f[0] - 1;
        memcpy(f[i], p, len);
        f[i][len] = 0;
        if (!e) {
            i++;
            break;
        }
        p = e + 1;
    }
    return i;
}

static const char *tsvop(unsigned op)
{
    static const char *const n[ETO_COUNT + 1] = {
        "+", "-", "*", "/", "&", "|", "AND", "OR", "EOR", "LSL", "ASL", "LSR", "ASR",
        "MOD", "^", "=", "<>", "<", "<=", ">", ">=", "BITTST", "BITSET", "BITCLR", "BITCHG", "END",
    };
    return op <= ETO_COUNT ? n[op] : "?";
}

/* a type letter; -1 unknown */
static int ptype(const char *s)
{
    static const char l[] = "-BWLQFAS";
    const char *c = s[0] && !s[1] ? strchr(l, s[0]) : NULL;

    return c ? (int)(c - l) : -1;
}

/* a conversion FROM>TO or - */
static int pconv(const char *s, unsigned char *from, unsigned char *to)
{
    char a[2] = { s[0], 0 }, b[2] = { 0, 0 };
    int x, y;

    if (!strcmp(s, "-")) {
        *from = *to = 0;
        return 0;
    }
    if (strlen(s) != 3 || s[1] != '>')
        return -1;
    b[0] = s[2];
    x = ptype(a);
    y = ptype(b);
    if (x < 0 || y < 0)
        return -1;
    *from = (unsigned char)x;
    *to = (unsigned char)y;
    return 0;
}

/* a string of n 0/1 digits into d */
static int pbits(const char *s, unsigned char *d, int n)
{
    if ((int)strlen(s) != n)
        return -1;
    for (int i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        d[i] = (unsigned char)(s[i] - '0');
    }
    return 0;
}

static int t_cutsraw(void *ctx, unsigned op, int *out)
{
    char key[64], f[1][16];
    unsigned char b;

    snprintf(key, sizeof key, "cutsraw\t%s", tsvop(op));
    if (lookup(ctx, key, f, 1) != 1 || pbits(f[0], &b, 1))
        return -1;
    *out = b;
    return 0;
}

static int t_leftop(void *ctx, unsigned op, unsigned left, int lraw, struct exprtype_left *out)
{
    char key[64], f[4][16];
    int l;

    snprintf(key, sizeof key, "left\t%s\t%s\t%d", tsvop(op), ett_tname(left), lraw != 0);
    if (lookup(ctx, key, f, 4) != 4 || (l = ptype(f[0])) < 0 || pbits(f[1], &out->lraw, 1)
        || pbits(f[2], &out->ownleft, 1) || pbits(f[3], &out->compare, 1))
        return -1;
    out->left = (unsigned char)l;
    return 0;
}

static int t_ownleft(void *ctx, unsigned from, unsigned to, int *out)
{
    char key[64], f[1][16];
    unsigned char b;

    snprintf(key, sizeof key, "ownleft\t%s\t%s", ett_tname(from), ett_tname(to));
    if (lookup(ctx, key, f, 1) != 1 || pbits(f[0], &b, 1))
        return -1;
    *out = b;
    return 0;
}

static int t_binop(void *ctx, unsigned op, unsigned left, unsigned right, const struct exprtype_binfacts *x,
                   struct exprtype_bin *d)
{
    static const char *const paths[] = { "plain", "compare", "cmplq", "mixed" };
    char key[96], f[9][16];
    unsigned char cut[2], lqrq[2], flags[3];
    int h, r, path = -1;

    snprintf(key, sizeof key, "binop\t%s\t%s\t%s\t%d%d%d%d%d%d%d%d", tsvop(op), ett_tname(left), ett_tname(right),
             x->lraw, x->rraw, x->rcmp, x->words, x->lbyte, x->lconst, x->rbyte, x->rconst);
    if (lookup(ctx, key, f, 9) != 9)
        return -1;
    memset(d, 0, sizeof *d);
    for (int i = 0; i < 4; i++)
        if (!strcmp(f[0], paths[i]))
            path = i;
    if (path < 0 || pconv(f[1], &d->rup_from, &d->rup_to) || pbits(f[2], cut, 2)
        || pconv(f[3], &d->lconv_from, &d->lconv_to) || pconv(f[4], &d->rconv_from, &d->rconv_to)
        || pbits(f[5], lqrq, 2) || (h = ptype(f[6])) < 0 || (r = ptype(f[7])) < 0 || pbits(f[8], flags, 3))
        return -1;
    d->path = (unsigned char)path;
    d->lcut = cut[0];
    d->rcut = cut[1];
    d->lq = lqrq[0];
    d->rq = lqrq[1];
    d->handler = (unsigned char)h;
    d->result = (unsigned char)r;
    d->cmpres = flags[0];
    d->wordop = flags[1];
    d->byteext = flags[2];
    return 0;
}

static int t_element(void *ctx, unsigned tsf, unsigned own, const struct exprtype_constfacts *c,
                     struct exprtype_elem *e)
{
    char key[64], f[3][16];
    unsigned char fl[3];
    int r;

    memset(e, 0, sizeof *e);
    if (c) {
        snprintf(key, sizeof key, "const\t%s\t%d%d%d%d%d\t%s", ett_tname(tsf), c->point, c->fix, c->whole, c->data,
                 c->qfit, ett_tname(c->size));
        if (lookup(ctx, key, f, 2) != 2 || (r = ptype(f[0])) < 0 || pbits(f[1], &e->litcut, 1))
            return -1;
        e->result = (unsigned char)r;
        return 0;
    }
    snprintf(key, sizeof key, "element\t%s\t%s", ett_tname(tsf), ett_tname(own));
    if (lookup(ctx, key, f, 3) != 3 || pconv(f[0], &e->conv_from, &e->conv_to) || (r = ptype(f[1])) < 0
        || pbits(f[2], fl, 3))
        return -1;
    e->result = (unsigned char)r;
    e->raw = fl[0];
    e->ext = fl[1];
    e->wordval = fl[2];
    return 0;
}

void ett_tsv_prims(struct ett_tsv *t, struct ett_prims *p)
{
    p->ctx = t;
    p->cutsraw = t_cutsraw;
    p->leftop = t_leftop;
    p->ownleft = t_ownleft;
    p->binop = t_binop;
    p->element = t_element;
}
