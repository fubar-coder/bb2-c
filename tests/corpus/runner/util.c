/*
 * util.c - strings and files for the corpus runners, see util.h.
 */
#define _XOPEN_SOURCE 700       /* nftw, strdup */
#include <errno.h>
#include <ftw.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

char *str_printf(const char *fmt, ...)
{
    va_list ap;
    char *s;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1)))
        return NULL;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}


void text_add(struct text *t, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (t->bad)
        return;
    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || (t->n + (size_t)n + 1 > t->cap
                  && !(t->p = realloc(t->p, t->cap = (t->n + (size_t)n + 1) * 2)))) {
        t->bad = 1;
        return;
    }
    va_start(ap, fmt);
    vsnprintf(t->p + t->n, (size_t)n + 1, fmt, ap);
    va_end(ap);
    t->n += (size_t)n;
}

static int rm_entry(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)st;
    (void)ftw;
    return type == FTW_DP ? rmdir(path) : unlink(path);
}

void rm_tree(const char *dir)
{
    nftw(dir, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
}

int mkdir_p(const char *path)
{
    char *p = strdup(path), *s;
    int r = 0;

    if (!p)
        return -1;
    for (s = p + 1; *s && !r; s++)
        if (*s == '/') {
            *s = 0;
            r = mkdir(p, 0755) && errno != EEXIST;
            *s = '/';
        }
    if (!r)
        r = mkdir(p, 0755) && errno != EEXIST;
    free(p);
    return r ? -1 : 0;
}

uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
        *len = (size_t)n;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

int write_file(const char *path, const void *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    int ok = f && fwrite(data, 1, len, f) == len;

    if (f && fclose(f))
        ok = 0;
    return ok ? 0 : -1;
}

int copy_file(const char *from, const char *to)
{
    size_t len;
    uint8_t *d = read_file(from, &len);
    int r = d ? write_file(to, d, len) : -1;

    free(d);
    return r;
}
