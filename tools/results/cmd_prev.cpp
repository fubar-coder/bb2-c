/*
 * results prev - the library files of this build against those of the build of
 * the previous commit (lib/prevcmp/prevcmp.h).
 *
 *   results prev [--all] [--intended PATH]... CUR PREV
 *
 * PATH is a library file.  An entry with a colon (corpus:NAME,
 * ted_ab:CASE, ... - PREV_INTENDED, lib/previntended) belongs to the
 * corpus and ted_ab: results prev only checks its syntax and ignores it.
 *
 * CUR and PREV are build directories (build/<preset>, build/<preset>/prev).
 * Exit status 0 = the library files are the same or differ as intended,
 * 1 = a difference not intended or an intended entry that does not differ,
 * 77 = PREV does not exist (no prev build: skipped), 2 = error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "prevcmp.h"
#include "previntended.h"
}
#include "results.h"

static const char *me; /* "results prev", the prefix of the messages */

void cmd_prev(CLI::App &app, int &status)
{
    struct options {
        bool all = false;
        std::vector<std::string> intended;
        std::string cur, prev;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("prev", "the library files of a build directory against those of the prev build");
    sub->add_flag("--all", o->all, "every difference is intended");
    cli::add_repeated(*sub, "--intended", o->intended, 1,
                      "a path relative to the build directory whose difference is intended (it must "
                      "differ); an entry with a colon (corpus:NAME, ted_ab:CASE ...) is one of the "
                      "corpus and ted_ab: checked for its syntax, otherwise ignored")
        ->type_name("PATH");
    sub->add_option("CUR", o->cur, "the build directory")->required();
    sub->add_option("PREV", o->prev, "the build of the previous commit")->required();
    sub->footer("Compares the library files (blitz2/acidlibs, blitz2/deflibs and every .obj below "
                "libs/ and variants/) of CUR byte by byte with those of PREV. A difference fails "
                "unless --all or --intended names it. Exit status 0 no difference that is not "
                "intended, 1 a difference or a stale --intended, 2 usage error or a malformed "
                "entry, 77 PREV does not exist.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        struct prevcmp_options po = {};
        struct prevint others = {};
        std::vector<const char *> intended;
        char err[600];
        struct stat st;
        int r;

        me = name.c_str();
        po.all = o->all;
        for (const std::string &path : o->intended) {
            int k = prevint_add(&others, path.c_str(), err, sizeof err);

            if (k < 0) {
                fprintf(stderr, "%s: %s\n", me, err);
                prevint_free(&others);
                status = 2;
                return;
            }
            if (k)
                intended.push_back(path.c_str());
        }
        intended.push_back(NULL);
        po.intended = intended.data();
        po.nintended = intended.size() - 1;
        prevint_free(&others);
        if (stat(o->prev.c_str(), &st) || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s: %s: no prev build\n", me, o->prev.c_str());
            status = 77;
            return;
        }
        r = prevcmp(o->cur.c_str(), o->prev.c_str(), &po, stdout, err, sizeof err);
        if (r == 2)
            fprintf(stderr, "%s: %s\n", me, err);
        status = r;
    });
}
