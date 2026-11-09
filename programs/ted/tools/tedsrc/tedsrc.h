/*
 * tedsrc.h - the subcommands of tedsrc (main.cpp): each one is in
 * cmd_NAME.cpp and adds itself to the program; the token list of both
 * (cmd_tok.cpp).
 */
#ifndef TEDSRC_H
#define TEDSRC_H

#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "hostenv.h"
}

void cmd_tok(CLI::App &app, int &status);
void cmd_detok(CLI::App &app, int &status);

/* the token list of a subcommand: the list file TOKENS (the first of
 * its FILE arguments) or --blitz2 BLITZ2 [--lib LIB]... */
struct tedsrc_tokens {
    std::string blitz2;
    std::vector<std::string> libs;
    std::vector<std::string> files;     /* [TOKENS] and the files of the subcommand */
};

/* the options --blitz2 and --lib and the arguments FILE... of SUB */
void tedsrc_token_options(CLI::App &sub, tedsrc_tokens &o, const std::string &files);

/* the index of the first FILE behind TOKENS (0 with --blitz2); -1 after
 * a message on stderr: --lib without --blitz2 (a usage error) */
int tedsrc_first(const CLI::App &sub, const char *me, const tedsrc_tokens &o);

/* the token list; NULL after a message on stderr */
struct tokennode *tedsrc_token_list(const CLI::App &sub, const char *me, const tedsrc_tokens &o);

#endif
