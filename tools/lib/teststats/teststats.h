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
 *   tests     per test: runs (passed, failed, skipped), min / median /
 *             max duration of the runs that were not skipped
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
 *             rounded up, at least 1)
 */
#ifndef TESTSTATS_H
#define TESTSTATS_H

#include <stddef.h>
#include <stdio.h>

struct teststats;

struct teststats_options {
    double factor;              /* time limit = max timed x factor (default 3) */
    double floor;               /* ... at least this (seconds, default 30) */
    int tsv;                    /* TSV instead of text tables */
};

#define TESTSTATS_FACTOR 3.0
#define TESTSTATS_FLOOR  30.0

struct teststats *teststats_new(void);
void teststats_free(struct teststats *t);

/* one JUnit file (len bytes of text, named source in messages) as a run.
   Problems go to diag: a malformed file (-1, nothing of it counts), a
   testcase left out, a broken @stat line (left out).  The number of
   problems, -1 for a file refused or memory out. */
int teststats_add(struct teststats *t, const char *source, const char *text, size_t len,
                  FILE *diag);

/* the number of runs (files) added */
size_t teststats_runs(const struct teststats *t);

/* the report; -1 if memory ran out */
int teststats_report(const struct teststats *t, const struct teststats_options *o, FILE *out);

#endif
