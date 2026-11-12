/*
 * bugorigcmp.h - the bug-orig tests (tests/bug-orig, tests/README.md
 * "bug-orig tests") in the terms of the shared test model (OpenSpec change
 * bug-orig-test-model): the observation files, their wildcard lines with a
 * reason, the comparison of an observation with the recorded original, the
 * projection of a test onto the cases of tools/lib/libscmp (fix-scope) and
 * the acceptance of two recording runs.
 *
 * An observation file is text in sections:
 *
 *   # NOTE                    optional comment lines before the first
 *   [NAME]                    section (the first is the note)
 *   LINE...                   the section's lines (NAME: a-z, 0-9)
 *
 * The Blitz tests (tests/corpus/runner/bugorig.h) have the sections
 * session, compile, run, blitz2 and log, the TED tests
 * (tests/ted-ab/runner/tedab.h, tedab_orig_format) state and problems.
 * The kinds of file:
 *   ID.expect   the expectation: a line ending in `*` matches every line
 *               that begins with the text before it, a line `...` any
 *               number of lines (also none)
 *   ID.orig     the recorded observation of the original: its lines are
 *               literal, except a wildcard line (`*` at the end, `...`)
 *               directly behind a comment line "# varies: REASON" (the
 *               line varies from run to run for a reason outside the tested
 *               behaviour); a wildcard line without it, a "# varies:" line
 *               without a reason or without a wildcard line behind it is an
 *               error.  The "# varies:" lines are no part of the section
 *   a run       an observation as a runner wrote it (corpusrun, tedab with
 *               record=write): literal lines
 */
#ifndef BUGORIGCMP_H
#define BUGORIGCMP_H

#include <stddef.h>
#include <stdio.h>
#include "libscmp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum bugorigcmp_kind { BUGORIGCMP_EXPECT, BUGORIGCMP_ORIG, BUGORIGCMP_RUN };

struct bugorigcmp_sec {
    char *name;                 /* without the brackets */
    char *text;                 /* its lines, each ending in LF, without "# varies:" lines */
    char *raw;                  /* the same with them (as in the file) */
    unsigned line;              /* the line of its header, from 1 */
    int wild;                   /* ID.orig: it has a wildcard line */
};

struct bugorigcmp_obs {
    char *note;                 /* the first comment line before the first section,
                                   without "# " (NULL: none) */
    struct bugorigcmp_sec *sec;
    size_t n;
};

/**
 * @brief Parse an observation file.
 * @param text  the file (0-terminated)
 * @param path  its name in messages
 * @param kind  what it is (the rule of the wildcard lines for BUGORIGCMP_ORIG)
 * @param err   the message of an error, "PATH:LINE: TEXT"
 * @return 0 = ok, -1 = out of memory, -2 = malformed (err); o is empty
 *         after an error
 */
int bugorigcmp_parse(const char *text, const char *path, enum bugorigcmp_kind kind,
                     struct bugorigcmp_obs *o, char *err, size_t errlen);
void bugorigcmp_free(struct bugorigcmp_obs *o);

/** @brief The section NAME of o, or NULL. */
const struct bugorigcmp_sec *bugorigcmp_get(const struct bugorigcmp_obs *o, const char *name);

/**
 * @brief The rule of the wildcard lines of an ID.orig (see above) on its
 * text, without parsing the sections.
 * @return 0 = ok, -2 = broken (err: "PATH:LINE: TEXT")
 */
int bugorigcmp_check_varies(const char *text, const char *path, char *err, size_t errlen);

/** @brief Remove the "# varies:" lines of an ID.orig from text (in place). */
void bugorigcmp_strip_varies(char *text);

/** @brief A line of a pattern is a wildcard: it ends in `*` or is `...`. */
int bugorigcmp_is_wild(const char *line, size_t n);

/**
 * @brief Lines (each ending in LF) against a pattern: a line ending in `*`
 * matches every line that begins with the text before it, a line `...`
 * any number of lines (also none), every other line itself.
 * @return 1 = they match, 0 = not
 */
int bugorigcmp_matches(const char *want, const char *got);

/**
 * @brief The first line of got where the pattern want does not match it.
 * @param w, g  that line of want and of got for messages (bytes outside
 *              32..126 as \xNN, cut with ...; "(the end)" for none)
 * @return its number in got (from 1), 0 = they match
 */
unsigned bugorigcmp_first_diff(const char *want, const char *got, char *w, size_t wlen, char *g,
                               size_t glen);

/**
 * @brief An observation against the recorded original: every section of
 * orig must match the observed one (a section missing there is empty),
 * and got must have no section orig lacks.
 * @param msg  for a difference: "[NAME] line N: recorded \"...\", observed
 *             \"...\"" (the first) or "[NAME]: observed, not in the
 *             recorded original"
 * @return 0 = it matches, 1 = it differs (msg)
 */
int bugorigcmp_compare(const struct bugorigcmp_obs *orig, const struct bugorigcmp_obs *got,
                       char *msg, size_t msglen);

/* --- the cases of the shared test model (fix-scope) ---------------------
 *
 * A test is projected onto lines of a libscmp case file (libscmp.h), one
 * case per section, ROUTINE the test's ID, CHECK the section, OCC 1:
 *   open test (no "; @fixed #N")
 *     ID|SECTION||1             a section of ID.expect
 *     ID|SECTION|orig|1         a section of ID.orig
 *   fixed test ("; @fixed #N" in ID.bas, or the directive in ID.drv)
 *     ID|SECTION||1 [fixed #N]  a section of ID.expect
 *     ID|SECTION|orig|1 [orig]  a section of ID.orig, paired with
 *     ID|SECTION|orig|1 [fixed #N]  the text "judged by ID.expect"
 * (openspec/changes/bug-orig-test-model/design.md, findings: with this the
 * open test's cases become cases of issue N when the fix adds the header).
 * A TED test's expectation is its script's "; @expect" lines, as the
 * section "expect".  A value is the section's text in quotes, bytes
 * outside 32..126, the quote and the backslash as \xHH; a text longer than
 * BUGORIGCMP_LONG escaped is "bytes N fnv HHHHHHHH" (its length and its
 * FNV-1a, 32 bit). */

enum { BUGORIGCMP_LONG = 200 };

/**
 * @brief The case lines of one test.
 * @param out     where they go
 * @param id      the test's ID
 * @param source  the text of ID.bas (Blitz) or ID.drv (ted: TED)
 * @param ted     a TED test
 * @param expect  the text of ID.expect (Blitz; NULL for TED, or missing:
 *                an error for a Blitz test)
 * @param orig    the text of ID.orig, or NULL (none)
 * @param path    the test's directory and ID in messages (DIR/ID)
 * @return 0 = ok, -1 = out of memory, -2 = malformed (err)
 */
int bugorigcmp_project_test(FILE *out, const char *id, const char *source, int ted,
                            const char *expect, const char *orig, const char *path, char *err,
                            size_t errlen);

/**
 * @brief The case file of every test below root (ROOT/AREA/ID.bas with
 * ID.expect, ROOT/AREA/ID.drv; ID.orig beside them; not ROOT/selftest),
 * in the order of the IDs, header "# capture bug-orig 1".  A root that
 * does not exist has no tests.
 * @param text  the file (malloc'd, 0-terminated), *len its length
 * @return 0 = ok, -1 = out of memory, -2 = malformed or unreadable (err)
 */
int bugorigcmp_project_tree(const char *root, char **text, size_t *len, char *err,
                            size_t errlen);

/**
 * @brief fix-scope of the bug-orig tests: the projection of base_root (of
 * main) against branch_root (of the branch), judged by libscmp_fix_scope;
 * a summary line "fix-scope: bug-orig: N cases changed, K not of the
 * issues ...".
 * @return 0 = only the named issues' cases changed, 1 = a case reported,
 *         2 = error (a malformed or unreadable test, on err)
 */
int bugorigcmp_fix_scope_trees(const char *base_root, const char *branch_root,
                               const struct libscmp_issues *is, FILE *out, FILE *err);

/* --- recording (the target bug-orig-record) --------------------------- */

/**
 * @brief The recorded original of a test from two recording runs of the
 * original (the second with another memory layout where the runner can
 * vary it).  The runs are compared as cases (one ID|SECTION|orig|1 per
 * section) by libscmp_accept: if they differ, "test not deterministic:
 * ID", the differing cases and the first differing line of each section
 * ("ID: [SECTION] line N: run 1 \"...\", run 2 \"...\"").  A section of
 * the committed file with wildcard lines is kept as it is (with its
 * "# varies:" lines) if both runs match it, and is no part of the
 * comparison; if a run does not match it, that is reported.  A line of a
 * run that would read as a wildcard is refused.  The new file is the note
 * of run 1 and its sections in its order; it must read back as them.
 * @param id      the test (the messages, the name of the cases)
 * @param run1, run2  the runs' observations, path1, path2 their names
 * @param old     the committed ID.orig, or NULL; old_path its name
 * @param out     the new ID.orig (malloc'd), NULL unless 0
 * @param report  the messages of a refusal
 * @return 0 = *out, 1 = refused, -1 = out of memory, -2 = a malformed
 *         file (err)
 */
int bugorigcmp_accept(const char *id, const char *run1, const char *path1, const char *run2,
                      const char *path2, const char *old, const char *old_path, char **out,
                      FILE *report, char *err, size_t errlen);

#ifdef __cplusplus
}
#endif

#endif
