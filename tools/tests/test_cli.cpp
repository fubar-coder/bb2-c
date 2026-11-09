// test_cli - the command-line shell of the host programs (tools/lib/cli,
// cli::run): help of the program and of a subcommand (0, stdout), every
// command line CLI11 rejects (2, stderr, nothing run), and the status of a
// subcommand passed on (1, 77).
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>

#include "cli.hpp"

namespace {

struct result {
    int status;
    std::string out, err;
    int runs;
};

// "prog" with the subcommands run (FILE, --code N, a validator that
// throws for FILE "throw") and other
result call(std::vector<std::string> args)
{
    CLI::App app{"a test program", "prog"};
    int status = 0;
    auto runs = std::make_shared<int>(0);
    auto file = std::make_shared<std::string>();
    auto code = std::make_shared<int>(0);
    app.require_subcommand(1);
    CLI::App *sub = app.add_subcommand("run", "runs");
    sub->add_option("--code", *code, "the status");
    sub->add_option("FILE", *file, "a file")->required()->check([](const std::string &s) {
        if (s == "throw")
            throw std::runtime_error("thrown");
        return std::string();
    });
    sub->callback([&status, runs, code] {
        ++*runs;
        status = *code;
    });
    app.add_subcommand("other", "other")->callback([&status, runs] {
        ++*runs;
        status = 0;
    });

    std::vector<char *> argv;
    args.insert(args.begin(), "prog");
    for (auto &a : args)
        argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream out, err;
    auto *old_out = std::cout.rdbuf(out.rdbuf());
    auto *old_err = std::cerr.rdbuf(err.rdbuf());
    int rc = cli::run(app, int(args.size()), argv.data(), status);
    std::cout.rdbuf(old_out);
    std::cerr.rdbuf(old_err);
    return {rc, out.str(), err.str(), *runs};
}

void usage_error(std::vector<std::string> args, const std::string &message)
{
    result r = call(std::move(args));
    CHECK(r.status == 2);
    CHECK(r.out.empty());
    CHECK(r.err.find(message) != std::string::npos);
    CHECK(r.err.find("Run with --help") != std::string::npos);
    CHECK(r.runs == 0);
}

} // namespace

TEST_CASE("help of the program and of a subcommand")
{
    for (const char *h : {"-h", "--help"}) {
        result r = call({h});
        CHECK(r.status == 0);
        CHECK(r.out.starts_with("Usage: prog [OPTIONS] SUBCOMMAND\n\na test program\n"));
        CHECK(r.out.find("\n  run ") != std::string::npos);
        CHECK(r.err.empty());
        r = call({"run", h});
        CHECK(r.status == 0);
        CHECK(r.out.starts_with("Usage: prog run [OPTIONS] FILE\n\nruns\n"));
        CHECK(r.out.find("--code") != std::string::npos);
        CHECK(r.err.empty());
        CHECK(r.runs == 0);
    }
}

TEST_CASE("usage errors are 2")
{
    usage_error({}, "A subcommand is required");
    usage_error({"nosuch"}, "prog: nosuch is no subcommand");
    usage_error({"run", "--nosuch", "f"}, "--nosuch");
    usage_error({"run", "f", "--code"}, "--code");
    usage_error({"run", "f", "g"}, "g");
    usage_error({"run"}, "FILE is required");
    usage_error({"run", "--code", "x", "f"}, "--code");
    usage_error({"run", "throw"}, "thrown");
}

TEST_CASE("the status of a subcommand")
{
    result r = call({"run", "f"});
    CHECK(r.status == 0);
    CHECK(r.runs == 1);
    r = call({"run", "--code", "1", "f"});
    CHECK(r.status == 1);
    r = call({"run", "--code", "77", "f"});
    CHECK(r.status == 77);
    CHECK(r.out.empty());
    CHECK(r.err.empty());
}
