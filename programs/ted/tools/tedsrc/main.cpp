/*
 * tedsrc - Blitz 2 sources through the C core of TED on the host:
 * tokenised as TED saves them (tok), shown as TED shows them (detok).
 * README.md here; the subcommands are cmd_tok.cpp and cmd_detok.cpp.
 */
#include "cli.hpp"
#include "tedsrc.h"

int main(int argc, char **argv)
{
    CLI::App app{"Blitz 2 sources through the C core of TED on the host.", "tedsrc"};
    int status = 0;

    app.require_subcommand(1);
    cmd_tok(app, status);
    cmd_detok(app, status);
    return cli::run(app, argc, argv, status);
}
