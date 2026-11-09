// fake_cli11 - a program with two subcommands, alpha and beta, on the
// command-line shell of the host programs (tools/lib/cli), for the tests
// of cmake/check-help-cli.cmake and cmake/check-readme-cli.cmake
// (tools/tests/CMakeLists.txt) and the example of docs/development.md.
// Built twice: fake_cli11_broken (FAKE_BROKEN) also writes a message to
// stderr with the help of beta.
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "cli.hpp"

// alpha [-x] FILE: prints its name and FILE
static void cmd_alpha(CLI::App &app, int &status)
{
    struct options {
        bool x = false;
        std::string file;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("alpha", "the first subcommand");
    sub->add_flag("-x", o->x, "a flag");
    sub->add_option("FILE", o->file, "the input")->required();
    sub->callback([sub, o, &status] {
        std::printf("%s: %s%s\n", cli::name(*sub).c_str(), o->file.c_str(), o->x ? " -x" : "");
        status = 0;
    });
}

// beta [--all]
static void cmd_beta(CLI::App &app, int &status)
{
    auto all = std::make_shared<bool>(false);
    CLI::App *sub = app.add_subcommand("beta", "the second subcommand");
    sub->add_flag("--all", *all, "all of them");
    sub->footer("Exit status: 0 with --all, 1 without, 2 usage error.");
#ifdef FAKE_BROKEN
    sub->preparse_callback([](std::size_t) { std::cerr << "fake_cli11 beta: a message\n"; });
#endif
    sub->callback([all, &status] { status = *all ? 0 : 1; });
}

int main(int argc, char **argv)
{
    CLI::App app{"The fake program of the help and README checks.", "fake_cli11"};
    int status = 0;
    app.require_subcommand(1);
    cmd_alpha(app, status);
    cmd_beta(app, status);
    return cli::run(app, argc, argv, status);
}
