/* toklist - see toklist.h */
#include "toklist.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *err, size_t errlen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

/* ---------------------------------------------------------------- text */

/* a growing string; OOM set once an allocation failed */
struct sb {
    char *p;
    size_t n, cap;
    bool oom;
};

static void sb_add(struct sb *s, const char *t, size_t n)
{
    if (s->oom)
        return;
    if (s->n + n + 1 > s->cap) {
        size_t cap = s->cap ? s->cap : 256;
        char *p;

        while (cap < s->n + n + 1)
            cap *= 2;
        p = realloc(s->p, cap);
        if (!p) {
            s->oom = true;
            return;
        }
        s->p = p;
        s->cap = cap;
    }
    memcpy(s->p + s->n, t, n);
    s->n += n;
    s->p[s->n] = 0;
}

static void sb_str(struct sb *s, const char *t)
{
    sb_add(s, t, strlen(t));
}

static void sb_printf(struct sb *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

static void sb_printf(struct sb *s, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0)
        s->oom = true;
    else
        sb_add(s, buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1);
}

/* the text of S, "" if empty; NULL (and S freed) when out of memory */
static char *sb_take(struct sb *s)
{
    if (!s->oom && !s->p)
        sb_add(s, "", 0);
    if (s->oom) {
        free(s->p);
        return NULL;
    }
    return s->p;
}

/* ---------------------------------------------------------------- the scanner */

struct scan {
    const char *p;
    unsigned line;
    char *err;
    size_t errlen;
};

static bool ident_start(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static bool ident_char(char c)
{
    return ident_start(c) || (c >= '0' && c <= '9');
}

static void step(struct scan *s)
{
    if (*s->p == '\n')
        s->line++;
    s->p++;
}

/* blanks and comments; -1 for a comment without its end */
static int skip_blank(struct scan *s)
{
    for (;;) {
        if (*s->p == ' ' || *s->p == '\t' || *s->p == '\n' || *s->p == '\r' || *s->p == '\f' || *s->p == '\v')
            step(s);
        else if (s->p[0] == '/' && s->p[1] == '*') {
            unsigned line = s->line;

            s->p += 2;
            while (*s->p && !(s->p[0] == '*' && s->p[1] == '/'))
                step(s);
            if (!*s->p)
                return fail(s->err, s->errlen, "line %u: comment not closed", line);
            s->p += 2;
        } else if (s->p[0] == '/' && s->p[1] == '/') {
            while (*s->p && *s->p != '\n')
                s->p++;
        } else
            return 0;
    }
}

/* a string or character literal at S->p (its quote) skipped; -1 if not closed */
static int skip_literal(struct scan *s)
{
    char q = *s->p;
    unsigned line = s->line;

    s->p++;
    while (*s->p && *s->p != q && *s->p != '\n') {
        if (*s->p == '\\' && s->p[1])
            s->p++;
        s->p++;
    }
    if (*s->p != q)
        return fail(s->err, s->errlen, "line %u: %s not closed", line, q == '"' ? "string" : "character literal");
    s->p++;
    return 0;
}

/* one argument of an entry: its text from START to END */
struct arg {
    const char *start, *end;
};

#define MAX_ARGS 16

/* the arguments of an entry, S->p just after its "(": up to the matching
   ")", split at the commas outside brackets and literals */
static int scan_args(struct scan *s, unsigned line, struct arg *a, size_t *n)
{
    int depth = 0;
    const char *start = s->p;

    *n = 0;
    for (;;) {
        char c = *s->p;

        if (!c)
            return fail(s->err, s->errlen, "line %u: entry not closed", line);
        if (c == '"' || c == '\'') {
            if (skip_literal(s))
                return -1;
            continue;
        }
        if (c == '/' && (s->p[1] == '*' || s->p[1] == '/')) {
            if (skip_blank(s))
                return -1;
            continue;
        }
        if (c == '(')
            depth++;
        else if ((c == ',' && depth == 0) || (c == ')' && depth == 0)) {
            if (*n == MAX_ARGS)
                return fail(s->err, s->errlen, "line %u: more than %d arguments", line, MAX_ARGS);
            a[*n].start = start;
            a[*n].end = s->p;
            (*n)++;
            step(s);
            if (c == ')')
                return 0;
            start = s->p;
            continue;
        } else if (c == ')')
            depth--;
        step(s);
    }
}

/* ---------------------------------------------------------------- the values */

/* a value: blanks and comments of an argument's text skipped */
struct val {
    const char *p, *end;
};

static void val_blank(struct val *v)
{
    while (v->p < v->end) {
        if (*v->p == ' ' || *v->p == '\t' || *v->p == '\n' || *v->p == '\r')
            v->p++;
        else if (v->end - v->p >= 2 && v->p[0] == '/' && v->p[1] == '*') {
            const char *e = strstr(v->p + 2, "*/");

            v->p = e && e + 2 <= v->end ? e + 2 : v->end;
        } else if (v->end - v->p >= 2 && v->p[0] == '/' && v->p[1] == '/') {
            while (v->p < v->end && *v->p != '\n')
                v->p++;
        } else
            return;
    }
}

/* one or more adjacent string literals at V, decoded into OUT; -1 with a
   message (no literal, an unknown escape) */
static int val_strings(struct val *v, struct sb *out, unsigned line, const char *what, char *err, size_t errlen)
{
    bool any = false;

    for (;;) {
        val_blank(v);
        if (v->p >= v->end || *v->p != '"')
            break;
        any = true;
        v->p++;
        while (v->p < v->end && *v->p != '"') {
            char c = *v->p++;

            if (c == '\\') {
                char e = v->p < v->end ? *v->p++ : 0;

                switch (e) {
                case '"': case '\\': case '\'':
                    c = e;
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 't':
                    c = '\t';
                    break;
                default:
                    return fail(err, errlen, "line %u: %s: escape \\%c not supported", line, what, e ? e : '?');
                }
            }
            sb_add(out, &c, 1);
        }
        v->p++;
    }
    if (!any)
        return fail(err, errlen, "line %u: %s: no string literal or M(...)", line, what);
    return 0;
}

/* the value of a name or help argument: a string literal, or M(blitz2's,
   AmiBlitz3's) of which blitz2's is taken; *OUT allocated */
static int value(const struct arg *a, unsigned line, const char *what, char **out, char *err, size_t errlen)
{
    struct val v = { a->start, a->end };
    struct sb s = { 0 };
    int rc;

    val_blank(&v);
    if (v.p < v.end && *v.p == 'M' && (v.p + 1 == v.end || !ident_char(v.p[1]))) {
        struct sb other = { 0 };

        v.p++;
        val_blank(&v);
        if (v.p >= v.end || *v.p != '(') {
            rc = fail(err, errlen, "line %u: %s: M without its arguments", line, what);
            goto out;
        }
        v.p++;
        if ((rc = val_strings(&v, &s, line, what, err, errlen)))
            goto out;
        val_blank(&v);
        if (v.p >= v.end || *v.p != ',') {
            rc = fail(err, errlen, "line %u: %s: M needs two strings", line, what);
            goto out;
        }
        v.p++;
        rc = val_strings(&v, &other, line, what, err, errlen);
        free(other.p);
        if (rc)
            goto out;
        val_blank(&v);
        if (v.p >= v.end || *v.p != ')') {
            rc = fail(err, errlen, "line %u: %s: M needs two strings", line, what);
            goto out;
        }
        v.p++;
    } else if ((rc = val_strings(&v, &s, line, what, err, errlen)))
        goto out;
    val_blank(&v);
    if (v.p < v.end) {
        rc = fail(err, errlen, "line %u: %s: more than a string", line, what);
        goto out;
    }
    if (!(*out = sb_take(&s)))
        return fail(err, errlen, "out of memory");
    return 0;
out:
    free(s.p);
    return rc;
}

/* ---------------------------------------------------------------- the list */

static const struct {
    const char *macro;
    enum tl_kind kind;
    size_t args;                        /* the number of arguments (TA: at least) */
    size_t name;                        /* the index of the name, the help behind it */
} macros[] = {
    { "TK", TL_TK, 6, 4 },
    { "TA", TL_TA, 6, 4 },
    { "TKLAST", TL_TKLAST, 5, 3 },
    { "TK3", TL_TK3, 6, 4 },
    { "TK3LAST", TL_TK3LAST, 5, 3 },
};

void tl_free(struct tl_list *l)
{
    for (size_t i = 0; i < l->n; i++) {
        free(l->v[i].name);
        free(l->v[i].help);
    }
    free(l->v);
    l->v = NULL;
    l->n = l->cap = 0;
}

static int add(struct tl_list *l, const struct tl_entry *e)
{
    if (l->n == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 256;
        struct tl_entry *v = realloc(l->v, cap * sizeof *v);

        if (!v)
            return -1;
        l->v = v;
        l->cap = cap;
    }
    l->v[l->n++] = *e;
    return 0;
}

/* one entry, S->p at its macro's name */
static int entry(struct scan *s, struct tl_list *l, int *phase)
{
    const char *id = s->p;
    unsigned line = s->line;
    struct arg a[MAX_ARGS];
    size_t n, m, len;
    struct tl_entry e = { 0 };

    while (ident_char(*s->p))
        s->p++;
    len = (size_t)(s->p - id);
    for (m = 0; m < sizeof macros / sizeof *macros; m++)
        if (strlen(macros[m].macro) == len && !memcmp(macros[m].macro, id, len))
            break;
    if (m == sizeof macros / sizeof *macros)
        return fail(s->err, s->errlen, "line %u: not an entry: %.*s", line, (int)(len < 40 ? len : 40), id);
    if (skip_blank(s))
        return -1;
    if (*s->p != '(')
        return fail(s->err, s->errlen, "line %u: %s without its arguments", line, macros[m].macro);
    s->p++;
    if (scan_args(s, line, a, &n))
        return -1;
    if (macros[m].kind == TL_TA ? n < macros[m].args : n != macros[m].args)
        return fail(s->err, s->errlen, "line %u: %s with %zu arguments, %s%zu wanted", line, macros[m].macro, n,
                    macros[m].kind == TL_TA ? "at least " : "", macros[m].args);

    /* the order: TK and TA, one TKLAST, TK3, at most one TK3LAST */
    switch (macros[m].kind) {
    case TL_TK:
    case TL_TA:
    case TL_TKLAST:
        if (*phase)
            return fail(s->err, s->errlen, "line %u: %s after TKLAST", line, macros[m].macro);
        break;
    case TL_TK3:
    case TL_TK3LAST:
        if (*phase == 0)
            return fail(s->err, s->errlen, "line %u: %s before TKLAST", line, macros[m].macro);
        if (*phase == 2)
            return fail(s->err, s->errlen, "line %u: %s after TK3LAST", line, macros[m].macro);
        break;
    }
    if (macros[m].kind == TL_TKLAST)
        *phase = 1;
    else if (macros[m].kind == TL_TK3LAST)
        *phase = 2;

    e.kind = macros[m].kind;
    e.line = line;
    if (value(&a[macros[m].name], line, "name", &e.name, s->err, s->errlen))
        return -1;
    if (!*e.name) {
        free(e.name);
        return fail(s->err, s->errlen, "line %u: empty name", line);
    }
    if (e.kind == TL_TA)
        e.help = strdup("");
    else if (value(&a[macros[m].name + 1], line, "help", &e.help, s->err, s->errlen)) {
        free(e.name);
        return -1;
    }
    if (!e.help || add(l, &e)) {
        free(e.name);
        free(e.help);
        return fail(s->err, s->errlen, "out of memory");
    }
    return 0;
}

int tl_parse(struct tl_list *l, const char *text, char *err, size_t errlen)
{
    struct scan s = { text, 1, err, errlen };
    int phase = 0;

    l->v = NULL;
    l->n = l->cap = 0;
    for (;;) {
        if (skip_blank(&s))
            break;
        if (!*s.p) {
            if (phase == 0) {
                fail(err, errlen, "line %u: no TKLAST", s.line);
                break;
            }
            return 0;
        }
        if (!ident_start(*s.p)) {
            fail(err, errlen, "line %u: not an entry: '%c'", s.line, *s.p);
            break;
        }
        if (entry(&s, l, &phase))
            break;
    }
    tl_free(l);
    return -1;
}

size_t tl_count(const struct tl_list *l, enum tl_kind kind)
{
    size_t n = 0;

    for (size_t i = 0; i < l->n; i++)
        n += l->v[i].kind == kind;
    return n;
}

/* ---------------------------------------------------------------- the JSON */

/* a JSON string: printable ASCII as it is (" and \ escaped), every other
   byte as \u00XX (as blitzlib cmdref writes them) */
static void json_str(struct sb *s, const char *t)
{
    sb_add(s, "\"", 1);
    for (const uint8_t *p = (const uint8_t *)t; *p; p++) {
        if (*p == '"' || *p == '\\') {
            char e[2] = { '\\', (char)*p };

            sb_add(s, e, 2);
        } else if (*p >= 0x20 && *p < 0x7f)
            sb_add(s, (const char *)p, 1);
        else
            sb_printf(s, "\\u%04x", *p);
    }
    sb_add(s, "\"", 1);
}

char *tl_json(const struct tl_list *l, const char *about, const char *source)
{
    struct sb s = { 0 };
    bool first = true;

    sb_str(&s, "{\n  \"compiler\": \"blitz2\",\n  \"generated\": ");
    json_str(&s, about);
    sb_str(&s, ",\n  \"source\": ");
    json_str(&s, source);
    sb_str(&s, ",\n  \"keywords\": [");
    for (size_t i = 0; i < l->n; i++) {
        if (l->v[i].kind != TL_TK && l->v[i].kind != TL_TKLAST)
            continue;
        sb_str(&s, first ? "\n    {\"name\": " : ",\n    {\"name\": ");
        json_str(&s, l->v[i].name);
        sb_str(&s, ", \"help\": ");
        json_str(&s, l->v[i].help);
        sb_str(&s, "}");
        first = false;
    }
    sb_str(&s, first ? "]\n}\n" : "\n  ]\n}\n");
    return sb_take(&s);
}

/* ---------------------------------------------------------------- the comparison */

/* the line of TEXT at *POS (up to END): its start and length without the
   newline; *POS behind it; *NL whether it ends with one; false at the end */
static bool next_line(const char *text, size_t end, size_t *pos, const char **line, size_t *len, bool *nl)
{
    const char *p, *e;

    if (*pos >= end)
        return false;
    p = text + *pos;
    e = memchr(p, '\n', end - *pos);
    *line = p;
    *len = e ? (size_t)(e - p) : end - *pos;
    *nl = e != NULL;
    *pos += *len + (e != NULL);
    return true;
}

int tl_compare(const char *want, size_t wantlen, const char *got, const char *file, char **report)
{
    struct sb s = { 0 };
    size_t gotlen = strlen(got), wp = 0, gp = 0, diffs = 0;
    unsigned line = 0;

    *report = NULL;
    for (;;) {
        const char *wl = NULL, *gl = NULL;
        size_t wn = 0, gn = 0;
        bool wnl = false, gnl = false;
        bool hw = next_line(want, wantlen, &wp, &wl, &wn, &wnl);
        bool hg = next_line(got, gotlen, &gp, &gl, &gn, &gnl);

        if (!hw && !hg)
            break;
        line++;
        if (hw && hg && wn == gn && !memcmp(wl, gl, wn) && wnl == gnl)
            continue;
        if (++diffs > 10)
            continue;
        if (hw && hg)
            sb_printf(&s, "%s:%u: differs from the generated file%s: %.*s\n  generated: %.*s\n", file, line,
                      wn == gn && !memcmp(wl, gl, wn) ? " (the newline at its end)" : "", (int)wn, wl, (int)gn,
                      gl);
        else if (hg)
            sb_printf(&s, "%s:%u: missing: %.*s\n", file, line, (int)gn, gl);
        else
            sb_printf(&s, "%s:%u: not in the generated file: %.*s\n", file, line, (int)wn, wl);
    }
    if (!diffs)
        return 0;
    sb_printf(&s, "%s: lines that differ from the generated file: %zu\n", file, diffs);
    if (!(*report = sb_take(&s)))
        return -1;
    return 1;
}
