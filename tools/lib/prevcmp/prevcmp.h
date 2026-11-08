/*
 * prevcmp.h - the results of this build against those of the build of the
 * previous commit (the `prev` build, CMakeLists.txt BLITZ2_PREV): every
 * library file byte by byte.
 *
 * The library files of a build directory are
 *   blitz2/acidlibs, blitz2/deflibs   the joined library files
 *   every *.obj below libs/ and variants/ (outside CMakeFiles/): the
 *                                     library files of acidlibs, deflibs,
 *                                     their variants and the userlibs
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
#include <stdio.h>

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

#endif
