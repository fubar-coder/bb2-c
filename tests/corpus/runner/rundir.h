/*
 * rundir.h - the run directory where a test hands data to another test
 * (a ctest fixture): the passes that make data other tests read publish
 * them there (pass key publish=NAME, the directory RUN/NAME), the readers
 * name their input relative to it (corpus_out=run:NAME/...), and a cleanup
 * test removes NAME when the last reader is done (corpusrun drop).
 *
 * RUN is the environment variable BB2C_TEST_RUN, else the pass key
 * run_dir=DIR (the build directory's Testing/runs/default).  The caller
 * who sets the variable owns the directory: the runners and the cleanup
 * only create and remove what they put there, RUN/NAME and
 * RUN/NAME.keep.  Nothing locks the default directory: two ctest runs in
 * one build directory that both use it would overwrite each other's data,
 * so every run but one sets the variable to a directory of its own.
 *
 * A test that fails after reading NAME marks it (RUN/NAME.keep): the
 * cleanup leaves the data then, the next publish of NAME replaces them
 * and removes the mark.  A passed run leaves nothing: the cleanup also
 * removes the default directory (and Testing/runs) if it is empty.
 */
#ifndef RUNDIR_H
#define RUNDIR_H

#include <stddef.h>

#define RUNDIR_ENV "BB2C_TEST_RUN"
#define RUNDIR_PREFIX "run:"
#define RUNDIR_KEEP ".keep"

/**
 * @brief The run directory: $BB2C_TEST_RUN if it is set and not empty,
 *        else dflt (the pass key run_dir=, may be NULL or "-").
 *
 * @return malloc'd path, NULL = none
 */
char *rundir_path(const char *dflt);

/**
 * @brief Make dir (a finished run) the fixture NAME of the run directory:
 *        replaces an earlier NAME and its keep mark.
 *
 * Moves dir (rename; a copy and removal across file systems), so dir is
 * gone afterwards.
 *
 * @return 0, or -1 with the reason in err
 */
int rundir_publish(const char *dflt, const char *name, const char *dir, char *err, size_t errlen);

/**
 * @brief The path of an input named relative to the run directory.
 *
 * spec is "run:NAME/REST": the result is RUN/NAME/REST, which must be an
 * existing directory.  The fixture name NAME goes to fixture.
 *
 * @return 0 and the malloc'd path in *path, or -1 with the reason in err
 *         (the message names BB2C_TEST_RUN)
 */
int rundir_input(const char *dflt, const char *spec, char **path, char *fixture,
                 size_t fixlen, char *err, size_t errlen);

/* marks the fixture as read by a failed test: the cleanup keeps it */
void rundir_keep(const char *dflt, const char *fixture);

/**
 * @brief The cleanup: removes the fixture NAME unless a failed test
 *        marked it, and the default directory if nothing is left in it.
 *
 * @return 0 removed (or nothing there), 1 kept (marked), -1 error (err)
 */
int rundir_drop(const char *dflt, const char *name, char *err, size_t errlen);

#endif
