/*
 * tedab - the TED A/B tests in Amiberry (run, lint) and the screen dumps of
 * teddrive they evaluate, by hand (info, text, cmp).  README.md here; the
 * subcommands are cmd_run.cpp (body: run.c) and cmd_dump.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"The TED A/B tests in Amiberry and the screen dumps of teddrive.", "tedab"};
    int status = 0;

    app.require_subcommand(1);
    app.footer("The exit status of run in a bug-orig pass is the worst verdict: 0 FIXED, 1 "
               "REPRODUCED, 3 BROKEN, 4 DIFFERS.");
    cmd_run(app, status);
    cmd_lint(app, status);
    cmd_info(app, status);
    cmd_text(app, status);
    cmd_cmp(app, status);
    return cli::run(app, argc, argv, status);
}
