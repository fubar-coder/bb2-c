/*
 * tedab run, tedab lint - the TED A/B tests in Amiberry (bb2
 * ted-c/tools/tedab.py), see tedab.h for the test scripts.
 *
 *   tedab run PASSFILE    run the tests, report
 *   tedab lint PASSFILE   only check the scripts and their sources
 *
 * The pass file (written by CMake), lines KEY=VALUE, '#' comments:
 *   work=DIR           work directory (emptied first)
 *   report=PATH        the report (also to stdout)
 *   scripts=DIR        the test scripts (*.drv; gate-*.drv are gate D's)
 *   filter=GLOB,...    only these tests (names without .drv; default all)
 *   combos=XX,...      the combinations, the reference first (default
 *                      R5,C5,Cc,Rc; P5 and Pc are for the prev build)
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
 *   ted_prev=, blitz2_prev=PATH   P and c of the previous commit (the
 *                      combinations P5, Pc; needed only when combos= names
 *                      them: P5 needs ted_prev, Pc both)
 *   intended=ENTRY     a PREV_INTENDED entry (ted_ab:CASE[:PAIR], or ON; any
 *                      number; entries of other tests are ignored): the
 *                      pairs C5-P5 and Cc-Pc of that case may differ, of all
 *                      cases with ON; a listed case must differ in one (else
 *                      STALE and the pass fails), an unlisted difference
 *                      fails
 *   timeout=SEC        per emulator session (default 120)
 *   budget=SEC         estimated run time (@est) per session (default 60:
 *                      boot ~15 s and gates ~10 s come on top)
 *   parallel=N         emulator sessions at the same time
 *   stack=N            Stack for teddrive (default 40000)
 *   gates=DIR          gate-d.drv and gate-d-rescue.drv (default: scripts)
 *   inject=, wbrun=PATH  optional: tools/inject, tools/wbrun for @command
 *                      (tests/bug-orig/amiga)
 *   bugorig=1          the bug-orig mode (tests/bug-orig/ted): every run in
 *                      a session of its own; each combination but the
 *                      first (the reference) gets a verdict against the
 *                      recorded original (orig=, tedab_bugorig_verdict);
 *                      exit status 0 = all FIXED, 1 = REPRODUCED, 3 =
 *                      BROKEN, 4 = DIFFERS (the worst of them: 3 before 4
 *                      before 1), 2 = error
 *   orig=PATH          with bugorig=1: the recorded observation of the
 *                      reference run (tedab.h, tedab_orig_format).  The
 *                      normal run starts no session of the reference; the
 *                      status is 3 (BROKEN) before any session if the file
 *                      says OK without problems, 2 if the file is missing.
 *                      With BUGORIG_RECORD=1 in the environment only the
 *                      reference runs and its observation is written to
 *                      PATH (exit status 0)
 *   orig_by=TEXT       the note in the first line of the file
 *
 * The runs are packed into sessions (the combinations of a test stay
 * together); the next session starts as soon as one has ended (up to
 * parallel=), so a long session holds back no others.  A session that ends
 * early (hang: time limit) leaves runs
 * that were not reached; they go into a later session once more if their
 * test did not start there or its first run is OK.  The rest stays
 * "NOTRUN".  Each session's directory: work/sNNN/vol (the volume TEDAB:)
 * and work/sNNN/emu (Amiberry); the dumps get a decoded NAME.txt.
 * Tests with @known are not run (unless the filter names them exactly); the
 * report lists them as KNOWN with their reason, they do not fail the run.
 *
 * A pair a script names in @fixed #NNN must differ (FIXED); the same is
 * BROKEN, a failure (the fix does not show: a stale directive).
 *
 * Exit status 0 = all gates and runs OK and every comparison the same (a
 * pair of @fixed: different), 1 = not, 77 = the emulator or a file is
 * missing (skipped), 2 = error.
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup */
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "cmds.h"
#include "statline_emulation.h"
#include "tedab.h"
#include "util.h"

#define MAXSESSIONS 200

static const char *me;          /* "tedab run" or "tedab lint", the prefix of the messages */

struct pass {
    char *work, *report, *scripts, *filter, *combos, *src, *corpus_src, *corpus_out;
    char *expect_dir, *cd, *oracle, *fonts;
    char *teddrive, *teddrivetest, *tedprobe, *reqtrap, *ted_ref, *ted_c, *blitz2_ref, *blitz2_c;
    char *ted_prev, *blitz2_prev;       /* the previous commit: P5, Pc */
    char *inject, *wbrun, *gates;
    char *orig, *orig_by;
    struct prevint intended;            /* intended=: PREV_INTENDED (previntended.h) */
    unsigned timeout, budget, parallel, stack, bugorig;
};

/* one run over all sessions */
struct run {
    struct tedab_run r;
    int state, attempts, done, gates_ok;  /* gates_ok: of its last session */
    unsigned session;           /* the last one it was in */
    int halted;                 /* that session ended because the CPU halted */
    char *dir;                  /* its run directory (work/sNNN/vol/runs/T/XX) */
    const char *verdict;        /* bug-orig: its verdict ("reference" for the reference) */
};

static struct pass P;
static struct tedab_test *tests;
static size_t ntests;
static struct tedab_test *known;  /* @known, not run */
static size_t nknown;
static struct run *runs;
static size_t nruns;
static int combo_order[TEDAB_NCOMBOS], ncombo_order;
static int record;                      /* the recording run (BUGORIG_RECORD=1) */

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
        { "ted_prev", &P.ted_prev }, { "blitz2_prev", &P.blitz2_prev },
        { "inject", &P.inject }, { "wbrun", &P.wbrun }, { "gates", &P.gates },
        { "orig", &P.orig }, { "orig_by", &P.orig_by },
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
        if (!found && !strcmp(key, "intended")) {
            char msg[300];
            int r = prevint_add(&P.intended, val, msg, sizeof msg);

            free(val);
            if (r) {
                snprintf(err, errlen, "line %u: intended=%.200s", no,
                         r < 0 ? msg : "ted_ab:CASE[:PAIR] or ON (a library path is not for ted_ab)");
                return -1;
            }
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
            if ((k == TEDAB_P5 || k == TEDAB_PC) && !P.ted_prev) {
                snprintf(err, errlen, "combos: %s needs ted_prev=", w);
                free(c);
                return -1;
            }
            if (k == TEDAB_PC && !P.blitz2_prev) {
                snprintf(err, errlen, "combos: Pc needs blitz2_prev=");
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
    /* the bug-orig mode is judged against the recorded original (orig=);
       its reference, the first combination, only runs for the recording
       (BUGORIG_RECORD=1) */
    if (P.bugorig && !P.orig) {
        snprintf(err, errlen, "bugorig=1 needs orig=, the recorded observation of the original");
        return -1;
    }
    if (P.orig && !P.bugorig) {
        snprintf(err, errlen, "orig= belongs to bugorig=1");
        return -1;
    }
    if (P.orig && ncombo_order < 2) {
        snprintf(err, errlen, "orig=: the reference and another combination are needed");
        return -1;
    }
    if (P.orig) {
        const char *r = getenv("BUGORIG_RECORD");

        record = r && !strcmp(r, "1");
        if (record)
            ncombo_order = 1;
        else {
            memmove(combo_order, combo_order + 1, (size_t)(--ncombo_order) * sizeof *combo_order);
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
        fprintf(log, "%s: no %s\n", me, P.scripts);
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
            fprintf(log, "%s: %s: no %s on the CD %s\n", me, t->name, t->src_arg, P.cd);
            r = 77;
        }
        break;
    }
    if (!r && src && copy_to(src, dir, t->file)) {
        fprintf(log, "%s: %s: cannot copy %s\n", me, t->name, src);
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
                fprintf(log, "%s: %s: no %s (the corpus test makes it)\n", me, t->name,
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
            r = copy_to(c == TEDAB_R5 || c == TEDAB_C5 || c == TEDAB_P5 ? P.blitz2_ref
                        : c == TEDAB_PC ? P.blitz2_prev : P.blitz2_c, dir, "blitz2")
                || copy_to(c == TEDAB_R5 || c == TEDAB_RC ? P.ted_ref
                           : c == TEDAB_P5 || c == TEDAB_PC ? P.ted_prev : P.ted_c, dir, "ted")
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
        fprintf(log, "%s: cannot set up %s\n", me, vol ? vol : "the session");
    free(vol);
    return r;
}

/* ------------------------------------------------------------ sessions */

struct session {
    unsigned no;
    size_t *ids, n;             /* its runs: runs[ids[0..n)] */
    char *script;               /* until it has started */
    struct amiga_emulation *e;
    int gates_ok;
    struct text gate, events;
    struct statline stat;       /* its @stat line (statline.h); test[0]: joined */
    size_t run;                 /* its first run (runs[]) */
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
        /* started, no end: the session ended at its limit, or earlier
           because the emulated CPU halted */
        if (r->halted)
            snprintf(buf, len, res && *res ? "HALT (%s)" : "HALT", res);
        else
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

/* bug-orig: the recorded original (orig=, loaded by load_orig) */
static char *orig_state, *orig_problems;

/* bug-orig: the verdict of every combination against the recorded original
   into the report; *worst = the worst exit status */
static void bugorig_verdicts(const struct tedab_test *t, struct run **tr, struct text *rep,
                             int *worst)
{
    int c;

    for (c = 0; c < ncombo_order; c++) {
        int cc = combo_order[c], v;
        char ps[300];
        struct text pp = { 0 };

        if (!tr[cc])
            continue;
        observe(t, tr[cc], ps, sizeof ps, &pp);
        if (!strncmp(ps, "NOTRUN", 6) || !strncmp(ps, "SKIPPED", 7) || !tr[cc]->gates_ok)
            v = 2;              /* not run (or the tools failed): no verdict */
        else
            /* the recorded state and problems are all of the original there
               is: no comparison of the whole run directories */
            v = tedab_bugorig_verdict(orig_state, orig_problems, ps, pp.p ? pp.p : "", 0);
        tr[cc]->verdict = v == 2 ? "NOTRUN" : tedab_verdict_name(v);
        text_add(rep, "  bug-orig %s against the recorded original: %s\n", tedab_combo_names[cc],
                 v == 2 ? "NOT RUN" : tedab_verdict_name(v));
        if (*worst < 0 || verdict_rank(v) > verdict_rank(*worst))
            *worst = v;
        free(pp.p);
    }
}

/* the recording run: the observation of the reference run into orig=;
   *worst = 2 if it cannot be made */
static void record_orig(const struct tedab_test *t, struct run **tr, struct text *rep, int *worst)
{
    int ref = combo_order[0];
    char rs[300], err[200];
    struct text rp = { 0 }, f;

    if (!tr[ref] || !tr[ref]->gates_ok) {
        text_add(rep, "  bug-orig: no run of the reference %s\n", tedab_combo_names[ref]);
        *worst = 2;
        return;
    }
    observe(t, tr[ref], rs, sizeof rs, &rp);
    if (!strncmp(rs, "NOTRUN", 6) || !strncmp(rs, "SKIPPED", 7)) {
        text_add(rep, "  bug-orig: the reference %s did not run\n", tedab_combo_names[ref]);
        *worst = 2;
    } else if (rp.bad || tedab_orig_format(rs, rp.p ? rp.p : "", P.orig_by, &f, err, sizeof err)) {
        text_add(rep, "  bug-orig: %s\n", rp.bad ? "out of memory" : err);
        *worst = 2;
    } else {
        if (write_file(P.orig, f.p, f.n)) {
            text_add(rep, "  bug-orig: cannot write %s\n", P.orig);
            *worst = 2;
        } else {
            text_add(rep, "  bug-orig %s: recorded in %s (state %s)\n", tedab_combo_names[ref],
                     P.orig, rs);
            if (*worst < 0)
                *worst = 0;
        }
        free(f.p);
    }
    tr[ref]->verdict = "recorded";
    free(rp.p);
}

/* the recorded original of a bug-orig pass: 0 = loaded, 3 = BROKEN (the
   original shows the expected behaviour: OK without a problem; the test
   would not show a bug), 2 = error (the file is missing: it is recorded
   by BUGORIG_RECORD=1 or ctest bug_orig_record_...) */
static int load_orig(FILE *log)
{
    size_t len;
    char err[200], *text = (char *)read_file(P.orig, &len);

    if (!text) {
        fprintf(log, "%s: no %s, the recorded observation of the original: record it with "
                "BUGORIG_RECORD=1 (ctest -L originals -R bug_orig_record_...)\n", me, P.orig);
        return 2;
    }
    if (tedab_orig_parse(text, &orig_state, &orig_problems, err, sizeof err)) {
        fprintf(log, "%s: %s: %s\n", me, P.orig, err);
        free(text);
        return 2;
    }
    free(text);
    if (!strcmp(orig_state, "OK") && !*orig_problems) {
        fprintf(log, "BROKEN TEST: the original shows the expected (correct) behaviour (%s) - "
                "it does not have the bug, or the expectation describes the bug\n", P.orig);
        return 3;
    }
    return 0;
}

static int run(FILE *log)
{
    /* the original TED only runs for the recording */
    const char *files[] = { P.teddrive, P.teddrivetest, P.tedprobe, P.reqtrap,
                            P.orig && !record ? P.ted_c : P.ted_ref, P.ted_c,
                            P.blitz2_ref, P.blitz2_c, P.oracle, P.ted_prev, P.blitz2_prev };
    struct session *ss = calloc(MAXSESSIONS + 1, sizeof *ss);
    size_t *queue = NULL, nq = 0, i, k;
    struct teddump_fonts fonts;
    unsigned nsess = 0, running = 0;     /* running: sessions started, not evaluated */
    struct session *pending = NULL;     /* staged, waiting for a slot */
    struct amiga_emulation **act = NULL; /* act[no - 1]: the running sessions */
    int told = 0;
    struct text rep = { 0 }, pdiff = { 0 };     /* pdiff: PREV_INTENDED entries of the differences */
    int rc = 2, all_ok = 1, c, worst = -1;

    /* the recorded original against the expectations, before any session:
       the test shows no bug (BROKEN), or the recording is missing */
    if (P.orig && !record && (rc = load_orig(log)))
        return rc;
    for (i = 0; i < sizeof files / sizeof files[0]; i++)
        if (files[i] && access(files[i], R_OK)) {
            fprintf(log, "%s: no %s - skipped\n", me, files[i]);
            return 77;
        }
    if (access(P.cd, R_OK | X_OK)) {
        fprintf(log, "%s: no CD %s - skipped\n", me, P.cd);
        return 77;
    }
    if (teddump_load_fonts(P.fonts, &fonts)) {
        fprintf(log, "%s: no fonts in %s\n", me, P.fonts);
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
            fprintf(log, "%s: %s needs ~%u s > budget %u s: shorten the test\n", me,
                    tests[i].name, cost, P.budget);
            goto out;
        }
    }
    fprintf(log, "%s: %zu tests, %zu runs, at most %u sessions at the same time\n", me, ntests,
            nruns, P.parallel);

    /* up to `parallel` sessions at a time: the next one is made from the
       front of the queue as soon as one ends, and a session that has
       ended is evaluated at once; its runs to repeat go to the front of the
       queue.  No free emulator slot while sessions run: the staged session
       is started again every AMIGA_EMULATION_SLOT_RETRY_MS (the slot limit
       is hard) */
    act = calloc(MAXSESSIONS + 1, sizeof *act);
    if (!act)
        goto out;
    while (running || pending || (nq && nsess < MAXSESSIONS)) {
        struct session *s;
        int ended;

        if (!pending && running < P.parallel && nq && nsess < MAXSESSIONS) {
            struct tedab_run *rs;
            char *script, *sp;
            size_t pos = 0, m = take(queue, nq, &pos);

            s = &ss[nsess];
            s->no = ++nsess;
            s->n = m;
            s->ids = malloc(m * sizeof *s->ids);
            rs = calloc(m, sizeof *rs);
            if (!s->ids || !rs) {
                free(rs);
                goto out;
            }
            memcpy(s->ids, queue, m * sizeof *s->ids);
            memmove(queue, queue + m, (nq - m) * sizeof *queue);
            nq -= m;
            for (k = 0; k < m; k++)
                rs[k] = runs[s->ids[k]].r;
            if (stage_session(s->no, rs, m, log)) {
                free(rs);
                goto out;
            }
            script = tedab_session_script(rs, m, P.stack);
            free(rs);
            if (!script || tedab_check_script(script, log)) {
                free(script);
                goto out;
            }
            sp = session_dir(s->no, "vol/session.script");
            if (sp)
                write_file(sp, script, strlen(script));
            free(sp);
            s->script = script;
            pending = s;
            told = 0;
        }
        if (pending) {
            char *vol = session_dir(pending->no, "vol"), *emu = session_dir(pending->no, "emu"),
                 err[300] = "";

            s = pending;
            if (vol && emu && !mkdir_p(emu)) {
                struct amiga_emulation_mount mnt = { TEDAB_VOL, vol, 1 };
                struct amiga_emulation_params ep = { 0 };

                ep.mounts = &mnt;
                ep.nmounts = 1;
                ep.script = s->script;
                ep.timeout = P.timeout;
                ep.work_dir = emu;
                s->e = amiga_emulation_start(&ep, err, sizeof err);
            }
            free(vol);
            free(emu);
            if (!s->e && running && amiga_emulation_no_slot()) {
                if (!told++)
                    fprintf(log, "%s: session %03u: %s - waiting for a slot\n", me, s->no, err);
            } else if (!s->e) {
                fprintf(log, "%s: %s - skipped\n", me, err);
                rc = 77;
                goto out;
            } else {
                for (k = 0; k < s->n; k++) {
                    struct run *r = &runs[s->ids[k]];

                    r->session = s->no;
                    r->attempts++;
                    free(r->dir);
                    r->dir = str_printf("%s/s%03u/vol/runs/%s/%s", P.work, s->no,
                                        r->r.test->name, tedab_combo_names[r->r.combo]);
                }
                fprintf(log, "%s: session %03u: %zu runs\n", me, s->no, s->n);
                fflush(log);
                free(s->script);
                s->script = NULL;
                act[s->no - 1] = s->e;
                pending = NULL;
                running++;
                continue;
            }
        }
        /* the next session that ends; none within the time: start again */
        if ((ended = amiga_emulation_wait_any(act, nsess, AMIGA_EMULATION_SLOT_RETRY_MS)) < 0)
            continue;
        s = &ss[ended];
        act[ended] = NULL;
        running--;
        {
            enum amiga_emulation_status st = amiga_emulation_join(s->e);
            char *lp = session_dir(s->no, "vol/session.log"), *gd = session_dir(s->no,
                                                                                "vol/gate/out");
            size_t len, again = 0, nn = 0;
            size_t *next = calloc(s->n, sizeof *next);
            char *text = lp ? (char *)read_file(lp, &len) : NULL;
            struct tedab_run *rs = calloc(s->n, sizeof *rs);
            int *state = calloc(s->n, sizeof *state), bad = !next || !rs || !state;
            char sname[16];

            /* the session's @stat line: the combination of its run in the
               bug-orig mode, else the session */
            snprintf(sname, sizeof sname, "s%03u", s->no);
            s->run = s->ids[0];
            statline_session(&s->stat, P.work,
                             P.bugorig ? tedab_combo_names[runs[s->run].r.combo] : sname, s->e);
            s->stat.runs = (int)s->n;
            amiga_emulation_free(s->e);
            s->e = NULL;
            for (k = 0; rs && k < s->n; k++)
                rs[k] = runs[s->ids[k]].r;
            if (rs && state)
                tedab_session_log(text, rs, s->n, state, &s->events);
            s->gates_ok = gd ? tedab_eval_gate(gd, &s->gate) : 0;
            for (k = 0; !bad && k < s->n; k++) {
                struct run *r = &runs[s->ids[k]];
                size_t f, d;
                int requeue = 0;

                r->state = state[k];
                r->gates_ok = s->gates_ok;
                r->halted = st == AMIGA_EMULATION_HALT;
                /* the run hit #340 (TED's a0toname, depends on the memory
                   layout), or was skipped because the run it waited for did
                   (tedab_session_waits_for): again, first in a new session.
                   Not with @command: teddrive takes the last word of any
                   command for the file argument */
                for (d = k; d && state[d] == TEDAB_SKIPPED; d = tedab_session_waits_for(rs, d))
                    ;
                if ((hit340(r) || (r->state == TEDAB_SKIPPED && hit340(&runs[s->ids[d]])))
                    && !r->r.test->command && r->attempts < 3) {
                    fprintf(log, "%s: %s %s: #340 (a0toname), again\n", me, r->r.test->name,
                            tedab_combo_names[r->r.combo]);
                    next[nn++] = s->ids[k];
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
                    char *ok = str_printf("%s/out/ok", runs[s->ids[f]].dir);

                    requeue = state[f] == TEDAB_NOTRUN || (ok && !access(ok, R_OK));
                    free(ok);
                }
                if (requeue && r->attempts < 2) {
                    next[nn++] = s->ids[k];
                    again++;
                } else
                    r->done = 1;
            }
            /* outside bug-orig: ok if the gates and every run are */
            if (!P.bugorig) {
                int ok = s->gates_ok && !bad;
                char buf[300];

                for (k = 0; ok && k < s->n; k++)
                    ok = !strcmp(state_text(&runs[s->ids[k]], buf, sizeof buf), "OK");
                statline_set(s->stat.verdict, sizeof s->stat.verdict, ok ? "ok" : "fail");
            }
            fprintf(log, "%s: session %03u: %s, gates %s, %zu runs again\n", me, s->no,
                    amiga_emulation_status_name(st), s->gates_ok ? "OK" : "FAILED", again);
            /* the runs to repeat go first */
            memmove(queue + nn, queue, nq * sizeof *queue);
            if (nn)
                memcpy(queue, next, nn * sizeof *queue);
            nq += nn;
            free(next);
            free(rs);
            free(state);
            free(text);
            free(lp);
            free(gd);
            if (bad)
                goto out;
        }
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
        size_t npairs = 0, nsame = 0, nfixed = 0, nok = 0, nintended = 0;

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
            for (k = 0; k < TEDAB_NPAIRS; k++) {
                int a = tedab_pairs[k][0], b = tedab_pairs[k][1], v;
                struct text d = { 0 };

                if (!tr[a] || !tr[b])
                    continue;
                npairs++;
                v = tedab_pair_verdict(&tests[i], k,
                                       tedab_compare_runs(tr[a]->dir, tr[b]->dir,
                                                          tedab_combo_names[a],
                                                          tedab_combo_names[b], &fonts,
                                                          tests[i].font, &d));
                v = tedab_pair_prev(&P.intended, &tests[i], k, v);
                if (v == TEDAB_PAIR_INTENDED || (b >= TEDAB_P5 && v == TEDAB_PAIR_DIFF))
                    text_add(&pdiff, "%sted_ab:%s:%s-%s", pdiff.n ? ";" : "", tests[i].name,
                             tedab_combo_names[a], tedab_combo_names[b]);
                text_add(&rep, "  %s<->%s: %s", tedab_combo_names[a], tedab_combo_names[b],
                         tedab_pair_verdict_name(v));
                if (v == TEDAB_PAIR_FIXED)
                    text_add(&rep, " #%u (differs)", tests[i].fixed[k]);
                else if (v == TEDAB_PAIR_BROKEN)
                    text_add(&rep, " (@fixed #%u: the pair does not differ, stale)",
                             tests[i].fixed[k]);
                text_add(&rep, "\n%s", d.p ? d.p : "");
                free(d.p);
                if (!tedab_pair_passes(v))
                    all_ok = 0;
                else if (v == TEDAB_PAIR_FIXED)
                    nfixed++;
                else if (v == TEDAB_PAIR_INTENDED)
                    nintended++;
                else
                    nsame++;
            }
            if (record)
                record_orig(&tests[i], tr, &rep, &worst);
            else if (P.bugorig)
                bugorig_verdicts(&tests[i], tr, &rep, &worst);
        }
        for (i = 0; i < nknown; i++)
            text_add(&rep, "%s: KNOWN %s\n", known[i].name, known[i].known);
        /* PREV_INTENDED: a listed case must differ (not judged: only if the
           run was filtered) */
        if (!P.bugorig) {
            size_t cur = 0;
            const char *e;

            while ((e = prevint_stale(&P.intended, "ted_ab", P.filter && *P.filter, &cur)) != NULL) {
                text_add(&rep, "STALE %s: intended, but prev does not differ in it\n", e);
                all_ok = 0;
            }
        }
        if (pdiff.n)
            text_add(&rep, "prev differs in: PREV_INTENDED=\"%s\"\n", pdiff.p);
        text_add(&rep, "== %zu tests, %zu runs (%zu OK), %u sessions, %zu comparisons "
                       "(%zu SAME, %zu FIXED, %zu INTENDED), %zu known\n", ntests, nruns, nok,
                 nsess, npairs, nsame, nfixed, nintended, nknown);
    }
    if (rep.bad || write_file(P.report, rep.p, rep.n)) {
        fprintf(log, "%s: cannot write %s\n", me, P.report);
        goto out;
    }
    fwrite(rep.p, 1, rep.n, log);
    rc = P.bugorig ? (worst >= 0 ? worst : 2) : all_ok ? 0 : 1;
out:
    /* the sessions' @stat lines, after the report; bug-orig: the verdict of
       the run in the session that decided it (its last one) */
    for (unsigned w = 0; w < nsess; w++) {
        struct session *s = &ss[w];

        if (!s->stat.test[0])
            continue;
        if (P.bugorig && runs && runs[s->run].session == s->no && runs[s->run].verdict)
            statline_set(s->stat.verdict, sizeof s->stat.verdict, runs[s->run].verdict);
        statline_write(log, &s->stat);
    }
    for (unsigned w = 0; w < nsess; w++) {
        if (ss[w].e) {
            amiga_emulation_join(ss[w].e);
            amiga_emulation_free(ss[w].e);
        }
        free(ss[w].gate.p);
        free(ss[w].events.p);
        free(ss[w].ids);
        free(ss[w].script);
    }
    free(ss);
    free(act);
    free(queue);
    free(rep.p);
    free(pdiff.p);
    prevint_free(&P.intended);
    return rc;
}

const char cmd_run_usage[] =
    "usage: tedab run PASSFILE\n"
    "Run the TED A/B tests of the pass file PASSFILE in Amiberry and write\n"
    "the report (the keys of the pass file: the header of\n"
    "tests/ted-ab/runner/cmd_run.c).  With\n"
    "BUGORIG_RECORD=1 in the environment a bug-orig pass (bugorig=1, orig=)\n"
    "records the observation of the original instead.\n";

const char cmd_lint_usage[] =
    "usage: tedab lint PASSFILE\n"
    "Only check the test scripts of the pass file PASSFILE and that their\n"
    "sources are there; no emulator.\n";

/* run and lint: the pass file, the scripts, then the run or the lint */
static int start(int argc, char **argv, int lint)
{
    char err[300];
    size_t len;
    char *text;
    int bad;

    me = argv[0];
    if (argc != 2) {
        fputs(lint ? cmd_lint_usage : cmd_run_usage, stderr);
        return 2;
    }
    if (!(text = (char *)read_file(argv[1], &len))) {
        fprintf(stderr, "%s: cannot read %s\n", me, argv[1]);
        return 2;
    }
    if (parse(text, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[1], err);
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
        printf("%s: %zu tests, %d problems\n", me, ntests, bad);
        return bad ? 1 : 0;
    }
    if (bad) {
        printf("%s: %d problems in the scripts\n", me, bad);
        return 2;
    }
    return run(stdout);
}

int cmd_run(int argc, char **argv)
{
    return start(argc, argv, 0);
}

int cmd_lint(int argc, char **argv)
{
    return start(argc, argv, 1);
}
