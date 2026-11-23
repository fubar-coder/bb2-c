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
 * blitz2.s).  With require_deviation (a bug-orig test) a test without any
 * declared deviation is broken: it would not show its bug.
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
 * @brief Run (or show) one case of a test.
 * @param out  the report, the diagnostics, the @stat lines
 * @return the verdict as above
 */
int caserun(const struct caserun_opts *o, FILE *out);

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
 * @brief The settings that decide the session of a cell, as one text: two
 *        cells share a session if their keys are the same.  A cell with
 *        serial=on gets a key of its own (the cell's number in it).
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
