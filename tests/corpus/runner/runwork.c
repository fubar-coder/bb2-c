/*
 * runwork.c - the run directory of a runner, see runwork.h.
 */
#define _DEFAULT_SOURCE         /* flock, mkdtemp, strdup */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "rundir.h"
#include "runwork.h"
#include "util.h"

/* the lock of base's run list (a flock on the directory base itself, no
   file in it), held while runs are created and removed; base is made if
   needed (-1 = error).  A runner that ended removes an empty base under
   the lock: the lock counts only if base is still the directory it was
   taken on, else it is taken again. */
static int lock_base(const char *base)
{
    for (int tries = 0; tries < 20; tries++) {
        struct stat a, b;
        int fd;

        if (mkdir_p(base))
            return -1;
        fd = open(base, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (fd < 0)
            return -1;
        if (!flock(fd, LOCK_EX) && !fstat(fd, &a) && !stat(base, &b) && a.st_ino == b.st_ino
            && a.st_dev == b.st_dev)
            return fd;
        close(fd);
    }
    return -1;
}

static void unlock_base(int fd)
{
    if (fd >= 0)
        close(fd);
}

/* removes base/name if no runner holds its lock; 1 = removed */
static int remove_unused(const char *base, const char *name)
{
    char *path = str_printf("%s/%s", base, name);
    int fd = path ? open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW) : -1, gone = 0;

    if (fd >= 0 && !flock(fd, LOCK_EX | LOCK_NB)) {
        rm_tree(path);
        gone = 1;
    }
    if (fd >= 0)
        close(fd);
    free(path);
    return gone;
}

/* runwork_clean with base's lock held */
static int clean_locked(const char *base)
{
    DIR *d = opendir(base);
    struct dirent *de;
    int n = 0;

    if (!d)
        return -1;
    while ((de = readdir(d)) != NULL)
        if (!strncmp(de->d_name, RUNWORK_PREFIX, strlen(RUNWORK_PREFIX)))
            n += remove_unused(base, de->d_name);
    closedir(d);
    return n;
}

int runwork_clean(const char *base)
{
    int fd = lock_base(base), n;

    if (fd < 0)
        return -1;
    n = clean_locked(base);
    unlock_base(fd);
    return n;
}

int runwork_open(struct runwork *w, const char *base, char *err, size_t errlen)
{
    int lk = -1;

    w->base = strdup(base);
    w->dir = str_printf("%s/" RUNWORK_PREFIX "XXXXXX", base);
    w->fd = -1;
    if (!w->base || !w->dir) {
        snprintf(err, errlen, "out of memory");
        goto fail;
    }
    if ((lk = lock_base(base)) < 0) {
        snprintf(err, errlen, "cannot create the work directory %s: %s", base, strerror(errno));
        goto fail;
    }
    clean_locked(base);
    if (!mkdtemp(w->dir)) {
        snprintf(err, errlen, "cannot create a run directory in %s: %s", base, strerror(errno));
        unlock_base(lk);
        goto fail;
    }
    /* locked before base's lock is given up: no cleaner sees it unlocked */
    if ((w->fd = open(w->dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC)) < 0
        || flock(w->fd, LOCK_EX | LOCK_NB)) {
        snprintf(err, errlen, "cannot lock %s: %s", w->dir, strerror(errno));
        rm_tree(w->dir);
        unlock_base(lk);
        goto fail;
    }
    unlock_base(lk);
    return 0;
fail:
    runwork_close(w);
    return -1;
}

int runwork_space(const char *dir, unsigned long long min_mib, char *err, size_t errlen)
{
    struct statvfs sv;
    unsigned long long avail;

    if (!min_mib)
        return 0;
    if (statvfs(dir, &sv)) {
        snprintf(err, errlen, "cannot read the free space of %s: %s", dir, strerror(errno));
        return -1;
    }
    avail = (unsigned long long)sv.f_bavail * sv.f_frsize >> 20;
    if (avail >= min_mib)
        return 0;
    snprintf(err, errlen, "no space left in %s (%llu MiB free, %llu MiB needed: min_free=)", dir,
             avail, min_mib);
    return -1;
}

void runwork_remove(struct runwork *w)
{
    if (w->dir && w->fd >= 0)
        rm_tree(w->dir);
}

int runwork_finish(struct runwork *w, int ran, int rc, const char *publish, const char *run_dflt,
                   FILE *log, const char *me)
{
    char err[400];
    int lk;

    if (w->fd < 0)
        return rc;
    if (!ran || (rc == 0 && !publish))
        runwork_remove(w);
    else if (rc == 0 && rundir_publish(run_dflt, publish, w->dir, err, sizeof err)) {
        fprintf(log, "%s: %s\n%s: the files of the run: %s\n", me, err, me, w->dir);
        rc = 2;
    } else if (rc)
        fprintf(log, "%s: the files of the run: %s\n", me, w->dir);
    /* nothing of a passed run stays: the work directory goes when it is
       empty (not while another run or a kept one is in it) */
    if ((lk = lock_base(w->base)) >= 0) {
        rmdir(w->base);
        unlock_base(lk);
    }
    runwork_close(w);
    return rc;
}

void runwork_close(struct runwork *w)
{
    if (w->fd >= 0)
        close(w->fd);
    w->fd = -1;
    free(w->base);
    free(w->dir);
    w->base = w->dir = NULL;
}
