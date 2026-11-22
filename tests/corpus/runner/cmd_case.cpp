/*
 * corpusrun case - one case of a Blitz test of the BASIC test model in
 * Amiberry, the cells of one side (caserun.h).
 *
 *   corpusrun case --env FILE --source ID.test.bas --case CASE --side port|originals
 *                  [--require-deviation] [--print-config] [--print-driver]
 *
 * Exit status the verdict: 0 passed, 1 reproduced, 2 error, 3 broken, 4
 * failed, 77 skipped.
 */
#include <stdio.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "caserun.h"
}
#include "cmds.h"

struct case_options {
    std::string env, source, case_name, side = "port";
    bool require_deviation = false, print_config = false, print_driver = false;
};

void cmd_case(CLI::App &app, int &status)
{
    auto o = std::make_shared<case_options>();
    CLI::App *sub = app.add_subcommand("case", "one case of a Blitz test of the BASIC test model (ID.test.bas) in "
                                               "Amiberry: the cells of one side compiled, run and judged");
    sub->add_option("--env", o->env, "the environment file of the build (tests/model-env.toml)")
        ->required()
        ->type_name("FILE");
    sub->add_option("--source", o->source, "the test source ID.test.bas")->required()->type_name("FILE");
    sub->add_option("--case", o->case_name, "the case (not with --print-config)")->type_name("CASE");
    sub->add_option("--side", o->side, "the cells without original material (port, the default) or with it "
                                       "(originals: the original compilers, the CD's libraries)")
        ->check(CLI::IsMember({ "port", "originals" }))
        ->type_name("SIDE");
    sub->add_flag("--require-deviation", o->require_deviation,
                  "a bug-orig test: a test that declares no deviation of the original is broken");
    sub->add_flag("--print-config", o->print_config,
                  "print the effective configuration (every key with its value and origin, the axes, the cells) "
                  "and run nothing");
    sub->add_flag("--print-driver", o->print_driver, "print the driver of the case and run nothing");
    sub->footer("The cells of a side that share the session settings run in one emulator session. Exit status: "
                "0 passed, 1 reproduced (an open bug shows as declared), 2 error (a configuration, source or "
                "result that cannot be judged; usage), 3 broken, 4 failed, 77 skipped (no emulator, or a file "
                "of the side is missing).");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        caserun_opts co = {};

        if (o->case_name.empty() && !o->print_config) {
            fprintf(stderr, "%s: --case is required\n", name.c_str());
            status = 2;
            return;
        }
        co.env = o->env.c_str();
        co.source = o->source.c_str();
        co.case_name = o->case_name.empty() ? nullptr : o->case_name.c_str();
        co.originals = o->side == "originals";
        co.require_deviation = o->require_deviation;
        co.print_config = o->print_config;
        co.print_driver = o->print_driver;
        status = caserun(&co, stdout);
    });
}
