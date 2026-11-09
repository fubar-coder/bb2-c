/*
 * blitzres - Blitz Basic 2 resident files: one with constants written
 * (create), the contents of one (dump), its constants as Blitz source
 * (source).  README.md here; the subcommands are in cmd_res.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"Blitz Basic 2 resident files: written with constants, shown, as Blitz source.", "blitzres"};
    int status = 0;

    app.require_subcommand(1);
    cmd_create(app, status);
    cmd_dump(app, status);
    cmd_source(app, status);
    return cli::run(app, argc, argv, status);
}
