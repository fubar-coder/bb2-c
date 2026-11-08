/*
 * blitzlib - Blitz Basic 2 library files: join them into a BigLib file
 * (join), take one out (extract), no library number twice (numbers), the
 * checks of a library file (check), the declarations against the original
 * (compare), the command table as JSON (cmdref).  README.md here; the
 * subcommands are cmd_biglib.c (join, extract, numbers) and cmd_NAME.c.
 */
#include "subcmd.h"
#include "cmds.h"

static const struct subcmd table[] = {
    { "join", cmd_join, cmd_join_usage },
    { "extract", cmd_extract, cmd_extract_usage },
    { "numbers", cmd_numbers, cmd_numbers_usage },
    { "check", cmd_check, cmd_check_usage },
    { "compare", cmd_compare, cmd_compare_usage },
    { "cmdref", cmd_cmdref, cmd_cmdref_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("blitzlib", table, sizeof table / sizeof table[0], argc, argv);
}
