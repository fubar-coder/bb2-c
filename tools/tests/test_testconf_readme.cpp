// test_testconf_readme - the examples of tests/README.md, "The run
// configuration": every ```toml block of the section is resolved as the
// file it names in its first comment (a test's ID.test.toml below a root,
// a directory's tests.toml, the environment file), so the examples stay
// valid configurations.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testconf.h"
}

namespace fs = std::filesystem;

static std::vector<std::string> blocks()
{
    std::ifstream f(TESTCONF_README);
    std::stringstream ss;
    std::vector<std::string> out;

    REQUIRE(f.good());
    ss << f.rdbuf();
    std::string text = ss.str();
    size_t sec = text.find("### The run configuration");
    REQUIRE(sec != std::string::npos);
    size_t end = text.find("\n## ", sec);
    for (size_t p = text.find("```toml\n", sec); p != std::string::npos && p < end;
         p = text.find("```toml\n", p)) {
        size_t s = p + 8, e = text.find("\n```", s);

        REQUIRE(e != std::string::npos);
        out.push_back(text.substr(s, e - s + 1));
        p = e + 4;
    }
    return out;
}

TEST_CASE("testconf_readme: the examples are valid", "[testconf]")
{
    std::vector<std::string> b = blocks();
    char err[600];

    REQUIRE(b.size() >= 4);
    for (const std::string &text : b) {
        INFO(text);
        std::string t = (fs::temp_directory_path() / "testconf-readme-XXXXXX").string();
        REQUIRE(mkdtemp(t.data()) != nullptr);
        fs::path root = t;
        tc_env env = {};

        if (text.find("top = ") != std::string::npos) {
            CHECK(tc_env_parse("env.toml", text.c_str(), &env, err, sizeof err) == 0);
            tc_env_free(&env);
            fs::remove_all(root);
            continue;
        }
        std::string first = text.substr(0, text.find('\n'));
        if (first.size() > 10 && first.compare(first.size() - 10, 10, ".test.toml") == 0) {
            std::ofstream(root / "tests.toml") << "root = true\n";
            std::ofstream(root / "x.test.toml") << text;
        } else {
            std::ofstream(root / "tests.toml") << (text.find("root = true") == std::string::npos ? "root = true\n" : "")
                                               << text;
        }
        std::ofstream(root / "x.test.bas") << "";
        std::string envtext = "top = \"" + root.string() + "\"\n";
        REQUIRE(tc_env_parse("env.toml", envtext.c_str(), &env, err, sizeof err) == 0);
        tc_config c = {};
        tc_matrix m = {};
        int r = tc_resolve((root / "x.test.bas").c_str(), &env, &c, err, sizeof err);
        if (r == 0)
            r = tc_matrix_build(&c, &m, err, sizeof err);
        if (r)
            INFO(err);
        CHECK(r == 0);
        tc_matrix_free(&m);
        tc_free(&c);
        tc_env_free(&env);
        fs::remove_all(root);
    }
}
