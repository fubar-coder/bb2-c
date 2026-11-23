/*
 * testreg - the registration of the Blitz tests of the model at build time
 * (tools/lib/testreg/testreg.h, README.md here):
 *
 *   testreg --prefix P --output FILE [--env FILE] [--depfile FILE]
 *           [--tokens FILE]... [--cell-fixture SELECTOR|FIXTURE]...
 *           [--timeout-default SEC] [--timeout-extra SEC] [--order FILE] [--costs FILE]
 *           [--test-env FILE] [--no-bundle] --command WORD [--command WORD]... SOURCE...
 *
 * Reads every source and its configuration, checks them, and writes the
 * ctest include file (only when nothing failed) and the depfile; the case
 * tests that may share a session are put into bundle tests unless
 * --no-bundle is given.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "testcosts.h"
#include "testreg.h"
}

namespace fs = std::filesystem;

static int run(const std::string &prefix, const std::string &output, const std::string &envfile,
               const std::string &depfile, const std::string &orderfile, const std::string &costfile,
               const std::string &testenv, const std::vector<std::string> &tokfiles,
               long long tdef, long long textra, bool bundle,
               const std::vector<std::string> &command, const std::vector<std::string> &cellfix,
               const std::vector<std::string> &sources)
{
    tc_env env = {};
    treg_strs tokens = {};
    treg_result r = {};
    treg_opts o = {};
    treg_entries order = {};
    tcosts costs = {};
    char err[800];
    int rc = 0;

    if (!envfile.empty()) {
        if (tc_env_load(envfile.c_str(), &env, err, sizeof err)) {
            fprintf(stderr, "testreg: %s\n", err);
            return 1;
        }
    }
    for (const std::string &t : tokfiles)
        if (treg_tokens_load(t.c_str(), &tokens, err, sizeof err)) {
            fprintf(stderr, "testreg: %s\n", err);
            rc = 1;
        }
    o.prefix = prefix.c_str();
    o.env = envfile.empty() ? nullptr : &env;
    o.tokens = tokens.v;
    o.ntokens = tokens.n;
    o.timeout_default = tdef;
    o.timeout_extra = textra;
    o.bundle = bundle;
    std::vector<const char *> cf;
    for (const std::string &c : cellfix) {
        if (treg_cell_fixture_ok(c.c_str(), err, sizeof err)) {
            fprintf(stderr, "testreg: --cell-fixture %s\n", err);
            rc = 1;
        }
        cf.push_back(c.c_str());
    }
    o.cell_fixtures = cf.data();
    o.ncell_fixtures = cf.size();
    for (const std::string &s : sources)
        if (treg_add_source(s.c_str(), &o, &r) < 0) {
            fprintf(stderr, "testreg: out of memory\n");
            rc = 1;
        }
    if (bundle && treg_make_bundles(&r, &o) < 0) {
        fprintf(stderr, "testreg: out of memory\n");
        rc = 1;
    }
    if (treg_check_names(&r) < 0) {
        fprintf(stderr, "testreg: out of memory\n");
        rc = 1;
    }
    if (!orderfile.empty()) {
        if (treg_entries_load(orderfile.c_str(), &order, err, sizeof err)) {
            fprintf(stderr, "testreg: %s\n", err);
            rc = 1;
        } else if (treg_check_entries(&r, &order) < 0) {
            fprintf(stderr, "testreg: out of memory\n");
            rc = 1;
        }
    }
    if (!costfile.empty()) {
        if (tcosts_load(costfile.c_str(), &costs, err, sizeof err)) {
            fprintf(stderr, "testreg: %s\n", err);
            rc = 1;
        } else {
            treg_apply_costs(&r, &costs);
        }
    }
    for (size_t i = 0; i < r.errors.n; i++)
        fprintf(stderr, "%s\n", r.errors.v[i]);
    if (r.errors.n)
        rc = 1;

    /* the depfile: also after an error, so that a change of a file read reruns the step */
    std::vector<std::string> deps;
    if (!envfile.empty())
        deps.push_back(envfile);
    deps.insert(deps.end(), tokfiles.begin(), tokfiles.end());
    if (!orderfile.empty())
        deps.push_back(orderfile);
    if (!costfile.empty())
        deps.push_back(costfile);
    for (const std::string &d : deps) {
        char **v = (char **)realloc(r.deps.v, (r.deps.n + 1) * sizeof *v);

        if (!v || !(v[r.deps.n] = strdup(d.c_str()))) {
            if (v)
                r.deps.v = v;
            fprintf(stderr, "testreg: out of memory\n");
            rc = 1;
            break;
        }
        r.deps.v = v;
        r.deps.n++;
    }
    if (!depfile.empty()) {
        FILE *f = fopen(depfile.c_str(), "w");

        if (!f || treg_write_depfile(f, output.c_str(), &r) || fclose(f)) {
            fprintf(stderr, "testreg: cannot write %s\n", depfile.c_str());
            rc = 1;
        }
    }
    if (!rc) {
        std::string tmp = output + ".tmp";
        std::vector<const char *> cmd;
        FILE *f = fopen(tmp.c_str(), "w");

        for (const std::string &c : command)
            cmd.push_back(c.c_str());
        if (!f || treg_write_include(f, &r, cmd.data(), cmd.size(), &order,
                                      testenv.empty() ? nullptr : testenv.c_str()) || fclose(f)) {
            fprintf(stderr, "testreg: cannot write %s (or a value cannot be quoted)\n", output.c_str());
            rc = 1;
        } else {
            std::error_code ec;

            fs::rename(tmp, output, ec);
            if (ec) {
                fprintf(stderr, "testreg: cannot write %s\n", output.c_str());
                rc = 1;
            }
        }
        if (rc)
            fs::remove(tmp);
    }
    treg_free(&r);
    treg_entries_free(&order);
    tcosts_free(&costs);
    treg_strs_free(&tokens);
    tc_env_free(&env);
    return rc;
}

int main(int argc, char **argv)
{
    CLI::App app{"The ctest tests of the Blitz test sources of the model (ID.test.bas), one per case and side, "
                 "from the sources and their run configurations.",
                 "testreg"};
    std::string prefix, output, envfile, depfile, orderfile, costfile, testenv;
    std::vector<std::string> tokfiles, command, cellfix, sources;
    long long tdef = 90, textra = 0;
    bool nobundle = false;
    int status = 0;

    app.add_option("SOURCE", sources, "the test sources (ID.test.bas)")->required()->type_name("FILE");
    app.add_option("--prefix", prefix, "the prefix of the test names (bug_orig_, regression_, blitz2_)")
        ->required()
        ->type_name("P");
    app.add_option("--output", output, "the ctest include file (TEST_INCLUDE_FILES)")
        ->required()
        ->type_name("FILE");
    app.add_option("--command", command,
                   "a word of the command of every test, once per word; then --source SOURCE --case CASE --side SIDE")
        ->required()
        ->expected(1)
        ->allow_extra_args(false)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
        ->type_name("WORD");
    app.add_option("--env", envfile, "the environment file of the build (placeholders, top)")->type_name("FILE");
    app.add_option("--depfile", depfile, "the depfile: every file read")->type_name("FILE");
    app.add_option("--tokens", tokfiles, "a token list (HEX NAME lines); case names must be no token")
        ->expected(1)
        ->allow_extra_args(false)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
        ->type_name("FILE");
    app.add_option("--cell-fixture", cellfix,
                   "SELECTOR|FIXTURE: a test whose side has a cell the selector matches requires the fixture "
                   "(any number)")
        ->expected(1)
        ->allow_extra_args(false)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)
        ->type_name("SPEC");
    app.add_option("--timeout-default", tdef, "the time limit of a cell without timeout (default 90)")
        ->check(CLI::PositiveNumber)
        ->type_name("SEC");
    app.add_option("--timeout-extra", textra, "added to the sum of the cells' time limits (default 0)")
        ->check(CLI::NonNegativeNumber)
        ->type_name("SEC");
    app.add_option("--order", orderfile,
                   "the bug-orig tests of the old form (cmake/bug-orig-order.cmake): declared with the tests of "
                   "the label bug-orig, the largest COST first")
        ->type_name("FILE");
    app.add_option("--costs", costfile,
                   "the measured COST of the tests (tests/data/test-costs.txt): an entry replaces the sum of "
                   "the cells' session limits, the configuration's cost wins over it; an entry that names no "
                   "registered test is ignored")
        ->type_name("FILE");
    app.add_option("--test-env", testenv,
                   "a ctest script that defines basic_test_environment(NAME LABELS), the environment of a test "
                   "(cmake/basic-test-model.cmake): included by the include file, called for each test")
        ->type_name("FILE");
    app.add_flag("--no-bundle", nobundle,
                 "no bundle tests: every case test runs alone (without it, the case tests that may share an "
                 "emulator session with other sources are put into bundle tests PREFIXbundle_N of 16 at most, "
                 "each served from its bundle)");
    app.footer("The include file is written only when every source and configuration passed the checks; "
               "the depfile is written in any case. Exit status 0 ok, 1 a check failed or a file cannot be read "
               "or written, 2 usage error.");
    app.callback([&] {
        status = run(prefix, output, envfile, depfile, orderfile, costfile, testenv, tokfiles, tdef, textra, !nobundle,
                     command, cellfix, sources);
    });
    return cli::run(app, argc, argv, status);
}
