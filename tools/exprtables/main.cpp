/*
 * exprtables - readable tables of the type rules of an expression in
 * blitz2 (tools/lib/exprtype_tables): the page
 * docs/blitz2/expression-type-tables.md, written or checked.  README.md
 * here; the subcommands are cmd_NAME.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"Readable tables of the type rules of an expression in blitz2, from the decisions of "
                 "programs/blitz2/src/exprtype.c.",
                 "exprtables"};
    int status = 0;

    app.require_subcommand(1);
    cmd_page(app, status);
    return cli::run(app, argc, argv, status);
}
