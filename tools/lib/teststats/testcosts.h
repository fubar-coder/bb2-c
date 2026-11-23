/*
 * testcosts.h - the measured COST of the tests, the checked-in file
 * tests/data/test-costs.txt (written by testinfo costs from ctest's JUnit
 * files, teststats.h teststats_costs; read by tools/testreg and by
 * cmake/test-cost.cmake): one line "NAME COST" per test (NAME a ctest
 * name of [A-Za-z0-9_.-], COST seconds, an integer of at least 1), sorted
 * by name, each name once; lines from "#" and empty lines are comments.
 */
#ifndef TESTCOSTS_H
#define TESTCOSTS_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief An entry. */
struct tcost {
    char *name;
    unsigned cost;
    unsigned line;              /* its line in the file read, 0: none */
};

/** @brief The entries, sorted by name. */
struct tcosts {
    struct tcost *v;
    size_t n;
};

/**
 * @brief Read entries from text (file: its name in the messages).  With
 *        sorted, the entries must be sorted by name (the checked-in file);
 *        else they are sorted (the output of teststats_costs is).
 * @return 0, -1 = refused (err: "FILE:LINE: TEXT"; a malformed line, a
 *         name twice, out of order), -2 = out of memory
 */
int tcosts_parse(const char *file, const char *text, int sorted, struct tcosts *c, char *err, size_t errlen);

/** @brief tcosts_parse of a file (sorted); a missing file: -1 with err. */
int tcosts_load(const char *file, struct tcosts *c, char *err, size_t errlen);

void tcosts_free(struct tcosts *c);

/** @brief The entry of name, or NULL. */
const struct tcost *tcosts_find(const struct tcosts *c, const char *name);

/**
 * @brief Put the entries of from into c, replacing an entry of the same
 *        name (a refreshed measurement); the others of c stay.
 * @return 0, -1 = out of memory
 */
int tcosts_merge(struct tcosts *c, const struct tcosts *from);

/** @brief Write the file: a header comment and the entries. 0, -1 = error. */
int tcosts_write(FILE *f, const struct tcosts *c);

#ifdef __cplusplus
}
#endif

#endif
