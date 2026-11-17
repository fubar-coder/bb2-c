/*
 * libscmp_sort.c - the order in which the writers of the expected data
 * (libs-accept, libs-update, libs-sort) put the cases (libscmp.h,
 * libscmp_sorted).
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "libscmp.h"
#include "libscmp_int.h"

/* the sort key of a case: the parts of its key as offsets into the text */
struct sortkey {
    const char *key;
    uint32_t part[3], partlen[3];       /* ROUTINE, CHECK, VECTOR (escaped) */
    uint32_t num;                       /* OCC, or N of BLOCK:N */
    uint32_t issue;
    uint8_t block, qual;
    uint32_t idx;                       /* the case */
};

static uint32_t number(const char *p, const char *end)
{
    uint32_t v = 0;

    for (; p < end && *p >= '0' && *p <= '9'; p++)
        v = 10 * v + (uint32_t)(*p - '0');
    return v;
}

/* the sort key of case i of f (a parsed file: the key is well-formed) */
static void sortkey_of(const struct libscmp_file *f, size_t i, struct sortkey *k)
{
    const struct libscmp_case *c = &f->cases[i];
    const char *p = f->text + c->key, *end = p + c->keylen, *q = p;
    int n;

    memset(k, 0, sizeof *k);
    k->key = p;
    k->qual = c->qual;
    k->issue = c->issue;
    k->idx = (uint32_t)i;
    if (c->keylen > 6 && !memcmp(p, "BLOCK:", 6) && p[6] >= '0' && p[6] <= '9') {
        k->block = 1;
        k->num = number(p + 6, end);
        return;
    }
    for (n = 0; n < 3; n++) {
        k->part[n] = (uint32_t)(q - p);
        while (*q != '|')               /* a part ends at its unescaped '|' */
            q += *q == '\\' ? 2 : 1;
        k->partlen[n] = (uint32_t)(q - p) - k->part[n];
        q++;
    }
    k->num = number(q, end);
}

static int cmp_text(const char *a, uint32_t na, const char *b, uint32_t nb)
{
    int r = memcmp(a, b, na < nb ? na : nb);

    if (r)
        return r;
    return na < nb ? -1 : na > nb;
}

static int cmp_sortkey(const void *x, const void *y)
{
    const struct sortkey *a = x, *b = y;
    int n, r;

    if (a->block != b->block)
        return a->block ? 1 : -1;       /* the blocks after the keyed cases */
    for (n = 0; !a->block && n < 3; n++)
        if ((r = cmp_text(a->key + a->part[n], a->partlen[n], b->key + b->part[n],
                          b->partlen[n])))
            return r;
    if (a->num != b->num)
        return a->num < b->num ? -1 : 1;
    /* one key: plain, [orig] before [fixed #N], or [orig #N] alone (two
       [fixed] cases, [orig] with [orig #N] and [orig #N] with [fixed #N]
       make a file malformed; the issue keeps the order total) */
    if (a->qual != b->qual)
        return a->qual < b->qual ? -1 : 1;
    if (a->issue != b->issue)
        return a->issue < b->issue ? -1 : 1;
    return a->idx < b->idx ? -1 : a->idx > b->idx;
}

int libscmp_sorted(const struct libscmp_file *f, char **out)
{
    struct sortkey *k = malloc((f->ncases + 1) * sizeof *k);
    char *text = NULL;
    size_t i, len;
    FILE *o;

    *out = NULL;
    if (!k)
        return -1;
    for (i = 0; i < f->ncases; i++)
        sortkey_of(f, i, &k[i]);
    qsort(k, f->ncases, sizeof *k, cmp_sortkey);
    if (!(o = open_memstream(&text, &len))) {
        free(k);
        return -1;
    }
    fprintf(o, "# capture %s 1\n", f->name);
    for (i = 0; i < f->ncases; i++) {
        const struct libscmp_case *c = &f->cases[k[i].idx];

        char q[32];

        fwrite(f->text + c->key, 1, c->keylen, o);
        fputs(libscmp_qual_text(c, q, sizeof q), o);
        fputc('\t', o);
        fwrite(f->text + c->val, 1, c->vallen, o);
        fputc('\n', o);
    }
    free(k);
    if (fclose(o)) {
        free(text);
        return -1;
    }
    *out = text;
    return 0;
}

int libscmp_sort_text(const char *text, size_t len, const char *path, char **out, char *err,
                      size_t errlen)
{
    struct libscmp_file f;
    int r;

    *out = NULL;
    r = libscmp_parse(&f, text, len, path, LIBSCMP_EXPECTED, err, errlen);
    if (!r && (r = libscmp_sorted(&f, out)))
        snprintf(err, errlen, "out of memory");
    libscmp_free(&f);
    return r;
}

int libscmp_sort_dirs(const char *exp_dir, const char *const *only, size_t nonly, FILE *out,
                      FILE *err)
{
    struct libscmp_names names = { 0 };
    size_t i, nwritten = 0, nfiles = 0;
    int status = 0;

    if (libscmp_program_names(&names, exp_dir, NULL, only, nonly, err)) {
        libscmp_names_free(&names);
        return 2;
    }
    for (i = 0; i < names.n; i++) {
        struct libscmp_file f;
        char *path = libscmp_path(exp_dir, names.v[i]), *text = NULL;

        memset(&f, 0, sizeof f);
        if (!path) {
            fprintf(err, "out of memory\n");
            status = 2;
            break;
        }
        if (libscmp_load_named(&f, exp_dir, names.v[i], LIBSCMP_EXPECTED, err))
            status = 2;
        else if (libscmp_sorted(&f, &text)) {
            fprintf(err, "out of memory\n");
            status = 2;
        } else {
            size_t n = strlen(text);

            nfiles++;
            if (n != f.len || memcmp(text, f.text, n)) {
                FILE *w = fopen(path, "wb");
                int ok = w && fwrite(text, 1, n, w) == n;

                if (w && fclose(w))
                    ok = 0;
                if (!ok) {
                    fprintf(err, "%s: cannot write it (%s)\n", path, strerror(errno));
                    status = 2;
                } else {
                    fprintf(out, "written: %s\n", path);
                    nwritten++;
                }
            }
        }
        libscmp_free(&f);
        free(text);
        free(path);
    }
    fprintf(out, "%zu files written, %zu unchanged\n", nwritten, nfiles - nwritten);
    libscmp_names_free(&names);
    return status;
}
