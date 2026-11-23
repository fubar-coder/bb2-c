// test_testmatrix - the matrix of the run configuration (tools/lib/
// testconf, testmatrix.c) on directory trees in a temporary directory:
// axes only in the matrix table and only run settings, cells as the
// product minus exclusions, their names and the side with original
// material, axes against settings of nearer files, cell selectors, the
// cells that run from the editor - each rule with broken input (a key that
// is no run setting among them) -, and the session key of a cell, in one
// test and across tests.
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
    t.write("tests.toml", "root = true\n[matrix]\nfiles = [\"a|b\"]\n");
    refused(t, ":3: files is a list setting and cannot be an axis");
    t.write("tests.toml", "root = true\n[matrix]\ncost = [\"1\"]\n");
    refused(t, ":3: cost is no run setting and cannot be an axis");
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

TEST_CASE("testmatrix: a run from the editor", "[testmatrix]")
{
    tree t;

    // valid: editor-run with a debugger module, wrap, jobline; the shell
    // settings and the assertions on rc in the cells that run from the shell
    t.write("tests.toml", "root = true\neditor-run = \"on\"\ndefaultdbug = \"/m\"\nwrap = \"/w|x\"\n"
                          "jobline = [\"a=b\"]\n[asserts]\noutput = \"== \\\"start\\n\\\"\"\n");
    {
        built b(t);
        CHECK(b.rc == 0);
    }
    t.write("tests.toml", "root = true\nrunargs = \"a\"\n[matrix]\neditor-run = [\"off\", \"on\"]\n"
                          "[asserts.\"editor-run=off\"]\nrc = \"== 5\"\nrequester = \"count == 1\"\n");
    t.write("x.test.toml", "exclude = [\"editor-run=on\"]\n");
    {
        built b(t);
        CHECK(b.rc == 0);
        CHECK(b.names() == names{ "off" });
    }
    fs::remove(t.root / "x.test.toml");
    // the settings of the shell start, naming file, line and both keys
    t.write("tests.toml", "root = true\neditor-run = \"on\"\n\nrunwrap = \"/r|guard\"\n");
    refused(t, t.path("tests.toml") + ":4: runwrap with editor-run = \"on\" (" + t.path("tests.toml") +
                   ":2, cell default): the program runs from the editor, not from the shell");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\nrunargs = \"a\"\n");
    refused(t, ":3: runargs with editor-run = \"on\"");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\nlauncherargs = \"-harden\"\n");
    refused(t, ":3: launcherargs with editor-run = \"on\"");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\npad = \"on\"\n");
    refused(t, ":3: pad = \"on\" with editor-run = \"on\"");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\npad = \"off\"\n");
    {
        built b(t);
        CHECK(b.rc == 0);
    }
    // an axis: the cell that runs from the editor is refused
    t.write("tests.toml", "root = true\nrunargs = \"a\"\n[matrix]\neditor-run = [\"off\", \"on\"]\n");
    refused(t, ":2: runargs with editor-run = \"on\" (" + t.path("tests.toml") + ":4, cell on)");
    // an assertion, a deviation, an alternative end on rc or requester
    t.write("tests.toml", "root = true\neditor-run = \"on\"\n[asserts]\n\nrc = \"== 0\"\n");
    refused(t, t.path("tests.toml") + ":5: rc: an assertion on rc in the cell default with editor-run = \"on\" (" +
                   t.path("tests.toml") + ":2): a run from the editor gives no exit code and no requesters");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\n[asserts.\"case=a\"]\nrequester = \"count == 0\"\n");
    refused(t, ":4: requester: an assertion on requester in the cell default");
    t.write("tests.toml", "root = true\n[matrix]\neditor-run = [\"off\", \"on\"]\n[asserts.\"runtime-errors=off\"]\n"
                          "rc = \"== 0\"\n");
    refused(t, ":5: rc: an assertion on rc in the cell on");
    t.write("tests.toml", "root = true\neditor-run = \"on\"\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    t.write("x.test.toml", "fixed-orig = 1\n[deviations.\"libs=cd\"]\nrc = \"== 3\"\n");
    refused(t, t.path("x.test.toml") + ":3: rc: a deviation on rc in the cell cd");
    t.write("x.test.toml", "fixed-orig = 1\n[ends.\"libs=cd\"]\nreason = \"r\"\nhangs = [\"session == timeout\", "
                           "\"rc not exists\"]\n");
    refused(t, t.path("x.test.toml") + ":4: hangs: an alternative end on rc in the cell cd");
    fs::remove(t.root / "x.test.toml");
    // the debugger module without a run from the editor
    t.write("tests.toml", "root = true\n\ndefaultdbug = \"/m\"\n");
    refused(t, t.path("tests.toml") + ":3: defaultdbug without editor-run = \"on\" (cell default): the debugger "
                                      "module is used only by a run from the editor");
    t.write("tests.toml", "root = true\ndefaultdbug = \"/m\"\n[matrix]\neditor-run = [\"on\", \"off\"]\n");
    refused(t, ":2: defaultdbug without editor-run = \"on\" (cell off)");
    // the job ID r001 is the run from the editor: free in a cell from the shell
    t.write("tests.toml", "root = true\njobline = [\"run=r001|CORPUS:a.bb2|RAM:con\"]\n");
    {
        built b(t);
        CHECK(b.rc == 0);
    }
    t.write("tests.toml", "root = true\n\njobline = [\"run=r001|CORPUS:a.bb2|RAM:con\"]\n[matrix]\n"
                          "editor-run = [\"off\", \"on\"]\n");
    refused(t, t.path("tests.toml") + ":3: jobline: the job ID r001 is the run from the editor in the cell on with "
                                      "editor-run = \"on\" (" + t.path("tests.toml") + ":5)");
}

// the session key of a cell of the test src
static std::string session_key(const tree &t, const std::string &src, size_t cell = 0)
{
    built b(t, src);
    std::string k;

    REQUIRE(b.rc == 0);
    REQUIRE(cell < b.m.ncells);
    char *s = tc_session_key(&b.c, &b.m, cell);
    REQUIRE(s != nullptr);
    k = s;
    free(s);
    return k;
}

// two tests a/x and b/y with the settings ta and tb in their own files
static void two(const tree &t, const std::string &ta, const std::string &tb)
{
    t.write("tests.toml", "root = true\n");
    t.write("a/x.test.bas", "");
    t.write("b/y.test.bas", "");
    t.write("a/x.test.toml", ta);
    t.write("b/y.test.toml", tb);
}

static bool same_session(const tree &t)
{
    return session_key(t, "a/x.test.bas") == session_key(t, "b/y.test.bas");
}

TEST_CASE("testmatrix: the session key across tests", "[testmatrix]")
{
    tree t;

    // the same session settings: the same key, whatever the cell's volume
    // and script hold
    two(t, "", "");
    CHECK(same_session(t));
    two(t,
        "machine = \"a600\"\nrtg = \"on\"\ntimeout = 30\nhang-after = 10\nuae = [\"cpu_speed=max\"]\n"
        "floppy = [\"0|/img/a.adf\"]\nmount = [\"BB2|/cd\"]\n",
        "machine = \"a600\"\nrtg = \"on\"\ntimeout = 30\nhang-after = 10\nuae = [\"cpu_speed=max\"]\n"
        "floppy = [\"0|/img/a.adf\"]\nmount = [\"BB2|/cd\"]\ncompiler = \"orig195\"\nlibs = \"cd\"\n"
        "runtime-errors = \"on\"\nstack = 9000\nfiles = [\"data/f|/abs/f\"]\ndirs = [\"d\"]\nremove = [\"d\"]\n"
        "wrap = \"/w\"\nrunwrap = \"/r\"\nrunargs = \"x\"\nlauncherargs = \"y\"\nrwe = [\"r\"]\n"
        "jobline = [\"max=3\"]\npad = \"off\"\nlabels = [\"l\"]\ncost = 5\n");
    CHECK(same_session(t));
    // each setting of the session on its own: another key
    for (const char *s : { "machine = \"a600\"\n", "rtg = \"on\"\n", "timeout = 30\n", "hang-after = 10\n",
                           "uae = [\"cpu_speed=max\"]\n", "floppy = [\"0|/img/a.adf\"]\n",
                           "mount = [\"BB2|/cd\"]\n" }) {
        INFO(s);
        two(t, "", s);
        CHECK(!same_session(t));
    }
    // another element of a list
    two(t, "uae = [\"cpu_speed=max\"]\n", "uae = [\"cpu_speed=real\"]\n");
    CHECK(!same_session(t));
    two(t, "mount = [\"BB2|/cd\"]\n", "mount = [\"BB2|/cd2\"]\n");
    CHECK(!same_session(t));
    two(t, "floppy = [\"0|/img/a.adf\"]\n", "floppy = [\"1|/img/a.adf\"]\n");
    CHECK(!same_session(t));
    // a relative path is relative to its file's directory: the same text,
    // two paths
    two(t, "floppy = [\"0|a.adf\"]\n", "floppy = [\"0|a.adf\"]\n");
    CHECK(!same_session(t));
    // the lists in their order: the order is the one the emulator gets
    two(t, "uae = [\"a=1\", \"b=2\"]\n", "uae = [\"a=1\", \"b=2\"]\n");
    CHECK(same_session(t));
    two(t, "uae = [\"a=1\", \"b=2\"]\n", "uae = [\"b=2\", \"a=1\"]\n");
    CHECK(!same_session(t));
    two(t, "mount = [\"A|/a\", \"B|/b\"]\n", "mount = [\"B|/b\", \"A|/a\"]\n");
    CHECK(!same_session(t));
    // a value cannot end its line early and pass for two elements
    two(t, "uae = [\"a=1\", \"b=2\"]\n", "uae = [\"a=1\\nuae=3:b=2\"]\n");
    CHECK(!same_session(t));
    // a time limit left to the environment is not one set to a value
    two(t, "", "timeout = 120\n");
    CHECK(!same_session(t));
    // serial and isolate: a key of its own, also against the same cell of
    // another test with the same settings
    for (const char *s : { "serial = \"on\"\n", "isolate = \"on\"\n" }) {
        INFO(s);
        two(t, s, s);
        CHECK(!same_session(t));
        CHECK(session_key(t, "a/x.test.bas") == session_key(t, "a/x.test.bas"));
        two(t, "", s);
        CHECK(!same_session(t));
    }
}

TEST_CASE("testmatrix: the session key in one test", "[testmatrix]")
{
    tree t;

    // the lists are the same in every cell: the axes of the session decide
    t.write("tests.toml", "root = true\nuae = [\"a=1\"]\nmount = [\"M|/m\"]\nfloppy = [\"1|/f.adf\"]\n[matrix]\n"
                          "libs = [\"cd\", \"clib\"]\nmachine = [\"a1200\", \"a600\"]\n");
    {
        built b(t);
        std::vector<std::string> k, mach;
        char buf[24];

        REQUIRE(b.rc == 0);
        REQUIRE(b.m.ncells == 4);
        for (size_t i = 0; i < 4; i++) {
            char *s = tc_session_key(&b.c, &b.m, i);

            REQUIRE(s != nullptr);
            k.push_back(s);
            free(s);
            mach.push_back(tc_cell_value(&b.c, &b.m, i, tc_key("machine"), buf));
        }
        for (size_t i = 0; i < 4; i++)
            for (size_t j = 0; j < i; j++) {
                INFO(b.m.cells[i].name, " ", b.m.cells[j].name);
                CHECK((k[i] == k[j]) == (mach[i] == mach[j]));
            }
    }
    // serial: every cell alone
    t.write("tests.toml", "root = true\nserial = \"on\"\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    CHECK(session_key(t, "x.test.bas", 0) != session_key(t, "x.test.bas", 1));
}
