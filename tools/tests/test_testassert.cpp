// test_testassert - the assertions of the run configuration (tools/lib/
// testconf, testassert.c): every query and predicate of the grammar of
// tests/README.md "Assertions" with broken forms, their check when a
// configuration is resolved (file and line), and the replacement per query:
// the nearest file, the most specific selector, the deviations of the
// cells with the faulty part, the defaults.
#include <climits>
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

static char msg[400];
static char err[600];

static int query(const char *text, int *kind = nullptr, std::string *ns = nullptr, std::string *rest = nullptr)
{
    tc_query q;
    int r = tc_query_parse(text, &q, msg, sizeof msg);

    if (r == 0) {
        if (kind)
            *kind = q.kind;
        if (ns)
            *ns = q.ns ? q.ns : "";
        if (rest)
            *rest = q.rest ? q.rest : "";
    }
    tc_query_free(&q);
    return r;
}

struct pr {
    int rc, op, cmp, vtype;
    long long num;
    std::string str;
};

static pr pred(const char *text, int qkind)
{
    tc_pred p;
    pr r = {};

    r.rc = tc_pred_parse(text, qkind, &p, msg, sizeof msg);
    if (r.rc == 0) {
        r.op = p.op;
        r.cmp = p.cmp;
        r.vtype = p.vtype;
        r.num = p.num;
        r.str = p.str ? p.str : "";
    }
    tc_pred_free(&p);
    return r;
}

static void pred_refused(const char *text, int qkind, const char *what)
{
    INFO(text);
    CHECK(pred(text, qkind).rc == -1);
    CHECK(strstr(msg, what) != nullptr);
    if (!strstr(msg, what))
        fprintf(stderr, "message: %s\n", msg);
}

TEST_CASE("testassert: queries", "[testassert]")
{
    int k;
    std::string ns, rest;

    const struct {
        const char *t;
        int k;
    } all[] = { { "rc", TC_Q_RC }, { "requester", TC_Q_REQUESTER }, { "compile", TC_Q_COMPILE },
                { "compile.log", TC_Q_COMPILE_LOG }, { "output", TC_Q_OUTPUT }, { "serial", TC_Q_SERIAL },
                { "inject", TC_Q_INJECT }, { "session", TC_Q_SESSION }, { "end.case", TC_Q_END_CASE },
                { "end.run", TC_Q_END_RUN }, { "compile.inject", TC_Q_COMPILE_INJECT } };
    for (const auto &q : all) {
        INFO(q.t);
        CHECK(query(q.t, &k) == 0);
        CHECK(k == q.k);
    }
    CHECK(query("check.frames_maximum.width", &k, &ns, &rest) == 0);
    CHECK(k == TC_Q_CHECK);
    CHECK(ns == "frames_maximum");
    CHECK(rest == "width");
    CHECK(query("check.Helper.set up.actual", &k, &ns, &rest) == 0);
    CHECK(ns == "Helper");
    CHECK(rest == "set up.actual");
    CHECK(query("check.init.x.expected", &k, &ns, &rest) == 0);
    CHECK(ns == "init");

    CHECK(query("RC") == -1);
    CHECK(!strcmp(msg, "unknown query \"RC\""));
    CHECK(query("exitcode") == -1);
    CHECK(query("end") == -1);
    CHECK(query("check") == -1);
    CHECK(query("check.a") == -1);
    CHECK(strstr(msg, "check.NAMESPACE.NAME expected") != nullptr);
    CHECK(query("check.a.") == -1);
    CHECK(query("check..x") == -1);
    CHECK(query("check.1a.x") == -1);
}

TEST_CASE("testassert: predicates", "[testassert]")
{
    pr p;

    // the comparisons, with integers in decimal and $hex (32 bits, signed)
    p = pred("== 0", TC_Q_RC);
    CHECK((p.rc == 0 && p.op == TC_P_EQ && p.vtype == TC_PV_INT && p.num == 0));
    p = pred("  !=   -5 ", TC_Q_RC);
    CHECK((p.rc == 0 && p.op == TC_P_NE && p.num == -5));
    CHECK(pred("< 20", TC_Q_RC).op == TC_P_LT);
    CHECK(pred("<= 20", TC_Q_RC).op == TC_P_LE);
    CHECK(pred("> 20", TC_Q_RC).op == TC_P_GT);
    CHECK(pred(">= 20", TC_Q_RC).op == TC_P_GE);
    CHECK(pred(">=20", TC_Q_RC).op == TC_P_GE);
    p = pred("== $FFFFFFFF", TC_Q_CHECK);
    CHECK((p.rc == 0 && p.num == -1));
    p = pred("== $10", TC_Q_CHECK);
    CHECK(p.num == 16);
    p = pred("== $7fffffff", TC_Q_CHECK);
    CHECK(p.num == 0x7fffffff);
    // words
    p = pred("== fail", TC_Q_CHECK);
    CHECK((p.rc == 0 && p.vtype == TC_PV_WORD && p.str == "fail"));
    CHECK(pred("!= not-run", TC_Q_CHECK).str == "not-run");
    CHECK(pred("== timeout", TC_Q_SESSION).str == "timeout");
    CHECK(pred("== ok", TC_Q_COMPILE).rc == 0);
    // strings, with the escapes \" and \\.
    p = pred("contains \"Shape too \\\"Big\\\" \\\\\"", TC_Q_OUTPUT);
    CHECK((p.rc == 0 && p.op == TC_P_CONTAINS && p.vtype == TC_PV_STR && p.str == "Shape too \"Big\" \\"));
    CHECK(pred("not contains \"x\"", TC_Q_OUTPUT).op == TC_P_NOT_CONTAINS);
    CHECK(pred("matches \"error # *\"", TC_Q_COMPILE).op == TC_P_MATCHES);
    CHECK(pred("== \"line one\nline two\n\"", TC_Q_SERIAL).rc == 0);
    CHECK(pred("== \"ab\"", TC_Q_CHECK).vtype == TC_PV_STR);
    // exists, count
    CHECK(pred("exists", TC_Q_END_CASE).op == TC_P_EXISTS);
    CHECK(pred("not exists", TC_Q_END_RUN).op == TC_P_NOT_EXISTS);
    CHECK(pred("exists", TC_Q_OUTPUT).rc == 0);
    p = pred("count == 0", TC_Q_REQUESTER);
    CHECK((p.rc == 0 && p.op == TC_P_COUNT && p.cmp == TC_P_EQ && p.num == 0));
    p = pred("count >= 2", TC_Q_REQUESTER);
    CHECK((p.op == TC_P_COUNT && p.cmp == TC_P_GE && p.num == 2));
    CHECK(pred("contains \"Shape too Big\"", TC_Q_REQUESTER).rc == 0);
    CHECK(!strcmp(tc_pop_name(TC_P_NOT_CONTAINS), "not contains"));
    // compile.inject: as inject, a text
    CHECK(pred("== \"inject: rc=0\n\"", TC_Q_COMPILE_INJECT).rc == 0);
    CHECK(pred("contains \"caches flushed\"", TC_Q_COMPILE_INJECT).rc == 0);
    CHECK(pred("not exists", TC_Q_COMPILE_INJECT).rc == 0);
    pred_refused("count == 1", TC_Q_COMPILE_INJECT, "count is not allowed for this query");
    pred_refused("== 0", TC_Q_COMPILE_INJECT, "the value does not fit ==");
}

TEST_CASE("testassert: refused predicates", "[testassert]")
{
    pred_refused("", TC_Q_RC, "an operator expected");
    pred_refused("= 0", TC_Q_RC, "an operator expected");
    pred_refused("equals 0", TC_Q_RC, "an operator expected");
    pred_refused("containsx \"a\"", TC_Q_OUTPUT, "an operator expected");
    // operators a query does not allow
    pred_refused("contains \"a\"", TC_Q_RC, "contains is not allowed for this query");
    pred_refused("count == 1", TC_Q_OUTPUT, "count is not allowed for this query");
    pred_refused("== 1", TC_Q_END_CASE, "== is not allowed for this query");
    pred_refused("exists", TC_Q_SESSION, "exists is not allowed for this query");
    pred_refused("exists", TC_Q_CHECK, "exists is not allowed for this query");
    pred_refused("< 3", TC_Q_REQUESTER, "< is not allowed for this query");
    // values that do not fit
    pred_refused("== \"0\"", TC_Q_RC, "the value does not fit ==");
    pred_refused("== ok", TC_Q_RC, "the value does not fit ==");
    pred_refused("== maybe", TC_Q_CHECK, "a value of this query expected (ok|fail|not-run)");
    pred_refused("== done", TC_Q_CHECK, "(ok|fail|not-run)");
    pred_refused("== crash", TC_Q_SESSION, "(done|timeout|halt)");
    pred_refused("== \"done\"", TC_Q_SESSION, "(done|timeout|halt)");
    pred_refused("== 3", TC_Q_OUTPUT, "the value does not fit ==");
    pred_refused("contains 3", TC_Q_OUTPUT, "the value does not fit contains");
    pred_refused("< \"a\"", TC_Q_CHECK, "the value does not fit <");
    pred_refused("count == \"1\"", TC_Q_REQUESTER, "the value does not fit ==");
    pred_refused("count 1", TC_Q_REQUESTER, "count needs a comparison");
    // malformed values
    pred_refused("==", TC_Q_RC, "a value expected after ==");
    pred_refused("== $", TC_Q_RC, "a malformed integer");
    pred_refused("== $123456789", TC_Q_RC, "a malformed integer");
    pred_refused("== 012", TC_Q_RC, "a malformed integer");
    pred_refused("== -", TC_Q_RC, "a malformed integer");
    pred_refused("== 99999999999999999999", TC_Q_RC, "a malformed integer");
    pred_refused("contains \"a", TC_Q_OUTPUT, "a string without its closing");
    pred_refused("contains \"a\\n\"", TC_Q_OUTPUT, "only \\\" and \\\\ are escapes");
    pred_refused("== 0 1", TC_Q_RC, "unexpected text \"1\"");
    pred_refused("exists x", TC_Q_OUTPUT, "unexpected text \"x\"");
    pred_refused("contains \"a\" b", TC_Q_OUTPUT, "unexpected text \"b\"");
}

// ------------------------------------------------------------ configurations

struct tree {
    fs::path root;

    tree()
    {
        std::string t = (fs::temp_directory_path() / "testassert-XXXXXX").string();

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

struct built {
    tc_env env = {};
    tc_config c = {};
    tc_matrix m = {};
    int rc = 0;

    built(const tree &t)
    {
        std::string text = "top = \"" + t.root.string() + "\"\n";

        REQUIRE(tc_env_parse("env.toml", text.c_str(), &env, err, sizeof err) == 0);
        err[0] = 0;
        rc = tc_resolve(t.path("x.test.bas").c_str(), &env, &c, err, sizeof err);
        if (rc == 0)
            rc = tc_matrix_build(&c, &m, err, sizeof err);
    }
    ~built()
    {
        tc_matrix_free(&m);
        tc_free(&c);
        tc_env_free(&env);
    }
    size_t cell(const char *name) const
    {
        for (size_t i = 0; i < m.ncells; i++)
            if (!strcmp(m.cells[i].name, name))
                return i;
        FAIL("no cell");
        return 0;
    }
    // the assertions of a cell and case as "query=pred@depth[D]" in order
    std::string asserts(const char *cellname, const char *case_name, int mode = TC_PLAIN) const
    {
        tc_assertions a;
        std::string r;

        if (tc_assertions_for(&c, &m, cell(cellname), case_name, mode, &a, err, sizeof err))
            return std::string("error: ") + err;
        for (size_t i = 0; i < a.n; i++) {
            if (!r.empty())
                r += "; ";
            r += std::string(a.a[i].query) + " " + a.a[i].pred;
            if (a.a[i].deviation)
                r += " [D]";
            else if (a.a[i].at.depth != UINT_MAX)
                r += " @" + std::to_string(a.a[i].at.depth);
        }
        tc_assertions_free(&a);
        return r;
    }
};

static void resolve_refused(const tree &t, const std::string &what)
{
    built b(t);

    INFO(what);
    CHECK(b.rc == -1);
    CHECK(strstr(err, what.c_str()) != nullptr);
    if (!strstr(err, what.c_str()))
        fprintf(stderr, "message: %s\n", err);
}

TEST_CASE("testassert: checked when resolved", "[testassert]")
{
    tree t;

    t.write("tests.toml", "root = true\n[asserts]\n\nexitcode = \"== 0\"\n");
    resolve_refused(t, t.path("tests.toml") + ":4: unknown query \"exitcode\"");
    t.write("tests.toml", "root = true\n[asserts]\nrc = \"contains \\\"x\\\"\"\n");
    resolve_refused(t, t.path("tests.toml") + ":3: rc: the predicate \"contains \"x\"\": contains is not allowed");
    t.write("tests.toml", "root = true\n[asserts.\"libs=cdd\"]\nrc = \"== 0\"\n");
    resolve_refused(t, ":3: [asserts.\"libs=cdd\"]: the cell selector \"libs=cdd\": libs must be one of");
    t.write("tests.toml", "root = true\n");
    t.write("x.test.toml", "[deviations.\"libs=cd\"]\n\"check.a\" = \"== fail\"\n");
    resolve_refused(t, t.path("x.test.toml") + ":2: the query \"check.a\"");
    t.write("x.test.toml", "fixed-orig = 7\n");
    resolve_refused(t, t.path("x.test.toml") + ":1: test x: fixed-orig without a declared deviation");
    t.write("x.test.toml", "fixed = 7\n[deviations.\"libs=cd\"]\n\"check.a.b\" = \"== fail\"\n");
    resolve_refused(t, ":1: test x: fixed with a declared deviation");
    t.write("x.test.toml", "extension = 7\n");
    resolve_refused(t, t.path("x.test.toml") + ":1: test x: extension without a declared deviation (what the "
                                               "original does instead)");
    // valid: the selector with case=
    t.write("x.test.toml", "[asserts.\"case=a,libs=cd\"]\nrc = \"== 5\"\n");
    built b(t);
    CHECK(b.rc == 0);
}

TEST_CASE("testassert: defaults", "[testassert]")
{
    size_t n;
    const tc_default *d = tc_defaults(&n);

    REQUIRE(n == 4);
    CHECK(!strcmp(d[0].query, "rc"));
    CHECK(!strcmp(d[0].pred, "== 0"));

    CHECK((d[0].shell && d[1].shell && !d[2].shell && !d[3].shell));

    tree t;
    t.write("tests.toml", "root = true\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("default", "a") == "rc == 0; requester count == 0; end.case exists; end.run exists");
    }
    // a cell that runs from the editor: no defaults of the exit code and the
    // requesters (a run from the editor captures neither)
    t.write("tests.toml", "root = true\n[matrix]\neditor-run = [\"off\", \"on\"]\n");
    built b(t);
    REQUIRE(b.rc == 0);
    CHECK(b.asserts("off", "a") == "rc == 0; requester count == 0; end.case exists; end.run exists");
    CHECK(b.asserts("on", "a") == "end.case exists; end.run exists");
}

TEST_CASE("testassert: a more specific assertion replaces", "[testassert]")
{
    tree t;

    // the test's own file replaces the root's exit code; the others stay
    t.write("tests.toml", "root = true\n[asserts]\nrc = \"== 0\"\noutput = \"contains \\\"ok\\\"\"\n");
    t.write("x.test.toml", "[asserts]\nrc = \"== 20\"\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("default", "a")
              == "rc == 20 @0; output contains \"ok\" @1; requester count == 0; end.case exists; end.run exists");
    }
    // in one file the most specific selector wins; a case selector counts
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\ncompiler = [\"port\", \"orig195\"]\n"
                          "[asserts]\nrc = \"== 0\"\n[asserts.\"libs=cd\"]\nrc = \"== 5\"\n"
                          "[asserts.\"libs=cd,case=b\"]\nrc = \"== 6\"\n");
    t.write("x.test.toml", "");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("port,clib", "a").rfind("rc == 0 @1;", 0) == 0);
        CHECK(b.asserts("port,cd", "a").rfind("rc == 5 @1;", 0) == 0);
        CHECK(b.asserts("port,cd", "b").rfind("rc == 6 @1;", 0) == 0);
        CHECK(b.asserts("port,clib", "b").rfind("rc == 0 @1;", 0) == 0);
    }
    // the nearest file wins before the selector: the test's rc for all cells
    t.write("x.test.toml", "[asserts]\nrc = \"== 20\"\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("port,cd", "b").rfind("rc == 20 @0;", 0) == 0);
    }
    // two selectors of the same rank for one cell: an error naming both
    t.write("x.test.toml", "[asserts.\"libs=cd\"]\nrc = \"== 1\"\n[asserts.\"compiler=orig195\"]\nrc = \"== 2\"\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("port,cd", "a").rfind("rc == 1 @0;", 0) == 0);
        CHECK(b.asserts("orig195,clib", "a").rfind("rc == 2 @0;", 0) == 0);
        std::string e = b.asserts("orig195,cd", "a");
        CHECK(e.find("error: ") == 0);
        CHECK(e.find(": rc: two assertions apply to cell orig195,cd alike (") != std::string::npos);
        CHECK(e.find(t.path("x.test.toml") + ":2)") != std::string::npos);
    }
    // the same predicate twice is no conflict
    t.write("x.test.toml", "[asserts.\"libs=cd\"]\nrc = \"== 1\"\n[asserts.\"compiler=orig195\"]\nrc = \"== 1\"\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.asserts("orig195,cd", "a").rfind("rc == 1 @0;", 0) == 0);
    }
}

TEST_CASE("testassert: deviations", "[testassert]")
{
    tree t;

    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n[asserts]\nrc = \"== 0\"\n");
    t.write("x.test.toml", "fixed-orig = 9\n[deviations.\"libs=cd\"]\n\"check.a.behind\" = \"== fail\"\n"
                           "\"check.a.behind.actual\" = \"== 0\"\nrc = \"== 3\"\n"
                           "[deviations.\"libs=cd,case=b\"]\nrc = \"== 4\"\n");
    built b(t);
    REQUIRE(b.rc == 0);
    CHECK(tc_cell_faulty(&b.c, &b.m, b.cell("cd"), "a"));
    CHECK(!tc_cell_faulty(&b.c, &b.m, b.cell("clib"), "a"));
    // plain: no deviation
    CHECK(b.asserts("cd", "a") == "rc == 0 @1; requester count == 0; end.case exists; end.run exists");
    // the faulty cell with its deviations; the more specific one for case b
    CHECK(b.asserts("cd", "a", TC_DEVIATED)
          == "check.a.behind == fail [D]; check.a.behind.actual == 0 [D]; rc == 3 [D]; requester count == 0; "
             "end.case exists; end.run exists");
    CHECK(b.asserts("cd", "b", TC_DEVIATED).find("rc == 4 [D]") != std::string::npos);
    // a cell without the faulty part: no deviation applies
    CHECK(b.asserts("clib", "a", TC_DEVIATED)
          == "rc == 0 @1; requester count == 0; end.case exists; end.run exists");
    // as if the axis terms matched (an open bug's other cells)
    CHECK(b.asserts("clib", "a", TC_DEVIATED_ANY).find("rc == 3 [D]") != std::string::npos);
    CHECK(b.asserts("clib", "b", TC_DEVIATED_ANY).find("rc == 4 [D]") != std::string::npos);
}

// the alternative ends of a cell as "SEL: NAME=Q P,Q P; NAME=..." per table
static std::string ends_of(const built &b, const char *cellname, const char *case_name, int mode)
{
    tc_ends e;
    std::string r;

    if (tc_ends_for(&b.c, &b.m, b.cell(cellname), case_name, mode, &e, err, sizeof err))
        return std::string("error: ") + err;
    for (size_t i = 0; i < e.n; i++) {
        r += std::string(r.empty() ? "" : " | ") + e.e[i].sel + " (" + e.e[i].reason + "):";
        for (size_t k = 0; k < e.e[i].nalt; k++) {
            r += std::string(" ") + e.e[i].alt[k].name + "=";
            for (size_t j = 0; j < e.e[i].alt[k].as.n; j++)
                r += std::string(j ? "," : "") + e.e[i].alt[k].as.a[j].query + " " + e.e[i].alt[k].as.a[j].pred
                     + (e.e[i].alt[k].as.a[j].deviation ? "" : "?");
        }
    }
    tc_ends_free(&e);
    return r;
}

TEST_CASE("testassert: alternative ends", "[testassert]")
{
    tree t;
    const std::string own = "[ends.\"libs=cd\"]\nreason = \"the memory behind the table\"\n";

    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    // valid: two alternatives; they count as the declared deviation of fixed-orig
    // and of an extension
    t.write("x.test.toml", "fixed-orig = 82\n" + own
                               + "returns = [\"session == done\", \"rc == 0\", \"check.a.range == ok\"]\n"
                                 "hangs = [\"session == timeout\", \"end.run not exists\"]\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(b.c.nends == 3);
        CHECK(tc_cell_faulty(&b.c, &b.m, b.cell("cd"), "a"));
        CHECK(!tc_cell_faulty(&b.c, &b.m, b.cell("clib"), "a"));
        CHECK(ends_of(b, "cd", "a", TC_DEVIATED)
              == "libs=cd (the memory behind the table): returns=session == done,rc == 0,check.a.range == ok "
                 "hangs=session == timeout,end.run not exists");
        // not in the cells without the faulty part, nor judged plain; as if
        // its axis terms matched for an open bug's other cells
        CHECK(ends_of(b, "clib", "a", TC_DEVIATED) == "");
        CHECK(ends_of(b, "cd", "a", TC_PLAIN) == "");
        CHECK(ends_of(b, "clib", "a", TC_DEVIATED_ANY).find("hangs=") != std::string::npos);
    }
    t.write("x.test.toml", "extension = 5\n" + own + "one = [\"output contains \\\"x\\\"\"]\n");
    {
        built b(t);
        CHECK(b.rc == 0);
    }
    // a case term restricts it
    t.write("x.test.toml", "[ends.\"libs=cd,case=b\"]\nreason = \"r\"\none = [\"rc == 0\"]\n");
    {
        built b(t);
        REQUIRE(b.rc == 0);
        CHECK(ends_of(b, "cd", "a", TC_DEVIATED) == "");
        CHECK(ends_of(b, "cd", "b", TC_DEVIATED) == "libs=cd,case=b (r): one=rc == 0");
    }
    // broken input, each naming file and line: no reason, an empty reason, no
    // alternative, an empty alternative, a string alternative, no selector, in
    // a tests.toml, with fixed, a refused query, an element without a
    // predicate, a malformed predicate, a malformed selector
    t.write("x.test.toml", "[ends.\"libs=cd\"]\nreturns = [\"rc == 0\"]\n");
    resolve_refused(t, t.path("x.test.toml") + ":1: [ends.\"libs=cd\"] has no reason");
    t.write("x.test.toml", "[ends.\"libs=cd\"]\nreason = \"\"\nreturns = [\"rc == 0\"]\n");
    resolve_refused(t, t.path("x.test.toml") + ":2: the reason of [ends.\"libs=cd\"] must be a non-empty string");
    t.write("x.test.toml", own);
    resolve_refused(t, t.path("x.test.toml") + ":1: [ends.\"libs=cd\"] declares no alternative end");
    t.write("x.test.toml", own + "returns = []\n");
    resolve_refused(t, t.path("x.test.toml") + ":3: the alternative end returns must be a non-empty list of "
                                               "\"QUERY PREDICATE\"");
    t.write("x.test.toml", own + "returns = \"rc == 0\"\n");
    resolve_refused(t, ":3: the alternative end returns must be a non-empty list");
    t.write("x.test.toml", "[ends]\nreason = \"r\"\n");
    resolve_refused(t, t.path("x.test.toml") + ":1: alternative ends need the cell selector");
    t.write("x.test.toml", "");
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n" + own + "one = [\"rc == 0\"]\n");
    resolve_refused(t, t.path("tests.toml") + ":4: ends is a key of a single test, valid only in its ID.test.toml");
    t.write("tests.toml", "root = true\n[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    t.write("x.test.toml", "fixed = 3\n" + own + "one = [\"rc == 0\"]\n");
    resolve_refused(t, t.path("x.test.toml") + ":1: test x: fixed with declared alternative ends");
    for (const char *q : { "compile == ok", "compile.log contains \\\"x\\\"", "serial contains \\\"x\\\"",
                           "inject contains \\\"x\\\"" }) {
        INFO(q);
        t.write("x.test.toml", own + "one = [\"rc == 0\", \"" + q + "\"]\n");
        resolve_refused(t, t.path("x.test.toml") + ":3: the alternative end one: the query ");
        resolve_refused(t, "does not vary with the end");
    }
    t.write("x.test.toml", own + "one = [\"session\"]\n");
    resolve_refused(t, t.path("x.test.toml") + ":3: the alternative end one: \"session\": QUERY PREDICATE expected");
    t.write("x.test.toml", own + "one = [\"session == maybe\"]\n");
    resolve_refused(t, t.path("x.test.toml") + ":3: the alternative end one: \"session == maybe\": ");
    t.write("x.test.toml", "[ends.\"libs=cdd\"]\nreason = \"r\"\none = [\"rc == 0\"]\n");
    resolve_refused(t, ":2: [ends.\"libs=cdd\"]: the cell selector \"libs=cdd\": libs must be one of");
}
