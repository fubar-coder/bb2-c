/*
 * testinfo stats - statistics of test runs from ctest's JUnit files and the
 * runners' @stat lines in them (lib/teststats/teststats.h): durations,
 * emulator sessions, flaky results, suggested time limits and COST
 * values.
 *
 *   testinfo stats [--tsv] [--factor X] [--floor SEC] JUNIT.xml...
 *
 * Every file is one run (ctest --preset P writes build/amiga/junit-P.xml;
 * keep runs apart with --output-junit NAME, tests/README.md).  Problems (a
 * malformed file, a testcase without name or time, a broken @stat line)
 * go to stderr; the rest is reported.  Exit status 0, 1 = problems
 * (reported), 2 = usage or a file that cannot be read.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "teststats.h"
}
#include "testinfo.h"

/* the defaults the usage names */
static_assert(TESTSTATS_FACTOR == 3.0 && TESTSTATS_FLOOR == 30.0, "the defaults in the help of --factor, --floor");

static const char *me; /* "testinfo stats", the prefix of the messages */

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        *len = (size_t)n;
        s[n] = 0;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static int run(const struct teststats_options &o, const std::vector<std::string> &files)
{
    struct teststats *t;
    int nfiles = 0, problems = 0, rc;

    if (!(t = teststats_new())) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    for (const std::string &file : files) {
        size_t len;
        char *text = read_all(file.c_str(), &len);
        int r;

        if (!text) {
            fprintf(stderr, "%s: cannot read %s\n", me, file.c_str());
            teststats_free(t);
            return 2;
        }
        r = teststats_add(t, file.c_str(), text, len, stderr);
        free(text);
        problems += r ? 1 : 0;
        nfiles++;
    }
    rc = teststats_report(t, &o, stdout);
    teststats_free(t);
    if (rc) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    if (problems)
        fprintf(stderr, "%s: problems in %d of %d files\n", me, problems, nfiles);
    return problems ? 1 : 0;
}

void cmd_stats(CLI::App &app, int &status)
{
    struct options {
        struct teststats_options o = { TESTSTATS_FACTOR, TESTSTATS_FLOOR, 0 };
        bool tsv = false;
        std::vector<std::string> files;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("stats", "statistics of ctest runs and their emulator sessions");
    sub->add_flag("--tsv", o->tsv, "tab-separated values instead of text tables");
    sub->add_option("--factor", o->o.factor, "the factor of the time limits (default 3)")
        ->type_name("X")
        ->check(CLI::PositiveNumber);
    sub->add_option("--floor", o->o.floor, "the smallest suggested limit (default 30)")
        ->type_name("SEC")
        ->check(CLI::NonNegativeNumber);
    sub->add_option("JUNIT.xml", o->files, "ctest's JUnit files, one per run")->required();
    sub->footer("Statistics of ctest runs (one JUnit file per run) and the @stat lines of the "
                "emulator sessions in them: durations, sessions, flaky results, suggested time "
                "limits (max timed x factor, at least the floor) and COST. Exit status 0, 1 "
                "problems in a file (reported on stderr), 2 usage error or a file that cannot be "
                "read.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        o->o.tsv = o->tsv;
        status = run(o->o, o->files);
    });
}
