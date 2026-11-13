/*
 * corpusexpect.c - the expected data of the corpus as cases (corpusexpect.h).
 */
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "corpusexpect.h"
#include "sha256.h"

static const char *const obs_names[CEXP_NOBS] = { "out", "serial", "exe" };

const char *cexp_obs_name(enum cexp_obs o)
{
    return (unsigned)o < CEXP_NOBS ? obs_names[o] : "?";
}

enum cexp_side cexp_side_of_tag(const char *tag)
{
    if (!strcmp(tag, "ref"))
        return CEXP_SIDE_ORIG;
    if (!strcmp(tag, "port") || !strcmp(tag, "orig-clib"))
        return CEXP_SIDE_EITHER;
    return CEXP_SIDE_PORT;
}

/* a name of a program or pass: [A-Za-z0-9_-], not empty */
static int name_ok(const char *s, size_t n)
{
    size_t i;

    if (!n || n >= 64)
        return 0;
    for (i = 0; i < n; i++)
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')
              || (s[i] >= '0' && s[i] <= '9') || s[i] == '_' || s[i] == '-'))
            return 0;
    return 1;
}

void cexp_file_name(const struct cexp_prog *p, enum cexp_obs o, enum cexp_qual q, char *buf,
                    size_t len)
{
    if (q == CEXP_ORIG)
        snprintf(buf, len, "%s.orig.%s", p->name, obs_names[o]);
    else if (q == CEXP_FIXED)
        snprintf(buf, len, "%s.fixed-%u.%s", p->name, p->issue[o], obs_names[o]);
    else
        snprintf(buf, len, "%s.%s", p->name, obs_names[o]);
}

static int seterr(char *err, size_t errlen, int r, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return r;
}

/* the program NAME (n bytes) of s, made if make; NULL: none / out of memory */
static struct cexp_prog *prog_of(struct cexp_set *s, const char *name, size_t n, int make)
{
    size_t lo = 0, hi = s->n;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        int c = strncmp(s->p[mid].name, name, n);

        if (!c && s->p[mid].name[n])
            c = 1;
        if (!c)
            return &s->p[mid];
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (!make)
        return NULL;
    if (s->n == s->cap) {
        size_t cap = s->cap ? 2 * s->cap : 32;
        struct cexp_prog *p = realloc(s->p, cap * sizeof *p);

        if (!p)
            return NULL;
        s->p = p;
        s->cap = cap;
    }
    memmove(s->p + lo + 1, s->p + lo, (s->n - lo) * sizeof *s->p);
    memset(&s->p[lo], 0, sizeof s->p[lo]);
    if (!(s->p[lo].name = strndup(name, n))) {
        memmove(s->p + lo, s->p + lo + 1, (s->n - lo) * sizeof *s->p);
        return NULL;
    }
    s->n++;
    return &s->p[lo];
}

const struct cexp_prog *cexp_find(const struct cexp_set *s, const char *name)
{
    return prog_of((struct cexp_set *)s, name, strlen(name), 0);
}

/* an .exe file's text: 64 lower-case hex digits and a LF */
static int hash_ok(const uint8_t *d, size_t n)
{
    size_t i;

    if (n != CEXP_HASHLEN || d[64] != '\n')
        return 0;
    for (i = 0; i < 64; i++)
        if (!((d[i] >= '0' && d[i] <= '9') || (d[i] >= 'a' && d[i] <= 'f')))
            return 0;
    return 1;
}

int cexp_add(struct cexp_set *s, const char *name, const void *data, size_t len, char *err,
             size_t errlen)
{
    const char *ext = strrchr(name, '.'), *dot;
    struct cexp_prog *p;
    enum cexp_obs o;
    enum cexp_qual q = CEXP_PLAIN;
    unsigned long issue = 0;
    size_t base;
    char other[160];

    if (!ext)
        return seterr(err, errlen, -2, "%s: not NAME.OBS (OBS out, serial or exe)", name);
    for (o = 0; o < CEXP_NOBS && strcmp(ext + 1, obs_names[o]); o++)
        ;
    if (o == CEXP_NOBS)
        return seterr(err, errlen, -2, "%s: unknown observation .%s (out, serial or exe)", name,
                      ext + 1);
    base = (size_t)(ext - name);
    /* the qualifier: the part between the last two dots */
    for (dot = ext; dot > name && dot[-1] != '.'; dot--)
        ;
    if (dot > name) {
        const char *qs = dot;
        size_t qn = (size_t)(ext - dot);

        base = (size_t)(dot - 1 - name);
        if (qn == 4 && !memcmp(qs, "orig", 4))
            q = CEXP_ORIG;
        else if (qn > 6 && !memcmp(qs, "fixed-", 6) && qs[6] >= '1' && qs[6] <= '9') {
            size_t i;

            for (i = 6; i < qn && qs[i] >= '0' && qs[i] <= '9' && issue < 100000000; i++)
                issue = 10 * issue + (unsigned long)(qs[i] - '0');
            if (i != qn)
                issue = 0;
            q = CEXP_FIXED;
        }
        if (q == CEXP_PLAIN || (q == CEXP_FIXED && !issue))
            return seterr(err, errlen, -2, "%s: unknown qualifier \"%.*s\" (orig or fixed-N)",
                          name, (int)qn, qs);
    }
    if (!name_ok(name, base))
        return seterr(err, errlen, -2, "%s: the program's name is not of [A-Za-z0-9_-]", name);
    if (s->run && q != CEXP_PLAIN)
        return seterr(err, errlen, -2, "%s: a run has no qualified files", name);
    if (o == CEXP_EXE && q == CEXP_PLAIN && !s->run)
        return seterr(err, errlen, -2, "%s: an executable is only a pair (NAME.orig.exe and "
                      "NAME.fixed-N.exe), a program whose executables agree has no .exe file",
                      name);
    if (o == CEXP_EXE && !hash_ok(data, len))
        return seterr(err, errlen, -2, "%s: not a SHA-256 (64 lower-case hex digits and a LF)",
                      name);
    if (!(p = prog_of(s, name, base, 1)))
        return seterr(err, errlen, -1, "out of memory");
    if (p->f[o][q].data) {
        cexp_file_name(p, o, q, other, sizeof other);
        return seterr(err, errlen, -2, q == CEXP_FIXED
                      ? "%s and %s: two fixed files of one observation"
                      : "%s and %s: the same case twice", other, name);
    }
    if (!(p->f[o][q].data = malloc(len ? len : 1)))
        return seterr(err, errlen, -1, "out of memory");
    memcpy(p->f[o][q].data, data, len);
    p->f[o][q].len = len;
    if (q == CEXP_FIXED)
        p->issue[o] = (unsigned)issue;
    return 0;
}

static int same_file(const struct cexp_file *a, const struct cexp_file *b)
{
    return a->data && b->data && a->len == b->len && !memcmp(a->data, b->data, a->len);
}

int cexp_check(const struct cexp_set *s, int strict, char *err, size_t errlen)
{
    size_t i;
    char a[160], b[160];

    for (i = 0; i < s->n; i++) {
        const struct cexp_prog *p = &s->p[i];
        enum cexp_obs o;

        for (o = 0; o < CEXP_NOBS; o++) {
            const struct cexp_file *f = p->f[o];

            if (f[CEXP_PLAIN].data && (f[CEXP_ORIG].data || f[CEXP_FIXED].data)) {
                cexp_file_name(p, o, CEXP_PLAIN, a, sizeof a);
                cexp_file_name(p, o, f[CEXP_ORIG].data ? CEXP_ORIG : CEXP_FIXED, b, sizeof b);
                return seterr(err, errlen, -2, "%s/%s and %s: an unqualified and a qualified "
                              "file of one observation", s->pass ? s->pass : ".", a, b);
            }
            if (same_file(&f[CEXP_ORIG], &f[CEXP_FIXED])) {
                cexp_file_name(p, o, CEXP_ORIG, a, sizeof a);
                cexp_file_name(p, o, CEXP_FIXED, b, sizeof b);
                return seterr(err, errlen, -2, "%s/%s and %s: a pair of equal files (no "
                              "difference: make it %s.%s)", s->pass ? s->pass : ".", a, b,
                              p->name, obs_names[o]);
            }
            if (strict && !f[CEXP_ORIG].data != !f[CEXP_FIXED].data) {
                enum cexp_qual have = f[CEXP_ORIG].data ? CEXP_ORIG : CEXP_FIXED;

                cexp_file_name(p, o, have, a, sizeof a);
                return seterr(err, errlen, -2, "%s/%s: without its %s (the record writes "
                              "NAME.orig.%s, the update with FIX NAME.fixed-N.%s)",
                              s->pass ? s->pass : ".", a,
                              have == CEXP_ORIG ? "[fixed #N] file" : "[orig] file",
                              obs_names[o], obs_names[o]);
            }
        }
    }
    return 0;
}

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL, *t;
    size_t n = 0, cap = 0, r;

    if (!f)
        return NULL;
    for (;;) {
        if (n == cap) {
            cap = cap ? 2 * cap : 4096;
            if (!(t = realloc(d, cap))) {
                free(d);
                fclose(f);
                return NULL;
            }
            d = t;
        }
        if (!(r = fread(d + n, 1, cap - n, f)))
            break;
        n += r;
    }
    if (ferror(f)) {
        free(d);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *len = n;
    return d;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* the names in dir (not beginning with '.'), sorted; -1 = out of memory, -3 = unreadable */
static int list_dir(const char *dir, char ***names, size_t *n, int dirs)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    char **v = NULL, **t;
    size_t k = 0, cap = 0;

    *names = NULL;
    *n = 0;
    if (!d)
        return -3;
    while ((e = readdir(d)) != NULL) {
        char path[4096];
        struct stat st;

        if (e->d_name[0] == '.')
            continue;
        snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        if (stat(path, &st) || !S_ISDIR(st.st_mode) != !dirs)
            continue;
        if (k == cap) {
            cap = cap ? 2 * cap : 64;
            if (!(t = realloc(v, cap * sizeof *v)))
                goto oom;
            v = t;
        }
        if (!(v[k] = strdup(e->d_name)))
            goto oom;
        k++;
    }
    closedir(d);
    qsort(v, k, sizeof *v, cmp_str);
    *names = v;
    *n = k;
    return 0;
oom:
    closedir(d);
    while (k)
        free(v[--k]);
    free(v);
    return -1;
}

static void free_names(char **v, size_t n)
{
    while (n)
        free(v[--n]);
    free(v);
}

int cexp_load(struct cexp_set *s, const char *dir, unsigned flags, char *err, size_t errlen)
{
    const char *base = strrchr(dir, '/') ? strrchr(dir, '/') + 1 : dir;
    char **names;
    size_t n, i;
    int r;
    struct stat st;

    memset(s, 0, sizeof *s);
    s->run = !!(flags & CEXP_RUN);
    if (!(s->pass = strdup(base)))
        return seterr(err, errlen, -1, "out of memory");
    if ((flags & CEXP_MISSING_OK) && stat(dir, &st) && errno == ENOENT)
        return 0;
    if ((r = list_dir(dir, &names, &n, 0)) != 0)
        return seterr(err, errlen, r, r == -1 ? "out of memory" : "%s: cannot read the directory",
                      dir);
    for (i = 0; i < n && !r; i++) {
        char path[4096];
        size_t len = 0;
        uint8_t *d;

        snprintf(path, sizeof path, "%s/%s", dir, names[i]);
        if (!(d = read_all(path, &len)))
            r = seterr(err, errlen, -3, "%s: cannot read it", path);
        else {
            char msg[400];

            if ((r = cexp_add(s, names[i], d, len, msg, sizeof msg)) != 0)
                seterr(err, errlen, r, "%s/%s", dir, msg);
        }
        free(d);
    }
    free_names(names, n);
    if (!r && (r = cexp_check(s, flags & CEXP_STRICT, err, errlen)) != 0) {
        char msg[700];

        /* the names as a path: DIR/... instead of PASS/... */
        snprintf(msg, sizeof msg, "%s", err);
        seterr(err, errlen, r, "%.*s%s", (int)(base - dir), dir, msg);
    }
    return r;
}

void cexp_free(struct cexp_set *s)
{
    size_t i;
    int o, q;

    for (i = 0; i < s->n; i++) {
        for (o = 0; o < CEXP_NOBS; o++)
            for (q = 0; q < CEXP_NQUAL; q++)
                free(s->p[i].f[o][q].data);
        free(s->p[i].name);
    }
    free(s->p);
    free(s->pass);
    memset(s, 0, sizeof *s);
}

int cexp_has(const struct cexp_prog *p, enum cexp_obs o)
{
    return p && (p->f[o][CEXP_PLAIN].data || p->f[o][CEXP_ORIG].data || p->f[o][CEXP_FIXED].data);
}

size_t cexp_applies(const struct cexp_prog *p, enum cexp_obs o, enum cexp_side side,
                    enum cexp_qual q[2])
{
    size_t n = 0;

    if (!p)
        return 0;
    if (p->f[o][CEXP_PLAIN].data) {
        q[n++] = CEXP_PLAIN;
        return n;
    }
    if (side != CEXP_SIDE_ORIG && p->f[o][CEXP_FIXED].data)
        q[n++] = CEXP_FIXED;
    if (side != CEXP_SIDE_PORT && p->f[o][CEXP_ORIG].data)
        q[n++] = CEXP_ORIG;
    return n;
}

int cexp_judge(const struct cexp_prog *p, enum cexp_obs o, enum cexp_side side,
               const uint8_t *got, size_t glen, enum cexp_qual *matched)
{
    enum cexp_qual q[2];
    size_t n = cexp_applies(p, o, side, q), i;

    if (!n)
        return -1;
    for (i = 0; got && i < n; i++) {
        const struct cexp_file *f = &p->f[o][q[i]];

        if (f->len == glen && !memcmp(f->data, got, glen)) {
            if (matched)
                *matched = q[i];
            return 0;
        }
    }
    return 1;
}

/* one line (without its LF) as "\"...\"", at most about 60 characters */
static void quote_line(char *dst, size_t len, const uint8_t *s, size_t n)
{
    size_t i, o = 0;

    if (o + 1 < len)
        dst[o++] = '"';
    for (i = 0; i < n && o + 8 < len; i++) {
        if (i == 60) {
            o += (size_t)snprintf(dst + o, len - o, "...");
            break;
        }
        if (s[i] >= 32 && s[i] < 127 && s[i] != '"' && s[i] != '\\')
            dst[o++] = (char)s[i];
        else
            o += (size_t)snprintf(dst + o, len - o, "\\x%02X", s[i]);
    }
    if (o + 1 < len)
        dst[o++] = '"';
    dst[o < len ? o : len - 1] = 0;
}

int cexp_diff(const uint8_t *want, size_t wlen, const uint8_t *got, size_t glen, char *what,
              size_t len)
{
    size_t wp = 0, gp = 0, line = 1;
    char a[300], b[300];

    if (!got)
        glen = 0;
    if (got && wlen == glen && !memcmp(want, got, wlen))
        return 0;
    if (!got) {
        snprintf(what, len, "no output");
        return 1;
    }
    for (;;) {
        const uint8_t *we = wp < wlen ? memchr(want + wp, '\n', wlen - wp) : NULL;
        const uint8_t *ge = gp < glen ? memchr(got + gp, '\n', glen - gp) : NULL;
        /* a line ends at its LF or at the end of the data */
        size_t wn = we ? (size_t)(we - (want + wp)) : wlen - wp;
        size_t gn = ge ? (size_t)(ge - (got + gp)) : glen - gp;
        int wend = wp >= wlen, gend = gp >= glen;

        if (wend || gend || wn != gn || memcmp(want + wp, got + gp, wn) || !we != !ge) {
            if (wend)
                snprintf(a, sizeof a, "the end");
            else
                quote_line(a, sizeof a, want + wp, wn);
            if (gend)
                snprintf(b, sizeof b, "the end");
            else
                quote_line(b, sizeof b, got + gp, gn);
            /* the same text, only the LF at the end of the data differs */
            snprintf(what, len, "line %zu: expected %s%s, got %s%s", line, a,
                     !wend && !we ? " (no LF)" : "", b, !gend && !ge ? " (no LF)" : "");
            return 1;
        }
        wp += wn + 1;
        gp += gn + 1;
        line++;
    }
}

void cexp_exe_hash(const uint8_t *exe, size_t len, char out[CEXP_HASHLEN + 1])
{
    struct sha256 s;
    uint8_t d[32];
    int i;

    sha256_init(&s);
    sha256_update(&s, exe, len);
    sha256_final(&s, d);
    for (i = 0; i < 32; i++)
        snprintf(out + 2 * i, 3, "%02x", d[i]);
    out[64] = '\n';
    out[65] = 0;
}

/* --- the plan ------------------------------------------------------------ */

void cexp_plan_free(struct cexp_plan *pl)
{
    size_t i;

    for (i = 0; i < pl->n; i++) {
        free(pl->op[i].name);
        free(pl->op[i].data);
    }
    free(pl->op);
    memset(pl, 0, sizeof *pl);
}

/* write name (data, len) or, with data NULL, remove it; -1 = out of memory */
static int plan_add(struct cexp_plan *pl, const char *name, const uint8_t *data, size_t len)
{
    struct cexp_op *op;

    if (pl->n == pl->cap) {
        size_t cap = pl->cap ? 2 * pl->cap : 16;

        if (!(op = realloc(pl->op, cap * sizeof *op)))
            return -1;
        pl->op = op;
        pl->cap = cap;
    }
    op = &pl->op[pl->n];
    if (!(op->name = strdup(name)))
        return -1;
    op->data = NULL;
    op->len = len;
    if (data && !(op->data = malloc(len ? len : 1))) {
        free(op->name);
        return -1;
    }
    if (data)
        memcpy(op->data, data, len);
    pl->n++;
    return 0;
}

static int plan_write(struct cexp_plan *pl, const struct cexp_prog *p, enum cexp_obs o,
                      enum cexp_qual q, unsigned issue, const uint8_t *data, size_t len)
{
    char name[160];
    struct cexp_prog t = *p;

    t.issue[o] = issue;
    cexp_file_name(&t, o, q, name, sizeof name);
    return plan_add(pl, name, data, len);
}

int cexp_plan_apply(const struct cexp_plan *pl, const char *dir, FILE *report, char *err,
                    size_t errlen)
{
    size_t i;

    if (pl->n && mkdir(dir, 0777) && errno != EEXIST)
        return seterr(err, errlen, -1, "%s: cannot create the directory", dir);
    for (i = 0; i < pl->n; i++) {
        const struct cexp_op *op = &pl->op[i];
        char path[4096];
        size_t olen = 0;
        uint8_t *old;

        snprintf(path, sizeof path, "%s/%s", dir, op->name);
        if (!op->data) {
            if (unlink(path) && errno != ENOENT)
                return seterr(err, errlen, -1, "%s: cannot remove it", path);
            if (report)
                fprintf(report, "removed %s\n", path);
            continue;
        }
        old = read_all(path, &olen);
        if (old && olen == op->len && !memcmp(old, op->data, olen)) {
            free(old);
            continue;
        }
        free(old);
        {
            FILE *f = fopen(path, "wb");

            if (!f || fwrite(op->data, 1, op->len, f) != op->len || fclose(f))
                return seterr(err, errlen, -1, "%s: cannot write it", path);
        }
        if (report)
            fprintf(report, "wrote %s\n", path);
    }
    return 0;
}

/* --- record and update ------------------------------------------------- */

static int named(const char *name, const char *const *only, size_t nonly)
{
    size_t i;

    if (!nonly)
        return 1;
    for (i = 0; i < nonly; i++)
        if (!strcmp(only[i], name))
            return 1;
    return 0;
}

int cexp_accept(const struct cexp_set *run1, const struct cexp_set *run2,
                const struct cexp_set *old, const char *const *only, size_t nonly,
                struct cexp_plan *pl, FILE *report)
{
    const char *pass = old && old->pass ? old->pass : run1->pass ? run1->pass : "?";
    size_t i, k;
    int rc = 0;

    for (k = 0; k < nonly; k++)
        if (!cexp_find(run1, only[k]) && !cexp_find(run2, only[k])) {
            fprintf(report, "%s:%s: not in the runs\n", pass, only[k]);
            rc = 1;
        }
    /* every program of run 1 (the ones only run 2 has: below) */
    for (i = 0; i < run1->n; i++) {
        const struct cexp_prog *a = &run1->p[i], *b, *op;
        const char *name = a->name;
        struct cexp_plan mine = { 0 };
        int refused = 0;
        enum cexp_obs o;

        if (!named(name, only, nonly))
            continue;
        b = cexp_find(run2, name);
        op = old ? cexp_find(old, name) : NULL;
        if (!b) {
            fprintf(report, "%s:%s: not in run 2\n", pass, name);
            rc = 1;
            continue;
        }
        for (o = 0; o < CEXP_NOBS; o++) {
            const struct cexp_file *fa = &a->f[o][CEXP_PLAIN], *fb = &b->f[o][CEXP_PLAIN];
            char what[700];

            if (!fa->data && !fb->data)
                continue;
            if (!fa->data || !fb->data || cexp_diff(fa->data, fa->len, fb->data, fb->len, what,
                                                    sizeof what)) {
                if (!refused)
                    fprintf(report, "test not deterministic: %s:%s\n", pass, name);
                if (!fa->data || !fb->data)
                    fprintf(report, "%s:%s: %s.%s only in run %d\n", pass, name, name,
                            obs_names[o], fa->data ? 1 : 2);
                else if (o == CEXP_EXE)
                    fprintf(report, "%s:%s: %s.exe: the executables differ\n", pass, name, name);
                else
                    fprintf(report, "%s:%s: %s.%s %s (run 1 expected, run 2 got)\n", pass, name,
                            name, obs_names[o], what);
                refused = 1;
            }
        }
        if (!refused && !a->f[CEXP_OUT][CEXP_PLAIN].data) {
            fprintf(report, "%s:%s: no output (it does not compile or did not run)\n", pass,
                    name);
            refused = 1;
        }
        if (!refused && cexp_has(op, CEXP_SERIAL) && !a->f[CEXP_SERIAL][CEXP_PLAIN].data) {
            fprintf(report, "%s:%s: no serial data\n", pass, name);
            refused = 1;
        }
        for (o = 0; !refused && o < CEXP_NOBS; o++) {
            const struct cexp_file *f = &a->f[o][CEXP_PLAIN];
            int fixed = op && op->f[o][CEXP_FIXED].data;

            if (!f->data)
                continue;
            if (o == CEXP_EXE && !fixed)
                continue;       /* no exe pair: ref and port are compared live */
            if (fixed) {
                if (same_file(f, &op->f[o][CEXP_FIXED])) {
                    char fn[160];

                    cexp_file_name(op, o, CEXP_FIXED, fn, sizeof fn);
                    fprintf(report, "%s:%s: the original gives %s (the pair would be equal: "
                            "make it %s.%s)\n", pass, name, fn, name, obs_names[o]);
                    refused = 1;
                    break;
                }
                if (plan_write(&mine, op, o, CEXP_ORIG, 0, f->data, f->len))
                    goto oom;
            } else if (plan_write(&mine, a, o, CEXP_PLAIN, 0, f->data, f->len))
                goto oom;
        }
        if (refused) {
            rc = 1;
            cexp_plan_free(&mine);
            continue;
        }
        for (k = 0; k < mine.n; k++)
            if (plan_add(pl, mine.op[k].name, mine.op[k].data, mine.op[k].len))
                goto oom;
        cexp_plan_free(&mine);
        continue;
oom:
        cexp_plan_free(&mine);
        return -1;
    }
    for (i = 0; i < run2->n; i++)
        if (named(run2->p[i].name, only, nonly) && !cexp_find(run1, run2->p[i].name)) {
            fprintf(report, "%s:%s: not in run 1\n", pass, run2->p[i].name);
            rc = 1;
        }
    return rc;
}

int cexp_parse_fix(const char *arg, struct cexp_fix *fix, char *err, size_t errlen)
{
    const char *colon = strchr(arg, ':'), *hash = strrchr(arg, '#');
    unsigned long n = 0;
    const char *p;

    memset(fix, 0, sizeof *fix);
    if (!colon || !hash || hash < colon || !name_ok(arg, (size_t)(colon - arg))
        || !name_ok(colon + 1, (size_t)(hash - colon - 1)) || hash[1] < '1' || hash[1] > '9')
        return seterr(err, errlen, -1, "%s: not PASS:NAME#N", arg);
    for (p = hash + 1; *p >= '0' && *p <= '9' && n < 100000000; p++)
        n = 10 * n + (unsigned long)(*p - '0');
    if (*p)
        return seterr(err, errlen, -1, "%s: not PASS:NAME#N", arg);
    memcpy(fix->pass, arg, (size_t)(colon - arg));
    memcpy(fix->name, colon + 1, (size_t)(hash - colon - 1));
    fix->issue = (unsigned)n;
    return 0;
}

/* the issue a fix gives the program name of pass, 0 = none */
static unsigned fix_of(const struct cexp_fix *fix, size_t nfix, const char *pass,
                       const char *name)
{
    size_t i;

    for (i = 0; i < nfix; i++)
        if (!strcmp(fix[i].pass, pass) && !strcmp(fix[i].name, name))
            return fix[i].issue;
    return 0;
}

int cexp_update(const struct cexp_set *run, const struct cexp_set *old, int serial,
                const struct cexp_fix *fix, size_t nfix, struct cexp_plan *pl, FILE *report)
{
    const char *pass = old && old->pass ? old->pass : run->pass ? run->pass : "?";
    size_t i, k;
    int rc = 0;

    for (i = 0; i < run->n; i++) {
        const struct cexp_prog *a = &run->p[i], *op = old ? cexp_find(old, a->name) : NULL;
        unsigned issue = fix_of(fix, nfix, pass, a->name);
        struct cexp_plan mine = { 0 };
        int refused = 0, changed = 0;
        enum cexp_obs o;

        for (o = 0; o < (serial ? 2 : 1); o++) {
            const struct cexp_file *f = &a->f[o][CEXP_PLAIN];
            char what[700], fn[160];

            if (!f->data) {
                fprintf(report, "%s:%s: no %s (%s)\n", pass, a->name,
                        o == CEXP_OUT ? "output" : "serial data",
                        o == CEXP_OUT ? "it does not compile or did not run" : "the run sent none");
                refused = 1;
                break;
            }
            if (!cexp_has(op, o)) {
                if (plan_write(&mine, a, o, CEXP_PLAIN, 0, f->data, f->len))
                    goto oom;
                changed = 1;
                continue;
            }
            if (op->f[o][CEXP_PLAIN].data) {
                const struct cexp_file *pf = &op->f[o][CEXP_PLAIN];

                if (!cexp_diff(pf->data, pf->len, f->data, f->len, what, sizeof what))
                    continue;
                cexp_file_name(op, o, CEXP_PLAIN, fn, sizeof fn);
                if (!issue) {
                    fprintf(report, "%s:%s: %s changed, %s: name the fix with FIX=%s:%s#N "
                            "(the pair %s.orig.%s / %s.fixed-N.%s), nothing written\n", pass,
                            a->name, fn, what, pass, a->name, a->name, obs_names[o], a->name,
                            obs_names[o]);
                    refused = 1;
                    break;
                }
                if (plan_write(&mine, op, o, CEXP_ORIG, 0, pf->data, pf->len)
                    || plan_write(&mine, op, o, CEXP_FIXED, issue, f->data, f->len)
                    || plan_add(&mine, fn, NULL, 0))
                    goto oom;
                changed = 1;
                continue;
            }
            /* a pair (or a lone [fixed #N] file): the port's side */
            if (issue && op->f[o][CEXP_FIXED].data && op->issue[o] != issue) {
                cexp_file_name(op, o, CEXP_FIXED, fn, sizeof fn);
                fprintf(report, "%s:%s: FIX names #%u, but the program has %s\n", pass, a->name,
                        issue, fn);
                refused = 1;
                break;
            }
            if (same_file(f, &op->f[o][CEXP_ORIG])) {
                cexp_file_name(op, o, CEXP_ORIG, fn, sizeof fn);
                fprintf(report, "%s:%s: the port gives %s again (the pair would be equal: "
                        "a regression, or make it %s.%s by hand)\n", pass, a->name, fn, a->name,
                        obs_names[o]);
                refused = 1;
                break;
            }
            if (!op->f[o][CEXP_FIXED].data && !issue) {
                fprintf(report, "%s:%s: %s.orig.%s has no [fixed #N] file: name the fix with "
                        "FIX=%s:%s#N\n", pass, a->name, a->name, obs_names[o], pass, a->name);
                refused = 1;
                break;
            }
            if (same_file(f, &op->f[o][CEXP_FIXED]))
                continue;
            if (plan_write(&mine, op, o, CEXP_FIXED,
                           op->f[o][CEXP_FIXED].data ? op->issue[o] : issue, f->data, f->len))
                goto oom;
            changed = 1;
        }
        if (refused) {
            rc = 1;
            cexp_plan_free(&mine);
            continue;
        }
        if (issue && !changed)
            fprintf(report, "%s:%s: FIX #%u: the output is unchanged (no output pair)\n", pass,
                    a->name, issue);
        for (k = 0; k < mine.n; k++)
            if (plan_add(pl, mine.op[k].name, mine.op[k].data, mine.op[k].len))
                goto oom;
        cexp_plan_free(&mine);
        continue;
oom:
        cexp_plan_free(&mine);
        return -1;
    }
    for (k = 0; k < nfix; k++)
        if (!strcmp(fix[k].pass, pass) && !cexp_find(run, fix[k].name)) {
            fprintf(report, "%s:%s: FIX names a program the run does not have\n", pass,
                    fix[k].name);
            rc = 1;
        }
    return rc;
}

int cexp_update_exe(const struct cexp_set *old, const char *name, const char *ref,
                    const char *port, const struct cexp_fix *fix, size_t nfix,
                    struct cexp_plan *pl, FILE *report)
{
    const char *pass = old && old->pass ? old->pass : "?";
    const struct cexp_prog *op = old ? cexp_find(old, name) : NULL;
    unsigned issue = fix_of(fix, nfix, pass, name);
    struct cexp_prog t;

    memset(&t, 0, sizeof t);
    t.name = (char *)name;
    if (op && op->f[CEXP_EXE][CEXP_FIXED].data) {
        const struct cexp_file *of = &op->f[CEXP_EXE][CEXP_ORIG];

        if (issue && issue != op->issue[CEXP_EXE]) {
            fprintf(report, "%s:%s: FIX names #%u, but the program has %s.fixed-%u.exe\n", pass,
                    name, issue, name, op->issue[CEXP_EXE]);
            return 1;
        }
        if (!port) {
            fprintf(report, "%s:%s: no executable of port\n", pass, name);
            return 1;
        }
        if (of->data && !memcmp(of->data, port, CEXP_HASHLEN)) {
            fprintf(report, "%s:%s: the port's executable is %s.orig.exe again (the pair would "
                    "be equal: a regression, or remove the pair by hand)\n", pass, name, name);
            return 1;
        }
        if (ref && of->data && memcmp(of->data, ref, CEXP_HASHLEN))
            fprintf(report, "%s:%s: the original's executable is not %s.orig.exe "
                    "(corpus-expect-record writes it)\n", pass, name, name);
        if (!of->data && ref && plan_write(pl, &t, CEXP_EXE, CEXP_ORIG, 0, (const uint8_t *)ref,
                                           CEXP_HASHLEN))
            return -1;
        return plan_write(pl, &t, CEXP_EXE, CEXP_FIXED, op->issue[CEXP_EXE],
                          (const uint8_t *)port, CEXP_HASHLEN) ? -1 : 0;
    }
    if (!issue)
        return 0;
    if (!ref || !port) {
        fprintf(report, "%s:%s: FIX #%u: no executable of %s\n", pass, name, issue,
                ref ? "port" : "ref");
        return 1;
    }
    if (!memcmp(ref, port, CEXP_HASHLEN)) {
        fprintf(report, "%s:%s: FIX #%u: the executables of ref and port agree (no exe pair)\n",
                pass, name, issue);
        return 0;
    }
    if (plan_write(pl, &t, CEXP_EXE, CEXP_ORIG, 0, (const uint8_t *)ref, CEXP_HASHLEN)
        || plan_write(pl, &t, CEXP_EXE, CEXP_FIXED, issue, (const uint8_t *)port, CEXP_HASHLEN))
        return -1;
    return 0;
}

/* --- fix-scope --------------------------------------------------------- */

static uint32_t fnv1a(const uint8_t *s, size_t n)
{
    uint32_t h = 0x811C9DC5u;

    while (n--)
        h = (h ^ *s++) * 0x01000193u;
    return h;
}

void cexp_project(FILE *out, const struct cexp_set *s)
{
    static const char *const qual[CEXP_NQUAL] = { "", " [orig]", " [fixed #" };
    size_t i;

    for (i = 0; i < s->n; i++) {
        const struct cexp_prog *p = &s->p[i];
        enum cexp_obs o;
        enum cexp_qual q;

        for (o = 0; o < CEXP_NOBS; o++)
            for (q = 0; q < CEXP_NQUAL; q++) {
                const struct cexp_file *f = &p->f[o][q];

                if (!f->data)
                    continue;
                fprintf(out, "%s|%s|%s|1%s", s->pass, p->name, obs_names[o], qual[q]);
                if (q == CEXP_FIXED)
                    fprintf(out, "%u]", p->issue[o]);
                if (o == CEXP_EXE)
                    fprintf(out, "\tsha256 %.64s\n", (const char *)f->data);
                else
                    fprintf(out, "\tbytes %zu fnv %08lX\n", f->len,
                            (unsigned long)fnv1a(f->data, f->len));
            }
    }
}

int cexp_project_tree(const char *root, char **text, size_t *len, char *err, size_t errlen)
{
    char **names = NULL;
    size_t n = 0, i;
    FILE *out;
    int r = 0;
    struct stat st;

    *text = NULL;
    *len = 0;
    if (!(out = open_memstream(text, len)))
        return seterr(err, errlen, -1, "out of memory");
    fprintf(out, "# capture corpus 1\n");
    if (!(stat(root, &st) && errno == ENOENT) && (r = list_dir(root, &names, &n, 1)) != 0)
        seterr(err, errlen, r, r == -1 ? "out of memory" : "%s: cannot read the directory", root);
    for (i = 0; i < n && !r; i++) {
        struct cexp_set s;
        char dir[4096];

        snprintf(dir, sizeof dir, "%s/%s", root, names[i]);
        if (!name_ok(names[i], strlen(names[i])))
            r = seterr(err, errlen, -2, "%s: the pass's name is not of [A-Za-z0-9_-]", dir);
        else if ((r = cexp_load(&s, dir, CEXP_STRICT, err, errlen)) == 0)
            cexp_project(out, &s);
        cexp_free(&s);
    }
    free_names(names, n);
    if (fclose(out) && !r)
        r = seterr(err, errlen, -1, "out of memory");
    if (r) {
        free(*text);
        *text = NULL;
        *len = 0;
    }
    return r;
}

int cexp_fix_scope_trees(const char *base_root, const char *branch_root,
                         const struct libscmp_issues *is, FILE *out, FILE *err)
{
    struct libscmp_file b, r;
    char *tb = NULL, *tr = NULL, msg[700];
    size_t lb, lr, nch = 0, nrep, i;
    int st;

    memset(&b, 0, sizeof b);
    memset(&r, 0, sizeof r);
    if (cexp_project_tree(base_root, &tb, &lb, msg, sizeof msg)
        || cexp_project_tree(branch_root, &tr, &lr, msg, sizeof msg)
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
    fprintf(out, "fix-scope: corpus: %zu cases changed, %zu not of the issues", nch, nrep);
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
