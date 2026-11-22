/*
 * classify.c - the class of every measured cell of the expression matrix
 * (OpenSpec change expression-type-matrix, design decision 6; exprmatrix.h,
 * em_classify): the measured cell table against the model (model.c), the
 * cell table with a class per cell, the questions (one per class, design
 * decision 7) and a report of the model's check against 1.95 (task 2.3).
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exprmatrix.h"

static const char *const col_names[EM_NCOLS] = {
    "c210", "c195", "portcd", "clib", "ab3", "c210_on", "c195_on", "portcd_on", "clib_on",
};

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
        size_t cap = b->cap ? b->cap * 2 : 1024;
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

/* ---- the measured table ---- */

const char *em_col_name(int col)
{
    return col >= 0 && col < EM_NCOLS ? col_names[col] : "?";
}

/* a measured value: a plain value (no blank), or !noncompile[:MESSAGE],
   !runtime[:MESSAGE], !outside, !garbage[:VALUE]; normalised to the form
   em_model gives (the message and the garbage value dropped) */
static int measured(const char *v, char *out)
{
    static const char *const marks[] = { "!noncompile", "!runtime", "!outside", "!garbage" };

    if (*v != '!') {
        if (!*v || strlen(v) >= EM_TEXT_MAX || strpbrk(v, " \t"))
            return 0;
        strcpy(out, v);
        return 1;
    }
    for (size_t i = 0; i < sizeof marks / sizeof marks[0]; i++) {
        size_t n = strlen(marks[i]);

        if (!strncmp(v, marks[i], n) && (!v[n] || (v[n] == ':' && i != 2))) {
            strcpy(out, marks[i]);
            return 1;
        }
    }
    return 0;
}

void em_table_free(struct em_table *t)
{
    for (size_t i = 0; i < t->n; i++)
        free(t->rows[i].key);
    free(t->rows);
    t->rows = NULL;
    t->n = 0;
}

int em_table_parse(const char *text, size_t len, const char *file, struct em_table *t, char **errors)
{
    struct buf err = { 0 };
    size_t cap = 0, nerr = 0;
    int line = 0;
    const char *p = text, *end = text + len;

    memset(t, 0, sizeof *t);
    *errors = NULL;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p));
        size_t n = eol ? (size_t)(eol - p) : (size_t)(end - p);
        char *l = malloc(n + 1), *f[2 + EM_NCOLS + 1];
        size_t nf = 0;

        if (!l)
            goto oom;
        memcpy(l, p, n);
        l[n] = 0;
        if (n && l[n - 1] == '\r')
            l[n - 1] = 0;
        p = eol ? eol + 1 : end;
        line++;
        if (line == 1 && !strncmp(l, "key\t", 4)) {
            free(l);
            continue;
        }
        if (!*l) {
            free(l);
            continue;
        }
        for (char *s = l;;) {
            char *tab = strchr(s, '\t');

            if (nf < sizeof f / sizeof f[0])
                f[nf] = s;
            nf++;
            if (!tab)
                break;
            *tab = 0;
            s = tab + 1;
        }
        if (nf != 2 + EM_NCOLS) {
            put(&err, "%s:%d: %zu fields, wanted %d (key, source and %d values)\n", file, line, nf,
                2 + EM_NCOLS, EM_NCOLS);
            nerr++;
            free(l);
            continue;
        }
        {
            struct em_row r;
            int bad = 0;

            memset(&r, 0, sizeof r);
            r.line = line;
            for (int c = 0; c < EM_NCOLS; c++) {
                const char *v = f[2 + c];

                if (!*v && c >= EM_C210_ON) {
                    r.has[c] = 0;
                    continue;
                }
                if (!measured(v, r.v[c])) {
                    put(&err, "%s:%d: %s: bad value \"%s\"\n", file, line, col_names[c], v);
                    nerr++;
                    bad = 1;
                    break;
                }
                r.has[c] = 1;
            }
            if (bad || !*f[0]) {
                if (!bad) {
                    put(&err, "%s:%d: no key\n", file, line);
                    nerr++;
                }
                free(l);
                continue;
            }
            r.key = strdup(f[0]);
            free(l);
            if (!r.key)
                goto oom;
            if (t->n == cap) {
                struct em_row *nr = realloc(t->rows, (cap = cap ? cap * 2 : 1024) * sizeof *nr);

                if (!nr) {
                    free(r.key);
                    goto oom;
                }
                t->rows = nr;
            }
            t->rows[t->n++] = r;
        }
    }
    if (err.oom)
        goto oom;
    if (nerr) {
        *errors = err.s;
        em_table_free(t);
        return 1;
    }
    free(err.s);
    return 0;
oom:
    free(err.s);
    em_table_free(t);
    return -1;
}

/* ---- the classes ---- */

/* the steps from the original's rules to the fixed ones, in the order the
   fixes come; a step that changes the value names its issue */
static const struct {
    unsigned rules;
    const char *issue;
} steps[] = {
    { EM_R_1177 | EM_R_1186, "1177" },
    { EM_R_1178, "1178" },
    { EM_R_1179, "1179" },
    { EM_R_1187, "1187" },
    { EM_R_1188, "1188" },
    { EM_R_1189, "1189" },
    { 0, "lib" },               /* the C libraries: EM_R_CDLIB off */
};
enum { NSTEPS = sizeof steps / sizeof steps[0] };

struct cls {
    const struct em_cell *cell;
    const struct em_row *row;
    char cls[48];               /* agree, baseline, fixed-N, open-N, port, new, question, noncompile */
    char issues[32];            /* "1177+1178" */
    char q;                     /* the question class: 'A'... (0: none) */
    char exp[EM_TEXT_MAX], exp_on[EM_TEXT_MAX];
    char note[256];
    struct em_value orig, orig_on, fixed, fixed_on;
    int chk;                    /* an index cell measured with checking too */
    int base;                   /* the expected value is the original's measured one (a baseline) */
};

static int is_index(const struct em_cell *c)
{
    return c->ctx == EM_I1 || c->ctx == EM_I2;
}

/* a model value equals a measured one ("?": not known, equal to anything) */
static int same(const char *model, const char *meas)
{
    return model[0] == '?' || !strcmp(model, meas);
}

/* the same for a cell: an index cell without its checked values printed
   anything outside the array */
static int same_k(const struct cls *k, const char *model, const char *meas)
{
    if (is_index(k->cell) && !k->chk && !strcmp(model, "!outside") && meas[0] != '!')
        return 1;
    return same(model, meas);
}

static void value(const struct em_matrix *m, const struct em_cell *c, unsigned rules, int checked,
                  struct em_value *v)
{
    em_model(m, c, rules, checked, v);
}

/* the value of the cell by rules as one text (unchecked|checked for an
   index cell) */
static void both(const struct em_matrix *m, const struct em_cell *c, int idx, unsigned rules, char *out, size_t n)
{
    struct em_value u, k;

    value(m, c, rules, 0, &u);
    if (idx) {
        value(m, c, rules, 1, &k);
        snprintf(out, n, "%s|%s", u.text, k.text);
    } else
        snprintf(out, n, "%s", u.text);
}

/* the same with the operands of an unknown power (a step that changes
   what ^ is computed from changes the value) */
static void both_sig(const struct em_matrix *m, const struct em_cell *c, int idx, unsigned rules, char *out,
                     size_t n)
{
    struct em_value u;
    size_t l;

    both(m, c, idx, rules, out, n);
    value(m, c, rules, 0, &u);
    l = strlen(out);
    if (u.text[0] == '?' && l < n)
        snprintf(out + l, n - l, ":%llx", u.sig);
}

static int by_key(const void *a, const void *b)
{
    return strcmp((*(const struct em_cell *const *)a)->key, (*(const struct em_cell *const *)b)->key);
}

static const struct em_cell *find_cell(const struct em_cell *const *idx, size_t n, const char *key)
{
    size_t lo = 0, hi = n;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        int c = strcmp(idx[mid]->key, key);

        if (!c)
            return idx[mid];
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return NULL;
}

/* the cells of #1049 (a folded constant MOD under a Byte destination
   before a Long/Float MOD of the same source: modlib's relocations are
   dropped; the measurement ran the source alone) */
static int is_1049(const struct em_cell *c, const struct em_matrix *m)
{
    (void)m;
    return c->ctx == EM_DB && c->op == EM_MOD;
}

static const char *const qnames[] = {
    ['B'] = "Q-B the two operand orders give different values after variant C",
    ['E'] = "Q-E ^ (power) without a known expected value",
    ['F'] = "Q-F 2.10 differs from 1.95 and no issue explains it",
};

/* a measured value v of cell c, negated in the type of the cell's context
   when neg (EM_R_1189: the sign of an odd power applied to what SPPow
   gives; every conversion after it is symmetric: SPFix towards zero,
   floattoquick, Print's sign; SPFix's saturation outside the Long range
   is not, and the model knows those cells itself, model.c pow_whole) */
static void negated(const struct em_cell *c, const char *v, int neg, char *out, size_t n)
{
    if (!neg || v[0] == '!' || !strcmp(v, "T") || !strcmp(v, "F")) {
        snprintf(out, n, "%s", v);
        return;
    }
    switch (c->ctx) {
    case EM_DQ: case EM_PQ:
        snprintf(out, n, "%08X", (unsigned)(-(uint32_t)strtoul(v, NULL, 16)));
        break;
    case EM_DF: case EM_PF: {
        uint32_t x = (uint32_t)strtoul(v, NULL, 16);

        snprintf(out, n, "%08X", (unsigned)((x & 0xff) ? x ^ 0x80 : x));
        break;
    }
    case EM_PRINT:
        if (v[0] == '-')
            snprintf(out, n, "%s", v + 1);
        else if (!strcmp(v, "0"))
            snprintf(out, n, "%s", v);
        else
            snprintf(out, n, "-%s", v);
        break;
    case EM_DB: snprintf(out, n, "%d", (int)(int8_t)-(int8_t)atoi(v)); break;
    case EM_DW: case EM_PW: snprintf(out, n, "%d", (int)(int16_t)-(int16_t)atoi(v)); break;
    default: snprintf(out, n, "%ld", (long)(int32_t)-(uint32_t)strtol(v, NULL, 10)); break;
    }
}

static void classify_cell(const struct em_matrix *m, struct cls *k)
{
    const struct em_cell *c = k->cell;
    struct em_row rr = *k->row;
    const struct em_row *r = &rr;
    char vals[NSTEPS + 1][2 * EM_TEXT_MAX + 2], port[2 * EM_TEXT_MAX + 2], portcd[2 * EM_TEXT_MAX + 2];
    char o[2 * EM_TEXT_MAX + 2], exp[2 * EM_TEXT_MAX + 2];
    unsigned rules = EM_RULES_ORIG;
    int idx = k->chk;
    unsigned f;

    value(m, c, EM_RULES_ORIG, 0, &k->orig);
    value(m, c, EM_RULES_FIXED, 0, &k->fixed);
    if (idx) {
        value(m, c, EM_RULES_ORIG, 1, &k->orig_on);
        value(m, c, EM_RULES_FIXED, 1, &k->fixed_on);
    }
    if (!strcmp(r->v[EM_C195], "!noncompile")) {
        strcpy(k->cls, "noncompile");
        return;
    }
    if (is_index(c) && !idx && !strcmp(k->fixed.text, "!outside")) {
        /* not measured with checking (block fs): what it printed outside
           the array counts as outside */
        for (int i = EM_C210; i <= EM_AB3; i++)
            if (rr.v[i][0] != '!')
                strcpy(rr.v[i], "!outside");
        snprintf(k->note, sizeof k->note, "outside the array; not measured with checking");
    }
    /* the issues: the steps that change the value */
    both_sig(m, c, idx, rules, vals[0], sizeof vals[0]);
    for (int i = 0; i < NSTEPS; i++) {
        rules = steps[i].rules ? rules | steps[i].rules : rules & ~(unsigned)EM_R_CDLIB;
        both_sig(m, c, idx, rules, vals[i + 1], sizeof vals[i + 1]);
        if (strcmp(vals[i + 1], vals[i])) {
            const char *is = steps[i].issue;

            if (!strcmp(is, "lib"))
                is = c->ctx == EM_PRINT ? "234" : "199";
            if (k->issues[0])
                strncat(k->issues, "+", sizeof k->issues - strlen(k->issues) - 1);
            strncat(k->issues, is, sizeof k->issues - strlen(k->issues) - 1);
        }
    }
    snprintf(k->exp, sizeof k->exp, "%s", k->fixed.text);
    if (idx)
        snprintf(k->exp_on, sizeof k->exp_on, "%s", k->fixed_on.text);
    f = k->fixed.flags | (idx ? k->fixed_on.flags : 0);
    if (is_1049(c, m) && (c->right == EM_L || c->right == EM_F || c->right == EM_CL || c->left == EM_L || c->left == EM_F || c->left == EM_CL))
        snprintf(k->note, sizeof k->note, "#1049 (fixed in the port): a Long/Float MOD after a folded constant MOD of the same source jumps into the compiler's freed buffer in 1.95 and 2.10");
    /* the expected value: from the recorded rules where they cover the
       cell; the original's measured value as the cell's baseline where
       they do not (overflow, bit operators with a Quick or Float, MOD with
       negative or fractional operands, ^): a regression check, so that a
       fix changes these cells only where it is meant to, regenerated after
       the fix; not a decision that the value is right ("Matrix questions
       Q-A, Q-C, Q-D, Q-E", ~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md 2026-11-22, and the
       "Correction by the user" there) */
    if (f & (EM_F_OVF | EM_F_BITQ | EM_F_MODNF | EM_F_POWINT))
        k->base = 1;
    /* a negative base with a fractional exponent: the original's measured
       value as the baseline, #1189 fixes only whole odd exponents
       ("Matrix: last Q-F and Q-E cells", ~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md 2026-11-22:
       "Baseline wie heute (Recommended)"; whether these become a runtime
       error stays open in #1189) */
    if (f & EM_F_POWNEG)
        k->base = 1;
    if (k->fixed.text[0] == '?' || (idx && k->fixed_on.text[0] == '?')) {
        /* ^ with a Quick or Float result: SPPow is not modelled; the same
           Float operation in both rule sets takes the original's measured
           value as its baseline, with the sign of an odd power where #1189
           applies ("?-") */
        if (k->orig.sig == k->fixed.sig && (!idx || k->orig_on.sig == k->fixed_on.sig)) {
            negated(c, r->v[EM_C195], !strcmp(k->fixed.text, "?-"), k->exp, sizeof k->exp);
            if (idx)
                negated(c, r->v[EM_C195_ON], !strcmp(k->fixed_on.text, "?-"), k->exp_on, sizeof k->exp_on);
            k->base = 1;
        } else
            k->q = 'E';
    }
    /* AmiBlitz3 differing from 1.95: the recorded rules decide, AmiBlitz3's
       value is named in docs/ab3/variant.md ("Matrix questions", Q-F,
       2026-11-22: "Unsere Regeln, AB3 dokumentiert (Recommended)", and for
       the cells no issue explains "Matrix: last Q-F and Q-E cells",
       2026-11-22: "Wie die anderen 92 (Recommended)").  2.10 differing
       from 1.95 where no issue explains it stays a question (no such cell
       was measured) */
    if (strcmp(r->v[EM_C210], r->v[EM_C195])
        || (idx && r->has[EM_C210_ON] && strcmp(r->v[EM_C210_ON], r->v[EM_C195_ON]))) {
        if (!k->q && !k->issues[0])
            k->q = 'F';
    }
    if (k->q) {
        strcpy(k->cls, "question");
        return;
    }
    if (idx) {
        snprintf(o, sizeof o, "%.47s|%.47s", r->v[EM_C195], r->v[EM_C195_ON]);
        snprintf(port, sizeof port, "%.47s|%.47s", r->v[EM_CLIB], r->v[EM_CLIB_ON]);
        snprintf(portcd, sizeof portcd, "%.47s|%.47s", r->v[EM_PORTCD], r->v[EM_PORTCD_ON]);
        snprintf(exp, sizeof exp, "%.47s|%.47s", k->exp, k->exp_on);
    } else {
        snprintf(o, sizeof o, "%.47s", r->v[EM_C195]);
        snprintf(port, sizeof port, "%.47s", r->v[EM_CLIB]);
        snprintf(portcd, sizeof portcd, "%.47s", r->v[EM_PORTCD]);
        snprintf(exp, sizeof exp, "%.47s", k->exp);
    }
    if (strstr(k->issues, "1187")) {
        /* the original's value is what d1 holds; the port's measurement was
           made before the fix */
        snprintf(k->cls, sizeof k->cls, "fixed-%s", k->issues);
        if (!k->note[0])
            snprintf(k->note, sizeof k->note, "port measured before the fix of #1187");
        return;
    }
    if (!strcmp(o, exp)) {
        if (!strcmp(port, exp) && !strcmp(portcd, exp))
            strcpy(k->cls, k->base ? "baseline" : "agree");
        else {
            char pm[2 * EM_TEXT_MAX + 2];

            strcpy(k->cls, "port");
            both(m, c, idx, EM_RULES_PORT | EM_R_1186, pm, sizeof pm);
            if (!strcmp(pm, exp) && strcmp(port, exp))
                snprintf(k->note, sizeof k->note, "#1186 (port defect, fixed after the measurement)");
            else
                snprintf(k->note, sizeof k->note, "port %s, portcd %s", port, portcd);
        }
        return;
    }
    if (!k->issues[0]) {
        strcpy(k->cls, "new");
        return;
    }
    if (!strcmp(port, exp))
        snprintf(k->cls, sizeof k->cls, "fixed-%s", k->issues);
    else if (!strcmp(port, o))
        snprintf(k->cls, sizeof k->cls, "open-%s", k->issues);
    else {
        char pm[2 * EM_TEXT_MAX + 2];

        both(m, c, idx, EM_RULES_PORT | EM_R_1186, pm, sizeof pm);
        if (!strcmp(pm, exp)) {
            snprintf(k->cls, sizeof k->cls, "fixed-%s", k->issues);
            snprintf(k->note, sizeof k->note, "#1186 (port defect, fixed after the measurement)");
        } else {
            snprintf(k->cls, sizeof k->cls, "open-%s", k->issues);
            snprintf(k->note, sizeof k->note, "port %s differs from 1.95 and the expected value", port);
        }
    }
    if (strcmp(portcd, port) && !k->note[0])
        snprintf(k->note, sizeof k->note, "portcd %s", portcd);
}

/* the mirror of a cell: the operands swapped (a comparison mirrored) */
static int mirror_key(const struct em_cell *c, const struct em_matrix *m, char *key, size_t n)
{
    static const enum em_op mir[EM_NOPS] = {
        [EM_ADD] = EM_ADD, [EM_MUL] = EM_MUL, [EM_AND] = EM_AND, [EM_OR] = EM_OR, [EM_EOR] = EM_EOR,
        [EM_EQ] = EM_EQ, [EM_NE] = EM_NE, [EM_LT] = EM_GT, [EM_GT] = EM_LT, [EM_LE] = EM_GE, [EM_GE] = EM_LE,
    };
    enum em_op o = mir[c->op];

    if (c->right < 0 || c->right == (int)c->left || (!o && c->op != EM_ADD))
        return 0;
    if (c->op != EM_ADD && !o)
        return 0;
    snprintf(key, n, "%s-%s-%s-%s-%s", em_ctx_name(c->ctx), em_op_name(o), em_kind_name((enum em_kind)c->right),
             em_kind_name(c->left), m->sets[c->set].name);
    return 1;
}

static void tsv_line(struct buf *t, const struct cls *k)
{
    const struct em_row *r = k->row;

    put(t, "%s\t%s\t%s\t%s%s%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", k->cell->key, k->cls, k->issues,
        k->q ? "Q-" : "", k->q ? (char[2]){ k->q, 0 } : "", "", k->exp, k->exp_on, r->v[EM_C210], r->v[EM_C195],
        r->v[EM_AB3], r->v[EM_CLIB], k->note);
}

/* three example cells of a question class, spread over the class */
static void examples(struct buf *q, const struct cls *ks, size_t n, char cls, size_t count)
{
    size_t got = 0, step = count > 3 ? count / 3 : 1, seen = 0;

    put(q, "| cell | 2.10 | 1.95 | AmiBlitz3 | port | model (fixed rules) | issues |\n|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < n && got < 3; i++) {
        const struct cls *k = &ks[i];

        if (k->q != cls)
            continue;
        if (seen++ % step)
            continue;
        put(q, "| `%s` | %s | %s | %s | %s | %s | %s |\n", k->cell->key, k->row->v[EM_C210], k->row->v[EM_C195],
            k->row->has[EM_AB3] ? k->row->v[EM_AB3] : "-", k->row->v[EM_CLIB], k->fixed.text,
            k->issues[0] ? k->issues : "-");
        got++;
    }
}

/* the questions still open; the classes Q-A, Q-C and Q-D are baselines
   since the answers of 2026-11-22 (classify_cell) */
static const char *const qtext[] = {
    ['B'] = "The two operand orders of a commutative operator (or a mirrored comparison) give different "
            "expected values after variant C (\"Reihenfolge wie heute reicht\"; the goal \"base+q*4 muss das "
            "gleiche ergeben wie q*4+base\" is the expression tree's, #1180). The one recorded datum is for "
            "three operands (\"#1178 Q1\": `NPrint 1+1+1+q` 4 until the tree, 4.5 after it).\n\n"
            "Options: (a) variant C's value of each order is the expected one until #1180, the tree's value "
            "recorded for #1180 (recommended, as for #1178 Q1); (b) the cells are left out until #1180; "
            "(c) an issue of its own.\n",
    ['E'] = "^ (power) whose operands a fix changes (#1178, #1188), so that SPPow's result is not "
            "measured (the model does not compute SPPow). Left out of the committed tests.\n\n"
            "Options: (a) left out until the fixes of #1178 and #1188 are measured (recommended); (b) the "
            "power of the fixed operands computed by the model.\n",
    ['F'] = "2.10 gives another value than 1.95, and no issue of the model's columns explains the cell "
            "(where AmiBlitz3 differs, the recorded rules decide and its value is named in "
            "docs/ab3/variant.md). Left out of the committed tests until answered.\n\n"
            "Options: (a) 1.95's value as the baseline; (b) 2.10's value; (c) left out of all tests.\n",
};

/* the class of every row of t (the cells, then Q-B); *ks_out the rows'
   classes, *idx_out the cells by key (both malloc'd); 0, 1 with the
   errors in *errors, -1 out of memory */
static int run_classes(const struct em_matrix *m, const struct em_table *t, struct cls **ks_out,
                       const struct em_cell ***idx_out, char **errors)
{
    const struct em_cell **idx = NULL;
    struct cls *ks = NULL;
    struct buf err = { 0 };
    size_t nerr = 0;
    char *seen = NULL;

    *errors = NULL;
    *ks_out = NULL;
    *idx_out = NULL;
    idx = malloc((m->ncells ? m->ncells : 1) * sizeof *idx);
    ks = calloc(t->n ? t->n : 1, sizeof *ks);
    seen = calloc(m->ncells ? m->ncells : 1, 1);
    if (!idx || !ks || !seen)
        goto oom;
    for (size_t i = 0; i < m->ncells; i++)
        idx[i] = &m->cells[i];
    qsort(idx, m->ncells, sizeof *idx, by_key);
    for (size_t i = 0; i < t->n; i++) {
        const struct em_cell *c = find_cell(idx, m->ncells, t->rows[i].key);

        if (!c) {
            put(&err, "line %d: %s: not a cell of the description\n", t->rows[i].line, t->rows[i].key);
            nerr++;
            continue;
        }
        if (seen[c - m->cells]++) {
            put(&err, "line %d: %s: measured twice\n", t->rows[i].line, t->rows[i].key);
            nerr++;
            continue;
        }
        if (is_index(c) && (t->rows[i].has[EM_C195_ON] + t->rows[i].has[EM_CLIB_ON] + t->rows[i].has[EM_PORTCD_ON]) % 3) {
            put(&err, "line %d: %s: an index cell with only some of its checked values\n", t->rows[i].line, c->key);
            nerr++;
            continue;
        }
        ks[i].cell = c;
        ks[i].row = &t->rows[i];
        ks[i].chk = is_index(c) && t->rows[i].has[EM_C195_ON];
    }
    for (size_t i = 0; i < m->ncells; i++)
        if (!seen[i]) {
            put(&err, "%s: not measured\n", m->cells[i].key);
            nerr++;
        }
    free(seen);
    seen = NULL;
    if (nerr) {
        free(idx);
        free(ks);
        if (err.oom) {
            free(err.s);
            return -1;
        }
        *errors = err.s;
        return 1;
    }
    for (size_t i = 0; i < t->n; i++)
        classify_cell(m, &ks[i]);
    /* Q-B: the two orders */
    {
        struct cls **byc = calloc(m->ncells ? m->ncells : 1, sizeof *byc);
        char mk[EM_KEY_MAX];

        if (!byc)
            goto oom;
        for (size_t i = 0; i < t->n; i++)
            byc[ks[i].cell - m->cells] = &ks[i];
        for (size_t i = 0; i < t->n; i++) {
            struct cls *k = &ks[i], *o;
            const struct em_cell *mc;

            if (strcmp(k->cls, "noncompile") == 0 || k->q || !mirror_key(k->cell, m, mk, sizeof mk))
                continue;
            mc = find_cell(idx, m->ncells, mk);
            if (!mc)
                continue;
            o = byc[mc - m->cells];
            if (!o || !strcmp(o->cls, "noncompile") || (o->q && o->q != 'B'))
                continue;
            /* a baseline cell on either side: the original's order
               dependence is part of its baseline */
            if (k->base || o->base)
                continue;
            if (strcmp(k->exp, o->exp) || strcmp(k->exp_on, o->exp_on)) {
                k->q = o->q = 'B';
                strcpy(k->cls, "question");
                strcpy(o->cls, "question");
            }
        }
        free(byc);
    }
    free(err.s);
    *ks_out = ks;
    *idx_out = idx;
    return 0;
oom:
    free(idx);
    free(ks);
    free(seen);
    free(err.s);
    return -1;
}

int em_classify(const struct em_matrix *m, const struct em_table *t, struct em_classified *out, char **errors)
{
    const struct em_cell **idx = NULL;
    struct cls *ks = NULL;
    struct buf tab = { 0 }, q = { 0 }, rep = { 0 };
    size_t counts_q[128] = { 0 };
    int rc;

    memset(out, 0, sizeof *out);
    rc = run_classes(m, t, &ks, &idx, errors);
    if (rc)
        return rc;
    /* the table, the counts, the model's check against 1.95 */
    put(&tab, "key\tclass\tissues\tquestion\texpected\texpected_on\tc210\tc195\tab3\tclib\tnote\n");
    {
        size_t nclass = 0;
        struct { char name[48]; size_t n; } classes[64];
        size_t mism = 0, pmism = 0, unknown = 0;
        struct buf mlist = { 0 }, plist = { 0 };

        for (size_t i = 0; i < t->n; i++) {
            struct cls *k = &ks[i];
            const struct em_row *r = k->row;
            size_t j;

            tsv_line(&tab, k);
            if (k->q)
                counts_q[(int)k->q]++;
            for (j = 0; j < nclass; j++)
                if (!strcmp(classes[j].name, k->cls))
                    break;
            if (j == nclass && nclass < 64) {
                snprintf(classes[nclass].name, sizeof classes[nclass].name, "%s", k->cls);
                classes[nclass++].n = 0;
            }
            if (j < nclass)
                classes[j].n++;
            /* the original's rules against 1.95, the port's against the port */
            {
                struct em_value pv;
                int bad = !same_k(k, k->orig.text, r->v[EM_C195]);

                if (k->chk)
                    bad |= !same_k(k, k->orig_on.text, r->v[EM_C195_ON]);
                if (!strcmp(k->orig.text, "?") || (k->chk && !strcmp(k->orig_on.text, "?")))
                    unknown++;
                if (bad) {
                    mism++;
                    put(&mlist, "%s\tmodel %s%s%s\t1.95 %s%s%s\n", k->cell->key, k->orig.text,
                        k->chk ? "|" : "", k->chk ? k->orig_on.text : "",
                        r->v[EM_C195], k->chk ? "|" : "", k->chk ? r->v[EM_C195_ON] : "");
                }
                for (int cd = 0; cd < 2; cd++) {
                    int col = cd ? EM_PORTCD : EM_CLIB, con = cd ? EM_PORTCD_ON : EM_CLIB_ON;

                    value(m, k->cell, cd ? EM_RULES_PORTCD : EM_RULES_PORT, 0, &pv);
                    bad = !same_k(k, pv.text, r->v[col]);
                    if (k->chk) {
                        struct em_value pk;

                        value(m, k->cell, cd ? EM_RULES_PORTCD : EM_RULES_PORT, 1, &pk);
                        bad |= !same_k(k, pk.text, r->v[con]);
                    }
                    if (bad) {
                        pmism++;
                        put(&plist, "%s\t%s\tmodel %s\tmeasured %s\n", k->cell->key, col_names[col], pv.text,
                            r->v[col]);
                    }
                }
            }
        }
        put(&rep, "cells %zu\n", t->n);
        for (size_t j = 0; j < nclass; j++)
            put(&rep, "class %s %zu\n", classes[j].name, classes[j].n);
        for (int c = 'A'; c <= 'F'; c++)
            put(&rep, "question Q-%c %zu\n", c, counts_q[c]);
        put(&rep, "not modelled (d1 of #1187, SPPow) %zu\n", unknown);
        put(&rep, "mismatches (original rules against 1.95) %zu\n", mism);
        if (mlist.s)
            put(&rep, "%s", mlist.s);
        put(&rep, "mismatches (the port's rules against the port) %zu\n", pmism);
        if (plist.s)
            put(&rep, "%s", plist.s);
        out->mismatches = mism;
        out->port_mismatches = pmism;
        free(mlist.s);
        free(plist.s);
    }
    /* the questions */
    put(&q, "# Expression matrix: questions\n\n"
            "Generated by `exprmatrix classify` (OpenSpec change expression-type-matrix, design decision 7): "
            "one question per class of cells that no recorded rule covers, with three example cells (the "
            "values of 2.10, 1.95, AmiBlitz3 and the port with the C libraries, and the model's value by "
            "the fixed rules). The cell table lists every cell of a class.\n");
    for (int c = 'A'; c <= 'F'; c++) {
        if (!counts_q[c])
            continue;
        put(&q, "\n## %s (%zu cells)\n\n%s\n", qnames[c], counts_q[c], qtext[c]);
        /* the issues the model names for the cells of the class */
        {
            struct { char is[32]; size_t n; } by[32];
            size_t nby = 0;

            for (size_t i = 0; i < t->n; i++) {
                size_t j;
                const char *is = ks[i].issues[0] ? ks[i].issues : "none";

                if (ks[i].q != c)
                    continue;
                for (j = 0; j < nby && strcmp(by[j].is, is); j++)
                    ;
                if (j == nby && nby < 32) {
                    snprintf(by[nby].is, sizeof by[nby].is, "%s", is);
                    by[nby++].n = 0;
                }
                if (j < nby)
                    by[j].n++;
            }
            put(&q, "Cells by the issues whose fixed rules change their value:");
            for (size_t j = 0; j < nby; j++)
                put(&q, "%s %s%s %zu", j ? "," : "", strcmp(by[j].is, "none") ? "#" : "", by[j].is, by[j].n);
            put(&q, ".\n\n");
        }
        examples(&q, ks, t->n, (char)c, counts_q[c]);
    }
    if (tab.oom || q.oom || rep.oom)
        goto oom;
    out->table = tab.s;
    out->questions = q.s;
    out->report = rep.s;
    free(idx);
    free(ks);
    return 0;
oom:
    free(idx);
    free(ks);
    free(tab.s);
    free(q.s);
    free(rep.s);
    return -1;
}

void em_classified_free(struct em_classified *c)
{
    free(c->table);
    free(c->questions);
    free(c->report);
    memset(c, 0, sizeof *c);
}

int em_classes(const struct em_matrix *m, const struct em_table *t, struct em_class **out, char **errors)
{
    const struct em_cell **idx = NULL;
    struct cls *ks = NULL;
    struct em_class *c;
    int rc;

    *out = NULL;
    rc = run_classes(m, t, &ks, &idx, errors);
    if (rc)
        return rc;
    c = calloc(t->n ? t->n : 1, sizeof *c);
    if (!c) {
        free(idx);
        free(ks);
        return -1;
    }
    for (size_t i = 0; i < t->n; i++) {
        c[i].cell = ks[i].cell;
        memcpy(c[i].cls, ks[i].cls, sizeof c[i].cls);
        memcpy(c[i].issues, ks[i].issues, sizeof c[i].issues);
        c[i].q = ks[i].q;
        c[i].base = ks[i].base;
        c[i].chk = ks[i].chk;
        memcpy(c[i].exp, ks[i].exp, sizeof c[i].exp);
        memcpy(c[i].exp_on, ks[i].exp_on, sizeof c[i].exp_on);
        if (!strcmp(ks[i].cls, "noncompile")) {
            /* refused by 1.95: the fixed rules' value (refused too, or
               what a fix makes of it) */
            memcpy(c[i].exp, ks[i].fixed.text, sizeof c[i].exp);
            if (ks[i].chk)
                memcpy(c[i].exp_on, ks[i].fixed_on.text, sizeof c[i].exp_on);
        }
    }
    free(idx);
    free(ks);
    *out = c;
    return 0;
}
