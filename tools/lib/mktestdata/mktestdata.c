/*
 * mktestdata.c - binary test data from a text description (mktestdata.h).
 * Two passes over the lines: the first places the labels, the second
 * writes the bytes (the sizes do not depend on the values).
 */
#include "mktestdata.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXDEPTH 16

struct label {
    char *name;
    size_t offset;
};

struct open_iff {
    size_t start;               /* offset of the id */
    int has_len;
    uint32_t len;               /* declared length */
};

struct state {
    int pass;                   /* 1: labels, 2: bytes */
    unsigned line;
    uint8_t *p;
    size_t n, cap;
    struct label *labels;
    size_t nlabels;
    struct open_iff open[MAXDEPTH];
    int depth;
    int later;                  /* an expression referred to a label not placed yet */
    char *err;
    size_t errlen;
};

static int fail(struct state *s, const char *fmt, ...)
{
    va_list ap;
    int k = snprintf(s->err, s->errlen, "line %u: ", s->line);

    va_start(ap, fmt);
    if (k >= 0 && (size_t)k < s->errlen)
        vsnprintf(s->err + k, s->errlen - (size_t)k, fmt, ap);
    va_end(ap);
    return -1;
}

static int put(struct state *s, uint8_t v)
{
    if (s->pass == 2) {
        if (s->n == s->cap) {
            size_t cap = s->cap ? 2 * s->cap : 4096;
            uint8_t *q = realloc(s->p, cap);

            if (!q)
                return fail(s, "out of memory");
            s->p = q;
            s->cap = cap;
        }
        s->p[s->n] = v;
    }
    s->n++;
    return 0;
}

static int put_be(struct state *s, uint32_t v, int size)
{
    int i;

    for (i = size - 1; i >= 0; i--)
        if (put(s, (uint8_t)(v >> (8 * i))))
            return -1;
    return 0;
}

static void patch32(struct state *s, size_t at, uint32_t v)
{
    if (s->pass == 2) {
        s->p[at] = (uint8_t)(v >> 24);
        s->p[at + 1] = (uint8_t)(v >> 16);
        s->p[at + 2] = (uint8_t)(v >> 8);
        s->p[at + 3] = (uint8_t)v;
    }
}

static const char *skip_space(const char *t)
{
    while (*t == ' ' || *t == '\t')
        t++;
    return t;
}

static int is_name_char(int c)
{
    return isalnum(c) || c == '_' || c == '.';
}

static struct label *find_label(struct state *s, const char *name, size_t len)
{
    size_t i;

    for (i = 0; i < s->nlabels; i++)
        if (strlen(s->labels[i].name) == len && !memcmp(s->labels[i].name, name, len))
            return &s->labels[i];
    return NULL;
}

/* one term at *t: number, 'ABCD' or @NAME */
static int term(struct state *s, const char **t, int64_t *v)
{
    const char *q = *t;

    if (*q == '@') {
        const char *name = ++q;
        struct label *l;

        while (is_name_char((unsigned char)*q))
            q++;
        if (q == name)
            return fail(s, "label name expected after @");
        l = find_label(s, name, (size_t)(q - name));
        if (l)
            *v = (int64_t)l->offset;
        else if (s->pass == 1) {
            *v = 0;
            s->later = 1;
        } else
            return fail(s, "unknown label %.*s", (int)(q - name), name);
    } else if (*q == '\'') {
        int k = 0;

        *v = 0;
        for (q++; *q && *q != '\''; q++, k++)
            *v = (*v << 8) | (uint8_t)*q;
        if (*q != '\'' || k < 1 || k > 4)
            return fail(s, "bad character constant (1-4 characters between quotes)");
        q++;
    } else {
        int base = 10;
        uint64_t x = 0;
        const char *d;

        if (*q == '$') {
            base = 16;
            q++;
        } else if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) {
            base = 16;
            q += 2;
        }
        d = q;
        for (;; q++) {
            int c = (unsigned char)*q, digit;

            if (isdigit(c))
                digit = c - '0';
            else if (base == 16 && isxdigit(c))
                digit = tolower(c) - 'a' + 10;
            else
                break;
            x = x * (unsigned)base + (unsigned)digit;
            if (x > 0xffffffffu)
                return fail(s, "number too large");
        }
        if (q == d || is_name_char((unsigned char)*q))
            return fail(s, "bad value at \"%.20s\"", *t);
        *v = (int64_t)x;
    }
    *t = q;
    return 0;
}

/* an expression: terms joined by + and -, ends at a blank or the end */
static int expr(struct state *s, const char **t, int64_t *v)
{
    int64_t x;
    int neg = 0;

    *v = 0;
    if (**t == '-') {
        neg = 1;
        (*t)++;
    }
    for (;;) {
        if (term(s, t, &x))
            return -1;
        *v += neg ? -x : x;
        if (**t == '+')
            neg = 0;
        else if (**t == '-')
            neg = 1;
        else
            break;
        (*t)++;
    }
    if (**t && **t != ' ' && **t != '\t')
        return fail(s, "bad value at \"%.20s\"", *t);
    return 0;
}

static int value(struct state *s, const char **t, int size, uint32_t *out)
{
    int64_t v, lo = -((int64_t)1 << (8 * size - 1)), hi = ((int64_t)1 << (8 * size)) - 1;

    if (expr(s, t, &v))
        return -1;
    if (v < lo || v > hi)
        return fail(s, "value %lld does not fit in %d byte(s)", (long long)v, size);
    *out = (uint32_t)v;
    *t = skip_space(*t);
    return 0;
}

/* a count (fill, align): no later labels, 0..2^28 */
static int count(struct state *s, const char **t, size_t *out)
{
    int64_t v;

    s->later = 0;
    if (expr(s, t, &v))
        return -1;
    if (s->later)
        return fail(s, "a count must not refer to a later label");
    if (v < 0 || v > (1 << 28))
        return fail(s, "bad count %lld", (long long)v);
    *out = (size_t)v;
    *t = skip_space(*t);
    return 0;
}

static int iff_id(struct state *s, const char **t, uint32_t *id)
{
    const char *q = *t;
    int i, quoted = *q == '\'';

    *id = 0;
    q += quoted;
    for (i = 0; i < 4; i++, q++) {
        if (*q < 0x20 || *q > 0x7e || (*q == ' ' && !quoted))
            return fail(s, "an IFF id has 4 characters");
        *id = (*id << 8) | (uint8_t)*q;
    }
    if (quoted && *q++ != '\'')
        return fail(s, "an IFF id has 4 characters");
    if (*q && *q != ' ' && *q != '\t')
        return fail(s, "an IFF id has 4 characters");
    *t = skip_space(q);
    return 0;
}

static int open_iff(struct state *s, const char **t, int form)
{
    struct open_iff *o;
    uint32_t id;

    if (s->depth == MAXDEPTH)
        return fail(s, "forms and chunks nested too deep");
    o = &s->open[s->depth];
    o->start = s->n;
    if (form) {
        if (put_be(s, 0x464f524d, 4))   /* FORM */
            return -1;
    } else {
        if (iff_id(s, t, &id) || put_be(s, id, 4))
            return -1;
    }
    if (put_be(s, 0, 4))
        return -1;
    if (form && (iff_id(s, t, &id) || put_be(s, id, 4)))
        return -1;
    o->has_len = **t != 0;
    if (o->has_len && value(s, t, 4, &o->len))
        return -1;
    s->depth++;
    return 0;
}

static int close_iff(struct state *s)
{
    struct open_iff *o;
    size_t len;

    if (!s->depth)
        return fail(s, "end without form or chunk");
    o = &s->open[--s->depth];
    len = s->n - o->start - 8;
    if (len > 0xffffffffu)
        return fail(s, "chunk too long");
    patch32(s, o->start + 4, o->has_len ? o->len : (uint32_t)len);
    if (len & 1)
        return put(s, 0);
    return 0;
}

static int string(struct state *s, const char **t)
{
    const char *q = *t;

    if (*q++ != '"')
        return fail(s, "s needs a text in double quotes");
    for (; *q && *q != '"'; q++) {
        int c = (unsigned char)*q;

        if (c == '\\') {
            q++;
            c = *q == 'n' ? '\n' : *q == '0' ? 0 : *q == '"' || *q == '\\' ? *q : -1;
            if (c < 0)
                return fail(s, "unknown escape in the text");
        }
        if (put(s, (uint8_t)c))
            return -1;
    }
    if (*q != '"')
        return fail(s, "unterminated text");
    *t = skip_space(q + 1);
    return 0;
}

static int add_label(struct state *s, const char *name, size_t len)
{
    struct label *l = find_label(s, name, len), *a;

    if (s->pass == 2)
        return 0;
    if (l)
        return fail(s, "label %.*s defined twice", (int)len, name);
    a = realloc(s->labels, (s->nlabels + 1) * sizeof *a);
    if (!a)
        return fail(s, "out of memory");
    s->labels = a;
    a[s->nlabels].name = malloc(len + 1);
    if (!a[s->nlabels].name)
        return fail(s, "out of memory");
    memcpy(a[s->nlabels].name, name, len);
    a[s->nlabels].name[len] = 0;
    a[s->nlabels].offset = s->n;
    s->nlabels++;
    return 0;
}

/* one line without its comment, blanks stripped at both ends */
static int line(struct state *s, const char *t)
{
    const char *w = t;
    size_t wl, i, n;
    uint32_t v;
    int size;

    while (is_name_char((unsigned char)*t))
        t++;
    wl = (size_t)(t - w);
    if (*t == ':' && wl) {
        if (add_label(s, w, wl))
            return -1;
        t = skip_space(t + 1);
        return *t ? fail(s, "text behind the label") : 0;
    }
    if (*t && *t != ' ' && *t != '\t')
        return fail(s, "unknown directive \"%.20s\"", w);
    t = skip_space(t);
#define IS(word) (wl == strlen(word) && !memcmp(w, word, wl))
    if (IS("b") || IS("w") || IS("l")) {
        size = *w == 'b' ? 1 : *w == 'w' ? 2 : 4;
        if (!*t)
            return fail(s, "%c needs a value", *w);
        while (*t)
            if (value(s, &t, size, &v) || put_be(s, v, size))
                return -1;
        return 0;
    }
    if (IS("s"))
        return string(s, &t) ? -1 : *t ? fail(s, "text behind the string") : 0;
    if (IS("fill")) {
        if (count(s, &t, &n) || value(s, &t, 1, &v))
            return -1;
        if (*t)
            return fail(s, "fill N V");
        for (i = 0; i < n; i++)
            if (put(s, (uint8_t)v))
                return -1;
        return 0;
    }
    if (IS("align")) {
        if (count(s, &t, &n))
            return -1;
        if (*t || !n)
            return fail(s, "align N (N > 0)");
        while (s->n % n)
            if (put(s, 0))
                return -1;
        return 0;
    }
    if (IS("form") || IS("chunk"))
        return open_iff(s, &t, *w == 'f') ? -1 : *t ? fail(s, "text behind the length") : 0;
    if (IS("end"))
        return *t ? fail(s, "text behind end") : close_iff(s);
#undef IS
    return fail(s, "unknown directive \"%.*s\"", (int)wl, w);
}

static int run(struct state *s, const char *spec)
{
    const char *p = spec;
    char buf[1024];

    s->n = 0;
    s->depth = 0;
    s->line = 0;
    while (*p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        size_t i;
        int quoted = 0;

        s->line++;
        if (len >= sizeof buf)
            return fail(s, "line too long");
        memcpy(buf, p, len);
        buf[len] = 0;
        p += len + (e != NULL);
        /* the comment: ';' or '#' outside a text or character constant */
        for (i = 0; buf[i]; i++) {
            if (buf[i] == '\\' && quoted == '"' && buf[i + 1])
                i++;
            else if ((buf[i] == '"' || buf[i] == '\'') && (!quoted || quoted == buf[i]))
                quoted = quoted ? 0 : buf[i];
            else if (!quoted && (buf[i] == ';' || buf[i] == '#')) {
                buf[i] = 0;
                break;
            }
        }
        while (len && (buf[len - 1] == ' ' || buf[len - 1] == '\t' || buf[len - 1] == '\r'
                       || !buf[len - 1]))
            buf[--len] = 0;
        if (*skip_space(buf) && line(s, skip_space(buf)))
            return -1;
    }
    if (s->depth)
        return fail(s, "%d form(s)/chunk(s) without end", s->depth);
    return 0;
}

int mktestdata_build(const char *spec, uint8_t **out, size_t *len, char *err, size_t errlen)
{
    struct state s = { 0 };
    size_t i;
    int r;

    s.err = err;
    s.errlen = errlen;
    s.pass = 1;
    r = run(&s, spec);
    if (!r) {
        s.pass = 2;
        r = run(&s, spec);
    }
    for (i = 0; i < s.nlabels; i++)
        free(s.labels[i].name);
    free(s.labels);
    if (r) {
        free(s.p);
        return -1;
    }
    if (!s.p && !(s.p = malloc(1))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    *out = s.p;
    *len = s.n;
    return 0;
}
