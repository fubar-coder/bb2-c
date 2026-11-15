#include "perfbudget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* a copy of s[0..n) */
static char *dup_n(const char *s, size_t n)
{
    char *d = malloc(n + 1);

    if (d) {
        memcpy(d, s, n);
        d[n] = 0;
    }
    return d;
}

/* the whitespace-separated fields of the line [s, e): up to max, their
   starts and lengths; the number of fields (max + 1 if there are more) */
static size_t split(const char *s, const char *e, const char **f, size_t *len, size_t max)
{
    size_t n = 0;

    while (s < e) {
        const char *b;

        while (s < e && (*s == ' ' || *s == '\t' || *s == '\r'))
            s++;
        if (s == e)
            break;
        b = s;
        while (s < e && *s != ' ' && *s != '\t' && *s != '\r')
            s++;
        if (n == max)
            return max + 1;
        f[n] = b;
        len[n] = (size_t)(s - b);
        n++;
    }
    return n;
}

/* the decimal number of [s, s+n) without sign: 0 = read (in *v) */
static int number(const char *s, size_t n, long *v)
{
    long x = 0;
    size_t i;

    if (!n || n > 9)
        return -1;
    for (i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        x = x * 10 + (s[i] - '0');
    }
    *v = x;
    return 0;
}

struct line {
    char *test;
    long t, n, per;                     /* per 0: a frame test */
};

static void free_lines(struct line *l, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        free(l[i].test);
    free(l);
}

int perf_read_out(struct perf_run *r, const char *program, const char *text, char *err,
                  size_t errlen)
{
    struct line *l = NULL;
    struct perf_meas *m;
    size_t nl = 0, i, lineno = 0;
    const char *s = text;
    double empty = 0;
    int rc_seen = 0, have_empty = 0;

#define FAIL(...) do { snprintf(err, errlen, __VA_ARGS__); free_lines(l, nl); return -1; } while (0)
    while (*s) {
        const char *e = strchr(s, '\n'), *f[5];
        size_t len[5], nf;

        if (!e)
            e = s + strlen(s);
        lineno++;
        nf = split(s, e, f, len, 4);
        if (nf == 0) {
            /* an empty line */
        } else if (rc_seen) {
            FAIL("%s: line %zu: after the exit status", program, lineno);
        } else if (nf == 1 && len[0] > 3 && !strncmp(f[0], "rc=", 3)) {
            long rc;

            if (number(f[0] + 3, len[0] - 3, &rc))
                FAIL("%s: line %zu: a malformed exit status \"%.*s\"", program, lineno,
                     (int)len[0], f[0]);
            if (rc)
                FAIL("%s: the program ended with rc=%ld", program, rc);
            rc_seen = 1;
        } else {
            struct line x = { 0 };
            struct line *nlp;

            if (nf != 4)
                FAIL("%s: line %zu: not \"TEST T N PER\": \"%.*s\"", program, lineno,
                     (int)(e - s), s);
            if (number(f[1], len[1], &x.t) || x.t > 65535)
                FAIL("%s: line %zu: the time \"%.*s\" is not 0 to 65535", program, lineno,
                     (int)len[1], f[1]);
            if (number(f[2], len[2], &x.n) || x.n <= 0)
                FAIL("%s: line %zu: the iterations \"%.*s\" are not a positive number",
                     program, lineno, (int)len[2], f[2]);
            if (len[3] == 5 && !strncmp(f[3], "frame", 5))
                x.per = 0;
            else if (number(f[3], len[3], &x.per) || x.per <= 0)
                FAIL("%s: line %zu: the calls per iteration \"%.*s\" are not a positive "
                     "number or frame", program, lineno, (int)len[3], f[3]);
            for (i = 0; i < nl; i++)
                if (strlen(l[i].test) == len[0] && !strncmp(l[i].test, f[0], len[0]))
                    FAIL("%s: line %zu: the test %.*s twice", program, lineno, (int)len[0],
                         f[0]);
            if (!(x.test = dup_n(f[0], len[0]))
                || !(nlp = realloc(l, (nl + 1) * sizeof *l))) {
                free(x.test);
                FAIL("out of memory");
            }
            l = nlp;
            l[nl++] = x;
            if (!strcmp(x.test, "empty")) {
                if (x.per != 1)
                    FAIL("%s: line %zu: the empty loop has one call per iteration", program,
                         lineno);
                empty = x.t * 1000.0 / x.n;
                have_empty = 1;
            }
        }
        s = *e ? e + 1 : e;
    }
    if (!rc_seen)
        FAIL("%s: no exit status (rc=N): the program did not end", program);
    if (!have_empty)
        FAIL("%s: no empty loop (the test empty)", program);
    if (!(m = realloc(r->m, (r->n + nl) * sizeof *m)))
        FAIL("out of memory");
    r->m = m;
    for (i = 0; i < nl; i++) {
        double it = l[i].t * 1000.0 / l[i].n;
        struct perf_meas *p = &r->m[r->n];

        memset(p, 0, sizeof *p);
        if (!(p->name = malloc(strlen(program) + strlen(l[i].test) + 2))) {
            perf_run_free(r);
            FAIL("out of memory");
        }
        sprintf(p->name, "%s.%s", program, l[i].test);
        p->empty = !strcmp(l[i].test, "empty");
        p->frame = !l[i].per;
        if (p->empty)
            p->value = it;
        else if (p->frame)
            p->value = it > 0 ? PERF_FRAME_CYCLES * (1.0 - empty / it) : 0;
        else
            p->value = (it - empty) / l[i].per;
        r->n++;
    }
#undef FAIL
    free_lines(l, nl);
    return 0;
}

void perf_run_free(struct perf_run *r)
{
    size_t i;

    for (i = 0; i < r->n; i++)
        free(r->m[i].name);
    free(r->m);
    r->m = NULL;
    r->n = 0;
}

void perf_table(const struct perf_run *r, FILE *f)
{
    size_t i;

    fprintf(f, "%-24s %12s\n", "command", "cycles");
    for (i = 0; i < r->n; i++) {
        const struct perf_meas *m = &r->m[i];

        fprintf(f, "%-24s %12.1f%s\n", m->name, m->value,
                m->empty ? "  (empty loop, per iteration)" : m->frame ? "  (per frame)" : "");
    }
}
