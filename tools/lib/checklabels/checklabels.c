/*
 * checklabels.c - code labels of an assembler source and their markers in
 * the C sources, see checklabels.h.
 */
#define _XOPEN_SOURCE 700       /* strndup */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "checklabels.h"

/* end labels of data blocks that are followed directly by code (so the
   rule sees an instruction); they are not code */
static const char *const not_code[] = { "dumsleepf", "finish" };

static const char *const data_ops[] = {
    "dc", "ds", "dcb", "incbin", "rs", "rsreset", "rsset", "even", "cnop",
    "include", "section", "xdef",
};

static const char *const equ_ops[] = { "equ", "=", "set", "macro", "endm" };

static int is_space(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

static int is_name_start(int c)
{
    return isalpha(c) || c == '_';
}

static int is_name(int c)
{
    return isalnum(c) || c == '_';
}

/* s (n bytes) is one of set; directives in any case, names exactly */
static int in(const char *s, size_t n, const char *const *set, size_t nset, int anycase)
{
    size_t i;

    for (i = 0; i < nset; i++)
        if (strlen(set[i]) == n && !(anycase ? strncasecmp(s, set[i], n) : strncmp(s, set[i], n)))
            return 1;
    return 0;
}

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* op (without size) is a data directive: NAME or NAME.b/.w/.l */
static int is_data(const char *op, size_t n)
{
    if (n > 2 && op[n - 2] == '.' && strchr("bwlBWL", op[n - 1]))
        if (in(op, n - 2, data_ops, COUNT(data_ops), 1))
            return 1;
    return in(op, n, data_ops, COUNT(data_ops), 1);
}

static int add(struct code_labels *l, unsigned line, const char *name, size_t n)
{
    if (l->n % 256 == 0) {
        struct code_label *v = realloc(l->v, (l->n + 256) * sizeof *v);

        if (!v)
            return -1;
        l->v = v;
    }
    if (!(l->v[l->n].name = strndup(name, n)))
        return -1;
    l->v[l->n++].line = line;
    return 0;
}

int code_labels_parse(const char *text, size_t len, struct code_labels *l)
{
    /* labels waiting for their first statement */
    struct { unsigned line; const char *name; size_t n; } pending[64];
    size_t npending = 0;
    const char *p = text, *end = text + len;
    unsigned lineno = 0;

    l->v = NULL;
    l->n = 0;
    while (p < end) {
        const char *line = p, *eol = memchr(p, '\n', (size_t)(end - p));
        const char *s, *e, *op, *ope;
        size_t i;

        if (!eol)
            eol = end;
        p = eol < end ? eol + 1 : end;
        lineno++;
        /* the statement: up to ';', without trailing white space */
        e = memchr(line, ';', (size_t)(eol - line));
        if (!e)
            e = eol;
        while (e > line && is_space((unsigned char)e[-1]))
            e--;
        for (s = line; s < e && is_space((unsigned char)*s); s++)
            ;
        if (s == e || *line == '*')
            continue;
        s = line;
        if (!is_space((unsigned char)*line)) {
            /* NAME[:] [white space STATEMENT] */
            const char *n = line;

            if (!is_name_start((unsigned char)*n))
                continue;
            while (n < e && is_name((unsigned char)*n))
                n++;
            s = n;
            if (s < e && *s == ':')
                s++;
            if (s < e && !is_space((unsigned char)*s))
                continue;
            if (npending < COUNT(pending)) {
                pending[npending].line = lineno;
                pending[npending].name = line;
                pending[npending++].n = (size_t)(n - line);
            }
            while (s < e && is_space((unsigned char)*s))
                s++;
            if (s == e)
                continue;
        }
        for (op = s; op < e && is_space((unsigned char)*op); op++)
            ;
        for (ope = op; ope < e && !is_space((unsigned char)*ope); ope++)
            ;
        if (!is_data(op, (size_t)(ope - op)) && !in(op, (size_t)(ope - op), equ_ops, COUNT(equ_ops), 1))
            for (i = 0; i < npending; i++)
                if (!in(pending[i].name, pending[i].n, not_code, COUNT(not_code), 0)
                    && add(l, pending[i].line, pending[i].name, pending[i].n)) {
                    code_labels_free(l);
                    return -1;
                }
        npending = 0;
    }
    return 0;
}

void code_labels_free(struct code_labels *l)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        free(l->v[i].name);
    free(l->v);
    l->v = NULL;
    l->n = 0;
}

long labels_check(const struct code_labels *l, const char *prefix,
                  const struct label_source *src, size_t nsrc, int verbose, FILE *out)
{
    /* per label: how often, the first two files */
    struct hit { unsigned count; const char *file[2]; } *hits = calloc(l->n ? l->n : 1, sizeof *hits);
    size_t plen = strlen(prefix), i, f;
    long missing = 0, twice = 0;

    if (!hits)
        return -1;
    for (f = 0; f < nsrc; f++) {
        const char *p = src[f].text, *end = src[f].text + src[f].len;

        /* PREFIX:DIGITS NAME (non-overlapping, left to right) */
        while (p + plen < end) {
            const char *q = p + plen, *name, *ne;
            unsigned long line;
            char *d;

            if (memcmp(p, prefix, plen) || *q != ':' || !isdigit((unsigned char)q[1])) {
                p++;
                continue;
            }
            line = strtoul(q + 1, &d, 10);
            name = d + 1;
            if (d >= end || *d != ' ' || name >= end || !is_name((unsigned char)*name)) {
                p++;
                continue;
            }
            for (ne = name; ne < end && is_name((unsigned char)*ne); ne++)
                ;
            for (i = 0; i < l->n; i++)
                if (l->v[i].line == line && strlen(l->v[i].name) == (size_t)(ne - name)
                    && !memcmp(l->v[i].name, name, (size_t)(ne - name))) {
                    if (hits[i].count < 2)
                        hits[i].file[hits[i].count] = src[f].name;
                    hits[i].count++;
                    break;
                }
            p = ne;
        }
    }
    for (i = 0; i < l->n; i++) {
        const struct code_label *c = &l->v[i];

        if (!hits[i].count) {
            missing++;
            fprintf(out, "MISSING %s:%u %s\n", prefix, c->line, c->name);
        } else if (hits[i].count > 1) {
            twice++;
            fprintf(out, "TWICE   %s:%u %s: %s, %s%s\n", prefix, c->line, c->name,
                    hits[i].file[0], hits[i].file[1], hits[i].count > 2 ? ", ..." : "");
        } else if (verbose)
            fprintf(out, "ok      %s:%u %s: %s\n", prefix, c->line, c->name, hits[i].file[0]);
    }
    fprintf(out, "checklabels: %zu code labels, %ld missing, %ld more than once\n", l->n,
            missing, twice);
    free(hits);
    return missing + twice;
}
