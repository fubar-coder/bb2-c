/* cdcmp.c - see cdcmp.h */
#include "cdcmp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NFIELDS 8
#define F_PATH 0
#define F_RESULT 1
#define F_LINE 2
#define F_MESSAGE 3
#define F_SIZE 4
#define F_SHA 5

struct row {
    char *f[NFIELDS];
};

struct table {
    char *text;
    struct row *rows;
    size_t n;
};

struct text {
    char *p;
    size_t n, cap;
};

static void text_add(struct text *t, const char *fmt, ...)
{
    va_list ap;
    char buf[4096];
    size_t k;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    k = strlen(buf);
    if (t->n + k + 1 > t->cap) {
        t->cap = (t->n + k + 1) * 2;
        t->p = realloc(t->p, t->cap);
        if (!t->p)
            abort();
    }
    memcpy(t->p + t->n, buf, k + 1);
    t->n += k;
}

/* rows of the table text (the header is skipped); 0 = ok, else the line
   number (1 based) that has fewer than NFIELDS fields */
static size_t parse(const char *src, struct table *t)
{
    char *l, *next;
    size_t cap = 0, line = 0;

    memset(t, 0, sizeof *t);
    t->text = strdup(src);
    if (!t->text)
        abort();
    for (l = t->text; *l; l = next) {
        char *e = strchr(l, '\n'), *p = l;
        struct row *r;
        size_t k;

        line++;
        if (e) {
            *e = 0;
            next = e + 1;
        } else {
            next = l + strlen(l);
        }
        if (line == 1)
            continue;
        if (t->n == cap) {
            cap = cap ? 2 * cap : 64;
            t->rows = realloc(t->rows, cap * sizeof *t->rows);
            if (!t->rows)
                abort();
        }
        r = &t->rows[t->n];
        for (k = 0; k < NFIELDS; k++) {
            r->f[k] = p;
            if (k < NFIELDS - 1) {
                if (!(p = strchr(p, '\t')))
                    return line;
                *p++ = 0;
            }
        }
        t->n++;
    }
    return 0;
}

static void free_table(struct table *t)
{
    free(t->rows);
    free(t->text);
}

static const struct row *find(const struct table *t, const char *path)
{
    size_t i;

    for (i = 0; i < t->n; i++)
        if (!strcmp(t->rows[i].f[F_PATH], path))
            return &t->rows[i];
    return NULL;
}

int cdcmp_compare(const char *refname, const char *outname, const char *ref, const char *out,
                  char **report, struct cdcmp_stats *st)
{
    struct table a, b;
    struct text rep = { 0 };
    size_t i, bad;
    int rc;

    memset(st, 0, sizeof *st);
    if ((bad = parse(ref, &a)) != 0) {
        text_add(&rep, "%s: line %zu: not %d tab separated fields\n", refname, bad, NFIELDS);
        free_table(&a);
        *report = rep.p;
        return 2;
    }
    if ((bad = parse(out, &b)) != 0) {
        text_add(&rep, "%s: line %zu: not %d tab separated fields\n", outname, bad, NFIELDS);
        free_table(&a);
        free_table(&b);
        *report = rep.p;
        return 2;
    }
    text_add(&rep, "== %s against %s\n", outname, refname);
    st->sources = a.n;
    for (i = 0; i < a.n; i++) {
        const struct row *x = &a.rows[i], *y = find(&b, x->f[F_PATH]);

        if (!y) {
            st->missing++;
            text_add(&rep, "   %s: not in %s\n", x->f[F_PATH], outname);
        } else if (strcmp(x->f[F_RESULT], y->f[F_RESULT]) || strcmp(x->f[F_LINE], y->f[F_LINE])
                   || strcmp(x->f[F_MESSAGE], y->f[F_MESSAGE])) {
            /* the compile result: reference -> now */
            static const char *const fields[] = { "result", "line", "message" };
            int f;

            st->different++;
            text_add(&rep, "   %s:", x->f[F_PATH]);
            for (f = F_RESULT; f <= F_MESSAGE; f++)
                if (strcmp(x->f[f], y->f[f]))
                    text_add(&rep, " %s %s -> %s", fields[f - F_RESULT], x->f[f], y->f[f]);
            text_add(&rep, "\n");
        } else {
            st->equal++;
            if (strcmp(x->f[F_SIZE], y->f[F_SIZE]) || strcmp(x->f[F_SHA], y->f[F_SHA])) {
                st->exe_differ++;
                text_add(&rep, "   %s: executable differs (informational): size %s -> %s, "
                         "sha256_masked %.12s -> %.12s\n", x->f[F_PATH], x->f[F_SIZE],
                         y->f[F_SIZE], x->f[F_SHA], y->f[F_SHA]);
            }
        }
    }
    for (i = 0; i < b.n; i++)
        if (!find(&a, b.rows[i].f[F_PATH])) {
            st->missing++;
            text_add(&rep, "   %s: not in the reference\n", b.rows[i].f[F_PATH]);
        }
    text_add(&rep, "%zu sources, %zu equal, %zu different, %zu missing", st->sources, st->equal,
             st->different, st->missing);
    if (st->exe_differ)
        text_add(&rep, " (%zu of the equal with another executable: information)", st->exe_differ);
    text_add(&rep, "\n");
    rc = st->different || st->missing ? 1 : 0;
    free_table(&a);
    free_table(&b);
    *report = rep.p;
    return rc;
}
