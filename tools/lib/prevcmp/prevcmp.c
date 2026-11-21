/*
 * prevcmp.c - the library files of two build directories byte by byte
 * (prevcmp.h).
 */
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "prevcmp.h"

struct list {
    char **v;
    size_t n, cap;
};

static int list_add(struct list *l, const char *s)
{
    if (l->n == l->cap) {
        size_t cap = l->cap ? 2 * l->cap : 64;
        char **v = realloc(l->v, cap * sizeof *v);

        if (!v)
            return -1;
        l->v = v;
        l->cap = cap;
    }
    if (!(l->v[l->n] = strdup(s)))
        return -1;
    l->n++;
    return 0;
}

static void list_free(struct list *l)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        free(l->v[i]);
    free(l->v);
    memset(l, 0, sizeof *l);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static int ends_with(const char *s, const char *tail)
{
    size_t l = strlen(s), t = strlen(tail);

    return l >= t && !strcmp(s + l - t, tail);
}

/* every *.obj below root/rel (not in a directory CMakeFiles or a run directory run-*, the work
   directory of a test run, which holds copies of libraries; at any depth, so a library
   directory of that name would drop out) into l, as
   paths relative to root; 0 = ok, -1 = out of memory or a path too long;
   a directory that is not there is empty */
static int walk(const char *root, const char *rel, struct list *l)
{
    char path[4096];
    DIR *d;
    struct dirent *de;
    int r = 0;

    if (snprintf(path, sizeof path, "%s/%s", root, rel) >= (int)sizeof path)
        return -1;
    if (!(d = opendir(path)))
        return 0;
    while (!r && (de = readdir(d)) != NULL) {
        char sub[4096];
        struct stat st;

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")
            || !strcmp(de->d_name, "CMakeFiles") || !strncmp(de->d_name, "run-", 4))
            continue;
        if (snprintf(sub, sizeof sub, "%s/%s", rel, de->d_name) >= (int)sizeof sub
            || snprintf(path, sizeof path, "%s/%s", root, sub) >= (int)sizeof path) {
            r = -1;
            break;
        }
        if (stat(path, &st))
            continue;
        if (S_ISDIR(st.st_mode))
            r = walk(root, sub, l);
        else if (S_ISREG(st.st_mode) && ends_with(de->d_name, ".obj"))
            r = list_add(l, sub);
    }
    closedir(d);
    return r;
}

/* the library files of a build directory, sorted */
static int collect(const char *root, struct list *l)
{
    static const char *const joined[] = { "blitz2/acidlibs", "blitz2/deflibs" };
    size_t i;
    char path[4096];

    for (i = 0; i < 2; i++) {
        struct stat st;

        if (snprintf(path, sizeof path, "%s/%s", root, joined[i]) < (int)sizeof path
            && !stat(path, &st) && S_ISREG(st.st_mode) && list_add(l, joined[i]))
            return -1;
    }
    if (walk(root, "libs", l) || walk(root, "variants", l))
        return -1;
    qsort(l->v, l->n, sizeof *l->v, cmp_str);
    return 0;
}

static uint8_t *read_file(const char *root, const char *rel, size_t *len)
{
    char path[4096];
    FILE *f;
    uint8_t *p = NULL;
    long n;

    if (snprintf(path, sizeof path, "%s/%s", root, rel) >= (int)sizeof path)
        return NULL;
    if (!(f = fopen(path, "rb")))
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (p = malloc((size_t)n + 1)) && fread(p, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(p);
        p = NULL;
    }
    fclose(f);
    return p;
}

static int in_list(const struct list *l, const char *s)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        if (!strcmp(l->v[i], s))
            return 1;
    return 0;
}

int prevcmp(const char *cur, const char *prev, const struct prevcmp_options *o, FILE *out,
            char *err, size_t errlen)
{
    struct list pf = { 0 }, cf = { 0 };
    char *used = NULL;
    size_t i, j, differ = 0, bad = 0, added = 0;
    int rc = 2;

    if (collect(prev, &pf) || collect(cur, &cf)) {
        snprintf(err, errlen, "out of memory or a path too long");
        goto out;
    }
    if (!pf.n) {
        snprintf(err, errlen, "%s: no library files (blitz2/acidlibs, libs/, variants/): "
                              "not a build directory of the previous commit", prev);
        goto out;
    }
    if (!(used = calloc(o->nintended ? o->nintended : 1, 1))) {
        snprintf(err, errlen, "out of memory");
        goto out;
    }
    for (i = 0; i < pf.n; i++) {
        size_t pl = 0, cl = 0;
        uint8_t *pd = read_file(prev, pf.v[i], &pl);
        uint8_t *cd = in_list(&cf, pf.v[i]) ? read_file(cur, pf.v[i], &cl) : NULL;
        int missing = !cd, intended = 0;

        if (!pd) {
            snprintf(err, errlen, "%s/%s: cannot read it", prev, pf.v[i]);
            free(cd);
            goto out;
        }
        if (missing || pl != cl || memcmp(pd, cd, pl)) {
            differ++;
            for (j = 0; j < o->nintended; j++)
                if (!strcmp(o->intended[j], pf.v[i])) {
                    used[j] = 1;
                    intended = 1;
                }
            intended |= o->all;
            fprintf(out, "%s %s%s\n", missing ? "MISSING" : "DIFFERS", pf.v[i],
                    intended ? " (intended)" : "");
            if (!intended)
                bad++;
        }
        free(pd);
        free(cd);
    }
    for (i = 0; i < cf.n; i++)
        if (!in_list(&pf, cf.v[i]))
            added++;
    for (j = 0; j < o->nintended; j++)
        if (!used[j]) {
            fprintf(out, "STALE %s: intended, but the file is the same as in the previous "
                         "build (or in neither)\n", o->intended[j]);
            bad++;
        }
    fprintf(out, "prevcmp: %zu library files, %zu differ, %zu not intended, %zu new\n", pf.n,
            differ, bad, added);
    rc = bad ? 1 : 0;
out:
    free(used);
    list_free(&pf);
    list_free(&cf);
    return rc;
}

void prevcmp_mask_traps(uint8_t *data, size_t len)
{
    size_t i;
    int pass;

    /* first all trap #1, then all trap #4, each search going on behind the zeroed block */
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i + 10 <= len;) {
            if (data[i] == 0x4E && data[i + 1] == (pass ? 0x44 : 0x41)) {
                memset(data + i + 2, 0, 8);
                i += 10;
            } else
                i++;
        }
}

int prevcmp_exe_same(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen, size_t *first)
{
    uint8_t *x = malloc(alen ? alen : 1), *y = malloc(blen ? blen : 1);
    size_t i, n = alen < blen ? alen : blen;
    int same = 0;

    if (!x || !y) {
        free(x);
        free(y);
        if (first)
            *first = 0;
        return 0;
    }
    memcpy(x, a, alen);
    memcpy(y, b, blen);
    prevcmp_mask_traps(x, alen);
    prevcmp_mask_traps(y, blen);
    for (i = 0; i < n && x[i] == y[i]; i++)
        ;
    same = i == n && alen == blen;
    if (!same && first)
        *first = i;
    free(x);
    free(y);
    return same;
}
