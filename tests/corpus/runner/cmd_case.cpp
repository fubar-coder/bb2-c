/*
 * corpusrun case - one case of a Blitz test of the BASIC test model in
 * Amiberry, or all cases of its source, the cells of one side (caserun.h).
 *
 *   corpusrun case --env FILE --source ID.test.bas [--case CASE] --side port|originals|oracle
 *                  [--require-deviation] [--print-config] [--print-driver]
 *   corpusrun case --env FILE --source ID.test.bas --write-driver NAME.bb2
 *   corpusrun case --env FILE --source ID.test.bas --prev --driver NAME.bb2 --test TEST
 *                  --prev-blitz2 FILE --prev-libs DIR [--intended ENTRY]...
 *   corpusrun case --env FILE --source ID.test.bas --case CASE --test TEST --from-bundle NAME
 *   corpusrun case --env FILE [--require-deviation] --bundle NAME --member TEST SOURCE CASE...
 *
 * Exit status the verdict: 0 passed, 1 reproduced (on the side originals
 * 0: the original shows the open bug as declared), 2 error, 3 broken, 4
 * failed, 77 skipped; without --case the worst verdict of the cases; with
 * --prev 0 the same, 1 differs, 2 error, 77 skipped; with --bundle 0 ran
 * through, 77 no session, 2 error of the bundle.
 */
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "caserun.h"
}
#include "cmds.h"

struct case_options {
    std::string env, source, case_name, side = "port", write_driver, driver, test, prev_blitz2, prev_libs;
    std::string from_bundle, bundle;
    std::vector<std::string> intended, member;
    bool require_deviation = false, print_config = false, print_driver = false, prev = false;
};

void cmd_case(CLI::App &app, int &status)
{
    auto o = std::make_shared<case_options>();
    CLI::App *sub = app.add_subcommand("case", "one case of a Blitz test of the BASIC test model (ID.test.bas), or "
                                               "all its cases, in Amiberry: the cells of one side compiled, run and "
                                               "judged");
    sub->add_option("--env", o->env, "the environment file of the build (tests/model-env.toml)")
        ->required()
        ->type_name("FILE");
    CLI::Option *source = sub->add_option("--source", o->source, "the test source ID.test.bas (not with --bundle)")
                              ->type_name("FILE");
    CLI::Option *case_opt = sub->add_option("--case", o->case_name,
                                            "the case; without it every case of the source (not with "
                                            "--print-config, --write-driver, --prev)")
                                ->type_name("CASE");
    sub->add_option("--side", o->side, "the cells without original material (port, the default), with it "
                                       "(originals: the original compilers, the CD's libraries), or of "
                                       "AmiBlitz3's compiler (oracle: compiler = \"amiblitz3\")")
        ->check(CLI::IsMember({ "port", "originals", "oracle" }))
        ->type_name("SIDE");
    sub->add_flag("--require-deviation", o->require_deviation,
                  "a bug-orig test: a test that declares no deviation of the original is broken");
    sub->add_flag("--print-config", o->print_config,
                  "print the effective configuration (every key with its value and origin, the axes, the cells) "
                  "and run nothing");
    sub->add_flag("--print-driver", o->print_driver,
                  "print the driver of the case (without --case of every case, the text --write-driver writes) "
                  "and run nothing");
    sub->add_option("--write-driver", o->write_driver,
                    "write the driver of all cases, tokenised, to FILE (its text beside it as .bas) and run "
                    "nothing: the build step of the comparison with the prev build")
        ->type_name("FILE");
    sub->add_flag("--prev", o->prev,
                  "compare with the prev build: the driver of --driver compiled by this build's compiler and by "
                  "--prev-blitz2, once per value of runtime-errors of the side port (the compiles ID and "
                  "ID-runerrs)");
    sub->add_option("--driver", o->driver, "with --prev: the driver --write-driver wrote")->type_name("FILE");
    CLI::Option *test = sub->add_option("--test", o->test,
                                        "with --prev: the ctest name, the TEST of the PREV_INTENDED entries; "
                                        "with --from-bundle the case test's name")
                            ->type_name("TEST");
    sub->add_option("--prev-blitz2", o->prev_blitz2, "with --prev: the previous commit's compiler")
        ->type_name("FILE");
    sub->add_option("--prev-libs", o->prev_libs, "with --prev: the directory of its deflibs and acidlibs")
        ->type_name("DIR");
    cli::add_repeated(*sub, "--intended", o->intended, 1,
                      "with --prev: a PREV_INTENDED entry (TEST:NAME of any test, or ON; any number)")
        ->type_name("ENTRY");
    sub->add_option("--from-bundle", o->from_bundle,
                    "with --case and --test: the bundle that may have stored the case's verdict; a fresh one is "
                    "printed and is the exit status, else the case runs alone")
        ->type_name("NAME")
        ->needs(case_opt)
        ->needs(test);
    CLI::Option *bundle = sub->add_option("--bundle", o->bundle,
                                          "run the members (--member) in shared emulator sessions and store each "
                                          "clean PASSED or REPRODUCED for its case test")
                              ->type_name("NAME")
                              ->excludes(source)
                              ->excludes(case_opt);
    cli::add_repeated(*sub, "--member", o->member, 3,
                      "with --bundle: a member, the case test's name, its source (an absolute path) and the case "
                      "(any number)")
        ->type_name("TEST SOURCE CASE")
        ->needs(bundle);
    sub->footer("The cells of a side that share the session settings run in one emulator session. Without "
                "--case every case of the source runs: the cases share one run of the driver of all of them, "
                "each judged from its own block; a case that asserts an observation of the whole program or "
                "announces a runtime error, the case open when a shared run did not end cleanly, and a case "
                "that did not pass in a shared run run on their own (\"(alone)\"), and a line per case and a "
                "summary line report them. Exit status: 0 passed, 1 reproduced (an open bug shows as "
                "declared; on the side originals 0, the report saying \"REPRODUCED (as declared, side "
                "originals)\"), 2 error (a configuration, source or result that cannot be judged; usage), 3 broken, "
                "4 failed, 77 skipped (no emulator, or a file of the side is missing); without --case the worst "
                "verdict of the cases, in the order error, broken, failed, skipped, reproduced, passed. With "
                "--prev: 0 the same, 1 a difference that is not intended or a stale entry, 2 error, 77 "
                "skipped. With --bundle: 0 it ran through (whatever its members' verdicts), 77 no session started "
                "(no emulator, no member that can run), 2 an error of the bundle.");
    sub->callback([o, &status] {
        caserun_opts co = {};
        std::vector<const char *> intended;

        for (const auto &e : o->intended)
            intended.push_back(e.c_str());
        if (!o->bundle.empty()) {
            std::vector<const char *> tests, sources, cases;
            cr_bundle b = {};

            for (size_t i = 0; i + 2 < o->member.size(); i += 3) {
                tests.push_back(o->member[i].c_str());
                sources.push_back(o->member[i + 1].c_str());
                cases.push_back(o->member[i + 2].c_str());
            }
            b.env = o->env.c_str();
            b.name = o->bundle.c_str();
            b.require_deviation = o->require_deviation;
            b.test = tests.data();
            b.source = sources.data();
            b.case_name = cases.data();
            b.n = tests.size();
            status = caserun_bundle(&b, stdout);
            return;
        }
        if (o->source.empty())
            throw CLI::RequiredError("--source");
        co.env = o->env.c_str();
        co.source = o->source.c_str();
        co.case_name = o->case_name.empty() || !o->write_driver.empty() || o->prev ? nullptr
                                                                                  : o->case_name.c_str();
        co.originals = o->side == "originals";
        co.oracle = o->side == "oracle";
        co.require_deviation = o->require_deviation;
        co.print_config = o->print_config;
        co.print_driver = o->print_driver;
        co.write_driver = o->write_driver.empty() ? nullptr : o->write_driver.c_str();
        co.prev = o->prev;
        co.driver = o->driver.empty() ? nullptr : o->driver.c_str();
        co.test = o->test.empty() ? nullptr : o->test.c_str();
        co.prev_blitz2 = o->prev_blitz2.empty() ? nullptr : o->prev_blitz2.c_str();
        co.prev_libs = o->prev_libs.empty() ? nullptr : o->prev_libs.c_str();
        co.intended = intended.data();
        co.nintended = intended.size();
        co.from_bundle = o->from_bundle.empty() ? nullptr : o->from_bundle.c_str();
        status = caserun(&co, stdout);
    });
}
