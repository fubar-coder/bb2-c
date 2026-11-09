/*
 * hunkcheck - checks of the hunks of a load file: the bytes at offsets of
 * the first hunk (start), the hunk layout (layout), every CODE/DATA hunk
 * stored whole (stored).  README.md here; the subcommands are in
 * cmd_check.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"Checks of the hunks of an AmigaDOS load file.", "hunkcheck"};
    int status = 0;

    app.require_subcommand(1);
    cmd_start(app, status);
    cmd_layout(app, status);
    cmd_stored(app, status);
    return cli::run(app, argc, argv, status);
}
