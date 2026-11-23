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

static int add_test(struct treg_result *r, const char *source, const struct tc_config *conf,
                    const struct tc_matrix *m, const struct tp_source *src, const char *case_name, int originals,
                    const struct treg_opts *o, int deviation)
{
    struct treg_test t;
    const struct tc_setting *s;
    char name[512], *p;

    memset(&t, 0, sizeof t);
    for (size_t c = 0; c < m->ncells; c++) {
        char buf[24];
        const char *to;

        if (m->cells[c].originals != originals)
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
    snprintf(name, sizeof name, "%s%s__%s%s", o->prefix, conf->id, case_name, originals ? "_originals" : "");
    for (p = name; *p; p++)
        if (*p == '-')
            *p = '_';
    if (!(t.name = strdup(name)) || !(t.source = strdup(source)) || !(t.case_name = strdup(case_name)))
        goto oom;
    t.originals = originals;
    if (originals) {
        if (push(&t.labels, "originals"))
            goto oom;
    } else {
        if (deviation && !tc_get(conf, "fixed-orig") && !tc_get(conf, "extension")
            && push(&t.labels, "bug-orig"))
            goto oom;
        if (push(&t.labels, "emulator"))
            goto oom;
    }
    if ((s = tc_get(conf, "labels")))
        for (size_t i = 0; i < s->value.nlist; i++)
            if (!has(&t.labels, s->value.list[i]) && push(&t.labels, s->value.list[i]))
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
            any = m->cells[c].originals == originals && tc_selector_matches(&sel, conf, m, c, NULL);
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
    if (tp_process(text, len, &par, &src) < 0 || tp_check_form(text, len, &par, &src) < 0)
        goto out;
    for (size_t i = 0; i < src.errors.n; i++) {
        /* the messages name the file as given */
        const char *msg = src.errors.v[i] + strlen(par.file);

        if (pushf(&r->errors, "%s%s", source, msg))
            goto out;
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

        if (add_test(r, abs, &conf, &m, &src, src.cases[c].name, 0, o, dev)
            || add_test(r, abs, &conf, &m, &src, src.cases[c].name, 1, o, dev))
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
static int write_test(FILE *f, const struct treg_test *t, const char *const *command, size_t ncommand, int testenv)
{
    if (fputs("add_test(", f) < 0 || bracket(f, t->name))
        return -1;
    for (size_t c = 0; c < ncommand; c++)
        if (bracket(f, command[c]))
            return -1;
    if (bracket(f, "--source") || bracket(f, t->source) || bracket(f, "--case") || bracket(f, t->case_name)
        || bracket(f, "--side") || bracket(f, t->originals ? "originals" : "port") || fputs(")\n", f) < 0)
        return -1;
    if (fputs("set_tests_properties(", f) < 0 || bracket(f, t->name) || fputs(" PROPERTIES LABELS", f) < 0
        || bracket_list(f, &t->labels) || fprintf(f, " TIMEOUT %lld SKIP_RETURN_CODE 77", t->timeout) < 0)
        return -1;
    if (t->has_cost && fprintf(f, " COST %lld", t->cost) < 0)
        return -1;
    if (t->fixtures.n && (fputs(" FIXTURES_REQUIRED", f) < 0 || bracket_list(f, &t->fixtures)))
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
        } else if (write_test(f, t, command, ncommand, testenv != NULL))
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
        if (v[i].t ? write_test(f, v[i].t, command, ncommand, testenv != NULL) : fputs(v[i].e->text, f) < 0)
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

int treg_apply_costs(struct treg_result *r, const char *prefix, const char *file, const struct tcosts *c)
{
    size_t plen = strlen(prefix);
    int bad = 0;

    for (size_t i = 0; i < r->n; i++) {
        const struct tcost *e = tcosts_find(c, r->t[i].name);

        /* the configuration's cost wins; a measurement replaces the estimate */
        if (e && !r->t[i].cost_configured) {
            r->t[i].has_cost = 1;
            r->t[i].cost = e->cost;
        }
    }
    for (size_t k = 0; k < c->n; k++) {
        const char *name = c->v[k].name;
        size_t i;

        if (strncmp(name, prefix, plen) || !strstr(name + plen, "__"))
            continue;
        for (i = 0; i < r->n && strcmp(r->t[i].name, name); i++)
            ;
        if (i == r->n) {
            if (pushf(&r->errors, "%s:%u: %s: no such test (a test of the model of %s* that is not "
                      "registered: refresh the file, tests/README.md \"The measured COST\")",
                      file, c->v[k].line, name, prefix))
                return -1;
            bad = 1;
        }
    }
    return bad;
}
