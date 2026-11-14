/*
 * test_runwork.c - the run directories of the runners (runwork.h): two runs
 * of one work directory get directories of their own; a run directory a
 * runner holds is never removed, one nobody holds is; a run that passes
 * leaves nothing (and the work directory goes when it is empty), a run
 * that passes with publish= becomes a fixture of the run directory, a run
 * that fails stays and says where; the free-space check refuses a work
 * directory with less than min_free= MiB free.
 *
 *   test_runwork DIR      (DIR: a scratch directory, emptied first)
 */
#define _DEFAULT_SOURCE         /* flock, open_memstream, unsetenv */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rundir.h"
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

/* runwork_finish with its log text in *text (malloc'd) */
static int finish(struct runwork *w, int ran, int rc, const char *publish, const char *run,
                  char **text)
{
    size_t n = 0;
    FILE *f = open_memstream(text, &n);
    int r = runwork_finish(w, ran, rc, publish, run, f, "test");

    fclose(f);
    return r;
}

int main(int argc, char **argv)
{
    char err[400], *base, *stale, *legacy, *run, *text = NULL;
    struct runwork a, b, c;

    if (argc != 2) {
        fprintf(stderr, "usage: test_runwork DIR\n");
        return 2;
    }
    unsetenv(RUNDIR_ENV);
    rm_tree(argv[1]);
    base = str_printf("%s/work", argv[1]);
    stale = str_printf("%s/run-stale1", base);
    legacy = str_printf("%s/clib", base);
    run = str_printf("%s/runs", argv[1]);

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

    /* a run that passes leaves nothing: its directory goes, the work
       directory stays while another run or an entry is in it */
    {
        char *adir = strdup(a.dir);

        CHECK(finish(&a, 1, 0, NULL, run, &text) == 0 && !*text);
        free(text);
        CHECK(!is_dir(adir) && is_dir(base) && is_dir(b.dir) && !a.dir);
        free(adir);
    }

    /* a run that fails stays and says where, the result is the rc */
    {
        char *bdir = strdup(b.dir);

        CHECK(finish(&b, 1, 1, NULL, run, &text) == 1);
        CHECK(strstr(text, "test: the files of the run: ") && strstr(text, bdir));
        free(text);
        CHECK(is_dir(bdir) && unlocked(bdir) == 1);
        /* a later run of the pass collects it */
        CHECK(runwork_open(&c, base, err, sizeof err) == 0);
        CHECK(!is_dir(bdir));
        free(bdir);
    }

    /* publish=NAME: the passing run becomes RUN/NAME with its files, an
       earlier NAME and its keep mark are replaced */
    rm_tree(legacy);
    {
        char *file = str_printf("%s/vol/out/x.exe", c.dir), *got = str_printf("%s/fx/vol/out/x.exe", run),
             *old = str_printf("%s/fx/old", run);
        size_t len;
        uint8_t *d;

        CHECK(mkdir_p(run) == 0);
        CHECK(mkdir_p(old) == 0);                       /* an earlier fixture: a dir with "old" */
        rm_tree(old);
        CHECK(mkdir_p(old) == 0);
        rundir_keep(run, "fx");
        {
            char *mark = str_printf("%s/fx" RUNDIR_KEEP, run);

            CHECK(!access(mark, F_OK));
            free(mark);
        }
        {
            char *vo = str_printf("%s/vol/out", c.dir);

            CHECK(mkdir_p(vo) == 0);
            free(vo);
        }
        CHECK(write_file(file, "exe", 3) == 0);
        CHECK(finish(&c, 1, 0, "fx", run, &text) == 0 && !*text);
        free(text);
        d = read_file(got, &len);
        CHECK(d && len == 3 && !memcmp(d, "exe", 3));
        free(d);
        CHECK(!is_dir(old));
        {
            char *mark = str_printf("%s/fx" RUNDIR_KEEP, run);

            CHECK(access(mark, F_OK) != 0);
            free(mark);
        }
        CHECK(!is_dir(base));                           /* nothing left in the work directory */
        free(file);
        free(got);
        free(old);
    }

    /* publish= without a run directory: the run is not published, it stays
       and the result is 2; a failed run publishes nothing */
    {
        char *adir;

        CHECK(runwork_open(&a, base, err, sizeof err) == 0);
        adir = strdup(a.dir);
        CHECK(finish(&a, 1, 0, "fx", NULL, &text) == 2 && strstr(text, RUNDIR_ENV));
        free(text);
        CHECK(is_dir(adir));
        CHECK(runwork_open(&a, base, err, sizeof err) == 0);    /* collects it */
        CHECK(!is_dir(adir));
        free(adir);
        CHECK(finish(&a, 1, 1, "fx2", run, &text) == 1);
        free(text);
        {
            char *p = str_printf("%s/fx2", run);

            CHECK(!is_dir(p));
            free(p);
        }
    }

    /* a run without a session (or one that is skipped) goes */
    {
        CHECK(runwork_open(&a, base, err, sizeof err) == 0);
        {
            char *adir = strdup(a.dir);

            CHECK(finish(&a, 0, 2, NULL, run, &text) == 2 && !*text);
            free(text);
            CHECK(!is_dir(adir));
            free(adir);
        }
    }

    /* the free space: no check, enough, not enough (a work directory on a
       full disk), a directory that is not there */
    CHECK(runwork_space(argv[1], 0, err, sizeof err) == 0);
    CHECK(runwork_space(argv[1], 1, err, sizeof err) == 0);
    CHECK(runwork_space(argv[1], 1ull << 50, err, sizeof err) == -1
          && !strncmp(err, "no space left in ", 17) && strstr(err, argv[1]));
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
    free(run);
    return failures != 0;
}
