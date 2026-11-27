/*
 * caserun.c - one case of a Blitz test of the BASIC test model (caserun.h).
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup */
#include "caserun.h"

#include <errno.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "basictest.h"
#include "bugorig.h"
#include "corpus.h"
#include "rundir.h"
#include "runwork.h"
#include "serial.h"
#include "statline_emulation.h"
#include "testpp.h"
#include "util.h"
#include "verdicts.h"

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

/* the side of a run: port, originals or oracle (AmiBlitz3's cells) */
static int on_side(const struct tc_cell *c, const struct caserun_opts *o)
{
    return o->oracle ? c->oracle : !c->oracle && c->originals == o->originals;
}

static const char *side_name(const struct caserun_opts *o)
{
    return o->oracle ? "oracle" : o->originals ? "originals" : "port";
}

/* the suffix of the side's names: the test, its work directory */
static const char *side_suffix(const struct caserun_opts *o)
{
    return o->oracle ? "_oracle" : o->originals ? "_originals" : "";
}

/* ------------------------------------------------------- the pass of a cell */

/* the compiler, deflibs and acidlibs of a build configuration (compiler,
   libs) of a cell; rwe_dir: the variant's directory of the cell's RWE user
   libraries (the C libraries of the cell), NULL none.  0, -1 (err) */
static int build_files(const struct tc_env *env, const char *compiler, const char *libs, const char *rwe_dir,
                       const char **blitz2, char **deflibs, char **acidlibs, char *err, size_t errlen)
{
    const char *d, *a;

    *deflibs = *acidlibs = NULL;
    if (!(*blitz2 = need(env, !strcmp(compiler, "port") ? "blitz2" : compiler, err, errlen)))
        return -1;
    if (!strcmp(libs, "cd")) {
        if (!(d = need(env, "cd-deflibs", err, errlen)) || !(a = need(env, "cd-acidlibs", err, errlen)))
            return -1;
        *deflibs = strdup(d);
        *acidlibs = strdup(a);
    } else if (rwe_dir) {
        *deflibs = str_printf("%s/deflibs", rwe_dir);
        *acidlibs = str_printf("%s/acidlibs", rwe_dir);
    } else {
        if (!(d = need(env, "clib-deflibs", err, errlen)) || !(a = need(env, "clib-acidlibs", err, errlen)))
            return -1;
        *deflibs = strdup(d);
        *acidlibs = strdup(a);
    }
    if (!*deflibs || !*acidlibs) {
        free(*deflibs);
        free(*acidlibs);
        *deflibs = *acidlibs = NULL;
        return fail(err, errlen, "out of memory");
    }
    return 0;
}

/* the variant directory of the cell's RWE user libraries, NULL none */
static const char *rwe_dir_of(const struct tc_config *conf, const struct tc_env *env)
{
    const struct tt_value *l = list(conf, "rwe");
    char key[200];
    const char *info, *bar1, *bar2;

    if (!l || !l->nlist)
        return NULL;
    snprintf(key, sizeof key, "rwe-%s", l->list[0]);
    if (!(info = tc_env_runner(env, key)) || !(bar1 = strchr(info, '|')) || !(bar2 = strchr(bar1 + 1, '|')))
        return NULL;            /* refused by cr_cell_pass */
    return bar2 + 1;
}

/* the build configuration of data entry d in a cell whose values are
   cell[TC_DO_N]: o[] its compiler, libraries and runtime error checking
   ("same" the cell's); 1 = another than the cell's */
static int data_config(const struct tc_data *d, const char *const *cell, const char **o)
{
    int other = 0;

    for (int k = 0; k < TC_DO_N; k++) {
        o[k] = strcmp(d->opt[k], "same") ? d->opt[k] : cell[k];
        other |= strcmp(o[k], cell[k]) != 0;
    }
    return other;
}

static int same_config(const char *const *a, const char *const *b)
{
    for (int k = 0; k < TC_DO_N; k++)
        if (strcmp(a[k], b[k]))
            return 0;
    return 1;
}

/* where the data source i (basic-compile) is compiled: 0 in the cell's job
   list, else K of its part ID.test/.build-K, the parts numbered in the
   order their configurations first come (*first: i is the first data
   source of its part); *job its job number NNN of bNNN, its position
   among the data sources */
static size_t data_part(const struct tc_config *conf, const char *const *cell, size_t i, int *first, size_t *job)
{
    const char *c[TC_DO_N];
    size_t k = 0;

    *first = 0;
    *job = 0;
    for (size_t j = 0; j < i; j++)
        *job += conf->data[j].action == TC_DA_BASIC_COMPILE;
    if (!data_config(&conf->data[i], cell, c))
        return 0;
    for (size_t j = 0; j <= i; j++) {
        const char *o[TC_DO_N];
        int firstj = 1;

        if (conf->data[j].action != TC_DA_BASIC_COMPILE || !data_config(&conf->data[j], cell, o))
            continue;
        for (size_t h = 0; h < j && firstj; h++) {
            const char *q[TC_DO_N];

            if (conf->data[h].action == TC_DA_BASIC_COMPILE && data_config(&conf->data[h], cell, q)
                && same_config(q, o))
                firstj = 0;
        }
        k += firstj;
        if (same_config(o, c)) {
            *first = j == i;
            return k;
        }
    }
    return k;                   /* not reached: j == i matches */
}

/* the cell's values of the options of basic-compile */
static void cell_config(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, char buf[TC_DO_N][24],
                        const char **v)
{
    v[TC_DO_COMPILER] = val(conf, m, cell, "compiler", buf[0]);
    v[TC_DO_LIBS] = val(conf, m, cell, "libs", buf[1]);
    v[TC_DO_RUNERR] = val(conf, m, cell, "runtime-errors", buf[2]);
}

char *cr_data_staged(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const char *file)
{
    char buf[TC_DO_N][24];
    const char *cv[TC_DO_N];

    cell_config(conf, m, cell, buf, cv);
    for (size_t i = 0; i < conf->ndata; i++)
        if (conf->data[i].action == TC_DA_BASIC_COMPILE && !strcmp(conf->data[i].file, file)) {
            int first;
            size_t job, k = data_part(conf, cv, i, &first, &job);

            return k ? str_printf("CORPUS:%s.test/.build-%zu/src/b%03zu.bb2", conf->id, k, job)
                     : str_printf("CORPUS:src/b%03zu.bb2", job);
        }
    return NULL;
}

int cr_data_pass(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env,
                 const char *const *built, char **text, char *err, size_t errlen)
{
    char buf[TC_DO_N][24];
    const char *cv[TC_DO_N];
    const char *rwe = rwe_dir_of(conf, env);
    struct text t = { 0 };
    int r = -2;

    *text = NULL;
    cell_config(conf, m, cell, buf, cv);
    for (size_t i = 0; i < conf->ndata; i++) {
        const struct tc_data *d = &conf->data[i];
        char *res = tc_data_result(d);
        const char *o[TC_DO_N];
        size_t k, job;
        int first;

        if (!res)
            goto out;
        if (d->action != TC_DA_BASIC_COMPILE) {
            text_add(&t, "file=%s.test/%s|%s\n", conf->id, res, built[i]);
            free(res);
            continue;
        }
        if (!(k = data_part(conf, cv, i, &first, &job))) {
            text_add(&t, "build_job=-|%s|%s.test/%s\n", built[i], conf->id, res);
            free(res);
            continue;
        }
        if (first) {
            const char *blitz2;
            char *def, *acid;

            data_config(d, cv, o);
            if (build_files(env, o[TC_DO_COMPILER], o[TC_DO_LIBS], strcmp(o[TC_DO_LIBS], "cd") ? rwe : NULL,
                            &blitz2, &def, &acid, err, errlen)) {
                free(res);
                r = -1;
                goto out;
            }
            text_add(&t, "build=%s.test/.build-%zu|%s|%s|%s|%s\n", conf->id, k, blitz2, def, acid,
                     o[TC_DO_RUNERR]);
            free(def);
            free(acid);
        }
        text_add(&t, "build_job=%s.test/.build-%zu|%s|%s.test/%s\n", conf->id, k, built[i], conf->id, res);
        free(res);
    }
    if (t.bad)
        goto out;
    *text = t.p ? t.p : strdup("");
    t.p = NULL;
    r = *text ? 0 : -2;
out:
    if (r == -2)
        fail(err, errlen, "out of memory");
    free(t.p);
    return r;
}


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
    /* the debugger module: the setting's, else the CD's for the cells with
       original material */
    if (*(v = val(conf, m, cell, "defaultdbug", b2)))
        text_add(&t, "defaultdbug=%s\n", v);
    else if (originals && (v = tc_env_runner(env, "cd-defaultdbug")))
        text_add(&t, "defaultdbug=%s\n", v);
    /* the program runs from the editor instead of from the shell */
    if (!strcmp(val(conf, m, cell, "editor-run", b2), "on"))
        text_add(&t, "editrun=on\n");
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
            { "floppy", "floppy" }, { "uae", "uae" },
        };

        for (size_t k = 0; k < sizeof lists / sizeof lists[0]; k++)
            if ((l = list(conf, lists[k].key)))
                for (size_t i = 0; i < l->nlist; i++)
                    text_add(&t, "%s=%s\n", lists[k].pass, l->list[i]);
    }
    /* the job lines, @DRIVER@ the driver's source on the cell's volume,
       @DATA:FILE@ the staged source of the data source FILE */
    if ((l = list(conf, "jobline"))) {
        const char *b = strrchr(source, '/');
        size_t bl;

        b = b ? b + 1 : source;
        bl = strlen(b);
        if (bl > 4 && !strcmp(b + bl - 4, ".bb2"))
            bl -= 4;
        for (size_t i = 0; i < l->nlist; i++) {
            const char *s = l->list[i], *p;

            text_add(&t, "jobline=");
            while (*s) {
                const char *dr = strstr(s, TC_DRIVER), *da = strstr(s, TC_DATA_PH), *q;

                if (da && (!dr || da < dr) && (q = strchr(da + strlen(TC_DATA_PH), '@'))) {
                    char *file = strndup(da + strlen(TC_DATA_PH), (size_t)(q - da - strlen(TC_DATA_PH)));
                    char *staged = file ? cr_data_staged(conf, m, cell, file) : NULL;

                    if (!staged) {
                        int oom = !file;

                        free(file);
                        free(t.p);
                        if (oom)
                            return -2;
                        return fail(err, errlen, "%s: jobline: %.*s names no data source of the test", conf->source,
                                    (int)(q - da + 1), da);
                    }
                    text_add(&t, "%.*s%s", (int)(da - s), s, staged);
                    free(file);
                    free(staged);
                    s = q + 1;
                } else if ((p = dr)) {
                    text_add(&t, "%.*sCORPUS:src/%.*s.bb2", (int)(p - s), s, (int)bl, b);
                    s = p + strlen(TC_DRIVER);
                } else {
                    text_add(&t, "%s", s);
                    break;
                }
            }
            text_add(&t, "\n");
        }
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
    /* the keys of a run, not of a compile (as prev_compile_test drops them; rcput stays: a compile
       on a Kickstart 1.x machine needs it, elsewhere it does nothing), and
       the ones this pass sets itself */
    static const char *const drop[] = { "cell", "launcher", "hang_after", "hang_from", "serial",
                                        "runwrap", "runargs", "launcherargs", "defaultdbug", "editrun", "floppy",
                                        "parallel", "report", "work", "source", "run", "test", "intended" };
    struct text t = { 0 };
    const char *l = cell_pass;
    int configs = 0, mask = 0;

    *text = NULL;
    text_add(&t, "test=%s\nwork=%s\nreport=%s.txt\nrun=off\nparallel=2\n", test, work, work);
    if (source)
        text_add(&t, "source=%s\n", source);
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

int cr_prev_data(const char *test, const char *const *sources, const char *const *names, size_t n,
                 const char *const *entries, size_t nentries, char **text)
{
    struct text t = { 0 };

    *text = NULL;
    for (size_t i = 0; i < n; i++) {
        text_add(&t, "source=%s|%s\n", sources[i], names[i]);
        for (size_t k = 0; k < nentries; k++) {
            const char *e = entry_name(entries[k], test);

            if (e && !strcmp(e, names[i]))
                text_add(&t, "intended=%s\n", entries[k]);
        }
    }
    if (t.bad) {
        free(t.p);
        return -2;
    }
    *text = t.p ? t.p : strdup("");
    return *text ? 0 : -2;
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

char *cr_inject_lines(const char *output)
{
    struct text t = { 0 };
    const char *p = output;

    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t n = e ? (size_t)(e - p) : strlen(p);

        if (n >= 7 && !memcmp(p, "inject:", 7))
            text_add(&t, "%.*s\n", (int)n, p);
        p = e ? e + 1 : p + n;
    }
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p ? t.p : strdup("");
}

/* whether the path field of a runwrap value (before its "|") is path */
static int runwrap_is(const char *runwrap, const char *path)
{
    size_t n = path ? strlen(path) : 0;

    return n && !strncmp(runwrap, path, n) && (runwrap[n] == '|' || !runwrap[n]);
}

/* whether a wrap or runwrap value starts its program through inject: @INJECT@
   of the configuration, or the runner's inject */
static int wrap_inject(const char *wrap, const struct tc_env *env)
{
    if (!*wrap)
        return 0;
    for (size_t i = 0; env && i < env->nph; i++)
        if (!strcmp(env->ph[i].name, "INJECT") && runwrap_is(wrap, env->ph[i].value))
            return 1;
    return runwrap_is(wrap, tc_env_runner(env, "inject"));
}

int cr_cell_inject(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env)
{
    char b[24];

    if (!strcmp(val(conf, m, cell, "pad", b), "on"))
        return 1;
    return wrap_inject(val(conf, m, cell, "runwrap", b), env);
}

int cr_cell_wrap_inject(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                        const struct tc_env *env)
{
    char b[24];

    return wrap_inject(val(conf, m, cell, "wrap", b), env);
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

/* ------------------------------------------------- the run of all cases */

/* the rank of a verdict: the higher, the worse */
static int rank(int v)
{
    switch (v) {
    case TJ_V_PASSED: return 0;
    case TJ_V_REPRODUCED: return 1;
    case CR_SKIPPED: return 2;
    case TJ_V_FAILED: return 3;
    case TJ_V_BROKEN: return 4;
    default: return 5;          /* error, and anything unknown */
    }
}

int cr_verdict_worse(int a, int b)
{
    return rank(a) > rank(b);
}

const char *cr_verdict_name(int v)
{
    static const char *const n[] = { "PASSED", "REPRODUCED", "ERROR", "BROKEN", "FAILED" };

    if (v == CR_SKIPPED)
        return "SKIPPED";
    return v >= 0 && v <= 4 ? n[v] : "?";
}

int cr_side_status(int v, int originals)
{
    return originals && v == TJ_V_REPRODUCED ? 0 : v;
}

const char *cr_side_verdict_name(int v, int originals)
{
    return originals && v == TJ_V_REPRODUCED ? "REPRODUCED (as declared, side originals)" : cr_verdict_name(v);
}

int cr_cell_clean(const struct tj_obs *o)
{
    const char *l, *end;
    int kinds[2] = { -1, -1 };  /* the last entry, the one before it */

    /* a run from the editor gives no exit code */
    if (!o->session || strcmp(o->session, "done") || !o->compile || strcmp(o->compile, "ok") || o->nreq
        || (!o->editor_run && (!o->has_rc || o->rc != 0)) || !o->has_result || !o->result || !o->result_len)
        return 0;
    end = o->result + o->result_len;
    if (end[-1] != '\n')
        return 0;
    for (int k = 0; k < 2 && end > o->result; k++) {
        uint8_t line[64];
        struct basictest_entry e;
        size_t n;

        for (l = end - 1; l > o->result && l[-1] != '\n'; l--)
            ;
        n = (size_t)(end - 1 - l);
        if (n < sizeof line) {
            memcpy(line, l, n);
            if (basictest_read(line, n, 0, &e, NULL) == BASICTEST_OK)
                kinds[k] = e.kind;
        }
        end = l;
    }
    /* the run's end entry, and the program did not end inside a block (the
       library's finit writes incomplete before the end entry then: End in a
       case, a crash it survived) */
    return kinds[0] == BASICTEST_END && kinds[1] != BASICTEST_INCOMPLETE;
}

int cr_open_case(const char *result, size_t len, char *name, size_t namelen)
{
    uint8_t *buf;
    int open = 0;
    size_t p = 0;

    if (namelen)
        name[0] = 0;
    if (!result || !len || !(buf = malloc(len)))
        return 0;
    /* the codec decodes in place */
    memcpy(buf, result, len);
    while (p < len) {
        uint8_t *nl = memchr(buf + p, '\n', len - p);
        struct basictest_entry e;
        struct basictest_error be;

        if (!nl)
            break;
        if (!basictest_read(buf + p, (size_t)(nl - (buf + p)), 0, &e, &be)) {
            if (e.kind == BASICTEST_BLOCK && e.block == BASICTEST_BLOCK_CASE) {
                size_t n = e.name.length < namelen ? e.name.length : namelen - 1;

                memcpy(name, e.name.data, n);
                name[n] = 0;
                open = 1;
            } else if (e.kind == BASICTEST_BLOCK || e.kind == BASICTEST_BLOCKEND)
                open = 0;
        }
        p = (size_t)(nl - buf) + 1;
    }
    free(buf);
    if (!open && namelen)
        name[0] = 0;
    return open;
}

void cr_plan_free(struct cr_plan *p)
{
    free(p->state);
    free(p->verdict);
    free(p->alone);
    free(p->shared);
    memset(p, 0, sizeof *p);
}

int cr_plan_init(struct cr_plan *p, size_t n, const int *alone, int isolated)
{
    memset(p, 0, sizeof *p);
    p->n = n;
    p->isolated = isolated;
    if (!(p->state = calloc(n + 1, sizeof *p->state)) || !(p->verdict = calloc(n + 1, sizeof *p->verdict))
        || !(p->alone = calloc(n + 1, sizeof *p->alone)) || !(p->shared = calloc(n + 1, sizeof *p->shared))) {
        cr_plan_free(p);
        return -1;
    }
    for (size_t i = 0; i < n; i++) {
        p->state[i] = alone && alone[i] ? CR_C_ALONE : CR_C_SHARED;
        p->verdict[i] = -1;
        p->shared[i] = -1;
    }
    return 0;
}

int cr_plan_next(struct cr_plan *p, size_t *set, size_t *nset, int *kind)
{
    size_t n = 0;

    for (size_t i = 0; i < p->n; i++)
        if (p->state[i] == CR_C_SHARED)
            set[n++] = i;
    if (n == 1)
        p->state[set[0]] = CR_C_ALONE;      /* a set of one: the case on its own */
    else if (n) {
        *nset = n;
        *kind = CR_RUN_SHARED;
        return 1;
    }
    for (size_t i = 0; i < p->n; i++)
        if (p->state[i] == CR_C_ISOLATED) {
            set[0] = i;
            *nset = 1;
            *kind = CR_RUN_ISOLATED;
            return 1;
        }
    for (size_t i = 0; i < p->n; i++)
        if (p->state[i] == CR_C_ALONE || p->state[i] == CR_C_CONFIRM) {
            set[0] = i;
            *nset = 1;
            *kind = CR_RUN_ALONE;
            return 1;
        }
    *nset = 0;
    return 0;
}

void cr_plan_unclean(struct cr_plan *p, const size_t *set, size_t nset, const long *open, size_t nopen)
{
    int any = 0, all = 0;

    for (size_t k = 0; k < nopen; k++) {
        size_t j;

        for (j = 0; j < nset && (open[k] < 0 || set[j] != (size_t)open[k]); j++)
            ;
        if (j == nset)
            all = 1;            /* no open case of the set: every case on its own */
        else {
            p->state[set[j]] = CR_C_ALONE;
            any = 1;
        }
    }
    if (all || !any)
        for (size_t j = 0; j < nset; j++)
            p->state[set[j]] = CR_C_ALONE;
}

int cr_plan_clean(struct cr_plan *p, const size_t *set, size_t nset)
{
    if (!p->isolated)
        return 1;
    for (size_t j = 0; j < nset; j++)
        p->state[set[j]] = CR_C_ISOLATED;
    return 0;
}

void cr_plan_judged(struct cr_plan *p, size_t c, int v, int alone)
{
    if (!alone && (v == TJ_V_FAILED || v == TJ_V_BROKEN || v == TJ_V_ERROR)) {
        p->shared[c] = v;
        p->state[c] = CR_C_CONFIRM;
        return;
    }
    p->verdict[c] = v;
    p->alone[c] = alone;
    p->state[c] = CR_C_DONE;
}

int cr_plan_worst(const struct cr_plan *p)
{
    int w = TJ_V_PASSED;

    for (size_t i = 0; i < p->n; i++)
        if (p->verdict[i] >= 0 && cr_verdict_worse(p->verdict[i], w))
            w = p->verdict[i];
    return w;
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
        /* the rows of docs/ab3/variant.md of AmiBlitz3's deviations */
        print_tab(out, "deviations", conf->variants, conf->nvariants);
    }
    if (conf->nends) {
        fprintf(out, "alternative ends:\n");
        print_tab(out, "ends", conf->ends, conf->nends);
    }
    if (conf->ndata) {
        fprintf(out, "data (%s.test/):\n", conf->id);
        for (size_t i = 0; i < conf->ndata; i++) {
            const struct tc_data *d = &conf->data[i];

            fprintf(out, "  \"%s\": action = \"%s\"    # %s:%u\n", d->file, tc_daction_name(d->action), d->at.file,
                    d->at.line);
            for (int k = 0; k < TC_DO_N && d->action == TC_DA_BASIC_COMPILE; k++) {
                if (d->optline[k])
                    fprintf(out, "    %s = \"%s\"    # %s:%u\n", tc_dopt_name(k), d->opt[k], d->at.file,
                            d->optline[k]);
                else
                    fprintf(out, "    %s = \"%s\"    # (default)\n", tc_dopt_name(k), d->opt[k]);
            }
        }
    }
    {
        size_t nd;
        const struct tc_default *d = tc_defaults(&nd);

        fprintf(out, "default assertions (where no file names the query):\n");
        for (size_t i = 0; i < nd; i++)
            fprintf(out, "  %s = \"%s\"%s\n", d[i].query, d[i].pred,
                    d[i].shell ? "    # not in a cell with editor-run = \"on\"" : "");
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
        fprintf(out, "  %s    # side %s\n", m->cells[i].name,
                m->cells[i].oracle ? "oracle" : m->cells[i].originals ? "originals" : "port");
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
    char tag[24];               /* c0, c1, ... */
    struct corpus_pass p;
    char *key;                  /* tc_session_key */
    int done;                   /* observed */
    enum amiga_emulation_status status;
    int inject;                 /* the program runs through inject (cr_cell_inject) */
    int wrap_inject;            /* the compiler runs through inject (cr_cell_wrap_inject) */
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
    /* a run from the editor ends inside the compile part */
    int r = b2 && exe && run && has_rc(b2) && (c->p.editrun || access(exe, F_OK) || has_rc(run));

    free(b2);
    free(exe);
    free(run);
    return r;
}

void cr_obs_free(struct cr_obs *ob)
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
    free(ob->inject);
    free(ob->con);
    free(ob->cinject);
    bugorig_free(&ob->bo);
    for (size_t i = 0; i < ob->ndata; i++)
        free(ob->dres[i]);
    free(ob->dres);
    free(ob->data);
    for (size_t i = 0; i < ob->nruns; i++)
        free(ob->rtext[i]);
    free(ob->rtext);
    free(ob->runs);
    for (size_t i = 0; i < ob->njtext; i++)
        free(ob->jtext[i]);
    free(ob->jtext);
    free(ob->jobs);
    memset(ob, 0, sizeof *ob);
}

static const char *status_name(enum amiga_emulation_status st)
{
    return st == AMIGA_EMULATION_DONE ? "done" : st == AMIGA_EMULATION_HALT ? "halt" : "timeout";
}

/* the compiles of the data sources of a cell (their jobs bNNN, in the order
   of the data sources: cr_data_pass) */
static int observe_data(const struct cr_cellrun *c, const struct tc_config *conf, struct cr_obs *ob)
{
    size_t n = 0;

    for (size_t i = 0; i < conf->ndata; i++)
        n += conf->data[i].action == TC_DA_BASIC_COMPILE;
    if (!n)
        return 0;
    if (!(ob->data = calloc(n, sizeof *ob->data)) || !(ob->dres = calloc(n, sizeof *ob->dres)))
        return -1;
    for (size_t i = 0, j = 0; i < conf->ndata && j < c->p->nbjobs; i++) {
        const struct corpus_build_job *b = &c->p->bjobs[j];
        char job[32], *path, *log;
        size_t len;

        if (conf->data[i].action != TC_DA_BASIC_COMPILE)
            continue;
        snprintf(job, sizeof job, "b%03zu", j);
        path = b->part ? str_printf("%s/%s/session.log", c->vol, b->part)
                       : str_printf("%s/out/session.log", c->vol);
        if (!path)
            return -1;
        log = (char *)read_file(path, &len);
        free(path);
        ob->dres[j] = bugorig_compile(log, job);
        free(log);
        if (!ob->dres[j])
            return -1;
        ob->data[j].file = conf->data[i].file;
        ob->data[j].result = ob->dres[j];
        ob->ndata = ++j;
    }
    ob->o.data = ob->data;
    ob->o.ndata = ob->ndata;
    return 0;
}

/* the output files of the started programs of a cell, CORPUS:out/run-K.out
   (also of one that did not return) */
static int observe_runs(const struct cr_cellrun *c, const struct tp_source *src, struct cr_obs *ob)
{
    size_t n = 0;

    for (size_t i = 0; i < src->nchecks; i++)
        n += src->checks[i].kind == TP_RUNPROGRAM;
    if (!n)
        return 0;
    if (!(ob->runs = calloc(n, sizeof *ob->runs)) || !(ob->rtext = calloc(n, sizeof *ob->rtext)))
        return -1;
    for (size_t i = 0; i < src->nchecks; i++) {
        const struct tp_check *k = &src->checks[i];
        char *path;
        size_t len;

        if (k->kind != TP_RUNPROGRAM)
            continue;
        if (!(path = str_printf("%s/out/run-%u.out", c->vol, k->k)))
            return -1;
        ob->rtext[ob->nruns] = (char *)read_file(path, &len);
        free(path);
        if (!ob->rtext[ob->nruns])
            continue;           /* not started */
        ob->runs[ob->nruns].k = k->k;
        ob->runs[ob->nruns].text = ob->rtext[ob->nruns];
        ob->nruns++;
    }
    ob->o.runs = ob->runs;
    ob->o.nruns = ob->nruns;
    return 0;
}

/* a text of the named jobs kept in ob (freed with it); s NULL: out of memory */
static char *keep_text(struct cr_obs *ob, char *s)
{
    char **v;

    if (!s)
        return NULL;
    if (!(v = realloc(ob->jtext, (ob->njtext + 1) * sizeof *v))) {
        free(s);
        return NULL;
    }
    ob->jtext = v;
    v[ob->njtext++] = s;
    return s;
}

/* a named job: its block of log (step s), with its compile result if
   result; 0, -1 = out of memory */
static int add_job(struct cr_obs *ob, const char *name, const char *log, const struct bugorig_step *s, int result)
{
    struct tj_job *j = &ob->jobs[ob->njobs];

    j->name = name;
    j->result = NULL;
    if (!(j->block = keep_text(ob, log ? bugorig_block(log, s) : strdup(""))))
        return -1;
    if (result && !(j->result = keep_text(ob, bugorig_compile(log, s->job))))
        return -1;
    ob->njobs++;
    return 0;
}

/* the named compile jobs of a cell (change named-compile-jobs): j000, the
   run from the editor r001, the jobline jobs (all from the cell's log) and
   the data sources (bNNN, from the log of the session that compiled them:
   the cell's or their part's), their ID written as their file, their
   staged source as @DATA:FILE@ */
static int observe_jobs(const struct cr_cellrun *c, const struct tc_config *conf, struct cr_obs *ob)
{
    const struct tt_value *jl = list(conf, "jobline");
    size_t n = 2 + (jl ? jl->nlist : 0) + c->p->nbjobs;
    struct bugorig_step s = { "j000", NULL, NULL, NULL };

    if (!(ob->jobs = calloc(n, sizeof *ob->jobs)))
        return -1;
    if (add_job(ob, "j000", ob->log, &s, 0))
        return -1;
    if (c->p->editrun) {
        s.job = "r001";
        if (add_job(ob, "r001", ob->log, &s, 0))
            return -1;
    }
    for (size_t i = 0; jl && i < jl->nlist; i++) {
        const char *id;
        size_t idlen;
        int kind = tc_jobline_job(jl->list[i], &id, &idlen);
        char *name;

        if (kind == TC_JOB_NONE)
            continue;
        if (!(name = keep_text(ob, strndup(id, idlen))))
            return -1;
        s.job = name;
        if (add_job(ob, name, ob->log, &s, kind == TC_JOB_JOB || kind == TC_JOB_RESIDENT))
            return -1;
    }
    for (size_t i = 0, j = 0; i < conf->ndata && j < c->p->nbjobs; i++) {
        const struct corpus_build_job *b = &c->p->bjobs[j];
        char job[32], *path, *log, *staged, *ph;
        size_t len;
        int r;

        if (conf->data[i].action != TC_DA_BASIC_COMPILE)
            continue;
        snprintf(job, sizeof job, "b%03zu", j);
        path = b->part ? str_printf("%s/%s/session.log", c->vol, b->part) : str_printf("%s/out/session.log", c->vol);
        staged = b->part ? str_printf("CORPUS:%s/src/%s.bb2", b->part, job) : str_printf("CORPUS:src/%s.bb2", job);
        ph = str_printf(TC_DATA_PH "%s@", conf->data[i].file);
        if (!path || !staged || !ph) {
            free(path);
            free(staged);
            free(ph);
            return -1;
        }
        log = (char *)read_file(path, &len);
        free(path);
        s.job = job;
        s.name = conf->data[i].file;
        s.path = staged;
        s.ph = ph;
        r = add_job(ob, conf->data[i].file, log, &s, 0);
        free(log);
        free(staged);
        free(ph);
        if (r)
            return -1;
        j++;
    }
    ob->o.jobs = ob->jobs;
    ob->o.njobs = ob->njobs;
    return 0;
}

int cr_observe(const struct cr_cellrun *c, const struct tc_config *conf, const struct tp_source *src,
               const struct tp_driver *d, const char *file, struct cr_obs *ob)
{
    char *dir = str_printf("%s/out", c->vol), *path;
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
    bout.session = c->session;
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
    ob->o.session = c->session;
    ob->o.compile = ob->compile;
    ob->o.compile_log = ob->compile_log;
    ob->o.output = run ? ob->run.output : NULL;
    ob->o.serial = ob->serial;
    /* inject's report: its lines of the output, which keeps them */
    if (c->inject && run && !(ob->o.inject = ob->inject = cr_inject_lines(ob->run.output)))
        goto out;
    /* inject's report of the compiler: its lines of the compiler's output */
    if (c->wrap_inject && ob->b2 && !(ob->o.compile_inject = ob->cinject = cr_inject_lines(ob->b2)))
        goto out;
    ob->o.has_rc = ob->run.has_rc;
    ob->o.rc = ob->run.rc;
    ob->o.req = ob->req;
    ob->o.nreq = ob->run.nreq;
    if (c->p->editrun) {
        /* the run from the editor: the output is its console; no exit code,
           no requesters */
        if ((path = str_printf("%s/" CR_PROGRAM ".con", dir)))
            ob->con = (char *)read_file(path, &len);
        free(path);
        ob->o.output = ob->con;
        ob->o.editor_run = 1;
        ob->o.has_rc = 0;
        ob->o.nreq = 0;
    }
    if (observe_data(c, conf, ob) || observe_runs(c, src, ob) || observe_jobs(c, conf, ob))
        goto out;
    r = 0;
out:
    free(run);
    free(dir);
    return r;
}

char *cr_session_script(const char *const *compile, const char *const *run, size_t n, int bundle)
{
    struct text t = { 0 };

    for (size_t i = 0; bundle && i < n; i++)
        text_add(&t, "Assign CELL%zu: " CR_BUNDLE_VOLUME ":c%zu\n", i, i);
    for (int part = 0; part < 2; part++)
        for (size_t i = 0; i < n; i++)
            text_add(&t, "Assign CORPUS: CELL%zu:\n%s", i, part ? run[i] : compile[i]);
    text_add(&t, "Assign CORPUS:\n");
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

size_t cr_session_volumes(const char *bundle, const char *const *dirs, size_t n, struct corpus_volume *vols,
                          char (*names)[32])
{
    if (bundle) {
        vols[0].volume = CR_BUNDLE_VOLUME;
        vols[0].host_dir = bundle;
        vols[0].cell = "c0";
        return 1;
    }
    for (size_t i = 0; i < n; i++) {
        snprintf(names[i], sizeof names[i], "CELL%zu", i);
        vols[i].volume = names[i];
        vols[i].host_dir = dirs[i];
        vols[i].cell = NULL;
    }
    return n;
}

/* the session of the cells cs[0..n): all compiles, then all runs (in a
   bundle after the assigns of the cells' directories) */
static char *session_script(struct cell **cs, size_t n, int bundle)
{
    char **comp = calloc(n + 1, sizeof *comp), **run = calloc(n + 1, sizeof *run), *s = NULL;
    size_t i = 0;

    if (comp && run)
        for (; i < n; i++)
            if (!(comp[i] = corpus_script_compile(&cs[i]->p)) || !(run[i] = corpus_script_run(&cs[i]->p)))
                break;
    if (comp && run && i == n)
        s = cr_session_script((const char *const *)comp, (const char *const *)run, n, bundle);
    for (size_t k = 0; comp && run && k < n; k++) {
        free(comp[k]);
        free(run[k]);
    }
    free(comp);
    free(run);
    return s;
}

/* runs the cells cs[0..n) in one session and marks those that ended; the
   others stay for another session.  bundle: the directory of the volume
   BUNDLE: (each cell's staged volume renamed to bundle/cK for the session
   and back after it), NULL: a volume CELLk: per cell.  0 = ok, 77 = no
   emulator, 2 = error */
static int run_session(struct cell **cs, size_t n, const char *base, FILE *log, size_t *nsessions,
                       const char *bundle)
{
    struct corpus_volume *vols = calloc(n, sizeof *vols);
    char (*names)[32] = calloc(n, sizeof *names), **dirs = calloc(n, sizeof *dirs), **bdirs = NULL;
    char *script = session_script(cs, n, bundle != NULL), err[400];
    struct amiga_emulation *e = NULL;
    struct responder *resp = NULL;
    unsigned port = 0;
    struct statline stl;
    enum amiga_emulation_status st;
    size_t nvols, moved = 0;
    int r = 2;

    if (!vols || !names || !dirs || !script)
        goto out;
    for (size_t i = 0; i < n; i++) {
        if (corpus_stage(&cs[i]->p, 0, log))
            goto out;
        if (!(dirs[i] = str_printf("%s/%s/vol", cs[i]->p.work, cs[i]->tag)))
            goto out;
    }
    nvols = cr_session_volumes(bundle, (const char *const *)dirs, n, vols, names);
    if (bundle) {
        if ((mkdir(bundle, 0700) && errno != EEXIST) || !(bdirs = calloc(n, sizeof *bdirs))) {
            fprintf(log, "corpusrun case: cannot make %s\n", bundle);
            goto out;
        }
        for (size_t i = 0; i < n; i++) {
            if (!(bdirs[i] = str_printf("%s/c%zu", bundle, i)))
                goto out;
            if (rename(dirs[i], bdirs[i])) {
                fprintf(log, "corpusrun case: cannot rename %s to %s: %s\n", dirs[i], bdirs[i], strerror(errno));
                goto out;
            }
            moved = i + 1;
        }
    }
    if (cs[0]->p.serial && (!(port = serial_free_port()) || !(resp = serial_start(port)))) {
        fprintf(log, "corpusrun case: no TCP port for the serial port\n");
        goto out;
    }
    fprintf(log, "corpusrun case: emulator session %zu:", *nsessions);
    for (size_t i = 0; i < n; i++)
        if (bundle)
            fprintf(log, " c%zu", i);
        else
            fprintf(log, " %s", cs[i]->tag);
    fprintf(log, "\n");
    fflush(log);
    while (!(e = corpus_session_start(&cs[0]->p, 0, script, port, vols, nvols, err, sizeof err))) {
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
    /* the cells' volumes back where their observations read them; one that
       cannot go back is an error (the run's files keep it) */
    {
        int back = 1;

        for (; moved > 0; moved--)
            if (rename(bdirs[moved - 1], dirs[moved - 1])) {
                fprintf(log, "corpusrun case: cannot rename %s back to %s: %s\n", bdirs[moved - 1],
                        dirs[moved - 1], strerror(errno));
                back = 0;
            }
        if (!back)
            goto out;
    }
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
        if (i + 1 < n && bundle)
            fprintf(log, "corpusrun case: session ended (%s) in c%zu; the cells behind it run again\n",
                    amiga_emulation_status_name(st), i);
        else if (i + 1 < n)
            fprintf(log, "corpusrun case: session ended (%s) in %s; the cells behind it run again\n",
                    amiga_emulation_status_name(st), cs[i]->tag);
        break;
    }
    statline_set(stl.verdict, sizeof stl.verdict, st == AMIGA_EMULATION_DONE ? "done" : "ended");
    statline_write(log, &stl);
    r = 0;
out:
    /* a session that did not start: the volumes go back */
    for (; moved > 0; moved--)
        rename(bdirs[moved - 1], dirs[moved - 1]);
    if (bundle)
        rmdir(bundle);
    if (resp)
        serial_stop(resp, ".");
    if (e)
        amiga_emulation_free(e);
    for (size_t i = 0; dirs && i < n; i++)
        free(dirs[i]);
    for (size_t i = 0; bdirs && i < n; i++)
        free(bdirs[i]);
    free(bdirs);
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
    for (size_t i = 0; i < p->nbuilds; i++) {
        const char *bf[] = { p->builds[i].blitz2, p->builds[i].deflibs, p->builds[i].acidlibs };

        for (size_t k = 0; k < 3; k++)
            if (access(bf[k], R_OK)) {
                fprintf(log, "corpusrun case: no %s - skipped\n", bf[k]);
                return 1;
            }
    }
    for (size_t i = 0; i < p->nmounts; i++)
        if (access(p->mount_dir[i], R_OK | X_OK)) {
            fprintf(log, "corpusrun case: no %s for %s: - skipped\n", p->mount_dir[i], p->mount_name[i]);
            return 1;
        }
    return 0;
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

/* the data of a test as an argument list of its own (prev_run has no ctx) */
struct dctx {
    const struct tc_config *conf;
    const struct tc_env *env;
    const char *testdir, *tokens;
    FILE *log;
    char **built;
};

/* the host files of the test-local data (built): a data source
   preprocessed (its placeholders, no check) and tokenised with the cells'
   token list, a .spec made by mktestdata, a copied file as it is; in
   dir/data.  0, -1 (reported) */
static int prepare_data(struct dctx *x, const char *dir)
{
    char err[800], *ddir = str_printf("%s/data", dir);
    int r = -1;

    if (!ddir || !(x->built = calloc(x->conf->ndata + 1, sizeof *x->built)) || mkdir_p(ddir)) {
        fprintf(x->log, "corpusrun case: cannot prepare the data in %s\n", ddir ? ddir : dir);
        goto out;
    }
    for (size_t i = 0; i < x->conf->ndata; i++) {
        const struct tc_data *d = &x->conf->data[i];
        char *src = str_printf("%s/%s.test/%s", x->conf->dir, x->conf->id, d->file);

        if (!src)
            goto oom;
        if (d->action == TC_DA_COPY) {
            x->built[i] = src;
            continue;
        }
        if (d->action == TC_DA_MKTESTDATA) {
            const char *mk = need(x->env, "mktestdata", err, sizeof err);
            char *argv[4] = { (char *)mk, src, NULL, NULL };

            if (!mk) {
                fprintf(x->log, "corpusrun case: %s\n", err);
                free(src);
                goto out;
            }
            if (!(x->built[i] = argv[2] = str_printf("%s/d%03zu", ddir, i))) {
                free(src);
                goto oom;
            }
            if (spawn(argv, x->log)) {
                fprintf(x->log, "corpusrun case: the data %s cannot be made\n", src);
                free(src);
                goto out;
            }
            free(src);
            continue;
        }
        {
            /* a data source */
            struct tp_params par = { 0 };
            struct tp_source ds = { 0 };
            size_t len;
            char *text = (char *)read_file(src, &len), *bas = str_printf("%s/b%03zu.bas", ddir, i);
            char *rel = str_printf("%s.test/%s", x->conf->id, d->file);
            const char *tedsrc = need(x->env, "tedsrc", err, sizeof err);
            int n;

            if (!text || !bas || !rel || !tedsrc) {
                fprintf(x->log, "corpusrun case: %s\n", !text ? "cannot read a data source" : !tedsrc ? err
                                                                                                       : "out of memory");
                if (!text)
                    fprintf(x->log, "corpusrun case: cannot read %s\n", src);
                free(text);
                free(bas);
                free(rel);
                free(src);
                goto out;
            }
            par.file = rel;
            par.test_id = x->conf->id;
            par.testdata = CR_TESTDATA;
            par.testdir = x->testdir;
            par.data = 1;
            n = tp_process(text, len, &par, &ds);
            free(text);
            for (size_t k = 0; n > 0 && k < ds.errors.n; k++)
                fprintf(x->log, "%s\n", ds.errors.v[k]);
            if (n == 0 && write_file(bas, ds.text, ds.len))
                n = -1;
            tp_free(&ds);
            free(rel);
            free(src);
            if (n) {
                fprintf(x->log, "corpusrun case: the data source %s.test/%s %s\n", x->conf->id, d->file,
                        n < 0 ? "cannot be written" : "is refused");
                free(bas);
                goto out;
            }
            if (!(x->built[i] = str_printf("%s/b%03zu.bb2", ddir, i))) {
                free(bas);
                goto oom;
            }
            {
                char *argv[] = { (char *)tedsrc, (char *)"tok", (char *)x->tokens, bas, x->built[i], NULL };

                n = spawn(argv, x->log);
            }
            free(bas);
            if (n) {
                fprintf(x->log, "corpusrun case: the data source %s.test/%s cannot be tokenised\n", x->conf->id,
                        d->file);
                goto out;
            }
        }
    }
    r = 0;
    goto out;
oom:
    fprintf(x->log, "corpusrun case: out of memory\n");
out:
    free(ddir);
    return r;
}
/* the comparison with the prev build (caserun.h): a pass per value of
   runtime-errors of the side port */
static int prev_run(const struct caserun_opts *o, const struct tc_env *env, const struct tc_config *conf,
                    const struct tc_matrix *m, FILE *log)
{
    static const char *const values[] = { "off", "on" };
    char err[800], *dir = NULL, *testdir = NULL;
    char **names = calloc(2 * conf->ndata + 3, sizeof *names);
    const char *work = need(env, "work", err, sizeof err);
    const struct tt_value *rwe = list(conf, "rwe");
    struct dctx dx = { conf, env, NULL, NULL, log, NULL };
    size_t nnames = 0, cur = 0;
    const char *stale;
    int rc = 0, any = 0, skipped = 0;

    if (!names) {
        fprintf(log, "corpusrun case: out of memory\n");
        return 2;
    }
    if (!o->driver || !o->test || !o->prev_blitz2 || !o->prev_libs) {
        fprintf(log, "corpusrun case: --prev needs --driver, --test, --prev-blitz2 and --prev-libs\n");
        free(names);
        return 2;
    }
    if (rwe && rwe->nlist) {
        fprintf(log, "corpusrun case: %s: a test with RWE user libraries is not compared with the prev build\n",
                conf->id);
        free(names);
        return 2;
    }
    if (!work || !(dir = str_printf("%s/%s", work, o->test)) || mkdir_p(dir)) {
        fprintf(log, "corpusrun case: %s\n", work ? "cannot create the work directory" : err);
        free(dir);
        free(names);
        return 2;
    }
    /* the data sources, tokenised as for a run */
    if (conf->ndata) {
        if (!(testdir = str_printf(CR_TESTDIR, conf->id)) || !(dx.tokens = need(env, "tokens", err, sizeof err))) {
            fprintf(log, "corpusrun case: %s\n", testdir ? err : "out of memory");
            rc = 2;
            goto done;
        }
        dx.testdir = testdir;
        if (prepare_data(&dx, dir)) {
            rc = 2;
            goto done;
        }
    }
    for (size_t v = 0; v < 2; v++) {
        char b[24], *src = NULL, *wdir = NULL, *cp = NULL, *pt = NULL, *dt = NULL;
        const char **dsrc = calloc(conf->ndata + 1, sizeof *dsrc), **dname = calloc(conf->ndata + 1, sizeof *dname);
        size_t cell, nd = 0;
        const char *drvname = NULL;
        struct corpus_pass p = { 0 };
        int r = 0;

        if (!dsrc || !dname) {
            free(dsrc);
            free(dname);
            fprintf(log, "corpusrun case: out of memory\n");
            rc = 2;
            break;
        }
        for (cell = 0; cell < m->ncells; cell++)
            if (!m->cells[cell].originals && !m->cells[cell].oracle
                && !strcmp(val(conf, m, cell, "runtime-errors", b), values[v]))
                break;
        /* the data sources compiled with this setting: the cell's, or a fixed one */
        for (size_t i = 0; i < conf->ndata; i++) {
            const struct tc_data *d = &conf->data[i];
            const char *w = d->action != TC_DA_BASIC_COMPILE ? NULL
                            : strcmp(d->opt[TC_DO_RUNERR], "same") ? d->opt[TC_DO_RUNERR]
                            : cell < m->ncells ? values[v] : NULL;

            if (!w || strcmp(w, values[v]))
                continue;
            if (!(names[nnames] = str_printf("%s/%s%s", conf->id, d->file, v ? CR_PREV_RUNERRS : ""))) {
                r = -2;
                break;
            }
            dsrc[nd] = dx.built[i];
            dname[nd++] = names[nnames++];
        }
        if (r == 0 && cell == m->ncells) {
            /* no cell with this setting: a pass of the data alone, with the
               settings of the first cell of the side port */
            if (!nd) {
                free(dsrc);
                free(dname);
                continue;
            }
            for (cell = 0; cell < m->ncells && (m->cells[cell].originals || m->cells[cell].oracle); cell++)
                ;
        } else if (r == 0) {
            if (!(names[nnames] = str_printf("%s%s", conf->id, v ? CR_PREV_RUNERRS : "")))
                r = -2;
            else
                drvname = names[nnames++];
        }
        if (r == 0 && cell < m->ncells
            && (!(src = str_printf("%s/%s.bb2", dir, drvname ? drvname : conf->id))
                || !(wdir = str_printf("%s/%s%s", dir, drvname ? drvname : conf->id, drvname ? "" : "-data"))))
            r = -2;
        if (r == -2) {
            fprintf(log, "corpusrun case: out of memory\n");
            r = 2;
        } else if (cell == m->ncells) {
            r = 0;
        } else if (drvname && copy_file(o->driver, src)) {
            fprintf(log, "corpusrun case: cannot copy the driver %s\n", o->driver);
            r = 2;
        } else if ((r = cr_cell_pass(conf, m, cell, env, "clib", wdir, src, &cp, err, sizeof err))
                   || (r = cr_prev_pass(cp, o->test, drvname ? drvname : "", wdir, drvname ? src : NULL,
                                        o->prev_blitz2, o->prev_libs, o->intended, o->nintended, &pt, err,
                                        sizeof err))
                   || (r = cr_prev_data(o->test, dsrc, dname, nd, o->intended, o->nintended, &dt))) {
            fprintf(log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
            r = 2;
        } else {
            char *all = str_printf("%s%s", pt, dt), *re;

            /* a pass of the data alone takes this setting of runtime error checking */
            if (all && !drvname && (re = strstr(all, "\nruntime_errors=")) && !strncmp(re + 16, values[!v],
                                                                                      strlen(values[!v]))) {
                char *fixed = str_printf("%.*s\nruntime_errors=%s%s", (int)(re - all), all, values[v],
                                         re + 16 + strlen(values[!v]));

                free(all);
                all = fixed;
            }
            if (!all) {
                fprintf(log, "corpusrun case: out of memory\n");
                r = 2;
            } else if (corpus_parse(all, &p, err, sizeof err)) {
                fprintf(log, "corpusrun case: %s: %s\n", drvname ? drvname : conf->id, err);
                r = 2;
            } else {
                corpus_select(&p);
                corpus_skip_missing(&p, log);
                r = corpus_run(&p, log);
                any = 1;
            }
            free(all);
        }
        corpus_free(&p);
        free(src);
        free(wdir);
        free(cp);
        free(pt);
        free(dt);
        free(dsrc);
        free(dname);
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
    while ((stale = cr_prev_unknown(o->test, (const char *const *)names, nnames, o->intended, o->nintended, &cur))
           != NULL) {
        fprintf(log, "STALE %s: intended, but the test has no such compile (its compiles:", stale);
        for (size_t i = 0; i < nnames; i++)
            fprintf(log, " %s", names[i]);
        fprintf(log, ")\n");
        if (rc < 1)
            rc = 1;
    }
    if (rc == 0 && skipped)
        rc = 77;
    if (rc == 0 && !any)
        rc = 2;
done:
    fprintf(log, "%s: the comparison with the prev build: %s\n", conf->id,
            rc == 0 ? "the same" : rc == 1 ? "DIFFERS" : rc == 77 ? "skipped" : "ERROR");
    for (size_t i = 0; i < nnames; i++)
        free(names[i]);
    free(names);
    for (size_t i = 0; dx.built && i < conf->ndata; i++)
        free(dx.built[i]);
    free(dx.built);
    free(testdir);
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

/* ------------------------------------------------------- the runs of a set */

/* what every run of a set of cases needs */
struct ctx {
    const struct caserun_opts *o;
    FILE *log;
    struct tc_env env;
    struct tc_config conf;
    struct tc_matrix m;
    struct tp_source src;
    const char *file;           /* the source's name in the report */
    char *testdir;              /* @TESTDIR@: CORPUS:ID.test */
    char **built;               /* per entry of conf.data its host file (cr_data_pass) */
    char *mac;                  /* the macro text of the driver */
    size_t mlen;
    char *tokens;               /* the token list */
    size_t *side;               /* the matrix cells of the side */
    size_t nside;
    const char *base;           /* the work base (the @stat lines) */
    size_t nsessions;
    int ran;                    /* a session has started */
};

/* a run of the driver of a set of cases in cells of the side */
struct set {
    const char **cases;         /* the cases the driver calls */
    size_t ncases;
    size_t *idx;                /* the matrix cells */
    size_t ncells;
    char *dir;                  /* its run directory */
    const char *label;          /* its name in the report (the base name of
                                   dir), NULL: the run of one case */
    struct tp_driver drv;
    struct cell *c;
    struct cr_obs *ob;
};

static void set_free(struct set *s)
{
    for (size_t i = 0; s->c && i < s->ncells; i++) {
        corpus_free(&s->c[i].p);
        free(s->c[i].key);
    }
    for (size_t i = 0; s->ob && i < s->ncells; i++)
        cr_obs_free(&s->ob[i]);
    free(s->c);
    free(s->ob);
    free(s->cases);
    free(s->idx);
    free(s->dir);
    tp_driver_free(&s->drv);
    memset(s, 0, sizeof *s);
}

/* the driver of the cases cases[0..n); 0, -1 (reported) */
static int make_driver(struct ctx *x, const char *const *cases, size_t n, struct tp_driver *drv)
{
    struct tp_driver_params dp = { 0 };
    char err[800];
    int r;

    dp.macros = x->mac;
    dp.macros_len = x->mlen;
    dp.result = CR_RESULT;
    dp.cases = cases;
    dp.ncases = n;
    if ((r = tp_driver_write(&x->src, &dp, drv, err, sizeof err))) {
        fprintf(x->log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
        return -1;
    }
    return 0;
}

/* the phases of a run of s (run_set, the bundle): set_prepare its driver
   tokenised in s->dir, a pass and a session key per cell; set_sessions the
   sessions (the cells with the same key, in order; again for the cells a
   session did not reach); set_observe the observations.  Each: 0 = done, 2
   = error, 77 = skipped (a file of a cell is missing, no emulator) */
static int set_prepare(struct ctx *x, struct set *s)
{
    char err[800], *drvpath = NULL, *bb2 = NULL;
    const char *tedsrc;
    int rc = 2;

    if (make_driver(x, s->cases, s->ncases, &s->drv))
        return 2;
    if (mkdir_p(s->dir) || !(drvpath = str_printf("%s/driver.bas", s->dir))
        || !(bb2 = str_printf("%s/" CR_PROGRAM ".bb2", s->dir)) || write_file(drvpath, s->drv.text, s->drv.len)) {
        fprintf(x->log, "corpusrun case: cannot write the driver\n");
        goto out;
    }
    if (!(tedsrc = need(&x->env, "tedsrc", err, sizeof err))) {
        fprintf(x->log, "corpusrun case: %s\n", err);
        goto out;
    }
    {
        char *argv[] = { (char *)tedsrc, (char *)"tok", x->tokens, drvpath, bb2, NULL };

        if (spawn(argv, x->log)) {
            fprintf(x->log, "corpusrun case: the driver %s cannot be tokenised\n", drvpath);
            goto out;
        }
    }
    if (!(s->c = calloc(s->ncells + 1, sizeof *s->c)) || !(s->ob = calloc(s->ncells + 1, sizeof *s->ob))) {
        fprintf(x->log, "corpusrun case: out of memory\n");
        goto out;
    }
    /* a pass per cell */
    for (size_t k = 0; k < s->ncells; k++) {
        struct cell *c = &s->c[k];
        char *pt;
        int r;

        c->idx = s->idx[k];
        c->inject = cr_cell_inject(&x->conf, &x->m, c->idx, &x->env);
        c->wrap_inject = cr_cell_wrap_inject(&x->conf, &x->m, c->idx, &x->env);
        snprintf(c->tag, sizeof c->tag, "c%zu", k);
        if ((r = cr_cell_pass(&x->conf, &x->m, c->idx, &x->env, c->tag, s->dir, bb2, &pt, err, sizeof err))) {
            fprintf(x->log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
            goto out;
        }
        if (x->conf.ndata) {
            char *dt, *all;

            if ((r = cr_data_pass(&x->conf, &x->m, c->idx, &x->env, (const char *const *)x->built, &dt, err,
                                  sizeof err))) {
                fprintf(x->log, "corpusrun case: %s\n", r == -2 ? "out of memory" : err);
                free(pt);
                goto out;
            }
            all = str_printf("%s%s", pt, dt);
            free(pt);
            free(dt);
            if (!(pt = all)) {
                fprintf(x->log, "corpusrun case: out of memory\n");
                goto out;
            }
        }
        r = corpus_parse(pt, &c->p, err, sizeof err);
        free(pt);
        if (r) {
            fprintf(x->log, "corpusrun case: cell %s: %s\n", x->m.cells[c->idx].name, err);
            goto out;
        }
        if (missing(&c->p, x->log)) {
            rc = 77;
            goto out;
        }
        if (!(c->key = tc_session_key(&x->conf, &x->m, c->idx, &x->env))) {
            fprintf(x->log, "corpusrun case: out of memory\n");
            goto out;
        }
    }
    rc = 0;
out:
    free(drvpath);
    free(bb2);
    return rc;
}

static int set_sessions(struct ctx *x, struct set *s)
{
    /* the sessions: the cells with the same key, in order; again for the cells
       a session did not reach */
    for (;;) {
        struct cell **group = calloc(s->ncells + 1, sizeof *group);
        size_t n = 0, first;
        int r;

        if (!group) {
            fprintf(x->log, "corpusrun case: out of memory\n");
            return 2;
        }
        for (first = 0; first < s->ncells && s->c[first].done; first++)
            ;
        if (first == s->ncells) {
            free(group);
            break;
        }
        for (size_t i = first; i < s->ncells; i++)
            if (!s->c[i].done && !strcmp(s->c[i].key, s->c[first].key))
                group[n++] = &s->c[i];
        x->ran = 1;
        r = run_session(group, n, x->base, x->log, &x->nsessions, NULL);
        free(group);
        if (r)
            return r;
    }
    return 0;
}

static int set_observe(struct ctx *x, struct set *s)
{
    for (size_t k = 0; k < s->ncells; k++) {
        const struct cell *c = &s->c[k];
        char *vol = str_printf("%s/%s/vol", c->p.work, c->tag);
        struct cr_cellrun cr = { vol, status_name(c->status), &c->p, c->inject, c->wrap_inject };
        int r = vol ? cr_observe(&cr, &x->conf, &x->src, &s->drv, x->file, &s->ob[k]) : -1;

        free(vol);
        if (r) {
            fprintf(x->log, "corpusrun case: out of memory\n");
            return 2;
        }
    }
    return 0;
}

/* runs s: set_prepare, set_sessions, set_observe.  0 = observed, 2 = error,
   77 = skipped (a file of a cell is missing, no emulator) */
static int run_set(struct ctx *x, struct set *s)
{
    int rc;

    if ((rc = set_prepare(x, s)) || (rc = set_sessions(x, s)))
        return rc;
    return set_observe(x, s);
}

/* judges the case name over n cells of the side, the i-th observed in from[i]
   (its cell at[i]): the block of the report to out, its verdict line marked
   with mark.  The verdict, -1 = out of memory */
static int judge_case(struct ctx *x, const char *name, const struct set *const *from, const size_t *at, size_t n,
                      const char *mark, FILE *out)
{
    size_t *idx = calloc(n + 1, sizeof *idx);
    struct tj_obs *to = calloc(n + 1, sizeof *to);
    struct tj_msgs msgs = { 0 };
    struct tj_test t = { x->file, &x->src, &x->conf, &x->m, name, x->o->require_deviation, NULL, 0 };
    int v = -1;

    if (!idx || !to)
        goto out;
    for (size_t i = 0; i < n; i++) {
        idx[i] = from[i]->c[at[i]].idx;
        to[i] = from[i]->ob[at[i]].o;
        /* the blocks of the other cases the driver called are theirs */
        if (from[i]->ncases > 1) {
            t.cases = from[i]->cases;
            t.ncases = from[i]->ncases;
        }
    }
    fprintf(out, "== %s, case %s, side %s:", x->file, name, side_name(x->o));
    for (size_t i = 0; i < n; i++)
        if (from[i]->label)
            fprintf(out, " %s (%s/%s)", x->m.cells[idx[i]].name, from[i]->label, from[i]->c[at[i]].tag);
        else
            fprintf(out, " %s (%s)", x->m.cells[idx[i]].name, from[i]->c[at[i]].tag);
    fprintf(out, "\n");
    for (size_t i = 0; i < n; i++)
        if (from[i]->ob[at[i]].where)
            fprintf(out, "cell %s: compile error: %s\n", x->m.cells[idx[i]].name, from[i]->ob[at[i]].where);
    v = tj_verdict(&t, idx, to, n, &msgs);
    for (size_t i = 0; i < msgs.n; i++)
        fprintf(out, "%s\n", msgs.v[i]);
    tj_msgs_free(&msgs);
    if (v >= 0)
        fprintf(out, "%s: case %s: %s%s\n", x->file, name, cr_side_verdict_name(v, x->o->originals), mark);
out:
    free(idx);
    free(to);
    return v;
}

/* ------------------------------------------------------- the run of all cases */

/* the position of matrix cell idx in s, or s->ncells */
static size_t cell_in(const struct set *s, size_t idx)
{
    size_t k;

    for (k = 0; k < s->ncells && s->idx[k] != idx; k++)
        ;
    return k;
}

/* the run of all cases of the source in rundir (caserun.h); the exit status */
static int run_all(struct ctx *x, const char *rundir)
{
    size_t n = x->src.ncases, nsets = 0, nshared = 0, nset, niso = 0, nsh = 0;
    int *alone = calloc(n + 1, sizeof *alone);
    size_t *set = calloc(n + 1, sizeof *set);
    long *shared_in = calloc(n + 1, sizeof *shared_in);
    char **block = calloc(n + 1, sizeof *block);
    const struct set **from = calloc(x->nside + 1, sizeof *from);
    size_t *at = calloc(x->nside + 1, sizeof *at);
    long *open = calloc(x->nside + 1, sizeof *open);
    size_t *iso = calloc(x->nside + 1, sizeof *iso), *sh = calloc(x->nside + 1, sizeof *sh);
    struct set **sets = NULL;
    struct cr_plan p = { 0 };
    int kind, rc = 2;

    if (!alone || !set || !shared_in || !block || !from || !at || !open || !iso || !sh)
        goto oom;
    /* the cells with a session of their own, and the others */
    for (size_t i = 0; i < x->nside; i++)
        if (tc_cell_isolated(&x->conf, &x->m, x->side[i]))
            iso[niso++] = x->side[i];
        else
            sh[nsh++] = x->side[i];
    for (size_t i = 0; i < n; i++) {
        int a = nsh ? tj_case_alone(&x->conf, &x->m, x->side, x->nside, &x->src, x->src.cases[i].name) : 1;

        if (a < 0)
            goto oom;
        alone[i] = a;
        shared_in[i] = -1;
    }
    if (cr_plan_init(&p, n, alone, niso && nsh))
        goto oom;
    while (cr_plan_next(&p, set, &nset, &kind)) {
        struct set *s, **v = realloc(sets, (nsets + 1) * sizeof *v);
        const size_t *cells = kind == CR_RUN_SHARED ? sh : kind == CR_RUN_ISOLATED ? iso : x->side;
        size_t ncells = kind == CR_RUN_SHARED ? nsh : kind == CR_RUN_ISOLATED ? niso : x->nside;
        int r;

        if (!v)
            goto oom;
        sets = v;
        if (!(s = calloc(1, sizeof *s)))
            goto oom;
        sets[nsets++] = s;
        if (!(s->cases = calloc(nset + 1, sizeof *s->cases)) || !(s->idx = calloc(ncells + 1, sizeof *s->idx)))
            goto oom;
        for (size_t k = 0; k < nset; k++)
            s->cases[k] = x->src.cases[set[k]].name;
        s->ncases = nset;
        memcpy(s->idx, cells, ncells * sizeof *s->idx);
        s->ncells = ncells;
        if (kind == CR_RUN_SHARED)
            s->dir = str_printf("%s/shared-%zu", rundir, nshared++);
        else
            s->dir = str_printf("%s/%s-%s", rundir, kind == CR_RUN_ISOLATED ? "isolated" : "alone", s->cases[0]);
        if (!s->dir)
            goto oom;
        s->label = base_name(s->dir);
        if (kind == CR_RUN_SHARED) {
            fprintf(x->log, "corpusrun case: %s: the cases", s->label);
            for (size_t k = 0; k < nset; k++)
                fprintf(x->log, " %s", s->cases[k]);
            fprintf(x->log, "\n");
        } else if (kind == CR_RUN_ISOLATED)
            fprintf(x->log, "corpusrun case: %s: the case %s in the cells of a session of their own\n", s->label,
                    s->cases[0]);
        else
            fprintf(x->log, "corpusrun case: %s: the case %s on its own\n", s->label, s->cases[0]);
        r = run_set(x, s);
        if (r == 77 && !x->ran) {
            rc = 77;            /* the side is skipped before any session */
            goto out;
        }
        if (r) {
            for (size_t k = 0; k < nset; k++) {
                int vd = r == 77 ? CR_SKIPPED : TJ_V_ERROR;

                cr_plan_judged(&p, set[k], vd, 1);
                free(block[set[k]]);
                if (!(block[set[k]] = str_printf("%s: case %s: %s\n", x->file, s->cases[k], cr_verdict_name(vd))))
                    goto oom;
            }
            continue;
        }
        if (kind == CR_RUN_SHARED) {
            size_t nopen = 0;

            for (size_t i = 0; i < s->ncells; i++) {
                char name[200];
                long c = -1;

                if (cr_cell_clean(&s->ob[i].o))
                    continue;
                if (cr_open_case(s->ob[i].o.result, s->ob[i].o.result_len, name, sizeof name))
                    for (size_t j = 0; j < n; j++)
                        if (!strcmp(x->src.cases[j].name, name))
                            c = (long)j;
                open[nopen++] = c;
                if (c >= 0)
                    fprintf(x->log, "corpusrun case: %s: cell %s did not end cleanly, the case %s open\n", s->label,
                            x->m.cells[s->c[i].idx].name, name);
                else
                    fprintf(x->log, "corpusrun case: %s: cell %s did not end cleanly, no case open\n", s->label,
                            x->m.cells[s->c[i].idx].name);
            }
            if (nopen) {
                cr_plan_unclean(&p, set, nset, open, nopen);
                continue;
            }
            for (size_t k = 0; k < nset; k++)
                shared_in[set[k]] = (long)(nsets - 1);
            if (!cr_plan_clean(&p, set, nset))
                continue;       /* the isolated cells come first */
        }
        for (size_t k = 0; k < nset; k++) {
            size_t c = set[k], blen = 0, nj = s->ncells;
            char *buf = NULL;
            FILE *mf = open_memstream(&buf, &blen);
            int vd;

            if (!mf)
                goto oom;
            if (kind == CR_RUN_ISOLATED) {
                /* every cell of the side: the isolated ones from this run, the
                   others from the shared run that judged the case clean */
                const struct set *shs = sets[shared_in[c]];

                for (size_t i = 0; i < x->nside; i++) {
                    size_t k2 = cell_in(s, x->side[i]);

                    from[i] = k2 < s->ncells ? s : shs;
                    at[i] = k2 < s->ncells ? k2 : cell_in(shs, x->side[i]);
                }
                nj = x->nside;
            } else
                for (size_t i = 0; i < s->ncells; i++) {
                    from[i] = s;
                    at[i] = i;
                }
            vd = judge_case(x, s->cases[k], from, at, nj, kind == CR_RUN_ALONE ? " (alone)" : "", mf);
            if (vd >= 0 && kind == CR_RUN_ALONE && p.shared[c] >= 0 && p.shared[c] != vd && shared_in[c] >= 0) {
                const struct set *shs = sets[shared_in[c]];
                int first = 1;

                fprintf(mf, "case %s: %s alone (%s in the run shared with", s->cases[k], cr_verdict_name(vd),
                        cr_verdict_name(p.shared[c]));
                for (size_t j = 0; j < shs->ncases; j++)
                    if (strcmp(shs->cases[j], s->cases[k])) {
                        fprintf(mf, "%s%s", first ? " " : ", ", shs->cases[j]);
                        first = 0;
                    }
                fprintf(mf, ")\n");
            }
            if (fclose(mf) || vd < 0) {
                free(buf);
                goto oom;
            }
            cr_plan_judged(&p, c, vd, kind == CR_RUN_ALONE);
            if (p.state[c] == CR_C_CONFIRM) {
                free(buf);      /* its verdict comes from its run on its own */
                continue;
            }
            free(block[c]);
            block[c] = buf;
        }
    }
    for (size_t i = 0; i < n; i++)
        if (block[i])
            fputs(block[i], x->log);
    rc = cr_plan_worst(&p);
    fprintf(x->log, "%s: %zu case%s, side %s: %s\n", x->file, n, n == 1 ? "" : "s",
            x->o->originals ? "originals" : "port", cr_side_verdict_name(rc, x->o->originals));
    rc = cr_side_status(rc, x->o->originals);
    goto out;
oom:
    fprintf(x->log, "corpusrun case: out of memory\n");
    rc = 2;
out:
    for (size_t i = 0; i < nsets; i++) {
        set_free(sets[i]);
        free(sets[i]);
    }
    for (size_t i = 0; block && i < n; i++)
        free(block[i]);
    free(sets);
    free(block);
    free(alone);
    free(set);
    free(shared_in);
    free(from);
    free(at);
    free(open);
    free(iso);
    free(sh);
    cr_plan_free(&p);
    return rc;
}

/* ------------------------------------------------- the key of a stored verdict */

/* the key of the inputs of the case of x in its cell with the pass p and
   the session key skey (verdicts.h vstore_key): the files the pass stages
   or runs, the emulator's inputs.  0, -1 = out of memory */
static int case_key(const struct ctx *x, const struct corpus_pass *p, const char *skey, const char *test,
                    struct vstore_cache *cache, char key[65])
{
    size_t max = 16 + p->nfiles + 3 * p->nbuilds + p->nmounts + CORPUS_FLOPPIES, n = 0;
    const char **f = calloc(max, sizeof *f);
    char *emu = str_printf("%016llx|%s", corpus_emulator_key(p), skey);
    struct vstore_keyin in = { 0 };
    int r = -1;

    if (f && emu) {
        const char *one[] = { p->configs ? p->configs[0].blitz2 : NULL, p->configs ? p->configs[0].deflibs : NULL,
                              p->configs ? p->configs[0].acidlibs : NULL, p->ted, p->launcher, p->gate,
                              p->defaultdbug, p->wrap, p->runwrap, p->rcput };

        for (size_t i = 0; i < sizeof one / sizeof one[0]; i++)
            f[n++] = one[i];
        for (size_t i = 0; i < p->nfiles; i++)
            f[n++] = p->file_src[i];
        for (size_t i = 0; i < p->nbuilds; i++) {
            f[n++] = p->builds[i].blitz2;
            f[n++] = p->builds[i].deflibs;
            f[n++] = p->builds[i].acidlibs;
        }
        for (size_t i = 0; i < p->nmounts; i++)
            f[n++] = p->mount_dir[i];
        for (size_t i = 0; i < CORPUS_FLOPPIES; i++)
            f[n++] = p->floppy[i];
        in.test = test;
        in.side = side_name(x->o);
        in.case_name = x->o->case_name;
        in.conf = &x->conf;
        in.env = &x->env;
        in.staged = f;
        in.nstaged = n;
        in.emulator = emu;
        r = vstore_key(cache, &in, key);
    }
    free(f);
    free(emu);
    return r;
}

/* the served case (caserun.h): the exit status of a fresh entry of the
   bundle, -1 = none (the case runs alone) */
static int served(struct ctx *x, FILE *log)
{
    const struct caserun_opts *o = x->o;
    char *dir = vstore_dir(&x->env), err[800], key[65], *pt = NULL, *skey = NULL;
    struct corpus_pass p;
    struct vstore_verdict v;
    size_t cell = 0, nside = 0;
    int have = 0, r, rc = -1;

    memset(&p, 0, sizeof p);
    if (!dir)
        return -1;
    for (size_t i = 0; i < x->m.ncells; i++)
        if (on_side(&x->m.cells[i], o)) {
            cell = i;
            nside++;
        }
    /* the key as the bundle makes it: the cell's pass (its paths below a
       work directory that is not made), the session key */
    if (nside == 1
        && !cr_cell_pass(&x->conf, &x->m, cell, &x->env, "c0", "/-", "/-/" CR_PROGRAM ".bb2", &pt, err, sizeof err)
        && !corpus_parse(pt, &p, err, sizeof err) && (skey = tc_session_key(&x->conf, &x->m, cell, &x->env))
        && !case_key(x, &p, skey, o->test, NULL, key))
        have = 1;
    r = vstore_claim(dir, o->test, have ? key : NULL, o->from_bundle, &v, err, sizeof err);
    if (r == VSTORE_SERVED) {
        fputs(v.block, log);
        fprintf(log, "corpusrun case: verdict from the bundle %s\n", o->from_bundle);
        fprintf(log, "@served v=1 test=%s bundle=%s\n", o->test, o->from_bundle);
        rc = v.status;
        vstore_verdict_free(&v);
    } else if (r == VSTORE_MALFORMED)
        fprintf(log, "corpusrun case: the verdict stored for %s is malformed (%s); the case runs alone\n", o->test,
                err);
    else if (r == VSTORE_STALE)
        fprintf(log, "corpusrun case: the verdict stored for %s is not of these inputs; the case runs alone\n",
                o->test);
    else if (r < 0)
        fprintf(log, "corpusrun case: %s; the case runs alone\n", err);
    corpus_free(&p);
    free(pt);
    free(skey);
    free(dir);
    return rc;
}

int caserun(const struct caserun_opts *o, FILE *log)
{
    struct ctx x;
    struct runwork rw = { NULL, NULL, -1 };
    struct set one = { 0 };
    char err[800], *text = NULL, *workbase = NULL;
    const char *macros_path;
    size_t len;
    int rc = 2;

    memset(&x, 0, sizeof x);
    x.o = o;
    x.log = log;
    x.file = base_name(o->source);
    if (tc_env_load(o->env, &x.env, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (tc_resolve(o->source, &x.env, &x.conf, err, sizeof err) || tc_matrix_build(&x.conf, &x.m, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (!(x.testdir = str_printf(CR_TESTDIR, x.conf.id))) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    if (o->print_config) {
        rc = cr_print_config(log, &x.conf, &x.m) ? 2 : 0;
        goto out;
    }
    if (o->prev) {
        rc = prev_run(o, &x.env, &x.conf, &x.m, log);
        goto out;
    }
    if (o->from_bundle && o->case_name && o->test) {
        int r = served(&x, log);

        if (r >= 0) {
            rc = r;
            goto out;
        }
    }
    if (!(text = (char *)read_file(o->source, &len))) {
        fprintf(log, "corpusrun case: cannot read %s\n", o->source);
        goto out;
    }
    {
        struct tp_params par = { 0 };
        int n;

        par.file = x.file;
        par.test_id = x.conf.id;
        par.testdata = CR_TESTDATA;
        par.testdir = x.testdir;
        if ((n = tp_process(text, len, &par, &x.src)) != 0) {
            for (size_t i = 0; i < x.src.errors.n; i++)
                fprintf(log, "%s\n", x.src.errors.v[i]);
            if (n < 0)
                fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        if (tj_editor_run_refused(x.file, &x.src, &x.conf, &x.m, err, sizeof err)) {
            fprintf(log, "%s\n", err);
            goto out;
        }
    }
    if (!(macros_path = need(&x.env, "macros", err, sizeof err))) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (!(x.mac = (char *)read_file(macros_path, &x.mlen))) {
        fprintf(log, "corpusrun case: cannot read %s\n", macros_path);
        goto out;
    }
    {
        /* the driver of the selection: the case, or every case (the build
           step, the run of all cases) */
        const char **cases = calloc(x.src.ncases + 1, sizeof *cases);
        struct tp_driver drv = { 0 };
        size_t ncases = 0;
        int r;

        if (!cases) {
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        if (o->case_name)
            cases[ncases++] = o->case_name;
        else
            for (; ncases < x.src.ncases; ncases++)
                cases[ncases] = x.src.cases[ncases].name;
        r = make_driver(&x, cases, ncases, &drv);
        free(cases);
        if (r)
            goto out;
        if (o->print_driver) {
            fwrite(drv.text, 1, drv.len, log);
            rc = ferror(log) ? 2 : 0;
        } else if (o->write_driver)
            rc = write_driver(o, &x.env, &x.conf, &drv, log);
        tp_driver_free(&drv);
        if (o->print_driver || o->write_driver)
            goto out;
    }

    /* the cells of the side */
    if (!(x.side = calloc(x.m.ncells + 1, sizeof *x.side))) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    for (size_t i = 0; i < x.m.ncells; i++)
        if (on_side(&x.m.cells[i], o))
            x.side[x.nside++] = i;
    if (!x.nside) {
        fprintf(log, "corpusrun case: %s has no cell of the side %s\n", x.conf.id,
                side_name(o));
        goto out;
    }
    {
        const char *work = need(&x.env, "work", err, sizeof err);

        if (!work)
            workbase = NULL;
        else if (o->case_name)
            workbase = str_printf("%s/%s__%s%s", work, x.conf.id, o->case_name, side_suffix(o));
        else
            workbase = str_printf("%s/%s%s", work, x.conf.id, side_suffix(o));
        if (!workbase) {
            fprintf(log, "corpusrun case: %s\n", work ? "out of memory" : err);
            goto out;
        }
    }
    x.base = workbase;
    if (runwork_open(&rw, workbase, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (runwork_space(workbase, RUNWORK_MIN_FREE_MIB, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (!(x.tokens = token_list(&x.conf, &x.env, rw.dir, log, err, sizeof err))) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    if (x.conf.ndata) {
        struct dctx dx = { &x.conf, &x.env, x.testdir, x.tokens, log, NULL };
        int r = prepare_data(&dx, rw.dir);

        x.built = dx.built;
        if (r) {
            rc = 2;
            goto out;
        }
    }
    if (!o->case_name) {
        rc = run_all(&x, rw.dir);
        goto out;
    }
    /* the run of one case: a set of one, in the run directory itself */
    if (!(one.cases = calloc(2, sizeof *one.cases)) || !(one.idx = calloc(x.nside + 1, sizeof *one.idx))
        || !(one.dir = strdup(rw.dir))) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    one.cases[0] = o->case_name;
    one.ncases = 1;
    memcpy(one.idx, x.side, x.nside * sizeof *one.idx);
    one.ncells = x.nside;
    if ((rc = run_set(&x, &one)) != 0)
        goto out;
    {
        const struct set **from = calloc(x.nside + 1, sizeof *from);
        size_t *at = calloc(x.nside + 1, sizeof *at);
        int v = -1;

        if (from && at) {
            for (size_t i = 0; i < x.nside; i++) {
                from[i] = &one;
                at[i] = i;
            }
            v = judge_case(&x, o->case_name, from, at, x.nside, "", log);
        }
        free(from);
        free(at);
        if (v < 0) {
            fprintf(log, "corpusrun case: out of memory\n");
            rc = 2;
            goto out;
        }
        rc = cr_side_status(v, o->originals);
    }
out:
    if (rw.fd >= 0)
        rc = runwork_finish(&rw, x.ran, rc, NULL, NULL, log, "corpusrun case");
    set_free(&one);
    free(text);
    free(workbase);
    free(x.mac);
    free(x.testdir);
    for (size_t i = 0; x.built && i < x.conf.ndata; i++)
        free(x.built[i]);
    free(x.built);
    free(x.tokens);
    free(x.side);
    tp_free(&x.src);
    tc_matrix_free(&x.m);
    tc_free(&x.conf);
    tc_env_free(&x.env);
    return rc;
}

/* ------------------------------------------------------------ the bundles */

size_t cr_bundle_chunks(const char *const *keys, size_t n, size_t *chunk)
{
    size_t nchunks = 0;

    for (size_t i = 0; i < n; i++)
        chunk[i] = (size_t)-1;
    for (size_t i = 0; i < n; i++) {
        size_t in = 0;

        if (chunk[i] != (size_t)-1)
            continue;
        /* the group of the key of cell i, CR_BUNDLE_CELLS a chunk */
        for (size_t k = i; k < n; k++)
            if (chunk[k] == (size_t)-1 && !strcmp(keys[k], keys[i])) {
                if (in == CR_BUNDLE_CELLS) {
                    nchunks++;
                    in = 0;
                }
                chunk[k] = nchunks;
                in++;
            }
        nchunks++;
    }
    return nchunks;
}

size_t cr_bundle_pending(const int *done, size_t n, size_t nsessions, size_t *pending)
{
    size_t k = 0;

    if (nsessions >= CR_BUNDLE_SESSIONS)
        return 0;
    for (size_t i = 0; i < n; i++)
        if (!done[i])
            pending[k++] = i;
    return k;
}

int cr_bundle_status(int error, size_t nsessions)
{
    return error ? 2 : nsessions ? 0 : CR_SKIPPED;
}

int cr_bundle_mount_ok(const struct corpus_pass *p, char *err, size_t errlen)
{
    for (size_t i = 0; i < p->nmounts; i++)
        if (!strcasecmp(p->mount_name[i], CR_BUNDLE_VOLUME)) {
            snprintf(err, errlen, "the mount %s: has the name of the bundle's volume " CR_BUNDLE_VOLUME ":",
                     p->mount_name[i]);
            return -1;
        }
    return 0;
}

/* a member of a bundle: its case as a set of one in its one cell */
struct member {
    struct ctx x;
    struct caserun_opts o;
    struct set one;
    char *text;                 /* the source */
    char key[65];               /* the key of its inputs (vstore_key) */
    int prepared;               /* its cell runs in the sessions */
    char why[900];              /* not prepared: why */
};

static void member_free(struct member *mb)
{
    struct ctx *x = &mb->x;

    set_free(&mb->one);
    free(mb->text);
    free(x->mac);
    free(x->testdir);
    free(x->tokens);
    free(x->side);
    tp_free(&x->src);
    tc_matrix_free(&x->m);
    tc_free(&x->conf);
    tc_env_free(&x->env);
    mb->text = NULL;
    memset(x, 0, sizeof *x);
}

/* not prepared: the reason */
#define NOT_PREPARED(mb, ...) do { snprintf((mb)->why, sizeof (mb)->why, __VA_ARGS__); return 0; } while (0)

/* prepares member i of b (its set in rundir/ID__CASE): 0 (prepared, or
   not with the reason in why), 2 = an error of the bundle (out of memory) */
static int member_prepare(struct member *mb, const struct cr_bundle *b, size_t i, const char *rundir,
                          const char *base, struct vstore_cache *cache, FILE *log)
{
    struct ctx *x = &mb->x;
    char err[800];
    const char *macros_path;
    size_t len, cell;
    int r;

    mb->o.env = b->env;
    mb->o.source = b->source[i];
    mb->o.case_name = b->case_name[i];
    mb->o.require_deviation = b->require_deviation;
    mb->o.test = b->test[i];
    x->o = &mb->o;
    x->log = log;
    x->file = base_name(b->source[i]);
    x->base = base;
    if (tc_env_load(b->env, &x->env, err, sizeof err) || tc_resolve(b->source[i], &x->env, &x->conf, err, sizeof err)
        || tc_matrix_build(&x->conf, &x->m, err, sizeof err))
        NOT_PREPARED(mb, "its configuration: %s", err);
    if (!(x->testdir = str_printf(CR_TESTDIR, x->conf.id)))
        return 2;
    if (!(mb->text = (char *)read_file(b->source[i], &len)))
        NOT_PREPARED(mb, "cannot read %s", b->source[i]);
    {
        struct tp_params par = { 0 };

        par.file = x->file;
        par.test_id = x->conf.id;
        par.testdata = CR_TESTDATA;
        par.testdir = x->testdir;
        if ((r = tp_process(mb->text, len, &par, &x->src)) != 0) {
            if (r < 0)
                return 2;
            for (size_t k = 0; k < x->src.errors.n; k++)
                fprintf(log, "%s\n", x->src.errors.v[k]);
            NOT_PREPARED(mb, "its source does not preprocess");
        }
        if (tj_editor_run_refused(x->file, &x->src, &x->conf, &x->m, err, sizeof err))
            NOT_PREPARED(mb, "%s", err);
    }
    if ((r = tj_bundle_case(&x->conf, &x->m, &x->src, &x->env, b->case_name[i], &cell)) < 0)
        return 2;
    if (!r)
        NOT_PREPARED(mb, "the case may not be bundled (tj_bundle_case)");
    if (!(macros_path = need(&x->env, "macros", err, sizeof err)))
        NOT_PREPARED(mb, "%s", err);
    if (!(x->mac = (char *)read_file(macros_path, &x->mlen)))
        NOT_PREPARED(mb, "cannot read %s", macros_path);
    if (!(x->side = calloc(2, sizeof *x->side)) || !(mb->one.cases = calloc(2, sizeof *mb->one.cases))
        || !(mb->one.idx = calloc(2, sizeof *mb->one.idx))
        || !(mb->one.dir = str_printf("%s/%s__%s", rundir, x->conf.id, b->case_name[i])))
        return 2;
    x->side[0] = cell;
    x->nside = 1;
    mb->one.cases[0] = b->case_name[i];
    mb->one.ncases = 1;
    mb->one.idx[0] = cell;
    mb->one.ncells = 1;
    if (mkdir_p(mb->one.dir))
        NOT_PREPARED(mb, "cannot make %s", mb->one.dir);
    if (!(x->tokens = token_list(&x->conf, &x->env, mb->one.dir, log, err, sizeof err)))
        NOT_PREPARED(mb, "%s", err);
    if ((r = set_prepare(x, &mb->one)) == CR_SKIPPED)
        NOT_PREPARED(mb, "skipped: a file of its cell is missing");
    if (r)
        NOT_PREPARED(mb, "its cell cannot be prepared");
    if (cr_bundle_mount_ok(&mb->one.c[0].p, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s: %s\n", b->test[i], err);
        NOT_PREPARED(mb, "refused: %s", err);
    }
    if (case_key(x, &mb->one.c[0].p, mb->one.c[0].key, b->test[i], cache, mb->key))
        return 2;
    mb->prepared = 1;
    return 0;
}

/* observes and judges a member whose cell ran, stores a clean PASSED or
   REPRODUCED: 0 (*stored), 2 = an error of the bundle */
static int member_judge(struct member *mb, const char *name, const char *store, int *stored, FILE *log)
{
    struct ctx *x = &mb->x;
    const struct set *from[1] = { &mb->one };
    size_t at[1] = { 0 }, blen = 0;
    char *block = NULL, err[800];
    FILE *mem;
    int v;

    *stored = 0;
    if (set_observe(x, &mb->one))
        return 2;
    if (!(mem = open_memstream(&block, &blen)))
        return 2;
    v = judge_case(x, mb->o.case_name, from, at, 1, "", mem);
    if (fclose(mem) || v < 0 || !block) {
        free(block);
        return 2;
    }
    /* the block, its verdict line marked */
    if (blen && block[blen - 1] == '\n')
        fprintf(log, "%.*s (bundle %s)\n", (int)(blen - 1), block, name);
    else
        fprintf(log, "%s", block);
    if (v != TJ_V_PASSED && v != TJ_V_REPRODUCED)
        fprintf(log, "%s: %s not stored (its case test runs it alone)\n", mb->o.test, cr_verdict_name(v));
    else if (!cr_cell_clean(&mb->one.ob[0].o))
        fprintf(log, "%s: %s not stored (its cell did not end cleanly)\n", mb->o.test, cr_verdict_name(v));
    else {
        char *e = vstore_entry(mb->key, name, cr_side_status(v, 0), block);

        if (!e || vstore_put(store, mb->o.test, e, err, sizeof err)) {
            fprintf(log, "corpusrun case: %s\n", e ? err : "out of memory");
            free(e);
            free(block);
            return 2;
        }
        free(e);
        fprintf(log, "%s: %s stored\n", mb->o.test, cr_verdict_name(v));
        *stored = 1;
    }
    free(block);
    return 0;
}

int caserun_bundle(const struct cr_bundle *b, FILE *log)
{
    struct tc_env env = { 0 };
    struct runwork rw = { NULL, NULL, -1 };
    struct vstore_cache cache = { 0 };
    struct member *mb = NULL;
    char err[800], *base = NULL, *store = NULL, *bdir = NULL;
    const char **keys = NULL;
    size_t *chunk = NULL, *pick = NULL, nchunks = 0, nprep = 0, nsessions = 0, nstored = 0;
    struct cell **cells = NULL;
    int *done = NULL, error = 0, ran = 0, stop = 0, rc = 2;

    if (!b->name || !*b->name || strchr(b->name, '/') || *b->name == '.') {
        fprintf(log, "corpusrun case: \"%s\" is no name of a bundle\n", b->name ? b->name : "");
        return 2;
    }
    for (size_t i = 0; i < b->n; i++)
        for (size_t k = 0; k < i; k++)
            if (!strcmp(b->test[i], b->test[k])) {
                fprintf(log, "corpusrun case: the member %s is named twice\n", b->test[i]);
                return 2;
            }
    if (tc_env_load(b->env, &env, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    {
        const char *work = need(&env, "work", err, sizeof err);

        if (!work) {
            fprintf(log, "corpusrun case: %s\n", err);
            goto out;
        }
        if (!(base = str_printf("%s/%s", work, b->name))) {
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
    }
    if (!(store = vstore_dir(&env))) {
        fprintf(log, "corpusrun case: no run directory for the verdicts (" RUNDIR_ENV ", run-dir)\n");
        goto out;
    }
    if (runwork_open(&rw, base, err, sizeof err) || runwork_space(base, RUNWORK_MIN_FREE_MIB, err, sizeof err)) {
        fprintf(log, "corpusrun case: %s\n", err);
        goto out;
    }
    for (size_t i = 0; i < b->n; i++)
        if (vstore_clear(store, b->test[i])) {
            fprintf(log, "corpusrun case: the verdicts of %s in %s cannot be removed\n", b->test[i], store);
            goto out;
        }
    mb = calloc(b->n + 1, sizeof *mb);
    keys = calloc(b->n + 1, sizeof *keys);
    chunk = calloc(b->n + 1, sizeof *chunk);
    pick = calloc(b->n + 1, sizeof *pick);
    cells = calloc(CR_BUNDLE_CELLS, sizeof *cells);
    done = calloc(CR_BUNDLE_CELLS, sizeof *done);
    bdir = str_printf("%s/bundle", rw.dir);
    if (!mb || !keys || !chunk || !pick || !cells || !done || !bdir) {
        fprintf(log, "corpusrun case: out of memory\n");
        goto out;
    }
    /* the members' cells; pick[j] the member of the j-th prepared cell */
    for (size_t i = 0; i < b->n; i++) {
        if (member_prepare(&mb[i], b, i, rw.dir, base, &cache, log)) {
            fprintf(log, "corpusrun case: out of memory\n");
            goto out;
        }
        if (!mb[i].prepared) {
            fprintf(log, "%s: not stored (%s)\n", b->test[i], mb[i].why);
            member_free(&mb[i]);
            continue;
        }
        keys[nprep] = mb[i].one.c[0].key;
        pick[nprep++] = i;
    }
    vstore_cache_free(&cache);
    nchunks = cr_bundle_chunks(keys, nprep, chunk);
    for (size_t ch = 0; ch < nchunks; ch++) {
        size_t in[CR_BUNDLE_CELLS], nin = 0, pend[CR_BUNDLE_CELLS];

        for (size_t j = 0; j < nprep; j++)
            if (chunk[j] == ch)
                in[nin++] = pick[j];
        /* the sessions of the chunk: the first, again for the cells behind
           an unclean end */
        for (size_t s = 0; !stop && !error; s++) {
            size_t np;
            int r;

            for (size_t k = 0; k < nin; k++)
                done[k] = mb[in[k]].one.c[0].done;
            if (!(np = cr_bundle_pending(done, nin, s, pend)))
                break;
            for (size_t k = 0; k < np; k++) {
                cells[k] = &mb[in[pend[k]]].one.c[0];
                fprintf(log, "corpusrun case: bundle cell c%zu: %s\n", k, b->test[in[pend[k]]]);
            }
            ran = 1;
            if ((r = run_session(cells, np, base, log, &nsessions, bdir)) == CR_SKIPPED)
                stop = 1;
            else if (r)
                error = 1;
        }
        /* judge what ran */
        for (size_t k = 0; k < nin; k++) {
            struct member *m = &mb[in[k]];
            int st;

            if (m->one.c[0].done) {
                if (member_judge(m, b->name, store, &st, log)) {
                    fprintf(log, "corpusrun case: %s cannot be judged or stored\n", b->test[in[k]]);
                    error = 1;
                } else
                    nstored += (size_t)st;
            } else if (stop)
                fprintf(log, "%s: not stored (skipped: no emulator)\n", b->test[in[k]]);
            else if (error)
                fprintf(log, "%s: not stored (an error of the bundle)\n", b->test[in[k]]);
            else
                fprintf(log, "%s: not stored (not reached in %d sessions)\n", b->test[in[k]], CR_BUNDLE_SESSIONS);
            member_free(m);
        }
    }
    fprintf(log, "%s: %zu members, %zu stored\n", b->name, b->n, nstored);
    rc = cr_bundle_status(error, nsessions);
    /* skipped before any session: nothing to keep */
    if (rc == CR_SKIPPED)
        ran = 0;
out:
    if (rw.fd >= 0)
        rc = runwork_finish(&rw, ran, rc, NULL, NULL, log, "corpusrun case");
    for (size_t i = 0; mb && i < b->n; i++)
        member_free(&mb[i]);
    vstore_cache_free(&cache);
    free(mb);
    free(keys);
    free(chunk);
    free(pick);
    free(cells);
    free(done);
    free(bdir);
    free(store);
    free(base);
    tc_env_free(&env);
    return rc;
}
