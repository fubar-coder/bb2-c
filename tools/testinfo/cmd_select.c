/*
 * testinfo select - the tests a change can affect, from the coverage of a
 * coverage run (lib/covsel/covsel.h; cmake/coverage-select.cmake runs git,
 * ninja and ctest and hands their output to it).
 *
 *   testinfo select --coverage DIR --rules RULES --tests JSON [--host-tests JSON]
 *                   [--diff DIFF] [--deps DEPS]... [--untracked LIST] [--stale]
 *                   [--root DIR] -o SELECTED
 *
 * DIR is the output of testinfo map: DIR/commit and the tracefiles
 * DIR/lcov/<test>.info (the test's name is the file's path below
 * DIR/lcov without ".info"; all.info is not read).  Writes the names of
 * the selected tests to SELECTED, one per line (for ctest
 * --tests-from-file), and why each one is selected to stdout.  Exit
 * status 0, 1 if an input is broken (the message on stderr), 2 on wrong
 * arguments or a file that cannot be read.
 */
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "covsel.h"
#include "testinfo.h"

const char cmd_select_usage[] =
    "usage: testinfo select --coverage DIR --rules RULES --tests JSON [--host-tests JSON]\n"
    "                       [--diff DIFF] [--deps DEPS]... [--untracked LIST] [--stale]\n"
    "                       [--root DIR] -o SELECTED\n"
    "Select the tests a change can affect, from the coverage of a coverage run.\n"
    "  --coverage DIR     testinfo map's output: DIR/commit, DIR/lcov/<test>.info\n"
    "  --rules RULES      what a change of a file selects (tools/testinfo/rules.txt)\n"
    "  --tests JSON       ctest --show-only=json-v1 of the build\n"
    "  --host-tests JSON  the same of its host project (tests host/NAME, run by\n"
    "                     host or host_emulator)\n"
    "  --diff DIFF        git diff -U0 against the map's commit\n"
    "  --deps DEPS        ninja -t deps of a build\n"
    "  --untracked LIST   files git does not track, one per line (new files)\n"
    "  --stale            the map's commit is not an ancestor of the base: all tests\n"
    "  --root DIR         the repository (default: the current directory)\n"
    "Writes the selected tests to SELECTED, one per line, the reasons to stdout.\n";

static const char *me; /* "testinfo select", the prefix of the messages */

static void usage(FILE *f)
{
    fputs(cmd_select_usage, f);
}

/* a whole file, 0-terminated */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

enum { IN_COVERAGE, IN_RULES, IN_TESTS, IN_HOST, IN_DIFF, IN_DEPS, IN_UNTRACKED, IN_COMMIT };

static int read_input(struct covsel *s, int kind, const char *path);

/* the tracefiles below the lcov directory (for nftw) */
static struct covsel *walk_sel;
static size_t walk_root;        /* length of the directory's path and '/' */
static int walk_r;

static int walk_file(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    size_t n = strlen(path);
    char *test, *text, err[512];

    (void)st;
    (void)ftw;
    if (type != FTW_F || n < walk_root + 6 || strcmp(path + n - 5, ".info")
        || !strcmp(path + walk_root, "all.info"))
        return 0;
    if (!(text = read_file(path))) {
        perror(path);
        walk_r = 2;
        return 1;
    }
    if (!(test = strndup(path + walk_root, n - walk_root - 5))) {
        free(text);
        walk_r = 2;
        return 1;
    }
    if (covsel_read_lcov(walk_sel, test, text, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        walk_r = 1;
    }
    free(test);
    free(text);
    return walk_r;
}

/* testinfo map's output: DIR/commit, and the tracefiles DIR/lcov/<test>.info */
static int read_coverage(struct covsel *s, const char *dir)
{
    size_t n = strlen(dir) + 16;
    char *path = malloc(n);
    int r;

    if (!path)
        return 2;
    snprintf(path, n, "%s/commit", dir);
    if (!(r = read_input(s, IN_COMMIT, path))) {
        snprintf(path, n, "%s/lcov", dir);
        walk_sel = s;
        walk_root = strlen(path) + 1;
        walk_r = 0;
        if (nftw(path, walk_file, 16, FTW_PHYS) && !walk_r) {
            perror(path);
            walk_r = 2;
        }
        r = walk_r;
    }
    free(path);
    return r;
}

static int read_input(struct covsel *s, int kind, const char *path)
{
    char *text, err[512];
    int r = 0;

    if (kind == IN_COVERAGE)
        return read_coverage(s, path);
    if (!(text = read_file(path))) {
        perror(path);
        return 2;
    }
    switch (kind) {
    case IN_COMMIT: r = covsel_set_commit(s, text, err, sizeof err); break;
    case IN_RULES: r = covsel_read_rules(s, text, err, sizeof err); break;
    case IN_TESTS: r = covsel_read_tests(s, text, 0, err, sizeof err); break;
    case IN_HOST: r = covsel_read_tests(s, text, 1, err, sizeof err); break;
    case IN_DIFF: r = covsel_read_diff(s, text, err, sizeof err); break;
    case IN_DEPS: r = covsel_read_deps(s, text, err, sizeof err); break;
    case IN_UNTRACKED: {
        size_t pos = 0, len = strlen(text), n;
        const char *line;

        while (!r && (line = cov_next_line(text, len, &pos, &n)))
            if (n) {
                char *p = strndup(line, n);

                r = !p || covsel_add_new_file(s, p);
                free(p);
                if (r)
                    snprintf(err, sizeof err, "out of memory");
            }
        break;
    }
    }
    free(text);
    if (r) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        return 1;
    }
    return 0;
}

int cmd_select(int argc, char **argv)
{
    static const struct {
        const char *opt;
        int kind;
    } opts[] = {
        { "--coverage", IN_COVERAGE }, { "--rules", IN_RULES }, { "--tests", IN_TESTS },
        { "--host-tests", IN_HOST }, { "--diff", IN_DIFF }, { "--deps", IN_DEPS },
        { "--untracked", IN_UNTRACKED },
    };
    const char *output = NULL, *paths[7] = { 0 };
    char *root = NULL;
    struct covsel *s;
    FILE *out;
    size_t i, k;
    int a, r = 0, stale = 0;

    me = argv[0];
    for (a = 1; a < argc; a++) {
        for (k = 0; k < sizeof opts / sizeof *opts; k++)
            if (!strcmp(argv[a], opts[k].opt))
                break;
        if (k < sizeof opts / sizeof *opts && a + 1 < argc) {
            if (opts[k].kind != IN_DEPS)
                paths[opts[k].kind] = argv[a + 1];
            a++;
        } else if (!strcmp(argv[a], "--stale"))
            stale = 1;
        else if (!strcmp(argv[a], "--root") && a + 1 < argc)
            root = argv[++a];
        else if (!strcmp(argv[a], "-o") && a + 1 < argc)
            output = argv[++a];
        else {
            usage(stderr);
            return 2;
        }
    }
    if (!output || !paths[IN_COVERAGE] || !paths[IN_RULES] || !paths[IN_TESTS]) {
        usage(stderr);
        return 2;
    }
    if (!(root = realpath(root ? root : ".", NULL))) {
        fprintf(stderr, "%s: --root: %s\n", me, strerror(errno));
        return 2;
    }
    s = covsel_new(root);
    free(root);
    if (!s)
        return 2;
    for (k = 0; k < 7 && !r; k++)
        if (paths[k])
            r = read_input(s, (int)k, paths[k]);
    for (a = 1; a < argc && !r; a++)
        if (!strcmp(argv[a], "--deps") && a + 1 < argc)
            r = read_input(s, IN_DEPS, argv[++a]);
        else if (strcmp(argv[a], "--stale"))
            a++;
    if (stale)
        covsel_set_stale(s);
    if (!r && covsel_run(s, stdout)) {
        fprintf(stderr, "%s: out of memory\n", me);
        r = 2;
    }
    if (!r) {
        if (!(out = fopen(output, "w"))) {
            perror(output);
            r = 2;
        } else {
            for (i = 0; i < covsel_count(s); i++)
                fprintf(out, "%s\n", covsel_name(s, i));
            if (fclose(out)) {
                perror(output);
                r = 2;
            }
        }
        if (!r)
            printf("%zu test(s) selected: %s\n", covsel_count(s), output);
    }
    covsel_free(s);
    return r;
}
