/*
 * blitzlib entrypoints - the __entrypoint routines of a library's objects
 * (BLITZ_ENTRYPOINT, tools/lib/epcheck):
 *
 *   blitzlib entrypoints --objdump OBJDUMP ARCHIVE|OBJECT...
 *
 * Runs the toolchain's objdump (-t and -d) on each file and refuses a
 * routine that writes a6 without saving it, a call or jump into a routine
 * from other code, and a routine folded into another one.  Silent if all
 * is right; exit status 1 with the findings on stderr.
 */
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "epcheck.h"
}
#include "cmds.h"

/* the standard output of cmd, or false */
static bool run(const std::string &cmd, std::string &out)
{
    FILE *p = popen(cmd.c_str(), "r");
    char buf[65536];
    size_t n;

    out.clear();
    if (!p)
        return false;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0)
        out.append(buf, n);
    return pclose(p) == 0;
}

/* a shell word: 'text' with ' as '\'' */
static std::string quote(const std::string &s)
{
    std::string q = "'";

    for (char c : s)
        q += c == '\'' ? std::string("'\\''") : std::string(1, c);
    return q + "'";
}

void cmd_entrypoints(CLI::App &app, int &status)
{
    auto files = std::make_shared<std::vector<std::string>>();
    auto objdump = std::make_shared<std::string>();
    CLI::App *sub = app.add_subcommand("entrypoints",
                                       "check the __entrypoint routines of a library's objects");
    sub->add_option("--objdump", *objdump, "the toolchain's objdump")
        ->type_name("OBJDUMP")
        ->required();
    sub->add_option("FILE", *files, "the static library or the objects of the library")
        ->required();
    sub->footer("A routine compiled with BLITZ_ENTRYPOINT (GCC's __entrypoint: no "
                "callee-saved register saved) and marked by BLITZ_ALIAS_EP must not write a6 "
                "without saving it at its start (the register rule keeps a6), must not be "
                "called or jumped into from other code (GCC does not know that it keeps no "
                "d2-d7/a2; identical code folding makes a function with the same body a jump "
                "to it), and must not begin with a jump into another routine. Silent if all "
                "is right. Exit status 0, 1 a finding (on stderr), 2 usage error.");
    sub->callback([sub, files, objdump, &status] {
        const std::string name = cli::name(*sub);
        int bad = 0;

        for (const std::string &path : *files) {
            std::string symtab, disasm;
            long n, found;

            if (!run(quote(*objdump) + " -t " + quote(path), symtab) ||
                !run(quote(*objdump) + " -d --no-show-raw-insn " + quote(path), disasm)) {
                fprintf(stderr, "%s: %s: objdump failed\n", name.c_str(), path.c_str());
                bad = 1;
                continue;
            }
            found = epcheck_text(symtab.c_str(), disasm.c_str(), stderr, &n);
            if (found < 0) {
                fprintf(stderr, "%s: %s: out of memory\n", name.c_str(), path.c_str());
                bad = 1;
            } else if (found) {
                fprintf(stderr, "%s: %s: %ld findings in %ld __entrypoint routines\n",
                        name.c_str(), path.c_str(), found, n);
                bad = 1;
            }
        }
        status = bad;
    });
}
