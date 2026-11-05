/*
 * covsel.c - the tests a change can affect (covsel.h).
 */
#include "covsel.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

enum { SEL_ALL = 1, SEL_NONE = 2, SEL_COVERAGE = 4, SEL_HEADER = 8 };

struct ids {
    int *v;
    size_t n, cap;
};

struct mapline {
    int prog, file, name;       /* name -1: the object line */
    int first, last;            /* 0: no range */
    struct ids tests;
};

struct rule {
    char *pattern;
    unsigned flags;
    char **labels, **names;
    size_t nlabels, nnames;
    int line;
};

struct test {
    int name, runner;           /* ids in tests */
    char **labels;
    size_t nlabels;
};

struct range {
    long first, count;          /* old lines first..first+count-1; count 0: after first */
};

struct change {
    char *path;
    int whole;                  /* the whole file (new, deleted, binary, renamed) */
    struct range *ranges;
    size_t nranges;
};

struct covsel {
    char *root;
    char *commit;
    struct strtab strs;         /* programs, files, function names */
    struct strtab tests;        /* test names: of the map and of ctest */
    struct mapline *lines;
    size_t nlines;
    char *in_map;               /* by test id: named by the map */
    size_t nin_map;
    struct rule *rules;
    size_t nrules;
    struct test *ctests;
    size_t nctests;
    struct cov_deps deps;
    struct change *changes;
    size_t nchanges;
    int stale;
    /* the result */
    char *selected;             /* by test id */
    int *result;                /* selected test ids, sorted by name */
    size_t nresult;
};

static int error(char *err, size_t errsz, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errsz, fmt, ap);
    va_end(ap);
    return -1;
}

#define OOM() error(err, errsz, "out of memory")

static int ids_add(struct ids *d, int v)
{
    if (d->n == d->cap) {
        size_t cap = d->cap ? 2 * d->cap : 4;
        int *x = realloc(d->v, cap * sizeof *x);

        if (!x)
            return -1;
        d->v = x;
        d->cap = cap;
    }
    d->v[d->n++] = v;
    return 0;
}

static void *grow(void *v, size_t n, size_t size)
{
    /* room for one more, doubling at powers of two */
    if (n && (n & (n - 1)))
        return v;
    return realloc(v, (n ? 2 * n : 1) * size);
}

struct covsel *covsel_new(const char *root)
{
    struct covsel *s = calloc(1, sizeof *s);

    if (s && root && !(s->root = strdup(root))) {
        free(s);
        return NULL;
    }
    return s;
}

static void free_strings(char **v, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        free(v[i]);
    free(v);
}

void covsel_free(struct covsel *s)
{
    size_t i;

    if (!s)
        return;
    for (i = 0; i < s->nlines; i++)
        free(s->lines[i].tests.v);
    free(s->lines);
    for (i = 0; i < s->nrules; i++) {
        free(s->rules[i].pattern);
        free_strings(s->rules[i].labels, s->rules[i].nlabels);
        free_strings(s->rules[i].names, s->rules[i].nnames);
    }
    free(s->rules);
    for (i = 0; i < s->nctests; i++)
        free_strings(s->ctests[i].labels, s->ctests[i].nlabels);
    free(s->ctests);
    for (i = 0; i < s->nchanges; i++) {
        free(s->changes[i].path);
        free(s->changes[i].ranges);
    }
    free(s->changes);
    cov_deps_free(&s->deps);
    strtab_free(&s->strs);
    strtab_free(&s->tests);
    free(s->in_map);
    free(s->selected);
    free(s->result);
    free(s->commit);
    free(s->root);
    free(s);
}

const char *covsel_commit(const struct covsel *s)
{
    return s->commit;
}

void covsel_set_stale(struct covsel *s)
{
    s->stale = 1;
}

/* ---- the map ---- */

static int mark_in_map(struct covsel *s, int t)
{
    if ((size_t)t >= s->nin_map) {
        size_t n = 2 * (size_t)t + 16;
        char *v = realloc(s->in_map, n);

        if (!v)
            return -1;
        memset(v + s->nin_map, 0, n - s->nin_map);
        s->in_map = v;
        s->nin_map = n;
    }
    s->in_map[t] = 1;
    return 0;
}

static int parse_line_number(const char *f, size_t n, int *v)
{
    long x = 0;
    size_t i;

    *v = 0;
    if (!n)
        return 1;
    for (i = 0; i < n; i++) {
        if (f[i] < '0' || f[i] > '9' || x > 100000000)
            return 0;
        x = 10 * x + (f[i] - '0');
    }
    *v = (int)x;
    return x > 0;
}

int covsel_read_map(struct covsel *s, const char *text, char *err, size_t errsz)
{
    size_t pos = 0, len = strlen(text), n, lineno = 0;
    const char *line;

    while ((line = cov_next_line(text, len, &pos, &n))) {
        const char *f[7], *p = line, *end = line + n;
        size_t fl[7];
        int k = 0;
        struct mapline *ml;

        lineno++;
        if (lineno == 1) {
            if (n < 10 || strncmp(line, "# commit ", 9))
                return error(err, errsz, "map line 1: no \"# commit\" line");
            free(s->commit);
            if (!(s->commit = strndup(line + 9, n - 9)))
                return OOM();
            continue;
        }
        if (!n || line[0] == '#')
            continue;
        for (;;) {
            const char *tab = memchr(p, '\t', (size_t)(end - p));

            f[k] = p;
            fl[k] = tab ? (size_t)(tab - p) : (size_t)(end - p);
            k++;
            if (!tab || k == 7)
                break;
            p = tab + 1;
        }
        if (k != 6)
            return error(err, errsz, "map line %zu: not 6 fields", lineno);
        if (!(ml = grow(s->lines, s->nlines, sizeof *ml)))
            return OOM();
        s->lines = ml;
        ml = &s->lines[s->nlines];
        memset(ml, 0, sizeof *ml);
        if (!parse_line_number(f[3], fl[3], &ml->first) || !parse_line_number(f[4], fl[4], &ml->last)
            || (ml->first && ml->last < ml->first) || !fl[0] || !fl[1])
            return error(err, errsz, "map line %zu: bad program, file or line range", lineno);
        s->nlines++;
        if ((ml->prog = strtab_addn(&s->strs, f[0], fl[0])) < 0
            || (ml->file = strtab_addn(&s->strs, f[1], fl[1])) < 0
            || (ml->name = fl[2] ? strtab_addn(&s->strs, f[2], fl[2]) : -1) < -1)
            return OOM();
        for (p = f[5]; p < end;) {
            const char *sp = memchr(p, ' ', (size_t)(end - p));
            size_t tl = sp ? (size_t)(sp - p) : (size_t)(end - p);
            int t;

            if (tl) {
                if ((t = strtab_addn(&s->tests, p, tl)) < 0 || ids_add(&ml->tests, t)
                    || mark_in_map(s, t))
                    return OOM();
            }
            p += tl + 1;
        }
    }
    if (!s->commit)
        return error(err, errsz, "the map is empty");
    return 0;
}

/* ---- the rules ---- */

int covsel_match(const char *pat, const char *str)
{
    for (;;) {
        if (!*pat)
            return !*str;
        if (pat[0] == '*' && pat[1] == '*') {
            pat += 2;
            if (*pat == '/') {
                /* "**" + "/": no directory, or any directories */
                if (covsel_match(pat + 1, str))
                    return 1;
                for (; *str; str++)
                    if (*str == '/' && covsel_match(pat + 1, str + 1))
                        return 1;
                return 0;
            }
            for (;; str++) {
                if (covsel_match(pat, str))
                    return 1;
                if (!*str)
                    return 0;
            }
        }
        if (*pat == '*') {
            pat++;
            for (;; str++) {
                if (covsel_match(pat, str))
                    return 1;
                if (!*str || *str == '/')
                    return 0;
            }
        }
        if (!*str || (*pat == '?' ? *str == '/' : *pat != *str))
            return 0;
        pat++;
        str++;
    }
}

static int push_string(char ***v, size_t *n, const char *s, size_t len)
{
    char **x = grow(*v, *n, sizeof *x);

    if (!x)
        return -1;
    *v = x;
    if (!(x[*n] = strndup(s, len)))
        return -1;
    (*n)++;
    return 0;
}

int covsel_read_rules(struct covsel *s, const char *text, char *err, size_t errsz)
{
    size_t pos = 0, len = strlen(text), n, lineno = 0;
    const char *line;

    while ((line = cov_next_line(text, len, &pos, &n))) {
        const char *p = line, *end = line + n;
        struct rule *r;
        int first = 1;

        lineno++;
        while (p < end && (*p == ' ' || *p == '\t'))
            p++;
        if (p == end || *p == '#')
            continue;
        if (!(r = grow(s->rules, s->nrules, sizeof *r)))
            return OOM();
        s->rules = r;
        r = &s->rules[s->nrules++];
        memset(r, 0, sizeof *r);
        r->line = (int)lineno;
        while (p < end) {
            const char *w = p;
            size_t wl;

            while (p < end && *p != ' ' && *p != '\t')
                p++;
            wl = (size_t)(p - w);
            while (p < end && (*p == ' ' || *p == '\t'))
                p++;
            if (wl && *w == '#')
                break;
            if (first) {
                if (!(r->pattern = strndup(w, wl)))
                    return OOM();
                first = 0;
            } else if (wl == 3 && !memcmp(w, "all", 3))
                r->flags |= SEL_ALL;
            else if (wl == 4 && !memcmp(w, "none", 4))
                r->flags |= SEL_NONE;
            else if (wl == 8 && !memcmp(w, "coverage", 8))
                r->flags |= SEL_COVERAGE;
            else if (wl == 6 && !memcmp(w, "header", 6))
                r->flags |= SEL_HEADER;
            else if (wl > 6 && !memcmp(w, "label:", 6)) {
                if (push_string(&r->labels, &r->nlabels, w + 6, wl - 6))
                    return OOM();
            } else if (wl > 5 && !memcmp(w, "name:", 5)) {
                if (push_string(&r->names, &r->nnames, w + 5, wl - 5))
                    return OOM();
            } else
                return error(err, errsz, "rules line %zu: unknown selector \"%.*s\"", lineno, (int)wl, w);
        }
        if (!r->flags && !r->nlabels && !r->nnames)
            return error(err, errsz, "rules line %zu: a pattern without a selector", lineno);
        if ((r->flags & SEL_NONE) && (r->flags != SEL_NONE || r->nlabels || r->nnames))
            return error(err, errsz, "rules line %zu: \"none\" with other selectors", lineno);
    }
    return 0;
}

/* ---- the tests ---- */

static int add_ctest(struct covsel *s, const char *name, const char *runner, const struct json *labels,
                     char *err, size_t errsz)
{
    struct test *t = grow(s->ctests, s->nctests, sizeof *t);
    size_t i;

    if (!t)
        return OOM();
    s->ctests = t;
    t = &s->ctests[s->nctests++];
    memset(t, 0, sizeof *t);
    if ((t->name = strtab_add(&s->tests, name)) < 0 || (t->runner = strtab_add(&s->tests, runner)) < 0)
        return OOM();
    for (i = 0; labels && i < labels->n; i++)
        if (labels->items[i].type == JSON_STRING
            && push_string(&t->labels, &t->nlabels, labels->items[i].string, strlen(labels->items[i].string)))
            return OOM();
    return 0;
}

int covsel_read_tests(struct covsel *s, const char *json, int host, char *err, size_t errsz)
{
    size_t pos = 0, i, k;
    char jerr[200];
    struct json *doc = json_parse(json, strlen(json), &pos, jerr, sizeof jerr);
    const struct json *tests = json_get(doc, "tests");
    int r = 0;

    if (!doc)
        return error(err, errsz, "ctest JSON: %s", jerr[0] ? jerr : "empty");
    if (!tests || tests->type != JSON_ARRAY) {
        json_free(doc);
        return error(err, errsz, "not ctest --show-only=json-v1 (no \"tests\")");
    }
    for (i = 0; i < tests->n && !r; i++) {
        const struct json *t = &tests->items[i], *props = json_get(t, "properties"), *labels = NULL;
        const char *name = json_get_string(t, "name");
        int emulator = 0;
        char *mapname;

        if (!name) {
            r = error(err, errsz, "not ctest --show-only=json-v1 (a test without \"name\")");
            break;
        }
        for (k = 0; props && props->type == JSON_ARRAY && k < props->n; k++) {
            const char *pn = json_get_string(&props->items[k], "name");
            const struct json *v = json_get(&props->items[k], "value");

            if (pn && !strcmp(pn, "LABELS") && v && v->type == JSON_ARRAY)
                labels = v;
        }
        for (k = 0; labels && k < labels->n; k++)
            if (labels->items[k].type == JSON_STRING && !strcmp(labels->items[k].string, "emulator"))
                emulator = 1;
        if (!host) {
            r = add_ctest(s, name, name, labels, err, errsz);
            continue;
        }
        if (!(mapname = malloc(strlen(name) + 6))) {
            r = OOM();
            break;
        }
        strcpy(mapname, "host/");
        strcat(mapname, name);
        r = add_ctest(s, mapname, emulator ? "host_emulator" : "host", labels, err, errsz);
        free(mapname);
    }
    json_free(doc);
    return r;
}

int covsel_read_deps(struct covsel *s, const char *text, char *err, size_t errsz)
{
    return cov_deps_parse(&s->deps, text, err, errsz);
}

/* ---- the changes ---- */

static struct change *add_change(struct covsel *s, const char *path, size_t n, int whole)
{
    struct change *c;
    size_t i;

    for (i = 0; i < s->nchanges; i++)
        if (!strncmp(s->changes[i].path, path, n) && !s->changes[i].path[n]) {
            s->changes[i].whole |= whole;
            return &s->changes[i];
        }
    if (!(c = grow(s->changes, s->nchanges, sizeof *c)))
        return NULL;
    s->changes = c;
    c = &s->changes[s->nchanges];
    memset(c, 0, sizeof *c);
    if (!(c->path = strndup(path, n)))
        return NULL;
    c->whole = whole;
    s->nchanges++;
    return c;
}

int covsel_add_new_file(struct covsel *s, const char *path)
{
    return add_change(s, path, strlen(path), 1) ? 0 : -1;
}

/* a path of a diff header without "a/" or "b/" and quotes; NULL for
   /dev/null */
static const char *diff_path(const char *p, size_t *n)
{
    while (*n && (p[*n - 1] == ' ' || p[*n - 1] == '\t'))
        (*n)--;
    if (*n >= 2 && p[0] == '"' && p[*n - 1] == '"') {
        p++;
        *n -= 2;
    }
    if (*n == 9 && !memcmp(p, "/dev/null", 9))
        return NULL;
    if (*n >= 2 && (p[0] == 'a' || p[0] == 'b') && p[1] == '/') {
        p += 2;
        *n -= 2;
    }
    return p;
}

struct diff_file {
    const char *old, *new;      /* in the text, NULL: /dev/null or not seen */
    size_t oldn, newn;
    struct range *ranges;
    size_t nranges;
    int any;                    /* a diff --git line seen */
};

static int flush_file(struct covsel *s, struct diff_file *d, char *err, size_t errsz)
{
    struct change *c;
    size_t i;
    int same = d->old && d->new && d->oldn == d->newn && !memcmp(d->old, d->new, d->oldn);

    if (!d->any)
        return 0;
    if (d->old) {
        /* the map's file: its changed lines, or all of it */
        if (!(c = add_change(s, d->old, d->oldn, !same || !d->nranges)))
            return OOM();
        for (i = 0; same && i < d->nranges; i++) {
            struct range *r = grow(c->ranges, c->nranges, sizeof *r);

            if (!r)
                return OOM();
            c->ranges = r;
            c->ranges[c->nranges++] = d->ranges[i];
        }
    }
    if (d->new && !same && !add_change(s, d->new, d->newn, 1))
        return OOM();
    free(d->ranges);
    memset(d, 0, sizeof *d);
    return 0;
}

static int parse_count(const char **p, long *v)
{
    char *end;

    *v = strtol(*p, &end, 10);
    if (end == *p || *v < 0)
        return 0;
    *p = end;
    return 1;
}

int covsel_read_diff(struct covsel *s, const char *text, char *err, size_t errsz)
{
    size_t pos = 0, len = strlen(text), n, lineno = 0;
    const char *line;
    struct diff_file d = { 0 };
    long minus = 0, plus = 0;   /* lines of the hunk still to come */

    while ((line = cov_next_line(text, len, &pos, &n))) {
        lineno++;
        if (minus > 0 || plus > 0) {
            if (n && line[0] == '-')
                minus--;
            else if (n && line[0] == '+')
                plus--;
            else if (!(n && line[0] == '\\')) {
                free(d.ranges);
                return error(err, errsz, "diff line %zu: the hunk ends early", lineno);
            }
            continue;
        }
        if (n && line[0] == '\\')
            continue;           /* "\ No newline at end of file" */
        if (n > 11 && !memcmp(line, "diff --git ", 11)) {
            const char *a = line + 11, *b = NULL;
            size_t i;

            if (flush_file(s, &d, err, errsz))
                return -1;
            d.any = 1;
            /* "a/OLD b/NEW": OLD and NEW from the ---/+++ lines if there
               are some; here for renames and binary files */
            for (i = 11; i + 3 <= n; i++)
                if (!memcmp(line + i, " b/", 3) || !memcmp(line + i, " \"b/", 4))
                    b = line + i + 1;
            if (b) {
                d.oldn = (size_t)(b - 1 - a);
                d.old = diff_path(a, &d.oldn);
                d.newn = (size_t)(line + n - b);
                d.new = diff_path(b, &d.newn);
            }
            continue;
        }
        if (!d.any) {
            if (!n)
                continue;
            free(d.ranges);
            return error(err, errsz, "diff line %zu: not git diff output", lineno);
        }
        if (n >= 4 && !memcmp(line, "--- ", 4)) {
            d.oldn = n - 4;
            d.old = diff_path(line + 4, &d.oldn);
        } else if (n >= 4 && !memcmp(line, "+++ ", 4)) {
            d.newn = n - 4;
            d.new = diff_path(line + 4, &d.newn);
        } else if (n > 12 && !memcmp(line, "rename from ", 12)) {
            d.old = line + 12;
            d.oldn = n - 12;
        } else if (n > 10 && !memcmp(line, "rename to ", 10)) {
            d.new = line + 10;
            d.newn = n - 10;
        } else if (n > 14 && !memcmp(line, "new file mode ", 14)) {
            d.old = NULL;
        } else if (n > 18 && !memcmp(line, "deleted file mode ", 18)) {
            d.new = NULL;
        } else if (n >= 3 && !memcmp(line, "@@ ", 3)) {
            /* "@@ -A[,B] +C[,D] @@" */
            const char *p = line + 3;
            struct range r = { 0, 1 };
            long c, dcount = 1;
            struct range *v;

            if (*p++ != '-' || !parse_count(&p, &r.first) || (*p == ',' && (p++, !parse_count(&p, &r.count)))
                || *p++ != ' ' || *p++ != '+' || !parse_count(&p, &c)
                || (*p == ',' && (p++, !parse_count(&p, &dcount))) || strncmp(p, " @@", 3)) {
                free(d.ranges);
                return error(err, errsz, "diff line %zu: bad hunk header", lineno);
            }
            if (!(v = grow(d.ranges, d.nranges, sizeof *v))) {
                free(d.ranges);
                return OOM();
            }
            d.ranges = v;
            d.ranges[d.nranges++] = r;
            minus = r.count;
            plus = dcount;
        }
        /* other extended headers (index, mode, similarity, Binary files)
           need nothing */
    }
    if (minus > 0 || plus > 0) {
        free(d.ranges);
        return error(err, errsz, "diff: the last hunk ends early");
    }
    return flush_file(s, &d, err, errsz);
}

/* ---- selecting ---- */

static void say(FILE *log, const char *fmt, ...)
{
    va_list ap;

    if (!log)
        return;
    va_start(ap, fmt);
    vfprintf(log, fmt, ap);
    va_end(ap);
}

static size_t count_selected(const struct covsel *s)
{
    size_t i, n = 0;

    for (i = 0; i < s->tests.n; i++)
        n += s->selected[i] != 0;
    return n;
}

static void select_all(struct covsel *s)
{
    size_t i;

    for (i = 0; i < s->nctests; i++)
        s->selected[s->ctests[i].runner] = 1;
}

/* the tests of the map with name id t: their runners; returns how many
   ctest tests have that name */
static size_t select_map_test(struct covsel *s, int t)
{
    size_t i, n = 0;

    for (i = 0; i < s->nctests; i++)
        if (s->ctests[i].name == t) {
            s->selected[s->ctests[i].runner] = 1;
            n++;
        }
    return n;
}

static void select_ids(struct covsel *s, const struct ids *d)
{
    size_t i;

    for (i = 0; i < d->n; i++)
        select_map_test(s, d->v[i]);
}

/* coverage of one file; 0 if the map does not know it, -1 if out of
   memory */
static int select_coverage(struct covsel *s, const struct change *c, FILE *log)
{
    int file = strtab_find(&s->strs, c->path);
    size_t i, j, k, nprog = 0;
    char *hit;

    if (file < 0)
        return 0;
    if (!(hit = calloc(s->nlines + 1, 1)))
        return -1;
    for (i = 0; i < s->nlines; i++) {
        const struct mapline *o = &s->lines[i];
        int whole = c->whole;

        if (o->name >= 0 || o->file != file)
            continue;
        nprog++;
        /* the changed lines in this program's functions of the file */
        for (k = 0; !whole && k < c->nranges; k++) {
            long a = c->ranges[k].first, b = a + c->ranges[k].count - 1, l;

            if (!c->ranges[k].count)
                b = a + 1;      /* inserted after line a: a and a + 1 */
            if (a < 1)
                a = 1;
            for (l = a; l <= b && !whole; l++) {
                int inside = 0;

                for (j = 0; j < s->nlines; j++) {
                    const struct mapline *f = &s->lines[j];

                    if (f->name >= 0 && f->prog == o->prog && f->file == file && f->first
                        && l >= f->first && l <= f->last)
                        hit[j] = 1, inside = 1;
                }
                whole = !inside;
            }
        }
        if (whole) {
            select_ids(s, &o->tests);
            say(log, "%s: %s, %s: the tests of all its functions: %zu test(s)\n", c->path,
                s->strs.str[o->prog], c->whole ? "the whole file" : "a line outside its functions",
                o->tests.n);
            continue;
        }
        for (j = 0; j < s->nlines; j++) {
            const struct mapline *f = &s->lines[j];

            if (hit[j] && f->prog == o->prog) {
                select_ids(s, &f->tests);
                say(log, "%s: %s %s: %zu test(s)\n", c->path, s->strs.str[o->prog],
                    s->strs.str[f->name], f->tests.n);
            }
        }
    }
    free(hit);
    return nprog > 0;
}

static int select_change(struct covsel *s, const struct change *c, FILE *log, int depth);

/* header: every source whose object depends on the file, as if that
   source changed as a whole (by its own rule); 0 if no object depends on
   it, -1 if out of memory */
static int select_header(struct covsel *s, const struct change *c, FILE *log, int depth)
{
    size_t i, k, nobj = 0;
    char **done = NULL;
    size_t ndone = 0;
    int r = 0;

    for (i = 0; i < s->deps.n && !r; i++) {
        const struct cov_dep *d = &s->deps.v[i];
        const char *src;
        char *rel;
        int found = 0;

        for (k = 0; k < d->ndeps && !found; k++) {
            char *p = cov_relpath(d->deps[k], s->root);

            found = p && !strcmp(p, c->path);
            free(p);
        }
        if (!found)
            continue;
        nobj++;
        if (!(src = cov_dep_source(d)) || !(rel = cov_relpath(src, s->root))) {
            say(log, "%s: %s depends on it, its source is not known: all tests\n", c->path, d->object);
            select_all(s);
            continue;
        }
        for (k = 0; k < ndone && strcmp(done[k], rel); k++)
            ;
        if (k < ndone || !strcmp(rel, c->path)) {
            free(rel);
            continue;           /* the same source in another object */
        }
        say(log, "%s: %s depends on it\n", c->path, rel);
        {
            struct change whole = { rel, 1, NULL, 0 };

            r = select_change(s, &whole, log, depth + 1);
        }
        if (push_string(&done, &ndone, rel, strlen(rel)))
            r = -1;
        free(rel);
    }
    free_strings(done, ndone);
    if (r)
        return r;
    if (!nobj)
        say(log, "%s: no object depends on it\n", c->path);
    return nobj > 0;
}

/* the tests a change selects by its rule */
static int select_change(struct covsel *s, const struct change *c, FILE *log, int depth)
{
    const struct rule *r = NULL;
    size_t k, l;

    for (k = 0; k < s->nrules && !r; k++)
        if (covsel_match(s->rules[k].pattern, c->path))
            r = &s->rules[k];
    if (!r) {
        say(log, "%s: no rule: all tests\n", c->path);
        select_all(s);
        return 0;
    }
    if (r->flags & SEL_ALL) {
        say(log, "%s: rule %d (%s): all tests\n", c->path, r->line, r->pattern);
        select_all(s);
    }
    if (r->flags & SEL_NONE)
        say(log, "%s: rule %d (%s): no test\n", c->path, r->line, r->pattern);
    if (r->flags & SEL_COVERAGE) {
        int found = select_coverage(s, c, log);

        if (found < 0)
            return -1;
        if (!found) {
            say(log, "%s: not in the map: all tests\n", c->path);
            select_all(s);
        }
    }
    if (r->flags & SEL_HEADER) {
        int found = depth ? 0 : select_header(s, c, log, depth);

        if (found < 0)
            return -1;
        if (!found) {
            say(log, "%s: all tests\n", c->path);
            select_all(s);
        }
    }
    for (k = 0; k < r->nlabels; k++) {
        size_t n = 0;

        for (l = 0; l < s->nctests; l++) {
            const struct test *t = &s->ctests[l];
            size_t m;

            for (m = 0; m < t->nlabels; m++)
                if (!strcmp(t->labels[m], r->labels[k])) {
                    s->selected[t->runner] = 1;
                    n++;
                    break;
                }
        }
        say(log, "%s: rule %d (%s): label %s: %zu test(s)\n", c->path, r->line, r->pattern,
            r->labels[k], n);
    }
    for (k = 0; k < r->nnames; k++) {
        size_t n = 0;

        for (l = 0; l < s->nctests; l++)
            if (covsel_match(r->names[k], s->tests.str[s->ctests[l].name])) {
                s->selected[s->ctests[l].runner] = 1;
                n++;
            }
        say(log, "%s: rule %d (%s): name %s: %zu test(s)\n", c->path, r->line, r->pattern,
            r->names[k], n);
    }
    return 0;
}

static const struct covsel *sort_sel;

static int cmp_result(const void *a, const void *b)
{
    return strcmp(sort_sel->tests.str[*(const int *)a], sort_sel->tests.str[*(const int *)b]);
}

int covsel_run(struct covsel *s, FILE *log)
{
    size_t i;

    free(s->selected);
    if (!(s->selected = calloc(s->tests.n + 1, 1)))
        return -1;
    if (s->stale) {
        say(log, "the map (commit %s) is not an ancestor of the base: all tests\n",
            s->commit ? s->commit : "?");
        select_all(s);
    } else {
        /* the runners of other tests (host, host_emulator) count by the
           tests they run */
        char *runs_others = calloc(s->tests.n + 1, 1);

        if (!runs_others)
            return -1;
        for (i = 0; i < s->nctests; i++)
            if (s->ctests[i].name != s->ctests[i].runner)
                runs_others[s->ctests[i].runner] = 1;
        for (i = 0; i < s->nctests; i++) {
            int t = s->ctests[i].name;

            if (runs_others[t])
                continue;
            if ((size_t)t >= s->nin_map || !s->in_map[t]) {
                say(log, "test %s: not in the map\n", s->tests.str[t]);
                s->selected[s->ctests[i].runner] = 1;
            }
        }
        free(runs_others);
        for (i = 0; i < s->nchanges; i++)
            if (select_change(s, &s->changes[i], log, 0))
                return -1;
    }
    free(s->result);
    s->nresult = 0;
    if (!(s->result = malloc((count_selected(s) + 1) * sizeof *s->result)))
        return -1;
    for (i = 0; i < s->tests.n; i++)
        if (s->selected[i])
            s->result[s->nresult++] = (int)i;
    sort_sel = s;
    qsort(s->result, s->nresult, sizeof *s->result, cmp_result);
    return 0;
}

size_t covsel_count(const struct covsel *s)
{
    return s->nresult;
}

const char *covsel_name(const struct covsel *s, size_t i)
{
    return i < s->nresult ? s->tests.str[s->result[i]] : NULL;
}
