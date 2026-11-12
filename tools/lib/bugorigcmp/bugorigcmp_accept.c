/*
 * bugorigcmp_accept.c - the recorded original of a bug-orig test from two
 * recording runs (bugorigcmp.h): the runs are compared as cases by
 * libscmp_accept, the wildcard lines of the committed file are kept.
 */
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "bugorigcmp.h"
#include "bugorigcmp_int.h"

/* the case file of a run for libscmp_accept: one case ID|SECTION|orig|1
   per section, the value of a kept section (wildcards of the committed
   file, matched by both runs) the same in both */
static char *cases(const char *id, const struct bugorigcmp_obs *r, const struct bugorigcmp_obs *old,
                   size_t *len)
{
    char *text = NULL;
    FILE *f = open_memstream(&text, len);
    size_t i;

    if (!f)
        return NULL;
    fprintf(f, "# capture %s 1\n", id);
    for (i = 0; i < r->n; i++) {
        const struct bugorigcmp_sec *o = old ? bugorigcmp_get(old, r->sec[i].name) : NULL;

        fprintf(f, "%s|%s|orig|1\t", id, r->sec[i].name);
        if (o && o->wild)
            fprintf(f, "kept: the wildcard lines of the recorded original match");
        else
            bugorigcmp_put_value(f, r->sec[i].text);
        fputc('\n', f);
    }
    if (fclose(f)) {
        free(text);
        return NULL;
    }
    return text;
}

/* the first line in which a and b differ, literally: its number (from
   1), 0 = equal */
static unsigned literal_diff(const char *a, const char *b, char *la, size_t lena, char *lb,
                             size_t lenb)
{
    unsigned no = 1;

    while (*a || *b) {
        size_t na = strcspn(a, "\n"), nb = strcspn(b, "\n");

        if (!*a || !*b || na != nb || memcmp(a, b, na)) {
            bugorigcmp_show(a, na, la, lena, !*a);
            bugorigcmp_show(b, nb, lb, lenb, !*b);
            return no;
        }
        a += na + (a[na] == '\n');
        b += nb + (b[nb] == '\n');
        no++;
    }
    return 0;
}

static const char *text_of(const struct bugorigcmp_obs *o, const char *name)
{
    const struct bugorigcmp_sec *s = bugorigcmp_get(o, name);

    return s ? s->text : "";
}

/* the wildcard sections of old against a run: 0 = both match, else the
   number of sections reported */
static unsigned wild_mismatch(const char *id, const struct bugorigcmp_obs *old,
                              const struct bugorigcmp_obs *r, int k, FILE *report)
{
    char w[120], g[120];
    unsigned bad = 0;
    size_t i;

    for (i = 0; i < old->n; i++) {
        const struct bugorigcmp_sec *o = &old->sec[i];
        unsigned no;

        if (!o->wild)
            continue;
        no = bugorigcmp_first_diff(o->text, text_of(r, o->name), w, sizeof w, g, sizeof g);
        if (no) {
            fprintf(report, "%s: [%s]: the wildcard lines of the recorded original do not "
                    "match run %d: line %u: recorded \"%s\", run %d \"%s\"\n", id, o->name, k,
                    no, w, k, g);
            bad++;
        }
    }
    return bad;
}

/* a line of a section that would read as a wildcard: 0 = none */
static unsigned wild_line(const char *id, const struct bugorigcmp_obs *r,
                          const struct bugorigcmp_obs *old, FILE *report)
{
    unsigned bad = 0;
    size_t i;

    for (i = 0; i < r->n; i++) {
        const struct bugorigcmp_sec *o = old ? bugorigcmp_get(old, r->sec[i].name) : NULL;
        const char *p = r->sec[i].text;
        unsigned no = 1;
        char l[120];

        if (o && o->wild)
            continue;
        for (; *p; no++) {
            size_t n = strcspn(p, "\n");

            if (bugorigcmp_is_wild(p, n)) {
                bugorigcmp_show(p, n, l, sizeof l, 0);
                fprintf(report, "%s: [%s] line %u: the original's line \"%s\" reads as a "
                        "wildcard (`*` at the end, `...`): give it a \"# varies: REASON\" line "
                        "in the recorded original, or change the test\n", id, r->sec[i].name,
                        no, l);
                bad++;
            }
            p += n + (p[n] == '\n');
        }
    }
    return bad;
}

int bugorigcmp_accept(const char *id, const char *run1, const char *path1, const char *run2,
                      const char *path2, const char *old, const char *old_path, char **out,
                      FILE *report, char *err, size_t errlen)
{
    struct bugorigcmp_obs a = { 0 }, b = { 0 }, o = { 0 }, back = { 0 };
    struct libscmp_file fa, fb;
    char *ta = NULL, *tb = NULL, *acc = NULL, *lrep = NULL;
    size_t la = 0, lb = 0, lrl = 0, i;
    FILE *rep = NULL;
    struct bstr t = { 0 };
    int r;

    *out = NULL;
    memset(&fa, 0, sizeof fa);
    memset(&fb, 0, sizeof fb);
    if ((r = bugorigcmp_parse(run1, path1, BUGORIGCMP_RUN, &a, err, errlen))
        || (r = bugorigcmp_parse(run2, path2, BUGORIGCMP_RUN, &b, err, errlen))
        || (old && (r = bugorigcmp_parse(old, old_path, BUGORIGCMP_ORIG, &o, err, errlen))))
        goto done;
    /* the wildcards of the committed file: kept where both runs match them */
    if (old && (wild_mismatch(id, &o, &a, 1, report) | wild_mismatch(id, &o, &b, 2, report))) {
        fprintf(report, "%s: refused: the recorded original is not rewritten\n", id);
        r = 1;
        goto done;
    }
    /* both runs the same (the cases of the shared model) */
    r = -1;
    if (!(ta = cases(id, &a, old ? &o : NULL, &la)) || !(tb = cases(id, &b, old ? &o : NULL, &lb)))
        goto oom;
    if ((r = libscmp_parse(&fa, ta, la, path1, LIBSCMP_RUN, err, errlen))
        || (r = libscmp_parse(&fb, tb, lb, path2, LIBSCMP_RUN, err, errlen))) {
        snprintf(err, errlen, "%s: the test's ID is not a program name of the cases "
                 "(A-Z, a-z, 0-9, _ . -)", id);
        r = r == -1 ? -1 : -2;
        goto done;
    }
    if (!(rep = open_memstream(&lrep, &lrl)))
        goto oom;
    r = libscmp_accept(&fa, &fb, NULL, &acc, rep);
    if (fclose(rep)) {
        rep = NULL;
        goto oom;
    }
    rep = NULL;
    if (r < 0)
        goto oom;
    if (r) {
        char x[120], y[120];

        fwrite(lrep, 1, lrl, report);
        for (i = 0; i < a.n; i++) {
            const struct bugorigcmp_sec *s = bugorigcmp_get(&b, a.sec[i].name);
            const struct bugorigcmp_sec *k = old ? bugorigcmp_get(&o, a.sec[i].name) : NULL;
            unsigned no;

            if (!s || (k && k->wild))
                continue;
            if ((no = literal_diff(a.sec[i].text, s->text, x, sizeof x, y, sizeof y)))
                fprintf(report, "%s: [%s] line %u: run 1 \"%s\", run 2 \"%s\"\n", id,
                        a.sec[i].name, no, x, y);
        }
        r = 1;
        goto done;
    }
    /* a line that would read as a wildcard */
    if (wild_line(id, &a, old ? &o : NULL, report)) {
        fprintf(report, "%s: refused: the recorded original is not rewritten\n", id);
        r = 1;
        goto done;
    }
    /* the file: the note of run 1, its sections, kept ones as they were */
    if (a.note)
        bstr_add(&t, "# ", 2), bstr_add(&t, a.note, strlen(a.note)), bstr_add(&t, "\n", 1);
    for (i = 0; i < a.n; i++) {
        const struct bugorigcmp_sec *k = old ? bugorigcmp_get(&o, a.sec[i].name) : NULL;
        const char *s = k && k->wild ? k->raw : a.sec[i].text;

        bstr_add(&t, "[", 1);
        bstr_add(&t, a.sec[i].name, strlen(a.sec[i].name));
        bstr_add(&t, "]\n", 2);
        bstr_add(&t, s, strlen(s));
    }
    if (!(*out = bstr_take(&t)))
        goto oom;
    /* it must read back as the observation */
    r = bugorigcmp_parse(*out, "the new recorded original", BUGORIGCMP_ORIG, &back, err, errlen);
    if (r == -1)
        goto oom;
    if (!r && back.n == a.n)
        for (i = 0; i < a.n && !r; i++) {
            const struct bugorigcmp_sec *k = old ? bugorigcmp_get(&o, a.sec[i].name) : NULL;

            if (!(k && k->wild) && strcmp(text_of(&back, a.sec[i].name), a.sec[i].text))
                r = 1;
        }
    if (r || back.n != a.n) {
        fprintf(report, "%s: refused: the observation does not read back as it is (%s)\n", id,
                r == -2 ? err : "a line that looks like a [section] header or a \"# varies:\" line");
        free(*out);
        *out = NULL;
        r = 1;
    }
    goto done;
oom:
    snprintf(err, errlen, "out of memory");
    r = -1;
done:
    if (r) {
        free(*out);
        *out = NULL;
    }
    free(t.p);
    free(ta);
    free(tb);
    free(acc);
    free(lrep);
    libscmp_free(&fa);
    libscmp_free(&fb);
    bugorigcmp_free(&a);
    bugorigcmp_free(&b);
    bugorigcmp_free(&o);
    bugorigcmp_free(&back);
    return r;
}
