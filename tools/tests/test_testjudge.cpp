// test_testjudge - the judge of the Blitz tests of the model (tools/lib/
// testjudge) on recorded result files and observations from memory: the
// verdicts of checks with several entries, preconditions, the end entries
// of the case and the run, announced runtime errors against the requesters,
// the default assertions and their replacement, the queries of the other
// observations, declared deviations with validators, the checks behind a
// declared crash, and the verdict of a case over its cells - each rule
// with broken input (a malformed result file among them).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "basictest.h"
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

// a checkstr line (with its LF) of check K: the whole strings exp and act
// with their lengths and CRC32s, written as their first ecut and acut
// bytes (all if npos); `cut` if one is shorter
static std::string cs(uint32_t k, bool ok, const std::string &exp, const std::string &act,
                      size_t ecut = std::string::npos, size_t acut = std::string::npos)
{
    basictest_entry e = {};
    std::vector<uint8_t> buf(2048);

    e.kind = BASICTEST_CHECKSTR;
    e.k = k;
    e.ok = ok;
    e.expected_str = { reinterpret_cast<const uint8_t *>(exp.data()), std::min(ecut, exp.size()) };
    e.expected_length = uint32_t(exp.size());
    e.expected_crc = basictest_crc32(reinterpret_cast<const uint8_t *>(exp.data()), exp.size());
    e.actual_str = { reinterpret_cast<const uint8_t *>(act.data()), std::min(acut, act.size()) };
    e.actual_length = uint32_t(act.size());
    e.actual_crc = basictest_crc32(reinterpret_cast<const uint8_t *>(act.data()), act.size());
    e.cut = e.expected_str.length < exp.size() || e.actual_str.length < act.size();
    size_t n = basictest_write(&e, buf.data(), buf.size());
    if (!n)                     // not at static initialisation: a REQUIRE would abort
        return "(the line does not fit)\n";
    return std::string(reinterpret_cast<const char *>(buf.data()), n);
}

static const std::string head = "begin|1\nblock|init\nblockend\nblock|case|frames_maximum\n";
static const std::string tail = "blockend\nend\n";
// a run of frames_maximum where every check passes (K7 announces nothing
// since the run takes the other way; K8 is not run)
static const std::string good = head
                                + "check|3|ok|$00000010|$00000010\n"
                                  "check|4|ok|$FFFFFFFF|$FFFFFFFF\n"
                                  "check|5|ok|$00000001|$00000001\n"
                                  "check|5|ok|$00000001|$00000001\n"
                                + cs(6, true, "ab", "ab") + tail;

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
    obs s(head + cs(6, false, "ab", "a\tb") + tail);
    CHECK(judge(f, s, m) == TJ_FAIL);
    CHECK(has(m, "check frames_maximum str (x.test.bas:12) failed: a$: expected \"ab\", actual \"a\\x09b\""));
    // values the library shortened to fit the line
    obs cut(head + cs(6, false, "abcde", "abdef", 3, 3) + tail);
    CHECK(judge(f, cut, m) == TJ_FAIL);
    CHECK(has(m, "a$: expected \"abc\" (cut, length 5), actual \"abd\" (cut, length 5)"));
    // one value cut, the other whole
    obs one(head + cs(6, false, "abcde", "xy", 3) + tail);
    CHECK(judge(f, one, m) == TJ_FAIL);
    CHECK(has(m, "a$: expected \"abc\" (cut, length 5), actual \"xy\""));
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
        CHECK(judge(f, obs(head + cs(6, false, "ab", "xb") + tail), m) == TJ_PASS);
        CHECK(judge(f, obs(head + cs(6, false, "ab", "xy") + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: \"xy\""));
        // contains on a cut value is a configuration error naming the assertion
        m.clear();
        CHECK(judge(f, obs(head + cs(6, false, "ab", "xbz", 2, 2) + tail), m) == TJ_ERROR);
        CHECK(has(m, "tests.toml:4: check.frames_maximum.str.actual: the value was cut to fit the line "
                     "(length 3); compare it with == or !="));
        // also when the other value of the entry is the cut one: the actual
        // value is whole, so contains holds
        CHECK(judge(f, obs(head + cs(6, false, "abc", "xb", 1) + tail), m) == TJ_PASS);
        fixture n("[asserts]\n\"check.frames_maximum.str.actual\" = \"matches \\\"x*\\\"\"\n");
        CHECK(judge(n, obs(head + cs(6, false, "ab", "xbz", 2, 2) + tail), m) == TJ_ERROR);
        CHECK(judge(n, obs(head + cs(6, false, "ab", "xbz") + tail), m) == TJ_PASS);
    }
    {
        // == and != by the length and CRC32 of the whole value
        fixture g("[asserts]\n\"check.frames_maximum.str.actual\" = \"== \\\"xyz\\\"\"\n");
        CHECK(judge(g, obs(head + cs(6, false, "ab", "xyz") + tail), m) == TJ_PASS);
        CHECK(judge(g, obs(head + cs(6, false, "ab", "xyz", 2, 2) + tail), m) == TJ_PASS);
        m.clear();
        CHECK(judge(g, obs(head + cs(6, false, "ab", "xyw", 2, 2) + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: \"xy\" (cut, length 3)"));
        CHECK(judge(g, obs(head + cs(6, false, "ab", "xy") + tail), m) == TJ_FAIL);
        CHECK(judge(g, obs(head + cs(6, false, "ab", "xyzz", 3, 3) + tail), m) == TJ_FAIL);
        // broken input: a cut value whose CRC32 is not that of the string
        // (the spec's scenario) does not pass ==
        m.clear();
        CHECK(judge(g, obs(head + "checkstr|6|fail|ab|2|$9E83486D|xy|3|$00000000|cut\n" + tail), m) == TJ_FAIL);
        CHECK(has(m, "does not hold: \"xy\" (cut, length 3)"));
        // the right CRC32 of "xyz" in the same line passes
        char right[120];
        snprintf(right, sizeof right, "checkstr|6|fail|ab|2|$9E83486D|xy|3|$%08X|cut\n",
                 unsigned(basictest_crc32(reinterpret_cast<const uint8_t *>("xyz"), 3)));
        CHECK(judge(g, obs(head + right + tail), m) == TJ_PASS);
        // a whole value with a wrong CRC32: the reader refuses the file
        m.clear();
        CHECK(judge(g, obs(head + "checkstr|6|fail|ab|2|$9E83486D|xyz|3|$00000000|-\n" + tail), m) == TJ_ERROR);
        CHECK(has(m, "line 5: checkstr: malformed value in field 9"));
        // !=
        fixture h("[asserts]\n\"check.frames_maximum.str.actual\" = \"!= \\\"xyz\\\"\"\n");
        CHECK(judge(h, obs(head + cs(6, false, "ab", "xyz", 2, 2) + tail), m) == TJ_FAIL);
        CHECK(judge(h, obs(head + cs(6, false, "ab", "xyw", 2, 2) + tail), m) == TJ_PASS);
        CHECK(judge(h, obs(head + cs(6, false, "ab", "xy") + tail), m) == TJ_PASS);
        // the expected value, and the empty string
        fixture e("[asserts]\n\"check.frames_maximum.str.expected\" = \"== \\\"\\\"\"\n");
        CHECK(judge(e, obs(head + cs(6, false, "", "xy") + tail), m) == TJ_PASS);
        CHECK(judge(e, obs(head + cs(6, false, "a", "xy") + tail), m) == TJ_FAIL);
        // a string of 400 bytes against a value cut to its first 100
        std::string x400(400, 'x'), y400 = x400;
        y400.back() = 'y';
        fixture l("[asserts]\n\"check.frames_maximum.str.actual\" = \"== \\\"" + x400 + "\\\"\"\n");
        CHECK(judge(l, obs(head + cs(6, false, "ab", x400, std::string::npos, 100) + tail), m) == TJ_PASS);
        m.clear();
        CHECK(judge(l, obs(head + cs(6, false, "ab", y400, std::string::npos, 100) + tail), m) == TJ_FAIL);
        CHECK(has(m, "(cut, length 400)"));
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

TEST_CASE("testjudge: inject's report of the compiler", "[testjudge]")
{
    std::string m;
    fixture f("[asserts]\n\"compile.inject\" = \"contains \\\"inject: CreateProc Debug: caches flushed before\\\"\"\n");
    obs o(good);

    // captured: the inject lines of the compiler's output
    o.o.compile_inject = "inject: rc=0\ninject: CreateProc Debug: caches flushed before\n";
    CHECK(judge(f, o, m) == TJ_PASS);
    o.o.compile_inject = "inject: rc=0\ninject: CreateProc Debug: caches not flushed before\n";
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "assertion compile.inject contains \"inject: CreateProc Debug: caches flushed before\" ("
                     + (f.root / "tests.toml").string() + ":3) does not hold: \"inject: rc=0\n"));
    // a compiler that does not run through inject: not captured
    o.o.compile_inject = nullptr;
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "assertion compile.inject contains") && has(m, "does not hold: not captured"));
    // the query of the program's report is another one
    o.o.inject = "inject: CreateProc Debug: caches flushed before\n";
    CHECK(judge(f, o, m) == TJ_FAIL);
}

TEST_CASE("testjudge: a run from the editor", "[testjudge]")
{
    std::string m;
    char er[300];
    fixture f("editor-run = \"on\"\n[asserts]\noutput = \"== \\\"start\\n\\\"\"\n");
    obs o(good);

    // no exit code, no requesters: their defaults do not apply
    o.o.has_rc = 0;
    o.o.editor_run = 1;
    o.o.output = "start\n";
    CHECK(judge(f, o, m) == TJ_PASS);
    CHECK(m == "");
    o.o.output = "start\nmore\n";
    CHECK(judge(f, o, m) == TJ_FAIL);
    CHECK(has(m, "assertion output == \"start\n\""));
    // the end entries stay
    o.o.output = "start\n";
    obs crashed(head + "check|3|ok|$00000010|$00000010\n");
    crashed.o.has_rc = 0;
    crashed.o.editor_run = 1;
    crashed.o.output = "start\n";
    CHECK(judge(f, crashed, m) == TJ_FAIL);
    CHECK(has(m, "assertion end.case exists ((default):0)"));
    // an announced runtime error is refused (the source's !ExpectError, line 13)
    CHECK(tj_editor_run_refused("x.test.bas", &f.src, &f.conf, &f.m, er, sizeof er) == 1);
    CHECK(has(er, "x.test.bas:13: !ExpectError in a test whose cell default runs from the editor (editor-run = "
                  "\"on\"): a run from the editor captures no requester"));
    fixture g("");
    CHECK(tj_editor_run_refused("x.test.bas", &g.src, &g.conf, &g.m, er, sizeof er) == 0);
    // only the cells that run from the editor
    fixture h("[matrix]\neditor-run = [\"off\"]\n");
    CHECK(tj_editor_run_refused("x.test.bas", &h.src, &h.conf, &h.m, er, sizeof er) == 0);
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
    CHECK(judge(f, obs(head + cs(3, true, "a", "a") + tail), m) == TJ_ERROR);
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

// a bug-orig test (require_deviation) must declare a deviation, else it is
// broken: open or fixed, whatever its cells show (bug-orig-basic-tests 1.2)
TEST_CASE("testjudge: a bug-orig test without a declared deviation", "[testjudge]")
{
    fixture f(matrix);
    std::string m;

    f.t.require_deviation = 1;
    CHECK(verdict(f, obs(good), obs(good), m) == TJ_V_BROKEN);
    CHECK(has(m, "x.test.bas: the test declares no deviation of the original, so it does not show its bug"));
    CHECK(verdict(f, obs(good), obs(width_fails), m) == TJ_V_BROKEN);
    // a cell that cannot be judged stays an error
    CHECK(verdict(f, obs("x\n"), obs(good), m) == TJ_V_ERROR);
    // with a deviation the rule does not apply
    fixture d(matrix, declared);
    d.t.require_deviation = 1;
    CHECK(verdict(d, obs(width_fails), obs(width_fails), m) == TJ_V_REPRODUCED);
    fixture x(matrix, "fixed-orig = 265\n" + declared);
    x.t.require_deviation = 1;
    CHECK(verdict(x, obs(width_fails), obs(good), m) == TJ_V_PASSED);
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

TEST_CASE("testjudge: the test of an extension", "[testjudge]")
{
    // judged like a fixed original bug: the deviations are what the original does instead
    fixture f(matrix, "extension = 1166\n" + declared);
    std::string m;

    CHECK(verdict(f, obs(width_fails), obs(good), m) == TJ_V_PASSED);
    // the port loses the extension: a cell without the original's part fails
    CHECK(verdict(f, obs(width_fails), obs(width_fails), m) == TJ_V_FAILED);
    CHECK(has(m, "case frames_maximum, cell clib:\n  check frames_maximum width (x.test.bas:9) failed"));
    // the original's cell no longer shows what it does instead
    CHECK(verdict(f, obs(good), obs(good), m) == TJ_V_FAILED);
    CHECK(has(m, "cell cd (the original's faulty part, its declared deviations):"));
    // a bug-orig test with an extension declares its deviation: not broken
    f.t.require_deviation = 1;
    CHECK(verdict(f, obs(width_fails), obs(good), m) == TJ_V_PASSED);
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
                 "deviations, but the test has no fixed issue (fixed-orig, or extension)"));
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

// ------------------------------------------------------------ calls out of order

TEST_CASE("testjudge: calls out of order", "[testjudge]")
{
    fixture f("", "", "t", typed_source);
    std::string m;

    // the library reports a check outside a block, then writes the check
    CHECK(judge(f, obs(thead + "blockend\nbroken|check outside a block\ncheckq|0|ok|$00018000|$00018000\nend\n"), m)
          == TJ_BROKEN);
    CHECK(has(m, "the test library reports its state broken (line 4): check outside a block"));
    CHECK(judge(f, obs(thead + "blockend\nbroken|block end without a block\nblockend\nend\n"), m) == TJ_BROKEN);
    // without the broken entry such a line is a malformed file
    CHECK(judge(f, obs(thead + "blockend\ncheckq|0|ok|$00018000|$00018000\nend\n"), m) == TJ_ERROR);
    // a failed precondition of the batch init repeated in the case
    fixture i("", "", "frames_maximum");
    CHECK(judge(i, obs("begin|1\nblock|init\nrequire|1|broken\nblockend\nblock|case|frames_maximum\n"
                       "require|1|broken\nblockend\nend\n"), m) == TJ_BROKEN);
    // a failed check of the batch init repeated in the case
    CHECK(judge(i, obs("begin|1\nblock|init\ncheck|2|fail|$00000001|$00000002\nblockend\nblock|case|frames_maximum\n"
                       "check|2|fail|$00000001|$00000002\nblockend\nend\n"), m) == TJ_FAIL);
    CHECK(has(m, "check init initcheck (x.test.bas:6) failed: 1: expected 1 ($00000001), actual 2 ($00000002) "
                 "(its first failed entry)"));
}

// two cases and a batch cleanup, run by one driver (the run of all cases of
// a source, corpusrun case without --case)
static const char two_cases[] =
    "!TestCase{a}\n"                                        // 1
    "  !CheckEq{1,1,\"one\"}\n"                             // 2  K0
    "!EndTestCase\n"                                        // 3
    "!TestCase{b}\n"                                        // 4
    "  !CheckEq{2,2,\"two\"}\n"                             // 5  K1
    "!EndTestCase\n"                                        // 6
    "!TestBatchCleanup\n"                                   // 7
    "  !CheckEq{3,3,\"fin\"}\n"                             // 8  K2
    "!EndTestBatchCleanup\n";                               // 9

TEST_CASE("testjudge: the blocks of the other cases the driver called", "[testjudge]")
{
    static const char *const ab[] = { "a", "b" };
    static const char *const a_only[] = { "a" };
    fixture fa("", "", "a", two_cases), fb("", "", "b", two_cases);
    std::string m;
    const std::string pre = "begin|1\nblock|case|a\n", mid = "blockend\nblock|case|b\n";
    const std::string fin = "blockend\nblock|final\ncheck|2|ok|$00000003|$00000003\nblockend\nend\n";

    fa.t.cases = ab;
    fa.t.ncases = 2;
    fb.t.cases = ab;
    fb.t.ncases = 2;
    // two cases in one result file, each judged from its own block
    obs both(pre + "check|0|ok|$00000001|$00000001\n" + mid + "check|1|ok|$00000002|$00000002\n" + fin);
    CHECK(judge(fa, both, m) == TJ_PASS);
    CHECK(m == "");
    CHECK(judge(fb, both, m) == TJ_PASS);
    CHECK(m == "");
    // a failed check in b does not fail a
    obs bfail(pre + "check|0|ok|$00000001|$00000001\n" + mid + "check|1|fail|$00000002|$00000005\n" + fin);
    CHECK(judge(fa, bfail, m) == TJ_PASS);
    CHECK(m == "");
    CHECK(judge(fb, bfail, m) == TJ_FAIL);
    CHECK(has(m, "check b two (x.test.bas:5) failed"));
    // nor does the library's broken state reported in b's block
    obs bbroken(pre + "check|0|ok|$00000001|$00000001\n" + mid + "broken|block end without a block\n" + fin);
    CHECK(judge(fa, bbroken, m) == TJ_PASS);
    CHECK(judge(fb, bbroken, m) == TJ_BROKEN);
    // a failed check of the batch cleanup fails both
    obs cfail(pre + "check|0|ok|$00000001|$00000001\n" + mid + "check|1|ok|$00000002|$00000002\n"
              "blockend\nblock|final\ncheck|2|fail|$00000003|$00000004\nblockend\nend\n");
    CHECK(judge(fa, cfail, m) == TJ_FAIL);
    CHECK(has(m, "check cleanup fin (x.test.bas:8) failed"));
    CHECK(judge(fb, cfail, m) == TJ_FAIL);
    CHECK(has(m, "check cleanup fin (x.test.bas:8) failed"));
    // the block of a case the driver did not call still fails the cell
    fa.t.cases = a_only;
    fa.t.ncases = 1;
    CHECK(judge(fa, both, m) == TJ_FAIL);
    CHECK(has(m, "the result file holds the case b (line 5), which was not selected"));
    fa.t.cases = nullptr;
    fa.t.ncases = 0;
    CHECK(judge(fa, both, m) == TJ_FAIL);
    CHECK(has(m, "the result file holds the case b (line 5), which was not selected"));
    // the judged case twice stays a fault of the file, called or not
    fa.t.cases = ab;
    fa.t.ncases = 2;
    obs twice(pre + "check|0|ok|$00000001|$00000001\nblockend\nblock|case|a\n" + fin);
    CHECK(judge(fa, twice, m) == TJ_FAIL);
    CHECK(has(m, "the result file holds the case a twice"));
}

// an original that returns or hangs depending on the memory: the two
// alternative ends of the cells with the CD's libraries
static const std::string ends_cd = "[ends.\"libs=cd\"]\n"
                                   "reason = \"the memory behind the table\"\n"
                                   "returns = [\"session == done\", \"rc == 0\", \"end.case exists\", "
                                   "\"end.run exists\"]\n"
                                   "hangs = [\"session == timeout\", \"rc not exists\", \"end.case not exists\", "
                                   "\"end.run not exists\", \"check.frames_maximum.width == fail\"]\n";

// a run that hangs after a failed check of width: no end entries, no exit code
static obs hang_obs(const char *session = "timeout")
{
    obs o(head + "check|3|fail|$00000010|$00000000\n");

    o.o.has_rc = 0;
    o.o.session = session;
    return o;
}

TEST_CASE("testjudge: alternative ends", "[testjudge]")
{
    std::string m;

    {
        // an open bug: both cells hang, the alternative hangs holds (and names
        // the failed check, which leaves the default of the checks)
        fixture f(matrix, ends_cd);
        CHECK(tj_case_has_deviation(&f.t));
        CHECK(verdict(f, hang_obs(), hang_obs(), m) == TJ_V_REPRODUCED);
        CHECK(has(m, "case frames_maximum, cell cd (an open bug: its declared deviations):\n  the end of "
                     "[ends.\"libs=cd\"] (" + (f.root / "x.test.toml").string() + ":2): the alternative hangs holds"));
        // a cell where the original returns: the other alternative
        CHECK(verdict(f, obs(good), hang_obs(), m) == TJ_V_REPRODUCED);
        CHECK(has(m, "cell cd (an open bug: its declared deviations):\n  the end of [ends.\"libs=cd\"] ("
                     + (f.root / "x.test.toml").string() + ":2): the alternative returns holds"));
        // the session halts: no alternative holds, each is named with its
        // failed assertion
        CHECK(verdict(f, hang_obs("halt"), hang_obs(), m) == TJ_V_FAILED);
        CHECK(has(m, "no alternative end of [ends.\"libs=cd\"] (" + (f.root / "x.test.toml").string()
                         + ":2) holds:\n    the alternative returns:\n      assertion session == done ("));
        CHECK(has(m, "    the alternative hangs:\n      assertion session == timeout ("));
        // a bug-orig test with only alternative ends declares its deviation
        f.t.require_deviation = 1;
        CHECK(verdict(f, hang_obs(), hang_obs(), m) == TJ_V_REPRODUCED);
    }
    {
        // a fixed bug: the original returns in the cell with the faulty part,
        // the port's cell is judged plain
        fixture f(matrix, "fixed-orig = 82\n" + ends_cd);
        CHECK(verdict(f, obs(good), obs(good), m) == TJ_V_PASSED);
        CHECK(has(m, "the alternative returns holds"));
        CHECK(verdict(f, hang_obs(), obs(good), m) == TJ_V_PASSED);
        CHECK(has(m, "the alternative hangs holds"));
        // the port hangs: no alternative there, the defaults fail
        CHECK(verdict(f, obs(good), hang_obs(), m) == TJ_V_FAILED);
        CHECK(has(m, "case frames_maximum, cell clib:\n"));
        CHECK(has(m, "assertion end.run exists ((default):0) does not hold: absent"));
        CHECK(m.find("alternative", m.find("cell clib:")) == std::string::npos);
    }
    {
        // the queries the alternatives name leave the other assertions: an
        // assertion of rc in the directory's file does not apply in cd
        fixture f(std::string(matrix) + "[asserts]\nrc = \"== 7\"\n", "fixed-orig = 82\n" + ends_cd);
        CHECK(verdict(f, hang_obs(), obs(good, 7), m) == TJ_V_PASSED);
    }
}

// a named announcement, judged like a check (section 6 of
// basic-test-runner-extension): the port shows the announced error, the
// original another one, which a deviation declares
static const char named_error[] =
    "!TestCase{a}\n"                                        // 1
    "  !CheckEq{1,1,\"before\"}\n"                          // 2  K0
    "  !ExpectError{\"Illegal Shape Depth\",\"refused\"}\n" // 3  K1
    "  !Check{0,\"not_refused\"}\n"                         // 4  K2
    "!EndTestCase\n";                                       // 5

TEST_CASE("testjudge: an announcement judged like a check", "[testjudge]")
{
    const std::string ann = "begin|1\nblock|case|a\ncheck|0|ok|$00000001|$00000001\nexpect-error|1\nincomplete\nend\n";
    const std::string orig_dev = "fixed-orig = 1\n[deviations.\"libs=cd\"]\n\"check.a.refused\" = \"== fail\"\n"
                                 "requester = \"contains \\\"Can't Read File\\\"\"\n\"end.case\" = \"not exists\"\n"
                                 "rc = \"exists\"\n";
    std::string m;

    {
        // the port's cell with the announced error, the original's with the
        // declared one: both pass
        fixture f(matrix, orig_dev, "a", named_error);
        CHECK(verdict(f, obs(ann, 20).requester("Can't Read File"), obs(ann, 20).requester("Illegal Shape Depth"), m)
              == TJ_V_PASSED);
        // the original raises the announced error after all: the deviation
        // wants it failed
        CHECK(verdict(f, obs(ann, 20).requester("Illegal Shape Depth"), obs(ann, 20).requester("Illegal Shape Depth"),
                      m)
              == TJ_V_FAILED);
        CHECK(has(m, "assertion check.a.refused == fail (" + (f.root / "x.test.toml").string()
                         + ":3, a declared deviation) does not hold: ok"));
        // the port without its error: the announcement fails as before
        CHECK(verdict(f, obs(ann, 20).requester("Can't Read File"), obs(ann, 20), m) == TJ_V_FAILED);
        CHECK(has(m, "case a, cell clib:\n"));
        CHECK(has(m, "  the announced runtime error \"Illegal Shape Depth\" (x.test.bas:3) did not "
                     "come"));
    }
    {
        // the deviation removed: the original's cell fails naming the announcement
        fixture f(matrix, "", "a", named_error);
        CHECK(verdict(f, obs(ann, 20).requester("Can't Read File"), obs(ann, 20).requester("Illegal Shape Depth"), m)
              == TJ_V_FAILED);
        CHECK(has(m, "case a, cell cd:\n"));
        CHECK(has(m, "  the announced runtime error \"Illegal Shape Depth\" (x.test.bas:3) did not "
                     "come"));
        CHECK(has(m, "requester count == 0 ((default):0) does not hold: 1, the first \"Can't Read File\""));
    }
    {
        // the verdicts of the query: ok, fail, not-run
        fixture ok("[asserts]\n\"check.a.refused\" = \"== ok\"\n", "", "a", named_error);
        CHECK(judge(ok, obs(ann, 20).requester("Illegal Shape Depth"), m) == TJ_PASS);
        CHECK(judge(ok, obs(ann, 20), m) == TJ_FAIL);
        CHECK(has(m, "the announced runtime error \"Illegal Shape Depth\" (x.test.bas:3) did not come"));
        CHECK(has(m, "assertion check.a.refused == ok"));
        fixture nr("[asserts]\n\"check.a.refused\" = \"== not-run\"\n\"end.case\" = \"not exists\"\n", "", "a",
                   named_error);
        CHECK(judge(nr, obs("begin|1\nblock|case|a\ncheck|0|ok|$00000001|$00000001\nincomplete\nend\n"), m)
              == TJ_PASS);
        // an announcement has no values
        fixture v("[asserts]\n\"check.a.refused.actual\" = \"== 1\"\n", "", "a", named_error);
        CHECK(judge(v, obs(ann, 20).requester("Illegal Shape Depth"), m) == TJ_ERROR);
        CHECK(has(m, "this check has no values"));
    }
}

// the compile of a data source (change basic-test-local-data, D5): data.FILE,
// by default == ok; an unexpected result makes the cell broken
TEST_CASE("testjudge: the compile of a data source", "[testjudge]")
{
    const std::string own = "[data.\"child.bas\"]\naction = \"basic-compile\"\n[data]\n\"in.txt\" = \"copy\"\n";
    std::string m;
    tj_data ok[] = { { "child.bas", "ok" } };
    tj_data bad[] = { { "child.bas", "error 3 Syntax Error" } };
    tj_data none[] = { { "child.bas", "none" } };

    {
        fixture f("", own);
        obs o(good);

        o.o.data = ok;
        o.o.ndata = 1;
        CHECK(judge(f, o, m) == TJ_PASS);
        CHECK(m == "");
        // an error without an assertion: broken, naming the data file and its line
        o.o.data = bad;
        CHECK(judge(f, o, m) == TJ_BROKEN);
        CHECK(has(m, "the data source x.test/child.bas does not compile"));
        CHECK(has(m, "x.test/child.bas:3: Syntax Error"));
        o.o.data = none;
        CHECK(judge(f, o, m) == TJ_BROKEN);
        CHECK(has(m, "its compile gave none, not ok"));
        // not captured at all
        o.o.data = nullptr;
        o.o.ndata = 0;
        CHECK(judge(f, o, m) == TJ_BROKEN);
        CHECK(has(m, "nothing (not captured)"));
        // a failed check stays a failure besides
        obs o2(head + "check|3|fail|$00000010|$00000000\n" + tail);
        o2.o.data = ok;
        o2.o.ndata = 1;
        CHECK(judge(f, o2, m) == TJ_FAIL);
    }
    {
        // a declared compile error of the original: the cells with it run
        // without the file; the others still need ok
        fixture f("", "fixed-orig = 7\n" + own + "[deviations.\"libs=cd\"]\n\"data.child.bas\" = \"contains \\\"Syntax Error\\\"\"\n"
                  "[matrix]\nlibs = [\"cd\", \"clib\"]\n");
        obs o(good);

        o.o.data = bad;
        o.o.ndata = 1;
        CHECK(judge(f, o, m, "cd", TC_DEVIATED) == TJ_PASS);
        CHECK(judge(f, o, m, "clib", TC_DEVIATED) == TJ_BROKEN);
        CHECK(m.find("assertion data.child.bas") == std::string::npos);
        o.o.data = ok;
        CHECK(judge(f, o, m, "cd", TC_DEVIATED) == TJ_BROKEN);
        CHECK(has(m, "assertion data.child.bas contains \"Syntax Error\""));
    }
}

// the queries of a started program (change basic-test-local-data):
// run.NS.NAME, .rc, .output; the default: every program that ran returned
TEST_CASE("testjudge: a started program", "[testjudge]")
{
    static const char runsrc[] = "!TestCase{ctrlc}\n"                                      // 1
                                 "  *r.TLRun=!RunProgram{\"CORPUS:x.test/child\",\"edit\"}\n"   // 2  K0
                                 "  !CheckEq{*r\\rc,20,\"rc\"}\n"                          // 3  K1
                                 "  !RunProgram{\"Echo x\"}\n"                              // 4  K2
                                 "!EndTestCase\n";
    const std::string hd = "begin|1\nblock|case|ctrlc\n";
    const std::string done = hd + "run|0\nrunend|0|$00000014\ncheck|1|ok|$00000014|$00000014\nrun|2\nrunend|2|$00000000\n"
                             "blockend\nend\n";
    std::string m;
    tj_runout outs[] = { { 0, "***Break\n" }, { 2, "x\n" } };

    {
        fixture f("", "", "ctrlc", runsrc);
        obs o(done);

        o.o.runs = outs;
        o.o.nruns = 2;
        // returned: passes by the defaults, the return code has none
        CHECK(judge(f, o, m) == TJ_PASS);
        CHECK(m == "");
        // open: the run without its runend, the session ended
        obs open(hd + "run|0\n", -1);
        open.o.has_rc = 0;
        open.o.session = "timeout";
        open.o.runs = outs;
        open.o.nruns = 1;
        CHECK(judge(f, open, m) == TJ_FAIL);
        CHECK(has(m, "the program !RunProgram \"edit\" (x.test.bas:2) did not return (no runend 0)"));
    }
    {
        // the queries: open with its output kept, declared
        fixture f("", "[asserts]\n\"run.ctrlc.edit\" = \"== open\"\n\"run.ctrlc.edit.output\" = \"contains \\\"Break\\\"\"\n"
                  "session = \"== timeout\"\n\"end.case\" = \"not exists\"\n\"end.run\" = \"not exists\"\n"
                  "rc = \"not exists\"\n",
                  "ctrlc", runsrc);
        obs open(hd + "run|0\n");
        open.o.has_rc = 0;
        open.o.session = "timeout";
        open.o.runs = outs;
        open.o.nruns = 1;
        CHECK(judge(f, open, m) == TJ_PASS);
        CHECK(m == "");
        // the same declared on a run that returned: fails, naming it
        obs o(done);
        o.o.runs = outs;
        o.o.nruns = 2;
        CHECK(judge(f, o, m) == TJ_FAIL);
        CHECK(has(m, "assertion run.ctrlc.edit == open"));
        CHECK(has(m, "does not hold: done"));
    }
    {
        // rc 20 asserted in the toml
        fixture f("", "[asserts]\n\"run.ctrlc.edit.rc\" = \"== 20\"\n", "ctrlc", runsrc);
        obs o(done);

        CHECK(judge(f, o, m) == TJ_PASS);
        obs o2(hd + "run|0\nrunend|0|$00000000\ncheck|1|ok|$00000014|$00000014\nblockend\nend\n");
        CHECK(judge(f, o2, m) == TJ_FAIL);
        CHECK(has(m, "assertion run.ctrlc.edit.rc == 20"));
        // the system gave no return code: cannot be judged
        obs o3(hd + "run|0\nrunend|0|-\ncheck|1|ok|$00000014|$00000014\nblockend\nend\n");
        CHECK(judge(f, o3, m) == TJ_ERROR);
        CHECK(has(m, "the system gave no return code (runend 0 -), it cannot be judged"));
    }
    {
        // not-run: the case did not reach it
        fixture f("", "[asserts]\n\"run.ctrlc.edit\" = \"== not-run\"\n", "ctrlc", runsrc);
        obs o(hd + "blockend\nend\n");

        CHECK(judge(f, o, m) == TJ_PASS);
    }
    {
        // a run entry of a number that is no !RunProgram
        fixture f("", "", "ctrlc", runsrc);
        obs o(hd + "run|1\nblockend\nend\n");

        CHECK(judge(f, o, m) == TJ_ERROR);
        CHECK(has(m, "line 3 names the run 1, which is no !RunProgram of the source"));
    }
}
