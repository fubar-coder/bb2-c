/*
 * testreg.h - the registration of the Blitz tests of the model of change
 * basic-test-runner at build time (design D7; tools/testreg): from the
 * sources (`ID.test.bas`) and their resolved configurations, without the
 * emulator, one ctest test per case and side.
 *
 * For each source: the preprocessor and the form check (testpp.h), the
 * configuration (testconf.h) and its cells; the check queries of its
 * assertions and deviations against the table and the case selectors
 * against the cases.  A case gets a test for the side without original
 * material ("port": its cells that need no original material) and one for
 * the side with it ("originals"); a side without cells is not
 * registered.  Names: PREFIX + ID ("-" as "_") + "__" + CASE, and
 * "_originals" behind it for the side with original material; only
 * [a-z0-9_]; a name given twice is an error.  Labels: "originals" for the
 * side with original material, else "bug-orig" and "emulator" for a case
 * with a declared deviation of an open bug (no fixed-orig or extension) and "emulator"
 * for the others, then the configuration's `labels`; the time limit is
 * the sum of the cells' `timeout` (else the default) plus an extra; the
 * cost and the fixtures are the configuration's `cost` and `fixtures`,
 * and the fixtures of the build that a cell of the side needs (the
 * option cell_fixtures: the build of blitz2.s for compiler=orig195).
 */
#ifndef TESTREG_H
#define TESTREG_H

#include <stddef.h>
#include <stdio.h>
#include "testconf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A list of strings. */
struct treg_strs {
    char **v;
    size_t n;
};

void treg_strs_free(struct treg_strs *s);

/**
 * @brief Add the names of a token list (lines "HEX NAME", as
 *        tests/bug-orig/tokens.txt; blank lines and lines from "#" left out).
 * @return 0, -1 = error (err)
 */
int treg_tokens_load(const char *file, struct treg_strs *names, char *err, size_t errlen);

/** @brief A registered test. */
struct treg_test {
    char *name;
    char *source;               /* the source, absolute */
    char *case_name;
    int originals;              /* the side with original material */
    struct treg_strs labels;
    long long timeout;
    int has_cost;
    long long cost;
    struct treg_strs fixtures;
    struct treg_strs cells;     /* the names of the side's cells */
};

/** @brief What the sources gave. */
struct treg_result {
    struct treg_test *t;
    size_t n;
    struct treg_strs deps;      /* every file read */
    struct treg_strs errors;    /* "FILE:LINE: TEXT" */
};

void treg_free(struct treg_result *r);

/** @brief The settings of a registration. */
struct treg_opts {
    const char *prefix;         /* [a-z0-9_]* */
    const struct tc_env *env;
    const char *const *tokens;  /* the names of the tokens (case names) */
    size_t ntokens;
    long long timeout_default;  /* a cell without timeout */
    long long timeout_extra;    /* added to the sum */
    const char *const *cell_fixtures;   /* "SELECTOR|FIXTURE": a test whose side
                                           has a cell the selector matches
                                           requires FIXTURE (the build of
                                           blitz2.s for compiler=orig195) */
    size_t ncell_fixtures;
};

/** @brief Whether spec is SELECTOR|FIXTURE with a valid cell selector (err: why not). */
int treg_cell_fixture_ok(const char *spec, char *err, size_t errlen);

/**
 * @brief Register the tests of one source; its errors go to r->errors.
 * @return 0, 1 = errors, -1 = out of memory
 */
int treg_add_source(const char *source, const struct treg_opts *o, struct treg_result *r);

/** @brief The names given twice; 0 = none, 1 = errors added, -1 = out of memory. */
int treg_check_names(struct treg_result *r);

/**
 * @brief Write the ctest include file (TEST_INCLUDE_FILES): an add_test()
 *        and a set_tests_properties() per test; the test runs COMMAND...
 *        --source SOURCE --case CASE --side port|originals.
 * @return 0, -1 = a value that cannot be quoted or a write error
 */
int treg_write_include(FILE *f, const struct treg_result *r, const char *const *command, size_t ncommand);

/** @brief Write a depfile "TARGET: DEP..." (Makefile syntax, as ninja reads it). */
int treg_write_depfile(FILE *f, const char *target, const struct treg_result *r);

#ifdef __cplusplus
}
#endif

#endif
