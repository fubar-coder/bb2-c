/*
 * caserun.c - one case of a Blitz test of the BASIC test model (caserun.h).
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup */
#include "caserun.h"

#include <errno.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "bugorig.h"
#include "corpus.h"
#include "runwork.h"
#include "serial.h"
#include "statline_emulation.h"
#include "testpp.h"
#include "util.h"

extern char **environ;

/* ------------------------------------------------------------ helpers */

static int fail(char *err, size_t errlen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

/* the value of a single-valued run setting in a cell */
static const char *val(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const char *key,
                       char buf[24])
{
    return tc_cell_value(conf, m, cell, tc_key(key), buf);
}

/* the list of a list setting, NULL: none */
static const struct tt_value *list(const struct tc_config *conf, const char *key)
{
    const struct tc_setting *s = tc_get(conf, key);

    return s && s->value.type == TT_LIST ? &s->value : NULL;
}

/* the runner's value key of env; NULL with err if it lacks it */
static const char *need(const struct tc_env *env, const char *key, char *err, size_t errlen)
{
    const char *v = tc_env_runner(env, key);

    if (!v)
        fail(err, errlen, "the environment file %s has no runner value %s", env && env->file ? env->file : "(none)",
             key);
    return v;
}

/* ------------------------------------------------------- the pass of a cell */

int cr_cell_pass(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env,
                 const char *tag, const char *work, const char *source, char **text, char *err, size_t errlen)
{
    char b1[24], b2[24], b3[24];
    const char *compiler = val(conf, m, cell, "compiler", b1), *libs = val(conf, m, cell, "libs", b1);
    const char *blitz2, *deflibs, *acidlibs, *v;
    const char *rwe_dir = NULL;
    const struct tt_value *l;
    struct text t = { 0 };
    const char *runwrap;
    int originals = m->cells[cell].originals;

    *text = NULL;
    blitz2 = need(env, !strcmp(compiler, "port") ? "blitz2" : compiler, err, errlen);
    if (!blitz2)
        return -1;
    /* the RWE user libraries: the C build for blitz2-c and 1.95, the reference
       for 2.10; the C libraries of their variant */
    if ((l = list(conf, "rwe"))) {
        for (size_t i = 0; i < l->nlist; i++) {
            char key[200];
            const char *info, *bar1, *bar2;

            snprintf(key, sizeof key, "rwe-%s", l->list[i]);
            if (!(info = tc_env_runner(env, key)) || !(bar1 = strchr(info, '|')) || !(bar2 = strchr(bar1 + 1, '|')))
                return fail(err, errlen, "%s: no RWE user library %s in the environment file (rwe-%s = "
                                         "\"OBJ|REF|VOUT\")",
                            conf->source, l->list[i], l->list[i]);
            if (rwe_dir && strcmp(rwe_dir, bar2 + 1))
                return fail(err, errlen, "%s: rwe: %s is built for another variant than the libraries before it",
                            conf->source, l->list[i]);
            rwe_dir = bar2 + 1;
        }
    }
    if (!strcmp(libs, "cd")) {
        if (!(deflibs = need(env, "cd-deflibs", err, errlen)) || !(acidlibs = need(env, "cd-acidlibs", err, errlen)))
            return -1;
        text_add(&t, "config=%s|ref|%s|%s|%s\n", tag, blitz2, deflibs, acidlibs);
    } else if (rwe_dir)
        text_add(&t, "config=%s|ref|%s|%s/deflibs|%s/acidlibs\n", tag, blitz2, rwe_dir, rwe_dir);
    else {
        if (!(deflibs = need(env, "clib-deflibs", err, errlen))
            || !(acidlibs = need(env, "clib-acidlibs", err, errlen)))
            return -1;
        text_add(&t, "config=%s|ref|%s|%s|%s\n", tag, blitz2, deflibs, acidlibs);
    }
    {
        const char *ted = need(env, "ted", err, errlen), *launcher = ted ? need(env, "launcher", err, errlen) : NULL;
        const char *rcput = launcher ? need(env, "rcput", err, errlen) : NULL;
        const char *testlib = rcput ? need(env, "testlib", err, errlen) : NULL;
        const char *timeout = val(conf, m, cell, "timeout", b2);

        if (!testlib) {
            free(t.p);
            return -1;
        }
        if (!*timeout && !(timeout = need(env, "timeout", err, errlen))) {
            free(t.p);
            return -1;
        }
        text_add(&t, "cell=on\nwork=%s\nreport=%s/%s.txt\nruntime_errors=%s\ntimeout=%s\nparallel=1\n", work, work, tag,
                 val(conf, m, cell, "runtime-errors", b3), timeout);
        text_add(&t, "ted=%s\nlauncher=%s\nrcput=%s\nsource=%s\n", ted, launcher, rcput, source);
        /* the test library: blitz2:userlibs/testlib/; 1.95 loads a user library
           from blitz2:DIR/ (and needs blitz2:userlibs/DIR/ to exist) */
        text_add(&t, "file=blitz2/userlibs/testlib/testlib.obj|%s\n", testlib);
        if (!strcmp(compiler, "orig195"))
            text_add(&t, "file=blitz2/testlib/testlib.obj|%s\n", testlib);
    }
    /* the debugger module of the CD for the cells with original material */
    if (originals && (v = tc_env_runner(env, "cd-defaultdbug")))
        text_add(&t, "defaultdbug=%s\n", v);
    if (*(v = val(conf, m, cell, "stack", b2)))
        text_add(&t, "stack=%s\n", v);
    {
        const char *hang = val(conf, m, cell, "hang-after", b2);

        if (!*hang)
            hang = tc_env_runner(env, "run-hang-after");
        if (hang && *hang && strcmp(hang, "0"))
            text_add(&t, "hang_after=%s\nhang_from=run\n", hang);
    }
    if (strcmp(v = val(conf, m, cell, "machine", b2), "a1200"))
        text_add(&t, "machine=%s\n", v);
    if (!strcmp(val(conf, m, cell, "rtg", b2), "on"))
        text_add(&t, "rtg=on\n");
    if (!strcmp(val(conf, m, cell, "serial", b2), "on"))
        text_add(&t, "serial=on\n");
    if (*(v = val(conf, m, cell, "wrap", b2)))
        text_add(&t, "wrap=%s\n", v);
    runwrap = val(conf, m, cell, "runwrap", b2);
    if (!strcmp(val(conf, m, cell, "pad", b3), "on")) {
        const char *inject;

        if (*runwrap) {
            free(t.p);
            return fail(err, errlen, "%s: pad = \"on\" starts the program through inject; it cannot have a "
                                     "runwrap too",
                        conf->source);
        }
        if (!(inject = need(env, "inject", err, errlen))) {
            free(t.p);
            return -1;
        }
        text_add(&t, "runwrap=%s|pad\n", inject);
    } else if (*runwrap)
        text_add(&t, "runwrap=%s\n", runwrap);
    if (*(v = val(conf, m, cell, "runargs", b2)))
        text_add(&t, "runargs=%s\n", v);
    if (*(v = val(conf, m, cell, "launcherargs", b2)))
        text_add(&t, "launcherargs=%s\n", v);
    if ((l = list(conf, "rwe")))
        for (size_t i = 0; i < l->nlist; i++) {
            char key[200];
            const char *info, *bar1;

            snprintf(key, sizeof key, "rwe-%s", l->list[i]);
            info = tc_env_runner(env, key);
            bar1 = strchr(info, '|');
            if (!strcmp(compiler, "orig210")) {
                const char *bar2 = strchr(bar1 + 1, '|');

                text_add(&t, "file=blitz2/userlibs/rwe/%s.obj|%.*s\n", l->list[i], (int)(bar2 - bar1 - 1), bar1 + 1);
            } else
                text_add(&t, "file=blitz2/userlibs/rwe/%s.obj|%.*s\n", l->list[i], (int)(bar1 - info), info);
        }
    /* the files: DEST|SRC, with a third field only for the cells its selector matches */
    if ((l = list(conf, "files")))
        for (size_t i = 0; i < l->nlist; i++) {
            const char *f = l->list[i], *bar1 = strchr(f, '|'), *bar2 = bar1 ? strchr(bar1 + 1, '|') : NULL;

            if (!bar1 || bar1 == f || bar1[1] == '|' || !bar1[1]) {
                free(t.p);
                return fail(err, errlen, "%s: files: %s is not DEST|SRC[|SELECTOR]", conf->source, f);
            }
            if (bar2) {
                struct tc_selector sel;
                char msg[300];
                int ok;

                if (tc_selector_parse(bar2 + 1, 0, &sel, msg, sizeof msg)) {
                    free(t.p);
                    return fail(err, errlen, "%s: files: %s: %s", conf->source, f, msg);
                }
                ok = tc_selector_matches(&sel, conf, m, cell, NULL);
                tc_selector_free(&sel);
                if (!ok)
                    continue;
                text_add(&t, "file=%.*s\n", (int)(bar2 - f), f);
            } else
                text_add(&t, "file=%s\n", f);
        }
    {
        static const struct { const char *key, *pass; } lists[] = {
            { "dirs", "dir" }, { "remove", "remove" }, { "mount", "mount" },
            { "floppy", "floppy" }, { "uae", "uae" }, { "jobline", "jobline" },
        };

        for (size_t k = 0; k < sizeof lists / sizeof lists[0]; k++)
            if ((l = list(conf, lists[k].key)))
                for (size_t i = 0; i < l->nlist; i++)
                    text_add(&t, "%s=%s\n", lists[k].pass, l->list[i]);
    }
    if (t.bad) {
        free(t.p);
        return -2;
    }
    *text = t.p;
    return 0;
}

/* --------------------------------------------- the comparison with prev */

/* entry TEST:NAME of test: NAME, else NULL */
static const char *entry_name(const char *entry, const char *test)
{
    size_t n = strlen(test);

    return !strncmp(entry, test, n) && entry[n] == ':' ? entry + n + 1 : NULL;
}

int cr_prev_pass(const char *cell_pass, const char *test, const char *name, const char *work, const char *source,
                 const char *prev_blitz2, const char *prev_libs, const char *const *entries, size_t nentries,
                 char **text, char *err, size_t errlen)
{
    /* the keys of a run, not of a compile (as prev_compile_test drops them), and
       the ones this pass sets itself */
    static const char *const drop[] = { "cell", "launcher", "rcput", "hang_after", "hang_from", "serial",
                                        "runwrap", "runargs", "launcherargs", "defaultdbug", "floppy",
                                        "parallel", "report", "work", "source", "run", "test", "intended" };
    struct text t = { 0 };
    const char *l = cell_pass;
    int configs = 0, mask = 0;

    *text = NULL;
    text_add(&t, "test=%s\nwork=%s\nreport=%s.txt\nrun=off\nparallel=2\nsource=%s\n", test, work, work, source);
    while (*l) {
        const char *e = strchr(l, '\n'), *eq = strchr(l, '=');
        size_t len = e ? (size_t)(e - l) : strlen(l), k;
        int skip = 0;

        if (eq && eq < l + len) {
            size_t klen = (size_t)(eq - l);

            for (k = 0; k < sizeof drop / sizeof drop[0]; k++)
                if (strlen(drop[k]) == klen && !memcmp(l, drop[k], klen))
                    skip = 1;
            if (klen == 7 && !memcmp(l, "jobline", 7) && len > 12 && !memcmp(eq + 1, "run=", 4))
                mask = 1;
        }
        if (!skip && len)
            text_add(&t, "%.*s\n", (int)len, l);
        if (!skip && len > 7 && !memcmp(l, "config=", 7)) {
            const char *bar = memchr(l + 7, '|', len - 7);

            if (!bar || configs++) {
                free(t.p);
                return fail(err, errlen, "the pass of the cell has %s", bar ? "two configurations"
                                                                            : "a malformed configuration");
            }
            text_add(&t, "config=prev|exe:%.*s|%s|%s/deflibs|%s/acidlibs\n", (int)(bar - l - 7), l + 7,
                     prev_blitz2, prev_libs, prev_libs);
        }
        l += len + (e != NULL);
    }
    if (!configs) {
        free(t.p);
        return fail(err, errlen, "the pass of the cell has no configuration");
    }
    if (mask)
        text_add(&t, "mask_traps=on\n");
    for (size_t i = 0; i < nentries; i++) {
        const char *n = entry_name(entries[i], test);

        if (!strcmp(entries[i], "ON") || (n && !strcmp(n, name)))
            text_add(&t, "intended=%s\n", entries[i]);
    }
    if (t.bad) {
        free(t.p);
        return -2;
    }
    *text = t.p;
    return 0;
}

const char *cr_prev_unknown(const char *test, const char *const *names, size_t nnames, const char *const *entries,
                            size_t nentries, size_t *cursor)
{
    for (; *cursor < nentries; (*cursor)++) {
        const char *n = entry_name(entries[*cursor], test);
        size_t k;

        if (!n)
            continue;
        for (k = 0; k < nnames && strcmp(n, names[k]); k++)
            ;
        if (k == nnames)
            return entries[(*cursor)++];
    }
    return NULL;
}

char *cr_session_key(const struct tc_config *conf, const struct tc_matrix *m, size_t cell)
{
    static const char *const keys[] = { "machine", "rtg", "timeout", "hang-after" };
    struct text t = { 0 };
    char buf[24];

    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        text_add(&t, "%s=%s\n", keys[i], val(conf, m, cell, keys[i], buf));
    /* the remote end of the serial port belongs to one program */
    if (!strcmp(val(conf, m, cell, "serial", buf), "on"))
        text_add(&t, "serial=%zu\n", cell);
    /* the lists are the same in every cell (no list is an axis) */
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

/* ------------------------------------------------------------ outputs */

int cr_parse_run(const char *run, struct cr_run *r)
{
    struct text out = { 0 };
    const char *p = run;

    memset(r, 0, sizeof *r);
    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        size_t k = n && p[n - 1] == '\r' ? n - 1 : n;

        if (k >= 10 && !memcmp(p, "REQUESTER:", 10)) {
            char **v = realloc(r->req, (r->nreq + 1) * sizeof *v);
            size_t s = k > 10 && p[10] == ' ' ? 11 : 10;

            if (!v)
                goto oom;
            r->req = v;
            if (!(v[r->nreq] = strndup(p + s, k - s)))
                goto oom;
            r->nreq++;
        } else if (k > 3 && !memcmp(p, "rc=", 3) && (!e || !e[1])) {
            /* the last line: the exit code */
            char *end;

            errno = 0;
            r->rc = strtoll(p + 3, &end, 10);
            r->has_rc = end == p + k && !errno;
            if (!r->has_rc)
                text_add(&out, "%.*s\n", (int)k, p);
        } else
            text_add(&out, "%.*s\n", (int)k, p);
        p = e ? e + 1 : p + n;
    }
    if (out.bad)
        goto oom;
    r->output = out.p ? out.p : strdup("");
    if (!r->output)
        goto oom;
    return 0;
oom:
    free(out.p);
    cr_run_free(r);
    return -1;
}

void cr_run_free(struct cr_run *r)
{
    for (size_t i = 0; i < r->nreq; i++)
        free(r->req[i]);
    free(r->req);
    free(r->output);
    memset(r, 0, sizeof *r);
}

char *cr_compile_mapped(const char *result, const struct tp_driver *d, const char *file, char **where)
{
    unsigned long dl;
    unsigned sl = 0;
    const char *name = NULL, *msg;
    char *end;

    if (where)
        *where = NULL;
    if (strncmp(result, "error ", 6) || !(dl = strtoul(result + 6, &end, 10)) || *end != ' ' || dl > 0xffffffu)
        return strdup(result);
    msg = end + 1;
    switch (tp_driver_where(d, (unsigned)dl, &sl, &name)) {
    case TP_IN_SOURCE:
        if (where && !(*where = str_printf("%s:%u: %s", file, sl, msg)))
            return NULL;
        return str_printf("error %u %s", sl, msg);
    case TP_IN_CALL:
        if (where
            && !(*where = str_printf("the driver's call of %s%s (line %lu): %s - is it in %s?",
                                     !strcmp(name, "init") ? "the batch init" : !strcmp(name, "final")
                                                                                 ? "the batch cleanup"
                                                                                 : "case ",
                                     !strcmp(name, "init") || !strcmp(name, "final") ? "" : name, dl, msg, file)))
            return NULL;
        break;
    default:
        if (where && !(*where = str_printf("line %lu of the driver (not of %s): %s", dl, file, msg)))
            return NULL;
        break;
    }
    return str_printf("error driver:%lu %s", dl, msg);
}

/* ------------------------------------------------------- the configuration */

static void print_value(FILE *out, const struct tt_value *v)
{
    switch (v->type) {
    case TT_STRING:
        fprintf(out, "\"%s\"", v->str);
        break;
    case TT_INTEGER:
        fprintf(out, "%lld", v->integer);
        break;
    case TT_BOOLEAN:
        fprintf(out, "%s", v->boolean ? "true" : "false");
        break;
    default:
        fprintf(out, "[");
        for (size_t i = 0; i < v->nlist; i++)
            fprintf(out, "%s\"%s\"", i ? ", " : "", v->list[i]);
        fprintf(out, "]");
    }
}

static void print_tab(FILE *out, const char *table, const struct tc_tabkey *k, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (k[i].sel)
            fprintf(out, "  %s.\"%s\".", table, k[i].sel);
        else
            fprintf(out, "  %s.", table);
        fprintf(out, "%s = ", k[i].key);
        print_value(out, &k[i].value);
        fprintf(out, "    # %s:%u\n", k[i].at.file, k[i].at.line);
    }
}

int cr_print_config(FILE *out, const struct tc_config *conf, const struct tc_matrix *m)
{
    size_t nkeys;
    const struct tc_keydef *keys = tc_keys(&nkeys);

    fprintf(out, "test %s (%s)\nfiles, nearest first:\n", conf->id, conf->source);
    for (size_t i = 0; i < conf->nfiles; i++)
        fprintf(out, "  %s\n", conf->files[i]);
    fprintf(out, "settings:\n");
    for (size_t k = 0; k < nkeys; k++) {
        const struct tc_setting *s = tc_get(conf, keys[k].name);
        size_t a;

        for (a = 0; a < m->naxes && m->axes[a].def != &keys[k]; a++)
            ;
        if (a < m->naxes)
            fprintf(out, "  %s: an axis, below\n", keys[k].name);
        else if (s) {
            fprintf(out, "  %s = ", keys[k].name);
            print_value(out, &s->value);
            fprintf(out, "    # %s:%u\n", s->at.file, s->at.line);
        } else if (keys[k].defval)
            fprintf(out, "  %s = \"%s\"    # (default)\n", keys[k].name, keys[k].defval);
    }
    if (conf->nmatrix) {
        fprintf(out, "matrix:\n");
        print_tab(out, "matrix", conf->matrix, conf->nmatrix);
    }
    if (conf->nasserts) {
        fprintf(out, "assertions:\n");
        print_tab(out, "asserts", conf->asserts, conf->nasserts);
    }
    if (conf->ndeviations) {
        fprintf(out, "deviations:\n");
        print_tab(out, "deviations", conf->deviations, conf->ndeviations);
    }
    {
        size_t nd;
        const struct tc_default *d = tc_defaults(&nd);

        fprintf(out, "default assertions (where no file names the query):\n");
        for (size_t i = 0; i < nd; i++)
            fprintf(out, "  %s = \"%s\"\n", d[i].query, d[i].pred);
        fprintf(out, "  every check with entries passed, unless an assertion names it\n");
    }
    fprintf(out, "axes:%s\n", m->naxes ? "" : " none");
    for (size_t i = 0; i < m->naxes; i++) {
        fprintf(out, "  %s:", m->axes[i].def->name);
        for (size_t k = 0; k < m->axes[i].nvalues; k++)
            fprintf(out, " %s", m->axes[i].values[k]);
        fprintf(out, "    # %s:%u\n", m->axes[i].at.file, m->axes[i].at.line);
    }
    fprintf(out, "cells:\n");
    for (size_t i = 0; i < m->ncells; i++)
        fprintf(out, "  %s    # side %s\n", m->cells[i].name, m->cells[i].originals ? "originals" : "port");
    return ferror(out) ? -1 : 0;
}

/* ------------------------------------------------------------ the run */

/* runs ARGV, its output to log; 0 = exit status 0 */
static int spawn(char *const argv[], FILE *log)
{
    pid_t pid;
    int st;

    fflush(log);
    if (posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ)) {
        fprintf(log, "corpusrun case: cannot start %s\n", argv[0]);
        return -1;
    }
    while (waitpid(pid, &st, 0) < 0)
        if (errno != EINTR)
            return -1;
    if (!WIFEXITED(st) || WEXITSTATUS(st)) {
        fprintf(log, "corpusrun case: %s failed\n", argv[0]);
        return -1;
    }
    return 0;
}

/* a cell of the run */
struct cell {
    size_t idx;                 /* in the matrix */
    char tag[16];               /* c0, c1, ... */
    struct corpus_pass p;
    char *key;                  /* cr_session_key */
    int done;                   /* observed */
    enum amiga_emulation_status status;
};

/* the last line of a file holds rc= */
static int has_rc(const char *path)
{
    size_t len;
    char *t = (char *)read_file(path, &len);
    int r = 0;

    if (t) {
        char *l = len && t[len - 1] == '\n' ? t + len - 1 : t + len;

        while (l > t && l[-1] != '\n')
            l--;
        r = !strncmp(l, "rc=", 3);
        free(t);
    }
    return r;
}

/* the cell's compile and (if it compiled) its run have ended */
static int cell_complete(const struct cell *c)
{
    char *b2 = str_printf("%s/%s/vol/out/blitz2.out", c->p.work, c->tag);
    char *exe = str_printf("%s/%s/vol/out/" CR_PROGRAM, c->p.work, c->tag);
    char *run = str_printf("%s/%s/vol/out/" CR_PROGRAM ".run", c->p.work, c->tag);
    int r = b2 && exe && run && has_rc(b2) && (access(exe, F_OK) || has_rc(run));

    free(b2);
    free(exe);
    free(run);
    return r;
}

/* the observation of a cell for the judge; the strings are kept in keep */
struct obs {
    struct tj_obs o;
    struct cr_run run;
    struct tj_requester *req;
    char *log, *b2, *result, *serial, *compile, *compile_log, *where;
    struct bugorig_obs bo;
};

static void obs_free(struct obs *ob)
{
    cr_run_free(&ob->run);
    free(ob->req);
    free(ob->log);
    free(ob->b2);
    free(ob->result);
    free(ob->serial);
    free(ob->compile);
    free(ob->compile_log);
    free(ob->where);
    bugorig_free(&ob->bo);
    memset(ob, 0, sizeof *ob);
}

static const char *status_name(enum amiga_emulation_status st)
{
    return st == AMIGA_EMULATION_DONE ? "done" : st == AMIGA_EMULATION_HALT ? "halt" : "timeout";
}

static int observe_cell(const struct cell *c, const struct tp_driver *d, const char *file, struct obs *ob)
{
    char *dir = str_printf("%s/%s/vol/out", c->p.work, c->tag), *path;
    size_t len;
    char *run = NULL;
    struct bugorig_outputs bout;
    const char *comp;
    int r = -1;

    memset(ob, 0, sizeof *ob);
    if (!dir)
        return -1;
    if ((path = str_printf("%s/session.log", dir)))
        ob->log = (char *)read_file(path, &len);
    free(path);
    if ((path = str_printf("%s/blitz2.out", dir)))
        ob->b2 = (char *)read_file(path, &len);
    free(path);
    if ((path = str_printf("%s/" CR_PROGRAM ".run", dir)))
        run = (char *)read_file(path, &len);
    free(path);
    if ((path = str_printf("%s/" CR_PROGRAM ".serial", dir)))
        ob->serial = (char *)read_file(path, &len);
    free(path);
    if ((path = str_printf("%s/" CR_PROGRAM ".res", dir))) {
        ob->result = (char *)read_file(path, &ob->o.result_len);
        ob->o.has_result = ob->result != NULL;
        ob->o.result = ob->result;
    }
    free(path);
    bout.session = status_name(c->status);
    bout.log = ob->log;
    bout.blitz2 = ob->b2;
    bout.run = run;
    bout.job = "j000";
    if (bugorig_observe(&bout, &ob->bo) || cr_parse_run(run, &ob->run))
        goto out;
    comp = bugorig_get(&ob->bo, "compile");
    {
        char *c1 = comp ? strndup(comp, strcspn(comp, "\n")) : strdup("none");

        if (!c1)
            goto out;
        ob->compile = cr_compile_mapped(c1, d, file, &ob->where);
        free(c1);
        if (!ob->compile)
            goto out;
    }
    {
        const char *b = bugorig_get(&ob->bo, "blitz2"), *l = bugorig_get(&ob->bo, "log");

        if (!(ob->compile_log = str_printf("%s%s", b ? b : "", l ? l : "")))
            goto out;
    }
    if (ob->run.nreq && !(ob->req = calloc(ob->run.nreq, sizeof *ob->req)))
        goto out;
    for (size_t i = 0; i < ob->run.nreq; i++) {
        ob->req[i].text = ob->run.req[i];
        /* the output does not say when the requester came against the
           result file: after every line of it */
        ob->req[i].after = (size_t)-1;
    }
    ob->o.session = status_name(c->status);
    ob->o.compile = ob->compile;
    ob->o.compile_log = ob->compile_log;
    ob->o.output = run ? ob->run.output : NULL;
    ob->o.serial = ob->serial;
    ob->o.inject = NULL;
    ob->o.has_rc = ob->run.has_rc;
    ob->o.rc = ob->run.rc;
    ob->o.req = ob->req;
    ob->o.nreq = ob->run.nreq;
    r = 0;
out:
    free(run);
    free(dir);
    return r;
}

/* the session of the cells cs[0..n): all compiles, then all runs */
static char *session_script(struct cell **cs, size_t n)
{
    struct text t = { 0 };

    for (int part = 0; part < 2; part++)
        for (size_t i = 0; i < n; i++) {
            char *s = part ? corpus_script_run(&cs[i]->p) : corpus_script_compile(&cs[i]->p);

            if (!s) {
                free(t.p);
                return NULL;
            }
            text_add(&t, "Assign CORPUS: CELL%zu:\n%s", i, s);
            free(s);
        }
    text_add(&t, "Assign CORPUS:\n");
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

/* runs the cells cs[0..n) in one session and marks those that ended; the
   others stay for another session.  0 = ok, 77 = no emulator, 2 = error */
static int run_session(struct cell **cs, size_t n, const char *base, FILE *log, size_t *nsessions)
{
    struct corpus_volume *vols = calloc(n, sizeof *vols);
    char **names = calloc(n, sizeof *names), **dirs = calloc(n, sizeof *dirs);
    char *script = session_script(cs, n), err[400];
    struct amiga_emulation *e = NULL;
    struct responder *resp = NULL;
    unsigned port = 0;
    struct statline stl;
    enum amiga_emulation_status st;
    int r = 2;

    if (!vols || !names || !dirs || !script)
        goto out;
    for (size_t i = 0; i < n; i++) {
        if (corpus_stage(&cs[i]->p, 0, log))
            goto out;
        if (!(names[i] = str_printf("CELL%zu", i)) || !(dirs[i] = str_printf("%s/%s/vol", cs[i]->p.work, cs[i]->tag)))
            goto out;
        vols[i].volume = names[i];
        vols[i].host_dir = dirs[i];
    }
    if (cs[0]->p.serial && (!(port = serial_free_port()) || !(resp = serial_start(port)))) {
        fprintf(log, "corpusrun case: no TCP port for the serial port\n");
        goto out;
    }
    fprintf(log, "corpusrun case: emulator session %zu:", *nsessions);
    for (size_t i = 0; i < n; i++)
        fprintf(log, " %s", cs[i]->tag);
    fprintf(log, "\n");
    fflush(log);
    while (!(e = corpus_session_start(&cs[0]->p, 0, script, port, vols, n, err, sizeof err))) {
        if (!amiga_emulation_no_slot()) {
            fprintf(log, "corpusrun case: %s - skipped\n", err);
            r = 77;
            goto out;
        }
        {
            struct timespec ts = { AMIGA_EMULATION_SLOT_RETRY_MS / 1000,
                                   (long)(AMIGA_EMULATION_SLOT_RETRY_MS % 1000) * 1000000L };

            nanosleep(&ts, NULL);
        }
    }
    st = amiga_emulation_join(e);
    {
        char tag[40];

        snprintf(tag, sizeof tag, "s%zu", *nsessions);
        statline_session(&stl, base, tag, e);
    }
    (*nsessions)++;
    if (resp) {
        char *out = str_printf("%s/out", dirs[0]);

        serial_stop(resp, out ? out : ".");
        resp = NULL;
        free(out);
    }
    /* the cells that ended; the first that did not gets the session's end */
    for (size_t i = 0; i < n; i++) {
        if (st == AMIGA_EMULATION_DONE || cell_complete(cs[i])) {
            cs[i]->done = 1;
            cs[i]->status = AMIGA_EMULATION_DONE;
            continue;
        }
        cs[i]->done = 1;
        cs[i]->status = st == AMIGA_EMULATION_HALT ? st : AMIGA_EMULATION_TIMEOUT;
        if (i + 1 < n)
            fprintf(log, "corpusrun case: session ended (%s) in %s; the cells behind it run again\n",
                    amiga_emulation_status_name(st), cs[i]->tag);
        break;
    }
    statline_set(stl.verdict, sizeof stl.verdict, st == AMIGA_EMULATION_DONE ? "done" : "ended");
    statline_write(log, &stl);
    r = 0;
out:
    if (resp)
        serial_stop(resp, ".");
    if (e)
        amiga_emulation_free(e);
    for (size_t i = 0; names && i < n; i++)
        free(names[i]);
    for (size_t i = 0; dirs && i < n; i++)
        free(dirs[i]);
    free(names);
    free(dirs);
    free(vols);
    free(script);
    return r;
}

/* the files a cell needs that are not there (the CD, the build of blitz2.s):
   1 = skip */
static int missing(const struct corpus_pass *p, FILE *log)
{
    const char *f[] = { p->configs[0].blitz2, p->configs[0].deflibs, p->configs[0].acidlibs, p->ted,
                        p->launcher, p->defaultdbug, p->wrap, p->runwrap };

    for (size_t i = 0; i < sizeof f / sizeof f[0]; i++)
        if (f[i] && access(f[i], R_OK)) {
            fprintf(log, "corpusrun case: no %s - skipped\n", f[i]);
            return 1;
        }
    for (size_t i = 0; i < p->nfiles; i++)
        if (access(p->file_src[i], R_OK)) {
            fprintf(log, "corpusrun case: no %s - skipped\n", p->file_src[i]);
            return 1;
        }
    for (size_t i = 0; i < p->nmounts; i++)
        if (access(p->mount_dir[i], R_OK | X_OK)) {
            fprintf(log, "corpusrun case: no %s for %s: - skipped\n", p->mount_dir[i], p->mount_name[i]);
            return 1;
        }
    return 0;
}

static const char *verdict_name(int v)
{
    static const char *const n[] = { "PASSED", "REPRODUCED", "ERROR", "BROKEN", "FAILED" };

    return v >= 0 && v <= 4 ? n[v] : "?";
}

/* the token list of the cells: the build's, or one with the RWE user libraries
   (bbsrc tokens), written to dir/tokens.txt */
static char *token_list(const struct tc_config *conf, const struct tc_env *env, const char *dir, FILE *log,
                        char *err, size_t errlen)
{
    const struct tt_value *l = list(conf, "rwe");
    const char *tokens = need(env, "tokens", err, errlen);
    const char *bbsrc, *blitz2, *testlib;
    char *out, *vdir = NULL, *acid = NULL, *def = NULL;
    char **argv;
    size_t n = 0;
    int bad;

    if (!tokens)
        return NULL;
    if (!l || !l->nlist)
        return strdup(tokens);
    if (!(bbsrc = need(env, "bbsrc", err, errlen)) || !(blitz2 = need(env, "blitz2", err, errlen))
        || !(testlib = need(env, "testlib", err, errlen)))
        return NULL;
    if (!(out = str_printf("%s/tokens.txt", dir)) || !(argv = calloc(l->nlist + 10, sizeof *argv))) {
        free(out);
        fail(err, errlen, "out of memory");
        return NULL;
    }
    argv[n++] = (char *)bbsrc;
    argv[n++] = (char *)"tokens";
    argv[n++] = (char *)"-o";
    argv[n++] = out;
    argv[n++] = (char *)blitz2;
    for (size_t i = 0; i < l->nlist; i++) {
        char key[200];
        const char *info, *bar1, *bar2;

        snprintf(key, sizeof key, "rwe-%s", l->list[i]);
        if (!(info = tc_env_runner(env, key)) || !(bar1 = strchr(info, '|')) || !(bar2 = strchr(bar1 + 1, '|'))) {
            fail(err, errlen, "%s: no RWE user library %s in the environment file", conf->source, l->list[i]);
            goto bad;
        }
        if (!vdir) {
            vdir = strdup(bar2 + 1);
            acid = str_printf("%s/acidlibs", bar2 + 1);
            def = str_printf("%s/deflibs", bar2 + 1);
            argv[n++] = acid;
            argv[n++] = def;
        }
        argv[n++] = strndup(info, (size_t)(bar1 - info));
    }
    argv[n++] = (char *)testlib;
    argv[n] = NULL;
    bad = spawn(argv, log);
    for (size_t i = 7; i + 1 < n; i++)
        free(argv[i]);
    free(argv);
    free(vdir);
    free(acid);
    free(def);
    if (bad) {
        fail(err, errlen, "the token list with the RWE user libraries cannot be made");
        free(out);
        return NULL;
    }
    return out;
bad:
    for (size_t i = 7; i < n; i++)
        free(argv[i]);
    free(argv);
    free(vdir);
    free(acid);
    free(def);
    free(out);
    return NULL;
}

/* the base name of a path */
static const char *base_name(const char *path)
{
    const char *s = strrchr(path, '/');

    return s ? s + 1 : path;
}

/* the comparison with the prev build (caserun.h): a pass per value of
   runtime-errors of the side port */
static int prev_run(const struct caserun_opts *o, const struct tc_env *env, const struct tc_config *conf,
                    const struct tc_matrix *m, FILE *log)
{
    static const char *const values[] = { "off", "on" };
    char err[800], *names[2] = { NULL, NULL }, *dir = NULL;
    const char *ok_names[2];
    const char *work = need(env, "work", err, sizeof err);
    const struct tt_value *rwe = list(conf, "rwe");
    size_t nnames = 0, cur = 0;
    const char *stale;
    int rc = 0, any = 0, skipped = 0;

    if (!o->driver || !o->test || !o->prev_blitz2 || !o->prev_libs) {
        fprintf(log, "corpusrun case: --prev needs --driver, --test, --prev-blitz2 and --prev-libs\n");
        return 2;
    }
    if (rwe && rwe->nlist) {
        fprintf(log, "corpusrun case: %s: a test with RWE user libraries is not compared with the prev build\n",
                conf->id);
        return 2;
    }
    if (!work || !(dir = str_printf("%s/%s", work, o->test)) || mkdir_p(dir)) {
        fprintf(log, "corpusrun case: %s\n", work ? "cannot create the work directory" : err);
        free(dir);
        return 2;
    }
    for (size_t v = 0; v < 2; v++) {
        char b[24], *src = NULL, *wdir = NULL, *cp = NULL, *pt = NULL;
        size_t cell;
        struct corpus_pass p = { 0 };
        int r;

        for (cell = 0; cell < m->ncells; cell++)
            if (!m->cells[cell].originals && !strcmp(val(conf, m, cell, "runtime-errors", b), values[v]))
                break;
        if (cell == m->ncells)
            continue;
        if (!(names[nnames] = str_printf("%s%s", conf->id, v ? CR_PREV_RUNERRS : ""))
            || !(src = str_printf("%s/%s.bb2", dir, names[nnames]))
            || !(wdir = str_printf("%s/%s", dir, names[nnames]))) {
            fprintf(log, "corpusrun case: out of memory\n");
            free(src);
            rc = 2;
            break;
        }
        ok_names[nnames] = names[nnames];
        nnames++;
        if (copy_file(o->driver, src)) {
            fprintf(log, "corpusrun case: cannot copy the driver %s\n", o->driver);
            r = 2;
        } else if ((r = cr_cell_pass(conf, m, cell, env, "clib", wdir, src, &cp, err, sizeof err))
                   || (r = cr_prev_pass(cp, o->test, names[nnames - 1], wdir, src, o->prev_blitz2, o->prev_libs,
                                        o->intended, o->nintended, &pt, err, sizeof err))) {
            fprintf(log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
            r = 2;
        } else if (corpus_parse(pt, &p, err, sizeof err)) {
            fprintf(log, "corpusrun case: %s: %s\n", names[nnames - 1], err);
            r = 2;
        } else {
            corpus_select(&p);
            corpus_skip_missing(&p, log);
            r = corpus_run(&p, log);
            any = 1;
        }
        corpus_free(&p);
        free(src);
        free(wdir);
        free(cp);
        free(pt);
        if (r == 77)
            skipped = 1;
        else if (r > rc)
            rc = r;
    }
    if (!nnames && rc == 0) {
        fprintf(log, "corpusrun case: %s has no cell of the side port\n", conf->id);
        rc = 2;
    }
    /* an entry of the test that names no compile of it: stale, as one without a difference */
    while ((stale = cr_prev_unknown(o->test, ok_names, nnames, o->intended, o->nintended, &cur)) != NULL) {
        fprintf(log, "STALE %s: intended, but the test has no such compile (its compiles:", stale);
        for (size_t i = 0; i < nnames; i++)
            fprintf(log, " %s", ok_names[i]);
        fprintf(log, ")\n");
        if (rc < 1)
            rc = 1;
    }
    if (rc == 0 && skipped)
        rc = 77;
    if (rc == 0 && !any)
        rc = 2;
    fprintf(log, "%s: the comparison with the prev build: %s\n", conf->id,
            rc == 0 ? "the same" : rc == 1 ? "DIFFERS" : rc == 77 ? "skipped" : "ERROR");
    free(names[0]);
    free(names[1]);
    free(dir);
    return rc;
}

/* the driver of all cases, tokenised, to o->write_driver (its text beside it) */
static int write_driver(const struct caserun_opts *o, const struct tc_env *env, const struct tc_config *conf,
                        const struct tp_driver *drv, FILE *log)
{
    const char *out = o->write_driver, *slash = strrchr(out, '/');
    size_t n = strlen(out);
    char err[800], *bas, *dir, *tokens = NULL;
    const char *tedsrc = need(env, "tedsrc", err, sizeof err);
    int rc = 2;

    if (n > 4 && !strcmp(out + n - 4, ".bb2"))
        bas = str_printf("%.*s.bas", (int)(n - 4), out);
    else
        bas = str_printf("%s.bas", out);
    dir = slash ? strndup(out, (size_t)(slash - out)) : strdup(".");
    if (!bas || !dir) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    if (!tedsrc) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (mkdir_p(dir) || write_file(bas, drv->text, drv->len)) {
        fprintf(log, "corpusrun case: cannot write %s\n", bas);
        goto out;
    }
    /* the token list with the RWE user libraries goes beside the driver,
       in a directory of its own (the drivers of a directory are written at
       the same time) */
    free(dir);
    if (!(dir = str_printf("%s.d", out)) || (list(conf, "rwe") && mkdir_p(dir))) {
        fprintf(log, "corpusrun case: cannot create %s\n", dir ? dir : "a directory");
        goto out;
    }
    if (!(tokens = token_list(conf, env, dir, log, err, sizeof err))) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    {
        char *argv[] = { (char *)tedsrc, (char *)"tok", tokens, bas, (char *)out, NULL };

        if (spawn(argv, log)) {
            fprintf(log, "corpusrun case: the driver %s cannot be tokenised\n", bas);
            goto out;
        }
    }
    rc = 0;
out:
    free(bas);
    free(dir);
    free(tokens);
    return rc;
}

int caserun(const struct caserun_opts *o, FILE *log)
{
    struct tc_env env = { 0 };
    struct tc_config conf = { 0 };
    struct tc_matrix m = { 0 };
    struct tp_source src = { 0 };
    struct tp_driver drv = { 0 };
    struct runwork rw = { NULL, NULL, -1 };
    struct cell *cells = NULL;
    struct obs *obs = NULL;
    size_t ncells = 0, nsessions = 0;
    char err[800], *text = NULL, *workbase = NULL, *drvpath = NULL, *bb2 = NULL, *tokens = NULL;
    const char *file = base_name(o->source), *macros_path;
    size_t len;
    int rc = 2, ran = 0;

    if (tc_env_load(o->env, &env, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (tc_resolve(o->source, &env, &conf, err, sizeof err) || tc_matrix_build(&conf, &m, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (o->print_config) {
        rc = cr_print_config(log, &conf, &m) ? 2 : 0;
        goto out;
    }
    if (o->prev) {
        rc = prev_run(o, &env, &conf, &m, log);
        goto out;
    }
    if (!(text = (char *)read_file(o->source, &len))) {
        fprintf(log, "corpusrun case: cannot read %s\n", o->source);
        goto out;
    }
    {
        struct tp_params par = { 0 };
        int n;

        par.file = file;
        par.test_id = conf.id;
        par.testdata = CR_TESTDATA;
        if ((n = tp_process(text, len, &par, &src)) != 0) {
            for (size_t i = 0; i < src.errors.n; i++)
                fprintf(log, "%s\n", src.errors.v[i]);
            if (n < 0)
                fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
    }
    {
        struct tp_driver_params dp = { 0 };
        size_t mlen;
        char *mac;
        int n;
        const char *one[1] = { o->case_name };
        const char **cases = one;
        size_t ncases = 1;

        /* the driver of the build step: every case */
        if (!o->case_name) {
            if (!(cases = calloc(src.ncases + 1, sizeof *cases))) {
                fprintf(log, "corpusrun case: out of memory\n");
                goto out;
            }
            for (ncases = 0; ncases < src.ncases; ncases++)
                cases[ncases] = src.cases[ncases].name;
        }

        if (!(macros_path = need(&env, "macros", err, sizeof err))) {
            fprintf(log, "corpusrun case: %s\n", err);
            if (cases != one)
                free(cases);
            goto out;
        }
        if (!(mac = (char *)read_file(macros_path, &mlen))) {
            fprintf(log, "corpusrun case: cannot read %s\n", macros_path);
            if (cases != one)
                free(cases);
            goto out;
        }
        dp.macros = mac;
        dp.macros_len = mlen;
        dp.result = CR_RESULT;
        dp.cases = cases;
        dp.ncases = ncases;
        n = tp_driver_write(&src, &dp, &drv, err, sizeof err);
        free(mac);
        if (cases != one)
            free(cases);
        if (n) {
            fprintf(log, "corpusrun case: %s\n", n == -2 ? "out of memory" : err);
            goto out;
        }
    }
    if (o->print_driver) {
        fwrite(drv.text, 1, drv.len, log);
        rc = ferror(log) ? 2 : 0;
        goto out;
    }
    if (o->write_driver) {
        rc = write_driver(o, &env, &conf, &drv, log);
        goto out;
    }

    /* the cells of the side */
    for (size_t i = 0; i < m.ncells; i++)
        if (m.cells[i].originals == o->originals)
            ncells++;
    if (!ncells) {
        fprintf(log, "corpusrun case: %s has no cell of the side %s\n", conf.id,
                o->originals ? "originals" : "port");
        goto out;
    }
    if (!(cells = calloc(ncells, sizeof *cells)) || !(obs = calloc(ncells, sizeof *obs))) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    {
        const char *work = need(&env, "work", err, sizeof err);

        if (!work || !(workbase = str_printf("%s/%s__%s%s", work, conf.id, o->case_name,
                                             o->originals ? "_originals" : ""))) {
            fprintf(log, "corpusrun case: %s\n", work ? "out of memory" : err);
            goto out;
        }
    }
    if (runwork_open(&rw, workbase, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (runwork_space(workbase, RUNWORK_MIN_FREE_MIB, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    /* the driver, tokenised */
    if (!(drvpath = str_printf("%s/driver.bas", rw.dir)) || !(bb2 = str_printf("%s/" CR_PROGRAM ".bb2", rw.dir))
        || write_file(drvpath, drv.text, drv.len)) {
        fprintf(log, "corpusrun case: cannot write the driver\n");
        goto out;
    }
    if (!(tokens = token_list(&conf, &env, rw.dir, log, err, sizeof err))) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    {
        const char *tedsrc = need(&env, "tedsrc", err, sizeof err);
        char *argv[] = { (char *)tedsrc, (char *)"tok", tokens, drvpath, bb2, NULL };

        if (!tedsrc) {
            fprintf(log, "corpusrun case: %s\n", err);
            goto out;
        }
        if (spawn(argv, log)) {
            fprintf(log, "corpusrun case: the driver %s cannot be tokenised\n", drvpath);
            goto out;
        }
    }
    /* a pass per cell */
    {
        size_t k = 0;

        for (size_t i = 0; i < m.ncells; i++) {
            struct cell *c;
            char *pt;
            int r;

            if (m.cells[i].originals != o->originals)
                continue;
            c = &cells[k];
            c->idx = i;
            snprintf(c->tag, sizeof c->tag, "c%zu", k);
            k++;
            if ((r = cr_cell_pass(&conf, &m, i, &env, c->tag, rw.dir, bb2, &pt, err, sizeof err))) {
                fprintf(log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
                goto out;
            }
            r = corpus_parse(pt, &c->p, err, sizeof err);
            free(pt);
            if (r) {
                fprintf(log, "corpusrun case: cell %s: %s\n", m.cells[i].name, err);
                goto out;
            }
            if (missing(&c->p, log)) {
                rc = 77;
                goto out;
            }
            if (!(c->key = cr_session_key(&conf, &m, i))) {
                fprintf(log, "corpusrun case: out of memory\n");
                goto out;
            }
        }
    }
    /* the sessions: the cells with the same key, in order; again for the cells
       a session did not reach */
    for (;;) {
        struct cell **group = calloc(ncells, sizeof *group);
        size_t n = 0, first;
        int r;

        if (!group) {
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        for (first = 0; first < ncells && cells[first].done; first++)
            ;
        if (first == ncells) {
            free(group);
            break;
        }
        for (size_t i = first; i < ncells; i++)
            if (!cells[i].done && !strcmp(cells[i].key, cells[first].key))
                group[n++] = &cells[i];
        ran = 1;
        r = run_session(group, n, workbase, log, &nsessions);
        free(group);
        if (r) {
            rc = r;
            goto out;
        }
    }
    /* the observations and the verdict */
    {
        size_t *idx = calloc(ncells, sizeof *idx);
        struct tj_obs *to = calloc(ncells, sizeof *to);
        struct tj_msgs msgs = { 0 };
        struct tj_test t = { file, &src, &conf, &m, o->case_name, o->require_deviation };
        int v;

        if (!idx || !to) {
            free(idx);
            free(to);
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        for (size_t i = 0; i < ncells; i++) {
            if (observe_cell(&cells[i], &drv, file, &obs[i])) {
                free(idx);
                free(to);
                fprintf(log, "corpusrun case: out of memory\n");
                goto out;
            }
            idx[i] = cells[i].idx;
            to[i] = obs[i].o;
        }
        fprintf(log, "== %s, case %s, side %s:", file, o->case_name, o->originals ? "originals" : "port");
        for (size_t i = 0; i < ncells; i++)
            fprintf(log, " %s (%s)", m.cells[cells[i].idx].name, cells[i].tag);
        fprintf(log, "\n");
        for (size_t i = 0; i < ncells; i++)
            if (obs[i].where)
                fprintf(log, "cell %s: compile error: %s\n", m.cells[cells[i].idx].name, obs[i].where);
        v = tj_verdict(&t, idx, to, ncells, &msgs);
        for (size_t i = 0; i < msgs.n; i++)
            fprintf(log, "%s\n", msgs.v[i]);
        tj_msgs_free(&msgs);
        free(idx);
        free(to);
        if (v < 0) {
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        fprintf(log, "%s: case %s: %s\n", file, o->case_name, verdict_name(v));
        rc = v;
    }
out:
    if (rw.fd >= 0)
        rc = runwork_finish(&rw, ran, rc, NULL, NULL, log, "corpusrun case");
    for (size_t i = 0; cells && i < ncells; i++) {
        corpus_free(&cells[i].p);
        free(cells[i].key);
    }
    for (size_t i = 0; obs && i < ncells; i++)
        obs_free(&obs[i]);
    free(obs);
    free(cells);
    free(text);
    free(workbase);
    free(drvpath);
    free(bb2);
    free(tokens);
    tp_driver_free(&drv);
    tp_free(&src);
    tc_matrix_free(&m);
    tc_free(&conf);
    tc_env_free(&env);
    return rc;
}
