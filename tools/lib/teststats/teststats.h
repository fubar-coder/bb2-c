/*
 * teststats.h - statistics of test runs from ctest's JUnit files
 * (junit.h) and the runners' @stat lines in them (statline.h), for
 * basing time limits and the order of the tests on measurements.
 *
 * Every JUnit file is one run of the tests in it.  A test's sessions are
 * the @stat lines of its <system-out>, grouped by config (if its lines
 * name more than one test=, each config gets "TEST:" in front).  The
 * report (plain text tables, or TSV: a line "#SECTION<TAB>COLUMN..." per
 * section, then "SECTION<TAB>VALUE..." per row):
 *
 *   tests     per test: runs (passed, failed, skipped, served), min /
 *             median / max duration of the runs that were neither skipped
 *             nor served
 *   sessions  per test and config: sessions, how the snapshot mode
 *             started them, max session time (without the slot wait),
 *             max of the time counted against the limit, max slot wait,
 *             sessions ended by timeout, the limit, the verdicts seen
 *   flaky     tests whose ctest result differs between runs; tests and
 *             configs whose verdict (the last one of a run) or whose end
 *             (by timeout or not) differs between runs
 *   limits    per test and config: sessions that end normally get max
 *             timed x factor, at least the floor (whole seconds); those
 *             that ended by timeout in every session of every run (the
 *             bug hangs) are listed as such, without a number (the
 *             lines do not tell when the last output came); per test
 *             the largest suggestion of its configs
 *   cost      per test: a COST for ctest, the median duration (seconds,
 *             rounded up, at least 1) of the same runs
 *
 * A served run is a testcase whose output holds an @served line: a case
 * test served from its bundle (it started no session, its bundle ran the
 * sessions):
 *
 *   @served v=1 test=T bundle=B
 *
 * the fields separated by one blank, in any order; a key it does not know
 * is ignored; a line with a key twice, without one of the three, with
 * another version, an empty value or a value of other characters than
 * [A-Za-z0-9._,:/+-] is reported as a broken @served line and left out
 * (the run counts as not served).
 */
#ifndef TESTSTATS_H
#define TESTSTATS_H

#include <stddef.h>
#include <stdio.h>
#include "testcosts.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The collected runs (opaque; teststats_new, teststats_free). */
struct teststats;

/** @brief The options of the report. */
struct teststats_options {
    double factor;              /* time limit = max timed x factor (default 3) */
    double floor;               /* ... at least this (seconds, default 30) */
    int tsv;                    /* TSV instead of text tables */
};

#define TESTSTATS_FACTOR 3.0
#define TESTSTATS_FLOOR  30.0

/** @brief New, empty statistics; NULL if out of memory. */
struct teststats *teststats_new(void);

/** @brief Free the statistics. */
void teststats_free(struct teststats *t);

/**
 * @brief Add one JUnit file as a run.
 *
 * Problems go to diag: a malformed file (-1, nothing of it counts), a
 * testcase left out, a broken @stat line (left out).
 *
 * @param t       the statistics
 * @param source  the file's name, in messages
 * @param text    the file
 * @param len     its length
 * @param diag    the problems
 * @return the number of problems, -1 for a file refused or memory out
 */
int teststats_add(struct teststats *t, const char *source, const char *text, size_t len,
                  FILE *diag);

/** @brief The number of runs (files) added. */
size_t teststats_runs(const struct teststats *t);

/**
 * @brief Write the report (the sections above).
 * @param t    the statistics
 * @param o    the options (may be NULL)
 * @param out  the report
 * @return 0 = ok, -1 if memory ran out
 */
int teststats_report(const struct teststats *t, const struct teststats_options *o, FILE *out);

/**
 * @brief The measured COST of the tests (tests/data/test-costs.txt;
 *        testinfo costs): a line "NAME COST" per test that has a session
 *        started warm (snapshot=restored) in the runs, sorted by name.  The
 *        COST of a run is the sum of its sessions' times without the slot
 *        wait (`session`), a session that did not start warm (cold,
 *        first-save, fallback: once per emulator slot and ctest run, not a
 *        property of the test; single) counted as the median of the test's
 *        warm sessions; the COST is the median over the runs that have
 *        sessions, rounded up, at least 1.  A test without a warm session
 *        (and a test without sessions, and one whose configuration only it
 *        uses: single) gets no line: it keeps the COST the build gives it
 *        without a measurement.  A served run (see the top) is no run
 *        here: its sessions, if any, do not count; a test whose every run
 *        was served gets no line either.
 * @return 0 = ok, -1 if memory ran out
 */
int teststats_costs(const struct teststats *t, FILE *out);

/**
 * @brief teststats_costs, a test whose every run was served keeping its
 *        entry of the previous file of measured COSTs.
 * @param t         the statistics
 * @param previous  the previous entries (NULL: none)
 * @param out       the lines
 * @param left_out  out (may be NULL): the tests whose every run was served
 *                  and that previous has no entry of (they get no line)
 * @return 0 = ok, -1 if memory ran out
 */
int teststats_costs_previous(const struct teststats *t, const struct tcosts *previous, FILE *out,
                             size_t *left_out);


#ifdef __cplusplus
}
#endif

#endif
