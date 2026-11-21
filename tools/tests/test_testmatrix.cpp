// test_testmatrix - the matrix of the run configuration (tools/lib/
// testconf, testmatrix.c) on directory trees in a temporary directory:
// axes only in the matrix table and only run settings, cells as the
// product minus exclusions, their names and the side with original
// material, axes against settings of nearer files, cell selectors - each
// rule with broken input (a key that is no run setting among them).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testconf.h"
}

namespace fs = std::filesystem;

static char err[600];

struct tree {
    fs::path root;

    tree()
    {
        std::string t = (fs::temp_directory_path() / "testmatrix-XXXXXX").string();

        REQUIRE(mkdtemp(t.data()) != nullptr);
        root = fs::canonical(t);
        write("x.test.bas", "");
    }
    ~tree() { fs::remove_all(root); }
    void write(const std::string &rel, const std::string &text) const
    {
        fs::path p = root / rel;

        fs::create_directories(p.parent_path());
        std::ofstream(p, std::ios::binary) << text;
    }
    std::string path(const std::string &rel) const { return (root / rel).string(); }
};

// the configuration and matrix of x.test.bas; rc of tc_resolve, then of
// tc_matrix_build
struct built {
    tc_env env = {};
    tc_config c = {};
    tc_matrix m = {};
    int rc = 0;

    built(const tree &t, const std::string &src = "x.test.bas")
    {
        std::string text = "top = \"" + t.root.string() + "\"\n";

        REQUIRE(tc_env_parse("env.toml", text.c_str(), &env, err, sizeof err) == 0);
        err[0] = 0;
        rc = tc_resolve(t.path(src).c_str(), &env, &c, err, sizeof err);
        if (rc == 0)
            rc = tc_matrix_build(&c, &m, err, sizeof err);
    }
    ~built()
    {
        tc_matrix_free(&m);
        tc_free(&c);
        tc_env_free(&env);
    }
    std::vector<std::string> names() const
    {
        std::vector<std::string> v;

        for (size_t i = 0; i < m.ncells; i++)
            v.push_back(m.cells[i].name);
        return v;
    }
};

static void refused(const tree &t, const std::string &what)
{
    built b(t);

    INFO(what);
    CHECK(b.rc == -1);
    CHECK(strstr(err, what.c_str()) != nullptr);
    if (!strstr(err, what.c_str()))
        fprintf(stderr, "message: %s\n", err);
}

using names = std::vector<std::string>;

TEST_CASE("testmatrix: no axes", "[testmatrix]")
{
    tree t;

    t.write("tests.toml", "root = true\nlibs = \"cd\"\n");
    built b(t);
    REQUIRE(b.rc == 0);
    CHECK(b.m.naxes == 0);
    CHECK(b.names() == names{ "default" });
    CHECK(b.m.cells[0].originals == 1);
}

TEST_CASE("testmatrix: runtime error checking as an axis", "[testmatrix]")
{
    tree t;

    t.write("tests.toml", "root = true\n[matrix]\nruntime-errors = [\"off\", \"on\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    REQUIRE(b.m.naxes == 1);
    CHECK(b.m.axes[0].def == tc_key("runtime-errors"));
    CHECK(b.m.axes[0].at.line == 3);
    CHECK(b.names() == names{ "off", "on" });
    // the default of the libraries is the C libraries: no original material
    CHECK(b.m.cells[0].originals == 0);
    CHECK(b.m.cells[1].originals == 0);
}

TEST_CASE("testmatrix: an excluded combination", "[testmatrix]")
{
    tree t;

    // the file's order of the axes does not matter: the order of the keys
    t.write("tests.toml", "root = true\nexclude = [\"compiler=orig195,libs=clib\"]\n"
                          "[matrix]\nlibs = [\"cd\", \"clib\"]\ncompiler = [\"port\", \"orig195\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    REQUIRE(b.m.naxes == 2);
    CHECK(b.m.axes[0].def == tc_key("compiler"));
    CHECK(b.names() == names{ "port,cd", "port,clib", "orig195,cd" });
    CHECK(b.m.cells[0].originals == 1);
    CHECK(b.m.cells[1].originals == 0);
    CHECK(b.m.cells[2].originals == 1);
    char buf[24];
    CHECK(!strcmp(tc_cell_value(&b.c, &b.m, 2, tc_key("compiler"), buf), "orig195"));
    CHECK(!strcmp(tc_cell_value(&b.c, &b.m, 2, tc_key("machine"), buf), "a1200"));
    CHECK(!strcmp(tc_cell_value(&b.c, &b.m, 2, tc_key("runargs"), buf), ""));
}

TEST_CASE("testmatrix: three axes and an integer axis", "[testmatrix]")
{
    tree t;

    t.write("tests.toml", "root = true\ntimeout = 30\n[matrix]\nmachine = [\"a1200\", \"a500\"]\n"
                          "compiler = [\"port\", \"orig210\"]\nstack = [\"4096\", \"20000\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    CHECK(b.names() == names{ "port,a1200,4096", "port,a1200,20000", "port,a500,4096", "port,a500,20000",
                              "orig210,a1200,4096", "orig210,a1200,20000", "orig210,a500,4096", "orig210,a500,20000" });
    CHECK(b.m.cells[0].originals == 0);
    CHECK(b.m.cells[4].originals == 1);
    char buf[24];
    CHECK(!strcmp(tc_cell_value(&b.c, &b.m, 0, tc_key("timeout"), buf), "30"));
}

TEST_CASE("testmatrix: axes against settings", "[testmatrix]")
{
    tree t;

    // a test that sets the libraries to the C libraries only: no cell with
    // the CD's libraries
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    t.write("x.test.toml", "libs = \"clib\"\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.m.naxes == 0);
        CHECK(b.names() == names{ "default" });
        CHECK(b.m.cells[0].originals == 0);
    }
    // the other way round: the test makes a setting of the directory an axis
    t.write("tests.toml", "root = true\nlibs = \"cd\"\n");
    t.write("x.test.toml", "[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.names() == names{ "cd", "clib" });
        CHECK(tc_get(&b.c, "libs") == nullptr);
    }
    // a nearer file narrows an axis (the table merges per key)
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\ncompiler = [\"port\", \"orig195\"]\n");
    t.write("x.test.toml", "[matrix]\nlibs = [\"clib\"]\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.names() == names{ "port,clib", "orig195,clib" });
    }
    // both in one file
    t.write("x.test.toml", "libs = \"cd\"\n[matrix]\nlibs = [\"clib\"]\n");
    refused(t, t.path("x.test.toml") + ":3: libs is a setting (line 1) and an axis (line 3) in one file");
}

TEST_CASE("testmatrix: keys that cannot be axes", "[testmatrix]")
{
    tree t;

    t.write("tests.toml", "root = true\n[matrix]\nasserts = [\"a\"]\n");
    refused(t, t.path("tests.toml") + ":3: asserts is no run setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\nfixed = [\"1\"]\n");
    refused(t, ":3: fixed is no run setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\nlabels = [\"a\", \"b\"]\n");
    refused(t, ":3: labels is no run setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\nexclude = [\"libs=cd\"]\n");
    refused(t, ":3: exclude is no run setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\ntimout = [\"1\"]\n");
    refused(t, ":3: timout is no run setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\nrwe = [\"a\", \"b\"]\n");
    refused(t, ":3: rwe is a list setting and cannot be an axis");
    // a list value outside the matrix table is no axis: the list setting
    // takes it as its value, a scalar setting refuses it
    t.write("tests.toml", "root = true\nlibs = [\"cd\", \"clib\"]\n");
    refused(t, ":2: libs must be a string, not a list of strings");
    // the values of an axis
    t.write("tests.toml", "root = true\n[matrix]\nlibs = \"cd\"\n");
    refused(t, ":3: the axis libs must be a list of strings, not a string");
    t.write("tests.toml", "root = true\n[matrix]\nlibs = []\n");
    refused(t, ":3: the axis libs has no value");
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"cd\"]\n");
    refused(t, ":3: the axis libs has the value \"cd\" twice");
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clibs\"]\n");
    refused(t, ":3: libs must be one of cd|clib, not \"clibs\"");
    t.write("tests.toml", "root = true\n[matrix]\ntimeout = [\"60\", \"1m\"]\n");
    refused(t, ":3: timeout must be an integer, not \"1m\"");
    t.write("tests.toml", "root = true\n[matrix]\ntimeout = [\"060\"]\n");
    refused(t, ":3: timeout must be an integer, not \"060\"");
    t.write("tests.toml", "root = true\n[matrix]\ntimeout = [\"0\"]\n");
    refused(t, ":3: timeout must be at least 1, not 0");
    t.write("tests.toml", "root = true\n[matrix]\nmachine = [\"\"]\n");
    refused(t, ":3: an empty value of machine");
}

TEST_CASE("testmatrix: exclusions", "[testmatrix]")
{
    tree t;

    t.write("tests.toml", "root = true\nexclude = [\"libs=cd\", \"libs=clib\"]\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    refused(t, t.path("tests.toml") + ":2: exclude leaves x no cell");
    t.write("tests.toml", "root = true\nexclude = [\"libz=cd\"]\n");
    refused(t, ":2: exclude: the cell selector \"libz=cd\": libz is no run setting");
    t.write("tests.toml", "root = true\nexclude = [\"case=a\"]\n");
    refused(t, ":2: exclude: the cell selector \"case=a\": case= is not allowed here");
    t.write("tests.toml", "root = true\nexclude = [\"libs = cd\"]\n");
    refused(t, ":2: exclude: a blank in the cell selector");
    t.write("tests.toml", "root = true\nexclude = [\"libs\"]\n");
    refused(t, ":2: exclude: the cell selector \"libs\": KEY=VALUE expected");
    t.write("tests.toml", "root = true\nexclude = [\"libs=x\"]\n");
    refused(t, ":2: exclude: the cell selector \"libs=x\": libs must be one of cd|clib");
    // an exclusion by a setting that is no axis matches every cell
    t.write("tests.toml", "root = true\nmachine = \"a500\"\nexclude = [\"machine=a500,libs=cd\"]\n"
                          "[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    CHECK(b.names() == names{ "clib" });
}

TEST_CASE("testmatrix: cell selectors", "[testmatrix]")
{
    tc_selector s;
    char msg[300];

    REQUIRE(tc_selector_parse("libs=cd,compiler=orig195,case=frames_max", 1, &s, msg, sizeof msg) == 0);
    REQUIRE(s.n == 3);
    CHECK(s.t[0].def == tc_key("libs"));
    CHECK(!strcmp(s.t[0].value, "cd"));
    CHECK(s.t[2].def == nullptr);
    CHECK(!strcmp(s.t[2].value, "frames_max"));
    tc_selector_free(&s);
    CHECK(tc_selector_parse("", 1, &s, msg, sizeof msg) == -1);
    CHECK(!strcmp(msg, "an empty cell selector"));
    CHECK(tc_selector_parse("libs=cd,", 1, &s, msg, sizeof msg) == -1);
    CHECK(tc_selector_parse("libs=cd,libs=clib", 1, &s, msg, sizeof msg) == -1);
    CHECK(!strcmp(msg, "the cell selector \"libs=cd,libs=clib\" names libs twice"));
    CHECK(tc_selector_parse("case=a,case=b", 1, &s, msg, sizeof msg) == -1);
    CHECK(tc_selector_parse("case=Bad", 1, &s, msg, sizeof msg) == -1);
    CHECK(!strcmp(msg, "the cell selector \"case=Bad\": \"Bad\" is no case name"));
    CHECK(tc_selector_parse("=cd", 1, &s, msg, sizeof msg) == -1);
    CHECK(tc_selector_parse("files=x", 1, &s, msg, sizeof msg) == -1);
    CHECK(!strcmp(msg, "the cell selector \"files=x\": files is a list setting"));
    CHECK(tc_selector_parse("libs=", 1, &s, msg, sizeof msg) == -1);
    CHECK(!strcmp(msg, "the cell selector \"libs=\": an empty value of libs"));

    // matching cells
    tree t;
    t.write("tests.toml", "root = true\nmachine = \"a500\"\n[matrix]\nlibs = [\"cd\", \"clib\"]\n"
                          "compiler = [\"port\", \"orig195\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    REQUIRE(b.names() == names{ "port,cd", "port,clib", "orig195,cd", "orig195,clib" });
    auto match = [&](const char *sel, const char *cs) {
        std::string r;
        tc_selector x;

        REQUIRE(tc_selector_parse(sel, 1, &x, msg, sizeof msg) == 0);
        for (size_t i = 0; i < b.m.ncells; i++)
            r += tc_selector_matches(&x, &b.c, &b.m, i, cs) ? '1' : '0';
        tc_selector_free(&x);
        return r;
    };
    CHECK(match("libs=cd", "a") == "1010");
    CHECK(match("libs=cd,compiler=orig195", "a") == "0010");
    CHECK(match("machine=a500", "a") == "1111");
    CHECK(match("machine=a1200", "a") == "0000");
    CHECK(match("runtime-errors=off", "a") == "1111");    // the default
    CHECK(match("case=a", "a") == "1111");
    CHECK(match("case=a", "b") == "0000");
    CHECK(match("case=a,libs=clib", "a") == "0101");
    CHECK(match("case=a", nullptr) == "0000");
}
