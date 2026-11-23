/*
 * caserun.h - one case of a Blitz test of the BASIC test model (change
 * basic-test-runner; tests/README.md, "Tests that judge themselves"): the
 * cells of one side compiled and run in Amiberry, judged from the result
 * file of the test library and the other observations of the run
 * (corpusrun case, cmd_case.cpp).
 *
 * The inputs:
 *   - the environment file of the build (testconf.h tc_env, written by
 *     cmake/basic-test-model.cmake): the placeholders of the configuration
 *     and, in its table `runner`, what the runner needs: the compilers
 *     (`blitz2`, `orig195`, `orig210`), the library sets (`cd-deflibs`,
 *     `cd-acidlibs`, `clib-deflibs`, `clib-acidlibs`), `cd-defaultdbug`,
 *     the programs of a session (`ted`, `launcher`, `rcput`, `inject`
 *     for `pad`), the test library (`testlib`, `macros`), the tokeniser
 *     (`tedsrc`, `tokens`, and `bbsrc` for the token list of RWE user
 *     libraries), the RWE user libraries (`rwe-NAME` = "OBJ|REF|VOUT": the
 *     C build, the reference for orig210, the variant's directory with its
 *     acidlibs and deflibs), the work directory `work`, the defaults
 *     `timeout` and `run-hang-after` (seconds);
 *   - the source ID.test.bas and its configuration (testconf.h), the case
 *     and the side.
 *
 * The run: the source is preprocessed (testpp.h), the driver written
 * (testdriver.h) with the result file CORPUS:out/test.res and tokenised
 * (tedsrc tok), and each cell of the side becomes a pass of corpus.h with
 * one configuration (cr_cell_pass: the compiler and libraries of the cell,
 * the test library in blitz2:userlibs/testlib/ - for orig195 also in
 * blitz2:testlib/ -, the run settings), staged in a directory of its own.
 * The cells whose session settings (cr_session_key: the machine, the
 * emulator changes, RTG, the floppies, the mounts, the time limit, the
 * hang check) agree share one emulator session: each cell's directory is
 * the volume CELLk:, `Assign CORPUS: CELLk:` selects it, the compiles of
 * all cells come first, then the runs (the hang check counts from the first
 * run).  A cell with serial=on has a session of its own.  When a session
 * does not end normally, the first cell whose compile or run did not end
 * gets the session's end; the cells behind it run again in a new session.
 *
 * The verdict (testjudge.h tj_verdict) is the exit status: 0 passed, 1
 * reproduced (an open bug shows as declared), 2 error (a configuration,
 * source or result that cannot be judged), 3 broken, 4 failed, 77 skipped
 * (no emulator, or a file of the side is missing: the CD, the build of
 * blitz2.s).  On the side originals an open bug that shows as declared
 * passes (cr_side_status): exit status 0, the report says "REPRODUCED (as
 * declared, side originals)"; only the side port fails with it.  With
 * require_deviation (a bug-orig test) a test without any declared deviation
 * is broken: it would not show its bug.
 */
#ifndef CASERUN_H
#define CASERUN_H

#include <stddef.h>
#include <stdio.h>
#include "testconf.h"
#include "testdriver.h"
#include "testjudge.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The Amiga names of a run: the program and its result file. */
#define CR_PROGRAM "test"
#define CR_RESULT "CORPUS:out/test.res"
/** @brief What @TESTDATA@ stands for in a test source (the test data a
 *  configuration stages with files = ["data/NAME|@TESTDATA@/NAME"]). */
#define CR_TESTDATA "CORPUS:data"
/** @brief What @TESTDIR@ stands for (printf format, the test id): the
 *  test's own data, the files of ID.test/ (change basic-test-local-data). */
#define CR_TESTDIR "CORPUS:%s.test"

/** @brief What to do. */
struct caserun_opts {
    const char *env;            /* the environment file */
    const char *source;         /* ID.test.bas */
    const char *case_name;      /* NULL: with print_config only */
    int originals;              /* the side with original material */
    int require_deviation;      /* a bug-orig test */
    int print_config;           /* print the effective configuration, run nothing */
    int print_driver;           /* print the driver of the case, run nothing */
    const char *write_driver;   /* write the driver of all cases, tokenised, to
                                   this file (NAME.bb2; its text beside it as
                                   NAME.bas) and run nothing: the build step */
    int prev;                   /* the comparison with the prev build (below) */
    const char *driver;         /* with prev: the driver write_driver wrote */
    const char *test;           /* with prev: the ctest name, the TEST of the
                                   PREV_INTENDED entries */
    const char *prev_blitz2;    /* with prev: the previous commit's compiler */
    const char *prev_libs;      /* with prev: the directory of its deflibs and
                                   acidlibs */
    const char *const *intended;    /* with prev: the PREV_INTENDED entries (TEST:NAME
                                       of any test, or ON) */
    size_t nintended;
};

/*
 * The comparison with the prev build (prev, CLAUDE.md rule 3; change
 * basic-test-runner D10): the driver of all cases is compiled by this
 * build's compiler and by the previous commit's, once per value of
 * runtime-errors among the cells without original material (the side
 * port), with the other settings of the first such cell: a pass of corpus.h
 * with run=off, the configuration clib (the cell's) and prev|exe:clib.  The
 * compile without runtime error checking is named ID, the one with it
 * ID-runerrs (CR_PREV_RUNERRS); a PREV_INTENDED entry TEST:NAME names one,
 * and an entry of TEST that names no compile of the test is stale.  The
 * exit status: 0 the same, 1 a difference that is not intended or a stale
 * entry, 2 error (also a test with RWE user libraries: not compared), 77
 * skipped.
 */
#define CR_PREV_RUNERRS "-runerrs"

/**
 * @brief The pass file text of one compile of the comparison: the cell's
 *        pass (cr_cell_pass, tag clib) without what belongs to a run (the
 *        launcher, rcput, the hang check, serial, runwrap, runargs,
 *        launcherargs, the debugger module, the floppies), with run=off,
 *        test=TEST, the configuration prev|exe:TAG after the cell's, the
 *        source, the work directory and its report WORK.txt, mask_traps=on
 *        for a job line run=, and the entries of TEST:NAME (or ON) as
 *        intended= lines.
 * @return 0 = ok, -1 = refused (err: a cell pass without a configuration),
 *         -2 = out of memory
 */
int cr_prev_pass(const char *cell_pass, const char *test, const char *name, const char *work, const char *source,
                 const char *prev_blitz2, const char *prev_libs, const char *const *entries, size_t nentries,
                 char **text, char *err, size_t errlen);

/**
 * @brief The next entry of test that names none of the compiles names[]
 *        (stale: the test has no such compile).
 * @param cursor  0 at the start
 * @return the entry, NULL = no more
 */
const char *cr_prev_unknown(const char *test, const char *const *names, size_t nnames, const char *const *entries,
                            size_t nentries, size_t *cursor);

/**
 * @brief Run (or show) one case of a test, or with case_name NULL all cases
 *        of the source (below).
 * @param out  the report, the diagnostics, the @stat lines
 * @return the verdict as above
 */
int caserun(const struct caserun_opts *o, FILE *out);

/*
 * The run of all cases of a source (case_name NULL; change
 * basic-test-runner-extension, D1-D4): the driver of a set of cases runs in
 * the cells of the side as the driver of one case does (the same passes and
 * sessions), and each case of the set is judged from its block of the
 * result file, the batch init and the batch cleanup, with the assertions and
 * deviations of its own run (tj_test.cases: the blocks of the other cases
 * of the driver are theirs).  The work directory is ID (ID_originals), each
 * run of a set a directory of its own in it: shared-N, alone-CASE.
 *
 *   - The cases that run on their own from the start (cr_case_alone): those
 *     whose assertions or deviations name an observation of the whole
 *     program other than by the defaults, and those that announce a runtime
 *     error; the others share the run.
 *   - A shared run whose cells all ended cleanly (cr_cell_clean) judges
 *     each of its cases; if a cell did not, the case whose block was open at
 *     its end (cr_open_case) runs on its own and the others run again
 *     without it, and with no open block every case of the set runs on its
 *     own.  A set of one case is a run of that case on its own.
 *   - A case judged failed, broken or error in a shared run runs again on
 *     its own, and that verdict counts (cr_plan).
 *
 * The report: per case its block (as for one case, the cells named with the
 * run they were judged from), its verdict line marked " (alone)" when it was
 * judged on its own, a line "case NAME: VERDICT alone (VERDICT2 in the run
 * shared with ...)" when the two verdicts differ, and at the end the line
 * "ID.test.bas: N cases, side SIDE: VERDICT".  The exit status is the worst
 * verdict (cr_verdict_worse) as cr_side_status gives it (on the side
 * originals REPRODUCED passes); a case whose run could not start after
 * others were judged is SKIPPED (CR_SKIPPED); 77 when the side is skipped
 * before any session.
 */

/** @brief The verdict of a case whose run could not start. */
#define CR_SKIPPED 77

/**
 * @brief Whether the verdict a is worse than b, in the order error (2),
 *        broken (3), failed (4), skipped (77), reproduced (1), passed (0).
 */
int cr_verdict_worse(int a, int b);

/** @brief The name of a verdict: PASSED, REPRODUCED, ERROR, BROKEN, FAILED, SKIPPED. */
const char *cr_verdict_name(int v);

/**
 * @brief The exit status of verdict v on a side: the verdict, except that
 *        on the side originals (originals != 0) REPRODUCED is 0 - the
 *        original shows the open bug as declared.
 */
int cr_side_status(int v, int originals);

/**
 * @brief The name of verdict v in the report of a side: cr_verdict_name,
 *        and on the side originals REPRODUCED is "REPRODUCED (as declared,
 *        side originals)".
 */
const char *cr_side_verdict_name(int v, int originals);

/**
 * @brief Whether a case runs on its own in the run of all cases: in a cell
 *        of cells[0..ncells) and a judging mode (plain, with the
 *        deviations, with them as if their axis terms matched) an
 *        assertion other than a default names an observation of the whole
 *        program (rc, requester, compile, compile.log, output, serial,
 *        inject, session, end.run), or the case's namespace holds an
 *        !ExpectError (the table of the preprocessor).  A conflict of
 *        assertions counts as such (the run of the case reports it).
 * @return 1 on its own, 0 shared, -1 out of memory
 */
int cr_case_alone(const struct tc_config *conf, const struct tc_matrix *m, const size_t *cells, size_t ncells,
                  const struct tp_source *src, const char *case_name);

/**
 * @brief Whether a cell's program in a shared run ended cleanly: its session
 *        ended (done), it compiled (ok), it showed no requester, its exit
 *        code is 0 and the result file has the run's end entry without an
 *        incomplete entry before it (the program did not end inside a
 *        block: End in a case, a crash the library's finit survived).
 */
int cr_cell_clean(const struct tj_obs *o);

/**
 * @brief The case whose block is open at the end of a result file (a block
 *        entry of a case without its block end).
 * @param name  its name (cut to namelen)
 * @return 1 = open (name), 0 = none
 */
int cr_open_case(const char *result, size_t len, char *name, size_t namelen);

/*
 * The cells of the side that isolate names (tc_cell_isolated; D5) run every
 * case on its own; the other cells run the shared runs.  A case that a
 * shared run judged clean then runs on its own in the isolated cells, and is
 * judged over all cells of the side: the shared cells from that shared run,
 * the isolated ones from its own run there.  A run on its own (from the
 * start, after an unclean end, the confirmation) runs in every cell of the
 * side, as the case's ctest test does.  Without shared cells (isolate =
 * "on", or every cell named) every case runs on its own.
 */

/** @brief The runs of the plan. */
enum cr_runkind {
    CR_RUN_SHARED,              /* a set of cases in the shared cells */
    CR_RUN_ISOLATED,            /* one case in the isolated cells */
    CR_RUN_ALONE                /* one case on its own in every cell */
};

/** @brief The state of a case in the plan. */
enum cr_cstate {
    CR_C_SHARED,                /* waits for a shared run */
    CR_C_ISOLATED,              /* a shared run ended cleanly: waits for its
                                   run in the isolated cells */
    CR_C_ALONE,                 /* waits for its run on its own */
    CR_C_CONFIRM,               /* judged not passing with a shared run: waits
                                   for its run on its own */
    CR_C_DONE                   /* its verdict counts */
};

/**
 * @brief The plan of the run of all cases: which run comes next, from the
 *        results of the runs before (D3, D5).
 */
struct cr_plan {
    size_t n;                   /* the cases, in the order of the source */
    int isolated;               /* the side has isolated and shared cells */
    int *state;                 /* enum cr_cstate */
    int *verdict;               /* the verdict that counts, -1 none yet */
    int *alone;                 /* the verdict comes from its run on its own */
    int *shared;                /* the verdict with a shared run that its run
                                   on its own replaced, -1 none */
};

/**
 * @brief Start a plan: the cases alone[i] != 0 run on their own, the others
 *        share; isolated: the side has isolated cells besides shared ones.
 * @return 0, -1 = out of memory
 */
int cr_plan_init(struct cr_plan *p, size_t n, const int *alone, int isolated);
void cr_plan_free(struct cr_plan *p);

/**
 * @brief The next run: its kind and its cases set[0..*nset) (set has room
 *        for every case).  The shared cases come first (a single one becomes
 *        a run on its own), then the runs in the isolated cells, then the
 *        runs on their own, each in the order of the source.
 * @return 1 = a run, 0 = every case has its verdict
 */
int cr_plan_next(struct cr_plan *p, size_t *set, size_t *nset, int *kind);

/**
 * @brief A shared run of set[0..nset) that did not end cleanly: open[k] (k
 *        < nopen) is the case open at the end of the k-th cell that did not
 *        end cleanly, or -1 if none was.  The open cases run on their own,
 *        the others of the set again; with a cell without an open case every
 *        case of the set runs on its own.
 */
void cr_plan_unclean(struct cr_plan *p, const size_t *set, size_t nset, const long *open, size_t nopen);

/**
 * @brief A shared run of set[0..nset) that ended cleanly in every cell.
 * @return 1 = judge its cases now; 0 = they wait for their runs in the
 *         isolated cells (CR_C_ISOLATED)
 */
int cr_plan_clean(struct cr_plan *p, const size_t *set, size_t nset);

/**
 * @brief A case judged: v the verdict, alone whether from its run on its
 *        own.  A case judged failed, broken or error with a shared run waits
 *        for its run on its own (CR_C_CONFIRM); any other verdict counts.
 */
void cr_plan_judged(struct cr_plan *p, size_t c, int v, int alone);

/** @brief The worst verdict that counts (cr_verdict_worse); 0 without cases. */
int cr_plan_worst(const struct cr_plan *p);

/**
 * @brief The pass file text (corpus.h) of one cell: configuration TAG,
 *        source the tokenised driver, work= WORK.
 * @param tag     the configuration's tag ("c0")
 * @param work    the run directory
 * @param source  the tokenised driver (its name without .bb2 is the program)
 * @param text    the result (malloc'd)
 * @return 0 = ok, -1 = refused (err: a runner value the environment lacks,
 *         pad with runwrap, an RWE library it does not know, a file whose
 *         selector is malformed), -2 = out of memory
 */
int cr_cell_pass(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                 const struct tc_env *env, const char *tag, const char *work, const char *source,
                 char **text, char *err, size_t errlen);

/**
 * @brief The pass lines of the test-local data of a cell (change
 *        basic-test-local-data, D3): for a file of copy or mktestdata
 *        file=ID.test/NAME|BUILT; for a data source (basic-compile) whose
 *        build configuration (compiler, libraries, runtime error checking,
 *        "same" the cell's value) is the cell's a job of the cell's job
 *        list, build_job=-|BUILT|ID.test/NAME, and for every other build
 *        configuration, in the order the data name them, a compile part
 *        build=ID.test/.build-N|BLITZ2|DEFLIBS|ACIDLIBS|on|off (N from 1)
 *        with the jobs build_job=ID.test/.build-N|BUILT|ID.test/NAME.
 *        NAME is tc_data_result's; the build_job= lines come in the order
 *        of the data sources, so job bNNN is the NNN-th data source.
 * @param built  per entry of conf->data the host file: the tokenised source
 *               of a data source, the file made by mktestdata, the file of
 *               a copy
 * @param text   the lines (malloc'd)
 * @return 0 = ok, -1 = refused (err: a runner value the environment lacks),
 *         -2 = out of memory
 */
int cr_data_pass(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env,
                 const char *const *built, char **text, char *err, size_t errlen);

/**
 * @brief The settings that decide the session of a cell, as one text: two
 *        cells share a session if their keys are the same.  A cell with
 *        serial=on, and a cell the setting isolate names (tc_cell_isolated),
 *        gets a key of its own (the cell's number in it).
 * @return the key (malloc'd), NULL = out of memory
 */
char *cr_session_key(const struct tc_config *conf, const struct tc_matrix *m, size_t cell);

/** @brief What a program's output (NAME.run) holds. */
struct cr_run {
    char *output;               /* the lines but the REQUESTER: lines and rc=N */
    char **req;                 /* the REQUESTER: lines (without the prefix) */
    size_t nreq;
    int has_rc;
    long long rc;
};

/** @brief Split a program's output (NULL: not run). 0 = ok, -1 = out of memory. */
int cr_parse_run(const char *run, struct cr_run *r);
void cr_run_free(struct cr_run *r);

/**
 * @brief Inject's report in a program's output: its lines that start with
 *        "inject:", each with its LF ("" if none); the output keeps them.
 * @return the lines (malloc'd), NULL = out of memory
 */
char *cr_inject_lines(const char *output);

/**
 * @brief Whether the program of a cell runs through inject: pad = "on", or
 *        a runwrap whose path is inject (the placeholder INJECT of env, or
 *        its runner value inject).  The query inject of such a cell holds
 *        inject's report (cr_inject_lines), of any other cell it is not
 *        captured.
 */
int cr_cell_inject(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env);

/**
 * @brief The compile result of the driver in the terms of the source:
 *        "error LINE MESSAGE" with LINE the driver's line becomes "error
 *        SLINE MESSAGE" for a line of the source (where, if not NULL, gets
 *        "FILE:SLINE: MESSAGE"), and for a call or a line of the driver's
 *        own text "error driver:LINE MESSAGE" (where: what the line calls).
 *        Any other result is kept.
 * @return the mapped result (malloc'd), NULL = out of memory
 */
char *cr_compile_mapped(const char *result, const struct tp_driver *d, const char *file, char **where);

/**
 * @brief Print the effective configuration of a test: every key of the top
 *        table and every table key with its value and origin (file:line),
 *        the axes and the cells with their side.
 * @return 0, -1 = out of memory
 */
int cr_print_config(FILE *out, const struct tc_config *conf, const struct tc_matrix *m);

#ifdef __cplusplus
}
#endif

#endif
