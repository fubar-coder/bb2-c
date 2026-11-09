/*
 * xtra - the companion settings of a Blitz source (shared/xtra, xtra.h):
 * all fields (dump), read and written again (check), one format into the
 * other (convert).  README.md here; the subcommands are in cmd_xtra.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"The companion settings of a Blitz source: the Blitz 2 .xtra (vers 0 and 1), the cursor "
                 "in the header of a BB22 source, AmiBlitz3's embedded header (docs/formats.md 1 and 7).",
                 "xtra"};
    int status = 0;

    app.require_subcommand(1);
    cmd_dump(app, status);
    cmd_check(app, status);
    cmd_convert(app, status);
    return cli::run(app, argc, argv, status);
}
