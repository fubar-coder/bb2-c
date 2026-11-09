/*
 * blitzlib check - check Blitz Basic 2 library files (see blitzlib.h).
 *
 *   blitzlib check LIB.obj...
 *
 * Silent if everything is right; exit status 1 if a file is not a valid
 * library file.
 */
#include <stdio.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "blitzlib.h"
}
#include "cmds.h"

static const char *me; /* "blitzlib check", the prefix of the messages */

void cmd_check(CLI::App &app, int &status)
{
    auto files = std::make_shared<std::vector<std::string>>();
    CLI::App *sub = app.add_subcommand("check", "check Blitz Basic 2 library files");
    sub->add_option("LIB.obj", *files, "the library files")->required();
    sub->footer("One code hunk, relocations only into it, the library header, the token table "
                "readable up to its end, no reference from the normal code into the error check "
                "area. Silent if all is right. Exit status 0, 1 a file is no valid library file "
                "(the findings on stderr), 2 usage error.");
    sub->callback([sub, files, &status] {
        const std::string name = cli::name(*sub);
        int bad = 0;

        me = name.c_str();
        for (const std::string &path : *files) {
            struct loadfile lf;
            char err[200];

            if (load_read(path.c_str(), &lf, err, sizeof err)) {
                fprintf(stderr, "%s: %s: %s\n", me, path.c_str(), err);
                bad = 1;
                continue;
            }
            if (blitzlib_check(&lf, path.c_str(), stderr))
                bad = 1;
            load_free(&lf);
        }
        status = bad;
    });
}
