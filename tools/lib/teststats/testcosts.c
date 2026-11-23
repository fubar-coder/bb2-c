/*
 * testcosts.c - the measured COST of the tests (testcosts.h).
 */
#define _POSIX_C_SOURCE 200809L /* strdup, strndup */
#include "testcosts.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void tcosts_free(struct tcosts *c)
{
    for (size_t i = 0; i < c->n; i++)
        free(c->v[i].name);
    free(c->v);
    memset(c, 0, sizeof *c);
}

static int name_char(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'
           || ch == '.' || ch == '-';
}

static int by_name(const void *a, const void *b)
{
    return strcmp(((const struct tcost *)a)->name, ((const struct tcost *)b)->name);
}

static int add(struct tcosts *c, const char *name, size_t nlen, unsigned cost, unsigned line)
{
    struct tcost *v = realloc(c->v, (c->n + 1) * sizeof *v);

    if (!v)
        return -2;
    c->v = v;
    if (!(v[c->n].name = strndup(name, nlen)))
        return -2;
    v[c->n].cost = cost;
    v[c->n].line = line;
    c->n++;
    return 0;
}

int tcosts_parse(const char *file, const char *text, int sorted, struct tcosts *c, char *err, size_t errlen)
{
    const char *p = text;
    unsigned line = 0;
    int r = 0;

    memset(c, 0, sizeof *c);
    while (*p && !r) {
        const char *e = strchr(p, '\n'), *q = p, *name;
        size_t len = e ? (size_t)(e - p) : strlen(p), nlen;
        unsigned long cost;
        char *end;

        line++;
        if (!len || *p == '#') {
            p += len + (e != NULL);
            continue;
        }
        for (name = q; q < p + len && name_char(*q); q++)
            ;
        nlen = (size_t)(q - name);
        errno = 0;
        if (!nlen || q >= p + len || *q != ' ' || q[1] < '0' || q[1] > '9'
            || (cost = strtoul(q + 1, &end, 10), errno) || end != p + len || cost < 1 || cost > 9999999) {
            snprintf(err, errlen, "%s:%u: a line is \"NAME COST\" (COST seconds, 1 to 9999999): %.*s", file, line,
                     (int)(len > 80 ? 80 : len), p);
            r = -1;
            break;
        }
        if (c->n && sorted) {
            int d = strncmp(c->v[c->n - 1].name, name, nlen);

            if (d == 0 && strlen(c->v[c->n - 1].name) == nlen) {
                snprintf(err, errlen, "%s:%u: %.*s a second time (line %u)", file, line, (int)nlen, name,
                         c->v[c->n - 1].line);
                r = -1;
                break;
            }
            if (d > 0 || (d == 0 && strlen(c->v[c->n - 1].name) > nlen)) {
                snprintf(err, errlen, "%s:%u: %.*s after %s: the file is sorted by name", file, line, (int)nlen,
                         name, c->v[c->n - 1].name);
                r = -1;
                break;
            }
        }
        r = add(c, name, nlen, (unsigned)cost, line);
        p += len + (e != NULL);
    }
    if (!r && !sorted) {
        qsort(c->v, c->n, sizeof *c->v, by_name);
        for (size_t i = 1; i < c->n; i++)
            if (!strcmp(c->v[i - 1].name, c->v[i].name)) {
                snprintf(err, errlen, "%s: %s twice", file, c->v[i].name);
                r = -1;
            }
    }
    if (r == -2)
        snprintf(err, errlen, "%s: out of memory", file);
    if (r)
        tcosts_free(c);
    return r;
}

int tcosts_load(const char *file, struct tcosts *c, char *err, size_t errlen)
{
    FILE *f = fopen(file, "rb");
    char *text = NULL;
    size_t n = 0, cap = 0, got;
    int r;

    memset(c, 0, sizeof *c);
    if (!f) {
        snprintf(err, errlen, "%s: cannot read: %s", file, strerror(errno));
        return -1;
    }
    do {
        if (cap - n < 4096) {
            char *t = realloc(text, (cap += 65536) + 1);

            if (!t) {
                free(text);
                fclose(f);
                snprintf(err, errlen, "%s: out of memory", file);
                return -2;
            }
            text = t;
        }
        got = fread(text + n, 1, cap - n, f);
        n += got;
    } while (got);
    fclose(f);
    text[n] = 0;
    r = tcosts_parse(file, text, 1, c, err, errlen);
    free(text);
    return r;
}

const struct tcost *tcosts_find(const struct tcosts *c, const char *name)
{
    struct tcost key = { (char *)name, 0, 0 };

    return c->n ? bsearch(&key, c->v, c->n, sizeof *c->v, by_name) : NULL;
}

int tcosts_merge(struct tcosts *c, const struct tcosts *from)
{
    for (size_t i = 0; i < from->n; i++) {
        struct tcost *e = (struct tcost *)tcosts_find(c, from->v[i].name);

        if (e) {
            e->cost = from->v[i].cost;
            continue;
        }
        if (add(c, from->v[i].name, strlen(from->v[i].name), from->v[i].cost, 0))
            return -1;
        qsort(c->v, c->n, sizeof *c->v, by_name);
    }
    return 0;
}

int tcosts_write(FILE *f, const struct tcosts *c)
{
    if (fputs("# The measured COST of the tests (seconds): the median over the runs of the sum of their\n"
              "# emulator sessions started warm, without the wait for a slot (testinfo costs; tests/README.md,\n"
              "# \"The measured COST\"). One line \"NAME COST\" per test, sorted by name.\n",
              f) < 0)
        return -1;
    for (size_t i = 0; i < c->n; i++)
        if (fprintf(f, "%s %u\n", c->v[i].name, c->v[i].cost) < 0)
            return -1;
    return ferror(f) ? -1 : 0;
}
