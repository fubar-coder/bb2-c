/*
 * results - a result against its expectation or the previous result: the
 * output of a test run against an expect file (match), the library files
 * against the prev build (prev), the CD result table from a fresh run
 * (cd-update, cd-reference), the oracle data of the TED tests from a fresh
 * run (ted-oracle), the cases of the library tests against their expected
 * data and that data written (libs-compare, libs-accept, libs-update,
 * libs-dump), the cases a branch changes beyond its issues (fix-scope), the
 * recorded original of a bug-orig test from two runs (bug-orig-accept), the
 * expected data of a corpus pass from two runs (corpus-accept).
 * README.md here; the subcommands are cmd_NAME.cpp (the libs-* ones in
 * cmd_libs.cpp).
 */
#include "cli.hpp"
#include "results.h"

int main(int argc, char **argv)
{
    CLI::App app{"A result against its expectation or the previous result.", "results"};
    int status = 0;

    app.require_subcommand(1);
    cmd_match(app, status);
    cmd_prev(app, status);
    cmd_cd_update(app, status);
    cmd_cd_reference(app, status);
    cmd_ted_oracle(app, status);
    cmd_libs_compare(app, status);
    cmd_libs_accept(app, status);
    cmd_libs_update(app, status);
    cmd_libs_dump(app, status);
    cmd_fix_scope(app, status);
    cmd_bug_orig_accept(app, status);
    cmd_corpus_accept(app, status);
    return cli::run(app, argc, argv, status);
}
