/*
 * results corpus-accept - the expected data of a corpus pass
 * (tests/corpus/expect/PASS) from two runs of the original
 * (lib/corpusexpect/corpusexpect.h, OpenSpec change corpus-test-model,
 * design D4).
 *
 *   results corpus-accept RUN1 RUN2 EXPECT_DIR [--only NAME]...
 *
 * RUN1 and RUN2 are the observations of the configuration ref as
 * corpusrun pass --observe writes them (NAME.out, NAME.serial, NAME.exe),
 * the second with another memory layout.  Exit status 0 = every program
 * accepted (its files written or unchanged), 1 = a program refused (the
 * runs differ, a run lacks its output, the pair would be equal; nothing
 * written for it, the others are), 2 = wrong arguments, a malformed or
 * unreadable directory, a file that cannot be written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "corpusexpect.h"
}
#include "results.h"

static int accept(const std::string &run1, const std::string &run2, const std::string &dir,
                  const std::vector<std::string> &only, FILE *err)
{
    struct cexp_set a, b, old;
    struct cexp_plan pl = {};
    std::vector<const char *> names;
    char msg[800];
    int r;

    for (const std::string &n : only)
        names.push_back(n.c_str());
    if (cexp_load(&a, run1.c_str(), CEXP_RUN, msg, sizeof msg)
        || cexp_load(&b, run2.c_str(), CEXP_RUN, msg, sizeof msg)
        || cexp_load(&old, dir.c_str(), CEXP_MISSING_OK, msg, sizeof msg)) {
        fprintf(err, "%s\n", msg);
        cexp_free(&a);
        cexp_free(&b);
        cexp_free(&old);
        return 2;
    }
    r = cexp_accept(&a, &b, &old, names.data(), names.size(), &pl, stdout);
    if (r < 0) {
        fprintf(err, "out of memory\n");
        r = 2;
    } else if (cexp_plan_apply(&pl, dir.c_str(), stdout, msg, sizeof msg)) {
        fprintf(err, "%s\n", msg);
        r = 2;
    } else
        printf("corpus-accept: %s: %zu programs, %s\n", old.pass, names.empty() ? a.n : names.size(),
               r ? "a program refused (above)" : "accepted");
    cexp_plan_free(&pl);
    cexp_free(&a);
    cexp_free(&b);
    cexp_free(&old);
    return r;
}

void cmd_corpus_accept(CLI::App &app, int &status)
{
    struct options {
        std::string run1, run2, dir;
        std::vector<std::string> only;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("corpus-accept", "the expected data of a corpus pass from two runs of the original");
    sub->add_option("RUN1", o->run1, "the observations of the first run (NAME.out, NAME.serial, NAME.exe)")
        ->required();
    sub->add_option("RUN2", o->run2, "the observations of the second run, another memory layout")
        ->required();
    sub->add_option("EXPECT_DIR", o->dir, "the expected data of the pass (tests/corpus/expect/PASS)")
        ->required();
    cli::add_repeated(*sub, "--only", o->only, 1, "only this program (any number; the others are not touched)")
        ->type_name("NAME");
    sub->footer("The runs must agree (\"test not deterministic: PASS:NAME\" and the first differing "
                "line otherwise). A program's output and serial data become NAME.out and "
                "NAME.serial, or NAME.orig.out and NAME.orig.serial where EXPECT_DIR has the "
                "[fixed #N] file (kept); its executable NAME.orig.exe where EXPECT_DIR has "
                "NAME.fixed-N.exe. Exit status 0 every program accepted, 1 a program refused "
                "(nothing written for it), 2 usage error or a malformed, unreadable or unwritable "
                "file.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        status = prefixed(name, [&](FILE *err) { return accept(o->run1, o->run2, o->dir, o->only, err); });
    });
}
