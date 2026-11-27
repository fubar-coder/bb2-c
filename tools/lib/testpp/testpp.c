/*
 * testpp.c - the preprocessor of the Blitz test sources (testpp.h).
 */
#include "testpp.h"
#include "testpp_int.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ the scanner */

char *tp_strndup(const char *s, size_t n)
{
    char *r = malloc(n + 1);

    if (r) {
        if (n)
            memcpy(r, s, n);
        r[n] = 0;
    }
    return r;
}

int tp_error(struct tp_errors *e, const char *file, unsigned line, const char *fmt, ...)
{
    char buf[600];
    int n = snprintf(buf, sizeof buf, "%s:%u: ", file, line);
    va_list ap;
    char **v;

    if (n < 0)
        n = 0;
    va_start(ap, fmt);
    vsnprintf(buf + n, sizeof buf - (size_t)n, fmt, ap);
    va_end(ap);
    v = realloc(e->v, (e->n + 1) * sizeof *v);
    if (!v)
        return -1;
    e->v = v;
    if (!(v[e->n] = tp_strndup(buf, strlen(buf))))
        return -1;
    e->n++;
    return 0;
}

size_t tp_blanks(const char *l, size_t p, size_t e)
{
    while (p < e && (l[p] == ' ' || l[p] == '\t'))
        p++;
    return p;
}

static int is_ident(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

size_t tp_statements(const char *l, size_t len, struct tp_stmt *st, size_t max, size_t *comment)
{
    size_t n = 0, start = 0, p = 0;
    int nest = 0;

    *comment = len;
    for (;;) {
        int end = p >= len;
        unsigned char c = end ? 0 : (unsigned char)l[p];

        if (!end && c == '"') {
            for (p++; p < len && l[p] != '"'; p++)
                ;
            if (p < len)
                p++;
            continue;
        }
        if (!end && c == ';') {
            *comment = p;
            end = 1;
        }
        if (end || (c == ':' && nest <= 0)) {
            size_t s = tp_blanks(l, start, p), e = end ? (*comment < len ? *comment : len) : p;

            while (e > s && (l[e - 1] == ' ' || l[e - 1] == '\t' || l[e - 1] == '\r'))
                e--;
            if (e > s && n < max) {
                st[n].start = s;
                st[n].end = e;
                n++;
            }
            if (end)
                return n;
            start = ++p;
            continue;
        }
        if (c == '(' || c == '{')
            nest++;
        else if (c == ')' || c == '}')
            nest--;
        p++;
    }
}

int tp_word(const char *l, size_t s, size_t e, const char *w, size_t *after)
{
    size_t n = strlen(w);

    if (e - s < n)
        return 0;
    for (size_t i = 0; i < n; i++)
        if (tolower((unsigned char)l[s + i]) != tolower((unsigned char)w[i]))
            return 0;
    if (s + n < e && is_ident((unsigned char)l[s + n]))
        return 0;
    if (after)
        *after = s + n;
    return 1;
}

int tp_args(const char *l, size_t len, size_t open, struct tp_stmt *args, int max, size_t *close)
{
    size_t p = open + 1, start = p;
    int n = 0, nest = 0, quote = 0;

    for (;;) {
        unsigned char c;

        if (p >= len)
            return -1;
        c = (unsigned char)l[p++];
        if (c == '"') {
            quote = !quote;
            continue;
        }
        if (quote)
            continue;
        if ((c == ',' && nest == 0) || (c == '}' && nest == 0)) {
            if (n < max) {
                args[n].start = start;
                args[n].end = p - 1;
            }
            n++;
            start = p;
            if (c == '}') {
                *close = p - 1;
                return n;
            }
            continue;
        }
        if (c == '{' || c == '(')
            nest++;
        else if (c == '}' || c == ')')
            nest--;
    }
}

size_t tp_findjoin(const char *l, size_t len)
{
    int quote = 0;

    for (size_t p = 0; p < len; p++) {
        if (l[p] == '"')
            quote = !quote;
        else if (!quote && l[p] == ';')
            break;
        else if (!quote && l[p] == '@' && p + 1 < len && l[p + 1] == '@')
            return p;
    }
    return len;
}

int tp_join(const char *p, const char *end, char **text, size_t *len, unsigned *extra, const char **next)
{
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    size_t n = nl ? (size_t)(nl - p) : (size_t)(end - p), at = tp_findjoin(p, n), tn = 0;
    unsigned k = 0;
    char *t = NULL;

    if (at == n)
        return 0;
    for (;;) {
        char *v;

        if (at == n) {
            /* the last line of the join: all of it */
            if (!(v = realloc(t, tn + n + 1)))
                break;
            t = v;
            memcpy(t + tn, p, n);
            tn += n;
            t[tn] = 0;
            *text = t;
            *len = tn;
            *extra = k;
            *next = nl ? nl + 1 : end;
            return 1;
        }
        if (!nl || nl + 1 == end) {
            free(t);
            *extra = k;
            return 2;
        }
        if (!(v = realloc(t, tn + at + 1)))
            break;
        t = v;
        memcpy(t + tn, p, at);
        tn += at;
        p = nl + 1;
        k++;
        nl = memchr(p, '\n', (size_t)(end - p));
        n = nl ? (size_t)(nl - p) : (size_t)(end - p);
        at = tp_findjoin(p, n);
    }
    free(t);
    return -1;
}

int tp_block_macro(const char *l, size_t s, size_t e, size_t len, const char **name, size_t *nlen)
{
    static const struct {
        const char *name;
        int b;
    } m[] = {
        { "TestBatchInit", TP_B_INIT }, { "EndTestBatchInit", TP_B_END_INIT },
        { "TestCase", TP_B_CASE }, { "EndTestCase", TP_B_END_CASE },
        { "TestBatchCleanup", TP_B_CLEANUP }, { "EndTestBatchCleanup", TP_B_END_CLEANUP },
    };
    size_t p = s + 1, ne;

    *nlen = 0;
    if (s >= e || l[s] != '!')
        return TP_B_NONE;
    for (ne = p; ne < e && is_ident((unsigned char)l[ne]); ne++)
        ;
    for (size_t i = 0; i < sizeof m / sizeof m[0]; i++)
        if (strlen(m[i].name) == ne - p && !memcmp(l + p, m[i].name, ne - p)) {
            if (m[i].b == TP_B_CASE) {
                size_t q = ne;
                struct tp_stmt a[2];
                size_t close;

                while (q < len && l[q] == ' ')
                    q++;
                if (q < len && l[q] == '{' && tp_args(l, len, q, a, 2, &close) == 1) {
                    size_t as = tp_blanks(l, a[0].start, a[0].end), ae = a[0].end;

                    while (ae > as && (l[ae - 1] == ' ' || l[ae - 1] == '\t'))
                        ae--;
                    *name = l + as;
                    *nlen = ae - as;
                }
            }
            return m[i].b;
        }
    return TP_B_NONE;
}

/* ------------------------------------------------------------ the preprocessor */

static const char *const kind_names[] = { "Check", "CheckEq", "CheckStrEq", "Require", "ExpectError",
                                          "CheckQEq", "CheckFltNear", "RunProgram" };

const char *tp_kind_name(int kind)
{
    return kind >= 0 && kind <= TP_RUNPROGRAM ? kind_names[kind] : "?";
}

/* the number of arguments of a kind without the name */
static const int nargs[] = { 1, 2, 2, 1, 1, 2, 3, 1 };

struct buf {
    char *p;
    size_t n, cap;
};

static int put(struct buf *b, const char *s, size_t n)
{
    if (b->n + n + 1 > b->cap) {
        size_t cap = (b->cap ? b->cap * 2 : 4096) + n;
        char *p = realloc(b->p, cap);

        if (!p)
            return -1;
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
    return 0;
}

struct nscount {
    char *ns;
    unsigned n;
};

struct ctx {
    const struct tp_params *par;
    struct tp_source *src;
    unsigned line;
    int in_macro;
    unsigned macro_line;
    char *macro_name;
    char *proc;
    int block;                  /* TP_B_NONE, TP_B_INIT, TP_B_CASE, TP_B_CLEANUP */
    char *case_name;
    unsigned k;
    struct nscount *nsc;
    size_t nnsc;
    int oom;
};

static int err(struct ctx *c, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static int err(struct ctx *c, const char *fmt, ...)
{
    char buf[500];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (tp_error(&c->src->errors, c->par->file, c->line, "%s", buf))
        c->oom = 1;
    return -1;
}

/* the check kind of the name l[s..e), -1 none */
static int check_kind(const char *l, size_t s, size_t e)
{
    for (int i = 0; i <= TP_RUNPROGRAM; i++)
        if (strlen(kind_names[i]) == e - s && !memcmp(l + s, kind_names[i], e - s))
            return i;
    return -1;
}

/* a string literal "..." without a " inside: its contents */
static int literal(const char *l, size_t s, size_t e, char **out)
{
    s = tp_blanks(l, s, e);
    while (e > s && (l[e - 1] == ' ' || l[e - 1] == '\t'))
        e--;
    if (e - s < 2 || l[s] != '"' || l[e - 1] != '"' || memchr(l + s + 1, '"', e - s - 2))
        return 0;
    *out = tp_strndup(l + s + 1, e - s - 2);
    return 1;
}

static char *trimmed(const char *l, size_t s, size_t e)
{
    s = tp_blanks(l, s, e);
    while (e > s && (l[e - 1] == ' ' || l[e - 1] == '\t'))
        e--;
    return tp_strndup(l + s, e - s);
}

static unsigned next_in_ns(struct ctx *c, const char *ns)
{
    struct nscount *v;

    for (size_t i = 0; i < c->nnsc; i++)
        if (!strcmp(c->nsc[i].ns, ns))
            return c->nsc[i].n++;
    v = realloc(c->nsc, (c->nnsc + 1) * sizeof *v);
    if (!v || !(v[c->nnsc].ns = tp_strndup(ns, strlen(ns)))) {
        if (v)
            c->nsc = v;
        c->oom = 1;
        return 0;
    }
    c->nsc = v;
    v[c->nnsc].n = 1;
    c->nnsc++;
    return 0;
}

/*
 * one check call: l[bang] is the '!', the name ends at l[ne]; writes the
 * rewritten call to out; *end = the offset behind the call.  Returns 0,
 * or -1 after a message (out then holds the call unchanged).
 */
static int one_call(struct ctx *c, const char *l, size_t len, size_t bang, size_t ne, int kind,
                    struct buf *out, size_t *end)
{
    struct tp_stmt a[8];
    size_t open = ne, close;
    int n, want = nargs[kind];
    struct tp_check chk;
    char kbuf[16];

    if (c->par->data) {
        /* a data source (basic-test-local-data): its program writes no
           result file */
        *end = ne;
        put(out, l + bang, ne - bang);
        return err(c, "!%s in a data source: a program the test builds has no result file (its observations are "
                   "its output, return code and requesters)", kind_names[kind]);
    }
    while (open < len && l[open] == ' ')
        open++;
    if (open >= len || l[open] != '{') {
        *end = ne;
        put(out, l + bang, ne - bang);
        return err(c, "!%s needs its arguments in braces", kind_names[kind]);
    }
    n = tp_args(l, len, open, a, 8, &close);
    if (n < 0) {
        *end = len;
        put(out, l + bang, len - bang);
        return err(c, "!%s: the line ends before the closing }", kind_names[kind]);
    }
    *end = close + 1;
    if (n > 8 || (n != want && n != want + 1)) {
        put(out, l + bang, *end - bang);
        if (kind == TP_EXPECTERROR)
            return err(c, "!ExpectError takes the message and an optional name, %d arguments given", n);
        return err(c, "!%s takes %d argument%s and an optional name, %d given", kind_names[kind], want,
                   want == 1 ? "" : "s", n);
    }
    for (int i = 0; i < n; i++)
        if (tp_blanks(l, a[i].start, a[i].end) == a[i].end) {
            put(out, l + bang, *end - bang);
            return err(c, "!%s: argument %d is empty", kind_names[kind], i + 1);
        }
    /* no check in the arguments of a check */
    for (size_t p = open + 1; p < close; p++) {
        if (l[p] == '"') {
            for (p++; p < close && l[p] != '"'; p++)
                ;
            continue;
        }
        if (l[p] == '!') {
            size_t q = p + 1;

            while (q < close && is_ident((unsigned char)l[q]))
                q++;
            if (check_kind(l, p + 1, q) >= 0) {
                put(out, l + bang, *end - bang);
                return err(c, "!%s inside the arguments of !%s", kind_names[check_kind(l, p + 1, q)],
                           kind_names[kind]);
            }
        }
    }

    memset(&chk, 0, sizeof chk);
    chk.kind = kind;
    chk.line = c->line;
    if (n == want + 1 && !literal(l, a[want].start, a[want].end, &chk.name)) {
        put(out, l + bang, *end - bang);
        return err(c, "the name of !%s must be a string literal", kind_names[kind]);
    }
    if (kind == TP_EXPECTERROR) {
        if (!literal(l, a[0].start, a[0].end, &chk.message)) {
            free(chk.name);
            put(out, l + bang, *end - bang);
            return err(c, "the message of !ExpectError must be a string literal");
        }
    } else {
        chk.expr = trimmed(l, a[0].start, a[0].end);
        if (want >= 2)
            chk.expected = trimmed(l, a[1].start, a[1].end);
        if (want == 3)
            chk.delta = trimmed(l, a[2].start, a[2].end);
    }

    /* the namespace */
    if (c->in_macro) {
        put(out, l + bang, *end - bang);
        err(c, "!%s inside the macro %s (line %u): checks stand in the blocks and procedures", kind_names[kind],
            c->macro_name ? c->macro_name : "?", c->macro_line);
        goto drop;
    }
    if (c->proc) {
        chk.nskind = TP_NS_PROC;
        chk.ns = tp_strndup(c->proc, strlen(c->proc));
    } else if (c->block == TP_B_CASE && c->case_name) {
        chk.nskind = TP_NS_CASE;
        chk.ns = tp_strndup(c->case_name, strlen(c->case_name));
    } else if (c->block == TP_B_INIT) {
        chk.nskind = TP_NS_INIT;
        chk.ns = tp_strndup("init", 4);
    } else if (c->block == TP_B_CLEANUP) {
        chk.nskind = TP_NS_CLEANUP;
        chk.ns = tp_strndup("cleanup", 7);
    } else {
        put(out, l + bang, *end - bang);
        err(c, "!%s outside the blocks and procedures", kind_names[kind]);
        goto drop;
    }
    if (!chk.ns)
        goto oom;
    if (chk.name)
        for (size_t i = 0; i < c->src->nchecks; i++)
            if (c->src->checks[i].name && !strcmp(c->src->checks[i].name, chk.name)
                && !strcmp(c->src->checks[i].ns, chk.ns)) {
                put(out, l + bang, *end - bang);
                err(c, "the check name \"%s\" is used twice in %s (line %u)", chk.name, chk.ns,
                    c->src->checks[i].line);
                goto drop;
            }
    chk.k = c->k++;
    chk.number = next_in_ns(c, chk.ns);
    {
        struct tp_check *v = realloc(c->src->checks, (c->src->nchecks + 1) * sizeof *v);

        if (!v)
            goto oom;
        c->src->checks = v;
        v[c->src->nchecks++] = chk;
    }

    /* the rewritten call: !NAME{K,ARG...} */
    snprintf(kbuf, sizeof kbuf, "%u", chk.k);
    if (put(out, l + bang, open + 1 - bang) || put(out, kbuf, strlen(kbuf)))
        c->oom = 1;
    if (kind != TP_EXPECTERROR)
        for (int i = 0; i < want; i++)
            if (put(out, ",", 1) || put(out, l + a[i].start, a[i].end - a[i].start))
                c->oom = 1;
    if (put(out, "}", 1))
        c->oom = 1;
    return 0;
oom:
    c->oom = 1;
drop:
    free(chk.ns);
    free(chk.name);
    free(chk.expr);
    free(chk.expected);
    free(chk.delta);
    free(chk.message);
    return -1;
}

/* the context of a statement: macro definitions, procedures, blocks */
static void context(struct ctx *c, const char *l, size_t len, size_t s, size_t e)
{
    size_t p;
    const char *name;
    size_t nlen;
    int b;

    if (tp_word(l, s, e, "End", &p)) {
        p = tp_blanks(l, p, e);
        if (tp_word(l, p, e, "Macro", NULL)) {
            c->in_macro = 0;
            free(c->macro_name);
            c->macro_name = NULL;
        } else if (!c->in_macro && (tp_word(l, p, e, "Statement", NULL) || tp_word(l, p, e, "Function", NULL))) {
            free(c->proc);
            c->proc = NULL;
        }
        return;
    }
    if (c->in_macro)
        return;
    if (tp_word(l, s, e, "Macro", &p)) {
        size_t q;

        p = tp_blanks(l, p, e);
        for (q = p; q < e && is_ident((unsigned char)l[q]); q++)
            ;
        c->in_macro = 1;
        c->macro_line = c->line;
        free(c->macro_name);
        c->macro_name = tp_strndup(l + p, q - p);
        return;
    }
    if (tp_word(l, s, e, "Statement", &p) || tp_word(l, s, e, "Function", &p)) {
        size_t q;

        if (p < e && l[p] == '.')       /* Function.l */
            for (p++; p < e && is_ident((unsigned char)l[p]); p++)
                ;
        p = tp_blanks(l, p, e);
        if (tp_word(l, p, e, "Return", NULL))
            return;
        for (q = p; q < e && is_ident((unsigned char)l[q]); q++)
            ;
        if (q > p) {
            free(c->proc);
            c->proc = tp_strndup(l + p, q - p);
        }
        return;
    }
    b = tp_block_macro(l, s, e, len, &name, &nlen);
    switch (b) {
    case TP_B_INIT:
    case TP_B_CLEANUP:
        c->block = b;
        break;
    case TP_B_CASE:
        c->block = b;
        free(c->case_name);
        c->case_name = nlen ? tp_strndup(name, nlen) : NULL;
        break;
    case TP_B_END_INIT:
    case TP_B_END_CASE:
    case TP_B_END_CLEANUP:
        c->block = TP_B_NONE;
        free(c->case_name);
        c->case_name = NULL;
        break;
    }
    if (b == TP_B_CASE && nlen) {
        struct tp_case *v = realloc(c->src->cases, (c->src->ncases + 1) * sizeof *v);

        if (!v) {
            c->oom = 1;
            return;
        }
        c->src->cases = v;
        v[c->src->ncases].name = tp_strndup(name, nlen);
        v[c->src->ncases].line = c->line;
        v[c->src->ncases].end_line = 0;
        c->src->ncases++;
    }
    if (b == TP_B_END_CASE && c->src->ncases && !c->src->cases[c->src->ncases - 1].end_line)
        c->src->cases[c->src->ncases - 1].end_line = c->line;
    if (b == TP_B_INIT && !c->src->init_line)
        c->src->init_line = c->line;
    if (b == TP_B_CLEANUP && !c->src->cleanup_line)
        c->src->cleanup_line = c->line;
}

static int ph_char(int ch, int first)
{
    return (ch >= 'A' && ch <= 'Z') || (!first && ((ch >= '0' && ch <= '9') || ch == '_'));
}

/* the placeholders of a line (code and strings, not the comment) to out */
static void placeholders(struct ctx *c, const char *l, size_t len, struct buf *out)
{
    size_t p = 0;
    int quote = 0;
    char num[16];

    while (p < len) {
        char ch = l[p];

        if (ch == '"')
            quote = !quote;
        if (ch == ';' && !quote) {
            put(out, l + p, len - p);
            return;
        }
        if (ch == '@' && p + 1 < len && ph_char((unsigned char)l[p + 1], 1)) {
            size_t q = p + 1;

            while (q < len && ph_char((unsigned char)l[q], 0))
                q++;
            if (q < len && l[q] == '@') {
                size_t n = q - p - 1;
                const char *v = NULL;

                if (n == 4 && !memcmp(l + p + 1, "LINE", 4)) {
                    snprintf(num, sizeof num, "%u", c->line);
                    v = num;
                } else if (n == 4 && !memcmp(l + p + 1, "CASE", 4)) {
                    if (c->block == TP_B_CASE && c->case_name)
                        v = c->case_name;
                    else {
                        err(c, "@CASE@ outside a case");
                        v = "";
                    }
                } else if (n == 4 && !memcmp(l + p + 1, "TEST", 4)) {
                    v = c->par->test_id ? c->par->test_id : "";
                } else if (n == 8 && !memcmp(l + p + 1, "TESTDATA", 8) && c->par->testdata) {
                    v = c->par->testdata;
                } else if (n == 7 && !memcmp(l + p + 1, "TESTDIR", 7) && c->par->testdir) {
                    v = c->par->testdir;
                } else {
                    err(c, "unknown placeholder %.*s", (int)(q - p + 1), l + p);
                    v = "";
                }
                if (put(out, v, strlen(v)))
                    c->oom = 1;
                p = q + 1;
                continue;
            }
        }
        if (put(out, l + p, 1))
            c->oom = 1;
        p++;
    }
}

/* one line of the source to out */
static void one_line(struct ctx *c, const char *l, size_t len, struct buf *out)
{
    struct tp_stmt st[64];
    size_t comment, nst = tp_statements(l, len, st, 64, &comment), si = 0;
    struct buf line = { 0 };
    size_t p = 0, copied = 0;
    int quote = 0;

    /* the calls, each in the context of its statement */
    for (;;) {
        unsigned char ch;

        while (si < nst && st[si].start <= p) {
            context(c, l, len, st[si].start, st[si].end);
            si++;
        }
        if (p >= len)
            break;
        ch = (unsigned char)l[p];
        if (ch == '"') {
            quote = !quote;
            p++;
            continue;
        }
        if (quote) {
            p++;
            continue;
        }
        if (ch == ';')
            break;
        if (ch == '!') {
            size_t q = p + 1;
            int kind;

            while (q < len && is_ident((unsigned char)l[q]))
                q++;
            kind = check_kind(l, p + 1, q);
            if (kind >= 0) {
                size_t end;

                if (put(&line, l + copied, p - copied))
                    c->oom = 1;
                one_call(c, l, len, p, q, kind, &line, &end);
                p = copied = end;
                continue;
            }
            p = q;
            continue;
        }
        p++;
    }
    while (si < nst) {
        context(c, l, len, st[si].start, st[si].end);
        si++;
    }
    if (put(&line, l + copied, len - copied))
        c->oom = 1;
    placeholders(c, line.p ? line.p : "", line.n, out);
    free(line.p);
}

void tp_free(struct tp_source *s)
{
    free(s->text);
    for (size_t i = 0; i < s->nchecks; i++) {
        free(s->checks[i].ns);
        free(s->checks[i].name);
        free(s->checks[i].expr);
        free(s->checks[i].expected);
        free(s->checks[i].delta);
        free(s->checks[i].message);
    }
    free(s->checks);
    for (size_t i = 0; i < s->ncases; i++)
        free(s->cases[i].name);
    free(s->cases);
    for (size_t i = 0; i < s->errors.n; i++)
        free(s->errors.v[i]);
    free(s->errors.v);
    memset(s, 0, sizeof *s);
}

const struct tp_check *tp_find(const struct tp_source *src, const char *ns, const char *name)
{
    for (size_t i = 0; i < src->nchecks; i++)
        if (src->checks[i].name && !strcmp(src->checks[i].ns, ns) && !strcmp(src->checks[i].name, name))
            return &src->checks[i];
    return NULL;
}

const struct tp_check *tp_by_k(const struct tp_source *src, unsigned k)
{
    for (size_t i = 0; i < src->nchecks; i++)
        if (src->checks[i].k == k)
            return &src->checks[i];
    return NULL;
}

/* whether the line l[0..len) holds a check macro outside strings and the
   comment */
static int has_check(const char *l, size_t len)
{
    int quote = 0;

    for (size_t p = 0; p < len; p++) {
        if (l[p] == '"')
            quote = !quote;
        else if (!quote && l[p] == ';')
            return 0;
        else if (!quote && l[p] == '!') {
            size_t q = p + 1;

            while (q < len && is_ident((unsigned char)l[q]))
                q++;
            if (check_kind(l, p + 1, q) >= 0)
                return 1;
        }
    }
    return 0;
}

int tp_process(const char *text, size_t len, const struct tp_params *par, struct tp_source *src)
{
    struct ctx c;
    struct buf out = { 0 };
    const char *p = text, *end = text + len;

    memset(src, 0, sizeof *src);
    memset(&c, 0, sizeof c);
    c.par = par;
    c.src = src;
    if (memchr(text, 0, len)) {
        c.line = 1;
        for (const char *q = text; *q; q++)
            c.line += *q == '\n';
        err(&c, "a 0 byte in the source");
        return c.oom ? -1 : (int)src->errors.n;
    }
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        size_t n = nl ? (size_t)(nl - p) : (size_t)(end - p);

        c.line++;
        if (par->ab3) {
            char *j;
            size_t jn;
            unsigned extra;
            const char *next;
            int r = tp_join(p, end, &j, &jn, &extra, &next);

            if (r < 0) {
                c.oom = 1;
                break;
            }
            if (r == 2) {
                /* the compiler: "No line to join with @@" */
                c.line += extra;
                err(&c, "the line ends with @@ and no line follows to join it with");
                break;
            }
            if (r == 1 && has_check(j, jn)) {
                /* the check on one line, an empty line for each line joined */
                one_line(&c, j, jn, &out);
                free(j);
                for (unsigned i = 0; i < extra; i++)
                    if (put(&out, "\n", 1))
                        c.oom = 1;
                c.line += extra;
                if (next > text && next[-1] == '\n' && put(&out, "\n", 1))
                    c.oom = 1;
                p = next;
                continue;
            }
            if (r == 1)
                free(j);
        }
        one_line(&c, p, n, &out);
        if (nl && put(&out, "\n", 1))
            c.oom = 1;
        p = nl ? nl + 1 : end;
    }
    /* behind a refused "@@" in the last line: the rest as it is (c.line
       counts its lines already) */
    if (p < end && put(&out, p, (size_t)(end - p)))
        c.oom = 1;
    src->nlines = c.line;
    /* a procedure with checks named like another namespace */
    for (size_t i = 0; i < src->nchecks; i++) {
        const struct tp_check *k = &src->checks[i];
        int clash = 0;

        if (k->nskind != TP_NS_PROC || (i && !strcmp(src->checks[i - 1].ns, k->ns)))
            continue;
        clash = !strcmp(k->ns, "init") || !strcmp(k->ns, "cleanup");
        for (size_t j = 0; j < src->ncases && !clash; j++)
            clash = !strcmp(src->cases[j].name, k->ns);
        if (clash) {
            c.line = k->line;
            err(&c, "the procedure %s holds checks and has the name of a case or of init or cleanup", k->ns);
        }
    }
    free(c.macro_name);
    free(c.proc);
    free(c.case_name);
    for (size_t i = 0; i < c.nnsc; i++)
        free(c.nsc[i].ns);
    free(c.nsc);
    if (!out.p && !(out.p = calloc(1, 1)))
        c.oom = 1;
    src->text = out.p;
    src->len = out.n;
    if (c.oom)
        return -1;
    return (int)src->errors.n;
}
