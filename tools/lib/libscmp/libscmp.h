/*
 * libscmp.h - the judge of the library tests (OpenSpec change
 * libs-tests-capture, design D3/D4): the cases a test program writes with
 * one library set (libs/tests/capture.[ch]) against the committed expected
 * data libs/tests/expected/NAME.txt.
 *
 * A file (a run's output or expected data) is text, one case per line
 * under a header:
 *
 *   # capture NAME 1
 *   KEY<TAB>VALUE
 *   KEY [orig]<TAB>VALUE              expected data only
 *   KEY [fixed #N]<TAB>VALUE          expected data only
 *
 * NAME is the program ([A-Za-z0-9_.-], the file is NAME.txt), 1 the
 * version of the format.  A KEY is ROUTINE|CHECK|VECTOR|OCC (OCC decimal
 * from 1, no leading zero) or BLOCK:N (a block of LIBSCMP_BLOCK
 * observations, N decimal from 0: observations N*4096 to N*4096+COUNT-1,
 * counted from 0).  The parts ROUTINE, CHECK and VECTOR hold printable
 * ASCII (0x20-0x7E) with the escapes \t (TAB), \| (|), \\ (\) and \xHH
 * (two upper-case hex digits) for every other byte; an escape is only
 * these and only where needed, so a key has one spelling and keys compare
 * as text.  A VALUE is printable ASCII, not empty, and opaque to the
 * judge except for three forms: "not captured: REASON" (a part that cannot
 * be reproduced: two such values pass whatever the reasons), "lost ..."
 * (a stated register lost: refused by an accept) and the "COUNT ..." of a
 * block (its observation range in the messages).  No other byte (a TAB in
 * a value, CR, a byte from 0x80) may occur; an empty line, a missing or
 * second header, a malformed key or qualifier, a duplicate key, a key both
 * unqualified and qualified and two [fixed] cases of one key make a file
 * malformed (reported with its line).  A qualifier is an error in a run's
 * output.  The last line may lack its newline.
 *
 * For a set (obj: the C build, orig: the original) the cases of the
 * expected data that apply are the unqualified ones and the [fixed #N]
 * (obj) or [orig] (orig) ones; the run must have exactly their keys, with
 * the same values.  Order does not count.
 */
#ifndef LIBSCMP_H
#define LIBSCMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { LIBSCMP_BLOCK = 4096 };          /* observations per BLOCK:N case */
enum { LIBSCMP_DETAILS = 11 };          /* findings shown per program */

enum libscmp_set { LIBSCMP_OBJ, LIBSCMP_ORIG };
enum libscmp_kind { LIBSCMP_RUN, LIBSCMP_EXPECTED };
enum libscmp_qual { LIBSCMP_PLAIN, LIBSCMP_QORIG, LIBSCMP_QFIXED };

/* one case: offsets into the file's text */
struct libscmp_case {
    uint32_t key, keylen;               /* the key without its qualifier */
    uint32_t val, vallen;
    uint32_t line;                      /* from 1 */
    uint32_t issue;                     /* [fixed #N]: N */
    uint32_t entry;                     /* index of its key in entries */
    uint8_t qual;                       /* enum libscmp_qual */
};

/* one key: its cases (index into cases, -1 = none) */
struct libscmp_entry {
    uint32_t key, keylen, hash;
    int32_t plain, orig, fixed;
};

struct libscmp_file {
    char *text;                         /* the file (owned, 0-terminated) */
    size_t len;
    char name[64];                      /* NAME of the header */
    struct libscmp_case *cases;         /* in file order */
    size_t ncases, capcases;
    struct libscmp_entry *entries;      /* in order of first appearance */
    size_t nentries, capentries;
    uint32_t *slots;                    /* hash index: entry + 1, 0 = free */
    size_t nslots;
};

/**
 * @brief Parse a file from memory (the text is copied).
 * @param f     the result (libscmp_free it, also after an error)
 * @param text  the file's bytes (may hold 0 bytes: an error)
 * @param len   their number
 * @param path  the file's name in messages
 * @param kind  LIBSCMP_RUN (no qualifiers) or LIBSCMP_EXPECTED
 * @param err   the message of an error, "PATH:LINE: TEXT"
 * @param errlen the size of err
 * @return 0 = ok, -1 = out of memory, -2 = malformed (err says where)
 */
int libscmp_parse(struct libscmp_file *f, const char *text, size_t len, const char *path,
                  enum libscmp_kind kind, char *err, size_t errlen);

/** @brief The same from a file; -3 = it cannot be read (err says so). */
int libscmp_load(struct libscmp_file *f, const char *path, enum libscmp_kind kind, char *err,
                 size_t errlen);

void libscmp_free(struct libscmp_file *f);

/** @brief The entry of a key (without qualifier), or NULL. */
const struct libscmp_entry *libscmp_find(const struct libscmp_file *f, const char *key,
                                         size_t keylen);

/** @brief The case of expected data that applies to set for an entry, or -1. */
int32_t libscmp_applies(const struct libscmp_entry *e, enum libscmp_set set);

/* the counts of a compare */
struct libscmp_stats {
    size_t cases;                       /* cases of the run */
    size_t equal, differ, missing, added, skipped;  /* skipped: not captured */
};

/**
 * @brief Compare a run's output with the expected data for a set.
 * Writes a line "NAME: KEY [QUAL]: expected VALUE, got VALUE" per differing
 * case, "NAME: KEY [QUAL]: missing (expected VALUE)" per case the run lacks
 * and "NAME: KEY: added (got VALUE)" per case it has in addition, a block
 * with its observation range; the first LIBSCMP_DETAILS findings, then
 * "NAME: ... N more", then the summary line "NAME: N cases, ...".
 * @return 0 = equal, 1 = a finding, -1 = out of memory
 */
int libscmp_compare(const struct libscmp_file *run, const struct libscmp_file *exp,
                    enum libscmp_set set, FILE *out, struct libscmp_stats *st);

/**
 * @brief libs-compare: the run files RUN_DIR/NAME.txt against
 * EXPECTED_DIR/NAME.txt.  The programs are the NAMEs of both directories,
 * or the ones named by only (then the others are not looked at); a
 * program with a file in only one directory fails.  The header's NAME must
 * be the file's.
 * @return 0 = all equal, 1 = a finding, 2 = error (a malformed or
 *         unreadable file, a named program without files; on err)
 */
int libscmp_compare_dirs(const char *run_dir, const char *exp_dir, enum libscmp_set set,
                         const char *const *only, size_t nonly, FILE *out, FILE *err);

/**
 * @brief libs-accept for one program: the expected data from two recording
 * runs of the original.  The runs must be identical (else "test not
 * deterministic: NAME" and the differing cases) and hold no case "lost"
 * (a stated register the original does not keep).  The new file has the
 * run's cases in its order, unqualified, except: a key with a [fixed #N]
 * case in old gets the run's value as KEY [orig] and keeps the [fixed #N]
 * case behind it; a key that has only an [orig] case in old stays [orig].
 * A [fixed #N] case of old whose key the run lacks is kept (a case of the
 * port only) after the case it followed and reported; an [orig] case of
 * old whose key the run lacks is dropped and reported.
 * @param old   the committed expected data, or NULL
 * @param out   the new file (malloc'd), NULL when refused
 * @param report  the messages
 * @return 0 = *out written, 1 = refused, -1 = out of memory
 */
int libscmp_accept(const struct libscmp_file *run1, const struct libscmp_file *run2,
                   const struct libscmp_file *old, char **out, FILE *report);

/**
 * @brief libs-accept on directories: every program of RUN1_DIR and RUN2_DIR
 * (or the ones only names) is accepted into EXPECTED_DIR/NAME.txt; a file
 * of EXPECTED_DIR without runs is an error unless only leaves it out.
 * Nothing is written unless every program is accepted; an unchanged file
 * is not rewritten.
 * @return 0 = written, 1 = refused (not deterministic, lost, a program
 *         without both runs), 2 = error (malformed or unreadable file, a
 *         file that cannot be written)
 */
int libscmp_accept_dirs(const char *run1_dir, const char *run2_dir, const char *exp_dir,
                        const char *const *only, size_t nonly, FILE *out, FILE *err);

/* a --fix KEY#N of libs-update; the counters are the result */
struct libscmp_fix {
    char *key;                          /* KEY (malloc'd) */
    size_t keylen;
    uint32_t issue;                     /* N */
    unsigned applied;                   /* programs where the pair was made */
    unsigned same;                      /* ... where the port gives the expected value */
    unsigned qualified;                 /* ... where KEY is qualified already */
};

/** @brief "KEY#N" (split at the last '#'; KEY a key without qualifier, N
 * from 1) into fix; 0 = ok, -1 = malformed (the message in err). */
int libscmp_parse_fix(const char *arg, struct libscmp_fix *fix, char *err, size_t errlen);
void libscmp_fix_free(struct libscmp_fix *fix);

/**
 * @brief libs-update for one program: the expected data exp with the value
 * of every [fixed #N] case taken from the port's run.  A fix whose KEY is
 * an unqualified case of exp makes it the pair KEY [orig] (the old value)
 * and KEY [fixed #N] (the run's value), or KEY [orig] alone if the run
 * lacks the key; a fix whose KEY only the run has adds KEY [fixed #N]
 * after the case the run has before it.  Nothing else changes; an
 * unqualified case the port gives another value is reported.
 * @return 0 = *out written (malloc'd), 1 = refused (a [fixed #N] case the
 *         run lacks; *out NULL), -1 = out of memory
 */
int libscmp_update(const struct libscmp_file *run, const struct libscmp_file *exp,
                   struct libscmp_fix *fixes, size_t nfixes, char **out, FILE *report);

/**
 * @brief libs-update on directories: every program with expected data in
 * EXPECTED_DIR (or the ones only names) from RUN_DIR; each fix must apply
 * in exactly one program.  Nothing is written unless all is well; an
 * unchanged file is not rewritten.
 * @return 0 = written, 1 = refused, 2 = error
 */
int libscmp_update_dirs(const char *run_dir, const char *exp_dir, const char *const *only,
                        size_t nonly, struct libscmp_fix *fixes, size_t nfixes, FILE *out,
                        FILE *err);

/* --- fix-scope (design D12) -------------------------------------------- */

/* issue numbers */
struct libscmp_issues {
    uint32_t *v;
    size_t n, cap;
};

/** @brief Add issue n (once); 0 = ok, -1 = out of memory. */
int libscmp_issues_add(struct libscmp_issues *is, uint32_t n);
/** @brief The issues of the lines "Fixes #N" of commit messages (leading
 * and trailing blanks allowed, nothing else on the line); 0 = ok, -1 = out
 * of memory. */
int libscmp_issues_from_log(struct libscmp_issues *is, const char *log);
void libscmp_issues_free(struct libscmp_issues *is);

/**
 * @brief The cases a branch changes in a program's expected data that are
 * not its own: every case (a key with its qualifier) added, changed or
 * removed from base to branch is reported unless it is a [fixed #N] case
 * of a named issue, the [orig] case paired with one (the same key has a
 * [fixed #N] case of a named issue in the branch, or for a removed [orig]
 * case in base), or an unqualified case that became such a pair.  A line
 * per reported case: "NAME: KEY [QUAL]: added (VALUE)", "...: changed
 * (BASE -> BRANCH)", "...: removed (VALUE)".
 * @param base    the program's expected data before, or NULL (a new file)
 * @param branch  the same on the branch, or NULL (a removed file)
 * @param nchanged  set to the number of cases added, changed or removed
 * @return the number of cases reported
 */
size_t libscmp_fix_scope(const struct libscmp_file *base, const struct libscmp_file *branch,
                         const struct libscmp_issues *is, FILE *report, size_t *nchanged);

/**
 * @brief fix-scope on directories: the expected data BASE_DIR/NAME.txt
 * (of main) against BRANCH_DIR/NAME.txt (of the branch), every NAME of
 * both directories.
 * @return 0 = only the named issues' cases changed, 1 = a case reported,
 *         2 = error (a malformed or unreadable file or directory)
 */
int libscmp_fix_scope_dirs(const char *base_dir, const char *branch_dir,
                           const struct libscmp_issues *is, FILE *out, FILE *err);

#ifdef __cplusplus
}
#endif

#endif
