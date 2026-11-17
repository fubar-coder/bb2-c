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
        size_t pl = strlen(program), tl = strlen(l[i].test);

        memset(p, 0, sizeof *p);
        if (!(p->name = malloc(pl + 1 + tl + 1))) {
            perf_run_free(r);
            FAIL("out of memory");
        }
        /* PROGRAM.TEST */
        memcpy(p->name, program, pl);
        p->name[pl] = '.';
        memcpy(p->name + pl + 1, l[i].test, tl + 1);
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

/* ------------------------------------------------------------ budgets */

void perf_budgets_free(struct perf_budgets *b)
{
    size_t i;

    for (i = 0; i < b->n; i++) {
        free(b->b[i].name);
        free(b->b[i].text);
    }
    free(b->b);
    b->b = NULL;
    b->n = 0;
}

/* the decimal number [s, s+n) without sign, with at most one decimal point
   followed by a digit: 0 = read (in *v) */
static int decimal(const char *s, size_t n, double *v)
{
    double x = 0, scale = 0;
    size_t i, digits = 0;

    if (!n || n > 15)
        return -1;
    for (i = 0; i < n; i++) {
        if (s[i] == '.' && !scale && digits) {
            scale = 1;
        } else if (s[i] >= '0' && s[i] <= '9') {
            x = x * 10 + (s[i] - '0');
            scale *= 10;
            digits++;
        } else {
            return -1;
        }
    }
    if (scale == 1)
        return -1;
    *v = scale ? x / scale : x;
    return 0;
}

int perf_read_budgets(struct perf_budgets *b, const char *text, char *err, size_t errlen)
{
    struct perf_budgets nb = { 0 };
    const char *s = text;
    size_t lineno = 0, i;

#define FAIL(...) do { snprintf(err, errlen, __VA_ARGS__); perf_budgets_free(&nb); return -1; } while (0)
    while (*s) {
        const char *e = strchr(s, '\n'), *f[4] = { 0 }, *t;
        size_t len[4], nf;
        struct perf_budget x = { 0 };
        struct perf_budget *p;

        if (!e)
            e = s + strlen(s);
        lineno++;
        nf = split(s, e, f, len, 3);
        if (nf == 0 || *f[0] == '#') {
            s = *e ? e + 1 : e;
            continue;
        }
        /* the text: the rest of the line after the third field */
        if (nf > 3) {
            f[3] = f[2] + len[2];
            while (*f[3] == ' ' || *f[3] == '\t')
                f[3]++;
        }
        if (nf <= 3)
            FAIL("line %zu: not \"COMMAND BUDGET TARGET TEXT\": \"%.*s\"", lineno, (int)(e - s),
                 s);
        if (!memchr(f[0], '.', len[0]) || f[0][0] == '.' || f[0][len[0] - 1] == '.')
            FAIL("line %zu: the command \"%.*s\" is not PROGRAM.TEST", lineno, (int)len[0], f[0]);
        if (len[0] > 6 && !strncmp(f[0] + len[0] - 6, ".empty", 6))
            FAIL("line %zu: %.*s is the empty loop, not a command", lineno, (int)len[0], f[0]);
        {
            /* "B" or "B~T" */
            const char *tilde = memchr(f[1], '~', len[1]);
            size_t bl = tilde ? (size_t)(tilde - f[1]) : len[1];

            if (number(f[1], bl, &x.budget))
                FAIL("line %zu: the budget \"%.*s\" is not a whole number of cycles", lineno,
                     (int)len[1], f[1]);
            if (tilde && (number(tilde + 1, len[1] - bl - 1, &x.tolerance) || x.tolerance <= 0))
                FAIL("line %zu: the tolerance in \"%.*s\" is not a positive whole number of "
                     "cycles", lineno, (int)len[1], f[1]);
        }
        if (len[2] == 1 && f[2][0] == '-')
            x.has_target = 0;
        else if (decimal(f[2], len[2], &x.target))
            FAIL("line %zu: the target \"%.*s\" is not a number of cycles or -", lineno,
                 (int)len[2], f[2]);
        else
            x.has_target = 1;
        for (i = 0; i < nb.n; i++)
            if (strlen(nb.b[i].name) == len[0] && !strncmp(nb.b[i].name, f[0], len[0]))
                FAIL("line %zu: the command %.*s twice", lineno, (int)len[0], f[0]);
        /* the text: from the fourth field to the end of the line */
        t = e;
        while (t > f[3] && (t[-1] == ' ' || t[-1] == '\t' || t[-1] == '\r'))
            t--;
        if (!(x.name = dup_n(f[0], len[0])) || !(x.text = dup_n(f[3], (size_t)(t - f[3])))
            || !(p = realloc(nb.b, (nb.n + 1) * sizeof *p))) {
            free(x.name);
            free(x.text);
            FAIL("out of memory");
        }
        nb.b = p;
        nb.b[nb.n++] = x;
        s = *e ? e + 1 : e;
    }
#undef FAIL
    perf_budgets_free(b);
    *b = nb;
    return 0;
}

static const struct perf_budget *budget_of(const struct perf_budgets *b, const char *name)
{
    size_t i;

    for (i = 0; i < b->n; i++)
        if (!strcmp(b->b[i].name, name))
            return &b->b[i];
    return NULL;
}

static int measured(const struct perf_run *r, const char *name)
{
    size_t i;

    for (i = 0; i < r->n; i++)
        if (!strcmp(r->m[i].name, name))
            return 1;
    return 0;
}

int perf_judge(const struct perf_run *r, const struct perf_budgets *b, FILE *f)
{
    size_t i;
    int failed = 0;

    for (i = 0; i < r->n; i++) {
        const struct perf_meas *m = &r->m[i];
        const struct perf_budget *p;
        const char *per = m->frame ? "frame" : "call";

        if (m->empty)
            continue;
        if (!(p = budget_of(b, m->name))) {
            fprintf(f, "%s: measured %.1f cycles per %s, no budget\n", m->name, m->value, per);
            failed++;
        } else if (m->value > p->budget + p->tolerance) {
            if (p->tolerance)
                fprintf(f, "%s: measured %.1f cycles per %s, budget %ld + tolerance %ld\n",
                        m->name, m->value, per, p->budget, p->tolerance);
            else
                fprintf(f, "%s: measured %.1f cycles per %s, budget %ld\n", m->name, m->value,
                        per, p->budget);
            failed++;
        }
    }
    for (i = 0; i < b->n; i++)
        if (!measured(r, b->b[i].name)) {
            fprintf(f, "%s: not measured (budget %ld)\n", b->b[i].name, b->b[i].budget);
            failed++;
        }
    return failed;
}

void perf_table(const struct perf_run *r, const struct perf_budgets *b, FILE *f)
{
    size_t i;

    if (b)
        fprintf(f, "%-24s %10s %8s %10s %7s  %s\n", "command", "cycles", "budget", "target",
                "factor", "loop");
    else
        fprintf(f, "%-24s %10s\n", "command", "cycles");
    for (i = 0; i < r->n; i++) {
        const struct perf_meas *m = &r->m[i];
        const struct perf_budget *p = b ? budget_of(b, m->name) : NULL;

        if (m->empty) {
            fprintf(f, "%-24s %10.1f  (empty loop, per iteration)\n", m->name, m->value);
        } else if (!p) {
            fprintf(f, "%-24s %10.1f%s\n", m->name, m->value, m->frame ? "  (per frame)" : "");
        } else {
            char target[24] = "-", factor[24] = "-";

            if (p->has_target) {
                snprintf(target, sizeof target, "%.1f", p->target);
                if (p->target >= 1)
                    snprintf(factor, sizeof factor, "%.2f", m->value / p->target);
            }
            char tol[32] = "";

            if (p->tolerance)
                snprintf(tol, sizeof tol, " (varies, tolerance %ld)", p->tolerance);
            fprintf(f, "%-24s %10.1f %8ld %10s %7s  %s%s%s\n", m->name, m->value,
                    p->budget + p->tolerance, target, factor, p->text,
                    m->frame ? " (per frame)" : "", tol);
        }
    }
}
