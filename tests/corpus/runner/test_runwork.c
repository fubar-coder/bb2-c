/*
 * test_runwork.c - the run directories of the runners (runwork.h): two runs
 * of one work directory get directories of their own; a run directory a
 * runner holds is never removed, one nobody holds is (unless work/last
 * names it); publishing makes the run work/last and removes the one
 * before; the free-space check refuses a work directory with less than
 * min_free= MiB free.
 *
 *   test_runwork DIR      (DIR: a scratch directory, emptied first)
 */
#define _DEFAULT_SOURCE         /* flock */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "runwork.h"
#include "util.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static int is_dir(const char *path)
{
    struct stat st;

    return !lstat(path, &st) && S_ISDIR(st.st_mode);
}

/* whether another runner could take path's lock (nobody holds it) */
static int unlocked(const char *path)
{
    int fd = open(path, O_RDONLY | O_DIRECTORY), r;

    if (fd < 0)
        return -1;
    r = !flock(fd, LOCK_EX | LOCK_NB);
    close(fd);
    return r;
}

/* the target of base/last, "" if none */
static const char *last(const char *base)
{
    static char buf[256];
    char *link = str_printf("%s/" RUNWORK_LAST, base);
    ssize_t n = link ? readlink(link, buf, sizeof buf - 1) : -1;

    free(link);
    buf[n > 0 ? n : 0] = 0;
    return buf;
}

int main(int argc, char **argv)
{
    char err[400], *base, *stale, *legacy;
    struct runwork a, b, c;

    if (argc != 2) {
        fprintf(stderr, "usage: test_runwork DIR\n");
        return 2;
    }
    rm_tree(argv[1]);
    base = str_printf("%s/work", argv[1]);
    stale = str_printf("%s/run-stale1", base);
    legacy = str_printf("%s/clib", base);

    /* two runs of one work directory: each its own directory, locked */
    CHECK(runwork_open(&a, base, err, sizeof err) == 0);
    CHECK(runwork_open(&b, base, err, sizeof err) == 0);
    CHECK(a.dir && b.dir && strcmp(a.dir, b.dir) && is_dir(a.dir) && is_dir(b.dir));
    CHECK(!strncmp(a.dir, base, strlen(base)) && !strncmp(a.dir + strlen(base), "/run-", 5));
    CHECK(unlocked(a.dir) == 0 && unlocked(b.dir) == 0);

    /* a leftover nobody holds goes, the runs that are held stay, and so do
       entries that are no run directory */
    CHECK(mkdir_p(stale) == 0 && mkdir_p(legacy) == 0);
    CHECK(runwork_clean(base) == 1);
    CHECK(!is_dir(stale) && is_dir(legacy) && is_dir(a.dir) && is_dir(b.dir));

    /* a publishes: it is work/last; then b replaces it, and a's directory
       (held no more) goes */
    CHECK(runwork_publish(&a, err, sizeof err) == 0);
    CHECK(!strcmp(last(base), strrchr(a.dir, '/') + 1));
    {
        char *adir = strdup(a.dir);

        runwork_close(&a);
        CHECK(is_dir(adir));            /* closed: unlocked, but still work/last */
        CHECK(runwork_clean(base) == 0 && is_dir(adir));
        CHECK(runwork_publish(&b, err, sizeof err) == 0);
        CHECK(!strcmp(last(base), strrchr(b.dir, '/') + 1));
        CHECK(!is_dir(adir));
        free(adir);
    }

    /* the run before is still held (a runner that has published but not
       ended): it stays, and the next start removes it once it is free */
    CHECK(runwork_open(&c, base, err, sizeof err) == 0);
    CHECK(runwork_publish(&c, err, sizeof err) == 0);
    CHECK(is_dir(b.dir) && !strcmp(last(base), strrchr(c.dir, '/') + 1));
    {
        char *bdir = strdup(b.dir);

        runwork_close(&b);
        CHECK(runwork_open(&a, base, err, sizeof err) == 0);
        CHECK(!is_dir(bdir) && is_dir(c.dir));
        free(bdir);
    }

    /* a run without a verdict removes its directory */
    {
        char *adir = strdup(a.dir);

        runwork_remove(&a);
        runwork_close(&a);
        CHECK(!is_dir(adir));
        free(adir);
    }
    runwork_close(&c);

    /* the free space: no check, enough, not enough (a work directory on a
       full disk), a directory that is not there */
    CHECK(runwork_space(base, 0, err, sizeof err) == 0);
    CHECK(runwork_space(base, 1, err, sizeof err) == 0);
    CHECK(runwork_space(base, 1ull << 50, err, sizeof err) == -1
          && !strncmp(err, "no space left in ", 17) && strstr(err, base));
    CHECK(runwork_space(stale, 1, err, sizeof err) == -1 && strstr(err, "cannot read"));

    /* a work directory that cannot be made */
    {
        char *file = str_printf("%s/file", argv[1]), *below = str_printf("%s/file/work", argv[1]);

        CHECK(write_file(file, "x", 1) == 0);
        CHECK(runwork_open(&a, below, err, sizeof err) == -1
              && strstr(err, "cannot create the work directory") && !a.dir && a.fd < 0);
        free(file);
        free(below);
    }

    rm_tree(argv[1]);
    free(base);
    free(stale);
    free(legacy);
    return failures != 0;
}
