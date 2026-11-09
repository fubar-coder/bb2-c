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
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "covsel.h"
}
#include "testinfo.h"

static const char *me; /* "testinfo select", the prefix of the messages */

/* a whole file, 0-terminated */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
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
    char *path = (char *)malloc(n);
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

struct select_options {
    std::string paths[7], root, output;
    std::vector<std::string> deps;
    bool stale = false;
};

static int run(const select_options &o)
{
    char *root;
    struct covsel *s;
    FILE *out;
    size_t i;
    int k, r = 0;

    if (!(root = realpath(o.root.empty() ? "." : o.root.c_str(), NULL))) {
        fprintf(stderr, "%s: --root: %s\n", me, strerror(errno));
        return 2;
    }
    s = covsel_new(root);
    free(root);
    if (!s)
        return 2;
    for (k = 0; k < 7 && !r; k++)
        if (k != IN_DEPS && !o.paths[k].empty())
            r = read_input(s, k, o.paths[k].c_str());
    for (const std::string &d : o.deps)
        if (!r)
            r = read_input(s, IN_DEPS, d.c_str());
    if (o.stale)
        covsel_set_stale(s);
    if (!r && covsel_run(s, stdout)) {
        fprintf(stderr, "%s: out of memory\n", me);
        r = 2;
    }
    if (!r) {
        if (!(out = fopen(o.output.c_str(), "w"))) {
            perror(o.output.c_str());
            r = 2;
        } else {
            for (i = 0; i < covsel_count(s); i++)
                fprintf(out, "%s\n", covsel_name(s, i));
            if (fclose(out)) {
                perror(o.output.c_str());
                r = 2;
            }
        }
        if (!r)
            printf("%zu test(s) selected: %s\n", covsel_count(s), o.output.c_str());
    }
    covsel_free(s);
    return r;
}

void cmd_select(CLI::App &app, int &status)
{
    auto o = std::make_shared<select_options>();
    CLI::App *sub = app.add_subcommand("select", "the tests a change can affect, from the coverage of a coverage run");
    sub->add_option("--coverage", o->paths[IN_COVERAGE], "testinfo map's output: DIR/commit, DIR/lcov/<test>.info")
        ->type_name("DIR")
        ->required();
    sub->add_option("--rules", o->paths[IN_RULES], "what a change of a file selects (tools/testinfo/rules.txt)")
        ->type_name("RULES")
        ->required();
    sub->add_option("--tests", o->paths[IN_TESTS], "ctest --show-only=json-v1 of the build")
        ->type_name("JSON")
        ->required();
    sub->add_option("--host-tests", o->paths[IN_HOST],
                    "the same of its host project (tests host/NAME, run by host or host_emulator)")
        ->type_name("JSON");
    sub->add_option("--diff", o->paths[IN_DIFF], "git diff -U0 against the map's commit")->type_name("DIFF");
    cli::add_repeated(*sub, "--deps", o->deps, 1, "ninja -t deps of a build")->type_name("DEPS");
    sub->add_option("--untracked", o->paths[IN_UNTRACKED], "files git does not track, one per line (new files)")
        ->type_name("LIST");
    sub->add_flag("--stale", o->stale, "the map's commit is not an ancestor of the base: all tests");
    sub->add_option("--root", o->root, "the repository (default: the current directory)")->type_name("DIR");
    sub->add_option("-o", o->output, "the file of the selected tests")->type_name("SELECTED")->required();
    sub->footer("Writes the selected tests to SELECTED, one per line (for ctest --tests-from-file), "
                "and why each one is selected to stdout. Exit status 0, 1 if an input is broken "
                "(the message on stderr), 2 usage error or a file that cannot be read.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = run(*o);
    });
}
