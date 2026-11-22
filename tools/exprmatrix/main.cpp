/*
 * exprmatrix - the expression matrix of blitz2 (tools/lib/exprmatrix): the
 * Blitz sources of its cells from the description (generate).  README.md
 * here; the subcommands are cmd_NAME.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"The expression matrix of blitz2: every cell (context, operator, operand kinds in order, "
                 "value set) a keyed line of a Blitz source.",
                 "exprmatrix"};
    int status = 0;

    app.require_subcommand(1);
    cmd_generate(app, status);
    cmd_classify(app, status);
    return cli::run(app, argc, argv, status);
}
