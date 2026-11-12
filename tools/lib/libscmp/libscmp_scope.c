/*
 * libscmp_scope.c - the fix-scope check (design D12 of the OpenSpec change
 * libs-tests-capture): the cases of the expected data a branch changes
 * that are not the [fixed #N] cases of its issues (libscmp.h).
 */
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "libscmp.h"
#include "libscmp_int.h"

int libscmp_issues_add(struct libscmp_issues *is, uint32_t n)
{
    size_t i;

    for (i = 0; i < is->n; i++)
        if (is->v[i] == n)
            return 0;
    if (is->n == is->cap) {
        size_t cap = is->cap ? 2 * is->cap : 16;
        uint32_t *v = realloc(is->v, cap * sizeof *v);

        if (!v)
            return -1;
        is->v = v;
        is->cap = cap;
    }
    is->v[is->n++] = n;
    return 0;
}

int libscmp_issues_from_log(struct libscmp_issues *is, const char *log)
{
    const char *p = log;

    while (*p) {
        const char *e = strchr(p, '\n'), *q = p;
        unsigned long n = 0;
        int digits = 0;

        if (!e)
            e = p + strlen(p);
        while (q < e && (*q == ' ' || *q == '\t'))
            q++;
        if (e - q > 7 && !memcmp(q, "Fixes #", 7) && q[7] != '0') {
            for (q += 7; q < e && *q >= '0' && *q <= '9' && digits < 9; q++, digits++)
                n = 10 * n + (unsigned long)(*q - '0');
            while (q < e && (*q == ' ' || *q == '\t' || *q == '\r'))
                q++;
            if (digits && q == e && libscmp_issues_add(is, (uint32_t)n))
                return -1;
        }
        p = *e ? e + 1 : e;
    }
    return 0;
}

void libscmp_issues_free(struct libscmp_issues *is)
{
    free(is->v);
    memset(is, 0, sizeof *is);
}

static int named(const struct libscmp_issues *is, uint32_t n)
{
    size_t i;

    for (i = 0; i < is->n; i++)
        if (is->v[i] == n)
            return 1;
    return 0;
}

/* the entry of key k (of case c of f) in g, or NULL */
static const struct libscmp_entry *other_entry(const struct libscmp_file *g,
                                               const struct libscmp_file *f,
                                               const struct libscmp_case *c)
{
    return g ? libscmp_find_h(g, f->text + c->key, c->keylen, f->entries[c->entry].hash) : NULL;
}

/* the case of entry e with the qualifier (and issue) of c, or NULL */
static const struct libscmp_case *same_case(const struct libscmp_file *g,
                                            const struct libscmp_entry *e,
                                            const struct libscmp_case *c)
{
    int32_t i;

    if (!e)
        return NULL;
    i = c->qual == LIBSCMP_PLAIN ? e->plain : c->qual == LIBSCMP_QORIG ? e->orig : e->fixed;
    if (i < 0 || (c->qual == LIBSCMP_QFIXED && g->cases[i].issue != c->issue))
        return NULL;
    return &g->cases[i];
}

/* the entry has a [fixed #N] case of a named issue */
static int fixed_named(const struct libscmp_file *g, const struct libscmp_entry *e,
                       const struct libscmp_issues *is)
{
    return e && e->fixed >= 0 && named(is, g->cases[e->fixed].issue);
}

/* a change of case c of f (added or changed: f is the branch; removed: f
   is base) that belongs to a named issue */
static int own(const struct libscmp_file *f, const struct libscmp_case *c,
               const struct libscmp_file *base, const struct libscmp_file *branch,
               const struct libscmp_issues *is)
{
    const struct libscmp_entry *eb = f == branch ? &branch->entries[c->entry]
                                                 : other_entry(branch, f, c);

    if (c->qual == LIBSCMP_QFIXED)
        return named(is, c->issue);
    if (fixed_named(branch, eb, is))     /* [orig] of the pair, or the case that became it */
        return c->qual == LIBSCMP_QORIG || f == base;
    if (c->qual == LIBSCMP_QORIG && f == base)
        return fixed_named(base, &base->entries[c->entry], is);
    return 0;
}

static const char *qual(const struct libscmp_case *c, char *buf, size_t len)
{
    if (c->qual == LIBSCMP_QORIG)
        return " [orig]";
    if (c->qual == LIBSCMP_QFIXED) {
        snprintf(buf, len, " [fixed #%lu]", (unsigned long)c->issue);
        return buf;
    }
    return "";
}

size_t libscmp_fix_scope(const struct libscmp_file *base, const struct libscmp_file *branch,
                         const struct libscmp_issues *is, FILE *report, size_t *nchanged)
{
    const char *name = branch ? branch->name : base->name;
    size_t i, nrep = 0, nch = 0;
    char q[32];

    for (i = 0; base && i < base->ncases; i++) {
        const struct libscmp_case *c = &base->cases[i];
        const struct libscmp_case *d = same_case(branch, other_entry(branch, base, c), c);

        if (d && d->vallen == c->vallen && !memcmp(branch->text + d->val, base->text + c->val, c->vallen))
            continue;
        nch++;
        if (own(d ? branch : base, d ? d : c, base, branch, is))
            continue;
        nrep++;
        if (d)
            fprintf(report, "%s: %.*s%s: changed (%.*s -> %.*s)\n", name, (int)c->keylen,
                    base->text + c->key, qual(c, q, sizeof q), (int)c->vallen, base->text + c->val,
                    (int)d->vallen, branch->text + d->val);
        else
            fprintf(report, "%s: %.*s%s: removed (%.*s)\n", name, (int)c->keylen, base->text + c->key,
                    qual(c, q, sizeof q), (int)c->vallen, base->text + c->val);
    }
    for (i = 0; branch && i < branch->ncases; i++) {
        const struct libscmp_case *d = &branch->cases[i];

        if (same_case(base, other_entry(base, branch, d), d))
            continue;
        nch++;
        if (own(branch, d, base, branch, is))
            continue;
        nrep++;
        fprintf(report, "%s: %.*s%s: added (%.*s)\n", name, (int)d->keylen, branch->text + d->key,
                qual(d, q, sizeof q), (int)d->vallen, branch->text + d->val);
    }
    if (nchanged)
        *nchanged = nch;
    return nrep;
}

int libscmp_fix_scope_dirs(const char *base_dir, const char *branch_dir,
                           const struct libscmp_issues *is, FILE *out, FILE *err)
{
    struct libscmp_names names = { 0 };
    size_t i, nch = 0, nrep = 0, nprog = 0;
    int status = 0;

    if (libscmp_program_names(&names, base_dir, branch_dir, NULL, 0, err)) {
        libscmp_names_free(&names);
        return 2;
    }
    for (i = 0; i < names.n; i++) {
        const char *name = names.v[i];
        char *pb = libscmp_path(base_dir, name), *pr = libscmp_path(branch_dir, name);
        struct libscmp_file b, r;
        int hb, hr, rb = 0, rr = 0;

        memset(&b, 0, sizeof b);
        memset(&r, 0, sizeof r);
        if (!pb || !pr) {
            fprintf(err, "out of memory\n");
            free(pb);
            free(pr);
            status = 2;
            break;
        }
        hb = libscmp_exists(pb);
        hr = libscmp_exists(pr);
        if (hb)
            rb = libscmp_load_named(&b, base_dir, name, LIBSCMP_EXPECTED, err);
        if (hr)
            rr = libscmp_load_named(&r, branch_dir, name, LIBSCMP_EXPECTED, err);
        if (rb || rr)
            status = 2;
        else {
            size_t n;

            nrep += libscmp_fix_scope(hb ? &b : NULL, hr ? &r : NULL, is, out, &n);
            nch += n;
            nprog += n != 0;
        }
        libscmp_free(&b);
        libscmp_free(&r);
        free(pb);
        free(pr);
    }
    fprintf(out, "fix-scope: %zu cases changed in %zu programs, %zu not of the issues", nch, nprog,
            nrep);
    if (!is->n)
        fprintf(out, " (no \"Fixes #N\" line: every changed case is reported)");
    for (i = 0; i < is->n; i++)
        fprintf(out, "%s#%lu", i ? ", " : " ", (unsigned long)is->v[i]);
    fprintf(out, "\n");
    libscmp_names_free(&names);
    if (!status && nrep)
        status = 1;
    return status;
}
