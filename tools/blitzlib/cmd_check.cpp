/*
 * blitzlib check - check Blitz Basic 2 library files (see blitzlib.h).
 *
 *   blitzlib check [--allow-33] LIB.obj...
 *
 * Silent if everything is right; exit status 1 if a file is not a valid
 * library file, or has a reserved number (32, or 33 without being the
 * literal library and without --allow-33; #1018).
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

int check_reserved(const char *pre, const unsigned char *data, size_t size, const char *what,
                   int allow33)
{
    struct loadfile lf;
    struct blitzlib lib;
    char err[200];
    int r;

    if (load_parse(data, size, &lf, err, sizeof err))
        return 0;                       /* no load file: not for this check */
    if (!lf.nhunks || blitzlib_read(&lf.hunks[0], &lib, err, sizeof err)) {
        load_free(&lf);
        return 0;
    }
    r = blitzlib_reserved(&lib, allow33);
    if (r == BLITZLIB_RESERVE_32 || r == BLITZLIB_RESERVE_33)
        fprintf(stderr, "%s: %s: library number %u is reserved for the literal characters "
                        "(libraries 32 and 33, docs/token-table.md)%s\n", pre, what,
                (unsigned)lib.libnum,
                r == BLITZLIB_RESERVE_33 ? "; 33 only for literallib or with --allow-33" : "");
    blitzlib_free(&lib);
    load_free(&lf);
    return r == BLITZLIB_RESERVE_32 || r == BLITZLIB_RESERVE_33;
}

void cmd_check(CLI::App &app, int &status)
{
    auto files = std::make_shared<std::vector<std::string>>();
    auto allow = std::make_shared<bool>(false);
    CLI::App *sub = app.add_subcommand("check", "check Blitz Basic 2 library files");
    sub->add_option("LIB.obj", *files, "the library files")->required();
    sub->add_flag("--allow-33", *allow,
                  "a library 33 other than literallib is allowed (RIDosLib in the RWE variants)");
    sub->footer("One code hunk, relocations only into it, the library header, the token table "
                "readable up to its end, no reference from the normal code into the error check "
                "area, not the reserved library numbers 32 and 33 (33 only for literallib, the "
                "literal characters, or with --allow-33). Silent if all is right. Exit status 0, "
                "1 a file is no valid library file "
                "(the findings on stderr), 2 usage error.");
    sub->callback([sub, files, allow, &status] {
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
            {
                FILE *f = fopen(path.c_str(), "rb");
                std::vector<unsigned char> buf;
                int c;

                while (f && (c = fgetc(f)) != EOF)
                    buf.push_back((unsigned char)c);
                if (f)
                    fclose(f);
                if (check_reserved(me, buf.data(), buf.size(), path.c_str(), *allow))
                    bad = 1;
            }
        }
        status = bad;
    });
}
