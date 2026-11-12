/*
 * results libs-compare, libs-accept, libs-update - the cases of the
 * library tests against the expected data, and the expected data written
 * from two recording runs of the original or from a run of the port
 * (lib/libscmp/libscmp.h).
 *
 *   results libs-compare RUN_DIR EXPECTED_DIR --set obj|orig [--only NAME]...
 *   results libs-accept RUN1_DIR RUN2_DIR EXPECTED_DIR [--only NAME]...
 *   results libs-update RUN_DIR EXPECTED_DIR [--only NAME]... [--fix KEY#N]...
 *
 * Exit status 0 = equal or written, 1 = a case differs, is missing or
 * added, a program lacks a file, or the runs are refused (nothing
 * written), 2 = wrong arguments, a malformed, unreadable or unwritable
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

void cmd_libs_accept(CLI::App &app, int &status)
{
    struct options {
        std::string run1, run2, expected;
        std::vector<std::string> only;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("libs-accept", "the expected data of the library tests from two recording runs of the original");
    sub->add_option("RUN1_DIR", o->run1, "the output of the first run (NAME.txt per program)")->required();
    sub->add_option("RUN2_DIR", o->run2, "the output of the second run, another memory layout")->required();
    sub->add_option("EXPECTED_DIR", o->expected, "the expected data written (libs/tests/expected)")->required();
    cli::add_repeated(*sub, "--only", o->only, 1, "only this program (the other files stay as they are)")
        ->type_name("NAME");
    sub->footer("The two runs of a program must be identical (else \"test not deterministic: NAME\" "
                "and the differing cases) and hold no \"lost\" case. The file gets the run's cases; "
                "a key with a \"[fixed #N]\" case keeps it and gets the run's value as \"[orig]\". "
                "Nothing is written unless every program is accepted. Exit status 0 written, 1 "
                "refused, 2 usage error or a malformed, unreadable or unwritable file.");
    sub->callback([sub, o, &status] {
        std::vector<const char *> only = c_strings(o->only);

        status = prefixed(cli::name(*sub), [&](FILE *err) {
            return libscmp_accept_dirs(o->run1.c_str(), o->run2.c_str(), o->expected.c_str(),
                                       only.data(), only.size(), stdout, err);
        });
    });
}

void cmd_libs_update(CLI::App &app, int &status)
{
    struct options {
        std::string run, expected;
        std::vector<std::string> only, fix;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("libs-update", "the [fixed #N] cases of the library tests from a run of the port");
    sub->add_option("RUN_DIR", o->run, "the output of the port's run (NAME.txt per program)")->required();
    sub->add_option("EXPECTED_DIR", o->expected, "the expected data (libs/tests/expected)")->required();
    cli::add_repeated(*sub, "--only", o->only, 1, "only this program (the other files stay as they are)")
        ->type_name("NAME");
    cli::add_repeated(*sub, "--fix", o->fix, 1,
                      "make the unqualified case KEY the pair \"KEY [orig]\" (the old value) and "
                      "\"KEY [fixed #N]\" (the port's)")
        ->type_name("KEY#N");
    sub->footer("Every \"[fixed #N]\" case gets the port's value; nothing else changes (an "
                "unqualified case the port gives another value is reported). A --fix must apply "
                "in exactly one program. Nothing is written unless all is well. Exit status 0 "
                "written, 1 refused, 2 usage error or a malformed, unreadable or unwritable file.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        std::vector<const char *> only = c_strings(o->only);
        std::vector<libscmp_fix> fixes(o->fix.size());
        char err[700];

        for (size_t i = 0; i < o->fix.size(); i++)
            if (libscmp_parse_fix(o->fix[i].c_str(), &fixes[i], err, sizeof err)) {
                fprintf(stderr, "%s: %s\n", name.c_str(), err);
                for (libscmp_fix &f : fixes)
                    libscmp_fix_free(&f);
                status = 2;
                return;
            }
        status = prefixed(name, [&](FILE *e) {
            return libscmp_update_dirs(o->run.c_str(), o->expected.c_str(), only.data(),
                                       only.size(), fixes.data(), fixes.size(), stdout, e);
        });
        for (libscmp_fix &f : fixes)
            libscmp_fix_free(&f);
    });
}
