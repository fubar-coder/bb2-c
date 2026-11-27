/*
 * testdriver.c - the driver of a Blitz test of the model (testdriver.h).
 */
#include "testdriver.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct out {
    char *p;
    size_t n, cap;
    unsigned lines;
    int bad;
};

static void add(struct out *o, const char *s, size_t n)
{
    if (o->bad)
        return;
    if (o->n + n + 1 > o->cap) {
        size_t cap = (o->n + n + 1) * 2;
        char *p = realloc(o->p, cap);

        if (!p) {
            o->bad = 1;
            return;
        }
        o->p = p;
        o->cap = cap;
    }
    memcpy(o->p + o->n, s, n);
    o->n += n;
    o->p[o->n] = 0;
    for (size_t i = 0; i < n; i++)
        o->lines += s[i] == '\n';
}

static void addf(struct out *o, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void addf(struct out *o, const char *fmt, ...)
{
    char buf[300];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof buf)
        o->bad = 1;
    else
        add(o, buf, (size_t)n);
}

/* a text with its lines ended: LF behind a last line without one */
static void add_lines(struct out *o, const char *s, size_t n)
{
    add(o, s, n);
    if (n && s[n - 1] != '\n')
        add(o, "\n", 1);
}

static int called(struct tp_driver *d, const char *name)
{
    char **v = realloc(d->called, (d->ncalled + 1) * sizeof *v);

    if (!v)
        return -1;
    d->called = v;
    if (!(v[d->ncalled] = strdup(name)))
        return -1;
    d->ncalled++;
    return 0;
}

static int selected(const struct tp_driver_params *dp, const char *name)
{
    for (size_t i = 0; i < dp->ncases; i++)
        if (!strcmp(dp->cases[i], name))
            return 1;
    return 0;
}

static int in_source(const struct tp_source *src, const char *name)
{
    for (size_t i = 0; i < src->ncases; i++)
        if (!strcmp(src->cases[i].name, name))
            return 1;
    return 0;
}

int tp_driver_write(const struct tp_source *src, const struct tp_driver_params *dp, struct tp_driver *d,
                    char *err, size_t errlen)
{
    struct out o = { 0 };
    size_t rl = dp->result ? strlen(dp->result) : 0;

    memset(d, 0, sizeof *d);
    if (!dp->result || !rl || rl > 100) {
        snprintf(err, errlen, "the result file's name must have 1 to 100 bytes");
        return -1;
    }
    for (size_t i = 0; i < rl; i++)
        if ((unsigned char)dp->result[i] < 0x20 || (unsigned char)dp->result[i] > 0x7e || dp->result[i] == '"') {
            snprintf(err, errlen, "the result file's name %s cannot stand in a string literal", dp->result);
            return -1;
        }
    for (size_t i = 0; i < dp->ncases; i++) {
        const char *c = dp->cases[i];
        size_t k;

        for (k = 0; c[k]; k++)
            if (!((c[k] >= 'a' && c[k] <= 'z') || (c[k] >= '0' && c[k] <= '9') || c[k] == '_'))
                break;
        if (!c[0] || c[k] || k > 60) {
            snprintf(err, errlen, "%s is no case name", c);
            return -1;
        }
    }

    if (src->header)
        add_lines(&o, src->header, src->header_len);
    add_lines(&o, dp->macros, dp->macros_len);
    addf(&o, "TLBegin \"%s\"\nGoto tc__skip\n", dp->result);
    d->first = o.lines + 1;
    add_lines(&o, src->text, src->len);
    d->nsource = o.lines + 1 - d->first;
    add(&o, ".tc__skip\n", 10);
    if (dp->ncases) {
        d->calls = o.lines + 1;
        if (src->init_line) {
            add(&o, "Gosub tc__init\n", 15);
            if (called(d, "init"))
                goto oom;
        }
        for (size_t i = 0; i < src->ncases; i++)
            if (selected(dp, src->cases[i].name)) {
                addf(&o, "Gosub tc_%s\n", src->cases[i].name);
                if (called(d, src->cases[i].name))
                    goto oom;
            }
        for (size_t i = 0; i < dp->ncases; i++)
            if (!in_source(src, dp->cases[i])) {
                addf(&o, "Gosub tc_%s\n", dp->cases[i]);
                if (called(d, dp->cases[i]))
                    goto oom;
            }
        if (src->cleanup_line) {
            add(&o, "Gosub tc__final\n", 16);
            if (called(d, "final"))
                goto oom;
        }
    }
    add(&o, "End\n", 4);
    if (o.bad)
        goto oom;
    d->text = o.p;
    d->len = o.n;
    d->nlines = o.lines;
    return 0;
oom:
    free(o.p);
    tp_driver_free(d);
    return -2;
}

void tp_driver_free(struct tp_driver *d)
{
    free(d->text);
    for (size_t i = 0; i < d->ncalled; i++)
        free(d->called[i]);
    free(d->called);
    memset(d, 0, sizeof *d);
}

int tp_driver_where(const struct tp_driver *d, unsigned driver_line, unsigned *line, const char **name)
{
    if (driver_line >= d->first && driver_line < d->first + d->nsource) {
        *line = driver_line - d->first + 1;
        return TP_IN_SOURCE;
    }
    if (d->calls && driver_line >= d->calls && driver_line < d->calls + d->ncalled) {
        *name = d->called[driver_line - d->calls];
        return TP_IN_CALL;
    }
    if (driver_line >= d->first + d->nsource)
        return TP_IN_END;
    return TP_IN_MACROS;
}
