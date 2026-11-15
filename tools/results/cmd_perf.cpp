/*
 * results perf - the benchmark of the libraries (tests/perf): the cycles per
 * call from the observations of a run (lib/perfbudget/perfbudget.h).
 *
 *   results perf OBSERVATIONS
 *
 * Exit status 0 the table written, 2 usage error, a directory or file that
 * cannot be read, a malformed observation.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "perfbudget.h"
}
#include "results.h"

/* a whole file as text, NULL = cannot be read */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        s[n] = 0;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* the observations NAME.out of dir, sorted by name, into r; 0 = read */
static int read_run(const char *me, const std::string &dir, perf_run *r)
{
    std::vector<std::string> names;
    std::error_code ec;
    char err[600];

    for (const auto &e : std::filesystem::directory_iterator(dir, ec))
        if (e.path().extension() == ".out")
            names.push_back(e.path().stem().string());
    if (ec) {
        fprintf(stderr, "%s: %s: %s\n", me, dir.c_str(), ec.message().c_str());
        return -1;
    }
    if (names.empty()) {
        fprintf(stderr, "%s: %s: no observation (NAME.out)\n", me, dir.c_str());
        return -1;
    }
    std::sort(names.begin(), names.end());
    for (const std::string &n : names) {
        std::string path = dir + "/" + n + ".out";
        char *text = read_file(path.c_str());
        int rc;

        if (!text) {
            fprintf(stderr, "%s: %s: %s\n", me, path.c_str(), strerror(errno));
            return -1;
        }
        rc = perf_read_out(r, n.c_str(), text, err, sizeof err);
        free(text);
        if (rc) {
            fprintf(stderr, "%s: %s: %s\n", me, dir.c_str(), err);
            return -1;
        }
    }
    return 0;
}

void cmd_perf(CLI::App &app, int &status)
{
    struct options {
        std::string obs;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("perf", "the cycles per call of the library benchmark "
                                       "from the observations of a run");
    sub->add_option("OBSERVATIONS", o->obs, "the directory of the observations (NAME.out of "
                    "corpusrun pass --observe)")->required();
    sub->footer("A line \"TEST T N PER\" of NAME.out is the loop TEST of the program NAME: T units "
                "of 1000 cycles for N iterations, PER calls per iteration or \"frame\"; the "
                "program's empty loop is subtracted. Exit status 0 the table written, 2 usage "
                "error, a file that cannot be read or a malformed observation.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        perf_run r = {};

        if (read_run(name.c_str(), o->obs, &r)) {
            perf_run_free(&r);
            status = 2;
            return;
        }
        perf_table(&r, stdout);
        perf_run_free(&r);
        status = 0;
    });
}
