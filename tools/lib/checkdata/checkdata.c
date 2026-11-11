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

void checkdata_intended_free(struct checkdata_intended *in)
{
    size_t i;

    for (i = 0; i < in->n; i++) {
        free(in->v[i].csym);
        free(in->v[i].note);
    }
    free(in->v);
    in->v = NULL;
    in->n = 0;
}

int checkdata_intended_parse(const char *text, size_t len, struct checkdata_intended *in,
                             char *err, size_t errlen)
{
    const char *p = text, *end = text + len;
    unsigned line = 0;

    in->v = NULL;
    in->n = 0;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *tab, *issue, *note, *d;
        struct checkdata_intended_entry *e;
        unsigned long nr = 0;
        size_t i, symlen, issuelen;

        if (!eol)
            eol = end;
        line++;
        if (p == eol || *p == '#') {
            p = eol + 1;
            continue;
        }
        /* CSYM<TAB>#NNN[<TAB>NOTE] */
        tab = memchr(p, '\t', (size_t)(eol - p));
        symlen = (size_t)((tab ? tab : eol) - p);
        issue = tab ? tab + 1 : eol;
        note = memchr(issue, '\t', (size_t)(eol - issue));
        issuelen = (size_t)((note ? note : eol) - issue);
        for (d = issue + 1; d < issue + issuelen && *d >= '0' && *d <= '9'; d++)
            nr = nr * 10 + (unsigned long)(*d - '0');
        if (!symlen || issuelen < 2 || *issue != '#' || d != issue + issuelen || !nr
            || nr > 99999) {
            snprintf(err, errlen, "line %u: expected CSYM #ISSUE [NOTE]", line);
            checkdata_intended_free(in);
            return -1;
        }
        for (i = 0; i < in->n; i++)
            if (strlen(in->v[i].csym) == symlen && !memcmp(in->v[i].csym, p, symlen)) {
                snprintf(err, errlen, "line %u: %s listed twice (line %u)", line, in->v[i].csym,
                         in->v[i].line);
                checkdata_intended_free(in);
                return -1;
            }
        if (in->n % 16 == 0) {
            struct checkdata_intended_entry *v = realloc(in->v, (in->n + 16) * sizeof *v);

            if (!v)
                goto nomem;
            in->v = v;
        }
        e = &in->v[in->n++];
        e->line = line;
        e->issue = (unsigned)nr;
        e->csym = strndup(p, symlen);
        e->note = note ? strndup(note + 1, (size_t)(eol - note - 1)) : strdup("");
        if (!e->csym || !e->note)
            goto nomem;
        p = eol + 1;
    }
    return 0;
nomem:
    snprintf(err, errlen, "out of memory");
    checkdata_intended_free(in);
    return -1;
}

/* the intended difference of a check, or -1 */
static long intended_index(const struct checkdata_intended *in, const char *csym)
{
    size_t i;

    for (i = 0; in && i < in->n; i++)
        if (!strcmp(in->v[i].csym, csym))
            return (long)i;
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

/* the third field of a check: the end label or the size */
static const char *end_field(const struct checkdata_entry *e, char *buf, size_t len)
{
    if (e->end)
        return e->end;
    snprintf(buf, len, "%lu", (unsigned long)e->size);
    return buf;
}

/* the reference data of a range, or NULL */
static const struct checkdata_ref_entry *ref_find(const struct checkdata_ref *r,
                                                  const char *start, const char *end)
{
    size_t i;

    for (i = 0; i < r->n; i++)
        if (!strcmp(r->v[i].start, start) && !strcmp(r->v[i].end, end))
            return &r->v[i];
    return NULL;
}

/* a new entry of r (zeroed), or NULL: out of memory */
static struct checkdata_ref_entry *ref_add(struct checkdata_ref *r)
{
    if (r->n % 64 == 0) {
        struct checkdata_ref_entry *v = realloc(r->v, (r->n + 64) * sizeof *v);

        if (!v)
            return NULL;
        r->v = v;
    }
    memset(&r->v[r->n], 0, sizeof r->v[r->n]);
    return &r->v[r->n++];
}

void checkdata_ref_free(struct checkdata_ref *r)
{
    size_t i;

    for (i = 0; i < r->n; i++) {
        free(r->v[i].start);
        free(r->v[i].end);
        free(r->v[i].data);
        free(r->v[i].skip);
    }
    free(r->v);
    r->v = NULL;
    r->n = 0;
}

long checkdata_ref_make(const struct checkdata_table *t, const struct listing *l,
                        const struct hunk *ref, struct checkdata_ref *r, FILE *msgs)
{
    uint8_t *skip = calloc((size_t)ref->size + 4, 1);
    long missing = 0;
    size_t i;
    uint32_t k;

    r->v = NULL;
    r->n = 0;
    if (!skip)
        return -1;
    for (k = 0; k < ref->nrelocs; k++) {
        uint32_t b;

        for (b = 0; b < 4 && ref->reloc_offset[k] + b < ref->size; b++)
            skip[ref->reloc_offset[k] + b] = 1;
    }
    for (i = 0; i < t->n; i++) {
        const struct checkdata_entry *e = &t->v[i];
        struct checkdata_ref_entry *x;
        uint32_t start, stop = 0;
        char buf[16];
        const char *end = end_field(e, buf, sizeof buf);

        if (ref_find(r, e->start, end))
            continue;
        if (label(l, e->start, &start) || (e->end && label(l, e->end, &stop))) {
            missing++;
            if (msgs)
                fprintf(msgs, "FAIL    %-14s %s/%s: label not in the listing (table line %u)\n",
                        e->csym, e->start, end, e->line);
            continue;
        }
        if (!e->end)
            stop = start + e->size;
        if (stop > ref->size)
            stop = ref->size;
        if (stop < start)
            stop = start;
        if (!(x = ref_add(r)))
            goto nomem;
        x->n = stop - start;
        x->start = strdup(e->start);
        x->end = strdup(end);
        x->data = malloc((size_t)x->n + 1);
        x->skip = malloc((size_t)x->n + 1);
        if (!x->start || !x->end || !x->data || !x->skip)
            goto nomem;
        memcpy(x->data, ref->data + start, x->n);
        memcpy(x->skip, skip + start, x->n);
    }
    free(skip);
    return missing;
nomem:
    free(skip);
    checkdata_ref_free(r);
    return -1;
}

void checkdata_ref_write(const struct checkdata_ref *r, const char *header, FILE *out)
{
    size_t i;
    uint32_t k;

    while (header && *header) {
        const char *eol = strchr(header, '\n');
        int n = eol ? (int)(eol - header) : (int)strlen(header);

        fprintf(out, "# %.*s\n", n, header);
        header += n + (eol != NULL);
    }
    for (i = 0; i < r->n; i++) {
        fprintf(out, "%s\t%s\t", r->v[i].start, r->v[i].end);
        for (k = 0; k < r->v[i].n; k++)
            if (r->v[i].skip[k])
                fputs("..", out);
            else
                fprintf(out, "%02x", r->v[i].data[k]);
        fputc('\n', out);
    }
}

static int hexdigit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

int checkdata_ref_parse(const char *text, size_t len, struct checkdata_ref *r, char *err,
                        size_t errlen)
{
    const char *p = text, *end = text + len;
    unsigned line = 0;

    r->v = NULL;
    r->n = 0;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *t1, *t2, *h;
        const struct checkdata_ref_entry *dup;
        struct checkdata_ref_entry *x;
        size_t hl;
        uint32_t k;

        if (!eol)
            eol = end;
        line++;
        if (p == eol || *p == '#') {
            p = eol + 1;
            continue;
        }
        t1 = memchr(p, '\t', (size_t)(eol - p));
        t2 = t1 ? memchr(t1 + 1, '\t', (size_t)(eol - t1 - 1)) : NULL;
        h = t2 ? t2 + 1 : eol;
        hl = (size_t)(eol - h);
        if (!t2 || t1 == p || t2 == t1 + 1 || hl % 2 || memchr(h, '\t', hl)) {
            snprintf(err, errlen, "line %u: expected START END|SIZE HEX", line);
            goto fail;
        }
        if (!(x = ref_add(r)))
            goto nomem;
        x->line = line;
        x->n = (uint32_t)(hl / 2);
        x->start = strndup(p, (size_t)(t1 - p));
        x->end = strndup(t1 + 1, (size_t)(t2 - t1 - 1));
        x->data = malloc((size_t)x->n + 1);
        x->skip = malloc((size_t)x->n + 1);
        if (!x->start || !x->end || !x->data || !x->skip)
            goto nomem;
        for (k = 0; k < x->n; k++) {
            int hi = hexdigit(h[2 * k]), lo = hexdigit(h[2 * k + 1]);

            x->skip[k] = h[2 * k] == '.' && h[2 * k + 1] == '.';
            if (!x->skip[k] && (hi < 0 || lo < 0)) {
                snprintf(err, errlen, "line %u: byte %u: expected two hex digits or ..", line,
                         (unsigned)k);
                goto fail;
            }
            x->data[k] = x->skip[k] ? 0 : (uint8_t)(hi << 4 | lo);
        }
        r->n--;
        dup = ref_find(r, x->start, x->end);
        r->n++;
        if (dup) {
            snprintf(err, errlen, "line %u: %s %s listed twice (line %u)", line, x->start,
                     x->end, dup->line);
            goto fail;
        }
        p = eol + 1;
    }
    return 0;
nomem:
    snprintf(err, errlen, "out of memory");
fail:
    checkdata_ref_free(r);
    return -1;
}

long checkdata_run(const struct checkdata_table *t, const struct checkdata_ref *r,
                   const struct loadfile *c, const struct checkdata_intended *in, int verbose,
                   FILE *out)
{
    size_t nin = in ? in->n : 0;
    unsigned char *seen = calloc(nin + 1, 1);
    long bad = 0, intended = 0;
    size_t i;
    uint32_t k;

    if (!seen) {
        fprintf(out, "checkdata: out of memory\n");
        return 1;
    }
    for (i = 0; i < t->n; i++) {
        const struct checkdata_entry *e = &t->v[i];
        const struct checkdata_ref_entry *x;
        uint32_t hunk, off, ndiff = 0, first = 0;
        const struct hunk *h;
        char buf[16];
        const char *end = end_field(e, buf, sizeof buf);
        long j;

        if (!(x = ref_find(r, e->start, end))) {
            bad++;
            fprintf(out, "FAIL    %-14s %s/%s: not in the reference data (table line %u)\n",
                    e->csym, e->start, end, e->line);
            continue;
        }
        if (load_symbol(c, e->csym, &hunk, &off)) {
            bad++;
            fprintf(out, "MISSING %-14s (%s, %s)\n", e->csym, e->start, e->note);
            continue;
        }
        h = &c->hunks[hunk];
        for (k = 0; k < x->n; k++) {
            /* behind the initialised part (or BSS): zero */
            uint8_t got = h->data && off + k < h->size ? h->data[off + k] : 0;

            if (x->skip[k] || got == x->data[k])
                continue;
            if (!ndiff++)
                first = k;
        }
        if (ndiff && (j = intended_index(in, e->csym)) >= 0) {
            /* a fixed original bug: the difference is the point */
            seen[j] = 1;
            intended++;
            fprintf(out, "intended %-13s %s: %u of %u bytes differ (first at +%u) - #%u\n",
                    e->csym, e->start, (unsigned)ndiff, (unsigned)x->n, (unsigned)first,
                    in->v[j].issue);
        } else if (ndiff) {
            bad++;
            fprintf(out, "FAIL    %-14s %s: %u of %u bytes differ (first at +%u) - %s\n", e->csym,
                    e->start, (unsigned)ndiff, (unsigned)x->n, (unsigned)first, e->note);
        } else if (verbose)
            fprintf(out, "ok      %-14s %s (%u bytes)\n", e->csym, e->start, (unsigned)x->n);
    }
    /* an entry of the list that did not occur: stale */
    for (i = 0; i < nin; i++)
        if (!seen[i]) {
            bad++;
            fprintf(out, "STALE   %-14s intended difference (#%u) not found (intended line %u)\n",
                    in->v[i].csym, in->v[i].issue, in->v[i].line);
        }
    if (in)
        fprintf(out, "checkdata: %zu checks, %ld failed, %ld intended\n", t->n, bad, intended);
    else
        fprintf(out, "checkdata: %zu checks, %ld failed\n", t->n, bad);
    free(seen);
    return bad;
}
