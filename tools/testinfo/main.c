/*
 * testinfo - records of test runs: statistics of the runs (stats), the
 * coverage of each test (map) and the tests a change can affect (select).
 * README.md here; the subcommands are cmd_NAME.c.
 */
#include "subcmd.h"
#include "testinfo.h"

static const struct subcmd table[] = {
    { "stats", cmd_stats, cmd_stats_usage },
    { "map", cmd_map, cmd_map_usage },
    { "select", cmd_select, cmd_select_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("testinfo", table, sizeof table / sizeof table[0], argc, argv);
}
