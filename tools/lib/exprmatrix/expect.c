/*
 * expect.c - the committed tests of the expression matrix (OpenSpec changes
 * expression-type-matrix, task 3.1, and exprmatrix-model-tests;
 * exprmatrix.h, em_expect): from the description, the model and the few
 * measured values the model cannot compute (em_measured), the tests of the
 * BASIC test model of the agreement, baseline and fixed cells, the
 * bug-orig tests of the open cells, the tests of the refused cells and the
 * list of the cells left out.
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
 * quick-long-arithmetic, design decision 13; ~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md 2026-11-22,
 * "T1 for all fixes", and rounds/tree/decisions.md 2026-11-24,
 * "exprmatrix-model-tests: Q1 and Q2"): o != e = p gives the open test
 * again, with fixed-orig = N in its configuration; a cell of two such
 * issues of which one is fixed stays in its test without fixed-orig (it
 * fails until the second fix).  A source holds the cells of one context,
 * value set, mode and class (design decision 2b), at most
 * cells-per-source; its foldable constant MOD cells come last (#1049: a
 * folded constant MOD as modlib's first use drops its relocations, and a
 * Long or Float MOD after it crashes the originals).
 *
 * A test checks each cell under its key (em_case_text) and declares the
 * values of the configurations with original material that differ from
 * the expected ones as deviations (deviate); the refused cells are data
 * sources of tests per operator or issue set (emit_refused).
 *
 * The agreement and fixed sources of a destination, parameter, If or Print
 * context and the unchecked index sources are also AmiBlitz3 cases (task
 * 3.5, design decision 9): NAME.ab3, a source that prints each cell
 * (em_source_text) without the cells AmiBlitz3 refuses, NAME.expect the
 * output of blitz2-ab3 in AB3 mode (the port's compiler, one core, with
 * AmiBlitz3's libraries, which have the CD's #199 and #234: the port's
 * value with the CD's libraries) and NAME.ref.expect AmiBlitz3's measured
 * output where it differs, listed in cases.cmake for tests/ab3.
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
#define DIR_BUG "tests/bug-orig/compiler/"
#define DIR_CORPUS "tests/bug-orig/exprmatrix/"
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

/* a file of the BASIC test model in dir: ID.test.bas, ID.test.toml or a
   data source ID.test/NAME.bas; *id the start of ID (rest of the path) */
static int model_file(const char *path, const char *dir, const char **id)
{
    size_t n = strlen(dir);
    const char *r = path + n, *slash, *t;

    if (strncmp(path, dir, n) || !*r)
        return 0;
    *id = r;
    slash = strchr(r, '/');
    if (!slash)
        return ends(r, ".test.bas") || ends(r, ".test.toml");
    if (strchr(slash + 1, '/') || !slash[1] || !ends(slash, ".bas"))
        return 0;
    t = slash - 5;
    return t > r && !strncmp(t, ".test", 5);
}

int em_expect_owns(const char *path)
{
    const char *r;

    if (!strcmp(path, LEFT_OUT) || model_file(path, DIR_CORPUS, &r))
        return 1;
    if (model_file(path, DIR_BUG, &r))
        return !strncmp(r, "compiler-expr-", 14) && r[14] >= '1' && r[14] <= '9';
    if (model_file(path, DIR_REG, &r))
        return !strncmp(r, "compiler-expr-refused-", 22);
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
                                       (EM_RULES_PORTNOW & ~EM_RULES_LIBFIX) | EM_R_CDLIB };

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
    char oc[EM_TEXT_MAX];       /* the original compiler's value with the C libraries (the corpus) */
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
                      const char *a, const char *pc, const char *oc)
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
    snprintf(g->e[g->n].oc, EM_TEXT_MAX, "%s", oc);
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
    { "1209", "NOT, AND, OR and EOR under a Quick type so far work on the 16.16 bits, the fraction included" },
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
        { "1187", EM_R_1187 }, { "1188", EM_R_1188 }, { "1189", EM_R_1189 }, { "1209", EM_R_1209 },
    };

    for (size_t i = 0; i < sizeof rules / sizeof rules[0]; i++)
        if (!strcmp(rules[i].issue, issue))
            return rules[i].rule;
    return 0;
}

/* the rules of the issues "1178+1189" */
static unsigned issue_rules(const char *issues)
{
    char tmp[32];
    char *p, *save = NULL;
    unsigned r = 0;

    snprintf(tmp, sizeof tmp, "%s", issues);
    for (p = strtok_r(tmp, "+", &save); p; p = strtok_r(NULL, "+", &save))
        r |= issue_rule(p);
    return r;
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

/* the issue of "fixed-orig = N" of a test of issues fixed after the matrix,
   all of them in the port's rules port: N the last of them (the order of
   the model's steps, the order the fixes are planned in); NULL: none */
static const char *fixed_issue(const char *issues, unsigned port)
{
    int nf, n;
    const char *last;

    if (!late_issues(issues, port, &nf, &n) || nf != n)
        return NULL;
    last = strrchr(issues, '+');
    return last ? last + 1 : issues;
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

static char *text_of(struct buf *b)
{
    if (b->oom) {
        free(b->s);
        return NULL;
    }
    return b->s ? b->s : strdup("");
}

/* the header head, every comment line of it longer than 96 characters
   wrapped (TED takes lines of at most 128); malloc'd, NULL out of memory */
static char *wrap_head(struct buf *head)
{
    struct buf b = { 0 };
    const char *p = head->s;

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
    return text_of(&b);
}

/* the source of the cells with the header head (wrap_head) in the old
   form (em_source_text); malloc'd, NULL out of memory */
static char *source_of(const struct em_matrix *m, const struct em_cell *const *cells, size_t n, struct buf *head,
                       int seterr, int *line_of)
{
    char *h = wrap_head(head), *src;

    if (!h)
        return NULL;
    src = em_source_text(m, cells, n, h, seterr, line_of);
    free(h);
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


/* the header of a bug-orig test of issues */
static void bug_header(struct buf *b, const char *id, const char *issues, const char *what, const char *correct)
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
    put(b, "%s", correct);
    if (two)
        put(b, "; The cells need the fixes of both issues: after the first fix the port\n"
               "; neither reproduces the original nor meets the expectation and the test fails;\n"
               "; the commit of the second fix marks it fixed.\n");
}

/* ------------------------------------------------- the model's files */

/* text as the inside of a TOML basic string (quote also escapes '"' and
   '\' a second time: a string of a predicate inside the TOML string) */
static void put_toml(struct buf *b, const char *text, int quote)
{
    for (const char *p = text; *p; p++) {
        if (*p == '\n')
            put(b, "\\n");
        else if (*p == '"' || *p == '\\')
            put(b, quote ? "\\\\\\%c" : "\\%c", *p);
        else
            put(b, "%c", *p);
    }
}

/* the configurations with original material of a test: the port with the
   CD's libraries, the original compiler with them and (the corpus) with
   the C libraries */
enum { CF_PC, CF_OD, CF_OC, CF_N };

/* the tables of the deviations: the original compiler's cells, the CD's
   libraries' cells, one per configuration */
enum { T_COMPILER, T_LIBS, T_PC, T_OD, T_OC, T_N };

struct devs {
    const char *orig;           /* "orig195" (bug-orig area), "orig210" (the corpus) */
    int has_oc;                 /* the configuration CF_OC is a cell */
    struct buf t[T_N];
    size_t n;                   /* the deviations written */
};

typedef void (*devline)(struct buf *b, const char *query, const char *value);

/* the deviation of query with the expected value e and the value v[CF]
   of each configuration: in the table of the original compiler when it
   alone differs (both library sets alike), in that of the CD's libraries
   when they alone differ (both compilers alike), else in the table of each
   configuration that differs; f writes its lines */
static void deviate(struct devs *d, const char *query, const char *e, const char *const v[CF_N], devline f)
{
    int s[CF_N] = { 0 };
    int nc = d->has_oc ? CF_N : CF_OC, any = 0;

    for (int c = 0; c < nc; c++)
        any |= s[c] = strcmp(v[c], e) != 0;
    if (!any)
        return;
    d->n++;
    if (s[CF_OD] && !s[CF_PC] && (!d->has_oc || (s[CF_OC] && !strcmp(v[CF_OD], v[CF_OC]))))
        f(&d->t[T_COMPILER], query, v[CF_OD]);
    else if (s[CF_PC] && s[CF_OD] && (!d->has_oc || !s[CF_OC]) && !strcmp(v[CF_PC], v[CF_OD]))
        f(&d->t[T_LIBS], query, v[CF_PC]);
    else
        for (int c = 0; c < nc; c++)
            if (s[c])
                f(&d->t[T_PC + c], query, v[c]);
}

/* the tables of d, each behind its header */
static void put_devs(struct buf *b, struct devs *d)
{
    for (int k = 0; k < T_N; k++) {
        if (!d->t[k].n)
            continue;
        switch (k) {
        case T_COMPILER: put(b, "\n[deviations.\"compiler=%s\"]\n", d->orig); break;
        case T_LIBS: put(b, "\n[deviations.\"libs=cd\"]\n"); break;
        case T_PC: put(b, "\n[deviations.\"compiler=port,libs=cd\"]\n"); break;
        case T_OD: put(b, "\n[deviations.\"compiler=%s,libs=cd\"]\n", d->orig); break;
        default: put(b, "\n[deviations.\"compiler=%s,libs=clib\"]\n", d->orig); break;
        }
        if (d->t[k].oom)
            b->oom = 1;
        else
            put(b, "%s", d->t[k].s);
    }
}

static void devs_free(struct devs *d)
{
    for (int k = 0; k < T_N; k++)
        free(d->t[k].s);
}

/* the compile result of a data source as a predicate value */
static void data_line(struct buf *b, const char *query, const char *value)
{
    put(b, "\"%s\" = \"== ", query);
    if (!strcmp(value, "ok"))
        put(b, "ok\"\n");
    else {
        put(b, "\\\"");
        put_toml(b, value, 1);
        put(b, "\\\"\"\n");
    }
}

/* the operands of a refused cell as the header names them */
static void operands_text(struct buf *b, const struct em_cell *c)
{
    if (c->right >= 0)
        put(b, "the operands %c and %c", kind_class(c->left), kind_class(c->right));
    else
        put(b, "the operand %c", kind_class(c->left));
}

/*
 * The test of the refused cells of the groups gl[0..n) (one operator, or
 * one issue set: bug): a data source ID.test/NAME.bas per group, the
 * source of one representative cell, compiled by each cell of the test,
 * which asserts its error; for a bug-orig test the original's compile,
 * which gives no error, is the deviation.  The case only makes the driver.
 */
static void emit_refused_test(struct ctx *x, struct group *const *gl, size_t n, const char *id, int bug)
{
    const struct em_matrix *m = x->m;
    const char *dir = bug ? DIR_BUG : DIR_REG;
    struct buf src = { 0 }, conf = { 0 }, opts = { 0 }, as = { 0 };
    struct devs d = { .orig = "orig195" };
    const char *op = em_op_text(gl[0]->e[0].c->op);
    size_t cells = 0;

    for (size_t i = 0; i < n; i++)
        cells += gl[i]->n;
    if (bug) {
        struct buf what = { 0 };

        put(&what, "the cells refused after the fix, %zu cells in %zu data sources of %s.test/, each one cell of "
                   "an operator and operand classes", cells, n, id);
        bug_header(&src, id, gl[0]->issues, what.oom ? "" : what.s,
                   "; Correct: the compile refuses each cell (Illegal Operator for Type); the original\n"
                   "; compiles it (the declared deviations).\n");
        free(what.s);
    } else
        put(&src, "; %s - expression matrix: the refused cells of the operator %s, %zu cells in %zu data sources "
                  "of %s.test/, each one cell of an operand class; blitz2 refuses them, as 1.95 and 2.10 do\n",
            id, op, cells, n, id);
    put(&src, "; The operand classes: I a whole number, Q a Quick, F a Float, C a constant with a fraction, S a "
              "string.  The compile of each data source is judged (%s.test.toml); the case only makes the "
              "driver.\n", id);
    put(&src, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    put(&src, "!TestCase{cells}\n  !Check{1,\"driver\"}\n!EndTestCase\n");
    add_file(&x->out, str_printf("%s%s.test.bas", dir, id), wrap_head(&src));
    free(src.s);
    put(&conf, "# %s.test.bas: the data sources %s.test/*.bas, each a cell the compilers refuse%s; generated by "
               "exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n",
        id, id, bug ? " after the fix (the original compiles it: the deviations)" : "", x->desc);
    if (bug && fixed_issue(gl[0]->issues, x->port))
        put(&conf, "fixed-orig = %s\n", fixed_issue(gl[0]->issues, x->port));
    put(&conf, "\n[data]\n");
    for (size_t i = 0; i < n; i++) {
        const struct group *g = gl[i];
        const struct ent *e = &g->e[0];
        const struct em_cell *c = e->c;
        struct buf head = { 0 };
        char name[64], query[80], err[64];
        const char *v[CF_N];
        int line = 0;
        char *text;

        /* OP-L-R, the regression tests' without the operator */
        snprintf(name, sizeof name, "%s%s.bas", bug ? g->tag : strchr(g->tag, '-') + 1, g->mode == 'c' ? "-c" : "");
        put(&head, "; %s.test/%s - expression matrix: a refused cell, %s of the %zu cells with the operator %s and ",
            id, name, c->key, g->n, em_op_text(c->op));
        operands_text(&head, c);
        put(&head, "%s\n; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n",
            mode_text(g->mode), x->desc);
        text = source_of(m, &c, 1, &head, 0, &line);
        free(head.s);
        add_file(&x->out, str_printf("%s%s.test/%s", dir, id, name), text);
        if (g->mode == 'c')
            put(&opts, "\n[data.\"%s\"]\naction = \"basic-compile\"\nruntime-errors = \"on\"\n", name);
        else
            put(&conf, "\"%s\" = \"basic-compile\"\n", name);
        snprintf(query, sizeof query, "data.%s", name);
        snprintf(err, sizeof err, "error %d Illegal Operator for Type", line);
        data_line(&as, query, err);
        v[CF_PC] = e->pc[0] == '!' ? err : "ok";
        v[CF_OD] = e->o[0] == '!' ? err : "ok";
        v[CF_OC] = err;
        if (bug)
            deviate(&d, query, err, v, data_line);
    }
    if (!opts.oom && opts.s)
        put(&conf, "%s", opts.s);
    put(&conf, "\n[asserts]\n");
    if (!as.oom && as.s)
        put(&conf, "%s", as.s);
    if (opts.oom || as.oom)
        conf.oom = 1;
    put_devs(&conf, &d);
    add_file(&x->out, str_printf("%s%s.test.toml", dir, id), text_of(&conf));
    free(opts.s);
    free(as.s);
    devs_free(&d);
    x->files_kind[bug ? K_OPEN_REFUSED : K_REFUSED]++;
}

/* the tests of the refused cells: one per operator (regression), one per
   issue set (bug-orig) */
static void emit_refused(struct ctx *x, struct groups *gs)
{
    struct group **gl = malloc((gs->n ? gs->n : 1) * sizeof *gl);
    unsigned char *done = calloc(gs->n ? gs->n : 1, 1);

    if (!gl || !done) {
        free(gl);
        free(done);
        x->out.oom = 1;
        return;
    }
    for (size_t i = 0; i < gs->n; i++) {
        struct group *g = &gs->g[i];
        int bug = g->kind == K_OPEN_REFUSED;
        size_t n = 0;
        char id[96], iss[32];

        if (done[i] || (g->kind != K_REFUSED && !bug))
            continue;
        for (size_t j = i; j < gs->n; j++) {
            struct group *h = &gs->g[j];

            if (done[j] || h->kind != g->kind
                || (bug ? strcmp(h->issues, g->issues) : h->e[0].c->op != g->e[0].c->op))
                continue;
            gl[n++] = h;
            done[j] = 1;
        }
        snprintf(iss, sizeof iss, "%s", g->issues);
        for (char *p = iss; *p; p++)
            if (*p == '+')
                *p = '-';
        if (bug)
            snprintf(id, sizeof id, "compiler-expr-%s-refused", iss);
        else
            snprintf(id, sizeof id, "compiler-expr-refused-%s", em_op_name(g->e[0].c->op));
        emit_refused_test(x, gl, n, id, bug);
    }
    free(gl);
    free(done);
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
    struct em_value ov;
    char oc[EM_TEXT_MAX];

    /* the original compiler with the C libraries (the corpus's 2.10 with
       them, never measured): the model's value, the original's where the
       model does not know it (a Float power, the If on a Float) */
    em_model(x->m, c, EM_RULES_ORIG & ~EM_R_CDLIB, chk, &ov);
    snprintf(oc, sizeof oc, "%s", ov.text[0] == '?' ? o : ov.text);

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
            group_add(gs, g, c, o, e, "", pc, "");
            x->counts[K_REFUSED]++;
        } else if ((!strcmp(p, o) && !strcmp(pc, o) && k->issues[0])
                   || (late && nf && (nf < ni || (!strcmp(p, e) && !strcmp(pc, e))))) {
            snprintf(tag, sizeof tag, "%s-%c-%c", em_op_name(c->op), kind_class(c->left),
                     c->right >= 0 ? kind_class(c->right) : 'x');
            g = group_of(gs, K_OPEN_REFUSED, mode, c, tag, k->issues);
            group_add(gs, g, c, o, e, "", pc, "");
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
        group_add(gs, g, c, o, e, ab, pc, oc);
        x->counts[K_CORPUS]++;
    } else if (strcmp(o, e) && late && nf
               && (nf < ni || (!strcmp(p, e) && (!strcmp(pc, e) || !strcmp(pc, o)
                                                 || (issue_rules(k->issues) & EM_RULES_LIBFIX))))) {
        /* T1: an issue fixed after the matrix keeps its bug-orig test, with
           fixed-orig when every issue of it is fixed (emit_open); with an
           issue fixed in the C libraries the port with the CD's libraries
           may give a third value (that issue's original rule with the
           port's other rules) */
        snprintf(tag, sizeof tag, "open-%s", k->issues);
        g = group_of(gs, K_OPEN, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, "", pc, "");
        x->counts[K_OPEN]++;
    } else if (strcmp(o, e) && !strcmp(p, e) && k->issues[0] && !strchr(k->issues, '+')
               && (!strcmp(pc, e) || !strcmp(pc, o))) {
        snprintf(tag, sizeof tag, "fixed-%s", k->issues);
        g = group_of(gs, K_CORPUS, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, ab, pc, oc);
        x->counts[K_CORPUS]++;
    } else if (strcmp(o, e) && !strcmp(p, o) && !strcmp(pc, o) && k->issues[0]) {
        snprintf(tag, sizeof tag, "open-%s", k->issues);
        g = group_of(gs, K_OPEN, mode, c, tag, k->issues);
        group_add(gs, g, c, o, e, "", pc, "");
        x->counts[K_OPEN]++;
    } else {
        char why[320];

        snprintf(why, sizeof why, "the port differs: original %s, expected %s, port %s, port with the CD's "
                 "libraries %s", o, e, p, pc);
        left_out(x, c, mode, why);
    }
}

/* an id of the BASIC test model is lower-case (the value sets are
   upper-case letters) */
static void lower(char *s)
{
    for (; *s; s++)
        if (*s >= 'A' && *s <= 'Z')
            *s = (char)(*s - 'A' + 'a');
}

/* a Quick or Float destination or parameter: the check takes the raw bits */
static int raw_bits(enum em_ctx x)
{
    return x == EM_DQ || x == EM_DF || x == EM_PQ || x == EM_PF;
}

/* a value of a cell in context x as the signed decimal Long its check
   sees: the raw bits of a Quick or Float, 1 or 0 of an If, -1 of the
   runtime error */
static void check_value(char *out, size_t n, enum em_ctx x, const char *v)
{
    if (!strcmp(v, "!runtime"))
        snprintf(out, n, "-1");
    else if (x == EM_IF)
        snprintf(out, n, "%d", !strcmp(v, "T"));
    else if (raw_bits(x))
        snprintf(out, n, "%ld", (long)(int)(unsigned)strtoul(v, NULL, 16));
    else
        snprintf(out, n, "%ld", strtol(v, NULL, 10));
}

/* the expected value of a cell as the check's constant: the raw bits as
   $BITS (as the old form printed them), else check_value */
static void check_literal(char *out, size_t n, enum em_ctx x, const char *v)
{
    if (raw_bits(x) && strcmp(v, "!runtime"))
        snprintf(out, n, "$%s", v);
    else
        check_value(out, n, x, v);
}

static void check_line(struct buf *b, const char *query, const char *value)
{
    put(b, "\"%s\" = \"== fail\"\n\"%s.actual\" = \"== %s\"\n", query, query, value);
}

static void output_line(struct buf *b, const char *query, const char *value)
{
    put(b, "%s = \"== \\\"", query);
    put_toml(b, value, 1);
    put(b, "\\\"\"\n");
}

/* the printed output of the cells, each "KEY VALUE" (which: 'e' the
   expected values, 'p' the port's with the CD's libraries, 'o' the
   original's, 'c' the original compiler's with the C libraries) */
static char *output_of(const struct ent *e, size_t n, char which)
{
    struct buf b = { 0 };

    for (size_t i = 0; i < n; i++)
        put(&b, "%s %s\n", e[i].c->key,
            which == 'e' ? e[i].e : which == 'p' ? e[i].pc : which == 'o' ? e[i].o : e[i].oc);
    return text_of(&b);
}

/*
 * A test of the BASIC test model of the cells e[0..n) of group g (an open
 * or T1 bug-orig test, corpus 0; a corpus source, corpus 1): ID.test.bas
 * with the header head and the case of the cells (em_case_text), and
 * ID.test.toml: fixed-orig for a test of fixed issues, runtime error
 * checking for the checked index cells, the output of the Print cells,
 * and the values of the configurations with original material that
 * differ from the expected ones as deviations.
 */
static void emit_model(struct ctx *x, struct group *g, const char *path, struct buf *head, struct ent *e, size_t n,
                       int corpus, const char *fixed)
{
    const struct em_matrix *m = x->m;
    const struct em_cell **cells = malloc((n ? n : 1) * sizeof *cells);
    char (*lit)[EM_TEXT_MAX] = malloc((n ? n : 1) * sizeof *lit);
    const char **lits = malloc((n ? n : 1) * sizeof *lits);
    struct buf conf = { 0 };
    struct devs d = { .orig = corpus ? "orig210" : "orig195", .has_oc = corpus };
    const char *id = strrchr(path, '/') + 1;
    char *h;

    if (!cells || !lit || !lits) {
        free(cells);
        free(lit);
        free(lits);
        x->out.oom = 1;
        return;
    }
    for (size_t i = 0; i < n; i++) {
        cells[i] = e[i].c;
        check_literal(lit[i], sizeof lit[i], g->ctx, e[i].e);
        lits[i] = lit[i];
    }
    h = wrap_head(head);
    add_file(&x->out, str_printf("%s.test.bas", path),
             h ? em_case_text(m, cells, n, h, g->ctx == EM_PRINT ? NULL : lits, g->mode == 'c') : NULL);
    free(h);
    put(&conf, "# %s.test.bas: generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n",
        id, x->desc);
    if (fixed && *fixed)
        put(&conf, "fixed-orig = %s\n", fixed);
    if (g->mode == 'c')
        put(&conf, "runtime-errors = \"on\"\n");
    if (g->ctx == EM_PRINT) {
        char *v[CF_N], *ex = output_of(e, n, 'e');

        v[CF_PC] = output_of(e, n, 'p');
        v[CF_OD] = output_of(e, n, 'o');
        v[CF_OC] = output_of(e, n, 'c');
        if (ex && v[CF_PC] && v[CF_OD] && v[CF_OC]) {
            put(&conf, "\n[asserts]\n");
            output_line(&conf, "output", ex);
            deviate(&d, "output", ex, (const char *const *)v, output_line);
        } else
            conf.oom = 1;
        free(ex);
        for (int c = 0; c < CF_N; c++)
            free(v[c]);
    } else
        for (size_t i = 0; i < n; i++) {
            char query[64], ev[24], vv[CF_N][24];
            const char *v[CF_N] = { vv[CF_PC], vv[CF_OD], vv[CF_OC] };

            snprintf(query, sizeof query, "check.cells.%s", e[i].c->key);
            check_value(ev, sizeof ev, g->ctx, e[i].e);
            check_value(vv[CF_PC], sizeof vv[CF_PC], g->ctx, e[i].pc);
            check_value(vv[CF_OD], sizeof vv[CF_OD], g->ctx, e[i].o);
            check_value(vv[CF_OC], sizeof vv[CF_OC], g->ctx, corpus ? e[i].oc : e[i].e);
            deviate(&d, query, ev, v, check_line);
        }
    put_devs(&conf, &d);
    add_file(&x->out, str_printf("%s.test.toml", path), text_of(&conf));
    devs_free(&d);
    free(cells);
    free(lit);
    free(lits);
}

static void emit_open(struct ctx *x, struct group *g, const char *id, struct ent *e, size_t n)
{
    const struct em_matrix *m = x->m;
    struct buf head = { 0 }, keys = { 0 }, what = { 0 };
    char *path;

    for (size_t i = 0; i < n; i++)
        put(&keys, "%s%s", i ? " " : "", e[i].c->key);
    put(&what, "open cells, context %s, value set %s%s, %zu cells", em_ctx_name(g->ctx), m->sets[g->set].name,
        mode_text(g->mode), n);
    bug_header(&head, id, g->issues, what.oom ? "" : what.s,
               "; Correct: the values of the fixed rules of the matrix's model (tools/lib/exprmatrix/\n"
               "; model.c); the original's values are the declared deviations of the test.\n");
    if (!keys.oom)
        put_wrapped(&head, "; cells: ", keys.s);
    put(&head, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    path = str_printf(DIR_BUG "%s", id);
    if (path)
        emit_model(x, g, path, &head, e, n, 0, fixed_issue(g->issues, x->port));
    else
        x->out.oom = 1;
    free(path);
    free(head.s);
    free(keys.s);
    free(what.s);
    x->files_kind[K_OPEN]++;
}

/* a source of agreement, baseline or fixed cells: a test of the BASIC
   test model in DIR_CORPUS (its id the lower-case name), and the
   AmiBlitz3 case of the agreement and fixed sources (name) */
static void emit_corpus(struct ctx *x, struct group *g, const char *name, struct ent *e, size_t n)
{
    const struct em_matrix *m = x->m;
    struct buf head = { 0 };
    char id[160];
    char *path;
    int fixed = !strncmp(g->tag, "fixed-", 6);

    snprintf(id, sizeof id, "%s", name);
    lower(id);
    put(&head, "; %s - expression matrix: context %s, value set %s%s, %zu cells\n", id,
        em_ctx_name(g->ctx), m->sets[g->set].name, mode_text(g->mode), n);
    if (!strcmp(g->tag, "agree"))
        put(&head, "; agreement cells: the value a recorded rule or the exact result gives, which the\n"
                   "; originals and the port give\n");
    else if (!strcmp(g->tag, "base"))
        put(&head, "; baseline cells (the original's measured value): no recorded rule covers them, the\n"
                   "; value is kept as a regression check, not as a decision that it is right\n");
    else
        put(&head, "; fixed cells of #%s (%s): the checks have the port's values, the original's are the "
                   "declared deviations of the test\n",
            g->issues, bug_text(g->issues) ? bug_text(g->issues) : "?");
    put(&head, "; generated by exprmatrix expect from %s (tools/exprmatrix/README.md); do not edit\n", x->desc);
    path = str_printf(DIR_CORPUS "%s", id);
    if (path)
        emit_model(x, g, path, &head, e, n, 1, fixed ? g->issues : NULL);
    else
        x->out.oom = 1;
    free(path);
    free(head.s);
    x->files_kind[K_CORPUS]++;
    if (g->mode != 'c' && strcmp(g->tag, "base"))
        emit_ab3(x, g, name, e, n);
}

/* the source names: CTX-SET[-MODE]-TAG-NN (corpus), compiler-expr-ISSUES-CTX-SET[-MODE]-NN (open);
   the refused cells: emit_refused */
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
    if (g->kind == K_REFUSED || g->kind == K_OPEN_REFUSED)
        return;                 /* emit_refused */
    order(m, g);
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
            lower(name);
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
            struct em_value o, p, pc, pf;

            if (on && !chk) {
                r->has[EM_C210_ON] = r->has[EM_C195_ON] = r->has[EM_PORTCD_ON] = r->has[EM_CLIB_ON] = 0;
                continue;
            }
            em_model(m, c, EM_RULES_ORIG, on, &o);
            em_model(m, c, port, on, &p);
            em_model(m, c, (port & ~EM_RULES_LIBFIX) | EM_R_CDLIB, on, &pc);
            /* a power the model does not compute: with the CD's libraries
               the port calls the original's SPPow; the same operands give
               the original's value */
            if (pc.text[0] == '?' && pc.sig == o.sig) {
                const char *ov = mrow[i] ? mrow[i]->v[on ? EM_C195_ON : EM_C195] : o.text;

                if (ov[0] != '?')
                    snprintf(pc.text, sizeof pc.text, "%s", ov);
            }
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
            em_model(m, c, port | EM_R_CDLIB, on, &pf);
            if (mrow[i] && (p.text[0] == '?') != (pf.text[0] == '?')) {
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
    emit_refused(&x, &gs);
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
