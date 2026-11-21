/*
 * cdcmp.h - the comparison of a CD result table (corpusrun cd) with a
 * reference table (tests/data/blitz2-cd.tsv, format: tools/lib/cdtable).
 *
 * What is compared is the compile result: result, line and message.  The
 * size and the hash of an executable (the fields of an `ok` row) are the
 * original's, reference data: a difference there is reported and counted
 * (`exe_differ`) and is no difference of the comparison (OpenSpec change
 * originals-compare-outputs: against the original only outputs and errors
 * are compared).
 */
#ifndef CDCMP_H
#define CDCMP_H

#include <stddef.h>

struct cdcmp_stats {
    size_t sources;             /* rows of the reference */
    size_t equal;               /* same compile result (the executable may differ) */
    size_t different;           /* another compile result */
    size_t missing;             /* in only one of the two tables */
    size_t exe_differ;          /* of the equal ones: another size or hash (information) */
};

/*
 * Compare the tables ref and out (the texts; refname and outname for the
 * report).  The report (malloc'd, one line per difference and a summary
 * line) in *report.  0 = no difference of the compile results, 1 = a
 * difference or a missing row, 2 = a table is malformed (the report says
 * which).
 */
int cdcmp_compare(const char *refname, const char *outname, const char *ref, const char *out,
                  char **report, struct cdcmp_stats *st);


#include "previntended.h"

/*
 * The comparison of the table of the previous commit's compiler (prev) with
 * that of this build's (cur) for the same sources (the CD, with the same
 * libraries): every field of a row - result, line, message, size, the hash
 * of the executable with the trap blocks zeroed - must be the same, and a
 * row must be in both.  A row that differs is judged against the intended
 * list (previntended.h, TEST = the ctest name testname, NAME = the path of
 * the source): listed, it is reported "(intended)", not listed it is an
 * error; a listed row that does not differ is stale (an error too).  The
 * report (malloc'd) ends, if some row differed, with the line
 * `prev differs in: PREV_INTENDED="..."`.  0 = no difference that is not
 * intended and no stale entry, 1 = one, 2 = a table is malformed.
 */
int cdcmp_prev(const char *testname, const char *curname, const char *prevname, const char *cur,
               const char *prev, struct prevint *intended, char **report, struct cdcmp_stats *st);

#endif
