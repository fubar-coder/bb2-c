/*
 * tedsrc - Blitz 2 sources through the C core of TED on the host:
 * tokenised as TED saves them (tok), shown as TED shows them (detok).
 * README.md here; the subcommands are cmd_tok.c and cmd_detok.c.
 */
#include "subcmd.h"
#include "tedsrc.h"

static const struct subcmd table[] = {
    { "tok", cmd_tok, cmd_tok_usage },
    { "detok", cmd_detok, cmd_detok_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("tedsrc", table, sizeof table / sizeof table[0], argc, argv);
}
