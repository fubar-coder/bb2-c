/*
 * covutil.c - helpers of covmap and covsel (covutil.h).
 */
#include "covutil.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t hash(const char *s, size_t n)
{
    uint32_t h = 2166136261u;

    while (n--)
        h = (h ^ (unsigned char)*s++) * 16777619u;
    return h;
}

void strtab_free(struct strtab *t)
{
    size_t i;

    for (i = 0; i < t->n; i++)
        free(t->str[i]);
    free(t->str);
    free(t->slot);
    memset(t, 0, sizeof *t);
}

static int find_slot(const struct strtab *t, const char *s, size_t n, size_t *slot)
{
    size_t i = hash(s, n) & (t->nslot - 1);

    while (t->slot[i] >= 0) {
        const char *x = t->str[t->slot[i]];

        if (!strncmp(x, s, n) && !x[n]) {
            *slot = i;
            return t->slot[i];
        }
        i = (i + 1) & (t->nslot - 1);
    }
    *slot = i;
    return -1;
}

static int rehash(struct strtab *t)
{
    size_t nslot = t->nslot ? 2 * t->nslot : 64, i, s;
    int *slot = malloc(nslot * sizeof *slot);

    if (!slot)
        return 0;
    free(t->slot);
    t->slot = slot;
    t->nslot = nslot;
    for (i = 0; i < nslot; i++)
        slot[i] = -1;
    for (i = 0; i < t->n; i++) {
        find_slot(t, t->str[i], strlen(t->str[i]), &s);
        slot[s] = (int)i;
    }
    return 1;
}

int strtab_addn(struct strtab *t, const char *s, size_t n)
{
    size_t slot;
    int id;
    char *copy;

    if (2 * (t->n + 1) > t->nslot && !rehash(t))
        return -1;
    if ((id = find_slot(t, s, n, &slot)) >= 0)
        return id;
    if (t->n == t->cap) {
        size_t cap = t->cap ? 2 * t->cap : 64;
        char **v = realloc(t->str, cap * sizeof *v);

        if (!v)
            return -1;
        t->str = v;
        t->cap = cap;
    }
    if (!(copy = malloc(n + 1)))
        return -1;
    memcpy(copy, s, n);
    copy[n] = 0;
    t->str[t->n] = copy;
    t->slot[slot] = (int)t->n;
    return (int)t->n++;
}

int strtab_add(struct strtab *t, const char *s)
{
    return strtab_addn(t, s, strlen(s));
}

int strtab_find(const struct strtab *t, const char *s)
{
    size_t slot;

    if (!t->nslot)
        return -1;
    return find_slot(t, s, strlen(s), &slot);
}

/* normalise in place: "//", "/./", "x/.." removed; returns 0 if a ".."
   remains at the start of a relative path */
static int normalise(char *p)
{
    char *out = p, *in = p;
    int absolute = *p == '/';
    size_t keep = absolute;    /* bytes of out that ".." must not remove */

    if (absolute)
        in++, out++;
    while (*in) {
        char *end = strchr(in, '/');
        size_t n = end ? (size_t)(end - in) : strlen(in);
        char next = in[n];     /* before the component is moved over it */

        if (n == 0 || (n == 1 && in[0] == '.')) {
            /* nothing */
        } else if (n == 2 && in[0] == '.' && in[1] == '.') {
            if ((size_t)(out - p) > keep) {
                out--;                          /* the '/' after the last component */
                while ((size_t)(out - p) > keep && out[-1] != '/')
                    out--;
            } else if (absolute) {
                /* "/.." is "/" */
            } else {
                return 0;
            }
        } else {
            memmove(out, in, n);
            out += n;
            *out++ = '/';
        }
        if (!next)
            break;
        in += n + 1;
    }
    if ((size_t)(out - p) > keep && out[-1] == '/')
        out--;
    *out = 0;
    return 1;
}

char *cov_relpath(const char *path, const char *root)
{
    char *p = strdup(path), *r;
    size_t n;

    if (!p)
        return NULL;
    if (!normalise(p)) {
        free(p);
        return NULL;
    }
    if (p[0] != '/')
        return p;
    if (!root || !(r = strdup(root))) {
        free(p);
        return NULL;
    }
    normalise(r);
    n = strlen(r);
    if (n > 1 && !strncmp(p, r, n) && p[n] == '/') {
        memmove(p, p + n + 1, strlen(p + n + 1) + 1);
        free(r);
        return p;
    }
    free(r);
    free(p);
    return NULL;
}

char *cov_join(const char *dir, const char *path)
{
    size_t a, b;
    char *s;

    if (path[0] == '/' || !dir || !*dir)
        return strdup(path);
    a = strlen(dir);
    b = strlen(path);
    if (!(s = malloc(a + b + 2)))
        return NULL;
    memcpy(s, dir, a);
    s[a] = '/';
    memcpy(s + a + 1, path, b + 1);
    return s;
}

const char *cov_next_line(const char *text, size_t len, size_t *pos, size_t *linelen)
{
    const char *line, *nl;
    size_t n;

    if (*pos >= len)
        return NULL;
    line = text + *pos;
    nl = memchr(line, '\n', len - *pos);
    n = nl ? (size_t)(nl - line) : len - *pos;
    *pos += n + (nl != NULL);
    if (n && line[n - 1] == '\r')
        n--;
    *linelen = n;
    return line;
}

void cov_deps_free(struct cov_deps *d)
{
    size_t i, j;

    for (i = 0; i < d->n; i++) {
        free(d->v[i].object);
        for (j = 0; j < d->v[i].ndeps; j++)
            free(d->v[i].deps[j]);
        free(d->v[i].deps);
    }
    free(d->v);
    memset(d, 0, sizeof *d);
}

static char *dupn(const char *s, size_t n)
{
    char *c = malloc(n + 1);

    if (c) {
        memcpy(c, s, n);
        c[n] = 0;
    }
    return c;
}

int cov_deps_parse(struct cov_deps *d, const char *text, char *err, size_t errsz)
{
    size_t pos = 0, n, lineno = 0;
    size_t len = strlen(text);
    const char *line;
    struct cov_dep *cur = NULL;

    while ((line = cov_next_line(text, len, &pos, &n))) {
        const char *mark;

        lineno++;
        if (!n) {
            cur = NULL;
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            char **v;

            if (!cur) {
                snprintf(err, errsz, "line %zu: a dependency without an object", lineno);
                return -1;
            }
            while (n && (*line == ' ' || *line == '\t'))
                line++, n--;
            if (!(v = realloc(cur->deps, (cur->ndeps + 1) * sizeof *v))
                || !(cur->deps = v, v[cur->ndeps] = dupn(line, n))) {
                snprintf(err, errsz, "out of memory");
                return -1;
            }
            cur->ndeps++;
            continue;
        }
        /* "OBJECT: #deps N, deps mtime M (VALID)" */
        mark = NULL;
        for (size_t i = 0; i + 8 <= n; i++)
            if (!memcmp(line + i, ": #deps ", 8)) {
                mark = line + i;
                break;
            }
        if (!mark) {
            snprintf(err, errsz, "line %zu: not a line of ninja -t deps: %.*s", lineno, (int)n, line);
            return -1;
        }
        if (d->n == d->cap) {
            size_t cap = d->cap ? 2 * d->cap : 64;
            struct cov_dep *v = realloc(d->v, cap * sizeof *v);

            if (!v) {
                snprintf(err, errsz, "out of memory");
                return -1;
            }
            d->v = v;
            d->cap = cap;
        }
        cur = &d->v[d->n];
        memset(cur, 0, sizeof *cur);
        if (!(cur->object = dupn(line, (size_t)(mark - line)))) {
            snprintf(err, errsz, "out of memory");
            return -1;
        }
        d->n++;
    }
    return 0;
}

static int is_source(const char *p)
{
    static const char *const ext[] = { ".c", ".cc", ".cpp", ".cxx", ".s", ".S", ".asm" };
    const char *dot = strrchr(p, '.');
    size_t i;

    if (!dot || strchr(dot, '/'))
        return 0;
    for (i = 0; i < sizeof ext / sizeof *ext; i++)
        if (!strcmp(dot, ext[i]))
            return 1;
    return 0;
}

const char *cov_dep_source(const struct cov_dep *dep)
{
    size_t i;

    for (i = 0; i < dep->ndeps; i++)
        if (is_source(dep->deps[i]))
            return dep->deps[i];
    return NULL;
}

const struct cov_dep *cov_deps_find(const struct cov_deps *d, const char *object)
{
    size_t i;

    for (i = 0; i < d->n; i++)
        if (!strcmp(d->v[i].object, object))
            return &d->v[i];
    return NULL;
}

char *cov_object_source(const char *object)
{
    const char *cm = strstr(object, "CMakeFiles/"), *rest, *ext;
    size_t dirlen, n;
    char *out, *o;

    if (!cm || (cm != object && cm[-1] != '/'))
        return NULL;
    rest = strchr(cm + strlen("CMakeFiles/"), '/');
    if (!rest || rest - cm < (long)strlen("CMakeFiles/") + 4 || memcmp(rest - 4, ".dir", 4))
        return NULL;
    rest++;
    ext = strrchr(rest, '.');
    if (!ext || strchr(ext, '/') || (strcmp(ext, ".o") && strcmp(ext, ".obj")))
        return NULL;
    dirlen = (size_t)(cm - object);
    n = (size_t)(ext - rest);
    if (!(out = malloc(dirlen + 2 * n + 1)))
        return NULL;
    memcpy(out, object, dirlen);
    o = out + dirlen;
    while (n) {
        const char *slash = memchr(rest, '/', n);
        size_t c = slash ? (size_t)(slash - rest) : n;

        if (c == 2 && rest[0] == '_' && rest[1] == '_') {
            *o++ = '.';
            *o++ = '.';
        } else {
            memcpy(o, rest, c);
            o += c;
        }
        rest += c;
        n -= c;
        if (n) {
            *o++ = '/';
            rest++;
            n--;
        }
    }
    *o = 0;
    return out;
}
