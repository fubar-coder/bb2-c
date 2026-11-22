// test_testpp - the preprocessor of the Blitz test sources (tools/lib/
// testpp) on sources from memory: the check calls found and their
// arguments split by the compiler's rules (the cases of spike 1: strings
// with commas and braces, nested braces and parentheses, nested macros, a
// check as a macro argument), the numbers over the source and in the
// namespaces, the table, the preprocessed text, the placeholders - and each
// refusal with file and line (an unknown placeholder among them).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testpp.h"
}

struct pp {
    tp_source s = {};
    int rc;

    pp(const std::string &text, const char *testdata = "TEST:data")
    {
        tp_params p = {};

        p.file = "x.test.bas";
        p.test_id = "x";
        p.testdata = testdata;
        rc = tp_process(text.data(), text.size(), &p, &s);
    }
    ~pp() { tp_free(&s); }
    std::string text() const { return std::string(s.text, s.len); }
    std::string errors() const
    {
        std::string r;

        for (size_t i = 0; i < s.errors.n; i++)
            r += std::string(s.errors.v[i]) + "\n";
        return r;
    }
};

// the source must be refused with exactly this message
static void refused(const std::string &text, const std::string &msg)
{
    pp p(text);

    INFO(text);
    CHECK(p.rc == 1);
    CHECK(p.errors() == msg + "\n");
}

TEST_CASE("testpp: the example of tests/README.md", "[testpp]")
{
    pp p("!TestBatchInit\n"
         "  w.l=16\n"
         "  !Require{w=16,\"set up\"}\n"
         "!EndTestBatchInit\n"
         "\n"
         "!TestCase{shape_size}\n"
         "  !CheckEq{w,16,\"width\"}\n"
         "  !Check{w>0}\n"
         "!EndTestCase\n"
         "\n"
         "!TestCase{tab_string}               ; a comment is allowed here\n"
         "  a$=\"a\"+Chr$(9)+\"b\"\n"
         "  !CheckStrEq{a$,\"ab\",\"tab\"}\n"
         "  Dim a(5)\n"
         "  !ExpectError{\"Array Subscript out of Range\"}\n"
         "  a(9)=1\n"
         "!EndTestCase\n");
    REQUIRE(p.rc == 0);
    CHECK(p.text()
          == "!TestBatchInit\n"
             "  w.l=16\n"
             "  !Require{0,w=16}\n"
             "!EndTestBatchInit\n"
             "\n"
             "!TestCase{shape_size}\n"
             "  !CheckEq{1,w,16}\n"
             "  !Check{2,w>0}\n"
             "!EndTestCase\n"
             "\n"
             "!TestCase{tab_string}               ; a comment is allowed here\n"
             "  a$=\"a\"+Chr$(9)+\"b\"\n"
             "  !CheckStrEq{3,a$,\"ab\"}\n"
             "  Dim a(5)\n"
             "  !ExpectError{4}\n"
             "  a(9)=1\n"
             "!EndTestCase\n");
    CHECK(p.s.nlines == 17);
    REQUIRE(p.s.nchecks == 5);
    const tp_check *c = p.s.checks;
    CHECK((c[0].k == 0 && c[0].kind == TP_REQUIRE && c[0].nskind == TP_NS_INIT));
    CHECK(!strcmp(c[0].ns, "init"));
    CHECK(c[0].number == 0);
    CHECK(!strcmp(c[0].name, "set up"));
    CHECK(c[0].line == 3);
    CHECK(!strcmp(c[0].expr, "w=16"));
    CHECK(c[0].expected == nullptr);
    CHECK((c[1].k == 1 && c[1].kind == TP_CHECKEQ && c[1].nskind == TP_NS_CASE));
    CHECK(!strcmp(c[1].ns, "shape_size"));
    CHECK(c[1].number == 0);
    CHECK(!strcmp(c[1].name, "width"));
    CHECK(!strcmp(c[1].expr, "w"));
    CHECK(!strcmp(c[1].expected, "16"));
    CHECK(c[1].line == 7);
    CHECK((c[2].k == 2 && c[2].number == 1 && c[2].name == nullptr));
    CHECK(!strcmp(c[2].expr, "w>0"));
    CHECK((c[3].k == 3 && c[3].kind == TP_CHECKSTREQ && c[3].number == 0));
    CHECK(!strcmp(c[3].ns, "tab_string"));
    CHECK((c[4].k == 4 && c[4].kind == TP_EXPECTERROR && c[4].number == 1));
    CHECK(!strcmp(c[4].message, "Array Subscript out of Range"));
    CHECK(c[4].expr == nullptr);
    // nothing of the names and the message in the program
    CHECK(p.text().find("width") == std::string::npos);
    CHECK(p.text().find("Subscript") == std::string::npos);
    // the cases and blocks
    REQUIRE(p.s.ncases == 2);
    CHECK(!strcmp(p.s.cases[0].name, "shape_size"));
    CHECK(p.s.cases[0].line == 6);
    CHECK(p.s.cases[0].end_line == 9);
    CHECK(p.s.cases[1].line == 11);
    CHECK(p.s.init_line == 1);
    CHECK(p.s.final_line == 0);
    CHECK(tp_find(&p.s, "shape_size", "width") == &c[1]);
    CHECK(tp_find(&p.s, "tab_string", "width") == nullptr);
    CHECK(tp_by_k(&p.s, 3) == &c[3]);
    CHECK(tp_by_k(&p.s, 5) == nullptr);
    CHECK(!strcmp(tp_kind_name(TP_CHECKSTREQ), "CheckStrEq"));
}

// the fenced block after the first line holding `after` in text
static std::string block_after(const std::string &text, const std::string &after)
{
    size_t at = text.find(after);

    REQUIRE(at != std::string::npos);
    size_t s = text.find("```\n", at);
    REQUIRE(s != std::string::npos);
    s += 4;
    size_t e = text.find("```\n", s);
    REQUIRE(e != std::string::npos);
    return text.substr(s, e - s);
}

TEST_CASE("testpp: the example of tests/README.md, from the file", "[testpp]")
{
    FILE *f = fopen(TESTPP_README, "rb");
    std::string text;
    char buf[4096];
    size_t n;

    REQUIRE(f != nullptr);
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        text.append(buf, n);
    fclose(f);
    std::string source = block_after(text, "### The source form");
    std::string want = block_after(text, "The example above becomes:");
    pp p(source);
    REQUIRE(p.rc == 0);
    CHECK(p.text() == want);
}

// the source of the emulator test of the numbering (basic-test-runner task
// 2.2): the same table on every run, the numbers in source order over the
// namespaces of procedures, init, cases and end
TEST_CASE("testpp: the table of tests/blitz2/model-numbering.test.bas is stable", "[testpp]")
{
    FILE *f = fopen(TESTPP_NUMBERING, "rb");
    std::string text;
    char buf[4096];
    size_t n;

    REQUIRE(f != nullptr);
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        text.append(buf, n);
    fclose(f);
    pp a(text), b(text);
    REQUIRE(a.rc == 0);
    REQUIRE(b.rc == 0);
    CHECK(a.text() == b.text());
    REQUIRE(a.s.nchecks == b.s.nchecks);
    static const char *const want[][2] = {
        { "count_up", "arg" }, { "twice", "inner" }, { "init", "base" },   { "first", "one" },
        { "first", "loop" },   { "first", "twice" }, { "first", "str" },   { "second", "two" },
        { "second", "skipped" }, { "second", "base_kept" }, { "end", "final" },
    };
    REQUIRE(a.s.nchecks == sizeof want / sizeof want[0]);
    for (size_t i = 0; i < a.s.nchecks; i++) {
        const tp_check &x = a.s.checks[i], &y = b.s.checks[i];

        INFO(i);
        CHECK(x.k == i);
        CHECK(std::string(x.ns) == want[i][0]);
        CHECK(std::string(x.name) == want[i][1]);
        CHECK(x.k == y.k);
        CHECK(x.number == y.number);
        CHECK(x.line == y.line);
        CHECK(std::string(x.ns) == y.ns);
        CHECK(std::string(x.name) == y.name);
        CHECK(std::string(x.expr) == y.expr);
    }
}

TEST_CASE("testpp: quick and float checks", "[testpp]")
{
    pp p("!TestCase{a}\n"
         "  !CheckQEq{q,0.5,\"half\"}\n"
         "  !CheckFltNear{w*1.5,24,0.01}\n"
         "  !CheckFltNear{Sin(x),f{1,2},0.001,\"sin\"}\n"
         "!EndTestCase\n");
    REQUIRE(p.rc == 0);
    CHECK(p.text()
          == "!TestCase{a}\n"
             "  !CheckQEq{0,q,0.5}\n"
             "  !CheckFltNear{1,w*1.5,24,0.01}\n"
             "  !CheckFltNear{2,Sin(x),f{1,2},0.001}\n"
             "!EndTestCase\n");
    REQUIRE(p.s.nchecks == 3);
    const tp_check *c = p.s.checks;
    CHECK((c[0].kind == TP_CHECKQEQ && c[0].number == 0 && !strcmp(c[0].name, "half")));
    CHECK((!strcmp(c[0].expr, "q") && !strcmp(c[0].expected, "0.5") && c[0].delta == nullptr));
    CHECK((c[1].kind == TP_CHECKFLTNEAR && c[1].name == nullptr));
    CHECK((!strcmp(c[1].expr, "w*1.5") && !strcmp(c[1].expected, "24") && !strcmp(c[1].delta, "0.01")));
    CHECK((!strcmp(c[2].expected, "f{1,2}") && !strcmp(c[2].delta, "0.001") && !strcmp(c[2].name, "sin")));
    CHECK(!strcmp(tp_kind_name(TP_CHECKFLTNEAR), "CheckFltNear"));
    CHECK(p.text().find("half") == std::string::npos);
}

TEST_CASE("testpp: numbers over two cases, namespaces", "[testpp]")
{
    pp p("Statement helper{v}\n"
         "  !CheckEq{v,1,\"one\"}\n"
         "End Statement\n"
         "Function.l f{a,b}\n"
         "  !Check{a<b}\n"
         "  Function Return a+b\n"
         "End Function\n"
         "!TestCase{a}\n"
         "  !Check{1}:!Check{2}\n"
         "  helper{1}\n"
         "!EndTestCase\n"
         "!TestCase{b}\n"
         "  For i=1 To 3 : !CheckEq{i,i} : Next\n"
         "!EndTestCase\n"
         "!TestBatchEnd\n"
         "  !Check{-1,\"done\"}\n"
         "!EndTestBatchEnd\n");
    REQUIRE(p.rc == 0);
    REQUIRE(p.s.nchecks == 6);
    const tp_check *c = p.s.checks;
    CHECK((c[0].nskind == TP_NS_PROC && !strcmp(c[0].ns, "helper") && c[0].number == 0 && c[0].k == 0));
    CHECK((c[1].nskind == TP_NS_PROC && !strcmp(c[1].ns, "f") && c[1].number == 0 && c[1].k == 1));
    CHECK((!strcmp(c[2].ns, "a") && c[2].number == 0 && c[2].k == 2));
    CHECK((!strcmp(c[3].ns, "a") && c[3].number == 1 && c[3].k == 3 && c[3].line == 9));
    CHECK((!strcmp(c[4].ns, "b") && c[4].number == 0 && c[4].k == 4));
    CHECK((c[5].nskind == TP_NS_END && !strcmp(c[5].ns, "end") && c[5].number == 0 && c[5].k == 5));
    CHECK(p.text().find("  !Check{2,1}:!Check{3,2}\n") != std::string::npos);
    CHECK(p.text().find("  For i=1 To 3 : !CheckEq{4,i,i} : Next\n") != std::string::npos);
    CHECK(p.text().find("!Check{5,-1}\n") != std::string::npos);
    CHECK(p.s.final_line == 15);
}

TEST_CASE("testpp: the arguments by the compiler's rules", "[testpp]")
{
    auto one = [](const std::string &call) {
        pp p("!TestCase{a}\n" + call + "\n!EndTestCase\n");

        REQUIRE(p.rc == 0);
        REQUIRE(p.s.nchecks >= 1);
        std::string t = p.text();
        size_t s = t.find('\n') + 1;
        return t.substr(s, t.find('\n', s) - s);
    };
    // commas and braces in strings
    CHECK(one("!CheckStrEq{Mid$(\"abc,def\",2,5),\"bc,de\",\"mid\"}") == "!CheckStrEq{0,Mid$(\"abc,def\",2,5),\"bc,de\"}");
    CHECK(one("!CheckEq{Len(\"a}b\"),3}") == "!CheckEq{0,Len(\"a}b\"),3}");
    CHECK(one("!CheckStrEq{a$,\"a,b\"}") == "!CheckStrEq{0,a$,\"a,b\"}");
    CHECK(one("!CheckStrEq{a$,\"{\"}") == "!CheckStrEq{0,a$,\"{\"}");
    // braces and parentheses nest: a procedure call, a function, an array
    CHECK(one("!CheckEq{f{1,2},3}") == "!CheckEq{0,f{1,2},3}");
    CHECK(one("!CheckEq{Peek.l(Addr(a,b)),$10}") == "!CheckEq{0,Peek.l(Addr(a,b)),$10}");
    CHECK(one("!Check{(a,b)}") == "!Check{0,(a,b)}");
    // the arguments as written, blanks kept; the name and its comma gone
    CHECK(one("!CheckEq{ a , 1 , \"n\" }") == "!CheckEq{0, a , 1 }");
    // blanks between the name and the brace (makename skips them)
    CHECK(one("!Check  {x}") == "!Check  {0,x}");
    // a check on the line of an If, and behind a statement
    CHECK(one("If c Then !Check{x}") == "If c Then !Check{0,x}");
    CHECK(one("y=2:!CheckEq{y,2} ; a comment !Check{z}") == "y=2:!CheckEq{0,y,2} ; a comment !Check{z}");
    // not a call of a check macro: a string, another macro name, case
    CHECK(one("a$=\"!Check{x}\":!Check{x}") == "a$=\"!Check{x}\":!Check{0,x}");
    {
        pp p("!TestCase{a}\n!check{x}\n!CheckEqX{a,b}\n!CheckEqual{a,b}\n!EndTestCase\n");
        CHECK(p.rc == 0);
        CHECK(p.s.nchecks == 0);
    }
    // the source is text: a byte >= $80 is one Latin-1 character (findit's
    // pair rule belongs to the tokenised source): an umlaut as the last
    // byte of a string argument, "ß,ß" as one argument, an umlaut in the
    // comment behind a call
    CHECK(one("!CheckStrEq{a$,\"x\xe4\"}") == "!CheckStrEq{0,a$,\"x\xe4\"}");
    CHECK(one("!CheckStrEq{a$,\"\xdf,\xdf\",\"sz\"}") == "!CheckStrEq{0,a$,\"\xdf,\xdf\"}");
    CHECK(one("!CheckEq{\xe4,1} ; Gr\xfc\xdf" "e \xe4") == "!CheckEq{0,\xe4,1} ; Gr\xfc\xdf" "e \xe4");
    CHECK(one("!Check{a$=\"\xe4\"}:!Check{b}") == "!Check{0,a$=\"\xe4\"}:!Check{1,b}");
    {
        pp p("!TestCase{a}\n!CheckStrEq{a$,\"\xdf,\xdf\",\"sz\"}\n!EndTestCase\n");
        REQUIRE(p.rc == 0);
        REQUIRE(p.s.nchecks == 1);
        CHECK(!strcmp(p.s.checks[0].expected, "\"\xdf,\xdf\""));
        CHECK(!strcmp(p.s.checks[0].name, "sz"));
    }
    CHECK(one("!CheckStrEq{a$,\"\xc3\xa4\"}") == "!CheckStrEq{0,a$,\"\xc3\xa4\"}");
    // a CR before the LF stays
    {
        pp p("!TestCase{a}\r\n!Check{1}\r\n!EndTestCase\r\n");
        CHECK(p.rc == 0);
        CHECK(p.text() == "!TestCase{a}\r\n!Check{0,1}\r\n!EndTestCase\r\n");
    }
}

TEST_CASE("testpp: macros of the test", "[testpp]")
{
    // a check as an argument of the test's own macro, also nested twice
    pp p("Macro Twice\n"
         "  `1 : `1\n"
         "End Macro\n"
         "Macro Wrap:`1:End Macro\n"
         "!TestCase{a}\n"
         "  !Twice{!CheckEq{a,1,\"twice\"}}\n"
         "  !Wrap{!Twice{!Check{Mid$(\"a,b\",1,2)=\"a,\"}}}\n"
         "  !Wrap{x,!Check{\"x}y\"<>\"\"}}\n"
         "!EndTestCase\n");
    REQUIRE(p.rc == 0);
    REQUIRE(p.s.nchecks == 3);
    CHECK(p.text().find("  !Twice{!CheckEq{0,a,1}}\n") != std::string::npos);
    CHECK(p.text().find("  !Wrap{!Twice{!Check{1,Mid$(\"a,b\",1,2)=\"a,\"}}}\n") != std::string::npos);
    CHECK(p.text().find("  !Wrap{x,!Check{2,\"x}y\"<>\"\"}}\n") != std::string::npos);
    CHECK(!strcmp(p.s.checks[1].expr, "Mid$(\"a,b\",1,2)=\"a,\""));

    // a check inside a macro definition: refused, also in a one-line macro
    refused("Macro M\n  !CheckEq{a,1}\nEnd Macro\n!TestCase{a}\n!M\n!EndTestCase\n",
            "x.test.bas:2: !CheckEq inside the macro M (line 1): checks stand in the blocks and procedures");
    refused("!TestCase{a}\n!EndTestCase\nmacro M2 : !Check{1} : end macro\n",
            "x.test.bas:3: !Check inside the macro M2 (line 3): checks stand in the blocks and procedures");
    // after End Macro checks count again
    pp q("Macro M\nx=1\nEnd Macro\n!TestCase{a}\n!Check{1}\n!EndTestCase\n");
    CHECK(q.rc == 0);
    CHECK(q.s.nchecks == 1);
}

TEST_CASE("testpp: placeholders", "[testpp]")
{
    pp p("!TestCase{frames}\n"
         "  NPrint \"@TEST@ @CASE@ line @LINE@\" ; @LINE@ @FOO@ in a comment\n"
         "  f$=\"@TESTDATA@/x.iff\":!CheckStrEq{f$,\"@TESTDATA@/x.iff\",\"@CASE@\"}\n"
         "  a=@LINE@\n"
         "  b=c`@d  ; the use counter of a macro, no placeholder\n"
         "!EndTestCase\n");
    REQUIRE(p.rc == 0);
    CHECK(p.text()
          == "!TestCase{frames}\n"
             "  NPrint \"x frames line 2\" ; @LINE@ @FOO@ in a comment\n"
             "  f$=\"TEST:data/x.iff\":!CheckStrEq{0,f$,\"TEST:data/x.iff\"}\n"
             "  a=4\n"
             "  b=c`@d  ; the use counter of a macro, no placeholder\n"
             "!EndTestCase\n");
    // the table keeps the text as written
    CHECK(!strcmp(p.s.checks[0].expected, "\"@TESTDATA@/x.iff\""));
    CHECK(!strcmp(p.s.checks[0].name, "@CASE@"));

    refused("!TestCase{a}\n\n  x$=\"@FOO@\"\n!EndTestCase\n", "x.test.bas:3: unknown placeholder @FOO@");
    refused("!TestCase{a}\n  x=@LINES@\n!EndTestCase\n", "x.test.bas:2: unknown placeholder @LINES@");
    refused("x$=\"@CASE@\"\n!TestCase{a}\n!EndTestCase\n", "x.test.bas:1: @CASE@ outside a case");
    {
        pp r("!TestCase{a}\nx$=\"@TESTDATA@\"\n!EndTestCase\n", nullptr);
        CHECK(r.errors() == "x.test.bas:2: unknown placeholder @TESTDATA@\n");
    }
    // not placeholders: lower case, an @ alone, no closing @
    pp n("!TestCase{a}\nx$=\"a@b @ @x@ @X\"\n!EndTestCase\n");
    CHECK(n.rc == 0);
    CHECK(n.text() == "!TestCase{a}\nx$=\"a@b @ @x@ @X\"\n!EndTestCase\n");
}

TEST_CASE("testpp: refused calls", "[testpp]")
{
    const std::string a = "!TestCase{a}\n", e = "\n!EndTestCase\n";

    refused(a + "!Check" + e, "x.test.bas:2: !Check needs its arguments in braces");
    refused(a + "!CheckEq{a,1" + e, "x.test.bas:2: !CheckEq: the line ends before the closing }");
    refused(a + "!CheckEq{a}" + e, "x.test.bas:2: !CheckEq takes 2 arguments and an optional name, 1 given");
    refused(a + "!CheckEq{a,1,\"n\",4}" + e, "x.test.bas:2: !CheckEq takes 2 arguments and an optional name, 4 given");
    refused(a + "!Check{a,b,c}" + e, "x.test.bas:2: !Check takes 1 argument and an optional name, 3 given");
    refused(a + "!CheckQEq{q}" + e, "x.test.bas:2: !CheckQEq takes 2 arguments and an optional name, 1 given");
    refused(a + "!CheckFltNear{f,2.5}" + e, "x.test.bas:2: !CheckFltNear takes 3 arguments and an optional name, 2 given");
    refused(a + "!CheckFltNear{f,2.5,0.1,n$}" + e, "x.test.bas:2: the name of !CheckFltNear must be a string literal");
    refused(a + "!Check{a,b}" + e, "x.test.bas:2: the name of !Check must be a string literal");
    refused(a + "!CheckStrEq{a$,b$,n$}" + e, "x.test.bas:2: the name of !CheckStrEq must be a string literal");
    refused(a + "!CheckEq{a,1,\"x\"+\"y\"}" + e, "x.test.bas:2: the name of !CheckEq must be a string literal");
    refused(a + "!CheckEq{a,}" + e, "x.test.bas:2: !CheckEq: argument 2 is empty");
    refused(a + "!Check{}" + e, "x.test.bas:2: !Check: argument 1 is empty");
    refused(a + "!ExpectError{m$}" + e, "x.test.bas:2: the message of !ExpectError must be a string literal");
    refused(a + "!ExpectError{}" + e, "x.test.bas:2: !ExpectError: argument 1 is empty");
    refused(a + "!ExpectError{\"a\",\"b\",\"c\"}" + e,
            "x.test.bas:2: !ExpectError takes the message and an optional name, 3 arguments given");
    refused(a + "!Check{!CheckEq{a,1}}" + e, "x.test.bas:2: !CheckEq inside the arguments of !Check");
    refused(a + "!CheckEq{a,1,\"n\"}\n!CheckEq{b,2,\"n\"}" + e,
            "x.test.bas:3: the check name \"n\" is used twice in a (line 2)");
    // the same name in another namespace is fine
    pp ok(a + "!CheckEq{a,1,\"n\"}\n!EndTestCase\n!TestCase{b}\n!CheckEq{b,2,\"n\"}" + e);
    CHECK(ok.rc == 0);
}

TEST_CASE("testpp: namespaces refused", "[testpp]")
{
    refused("!Check{1}\n!TestCase{a}\n!EndTestCase\n", "x.test.bas:1: !Check outside the blocks and procedures");
    refused("!TestCase{a}\n!EndTestCase\n!Require{1}\n", "x.test.bas:3: !Require outside the blocks and procedures");
    refused("Statement a{}\n!Check{1}\nEnd Statement\n!TestCase{a}\n!EndTestCase\n",
            "x.test.bas:2: the procedure a holds checks and has the name of a case or of init or end");
    refused("Function init{}\n!Check{1}\n!Check{2}\nEnd Function\n!TestCase{a}\n!EndTestCase\n",
            "x.test.bas:2: the procedure init holds checks and has the name of a case or of init or end");
    // Statement Return is no definition
    pp p("Statement s{}\n  Statement Return\n  !Check{1}\nEnd Statement\n!TestCase{a}\n!EndTestCase\n");
    CHECK(p.rc == 0);
    CHECK(!strcmp(p.s.checks[0].ns, "s"));
    // several messages at once
    pp m("!Check{1}\n!TestCase{a}\n!CheckEq{a}\nx=@NO@\n!EndTestCase\n");
    CHECK(m.rc == 3);
    CHECK(m.errors()
          == "x.test.bas:1: !Check outside the blocks and procedures\n"
             "x.test.bas:3: !CheckEq takes 2 arguments and an optional name, 1 given\n"
             "x.test.bas:4: unknown placeholder @NO@\n");
    // a 0 byte
    const char zs[] = "!TestCase{a}\nx\0y\n";
    pp zz(std::string(zs, sizeof zs - 1));
    CHECK(zz.errors() == "x.test.bas:2: a 0 byte in the source\n");
}
