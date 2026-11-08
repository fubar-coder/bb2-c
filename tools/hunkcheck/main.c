/*
 * hunkcheck - checks of the hunks of a load file: the bytes at offsets of
 * the first hunk (start), the hunk layout (layout), every CODE/DATA hunk
 * stored whole (stored).  README.md here; the subcommands are in
 * cmd_check.c.
 */
#include "subcmd.h"
#include "cmds.h"

static const struct subcmd table[] = {
    { "start", cmd_start, cmd_start_usage },
    { "layout", cmd_layout, cmd_layout_usage },
    { "stored", cmd_stored, cmd_stored_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("hunkcheck", table, sizeof table / sizeof table[0], argc, argv);
}
