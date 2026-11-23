/*
 * test_caserun.c - the logic of corpusrun case (caserun.h) without the
 * emulator, on configuration trees written to a work directory: the pass
 * of a cell (the compiler and libraries of each value, the test library,
 * the run settings, the files a selector restricts to some cells, the RWE
 * user libraries, pad), the session keys that group the cells, the split
 * of a program's output, the compile result in the terms of the source,
 * and the printed configuration of a three-level hierarchy with the origin
 * of every key - and the refusals (deliberately broken input: a runner
 * value the environment lacks, pad with a runwrap, an RWE library the
 * environment does not know, a malformed files entry and selector) - and
 * the passes of the comparison with the prev build (cr_prev_pass: a
 * compile without the settings of a run, with the configuration prev and
 * the entries of its name; cr_prev_unknown: an entry naming no compile of
 * the test) with broken input (a cell pass without a configuration, with
 * two, an entry of another test, of another name, ON).
 *   test_caserun WORKDIR
 */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "caserun.h"
#include "corpus.h"
#include "util.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static char work[1024];

static void put(const char *rel, const char *text)
{
    char *p = str_printf("%s/%s", work, rel), *slash = p ? strrchr(p, '/') : NULL;

    if (slash) {
        *slash = 0;
        mkdir_p(p);
        *slash = '/';
    }
    CHECK(p && write_file(p, text, strlen(text)) == 0);
    free(p);
}

#define ENV_RUNNER "[runner]\nblitz2 = \"/b/blitz2\"\norig195 = \"/b/ref/blitz2\"\norig210 = \"/cd/BLITZ2\"\n" \
    "cd-deflibs = \"/cd/DEFLIBS\"\ncd-acidlibs = \"/cd/ACIDLIBS\"\ncd-defaultdbug = \"/cd/DEFAULTDBUG\"\n" \
    "clib-deflibs = \"/b/deflibs\"\nclib-acidlibs = \"/b/acidlibs\"\nted = \"/b/tedstub\"\n" \
    "launcher = \"/b/reqtrap\"\nrcput = \"/b/rcput\"\ntestlib = \"/b/testlib.obj\"\ntimeout = \"90\"\n" \
    "run-hang-after = \"8\"\ninject = \"/b/inject\"\nrwe-rixlib = \"/v/rixlib.obj|/cd/RIXLIB.OBJ|/v\"\n"

struct t {
    struct tc_env env;
    struct tc_config conf;
    struct tc_matrix m;
    int ok;
};

/* the test SRC (relative to work) with the environment's runner table runner */
static void load(struct t *t, const char *src, const char *runner)
{
    char err[600], *text = str_printf("top = \"%s\"\n[placeholders]\nTESTDATA = \"/td\"\n%s", work, runner);
    char *path = str_printf("%s/%s", work, src);

    memset(t, 0, sizeof *t);
    t->ok = text && path && tc_env_parse("env.toml", text, &t->env, err, sizeof err) == 0
            && tc_resolve(path, &t->env, &t->conf, err, sizeof err) == 0
            && tc_matrix_build(&t->conf, &t->m, err, sizeof err) == 0;
    if (!t->ok)
        fprintf(stderr, "load %s: %s\n", src, err);
    CHECK(t->ok);
    free(text);
    free(path);
}

static void unload(struct t *t)
{
    tc_matrix_free(&t->m);
    tc_free(&t->conf);
    tc_env_free(&t->env);
}

/* the cell named name */
static size_t cell(const struct t *t, const char *name)
{
    for (size_t i = 0; i < t->m.ncells; i++)
        if (!strcmp(t->m.cells[i].name, name))
            return i;
    fprintf(stderr, "no cell %s\n", name);
    failures++;
    return 0;
}

/* the pass of a cell, checked by corpus_parse */
static char *pass(const struct t *t, const char *name, char *err, size_t errlen)
{
    char *text = NULL;

    if (cr_cell_pass(&t->conf, &t->m, cell(t, name), &t->env, "c0", "/w/run", "/w/run/test.bb2", &text, err,
                     errlen))
        return NULL;
    {
        struct corpus_pass p;
        char perr[400];

        if (corpus_parse(text, &p, perr, sizeof perr)) {
            fprintf(stderr, "the pass of %s does not parse: %s\n%s", name, perr, text);
            failures++;
        } else
            corpus_free(&p);
    }
    return text;
}

static int has(const char *text, const char *line)
{
    char *l = str_printf("\n%s\n", line), *t = str_printf("\n%s", text);
    int r = l && t && strstr(t, l) != NULL;

    if (!r)
        fprintf(stderr, "no line \"%s\" in:\n%s", line, text);
    free(l);
    free(t);
    return r;
}

static void test_cell_passes(void)
{
    struct t t;
    char err[600], *p;

    put("a/tests.toml", "root = true\ntimeout = 60\nexclude = [\"compiler=orig195,libs=clib\"]\n"
        "[matrix]\ncompiler = [\"port\", \"orig195\", \"orig210\"]\nlibs = [\"cd\", \"clib\"]\n");
    put("a/x.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("a/x.test.toml", "stack = 20000\nmachine = \"a600\"\nwrap = \"@TESTDATA@/w|x y\"\n"
        "files = [\"data/f|f.bin\", \"blitz2/x|/abs/x|libs=cd\", \"data/g|/abs/g|compiler=orig210,libs=cd\"]\n"
        "dirs = [\"d1\"]\nremove = [\"blitz2/ted\"]\nmount = [\"BB2|/cd\"]\nuae = [\"chipmem_size=4\"]\n"
        "jobline = [\"run=r002|CORPUS:src/x.bb2|RAM:con\"]\nrunargs = \"a b\"\nlauncherargs = \"-harden\"\n");
    load(&t, "a/x.test.bas", ENV_RUNNER);
    CHECK(t.m.ncells == 5);
    /* the port with the C libraries: the port side, no debugger module */
    p = pass(&t, "port,clib", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/b/blitz2|/b/deflibs|/b/acidlibs"));
    CHECK(p && has(p, "cell=on"));
    CHECK(p && has(p, "work=/w/run"));
    CHECK(p && has(p, "timeout=60"));
    CHECK(p && has(p, "runtime_errors=off"));
    CHECK(p && has(p, "source=/w/run/test.bb2"));
    CHECK(p && has(p, "file=blitz2/userlibs/testlib/testlib.obj|/b/testlib.obj"));
    CHECK(p && !strstr(p, "file=blitz2/testlib/"));
    CHECK(p && !strstr(p, "defaultdbug="));
    CHECK(p && has(p, "stack=20000"));
    CHECK(p && has(p, "machine=a600"));
    CHECK(p && has(p, "hang_after=8"));
    CHECK(p && has(p, "hang_from=run"));
    CHECK(p && has(p, "wrap=/td/w|x y"));
    CHECK(p && has(p, "runargs=a b"));
    CHECK(p && has(p, "launcherargs=-harden"));
    CHECK(p && has(p, "dir=d1"));
    CHECK(p && has(p, "remove=blitz2/ted"));
    CHECK(p && has(p, "mount=BB2|/cd"));
    CHECK(p && has(p, "uae=chipmem_size=4"));
    CHECK(p && has(p, "jobline=run=r002|CORPUS:src/x.bb2|RAM:con"));
    {
        char *f = str_printf("file=data/f|%s/a/f.bin", work);

        CHECK(p && f && has(p, f));
        free(f);
    }
    /* the files of other cells are left out */
    CHECK(p && !strstr(p, "file=blitz2/x|"));
    CHECK(p && !strstr(p, "file=data/g|"));
    free(p);
    /* the port with the CD's libraries: the originals side, the debugger module */
    p = pass(&t, "port,cd", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/b/blitz2|/cd/DEFLIBS|/cd/ACIDLIBS"));
    CHECK(p && has(p, "defaultdbug=/cd/DEFAULTDBUG"));
    CHECK(p && has(p, "file=blitz2/x|/abs/x"));
    CHECK(p && !strstr(p, "file=data/g|"));
    free(p);
    /* 1.95 loads the user library from blitz2:DIR/ too */
    p = pass(&t, "orig195,cd", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/b/ref/blitz2|/cd/DEFLIBS|/cd/ACIDLIBS"));
    CHECK(p && has(p, "file=blitz2/testlib/testlib.obj|/b/testlib.obj"));
    CHECK(p && has(p, "file=blitz2/userlibs/testlib/testlib.obj|/b/testlib.obj"));
    free(p);
    p = pass(&t, "orig210,cd", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/cd/BLITZ2|/cd/DEFLIBS|/cd/ACIDLIBS"));
    CHECK(p && has(p, "file=data/g|/abs/g"));
    CHECK(p && !strstr(p, "file=blitz2/testlib/"));
    free(p);
    p = pass(&t, "orig210,clib", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/cd/BLITZ2|/b/deflibs|/b/acidlibs"));
    CHECK(p && has(p, "defaultdbug=/cd/DEFAULTDBUG"));
    free(p);
    unload(&t);
}

static void test_cell_settings(void)
{
    struct t t;
    char err[600], *p;

    /* runtime error checking, rtg, serial, hang-after, pad */
    put("b/tests.toml", "root = true\n");
    put("b/y.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("b/y.test.toml", "rtg = \"on\"\nserial = \"on\"\npad = \"on\"\n"
        "[matrix]\nruntime-errors = [\"off\", \"on\"]\n");
    load(&t, "b/y.test.bas", ENV_RUNNER);
    p = pass(&t, "on", err, sizeof err);
    CHECK(p && has(p, "runtime_errors=on"));
    CHECK(p && has(p, "rtg=on"));
    CHECK(p && has(p, "serial=on"));
    CHECK(p && has(p, "timeout=90"));          /* the environment's default */
    CHECK(p && has(p, "runwrap=/b/inject|pad"));
    free(p);
    unload(&t);

    /* the RWE user libraries: the C build and its variant's libraries, the
       reference for 2.10 */
    put("c/tests.toml", "root = true\nrwe = [\"rixlib\"]\n[matrix]\ncompiler = [\"port\", \"orig210\"]\n"
        "libs = [\"cd\", \"clib\"]\n");
    put("c/z.test.bas", "!TestCase{c}\n!EndTestCase\n");
    load(&t, "c/z.test.bas", ENV_RUNNER);
    p = pass(&t, "port,clib", err, sizeof err);
    CHECK(p && has(p, "config=c0|ref|/b/blitz2|/v/deflibs|/v/acidlibs"));
    CHECK(p && has(p, "file=blitz2/userlibs/rwe/rixlib.obj|/v/rixlib.obj"));
    free(p);
    p = pass(&t, "orig210,cd", err, sizeof err);
    CHECK(p && has(p, "file=blitz2/userlibs/rwe/rixlib.obj|/cd/RIXLIB.OBJ"));
    free(p);
    unload(&t);
}

static void test_cell_refused(void)
{
    struct t t;
    char err[600], *p;

    /* a runner value the environment lacks */
    put("d/tests.toml", "root = true\n");
    put("d/r.test.bas", "!TestCase{c}\n!EndTestCase\n");
    load(&t, "d/r.test.bas", "[runner]\nblitz2 = \"/b/blitz2\"\n");
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "has no runner value clib-deflibs") != NULL);
    unload(&t);
    load(&t, "d/r.test.bas", "[runner]\n");
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "has no runner value blitz2") != NULL);
    unload(&t);

    /* pad with a runwrap of its own */
    put("d/s.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("d/s.test.toml", "pad = \"on\"\nrunwrap = \"/x/w\"\n");
    load(&t, "d/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "cannot have a runwrap too") != NULL);
    unload(&t);

    /* an RWE user library the environment does not know */
    put("d/u.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("d/u.test.toml", "rwe = [\"nolib\"]\n");
    load(&t, "d/u.test.bas", ENV_RUNNER);
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "no RWE user library nolib") != NULL);
    unload(&t);

    /* a files entry that is no DEST|SRC, a selector that is none */
    put("d/v.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("d/v.test.toml", "files = [\"|/abs/x\"]\n");
    load(&t, "d/v.test.bas", ENV_RUNNER);
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "is not DEST|SRC[|SELECTOR]") != NULL);
    unload(&t);
    put("d/v.test.toml", "files = [\"data/x|/abs/x|nokey=1\"]\n");
    load(&t, "d/v.test.bas", ENV_RUNNER);
    CHECK(cr_cell_pass(&t.conf, &t.m, 0, &t.env, "c0", "/w", "/w/t.bb2", &p, err, sizeof err) == -1);
    CHECK(strstr(err, "files: data/x|/abs/x|nokey=1:") != NULL);
    unload(&t);
}

static void test_sessions(void)
{
    struct t t;
    char *k[4];

    put("e/tests.toml", "root = true\n");
    put("e/s.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("e/s.test.toml", "[matrix]\nlibs = [\"cd\", \"clib\"]\nruntime-errors = [\"off\", \"on\"]\n");
    load(&t, "e/s.test.bas", ENV_RUNNER);
    CHECK(t.m.ncells == 4);
    for (size_t i = 0; i < 4; i++)
        k[i] = cr_session_key(&t.conf, &t.m, i);
    /* the libraries and runtime error checking do not decide the session */
    CHECK(k[0] && k[1] && k[2] && k[3] && !strcmp(k[0], k[1]) && !strcmp(k[0], k[2]) && !strcmp(k[0], k[3]));
    for (size_t i = 0; i < 4; i++)
        free(k[i]);
    unload(&t);
    /* the machine and the time limit do; a serial cell is alone */
    put("e/s.test.toml", "[matrix]\nmachine = [\"a1200\", \"a600\"]\ntimeout = [\"30\", \"60\"]\n");
    load(&t, "e/s.test.bas", ENV_RUNNER);
    CHECK(t.m.ncells == 4);
    for (size_t i = 0; i < 4; i++)
        k[i] = cr_session_key(&t.conf, &t.m, i);
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < i; j++)
            CHECK(strcmp(k[i], k[j]));
    for (size_t i = 0; i < 4; i++)
        free(k[i]);
    unload(&t);
    put("e/s.test.toml", "serial = \"on\"\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    load(&t, "e/s.test.bas", ENV_RUNNER);
    k[0] = cr_session_key(&t.conf, &t.m, 0);
    k[1] = cr_session_key(&t.conf, &t.m, 1);
    CHECK(k[0] && k[1] && strcmp(k[0], k[1]));
    free(k[0]);
    free(k[1]);
    unload(&t);
}

static void test_parse_run(void)
{
    struct cr_run r;

    CHECK(cr_parse_run("hello\nREQUESTER: AutoRequest body \"Array\" -> 1\nworld\r\nrc=5\n", &r) == 0);
    CHECK(!strcmp(r.output, "hello\nworld\n"));
    CHECK(r.nreq == 1 && !strcmp(r.req[0], "AutoRequest body \"Array\" -> 1"));
    CHECK(r.has_rc && r.rc == 5);
    cr_run_free(&r);
    /* rc= not on the last line is output; no rc at all */
    CHECK(cr_parse_run("rc=1\nmore\n", &r) == 0);
    CHECK(!strcmp(r.output, "rc=1\nmore\n") && !r.has_rc);
    cr_run_free(&r);
    CHECK(cr_parse_run("rc=x\n", &r) == 0);
    CHECK(!r.has_rc && !strcmp(r.output, "rc=x\n"));
    cr_run_free(&r);
    CHECK(cr_parse_run(NULL, &r) == 0);
    CHECK(!strcmp(r.output, "") && !r.has_rc && !r.nreq);
    cr_run_free(&r);
}

static void test_compile_mapped(void)
{
    struct tp_source src = { 0 };
    struct tp_params par = { 0 };
    struct tp_driver d = { 0 };
    struct tp_driver_params dp = { 0 };
    const char *cases[] = { "a", "gone" };
    const char *text = "!TestCase{a}\n  x=\n!EndTestCase\n";
    char err[200], *where, *r;

    par.file = "x.test.bas";
    par.test_id = "x";
    CHECK(tp_process(text, strlen(text), &par, &src) == 0);
    dp.macros = "M1\nM2\n";
    dp.macros_len = 6;
    dp.result = CR_RESULT;
    dp.cases = cases;
    dp.ncases = 2;
    CHECK(tp_driver_write(&src, &dp, &d, err, sizeof err) == 0);
    /* lines: M1 M2 TLBegin Goto | 5-7 the source | .tc__skip 8 | 9 a, 10 gone | End */
    r = cr_compile_mapped("error 6 Syntax Error", &d, "x.test.bas", &where);
    CHECK(r && !strcmp(r, "error 2 Syntax Error"));
    CHECK(where && !strcmp(where, "x.test.bas:2: Syntax Error"));
    free(r);
    free(where);
    r = cr_compile_mapped("error 10 Label not Found", &d, "x.test.bas", &where);
    CHECK(r && !strcmp(r, "error driver:10 Label not Found"));
    CHECK(where && strstr(where, "call of case gone (line 10): Label not Found") != NULL);
    free(r);
    free(where);
    r = cr_compile_mapped("error 3 Bad", &d, "x.test.bas", &where);
    CHECK(r && !strcmp(r, "error driver:3 Bad"));
    CHECK(where && strstr(where, "line 3 of the driver (not of x.test.bas)") != NULL);
    free(r);
    free(where);
    r = cr_compile_mapped("ok", &d, "x.test.bas", &where);
    CHECK(r && !strcmp(r, "ok") && !where);
    free(r);
    r = cr_compile_mapped("error x y", &d, "x.test.bas", &where);
    CHECK(r && !strcmp(r, "error x y") && !where);
    free(r);
    tp_driver_free(&d);
    tp_free(&src);
}

/* the effective configuration of a three-level hierarchy, with the origins */
static void test_print_config(void)
{
    struct t t;
    char *buf = NULL, *want;
    size_t len = 0;
    FILE *f;

    put("h/tests.toml", "root = true\ntimeout = 60\n[asserts]\nrc = \"== 0\"\n");
    put("h/sub/tests.toml", "labels = [\"libs\"]\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    put("h/sub/deep/p.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("h/sub/deep/p.test.toml", "fixed-orig = 12\n[deviations.\"libs=cd\"]\n\"check.c.n\" = \"== fail\"\n");
    load(&t, "h/sub/deep/p.test.bas", ENV_RUNNER);
    f = open_memstream(&buf, &len);
    CHECK(f && cr_print_config(f, &t.conf, &t.m) == 0);
    fclose(f);
    want = str_printf("test p (%s/h/sub/deep/p.test.bas)\n"
                      "files, nearest first:\n"
                      "  %s/h/sub/deep/p.test.toml\n"
                      "  %s/h/sub/tests.toml\n"
                      "  %s/h/tests.toml\n"
                      "settings:\n"
                      "  compiler = \"port\"    # (default)\n"
                      "  libs: an axis, below\n"
                      "  machine = \"a1200\"    # (default)\n"
                      "  runtime-errors = \"off\"    # (default)\n"
                      "  timeout = 60    # %s/h/tests.toml:2\n"
                      "  serial = \"off\"    # (default)\n"
                      "  rtg = \"off\"    # (default)\n"
                      "  pad = \"off\"    # (default)\n"
                      "  labels = [\"libs\"]    # %s/h/sub/tests.toml:1\n"
                      "  root = true    # %s/h/tests.toml:1\n"
                      "  fixed-orig = 12    # %s/h/sub/deep/p.test.toml:1\n"
                      "matrix:\n"
                      "  matrix.libs = [\"cd\", \"clib\"]    # %s/h/sub/tests.toml:3\n"
                      "assertions:\n"
                      "  asserts.rc = \"== 0\"    # %s/h/tests.toml:4\n"
                      "deviations:\n"
                      "  deviations.\"libs=cd\".check.c.n = \"== fail\"    # %s/h/sub/deep/p.test.toml:3\n"
                      "default assertions (where no file names the query):\n"
                      "  rc = \"== 0\"\n"
                      "  requester = \"count == 0\"\n"
                      "  end.case = \"exists\"\n"
                      "  end.run = \"exists\"\n"
                      "  every check with entries passed, unless an assertion names it\n"
                      "axes:\n"
                      "  libs: cd clib    # %s/h/sub/tests.toml:3\n"
                      "cells:\n"
                      "  cd    # side originals\n"
                      "  clib    # side port\n",
                      work, work, work, work, work, work, work, work, work, work, work, work);
    CHECK(buf && want && !strcmp(buf, want));
    if (buf && want && strcmp(buf, want))
        fprintf(stderr, "printed:\n%s\nwanted:\n%s", buf, want);
    free(want);
    free(buf);
    unload(&t);
}

/* the comparison with prev: a cell's pass becomes a compile with clib and prev */
static void test_prev_pass(void)
{
    struct t t;
    char err[600], *cp = NULL, *p = NULL;
    const char *entries[] = { "blitz2_x_prev:x-runerrs", "blitz2_x_prev:x", "blitz2_y_prev:x", "corpus:x",
                              "blitz2_x_prev:nosuch", "blitz2_x_prev:x-runerrsx" };
    const size_t nentries = sizeof entries / sizeof entries[0];
    const char *names[] = { "x", "x-runerrs" };
    const char *stale;
    size_t cur = 0;

    put("p/tests.toml", "root = true\n[matrix]\nruntime-errors = [\"off\", \"on\"]\n");
    put("p/x.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("p/x.test.toml", "stack = 20000\nmachine = \"a600\"\nfiles = [\"data/f|/abs/f\"]\nserial = \"on\"\n"
        "runargs = \"a b\"\nlauncherargs = \"-harden\"\nfloppy = [\"1|/abs/d.adf\"]\n"
        "jobline = [\"run=r002|CORPUS:src/x.bb2|RAM:con\"]\n");
    load(&t, "p/x.test.bas", ENV_RUNNER);
    CHECK(cr_cell_pass(&t.conf, &t.m, cell(&t, "on"), &t.env, "clib", "/w/x", "/w/x.bb2", &cp, err,
                       sizeof err) == 0);
    CHECK(cp && cr_prev_pass(cp, "blitz2_x_prev", "x-runerrs", "/w/p/x-runerrs", "/w/p/x-runerrs.bb2",
                             "/prev/blitz2", "/prev", entries, nentries, &p, err, sizeof err) == 0);
    if (p) {
        struct corpus_pass cpp;
        char perr[400];

        if (corpus_parse(p, &cpp, perr, sizeof perr)) {
            fprintf(stderr, "the prev pass does not parse: %s\n%s", perr, p);
            failures++;
        } else {
            CHECK(cpp.nconfigs == 2 && !cpp.run && cpp.runtime_errors && cpp.mask_traps);
            corpus_free(&cpp);
        }
    }
    CHECK(p && has(p, "config=clib|ref|/b/blitz2|/b/deflibs|/b/acidlibs"));
    CHECK(p && has(p, "config=prev|exe:clib|/prev/blitz2|/prev/deflibs|/prev/acidlibs"));
    CHECK(p && has(p, "test=blitz2_x_prev"));
    CHECK(p && has(p, "run=off"));
    CHECK(p && has(p, "work=/w/p/x-runerrs"));
    CHECK(p && has(p, "report=/w/p/x-runerrs.txt"));
    CHECK(p && has(p, "source=/w/p/x-runerrs.bb2"));
    CHECK(p && has(p, "runtime_errors=on"));
    CHECK(p && has(p, "file=blitz2/userlibs/testlib/testlib.obj|/b/testlib.obj"));
    CHECK(p && has(p, "file=data/f|/abs/f"));
    CHECK(p && has(p, "stack=20000"));
    CHECK(p && has(p, "machine=a600"));
    CHECK(p && has(p, "jobline=run=r002|CORPUS:src/x.bb2|RAM:con"));
    CHECK(p && has(p, "mask_traps=on"));
    /* only the entry of this test and name */
    CHECK(p && has(p, "intended=blitz2_x_prev:x-runerrs"));
    CHECK(p && !strstr(p, "intended=blitz2_x_prev:x\n"));
    CHECK(p && !strstr(p, "intended=blitz2_y_prev"));
    CHECK(p && !strstr(p, "intended=corpus"));
    CHECK(p && !strstr(p, "intended=blitz2_x_prev:x-runerrsx"));
    /* what belongs to a run is gone */
    CHECK(p && !strstr(p, "cell=") && !strstr(p, "launcher=") && !strstr(p, "rcput=") && !strstr(p, "hang_")
          && !strstr(p, "serial=") && !strstr(p, "runargs=") && !strstr(p, "launcherargs=")
          && !strstr(p, "floppy=") && !strstr(p, "/w/x"));
    free(p);
    p = NULL;
    /* ON: every compile may differ */
    {
        const char *on[] = { "ON" };

        CHECK(cr_prev_pass(cp, "blitz2_x_prev", "x", "/w/p/x", "/w/p/x.bb2", "/prev/blitz2", "/prev", on, 1, &p,
                           err, sizeof err) == 0);
        CHECK(p && has(p, "intended=ON"));
        free(p);
        p = NULL;
    }
    free(cp);
    unload(&t);
    /* broken: no configuration, two, a malformed one */
    CHECK(cr_prev_pass("work=/w\nsource=/s.bb2\n", "t_prev", "x", "/w", "/s.bb2", "/p/b", "/p", NULL, 0, &p, err,
                       sizeof err) == -1 && !p && strstr(err, "no configuration"));
    CHECK(cr_prev_pass("config=a|ref|/b|/d|/a\nconfig=b|ref|/b|/d|/a\n", "t_prev", "x", "/w", "/s.bb2", "/p/b",
                       "/p", NULL, 0, &p, err, sizeof err) == -1 && !p && strstr(err, "two configurations"));
    CHECK(cr_prev_pass("config=abc\n", "t_prev", "x", "/w", "/s.bb2", "/p/b", "/p", NULL, 0, &p, err,
                       sizeof err) == -1 && !p && strstr(err, "malformed"));
    /* the entries of the test that name no compile of it, in order; other tests' are not its */
    stale = cr_prev_unknown("blitz2_x_prev", names, 2, entries, nentries, &cur);
    CHECK(stale && !strcmp(stale, "blitz2_x_prev:nosuch"));
    stale = cr_prev_unknown("blitz2_x_prev", names, 2, entries, nentries, &cur);
    CHECK(stale && !strcmp(stale, "blitz2_x_prev:x-runerrsx"));
    CHECK(!cr_prev_unknown("blitz2_x_prev", names, 2, entries, nentries, &cur));
    /* a test without runtime error checking: its entry x-runerrs is unknown */
    cur = 0;
    stale = cr_prev_unknown("blitz2_x_prev", names, 1, entries, 2, &cur);
    CHECK(stale && !strcmp(stale, "blitz2_x_prev:x-runerrs"));
    CHECK(!cr_prev_unknown("blitz2_x_prev", names, 1, entries, 2, &cur));
}

/* ------------------------------------------------- the run of all cases */

/* the preprocessed source rel (relative to work) */
static int process(const char *rel, struct tp_source *src)
{
    char *path = str_printf("%s/%s", work, rel);
    size_t len;
    char *text = path ? (char *)read_file(path, &len) : NULL;
    struct tp_params par = { 0 };
    int r = -1;

    memset(src, 0, sizeof *src);
    par.file = rel;
    par.test_id = "x";
    par.testdata = "CORPUS:data";
    if (text)
        r = tp_process(text, len, &par, src);
    CHECK(r == 0);
    free(text);
    free(path);
    return r;
}

/* whether case c of the test runs on its own over all its cells */
static int alone_of(const struct t *t, const struct tp_source *src, const char *c)
{
    size_t cells[16];

    for (size_t i = 0; i < t->m.ncells && i < 16; i++)
        cells[i] = i;
    return cr_case_alone(&t->conf, &t->m, cells, t->m.ncells, src, c);
}

static void test_case_alone(void)
{
    /* every query of the whole program, asserted for case b only */
    static const char *const whole[][2] = {
        { "rc", "== 0" }, { "requester", "count == 0" }, { "compile", "== ok" },
        { "\"compile.log\"", "contains \\\"x\\\"" }, { "output", "contains \\\"ok\\\"" },
        { "serial", "contains \\\"x\\\"" }, { "inject", "contains \\\"x\\\"" }, { "session", "== done" },
        { "\"end.run\"", "exists" },
    };
    struct tp_source src;
    struct t t;

    put("al/tests.toml", "root = true\n");
    put("al/s.test.bas", "!TestCase{a}\n  !CheckEq{1,1,\"one\"}\n!EndTestCase\n"
                         "!TestCase{b}\n  !CheckEq{2,2,\"two\"}\n!EndTestCase\n"
                         "!TestCase{e}\n  !ExpectError{\"Array\"}\n!EndTestCase\n");
    if (process("al/s.test.bas", &src))
        return;
    /* without assertions only the case with !ExpectError runs on its own */
    load(&t, "al/s.test.bas", ENV_RUNNER);
    CHECK(alone_of(&t, &src, "a") == 0);
    CHECK(alone_of(&t, &src, "b") == 0);
    CHECK(alone_of(&t, &src, "e") == 1);
    unload(&t);
    for (size_t q = 0; q < sizeof whole / sizeof whole[0]; q++) {
        char *toml = str_printf("[asserts.\"case=b\"]\n%s = \"%s\"\n", whole[q][0], whole[q][1]);

        put("al/s.test.toml", toml);
        free(toml);
        load(&t, "al/s.test.bas", ENV_RUNNER);
        if (alone_of(&t, &src, "b") != 1)
            fprintf(stderr, "the query %s does not set b apart\n", whole[q][0]);
        CHECK(alone_of(&t, &src, "b") == 1);
        CHECK(alone_of(&t, &src, "a") == 0);
        unload(&t);
    }
    /* the queries of a case share: its end entry, its checks */
    put("al/s.test.toml", "[asserts.\"case=b\"]\n\"end.case\" = \"not exists\"\n\"check.b.two\" = \"== fail\"\n");
    load(&t, "al/s.test.bas", ENV_RUNNER);
    CHECK(alone_of(&t, &src, "b") == 0);
    unload(&t);
    /* an assertion of all cases sets every case apart */
    put("al/s.test.toml", "[asserts]\nrc = \"== 20\"\n");
    load(&t, "al/s.test.bas", ENV_RUNNER);
    CHECK(alone_of(&t, &src, "a") == 1 && alone_of(&t, &src, "b") == 1);
    unload(&t);
    /* a deviation of an open bug in a cell of another side: judged as if its
       axis term matched, so it sets the case apart on this side too */
    put("al/s.test.toml", "[matrix]\nlibs = [\"cd\", \"clib\"]\n[deviations.\"libs=cd,case=b\"]\n"
                          "session = \"== timeout\"\n");
    load(&t, "al/s.test.bas", ENV_RUNNER);
    {
        size_t clib = cell(&t, "clib");

        CHECK(cr_case_alone(&t.conf, &t.m, &clib, 1, &src, "b") == 1);
        CHECK(cr_case_alone(&t.conf, &t.m, &clib, 1, &src, "a") == 0);
    }
    unload(&t);
    /* any judging mode counts (D2), also for a fixed bug: its deviation of
       the other side's cells sets the case apart on both sides; case a not */
    put("al/s.test.toml", "fixed-orig = 7\n[matrix]\nlibs = [\"cd\", \"clib\"]\n[deviations.\"libs=cd,case=b\"]\n"
                          "session = \"== timeout\"\n");
    load(&t, "al/s.test.bas", ENV_RUNNER);
    {
        size_t clib = cell(&t, "clib"), cd = cell(&t, "cd");

        CHECK(cr_case_alone(&t.conf, &t.m, &clib, 1, &src, "b") == 1);
        CHECK(cr_case_alone(&t.conf, &t.m, &clib, 1, &src, "a") == 0);
        CHECK(cr_case_alone(&t.conf, &t.m, &cd, 1, &src, "b") == 1);
    }
    unload(&t);
    put("al/s.test.toml", "");
    tp_free(&src);
}

/* the observations of a cell that ended cleanly; result with "|" for TAB */
static struct tj_obs clean_obs(char *buf, const char *result)
{
    struct tj_obs o = { 0 };
    size_t i;

    for (i = 0; result[i]; i++)
        buf[i] = result[i] == '|' ? '\t' : result[i];
    buf[i] = 0;
    o.session = "done";
    o.compile = "ok";
    o.has_rc = 1;
    o.rc = 0;
    o.has_result = 1;
    o.result = buf;
    o.result_len = i;
    return o;
}

static void test_clean(void)
{
    static const char ok[] = "begin|1\nblock|case|a\nblockend\nblock|case|b\nblockend\nend\n";
    char buf[200];
    struct tj_obs o = clean_obs(buf, ok);
    struct tj_requester rq = { "AutoRequest", (size_t)-1 };

    CHECK(cr_cell_clean(&o));
    o.session = "timeout";
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.session = "halt";
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.compile = "error 3 Syntax Error";
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.req = &rq;
    o.nreq = 1;
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.rc = 5;
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.has_rc = 0;
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, ok);
    o.has_result = 0;
    CHECK(!cr_cell_clean(&o));
    /* the program ended inside a block (End in a case: the library writes
       incomplete, then the end entry), the run's end entry missing, a last
       line without its LF */
    o = clean_obs(buf, "begin|1\nblock|case|a\nblockend\nblock|case|b\nincomplete\nend\n");
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, "begin|1\nblock|case|a\nblockend\nblock|case|b\n");
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, "begin|1\nblock|case|a\nblockend\nend");
    CHECK(!cr_cell_clean(&o));
    o = clean_obs(buf, "");
    CHECK(!cr_cell_clean(&o));
}

static void test_open_case(void)
{
    char name[16], buf[200];
    struct tj_obs o;

    o = clean_obs(buf, "begin|1\nblock|init\nblockend\nblock|case|a\nblockend\nblock|case|b\ncheck|0|ok|$1|$1\n");
    CHECK(cr_open_case(o.result, o.result_len, name, sizeof name) == 1 && !strcmp(name, "b"));
    /* every block closed; the batch init or cleanup open is no case */
    o = clean_obs(buf, "begin|1\nblock|case|a\nblockend\nend\n");
    CHECK(cr_open_case(o.result, o.result_len, name, sizeof name) == 0 && !name[0]);
    o = clean_obs(buf, "begin|1\nblock|init\n");
    CHECK(cr_open_case(o.result, o.result_len, name, sizeof name) == 0);
    o = clean_obs(buf, "begin|1\nblock|case|a\nblockend\nblock|final\n");
    CHECK(cr_open_case(o.result, o.result_len, name, sizeof name) == 0);
    /* a name longer than the room is cut */
    o = clean_obs(buf, "begin|1\nblock|case|a_very_long_case_name\n");
    CHECK(cr_open_case(o.result, o.result_len, name, sizeof name) == 1 && strlen(name) == sizeof name - 1);
    CHECK(cr_open_case(NULL, 0, name, sizeof name) == 0);
}

/* the next run of p: its kind and cases as text ("shared 0,1" / "alone 2"
   / "none") */
static const char *next(struct cr_plan *p, char *out, size_t size)
{
    size_t set[16], n;
    int kind;

    if (!cr_plan_next(p, set, &n, &kind)) {
        snprintf(out, size, "none");
        return out;
    }
    snprintf(out, size, "%s ", kind == CR_RUN_SHARED ? "shared" : "alone");
    for (size_t i = 0; i < n; i++)
        snprintf(out + strlen(out), size - strlen(out), "%s%zu", i ? "," : "", set[i]);
    return out;
}

static void test_plan(void)
{
    struct cr_plan p;
    char s[64];

    /* a clean shared run: the passing cases count, a failed one is confirmed
       on its own, then the case that runs on its own from the start */
    {
        const int alone[] = { 0, 0, 0, 1 };

        CHECK(cr_plan_init(&p, 4, alone) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2"));
        cr_plan_judged(&p, 0, TJ_V_PASSED, 0);
        cr_plan_judged(&p, 1, TJ_V_FAILED, 0);
        cr_plan_judged(&p, 2, TJ_V_REPRODUCED, 0);
        CHECK(p.state[1] == CR_C_CONFIRM && p.shared[1] == TJ_V_FAILED);
        CHECK(p.state[0] == CR_C_DONE && p.verdict[0] == TJ_V_PASSED && !p.alone[0]);
        CHECK(!strcmp(next(&p, s, sizeof s), "alone 1"));
        cr_plan_judged(&p, 1, TJ_V_PASSED, 1);
        CHECK(p.verdict[1] == TJ_V_PASSED && p.alone[1] && p.shared[1] == TJ_V_FAILED);
        CHECK(!strcmp(next(&p, s, sizeof s), "alone 3"));
        cr_plan_judged(&p, 3, TJ_V_FAILED, 1);
        CHECK(!strcmp(next(&p, s, sizeof s), "none"));
        CHECK(cr_plan_worst(&p) == TJ_V_FAILED);
        cr_plan_free(&p);
    }
    /* an unclean shared run: the open case on its own, the others again;
       with no open case every case on its own */
    {
        const size_t all[] = { 0, 1, 2, 3 }, rest[] = { 0, 2, 3 };
        const long open_b[] = { 1 }, none[] = { -1 };

        CHECK(cr_plan_init(&p, 4, NULL) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2,3"));
        cr_plan_unclean(&p, all, 4, open_b, 1);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,2,3"));
        cr_plan_unclean(&p, rest, 3, none, 1);
        CHECK(!strcmp(next(&p, s, sizeof s), "alone 0"));
        for (size_t c = 0; c < 4; c++)
            CHECK(p.state[c] == CR_C_ALONE);
        cr_plan_free(&p);
    }
    /* two unclean cells, each with its open case; an open case the set does
       not hold counts as none; a set of one runs on its own */
    {
        const size_t all[] = { 0, 1, 2, 3 }, rest[] = { 1, 3 };
        const long two[] = { 0, 2 }, other[] = { 0 };

        CHECK(cr_plan_init(&p, 4, NULL) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2,3"));
        cr_plan_unclean(&p, all, 4, two, 2);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 1,3"));
        cr_plan_unclean(&p, rest, 2, other, 1);
        CHECK(p.state[1] == CR_C_ALONE && p.state[3] == CR_C_ALONE);
        cr_plan_free(&p);
        CHECK(cr_plan_init(&p, 3, NULL) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2"));
        cr_plan_unclean(&p, all, 3, (const long[]){ 0 }, 1);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 1,2"));
        cr_plan_unclean(&p, rest, 1, (const long[]){ 1 }, 1);
        /* left: case 2 alone in the shared state, a set of one */
        CHECK(!strcmp(next(&p, s, sizeof s), "alone 0"));
        CHECK(p.state[2] == CR_C_ALONE);
        cr_plan_free(&p);
    }
    /* the loop ends: every shared run unclean with its last case open takes
       at most one shared run per case, then one run on its own per case */
    {
        size_t set[16], n, runs = 0;
        int kind;

        CHECK(cr_plan_init(&p, 8, NULL) == 0);
        while (cr_plan_next(&p, set, &n, &kind) && runs < 100) {
            runs++;
            if (kind == CR_RUN_SHARED) {
                long last = (long)set[n - 1];

                cr_plan_unclean(&p, set, n, &last, 1);
            } else
                cr_plan_judged(&p, set[0], TJ_V_PASSED, 1);
        }
        CHECK(runs <= 2 * 8);
        CHECK(cr_plan_worst(&p) == TJ_V_PASSED);
        for (size_t c = 0; c < 8; c++)
            CHECK(p.state[c] == CR_C_DONE && p.alone[c]);
        cr_plan_free(&p);
    }
    /* a verdict on its own never waits again */
    CHECK(cr_plan_init(&p, 1, (const int[]){ 1 }) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 0"));
    cr_plan_judged(&p, 0, TJ_V_ERROR, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "none"));
    CHECK(cr_plan_worst(&p) == TJ_V_ERROR);
    cr_plan_free(&p);
}

static void test_verdict_order(void)
{
    /* error, broken, failed, skipped, reproduced, passed */
    static const int order[] = { TJ_V_ERROR, TJ_V_BROKEN, TJ_V_FAILED, CR_SKIPPED, TJ_V_REPRODUCED, TJ_V_PASSED };
    struct cr_plan p;

    for (size_t i = 0; i < 6; i++)
        for (size_t j = 0; j < 6; j++)
            CHECK(cr_verdict_worse(order[i], order[j]) == (i < j));
    CHECK(!strcmp(cr_verdict_name(CR_SKIPPED), "SKIPPED") && !strcmp(cr_verdict_name(TJ_V_BROKEN), "BROKEN"));
    /* the worst of the cases: one failed, one reproduced; an error and a failure;
       a skipped case and a reproduced one; skipped and failed */
    {
        static const int sets[][2] = { { TJ_V_REPRODUCED, TJ_V_FAILED }, { TJ_V_FAILED, TJ_V_ERROR },
                                       { CR_SKIPPED, TJ_V_REPRODUCED }, { CR_SKIPPED, TJ_V_FAILED },
                                       { TJ_V_PASSED, TJ_V_REPRODUCED } };
        static const int worst[] = { TJ_V_FAILED, TJ_V_ERROR, CR_SKIPPED, TJ_V_FAILED, TJ_V_REPRODUCED };

        for (size_t k = 0; k < sizeof sets / sizeof sets[0]; k++) {
            CHECK(cr_plan_init(&p, 2, (const int[]){ 1, 1 }) == 0);
            cr_plan_judged(&p, 0, sets[k][0], 1);
            cr_plan_judged(&p, 1, sets[k][1], 1);
            CHECK(cr_plan_worst(&p) == worst[k]);
            cr_plan_free(&p);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: test_caserun WORKDIR\n");
        return 2;
    }
    snprintf(work, sizeof work, "%s", argv[1]);
    rm_tree(work);
    if (mkdir_p(work)) {
        fprintf(stderr, "cannot create %s\n", work);
        return 2;
    }
    test_cell_passes();
    test_cell_settings();
    test_cell_refused();
    test_sessions();
    test_parse_run();
    test_compile_mapped();
    test_print_config();
    test_prev_pass();
    test_case_alone();
    test_clean();
    test_open_case();
    test_plan();
    test_verdict_order();
    if (!failures)
        rm_tree(work);
    printf("test_caserun: %d failures\n", failures);
    return failures != 0;
}
