/*
 * tedab run, tedab lint - the subcommands of the TED A/B tests in Amiberry;
 * their body is run.c (the keys of the pass file: its header).
 *
 *   tedab run PASSFILE    run the tests, report
 *   tedab lint PASSFILE   only check the scripts and their sources
 */
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "run.h"
}
#include "cmds.h"

/* run (lint 0) and lint (lint 1) */
static void add(CLI::App &app, int &status, int lint)
{
    auto passfile = std::make_shared<std::string>();
    CLI::App *sub = lint ? app.add_subcommand("lint", "check the test scripts of a pass file and that their sources are there")
                         : app.add_subcommand("run", "run the TED A/B tests of a pass file in Amiberry and write the report");
    sub->add_option("PASSFILE", *passfile, "the pass file (written by CMake; its keys: the header of "
                                           "tests/ted-ab/runner/run.c)")
        ->required();
    if (lint)
        sub->footer("No emulator. Exit status 0 no problem, 1 problems, 2 usage error or a pass "
                    "file that cannot be read.");
    else
        sub->footer("With the pass key record=check a bug-orig pass (bugorig=1, orig=) runs only "
                    "the original and compares its observation with the recorded one, with "
                    "record=write it writes the observation to record_to= (the target "
                    "bug-orig-record). Exit status 0 all gates and runs OK and every comparison as expected, 1 not, 2 "
                    "error (usage, pass file, scripts), 77 the emulator or a file is missing "
                    "(skipped). A bug-orig pass gives the worst verdict: 0 all FIXED, 1 "
                    "REPRODUCED, 3 BROKEN, 4 DIFFERS (with record=check 0 the same, 1 it "
                    "differs).");
    sub->callback([sub, passfile, lint, &status] {
        const std::string name = cli::name(*sub);

        status = tedab_start(name.c_str(), passfile->c_str(), lint);
    });
}

void cmd_run(CLI::App &app, int &status)
{
    add(app, status, 0);
}

void cmd_lint(CLI::App &app, int &status)
{
    add(app, status, 1);
}
