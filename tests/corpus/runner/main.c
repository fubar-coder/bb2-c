/*
 * corpusrun - the emulator passes of the corpus (pass: corpus.h) and of
 * the CD through one compiler (cd).  README.md here; the subcommands are
 * cmd_pass.c and cmd_cd.c.
 */
#include "subcmd.h"
#include "cmds.h"

static const struct subcmd table[] = {
    { "pass", cmd_pass, cmd_pass_usage },
    { "cd", cmd_cd, cmd_cd_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("corpusrun", table, sizeof table / sizeof table[0], argc, argv);
}
