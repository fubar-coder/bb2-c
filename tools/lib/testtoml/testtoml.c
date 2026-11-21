/*
 * testtoml.c - the strict TOML subset of the run configuration
 * (testtoml.h).
 */
#include "testtoml.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the state of one parse */
struct pstate {
    const char *file;
    unsigned line;
    char *err;
    size_t errlen;
    int oom;
};

static int fail(struct pstate *s, const char *fmt, ...)
{
    va_list ap;
    int n = snprintf(s->err, s->errlen, "%s:%u: ", s->file, s->line);

    if (n < 0)
        n = 0;
    if ((size_t)n < s->errlen) {
        va_start(ap, fmt);
        vsnprintf(s->err + n, s->errlen - (size_t)n, fmt, ap);
        va_end(ap);
    }
    return -1;
}

static int oom(struct pstate *s)
{
    s->oom = 1;
    snprintf(s->err, s->errlen, "%s:%u: out of memory", s->file, s->line);
    return -1;
}

static char *dupn(const char *p, size_t n)
{
    char *r = malloc(n + 1);

    if (r) {
        memcpy(r, p, n);
        r[n] = 0;
    }
    return r;
}

static void free_strv(char **v, size_t n)
{
    for (size_t i = 0; i < n; i++)
        free(v[i]);
    free(v);
}

void tt_value_free(struct tt_value *v)
{
    free(v->str);
    free_strv(v->list, v->nlist);
    memset(v, 0, sizeof *v);
}

int tt_value_copy(struct tt_value *out, const struct tt_value *v)
{
    memset(out, 0, sizeof *out);
    out->type = v->type;
    out->integer = v->integer;
    out->boolean = v->boolean;
    if (v->str && !(out->str = dupn(v->str, strlen(v->str))))
        return -1;
    if (v->type == TT_LIST) {
        out->list = calloc(v->nlist ? v->nlist : 1, sizeof *out->list);
        if (!out->list)
            goto fail;
        for (size_t i = 0; i < v->nlist; i++) {
            if (!(out->list[i] = dupn(v->list[i], strlen(v->list[i]))))
                goto fail;
            out->nlist++;
        }
    }
    return 0;
fail:
    tt_value_free(out);
    return -1;
}

const char *tt_type_name(int type)
{
    switch (type) {
    case TT_STRING: return "string";
    case TT_INTEGER: return "integer";
    case TT_BOOLEAN: return "boolean";
    case TT_LIST: return "list of strings";
    }
    return "?";
}

void tt_free(struct tt_doc *doc)
{
    for (size_t i = 0; i < doc->n; i++) {
        free_strv(doc->e[i].table, doc->e[i].ntable);
        free(doc->e[i].key);
        tt_value_free(&doc->e[i].value);
    }
    free(doc->e);
    for (size_t i = 0; i < doc->ntables; i++)
        free_strv(doc->tables[i].path, doc->tables[i].npath);
    free(doc->tables);
    free(doc->file);
    memset(doc, 0, sizeof *doc);
}

/* a growing list of strings */
struct strv {
    char **v;
    size_t n, cap;
};

static int strv_push(struct strv *l, char *s)
{
    if (!s)
        return -1;
    if (l->n == l->cap) {
        size_t cap = l->cap ? 2 * l->cap : 4;
        char **v = realloc(l->v, cap * sizeof *v);

        if (!v) {
            free(s);
            return -1;
        }
        l->v = v;
        l->cap = cap;
    }
    l->v[l->n++] = s;
    return 0;
}

static int is_bare(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
           || c == '_' || c == '-';
}

static const char *skip_ws(const char *p, const char *end)
{
    while (p < end && (*p == ' ' || *p == '\t'))
        p++;
    return p;
}

/* the rest of the line from p: blanks and an optional comment only */
static int line_end(struct pstate *s, const char *p, const char *end, const char *what)
{
    p = skip_ws(p, end);
    if (p < end && *p != '#')
        return fail(s, "unexpected text after %s: \"%.*s\"", what, (int)(end - p), p);
    for (; p < end; p++)
        if ((unsigned char)*p < 32 && *p != '\t')
            return fail(s, "a control character in a comment");
    return 0;
}

/*
 * a quoted string at *pp ('"' or '\''): its contents (malloc'd) in *out,
 * *pp behind the closing quote
 */
static int quoted(struct pstate *s, const char **pp, const char *end, char **out)
{
    const char *p = *pp;
    char q = *p++;
    char *buf = malloc((size_t)(end - p) + 1);
    size_t n = 0;

    if (!buf)
        return oom(s);
    if (p + 1 < end && p[0] == q && p[1] == q) {
        free(buf);
        return fail(s, "multi-line strings are not part of the subset");
    }
    for (;;) {
        unsigned char c;

        if (p >= end) {
            free(buf);
            return fail(s, "a string without its closing %c", q);
        }
        c = (unsigned char)*p++;
        if (c == (unsigned char)q)
            break;
        if (c < 32 && c != '\t') {
            free(buf);
            return fail(s, "a control character in a string");
        }
        if (c == 127) {
            free(buf);
            return fail(s, "a control character in a string");
        }
        if (q == '"' && c == '\\') {
            if (p >= end) {
                free(buf);
                return fail(s, "a string without its closing %c", q);
            }
            c = (unsigned char)*p++;
            switch (c) {
            case '"': break;
            case '\\': break;
            case 't': c = '\t'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            default:
                free(buf);
                return fail(s, "the escape \\%c is not part of the subset (only \\\" \\\\ \\t \\n \\r)", c);
            }
        }
        buf[n++] = (char)c;
    }
    buf[n] = 0;
    *out = buf;
    *pp = p;
    return 0;
}

/* a key (bare or quoted) at *pp */
static int key(struct pstate *s, const char **pp, const char *end, char **out)
{
    const char *p = *pp;

    if (p < end && (*p == '"' || *p == '\''))
        return quoted(s, pp, end, out);
    while (p < end && is_bare((unsigned char)*p))
        p++;
    if (p == *pp)
        return fail(s, "a key expected, found \"%.*s\"", (int)(end - *pp), *pp);
    if (!(*out = dupn(*pp, (size_t)(p - *pp))))
        return oom(s);
    *pp = p;
    return 0;
}

static int integer(struct pstate *s, const char *p, const char *e, long long *out)
{
    const char *d = p;
    char buf[32];
    char *stop;
    long long v;

    if (d < e && (*d == '+' || *d == '-'))
        d++;
    if (d == e)
        return fail(s, "a value expected");
    for (const char *q = d; q < e; q++)
        if (*q < '0' || *q > '9')
            return fail(s, "\"%.*s\" is no value of the subset (string, integer, boolean or a list of strings)",
                        (int)(e - p), p);
    if (e - d > 1 && *d == '0')
        return fail(s, "an integer with a leading zero: \"%.*s\"", (int)(e - p), p);
    if ((size_t)(e - p) >= sizeof buf)
        return fail(s, "the integer \"%.*s\" is too large", (int)(e - p), p);
    memcpy(buf, p, (size_t)(e - p));
    buf[e - p] = 0;
    errno = 0;
    v = strtoll(buf, &stop, 10);
    if (errno == ERANGE)
        return fail(s, "the integer \"%s\" is too large", buf);
    *out = v;
    return 0;
}

/* the value at p (up to the end of the line); *rest behind it */
static int value(struct pstate *s, const char *p, const char *end, struct tt_value *v, const char **rest)
{
    memset(v, 0, sizeof *v);
    if (p >= end || *p == '#')
        return fail(s, "a value expected");
    if (*p == '"' || *p == '\'') {
        v->type = TT_STRING;
        if (quoted(s, &p, end, &v->str))
            return -1;
        *rest = p;
        return 0;
    }
    if (*p == '[') {
        struct strv l = {0};

        v->type = TT_LIST;
        p = skip_ws(p + 1, end);
        while (p < end && *p != ']') {
            char *str;

            if (*p != '"' && *p != '\'') {
                free_strv(l.v, l.n);
                if (*p == '#' || p >= end)
                    return fail(s, "a list must stand on one line");
                return fail(s, "a list holds strings only");
            }
            if (quoted(s, &p, end, &str)) {
                free_strv(l.v, l.n);
                return -1;
            }
            if (strv_push(&l, str)) {
                free_strv(l.v, l.n);
                return oom(s);
            }
            p = skip_ws(p, end);
            if (p < end && *p == ',') {
                p = skip_ws(p + 1, end);
                continue;
            }
            if (p < end && *p != ']') {
                free_strv(l.v, l.n);
                return fail(s, "a ',' or ']' expected in the list");
            }
        }
        if (p >= end) {
            free_strv(l.v, l.n);
            return fail(s, "a list must stand on one line");
        }
        if (!l.v && !(l.v = malloc(sizeof *l.v)))
            return oom(s);
        v->list = l.v;
        v->nlist = l.n;
        *rest = p + 1;
        return 0;
    }
    if (*p == '{')
        return fail(s, "inline tables are not part of the subset");
    {
        const char *e = p;

        while (e < end && *e != ' ' && *e != '\t' && *e != '#')
            e++;
        if ((e - p == 4 && !memcmp(p, "true", 4)) || (e - p == 5 && !memcmp(p, "false", 5))) {
            v->type = TT_BOOLEAN;
            v->boolean = *p == 't';
            *rest = e;
            return 0;
        }
        v->type = TT_INTEGER;
        if (integer(s, p, e, &v->integer))
            return -1;
        *rest = e;
        return 0;
    }
}

static int same_path(char **a, size_t na, char **b, size_t nb)
{
    if (na != nb)
        return 0;
    for (size_t i = 0; i < na; i++)
        if (strcmp(a[i], b[i]))
            return 0;
    return 1;
}

/* path a is a prefix of b (shorter) */
static int prefix_path(char **a, size_t na, char **b, size_t nb)
{
    if (na >= nb)
        return 0;
    for (size_t i = 0; i < na; i++)
        if (strcmp(a[i], b[i]))
            return 0;
    return 1;
}

static char *path_text(char **p, size_t n)
{
    size_t len = 1;
    char *r;

    for (size_t i = 0; i < n; i++)
        len += strlen(p[i]) + 1;
    r = malloc(len);
    if (!r)
        return NULL;
    r[0] = 0;
    for (size_t i = 0; i < n; i++) {
        if (i)
            strcat(r, ".");
        strcat(r, p[i]);
    }
    return r;
}

static int copy_path(char ***out, char **p, size_t n)
{
    *out = calloc(n ? n : 1, sizeof **out);
    if (!*out)
        return -1;
    for (size_t i = 0; i < n; i++)
        if (!((*out)[i] = dupn(p[i], strlen(p[i])))) {
            free_strv(*out, i);
            *out = NULL;
            return -1;
        }
    return 0;
}

static int header(struct pstate *s, struct tt_doc *doc, const char *p, const char *end,
                  struct strv *cur)
{
    struct strv path = {0};
    struct tt_table *t;

    if (p + 1 < end && p[1] == '[')
        return fail(s, "arrays of tables ([[...]]) are not part of the subset");
    p = skip_ws(p + 1, end);
    for (;;) {
        char *k = NULL;

        if (key(s, &p, end, &k)) {
            free_strv(path.v, path.n);
            return -1;
        }
        if (strv_push(&path, k)) {
            free_strv(path.v, path.n);
            return oom(s);
        }
        p = skip_ws(p, end);
        if (p < end && *p == '.') {
            p = skip_ws(p + 1, end);
            continue;
        }
        if (p < end && *p == ']')
            break;
        free_strv(path.v, path.n);
        return fail(s, "a '.' or ']' expected in the table header");
    }
    if (line_end(s, p + 1, end, "the table header")) {
        free_strv(path.v, path.n);
        return -1;
    }
    for (size_t i = 0; i < doc->ntables; i++)
        if (same_path(doc->tables[i].path, doc->tables[i].npath, path.v, path.n)) {
            char *t2 = path_text(path.v, path.n);

            fail(s, "the table [%s] is given twice (line %u)", t2 ? t2 : "?", doc->tables[i].line);
            free(t2);
            free_strv(path.v, path.n);
            return -1;
        }
    if (doc->ntables == doc->captables) {
        size_t cap = doc->captables ? 2 * doc->captables : 4;
        struct tt_table *v = realloc(doc->tables, cap * sizeof *v);

        if (!v) {
            free_strv(path.v, path.n);
            return oom(s);
        }
        doc->tables = v;
        doc->captables = cap;
    }
    t = &doc->tables[doc->ntables++];
    t->path = path.v;
    t->npath = path.n;
    t->line = s->line;
    if (!t->path && !(t->path = malloc(sizeof *t->path)))
        return oom(s);
    cur->v = t->path;
    cur->n = t->npath;
    return 0;
}

static int keyvalue(struct pstate *s, struct tt_doc *doc, const char *p, const char *end,
                    const struct strv *cur)
{
    char *k;
    struct tt_value v;
    const char *rest;
    struct tt_entry *e;

    if (key(s, &p, end, &k))
        return -1;
    p = skip_ws(p, end);
    if (p < end && *p == '.') {
        free(k);
        return fail(s, "dotted keys are not part of the subset (one key per line, tables by their header)");
    }
    if (p >= end || *p != '=') {
        free(k);
        return fail(s, "a '=' expected after the key");
    }
    p = skip_ws(p + 1, end);
    if (value(s, p, end, &v, &rest)) {
        free(k);
        return -1;
    }
    if (line_end(s, rest, end, "the value")) {
        free(k);
        tt_value_free(&v);
        return -1;
    }
    for (size_t i = 0; i < doc->n; i++)
        if (!strcmp(doc->e[i].key, k)
            && same_path(doc->e[i].table, doc->e[i].ntable, cur->v, cur->n)) {
            fail(s, "the key %s is given twice in its table (line %u)", k, doc->e[i].line);
            free(k);
            tt_value_free(&v);
            return -1;
        }
    if (doc->n == doc->cap) {
        size_t cap = doc->cap ? 2 * doc->cap : 8;
        struct tt_entry *ne = realloc(doc->e, cap * sizeof *ne);

        if (!ne) {
            free(k);
            tt_value_free(&v);
            return oom(s);
        }
        doc->e = ne;
        doc->cap = cap;
    }
    e = &doc->e[doc->n];
    memset(e, 0, sizeof *e);
    if (copy_path(&e->table, cur->v, cur->n)) {
        free(k);
        tt_value_free(&v);
        return oom(s);
    }
    e->ntable = cur->n;
    e->key = k;
    e->value = v;
    e->line = s->line;
    doc->n++;
    return 0;
}

/* a key that is also a table: a key K in table T and a header T.K... */
static int key_table_clash(struct pstate *s, struct tt_doc *doc)
{
    for (size_t i = 0; i < doc->n; i++) {
        const struct tt_entry *e = &doc->e[i];

        for (size_t j = 0; j < doc->ntables; j++) {
            const struct tt_table *t = &doc->tables[j];

            if (prefix_path(e->table, e->ntable, t->path, t->npath) && !strcmp(t->path[e->ntable], e->key)) {
                s->line = t->line > e->line ? t->line : e->line;
                return fail(s, "%s is a key (line %u) and a table (line %u)", e->key, e->line, t->line);
            }
        }
    }
    return 0;
}

int tt_parse(const char *file, const char *text, size_t len, struct tt_doc *doc,
             char *err, size_t errlen)
{
    struct pstate s = { file, 0, err, errlen, 0 };
    struct strv cur = {0};          /* the current table's path (owned by doc->tables) */
    const char *p = text, *end = text + len;

    err[0] = 0;
    if (!(doc->file = dupn(file, strlen(file))))
        return -2;
    if (memchr(text, 0, len)) {
        s.line = 1;
        for (const char *q = text; *q; q++)
            s.line += *q == '\n';
        fail(&s, "a 0 byte in the file");
        return -1;
    }
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = nl ? nl : end;
        const char *q;
        int r;

        s.line++;
        if (le > p && le[-1] == '\r')
            le--;
        q = skip_ws(p, le);
        if (q >= le || *q == '#')
            r = line_end(&s, q, le, "");
        else if (*q == '[')
            r = header(&s, doc, q, le, &cur);
        else
            r = keyvalue(&s, doc, q, le, &cur);
        if (r)
            return s.oom ? -2 : -1;
        p = nl ? nl + 1 : end;
    }
    if (key_table_clash(&s, doc))
        return -1;
    return 0;
}
