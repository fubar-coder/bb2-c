/*
 * testassert.c - the assertions of the run configuration: queries,
 * predicates and the replacement per query (testconf.h, "assertions";
 * the grammar: tests/README.md, "Assertions").
 */
#include "testconf.h"
#include "testconf_int.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ queries */

static const struct {
    const char *name;
    int kind;
} fixed_queries[] = {
    { "rc", TC_Q_RC },
    { "requester", TC_Q_REQUESTER },
    { "compile", TC_Q_COMPILE },
    { "compile.log", TC_Q_COMPILE_LOG },
    { "output", TC_Q_OUTPUT },
    { "serial", TC_Q_SERIAL },
    { "inject", TC_Q_INJECT },
    { "session", TC_Q_SESSION },
    { "end.case", TC_Q_END_CASE },
    { "end.run", TC_Q_END_RUN },
};

void tc_query_free(struct tc_query *q)
{
    free(q->ns);
    free(q->rest);
    memset(q, 0, sizeof *q);
}

static int ident_char(int c, int first)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || (!first && c >= '0' && c <= '9');
}

int tc_query_parse(const char *text, struct tc_query *q, char *msg, size_t msglen)
{
    memset(q, 0, sizeof *q);
    for (size_t i = 0; i < sizeof fixed_queries / sizeof fixed_queries[0]; i++)
        if (!strcmp(text, fixed_queries[i].name)) {
            q->kind = fixed_queries[i].kind;
            return 0;
        }
    if (!strncmp(text, "check.", 6)) {
        const char *ns = text + 6, *p = ns;

        while (ident_char((unsigned char)*p, p == ns))
            p++;
        if (p == ns || *p != '.' || !p[1])
            return tc_err(msg, msglen,
                          "the query \"%s\": check.NAMESPACE.NAME expected (NAMESPACE a case, a procedure, "
                          "init or end)", text);
        q->kind = TC_Q_CHECK;
        if (!(q->ns = malloc((size_t)(p - ns) + 1)) || !(q->rest = tc_strdup(p + 1))) {
            tc_query_free(q);
            return -2;
        }
        memcpy(q->ns, ns, (size_t)(p - ns));
        q->ns[p - ns] = 0;
        return 0;
    }
    return tc_err(msg, msglen, "unknown query \"%s\"", text);
}

/* ------------------------------------------------------------ predicates */

static const char *const pop_names[] = {
    "==", "!=", "<", "<=", ">", ">=", "contains", "not contains", "matches", "exists", "not exists", "count"
};

const char *tc_pop_name(int op)
{
    return op >= 0 && op <= TC_P_COUNT ? pop_names[op] : "?";
}

void tc_pred_free(struct tc_pred *p)
{
    free(p->str);
    memset(p, 0, sizeof *p);
}

/* the operators a query allows, bit per enum tc_pop */
#define B(op) (1u << (op))
#define CMP (B(TC_P_EQ) | B(TC_P_NE) | B(TC_P_LT) | B(TC_P_LE) | B(TC_P_GT) | B(TC_P_GE))
#define TEXT (B(TC_P_EQ) | B(TC_P_NE) | B(TC_P_CONTAINS) | B(TC_P_NOT_CONTAINS) | B(TC_P_MATCHES))
#define EX (B(TC_P_EXISTS) | B(TC_P_NOT_EXISTS))

static unsigned allowed_ops(int qkind)
{
    switch (qkind) {
    case TC_Q_RC: return CMP | EX;
    case TC_Q_REQUESTER: return B(TC_P_CONTAINS) | B(TC_P_NOT_CONTAINS) | B(TC_P_MATCHES) | EX | B(TC_P_COUNT);
    case TC_Q_COMPILE: return TEXT;
    case TC_Q_COMPILE_LOG:
    case TC_Q_OUTPUT:
    case TC_Q_SERIAL:
    case TC_Q_INJECT: return TEXT | EX;
    case TC_Q_SESSION: return B(TC_P_EQ) | B(TC_P_NE);
    case TC_Q_END_CASE:
    case TC_Q_END_RUN: return EX;
    case TC_Q_CHECK: return CMP | B(TC_P_CONTAINS) | B(TC_P_NOT_CONTAINS) | B(TC_P_MATCHES);
    }
    return 0;
}

/* the words a query allows after == and != ("|" between them) */
static const char *allowed_words(int qkind)
{
    switch (qkind) {
    case TC_Q_COMPILE: return "ok|none";
    case TC_Q_SESSION: return "done|timeout|halt";
    case TC_Q_CHECK: return "ok|fail|not-run";
    }
    return NULL;
}

static int word_in(const char *list, const char *w)
{
    struct tc_keydef d = { 0 };

    d.values = list;
    return list && tc_enum_ok(&d, w);
}

static const char *skip_blanks(const char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/* an integer value: -?[0-9]+ or $ and 1 to 8 hex digits (32 bits, signed) */
static int parse_int(const char *p, const char **end, long long *out)
{
    if (*p == '$') {
        uint32_t v = 0;
        const char *s = ++p;

        while ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')) {
            v = (v << 4) | (uint32_t)(*p <= '9' ? *p - '0' : (*p | 32) - 'a' + 10);
            p++;
        }
        if (p == s || p - s > 8)
            return -1;
        *out = (int32_t)v;
        *end = p;
        return 0;
    }
    {
        int neg = *p == '-';
        const char *s = p + neg;
        long long v = 0;

        p = s;
        while (*p >= '0' && *p <= '9') {
            if (v > LLONG_MAX / 10 - 10)
                return -1;
            v = v * 10 + (*p - '0');
            p++;
        }
        if (p == s || (p - s > 1 && *s == '0'))
            return -1;
        *out = neg ? -v : v;
        *end = p;
        return 0;
    }
}

int tc_pred_parse(const char *text, int qkind, struct tc_pred *pr, char *msg, size_t msglen)
{
    static const struct {
        const char *tok;
        int op;
    } ops[] = {
        { "==", TC_P_EQ }, { "!=", TC_P_NE }, { "<=", TC_P_LE }, { ">=", TC_P_GE }, { "<", TC_P_LT },
        { ">", TC_P_GT }, { "not contains", TC_P_NOT_CONTAINS }, { "contains", TC_P_CONTAINS },
        { "matches", TC_P_MATCHES }, { "not exists", TC_P_NOT_EXISTS }, { "exists", TC_P_EXISTS },
        { "count", TC_P_COUNT },
    };
    const char *p = skip_blanks(text);
    size_t i;

    memset(pr, 0, sizeof *pr);
    for (i = 0; i < sizeof ops / sizeof ops[0]; i++) {
        size_t n = strlen(ops[i].tok);

        if (!strncmp(p, ops[i].tok, n)
            && (ops[i].op <= TC_P_GE || p[n] == 0 || p[n] == ' ' || p[n] == '\t'))
            break;
    }
    if (i == sizeof ops / sizeof ops[0])
        return tc_err(msg, msglen, "the predicate \"%s\": an operator expected (== != < <= > >= contains, "
                      "not contains, matches, exists, not exists, count)", text);
    pr->op = ops[i].op;
    if (!(allowed_ops(qkind) & B(pr->op)))
        return tc_err(msg, msglen, "the predicate \"%s\": %s is not allowed for this query", text,
                      tc_pop_name(pr->op));
    p = skip_blanks(p + strlen(ops[i].tok));
    if (pr->op == TC_P_COUNT) {
        for (i = 0; i < 6; i++) {
            size_t n = strlen(ops[i].tok);

            if (!strncmp(p, ops[i].tok, n))
                break;
        }
        if (i == 6)
            return tc_err(msg, msglen, "the predicate \"%s\": count needs a comparison (count == N)", text);
        pr->cmp = ops[i].op;
        p = skip_blanks(p + strlen(ops[i].tok));
    }
    if (pr->op == TC_P_EXISTS || pr->op == TC_P_NOT_EXISTS) {
        pr->vtype = TC_PV_NONE;
    } else if (*p == '"') {
        size_t n = 0;
        char *s = malloc(strlen(p) + 1);

        if (!s)
            return -2;
        for (p++;; p++) {
            if (!*p) {
                free(s);
                return tc_err(msg, msglen, "the predicate \"%s\": a string without its closing \"", text);
            }
            if (*p == '"')
                break;
            if (*p == '\\') {
                p++;
                if (*p != '"' && *p != '\\') {
                    free(s);
                    return tc_err(msg, msglen, "the predicate \"%s\": only \\\" and \\\\ are escapes", text);
                }
            }
            s[n++] = *p;
        }
        s[n] = 0;
        p++;
        pr->vtype = TC_PV_STR;
        pr->str = s;
    } else if (*p == '$' || *p == '-' || (*p >= '0' && *p <= '9')) {
        if (parse_int(p, &p, &pr->num))
            return tc_err(msg, msglen, "the predicate \"%s\": a malformed integer", text);
        pr->vtype = TC_PV_INT;
    } else if (*p >= 'a' && *p <= 'z') {
        const char *s = p;

        while ((*p >= 'a' && *p <= 'z') || *p == '-')
            p++;
        pr->vtype = TC_PV_WORD;
        if (!(pr->str = malloc((size_t)(p - s) + 1)))
            return -2;
        memcpy(pr->str, s, (size_t)(p - s));
        pr->str[p - s] = 0;
    } else {
        return tc_err(msg, msglen, "the predicate \"%s\": a value expected after %s", text, tc_pop_name(pr->op));
    }
    if (*skip_blanks(p)) {
        tc_pred_free(pr);
        return tc_err(msg, msglen, "the predicate \"%s\": unexpected text \"%s\"", text, skip_blanks(p));
    }

    /* the value against the operator and the query */
    {
        int op = pr->op == TC_P_COUNT ? pr->cmp : pr->op;
        int ok;

        if (pr->op == TC_P_COUNT)
            ok = pr->vtype == TC_PV_INT;
        else if (op == TC_P_CONTAINS || op == TC_P_NOT_CONTAINS || op == TC_P_MATCHES)
            ok = pr->vtype == TC_PV_STR;
        else if (op == TC_P_EXISTS || op == TC_P_NOT_EXISTS)
            ok = 1;
        else if (op == TC_P_EQ || op == TC_P_NE)
            ok = (pr->vtype == TC_PV_INT && (qkind == TC_Q_RC || qkind == TC_Q_CHECK))
                 || (pr->vtype == TC_PV_STR && qkind != TC_Q_RC && qkind != TC_Q_SESSION)
                 || (pr->vtype == TC_PV_WORD && word_in(allowed_words(qkind), pr->str));
        else
            ok = pr->vtype == TC_PV_INT;
        if (!ok) {
            const char *w = allowed_words(qkind);

            tc_pred_free(pr);
            if (w && (op == TC_P_EQ || op == TC_P_NE))
                return tc_err(msg, msglen, "the predicate \"%s\": a value of this query expected (%s)", text, w);
            return tc_err(msg, msglen, "the predicate \"%s\": the value does not fit %s", text,
                          tc_pop_name(op));
        }
    }
    return 0;
}

/* ------------------------------------------------------------ validation */

static int check_entries(const struct tc_tabkey *v, size_t n, const char *table, char *err, size_t errlen)
{
    for (size_t i = 0; i < n; i++) {
        const struct tc_tabkey *t = &v[i];
        struct tc_selector sel;
        struct tc_query q;
        struct tc_pred p;
        char msg[300];
        int r;

        if (t->sel) {
            r = tc_selector_parse(t->sel, 1, &sel, msg, sizeof msg);
            if (r == -2)
                return tc_err(err, errlen, "out of memory");
            if (r)
                return tc_err(err, errlen, "%s:%u: [%s.\"%s\"]: %s", t->at.file, t->at.line, table, t->sel, msg);
            tc_selector_free(&sel);
        }
        r = tc_query_parse(t->key, &q, msg, sizeof msg);
        if (r == -2)
            return tc_err(err, errlen, "out of memory");
        if (r)
            return tc_err(err, errlen, "%s:%u: %s", t->at.file, t->at.line, msg);
        r = tc_pred_parse(t->value.str, q.kind, &p, msg, sizeof msg);
        tc_query_free(&q);
        if (r == -2)
            return tc_err(err, errlen, "out of memory");
        if (r)
            return tc_err(err, errlen, "%s:%u: %s: %s", t->at.file, t->at.line, t->key, msg);
        tc_pred_free(&p);
    }
    return 0;
}

int tc_end_split(const char *text, char **query, char **pred)
{
    const char *b = strchr(text, ' '), *p;

    *query = *pred = NULL;
    if (!b || b == text)
        return -1;
    for (p = b; *p == ' '; p++)
        ;
    if (!*p)
        return -1;
    if (!(*query = malloc((size_t)(b - text) + 1)) || !(*pred = tc_strdup(p))) {
        free(*query);
        *query = NULL;
        return -2;
    }
    memcpy(*query, text, (size_t)(b - text));
    (*query)[b - text] = 0;
    return 0;
}

/* a query an alternative end may name: one whose value varies with the end */
static int end_query(int kind)
{
    switch (kind) {
    case TC_Q_SESSION:
    case TC_Q_RC:
    case TC_Q_REQUESTER:
    case TC_Q_END_CASE:
    case TC_Q_END_RUN:
    case TC_Q_OUTPUT:
    case TC_Q_CHECK:
        return 1;
    }
    return 0;
}

/* the selectors, queries and predicates of the alternative ends */
static int check_ends(const struct tc_config *c, char *err, size_t errlen)
{
    for (size_t i = 0; i < c->nends; i++) {
        const struct tc_tabkey *t = &c->ends[i];
        struct tc_selector sel;
        char msg[300];
        int r = tc_selector_parse(t->sel, 1, &sel, msg, sizeof msg);

        if (r == -2)
            return tc_err(err, errlen, "out of memory");
        if (r)
            return tc_err(err, errlen, "%s:%u: [ends.\"%s\"]: %s", t->at.file, t->at.line, t->sel, msg);
        tc_selector_free(&sel);
        if (!strcmp(t->key, "reason"))
            continue;
        for (size_t k = 0; k < t->value.nlist; k++) {
            const char *a = t->value.list[k];
            char *qt, *pt;
            struct tc_query q;
            struct tc_pred p;

            r = tc_end_split(a, &qt, &pt);
            if (r == -2)
                return tc_err(err, errlen, "out of memory");
            if (r)
                return tc_err(err, errlen, "%s:%u: the alternative end %s: \"%s\": QUERY PREDICATE expected",
                              t->at.file, t->at.line, t->key, a);
            r = tc_query_parse(qt, &q, msg, sizeof msg);
            if (r == 0 && !end_query(q.kind)) {
                tc_query_free(&q);
                tc_err(err, errlen, "%s:%u: the alternative end %s: the query %s does not vary with the end "
                       "(an alternative names session, rc, requester, end.case, end.run, output or check.*)",
                       t->at.file, t->at.line, t->key, qt);
                free(qt);
                free(pt);
                return -1;
            }
            if (r == 0) {
                r = tc_pred_parse(pt, q.kind, &p, msg, sizeof msg);
                tc_query_free(&q);
                if (r == 0)
                    tc_pred_free(&p);
            }
            if (r == -2) {
                free(qt);
                free(pt);
                return tc_err(err, errlen, "out of memory");
            }
            if (r) {
                tc_err(err, errlen, "%s:%u: the alternative end %s: \"%s\": %s", t->at.file, t->at.line, t->key, a,
                       msg);
                free(qt);
                free(pt);
                return -1;
            }
            free(qt);
            free(pt);
        }
    }
    return 0;
}

int tc_assert_validate(const struct tc_config *c, char *err, size_t errlen)
{
    const struct tc_setting *fo = tc_get(c, "fixed-orig"), *fx = tc_get(c, "fixed");
    const struct tc_setting *ex = tc_get(c, "extension");

    if (check_entries(c->asserts, c->nasserts, "asserts", err, errlen)
        || check_entries(c->deviations, c->ndeviations, "deviations", err, errlen) || check_ends(c, err, errlen))
        return -1;
    /* the alternative ends are a declared deviation */
    if (fo && c->ndeviations == 0 && c->nends == 0)
        return tc_err(err, errlen, "%s:%u: test %s: fixed-orig without a declared deviation", fo->at.file,
                      fo->at.line, c->id);
    if (ex && c->ndeviations == 0 && c->nends == 0)
        return tc_err(err, errlen, "%s:%u: test %s: extension without a declared deviation (what the original "
                      "does instead)", ex->at.file, ex->at.line, c->id);
    if (fx && c->ndeviations)
        return tc_err(err, errlen, "%s:%u: test %s: fixed with a declared deviation (an original bug is "
                      "fixed-orig)", fx->at.file, fx->at.line, c->id);
    if (fx && c->nends)
        return tc_err(err, errlen, "%s:%u: test %s: fixed with declared alternative ends (an original bug is "
                      "fixed-orig)", fx->at.file, fx->at.line, c->id);
    return 0;
}

/* ------------------------------------------------------------ replacement per query */

static const struct tc_default defaults[] = {
    { "rc", "== 0" },
    { "requester", "count == 0" },
    { "end.case", "exists" },
    { "end.run", "exists" },
};

const struct tc_default *tc_defaults(size_t *n)
{
    *n = sizeof defaults / sizeof defaults[0];
    return defaults;
}

void tc_assertions_free(struct tc_assertions *a)
{
    for (size_t i = 0; i < a->n; i++) {
        tc_query_free(&a->a[i].q);
        tc_pred_free(&a->a[i].p);
    }
    free(a->a);
    memset(a, 0, sizeof *a);
}

/* whether t's selector matches the cell; *spec its number of terms */
static int entry_matches(const struct tc_tabkey *t, const struct tc_config *conf, const struct tc_matrix *m,
                         size_t cell, const char *case_name, int axes_too, size_t *spec, int *oom)
{
    struct tc_selector sel;
    char msg[300];
    int r;

    *spec = 0;
    if (!t->sel)
        return 1;
    r = tc_selector_parse(t->sel, 1, &sel, msg, sizeof msg);
    if (r) {
        *oom = r == -2;
        return 0;
    }
    *spec = sel.n;
    if (axes_too)
        r = tc_selector_matches(&sel, conf, m, cell, case_name);
    else {
        /* the case terms only */
        r = 1;
        for (size_t i = 0; i < sel.n; i++)
            if (!sel.t[i].def && (!case_name || strcmp(sel.t[i].value, case_name)))
                r = 0;
    }
    tc_selector_free(&sel);
    return r;
}

int tc_cell_faulty(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const char *case_name)
{
    for (size_t i = 0; i < conf->ndeviations; i++) {
        size_t spec;
        int oom = 0;

        if (entry_matches(&conf->deviations[i], conf, m, cell, case_name, 1, &spec, &oom))
            return 1;
    }
    for (size_t i = 0; i < conf->nends; i++) {
        size_t spec;
        int oom = 0;

        if (entry_matches(&conf->ends[i], conf, m, cell, case_name, 1, &spec, &oom))
            return 1;
    }
    return 0;
}

static int push(struct tc_assertions *out, const char *query, const char *pred, const char *sel,
                const struct tc_origin *at, int deviation)
{
    struct tc_assertion *a = realloc(out->a, (out->n + 1) * sizeof *a);
    char msg[300];

    if (!a)
        return -2;
    out->a = a;
    a = &a[out->n];
    memset(a, 0, sizeof *a);
    a->query = query;
    a->pred = pred;
    a->sel = sel;
    a->at = *at;
    a->deviation = deviation;
    if (tc_query_parse(query, &a->q, msg, sizeof msg) || tc_pred_parse(pred, a->q.kind, &a->p, msg, sizeof msg)) {
        tc_query_free(&a->q);
        return -1;
    }
    out->n++;
    return 0;
}

/* the best entry of v for query among the matching ones: the nearest file
   (by_depth), then the most specific selector; -1 none, -2 a tie (err),
   -3 out of memory */
static long best(const struct tc_tabkey *v, size_t n, const char *query, const struct tc_config *conf,
                 const struct tc_matrix *m, size_t cell, const char *case_name, int axes_too, int by_depth,
                 char *err, size_t errlen)
{
    long b = -1;
    size_t bspec = 0;

    for (size_t i = 0; i < n; i++) {
        size_t spec;
        int oom = 0;

        if (strcmp(v[i].key, query))
            continue;
        if (!entry_matches(&v[i], conf, m, cell, case_name, axes_too, &spec, &oom)) {
            if (oom)
                return -3;
            continue;
        }
        if (b < 0 || (by_depth && v[i].at.depth < v[b].at.depth)
            || ((!by_depth || v[i].at.depth == v[b].at.depth) && spec > bspec)) {
            b = (long)i;
            bspec = spec;
            continue;
        }
        if ((!by_depth || v[i].at.depth == v[b].at.depth) && spec == bspec
            && strcmp(v[i].value.str, v[b].value.str)) {
            tc_err(err, errlen, "%s:%u: %s: two assertions apply to cell %s alike (%s:%u)", v[i].at.file,
                   v[i].at.line, query, m->cells[cell].name, v[b].at.file, v[b].at.line);
            return -2;
        }
    }
    return b;
}

int tc_assertions_for(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                      const char *case_name, int mode, struct tc_assertions *out, char *err, size_t errlen)
{
    /* the queries: those of the deviations, the assertions and the defaults */
    const char **queries = malloc((conf->nasserts + conf->ndeviations + 8) * sizeof *queries);
    size_t nq = 0;
    int r = 0;
    static const struct tc_origin def_at = { "(default)", 0, UINT_MAX };

    memset(out, 0, sizeof *out);
    err[0] = 0;
    if (!queries)
        return -2;
    for (size_t i = 0; i < conf->ndeviations + conf->nasserts + sizeof defaults / sizeof defaults[0]; i++) {
        const char *q = i < conf->ndeviations ? conf->deviations[i].key
                        : i < conf->ndeviations + conf->nasserts ? conf->asserts[i - conf->ndeviations].key
                        : defaults[i - conf->ndeviations - conf->nasserts].query;
        size_t j;

        if (i < conf->ndeviations && mode == TC_PLAIN)
            continue;
        for (j = 0; j < nq && strcmp(queries[j], q); j++)
            ;
        if (j == nq)
            queries[nq++] = q;
    }
    for (size_t j = 0; j < nq && r == 0; j++) {
        long b = -1;

        if (mode != TC_PLAIN) {
            b = best(conf->deviations, conf->ndeviations, queries[j], conf, m, cell, case_name,
                     mode == TC_DEVIATED, 0, err, errlen);
            if (b >= 0) {
                const struct tc_tabkey *t = &conf->deviations[b];

                r = push(out, t->key, t->value.str, t->sel, &t->at, 1);
                continue;
            }
        }
        if (b == -1)
            b = best(conf->asserts, conf->nasserts, queries[j], conf, m, cell, case_name, 1, 1, err, errlen);
        if (b >= 0) {
            const struct tc_tabkey *t = &conf->asserts[b];

            r = push(out, t->key, t->value.str, t->sel, &t->at, 0);
            continue;
        }
        if (b == -2) {
            r = -1;
            break;
        }
        if (b == -3) {
            r = -2;
            break;
        }
        for (size_t d = 0; d < sizeof defaults / sizeof defaults[0]; d++)
            if (!strcmp(defaults[d].query, queries[j]))
                r = push(out, defaults[d].query, defaults[d].pred, NULL, &def_at, 0);
    }
    free(queries);
    if (r == -1 && !err[0])
        tc_err(err, errlen, "a malformed assertion");
    if (r == -2)
        tc_err(err, errlen, "out of memory");
    if (r)
        tc_assertions_free(out);
    return r;
}

/* ------------------------------------------------------------ alternative ends */

int tc_query_same(const struct tc_query *a, const struct tc_query *b)
{
    if (a->kind != b->kind)
        return 0;
    if (a->kind != TC_Q_CHECK)
        return 1;
    return a->ns && b->ns && a->rest && b->rest && !strcmp(a->ns, b->ns) && !strcmp(a->rest, b->rest);
}

void tc_ends_free(struct tc_ends *e)
{
    for (size_t i = 0; i < e->n; i++) {
        for (size_t k = 0; k < e->e[i].nalt; k++) {
            struct tc_end_alt *a = &e->e[i].alt[k];

            tc_assertions_free(&a->as);
            for (size_t j = 0; j < a->ntexts; j++)
                free(a->texts[j]);
            free(a->texts);
        }
        free(e->e[i].alt);
    }
    free(e->e);
    memset(e, 0, sizeof *e);
}

int tc_ends_name(const struct tc_ends *e, const struct tc_query *q)
{
    for (size_t i = 0; i < e->n; i++)
        for (size_t k = 0; k < e->e[i].nalt; k++)
            for (size_t j = 0; j < e->e[i].alt[k].as.n; j++)
                if (tc_query_same(&e->e[i].alt[k].as.a[j].q, q))
                    return 1;
    return 0;
}

/* the alternative t (a key of the table of end) added to end */
static int add_alt(struct tc_end *end, const struct tc_tabkey *t, char *err, size_t errlen)
{
    struct tc_end_alt *v = realloc(end->alt, (end->nalt + 1) * sizeof *v), *a;

    if (!v)
        return -2;
    end->alt = v;
    a = &v[end->nalt++];
    memset(a, 0, sizeof *a);
    a->name = t->key;
    if (!(a->texts = calloc(2 * t->value.nlist + 1, sizeof *a->texts)))
        return -2;
    for (size_t k = 0; k < t->value.nlist; k++) {
        char *q, *p;
        int r = tc_end_split(t->value.list[k], &q, &p);

        if (r == -2)
            return -2;
        if (r)
            return tc_err(err, errlen, "%s:%u: the alternative end %s: \"%s\": QUERY PREDICATE expected", t->at.file,
                          t->at.line, t->key, t->value.list[k]);
        a->texts[a->ntexts++] = q;
        a->texts[a->ntexts++] = p;
        r = push(&a->as, q, p, end->sel, &t->at, 1);
        if (r == -2)
            return -2;
        if (r)
            return tc_err(err, errlen, "%s:%u: the alternative end %s: \"%s\" is malformed", t->at.file, t->at.line,
                          t->key, t->value.list[k]);
    }
    return 0;
}

int tc_ends_for(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const char *case_name,
                int mode, struct tc_ends *out, char *err, size_t errlen)
{
    int r = 0;

    memset(out, 0, sizeof *out);
    if (mode == TC_PLAIN)
        return 0;
    for (size_t i = 0; i < conf->nends && r == 0; i++) {
        const struct tc_tabkey *t = &conf->ends[i];
        size_t spec, e;
        int oom = 0;

        if (!entry_matches(t, conf, m, cell, case_name, mode == TC_DEVIATED, &spec, &oom)) {
            if (oom)
                r = -2;
            continue;
        }
        /* the end of its selector, a new one at its first key */
        for (e = 0; e < out->n && strcmp(out->e[e].sel, t->sel); e++)
            ;
        if (e == out->n) {
            struct tc_end *v = realloc(out->e, (out->n + 1) * sizeof *v);

            if (!v) {
                r = -2;
                break;
            }
            out->e = v;
            memset(&v[out->n], 0, sizeof v[out->n]);
            v[out->n].sel = t->sel;
            v[out->n].at = t->at;
            out->n++;
        }
        if (!strcmp(t->key, "reason")) {
            out->e[e].reason = t->value.str;
            out->e[e].at = t->at;
        } else
            r = add_alt(&out->e[e], t, err, errlen);
    }
    if (r == -2)
        tc_err(err, errlen, "out of memory");
    if (r)
        tc_ends_free(out);
    return r;
}
