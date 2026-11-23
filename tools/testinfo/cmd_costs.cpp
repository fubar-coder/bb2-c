/*
 * testinfo costs - the measured COST of the tests from ctest's JUnit files
 * (lib/teststats teststats_costs, the file format testcosts.h):
 *
 *   testinfo costs [--update FILE] JUNIT.xml...
 *
 * Without --update the entries go to stdout; with it they replace the
 * entries of the same tests in FILE (tests/data/test-costs.txt), the others
 * stay, and FILE is written sorted.  Exit status 0, 1 = problems in a JUnit
 * file (reported, the rest counts), 2 = usage, a file that cannot be read
 * or written, a malformed FILE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "testcosts.h"
#include "teststats.h"
}
#include "testinfo.h"

static char *read_file(const char *path, size_t *len)
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

static int run(const char *me, const std::string &update, const std::vector<std::string> &files)
{
    struct teststats *t = teststats_new();
    struct tcosts measured = {}, old = {};
    char *buf = NULL, err[400];
    size_t blen = 0;
    int problems = 0, rc = 2;
    FILE *mf;

    if (!t) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    for (const std::string &file : files) {
        size_t len;
        char *text = read_file(file.c_str(), &len);

        if (!text) {
            fprintf(stderr, "%s: cannot read %s\n", me, file.c_str());
            goto out;
        }
        problems += teststats_add(t, file.c_str(), text, len, stderr) ? 1 : 0;
        free(text);
    }
    if (!(mf = open_memstream(&buf, &blen)) || teststats_costs(t, mf) || fclose(mf)) {
        fprintf(stderr, "%s: out of memory\n", me);
        goto out;
    }
    if (tcosts_parse("(measured)", buf, 0, &measured, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", me, err);
        goto out;
    }
    if (!update.empty()) {
        if (std::filesystem::exists(update) && tcosts_load(update.c_str(), &old, err, sizeof err)) {
            fprintf(stderr, "%s: %s\n", me, err);
            goto out;
        }
        if (tcosts_merge(&old, &measured)) {
            fprintf(stderr, "%s: out of memory\n", me);
            goto out;
        }
        std::string tmp = update + ".tmp";
        FILE *f = fopen(tmp.c_str(), "w");
        std::error_code ec;

        if (!f || tcosts_write(f, &old) || fclose(f)) {
            fprintf(stderr, "%s: cannot write %s\n", me, update.c_str());
            std::filesystem::remove(tmp, ec);
            goto out;
        }
        std::filesystem::rename(tmp, update, ec);
        if (ec) {
            fprintf(stderr, "%s: cannot write %s\n", me, update.c_str());
            goto out;
        }
        fprintf(stderr, "%s: %zu tests measured, %zu entries in %s\n", me, measured.n, old.n, update.c_str());
    } else if (tcosts_write(stdout, &measured)) {
        fprintf(stderr, "%s: cannot write the entries\n", me);
        goto out;
    }
    if (problems)
        fprintf(stderr, "%s: problems in %d of %zu files\n", me, problems, files.size());
    rc = problems ? 1 : 0;
out:
    free(buf);
    tcosts_free(&measured);
    tcosts_free(&old);
    teststats_free(t);
    return rc;
}

void cmd_costs(CLI::App &app, int &status)
{
    struct options {
        std::string update;
        std::vector<std::string> files;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("costs", "the measured COST of the tests (tests/data/test-costs.txt)");
    sub->add_option("--update", o->update,
                    "put the measured entries into FILE, replacing those of the same tests (the others stay)")
        ->type_name("FILE");
    sub->add_option("JUNIT.xml", o->files, "ctest's JUnit files, one per run")->required();
    sub->footer("The COST of a test is the median over the runs of the sum of its emulator sessions' times "
                "without the slot wait, a session that did not start from a snapshot (cold, first-save, "
                "fallback) counted as the median of the test's sessions that did; a test without such a "
                "session gets no entry. Exit status 0, 1 problems in a JUnit file (reported on stderr), 2 "
                "usage error, a file that cannot be read or written, a malformed FILE.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        status = run(name.c_str(), o->update, o->files);
    });
}
