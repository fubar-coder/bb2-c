/*
 * blitzres - Blitz Basic 2 resident files: one with constants written
 * (create), the contents of one (dump), its constants as Blitz source
 * (source).  README.md here; the subcommands are in cmd_res.c.
 */
#include "subcmd.h"
#include "cmds.h"

static const struct subcmd table[] = {
    { "create", cmd_create, cmd_create_usage },
    { "dump", cmd_dump, cmd_dump_usage },
    { "source", cmd_source, cmd_source_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("blitzres", table, sizeof table / sizeof table[0], argc, argv);
}
