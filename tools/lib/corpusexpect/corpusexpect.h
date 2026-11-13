/*
 * corpusexpect.h - the expected data of the corpus (tests/corpus/expect) as
 * cases of the shared test model (openspec/specs/testing/test-model, OpenSpec
 * change corpus-test-model, design D1 to D5): the files of a pass directory,
 * which of them applies to a configuration, the expected data written from
 * two runs of the original (record) or from the port (update), and the
 * cases of the directory for fix-scope.
 *
 * A pass directory (tests/corpus/expect/PASS) holds per program NAME (the
 * source without .bb2) and observation OBS (out: what the program writes,
 * serial: the bytes it sends over the serial port, exe: the SHA-256 of its
 * executable as 64 lower-case hex digits and a LF):
 *
 *   NAME.OBS           the unqualified case: original and port agree
 *   NAME.orig.OBS      the case [orig]: the original's value
 *   NAME.fixed-N.OBS   the case [fixed #N]: the port's value after the fix of
 *                      issue N
 *
 * Each file is the case PASS|NAME|OBS|1.  An observation has either the
 * unqualified file or the pair of qualified ones; an executable only the
 * pair (a program whose executables agree has no .exe file).  Malformed,
 * reported with the file names: a file whose name is not of this form (an
 * unknown observation or qualifier), an unqualified and a qualified file
 * of one observation, two fixed files of one observation, an unqualified
 * NAME.exe, an .exe file that is not a hash, a pair of equal files, and
 * (cexp_check with strict) one file of a pair without the other.  Names
 * beginning with '.' are not looked at.
 */
#ifndef CORPUSEXPECT_H
#define CORPUSEXPECT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "libscmp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum cexp_obs { CEXP_OUT, CEXP_SERIAL, CEXP_EXE, CEXP_NOBS };
enum cexp_qual { CEXP_PLAIN, CEXP_ORIG, CEXP_FIXED, CEXP_NQUAL };

/* the cases a configuration is judged against (design D2) */
enum cexp_side {
    CEXP_SIDE_PORT,             /* clib, prev: unqualified or [fixed #N] */
    CEXP_SIDE_ORIG,             /* ref: unqualified or [orig] */
    CEXP_SIDE_EITHER            /* port, orig-clib: unqualified, [orig] or [fixed #N] */
};

enum { CEXP_HASHLEN = 65 };     /* the text of an .exe file: 64 hex digits and LF */

struct cexp_file {
    uint8_t *data;              /* the content (owned; NULL: no such file) */
    size_t len;
};

struct cexp_prog {
    char *name;
    struct cexp_file f[CEXP_NOBS][CEXP_NQUAL];
    unsigned issue[CEXP_NOBS];  /* the N of the fixed file */
};

struct cexp_set {
    char *pass;                 /* the pass (the directory's name) */
    int run;                    /* a run's observations: plain files only, NAME.exe allowed */
    struct cexp_prog *p;        /* in the order of the names */
    size_t n, cap;
};

/** @brief The extension of an observation: "out", "serial", "exe". */
const char *cexp_obs_name(enum cexp_obs o);

/** @brief The side of a configuration by its tag: ref the original, port
 * and orig-clib either, every other (clib, prev) the port. */
enum cexp_side cexp_side_of_tag(const char *tag);

/**
 * @brief Add one file of a pass directory (from memory; the data are copied).
 * @param name  the file's name (NAME.OBS, NAME.orig.OBS, NAME.fixed-N.OBS;
 *              in a run's set NAME.OBS only)
 * @return 0 = ok, -1 = out of memory, -2 = malformed (err names the file)
 */
int cexp_add(struct cexp_set *s, const char *name, const void *data, size_t len, char *err,
             size_t errlen);

/**
 * @brief The rules of a set: the forms above; strict: also no file of a pair
 * without the other (the judge; record and update complete a pair).
 * @return 0 = ok, -2 = malformed (err names the files)
 */
int cexp_check(const struct cexp_set *s, int strict, char *err, size_t errlen);

/* cexp_load */
enum {
    CEXP_STRICT = 1,            /* cexp_check strict */
    CEXP_MISSING_OK = 2,        /* a directory that does not exist is an empty set */
    CEXP_RUN = 4                /* a run's observations (corpusrun pass --observe): plain
                                   files only, NAME.exe the hash of its executable */
};

/**
 * @brief The files of the pass directory dir (cexp_add each, then cexp_check).
 * @param flags  CEXP_STRICT, CEXP_MISSING_OK, CEXP_RUN
 * @return 0 = ok, -1 = out of memory, -2 = malformed, -3 = unreadable (err)
 */
int cexp_load(struct cexp_set *s, const char *dir, unsigned flags, char *err, size_t errlen);

void cexp_free(struct cexp_set *s);

/** @brief The program NAME of s, or NULL. */
const struct cexp_prog *cexp_find(const struct cexp_set *s, const char *name);

/** @brief The file name of a case: NAME.OBS, NAME.orig.OBS, NAME.fixed-N.OBS. */
void cexp_file_name(const struct cexp_prog *p, enum cexp_obs o, enum cexp_qual q, char *buf,
                    size_t len);

/** @brief An observation has a case at all (one of its files). */
int cexp_has(const struct cexp_prog *p, enum cexp_obs o);

/**
 * @brief The files that apply to side for an observation, in the order of the
 * report (the unqualified file; for the port's side [fixed #N]; for the
 * original's [orig]; for either [fixed #N] then [orig]).
 * @param q  out: the qualifiers (at most 2)
 * @return their number (0: the observation has no case)
 */
size_t cexp_applies(const struct cexp_prog *p, enum cexp_obs o, enum cexp_side side,
                    enum cexp_qual q[2]);

/**
 * @brief An observation got against the files that apply to side.
 * @param matched  out (may be NULL): the qualifier of the file it equals
 * @return 0 = it equals one of them, 1 = none, -1 = no file applies
 */
int cexp_judge(const struct cexp_prog *p, enum cexp_obs o, enum cexp_side side,
               const uint8_t *got, size_t glen, enum cexp_qual *matched);

/**
 * @brief The data got (glen bytes, NULL = none) against want: 0 = the same,
 * 1 = they differ, and what says where: "line N: expected \"...\", got
 * \"...\"" (the first differing line, without its LF; characters outside
 * 32..126 as \xNN, long lines cut with ...; "the end" for a missing line,
 * " (no LF)" behind a last line without one), "no output" for none.
 */
int cexp_diff(const uint8_t *want, size_t wlen, const uint8_t *got, size_t glen, char *what,
              size_t len);

/** @brief The text of an .exe file for an executable: SHA-256 in hex, LF. */
void cexp_exe_hash(const uint8_t *exe, size_t len, char out[CEXP_HASHLEN + 1]);

/* --- writing: a plan of file changes, applied to the directory --------- */

struct cexp_op {
    char *name;                 /* the file */
    uint8_t *data;              /* the new content (owned), NULL: remove the file */
    size_t len;
};

struct cexp_plan {
    struct cexp_op *op;
    size_t n, cap;
};

void cexp_plan_free(struct cexp_plan *pl);

/**
 * @brief Apply a plan to the directory dir: every file written (an unchanged
 * one is not rewritten), every removal done; the directory is made if needed.
 * @return 0 = ok, -1 = a file could not be written or removed (err)
 */
int cexp_plan_apply(const struct cexp_plan *pl, const char *dir, FILE *report, char *err,
                    size_t errlen);

/**
 * @brief The record (target corpus-expect-record): the expected data from two
 * runs of the original (run1, run2: plain files, as corpusrun pass --observe
 * writes them).  A program of the runs (only those named by only, if nonly)
 * is refused with "test not deterministic: PASS:NAME" and the first
 * differing line of each observation if the runs differ, and with a reason
 * if a run lacks its output or a serial pass its serial data.  Otherwise its
 * out and serial become the unqualified file, or the [orig] file where old
 * has a [fixed #N] one (kept); its exe the [orig] file where old has a
 * [fixed #N] exe, else nothing.  A value equal to the [fixed #N] file
 * refuses the program (the pair would be equal).  A refused program gets no
 * change.
 * @param old  the committed set (not strict: a lone [fixed #N] file is
 *             completed)
 * @return 0 = every program accepted, 1 = one refused, -1 = out of memory
 */
int cexp_accept(const struct cexp_set *run1, const struct cexp_set *run2,
                const struct cexp_set *old, const char *const *only, size_t nonly,
                struct cexp_plan *pl, FILE *report);

/* a FIX=PASS:NAME#N of the update */
struct cexp_fix {
    char pass[64], name[64];
    unsigned issue;
};

/** @brief "PASS:NAME#N" into fix; 0 = ok, -1 = malformed (err). */
int cexp_parse_fix(const char *arg, struct cexp_fix *fix, char *err, size_t errlen);

/**
 * @brief The update from the port (target corpus-expect-update, configuration
 * clib): for each program of run, its out (and with serial its serial): a
 * new program gets the unqualified file; an unqualified file the port
 * gives another value is refused with the program's name and the first
 * differing line, unless a fix names the program (PASS:NAME#N, PASS the
 * set's pass), which makes it the pair NAME.orig.OBS (the old content) and
 * NAME.fixed-N.OBS (the port's); a [fixed #N] file is rewritten with the
 * port's value (refused if that is the [orig] file's: an equal pair).  A
 * fix of another issue than the program's [fixed #N] files is refused.
 * @return 0 = every program done, 1 = one refused (nothing planned for it),
 *         -1 = out of memory
 */
int cexp_update(const struct cexp_set *run, const struct cexp_set *old, int serial,
                const struct cexp_fix *fix, size_t nfix, struct cexp_plan *pl, FILE *report);

/**
 * @brief The update of the executables (configurations ref and port of a run
 * with the CD's libraries): for a program with a [fixed #N] exe its file is
 * rewritten with the port's hash (refused if that is the [orig] one; a ref
 * hash other than the [orig] file is reported: the record writes it); a
 * program named by a fix without an exe pair gets the pair if ref's and
 * port's hashes differ.
 * @param ref, port  the hashes (CEXP_HASHLEN bytes; NULL: no executable)
 * @return 0 = ok, 1 = refused, -1 = out of memory
 */
int cexp_update_exe(const struct cexp_set *old, const char *name, const char *ref,
                    const char *port, const struct cexp_fix *fix, size_t nfix,
                    struct cexp_plan *pl, FILE *report);

/* --- fix-scope (design D5) --------------------------------------------- */

/**
 * @brief The case lines of a set, "PASS|NAME|OBS|1[ [orig]| [fixed #N]]<TAB>VALUE",
 * VALUE "bytes N fnv HHHHHHHH" (the length and FNV-1a of the file) or for
 * an .exe file "sha256 HEX".
 */
void cexp_project(FILE *out, const struct cexp_set *s);

/**
 * @brief The case file of every pass directory below root (tests/corpus/expect),
 * header "# capture corpus 1"; a root that does not exist has no cases.
 * @return 0 = ok, -1 = out of memory, -2 = malformed, -3 = unreadable (err)
 */
int cexp_project_tree(const char *root, char **text, size_t *len, char *err, size_t errlen);

/**
 * @brief fix-scope of the corpus expected data: base_root (of main) against
 * branch_root (of the branch), judged by libscmp_fix_scope; a summary line
 * "fix-scope: corpus: N cases changed, K not of the issues ...".
 * @return 0 = only the named issues' cases changed, 1 = a case reported, 2 =
 *         error (a malformed or unreadable directory, on err)
 */
int cexp_fix_scope_trees(const char *base_root, const char *branch_root,
                         const struct libscmp_issues *is, FILE *out, FILE *err);

#ifdef __cplusplus
}
#endif

#endif
