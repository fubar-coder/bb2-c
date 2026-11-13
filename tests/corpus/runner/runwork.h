/*
 * runwork.h - the run directory of a runner (corpusrun pass, tedab run):
 * every run works in a directory of its own below the pass file's work=
 * directory, so that two runs of the same pass at the same time (two
 * ctest runs in one build directory) do not share their files.
 *
 *   WORK/run-XXXXXX   a run (mkdtemp), locked (flock) while its runner lives
 *   WORK/last         link to the run that finished last with a verdict: the
 *                     fixed path other tests read a pass's results from
 *                     (corpus_out=, serial_same_as=, ...) and where people
 *                     look for the files of a failed run
 *   WORK/.runs.lock   serialises creating, publishing and removing runs
 *
 * A run directory is removed only when no runner holds its lock: when a
 * later run is published in its place as WORK/last, or at the start of a
 * later run (a run that crashed or ended without a verdict).  So a failed
 * run's files stay until the next run of the same pass replaces them, as
 * the fixed work directory did before.
 */
#ifndef RUNWORK_H
#define RUNWORK_H

#include <stddef.h>

/* the default of the pass key min_free= (MiB): less free space in the
   work directory before or after the sessions is an error, not a verdict */
#define RUNWORK_MIN_FREE_MIB 64u
#define RUNWORK_LAST "last"
#define RUNWORK_PREFIX "run-"

struct runwork {
    char *base;         /* work= */
    char *dir;          /* base/run-XXXXXX */
    int fd;             /* the run directory, locked; -1 = none */
};

/**
 * @brief Create and lock a new run directory below base.
 *
 * Makes base if needed and removes the run directories in it no runner
 * holds (except the one WORK/last names).
 *
 * @return 0, or -1 with the reason in err
 */
int runwork_open(struct runwork *w, const char *base, char *err, size_t errlen);

/**
 * @brief Whether dir's file system has at least min_mib MiB free.
 *
 * @return 0 enough (or min_mib 0), -1 not (err: "no space left in DIR
 *         (N MiB free, M MiB needed)") or statvfs failed
 */
int runwork_space(const char *dir, unsigned long long min_mib, char *err, size_t errlen);

/**
 * @brief Make the run WORK/last (atomically) and remove the run it named
 *        before, unless a runner still holds it.
 *
 * @return 0, or -1 with the reason in err (the run directory stays)
 */
int runwork_publish(struct runwork *w, char *err, size_t errlen);

/**
 * @brief Remove the run directories below base that no runner holds and
 *        WORK/last does not name.
 *
 * @return how many were removed, -1 on an error
 */
int runwork_clean(const char *base);

/* removes the run directory (a run without a verdict, nothing to keep) */
void runwork_remove(struct runwork *w);
/* unlocks the run directory (it stays) and frees w */
void runwork_close(struct runwork *w);

#endif
