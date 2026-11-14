/*
 * test_rundir.c - the run directory of the fixtures (rundir.h): which
 * directory is RUN (the variable of the caller, else the default), an input
 * named relative to it (a missing one is refused with a message that
 * names BB2C_TEST_RUN), the cleanup (removes the fixture and the default
 * directory, keeps a fixture a failed test marked, leaves the caller's
 * directory and what is not the fixture's in it), and a publish across file
 * systems (the copy).
 *
 *   test_rundir DIR      (DIR: a scratch directory, emptied first)
 */
#define _DEFAULT_SOURCE         /* setenv */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rundir.h"
#include "util.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static int is_dir(const char *path)
{
    struct stat st;

    return !lstat(path, &st) && S_ISDIR(st.st_mode);
}

/* RUN/fx/clib/vol/out/x as a published run would leave it */
static void make_fixture(const char *run, const char *fx)
{
    char *d = str_printf("%s/%s/clib/vol/out", run, fx), *f = str_printf("%s/x", d);

    CHECK(mkdir_p(d) == 0 && write_file(f, "x", 1) == 0);
    free(d);
    free(f);
}

int main(int argc, char **argv)
{
    char err[400], fx[64], *p = NULL, *dflt, *caller, *spec;

    if (argc != 2) {
        fprintf(stderr, "usage: test_rundir DIR\n");
        return 2;
    }
    rm_tree(argv[1]);
    dflt = str_printf("%s/Testing/runs/default", argv[1]);
    caller = str_printf("%s/mine", argv[1]);

    /* RUN: the variable, else the pass key, else none ("-" is none) */
    unsetenv(RUNDIR_ENV);
    p = rundir_path(dflt);
    CHECK(p && !strcmp(p, dflt));
    free(p);
    CHECK((p = rundir_path(NULL)) == NULL);
    CHECK((p = rundir_path("-")) == NULL);
    setenv(RUNDIR_ENV, caller, 1);
    p = rundir_path(dflt);
    CHECK(p && !strcmp(p, caller));
    free(p);
    setenv(RUNDIR_ENV, "", 1);                  /* empty: as not set */
    p = rundir_path(dflt);
    CHECK(p && !strcmp(p, dflt));
    free(p);
    unsetenv(RUNDIR_ENV);

    /* an input: the fixture must be there; the message says what to do */
    spec = "run:corpus/clib/vol/out";
    CHECK(rundir_input(dflt, spec, &p, fx, sizeof fx, err, sizeof err) == -1 && !p
          && strstr(err, RUNDIR_ENV) && strstr(err, "no fixture data"));
    CHECK(rundir_input(NULL, spec, &p, fx, sizeof fx, err, sizeof err) == -1
          && strstr(err, RUNDIR_ENV) && strstr(err, "no run directory"));
    make_fixture(dflt, "corpus");
    CHECK(rundir_input(dflt, spec, &p, fx, sizeof fx, err, sizeof err) == 0 && p
          && !strcmp(fx, "corpus") && is_dir(p) && strstr(p, "Testing/runs/default/corpus/clib"));
    free(p);
    CHECK(rundir_input(dflt, "run:../x", &p, fx, sizeof fx, err, sizeof err) == -1
          && strstr(err, "not a fixture path"));
    CHECK(rundir_input(dflt, "run:", &p, fx, sizeof fx, err, sizeof err) == -1);
    CHECK(rundir_input(dflt, "run:corpus/none", &p, fx, sizeof fx, err, sizeof err) == -1
          && strstr(err, "none"));

    /* the cleanup of the default directory: the fixture goes, and the
       directory and Testing/runs with it, as nothing else is in them */
    CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 0);
    CHECK(!is_dir(dflt));
    {
        char *runs = str_printf("%s/Testing/runs", argv[1]), *testing = str_printf("%s/Testing", argv[1]);

        CHECK(!is_dir(runs) && is_dir(testing));        /* Testing/ is ctest's: it stays */
        free(runs);
        free(testing);
    }
    CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 0);   /* nothing there: fine */

    /* a failed test marks what it read: the cleanup keeps it (and the
       default directory), a publish of the fixture replaces data and mark */
    make_fixture(dflt, "corpus");
    make_fixture(dflt, "corpus_originals");
    rundir_keep(dflt, "corpus");
    CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 1);
    CHECK(rundir_input(dflt, spec, &p, fx, sizeof fx, err, sizeof err) == 0);
    free(p);
    CHECK(rundir_drop(dflt, "corpus_originals", err, sizeof err) == 0);
    CHECK(is_dir(dflt));                                /* the kept one is still in it */
    {
        char *src = str_printf("%s/newrun", argv[1]);

        make_fixture(src, "x");                         /* src/x/vol/out/x: any tree */
        {
            char *tree = str_printf("%s/x", src);

            CHECK(rundir_publish(dflt, "corpus", tree, err, sizeof err) == 0 && !is_dir(tree));
            free(tree);
        }
        CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 0);       /* the mark is gone */
        CHECK(!is_dir(dflt));
        free(src);
    }

    /* the caller's directory (BB2C_TEST_RUN) is the caller's: only the
       fixture goes, the directory and what else is in it stay */
    setenv(RUNDIR_ENV, caller, 1);
    make_fixture(caller, "corpus");
    {
        char *other = str_printf("%s/other.txt", caller);

        CHECK(write_file(other, "keep", 4) == 0);
        CHECK(rundir_input(dflt, spec, &p, fx, sizeof fx, err, sizeof err) == 0 && strstr(p, "/mine/"));
        free(p);
        CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 0);
        CHECK(is_dir(caller) && !access(other, F_OK));
        CHECK(rundir_input(dflt, spec, &p, fx, sizeof fx, err, sizeof err) == -1);
        /* the default directory is not touched while the variable is set */
        make_fixture(dflt, "corpus");
        CHECK(rundir_drop(dflt, "corpus", err, sizeof err) == 0 && is_dir(dflt));
        free(other);
    }
    unsetenv(RUNDIR_ENV);

    /* a name that is no name is refused, nothing outside RUN is touched */
    {
        char *tree = str_printf("%s/t", argv[1]);

        CHECK(mkdir_p(tree) == 0);
        CHECK(rundir_publish(dflt, "a/b", tree, err, sizeof err) == -1 && is_dir(tree));
        CHECK(rundir_publish(NULL, "fx", tree, err, sizeof err) == -1 && strstr(err, RUNDIR_ENV)
              && is_dir(tree));
        free(tree);
    }

    /* a copy of a tree (the publish across file systems): files with their
       mode, a link as a link */
    {
        char *from = str_printf("%s/cp/from/d", argv[1]), *to = str_printf("%s/cp/to", argv[1]),
             *f = str_printf("%s/f", from), *l = str_printf("%s/l", from),
             *g = str_printf("%s/f", to), *m = str_printf("%s/l", to);
        struct stat st;
        char buf[16];
        ssize_t n;

        CHECK(mkdir_p(from) == 0 && write_file(f, "abc", 3) == 0 && chmod(f, 0755) == 0);
        CHECK(symlink("f", l) == 0);
        CHECK(mkdir_p(to) == 0);
        CHECK(copy_tree(from, to) == 0);
        CHECK(!stat(g, &st) && st.st_size == 3 && (st.st_mode & 0111));
        n = readlink(m, buf, sizeof buf - 1);
        CHECK(n == 1 && buf[0] == 'f');
        /* a source that is not there fails */
        CHECK(copy_tree("/nonexistent-dir-x", to) == -1);
        free(from);
        free(to);
        free(f);
        free(l);
        free(g);
        free(m);
    }

    rm_tree(argv[1]);
    free(dflt);
    free(caller);
    return failures != 0;
}
