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
#include "runwork.h"
#include "util.h"

#define LOCKFILE ".runs.lock"

/* the lock of base's run list (LOCKFILE), held while runs are created,
   published or removed; -1 = error */
static int lock_base(const char *base)
{
    char *path = str_printf("%s/" LOCKFILE, base);
    int fd = path ? open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0644) : -1;

    free(path);
    if (fd >= 0 && flock(fd, LOCK_EX)) {
        close(fd);
        fd = -1;
    }
    return fd;
}

static void unlock_base(int fd)
{
    if (fd >= 0)
        close(fd);
}

/* the name of the run WORK/last links to (malloc'd), NULL = none */
static char *last_target(const char *base)
{
    char *link = str_printf("%s/" RUNWORK_LAST, base), buf[256];
    ssize_t n = link ? readlink(link, buf, sizeof buf - 1) : -1;

    free(link);
    if (n <= 0)
        return NULL;
    buf[n] = 0;
    return strdup(buf);
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
    char *keep = last_target(base);
    int n = 0;

    if (!d) {
        free(keep);
        return -1;
    }
    while ((de = readdir(d)) != NULL)
        if (!strncmp(de->d_name, RUNWORK_PREFIX, strlen(RUNWORK_PREFIX))
            && (!keep || strcmp(de->d_name, keep)))
            n += remove_unused(base, de->d_name);
    closedir(d);
    free(keep);
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
    if (mkdir_p(base) || (lk = lock_base(base)) < 0) {
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

int runwork_publish(struct runwork *w, char *err, size_t errlen)
{
    const char *name = strrchr(w->dir, '/') + 1;
    char *link = str_printf("%s/" RUNWORK_LAST, w->base);
    char *tmp = str_printf("%s/.%s.%ld", w->base, RUNWORK_LAST, (long)getpid());
    char *old = NULL;
    int lk = -1, r = -1;

    if (!link || !tmp) {
        snprintf(err, errlen, "out of memory");
        goto out;
    }
    if ((lk = lock_base(w->base)) < 0) {
        snprintf(err, errlen, "cannot lock %s: %s", w->base, strerror(errno));
        goto out;
    }
    old = last_target(w->base);
    unlink(tmp);
    /* a relative link: the build directory may move */
    if (symlink(name, tmp) || rename(tmp, link)) {
        snprintf(err, errlen, "cannot make %s a link to %s: %s", link, name, strerror(errno));
        unlink(tmp);
        goto out;
    }
    if (old && strcmp(old, name) && !strncmp(old, RUNWORK_PREFIX, strlen(RUNWORK_PREFIX))
        && !strchr(old, '/'))
        remove_unused(w->base, old);
    r = 0;
out:
    unlock_base(lk);
    free(old);
    free(link);
    free(tmp);
    return r;
}

void runwork_remove(struct runwork *w)
{
    if (w->dir && w->fd >= 0)
        rm_tree(w->dir);
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
