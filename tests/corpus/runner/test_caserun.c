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
 * two, an entry of another test, of another name, ON) - and the exit
 * status of a side (cr_side_status: an open bug shown as declared passes
 * on the side originals, fails on the side port; a cell without its
 * declared deviation fails either side).
 *   test_caserun WORKDIR
 */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
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
        "jobline = [\"run=r002|CORPUS:src/x.bb2|RAM:con\", \"job=j001|@DRIVER@|CORPUS:out/again\"]\n"
        "runargs = \"a b\"\nlauncherargs = \"-harden\"\n");
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
    /* @DRIVER@: the driver's source on the cell's volume */
    CHECK(p && has(p, "jobline=job=j001|CORPUS:src/test.bb2|CORPUS:out/again"));
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
        k[i] = tc_session_key(&t.conf, &t.m, i, &t.env);
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
        k[i] = tc_session_key(&t.conf, &t.m, i, &t.env);
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < i; j++)
            CHECK(strcmp(k[i], k[j]));
    for (size_t i = 0; i < 4; i++)
        free(k[i]);
    unload(&t);
    put("e/s.test.toml", "serial = \"on\"\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    load(&t, "e/s.test.bas", ENV_RUNNER);
    k[0] = tc_session_key(&t.conf, &t.m, 0, &t.env);
    k[1] = tc_session_key(&t.conf, &t.m, 1, &t.env);
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

/* inject's report: its lines of the output, and which cells run through inject */
static void test_inject(void)
{
    struct cr_run r;
    char *l;
    struct t t;

    CHECK(cr_parse_run("hello\ninject: guard: written behind an allocation of 8 bytes\nworld\r\n"
                       "inject: rc=0\nrc=0\n", &r) == 0);
    l = cr_inject_lines(r.output);
    CHECK(l && !strcmp(l, "inject: guard: written behind an allocation of 8 bytes\ninject: rc=0\n"));
    free(l);
    /* the output keeps the lines */
    CHECK(strstr(r.output, "inject: guard: written behind") && strstr(r.output, "hello\n"));
    cr_run_free(&r);
    /* none, no line end at the end, a line that only contains it */
    l = cr_inject_lines("hello\n");
    CHECK(l && !strcmp(l, ""));
    free(l);
    l = cr_inject_lines("x inject: no\ninject: last");
    CHECK(l && !strcmp(l, "inject: last\n"));
    free(l);
    l = cr_inject_lines("");
    CHECK(l && !strcmp(l, ""));
    free(l);
    /* the cells: pad, a runwrap through the runner's inject, another runwrap,
       none */
    put("inj/tests.toml", "root = true\n");
    put("inj/s.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("inj/s.test.toml", "pad = \"on\"\n");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 1);
    unload(&t);
    put("inj/s.test.toml", "runwrap = \"/b/inject|guard\"\n");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 1);
    unload(&t);
    put("inj/s.test.toml", "runwrap = \"/b/inject\"\n");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 1);
    unload(&t);
    put("inj/s.test.toml", "runwrap = \"/b/injector|guard\"\n");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 0);
    unload(&t);
    put("inj/s.test.toml", "runwrap = \"/x/w|guard\"\n");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 0);
    unload(&t);
    put("inj/s.test.toml", "");
    load(&t, "inj/s.test.bas", ENV_RUNNER);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 0);
    unload(&t);
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
                      "  isolate = \"off\"    # (default)\n"
                      "  editor-run = \"off\"    # (default)\n"
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
                      "  rc = \"== 0\"    # not in a cell with editor-run = \"on\"\n"
                      "  requester = \"count == 0\"    # not in a cell with editor-run = \"on\"\n"
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
    /* what belongs to a run is gone; rcput stays (a Kickstart 1.x compile needs it) */
    CHECK(p && has(p, "rcput=/b/rcput"));
    CHECK(p && !strstr(p, "cell=") && !strstr(p, "launcher=") && !strstr(p, "hang_")
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
    /* the data sources of a test (change basic-test-local-data): source=SRC|ID/FILE and their
       entries; a pass of the data alone has no driver source */
    {
        static const char *const dsrc[] = { "/w/d/b002.bb2", "/w/d/b003.bb2" };
        static const char *const dname[] = { "x/child.bas", "x/errs.bas-runerrs" };
        static const char *const dent[] = { "blitz2_x_prev:x/child.bas", "other_prev:x/errs.bas-runerrs", "ON" };

        CHECK(cr_prev_data("blitz2_x_prev", dsrc, dname, 2, dent, 3, &p) == 0);
        CHECK(p && !strcmp(p, "source=/w/d/b002.bb2|x/child.bas\nintended=blitz2_x_prev:x/child.bas\n"
                              "source=/w/d/b003.bb2|x/errs.bas-runerrs\n"));
        free(p);
        CHECK(cr_prev_data("blitz2_x_prev", dsrc, dname, 0, dent, 3, &p) == 0 && p && !*p);
        free(p);
        CHECK(cr_prev_pass("config=c0|ref|/b|/d|/a\nsource=/s.bb2\n", "t_prev", "", "/w", NULL, "/p/b", "/p", NULL,
                           0, &p, err, sizeof err) == 0);
        CHECK(p && !strstr(p, "source="));
        free(p);
        /* a source with its name parses (corpus.h source=PATH|NAME) */
        {
            struct corpus_pass cp2;
            char perr[400];

            CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nrun=off\nconfig=clib|ref|/b|/d|/a\nconfig=prev|exe:clib|/p|/q|/r\n"
                               "source=/w/d/b002.bb2|x/child.bas\nsource=/w/x.bb2\n", &cp2, perr, sizeof perr) == 0);
            {
                char *n0 = corpus_source_name(&cp2, 0), *n1 = corpus_source_name(&cp2, 1);

                CHECK(n0 && !strcmp(n0, "x/child.bas") && n1 && !strcmp(n1, "x"));
                free(n0);
                free(n1);
                n0 = corpus_source_file(&cp2, 0);
                n1 = corpus_source_file(&cp2, 1);
                CHECK(n0 && !strcmp(n0, "n000") && n1 && !strcmp(n1, "x"));
                free(n0);
                free(n1);
            }
            corpus_free(&cp2);
            CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nrun=off\nconfig=clib|ref|/b|/d|/a\nconfig=prev|exe:clib|/p|/q|/r\n"
                               "source=/w/d/b002.bb2|../child\n", &cp2, perr, sizeof perr) == -1
                  && strstr(perr, "source=PATH[|NAME]"));
        }
    }
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
    /* a run from the editor gives no exit code */
    o.editor_run = 1;
    CHECK(cr_cell_clean(&o));
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
    snprintf(out, size, "%s ", kind == CR_RUN_SHARED ? "shared" : kind == CR_RUN_ISOLATED ? "isolated" : "alone");
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

        const size_t set[] = { 0, 1, 2 };

        CHECK(cr_plan_init(&p, 4, alone, 0) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2"));
        /* without isolated cells the shared run judges its cases at once */
        CHECK(cr_plan_clean(&p, set, 3) == 1);
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

        CHECK(cr_plan_init(&p, 4, NULL, 0) == 0);
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

        CHECK(cr_plan_init(&p, 4, NULL, 0) == 0);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2,3"));
        cr_plan_unclean(&p, all, 4, two, 2);
        CHECK(!strcmp(next(&p, s, sizeof s), "shared 1,3"));
        cr_plan_unclean(&p, rest, 2, other, 1);
        CHECK(p.state[1] == CR_C_ALONE && p.state[3] == CR_C_ALONE);
        cr_plan_free(&p);
        CHECK(cr_plan_init(&p, 3, NULL, 0) == 0);
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

        CHECK(cr_plan_init(&p, 8, NULL, 0) == 0);
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
    CHECK(cr_plan_init(&p, 1, (const int[]){ 1 }, 0) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 0"));
    cr_plan_judged(&p, 0, TJ_V_ERROR, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "none"));
    CHECK(cr_plan_worst(&p) == TJ_V_ERROR);
    cr_plan_free(&p);
}

/* the plan with isolated cells (D5): which case runs where */
static void test_plan_isolated(void)
{
    struct cr_plan p;
    char s[64];
    const size_t all[] = { 0, 1, 2 }, rest[] = { 0, 2 };
    const long open_b[] = { 1 };

    /* case 2 runs on its own from the start; the shared run of 0 and 1 ends
       cleanly: each then runs in the isolated cells, and is judged over all
       cells; a failure is confirmed on its own in every cell */
    CHECK(cr_plan_init(&p, 3, (const int[]){ 0, 0, 1 }, 1) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1"));
    CHECK(cr_plan_clean(&p, all, 2) == 0);
    CHECK(p.state[0] == CR_C_ISOLATED && p.state[1] == CR_C_ISOLATED);
    CHECK(!strcmp(next(&p, s, sizeof s), "isolated 0"));
    cr_plan_judged(&p, 0, TJ_V_PASSED, 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "isolated 1"));
    cr_plan_judged(&p, 1, TJ_V_BROKEN, 0);
    CHECK(p.state[1] == CR_C_CONFIRM);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 1"));
    cr_plan_judged(&p, 1, TJ_V_BROKEN, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 2"));
    cr_plan_judged(&p, 2, TJ_V_PASSED, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "none"));
    CHECK(cr_plan_worst(&p) == TJ_V_BROKEN && !p.alone[0] && p.alone[1]);
    cr_plan_free(&p);
    /* an unclean shared run: the open case on its own in every cell (no run
       in the isolated cells), the others again in the shared cells */
    CHECK(cr_plan_init(&p, 3, NULL, 1) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,1,2"));
    cr_plan_unclean(&p, all, 3, open_b, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "shared 0,2"));
    CHECK(cr_plan_clean(&p, rest, 2) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "isolated 0"));
    cr_plan_judged(&p, 0, TJ_V_PASSED, 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "isolated 2"));
    cr_plan_judged(&p, 2, TJ_V_PASSED, 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 1"));
    cr_plan_judged(&p, 1, TJ_V_PASSED, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "none"));
    cr_plan_free(&p);
    /* every cell isolated: the caller sets every case apart, no shared run */
    CHECK(cr_plan_init(&p, 2, (const int[]){ 1, 1 }, 0) == 0);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 0"));
    cr_plan_judged(&p, 0, TJ_V_PASSED, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "alone 1"));
    cr_plan_judged(&p, 1, TJ_V_PASSED, 1);
    CHECK(!strcmp(next(&p, s, sizeof s), "none"));
    cr_plan_free(&p);
}

/* the session keys of the cells with isolate */
static void test_isolate_keys(void)
{
    struct t t;
    char *k[3];

    put("iso/tests.toml", "root = true\nexclude = [\"compiler=orig195,libs=clib\"]\n"
                          "[matrix]\ncompiler = [\"port\", \"orig195\"]\nlibs = [\"cd\", \"clib\"]\n");
    put("iso/s.test.bas", "!TestCase{c}\n!EndTestCase\n");
    /* without isolate the three cells share one session */
    load(&t, "iso/s.test.bas", ENV_RUNNER);
    CHECK(t.m.ncells == 3);
    for (size_t i = 0; i < 3; i++)
        k[i] = tc_session_key(&t.conf, &t.m, i, &t.env);
    CHECK(k[0] && k[1] && k[2] && !strcmp(k[0], k[1]) && !strcmp(k[0], k[2]));
    for (size_t i = 0; i < 3; i++)
        free(k[i]);
    unload(&t);
    /* "on": three keys for three cells */
    put("iso/s.test.toml", "isolate = \"on\"\n");
    load(&t, "iso/s.test.bas", ENV_RUNNER);
    for (size_t i = 0; i < 3; i++) {
        k[i] = tc_session_key(&t.conf, &t.m, i, &t.env);
        CHECK(tc_cell_isolated(&t.conf, &t.m, i));
    }
    CHECK(k[0] && k[1] && k[2] && strcmp(k[0], k[1]) && strcmp(k[0], k[2]) && strcmp(k[1], k[2]));
    for (size_t i = 0; i < 3; i++)
        free(k[i]);
    unload(&t);
    /* "off" as if the key were absent */
    put("iso/s.test.toml", "isolate = \"off\"\n");
    load(&t, "iso/s.test.bas", ENV_RUNNER);
    for (size_t i = 0; i < 3; i++)
        k[i] = tc_session_key(&t.conf, &t.m, i, &t.env);
    CHECK(k[0] && k[1] && k[2] && !strcmp(k[0], k[1]) && !strcmp(k[0], k[2]));
    for (size_t i = 0; i < 3; i++)
        free(k[i]);
    unload(&t);
    /* ["libs=cd"]: a key each for port,cd and orig195,cd, port,clib with
       the key of the cells without isolate */
    put("iso/s.test.toml", "isolate = [\"libs=cd\"]\n");
    load(&t, "iso/s.test.bas", ENV_RUNNER);
    {
        size_t pcd = cell(&t, "port,cd"), pclib = cell(&t, "port,clib"), ocd = cell(&t, "orig195,cd");
        char *plain;

        k[0] = tc_session_key(&t.conf, &t.m, pcd, &t.env);
        k[1] = tc_session_key(&t.conf, &t.m, ocd, &t.env);
        k[2] = tc_session_key(&t.conf, &t.m, pclib, &t.env);
        CHECK(k[0] && k[1] && k[2] && strcmp(k[0], k[1]) && strcmp(k[0], k[2]) && strcmp(k[1], k[2]));
        CHECK(tc_cell_isolated(&t.conf, &t.m, pcd) && tc_cell_isolated(&t.conf, &t.m, ocd)
              && !tc_cell_isolated(&t.conf, &t.m, pclib));
        for (size_t i = 0; i < 3; i++)
            free(k[i]);
        unload(&t);
        put("iso/s.test.toml", "");
        load(&t, "iso/s.test.bas", ENV_RUNNER);
        plain = tc_session_key(&t.conf, &t.m, cell(&t, "port,clib"), &t.env);
        unload(&t);
        put("iso/s.test.toml", "isolate = [\"libs=cd\"]\n");
        load(&t, "iso/s.test.bas", ENV_RUNNER);
        k[2] = tc_session_key(&t.conf, &t.m, cell(&t, "port,clib"), &t.env);
        CHECK(plain && k[2] && !strcmp(plain, k[2]));
        free(plain);
        free(k[2]);
    }
    unload(&t);
    /* two selectors: the union */
    put("iso/s.test.toml", "isolate = [\"compiler=orig195\", \"libs=clib\"]\n");
    load(&t, "iso/s.test.bas", ENV_RUNNER);
    CHECK(!tc_cell_isolated(&t.conf, &t.m, cell(&t, "port,cd")));
    CHECK(tc_cell_isolated(&t.conf, &t.m, cell(&t, "port,clib")));
    CHECK(tc_cell_isolated(&t.conf, &t.m, cell(&t, "orig195,cd")));
    unload(&t);
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
            CHECK(cr_plan_init(&p, 2, (const int[]){ 1, 1 }, 0) == 0);
            cr_plan_judged(&p, 0, sets[k][0], 1);
            cr_plan_judged(&p, 1, sets[k][1], 1);
            CHECK(cr_plan_worst(&p) == worst[k]);
            cr_plan_free(&p);
        }
    }
}

/* the exit status of a side (cr_side_status): an open bug that shows as
   declared passes on the side originals and fails on the side port; a cell
   of the side originals without the declared deviation still fails it */
static void test_side_status(void)
{
    static const char shows[] = "begin|1\nblock|case|a\ncheck|0|fail|$00000001|$00000000\nblockend\nend\n";
    static const char gone[] = "begin|1\nblock|case|a\ncheck|0|ok|$00000001|$00000001\nblockend\nend\n";
    char buf[200];
    struct tp_source src;
    struct t t;

    put("os/tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    put("os/s.test.bas", "!TestCase{a}\n  !CheckEq{x,1,\"v\"}\n!EndTestCase\n");
    put("os/s.test.toml", "[deviations.\"libs=cd\"]\n\"check.a.v\" = \"== fail\"\n");
    if (process("os/s.test.bas", &src))
        return;
    load(&t, "os/s.test.bas", ENV_RUNNER);
    {
        struct tj_test jt = { "s.test.bas", &src, &t.conf, &t.m, "a", 1, NULL, 0 };
        size_t cd = cell(&t, "cd"), clib = cell(&t, "clib");
        struct tj_obs o;
        struct tj_msgs msgs = { 0 };
        int v;

        CHECK(t.m.cells[cd].originals && !t.m.cells[clib].originals);
        /* the side originals: the deviation holds - REPRODUCED, exit 0 */
        o = clean_obs(buf, shows);
        v = tj_verdict(&jt, &cd, &o, 1, &msgs);
        CHECK(v == TJ_V_REPRODUCED);
        CHECK(cr_side_status(v, 1) == 0);
        CHECK(!strcmp(cr_side_verdict_name(v, 1), "REPRODUCED (as declared, side originals)"));
        /* it does not hold - FAILED, exit 4 */
        o = clean_obs(buf, gone);
        v = tj_verdict(&jt, &cd, &o, 1, &msgs);
        CHECK(v == TJ_V_FAILED);
        CHECK(cr_side_status(v, 1) == TJ_V_FAILED && !strcmp(cr_side_verdict_name(v, 1), "FAILED"));
        /* the side port: the open bug shows - REPRODUCED, exit 1, as before */
        o = clean_obs(buf, shows);
        v = tj_verdict(&jt, &clib, &o, 1, &msgs);
        CHECK(v == TJ_V_REPRODUCED);
        CHECK(cr_side_status(v, 0) == 1 && !strcmp(cr_side_verdict_name(v, 0), "REPRODUCED"));
        /* the bug gone in the port without a fixed issue - FAILED */
        o = clean_obs(buf, gone);
        v = tj_verdict(&jt, &clib, &o, 1, &msgs);
        CHECK(v == TJ_V_FAILED && cr_side_status(v, 0) == TJ_V_FAILED);
        tj_msgs_free(&msgs);
    }
    /* every other verdict keeps its exit status on both sides */
    {
        static const int other[] = { TJ_V_PASSED, TJ_V_ERROR, TJ_V_BROKEN, TJ_V_FAILED, CR_SKIPPED };

        for (size_t i = 0; i < sizeof other / sizeof other[0]; i++)
            CHECK(cr_side_status(other[i], 1) == other[i] && cr_side_status(other[i], 0) == other[i]
                  && !strcmp(cr_side_verdict_name(other[i], 1), cr_verdict_name(other[i])));
    }
    unload(&t);
    tp_free(&src);
}

/* the pass lines of the test-local data (cr_data_pass, change
   basic-test-local-data D3): copy and mktestdata as files, a data source of
   the cell's build configuration in its job list, the others in compile
   parts of their own, one per build configuration */
static void test_data_pass(void)
{
    static const char *const built[] = { "/w/d/in.txt", "/w/d/d001", "/w/d/b002.bb2", "/w/d/b003.bb2",
                                         "/w/d/b004.bb2", "/w/d/b005.bb2" };
    struct t t;
    char err[600], *p;

    put("data/tests.toml", "root = true\n");
    put("data/x.test.bas", "!TestCase{a}\n!EndTestCase\n");
    put("data/x.test.toml",
        "[matrix]\ncompiler = [\"port\", \"orig195\"]\nlibs = [\"cd\", \"clib\"]\n"
        "[data]\n\"in.txt\" = \"copy\"\n\"cmap.spec\" = \"mktestdata\"\n\"same.bas\" = \"basic-compile\"\n"
        "[data.\"port.bas\"]\naction = \"basic-compile\"\ncompiler = \"port\"\n"
        "[data.\"runerr.bas\"]\naction = \"basic-compile\"\nruntime-errors = \"on\"\n"
        "[data.\"port2.bas\"]\naction = \"basic-compile\"\ncompiler = \"port\"\n");
    load(&t, "data/x.test.bas", ENV_RUNNER);
    if (!t.ok)
        return;
    /* the cell port,clib: port.bas and port2.bas are the cell's, runerr.bas a part */
    CHECK(cr_data_pass(&t.conf, &t.m, cell(&t, "port,clib"), &t.env, built, &p, err, sizeof err) == 0);
    CHECK(p && !strcmp(p, "file=x.test/in.txt|/w/d/in.txt\n"
                          "file=x.test/cmap|/w/d/d001\n"
                          "build_job=-|/w/d/b002.bb2|x.test/same\n"
                          "build_job=-|/w/d/b003.bb2|x.test/port\n"
                          "build=x.test/.build-1|/b/blitz2|/b/deflibs|/b/acidlibs|on\n"
                          "build_job=x.test/.build-1|/w/d/b004.bb2|x.test/runerr\n"
                          "build_job=-|/w/d/b005.bb2|x.test/port2\n"));
    free(p);
    /* the cell orig195,cd: the two port sources share one part */
    CHECK(cr_data_pass(&t.conf, &t.m, cell(&t, "orig195,cd"), &t.env, built, &p, err, sizeof err) == 0);
    CHECK(p && !strcmp(p, "file=x.test/in.txt|/w/d/in.txt\n"
                          "file=x.test/cmap|/w/d/d001\n"
                          "build_job=-|/w/d/b002.bb2|x.test/same\n"
                          "build=x.test/.build-1|/b/blitz2|/cd/DEFLIBS|/cd/ACIDLIBS|off\n"
                          "build_job=x.test/.build-1|/w/d/b003.bb2|x.test/port\n"
                          "build=x.test/.build-2|/b/ref/blitz2|/cd/DEFLIBS|/cd/ACIDLIBS|on\n"
                          "build_job=x.test/.build-2|/w/d/b004.bb2|x.test/runerr\n"
                          "build_job=x.test/.build-1|/w/d/b005.bb2|x.test/port2\n"));
    /* the pass with them parses */
    {
        char *cp = NULL, *all;
        struct corpus_pass cpass;
        char perr[400];

        CHECK(cr_cell_pass(&t.conf, &t.m, cell(&t, "orig195,cd"), &t.env, "c0", "/w/run", "/w/run/test.bb2", &cp,
                           err, sizeof err) == 0);
        all = str_printf("%s%s", cp ? cp : "", p ? p : "");
        int parsed = all && corpus_parse(all, &cpass, perr, sizeof perr) == 0;

        CHECK(parsed);
        if (parsed) {
            CHECK(cpass.nbuilds == 2 && cpass.nbjobs == 4 && !cpass.bjobs[0].part
                  && !strcmp(cpass.bjobs[1].part, "x.test/.build-1"));
            corpus_free(&cpass);
        }
        free(all);
        free(cp);
    }
    free(p);
    unload(&t);
    /* a runner value the environment lacks */
    load(&t, "data/x.test.bas", "[runner]\nblitz2 = \"/b/blitz2\"\n");
    if (t.ok) {
        CHECK(cr_data_pass(&t.conf, &t.m, cell(&t, "orig195,cd"), &t.env, built, &p, err, sizeof err) == -1
              && strstr(err, "no runner value"));
        unload(&t);
    }
}

/* the cell pass of a run from the editor: editrun=on, the debugger module
   of the setting on either side, the driver's source in a job line; the
   prev pass without them */
static void test_editor_run_pass(void)
{
    struct t t;
    char err[600], *p, *cp = NULL, *pp = NULL;

    put("er/tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    put("er/x.test.bas", "!TestCase{c}\n!EndTestCase\n");
    put("er/x.test.toml", "editor-run = \"on\"\nruntime-errors = \"on\"\ndefaultdbug = \"@TESTDATA@/DBG\"\n"
        "wrap = \"/b/inject|cacheflush=Debug\"\njobline = [\"job=j001|@DRIVER@|CORPUS:out/again\", \"progress=on\"]\n");
    load(&t, "er/x.test.bas", ENV_RUNNER);
    /* the side port: the setting's module (the cell has no original material) */
    p = pass(&t, "clib", err, sizeof err);
    CHECK(p && has(p, "editrun=on"));
    CHECK(p && has(p, "defaultdbug=/td/DBG"));
    CHECK(p && has(p, "jobline=job=j001|CORPUS:src/test.bb2|CORPUS:out/again"));
    CHECK(p && has(p, "jobline=progress=on"));
    CHECK(p && has(p, "wrap=/b/inject|cacheflush=Debug"));
    CHECK(p && !strstr(p, "runwrap=") && !strstr(p, "@DRIVER@"));
    if (p) {
        struct corpus_pass cpp;
        char perr[400];

        CHECK(corpus_parse(p, &cpp, perr, sizeof perr) == 0);
        CHECK(cpp.editrun && cpp.cell);
        corpus_free(&cpp);
    }
    free(p);
    /* the side originals: the setting's module replaces the CD's */
    p = pass(&t, "cd", err, sizeof err);
    CHECK(p && has(p, "defaultdbug=/td/DBG"));
    CHECK(p && !strstr(p, "defaultdbug=/cd/DEFAULTDBUG"));
    CHECK(p && has(p, "editrun=on"));
    free(p);
    CHECK(cr_cell_wrap_inject(&t.conf, &t.m, 0, &t.env) == 1);
    CHECK(cr_cell_inject(&t.conf, &t.m, 0, &t.env) == 0);
    /* the prev pass: the driver compiled as job j000, nothing run, no module;
       the job line names the prev pass's driver */
    CHECK(cr_cell_pass(&t.conf, &t.m, cell(&t, "clib"), &t.env, "clib", "/w/p/x", "/w/p/x.bb2", &cp, err,
                       sizeof err) == 0);
    CHECK(cp && has(cp, "jobline=job=j001|CORPUS:src/x.bb2|CORPUS:out/again"));
    CHECK(cp && cr_prev_pass(cp, "blitz2_x_prev", "x", "/w/p/x", "/w/p/x.bb2", "/prev/blitz2", "/prev", NULL, 0,
                             &pp, err, sizeof err) == 0);
    CHECK(pp && !strstr(pp, "editrun=") && !strstr(pp, "defaultdbug=") && has(pp, "run=off"));
    CHECK(pp && has(pp, "jobline=job=j001|CORPUS:src/x.bb2|CORPUS:out/again"));
    if (pp) {
        struct corpus_pass cpp;
        char perr[400];

        CHECK(corpus_parse(pp, &cpp, perr, sizeof perr) == 0);
        corpus_free(&cpp);
    }
    free(cp);
    free(pp);
    unload(&t);
    /* without the setting: the CD's module in the cells with original
       material only, and no run from the editor */
    put("er/x.test.toml", "");
    load(&t, "er/x.test.bas", ENV_RUNNER);
    p = pass(&t, "clib", err, sizeof err);
    CHECK(p && !strstr(p, "defaultdbug=") && !strstr(p, "editrun="));
    free(p);
    p = pass(&t, "cd", err, sizeof err);
    CHECK(p && has(p, "defaultdbug=/cd/DEFAULTDBUG"));
    free(p);
    CHECK(cr_cell_wrap_inject(&t.conf, &t.m, 0, &t.env) == 0);
    unload(&t);
    /* a wrap that is not inject */
    put("er/x.test.toml", "wrap = \"/x/w|track\"\n");
    load(&t, "er/x.test.bas", ENV_RUNNER);
    CHECK(cr_cell_wrap_inject(&t.conf, &t.m, 0, &t.env) == 0);
    unload(&t);
}

/* the observation of a recorded cell directory (cr_observe): a run from the
   shell and a run from the editor, a compiler through inject and one without */
static void test_observe(void)
{
    static const char src_text[] = "!TestCase{c}\n  !CheckEq{1,1,\"one\"}\n!EndTestCase\n";
    static const char res[] = "begin\t1\nblock\tcase\tc\ncheck\t0\tok\t$00000001\t$00000001\nblockend\nend\n";
    static const char b2[] = "inject: track: 0 allocations left, 0 bytes\ninject: rc=0\nrc=0\n";
    static const char log[] = "BATCH\tstart\nJOB\tj000\tCORPUS:src/test.bb2\nRESULT\tj000\tok\t1234\n"
                              "JOB\tr001\tCORPUS:src/test.bb2\nOPENIO\tr001\nCLOSEIO\tr001\nOUTPUT\tr001\tstart\n"
                              "REQUESTERS\tr001\t0\nBATCH\tend\n";
    struct tp_source src = { 0 };
    struct tp_params par = { 0 };
    struct tp_driver drv = { 0 };
    struct corpus_pass p;
    struct cr_obs ob;
    struct t t;
    char err[400];

    par.file = "s.test.bas";
    par.test_id = "s";
    CHECK(tp_process(src_text, strlen(src_text), &par, &src) == 0);
    put("ob/tests.toml", "root = true\n");
    put("ob/s.test.bas", src_text);
    load(&t, "ob/s.test.bas", ENV_RUNNER);
    /* the shell: the program's output and exit code; the compiler through inject */
    put("ob/sh/out/session.log", "BATCH\tstart\nJOB\tj000\tCORPUS:src/test.bb2\nRESULT\tj000\tok\t1234\nBATCH\tend\n");
    put("ob/sh/out/blitz2.out", b2);
    put("ob/sh/out/test.run", "hello\nrc=5\n");
    put("ob/sh/out/test.res", res);
    CHECK(corpus_parse("cell=on\nwork=/w\nreport=/w/r.txt\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n"
                       "source=/s/test.bb2\n", &p, err, sizeof err) == 0);
    {
        char *vol = str_printf("%s/ob/sh", work);
        struct cr_cellrun c = { vol, "done", &p, 0, 1 };

        CHECK(cr_observe(&c, &t.conf, &src, &drv, "s.test.bas", &ob) == 0);
        CHECK(ob.o.output && !strcmp(ob.o.output, "hello\n"));
        CHECK(ob.o.has_rc && ob.o.rc == 5 && !ob.o.editor_run);
        CHECK(ob.o.compile && !strcmp(ob.o.compile, "ok"));
        CHECK(ob.o.compile_inject && !strcmp(ob.o.compile_inject, "inject: track: 0 allocations left, 0 bytes\n"
                                                                  "inject: rc=0\n"));
        CHECK(!ob.o.inject);
        CHECK(ob.o.has_result);
        cr_obs_free(&ob);
        /* without inject on the compiler: not captured */
        c.wrap_inject = 0;
        CHECK(cr_observe(&c, &t.conf, &src, &drv, "s.test.bas", &ob) == 0);
        CHECK(!ob.o.compile_inject);
        cr_obs_free(&ob);
        free(vol);
    }
    corpus_free(&p);
    /* the editor: the console is the output, no exit code and no requesters
       (a test.run of another run does not count) */
    put("ob/ed/out/session.log", log);
    put("ob/ed/out/blitz2.out", "rc=0\n");
    put("ob/ed/out/test.con", "start\n");
    put("ob/ed/out/test.run", "REQUESTER: x\nother\nrc=0\n");
    put("ob/ed/out/test.res", res);
    CHECK(corpus_parse("cell=on\neditrun=on\nwork=/w\nreport=/w/r.txt\nted=/t\nlauncher=/l\n"
                       "config=c0|ref|/b|/d|/a\nsource=/s/test.bb2\n", &p, err, sizeof err) == 0);
    {
        char *vol = str_printf("%s/ob/ed", work);
        struct cr_cellrun c = { vol, "done", &p, 0, 0 };

        CHECK(cr_observe(&c, &t.conf, &src, &drv, "s.test.bas", &ob) == 0);
        CHECK(ob.o.output && !strcmp(ob.o.output, "start\n"));
        CHECK(ob.o.editor_run && !ob.o.has_rc && ob.o.nreq == 0);
        CHECK(ob.o.compile_log && strstr(ob.o.compile_log, "OUTPUT\tr001\tstart\nREQUESTERS\tr001\t0\n"));
        CHECK(cr_cell_clean(&ob.o));
        cr_obs_free(&ob);
        /* the program did not start: no console, the output not captured */
        {
            char *con = str_printf("%s/ob/ed/out/test.con", work);

            CHECK(con && unlink(con) == 0);
            free(con);
        }
        CHECK(cr_observe(&c, &t.conf, &src, &drv, "s.test.bas", &ob) == 0);
        CHECK(!ob.o.output);
        cr_obs_free(&ob);
        free(vol);
    }
    corpus_free(&p);
    unload(&t);
    tp_free(&src);
}

/* the named compile jobs (change named-compile-jobs): @DATA:FILE@ in the
   cell pass from the count of cr_data_pass, the blocks and results of the
   jobs in the observation of a recorded cell directory (j000, jobline
   jobs, a data source in the cell's job list and one in a part) */
static void test_named_jobs(void)
{
    static const char *const built[] = { "/w/d/b000.bb2", "/w/d/in.txt", "/w/d/b001.bb2", "/w/d/b002.bb2" };
    static const char src_text[] = "!TestCase{c}\n!EndTestCase\n";
    struct t t;
    char err[600], *p, *s;

    put("nj/tests.toml", "root = true\n");
    put("nj/x.test.bas", src_text);
    put("nj/x.test.toml",
        "jobline = [\"run=r001|@DATA:victim.bas@|RAM:con|100\", \"job=j002|@DATA:runerr.bas@|CORPUS:out/x\", "
        "\"x=@DATA:same.bas@@DATA:victim.bas@ @DRIVER@\"]\n"
        "[matrix]\ncompiler = [\"port\", \"orig195\"]\nlibs = [\"cd\", \"clib\"]\n"
        "[data]\n\"same.bas\" = \"basic-compile\"\n\"in.txt\" = \"copy\"\n\"victim.bas\" = \"basic-compile\"\n"
        "[data.\"runerr.bas\"]\naction = \"basic-compile\"\nruntime-errors = \"on\"\n");
    load(&t, "nj/x.test.bas", ENV_RUNNER);
    if (!t.ok)
        return;
    /* the second data source is b001, whatever comes between; the one of
       another build configuration has the part's path */
    s = cr_data_staged(&t.conf, &t.m, cell(&t, "port,clib"), "victim.bas");
    CHECK(s && !strcmp(s, "CORPUS:src/b001.bb2"));
    free(s);
    s = cr_data_staged(&t.conf, &t.m, cell(&t, "port,clib"), "runerr.bas");
    CHECK(s && !strcmp(s, "CORPUS:x.test/.build-1/src/b002.bb2"));
    free(s);
    CHECK(!cr_data_staged(&t.conf, &t.m, cell(&t, "port,clib"), "in.txt"));
    CHECK(!cr_data_staged(&t.conf, &t.m, cell(&t, "port,clib"), "other.bas"));
    p = pass(&t, "port,clib", err, sizeof err);
    CHECK(p && has(p, "jobline=run=r001|CORPUS:src/b001.bb2|RAM:con|100"));
    CHECK(p && has(p, "jobline=job=j002|CORPUS:x.test/.build-1/src/b002.bb2|CORPUS:out/x"));
    CHECK(p && has(p, "jobline=x=CORPUS:src/b000.bb2CORPUS:src/b001.bb2 CORPUS:src/test.bb2"));
    CHECK(p && !strstr(p, "@DATA:"));
    /* the same count as the data pass: its build_job lines stage there */
    {
        char *dp = NULL, *all;
        struct corpus_pass cp;
        char perr[400];

        CHECK(cr_data_pass(&t.conf, &t.m, cell(&t, "port,clib"), &t.env, built, &dp, err, sizeof err) == 0);
        all = str_printf("%s%s", p ? p : "", dp ? dp : "");
        CHECK(all && corpus_parse(all, &cp, perr, sizeof perr) == 0);
        if (all && cp.nbjobs == 3) {
            CHECK(!cp.bjobs[1].part && !strcmp(cp.bjobs[1].src, "/w/d/b001.bb2"));
            CHECK(cp.bjobs[2].part && !strcmp(cp.bjobs[2].part, "x.test/.build-1"));
            corpus_free(&cp);
        } else
            CHECK(!"three build jobs");
        free(all);
        free(dp);
    }
    free(p);
    unload(&t);

    /* the observation: the jobs of the cell's log and the data sources, one
       in the cell's log, one in its part's */
    put("nj/y.test.bas", src_text);
    put("nj/y.test.toml",
        "jobline = [\"job=j001|@DRIVER@|CORPUS:out/again\", \"run=r002|CORPUS:a.bb2|RAM:con\", \"menu=m1|9\", "
        "\"progress=on\", \"job=j003|CORPUS:b.bb2|CORPUS:out/b\"]\n"
        "[data]\n\"victim.bas\" = \"basic-compile\"\n"
        "[data.\"part.bas\"]\naction = \"basic-compile\"\nruntime-errors = \"on\"\n");
    load(&t, "nj/y.test.bas", ENV_RUNNER);
    if (!t.ok)
        return;
    put("nj/c/out/session.log",
        "BATCH\tstart\nJOB\tb000\tCORPUS:src/b000.bb2\nRESULT\tb000\tok\t300\n"
        "JOB\tj000\tCORPUS:src/test.bb2\nRESULT\tj000\tok\t1234\nGUARD\tj000\tok\n"
        "JOB\tj001\tCORPUS:src/test.bb2\nRESULT\tj001\terror\t7\tSyntax Error\tx=\nGUARD\tj001\tok\n"
        "JOB\tr002\tCORPUS:a.bb2\nOPENIO\tr002\nOUTPUT\tr002\thi\nCLOSEIO\tr002\n"
        "ezfreq \"Load\" -> blitz2:dbg\nMENU\tm1\t9\t0\nBATCH\tend\n");
    put("nj/c/y.test/.build-1/session.log",
        "BATCH\tstart\nJOB\tb001\tCORPUS:y.test/.build-1/src/b001.bb2\nRESULT\tb001\terror\t2\tSyntax Error\ty\n"
        "BATCH\tend\n");
    put("nj/c/out/blitz2.out", "rc=0\n");
    {
        struct corpus_pass cp;
        char *vol = str_printf("%s/nj/c", work);
        struct cr_cellrun c = { vol, "done", &cp, 0, 0 };
        struct tp_source src = { 0 };
        struct tp_params par = { 0 };
        struct tp_driver drv = { 0 };
        struct cr_obs ob;
        const struct tj_job *j;

        par.file = "y.test.bas";
        par.test_id = "y";
        CHECK(tp_process(src_text, strlen(src_text), &par, &src) == 0);
        CHECK(corpus_parse("cell=on\nwork=/w\nreport=/w/r.txt\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n"
                           "source=/s/test.bb2\nbuild=y.test/.build-1|/b|/d|/a|on\n"
                           "build_job=-|/w/d/b000.bb2|y.test/victim\n"
                           "build_job=y.test/.build-1|/w/d/b001.bb2|y.test/part\n", &cp, err, sizeof err) == 0);
        CHECK(cr_observe(&c, &t.conf, &src, &drv, "y.test.bas", &ob) == 0);
        /* j000, the four jobs of jobline (not progress=), the two data sources */
        CHECK(ob.o.njobs == 7 && ob.o.jobs == ob.jobs);
        if (ob.o.njobs == 7) {
            j = &ob.o.jobs[0];
            CHECK(!strcmp(j->name, "j000") && !j->result);
            CHECK(!strcmp(j->block, "JOB\tj000\tCORPUS:src/test.bb2\nRESULT\tj000\tok\nGUARD\tj000\tok\n"));
            j = &ob.o.jobs[1];
            CHECK(!strcmp(j->name, "j001") && j->result && !strcmp(j->result, "error 7 Syntax Error"));
            CHECK(!strcmp(j->block, "JOB\tj001\tCORPUS:src/test.bb2\nRESULT\tj001\terror\t7\tSyntax Error\tx=\n"
                                    "GUARD\tj001\tok\n"));
            j = &ob.o.jobs[2];
            CHECK(!strcmp(j->name, "r002") && !j->result);
            CHECK(!strcmp(j->block, "JOB\tr002\tCORPUS:a.bb2\nOPENIO\tr002\nOUTPUT\tr002\thi\nCLOSEIO\tr002\n"));
            j = &ob.o.jobs[3];
            CHECK(!strcmp(j->name, "m1") && !j->result);
            CHECK(!strcmp(j->block, "ezfreq \"Load\" -> blitz2:dbg\nMENU\tm1\t9\t0\n"));
            /* a job that did not run: no block, its result none */
            j = &ob.o.jobs[4];
            CHECK(!strcmp(j->name, "j003") && !strcmp(j->block, "") && j->result && !strcmp(j->result, "none"));
            /* the data sources under their files */
            j = &ob.o.jobs[5];
            CHECK(!strcmp(j->name, "victim.bas") && !j->result);
            CHECK(!strcmp(j->block, "JOB\tvictim.bas\t@DATA:victim.bas@\nRESULT\tvictim.bas\tok\n"));
            j = &ob.o.jobs[6];
            CHECK(!strcmp(j->name, "part.bas"));
            CHECK(!strcmp(j->block, "JOB\tpart.bas\t@DATA:part.bas@\nRESULT\tpart.bas\terror\t2\tSyntax Error\ty\n"));
        }
        cr_obs_free(&ob);
        corpus_free(&cp);
        tp_free(&src);
        free(vol);
    }
    unload(&t);
}

/* the bundles (caserun.h, "The bundles"): the chunks, the volume and the
   script of a bundle's session, the hang check's file, the session budget,
   a mount named BUNDLE (broken input: refused), the exit status */
static void test_bundle(void)
{
    const char *keys[40];
    size_t chunk[40], pend[40];

    /* 1, 16, 17, 33 cells of one key: 1, 1, 2, 3 chunks of at most 16 */
    for (size_t i = 0; i < 40; i++)
        keys[i] = "k";
    CHECK(cr_bundle_chunks(keys, 1, chunk) == 1 && chunk[0] == 0);
    CHECK(cr_bundle_chunks(keys, 16, chunk) == 1 && chunk[15] == 0);
    CHECK(cr_bundle_chunks(keys, 17, chunk) == 2 && chunk[15] == 0 && chunk[16] == 1);
    CHECK(cr_bundle_chunks(keys, 33, chunk) == 3 && chunk[16] == 1 && chunk[31] == 1 && chunk[32] == 2);
    CHECK(cr_bundle_chunks(keys, 0, chunk) == 0);
    /* two keys mixed: grouped by key in the order of their first cell */
    for (size_t i = 0; i < 20; i++)
        keys[i] = i % 2 ? "b" : "a";
    CHECK(cr_bundle_chunks(keys, 20, chunk) == 2);
    for (size_t i = 0; i < 20; i++)
        CHECK(chunk[i] == i % 2);
    for (size_t i = 0; i < 20; i++)
        keys[i] = i < 18 ? "a" : "b";
    keys[1] = "b";
    CHECK(cr_bundle_chunks(keys, 20, chunk) == 3);
    CHECK(chunk[0] == 0 && chunk[17] == 1 && chunk[1] == 2 && chunk[18] == 2 && chunk[19] == 2);
    CHECK(chunk[16] == 0 && chunk[2] == 0);

    /* the volume: one BUNDLE: whatever the number of cells, its cell c0;
       without a bundle a volume CELLk: per cell */
    {
        const char *dirs[3] = { "/w/a/c0/vol", "/w/b/c0/vol", "/w/c/c0/vol" };
        struct corpus_volume v[3];
        char names[3][32];

        CHECK(cr_session_volumes("/w/run/bundle", dirs, 3, v, names) == 1);
        CHECK(!strcmp(v[0].volume, "BUNDLE") && !strcmp(v[0].host_dir, "/w/run/bundle") && !strcmp(v[0].cell, "c0"));
        CHECK(cr_session_volumes("/w/run/bundle", dirs, 1, v, names) == 1);
        CHECK(cr_session_volumes(NULL, dirs, 3, v, names) == 3);
        CHECK(!strcmp(v[2].volume, "CELL2") && !strcmp(v[2].host_dir, "/w/c/c0/vol") && !v[2].cell);
    }
    /* the script: the assigns of the cells' directories first, then the
       parts as without a bundle */
    {
        const char *comp[2] = { "compile a\n", "compile b\n" }, *run[2] = { "run a\n", "run b\n" };
        char *s = cr_session_script(comp, run, 2, 1), *t = cr_session_script(comp, run, 2, 0);

        CHECK(s && !strcmp(s, "Assign CELL0: BUNDLE:c0\nAssign CELL1: BUNDLE:c1\n"
                              "Assign CORPUS: CELL0:\ncompile a\nAssign CORPUS: CELL1:\ncompile b\n"
                              "Assign CORPUS: CELL0:\nrun a\nAssign CORPUS: CELL1:\nrun b\nAssign CORPUS:\n"));
        CHECK(t && !strcmp(t, "Assign CORPUS: CELL0:\ncompile a\nAssign CORPUS: CELL1:\ncompile b\n"
                              "Assign CORPUS: CELL0:\nrun a\nAssign CORPUS: CELL1:\nrun b\nAssign CORPUS:\n"));
        free(s);
        free(t);
    }
    /* the hang check: the first cell's run in the bundle's volume */
    {
        struct corpus_pass p;
        char perr[400], *a;

        CHECK(corpus_parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nhang_after=8\nhang_from=run\n"
                           "config=c0|ref|/b|/d|/a\nsource=/w/test.bb2\n",
                           &p, perr, sizeof perr) == 0);
        a = corpus_hang_arm(&p, "/w/run/bundle", "c0");
        CHECK(a && !strcmp(a, "/w/run/bundle/c0/out/test.run"));
        free(a);
        a = corpus_hang_arm(&p, "/w/run/c0/vol", NULL);
        CHECK(a && !strcmp(a, "/w/run/c0/vol/out/test.run"));
        free(a);
        corpus_free(&p);
    }
    /* the budget: the cells not done, at most CR_BUNDLE_SESSIONS sessions */
    {
        int done[5] = { 1, 1, 0, 0, 0 };

        CHECK(cr_bundle_pending(done, 5, 0, pend) == 3 && pend[0] == 2 && pend[2] == 4);
        CHECK(cr_bundle_pending(done, 5, 2, pend) == 3);
        CHECK(cr_bundle_pending(done, 5, CR_BUNDLE_SESSIONS, pend) == 0);
        done[2] = done[3] = done[4] = 1;
        CHECK(cr_bundle_pending(done, 5, 1, pend) == 0);
    }
    /* a mount named BUNDLE: refused, naming it (any case) */
    {
        struct t t;
        char err[400], *text;
        struct corpus_pass p;

        put("bu/tests.toml", "root = true\n");
        put("bu/m.test.bas", "!TestCase{c}\n!EndTestCase\n");
        put("bu/m.test.toml", "mount = [\"Bundle|/data/x\"]\n");
        load(&t, "bu/m.test.bas", ENV_RUNNER);
        text = pass(&t, t.m.cells[0].name, err, sizeof err);
        CHECK(text && corpus_parse(text, &p, err, sizeof err) == 0);
        if (text) {
            CHECK(cr_bundle_mount_ok(&p, err, sizeof err) == -1 && strstr(err, "the mount Bundle:"));
            corpus_free(&p);
        }
        free(text);
        unload(&t);
        put("bu/m.test.toml", "mount = [\"DATA|/data/x\"]\n");
        load(&t, "bu/m.test.bas", ENV_RUNNER);
        text = pass(&t, t.m.cells[0].name, err, sizeof err);
        CHECK(text && corpus_parse(text, &p, err, sizeof err) == 0);
        if (text) {
            CHECK(cr_bundle_mount_ok(&p, err, sizeof err) == 0);
            corpus_free(&p);
        }
        free(text);
        unload(&t);
    }
    /* the exit status: 0 when a session ran (whatever the members'
       verdicts), 77 when none did, 2 for an error of its own */
    CHECK(cr_bundle_status(0, 1) == 0);
    CHECK(cr_bundle_status(0, 4) == 0);
    CHECK(cr_bundle_status(0, 0) == 77);
    CHECK(cr_bundle_status(1, 3) == 2);
    CHECK(cr_bundle_status(1, 0) == 2);
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
    test_inject();
    test_compile_mapped();
    test_print_config();
    test_prev_pass();
    test_clean();
    test_open_case();
    test_plan();
    test_plan_isolated();
    test_isolate_keys();
    test_verdict_order();
    test_side_status();
    test_data_pass();
    test_editor_run_pass();
    test_observe();
    test_named_jobs();
    test_bundle();
    if (!failures)
        rm_tree(work);
    printf("test_caserun: %d failures\n", failures);
    return failures != 0;
}
