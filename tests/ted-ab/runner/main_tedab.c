/*
 * tedab - the TED A/B tests in Amiberry (bb2 ted-c/tools/tedab.py), see
 * tedab.h for the test scripts.
 *
 *   tedab PASSFILE          run the tests, report
 *   tedab --lint PASSFILE   only check the scripts and their sources
 *
 * The pass file (written by CMake), lines KEY=VALUE, '#' comments:
 *   work=DIR           work directory (emptied first)
 *   report=PATH        the report (also to stdout)
 *   scripts=DIR        the test scripts (*.drv; gate-*.drv are gate D's)
 *   filter=GLOB,...    only these tests (names without .drv; default all)
 *   combos=XX,...      the combinations, the reference first (default
 *                      R5,C5,Cc,Rc)
 *   src=DIR            the sources of @source E / S
 *   corpus_src=DIR     the tokenised corpus tests (@source T: NAME.bb2)
 *   corpus_out=DIR     the corpus ref run's results: its executables
 *                      (@expect corpus) and defaults.xtra (@xtra norunerr
 *                      without the source's own .xtra)
 *   expect_dir=DIR     the scripts' NAME.expect tokenised (NAME.expect.bb2)
 *   cd=DIR             the CD (@source K, @expect cd, the libraries)
 *   oracle=PATH        the TED oracle manifest (@expect tedsave)
 *   fonts=DIR          chars8.b / chars.b of TED (decoding the dumps)
 *   teddrive=, teddrivetest=, tedprobe=, reqtrap=PATH   the tools
 *   ted_ref=, ted_c=, blitz2_ref=, blitz2_c=PATH        R/C and 5/c
 *   timeout=SEC        per emulator session (default 120)
 *   budget=SEC         estimated run time (@est) per session (default 60:
 *                      boot ~15 s and gates ~10 s come on top)
 *   parallel=N         emulator sessions at the same time
 *   stack=N            Stack for teddrive (default 40000)
 *   gates=DIR          gate-d.drv and gate-d-rescue.drv (default: scripts)
 *   inject=, wbrun=PATH  optional: tools/inject, tools/wbrun for @command
 *                      (tests/bug-orig/amiga)
 *   bugorig=1          the bug-orig mode (tests/bug-orig/ted): every run in
 *                      a session of its own; the reference (the first
 *                      combination) and each other one get a verdict
 *                      (tedab_bugorig_verdict); exit status 0 = all FIXED,
 *                      1 = REPRODUCED, 3 = BROKEN, 4 = DIFFERS (the worst
 *                      of them: 3 before 4 before 1), 2 = error
 *
 * The runs are packed into sessions (the combinations of a test stay
 * together).  A session that ends early (hang: time limit) leaves runs
 * that were not reached; they go into a later session once more if their
 * test did not start there or its first run is OK.  The rest stays
 * "NOTRUN".  Each session's directory: work/sNNN/vol (the volume TEDAB:)
 * and work/sNNN/emu (Amiberry); the dumps get a decoded NAME.txt.
 * Tests with @known are not run (unless the filter names them exactly); the
 * report lists them as KNOWN with their reason, they do not fail the run.
 *
 * Exit status 0 = all gates and runs OK and every comparison the same,
 * 1 = not, 77 = the emulator or a file is missing (skipped), 2 = error.
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup */
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "tedab.h"
#include "util.h"

#define MAXSESSIONS 200

struct pass {
    char *work, *report, *scripts, *filter, *combos, *src, *corpus_src, *corpus_out;
    char *expect_dir, *cd, *oracle, *fonts;
    char *teddrive, *teddrivetest, *tedprobe, *reqtrap, *ted_ref, *ted_c, *blitz2_ref, *blitz2_c;
    char *inject, *wbrun, *gates;
    unsigned timeout, budget, parallel, stack, bugorig;
};

/* one run over all sessions */
struct run {
    struct tedab_run r;
    int state, attempts, done, gates_ok;  /* gates_ok: of its last session */
    unsigned session;           /* the last one it was in */
    char *dir;                  /* its run directory (work/sNNN/vol/runs/T/XX) */
};

static struct pass P;
static struct tedab_test *tests;
static size_t ntests;
static struct tedab_test *known;  /* @known, not run */
static size_t nknown;
static struct run *runs;
static size_t nruns;
static int combo_order[TEDAB_NCOMBOS], ncombo_order;

/* ----------------------------------------------------------- pass file */

static int parse(const char *text, char *err, size_t errlen)
{
    static const struct {
        const char *key;
        char **dst;
    } strs[] = {
        { "work", &P.work }, { "report", &P.report }, { "scripts", &P.scripts },
        { "filter", &P.filter }, { "combos", &P.combos }, { "src", &P.src },
        { "corpus_src", &P.corpus_src }, { "corpus_out", &P.corpus_out },
        { "expect_dir", &P.expect_dir }, { "cd", &P.cd }, { "oracle", &P.oracle },
        { "fonts", &P.fonts }, { "teddrive", &P.teddrive }, { "teddrivetest", &P.teddrivetest },
        { "tedprobe", &P.tedprobe }, { "reqtrap", &P.reqtrap }, { "ted_ref", &P.ted_ref },
        { "ted_c", &P.ted_c }, { "blitz2_ref", &P.blitz2_ref }, { "blitz2_c", &P.blitz2_c },
        { "inject", &P.inject }, { "wbrun", &P.wbrun }, { "gates", &P.gates },
    };
    static const struct {
        const char *key;
        unsigned *dst;
    } nums[] = {
        { "timeout", &P.timeout }, { "budget", &P.budget }, { "parallel", &P.parallel },
        { "stack", &P.stack }, { "bugorig", &P.bugorig },
    };
    const char *line = text;
    unsigned no = 0;
    size_t i;

    P.timeout = 120;
    P.budget = 60;
    P.parallel = 1;
    P.stack = 40000;
    while (*line) {
        const char *eol = strchr(line, '\n'), *v;
        char key[32], *val;
        size_t n;
        int found = 0;

        if (!eol)
            eol = line + strlen(line);
        no++;
        n = (size_t)(eol - line);
        if (n && line[n - 1] == '\r')
            n--;
        if (!n || *line == '#') {
            line = *eol ? eol + 1 : eol;
            continue;
        }
        if (!(v = memchr(line, '=', n)) || (size_t)(v - line) >= sizeof key) {
            snprintf(err, errlen, "line %u: expected KEY=VALUE", no);
            return -1;
        }
        memcpy(key, line, (size_t)(v - line));
        key[v - line] = 0;
        v++;
        val = strndup(v, n - (size_t)(v - line));
        line = *eol ? eol + 1 : eol;
        if (!val)
            return -1;
        for (i = 0; !found && i < sizeof strs / sizeof strs[0]; i++)
            if (!strcmp(key, strs[i].key)) {
                free(*strs[i].dst);
                *strs[i].dst = val;
                found = 1;
            }
        for (i = 0; !found && i < sizeof nums / sizeof nums[0]; i++)
            if (!strcmp(key, nums[i].key)) {
                char *e;
                unsigned long u = strtoul(val, &e, 10);

                if (!*val || *e || !u) {
                    snprintf(err, errlen, "line %u: %s needs a number > 0", no, key);
                    free(val);
                    return -1;
                }
                *nums[i].dst = (unsigned)u;
                free(val);
                found = 1;
            }
        if (!found) {
            snprintf(err, errlen, "line %u: unknown key %s", no, key);
            free(val);
            return -1;
        }
    }
    {
        char **need[] = { &P.work, &P.report, &P.scripts, &P.src, &P.corpus_src, &P.corpus_out,
                          &P.expect_dir, &P.cd, &P.oracle, &P.fonts, &P.teddrive,
                          &P.teddrivetest, &P.tedprobe, &P.reqtrap, &P.ted_ref, &P.ted_c,
                          &P.blitz2_ref, &P.blitz2_c };

        for (i = 0; i < sizeof need / sizeof need[0]; i++)
            if (!*need[i]) {
                snprintf(err, errlen, "work, report, scripts, src, corpus_src, corpus_out, "
                                      "expect_dir, cd, oracle, fonts and the tools are needed");
                return -1;
            }
    }
    /* the combinations */
    {
        char *c = strdup(P.combos ? P.combos : "R5,C5,Cc,Rc"), *w, *save = NULL;

        for (w = c ? strtok_r(c, ",", &save) : NULL; w; w = strtok_r(NULL, ",", &save)) {
            int k = tedab_combo(w);

            if (k < 0 || ncombo_order >= TEDAB_NCOMBOS) {
                snprintf(err, errlen, "combos: unknown or too many: %s", w);
                free(c);
                return -1;
            }
            combo_order[ncombo_order++] = k;
        }
        free(c);
        if (!ncombo_order) {
            snprintf(err, errlen, "combos: none");
            return -1;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------- tests */

static int by_name(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* 0 = not selected, 1 = by a glob (or no filter), 2 = by its exact name */
static int selected(const char *name)
{
    char *f, *w, *save = NULL;
    int hit = 0;

    if (!P.filter || !*P.filter)
        return 1;
    f = strdup(P.filter);
    for (w = f ? strtok_r(f, ",", &save) : NULL; w && !hit; w = strtok_r(NULL, ",", &save))
        hit = !strcmp(w, name) ? 2 : !fnmatch(w, name, 0);
    free(f);
    return hit;
}

/* all scripts parsed and checked (lint); the selected ones in tests[];
   returns the number of problems */
static int load_tests(FILE *log)
{
    DIR *d = opendir(P.scripts);
    struct dirent *e;
    char **names = NULL;
    size_t n = 0, i;
    int bad = 0;

    if (!d) {
        fprintf(log, "tedab: no %s\n", P.scripts);
        return 1;
    }
    while ((e = readdir(d))) {
        size_t l = strlen(e->d_name);
        char **nn;

        if (l < 5 || strcmp(e->d_name + l - 4, ".drv"))
            continue;
        if (!(nn = realloc(names, (n + 1) * sizeof *nn)))
            break;
        names = nn;
        names[n++] = strndup(e->d_name, l - 4);
    }
    closedir(d);
    qsort(names, n, sizeof *names, by_name);
    tests = calloc(n + 1, sizeof *tests);
    known = calloc(n + 1, sizeof *known);
    for (i = 0; tests && known && i < n; i++) {
        char *path = str_printf("%s/%s.drv", P.scripts, names[i]), err[300];
        size_t len;
        char *text = path ? (char *)read_file(path, &len) : NULL;
        int gate = !strncmp(names[i], "gate-", 5), k, sel;

        free(path);
        if (!text) {
            fprintf(log, "%s.drv: cannot read\n", names[i]);
            bad++;
            continue;
        }
        {
            char *buf = NULL;
            size_t blen = 0;
            FILE *m = open_memstream(&buf, &blen);

            if (m) {
                k = tedab_lint(text, m);
                fclose(m);
                if (k) {
                    const char *l;

                    for (l = buf; l && *l;) {
                        const char *eol = strchr(l, '\n');

                        if (!eol)
                            eol = l + strlen(l);
                        fprintf(log, "%s.drv: %.*s\n", names[i], (int)(eol - l), l);
                        l = *eol ? eol + 1 : eol;
                    }
                    bad += k;
                }
                free(buf);
            }
        }
        if (!gate) {
            struct tedab_test t;

            if (tedab_test_parse(names[i], text, &t, err, sizeof err)) {
                fprintf(log, "%s\n", err);
                tedab_test_free(&t);
                bad++;
            } else if (!(sel = selected(names[i])))
                tedab_test_free(&t);
            else if (t.known && sel < 2)
                known[nknown++] = t;       /* a known failure: only by name */
            else
                tests[ntests++] = t;
        }
        free(text);
    }
    for (i = 0; i < n; i++)
        free(names[i]);
    free(names);
    return bad;
}

/* -------------------------------------------------------------- staging */

static int copy_to(const char *from, const char *dir, const char *name)
{
    char *to = str_printf("%s/%s", dir, name);
    int r = to && !copy_file(from, to) ? 0 : -1;

    free(to);
    return r;
}

/* the files of a test below work/staged/TEST/: the source (t->file), its
   .xtra, blitzeditor.opts; 0 = ok, 77 = something missing (message) */
static int stage_test(const struct tedab_test *t, FILE *log)
{
    char *dir = str_printf("%s/staged/%s", P.work, t->name), *src = NULL, *xtra = NULL;
    size_t len;
    uint8_t *data;
    int r = 0;

    if (!dir || mkdir_p(dir)) {
        free(dir);
        return 2;
    }
    switch (t->src_kind) {
    case 'T':
        src = str_printf("%s/%s.bb2", P.corpus_src, t->src_arg);
        break;
    case 'E':
        /* as text: TED tokenises it when it loads it (the long line is
           longer than tedtok takes) */
        src = str_printf("%s/%s.bas", P.src, t->src_arg);
        break;
    case 'S':
        src = str_printf("%s/%s.bb2", P.src, t->src_arg);
        xtra = str_printf("%s/%s.bb2.xtra", P.src, t->src_arg);
        break;
    case 'F':                   /* one of the @file, below */
        break;
    case 'K':
        src = tedab_resolve(P.cd, t->src_arg);
        {
            char *x = str_printf("%s.xtra", t->src_arg);

            xtra = x ? tedab_resolve(P.cd, x) : NULL;
            free(x);
        }
        if (!src) {
            fprintf(log, "tedab: %s: no %s on the CD %s\n", t->name, t->src_arg, P.cd);
            r = 77;
        }
        break;
    }
    if (!r && src && copy_to(src, dir, t->file)) {
        fprintf(log, "tedab: %s: cannot copy %s\n", t->name, src);
        r = 77;
    }
    for (size_t k = 0; !r && k < t->nfiles; k++) {
        char *to = str_printf("%s/%s", dir, t->files[k].name);

        r = to && !write_file(to, t->files[k].data, t->files[k].len) ? 0 : 2;
        free(to);
    }
    if (!r && t->file) {
        char *xname = str_printf("%s.xtra", t->file), *from = NULL;
        int own = xtra && !access(xtra, R_OK);

        if (t->norunerr) {
            /* runtime errors and debugger off: bytes 2-5 of the .xtra */
            char *dflt = str_printf("%s/defaults.xtra", P.corpus_out);

            from = own ? xtra : dflt;
            data = from ? read_file(from, &len) : NULL;
            if (!data || len < 6) {
                fprintf(log, "tedab: %s: no %s (the corpus test makes it)\n", t->name,
                        from ? from : "defaults.xtra");
                r = 77;
            } else {
                char *to = str_printf("%s/%s", dir, xname);

                memset(data + 2, 0, 4);
                r = to && !write_file(to, data, len) ? 0 : 2;
                free(to);
            }
            free(data);
            free(dflt);
        } else if (own && copy_to(xtra, dir, xname))
            r = 2;
        free(xname);
    }
    if (!r && t->opts) {
        uint8_t o[50];
        char err[200], *to = str_printf("%s/blitzeditor.opts", dir);

        r = !tedab_opts_file(t->opts, o, err, sizeof err) && to && !write_file(to, o, sizeof o)
            ? 0 : 2;
        free(to);
    }
    free(src);
    free(xtra);
    free(dir);
    return r;
}

static char *session_dir(unsigned s, const char *sub)
{
    return str_printf("%s/s%03u/%s", P.work, s, sub);
}

/* copies the files of staged/TEST: blitzeditor.opts to L/, the rest to work/ */
static int stage_run(const struct tedab_test *t, const char *rundir)
{
    char *from = str_printf("%s/staged/%s", P.work, t->name);
    char *work = str_printf("%s/work", rundir), *l = str_printf("%s/L", rundir);
    char *out = str_printf("%s/out", rundir);
    DIR *d = from ? opendir(from) : NULL;
    struct dirent *e;
    int r = d && work && l && out && !mkdir_p(work) && !mkdir_p(l) && !mkdir_p(out) ? 0 : -1;

    while (!r && (e = readdir(d))) {
        char *f;

        if (e->d_name[0] == '.')
            continue;
        f = str_printf("%s/%s", from, e->d_name);
        r = f && !copy_to(f, strcmp(e->d_name, "blitzeditor.opts") ? work : l, e->d_name)
            ? 0 : -1;
        free(f);
    }
    if (d)
        closedir(d);
    free(from);
    free(work);
    free(l);
    free(out);
    return r;
}

/* the session's volume: tools, gate, the combinations, the runs */
static int stage_session(unsigned s, const struct tedab_run *rs, size_t n, FILE *log)
{
    static const char *const libs[] = { "deflibs", "acidlibs", "defaultdbug", "help",
                                        "help.dat" };
    char *vol = session_dir(s, "vol"), *dir;
    const char *tools[6] = { P.tedprobe, P.teddrive, P.teddrivetest, P.reqtrap, P.inject,
                             P.wbrun };
    static const char *const tnames[6] = { "tedprobe", "teddrive", "teddrivetest", "reqtrap",
                                           "inject", "wbrun" };
    int r = vol ? 0 : -1, c;
    size_t i, k;

    for (i = 0; !r && i < 6; i++) {
        if (!tools[i])
            continue;
        dir = str_printf("%s/tools", vol);
        r = dir && !mkdir_p(dir) && !copy_to(tools[i], dir, tnames[i]) ? 0 : -1;
        free(dir);
    }
    if (!r) {
        char *g = str_printf("%s/gate", vol), *go = str_printf("%s/gate/out", vol);
        char *d1 = str_printf("%s/gate-d.drv", P.gates ? P.gates : P.scripts);
        char *d2 = str_printf("%s/gate-d-rescue.drv", P.gates ? P.gates : P.scripts);

        r = g && go && d1 && d2 && !mkdir_p(go) && !copy_to(d1, g, "gate-d.drv")
            && !copy_to(d2, g, "gate-d-rescue.drv") ? 0 : -1;
        free(g);
        free(go);
        free(d1);
        free(d2);
    }
    for (c = 0; !r && c < TEDAB_NCOMBOS; c++) {
        int used = 0;

        for (i = 0; i < n; i++)
            used |= rs[i].combo == c;
        if (!used)
            continue;
        dir = str_printf("%s/combo/%s/blitz2", vol, tedab_combo_names[c]);
        r = dir && !mkdir_p(dir) ? 0 : -1;
        if (!r)
            r = copy_to(c == TEDAB_R5 || c == TEDAB_C5 ? P.blitz2_ref : P.blitz2_c, dir, "blitz2")
                || copy_to(c == TEDAB_R5 || c == TEDAB_RC ? P.ted_ref : P.ted_c, dir, "ted")
                ? -1 : 0;
        for (k = 0; !r && k < sizeof libs / sizeof libs[0]; k++) {
            char *rel = str_printf("BLITZ2/%s", libs[k]), *from = rel ? tedab_resolve(P.cd, rel)
                                                                      : NULL;

            if (from && copy_to(from, dir, libs[k]))
                r = -1;
            free(from);
            free(rel);
        }
        if (!r) {
            char *u = str_printf("%s/userlibs", dir);

            r = u && !mkdir_p(u) ? 0 : -1;
            free(u);
        }
        free(dir);
    }
    for (i = 0; !r && i < n; i++) {
        char *from = str_printf("%s/%s.drv", P.scripts, rs[i].test->name);
        char *td = str_printf("%s/tests", vol), *name = str_printf("%s.drv", rs[i].test->name);
        char *rd = str_printf("%s/runs/%s/%s", vol, rs[i].test->name,
                              tedab_combo_names[rs[i].combo]);

        r = from && td && name && rd && !mkdir_p(td) && !copy_to(from, td, name)
            && !stage_run(rs[i].test, rd) ? 0 : -1;
        free(from);
        free(td);
        free(name);
        free(rd);
    }
    if (r)
        fprintf(log, "tedab: cannot set up %s\n", vol ? vol : "the session");
    free(vol);
    return r;
}

/* ------------------------------------------------------------ sessions */

struct session {
    unsigned no;
    size_t first, n;            /* runs list[first..first+n) */
    struct amiga_emulation *e;
    int gates_ok;
    struct text gate, events;
};

/* the next session from the queue: whole tests up to the budget */
static size_t take(size_t *queue, size_t nq, size_t *pos)
{
    size_t start = *pos, i = start;
    unsigned used = 0;

    if (P.bugorig) {            /* one run per session */
        *pos = start + (start < nq);
        return *pos - start;
    }
    while (i < nq) {
        const struct tedab_test *t = runs[queue[i]].r.test;
        size_t j = i;
        unsigned cost = 0;

        while (j < nq && runs[queue[j]].r.test == t) {
            cost += (unsigned)t->est;
            j++;
        }
        if (i > start && used + cost > P.budget)
            break;
        used += cost;
        i = j;
    }
    *pos = i;
    return i - start;
}

/* decoded text next to every dump of a run */
static void decode_dumps(const char *rundir, const struct teddump_fonts *f, int fh)
{
    char *out = str_printf("%s/out", rundir);
    DIR *d = out ? opendir(out) : NULL;
    struct dirent *e;

    while (d && (e = readdir(d))) {
        size_t l = strlen(e->d_name);
        char *p, *q, *txt, err[200];
        struct teddump dump;

        if (l < 5 || strcmp(e->d_name + l - 4, ".scr"))
            continue;
        p = str_printf("%s/%s", out, e->d_name);
        q = str_printf("%s/%.*s.txt", out, (int)(l - 4), e->d_name);
        if (p && q && !teddump_load(p, &dump, err, sizeof err)) {
            if ((txt = teddump_text(&dump, f, fh))) {
                write_file(q, txt, strlen(txt));
                free(txt);
            }
            teddump_free(&dump);
        }
        free(p);
        free(q);
    }
    if (d)
        closedir(d);
    free(out);
}

/* teddrive marked the failure as #340 (TED's a0toname split the file name) */
static int hit340(const struct run *r)
{
    char *res = NULL, *out = r->dir ? str_printf("%s/out/result", r->dir) : NULL;
    size_t n;
    int hit;

    if (out)
        res = (char *)read_file(out, &n);
    free(out);
    hit = res && strstr(res, "(#340") != NULL;
    free(res);
    return hit;
}

static const char *state_text(const struct run *r, char *buf, size_t len)
{
    char *res = NULL, *out = r->dir ? str_printf("%s/out/result", r->dir) : NULL;
    size_t n;

    if (out)
        res = (char *)read_file(out, &n);
    free(out);
    if (res) {
        char *e = res + strlen(res);

        while (e > res && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' '))
            *--e = 0;
    }
    switch (r->state) {
    case TEDAB_NOTRUN:
        snprintf(buf, len, "NOTRUN");
        break;
    case TEDAB_SKIPPED:
        snprintf(buf, len, "SKIPPED (reference not OK)");
        break;
    case TEDAB_STARTED:
        snprintf(buf, len, res && *res ? "TIMEOUT (%s)" : "TIMEOUT", res);
        break;
    default:
        snprintf(buf, len, "%s", res && *res ? res : "NORESULT");
    }
    free(res);
    return buf;
}

/* bug-orig: the order of the verdicts for the exit status (the worst wins:
   an error, BROKEN, DIFFERS, REPRODUCED, FIXED) */
static int verdict_rank(int v)
{
    return v == 2 ? 4 : v == TEDAB_BROKEN ? 3 : v == TEDAB_DIFFERS ? 2 : v;
}

/* the observation of a run: its state and the problems of its
   expectations (without the combination name) */
static void observe(const struct tedab_test *t, const struct run *r, char *state, size_t slen,
                    struct text *problems)
{
    char *staged = str_printf("%s/staged/%s/%s", P.work, t->name, t->file ? t->file : "");
    struct tedab_sources src = { "-", staged, P.expect_dir, P.corpus_out, P.cd, P.oracle };

    state_text(r, state, slen);
    tedab_check_expects(t, r->dir, &src, problems);
    free(staged);
}

/* bug-orig: the verdict of every combination against the reference (the
   first one of combos) into the report; *worst = the worst exit status */
static void bugorig_verdicts(const struct tedab_test *t, struct run **tr,
                             const struct teddump_fonts *f, struct text *rep, int *worst)
{
    int ref = combo_order[0], c;
    char rs[300];
    struct text rp = { 0 };

    if (!tr[ref]) {
        text_add(rep, "  bug-orig: no run of the reference %s\n", tedab_combo_names[ref]);
        *worst = 2;
        return;
    }
    observe(t, tr[ref], rs, sizeof rs, &rp);
    for (c = 1; c < ncombo_order; c++) {
        int cc = combo_order[c], v, cmp = -1;
        char ps[300];
        struct text pp = { 0 };

        if (!tr[cc])
            continue;
        observe(t, tr[cc], ps, sizeof ps, &pp);
        if (!strcmp(rs, "OK") && !strcmp(ps, "OK")) {
            struct text d = { 0 };

            cmp = tedab_compare_runs(tr[ref]->dir, tr[cc]->dir, tedab_combo_names[ref],
                                     tedab_combo_names[cc], f, t->font, &d);
            free(d.p);
        }
        if (!strncmp(rs, "NOTRUN", 6) || !strncmp(rs, "SKIPPED", 7)
            || !strncmp(ps, "NOTRUN", 6) || !strncmp(ps, "SKIPPED", 7)
            || !tr[ref]->gates_ok || !tr[cc]->gates_ok)
            v = 2;              /* not run (or the tools failed): no verdict */
        else
            v = tedab_bugorig_verdict(rs, rp.p ? rp.p : "", ps, pp.p ? pp.p : "", cmp);
        text_add(rep, "  bug-orig %s against %s: %s\n", tedab_combo_names[cc],
                 tedab_combo_names[ref], v == 2 ? "NOT RUN" : tedab_verdict_name(v));
        if (v == TEDAB_BROKEN)
            text_add(rep, "    the reference shows the expected behaviour: the original does "
                          "not have the bug, or the expectation describes it\n");
        if (*worst < 0 || verdict_rank(v) > verdict_rank(*worst))
            *worst = v;
        free(pp.p);
    }
    free(rp.p);
}

static int run(FILE *log)
{
    const char *files[] = { P.teddrive, P.teddrivetest, P.tedprobe, P.reqtrap, P.ted_ref, P.ted_c,
                            P.blitz2_ref, P.blitz2_c, P.oracle };
    struct session *ss = calloc(MAXSESSIONS + 1, sizeof *ss);
    size_t *queue = NULL, nq = 0, i, k;
    struct teddump_fonts fonts;
    unsigned nsess = 0;
    struct text rep = { 0 };
    int rc = 2, all_ok = 1, c, worst = -1;

    for (i = 0; i < sizeof files / sizeof files[0]; i++)
        if (access(files[i], R_OK)) {
            fprintf(log, "tedab: no %s - skipped\n", files[i]);
            return 77;
        }
    if (access(P.cd, R_OK | X_OK)) {
        fprintf(log, "tedab: no CD %s - skipped\n", P.cd);
        return 77;
    }
    if (teddump_load_fonts(P.fonts, &fonts)) {
        fprintf(log, "tedab: no fonts in %s\n", P.fonts);
        return 2;
    }
    if (!ss)
        return 2;
    rm_tree(P.work);
    for (i = 0; i < ntests; i++)
        if ((rc = stage_test(&tests[i], log))) {
            free(ss);
            return rc;
        }
    rc = 2;
    /* the runs: per test in the order of combos */
    runs = calloc(ntests * TEDAB_NCOMBOS + 1, sizeof *runs);
    queue = calloc(ntests * TEDAB_NCOMBOS + 1, sizeof *queue);
    if (!runs || !queue)
        goto out;
    for (i = 0; i < ntests; i++)
        for (c = 0; c < ncombo_order; c++)
            if (tests[i].combos & (1u << combo_order[c])) {
                runs[nruns].r.test = &tests[i];
                runs[nruns].r.combo = combo_order[c];
                queue[nq++] = nruns++;
            }
    for (i = 0; i < ntests; i++) {
        unsigned cost = 0;

        for (k = 0; k < nruns; k++)
            if (runs[k].r.test == &tests[i])
                cost += (unsigned)tests[i].est;
        if (!P.bugorig && cost > P.budget) {
            fprintf(log, "tedab: %s needs ~%u s > budget %u s: shorten the test\n",
                    tests[i].name, cost, P.budget);
            goto out;
        }
    }
    fprintf(log, "tedab: %zu tests, %zu runs, at most %u sessions at the same time\n", ntests,
            nruns, P.parallel);

    /* rounds: up to `parallel` sessions from the queue */
    while (nq && nsess < MAXSESSIONS) {
        size_t pos = 0, *list = calloc(nq, sizeof *list), *next = calloc(nq, sizeof *next),
               nn = 0;
        unsigned round = 0, first = nsess;

        if (!list || !next) {
            free(list);
            free(next);
            goto out;
        }
        memcpy(list, queue, nq * sizeof *list);
        while (round < P.parallel && pos < nq && nsess < MAXSESSIONS) {
            struct session *s = &ss[nsess];
            struct tedab_run *rs;
            char *script, *vol, *emu, err[300];
            size_t at = pos, m = take(list, nq, &pos);

            s->no = ++nsess;
            s->first = at;
            s->n = m;
            rs = calloc(m, sizeof *rs);
            for (k = 0; rs && k < m; k++)
                rs[k] = runs[list[at + k]].r;
            if (!rs || stage_session(s->no, rs, m, log)) {
                free(rs);
                free(list);
                free(next);
                goto out;
            }
            script = tedab_session_script(rs, m, P.stack);
            free(rs);
            if (!script || tedab_check_script(script, log)) {
                free(script);
                free(list);
                free(next);
                goto out;
            }
            vol = session_dir(s->no, "vol");
            emu = session_dir(s->no, "emu");
            {
                char *sp = session_dir(s->no, "vol/session.script");

                if (sp)
                    write_file(sp, script, strlen(script));
                free(sp);
            }
            if (vol && emu && !mkdir_p(emu)) {
                struct amiga_emulation_mount mnt = { TEDAB_VOL, vol, 1 };
                struct amiga_emulation_params ep = { 0 };

                ep.mounts = &mnt;
                ep.nmounts = 1;
                ep.script = script;
                ep.timeout = P.timeout;
                ep.work_dir = emu;
                s->e = amiga_emulation_start(&ep, err, sizeof err);
            }
            free(script);
            free(vol);
            free(emu);
            /* no free emulator slot while sessions of this round run: the
               round ends here (the slot limit is hard), the rest of the
               queue goes to the next one */
            if (!s->e && round && amiga_emulation_no_slot()) {
                char *sd = session_dir(s->no, "");

                fprintf(log, "tedab: %s - the round ends with %u sessions\n", err, round);
                if (sd)
                    rm_tree(sd);
                free(sd);
                nsess--;
                pos = at;
                break;
            }
            if (!s->e) {
                fprintf(log, "tedab: %s - skipped\n", err);
                free(list);
                free(next);
                rc = 77;
                goto out;
            }
            for (k = 0; k < m; k++) {
                struct run *r = &runs[list[at + k]];

                r->session = s->no;
                r->attempts++;
                free(r->dir);
                r->dir = str_printf("%s/s%03u/vol/runs/%s/%s", P.work, s->no, r->r.test->name,
                                    tedab_combo_names[r->r.combo]);
            }
            fprintf(log, "tedab: session %03u: %zu runs\n", s->no, m);
            round++;
        }
        /* the rest of the queue waits for the next round */
        for (k = pos; k < nq; k++)
            next[nn++] = list[k];
        for (unsigned w = first; w < nsess; w++) {
            struct session *s = &ss[w];
            enum amiga_emulation_status st = amiga_emulation_join(s->e);
            char *lp = session_dir(s->no, "vol/session.log"), *gd = session_dir(s->no,
                                                                                "vol/gate/out");
            size_t len, again = 0;
            char *text = lp ? (char *)read_file(lp, &len) : NULL;
            struct tedab_run *rs = calloc(s->n, sizeof *rs);
            int *state = calloc(s->n, sizeof *state);

            amiga_emulation_free(s->e);
            s->e = NULL;
            for (k = 0; rs && k < s->n; k++)
                rs[k] = runs[list[s->first + k]].r;
            if (rs && state)
                tedab_session_log(text, rs, s->n, state, &s->events);
            s->gates_ok = gd ? tedab_eval_gate(gd, &s->gate) : 0;
            for (k = 0; rs && state && k < s->n; k++) {
                struct run *r = &runs[list[s->first + k]];
                size_t f, d;
                int requeue = 0;

                r->state = state[k];
                r->gates_ok = s->gates_ok;
                /* the run hit #340 (TED's a0toname, depends on the memory
                   layout), or was skipped because the run it waited for did
                   (tedab_session_waits_for): again, first in a new session.
                   Not with @command: teddrive takes the last word of any
                   command for the file argument */
                for (d = k; d && state[d] == TEDAB_SKIPPED; d = tedab_session_waits_for(rs, d))
                    ;
                if ((hit340(r) || (r->state == TEDAB_SKIPPED && hit340(&runs[list[s->first + d]])))
                    && !r->r.test->command && r->attempts < 3) {
                    fprintf(log, "tedab: %s %s: #340 (a0toname), again\n", r->r.test->name,
                            tedab_combo_names[r->r.combo]);
                    next[nn++] = list[s->first + k];
                    again++;
                    continue;
                }
                if (r->state != TEDAB_NOTRUN) {
                    r->done = 1;
                    if (r->dir)
                        decode_dumps(r->dir, &fonts, r->r.test->font);
                    continue;
                }
                /* not reached: again if the test's first run in this
                   session was not reached either or is OK */
                for (f = 0; f < s->n && rs[f].test != r->r.test; f++)
                    ;
                if (f == k)
                    requeue = 1;
                else {
                    char *ok = str_printf("%s/out/ok", runs[list[s->first + f]].dir);

                    requeue = state[f] == TEDAB_NOTRUN || (ok && !access(ok, R_OK));
                    free(ok);
                }
                if (requeue && r->attempts < 2) {
                    next[nn++] = list[s->first + k];
                    again++;
                } else
                    r->done = 1;
            }
            fprintf(log, "tedab: session %03u: %s, gates %s, %zu runs again\n", s->no,
                    amiga_emulation_status_name(st), s->gates_ok ? "OK" : "FAILED", again);
            free(rs);
            free(state);
            free(text);
            free(lp);
            free(gd);
        }
        /* the runs to repeat go first */
        {
            size_t nr = 0;
            size_t *tmp = calloc(nn + 1, sizeof *tmp);

            if (!tmp) {
                free(list);
                free(next);
                goto out;
            }
            for (k = 0; k < nn; k++)
                if (runs[next[k]].attempts)
                    tmp[nr++] = next[k];
            for (k = 0; k < nn; k++)
                if (!runs[next[k]].attempts)
                    tmp[nr++] = next[k];
            memcpy(queue, tmp, nr * sizeof *queue);
            nq = nr;
            free(tmp);
        }
        free(list);
        free(next);
        fflush(log);
    }

    /* ------------------------------------------------------ the report */
    for (unsigned w = 0; w < nsess; w++) {
        struct session *s = &ss[w];

        text_add(&rep, "== session s%03u: %s\n%s== gates %s\n", s->no,
                 s->events.p ? s->events.p : "", s->gate.p ? s->gate.p : "",
                 s->gates_ok ? "OK" : "FAILED");
        all_ok &= s->gates_ok;
    }
    {
        size_t npairs = 0, nsame = 0, nok = 0;

        for (i = 0; i < ntests; i++) {
            struct run *tr[TEDAB_NCOMBOS] = { 0 };
            int ok[TEDAB_NCOMBOS] = { 0 }, any = 0;
            struct text exp = { 0 };
            char buf[300];

            for (k = 0; k < nruns; k++)
                if (runs[k].r.test == &tests[i])
                    tr[runs[k].r.combo] = &runs[k];
            text_add(&rep, "%s:", tests[i].name);
            for (c = 0; c < ncombo_order; c++) {
                int cc = combo_order[c];
                struct run *r = tr[cc];
                const char *st;

                if (!r)
                    continue;
                st = state_text(r, buf, sizeof buf);
                if (!strcmp(st, "OK")) {
                    char *staged = str_printf("%s/staged/%s/%s", P.work, tests[i].name,
                                              tests[i].file ? tests[i].file : "");
                    struct tedab_sources src = { tedab_combo_names[cc], staged, P.expect_dir,
                                                 P.corpus_out, P.cd, P.oracle };

                    if (tedab_check_expects(&tests[i], r->dir, &src, &exp))
                        st = "FAIL (expectations)";     /* a result OK alone is no pass */
                    else
                        ok[cc] = 1, nok++;
                    free(staged);
                }
                text_add(&rep, "%s %s %s", any++ ? "," : "", tedab_combo_names[cc], st);
                if (!ok[cc])
                    all_ok = 0;
            }
            text_add(&rep, "\n%s", exp.p ? exp.p : "");
            free(exp.p);
            for (k = 0; k < 4; k++) {
                int a = tedab_pairs[k][0], b = tedab_pairs[k][1], v;
                struct text d = { 0 };

                if (!tr[a] || !tr[b])
                    continue;
                npairs++;
                v = tedab_compare_runs(tr[a]->dir, tr[b]->dir, tedab_combo_names[a],
                                       tedab_combo_names[b], &fonts, tests[i].font, &d);
                text_add(&rep, "  %s<->%s: %s\n%s", tedab_combo_names[a], tedab_combo_names[b],
                         v < 0 ? "FAIL" : v ? "DIFF" : "SAME", d.p ? d.p : "");
                free(d.p);
                if (v)
                    all_ok = 0;
                else
                    nsame++;
            }
            if (P.bugorig)
                bugorig_verdicts(&tests[i], tr, &fonts, &rep, &worst);
        }
        for (i = 0; i < nknown; i++)
            text_add(&rep, "%s: KNOWN %s\n", known[i].name, known[i].known);
        text_add(&rep, "== %zu tests, %zu runs (%zu OK), %u sessions, %zu comparisons "
                       "(%zu SAME), %zu known\n", ntests, nruns, nok, nsess, npairs, nsame,
                 nknown);
    }
    if (rep.bad || write_file(P.report, rep.p, rep.n)) {
        fprintf(log, "tedab: cannot write %s\n", P.report);
        goto out;
    }
    fwrite(rep.p, 1, rep.n, log);
    rc = P.bugorig ? (worst >= 0 ? worst : 2) : all_ok ? 0 : 1;
out:
    for (unsigned w = 0; w < nsess; w++) {
        if (ss[w].e) {
            amiga_emulation_join(ss[w].e);
            amiga_emulation_free(ss[w].e);
        }
        free(ss[w].gate.p);
        free(ss[w].events.p);
    }
    free(ss);
    free(queue);
    free(rep.p);
    return rc;
}

int main(int argc, char **argv)
{
    char err[300];
    size_t len;
    char *text;
    int lint = argc == 3 && !strcmp(argv[1], "--lint"), bad;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        printf("usage: tedab [--lint] PASSFILE\n"
               "  the TED A/B tests in Amiberry (see main_tedab.c for the pass file);\n"
               "  --lint only checks the scripts and their sources\n");
        return 0;
    }
    if (argc != 2 && !lint) {
        fprintf(stderr, "usage: tedab [--lint] PASSFILE\n");
        return 2;
    }
    if (!(text = (char *)read_file(argv[argc - 1], &len))) {
        fprintf(stderr, "tedab: cannot read %s\n", argv[argc - 1]);
        return 2;
    }
    if (parse(text, err, sizeof err)) {
        fprintf(stderr, "tedab: %s: %s\n", argv[argc - 1], err);
        return 2;
    }
    free(text);
    bad = load_tests(stdout);
    if (lint) {
        /* the sources of the T and E/S tests must be there (K: the CD) */
        size_t i;

        for (i = 0; i < ntests; i++) {
            const struct tedab_test *t = &tests[i];
            char *p = t->src_kind == 'T' ? str_printf("%s/%s.bb2", P.corpus_src, t->src_arg)
                      : t->src_kind == 'E' ? str_printf("%s/%s.bas", P.src, t->src_arg)
                      : t->src_kind == 'S' ? str_printf("%s/%s.bb2", P.src, t->src_arg) : NULL;

            if (p && access(p, R_OK)) {
                printf("%s: no %s\n", t->name, p);
                bad++;
            }
            free(p);
            for (size_t k = 0; k < t->nexpects; k++)
                if (!strcmp(t->expects[k].kind, "tok")) {
                    char *q = str_printf("%s/%s.bb2", P.expect_dir, t->expects[k].b);

                    if (q && access(q, R_OK)) {
                        printf("%s: no %s\n", t->name, q);
                        bad++;
                    }
                    free(q);
                }
        }
        printf("tedab: %zu tests, %d problems\n", ntests, bad);
        return bad ? 1 : 0;
    }
    if (bad) {
        printf("tedab: %d problems in the scripts\n", bad);
        return 2;
    }
    return run(stdout);
}
