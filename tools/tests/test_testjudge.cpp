// test_testjudge - the judge of the Blitz tests of the model (tools/lib/
// testjudge) on recorded result files and observations from memory: the
// verdicts of checks with several entries, preconditions, the end entries
// of the case and the run, announced runtime errors against the requesters,
// the default assertions and their replacement, the queries of the other
// observations, declared deviations with validators, the checks behind a
// declared crash, and the verdict of a case over its cells - each rule
// with broken input (a malformed result file among them).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testjudge.h"
}

namespace fs = std::filesystem;

static char err[600];

// the source of the tests: two cases, a procedure, a batch init
static const char source[] =
    "Statement helper{v}\n"                                 // 1
    "  !CheckEq{v,1,\"helper\"}\n"                          // 2  K0
    "End Statement\n"                                       // 3
    "!TestBatchInit\n"                                      // 4
    "  !Require{-1,\"setup\"}\n"                            // 5  K1
    "  !CheckEq{1,1,\"initcheck\"}\n"                       // 6  K2
    "!EndTestBatchInit\n"                                   // 7
    "!TestCase{frames_maximum}\n"                           // 8
    "  !CheckEq{ShapeWidth(99),16,\"width\"}\n"             // 9  K3
    "  !Check{Len(\"abc\")=3,\"len\"}\n"                    // 10 K4
    "  For i=1 To 3 : !CheckEq{a(i),1,\"loop\"} : Next\n"   // 11 K5
    "  !CheckStrEq{a$,\"ab\",\"str\"}\n"                    // 12 K6
    "  !ExpectError{\"Shape too Big\"}\n"                   // 13 K7
    "  !CheckEq{x,2}\n"                                     // 14 K8
    "!EndTestCase\n"                                        // 15
    "!TestCase{other}\n"                                    // 16
    "  !CheckEq{y,3,\"y\"}\n"                               // 17 K9
    "!EndTestCase\n";                                       // 18

struct fixture {
    fs::path root;
    tp_source src = {};
    tc_env env = {};
    tc_config conf = {};
    tc_matrix m = {};
    tj_test t = {};

    fixture(const std::string &dir_toml, const std::string &own_toml = "", const char *case_name = "frames_maximum",
            const char *text = source)
    {
        std::string tmp = (fs::temp_directory_path() / "testjudge-XXXXXX").string();

        REQUIRE(mkdtemp(tmp.data()) != nullptr);
        root = tmp;
        std::ofstream(root / "tests.toml") << "root = true\n" << dir_toml;
        if (!own_toml.empty())
            std::ofstream(root / "x.test.toml") << own_toml;
        std::ofstream(root / "x.test.bas") << text;
        tp_params p = {};
        p.file = "x.test.bas";
        p.test_id = "x";
        REQUIRE(tp_process(text, strlen(text), &p, &src) == 0);
        REQUIRE((text != source || src.nchecks == 10));
        std::string e = "top = \"" + root.string() + "\"\n";
        REQUIRE(tc_env_parse("env.toml", e.c_str(), &env, err, sizeof err) == 0);
        if (tc_resolve((root / "x.test.bas").c_str(), &env, &conf, err, sizeof err))
            FAIL(err);
        REQUIRE(tc_matrix_build(&conf, &m, err, sizeof err) == 0);
        t.file = "x.test.bas";
        t.src = &src;
        t.conf = &conf;
        t.m = &m;
        t.case_name = case_name;
    }
    ~fixture()
    {
        tc_matrix_free(&m);
        tc_free(&conf);
        tc_env_free(&env);
        tp_free(&src);
        fs::remove_all(root);
    }
    size_t cell(const char *name) const
    {
        for (size_t i = 0; i < m.ncells; i++)
            if (!strcmp(m.cells[i].name, name))
                return i;
        FAIL("no cell");
        return 0;
    }
};

// observations: a result file (lines with "|" for TAB), exit code 0,
// session done
struct obs {
    std::string result;
    std::vector<tj_requester> req;
    tj_obs o = {};

    obs(const std::string &lines, long long rc = 0)
    {
        for (char c : lines)
            result += c == '|' ? '\t' : c;
        o.has_result = 1;
        o.result = result.data();
        o.result_len = result.size();
        o.has_rc = 1;
        o.rc = rc;
        o.session = "done";
        o.compile = "ok";
    }
    obs &requester(const char *text, size_t after = (size_t)-1)
    {
        req.push_back({ text, after });
        o.req = req.data();
        o.nreq = req.size();
        return *this;
    }
};

static const std::string head = "begin|1\nblock|init\nblockend\nblock|case|frames_maximum\n";
static const std::string tail = "blockend\nend\n";
// a run of frames_maximum where every check passes (K7 announces nothing
// since the run takes the other way; K8 is not run)
static const std::string good = head
                                + "check|3|ok|$00000010|$00000010\n"
                                  "check|4|ok|$FFFFFFFF|$FFFFFFFF\n"
                                  "check|5|ok|$00000001|$00000001\n"
                                  "check|5|ok|$00000001|$00000001\n"
                                  "checkstr|6|ok|ab|ab|-\n"
                                + tail;

static std::string join(const tj_msgs &m)
{
    std::string r;

    for (size_t i = 0; i < m.n; i++)
        r += std::string(m.v[i]) + "\n";
    return r;
}

// judge one cell (plain by default): the result and the messages
static int judge(const fixture &f, const obs &o, std::string &out, const char *cell = "default", int mode = TC_PLAIN)
{
    tj_msgs m = {};
    int r = tj_judge_cell(&f.t, f.cell(cell), mode, &o.o, &m);

    out = join(m);
    tj_msgs_free(&m);
    return r;
}

static bool has(const std::string &text, const std::string &part)
{
    if (text.find(part) != std::string::npos)
        return true;
    fprintf(stderr, "missing \"%s\" in:\n%s", part.c_str(), text.c_str());
    return false;
}

TEST_CASE("testjudge: a passing case", "[testjudge]")
{
    fixture f("");
    std::string m;

    CHECK(judge(f, obs(good), m) == TJ_PASS);
    CHECK(m == "");
}

TEST_CASE("testjudge: a failed check and its report", "[testjudge]")
{
    fixture f("");
    std::string m;

    // the report names the case, the name, the line, the expression and
    // both values
    obs o(head + "check|3|fail|$00000010|$00000000\n" + tail);
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum width (x.test.bas:9) failed: ShapeWidth(99): expected 16 ($00000010), "
                 "actual 0 ($00000000)"));
    // the expression as written, quotes and all
    obs q(head + "check|4|fail|$FFFFFFFF|$00000000\n" + tail);
    CHECK(judge(f, q, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum len (x.test.bas:10) failed: Len(\"abc\")=3 is 0"));
    // a string check
    obs s(head + "checkstr|6|fail|ab|a\\x09b|-\n" + tail);
    CHECK(judge(f, s, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum str (x.test.bas:12) failed: a$: expected \"ab\", actual \"a\\x09b\""));
    // values the library shortened to fit the line
    obs cut(head + "checkstr|6|fail|abc|abd|cut\n" + tail);
    CHECK(judge(f, cut, m) == TJ_FAIL);
    CHECK(has(m, "a$: expected \"abc\" (cut), actual \"abd\" (cut)"));
    // a check without a name is reported by its number in the namespace
    obs n(head + "check|8|fail|$00000002|$00000003\n" + tail);
    CHECK(judge(f, n, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum #5 (x.test.bas:14) failed: x: expected 2"));
    // a failed check of the batch init fails the case
    obs i("begin|1\nblock|init\ncheck|2|fail|$00000001|$00000002\nblockend\nblock|case|frames_maximum\n" + tail);
    CHECK(judge(f, i, m) == TJ_FAIL);
    CHECK(has(m, "check init initcheck (x.test.bas:6) failed"));
    // a check of the procedure while the case ran
    obs p(head + "check|0|fail|$00000001|$00000005\n" + tail);
    CHECK(judge(f, p, m) == TJ_FAIL);
    CHECK(has(m, "check helper helper (x.test.bas:2) failed: v: expected 1"));
}

TEST_CASE("testjudge: a check with several entries", "[testjudge]")
{
    // a check in a loop passes twice and fails the third time with 7
    fixture f("[asserts]\n\"check.frames_maximum.loop\" = \"== fail\"\n"
              "\"check.frames_maximum.loop.actual\" = \"== 7\"\n"
              "\"check.frames_maximum.loop.expected\" = \"== 1\"\n");
    std::string m;
    obs o(head + "check|5|ok|$00000001|$00000001\ncheck|5|ok|$00000001|$00000001\n"
                 "check|5|fail|$00000001|$00000007\ncheck|5|fail|$00000001|$00000009\n" + tail);
    CHECK(judge(f, o, m) == TJ_PASS);
    // without a failure: the first entry
    fixture g("[asserts]\n\"check.frames_maximum.loop.actual\" = \"== 2\"\n");
    obs p(head + "check|5|ok|$00000002|$00000002\ncheck|5|ok|$00000003|$00000003\n" + tail);
    CHECK(judge(g, p, m) == TJ_PASS);
    // the verdict fails: the default reports the failure
    fixture d("");
    CHECK(judge(d, o, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum loop (x.test.bas:11) failed: a(i): expected 1 ($00000001), actual 7 "
                 "($00000007) (its first failed entry)"));
}

TEST_CASE("testjudge: queries of checks", "[testjudge]")
{
    std::string m;
    {
        // a verdict that does not hold, with the details
        fixture f("[asserts]\n\"check.frames_maximum.width\" = \"== ok\"\n");
        CHECK(judge(f, obs(head + "check|3|fail|$00000010|$00000000\n" + tail), m) == TJ_FAIL);
        CHECK(has(m, "assertion check.frames_maximum.width == ok (" + (f.root / "tests.toml").string()
                         + ":3) does not hold: fail"));
        CHECK(has(m, "check frames_maximum width (x.test.bas:9) failed"));
        // a check that did not run
        CHECK(judge(f, obs(head + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: not-run"));
    }
    {
        fixture f("[asserts]\n\"check.frames_maximum.width\" = \"== not-run\"\n"
                  "\"check.frames_maximum.str.actual\" = \"contains \\\"b\\\"\"\n");
        CHECK(judge(f, obs(head + "checkstr|6|fail|ab|xb|-\n" + tail), m) == TJ_PASS);
        CHECK(judge(f, obs(head + "checkstr|6|fail|ab|xy|-\n" + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: \"xy\""));
        // a cut value: contains still works and the report says it was cut
        CHECK(judge(f, obs(head + "checkstr|6|fail|ab|xb|cut\n" + tail), m) == TJ_PASS);
        CHECK(judge(f, obs(head + "checkstr|6|fail|ab|xy|cut\n" + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: \"xy\" (cut)"));
        // == cannot be decided on a cut value
        fixture g("[asserts]\n\"check.frames_maximum.str.actual\" = \"== \\\"xy\\\"\"\n");
        CHECK(judge(g, obs(head + "checkstr|6|fail|ab|xy|cut\n" + tail), m) == TJ_ERROR);
        CHECK(has(m, "the value was cut to fit the line; compare it with contains or matches"));
        CHECK(judge(g, obs(head + "checkstr|6|fail|ab|xy|-\n" + tail), m) == TJ_PASS);
    }
    {
        // a value of a check that did not run
        fixture f("[asserts]\n\"check.frames_maximum.width.actual\" = \"== 0\"\n");
        CHECK(judge(f, obs(head + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: not run"));
        // $hex and signed values
        fixture g("[asserts]\n\"check.frames_maximum.width.actual\" = \"== -1\"\n");
        CHECK(judge(g, obs(head + "check|3|fail|$00000010|$FFFFFFFF\n" + tail), m) == TJ_PASS);
    }
    {
        // queries the table cannot answer
        fixture f("[asserts]\n\"check.frames_maximum.nope\" = \"== ok\"\n");
        CHECK(judge(f, obs(good), m) == TJ_ERROR);
        CHECK(has(m, "check.frames_maximum.nope: the source has no check nope in frames_maximum"));
        fixture g("[asserts]\n\"check.frames_maximum.str.actual\" = \"== 3\"\n");
        CHECK(judge(g, obs(good), m) == TJ_ERROR);
        CHECK(has(m, "the value of this check is a string"));
        fixture h("[asserts]\n\"check.frames_maximum.width.actual\" = \"== \\\"x\\\"\"\n");
        CHECK(judge(h, obs(good), m) == TJ_ERROR);
        CHECK(has(m, "the value of this check is a long"));
        fixture i("[asserts]\n\"check.frames_maximum.width\" = \"== 3\"\n");
        CHECK(judge(i, obs(good), m) == TJ_ERROR);
        CHECK(has(m, "a verdict is ok, fail or not-run"));
    }
    {
        // a query of another case's check does not apply to this case
        fixture f("[asserts]\n\"check.other.y\" = \"== fail\"\n");
        CHECK(judge(f, obs(good), m) == TJ_PASS);
    }
}

TEST_CASE("testjudge: preconditions and a broken library", "[testjudge]")
{
    fixture f("");
    std::string m;

    CHECK(judge(f, obs("begin|1\nblock|init\nrequire|1|broken\nblockend\nblock|case|frames_maximum\n" + tail), m)
          == TJ_BROKEN);
    CHECK(has(m, "precondition init setup (x.test.bas:5) failed: -1"));
    CHECK(judge(f, obs(good.substr(0, good.size() - 4) + "broken|guard area\\x09\nend\n"), m) == TJ_BROKEN);
    CHECK(has(m, "the test library reports its state broken (line 11): guard area\\x09"));
}

TEST_CASE("testjudge: the end entries", "[testjudge]")
{
    fixture f("");
    std::string m;

    // End inside a case: no end entry of the case
    CHECK(judge(f, obs(head + "check|3|ok|$00000010|$00000010\nincomplete\nend\n"), m) == TJ_FAIL);
    CHECK(has(m, "assertion end.case exists ((default):0) does not hold: absent"));
    // a crash: no end of the run
    CHECK(judge(f, obs(head + "check|3|ok|$00000010|$00000010\n"), m) == TJ_FAIL);
    CHECK(has(m, "assertion end.run exists ((default):0) does not hold: absent"));
    // no result file at all
    obs none("");
    none.o.has_result = 0;
    CHECK(judge(f, none, m) == TJ_FAIL);
    CHECK(has(m, "no result file"));
    CHECK(has(m, "end.run exists"));
}

TEST_CASE("testjudge: announced runtime errors", "[testjudge]")
{
    fixture f("");
    std::string m;
    const std::string ann = head + "check|3|ok|$00000010|$00000010\nexpect-error|7\nincomplete\nend\n";

    // the error comes after the announcement (line 6): the case ended as
    // expected, its exit code does not count, the requester is not shown
    // to the query requester
    CHECK(judge(f, obs(ann, 20).requester("Program Error: Shape too Big", 6), m) == TJ_PASS);
    CHECK(judge(f, obs(ann, 20).requester("Shape too Big"), m) == TJ_PASS);
    // it does not come
    CHECK(judge(f, obs(ann, 20), m) == TJ_FAIL);
    CHECK(has(m, "the announced runtime error \"Shape too Big\" (x.test.bas:13) did not come"));
    // another one comes
    CHECK(judge(f, obs(ann, 20).requester("Out of Memory"), m) == TJ_FAIL);
    CHECK(has(m, "did not come"));
    CHECK(has(m, "assertion requester count == 0 ((default):0) does not hold: 1, the first \"Out of Memory\""));
    // it came before the announcement
    CHECK(judge(f, obs(ann, 20).requester("Shape too Big", 5), m) == TJ_FAIL);
    CHECK(has(m, "did not come"));
    // a requester nobody announced
    CHECK(judge(f, obs(good).requester("Software Failure"), m) == TJ_FAIL);
    CHECK(has(m, "requester count == 0"));
    // an assertion may expect a requester
    fixture g("[asserts]\nrequester = \"contains \\\"Software\\\"\"\n");
    CHECK(judge(g, obs(good).requester("Software Failure"), m) == TJ_PASS);
    CHECK(judge(g, obs(good), m) == TJ_FAIL);
    CHECK(has(m, "does not hold: none"));
}

TEST_CASE("testjudge: the defaults and their replacement", "[testjudge]")
{
    std::string m;
    {
        // exit code: the default 0
        fixture f("");
        CHECK(judge(f, obs(good, 5), m) == TJ_FAIL);
        CHECK(has(m, "assertion rc == 0 ((default):0) does not hold: 5"));
        obs no(good);
        no.o.has_rc = 0;
        CHECK(judge(f, no, m) == TJ_FAIL);
        CHECK(has(m, "does not hold: no exit code"));
    }
    {
        // the root asserts 0, the test's own file 20: judged against 20 only
        fixture f("[asserts]\nrc = \"== 0\"\n", "[asserts]\nrc = \"== 20\"\n");
        CHECK(judge(f, obs(good, 20), m) == TJ_PASS);
        CHECK(judge(f, obs(good, 0), m) == TJ_FAIL);
        CHECK(has(m, "assertion rc == 20 (" + (f.root / "x.test.toml").string() + ":2) does not hold: 0"));
    }
    {
        // a crash declared as the expected end
        fixture f("[asserts]\n\"end.case\" = \"not exists\"\n\"end.run\" = \"not exists\"\nrc = \"exists\"\n");
        CHECK(judge(f, obs(head + "check|3|ok|$00000010|$00000010\n", 3), m) == TJ_PASS);
        CHECK(judge(f, obs(good), m) == TJ_FAIL);
        CHECK(has(m, "assertion end.case not exists"));
    }
}

TEST_CASE("testjudge: the other observations", "[testjudge]")
{
    std::string m;
    fixture f("[asserts]\noutput = \"contains \\\"ok\\\"\"\nsession = \"== done\"\ncompile = \"== ok\"\n"
              "serial = \"matches \\\"rx # bytes\\\"\"\ninject = \"not contains \\\"leak\\\"\"\n"
              "\"compile.log\" = \"exists\"\n");
    obs o(good);
    o.o.output = "first\nok\n";
    o.o.serial = "start\r\nrx 12 bytes\r\n";
    o.o.inject = "opens 0\n";
    o.o.compile_log = "RESULT x ok\n";
    CHECK(judge(f, o, m) == TJ_PASS);
    // printed nothing
    o.o.output = "";
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "assertion output contains \"ok\" (" + (f.root / "tests.toml").string() + ":3) does not hold: \"\""));
    o.o.output = "ok";
    o.o.serial = "rx bytes\n";
    o.o.inject = "a leak of 8 bytes\n";
    o.o.session = "timeout";
    o.o.compile = "error 3 Syntax Error";
    o.o.compile_log = nullptr;
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "serial matches"));
    CHECK(has(m, "inject not contains"));
    CHECK(has(m, "session == done"));
    CHECK(has(m, "compile == ok"));
    CHECK(has(m, "assertion compile.log exists (" + (f.root / "tests.toml").string() + ":8) does not hold: not captured"));
}

TEST_CASE("testjudge: malformed result files", "[testjudge]")
{
    fixture f("");
    std::string m;

    CHECK(judge(f, obs("begin|1\nblock|case|frames_maximum\ncheck|03|ok|$00000001|$00000001\n" + tail), m)
          == TJ_ERROR);
    CHECK(has(m, "the result file: line 3: check: malformed value in field 2"));
    CHECK(judge(f, obs("block|case|frames_maximum\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "line 1 is no begin entry"));
    CHECK(judge(f, obs(""), m) == TJ_ERROR);
    CHECK(has(m, "the result file is empty"));
    CHECK(judge(f, obs(good + "blockend\n"), m) == TJ_ERROR);
    CHECK(has(m, "line 12 follows the end entry"));
    CHECK(judge(f, obs(head + "check|3|ok|$00000010|$00000010"), m) == TJ_ERROR);
    CHECK(has(m, "line 5 has no line end"));
    CHECK(judge(f, obs("begin|1\ncheck|3|ok|$00000010|$00000010\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "an entry outside a block in line 2"));
    CHECK(judge(f, obs("begin|1\nblockend\nend\n"), m) == TJ_ERROR);
    CHECK(has(m, "a blockend outside a block in line 2"));
    CHECK(judge(f, obs("begin|1\nbegin|1\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "a second begin entry in line 2"));
    // numbers and kinds the table does not have
    CHECK(judge(f, obs(head + "check|42|ok|$00000010|$00000010\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "line 5 names check 42, which the source does not have"));
    CHECK(judge(f, obs(head + "checkstr|3|ok|a|a|-\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "line 5 is no entry of check 3 (!CheckEq, x.test.bas:9)"));
    // a case that was not selected, the case twice
    CHECK(judge(f, obs(good.substr(0, good.size() - 4) + "block|case|other\nblockend\nend\n"), m) == TJ_FAIL);
    CHECK(has(m, "the result file holds the case other (line 11), which was not selected"));
    CHECK(judge(f, obs(good.substr(0, good.size() - 4) + "block|case|frames_maximum\nblockend\nend\n"), m)
          == TJ_FAIL);
    CHECK(has(m, "holds the case frames_maximum twice"));
}

// ------------------------------------------------------------ deviations and verdicts

static const char *const matrix = "[matrix]\nlibs = [\"cd\", \"clib\"]\n";
static const std::string declared = "[deviations.\"libs=cd\"]\n\"check.frames_maximum.width\" = \"== fail\"\n"
                                    "\"check.frames_maximum.width.actual\" = \"== 0\"\n";
static const std::string width_fails = head + "check|3|fail|$00000010|$00000000\n" + tail;
static const std::string width_wrong = head + "check|3|fail|$00000010|$00000005\n" + tail;

static int verdict(const fixture &f, const obs &cd, const obs &clib, std::string &out)
{
    size_t cells[2] = { f.cell("cd"), f.cell("clib") };
    tj_obs o[2] = { cd.o, clib.o };
    tj_msgs m = {};
    int r = tj_verdict(&f.t, cells, o, 2, &m);

    out = join(m);
    tj_msgs_free(&m);
    return r;
}

TEST_CASE("testjudge: a test without deviations", "[testjudge]")
{
    fixture f(matrix);
    std::string m;

    CHECK(!tj_case_has_deviation(&f.t));
    CHECK(verdict(f, obs(good), obs(good), m) == TJ_V_PASSED);
    CHECK(m == "");
    CHECK(verdict(f, obs(good), obs(width_fails), m) == TJ_V_FAILED);
    CHECK(has(m, "case frames_maximum, cell clib:\n  check frames_maximum width"));
    CHECK(verdict(f, obs(good), obs("begin|1\nblock|init\nrequire|1|broken\nblockend\nend\n"), m) == TJ_V_BROKEN);
    CHECK(verdict(f, obs("x\n"), obs(good), m) == TJ_V_ERROR);
}

TEST_CASE("testjudge: a fixed original bug", "[testjudge]")
{
    fixture f(matrix, "fixed-orig = 265\n" + declared);
    std::string m;

    CHECK(tj_case_has_deviation(&f.t));
    // the CD's libraries show the declared deviation, the C libraries pass
    CHECK(verdict(f, obs(width_fails), obs(good), m) == TJ_V_PASSED);
    // the port regresses: a cell without the faulty part fails the check
    CHECK(verdict(f, obs(width_fails), obs(width_fails), m) == TJ_V_FAILED);
    CHECK(has(m, "case frames_maximum, cell clib:\n  check frames_maximum width (x.test.bas:9) failed"));
    // the original no longer deviates
    CHECK(verdict(f, obs(good), obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "cell cd (the original's faulty part, its declared deviations):"));
    CHECK(has(m, "assertion check.frames_maximum.width == fail (" + (f.root / "x.test.toml").string()
                     + ":3, a declared deviation) does not hold: ok"));
    // the validator: another wrong value
    CHECK(verdict(f, obs(width_wrong), obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "check.frames_maximum.width.actual == 0"));
    CHECK(has(m, "does not hold: 5 ($00000005)"));
    // another check fails in the faulty cell
    CHECK(verdict(f, obs(head + "check|3|fail|$00000010|$00000000\ncheck|4|fail|$FFFFFFFF|$00000000\n" + tail),
                  obs(good), m)
          == TJ_V_FAILED);
    CHECK(has(m, "check frames_maximum len (x.test.bas:10) failed"));
    // the other case has no deviation: plain in every cell
    fixture o(matrix, "fixed-orig = 265\n" + declared, "other");
    CHECK(!tj_case_has_deviation(&o.t));
}

TEST_CASE("testjudge: an open original bug", "[testjudge]")
{
    fixture f(matrix, declared);
    std::string m;

    // both cells show the declared deviation: reproduced
    CHECK(verdict(f, obs(width_fails), obs(width_fails), m) == TJ_V_REPRODUCED);
    CHECK(m == "");
    // the C libraries pass: the bug is gone without a fixed issue
    CHECK(verdict(f, obs(width_fails), obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "cell clib (an open bug: its declared deviations):\n  the case passes without the declared "
                 "deviations, but the test has no fixed issue (fixed-orig)"));
    // the port fails differently: neither the bug nor the fix
    CHECK(verdict(f, obs(width_fails), obs(width_wrong), m) == TJ_V_FAILED);
    CHECK(has(m, "check.frames_maximum.width.actual == 0"));
    // broken never counts as reproduced
    CHECK(verdict(f, obs(width_fails), obs("begin|1\nblock|init\nrequire|1|broken\nblockend\nend\n"), m)
          == TJ_V_BROKEN);
}

TEST_CASE("testjudge: a declared crash", "[testjudge]")
{
    // the original crashes after the first check: the checks behind it are
    // not run and the case is reproduced
    fixture f(matrix, "[deviations.\"libs=cd\"]\n\"end.case\" = \"not exists\"\n\"end.run\" = \"not exists\"\n"
                      "rc = \"exists\"\nsession = \"== timeout\"\n");
    std::string m;
    obs crash(head + "check|3|ok|$00000010|$00000010\n", 0);
    crash.o.session = "timeout";

    CHECK(verdict(f, crash, crash, m) == TJ_V_REPRODUCED);
    // a cell that runs to its end passes plainly: not reproduced
    CHECK(verdict(f, crash, obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "the case passes without the declared deviations"));
    // fixed: the faulty cell crashes, the other passes
    fixture g(matrix, "fixed-orig = 7\n[deviations.\"libs=cd\"]\n\"end.case\" = \"not exists\"\n"
                      "\"end.run\" = \"not exists\"\n");
    CHECK(verdict(g, crash, obs(good), m) == TJ_V_PASSED);
    // a check that fails before the declared crash still fails
    obs bad(head + "check|3|fail|$00000010|$00000000\n");
    CHECK(verdict(g, bad, obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "check frames_maximum width (x.test.bas:9) failed"));
}

// ------------------------------------------------------------ typed checks

static const char typed_source[] =
    "!TestCase{t}\n"                                        // 1
    "  !CheckQEq{q,1.5,\"q\"}\n"                            // 2 K0
    "  !CheckFltNear{f,2.5,0.01,\"f\"}\n"                   // 3 K1
    "!EndTestCase\n";

static const std::string thead = "begin|1\nblock|case|t\n";

TEST_CASE("testjudge: quick and float checks", "[testjudge]")
{
    fixture f("", "", "t", typed_source);
    std::string m;

    CHECK(judge(f, obs(thead + "checkq|0|ok|$00018000|$00018000\ncheckflt|1|ok|$A0000042|$A0000042|$A3D70A3A\n"
                       + tail), m) == TJ_PASS);
    // the report shows the quick and the FFP values in decimal, the float's delta too
    CHECK(judge(f, obs(thead + "checkq|0|fail|$00018000|$FFFFC000\n" + tail), m) == TJ_FAIL);
    CHECK(has(m, "check t q (x.test.bas:2) failed: q: expected 1.5 ($00018000), actual -0.25 ($FFFFC000)"));
    CHECK(judge(f, obs(thead + "checkflt|1|fail|$A0000042|$C00000C1|$A3D70A3A\n" + tail), m) == TJ_FAIL);
    CHECK(has(m, "check t f (x.test.bas:3) failed: f: expected 2.5 ($A0000042), actual -1.5 ($C00000C1), "
                 "delta 0.01 ($A3D70A3A)"));
    CHECK(judge(f, obs(thead + "checkflt|1|fail|$A0000042|$A6666642|$A3D70A3A\n" + tail), m) == TJ_FAIL);
    CHECK(has(m, "actual 2.6 ($A6666642)"));
    CHECK(judge(f, obs(thead + "checkflt|1|fail|$A0000042|$00000000|$A3D70A3A\n" + tail), m) == TJ_FAIL);
    CHECK(has(m, "actual 0 ($00000000)"));
    // the entry kind must fit the macro
    CHECK(judge(f, obs(thead + "check|0|ok|$00018000|$00018000\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "line 3 is no entry of check 0 (!CheckQEq, x.test.bas:2)"));
    // queries of the verdict and the values (their bits)
    fixture g("[asserts]\n\"check.t.q\" = \"== fail\"\n\"check.t.q.actual\" = \"== $FFFFC000\"\n"
              "\"check.t.f.expected\" = \"== $A0000042\"\n",
              "", "t", typed_source);
    CHECK(judge(g, obs(thead + "checkq|0|fail|$00018000|$FFFFC000\ncheckflt|1|ok|$A0000042|$A0000042|$A3D70A3A\n"
                       + tail), m) == TJ_PASS);
    CHECK(judge(g, obs(thead + "checkq|0|fail|$00018000|$00010000\ncheckflt|1|ok|$A0000042|$A0000042|$A3D70A3A\n"
                       + tail), m) == TJ_FAIL);
    CHECK(has(m, "assertion check.t.q.actual == $FFFFC000"));
    CHECK(has(m, "does not hold: 1 ($00010000)"));
    fixture h("[asserts]\n\"check.t.f.actual\" = \"contains \\\"2\\\"\"\n", "", "t", typed_source);
    CHECK(judge(h, obs(thead + "checkflt|1|ok|$A0000042|$A0000042|$A3D70A3A\n" + tail), m) == TJ_ERROR);
    CHECK(has(m, "the value of this check is a long, compare it with an integer"));
}
