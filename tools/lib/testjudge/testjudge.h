/*
 * testjudge.h - the judge of the Blitz tests of the model of change
 * basic-test-runner: a cell of a case from the result file of the test
 * library (read with the codec of shared/basictest) and the other
 * observations of the run, against the assertions of the run
 * configuration (testconf.h), the table of the preprocessor (testpp.h) and
 * the declared deviations of the original.
 *
 * A cell of a case passes if every assertion that applies to it holds
 * (tc_assertions_for: the configured ones, the deviations of the cell when
 * it is judged with them, the defaults rc == 0, requester count == 0,
 * end.case exists, end.run exists) and every check with entries in the
 * blocks of the run (the batch init, the case, the batch cleanup) passed,
 * unless an assertion names the check.  A failed precondition (`require`)
 * or a `broken` entry makes the cell broken; an entry the library wrote
 * outside a block after a `broken` one (a call out of order) belongs to
 * no block.  The checks are `check`, `checkq` (quick values, reported in
 * decimal), `checkflt` (FFP floats, reported in decimal with the delta)
 * and `checkstr` (a value cut to fit the line is reported as its start
 * with "(cut, length N)"); a query of a value compares the 32 bits of the
 * long kinds; `==` and `!=` on a string value compare the length and the
 * CRC32 of the whole value (also of a cut one) with those of the
 * assertion's string, and `contains` and `matches` on a cut value cannot
 * be judged (TJ_ERROR).  An announced runtime error (`expect-error`) must be matched by a requester shown after the
 * announcement whose text holds its message; the matched requesters do
 * not count for the query `requester`, and a case whose last entry is a
 * matched announcement has ended as expected (end.case holds, the default
 * rc == 0 does not apply).  A check without entries has the verdict
 * `not-run` and fails no default: the checks behind a declared crash count
 * as not run.
 *
 * The compile of every data source of ID.test/ (the action basic-compile,
 * change basic-test-local-data) is the query data.FILE, by default == ok;
 * one that does not give what its assertion (or the default) asks makes
 * the cell broken: the setup failed, the tested behaviour was not reached.
 *
 * A program a case started (!RunProgram, change basic-test-local-data) is
 * the query run.NS.NAME: done (its runend entry is there), open (its run
 * entry only: it did not return), not-run; run.NS.NAME.rc its return code
 * (a query of it cannot be judged where the system gave none, `-`) and
 * run.NS.NAME.output what it wrote (tj_obs.runs).  Every program that ran
 * must have returned unless an assertion names its query run.NS.NAME; its
 * return code has no default.
 *
 * The query compile.inject is inject's report of the compiler (a cell whose
 * compiler runs through inject; tj_obs.compile_inject, NULL: not captured).
 * A program run from the editor (editor-run = "on", change
 * basic-test-editor-run) gives no exit code and no requesters: the defaults
 * of rc and requester do not apply in its cell (tc_assertions_for), and
 * tj_editor_run_refused refuses a test with such a cell and an announced
 * runtime error, which only a requester shows.
 *
 * Where the deviations apply, the alternative ends of the cell (testconf.h
 * tc_ends_for) judge the queries their alternatives name instead of the
 * assertions and the defaults: one alternative must hold wholly; the report
 * names the alternative that held (also for a cell that passed), or each
 * alternative with its failed assertions.
 *
 * The verdict of a case over the cells of a side (tj_verdict):
 *   - no deviation declared for the case: every cell must pass (PASSED);
 *   - deviations and fixed-orig (or extension, the test of an extension of
 *     the port: its deviations are what the original does instead): the
 *     cells with the faulty part must pass with their deviations, the
 *     others without (PASSED, "fixed");
 *   - deviations without a fixed issue (an open bug): every cell must pass
 *     with the deviations, the cells without the faulty part as if their
 *     selector matched (REPRODUCED); such a cell that passes without the
 *     deviations fails, since the bug is gone without a fixed issue.
 * A broken cell makes the case BROKEN, never REPRODUCED.  A bug-orig test
 * (tj_test.require_deviation) that declares no deviation at all is BROKEN,
 * open or fixed, unless a cell cannot be judged (ERROR).
 */
#ifndef TESTJUDGE_H
#define TESTJUDGE_H

#include <stddef.h>
#include "testconf.h"
#include "testpp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A requester the run showed. */
struct tj_requester {
    const char *text;           /* its title and body as captured */
    size_t after;               /* the number of result file lines written
                                   before it was shown; (size_t)-1: after
                                   all of them */
};

/** @brief What a started program wrote: the file CORPUS:out/run-K.out. */
struct tj_runout {
    unsigned long k;            /* the number of its !RunProgram */
    const char *text;           /* the file, 0-terminated */
};

/** @brief The compile of a data source of ID.test/ (basic-compile). */
struct tj_data {
    const char *file;           /* its path relative to ID.test/ */
    const char *result;         /* "ok", "error LINE MESSAGE", "none"; NULL: not captured */
};

/** @brief A named compile job of the cell (change named-compile-jobs). */
struct tj_job {
    const char *name;           /* j000, r001, a jobline ID, a data source's file */
    const char *block;          /* its block of tedstub's log (bugorig_block);
                                   NULL or "": not captured */
    const char *result;         /* a jobline job= or resident= job: its compile
                                   result ("ok", "error LINE MESSAGE", "none");
                                   NULL otherwise */
};

/**
 * @brief The observations of one cell's run.  NULL (or has_ = 0): not
 *        captured, which fails any assertion that queries it.
 */
struct tj_obs {
    const char *session;        /* "done", "timeout", "halt" */
    const char *compile;        /* "ok", "error LINE MESSAGE", "none" */
    const char *compile_log;
    const char *output;         /* the printed output */
    const char *serial;
    const char *inject;
    const char *compile_inject; /* inject's report of the compiler (a wrap of inject) */
    int editor_run;             /* the program ran from the editor: its exit
                                   code and its requesters are not captured */
    int has_rc;
    long long rc;
    const struct tj_requester *req;
    size_t nreq;
    int has_result;             /* the result file exists */
    const char *result;         /* its bytes */
    size_t result_len;
    const struct tj_data *data; /* the compiles of the data sources of the cell */
    size_t ndata;
    const struct tj_runout *runs;       /* the output files of the started programs */
    size_t nruns;
    const struct tj_job *jobs;  /* the named compile jobs (compile.log.NAME,
                                   compile.NAME) */
    size_t njobs;
};

/** @brief The result of a cell. */
enum tj_result {
    TJ_PASS,
    TJ_FAIL,
    TJ_BROKEN,
    TJ_ERROR                    /* cannot be judged: a malformed result
                                   file, a check number or query the table
                                   does not have, a conflict of assertions */
};

/** @brief Messages, one line each. */
struct tj_msgs {
    char **v;
    size_t n;
};

void tj_msgs_free(struct tj_msgs *m);

/** @brief What the judge needs besides the observations. */
struct tj_test {
    const char *file;           /* the source's name in the report (ID.test.bas) */
    const struct tp_source *src;    /* the table of the preprocessor */
    const struct tc_config *conf;
    const struct tc_matrix *m;
    const char *case_name;
    int require_deviation;      /* a bug-orig test: one without any declared
                                   deviation is broken (it would not show its
                                   bug), whatever its cells show */
    const char *const *cases;   /* the cases the driver called (the run of all
                                   cases of a source, caserun.h); NULL: the
                                   judged case alone.  A block of one of the
                                   others belongs to that case, its entries
                                   are not judged; a block of a case the
                                   driver did not call fails the cell */
    size_t ncases;
};

/**
 * @brief Judge one cell of a case.
 * @param mode  TC_PLAIN, TC_DEVIATED or TC_DEVIATED_ANY (testconf.h)
 * @param msgs  why it did not pass (appended)
 * @return enum tj_result, -1 = out of memory
 */
int tj_judge_cell(const struct tj_test *t, size_t cell, int mode, const struct tj_obs *obs,
                  struct tj_msgs *msgs);

/**
 * @brief A test with a cell whose program runs from the editor
 *        (editor-run = "on") and an announced runtime error (!ExpectError,
 *        judged by its requester, which such a run does not capture) is
 *        refused.
 * @param file  the source's name in the message
 * @return 0 = none, 1 = refused (err: "FILE:LINE: TEXT", the line of the
 *         announcement)
 */
int tj_editor_run_refused(const char *file, const struct tp_source *src, const struct tc_config *conf,
                          const struct tc_matrix *m, char *err, size_t errlen);

/**
 * @brief Whether a case runs on its own in the run of all cases: in a cell
 *        of cells[0..ncells), in the one mode tj_verdict judges that cell
 *        in (no deviation applies: plain; fixed-orig or extension: the
 *        faulty cell with the deviations, the others plain; an open bug:
 *        the faulty cell with the deviations, the others as if their axis
 *        terms matched), an assertion other than a default names an
 *        observation of the whole program (rc, requester, compile,
 *        compile.log, compile.inject, output, serial, inject, session,
 *        end.run), or the case's namespace holds an
 *        !ExpectError or a !RunProgram (the table of the preprocessor), or
 *        a !RunProgram stands outside every case (a procedure, the batch
 *        init or cleanup).  A conflict of
 *        assertions counts as such (the run of the case reports it).
 * @return 1 on its own, 0 shared, -1 out of memory
 */
int tj_case_alone(const struct tc_config *conf, const struct tc_matrix *m, const size_t *cells, size_t ncells,
                  const struct tp_source *src, const char *case_name);

/**
 * @brief The key by which the case tests of a cell are grouped into
 *        bundles: its session key (tc_session_key), for a cell of
 *        compiler = "ab3" with the line "compiler=ab3" before it, so the
 *        cases of blitz2-ab3 share bundles with each other and never with
 *        those of the port (change ab3-test-model D14).
 * @return malloc'd, NULL = out of memory
 */
char *tj_bundle_key(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env);

/**
 * @brief Whether a case may be a member of a run shared with other sources
 *        (a bundle), and the cell it would run in: the source has exactly
 *        one cell on the side port (none needs original material, and a
 *        cell of amiblitz3 is not on that side: never bundled), that
 *        cell is not isolated (tc_cell_isolated), its `serial` is not "on",
 *        it does not run from the editor (`editor-run`), the test has no
 *        test-local data (`data`) and sets no `hang-after`, the cell's
 *        session key (tc_session_key with env) is the key of a
 *        configuration with only `timeout` set to the environment's value,
 *        to 30 or to 60, and the case would not run on its own in the run
 *        of all cases (tj_case_alone).
 * @param cell  the cell (set when 1 is returned)
 * @return 1 may be bundled, 0 not, -1 out of memory
 */
int tj_bundle_case(const struct tc_config *conf, const struct tc_matrix *m, const struct tp_source *src,
                   const struct tc_env *env, const char *case_name, size_t *cell);

/** @brief Whether a deviation is declared that applies to the case. */
int tj_case_has_deviation(const struct tj_test *t);

/** @brief The verdict of a case over the cells of a side. */
enum tj_verdict {
    TJ_V_PASSED = 0,            /* also: fixed */
    TJ_V_REPRODUCED = 1,        /* an open bug shows as declared */
    TJ_V_ERROR = 2,
    TJ_V_BROKEN = 3,
    TJ_V_FAILED = 4
};

/**
 * @brief The verdict of a case: cells[i] (the cell numbers of the side)
 *        run with obs[i].
 * @param msgs  the report (appended): a line per cell that did not do what
 *              it should, then its messages, indented
 * @return enum tj_verdict, -1 = out of memory
 */
int tj_verdict(const struct tj_test *t, const size_t *cells, const struct tj_obs *obs, size_t n,
               struct tj_msgs *msgs);

#ifdef __cplusplus
}
#endif

#endif
