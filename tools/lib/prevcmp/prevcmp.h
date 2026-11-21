/*
 * prevcmp.h - the results of this build against those of the build of the
 * previous commit (the `prev` build, CMakeLists.txt BLITZ2_PREV): every
 * library file byte by byte.
 *
 * The library files of a build directory are
 *   blitz2/acidlibs, blitz2/deflibs   the joined library files
 *   every *.obj below libs/ and variants/ (outside CMakeFiles/ and the run
 *                                     directories named run-... of the test runs): the
 *                                     library files of acidlibs, deflibs,
 *                                     their variants and the userlibs
 * (A directory named run-... is skipped at any depth below libs/ and variants/, not only in
 * the tests' directories libs/tests/: a future library directory with such a name would
 * silently drop out of the comparison.)
 * (paths relative to the build directory, '/' separated).  Every one the
 * previous build has must exist in the current build with the same bytes;
 * a file only the current build has (a new library) is counted, not
 * judged.  A difference is an error unless it is intended: named in the
 * list of intended paths (each must differ: a stale entry is an error,
 * like the intended lists of blitzlibcompare and checkdata) or all
 * differences are allowed.
 */
#ifndef PREVCMP_H
#define PREVCMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct prevcmp_options {
    int all;                            /* every difference is intended */
    const char *const *intended;        /* the paths allowed to differ */
    size_t nintended;
};

/**
 * @brief Compare the library files of two build directories.
 * @param cur     the current build directory
 * @param prev    the build directory of the previous commit
 * @param o       the intended differences
 * @param out     the report (a line per difference, then the summary)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = no unintended difference, 1 = a difference that is not
 *         intended or a stale intended entry, 2 = error (unreadable
 *         directory, no library file in prev)
 */
int prevcmp(const char *cur, const char *prev, const struct prevcmp_options *o, FILE *out,
            char *err, size_t errlen);

/**
 * @brief Zero the parameter blocks of the traps in an executable: the 8 bytes behind each
 *        `4E41` (trap #1), then behind each `4E44` (trap #4), the search going on behind a
 *        zeroed block.  The blocks of a program with runtime error checking or debugging hold
 *        addresses of the compiler's memory (the line item of the editor), which depend on
 *        the memory layout of the session.
 */
void prevcmp_mask_traps(uint8_t *data, size_t len);

/**
 * @brief Two executables the same, the trap blocks masked (prevcmp_mask_traps)?
 * @param first  the offset of the first difference, if they differ (may be NULL)
 * @return 1 = the same, 0 = they differ (another length too)
 */
int prevcmp_exe_same(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen, size_t *first);

#ifdef __cplusplus
}
#endif

#endif
