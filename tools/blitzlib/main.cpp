/*
 * blitzlib - Blitz Basic 2 library files: join them into a BigLib file
 * (join), take one out (extract), no library number twice (numbers), the
 * checks of a library file (check), the declarations against the original
 * (compare), the command table as JSON (cmdref).  README.md here; the
 * subcommands are cmd_biglib.cpp (join, extract, numbers) and
 * cmd_NAME.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"Blitz Basic 2 library files: joined, taken out, checked, compared, as JSON.", "blitzlib"};
    int status = 0;

    app.require_subcommand(1);
    cmd_join(app, status);
    cmd_extract(app, status);
    cmd_numbers(app, status);
    cmd_check(app, status);
    cmd_compare(app, status);
    cmd_decls(app, status);
    cmd_cmdref(app, status);
    return cli::run(app, argc, argv, status);
}
