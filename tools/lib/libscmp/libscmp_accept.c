/*
 * libscmp_accept.c - writing expected data of the library tests: accepting
 * two recording runs of the original (libs-accept) and taking the
 * [fixed #N] values from a run of the port (libs-update); libscmp.h.
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "libscmp.h"
#include "libscmp_int.h"

#define KEY(f, c) (int)(c)->keylen, (f)->text + (c)->key
#define VAL(f, c) (int)(c)->vallen, (f)->text + (c)->val

static const struct libscmp_entry *find_case(const struct libscmp_file *f,
                                             const struct libscmp_file *g,
                                             const struct libscmp_case *c)
{
    return libscmp_find_h(f, g->text + c->key, c->keylen, g->entries[c->entry].hash);
}

/* KEY QUAL<TAB>VALUE; qual an enum libscmp_qual, issue its N ([orig #N]:
   N from 1, [orig]: 0) */
static void put_case(FILE *o, const char *key, size_t keylen, int qual, uint32_t issue,
                     const char *val, size_t vallen)
{
    struct libscmp_case c = { 0 };
    char q[32];

    c.qual = (uint8_t)qual;
    c.issue = issue;
    fprintf(o, "%.*s%s\t%.*s\n", (int)keylen, key, libscmp_qual_text(&c, q, sizeof q),
            (int)vallen, val);
}

#define PLAIN LIBSCMP_PLAIN
#define ORIG LIBSCMP_QORIG
#define FIXED LIBSCMP_QFIXED

static int is_lost(const char *v, uint32_t n)
{
    return (n == 4 && !memcmp(v, "lost", 4)) || (n > 4 && !memcmp(v, "lost ", 5));
}

/* the cases in which two runs differ, under "test not deterministic" */
static void nondeterministic(const struct libscmp_file *a, const struct libscmp_file *b, FILE *r)
{
    size_t i, shown = 0;
    uint32_t block = 0;

    fprintf(r, "test not deterministic: %s\n", a->name);
#define SHOW(...) do { \
        if (++shown <= LIBSCMP_DETAILS) \
            fprintf(r, __VA_ARGS__); \
    } while (0)
    for (i = 0; i < a->ncases; i++) {
        const struct libscmp_case *c = &a->cases[i];
        const struct libscmp_entry *e = find_case(b, a, c);

        if (!e)
            SHOW("%s: %.*s: only in run 1 (%.*s)\n", a->name, KEY(a, c), VAL(a, c));
        else {
            const struct libscmp_case *d = &b->cases[e->plain];

            if (d->vallen != c->vallen || memcmp(b->text + d->val, a->text + c->val, c->vallen)) {
                SHOW("%s: %.*s: run 1 %.*s, run 2 %.*s\n", a->name, KEY(a, c), VAL(a, c), VAL(b, d));
                if (!block && c->keylen > 6 && !memcmp(a->text + c->key, "BLOCK:", 6))
                    block = (uint32_t)strtoul(a->text + c->key + 6, NULL, 10);
            }
        }
    }
    for (i = 0; i < b->ncases; i++) {
        const struct libscmp_case *d = &b->cases[i];

        if (!find_case(a, b, d))
            SHOW("%s: %.*s: only in run 2 (%.*s)\n", a->name, KEY(b, d), VAL(b, d));
    }
#undef SHOW
    if (!shown)
        fprintf(r, "%s: the same cases in another order\n", a->name);
    else if (shown > LIBSCMP_DETAILS)
        fprintf(r, "%s: ... %zu more\n", a->name, shown - LIBSCMP_DETAILS);
    /* the first differing block: its first differing observation */
    if (block && a->bin && b->bin) {
        char msg[700];
        int x = libscmp_bin_first_diff(a, b, block, "run 1", "run 2", r, msg, sizeof msg);

        if (x < 0)
            fprintf(r, "%s\n", x == -1 ? "out of memory" : msg);
    }
}

/* the observations "lost" in the blocks of run (its NAME.bin), reported;
   their number, -1 = out of memory or a malformed file (reported) */
static long lost_in_blocks(const struct libscmp_file *run, size_t shown, FILE *report)
{
    struct libscmp_obs o;
    char msg[700];
    long n = 0;
    int ev;

    if (!run->bin)
        return 0;
    if (libscmp_obs_begin(&o, run->bin, run->binlen, run->binpath, msg, sizeof msg)) {
        fprintf(report, "%s\n", msg);
        return -1;
    }
    while ((ev = libscmp_obs_next(&o, msg, sizeof msg)) > 0)
        if (ev == LIBSCMP_OBS && o.type == 0x15 && o.lost) {
            n++;
            if (shown + (size_t)n <= LIBSCMP_DETAILS)
                fprintf(report, "%s: %s (BLOCK:%lu): %s: the original does not keep a stated "
                        "register (the stated set is too strict)\n", run->name, o.key,
                        (unsigned long)o.block, o.val);
        }
    if (ev < 0) {
        fprintf(report, "%s\n", ev == -1 ? "out of memory" : msg);
        n = -1;
    }
    libscmp_obs_free(&o);
    return n;
}

/* the text written to o (a memstream on *text) in the order of
   libscmp_sorted, into *out; 0 = ok, 1 = refused (the text is malformed:
   reported), -1 = out of memory */
static int close_sorted(FILE *o, char **text, size_t *len, char **out, FILE *report)
{
    char msg[700];
    int r;

    if (fclose(o)) {
        free(*text);
        return -1;
    }
    r = libscmp_sort_text(*text, *len, "the new expected data", out, msg, sizeof msg);
    free(*text);
    if (r == -2) {
        fprintf(report, "%s\n", msg);
        return 1;
    }
    return r;
}

int libscmp_accept(const struct libscmp_file *run1, const struct libscmp_file *run2,
                   const struct libscmp_file *old, char **out, FILE *report)
{
    size_t i, nlost = 0, nfixed = 0;
    char *text = NULL;
    size_t len;
    FILE *o;
    int r;

    *out = NULL;
    if (run1->len != run2->len || memcmp(run1->text, run2->text, run1->len)) {
        nondeterministic(run1, run2, report);
        return 1;
    }
    for (i = 0; i < run1->ncases; i++) {
        const struct libscmp_case *c = &run1->cases[i];

        if (is_lost(run1->text + c->val, c->vallen)) {
            if (++nlost <= LIBSCMP_DETAILS)
                fprintf(report, "%s: %.*s: %.*s: the original does not keep a stated register "
                        "(the stated set is too strict)\n", run1->name, KEY(run1, c), VAL(run1, c));
        }
    }
    {
        long n = lost_in_blocks(run1, nlost, report);

        if (n < 0)
            return 1;
        nlost += (size_t)n;
    }
    if (nlost) {
        if (nlost > LIBSCMP_DETAILS)
            fprintf(report, "%s: ... %zu more\n", run1->name, nlost - LIBSCMP_DETAILS);
        fprintf(report, "%s: refused: %zu cases lost\n", run1->name, nlost);
        return 1;
    }
    if (!(o = open_memstream(&text, &len)))
        return -1;
    fprintf(o, "# capture %s 1\n", run1->name);
    for (i = 0; i < run1->ncases; i++) {
        const struct libscmp_case *c = &run1->cases[i];
        const struct libscmp_entry *e = old ? find_case(old, run1, c) : NULL;
        const char *key = run1->text + c->key, *val = run1->text + c->val;

        if (e && e->fixed >= 0) {
            const struct libscmp_case *f = &old->cases[e->fixed];

            put_case(o, key, c->keylen, ORIG, 0, val, c->vallen);
            put_case(o, key, c->keylen, FIXED, f->issue, old->text + f->val, f->vallen);
            nfixed++;
        }
        else if (e && e->orig >= 0)    /* [orig] or [orig #N] alone: kept */
            put_case(o, key, c->keylen, ORIG, old->cases[e->orig].issue, val, c->vallen);
        else
            put_case(o, key, c->keylen, PLAIN, 0, val, c->vallen);
    }
    /* the cases of old whose key the run lacks */
    for (i = 0; old && i < old->ncases; i++) {
        const struct libscmp_case *c = &old->cases[i];

        if (find_case(run1, old, c))
            continue;
        if (c->qual == LIBSCMP_QFIXED) {
            put_case(o, old->text + c->key, c->keylen, FIXED, c->issue, old->text + c->val,
                     c->vallen);
            nfixed++;
            fprintf(report, "%s: %.*s [fixed #%lu]: not in the original's run, kept as a case "
                    "of the port only (did the scenario change?)\n", run1->name, KEY(old, c),
                    (unsigned long)c->issue);
        } else {
            char q[32];

            fprintf(report, "%s: %.*s%s: not in the original's run any more, removed\n",
                    run1->name, KEY(old, c), libscmp_qual_text(c, q, sizeof q));
        }
    }
    if ((r = close_sorted(o, &text, &len, out, report)))
        return r;
    fprintf(report, "%s: %zu cases of the original, %zu [fixed #N] cases kept\n", run1->name,
            run1->ncases, nfixed);
    return 0;
}

/* --- libs-update ------------------------------------------------------- */

int libscmp_parse_fix(const char *arg, struct libscmp_fix *fix, char *err, size_t errlen)
{
    const char *h = strrchr(arg, '#'), *p;
    struct libscmp_file f;
    char *text, msg[300];
    unsigned long n = 0;
    size_t len;
    int r;

    memset(fix, 0, sizeof *fix);
    if (!h || h == arg || !h[1] || h[1] == '0') {
        snprintf(err, errlen, "--fix %s: not KEY#N", arg);
        return -1;
    }
    for (p = h + 1; *p; p++) {
        if (*p < '0' || *p > '9' || p - h > 9) {
            snprintf(err, errlen, "--fix %s: not KEY#N (N an issue number)", arg);
            return -1;
        }
        n = 10 * n + (unsigned long)(*p - '0');
    }
    /* the key as a run's case line: its syntax, no qualifier */
    len = (size_t)(h - arg) + 32;
    if (!(text = malloc(len))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    snprintf(text, len, "# capture fix 1\n%.*s\tx\n", (int)(h - arg), arg);
    r = libscmp_parse(&f, text, strlen(text), "--fix", LIBSCMP_RUN, msg, sizeof msg);
    free(text);
    libscmp_free(&f);
    if (r) {
        snprintf(err, errlen, "--fix %s: %s", arg, r == -2 && strlen(msg) > 9 ? msg + 9 : msg);
        return -1;
    }
    fix->keylen = (size_t)(h - arg);
    if (!(fix->key = strndup(arg, fix->keylen))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    fix->issue = (uint32_t)n;
    return 0;
}

void libscmp_fix_free(struct libscmp_fix *fix)
{
    free(fix->key);
    memset(fix, 0, sizeof *fix);
}

static struct libscmp_fix *fix_of(struct libscmp_fix *fixes, size_t n, const char *key,
                                  size_t keylen)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (fixes[i].keylen == keylen && !memcmp(fixes[i].key, key, keylen))
            return &fixes[i];
    return NULL;
}

int libscmp_update(const struct libscmp_file *run, const struct libscmp_file *exp,
                   struct libscmp_fix *fixes, size_t nfixes, char **out, FILE *report)
{
    size_t i, j, nchanged = 0, nbad = 0;
    char *text = NULL;
    size_t len;
    FILE *o;
    int r;

    *out = NULL;
    if (!(o = open_memstream(&text, &len)))
        return -1;
    fprintf(o, "# capture %s 1\n", exp->name);
    for (i = 0; i < exp->ncases; i++) {
        const struct libscmp_case *c = &exp->cases[i];
        const char *key = exp->text + c->key;
        const struct libscmp_entry *re = find_case(run, exp, c);
        const struct libscmp_case *rc = re ? &run->cases[re->plain] : NULL;
        struct libscmp_fix *fx = fix_of(fixes, nfixes, key, c->keylen);
        int same = rc && rc->vallen == c->vallen
                   && !memcmp(run->text + rc->val, exp->text + c->val, c->vallen);

        if (c->qual == LIBSCMP_QFIXED) {
            if (!rc) {
                fprintf(report, "%s: %.*s [fixed #%lu]: not in the port's run\n", exp->name,
                        KEY(exp, c), (unsigned long)c->issue);
                nbad++;
                continue;
            }
            put_case(o, key, c->keylen, FIXED, c->issue, run->text + rc->val, rc->vallen);
            if (!same) {
                fprintf(report, "%s: %.*s [fixed #%lu]: %.*s, was %.*s\n", exp->name, KEY(exp, c),
                        (unsigned long)c->issue, VAL(run, rc), VAL(exp, c));
                nchanged++;
            }
            if (fx)
                fx->qualified++;
            continue;
        }
        if (c->qual == LIBSCMP_QORIG) {
            if (fx && !c->issue && exp->entries[c->entry].fixed < 0 && !rc) {
                /* [orig] alone, the port lacks the key: the fix's case */
                put_case(o, key, c->keylen, ORIG, fx->issue, exp->text + c->val, c->vallen);
                fprintf(report, "%s: %.*s [orig #%lu]: was [orig], a case of the original "
                        "only\n", exp->name, KEY(exp, c), (unsigned long)fx->issue);
                fx->applied++;
                nchanged++;
                continue;
            }
            put_case(o, key, c->keylen, ORIG, c->issue, exp->text + c->val, c->vallen);
            if (fx && exp->entries[c->entry].fixed < 0)
                fx->qualified++;
            continue;
        }
        if (fx && !same) {
            put_case(o, key, c->keylen, ORIG, rc ? 0 : fx->issue, exp->text + c->val, c->vallen);
            if (rc) {
                put_case(o, key, c->keylen, FIXED, fx->issue, run->text + rc->val, rc->vallen);
                fprintf(report, "%s: %.*s: the pair [orig] %.*s, [fixed #%lu] %.*s\n", exp->name,
                        KEY(exp, c), VAL(exp, c), (unsigned long)fx->issue, VAL(run, rc));
            } else
                fprintf(report, "%s: %.*s [orig #%lu]: not in the port's run, a case of the "
                        "original only\n", exp->name, KEY(exp, c), (unsigned long)fx->issue);
            fx->applied++;
            nchanged++;
            continue;
        }
        if (fx)
            fx->same++;
        else if (!same) {
            if (rc)
                fprintf(report, "%s: %.*s: the port gives %.*s, expected %.*s (not touched; "
                        "--fix KEY#N makes the pair)\n", exp->name, KEY(exp, c), VAL(run, rc),
                        VAL(exp, c));
            else
                fprintf(report, "%s: %.*s: not in the port's run (not touched)\n", exp->name,
                        KEY(exp, c));
        }
        put_case(o, key, c->keylen, PLAIN, 0, exp->text + c->val, c->vallen);
    }
    /* the fixes whose key only the run has */
    for (j = 0; j < nfixes; j++) {
        const struct libscmp_entry *re = libscmp_find(run, fixes[j].key, fixes[j].keylen);
        const struct libscmp_case *c;

        if (!re || libscmp_find(exp, fixes[j].key, fixes[j].keylen))
            continue;
        c = &run->cases[re->plain];
        put_case(o, run->text + c->key, c->keylen, FIXED, fixes[j].issue, run->text + c->val,
                 c->vallen);
        fprintf(report, "%s: %.*s [fixed #%lu]: added (%.*s), a case of the port only\n",
                exp->name, KEY(run, c), (unsigned long)fixes[j].issue, VAL(run, c));
        fixes[j].applied++;
        nchanged++;
    }
    if (nbad) {
        fclose(o);
        free(text);
        fprintf(report, "%s: refused: %zu [fixed #N] cases not in the port's run\n", exp->name, nbad);
        return 1;
    }
    if ((r = close_sorted(o, &text, &len, out, report)))
        return r;
    fprintf(report, "%s: %zu cases changed\n", exp->name, nchanged);
    return 0;
}

int libscmp_update_new(const struct libscmp_file *run, uint32_t issue, char **out, FILE *report)
{
    char *text = NULL;
    size_t i, len;
    FILE *o;
    int r;

    *out = NULL;
    if (!(o = open_memstream(&text, &len)))
        return -1;
    fprintf(o, "# capture %s 1\n", run->name);
    for (i = 0; i < run->ncases; i++) {
        const struct libscmp_case *c = &run->cases[i];

        put_case(o, run->text + c->key, c->keylen, FIXED, issue, run->text + c->val, c->vallen);
    }
    if ((r = close_sorted(o, &text, &len, out, report)))
        return r;
    fprintf(report, "%s: new, %zu cases [fixed #%lu], of the port only\n", run->name, run->ncases,
            (unsigned long)issue);
    return 0;
}

/* --- directories ------------------------------------------------------- */

/* text into path unless it holds it already; 0 = unchanged, 1 = written,
   -1 = error (on err) */
static int write_if_changed(const char *path, const char *text, FILE *err)
{
    size_t n = strlen(text), len;
    char *old = libscmp_read_file(path, &len);
    FILE *f;

    if (old && len == n && !memcmp(old, text, n)) {
        free(old);
        return 0;
    }
    free(old);
    if (!(f = fopen(path, "wb")) || fwrite(text, 1, n, f) != n || fclose(f)) {
        fprintf(err, "%s: cannot write it (%s)\n", path, strerror(errno));
        return -1;
    }
    return 1;
}

struct newfile {
    char *path, *text;
};

static int write_all(struct newfile *v, size_t n, FILE *out, FILE *err)
{
    size_t i, nwritten = 0;
    int status = 0;

    for (i = 0; i < n; i++) {
        int r = write_if_changed(v[i].path, v[i].text, err);

        if (r < 0)
            status = 2;
        else if (r) {
            fprintf(out, "written: %s\n", v[i].path);
            nwritten++;
        }
    }
    fprintf(out, "%zu files written, %zu unchanged\n", nwritten, n - nwritten);
    return status;
}

static void free_new(struct newfile *v, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        free(v[i].path);
        free(v[i].text);
    }
    free(v);
}

int libscmp_accept_dirs(const char *run1_dir, const char *run2_dir, const char *exp_dir,
                        const char *const *only, size_t nonly, FILE *out, FILE *err)
{
    struct libscmp_names names = { 0 }, expnames = { 0 };
    struct newfile *nv = NULL;
    size_t i, nn = 0;
    int status = 0;

    if (libscmp_program_names(&names, run1_dir, run2_dir, only, nonly, err)
        || (!nonly && libscmp_names_dir(&expnames, exp_dir, err))) {
        libscmp_names_free(&names);
        libscmp_names_free(&expnames);
        return 2;
    }
    for (i = 0; i < expnames.n; i++)
        if (libscmp_names_add(&names, expnames.v[i], strlen(expnames.v[i]))) {
            fprintf(err, "out of memory\n");
            status = 2;
        }
    libscmp_names_free(&expnames);
    libscmp_names_sort(&names);
    if (!status && !(nv = calloc(names.n + 1, sizeof *nv))) {
        fprintf(err, "out of memory\n");
        status = 2;
    }
    for (i = 0; i < names.n && status != 2; i++) {
        const char *name = names.v[i];
        char *p1 = libscmp_path(run1_dir, name), *p2 = libscmp_path(run2_dir, name);
        char *pe = libscmp_path(exp_dir, name);
        int h1, h2, he;

        if (!p1 || !p2 || !pe) {
            fprintf(err, "out of memory\n");
            free(p1);
            free(p2);
            free(pe);
            status = 2;
            break;
        }
        h1 = libscmp_exists(p1);
        h2 = libscmp_exists(p2);
        he = libscmp_exists(pe);
        if (!h1 || !h2) {
            if (!h1 && !h2)
                fprintf(out, he ? "%s: expected data, but no output of the runs (%s)\n"
                                : "%s: no output of the runs (%s)\n", name, p1);
            else
                fprintf(out, "test not deterministic: %s\n%s: no output of run %d (%s)\n", name,
                        name, h1 ? 2 : 1, h1 ? p2 : p1);
            if (!status)
                status = 1;
        } else {
            struct libscmp_file r1, r2, old;
            int a = libscmp_load_named(&r1, run1_dir, name, LIBSCMP_RUN, err);
            int b = libscmp_load_named(&r2, run2_dir, name, LIBSCMP_RUN, err);
            int c = he ? libscmp_load_named(&old, exp_dir, name, LIBSCMP_EXPECTED, err) : 0;

            if (!he)
                memset(&old, 0, sizeof old);
            if (a || b || c)
                status = 2;
            else {
                char *text;
                int r = libscmp_accept(&r1, &r2, he ? &old : NULL, &text, out);

                if (r < 0) {
                    fprintf(err, "out of memory\n");
                    status = 2;
                } else if (r) {
                    if (!status)
                        status = 1;
                } else {
                    nv[nn].path = pe;
                    nv[nn].text = text;
                    nn++;
                    pe = NULL;
                }
            }
            libscmp_free(&r1);
            libscmp_free(&r2);
            libscmp_free(&old);
        }
        free(p1);
        free(p2);
        free(pe);
    }
    if (!status)
        status = write_all(nv, nn, out, err);
    else
        fprintf(out, "libs-accept: nothing written\n");
    free_new(nv, nn);
    libscmp_names_free(&names);
    return status;
}

int libscmp_update_dirs(const char *run_dir, const char *exp_dir, const char *const *only,
                        size_t nonly, struct libscmp_fix *fixes, size_t nfixes,
                        uint32_t new_issue, FILE *out, FILE *err)
{
    struct libscmp_names names = { 0 };
    struct newfile *nv = NULL;
    size_t i, nn = 0;
    int status = 0;

    if (libscmp_program_names(&names, exp_dir, NULL, only, nonly, err)) {
        libscmp_names_free(&names);
        return 2;
    }
    if (!(nv = calloc(names.n + 1, sizeof *nv))) {
        fprintf(err, "out of memory\n");
        libscmp_names_free(&names);
        return 2;
    }
    for (i = 0; i < names.n && status != 2; i++) {
        const char *name = names.v[i];
        char *pr = libscmp_path(run_dir, name), *pe = libscmp_path(exp_dir, name);

        if (!pr || !pe) {
            fprintf(err, "out of memory\n");
            status = 2;
        } else if (!libscmp_exists(pe) && (!new_issue || !libscmp_exists(pr))) {
            if (new_issue)
                fprintf(err, "%s: no expected data (%s) and no output of the run (%s)\n", name,
                        pe, pr);
            else
                fprintf(err, "%s: no expected data (%s; record it with libs-capture, or --new N "
                        "for a program of the port only)\n", name, pe);
            status = 2;
        } else if (!libscmp_exists(pe)) {
            /* --new: every case of the port's run as [fixed #N] */
            struct libscmp_file run;

            if (libscmp_load_named(&run, run_dir, name, LIBSCMP_RUN, err))
                status = 2;
            else {
                char *text;
                int r = libscmp_update_new(&run, new_issue, &text, out);

                if (r < 0) {
                    fprintf(err, "out of memory\n");
                    status = 2;
                } else if (r) {
                    if (!status)
                        status = 1;
                } else {
                    nv[nn].path = pe;
                    nv[nn].text = text;
                    nn++;
                    pe = NULL;
                }
            }
            libscmp_free(&run);
        } else if (!libscmp_exists(pr)) {
            fprintf(out, "%s: expected data, but no output of the run (%s)\n", name, pr);
            if (!status)
                status = 1;
        } else {
            struct libscmp_file run, exp;
            int a = libscmp_load_named(&run, run_dir, name, LIBSCMP_RUN, err);
            int b = libscmp_load_named(&exp, exp_dir, name, LIBSCMP_EXPECTED, err);

            if (a || b)
                status = 2;
            else {
                char *text;
                int r = libscmp_update(&run, &exp, fixes, nfixes, &text, out);

                if (r < 0) {
                    fprintf(err, "out of memory\n");
                    status = 2;
                } else if (r) {
                    if (!status)
                        status = 1;
                } else {
                    nv[nn].path = pe;
                    nv[nn].text = text;
                    nn++;
                    pe = NULL;
                }
            }
            libscmp_free(&run);
            libscmp_free(&exp);
        }
        free(pr);
        free(pe);
    }
    for (i = 0; i < nfixes && status != 2; i++) {
        const struct libscmp_fix *f = &fixes[i];
        const char *why = NULL;

        if (f->applied > 1)
            why = "the key is in more than one program (name it with --only)";
        else if (f->applied)
            continue;
        else if (f->qualified)
            why = "the key is qualified already";
        else if (f->same)
            why = "the port gives the expected value (nothing to fix)";
        else
            why = "the key is in no program's expected data or run";
        fprintf(out, "--fix %.*s#%lu: %s\n", (int)f->keylen, f->key, (unsigned long)f->issue, why);
        if (!status)
            status = 1;
    }
    if (!status)
        status = write_all(nv, nn, out, err);
    else
        fprintf(out, "libs-update: nothing written\n");
    free_new(nv, nn);
    libscmp_names_free(&names);
    return status;
}
