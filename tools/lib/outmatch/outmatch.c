/*
 * outmatch.c - an output against an expect file (outmatch.h).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "outmatch.h"

struct line {
    const char *p;
    size_t n;
};

struct lines {
    struct line *l;
    size_t n;
};

/* the lines of a text (LF, a CR before it dropped; no empty last line) */
static int split(const char *s, struct lines *out)
{
    size_t cap = 64;

    out->n = 0;
    if (!(out->l = malloc(cap * sizeof *out->l)))
        return -1;
    while (*s) {
        const char *e = strchr(s, '\n');
        size_t n = e ? (size_t)(e - s) : strlen(s);

        if (out->n == cap) {
            struct line *t = realloc(out->l, (cap *= 2) * sizeof *out->l);

            if (!t)
                return -1;
            out->l = t;
        }
        out->l[out->n].p = s;
        out->l[out->n].n = n && s[n - 1] == '\r' ? n - 1 : n;
        out->n++;
        s += n + (e != NULL);
    }
    return 0;
}

/* "[NAME]" */
static int is_section(const struct line *l)
{
    size_t i;

    if (l->n < 3 || l->p[0] != '[' || l->p[l->n - 1] != ']')
        return 0;
    for (i = 1; i < l->n - 1; i++) {
        char c = l->p[i];

        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9')
            && c != '-' && c != '_')
            return 0;
    }
    return 1;
}

/* the lines of section NAME: [*from, *to); 0 if there is none */
static int find_section(const struct lines *t, const char *name, size_t *from, size_t *to)
{
    size_t i, len = strlen(name);

    for (i = 0; i < t->n; i++)
        if (is_section(&t->l[i]) && t->l[i].n == len + 2 && !memcmp(t->l[i].p + 1, name, len)) {
            *from = ++i;
            while (i < t->n && !is_section(&t->l[i]))
                i++;
            *to = i;
            return 1;
        }
    return 0;
}

static int digit(char c)
{
    return c >= '0' && c <= '9';
}

int outmatch_line(const char *pat, const char *line, size_t len)
{
    size_t i, k;

    for (;;) {
        switch (*pat) {
        case 0:
            return len == 0;
        case '*':
            for (i = 0; i <= len; i++)
                if (outmatch_line(pat + 1, line + i, len - i))
                    return 1;
            return 0;
        case '#':
            i = len && *line == '-';
            for (k = i; k < len && digit(line[k]); k++)
                ;
            for (; k > i; k--)
                if (outmatch_line(pat + 1, line + k, len - k))
                    return 1;
            return 0;
        case '\\':
            if (!pat[1])
                return 0;       /* a pattern ending in '\' matches nothing */
            pat++;
            /* fall through */
        default:
            if (!len || *line != *pat)
                return 0;
            pat++;
            line++;
            len--;
        }
    }
}

/* a line as "..." into msg (cut, control characters as '?') */
static void quote(char *msg, size_t cap, const char *what, size_t at, const struct line *l)
{
    char q[72];
    size_t i, n = l ? (l->n < sizeof q - 4 ? l->n : sizeof q - 4) : 0;

    if (!l) {
        snprintf(msg, cap, "%s line %zu: (none)", what, at);
        return;
    }
    for (i = 0; i < n; i++)
        q[i] = (unsigned char)l->p[i] < ' ' ? '?' : l->p[i];
    if (n < l->n)
        memcpy(q + n, "...", 4);
    else
        q[n] = 0;
    snprintf(msg, cap, "%s line %zu: \"%s\"", what, at, q);
}

/* an expect line against an output line */
static int line_ok(const struct line *e, const struct line *o)
{
    char pat[1024];
    size_t n = e->n - 1;

    if (e->n && (e->p[0] == '~' || e->p[0] == '+')) {
        if (n >= sizeof pat)
            return 0;
        memcpy(pat, e->p + 1, n);
        pat[n] = 0;
        return outmatch_line(pat, o->p, o->n);
    }
    if (e->n && e->p[0] == '\\')
        return o->n == n && !memcmp(o->p, e->p + 1, n);
    return o->n == e->n && !memcmp(o->p, e->p, e->n);
}

int outmatch(const char *expect, const char *output, const char *section, char *msg, size_t cap)
{
    struct lines e = { 0 }, o = { 0 };
    size_t ei, ee, oi, oe, ef, of;
    int r = 1;

    msg[0] = 0;
    if (split(expect, &e) || split(output, &o)) {
        snprintf(msg, cap, "out of memory");
        r = -1;
        goto out;
    }
    ei = oi = 0;
    ee = e.n;
    oe = o.n;
    if (section) {
        if (!find_section(&e, section, &ei, &ee)) {
            snprintf(msg, cap, "no section [%s] in the expect file", section);
            r = -1;
            goto out;
        }
        if (!find_section(&o, section, &oi, &oe)) {
            snprintf(msg, cap, "no section [%s] in the output", section);
            goto out;
        }
    }
    for (ef = ei; ef < ee; ef++)
        if (e.l[ef].n == 1 && (e.l[ef].p[0] == '~' || e.l[ef].p[0] == '+')) {
            snprintf(msg, cap, "expect line %zu: '%c' without a pattern", ef + 1, e.l[ef].p[0]);
            r = -1;
            goto out;
        }
    while (ei < ee) {
        const struct line *el = &e.l[ei];

        if (oi == oe || !line_ok(el, &o.l[oi]))
            break;
        oi++;
        if (el->p[0] == '+')
            while (oi < oe && line_ok(el, &o.l[oi]))
                oi++;
        ei++;
    }
    if (ei == ee && oi == oe) {
        r = 0;
        goto out;
    }
    ef = ei + 1;
    of = oi + 1;
    {
        char a[200], b[200];

        quote(a, sizeof a, "expect", ef, ei < ee ? &e.l[ei] : NULL);
        quote(b, sizeof b, "output", of, oi < oe ? &o.l[oi] : NULL);
        snprintf(msg, cap, "%s, %s", a, b);
    }
out:
    free(e.l);
    free(o.l);
    return r;
}
