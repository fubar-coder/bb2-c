/*
 * testinfo - records of test runs: statistics of the runs (stats), the
 * coverage of each test (map) and the tests a change can affect (select).
 * README.md here; the subcommands are cmd_NAME.cpp.
 */
#include "cli.hpp"
#include "testinfo.h"

int main(int argc, char **argv)
{
    CLI::App app{"Records of test runs: statistics, the coverage of each test, the tests a change can affect.",
                 "testinfo"};
    int status = 0;

    app.require_subcommand(1);
    cmd_stats(app, status);
    cmd_map(app, status);
    cmd_select(app, status);
    return cli::run(app, argc, argv, status);
}
