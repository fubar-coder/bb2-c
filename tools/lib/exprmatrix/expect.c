/*
 * expect.c - the committed tests of the expression matrix (OpenSpec change
 * expression-type-matrix, task 3.1; exprmatrix.h, em_expect): from the
 * description, the model and the few measured values the model cannot
 * compute (em_measured), the corpus sources with their expected files,
 * the bug-orig tests of the open cells, the tests of the refused cells and
 * the list of the cells left out.
 *
 * Every cell is looked at in its modes: a cell of a destination,
 * parameter, If or Print context in one ('v'), an index cell unchecked
 * ('u', only where every compiler reads an element of the array) and
 * checked ('c', runtime error checking on; a cell outside the array is
 * the runtime error, caught by SetErr).  In a mode the original's value o,
 * the expected value e and the port's value p (the port of this commit,
 * EM_RULES_PORTNOW) give the cell's class there: o = e = p agreement (or
 * baseline: the original's measured value where no recorded rule covers
 * the cell), o != e = p fixed by its issue, o = p != e open, anything
 * else left out (a port defect, reported).  The cells of an issue fixed
 * after the matrix was committed (its rule outside EM_RULES_MATRIX: #1178
 * and later) stay in their bug-orig tests when the fix comes (option T1 of
 * quick-long-arithmetic, design decision 13; task-decisions.md 2026-11-22,
 * "T1 for all fixes"): o != e = p gives the open test again, with
 * "; @fixed #N" in its header; a cell of two such issues of which one is
 * fixed stays in its test without @fixed (DIFFERS until the second fix).
 * A source holds the cells of
 * one context, value set, mode and class (design decision 2b), at most
 * cells-per-source; its foldable constant MOD cells come last (#1049: a
 * folded constant MOD as modlib's first use drops its relocations, and a
 * Long or Float MOD after it crashes the originals).
 *
 * The agreement and fixed sources of a destination, parameter, If or Print
 * context and the unchecked index sources are also AmiBlitz3 cases (task
 * 3.5, design decision 9): NAME.ab3, the same text without the cells
 * AmiBlitz3 refuses, NAME.expect the output of blitz2-ab3 in AB3 mode (the
 * port's compiler, one core, with AmiBlitz3's libraries, which have the
 * CD's #199 and #234: the port's value with the CD's libraries) and
 * NAME.ref.expect AmiBlitz3's measured output where it differs, listed in
 * cases.cmake for tests/ab3.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exprmatrix.h"

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

/* ---------------------------------------------------------- the places */

#define DIR_SRC "tests/corpus/tests-exprmatrix/"
#define DIR_EXP "tests/corpus/expect/corpus_exprmatrix/"
#define DIR_EXPR "tests/corpus/expect/corpus_exprmatrix_runerr/"
#define DIR_BUG "tests/bug-orig/compiler/"
#define DIR_REG "tests/bug-orig/regression/"
#define DIR_AB3 "tests/ab3/cli/exprmatrix/"
#define LEFT_OUT DIR_SRC "left-out.txt"
#define AB3_CASES DIR_AB3 "cases.cmake"

static int ends(const char *s, const char *suffix)
{
    size_t n = strlen(s), m = strlen(suffix);

    return n >= m && !strcmp(s + n - m, suffix);
}

static int under(const char *path, const char *dir, const char **rest)
{
    size_t n = strlen(dir);

    if (strncmp(path, dir, n) || !path[n] || strchr(path + n, '/'))
        return 0;
    *rest = path + n;
    return 1;
}

int em_expect_owns(const char *path)
{
    const char *r;

    if (under(path, DIR_SRC, &r))
        return ends(r, ".bas") || !strcmp(path, LEFT_OUT);
    if (under(path, DIR_EXP, &r) || under(path, DIR_EXPR, &r))
        return r[0] != '.';
    if (under(path, DIR_BUG, &r))
        return !strncmp(r, "compiler-expr-", 14) && r[14] >= '1' && r[14] <= '9'
               && (ends(r, ".bas") || ends(r, ".expect"));
    if (under(path, DIR_REG, &r))
        return !strncmp(r, "compiler-expr-refused-", 22) && (ends(r, ".bas") || ends(r, ".expect"));
    if (under(path, DIR_AB3, &r))
        return r[0] != '.';
    return 0;
}

/* ------------------------------------------------- the measured values */

static int is_index(const struct em_cell *c)
{
    return c->ctx == EM_I1 || c->ctx == EM_I2;
}

/* an index cell with checked values (the string cells were measured
   unchecked only) */
static int has_checked(const struct em_cell *c)
{
    return is_index(c) && c->left != EM_S;
}

/* the rule sets whose value a row of the measured table may have to give */
static const unsigned want_rules[] = { EM_RULES_ORIG, EM_RULES_FIXED, EM_RULES_PORTNOW,
                                       EM_RULES_PORTNOW | EM_R_CDLIB };

static int model_unknown(const struct em_matrix *m, const struct em_cell *c)
{
    for (size_t i = 0; i < sizeof want_rules / sizeof want_rules[0]; i++)
        for (int chk = 0; chk <= has_checked(c); chk++) {
            struct em_value v;

            em_model(m, c, want_rules[i], chk, &v);
            if (v.text[0] == '?')
                return 1;
        }
    return 0;
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

static const char *const header =
    "key\tsource\tc210\tc195\tportcd\tclib\tab3\tc210_on\tc195_on\tportcd_on\tclib_on\n";

char *em_measured(const struct em_matrix *m, const struct em_table *t)
{
    const struct em_cell **idx = malloc((m->ncells ? m->ncells : 1) * sizeof *idx);
    struct buf b = { 0 };

    if (!idx)
        return NULL;
    for (size_t i = 0; i < m->ncells; i++)
        idx[i] = &m->cells[i];
    qsort(idx, m->ncells, sizeof *idx, by_key);
    put(&b, "%s", header);
    for (size_t i = 0; i < t->n; i++) {
        const struct em_row *r = &t->rows[i];
        const struct em_cell *c = find_cell(idx, m->ncells, r->key);

        if (!c)
            continue;
        if (!model_unknown(m, c)
            && !(r->has[EM_AB3] && strcmp(r->v[EM_AB3], r->v[EM_C195])))
            continue;
        put(&b, "%s\t-", r->key);
        for (int k = 0; k < EM_NCOLS; k++)
            put(&b, "\t%s", r->has[k] ? r->v[k] : "");
        put(&b, "\n");
    }
    free(idx);
    if (b.oom) {
        free(b.s);
        return NULL;
    }
    return b.s;
}

/* ---------------------------------------------------- the files */

void em_files_free(struct em_file *f, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        free(f[i].path);
        free(f[i].text);
    }
    free(f);
}

struct out {
    struct em_file *f;
    size_t n, cap;
    int oom;
};

/* adds a file (takes text) */
static void add_file(struct out *o, char *path, char *text)
{
    if (o->oom || !path || !text) {
        free(path);
        free(text);
        o->oom = 1;
        return;
    }
    if (o->n == o->cap) {
        size_t cap = o->cap ? o->cap * 2 : 256;
        struct em_file *f = realloc(o->f, cap * sizeof *f);

        if (!f) {
            free(path);
            free(text);
            o->oom = 1;
            return;
        }
        o->f = f;
        o->cap = cap;
    }
    o->f[o->n].path = path;
    o->f[o->n].text = text;
    o->n++;
}

static char *str_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static char *str_printf(const char *fmt, ...)
{
    va_list ap;
    int n;
    char *s;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1)))
        return NULL;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}

/* ------------------------------------------------- the cells by mode */

enum kind_of {
    K_CORPUS,                   /* agreement, baseline, fixed: a corpus source */
    K_OPEN,                     /* open: a bug-orig test */
    K_REFUSED,                  /* refused by the original and the port: one representative */
    K_OPEN_REFUSED              /* refused after a fix: one representative, a bug-orig test */
};

struct ent {
    const struct em_cell *c;
    char o[EM_TEXT_MAX], e[EM_TEXT_MAX];
    char a[EM_TEXT_MAX];        /* AmiBlitz3's value (unchecked only; "" in mode c) */
    char pc[EM_TEXT_MAX];       /* the port's value with the CD's libraries */
};

struct group {
    enum kind_of kind;
    char mode;                  /* 'v', 'u', 'c' */
    enum em_ctx ctx;
    size_t set;
    char tag[48];               /* agree, base, fixed-N, open-N; refused: OP-L-R */
    char issues[32];
    struct ent *e;
    size_t n, cap;
};

/* the operand class of a refused cell: I a whole number, Q, F, C (a
   fraction constant), S */
static char kind_class(int k)
{
    switch (k) {
    case EM_Q: return 'Q';
    case EM_F: return 'F';
    case EM_CF: return 'C';
    case EM_S: return 'S';
    default: return 'I';
    }
}

static int is_const(int k)
{
    return k == EM_CB || k == EM_CW || k == EM_CL || k == EM_CF;
}

/* a MOD the original folds: both operands constants and the operation
   Byte, Word or Quick (a Long or Float MOD is not folded).  #1049: such a
   MOD as the first use of modlib drops modlib's relocations and a Long or
   Float MOD after it crashes the originals, so these go last */
static int folded_mod(const struct em_matrix *m, const struct em_cell *c, int checked)
{
    struct em_value v;

    if (c->op != EM_MOD || !is_const(c->left) || c->right < 0 || !is_const(c->right))
        return 0;
    em_model(m, c, EM_RULES_ORIG, checked, &v);
    return !(v.flags & EM_F_MODLF);
}

struct groups {
    struct group *g;
    size_t n, cap;
    int oom;
};

static struct group *group_of(struct groups *gs, enum kind_of kind, char mode, const struct em_cell *c,
                              const char *tag, const char *issues)
{
    for (size_t i = 0; i < gs->n; i++) {
        struct group *g = &gs->g[i];

        if (g->kind == kind && g->mode == mode && !strcmp(g->tag, tag) && !strcmp(g->issues, issues)
            && (kind == K_REFUSED || kind == K_OPEN_REFUSED || (g->ctx == c->ctx && g->set == c->set)))
            return g;
    }
    if (gs->n == gs->cap) {
        size_t cap = gs->cap ? gs->cap * 2 : 64;
        struct group *g = realloc(gs->g, cap * sizeof *g);

        if (!g) {
            gs->oom = 1;
            return NULL;
        }
        gs->g = g;
        gs->cap = cap;
    }
    struct group *g = &gs->g[gs->n++];

    memset(g, 0, sizeof *g);
    g->kind = kind;
    g->mode = mode;
    g->ctx = c->ctx;
    g->set = c->set;
    snprintf(g->tag, sizeof g->tag, "%s", tag);
    snprintf(g->issues, sizeof g->issues, "%s", issues);
    return g;
}

static void group_add(struct groups *gs, struct group *g, const struct em_cell *c, const char *o, const char *e,
                      const char *a, const char *pc)
{
    if (!g)
        return;
    if (g->n == g->cap) {
        size_t cap = g->cap ? g->cap * 2 : 64;
        struct ent *n = realloc(g->e, cap * sizeof *n);

        if (!n) {
            gs->oom = 1;
            return;
        }
        g->e = n;
        g->cap = cap;
    }
    g->e[g->n].c = c;
    snprintf(g->e[g->n].o, EM_TEXT_MAX, "%s", o);
    snprintf(g->e[g->n].e, EM_TEXT_MAX, "%s", e);
    snprintf(g->e[g->n].a, EM_TEXT_MAX, "%s", a);
    snprintf(g->e[g->n].pc, EM_TEXT_MAX, "%s", pc);
    g->n++;
}

/* a printed value or a runtime error caught by SetErr (mode c) */
static int printable(const char *v, char mode)
{
    if (v[0] != '!')
        return v[0] != '?';
    return mode == 'c' && !strcmp(v, "!runtime");
}

/* ------------------------------------------------------------- texts */

static const struct {
    const char *issue, *bug;
} bugs[] = {
    { "1177", "a comparison is computed in the type so far of its context, not with its operands' own values" },
    { "1178", "a Quick fetched while the type so far is Long is cut to a whole number at once" },
    { "1179", "a later array index is computed as a Word without runtime error checking" },
    { "1187", "a Float condition tests d1, what the code before left there, not the value" },
    { "1188", "a constant with a fraction is cut to a whole number when the type so far is Byte" },
    { "1189", "^ with a negative base drops the sign of the power" },
    { "199", "the CD's quickmult adds the high words' product beyond the Quick range" },
    { "234", "the CD's printlib prints a Quick that rounds to 0 as \"-.\"" },
};

static const char *bug_text(const char *issue)
{
    for (size_t i = 0; i < sizeof bugs / sizeof bugs[0]; i++)
        if (!strcmp(bugs[i].issue, issue))
            return bugs[i].bug;
    return NULL;
}

/* the rule of an issue of the model (0: none, a library issue) */
static unsigned issue_rule(const char *issue)
{
    static const struct {
        const char *issue;
        unsigned rule;
    } rules[] = {
        { "1177", EM_R_1177 }, { "1178", EM_R_1178 }, { "1179", EM_R_1179 },
        { "1187", EM_R_1187 }, { "1188", EM_R_1188 }, { "1189", EM_R_1189 },
    };

    for (size_t i = 0; i < sizeof rules / sizeof rules[0]; i++)
        if (!strcmp(rules[i].issue, issue))
            return rules[i].rule;
    return 0;
}

/* issues ("1178+1189") all fixed after the matrix was committed (their
   rules outside EM_RULES_MATRIX): returns 1 with *nfixed the number of
   them in the port's rules port and *n their number; 0 otherwise */
static int late_issues(const char *issues, unsigned port, int *nfixed, int *n)
{
    char tmp[32];
    char *p, *save = NULL;

    *nfixed = *n = 0;
    if (!issues[0])
        return 0;
    snprintf(tmp, sizeof tmp, "%s", issues);
    for (p = strtok_r(tmp, "+", &save); p; p = strtok_r(NULL, "+", &save)) {
        unsigned r = issue_rule(p);

        if (!r || (r & EM_RULES_MATRIX))
            return 0;
        ++*n;
        if (r & port)
            ++*nfixed;
    }
    return 1;
}

/* "; @fixed #N" for a test of issues fixed after the matrix, all of them
   in the port's rules: N the last of them (the order of the model's steps,
   the order the fixes are planned in) */
static void put_fixed(struct buf *b, const char *issues, unsigned port)
{
    int nf, n;
    const char *last;

    if (!late_issues(issues, port, &nf, &n) || nf != n)
        return;
    last = strrchr(issues, '+');
    put(b, "; @fixed #%s\n", last ? last + 1 : issues);
}

/* "1178+1189" -> "#1178 and #1189" */
static void issue_list(char *out, size_t n, const char *issues)
{
    char tmp[32];
    char *p, *save = NULL;
    size_t k = 0, count = 0;

    snprintf(tmp, sizeof tmp, "%s", issues);
    for (p = strtok_r(tmp, "+", &save); p; p = strtok_r(NULL, "+", &save))
        count++;
    snprintf(tmp, sizeof tmp, "%s", issues);
    out[0] = 0;
    for (p = strtok_r(tmp, "+", &save); p; p = strtok_r(NULL, "+", &save)) {
        k++;
        snprintf(out + strlen(out), n - strlen(out), "%s#%s", k == 1 ? "" : k == count ? " and " : ", ", p);
    }
}

static void put_wrapped(struct buf *b, const char *prefix, const char *text)
{
    const char *p = text;

    while (*p) {
        const char *end = p, *brk = NULL;
        size_t width = 72 - strlen(prefix);

        while (*end && (size_t)(end - p) <= width) {
            if (*end == ' ')
                brk = end;
            end++;
        }
        if (!*end)
            brk = end;
        else if (!brk)
            brk = end;
        put(b, "%s%.*s\n", prefix, (int)(brk - p), p);
        p = *brk ? brk + 1 : brk;
    }
}

static const char *mode_text(char mode)
{
    return mode == 'c' ? ", runtime error checking on" : mode == 'u' ? ", runtime error checking off" : "";
}

/* ---------------------------------------------------------- em_expect */

struct ctx {
    const struct em_matrix *m;
    const char *desc;
    unsigned port;              /* the port's rules (EM_RULES_PORTNOW) */
    struct out out;
    struct buf left, rep, err;
    size_t nerr;
    size_t counts[8];           /* cells per kind_of, and left out (4) */
    size_t files_kind[4];
    struct buf ab3;             /* the AmiBlitz3 cases (cases.cmake) */
    size_t ab3_cases, ab3_cells, ab3_refused, ab3_d1, ab3_ref, ab3_differ;
};

/* the cells of a group in the order they are written: the folded
   constant MOD cells last */
static void order(const struct em_matrix *m, struct group *g)
{
    size_t k = 0;
    struct ent *tmp = malloc((g->n ? g->n : 1) * sizeof *tmp);

    if (!tmp)
        return;
    for (int pass = 0; pass < 2; pass++)
        for (size_t i = 0; i < g->n; i++)
            if (folded_mod(m, g->e[i].c, g->mode == 'c') == pass)
                tmp[k++] = g->e[i];
    memcpy(g->e, tmp, g->n * sizeof *tmp);
    free(tmp);
}

static void lines(struct buf *b, const struct ent *e, size_t n, int orig)
{
    for (size_t i = 0; i < n; i++)
        put(b, "%s %s\n", e[i].c->key, orig ? e[i].o : e[i].e);
}

static int all_same(const struct ent *e, size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (strcmp(e[i].o, e[i].e))
            return 0;
    return 1;
}

static char *text_of(struct buf *b)
{
    if (b->oom) {
        free(b->s);
        return NULL;
    }
    return b->s ? b->s : strdup("");
}

/* the source of the cells with the header head, every comment line of it
   longer than 96 characters wrapped (TED takes lines of at most 128);
   malloc'd, NULL out of memory */
static char *source_of(const struct em_matrix *m, const struct em_cell *const *cells, size_t n, struct buf *head,
                       int seterr, int *line_of)
{
    struct buf b = { 0 };
    const char *p = head->s;
    char *src;

    if (head->oom || !p)
        return NULL;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);

        if (len > 96 && !strncmp(p, "; ", 2)) {
            char *line = strndup(p + 2, len - 2);

            if (!line)
                b.oom = 1;
            else
                put_wrapped(&b, "; ", line);
            free(line);
        } else
            put(&b, "%.*s\n", (int)len, p);
        p += len + (eol ? 1 : 0);
    }
    if (b.oom) {
        free(b.s);
        return NULL;
    }
    src = em_source_text(m, cells, n, b.s, seterr, line_of);
    free(b.s);
    return src;
}

/* an If cell that AmiBlitz3 folds to a Float constant: arithmetic or a
   unary minus of constants, one with a fraction (AmiBlitz3 keeps it, as the
   fix of #1188).  Its condition is a Float, which AmiBlitz3 tests in d1
   without loading it there (#1187), and a folded constant leaves d1 as the
   line before left it: the value depends on the cells before it in the
   source (measured: if-div-cb-cf-A F in the measurement, T in its corpus
   source), so it is no value of the cell */
static int ab3_d1(const struct em_cell *c)
{
    if (c->ctx != EM_IF || (c->op > EM_POW && c->op != EM_NEG))
        return 0;
    return is_const(c->left) && (c->right < 0 || is_const(c->right)) && (c->left == EM_CF || c->right == EM_CF);
}

/* the AmiBlitz3 case of the corpus source name (task 3.5): the cells
   AmiBlitz3 compiles and prints, the output of blitz2-ab3 in AB3 mode
   (the port's values with the CD's libraries: AB3 mode links AmiBlitz3's
   libraries, measured with the same values in the cells of #199 and
   #234) and, where AmiBlitz3's measured values differ, its own expected
   file; compiled with -r (runtime error checking off, as the corpus
   source: AB3 mode's own default is on) */
static void emit_ab3(struct ctx *x, struct group *g, const char *name, const struct ent *e, size_t n)
{
    const struct em_matrix *m = x->m;
    const struct em_cell **cells = malloc((n ? n : 1) * sizeof *cells);
    struct buf head = { 0 }, ex = { 0 }, ref = { 0 };
    size_t k = 0, refused = 0, differ = 0, d1 = 0;
    char *src;

    if (!cells) {
        x->out.oom = 1;
        return;
    }
    put(&ex, "[compile]\nrc=0\n[run]\n");
    put(&ref, "[compile]\nrc=0\n[run]\n");
    for (size_t i = 0; i < n; i++) {
        /* refused by AmiBlitz3 ("Illegal operator for this type"): left
           out of its variant, as the measurement did */
        if (e[i].a[0] == '!' || !e[i].a[0]) {
            refused++;
            continue;
        }
        if (ab3_d1(e[i].c)) {
            d1++;
            continue;
        }
        cells[k++] = e[i].c;
        put(&ex, "%s %s\n", e[i].c->key, e[i].pc);
        put(&ref, "%s %s\n", e[i].c->key, e[i].a);
        if (strcmp(e[i].a, e[i].pc))
            differ++;
    }
    put(&ex, "rc=0\n");
    put(&ref, "rc=0\n");
    if (k) {
        put(&head, "; %s - expression matrix, AmiBlitz3 case of tests/corpus/tests-exprmatrix/%s.bas: %zu of its "
                   "%zu cells", name, name, k, n);
        if (refused)
            put(&head, ", the %zu AmiBlitz3 refuses left out", refused);
        if (d1)
            put(&head, ", %zu If on a folded Float constant left out (AmiBlitz3 tests d1, #1187)", d1);
        put(&head, "\n; %s.expect: the output of blitz2-ab3 -r in AB3 mode (the port's compiler with AmiBlitz3's "
                   "libraries)", name);
        if (differ)
            put(&head, "; %s.ref.expect: AmiBlitz3's measured output, %zu cells differ (docs/ab3/variant.md)",
                name, differ);
        put(&head, "\n; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n",
            x->desc);
        src = source_of(m, cells, k, &head, 0, NULL);
        add_file(&x->out, str_printf(DIR_AB3 "%s.ab3", name), src);
        add_file(&x->out, str_printf(DIR_AB3 "%s.expect", name), text_of(&ex));
        if (differ) {
            add_file(&x->out, str_printf(DIR_AB3 "%s.ref.expect", name), text_of(&ref));
            put(&x->ab3, "ab3_cli_case(%s %s.ab3 DIR exprmatrix ARGS -r REF REF_EXPECT %s.ref.expect)\n", name,
                name, name);
            x->ab3_ref++;
        } else {
            free(ref.s);
            put(&x->ab3, "ab3_cli_case(%s %s.ab3 DIR exprmatrix ARGS -r REF)\n", name, name);
        }
        x->ab3_cases++;
        x->ab3_cells += k;
        x->ab3_differ += differ;
    } else {
        free(ex.s);
        free(ref.s);
    }
    x->ab3_refused += refused;
    x->ab3_d1 += d1;
    (void)g;
    free(head.s);
    free(cells);
}

static void emit_corpus(struct ctx *x, struct group *g, const char *name, struct ent *e, size_t n)
{
    const struct em_matrix *m = x->m;
    const struct em_cell **cells = malloc(n * sizeof *cells);
    struct buf head = { 0 }, a = { 0 }, f = { 0 };
    const char *dir = g->mode == 'c' ? DIR_EXPR : DIR_EXP;
    char *src;

    if (!cells) {
        x->out.oom = 1;
        return;
    }
    for (size_t i = 0; i < n; i++)
        cells[i] = e[i].c;
    put(&head, "; %s - expression matrix: context %s, value set %s%s, %zu cells\n", name,
        em_ctx_name(g->ctx), m->sets[g->set].name, mode_text(g->mode), n);
    if (!strcmp(g->tag, "agree"))
        put(&head, "; agreement cells: the value a recorded rule or the exact result gives, which the\n"
                   "; originals and the port give\n");
    else if (!strcmp(g->tag, "base"))
        put(&head, "; baseline cells (the original's measured value): no recorded rule covers them, the\n"
                   "; value is kept as a regression check, not as a decision that it is right\n");
    else
        put(&head, "; fixed cells of #%s (%s): %s.orig.out the original's value, %s.fixed-%s.out the port's\n",
            g->issues, bug_text(g->issues) ? bug_text(g->issues) : "?", name, name, g->issues);
    put(&head, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    src = source_of(m, cells, n, &head, g->mode == 'c', NULL);
    free(head.s);
    free(cells);
    add_file(&x->out, str_printf(DIR_SRC "%s.bas", name), src);
    if (all_same(e, n)) {
        lines(&a, e, n, 0);
        put(&a, "rc=0\n");
        add_file(&x->out, str_printf("%s%s.out", dir, name), text_of(&a));
    } else {
        lines(&a, e, n, 1);
        put(&a, "rc=0\n");
        lines(&f, e, n, 0);
        put(&f, "rc=0\n");
        add_file(&x->out, str_printf("%s%s.orig.out", dir, name), text_of(&a));
        add_file(&x->out, str_printf("%s%s.fixed-%s.out", dir, name, g->issues), text_of(&f));
    }
    x->files_kind[K_CORPUS]++;
    if (g->mode != 'c' && strcmp(g->tag, "base"))
        emit_ab3(x, g, name, e, n);
}

/* the header of a bug-orig test of issues */
static void bug_header(struct buf *b, const char *id, const char *issues, const char *what)
{
    char list[64], tmp[32];
    char *p, *save = NULL;
    int two = strchr(issues, '+') != NULL;

    issue_list(list, sizeof list, issues);
    put(b, "; %s (%s) - expression matrix: %s\n", id, list, what);
    snprintf(tmp, sizeof tmp, "%s", issues);
    for (p = strtok_r(tmp, "+", &save); p; p = strtok_r(NULL, "+", &save)) {
        const char *t = bug_text(p);
        struct buf l = { 0 };

        put(&l, "BUG(orig) (#%s): %s.", p, t ? t : "?");
        if (!l.oom)
            put_wrapped(b, "; ", l.s);
        free(l.s);
    }
    put(b, "; Correct: the values of the fixed rules of the matrix's model (tools/lib/exprmatrix/\n"
           "; model.c); the original's values are ID.orig.\n");
    if (two)
        put(b, "; The cells need the fixes of both issues: after the first fix the test comes out\n"
               "; DIFFERS (the port neither reproduces the original nor meets the expectation), the\n"
               "; commit of the second fix marks it fixed.\n");
}

static void emit_open(struct ctx *x, struct group *g, const char *id, struct ent *e, size_t n)
{
    const struct em_matrix *m = x->m;
    const struct em_cell **cells = malloc(n * sizeof *cells);
    struct buf head = { 0 }, ex = { 0 }, keys = { 0 }, what = { 0 };
    char *src;

    if (!cells) {
        x->out.oom = 1;
        return;
    }
    for (size_t i = 0; i < n; i++) {
        cells[i] = e[i].c;
        put(&keys, "%s%s", i ? " " : "", e[i].c->key);
    }
    put(&what, "open cells, context %s, value set %s%s, %zu cells", em_ctx_name(g->ctx), m->sets[g->set].name,
        mode_text(g->mode), n);
    bug_header(&head, id, g->issues, what.oom ? "" : what.s);
    if (!keys.oom)
        put_wrapped(&head, "; cells: ", keys.s);
    put(&head, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    put_fixed(&head, g->issues, x->port);
    if (g->mode == 'c')
        put(&head, "; @pass runtime_errors=on\n");
    src = source_of(m, cells, n, &head, g->mode == 'c', NULL);
    put(&ex, "[compile]\nok\n[run]\n");
    lines(&ex, e, n, 0);
    put(&ex, "rc=0\n");
    add_file(&x->out, str_printf(DIR_BUG "%s.bas", id), src);
    add_file(&x->out, str_printf(DIR_BUG "%s.expect", id), text_of(&ex));
    free(head.s);
    free(keys.s);
    free(what.s);
    free(cells);
    x->files_kind[K_OPEN]++;
}

static void emit_refused(struct ctx *x, struct group *g, const char *id, const struct ent *e)
{
    const struct em_matrix *m = x->m;
    const struct em_cell *c = e->c;
    struct buf head = { 0 }, ex = { 0 }, what = { 0 };
    int line = 0;
    char *src;
    char l = kind_class(c->left), r = c->right >= 0 ? kind_class(c->right) : '-';

    if (g->kind == K_REFUSED) {
        put(&head, "; %s - expression matrix: a refused cell, %s of the %zu cells with the\n", id, c->key, g->n);
        put(&head, "; operator %s and the operands %c%s%c (I a whole number, Q a Quick, F a Float, C a\n",
            em_op_text(c->op), l, c->right >= 0 ? " and " : "", c->right >= 0 ? r : ' ');
        put(&head, "; constant with a fraction)%s: blitz2 refuses it, as 1.95 and 2.10 do\n", mode_text(g->mode));
    } else {
        put(&what, "a cell refused after the fix, %s of the %zu cells with the operator %s and the operands %c%s%c%s",
            c->key, g->n, em_op_text(c->op), l, c->right >= 0 ? " and " : "", c->right >= 0 ? r : ' ',
            mode_text(g->mode));
        bug_header(&head, id, g->issues, what.oom ? "" : what.s);
    }
    put(&head, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    if (g->kind == K_OPEN_REFUSED)
        put_fixed(&head, g->issues, x->port);
    if (g->mode == 'c')
        put(&head, "; @pass runtime_errors=on\n");
    src = source_of(m, &c, 1, &head, 0, &line);
    put(&ex, "[compile]\nerror %d Illegal Operator for Type\n", line);
    add_file(&x->out, str_printf("%s%s.bas", g->kind == K_REFUSED ? DIR_REG : DIR_BUG, id), src);
    add_file(&x->out, str_printf("%s%s.expect", g->kind == K_REFUSED ? DIR_REG : DIR_BUG, id), text_of(&ex));
    free(head.s);
    free(what.s);
    x->files_kind[g->kind]++;
}

static void left_out(struct ctx *x, const struct em_cell *c, char mode, const char *why)
{
    put(&x->left, "%s%s%s\t%s\n", c->key, mode == 'c' ? " (checked)" : mode == 'u' ? " (unchecked)" : "", "",
        why);
    x->counts[4]++;
}

/* the cell k in mode (0 unchecked or 'v', 1 checked) of the table row r */
static void sort_cell(struct ctx *x, struct groups *gs, const struct em_class *k, const struct em_row *r, int chk)
{
    const struct em_cell *c = k->cell;
    char mode = !is_index(c) ? 'v' : chk ? 'c' : 'u';
    const char *o = r->v[chk ? EM_C195_ON : EM_C195], *p = r->v[chk ? EM_CLIB_ON : EM_CLIB],
               *pc = r->v[chk ? EM_PORTCD_ON : EM_PORTCD], *e = chk ? k->exp_on : k->exp,
               *ab = chk || !r->has[EM_AB3] ? "" : r->v[EM_AB3];
    char tag[48];
    struct group *g;
    int nf, ni, late = late_issues(k->issues, x->port, &nf, &ni);

    if (!strcmp(e, "!noncompile")) {
        /* one representative per operator and operand classes; a checked
           index cell refused unchecked too is represented by that */
        if (mode == 'c' && !strcmp(k->exp, e) && !strcmp(r->v[EM_C195], o))
            return;
        if (mode == 'u')
            mode = 'v';
        if (!strcmp(o, "!noncompile") && !strcmp(p, e)) {
            snprintf(tag, sizeof tag, "%s-%c-%c", em_op_name(c->op), kind_class(c->left),
                     c->right >= 0 ? kind_class(c->right) : 'x');
            g = group_of(gs, K_REFUSED, mode, c, tag, "");
            group_add(gs, g, c, o, e, "", "");
            x->counts[K_REFUSED]++;
        } else if ((!strcmp(p, o) && !strcmp(pc, o) && k->issues[0])
                   || (late && nf && (nf < ni || (!strcmp(p, e) && !strcmp(pc, e))))) {
            snprintf(tag, sizeof tag, "%s-%c-%c", em_op_name(c->op), kind_class(c->left),
                     c->right >= 0 ? kind_class(c->right) : 'x');
            g = group_of(gs, K_OPEN_REFUSED, mode, c, tag, k->issues);
            group_add(gs, g, c, o, e, "", "");
            x->counts[K_OPEN_REFUSED]++;
        } else
            left_out(x, c, mode, "refused after a fix, but the port differs");
        return;
    }
    if (mode == 'u' && (o[0] == '!' || e[0] == '!' || p[0] == '!' || pc[0] == '!'))
        return;                 /* outside the array: only with checking (design decision 8) */
    if (!printable(o, mode) || !printable(e, mode) || !printable(p, mode) || !printable(pc, mode)) {
        left_out(x, c, mode, "no value to compare");
        return;
    }
    if (!strcmp(o, e) && !strcmp(p, e) && !strcmp(pc, e)) {
        g = group_of(gs, K_CORPUS, mode, c, k->base ? "base" : "agree", "");
        group_add(gs, g, c, o, e, ab, pc);
        x->counts[K_CORPUS]++;
    } else if (strcmp(o, e) && late && nf
               && (nf < ni || (!strcmp(p, e) && (!strcmp(pc, e) || !strcmp(pc, o))))) {
        /* T1: an issue fixed after the matrix keeps its bug-orig test, with
           @fixed when every issue of it is fixed (emit_open) */
        snprintf(tag, sizeof tag, "open-%s", k->issues);
        g = group_of(gs, K_OPEN, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, "", "");
        x->counts[K_OPEN]++;
    } else if (strcmp(o, e) && !strcmp(p, e) && k->issues[0] && !strchr(k->issues, '+')
               && (!strcmp(pc, e) || !strcmp(pc, o))) {
        snprintf(tag, sizeof tag, "fixed-%s", k->issues);
        g = group_of(gs, K_CORPUS, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, ab, pc);
        x->counts[K_CORPUS]++;
    } else if (strcmp(o, e) && !strcmp(p, o) && !strcmp(pc, o) && k->issues[0]) {
        snprintf(tag, sizeof tag, "open-%s", k->issues);
        g = group_of(gs, K_OPEN, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, "", "");
        x->counts[K_OPEN]++;
    } else {
        char why[320];

        snprintf(why, sizeof why, "the port differs: original %s, expected %s, port %s, port with the CD's "
                 "libraries %s", o, e, p, pc);
        left_out(x, c, mode, why);
    }
}

/* the source names: CTX-SET[-MODE]-TAG-NN (corpus), compiler-expr-ISSUES-CTX-SET[-MODE]-NN (open),
   compiler-expr-refused-OP-L-R[-c], compiler-expr-ISSUES-refused-OP-L-R[-c] */
static void emit_group(struct ctx *x, struct group *g)
{
    const struct em_matrix *m = x->m;
    char iss[32], ms[4] = "";
    size_t per = m->per_source > 0 ? (size_t)m->per_source : 100;

    snprintf(iss, sizeof iss, "%s", g->issues);
    for (char *p = iss; *p; p++)
        if (*p == '+')
            *p = '-';
    if (g->mode != 'v')
        snprintf(ms, sizeof ms, "-%c", g->mode);
    if (g->kind == K_REFUSED || g->kind == K_OPEN_REFUSED) {
        char id[160];

        if (g->kind == K_REFUSED)
            snprintf(id, sizeof id, "compiler-expr-refused-%s%s", g->tag, g->mode == 'c' ? "-c" : "");
        else
            snprintf(id, sizeof id, "compiler-expr-%s-refused-%s%s", iss, g->tag, g->mode == 'c' ? "-c" : "");
        emit_refused(x, g, id, &g->e[0]);
        return;
    }
    order(m, g);
    /* a checked source (SetErr per cell) holds half as many: the port's
       compile of 51 of the checked i2 shift cells of value set J with
       their SetErr lines does not end (1.95 and 2.10 compile all 56; the
       port defect #1190, found by task 3.2, design.md) */
    if (g->mode == 'c' && per > 1)
        per /= 2;
    for (size_t first = 0, part = 1; first < g->n; first += per, part++) {
        size_t k = g->n - first < per ? g->n - first : per;
        char name[160];

        if (g->kind == K_CORPUS) {
            const char *t = !strncmp(g->tag, "fixed-", 6) ? "fix" : g->tag;

            snprintf(name, sizeof name, "%s-%s%s-%s%s-%02zu", em_ctx_name(g->ctx), m->sets[g->set].name, ms, t,
                     !strncmp(g->tag, "fixed-", 6) ? g->issues : "", part);
            emit_corpus(x, g, name, g->e + first, k);
        } else {
            snprintf(name, sizeof name, "compiler-expr-%s-%s-%s%s-%02zu", iss, em_ctx_name(g->ctx),
                     m->sets[g->set].name, ms, part);
            emit_open(x, g, name, g->e + first, k);
        }
    }
}

static int by_path(const void *a, const void *b)
{
    return strcmp(((const struct em_file *)a)->path, ((const struct em_file *)b)->path);
}

int em_expect(const struct em_matrix *m, const char *desc, const struct em_table *meas, struct em_file **files,
              size_t *nfiles, char **report, char **errors)
{
    return em_expect_port(m, desc, meas, EM_RULES_PORTNOW, files, nfiles, report, errors);
}

int em_expect_port(const struct em_matrix *m, const char *desc, const struct em_table *meas, unsigned port,
                   struct em_file **files, size_t *nfiles, char **report, char **errors)
{
    struct ctx x;
    struct em_table t = { 0 };
    struct em_class *k = NULL;
    struct groups gs = { 0 };
    const struct em_cell **idx = NULL;
    const struct em_row **mrow = NULL;
    char *cerr = NULL;
    int rc = -1;

    memset(&x, 0, sizeof x);
    x.m = m;
    x.desc = desc;
    x.port = port;
    *files = NULL;
    *nfiles = 0;
    *report = NULL;
    *errors = NULL;
    idx = malloc((m->ncells ? m->ncells : 1) * sizeof *idx);
    mrow = calloc(m->ncells ? m->ncells : 1, sizeof *mrow);
    t.rows = calloc(m->ncells ? m->ncells : 1, sizeof *t.rows);
    if (!idx || !mrow || !t.rows)
        goto out;
    for (size_t i = 0; i < m->ncells; i++)
        idx[i] = &m->cells[i];
    qsort(idx, m->ncells, sizeof *idx, by_key);
    for (size_t i = 0; i < meas->n; i++) {
        const struct em_cell *c = find_cell(idx, m->ncells, meas->rows[i].key);

        if (!c) {
            put(&x.err, "measured line %d: %s: not a cell of the description\n", meas->rows[i].line,
                meas->rows[i].key);
            x.nerr++;
            continue;
        }
        mrow[c - m->cells] = &meas->rows[i];
    }
    /* the table of every cell: the model's values, the measured ones where
       the model does not know them; the port is the port of this commit */
    for (size_t i = 0; i < m->ncells; i++) {
        const struct em_cell *c = &m->cells[i];
        struct em_row *r = &t.rows[t.n];
        int chk = has_checked(c);

        memset(r, 0, sizeof *r);
        if (mrow[i])
            *r = *mrow[i];
        r->key = (char *)c->key;
        for (int on = 0; on <= 1; on++) {
            struct em_value o, p, pc;

            if (on && !chk) {
                r->has[EM_C210_ON] = r->has[EM_C195_ON] = r->has[EM_PORTCD_ON] = r->has[EM_CLIB_ON] = 0;
                continue;
            }
            em_model(m, c, EM_RULES_ORIG, on, &o);
            em_model(m, c, port, on, &p);
            em_model(m, c, port | EM_R_CDLIB, on, &pc);
            if (!mrow[i]) {
                if (o.text[0] == '?' || p.text[0] == '?' || pc.text[0] == '?') {
                    put(&x.err, "%s: the model does not know its value and the measured table has no row\n",
                        c->key);
                    x.nerr++;
                }
                snprintf(r->v[on ? EM_C210_ON : EM_C210], EM_TEXT_MAX, "%s", o.text);
                snprintf(r->v[on ? EM_C195_ON : EM_C195], EM_TEXT_MAX, "%s", o.text);
                if (!on)
                    snprintf(r->v[EM_AB3], EM_TEXT_MAX, "%s", o.text);
            }
            if (p.text[0] != '?')
                snprintf(r->v[on ? EM_CLIB_ON : EM_CLIB], EM_TEXT_MAX, "%s", p.text);
            if (pc.text[0] != '?')
                snprintf(r->v[on ? EM_PORTCD_ON : EM_PORTCD], EM_TEXT_MAX, "%s", pc.text);
            if (mrow[i] && (p.text[0] == '?') != (pc.text[0] == '?')) {
                put(&x.err, "%s: the model knows the port's value with one library set only\n", c->key);
                x.nerr++;
            }
            if (on)
                r->has[EM_C210_ON] = r->has[EM_C195_ON] = r->has[EM_PORTCD_ON] = r->has[EM_CLIB_ON] = 1;
            else
                r->has[EM_C210] = r->has[EM_C195] = r->has[EM_PORTCD] = r->has[EM_CLIB] = r->has[EM_AB3] = 1;
        }
        t.n++;
    }
    if (x.nerr) {
        rc = 1;
        goto out;
    }
    rc = em_classes(m, &t, &k, &cerr);
    if (rc) {
        if (rc == 1)
            put(&x.err, "%s", cerr ? cerr : "");
        free(cerr);
        goto out;
    }
    rc = -1;
    for (size_t i = 0; i < t.n; i++) {
        const struct em_class *kk = &k[i];
        struct em_value ov;

        /* BUG(orig) (#1187): the original's If on a Float tests d1, and
           what d1 holds depends on the libraries (measured: the port before
           the fix gave another value with the C libraries than with the
           CD's); the original's value is no value for the comparison with
           2.10 and the C libraries (orig-clib) */
        em_model(m, kk->cell, EM_RULES_ORIG, 0, &ov);
        if ((ov.flags & EM_F_D1) && mrow[i] && strcmp(mrow[i]->v[EM_CLIB], mrow[i]->v[EM_PORTCD])) {
            left_out(&x, kk->cell, 'v', "the original's value depends on the libraries (d1, #1187)");
            continue;
        }
        if (!strcmp(kk->cls, "question")) {
            char why[64];

            snprintf(why, sizeof why, "question Q-%c (design.md, findings of task 2.4)", kk->q);
            left_out(&x, kk->cell, 'v', why);
            continue;
        }
        if (!strcmp(kk->cls, "new") || !strcmp(kk->cls, "port")) {
            left_out(&x, kk->cell, 'v', kk->cls);
            continue;
        }
        sort_cell(&x, &gs, kk, &t.rows[i], 0);
        if (has_checked(kk->cell))
            sort_cell(&x, &gs, kk, &t.rows[i], 1);
    }
    if (gs.oom)
        goto out;
    for (size_t i = 0; i < gs.n; i++)
        emit_group(&x, &gs.g[i]);
    add_file(&x.out, strdup(LEFT_OUT),
             str_printf("# the cells of the matrix left out of the committed tests, with the reason\n"
                        "# generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n%s",
                        desc, x.left.s ? x.left.s : ""));
    add_file(&x.out, strdup(AB3_CASES),
             str_printf("# the AmiBlitz3 cases of the expression matrix (tests/ab3/CMakeLists.txt)\n"
                        "# generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n%s",
                        desc, x.ab3.s ? x.ab3.s : ""));
    if (x.out.oom || x.left.oom || x.rep.oom || x.ab3.oom)
        goto out;
    qsort(x.out.f, x.out.n, sizeof *x.out.f, by_path);
    put(&x.rep, "cells in corpus sources %zu (%zu sources)\n", x.counts[K_CORPUS], x.files_kind[K_CORPUS]);
    put(&x.rep, "cells in bug-orig tests %zu (%zu tests)\n", x.counts[K_OPEN], x.files_kind[K_OPEN]);
    put(&x.rep, "refused cells %zu (%zu representatives)\n", x.counts[K_REFUSED], x.files_kind[K_REFUSED]);
    put(&x.rep, "cells refused after a fix %zu (%zu representatives, bug-orig tests)\n", x.counts[K_OPEN_REFUSED],
        x.files_kind[K_OPEN_REFUSED]);
    put(&x.rep, "AmiBlitz3 cases %zu (%zu cells; left out: %zu refused by AmiBlitz3, %zu If on a folded Float "
        "constant; %zu cases with REF_EXPECT, %zu cells differ)\n", x.ab3_cases, x.ab3_cells, x.ab3_refused,
        x.ab3_d1, x.ab3_ref, x.ab3_differ);
    put(&x.rep, "left out %zu\nfiles %zu\n", x.counts[4], x.out.n);
    if (x.rep.oom)
        goto out;
    *files = x.out.f;
    *nfiles = x.out.n;
    x.out.f = NULL;
    x.out.n = 0;
    *report = x.rep.s;
    x.rep.s = NULL;
    rc = 0;
out:
    if (rc == 1) {
        *errors = x.err.s;
        x.err.s = NULL;
    }
    for (size_t i = 0; i < gs.n; i++)
        free(gs.g[i].e);
    free(gs.g);
    free(k);
    free(idx);
    free(mrow);
    free(t.rows);
    em_files_free(x.out.f, x.out.n);
    free(x.left.s);
    free(x.ab3.s);
    free(x.rep.s);
    free(x.err.s);
    return rc;
}
