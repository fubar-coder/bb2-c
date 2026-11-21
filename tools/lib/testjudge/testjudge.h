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
 * blocks of the run (the batch init, the case, the batch end) passed,
 * unless an assertion names the check.  A failed precondition (`require`)
 * or a `broken` entry makes the cell broken.  An announced runtime error
 * (`expect-error`) must be matched by a requester shown after the
 * announcement whose text holds its message; the matched requesters do
 * not count for the query `requester`, and a case whose last entry is a
 * matched announcement has ended as expected (end.case holds, the default
 * rc == 0 does not apply).  A check without entries has the verdict
 * `not-run` and fails no default: the checks behind a declared crash count
 * as not run.
 *
 * The verdict of a case over the cells of a side (tj_verdict):
 *   - no deviation declared for the case: every cell must pass (PASSED);
 *   - deviations and fixed-orig: the cells with the faulty part must pass
 *     with their deviations, the others without (PASSED, "fixed");
 *   - deviations without a fixed issue (an open bug): every cell must pass
 *     with the deviations, the cells without the faulty part as if their
 *     selector matched (REPRODUCED); such a cell that passes without the
 *     deviations fails, since the bug is gone without a fixed issue.
 * A broken cell makes the case BROKEN, never REPRODUCED.
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
    int has_rc;
    long long rc;
    const struct tj_requester *req;
    size_t nreq;
    int has_result;             /* the result file exists */
    const char *result;         /* its bytes */
    size_t result_len;
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
};

/**
 * @brief Judge one cell of a case.
 * @param mode  TC_PLAIN, TC_DEVIATED or TC_DEVIATED_ANY (testconf.h)
 * @param msgs  why it did not pass (appended)
 * @return enum tj_result, -1 = out of memory
 */
int tj_judge_cell(const struct tj_test *t, size_t cell, int mode, const struct tj_obs *obs,
                  struct tj_msgs *msgs);

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
