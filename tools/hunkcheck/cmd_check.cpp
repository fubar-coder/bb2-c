/*
 * hunkcheck start, layout, stored - checks of the hunks of a load file
 * (see hunkcheck.h).
 *
 *   hunkcheck start FILE SPEC...       e.g. hunkcheck start ted 0:6100 4:48e7
 *   hunkcheck layout LAYOUT FILE       e.g. hunkcheck layout code,bss exe
 *   hunkcheck stored FILE              every CODE/DATA hunk stored whole
 *
 * Silent if everything is right; exit status 1 otherwise.
 */
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "hunkcheck.h"
}
#include "cmds.h"

namespace {

const char *const footer = "Exit status: 0 every check holds, 1 a check fails or FILE cannot be read "
                           "(a message on stderr), 2 usage error.";

/* the load file PATH; 1 with a message if it cannot be read */
int read_file(const std::string &me, const std::string &path, struct loadfile *lf)
{
    char err[200];

    if (load_read(path.c_str(), lf, err, sizeof err)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), path.c_str(), err);
        return 1;
    }
    return 0;
}

} // namespace

void cmd_start(CLI::App &app, int &status)
{
    struct options {
        std::string file;
        std::vector<std::string> specs;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("start", "the bytes at offsets of the first hunk of a load file");
    sub->add_option("FILE", o->file, "the load file")->required();
    sub->add_option("SPEC", o->specs,
                    "OFFSET:HEXBYTES, an offset into the first hunk (decimal, or hexadecimal with 0x) "
                    "and the bytes expected there in hexadecimal, e.g. 0:6100")
        ->required();
    sub->footer(footer);
    sub->callback([sub, o, &status] {
        struct loadfile lf;
        std::vector<char *> specs;

        if ((status = read_file(cli::name(*sub), o->file, &lf)))
            return;
        for (auto &s : o->specs)
            specs.push_back(s.data());
        status = hunk_check(&lf, int(specs.size()), specs.data(), o->file.c_str(), stderr);
        load_free(&lf);
    });
}

void cmd_layout(CLI::App &app, int &status)
{
    struct options {
        std::string layout, file;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("layout", "the hunks of a load file: number, types and chip flags");
    sub->add_option("LAYOUT", o->layout,
                    "code|data|bss[+chip],... - the hunks in order, e.g. code,bss,data+chip")
        ->required();
    sub->add_option("FILE", o->file, "the load file")->required();
    sub->footer(footer);
    sub->callback([sub, o, &status] {
        struct loadfile lf;

        if ((status = read_file(cli::name(*sub), o->file, &lf)))
            return;
        status = hunk_layout(&lf, o->layout.c_str(), o->file.c_str(), stderr);
        load_free(&lf);
    });
}

void cmd_stored(CLI::App &app, int &status)
{
    auto file = std::make_shared<std::string>();
    CLI::App *sub = app.add_subcommand("stored", "no CODE/DATA hunk of a load file is larger than its stored block");
    sub->add_option("FILE", *file, "the load file")->required();
    sub->footer(footer);
    sub->callback([sub, file, &status] {
        struct loadfile lf;

        if ((status = read_file(cli::name(*sub), *file, &lf)))
            return;
        status = hunk_stored(&lf, file->c_str(), stderr);
        load_free(&lf);
    });
}
