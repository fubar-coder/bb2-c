/*
 * cdcache.c - the cache of a corpusrun cd pass, see cdcache.h.
 */
#define _XOPEN_SOURCE 700       /* nftw, strdup */
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdcache.h"
#include "sha256.h"
#include "util.h"

#define MAGIC "cdrun-cache 1"

/* one entry: its name (as given, or relative to its directory), its size
   and its contents */
static int add_file(struct sha256 *s, const char *name, const char *path)
{
    size_t len;
    uint8_t *d = read_file(path, &len);
    char size[32];

    if (!d)
        return -1;
    snprintf(size, sizeof size, "%zu", len);
    sha256_update(s, name, strlen(name) + 1);
    sha256_update(s, size, strlen(size) + 1);
    sha256_update(s, d, len);
    free(d);
    return 0;
}

/* the files below one directory (nftw has no user pointer) */
static char **found;
static size_t nfound;
static int found_bad;

static int collect(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    char **n;

    (void)st;
    (void)ftw;
    if (type == FTW_DNR || type == FTW_NS || type == FTW_SLN) {
        found_bad = 1;
        return 1;
    }
    if (type != FTW_F)
        return 0;
    if (!(n = realloc(found, (nfound + 1) * sizeof *found)) || !(n[nfound] = strdup(path))) {
        if (n)
            found = n;
        found_bad = 1;
        return 1;
    }
    found = n;
    nfound++;
    return 0;
}

static int by_name(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static int add_dir(struct sha256 *s, const char *dir)
{
    size_t i, dlen = strlen(dir);
    int r = 0;

    found = NULL;
    nfound = 0;
    found_bad = 0;
    if (nftw(dir, collect, 16, 0) || found_bad)
        r = -1;
    if (!r)
        qsort(found, nfound, sizeof *found, by_name);
    sha256_update(s, dir, dlen + 1);
    for (i = 0; i < nfound; i++) {
        if (!r)
            r = add_file(s, found[i] + dlen, found[i]);
        free(found[i]);
    }
    free(found);
    found = NULL;
    return r;
}

int cdcache_digest(const char *text, const char *const *files, size_t nfiles,
                   const char *const *dirs, size_t ndirs, char hex[65])
{
    struct sha256 s;
    size_t i;

    sha256_init(&s);
    sha256_update(&s, MAGIC, sizeof MAGIC);
    sha256_update(&s, text, strlen(text) + 1);
    for (i = 0; i < nfiles; i++)
        if (add_file(&s, files[i], files[i]))
            return -1;
    for (i = 0; i < ndirs; i++)
        if (add_dir(&s, dirs[i]))
            return -1;
    sha256_hex(&s, hex);
    return 0;
}

static int hash_of(const char *path, char hex[65])
{
    size_t len;
    uint8_t *d = read_file(path, &len);

    if (!d)
        return -1;
    sha256_hex_of(d, len, hex);
    free(d);
    return 0;
}

/* the record text for the digest and the current outputs (NULL: an
   output is missing) */
static char *record(const char *digest, const char *const *outs, size_t nouts)
{
    struct text t = { 0 };
    size_t i;
    char hex[65];

    text_add(&t, MAGIC "\ninputs %s\n", digest);
    for (i = 0; i < nouts; i++) {
        if (hash_of(outs[i], hex)) {
            free(t.p);
            return NULL;
        }
        text_add(&t, "%s %s\n", hex, outs[i]);
    }
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

int cdcache_hit(const char *path, const char *digest, const char *const *outs, size_t nouts)
{
    size_t len;
    char *have = (char *)read_file(path, &len), *want;
    int hit;

    if (!have)
        return 0;
    want = record(digest, outs, nouts);
    /* the whole text: a record with a 0 byte in it is not equal either */
    hit = want && strlen(want) == len && !memcmp(have, want, len);
    free(want);
    free(have);
    return hit;
}

int cdcache_write(const char *path, const char *digest, const char *const *outs, size_t nouts)
{
    char *t = record(digest, outs, nouts);
    int r = t ? write_file(path, t, strlen(t)) : -1;

    if (r)
        remove(path);
    free(t);
    return r;
}
