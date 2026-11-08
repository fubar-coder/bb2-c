/*
 * results - a result against its expectation or the previous result: the
 * output of a test run against an expect file (match), the library files
 * against the prev build (prev), the CD result table from a fresh run
 * (cd-update, cd-reference).  README.md here; the subcommands are
 * cmd_NAME.c.
 */
#include "subcmd.h"
#include "results.h"

static const struct subcmd table[] = {
    { "match", cmd_match, cmd_match_usage },
    { "prev", cmd_prev, cmd_prev_usage },
    { "cd-update", cmd_cd_update, cmd_cd_update_usage },
    { "cd-reference", cmd_cd_reference, cmd_cd_reference_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("results", table, sizeof table / sizeof table[0], argc, argv);
}
