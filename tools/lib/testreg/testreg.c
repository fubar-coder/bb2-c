/*
 * testreg.c - the registration of the Blitz tests of the model at build
 * time (testreg.h).
 */
#include "testreg.h"
#include "testjudge.h"
#include "testpp.h"
#include "testcosts.h"

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void treg_strs_free(struct treg_strs *s)
{
    for (size_t i = 0; i < s->n; i++)
        free(s->v[i]);
    free(s->v);
    memset(s, 0, sizeof *s);
}

static int push(struct treg_strs *s, const char *text)
{
    char **v = realloc(s->v, (s->n + 1) * sizeof *v);

    if (!v)
        return -1;
    s->v = v;
    if (!(v[s->n] = strdup(text)))
        return -1;
    s->n++;
    return 0;
}

static int pushf(struct treg_strs *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static int pushf(struct treg_strs *s, const char *fmt, ...)
{
    char buf[800];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return push(s, buf);
}

static int has(const struct treg_strs *s, const char *text)
{
    for (size_t i = 0; i < s->n; i++)
        if (!strcmp(s->v[i], text))
            return 1;
    return 0;
}

/* a file into memory, 0-terminated */
static char *slurp(const char *file, size_t *len)
{
    FILE *f = fopen(file, "rb");
    char *buf = NULL;
    size_t n = 0, cap = 0;

    if (!f)
        return NULL;
    for (;;) {
        size_t got;

        if (cap - n < 4096) {
            char *b = realloc(buf, (cap += 65536) + 1);

            if (!b) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = b;
        }
        got = fread(buf + n, 1, cap - n, f);
        n += got;
        if (!got)
            break;
    }
    if (ferror(f)) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    buf[n] = 0;
    *len = n;
    return buf;
}

int treg_variants_load(const char *file, struct treg_strs *names, char *err, size_t errlen)
{
    size_t len;
    char *text = slurp(file, &len), *p;

    if (!text) {
        snprintf(err, errlen, "%s: cannot read: %s", file, strerror(errno));
        return -1;
    }
    for (p = text; *p;) {
        char *nl = strchr(p, '\n'), *cell, *end;

        if (nl)
            *nl = 0;
        /* a table row: its first cell up to the next "|" */
        if (p[0] == '|' && (end = strchr(p + 1, '|'))) {
            *end = 0;
            for (cell = p + 1; (cell = strchr(cell, '`'));) {
                char *close = strchr(cell + 1, '`');
                size_t n;

                if (!close)
                    break;
                *close = 0;
                n = strspn(cell + 1, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-./");
                if (n && !cell[1 + n] && !has(names, cell + 1) && push(names, cell + 1)) {
                    free(text);
                    snprintf(err, errlen, "out of memory");
                    return -1;
                }
                cell = close + 1;
            }
        }
        if (!nl)
            break;
        p = nl + 1;
    }
    free(text);
    return 0;
}

int treg_tokens_load(const char *file, struct treg_strs *names, char *err, size_t errlen)
{
    size_t len;
    char *text = slurp(file, &len), *p;
    unsigned line = 0;

    if (!text) {
        snprintf(err, errlen, "%s: cannot read: %s", file, strerror(errno));
        return -1;
    }
    for (p = text; *p;) {
        char *nl = strchr(p, '\n'), *name;

        line++;
        if (nl)
            *nl = 0;
        if (*p && *p != '#') {
            char *e;

            strtoul(p, &e, 16);
            if (e == p || (*e != ' ' && *e != '\t')) {
                snprintf(err, errlen, "%s:%u: a line \"HEX NAME\" expected", file, line);
                free(text);
                return -1;
            }
            for (name = e; *name == ' ' || *name == '\t'; name++)
                ;
            name[strcspn(name, " \t\r")] = 0;
            if (*name && push(names, name)) {
                free(text);
                snprintf(err, errlen, "out of memory");
                return -1;
            }
        }
        if (!nl)
            break;
        p = nl + 1;
    }
    free(text);
    return 0;
}

void treg_free(struct treg_result *r)
{
    for (size_t i = 0; i < r->n; i++) {
        struct treg_test *t = &r->t[i];

        free(t->name);
        free(t->source);
        free(t->case_name);
        treg_strs_free(&t->labels);
        treg_strs_free(&t->fixtures);
        treg_strs_free(&t->cells);
        free(t->key);
        free(t->from_bundle);
        free(t->members);
    }
    free(r->t);
    treg_strs_free(&r->deps);
    treg_strs_free(&r->errors);
    memset(r, 0, sizeof *r);
}

static int id_ok(const char *id)
{
    if (!*id || *id == '-')
        return 0;
    for (const char *p = id; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-'))
            return 0;
        if (*p == '-' && (p[1] == '-' || p[1] == 0))
            return 0;
    }
    return 1;
}

/* the query text of t (its key, or a query of an alternative end): the
   source must have the check it names */
static int check_query(const char *text, const struct tc_tabkey *t, const struct tp_source *src,
                       struct treg_strs *errs)
{
    struct tc_query q;
    char m[300];
    int r = 0;

    if (tc_query_parse(text, &q, m, sizeof m))
        return 0;
    if (q.kind == TC_Q_RUN || q.kind == TC_Q_RUN_RC || q.kind == TC_Q_RUN_OUTPUT) {
        const struct tp_check *c = tp_find(src, q.ns, q.rest);

        if ((!c || c->kind != TP_RUNPROGRAM)
            && pushf(errs, "%s:%u: %s: the source has no !RunProgram named %s in %s", t->at.file, t->at.line, text,
                     q.rest, q.ns))
            r = -1;
    }
    if (q.kind == TC_Q_CHECK) {
        size_t len = strlen(q.rest);
        int found = tp_find(src, q.ns, q.rest) != NULL;

        for (size_t s = 0; !found && s < 2; s++) {
            const char *suffix = s ? ".actual" : ".expected";
            size_t sl = strlen(suffix);

            if (len > sl && !strcmp(q.rest + len - sl, suffix)) {
                char *name = strndup(q.rest, len - sl);

                found = name && tp_find(src, q.ns, name) != NULL;
                free(name);
            }
        }
        if (!found && pushf(errs, "%s:%u: %s: the source has no check named %s in %s", t->at.file, t->at.line,
                            text, q.rest, q.ns))
            r = -1;
    }
    tc_query_free(&q);
    return r;
}

/* the check queries and case selectors of a table against the table of the
   source (for the alternative ends the queries of each alternative) */
static int check_queries(const struct tc_tabkey *v, size_t n, const struct tp_source *src, struct treg_strs *errs)
{
    for (size_t i = 0; i < n; i++) {
        const struct tc_tabkey *t = &v[i];
        char m[300];

        if (t->sel) {
            struct tc_selector sel;

            if (tc_selector_parse(t->sel, 1, &sel, m, sizeof m) == 0) {
                for (size_t j = 0; j < sel.n; j++) {
                    int found = 0;

                    if (sel.t[j].def)
                        continue;
                    for (size_t c = 0; c < src->ncases; c++)
                        found |= !strcmp(src->cases[c].name, sel.t[j].value);
                    if (!found && pushf(errs, "%s:%u: the cell selector \"%s\" names the case %s, which the source "
                                        "does not have", t->at.file, t->at.line, t->sel, sel.t[j].value)) {
                        tc_selector_free(&sel);
                        return -1;
                    }
                }
                tc_selector_free(&sel);
            }
        }
        if (t->value.type == TT_LIST) {
            for (size_t k = 0; k < t->value.nlist; k++) {
                char *qt, *pt;
                int r;

                if (tc_end_split(t->value.list[k], &qt, &pt))
                    continue;
                r = check_query(qt, t, src, errs);
                free(qt);
                free(pt);
                if (r)
                    return -1;
            }
        } else if (t->value.type == TT_STRING && strcmp(t->key, "reason") && check_query(t->key, t, src, errs))
            return -1;
    }
    return 0;
}

/* the sides of a case: its cells without original material, with it, and
   AmiBlitz3's */
enum { TREG_PORT, TREG_ORIGINALS, TREG_ORACLE };
static const char *const side_suffix[] = { "", "_originals", "_oracle" };

static int in_side(const struct tc_matrix *m, size_t c, int side)
{
    if (side == TREG_ORACLE)
        return m->cells[c].oracle;
    return !m->cells[c].oracle && m->cells[c].originals == (side == TREG_ORIGINALS);
}

static int add_test(struct treg_result *r, const char *source, const struct tc_config *conf,
                    const struct tc_matrix *m, const struct tp_source *src, const char *case_name, int side,
                    const struct treg_opts *o, int deviation)
{
    struct treg_test t;
    const struct tc_setting *s;
    char name[512], *p;

    memset(&t, 0, sizeof t);
    for (size_t c = 0; c < m->ncells; c++) {
        char buf[24];
        const char *to;

        if (!in_side(m, c, side))
            continue;
        if (push(&t.cells, m->cells[c].name))
            goto oom;
        to = tc_cell_value(conf, m, c, tc_key("timeout"), buf);
        t.timeout += *to ? strtoll(to, NULL, 10) : o->timeout_default;
    }
    if (!t.cells.n)
        return 0;               /* an empty side */
    (void)src;
    t.timeout += o->timeout_extra;
    snprintf(name, sizeof name, "%s%s__%s%s", o->prefix, conf->id, case_name, side_suffix[side]);
    for (p = name; *p; p++)
        if (*p == '-')
            *p = '_';
    if (!(t.name = strdup(name)) || !(t.source = strdup(source)) || !(t.case_name = strdup(case_name)))
        goto oom;
    t.originals = side == TREG_ORIGINALS;
    t.oracle = side == TREG_ORACLE;
    if (side == TREG_ORACLE) {
        if (push(&t.labels, "oracle"))
            goto oom;
    } else if (side == TREG_ORIGINALS) {
        if (push(&t.labels, "originals"))
            goto oom;
    } else {
        if (deviation && !tc_get(conf, "fixed-orig") && !tc_get(conf, "extension") && !tc_variants_only(conf)
            && push(&t.labels, "bug-orig"))
            goto oom;
        if (push(&t.labels, "emulator"))
            goto oom;
    }
    if ((s = tc_get(conf, "labels")))
        for (size_t i = 0; i < s->value.nlist; i++)
            if (!has(&t.labels, s->value.list[i]) && push(&t.labels, s->value.list[i]))
                goto oom;
    /* a known flaky test: the label finds it; the verdict stays the runner's */
    if (tc_get(conf, "flaky") && !has(&t.labels, "flaky") && push(&t.labels, "flaky"))
        goto oom;
    if ((s = tc_get(conf, "cost"))) {
        t.has_cost = 1;
        t.cost_configured = 1;
        t.cost = s->value.integer;
    } else if (has(&t.labels, "bug-orig")) {
        /* an open bug fails on purpose, so ctest never learns its cost: the
           time it can take, the sum of its cells' session limits, puts it in
           the order of the bug-orig tests (cmake/bug-orig-order.cmake) */
        t.has_cost = 1;
        t.cost = t.timeout - o->timeout_extra;
    }
    if ((s = tc_get(conf, "fixtures")))
        for (size_t i = 0; i < s->value.nlist; i++)
            if (push(&t.fixtures, s->value.list[i]))
                goto oom;
    /* the fixtures of the build that some cells need (the build of blitz2.s
       for orig195) */
    for (size_t i = 0; i < o->ncell_fixtures; i++) {
        const char *spec = o->cell_fixtures[i], *bar = strchr(spec, '|');
        struct tc_selector sel;
        char msg[300], *selt;
        int any = 0, rc;

        if (!bar || !(selt = strndup(spec, (size_t)(bar - spec))))
            continue;           /* checked by treg_cell_fixture_ok */
        rc = tc_selector_parse(selt, 0, &sel, msg, sizeof msg);
        free(selt);
        if (rc == -2)
            goto oom;
        if (rc)
            continue;
        for (size_t c = 0; c < m->ncells && !any; c++)
            any = in_side(m, c, side) && tc_selector_matches(&sel, conf, m, c, NULL);
        tc_selector_free(&sel);
        if (any && !has(&t.fixtures, bar + 1) && push(&t.fixtures, bar + 1))
            goto oom;
    }
    {
        struct treg_test *v = realloc(r->t, (r->n + 1) * sizeof *v);

        if (!v)
            goto oom;
        r->t = v;
        v[r->n++] = t;
    }
    return 0;
oom:
    free(t.name);
    free(t.source);
    free(t.case_name);
    treg_strs_free(&t.labels);
    treg_strs_free(&t.fixtures);
    treg_strs_free(&t.cells);
    return -1;
}

int treg_cell_fixture_ok(const char *spec, char *err, size_t errlen)
{
    const char *bar = strchr(spec, '|');
    struct tc_selector sel;
    char msg[300], *selt;
    int rc;

    if (!bar || bar == spec || !bar[1] || strchr(bar + 1, '|')) {
        snprintf(err, errlen, "%s is not SELECTOR|FIXTURE", spec);
        return -1;
    }
    if (!(selt = strndup(spec, (size_t)(bar - spec)))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    rc = tc_selector_parse(selt, 0, &sel, msg, sizeof msg);
    free(selt);
    if (rc) {
        snprintf(err, errlen, "%s: %s", spec, rc == -2 ? "out of memory" : msg);
        return -1;
    }
    tc_selector_free(&sel);
    return 0;
}

static int by_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* a + b + c (malloc'd), NULL = out of memory */
static char *cat3(const char *a, const char *b, const char *c)
{
    size_t na = strlen(a), nb = strlen(b), nc = strlen(c);
    char *r = malloc(na + nb + nc + 1);

    if (r) {
        memcpy(r, a, na);
        memcpy(r + na, b, nb);
        memcpy(r + na + nb, c, nc + 1);
    }
    return r;
}

/* the files below dir (their paths relative to the top, prefix rel: "" or
   "sub/"), and dir and its directories into dirs */
static int list_files(const char *dir, const char *rel, struct treg_strs *files, struct treg_strs *dirs)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int r = 0;

    if (!d)
        return 0;               /* named by the missing files */
    if (push(dirs, dir)) {
        closedir(d);
        return -1;
    }
    while (r == 0 && (e = readdir(d))) {
        char *path, *sub;
        struct stat st;

        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        path = cat3(dir, "/", e->d_name);
        sub = cat3(rel, "", e->d_name);
        if (!path || !sub) {
            free(path);
            free(sub);
            closedir(d);
            return -1;
        }
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            char *subrel;

            if (!(subrel = cat3(sub, "/", "")))
                r = -1;
            else {
                r = list_files(path, subrel, files, dirs);
                free(subrel);
            }
        } else
            r = push(files, sub);
        free(path);
        free(sub);
    }
    closedir(d);
    return r;
}

/* the test-local data (basic-test-local-data, task 1.2): every file of
   ID.test/ declared in the table data and every declared one there; the
   directory, its subdirectories (a file added or removed changes them) and
   the files are dependencies of the registration */
static int check_data(const struct tc_config *c, struct treg_result *r)
{
    struct treg_strs files = { 0 }, dirs = { 0 };
    char *dd = NULL, *own = NULL;
    int rc = -1;

    if (!(own = cat3(c->dir, "/", c->id)) || !(dd = cat3(own, ".test", "")))
        goto out;
    {
        char *t = cat3(dd, ".toml", "");

        free(own);
        if (!(own = t))
            goto out;
    }
    if (list_files(dd, "", &files, &dirs))
        goto out;
    if (files.n)
        qsort(files.v, files.n, sizeof *files.v, by_str);
    for (size_t i = 0; i < dirs.n; i++)
        if (!has(&r->deps, dirs.v[i]) && push(&r->deps, dirs.v[i]))
            goto out;
    for (size_t i = 0; i < files.n; i++) {
        int declared = 0;

        for (size_t k = 0; k < c->ndata && !declared; k++)
            declared = !strcmp(c->data[k].file, files.v[i]);
        if (pushf(&r->deps, "%s/%s", dd, files.v[i]))
            goto out;
        if (!declared && pushf(&r->errors, "%s/%s: a file of %s.test/ that %s does not declare in the table data",
                               dd, files.v[i], c->id, own))
            goto out;
    }
    for (size_t k = 0; k < c->ndata; k++) {
        const struct tc_data *d = &c->data[k];

        if (!has(&files, d->file)) {
            if (pushf(&r->errors, "%s:%u: data \"%s\": no file %s/%s", d->at.file, d->at.line, d->file, dd,
                      d->file))
                goto out;
            continue;
        }
        if (d->action == TC_DA_BASIC_COMPILE) {
            /* a data source: the placeholders, and no check macro */
            struct tp_params par = { 0 };
            struct tp_source src = { 0 };
            char *path = cat3(dd, "/", d->file), *text;
            size_t len;
            int n;

            if (!path)
                goto out;
            if (!(text = slurp(path, &len))) {
                n = pushf(&r->errors, "%s: cannot read: %s", path, strerror(errno));
                free(path);
                if (n)
                    goto out;
                continue;
            }
            par.file = path;
            par.test_id = c->id;
            par.testdata = "";
            par.testdir = "";
            par.data = 1;
            n = tp_process(text, len, &par, &src);
            for (size_t i = 0; n > 0 && i < src.errors.n; i++)
                if (push(&r->errors, src.errors.v[i]))
                    n = -1;
            tp_free(&src);
            free(text);
            free(path);
            if (n < 0)
                goto out;
        }
    }
    rc = 0;
out:
    free(dd);
    free(own);
    treg_strs_free(&files);
    treg_strs_free(&dirs);
    return rc;
}

int treg_add_source(const char *source, const struct treg_opts *o, struct treg_result *r)
{
    size_t len, before = r->errors.n;
    char *text = NULL, *abs = NULL;
    struct tp_params par = { 0 };
    struct tp_source src = { 0 };
    struct tc_config conf = { 0 };
    struct tc_matrix m = { 0 };
    char err[800];
    const char *base = strrchr(source, '/');
    int rc = -1;

    for (const char *p = o->prefix; *p; p++)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_'))
            return pushf(&r->errors, "the prefix \"%s\" holds other characters than [a-z0-9_]", o->prefix) ? -1 : 1;
    if (!has(&r->deps, source) && push(&r->deps, source))
        return -1;
    if (!(text = slurp(source, &len)))
        return pushf(&r->errors, "%s: cannot read: %s", source, strerror(errno)) ? -1 : 1;

    /* the configuration: its files are dependencies even if it fails */
    if (tc_resolve(source, o->env, &conf, err, sizeof err)) {
        if (pushf(&r->errors, "%s", err))
            goto out;
    }
    for (size_t i = 0; i < conf.nfiles; i++)
        if (!has(&r->deps, conf.files[i]) && push(&r->deps, conf.files[i]))
            goto out;
    if (conf.id && !id_ok(conf.id) && pushf(&r->errors, "%s: the test id %s is not lower-case kebab-case "
                                            "([a-z0-9]+, \"-\" between)", source, conf.id))
        goto out;
    if (conf.id && conf.dir && check_data(&conf, r))
        goto out;

    par.file = base ? base + 1 : source;
    par.test_id = conf.id ? conf.id : "";
    par.testdata = "";
    par.testdir = "";
    par.tokens = o->tokens;
    par.ntokens = o->ntokens;
    /* the source modes of the cells (bit 0 BB2, bit 1 AB3): the source is
       read in each, AB3 first, whose table is the test's */
    {
        struct tc_matrix probe = { 0 };
        int modes = 1;

        if (r->errors.n == before && conf.id) {
            if (tc_matrix_build(&conf, &probe, err, sizeof err)) {
                /* without cells no source mode: the matrix's error alone */
                rc = pushf(&r->errors, "%s", err) ? -1 : 1;
                goto out;
            }
            modes = 0;
            for (size_t c = 0; c < probe.ncells; c++)
                modes |= tc_cell_ab3(&conf, &probe, c) ? 2 : 1;
            tc_matrix_free(&probe);
        }
        for (int k = 1; k >= 0; k--) {
            struct tp_source other = { 0 }, *s = src.text ? &other : &src;
            int bad;

            if (!(modes & (1 << k)))
                continue;
            par.ab3 = k;
            bad = tp_process(text, len, &par, s) < 0 || tp_check_form(text, len, &par, s) < 0;
            for (size_t i = 0; !bad && i < s->errors.n; i++) {
                /* the messages name the file as given; once if both modes give it */
                const char *msg = s->errors.v[i] + strlen(par.file);
                char *full = malloc(strlen(source) + strlen(msg) + 1);

                if (!full)
                    bad = 1;
                else {
                    sprintf(full, "%s%s", source, msg);
                    bad = !has(&r->errors, full) && push(&r->errors, full);
                    free(full);
                }
            }
            if (s == &other)
                tp_free(&other);
            if (bad)
                goto out;
        }
    }
    if (r->errors.n > before) {
        rc = 1;
        goto out;
    }
    if (check_queries(conf.asserts, conf.nasserts, &src, &r->errors)
        || check_queries(conf.deviations, conf.ndeviations, &src, &r->errors)
        || check_queries(conf.ends, conf.nends, &src, &r->errors))
        goto out;
    if (tc_matrix_build(&conf, &m, err, sizeof err)) {
        rc = pushf(&r->errors, "%s", err) ? -1 : 1;
        goto out;
    }
    if (tj_editor_run_refused(source, &src, &conf, &m, err, sizeof err)) {
        rc = pushf(&r->errors, "%s", err) ? -1 : 1;
        goto out;
    }
    /* the variant of a deviation of AmiBlitz3 names a row of
       docs/ab3/variant.md (the first column) */
    for (size_t i = 0; i < conf.nvariants; i++) {
        const struct tc_tabkey *v = &conf.variants[i];

        if (!o->variants && pushf(&r->errors, "%s:%u: test %s: variant \"%s\": no table of variants to check it "
                                  "against (testreg --variants)", v->at.file, v->at.line, conf.id, v->value.str))
            goto out;
        if (o->variants && !has(o->variants, v->value.str)
            && pushf(&r->errors, "%s:%u: test %s: variant \"%s\" names no row of %s (a name in the first column "
                     "of a table)", v->at.file, v->at.line, conf.id, v->value.str, o->variants_file))
            goto out;
    }
    /* AmiBlitz3 runs without its debugger: no !ExpectError beside its cells */
    for (size_t i = 0; i < src.nchecks; i++)
        if (src.checks[i].kind == TP_EXPECTERROR
            && tc_expect_error_ok(&m, source, src.checks[i].line, err, sizeof err)) {
            rc = pushf(&r->errors, "%s", err) ? -1 : 1;
            goto out;
        }
    if (r->errors.n > before) {
        rc = 1;
        goto out;
    }
    /* the tests name the source by its absolute path (ctest runs elsewhere) */
    if (!(abs = malloc(strlen(conf.dir) + strlen(par.file) + 2)))
        goto out;
    sprintf(abs, "%s/%s", conf.dir, par.file);
    for (size_t c = 0; c < src.ncases; c++) {
        struct tj_test jt = { par.file, &src, &conf, &m, src.cases[c].name, 0, NULL, 0 };
        int dev = tj_case_has_deviation(&jt);
        size_t n = r->n;

        if (add_test(r, abs, &conf, &m, &src, src.cases[c].name, TREG_PORT, o, dev))
            goto out;
        /* the case test of the side port that may be bundled: its key */
        if (o->bundle && r->n > n) {
            struct treg_test *t = &r->t[r->n - 1];
            size_t cell;
            int b = tj_bundle_case(&conf, &m, &src, o->env, src.cases[c].name, &cell);

            if (b < 0)
                goto out;
            if (b) {
                char buf[24];
                const char *to = tc_cell_value(&conf, &m, cell, tc_key("timeout"), buf);

                if (!(t->key = tj_bundle_key(&conf, &m, cell, o->env)))
                    goto out;
                t->limit = *to ? strtoll(to, NULL, 10) : o->timeout_default;
            }
        }
        if (add_test(r, abs, &conf, &m, &src, src.cases[c].name, TREG_ORIGINALS, o, dev)
            || add_test(r, abs, &conf, &m, &src, src.cases[c].name, TREG_ORACLE, o, dev))
            goto out;
    }
    rc = 0;
out:
    free(abs);
    free(text);
    tp_free(&src);
    tc_matrix_free(&m);
    tc_free(&conf);
    return rc;
}

int treg_check_names(struct treg_result *r)
{
    int bad = 0;

    for (size_t i = 0; i < r->n; i++)
        for (size_t j = 0; j < i; j++)
            if (!strcmp(r->t[i].name, r->t[j].name)) {
                if (pushf(&r->errors, "%s: the test %s is registered twice (also by %s)", r->t[i].source,
                          r->t[i].name, r->t[j].source))
                    return -1;
                bad = 1;
                break;
            }
    return bad;
}

/* whether a and b hold the same strings (as sets of distinct ones: labels,
   fixtures) */
static int same_set(const struct treg_strs *a, const struct treg_strs *b)
{
    for (size_t i = 0; i < a->n; i++)
        if (!has(b, a->v[i]))
            return 0;
    for (size_t i = 0; i < b->n; i++)
        if (!has(a, b->v[i]))
            return 0;
    return 1;
}

static int copy_strs(struct treg_strs *to, const struct treg_strs *from)
{
    for (size_t i = 0; i < from->n; i++)
        if (push(to, from->v[i]))
            return -1;
    return 0;
}

/* the COST of bundle b: the sum of its members' COSTs (none if no member has one) */
static void bundle_cost(struct treg_result *r, struct treg_test *b)
{
    b->has_cost = 0;
    b->cost = 0;
    for (size_t k = 0; k < b->nmembers; k++) {
        const struct treg_test *t = &r->t[b->members[k]];

        if (t->has_cost) {
            b->has_cost = 1;
            b->cost += t->cost;
        }
    }
}

int treg_make_bundles(struct treg_result *r, const struct treg_opts *o)
{
    size_t ncases = r->n, nbundles = 0, *chunk = malloc(TREG_BUNDLE_MAX * sizeof *chunk);

    if (!chunk)
        return -1;
    for (size_t i = 0; i < ncases; i++) {
        size_t n = 0;

        if (!r->t[i].key || r->t[i].from_bundle)
            continue;
        /* the group of i, in the order of r, chunk by chunk */
        for (size_t j = i; j <= ncases; j++) {
            if (j < ncases) {
                const struct treg_test *a = &r->t[i], *t = &r->t[j];

                if (!t->key || t->from_bundle || strcmp(t->key, a->key) || !same_set(&t->labels, &a->labels)
                    || !same_set(&t->fixtures, &a->fixtures))
                    continue;
                chunk[n++] = j;
            }
            if (n == TREG_BUNDLE_MAX || (j == ncases && n)) {
                struct treg_test b, *v;
                char name[512];

                memset(&b, 0, sizeof b);
                snprintf(name, sizeof name, "%sbundle_%zu", o->prefix, ++nbundles);
                b.bundle = 1;
                b.nmembers = n;
                if (!(b.name = strdup(name)) || !(b.source = strdup(r->t[chunk[0]].source))
                    || !(b.members = malloc(n * sizeof *b.members)) || copy_strs(&b.labels, &r->t[chunk[0]].labels)
                    || copy_strs(&b.fixtures, &r->t[chunk[0]].fixtures)
                    || !(v = realloc(r->t, (r->n + 1) * sizeof *v))) {
                    free(b.name);
                    free(b.source);
                    free(b.members);
                    treg_strs_free(&b.labels);
                    treg_strs_free(&b.fixtures);
                    free(chunk);
                    return -1;
                }
                r->t = v;
                memcpy(b.members, chunk, n * sizeof *chunk);
                /* 3 sessions at most (basic-test-bundle-runner, design decision 4) */
                b.timeout = 3 * r->t[chunk[0]].limit + o->timeout_extra;
                for (size_t k = 0; k < n; k++)
                    if (!(r->t[chunk[k]].from_bundle = strdup(name))) {
                        r->t[r->n++] = b;   /* freed with r */
                        free(chunk);
                        return -1;
                    }
                r->t[r->n++] = b;
                bundle_cost(r, &r->t[r->n - 1]);
                n = 0;
            }
        }
    }
    free(chunk);
    return 0;
}

/* a bracket argument [==[...]==] */
static int bracket(FILE *f, const char *s)
{
    if (strstr(s, "]==]"))
        return -1;
    return fprintf(f, " [==[%s]==]", s) < 0 ? -1 : 0;
}

static int bracket_list(FILE *f, const struct treg_strs *l)
{
    if (fputs(" [==[", f) < 0)
        return -1;
    for (size_t i = 0; i < l->n; i++) {
        if (strstr(l->v[i], "]==]") || strchr(l->v[i], ';'))
            return -1;
        if (fprintf(f, "%s%s", i ? ";" : "", l->v[i]) < 0)
            return -1;
    }
    return fputs("]==]", f) < 0 ? -1 : 0;
}

/* one registered test; testenv: the call of the test's environment */
static int write_test(FILE *f, const struct treg_result *r, const struct treg_test *t, const char *const *command,
                      size_t ncommand, int testenv)
{
    if (fputs("add_test(", f) < 0 || bracket(f, t->name))
        return -1;
    for (size_t c = 0; c < ncommand; c++)
        if (bracket(f, command[c]))
            return -1;
    if (t->bundle) {
        /* the bundle test (basic-test-bundle-key, I1) */
        if (bracket(f, "--bundle") || bracket(f, t->name))
            return -1;
        for (size_t k = 0; k < t->nmembers; k++) {
            const struct treg_test *m = &r->t[t->members[k]];

            if (bracket(f, "--member") || bracket(f, m->name) || bracket(f, m->source) || bracket(f, m->case_name))
                return -1;
        }
    } else if (bracket(f, "--source") || bracket(f, t->source) || bracket(f, "--case") || bracket(f, t->case_name)
               || bracket(f, "--side") || bracket(f, t->oracle ? "oracle" : t->originals ? "originals" : "port"))
        return -1;
    /* a member: served from its bundle (I2) */
    if (t->from_bundle
        && (bracket(f, "--test") || bracket(f, t->name) || bracket(f, "--from-bundle") || bracket(f, t->from_bundle)))
        return -1;
    if (fputs(")\n", f) < 0)
        return -1;
    if (fputs("set_tests_properties(", f) < 0 || bracket(f, t->name) || fputs(" PROPERTIES LABELS", f) < 0
        || bracket_list(f, &t->labels) || fprintf(f, " TIMEOUT %lld SKIP_RETURN_CODE 77", t->timeout) < 0)
        return -1;
    if (t->has_cost && fprintf(f, " COST %lld", t->cost) < 0)
        return -1;
    if (t->fixtures.n && (fputs(" FIXTURES_REQUIRED", f) < 0 || bracket_list(f, &t->fixtures)))
        return -1;
    /* the order only: a case test runs also when its bundle failed or was
       not selected */
    if (t->from_bundle && (fputs(" DEPENDS", f) < 0 || bracket(f, t->from_bundle)))
        return -1;
    if (fputs(")\n", f) < 0)
        return -1;
    if (testenv && (fputs("basic_test_environment(", f) < 0 || bracket(f, t->name) || bracket_list(f, &t->labels)
                    || fputs(")\n", f) < 0))
        return -1;
    return 0;
}

/* a test of the order: a registered one or an entry */
struct ordered {
    const char *name;
    long long cost;
    const struct treg_test *t;
    const struct treg_entry *e;
};

static int by_cost(const void *a, const void *b)
{
    const struct ordered *x = a, *y = b;

    if (x->cost != y->cost)
        return x->cost > y->cost ? -1 : 1;
    return strcmp(x->name, y->name);
}

int treg_write_include(FILE *f, const struct treg_result *r, const char *const *command, size_t ncommand,
                       const struct treg_entries *order, const char *testenv)
{
    size_t ne = order ? order->n : 0, n = 0;
    struct ordered *v = calloc(r->n + ne + 1, sizeof *v);
    int rc = -1;

    if (!v)
        return -1;
    if (fprintf(f, "# the Blitz tests of the model (ID.test.bas), written by testreg: do not edit\n") < 0)
        goto out;
    /* the environment of the tests (cmake/basic-test-model.cmake) */
    if (testenv && (fputs("include(", f) < 0 || bracket(f, testenv) || fputs(")\n", f) < 0))
        goto out;
    for (size_t i = 0; i < r->n; i++) {
        const struct treg_test *t = &r->t[i];

        if (has(&t->labels, "bug-orig")) {
            v[n].name = t->name;
            v[n].cost = t->has_cost ? t->cost : 0;
            v[n++].t = t;
        } else if (write_test(f, r, t, command, ncommand, testenv != NULL))
            goto out;
    }
    for (size_t i = 0; i < ne; i++) {
        v[n].name = order->v[i].name;
        v[n].cost = order->v[i].cost;
        v[n++].e = &order->v[i];
    }
    /* the tests of the label bug-orig, the largest COST first (cmake/bug-orig-order.cmake) */
    qsort(v, n, sizeof *v, by_cost);
    if (n && fprintf(f, "# the tests of the label bug-orig, the largest COST first\n") < 0)
        goto out;
    for (size_t i = 0; i < n; i++)
        if (v[i].t ? write_test(f, r, v[i].t, command, ncommand, testenv != NULL) : fputs(v[i].e->text, f) < 0)
            goto out;
    rc = 0;
out:
    free(v);
    return rc;
}

void treg_entries_free(struct treg_entries *e)
{
    for (size_t i = 0; i < e->n; i++) {
        free(e->v[i].name);
        free(e->v[i].text);
    }
    free(e->v);
    memset(e, 0, sizeof *e);
}

int treg_entries_load(const char *file, struct treg_entries *e, char *err, size_t errlen)
{
    FILE *f = fopen(file, "r");
    char line[8192];
    unsigned ln = 0;
    struct treg_entry *cur = NULL;
    size_t curlen = 0;

    memset(e, 0, sizeof *e);
    if (!f) {
        snprintf(err, errlen, "%s: cannot read: %s", file, strerror(errno));
        return -1;
    }
    while (fgets(line, sizeof line, f)) {
        size_t len = strlen(line);

        ln++;
        if (len && line[len - 1] != '\n' && !feof(f)) {
            snprintf(err, errlen, "%s:%u: a line longer than %zu bytes", file, ln, sizeof line - 1);
            goto bad;
        }
        if (!strncmp(line, "# @test ", 8)) {
            char name[512], *end;
            long long cost;
            struct treg_entry *nv;

            if (sscanf(line + 8, "%511s", name) != 1 || !(end = strchr(line + 8, ' '))
                || (errno = 0, cost = strtoll(end + 1, &end, 10), errno) || cost < 0
                || (*end && *end != '\n')) {
                snprintf(err, errlen, "%s:%u: an entry is \"# @test NAME COST\" (COST an integer >= 0)", file, ln);
                goto bad;
            }
            if (!(nv = realloc(e->v, (e->n + 1) * sizeof *nv)))
                goto oom;
            e->v = nv;
            cur = &nv[e->n++];
            memset(cur, 0, sizeof *cur);
            cur->cost = cost;
            curlen = 0;
            if (!(cur->name = strdup(name)) || !(cur->text = strdup("")))
                goto oom;
            continue;
        }
        if (!cur) {
            if (line[0] == '#' || line[0] == '\n')
                continue;
            snprintf(err, errlen, "%s:%u: text before the first entry (\"# @test NAME COST\")", file, ln);
            goto bad;
        }
        {
            char *t = realloc(cur->text, curlen + len + 2);

            if (!t)
                goto oom;
            cur->text = t;
            memcpy(t + curlen, line, len);
            curlen += len;
            if (curlen && t[curlen - 1] != '\n')
                t[curlen++] = '\n';
            t[curlen] = 0;
        }
    }
    fclose(f);
    return 0;
oom:
    snprintf(err, errlen, "%s: out of memory", file);
bad:
    fclose(f);
    treg_entries_free(e);
    return -1;
}

int treg_check_entries(struct treg_result *r, const struct treg_entries *e)
{
    int bad = 0;

    for (size_t i = 0; i < r->n; i++)
        for (size_t j = 0; j < e->n; j++)
            if (!strcmp(r->t[i].name, e->v[j].name)) {
                if (pushf(&r->errors, "%s: the test %s is also a test of the old form", r->t[i].source,
                          r->t[i].name))
                    return -1;
                bad = 1;
            }
    return bad;
}

static int dep_path(FILE *f, const char *s)
{
    for (; *s; s++) {
        if ((*s == ' ' || *s == '#' || *s == '\\') && fputc('\\', f) == EOF)
            return -1;
        if (*s == '$' && fputc('$', f) == EOF)
            return -1;
        if (fputc(*s, f) == EOF)
            return -1;
    }
    return 0;
}

int treg_write_depfile(FILE *f, const char *target, const struct treg_result *r)
{
    if (dep_path(f, target) || fputc(':', f) == EOF)
        return -1;
    for (size_t i = 0; i < r->deps.n; i++)
        if (fputs(" \\\n  ", f) < 0 || dep_path(f, r->deps.v[i]))
            return -1;
    return fputc('\n', f) == EOF ? -1 : 0;
}

void treg_apply_costs(struct treg_result *r, const struct tcosts *c)
{
    for (size_t i = 0; i < r->n; i++) {
        const struct tcost *e = tcosts_find(c, r->t[i].name);

        /* the configuration's cost wins; a measurement replaces the estimate */
        if (e && !r->t[i].cost_configured && !r->t[i].bundle) {
            r->t[i].has_cost = 1;
            r->t[i].cost = e->cost;
        }
    }
    /* a bundle: its entry, else the sum of its members' COSTs as measured */
    for (size_t i = 0; i < r->n; i++) {
        const struct tcost *e;

        if (!r->t[i].bundle)
            continue;
        if ((e = tcosts_find(c, r->t[i].name))) {
            r->t[i].has_cost = 1;
            r->t[i].cost = e->cost;
        } else
            bundle_cost(r, &r->t[i]);
    }
}
