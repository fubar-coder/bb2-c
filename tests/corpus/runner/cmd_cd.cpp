/*
 * corpusrun cd - the subcommand of the CD run (every .bb2/.bb of the CD
 * compiled by one compiler in Amiberry); its body is cd.c (the keys of
 * the pass file: its header).
 *
 *   corpusrun cd PASSFILE
 */
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "cd.h"
}
#include "cmds.h"

void cmd_cd(CLI::App &app, int &status)
{
    auto passfile = std::make_shared<std::string>();
    CLI::App *sub = app.add_subcommand("cd", "compile every .bb2/.bb of the CD with one compiler in Amiberry, a table of the results");
    sub->add_option("PASSFILE", *passfile, "the pass file (written by CMake; its keys: the header of "
                                           "tests/corpus/runner/cd.c)")
        ->required();
    sub->footer("The table is optionally compared with a reference table. CDRUN_FORCE=1 in the "
                "environment runs even if the cache= record says nothing changed. Exit status 0 "
                "written (and no difference), 1 differences or a source= that does not compile, "
                "2 error (usage, pass file), 77 the emulator or a file is missing (skipped).");
    sub->callback([sub, passfile, &status] {
        const std::string name = cli::name(*sub);

        status = corpusrun_cd(name.c_str(), passfile->c_str());
    });
}
