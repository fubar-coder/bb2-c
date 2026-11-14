/*
 * rundir.c - the run directory of the fixtures, see rundir.h.
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rundir.h"
#include "util.h"

char *rundir_path(const char *dflt)
{
    const char *e = getenv(RUNDIR_ENV);

    if (e && *e)
        return strdup(e);
    return dflt && *dflt && strcmp(dflt, "-") ? strdup(dflt) : NULL;
}

/* a fixture name is one path component */
static int plain_name(const char *n)
{
    return n && *n && !strchr(n, '/') && strcmp(n, ".") && strcmp(n, "..");
}

int rundir_publish(const char *dflt, const char *name, const char *dir, char *err, size_t errlen)
{
    char *run = rundir_path(dflt), *to = NULL, *keep = NULL;
    int r = -1;

    if (!run) {
        snprintf(err, errlen, "publish=%s needs a run directory: set " RUNDIR_ENV
                 " or the pass key run_dir=", name);
        goto out;
    }
    to = str_printf("%s/%s", run, name);
    keep = str_printf("%s/%s" RUNDIR_KEEP, run, name);
    if (!plain_name(name) || !to || !keep) {
        snprintf(err, errlen, "publish=%s: not a name", name);
        goto out;
    }
    if (mkdir_p(run)) {
        snprintf(err, errlen, "cannot create the run directory %s: %s", run, strerror(errno));
        goto out;
    }
    rm_tree(to);
    unlink(keep);
    if (rename(dir, to) && (errno != EXDEV || copy_tree(dir, to))) {
        snprintf(err, errlen, "cannot move %s to %s: %s", dir, to, strerror(errno));
        goto out;
    }
    rm_tree(dir);               /* a copy leaves the original, a rename nothing */
    r = 0;
out:
    free(run);
    free(to);
    free(keep);
    return r;
}

int rundir_input(const char *dflt, const char *spec, char **path, char *fixture,
                 size_t fixlen, char *err, size_t errlen)
{
    const char *rel = spec + strlen(RUNDIR_PREFIX), *slash = strchr(rel, '/');
    char *run = rundir_path(dflt);
    size_t n = slash ? (size_t)(slash - rel) : strlen(rel);
    struct stat st;

    *path = NULL;
    if (n == 0 || n >= fixlen || strstr(rel, "..")) {
        snprintf(err, errlen, "%s: not a fixture path (run:NAME/DIR)", spec);
        free(run);
        return -1;
    }
    memcpy(fixture, rel, n);
    fixture[n] = 0;
    if (!run) {
        snprintf(err, errlen, "no fixture data for %s: no run directory (set " RUNDIR_ENV
                 " to the directory of the test that makes them, or run ctest from the top of "
                 "the build directory)", spec);
        return -1;
    }
    *path = str_printf("%s/%s", run, rel);
    free(run);
    if (!*path || stat(*path, &st) || !S_ISDIR(st.st_mode)) {
        snprintf(err, errlen, "no fixture data %s: the test that makes them has not run (ctest "
                 "runs it first), or " RUNDIR_ENV " is not the same for both (ctest runs in one "
                 "build directory at the same time each set it to a directory of their own)",
                 *path ? *path : spec);
        free(*path);
        *path = NULL;
        return -1;
    }
    return 0;
}

void rundir_keep(const char *dflt, const char *fixture)
{
    char *run = rundir_path(dflt), *m = run ? str_printf("%s/%s" RUNDIR_KEEP, run, fixture) : NULL;

    if (m && plain_name(fixture))
        write_file(m, "failed\n", 7);
    free(run);
    free(m);
}

int rundir_drop(const char *dflt, const char *name, char *err, size_t errlen)
{
    char *run = rundir_path(dflt), *dir = NULL, *keep = NULL, *slash;
    const char *env = getenv(RUNDIR_ENV);
    int r = -1;

    if (!run || !plain_name(name)) {
        free(run);
        return 0;               /* nothing was made without a run directory */
    }
    dir = str_printf("%s/%s", run, name);
    keep = str_printf("%s/%s" RUNDIR_KEEP, run, name);
    if (!dir || !keep) {
        snprintf(err, errlen, "out of memory");
        goto out;
    }
    if (!access(keep, F_OK)) {
        r = 1;
        goto out;
    }
    rm_tree(dir);
    if (!access(dir, F_OK)) {
        snprintf(err, errlen, "cannot remove %s", dir);
        goto out;
    }
    r = 0;
    /* the default directory belongs to the build: gone when empty, and
       Testing/runs with it; a directory the caller gave stays */
    if (!env || !*env) {
        rmdir(run);
        if ((slash = strrchr(run, '/')) != NULL) {
            *slash = 0;
            rmdir(run);
        }
    }
out:
    free(run);
    free(dir);
    free(keep);
    return r;
}
