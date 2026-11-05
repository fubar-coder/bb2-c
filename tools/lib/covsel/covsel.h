/*
 * covsel.h - the tests a change can affect (tools/covsel, openspec change
 * test-coverage, D6-D8), from the coverage of a coverage run (covmap.h).
 *
 * Inputs, all as text in memory:
 *   - the map: the lcov tracefile of each test (covmap's lcov/<test>.info,
 *     any lcov tracefile of that test; its TN: the test's name or lcov's
 *     form of it, covutil.h), read into the functions of each
 *     file (path relative to the root; files outside it are left out),
 *     their line ranges (FN: with a last line; without one the range is
 *     not known) and the tests that executed them (FNDA: above 0); a test
 *     whose tracefile has no such function is not named by the map; and
 *     the commit the coverage was made from (covmap's commit file);
 *   - the rules (tools/covsel/rules.txt): per line a path pattern and
 *     what a change of a matching file selects; the first matching rule
 *     counts, a file no rule matches selects all tests.  Patterns: '*'
 *     any characters but '/', '**' any characters, "**" followed by '/'
 *     also no directory at all, '?' one character but '/'.  Selectors:
 *       all          all tests
 *       none         no test
 *       coverage     the tests that executed a changed function (map);
 *                    a changed line outside every function, or a file
 *                    whose functions have no line ranges: the tests of
 *                    all functions of the file; a file not in the map:
 *                    all tests
 *       header       the source of each object that depends on the file
 *                    (ninja -t deps) counts as changed as a whole, by its
 *                    own rule (a measured source: the tests of all its
 *                    functions; glue or an unknown file: all); a file no
 *                    object depends on: all
 *       label:L      the tests with ctest label L
 *       name:PATTERN the tests whose name matches PATTERN
 *   - the tests: ctest --show-only=json-v1 of the build, and of its host
 *     project (each host test X is test host/X of the map and is run by
 *     the test host_emulator if it has the label emulator, else by host);
 *   - the changes: git diff -U0 against the map's commit (line numbers of
 *     the map's sources), and files git does not track yet (new files);
 *   - ninja -t deps of the builds.
 * Tests of the build that the map does not name are always selected (new
 * tests, tests that ran no measured code).  A stale map (its commit not
 * an ancestor of the base) selects all tests.
 *
 * The result: the names of the tests to run, for ctest --tests-from-file.
 */
#ifndef COVSEL_H
#define COVSEL_H

#include <stddef.h>
#include <stdio.h>
#include "covutil.h"

struct covsel;

struct covsel *covsel_new(const char *root);
void covsel_free(struct covsel *s);

/* inputs; each returns 0, or -1 with a message in err */
int covsel_read_lcov(struct covsel *s, const char *test, const char *text, char *err,
                     size_t errsz);
int covsel_set_commit(struct covsel *s, const char *text, char *err, size_t errsz);
int covsel_read_rules(struct covsel *s, const char *text, char *err, size_t errsz);
int covsel_read_tests(struct covsel *s, const char *json, int host, char *err, size_t errsz);
int covsel_read_deps(struct covsel *s, const char *text, char *err, size_t errsz);
int covsel_read_diff(struct covsel *s, const char *text, char *err, size_t errsz);
/* a file git does not track: a new file */
int covsel_add_new_file(struct covsel *s, const char *path);
/* the map's commit is not an ancestor of the base */
void covsel_set_stale(struct covsel *s);

/* the map's commit (after covsel_set_commit) */
const char *covsel_commit(const struct covsel *s);

/* select; the reasons go to log (may be NULL); 0, or -1 if out of memory */
int covsel_run(struct covsel *s, FILE *log);
/* the selected tests, sorted */
size_t covsel_count(const struct covsel *s);
const char *covsel_name(const struct covsel *s, size_t i);

/* the pattern of the rules against a path (exposed for the tests) */
int covsel_match(const char *pattern, const char *path);

#endif
