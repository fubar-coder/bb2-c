/*
 * corpusrun drop - the cleanup of a fixture (rundir.h): removes the data a
 * pass with publish=NAME put into the run directory, after the last test
 * that reads them.
 *
 *   corpusrun drop [--run-dir DIR] NAME...
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "rundir.h"
}
#include "cmds.h"

struct drop_options {
    std::string run_dir;
    std::vector<std::string> names;
};

static int drop(const char *me, const drop_options &o)
{
    int status = 0;

    for (const std::string &n : o.names) {
        char err[400];
        int r = rundir_drop(o.run_dir.c_str(), n.c_str(), err, sizeof err);

        if (r < 0) {
            fprintf(stderr, "%s: %s\n", me, err);
            status = 1;
        } else if (r > 0) {
            char *run = rundir_path(o.run_dir.c_str());

            printf("%s: kept %s/%s: a test that read it failed (the next run of its pass "
                   "replaces it)\n", me, run ? run : "?", n.c_str());
            free(run);
        }
    }
    return status;
}

void cmd_drop(CLI::App &app, int &status)
{
    auto o = std::make_shared<drop_options>();
    CLI::App *sub = app.add_subcommand("drop", "remove the data a pass published for other tests (the cleanup of a ctest fixture)");
    sub->add_option("--run-dir", o->run_dir, "the run directory when the environment variable "
                    "BB2C_TEST_RUN is not set (the default one is removed with Testing/runs if "
                    "empty)")
        ->type_name("DIR");
    sub->add_option("NAME", o->names, "the fixtures (the publish= of the passes)")
        ->required();
    sub->footer("A fixture that a failed test read stays (the next run of the passes that make it "
                "replaces it); a directory given by BB2C_TEST_RUN is the caller's: only the "
                "fixtures are removed from it. Exit status 0 done (also nothing "
                "to remove), 1 a fixture could not be removed.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        status = drop(name.c_str(), *o);
    });
}
