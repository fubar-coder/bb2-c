/* siteexamples - see siteexamples.h */
#include "siteexamples.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *err, size_t errlen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

static char *dup_n(const char *s, size_t n)
{
    char *p = malloc(n + 1);

    if (p) {
        memcpy(p, s, n);
        p[n] = 0;
    }
    return p;
}

/* the line at *P (to its newline, or the end): its start and length, and *P moved behind it */
static const char *next_line(const char **p, size_t *len)
{
    const char *s = *p, *e = strchr(s, '\n');

    *len = e ? (size_t)(e - s) : strlen(s);
    *p = e ? e + 1 : s + *len;
    return s;
}

/* a line "```blitzbasic" (indented, with nothing or blank-separated words
   behind it): the indentation, or -1 */
static int opening(const char *s, size_t len)
{
    size_t i = 0;

    while (i < len && s[i] == ' ')
        i++;
    if (len - i < 13 || strncmp(s + i, "```blitzbasic", 13))
        return -1;
    if (len - i > 13 && s[i + 13] != ' ' && s[i + 13] != '\t' && s[i + 13] != '\r')
        return -1;
    return (int)i;
}

static int closing(const char *s, size_t len)
{
    size_t i = 0;

    while (i < len && s[i] == ' ')
        i++;
    if (len - i < 3 || strncmp(s + i, "```", 3))
        return 0;
    for (i += 3; i < len; i++)
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\r')
            return 0;
    return 1;
}

int se_scan_text(struct se_blocks *b, const char *file, const char *text, char *err,
                 size_t errlen)
{
    const char *p = text;
    unsigned line = 0, index = 0;

    while (*p) {
        size_t len;
        const char *s = next_line(&p, &len);
        int indent = opening(s, len);
        unsigned start;
        size_t cap = 256, n = 0;
        char *body;

        line++;
        if (indent < 0)
            continue;
        start = line;
        if (!(body = malloc(cap)))
            return fail(err, errlen, "out of memory");
        for (;;) {
            size_t l2, drop;
            const char *t;

            if (!*p) {
                free(body);
                return fail(err, errlen, "%s: line %u: the block has no closing fence", file,
                            start);
            }
            t = next_line(&p, &l2);
            line++;
            if (closing(t, l2))
                break;
            /* the indentation of the opening fence off (less where the line has less) */
            for (drop = 0; drop < (size_t)indent && drop < l2 && t[drop] == ' '; drop++)
                ;
            t += drop;
            l2 -= drop;
            if (l2 && t[l2 - 1] == '\r')
                l2--;
            if (n + l2 + 2 > cap) {
                char *nb;

                while (n + l2 + 2 > cap)
                    cap *= 2;
                if (!(nb = realloc(body, cap))) {
                    free(body);
                    return fail(err, errlen, "out of memory");
                }
                body = nb;
            }
            memcpy(body + n, t, l2);
            n += l2;
            body[n++] = '\n';
        }
        body[n] = 0;
        if (b->n == b->cap) {
            size_t nc = b->cap ? b->cap * 2 : 16;
            struct se_block *nv = realloc(b->v, nc * sizeof *nv);

            if (!nv) {
                free(body);
                return fail(err, errlen, "out of memory");
            }
            b->v = nv;
            b->cap = nc;
        }
        b->v[b->n].file = dup_n(file, strlen(file));
        b->v[b->n].index = ++index;
        b->v[b->n].line = start;
        b->v[b->n].text = body;
        if (!b->v[b->n].file) {
            free(body);
            return fail(err, errlen, "out of memory");
        }
        b->n++;
    }
    return 0;
}

static int name_ok(const char *s, size_t n)
{
    size_t i;

    if (!n)
        return 0;
    for (i = 0; i < n; i++)
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '-'))
            return 0;
    return 1;
}

int se_manifest_parse(struct se_manifest *m, const char *text, char *err, size_t errlen)
{
    const char *p = text;
    unsigned line = 0;

    while (*p) {
        size_t len, i = 0, ns, ms, fs;
        const char *s = next_line(&p, &len), *name, *mode, *file, *hash;
        struct se_entry e;
        char *end;
        char libs[200];
        unsigned long idx;

        line++;
        while (len && (s[len - 1] == '\r' || s[len - 1] == ' ' || s[len - 1] == '\t'))
            len--;
        while (i < len && (s[i] == ' ' || s[i] == '\t'))
            i++;
        if (i == len || s[i] == '#')
            continue;
        /* NAME MODE FILE#N [REASON] */
        name = s + i;
        for (ns = 0; i + ns < len && s[i + ns] != ' ' && s[i + ns] != '\t'; ns++)
            ;
        i += ns;
        while (i < len && (s[i] == ' ' || s[i] == '\t'))
            i++;
        mode = s + i;
        for (ms = 0; i + ms < len && s[i + ms] != ' ' && s[i + ms] != '\t'; ms++)
            ;
        i += ms;
        while (i < len && (s[i] == ' ' || s[i] == '\t'))
            i++;
        file = s + i;
        for (fs = 0; i + fs < len && s[i + fs] != ' ' && s[i + fs] != '\t'; fs++)
            ;
        i += fs;
        while (i < len && (s[i] == ' ' || s[i] == '\t'))
            i++;
        if (!ms || !fs)
            return fail(err, errlen, "line %u: NAME MODE FILE#N [rwe=LIB...] [REASON] expected", line);
        if (!name_ok(name, ns))
            return fail(err, errlen, "line %u: the name must be lower case letters, digits and -",
                        line);
        memset(&e, 0, sizeof e);
        if (ms == 3 && !strncmp(mode, "run", 3))
            e.mode = SE_RUN;
        else if (ms == 7 && !strncmp(mode, "compile", 7))
            e.mode = SE_COMPILE;
        else
            return fail(err, errlen, "line %u: the mode is run or compile", line);
        for (hash = file + fs; hash > file && hash[-1] != '#'; hash--)
            ;
        if (hash == file || hash[-1] != '#' || hash - 1 == file)
            return fail(err, errlen, "line %u: FILE#N expected", line);
        idx = strtoul(hash, &end, 10);
        if (end != file + fs || idx < 1 || idx > 100000)
            return fail(err, errlen, "line %u: the block number after # is 1 or more", line);
        e.index = (unsigned)idx;
        e.line = line;
        /* rwe=LIB words */
        libs[0] = 0;
        while (len - i >= 4 && !strncmp(s + i, "rwe=", 4)) {
            size_t ls;

            for (ls = 0; i + 4 + ls < len && s[i + 4 + ls] != ' ' && s[i + 4 + ls] != '\t'; ls++)
                ;
            if (!name_ok(s + i + 4, ls) || strlen(libs) + ls + 2 > sizeof libs)
                return fail(err, errlen, "line %u: rwe= needs a library name (lower case letters and digits)",
                            line);
            if (libs[0])
                strcat(libs, ",");
            strncat(libs, s + i + 4, ls);
            i += 4 + ls;
            while (i < len && (s[i] == ' ' || s[i] == '\t'))
                i++;
        }
        if (e.mode == SE_COMPILE && i == len)
            return fail(err, errlen, "line %u: a compile entry needs the reason it is not run",
                        line);
        if (e.mode == SE_RUN && i < len)
            return fail(err, errlen, "line %u: a run entry has no reason", line);
        for (size_t k = 0; k < m->n; k++)
            if (strlen(m->v[k].name) == ns && !strncmp(m->v[k].name, name, ns))
                return fail(err, errlen, "line %u: the name %.*s is used in line %u", line, (int)ns,
                            name, m->v[k].line);
        if (m->n == m->cap) {
            size_t nc = m->cap ? m->cap * 2 : 16;
            struct se_entry *nv = realloc(m->v, nc * sizeof *nv);

            if (!nv)
                return fail(err, errlen, "out of memory");
            m->v = nv;
            m->cap = nc;
        }
        e.name = dup_n(name, ns);
        e.file = dup_n(file, (size_t)(hash - 1 - file));
        e.reason = dup_n(s + i, len - i);
        e.libs = dup_n(libs, strlen(libs));
        if (!e.name || !e.file || !e.reason || !e.libs) {
            free(e.name);
            free(e.file);
            free(e.reason);
            free(e.libs);
            return fail(err, errlen, "out of memory");
        }
        m->v[m->n++] = e;
    }
    return 0;
}

const struct se_block *se_find(const struct se_blocks *b, const struct se_entry *e)
{
    for (size_t i = 0; i < b->n; i++)
        if (b->v[i].index == e->index && !strcmp(b->v[i].file, e->file))
            return &b->v[i];
    return NULL;
}

int se_check(const struct se_manifest *m, const struct se_blocks *b, char *err, size_t errlen)
{
    for (size_t i = 0; i < m->n; i++) {
        const struct se_entry *e = &m->v[i];

        if (!se_find(b, e))
            return fail(err, errlen,
                        "manifest line %u: %s: the site has no block %s#%u (the example changed or "
                        "went away)",
                        e->line, e->name, e->file, e->index);
        for (size_t k = 0; k < i; k++)
            if (m->v[k].index == e->index && !strcmp(m->v[k].file, e->file))
                return fail(err, errlen, "manifest line %u: %s#%u is already listed in line %u",
                            e->line, e->file, e->index, m->v[k].line);
    }
    for (size_t i = 0; i < b->n; i++) {
        size_t k;

        for (k = 0; k < m->n; k++)
            if (m->v[k].index == b->v[i].index && !strcmp(m->v[k].file, b->v[i].file))
                break;
        if (k == m->n)
            return fail(err, errlen,
                        "%s#%u (line %u): the example has no entry in the manifest (run it, or "
                        "compile it only with the reason)",
                        b->v[i].file, b->v[i].index, b->v[i].line);
    }
    return 0;
}

size_t se_count(const struct se_manifest *m, enum se_mode mode)
{
    size_t n = 0;

    for (size_t i = 0; i < m->n; i++)
        n += m->v[i].mode == mode;
    return n;
}

void se_blocks_free(struct se_blocks *b)
{
    for (size_t i = 0; i < b->n; i++) {
        free(b->v[i].file);
        free(b->v[i].text);
    }
    free(b->v);
    memset(b, 0, sizeof *b);
}

void se_manifest_free(struct se_manifest *m)
{
    for (size_t i = 0; i < m->n; i++) {
        free(m->v[i].name);
        free(m->v[i].file);
        free(m->v[i].reason);
        free(m->v[i].libs);
    }
    free(m->v);
    memset(m, 0, sizeof *m);
}
