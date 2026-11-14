/*
 * corpusrun - the emulator passes of the corpus (pass: corpus.h) and of
 * the CD through one compiler (cd).  README.md here; the subcommands are
 * cmd_pass.cpp and cmd_cd.cpp (body: cd.c).
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"The corpus passes and the CD runs in Amiberry.", "corpusrun"};
    int status = 0;

    app.require_subcommand(1);
    app.footer("The exit status of pass in a bug-orig pass is the worst verdict: 0 FIXED, 1 "
               "REPRODUCED, 3 BROKEN, 4 DIFFERS.");
    cmd_pass(app, status);
    cmd_cd(app, status);
    cmd_drop(app, status);
    return cli::run(app, argc, argv, status);
}
