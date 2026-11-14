/*
 * runwork.h - the run directory of a runner (corpusrun pass, tedab run):
 * every run works in a directory of its own below the pass file's work=
 * directory, so that two runs of the same pass at the same time (two
 * ctest runs in one build directory) do not share their files.
 *
 *   WORK/run-XXXXXX   a run (mkdtemp), locked (flock) while its runner lives
 *   WORK              itself locked (flock) while runs are created or removed
 *
 * The end of a run (runwork_finish): a run that passes leaves nothing (the
 * run directory goes, and WORK if it is empty), except that a pass with
 * publish=NAME hands its run to the tests that read it (rundir.h); a run
 * that fails stays, and the runner prints where.  A run directory is
 * removed only when no runner holds its lock: at the start of a later run
 * of the same pass (a failed run's files stay until then, a run that
 * crashed or ended without a verdict is collected then).
 */
#ifndef RUNWORK_H
#define RUNWORK_H

#include <stddef.h>
#include <stdio.h>

/* the default of the pass key min_free= (MiB): less free space in the
   work directory before or after the sessions is an error, not a verdict */
#define RUNWORK_MIN_FREE_MIB 64u
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
 * holds.
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
 * @brief The end of a run, then w is closed (runwork_close).
 *
 * ran: a session has started.  A run that did not run, or passed (rc 0)
 * without publish, is removed.  rc 0 with publish=NAME: the run becomes
 * RUN/NAME (rundir_publish; run_dflt is the pass key run_dir=); if that
 * fails the run stays and the result is 2.  Any other rc: the run
 * stays and log gets "ME: the files of the run: DIR".  WORK is removed
 * if nothing is left in it.
 *
 * @return rc, or 2 if the run could not be published
 */
int runwork_finish(struct runwork *w, int ran, int rc, const char *publish, const char *run_dflt,
                   FILE *log, const char *me);

/**
 * @brief Remove the run directories below base that no runner holds.
 *
 * @return how many were removed, -1 on an error
 */
int runwork_clean(const char *base);

/* removes the run directory (a run without a verdict, nothing to keep) */
void runwork_remove(struct runwork *w);
/* unlocks the run directory (it stays) and frees w */
void runwork_close(struct runwork *w);

#endif
