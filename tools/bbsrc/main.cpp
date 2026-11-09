/*
 * bbsrc - Blitz Basic 2 sources and tokens without TED: the token list of
 * a compiler and its libraries (tokens), a tokenised source as text
 * (detok), lines of a tokenised source edited (comment), BB22 sources in
 * TED's format (bb22, bb22-info), a source tokenised as AmiBlitz3 loads it
 * (ab3, ab3-cmp).  README.md here; the subcommands are cmd_*.cpp.
 */
#include "cli.hpp"
#include "bbsrc.h"

int main(int argc, char **argv)
{
    CLI::App app{"Blitz Basic 2 sources and tokens without TED.", "bbsrc"};
    int status = 0;

    app.require_subcommand(1);
    cmd_tokens(app, status);
    cmd_detok(app, status);
    cmd_comment(app, status);
    cmd_bb22(app, status);
    cmd_bb22_info(app, status);
    cmd_ab3(app, status);
    cmd_ab3_cmp(app, status);
    return cli::run(app, argc, argv, status);
}
