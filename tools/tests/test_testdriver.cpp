// test_testdriver - the driver of a run of a Blitz test of the model
// (tools/lib/testpp, testdriver.c): the order of the spec
// testing/basic-test-interface (macro text, TLBegin with the result file,
// the jump over the copied text, the batch init only if a case is
// selected, the selected cases in source order, the batch cleanup, End), a
// selected case the source does not have, the map of the driver's lines
// to the source's lines and calls - and the refusals (deliberately broken
// input: a result name that cannot stand in a string literal, a case name
// that is none).
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testdriver.h"
}

static const char macros[] = "Macro TestCase\n  .tc_`1\nEnd Macro\n";

struct drv {
    tp_source s = {};
    tp_driver d = {};
    int rc = -3;
    char err[300] = "";

    drv(const std::string &text, std::vector<const char *> cases, const char *result = "CORPUS:c0/out/x.result",
        const char *mac = macros, int ab3 = 0)
    {
        tp_params p = {};

        p.ab3 = ab3;
        p.file = "x.test.bas";
        p.test_id = "x";
        p.testdata = "TEST:data";
        REQUIRE(tp_process(text.data(), text.size(), &p, &s) == 0);
        tp_driver_params dp = {};
        dp.macros = mac;
        dp.macros_len = strlen(mac);
        dp.result = result;
        dp.cases = cases.data();
        dp.ncases = cases.size();
        rc = tp_driver_write(&s, &dp, &d, err, sizeof err);
    }
    ~drv()
    {
        tp_driver_free(&d);
        tp_free(&s);
    }
    std::string text() const { return d.text ? std::string(d.text, d.len) : std::string(); }
};

static const std::string source = "#n=1\n"
                                  "!TestBatchInit\n"
                                  "  g=5\n"
                                  "!EndTestBatchInit\n"
                                  "!TestCase{a}\n"
                                  "  !CheckEq{g,5,\"five\"}\n"
                                  "!EndTestCase\n"
                                  "!TestCase{b}\n"
                                  "  !Check{g=5}\n"
                                  "!EndTestCase\n"
                                  "!TestBatchCleanup\n"
                                  "!EndTestBatchCleanup";   // no LF at the end

static const std::string head = std::string(macros) + "TLBegin \"CORPUS:c0/out/x.result\"\nGoto tc__skip\n";
static const std::string body = "#n=1\n"
                                "!TestBatchInit\n"
                                "  g=5\n"
                                "!EndTestBatchInit\n"
                                "!TestCase{a}\n"
                                "  !CheckEq{0,g,5}\n"
                                "!EndTestCase\n"
                                "!TestCase{b}\n"
                                "  !Check{1,g=5}\n"
                                "!EndTestCase\n"
                                "!TestBatchCleanup\n"
                                "!EndTestBatchCleanup\n";

TEST_CASE("testdriver: one case selected", "[testdriver]")
{
    drv x(source, { "b" });

    REQUIRE(x.rc == 0);
    CHECK(x.text() == head + body + ".tc__skip\nGosub tc__init\nGosub tc_b\nGosub tc__final\nEnd\n");
    CHECK(x.d.first == 6);
    CHECK(x.d.nsource == 12);
    CHECK(x.d.nlines == 5 + 12 + 5);
    // nothing of the names or messages in the driver
    CHECK(x.text().find("five") == std::string::npos);
}

TEST_CASE("testdriver: the cases in source order, not in the order given", "[testdriver]")
{
    drv x(source, { "b", "a" });

    REQUIRE(x.rc == 0);
    CHECK(x.text() == head + body + ".tc__skip\nGosub tc__init\nGosub tc_a\nGosub tc_b\nGosub tc__final\nEnd\n");
}

TEST_CASE("testdriver: no case selected calls neither the batch init nor the batch cleanup", "[testdriver]")
{
    drv x(source, {});

    REQUIRE(x.rc == 0);
    CHECK(x.text() == head + body + ".tc__skip\nEnd\n");
    CHECK(x.d.calls == 0);
    CHECK(x.d.ncalled == 0);
}

TEST_CASE("testdriver: a source without batch init and end", "[testdriver]")
{
    drv x("!TestCase{only}\n!Check{1}\n!EndTestCase\n", { "only" });

    REQUIRE(x.rc == 0);
    CHECK(x.text() == head + "!TestCase{only}\n!Check{0,1}\n!EndTestCase\n.tc__skip\nGosub tc_only\nEnd\n");
}

TEST_CASE("testdriver: a case the source does not have is called after the others", "[testdriver]")
{
    drv x(source, { "renamed", "a" });

    REQUIRE(x.rc == 0);
    CHECK(x.text()
          == head + body + ".tc__skip\nGosub tc__init\nGosub tc_a\nGosub tc_renamed\nGosub tc__final\nEnd\n");
    unsigned line = 0;
    const char *name = nullptr;
    // .tc__skip is line 18, the calls from 19 on
    CHECK(x.d.calls == 19);
    CHECK(tp_driver_where(&x.d, 21, &line, &name) == TP_IN_CALL);
    CHECK(std::string(name) == "renamed");
    CHECK(tp_driver_where(&x.d, 19, &line, &name) == TP_IN_CALL);
    CHECK(std::string(name) == "init");
    CHECK(tp_driver_where(&x.d, 22, &line, &name) == TP_IN_CALL);
    CHECK(std::string(name) == "final");
}

TEST_CASE("testdriver: the map of the driver's lines", "[testdriver]")
{
    drv x(source, { "a" });
    unsigned line = 0;
    const char *name = nullptr;

    REQUIRE(x.rc == 0);
    CHECK(tp_driver_where(&x.d, 1, &line, &name) == TP_IN_MACROS);
    CHECK(tp_driver_where(&x.d, 5, &line, &name) == TP_IN_MACROS);    // Goto tc__skip
    CHECK(tp_driver_where(&x.d, 6, &line, &name) == TP_IN_SOURCE);
    CHECK(line == 1);
    CHECK(tp_driver_where(&x.d, 11, &line, &name) == TP_IN_SOURCE);
    CHECK(line == 6);                                                  // !CheckEq{0,g,5}
    CHECK(tp_driver_where(&x.d, 17, &line, &name) == TP_IN_SOURCE);
    CHECK(line == 12);
    CHECK(tp_driver_where(&x.d, 18, &line, &name) == TP_IN_END);       // .tc__skip
    CHECK(tp_driver_where(&x.d, 22, &line, &name) == TP_IN_END);       // End
    // the line of the source is the line in the driver minus the head
    std::string t = x.text();
    size_t p = 0;
    for (unsigned i = 1; i < 11; i++)
        p = t.find('\n', p) + 1;
    CHECK(t.substr(p, t.find('\n', p) - p) == "  !CheckEq{0,g,5}");
}

TEST_CASE("testdriver: macro text without LF at its end", "[testdriver]")
{
    drv x("!TestCase{a}\n!EndTestCase\n", { "a" }, "RAM:r", "Macro M\nEnd Macro");

    REQUIRE(x.rc == 0);
    CHECK(x.text() == "Macro M\nEnd Macro\nTLBegin \"RAM:r\"\nGoto tc__skip\n!TestCase{a}\n!EndTestCase\n"
                      ".tc__skip\nGosub tc_a\nEnd\n");
    CHECK(x.d.first == 5);
}

TEST_CASE("testdriver: refused input", "[testdriver]")
{
    {
        drv x("!TestCase{a}\n!EndTestCase\n", { "a" }, "CORPUS:a\"b");

        CHECK(x.rc == -1);
        CHECK(std::string(x.err) == "the result file's name CORPUS:a\"b cannot stand in a string literal");
        CHECK(x.d.text == nullptr);
    }
    {
        drv x("!TestCase{a}\n!EndTestCase\n", { "a" }, "");

        CHECK(x.rc == -1);
        CHECK(std::string(x.err) == "the result file's name must have 1 to 100 bytes");
    }
    {
        std::string longname(101, 'x');
        drv x("!TestCase{a}\n!EndTestCase\n", { "a" }, longname.c_str());

        CHECK(x.rc == -1);
    }
    {
        drv x("!TestCase{a}\n!EndTestCase\n", { "a" }, "RAM:r\t");

        CHECK(x.rc == -1);
    }
    {
        drv x("!TestCase{a}\n!EndTestCase\n", { "A b" });

        CHECK(x.rc == -1);
        CHECK(std::string(x.err) == "A b is no case name");
    }
    {
        drv x("!TestCase{a}\n!EndTestCase\n", { "" });

        CHECK(x.rc == -1);
    }
}

// AB3 mode (change ab3-test-model D9): the header block of the source as
// the first lines of the driver, where the compilers read it
TEST_CASE("testdriver: the header block of an AB3 source", "[testdriver]")
{
    static const std::string tail = "TLBegin \"CORPUS:c0/out/x.result\"\nGoto tc__skip\n";
    static const std::string calls = ".tc__skip\nGosub tc_a\nEnd\n";
    static const std::string cases = "!TestCase{a}\n!EndTestCase\n";
    {
        // a header of three lines: first in the driver, and in the copied source
        std::string src = "; XTRA\n;  RuntimeDebug  = 1\n; /XTRA\n; the case\n" + cases;
        drv x(src, { "a" }, "CORPUS:c0/out/x.result", macros, 1);

        REQUIRE(x.rc == 0);
        CHECK(x.text() == "; XTRA\n;  RuntimeDebug  = 1\n; /XTRA\n" + std::string(macros) + tail + src + calls);
        unsigned line = 0;
        const char *name = nullptr;
        CHECK(x.d.first == 9);
        CHECK(tp_driver_where(&x.d, 9, &line, &name) == TP_IN_SOURCE);
        CHECK(line == 1);
        CHECK(tp_driver_where(&x.d, 1, &line, &name) == TP_IN_MACROS);
    }
    {
        // without /XTRA: the comment lines up to the first other line
        std::string src = "; RuntimeDebug = 0\n;\n\n" + cases;
        drv x(src, { "a" }, "CORPUS:c0/out/x.result", macros, 1);

        REQUIRE(x.rc == 0);
        CHECK(x.text() == "; RuntimeDebug = 0\n;\n" + std::string(macros) + tail + src + calls);
    }
    {
        // no header: comments without KEY = VALUE, a header after code, BB2 mode
        for (const std::string &src : { "; a comment\n; no key = here\n" + cases, "#a=1\n; RuntimeDebug = 0\n" + cases,
                                         "; Version: 1 = 2\n" + cases }) {
            drv x(src, { "a" }, "CORPUS:c0/out/x.result", macros, 1);

            INFO(src);
            REQUIRE(x.rc == 0);
            CHECK(x.text() == std::string(macros) + tail + src + calls);
        }
        std::string src = "; RuntimeDebug = 0\n" + cases;
        drv b(src, { "a" });

        REQUIRE(b.rc == 0);
        CHECK(b.text() == std::string(macros) + tail + src + calls);
    }
}
