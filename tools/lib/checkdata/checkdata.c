/*
 * checkdata.c - data of a C build against the reference, see checkdata.h.
 */
#define _XOPEN_SOURCE 700       /* strndup */
#include <stdlib.h>
#include <string.h>
#include "checkdata.h"

void checkdata_table_free(struct checkdata_table *t)
{
    size_t i;

    for (i = 0; i < t->n; i++) {
        free(t->v[i].csym);
        free(t->v[i].start);
        free(t->v[i].end);
        free(t->v[i].note);
    }
    free(t->v);
    t->v = NULL;
    t->n = 0;
}

int checkdata_table_parse(const char *text, size_t len, struct checkdata_table *t,
                          char *err, size_t errlen)
{
    const char *p = text, *end = text + len;
    unsigned line = 0;

    t->v = NULL;
    t->n = 0;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *f[4];
        size_t fl[4], k = 0;
        struct checkdata_entry *e;
        char *d;

        if (!eol)
            eol = end;
        line++;
        if (p == eol || *p == '#') {
            p = eol + 1;
            continue;
        }
        /* up to 4 fields; the note (the last one) may contain anything */
        while (k < 4) {
            const char *tab = k < 3 ? memchr(p, '\t', (size_t)(eol - p)) : NULL;

            f[k] = p;
            fl[k] = (size_t)((tab ? tab : eol) - p);
            k++;
            if (!tab)
                break;
            p = tab + 1;
        }
        if (k < 3 || !fl[0] || !fl[1] || !fl[2]) {
            snprintf(err, errlen, "line %u: expected CSYM START END|SIZE [NOTE]", line);
            checkdata_table_free(t);
            return -1;
        }
        if (t->n % 64 == 0) {
            struct checkdata_entry *v = realloc(t->v, (t->n + 64) * sizeof *v);

            if (!v)
                goto nomem;
            t->v = v;
        }
        e = &t->v[t->n++];
        memset(e, 0, sizeof *e);
        e->line = line;
        e->csym = strndup(f[0], fl[0]);
        e->start = strndup(f[1], fl[1]);
        e->note = strndup(k > 3 ? f[3] : "", k > 3 ? fl[3] : 0);
        e->size = (uint32_t)strtoul(f[2], &d, 10);
        if (d != f[2] + fl[2] && !(e->end = strndup(f[2], fl[2])))
            goto nomem;
        if (!e->csym || !e->start || !e->note)
            goto nomem;
        p = eol + 1;
    }
    return 0;
nomem:
    snprintf(err, errlen, "out of memory");
    checkdata_table_free(t);
    return -1;
}

/* value of a label of the listing (the last definition wins) */
static int label(const struct listing *l, const char *name, uint32_t *value)
{
    size_t i;
    int found = -1;

    for (i = 0; i < l->nsyms; i++)
        if (!strcmp(l->syms[i].name, name)) {
            *value = l->syms[i].value;
            found = 0;
        }
    return found;
}

long checkdata_run(const struct checkdata_table *t, const struct listing *l,
                   const struct hunk *ref, const struct loadfile *c, int verbose, FILE *out)
{
    uint8_t *skip = calloc((size_t)ref->size + 4, 1);
    long bad = 0;
    size_t i;
    uint32_t r, k;

    if (!skip) {
        fprintf(out, "checkdata: out of memory\n");
        return 1;
    }
    for (r = 0; r < ref->nrelocs; r++)
        for (k = 0; k < 4 && ref->reloc_offset[r] + k < ref->size; k++)
            skip[ref->reloc_offset[r] + k] = 1;
    for (i = 0; i < t->n; i++) {
        const struct checkdata_entry *e = &t->v[i];
        uint32_t start, stop = 0, hunk, off, n, ndiff = 0, first = 0;
        const struct hunk *h;

        if (label(l, e->start, &start) || (e->end && label(l, e->end, &stop))) {
            bad++;
            fprintf(out, "FAIL    %-14s %s/%s: label not in the listing (table line %u)\n",
                    e->csym, e->start, e->end ? e->end : "-", e->line);
            continue;
        }
        if (!e->end)
            stop = start + e->size;
        if (stop > ref->size)
            stop = ref->size;
        if (stop < start)
            stop = start;
        if (load_symbol(c, e->csym, &hunk, &off)) {
            bad++;
            fprintf(out, "MISSING %-14s (%s, %s)\n", e->csym, e->start, e->note);
            continue;
        }
        h = &c->hunks[hunk];
        n = stop - start;
        for (k = 0; k < n; k++) {
            /* behind the initialised part (or BSS): zero */
            uint8_t got = h->data && off + k < h->size ? h->data[off + k] : 0;

            if (skip[start + k] || got == ref->data[start + k])
                continue;
            if (!ndiff++)
                first = k;
        }
        if (ndiff) {
            bad++;
            fprintf(out, "FAIL    %-14s %s: %u of %u bytes differ (first at +%u) - %s\n", e->csym,
                    e->start, (unsigned)ndiff, (unsigned)n, (unsigned)first, e->note);
        } else if (verbose)
            fprintf(out, "ok      %-14s %s (%u bytes)\n", e->csym, e->start, (unsigned)n);
    }
    fprintf(out, "checkdata: %zu checks, %ld failed\n", t->n, bad);
    free(skip);
    return bad;
}
