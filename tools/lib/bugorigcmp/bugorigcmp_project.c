/*
 * bugorigcmp_project.c - a bug-orig test as the cases of the shared test
 * model, and fix-scope over two trees of tests (bugorigcmp.h).
 */
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "bugorigcmp.h"
#include "bugorigcmp_int.h"

static uint32_t fnv1a(const char *s, size_t n)
{
    uint32_t h = 0x811C9DC5u;

    while (n--)
        h = (h ^ (unsigned char)*s++) * 0x01000193u;
    return h;
}

/* the value of a section's text */
static void put_value(FILE *out, const char *s)
{
    size_t n = strlen(s), len = 2, i;

    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];

        len += c < 32 || c > 126 || c == '\\' || c == '"' ? 4 : 1;
    }
    if (len > BUGORIGCMP_LONG) {
        fprintf(out, "bytes %zu fnv %08lX", n, (unsigned long)fnv1a(s, n));
        return;
    }
    fputc('"', out);
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];

        if (c < 32 || c > 126 || c == '\\' || c == '"')
            fprintf(out, "\\x%02X", c);
        else
            fputc(c, out);
    }
    fputc('"', out);
}

/* the line at p without LF and trailing blanks */
static size_t trimmed(const char *p, size_t *full)
{
    const char *e = strchr(p, '\n');
    size_t n = e ? (size_t)(e - p) : strlen(p);

    *full = n;
    while (n && (p[n - 1] == ' ' || p[n - 1] == '\t' || p[n - 1] == '\r'))
        n--;
    return n;
}

/* the issue of the line "; @fixed #N" of source: N, 0 = none, -2 = broken */
static long fixed_issue(const char *source, const char *path, char *err, size_t errlen)
{
    static const char tag[] = "; @fixed";
    const char *p = source;
    unsigned no = 0;
    long issue = 0;

    while (*p) {
        size_t full, n = trimmed(p, &full);

        no++;
        if (n >= sizeof tag - 1 && !memcmp(p, tag, sizeof tag - 1)) {
            size_t k = sizeof tag - 1;
            long v = 0;

            if (k + 2 < n && p[k] == ' ' && p[k + 1] == '#' && p[k + 2] >= '1' && p[k + 2] <= '9')
                for (k += 2; k < n && p[k] >= '0' && p[k] <= '9' && v < 100000000; k++)
                    v = 10 * v + (p[k] - '0');
            if (!v || k != n) {
                snprintf(err, errlen, "%s:%u: not \"; @fixed #N\"", path, no);
                return -2;
            }
            if (issue) {
                snprintf(err, errlen, "%s:%u: @fixed twice", path, no);
                return -2;
            }
            issue = v;
        }
        p += p[full] ? full + 1 : full;
    }
    return issue;
}

/* the "; @expect" lines of a TED script (malloc'd, "" if none) */
static char *ted_expects(const char *source)
{
    static const char tag[] = "; @expect";
    struct bstr s = { 0 };
    const char *p = source;

    while (*p) {
        size_t full, n = trimmed(p, &full);

        if (n >= sizeof tag - 1 && !memcmp(p, tag, sizeof tag - 1)
            && (n == sizeof tag - 1 || p[sizeof tag - 1] == ' '))
            bstr_add(&s, p, n), bstr_add(&s, "\n", 1);
        p += p[full] ? full + 1 : full;
    }
    return bstr_take(&s);
}

static int id_ok(const char *id)
{
    const char *p;

    if (!*id)
        return 0;
    for (p = id; *p; p++)
        if ((unsigned char)*p < 33 || (unsigned char)*p > 126 || *p == '|' || *p == '\\'
            || *p == '[')
            return 0;
    return 1;
}

int bugorigcmp_project_test(FILE *out, const char *id, const char *source, int ted,
                            const char *expect, const char *orig, const char *path, char *err,
                            size_t errlen)
{
    struct bugorigcmp_obs e = { 0 }, o = { 0 };
    char sp[600];
    long issue;
    size_t i;
    int r;

    if (!id_ok(id)) {
        snprintf(err, errlen, "%s: the ID is not a key part", path);
        return -2;
    }
    snprintf(sp, sizeof sp, "%s.%s", path, ted ? "drv" : "bas");
    if ((issue = fixed_issue(source, sp, err, errlen)) < 0)
        return -2;
    if (ted) {
        char *x = ted_expects(source);

        if (!x) {
            snprintf(err, errlen, "out of memory");
            return -1;
        }
        r = 0;
        if (*x) {
            r = -1;
            if ((e.sec = calloc(1, sizeof *e.sec)) && (e.sec->name = strdup("expect"))) {
                e.n = 1;
                e.sec->text = x;
                x = NULL;
                r = 0;
            }
        }
        free(x);
        if (r) {
            bugorigcmp_free(&e);
            snprintf(err, errlen, "out of memory");
            return -1;
        }
    } else {
        if (!expect) {
            snprintf(err, errlen, "%s.bas has no %s.expect", path, id);
            return -2;
        }
        snprintf(sp, sizeof sp, "%s.expect", path);
        if ((r = bugorigcmp_parse(expect, sp, BUGORIGCMP_EXPECT, &e, err, errlen)))
            return r;
    }
    snprintf(sp, sizeof sp, "%s.orig", path);
    if (orig && (r = bugorigcmp_parse(orig, sp, BUGORIGCMP_ORIG, &o, err, errlen))) {
        bugorigcmp_free(&e);
        return r;
    }
    for (i = 0; i < e.n; i++) {
        fprintf(out, "%s|%s||1", id, e.sec[i].name);
        if (issue)
            fprintf(out, " [fixed #%ld]", issue);
        fputc('\t', out);
        put_value(out, e.sec[i].text);
        fputc('\n', out);
    }
    for (i = 0; i < o.n; i++) {
        fprintf(out, "%s|%s|orig|1%s\t", id, o.sec[i].name, issue ? " [orig]" : "");
        put_value(out, o.sec[i].text);
        fputc('\n', out);
        if (issue)
            fprintf(out, "%s|%s|orig|1 [fixed #%ld]\tjudged by ID.expect\n", id, o.sec[i].name,
                    issue);
    }
    bugorigcmp_free(&e);
    bugorigcmp_free(&o);
    return 0;
}

/* ------------------------------------------------------------------ trees */

struct test {
    char *id, *dir;
    int ted;
};

static int by_id(const void *a, const void *b)
{
    return strcmp(((const struct test *)a)->id, ((const struct test *)b)->id);
}

/* the file (malloc'd, 0-terminated); NULL: missing (*missing) or an error */
static char *read_text(const char *path, int *missing)
{
    FILE *f = fopen(path, "rb");
    struct bstr s = { 0 };
    char buf[8192];
    size_t n;

    *missing = !f && errno == ENOENT;
    if (!f)
        return NULL;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        bstr_add(&s, buf, n);
    if (ferror(f))
        s.bad = 1;
    fclose(f);
    return bstr_take(&s);
}

static char *join(const char *a, const char *b, const char *c)
{
    size_t n = strlen(a) + strlen(b) + strlen(c) + 2;
    char *p = malloc(n);

    if (p)
        snprintf(p, n, "%s/%s%s", a, b, c);
    return p;
}

static void free_tests(struct test *t, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        free(t[i].id);
        free(t[i].dir);
    }
    free(t);
}

/* the tests below root into *tests; 0 = ok, -1 = out of memory, -2 = error */
static int list_tests(const char *root, struct test **tests, size_t *ntests, char *err,
                      size_t errlen)
{
    DIR *d = opendir(root), *a;
    struct dirent *de, *fe;
    struct test *t = NULL;
    size_t n = 0;
    int r = 0;

    *tests = NULL;
    *ntests = 0;
    if (!d) {
        if (errno == ENOENT)
            return 0;
        snprintf(err, errlen, "%s: cannot read the directory", root);
        return -2;
    }
    while (!r && (de = readdir(d))) {
        char *dir;
        struct stat st;

        if (de->d_name[0] == '.' || !strcmp(de->d_name, "selftest"))
            continue;
        if (!(dir = join(root, de->d_name, ""))) {
            r = -1;
            break;
        }
        if (stat(dir, &st) || !S_ISDIR(st.st_mode) || !(a = opendir(dir))) {
            free(dir);
            continue;
        }
        while (!r && (fe = readdir(a))) {
            size_t len = strlen(fe->d_name);
            int ted = len > 4 && !strcmp(fe->d_name + len - 4, ".drv");
            struct test *nt;

            if (!ted && !(len > 4 && !strcmp(fe->d_name + len - 4, ".bas")))
                continue;
            if (!(nt = realloc(t, (n + 1) * sizeof *t))) {
                r = -1;
                break;
            }
            t = nt;
            t[n].ted = ted;
            t[n].id = strndup(fe->d_name, len - 4);
            t[n].dir = strdup(dir);
            if (!t[n].id || !t[n].dir) {
                free(t[n].id);
                free(t[n].dir);
                r = -1;
                break;
            }
            n++;
        }
        closedir(a);
        free(dir);
    }
    closedir(d);
    if (r) {
        free_tests(t, n);
        if (r == -1)
            snprintf(err, errlen, "out of memory");
        return r;
    }
    if (n)
        qsort(t, n, sizeof *t, by_id);
    *tests = t;
    *ntests = n;
    return 0;
}

int bugorigcmp_project_tree(const char *root, char **text, size_t *len, char *err,
                            size_t errlen)
{
    struct test *t;
    size_t n, i;
    FILE *out;
    int r;

    *text = NULL;
    *len = 0;
    if ((r = list_tests(root, &t, &n, err, errlen)))
        return r;
    if (!(out = open_memstream(text, len))) {
        free_tests(t, n);
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    fprintf(out, "# capture bug-orig 1\n");
    for (i = 0; !r && i < n; i++) {
        char *base = join(t[i].dir, t[i].id, ""), *src = join(t[i].dir, t[i].id, t[i].ted ? ".drv" : ".bas");
        char *ep = join(t[i].dir, t[i].id, ".expect"), *op = join(t[i].dir, t[i].id, ".orig");
        char *s = NULL, *e = NULL, *o = NULL;
        int miss;

        if (!base || !src || !ep || !op) {
            snprintf(err, errlen, "out of memory");
            r = -1;
        } else if (!(s = read_text(src, &miss))) {
            snprintf(err, errlen, "%s: cannot read it", src);
            r = -2;
        } else {
            if (!t[i].ted && !(e = read_text(ep, &miss)) && !miss) {
                snprintf(err, errlen, "%s: cannot read it", ep);
                r = -2;
            }
            if (!r && !(o = read_text(op, &miss)) && !miss) {
                snprintf(err, errlen, "%s: cannot read it", op);
                r = -2;
            }
            if (!r)
                r = bugorigcmp_project_test(out, t[i].id, s, t[i].ted, e, o, base, err, errlen);
        }
        free(s);
        free(e);
        free(o);
        free(base);
        free(src);
        free(ep);
        free(op);
    }
    free_tests(t, n);
    if (fclose(out) && !r) {
        snprintf(err, errlen, "out of memory");
        r = -1;
    }
    if (r) {
        free(*text);
        *text = NULL;
        *len = 0;
    }
    return r;
}

int bugorigcmp_fix_scope_trees(const char *base_root, const char *branch_root,
                               const struct libscmp_issues *is, FILE *out, FILE *err)
{
    struct libscmp_file b, r;
    char *tb = NULL, *tr = NULL, msg[700];
    size_t lb, lr, nch = 0, nrep, i;
    int st;

    memset(&b, 0, sizeof b);
    memset(&r, 0, sizeof r);
    if (bugorigcmp_project_tree(base_root, &tb, &lb, msg, sizeof msg)
        || bugorigcmp_project_tree(branch_root, &tr, &lr, msg, sizeof msg)
        || libscmp_parse(&b, tb, lb, base_root, LIBSCMP_EXPECTED, msg, sizeof msg)
        || libscmp_parse(&r, tr, lr, branch_root, LIBSCMP_EXPECTED, msg, sizeof msg)) {
        fprintf(err, "%s\n", msg);
        free(tb);
        free(tr);
        libscmp_free(&b);
        libscmp_free(&r);
        return 2;
    }
    nrep = libscmp_fix_scope(&b, &r, is, out, &nch);
    fprintf(out, "fix-scope: bug-orig: %zu cases changed, %zu not of the issues", nch, nrep);
    if (!is->n)
        fprintf(out, " (no \"Fixes #N\" line: every changed case is reported)");
    for (i = 0; i < is->n; i++)
        fprintf(out, "%s#%lu", i ? ", " : " ", (unsigned long)is->v[i]);
    fprintf(out, "\n");
    st = nrep ? 1 : 0;
    free(tb);
    free(tr);
    libscmp_free(&b);
    libscmp_free(&r);
    return st;
}
