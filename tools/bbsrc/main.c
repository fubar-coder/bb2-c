/*
 * bbsrc - Blitz Basic 2 sources and tokens without TED: the token list of
 * a compiler and its libraries (tokens), a tokenised source as text
 * (detok), lines of a tokenised source edited (comment), BB22 sources in
 * TED's format (bb22, bb22-info), a source tokenised as AmiBlitz3 loads it
 * (ab3, ab3-cmp).  README.md here; the subcommands are cmd_*.c.
 */
#include "subcmd.h"
#include "bbsrc.h"

static const struct subcmd table[] = {
    { "tokens", cmd_tokens, cmd_tokens_usage },
    { "detok", cmd_detok, cmd_detok_usage },
    { "comment", cmd_comment, cmd_comment_usage },
    { "bb22", cmd_bb22, cmd_bb22_usage },
    { "bb22-info", cmd_bb22_info, cmd_bb22_info_usage },
    { "ab3", cmd_ab3, cmd_ab3_usage },
    { "ab3-cmp", cmd_ab3_cmp, cmd_ab3_cmp_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("bbsrc", table, sizeof table / sizeof table[0], argc, argv);
}
