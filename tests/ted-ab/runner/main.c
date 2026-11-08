/*
 * tedab - the TED A/B tests in Amiberry (run, lint) and the screen dumps of
 * teddrive they evaluate, by hand (info, text, cmp).  README.md here; the
 * subcommands are cmd_run.c and cmd_dump.c.
 */
#include "subcmd.h"
#include "cmds.h"

static const struct subcmd table[] = {
    { "run", cmd_run, cmd_run_usage },
    { "lint", cmd_lint, cmd_lint_usage },
    { "info", cmd_info, cmd_info_usage },
    { "text", cmd_text, cmd_text_usage },
    { "cmp", cmd_cmp, cmd_cmp_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("tedab", table, sizeof table / sizeof table[0], argc, argv);
}
