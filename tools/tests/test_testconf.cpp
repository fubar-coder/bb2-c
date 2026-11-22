// test_testconf - the hierarchy of the run configuration (tools/lib/
// testconf) on directory trees written to a temporary directory of the
// test (removed at the end): the search up to root = true, the nearest
// file per key, tables merged per key, lists replaced, the keys of a single
// test and of directory files, unknown keys, types, relative paths and the
// placeholders of the environment file - each rule with broken input.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "testconf.h"
}

namespace fs = std::filesystem;

static char err[600];

// a directory tree of its own, removed by the destructor
struct tree {
    fs::path root;

    tree()
    {
        std::string t = (fs::temp_directory_path() / "testconf-XXXXXX").string();

        REQUIRE(mkdtemp(t.data()) != nullptr);
        root = fs::canonical(t);
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

// an environment whose top is the tree (so the search never leaves it)
static tc_env env_of(const tree &t, const std::string &placeholders = "")
{
    tc_env e = {};
    std::string text = "top = \"" + t.root.string() + "\"\n[placeholders]\n" + placeholders;

    REQUIRE(tc_env_parse("env.toml", text.c_str(), &e, err, sizeof err) == 0);
    return e;
}

static int resolve(const tree &t, const std::string &src, tc_config *c, const tc_env *e)
{
    *c = {};
    err[0] = 0;
    return tc_resolve(t.path(src).c_str(), e, c, err, sizeof err);
}

// resolving src must fail with a message holding every part of what
static void refused(const tree &t, const std::string &src, std::initializer_list<std::string> what)
{
    tc_env e = env_of(t);
    tc_config c;

    CHECK(resolve(t, src, &c, &e) == -1);
    for (const std::string &w : what) {
        INFO(w);
        CHECK(strstr(err, w.c_str()) != nullptr);
    }
    if (err[0])
        INFO(err);
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: the keys", "[testconf]")
{
    size_t n;
    const tc_keydef *k = tc_keys(&n);

    REQUIRE(n > 20);
    CHECK(!strcmp(k[0].name, "compiler"));
    CHECK(tc_key("timeout")->vkind == TC_V_INT);
    CHECK(tc_key("timout") == nullptr);
    CHECK(tc_key("root")->where == TC_IN_DIR);
    CHECK(tc_key("fixed-orig")->where == TC_IN_TEST);
    CHECK(tc_enum_ok(tc_key("libs"), "clib"));
    CHECK(!tc_enum_ok(tc_key("libs"), "cli"));
    CHECK(!tc_enum_ok(tc_key("libs"), "clibs"));
}

TEST_CASE("testconf: the hierarchy", "[testconf]")
{
    tree t;
    tc_env e = env_of(t);
    tc_config c;

    t.write("tests.toml", "root = true\nrwe = [\"a\", \"b\"]\ntimeout = 60\nlabels = [\"x\"]\n"
                          "[asserts]\nrc = \"== 0\"\nrequester = \"count == 0\"\n");
    t.write("libs/tests.toml", "# narrower\nrwe = [\"c\"]\n[asserts]\noutput = \"contains \\\"ok\\\"\"\n");
    t.write("libs/sub/x.test.toml", "fixed-orig = 12\n[asserts]\nrc = \"== 20\"\n"
                                    "[deviations.\"libs=cd\"]\n\"check.a.b\" = \"== fail\"\n");
    t.write("libs/sub/x.test.bas", "");

    REQUIRE(resolve(t, "libs/sub/x.test.bas", &c, &e) == 0);
    CHECK(!strcmp(c.id, "x"));
    CHECK(c.dir == t.path("libs/sub"));
    CHECK(c.has_own == 1);
    REQUIRE(c.nfiles == 3);
    CHECK(c.files[0] == t.path("libs/sub/x.test.toml"));
    CHECK(c.files[1] == t.path("libs/tests.toml"));
    CHECK(c.files[2] == t.path("tests.toml"));

    // a list replaces: the subdirectory's one value
    const tc_setting *s = tc_get(&c, "rwe");
    REQUIRE(s != nullptr);
    REQUIRE(s->value.nlist == 1);
    CHECK(!strcmp(s->value.list[0], "c"));
    CHECK(s->at.file == t.path("libs/tests.toml"));
    CHECK(s->at.line == 2);
    CHECK(s->at.depth == 1);
    // a key only the root sets, with its origin
    s = tc_get(&c, "timeout");
    REQUIRE(s != nullptr);
    CHECK(s->value.integer == 60);
    CHECK(s->at.file == t.path("tests.toml"));
    CHECK(s->at.line == 3);
    CHECK(s->at.depth == 2);
    s = tc_get(&c, "fixed-orig");
    REQUIRE(s != nullptr);
    CHECK(s->at.depth == 0);
    CHECK(tc_get(&c, "machine") == nullptr);

    // tables merge per key: rc from the test, requester from the root,
    // output from the directory
    REQUIRE(c.nasserts == 3);
    CHECK(!strcmp(c.asserts[0].key, "rc"));
    CHECK(!strcmp(c.asserts[0].value.str, "== 20"));
    CHECK(c.asserts[0].at.depth == 0);
    CHECK(!strcmp(c.asserts[1].key, "output"));
    CHECK(c.asserts[1].at.depth == 1);
    CHECK(!strcmp(c.asserts[2].key, "requester"));
    CHECK(c.asserts[2].at.depth == 2);
    CHECK(c.asserts[2].sel == nullptr);
    tc_free(&c);

    // a test without its own file
    t.write("libs/y.test.bas", "");
    REQUIRE(resolve(t, "libs/y.test.bas", &c, &e) == 0);
    CHECK(c.has_own == 0);
    CHECK(c.nfiles == 2);
    tc_free(&c);

    // a directory without tests.toml between two that have one
    t.write("libs/a/b/z.test.bas", "");
    REQUIRE(resolve(t, "libs/a/b/z.test.bas", &c, &e) == 0);
    CHECK(c.nfiles == 2);
    CHECK(tc_get(&c, "rwe")->at.depth == 1);
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: the search stops at root = true", "[testconf]")
{
    tree t;
    tc_env e = env_of(t);
    tc_config c;

    // a file above the root is not read, not even a broken one
    t.write("tests.toml", "this is no TOML\n");
    t.write("a/tests.toml", "root = true\ntimeout = 5\n");
    t.write("a/b/tests.toml", "root = false\ntimeout = 6\n");
    t.write("a/b/x.test.bas", "");
    REQUIRE(resolve(t, "a/b/x.test.bas", &c, &e) == 0);
    CHECK(c.nfiles == 2);
    CHECK(tc_get(&c, "timeout")->value.integer == 6);
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: no root", "[testconf]")
{
    tree t;

    t.write("a/tests.toml", "timeout = 5\n");
    t.write("a/x.test.bas", "");
    refused(t, "a/x.test.bas", { "a/x.test.bas: no tests.toml with root = true", t.root.string() });
    // root = false is no root
    t.write("tests.toml", "root = false\n");
    refused(t, "a/x.test.bas", { "no tests.toml with root = true" });
}

TEST_CASE("testconf: keys of a single test and of directories", "[testconf]")
{
    tree t;

    t.write("x.test.bas", "");
    t.write("tests.toml", "root = true\nfixed-orig = 17\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":2: fixed-orig is a key of a single test" });
    t.write("tests.toml", "root = true\n\nfixed = 17\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":3: fixed is a key of a single test" });
    t.write("tests.toml", "root = true\n[deviations.\"libs=cd\"]\n\"check.a.b\" = \"== fail\"\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":2: deviations is a key of a single test" });
    t.write("tests.toml", "root = true\n");
    t.write("x.test.toml", "root = true\n");
    refused(t, "x.test.bas", { t.path("x.test.toml") + ":1: root is valid only in a directory's tests.toml" });
    t.write("x.test.toml", "[deviations]\nrc = \"== 5\"\n");
    refused(t, "x.test.bas", { t.path("x.test.toml") + ":2: a deviation needs the cell selector" });
    t.write("x.test.toml", "fixed = 3\nfixed-orig = 4\n");
    refused(t, "x.test.bas", { "test x: fixed-orig and fixed exclude each other" });

    // the same keys where they belong
    tc_env e = env_of(t);
    tc_config c;
    t.write("x.test.toml", "fixed-orig = 4\n[deviations.\"libs=cd\"]\n\"check.a.b\" = \"== fail\"\n");
    REQUIRE(resolve(t, "x.test.bas", &c, &e) == 0);
    REQUIRE(c.ndeviations == 1);
    CHECK(!strcmp(c.deviations[0].sel, "libs=cd"));
    CHECK(!strcmp(c.deviations[0].key, "check.a.b"));
    CHECK(c.deviations[0].at.line == 3);
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: unknown keys and wrong values", "[testconf]")
{
    tree t;

    t.write("x.test.bas", "");
    t.write("tests.toml", "root = true\n\ntimout = 60\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":3: unknown key timout" });
    t.write("tests.toml", "root = true\n[runs]\na = 1\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":2: unknown table [runs]" });
    t.write("tests.toml", "root = true\n[asserts.\"libs=cd\".x]\n");
    refused(t, "x.test.bas", { ":2: the table [asserts] has no sub tables" });
    t.write("tests.toml", "root = true\n[matrix.\"libs=cd\"]\n");
    refused(t, "x.test.bas", { ":2: the table [matrix] has no sub tables" });
    t.write("tests.toml", "root = true\ntimeout = \"60\"\n");
    refused(t, "x.test.bas", { ":2: timeout must be an integer, not a string" });
    t.write("tests.toml", "root = true\ntimeout = 0\n");
    refused(t, "x.test.bas", { ":2: timeout must be at least 1, not 0" });
    t.write("tests.toml", "root = true\nlibs = \"clibs\"\n");
    refused(t, "x.test.bas", { ":2: libs must be one of cd|clib, not \"clibs\"" });
    t.write("tests.toml", "root = true\ncompiler = \"orig\"\n");
    refused(t, "x.test.bas", { ":2: compiler must be one of port|orig195|orig210, not \"orig\"" });
    t.write("tests.toml", "root = true\nrwe = \"a\"\n");
    refused(t, "x.test.bas", { ":2: rwe must be a list of strings, not a string" });
    t.write("tests.toml", "root = 1\n");
    refused(t, "x.test.bas", { ":1: root must be a boolean, not an integer" });
    t.write("tests.toml", "root = true\n[asserts]\nrc = 0\n");
    refused(t, "x.test.bas", { ":3: the assertion rc must be a string" });
    // the subset's own refusals carry the file and line too
    t.write("tests.toml", "root = true\ntimeout = 1.5\n");
    refused(t, "x.test.bas", { t.path("tests.toml") + ":2: " });
    // a source that is no test of the model
    t.write("tests.toml", "root = true\n");
    refused(t, "x.bas", { "x.bas: a test source of the model is named ID.test.bas" });
    refused(t, ".test.bas", { "is named ID.test.bas" });
}

TEST_CASE("testconf: relative paths", "[testconf]")
{
    tree t;
    tc_env e = env_of(t, "TESTDATA = \"/build/testdata\"\n");
    tc_config c;

    t.write("tests.toml", "root = true\nwrap = \"tools/w|-x\"\nrunargs = \"data/a\"\n");
    t.write("libs/tests.toml", "files = [\"x.iff|data/x.iff\", \"y|../up/y|TAG\", \"z|/abs/z\", "
                               "\"w|BB2:w\", \"t|@TESTDATA@/t\"]\nmount = [\"BB2|cd\"]\n");
    t.write("libs/a/b/x.test.bas", "");
    REQUIRE(resolve(t, "libs/a/b/x.test.bas", &c, &e) == 0);
    const tc_setting *s = tc_get(&c, "files");
    REQUIRE(s != nullptr);
    REQUIRE(s->value.nlist == 5);
    CHECK(s->value.list[0] == "x.iff|" + t.path("libs/data/x.iff"));
    CHECK(s->value.list[1] == "y|" + t.path("up/y") + "|TAG");
    CHECK(!strcmp(s->value.list[2], "z|/abs/z"));
    CHECK(!strcmp(s->value.list[3], "w|BB2:w"));
    CHECK(!strcmp(s->value.list[4], "t|/build/testdata/t"));
    CHECK(tc_get(&c, "mount")->value.list[0] == "BB2|" + t.path("libs/cd"));
    // the path field of wrap is the first; runargs has none
    CHECK(tc_get(&c, "wrap")->value.str == t.path("tools/w") + "|-x");
    CHECK(!strcmp(tc_get(&c, "runargs")->value.str, "data/a"));
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: placeholders", "[testconf]")
{
    tree t;
    tc_env e = env_of(t, "TESTDATA = \"/td\"\nUSER_LIB2 = \"/u\"\n");
    char *out;
    char ph[64];

    CHECK(tc_placeholders(&e, "a@TESTDATA@b@USER_LIB2@", &out, ph, sizeof ph) == 0);
    CHECK(!strcmp(out, "a/tdb/u"));
    free(out);
    // not placeholders: lower case, an @ alone, an empty name
    CHECK(tc_placeholders(&e, "a@b.c@x @@ @ 1@", &out, ph, sizeof ph) == 0);
    CHECK(!strcmp(out, "a@b.c@x @@ @ 1@"));
    free(out);
    CHECK(tc_placeholders(&e, "x@FOO@y", &out, ph, sizeof ph) == -1);
    CHECK(!strcmp(ph, "@FOO@"));
    CHECK(tc_placeholders(nullptr, "@TESTDATA@", &out, ph, sizeof ph) == -1);

    // an unknown placeholder in a file names file, line and placeholder
    tc_config c;
    t.write("tests.toml", "root = true\n\nruns = 1\n");
    t.write("x.test.bas", "");
    t.write("tests.toml", "root = true\n\nfiles = [\"a|b\", \"c|@FOO@/c\"]\n");
    CHECK(resolve(t, "x.test.bas", &c, &e) == -1);
    CHECK(std::string(err) == t.path("tests.toml") + ":3: unknown placeholder @FOO@");
    tc_free(&c);
    t.write("tests.toml", "root = true\n[matrix]\nmachine = [\"@NOPE@\"]\n");
    CHECK(resolve(t, "x.test.bas", &c, &e) == -1);
    CHECK(std::string(err) == t.path("tests.toml") + ":3: unknown placeholder @NOPE@");
    tc_free(&c);
    tc_env_free(&e);
}

TEST_CASE("testconf: the environment file", "[testconf]")
{
    tc_env e = {};

    CHECK(tc_env_parse("env.toml", "top = \"/r\"\n[placeholders]\nCD = \"/cd\"\n", &e, err, sizeof err) == 0);
    CHECK(!strcmp(e.top, "/r"));
    REQUIRE(e.nph == 1);
    CHECK(!strcmp(e.ph[0].name, "CD"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "tops = \"/r\"\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:1: unknown key tops in the environment file"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "[placeholders]\ncd = \"/cd\"\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:2: cd is no placeholder name ([A-Z][A-Z0-9_]*)"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "[placeholders]\nCD = 1\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:2: the placeholder CD must be a string"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "[other]\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:1: unknown table in the environment file"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "top = 1\n", &e, err, sizeof err) == -1);
    tc_env_free(&e);
    // the table of the runner: strings under any key, not placeholders
    CHECK(tc_env_parse("env.toml", "[runner]\nblitz2 = \"/b/blitz2\"\nclib-acidlibs = \"/b/a\"\n", &e, err,
                       sizeof err)
          == 0);
    CHECK(e.nph == 0);
    REQUIRE(e.nrun == 2);
    CHECK(!strcmp(tc_env_runner(&e, "blitz2"), "/b/blitz2"));
    CHECK(!strcmp(tc_env_runner(&e, "clib-acidlibs"), "/b/a"));
    CHECK(tc_env_runner(&e, "ted") == nullptr);
    CHECK(tc_env_runner(nullptr, "ted") == nullptr);
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "[runner]\ntimeout = 90\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:2: the runner's timeout must be a string"));
    tc_env_free(&e);
    CHECK(tc_env_parse("env.toml", "[runner.x]\n", &e, err, sizeof err) == -1);
    CHECK(!strcmp(err, "env.toml:1: unknown table in the environment file"));
    tc_env_free(&e);
    CHECK(tc_env_load("/nonexistent/env.toml", &e, err, sizeof err) == -1);
    CHECK(strstr(err, "/nonexistent/env.toml: cannot read") != nullptr);
    tc_env_free(&e);
}
