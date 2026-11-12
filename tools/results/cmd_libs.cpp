/*
 * results libs-compare - the cases of a library test run against the
 * expected data (lib/libscmp/libscmp.h).
 *
 *   results libs-compare RUN_DIR EXPECTED_DIR --set obj|orig [--only NAME]...
 *
 * Exit status 0 = equal, 1 = a case differs, is missing or added, or a
 * program lacks a file, 2 = wrong arguments, a malformed or unreadable
 * file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "libscmp.h"
}
#include "results.h"

/* the names as C strings (they live as long as v) */
static std::vector<const char *> c_strings(const std::vector<std::string> &v)
{
    std::vector<const char *> r;

    for (const std::string &s : v)
        r.push_back(s.c_str());
    return r;
}

/* fn(err) with its error messages prefixed by "results SUB: " on stderr */
static int prefixed(const std::string &name, const std::function<int(FILE *)> &fn)
{
    char *text = NULL;
    size_t len = 0;
    FILE *err = open_memstream(&text, &len);
    int r;

    if (!err) {
        fprintf(stderr, "%s: out of memory\n", name.c_str());
        return 2;
    }
    fflush(stdout);
    r = fn(err);
    fclose(err);
    for (char *p = text, *e; p && *p; p = e + 1) {
        if (!(e = strchr(p, '\n')))
            e = p + strlen(p) - 1;
        fprintf(stderr, "%s: %.*s\n", name.c_str(), (int)(e - p + (*e != '\n')), p);
    }
    free(text);
    return r;
}

void cmd_libs_compare(CLI::App &app, int &status)
{
    struct options {
        std::string run, expected;
        std::string set;
        std::vector<std::string> only;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("libs-compare", "the cases of a library test run against the expected data");
    sub->add_option("RUN_DIR", o->run, "the run's output (NAME.txt per program)")->required();
    sub->add_option("EXPECTED_DIR", o->expected, "the expected data (libs/tests/expected)")->required();
    sub->add_option("--set", o->set, "the library set of the run (required): obj (the C build) or "
                    "orig (the original)")
        ->type_name("SET");
    cli::add_repeated(*sub, "--only", o->only, 1, "only this program (the others are not looked at)")
        ->type_name("NAME");
    sub->footer("A case is KEY<TAB>VALUE; for --set obj the unqualified and the \"KEY [fixed #N]\" "
                "cases apply, for orig the unqualified and the \"KEY [orig]\" ones. Every differing, "
                "missing and added case is a finding (the first 11 per program on stdout). Exit "
                "status 0 equal, 1 a finding, 2 usage error or a malformed or unreadable file.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        std::vector<const char *> only = c_strings(o->only);

        if (o->set != "obj" && o->set != "orig") {
            fprintf(stderr, "%s: --set obj or --set orig is required\n", name.c_str());
            status = 2;
            return;
        }
        status = prefixed(name, [&](FILE *err) {
            return libscmp_compare_dirs(o->run.c_str(), o->expected.c_str(),
                                        o->set == "obj" ? LIBSCMP_OBJ : LIBSCMP_ORIG, only.data(),
                                        only.size(), stdout, err);
        });
    });
}
