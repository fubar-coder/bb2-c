/*
 * results fix-scope - the cases of the expected data a branch changes that
 * are not the [fixed #N] cases of its issues (lib/libscmp/libscmp.h,
 * design D12 of the OpenSpec change libs-tests-capture).
 *
 *   results fix-scope BASE_DIR BRANCH_DIR [--fixes N]... [--log FILE] [--bug-orig|--corpus]
 *
 * With --bug-orig the directories are trees of bug-orig tests
 * (tests/bug-orig), projected onto cases (lib/bugorigcmp/bugorigcmp.h), with
 * --corpus the expected data of the corpus (tests/corpus/expect, one
 * directory per pass; lib/corpusexpect/corpusexpect.h).
 *
 * Exit status 0 = only the named issues' cases changed, 1 = a case
 * reported, 2 = wrong arguments, a malformed or unreadable file (or test).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "bugorigcmp.h"
#include "corpusexpect.h"
#include "libscmp.h"
}
#include "results.h"

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    std::string s;
    char buf[4096];
    size_t n;

    if (!f)
        return NULL;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        s.append(buf, n);
    fclose(f);
    char *r = (char *)malloc(s.size() + 1);
    if (r)
        memcpy(r, s.c_str(), s.size() + 1);
    return r;
}

void cmd_fix_scope(CLI::App &app, int &status)
{
    struct options {
        std::string base, branch, log;
        std::vector<unsigned> fixes;
        bool bug_orig = false, corpus = false;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("fix-scope", "the cases of the expected data a branch changes beyond its issues");
    sub->add_option("BASE_DIR", o->base, "the expected data of main (NAME.txt per program)")->required();
    sub->add_option("BRANCH_DIR", o->branch, "the expected data of the branch")->required();
    sub->add_option("--fixes", o->fixes, "an issue the branch fixes")
        ->type_size(1)
        ->expected(1)
        ->allow_extra_args(false)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
        ->type_name("N");
    sub->add_option("--log", o->log, "commit messages: their lines \"Fixes #N\" name issues too")
        ->type_name("FILE");
    sub->add_flag("--bug-orig", o->bug_orig, "BASE_DIR and BRANCH_DIR are trees of bug-orig tests "
                  "(tests/bug-orig), each test projected onto cases");
    sub->add_flag("--corpus", o->corpus, "BASE_DIR and BRANCH_DIR are the expected data of the corpus "
                  "(tests/corpus/expect), each file a case PASS|NAME|OBS|1");
    sub->footer("Every case (a key with its qualifier) added, changed or removed is reported unless "
                "it is a \"[fixed #N]\" case of a named issue, the \"[orig]\" case paired with one, "
                "or an unqualified case that became such a pair. A bug-orig test's cases: one per "
                "section of ID.expect and ID.orig, \"[fixed #N]\" with its header \"; @fixed #N\". "
                "A corpus file's case: NAME.OBS unqualified, NAME.orig.OBS [orig], "
                "NAME.fixed-N.OBS [fixed #N]. "
                "Exit status 0 nothing reported, 1 "
                "a case reported, 2 usage error or a malformed or unreadable file.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        struct libscmp_issues is = {};
        int bad = 0;

        for (unsigned n : o->fixes) {
            if (!n || n > 999999999u) {
                fprintf(stderr, "%s: --fixes %u: not an issue number\n", name.c_str(), n);
                libscmp_issues_free(&is);
                status = 2;
                return;
            }
            bad |= libscmp_issues_add(&is, n);
        }
        if (!o->log.empty()) {
            char *log = read_text(o->log.c_str());

            if (!log) {
                fprintf(stderr, "%s: %s: cannot read it\n", name.c_str(), o->log.c_str());
                libscmp_issues_free(&is);
                status = 2;
                return;
            }
            bad |= libscmp_issues_from_log(&is, log);
            free(log);
        }
        if (bad) {
            fprintf(stderr, "%s: out of memory\n", name.c_str());
            libscmp_issues_free(&is);
            status = 2;
            return;
        }
        if (o->bug_orig && o->corpus) {
            fprintf(stderr, "%s: --bug-orig and --corpus exclude each other\n", name.c_str());
            libscmp_issues_free(&is);
            status = 2;
            return;
        }
        status = prefixed(name, [&](FILE *err) {
            if (o->corpus)
                return cexp_fix_scope_trees(o->base.c_str(), o->branch.c_str(), &is, stdout, err);
            if (o->bug_orig)
                return bugorigcmp_fix_scope_trees(o->base.c_str(), o->branch.c_str(), &is, stdout,
                                                  err);
            return libscmp_fix_scope_dirs(o->base.c_str(), o->branch.c_str(), &is, stdout, err);
        });
        libscmp_issues_free(&is);
    });
}
