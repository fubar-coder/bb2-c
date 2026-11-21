// test_testform - the form of the Blitz test sources (tools/lib/testpp,
// tp_check_form), the registration's check: blocks on lines of their own,
// paired in source order with the case at fault named, their order, no
// conditional compilation or loop crossing a block, no executable code
// outside the blocks, the case names - each rule with broken input.
#include <cstdio>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "testpp.h"
}

static const char *const tokens[] = { "Free", "NPrint", "End", "Shape" };

// the messages of preprocessing and checking the form, one per line
static std::string form(const std::string &text)
{
    tp_params p = {};
    tp_source s = {};
    std::string r;

    p.file = "x.test.bas";
    p.test_id = "x";
    p.testdata = "TEST:";
    p.tokens = tokens;
    p.ntokens = sizeof tokens / sizeof tokens[0];
    REQUIRE(tp_process(text.data(), text.size(), &p, &s) >= 0);
    REQUIRE(tp_check_form(text.data(), text.size(), &p, &s) >= 0);
    for (size_t i = 0; i < s.errors.n; i++)
        r += std::string(s.errors.v[i]) + "\n";
    tp_free(&s);
    return r;
}

static void refused(const std::string &text, const std::string &msg)
{
    INFO(text);
    CHECK(form(text) == msg + "\n");
}

static const std::string ok_case = "!TestCase{a}\n!EndTestCase\n";

TEST_CASE("testform: a good source", "[testform]")
{
    CHECK(form("; the header comment\n"
               "#width=16 : #height=8\n"
               "OPTIMIZE 1\n"
               "WBStartup\n"
               "DEFTYPE.l\n"
               "NEWTYPE .pt\n"
               "  x.w : y.w\n"
               "End NEWTYPE\n"
               "Macro Twice\n"
               "  `1 : `1\n"
               "End Macro\n"
               "Statement helper{v}\n"
               "  Dim a(5)\n"
               "  !CheckEq{v,1}\n"
               "  For i=1 To 2 : Next\n"
               "End Statement\n"
               "Function.l f{a}\n"
               "  Function Return a\n"
               "End Function\n"
               ".table\n"
               "Data.w 1,2,3\n"
               "Even\n"
               "\n"
               "!TestBatchInit ; the shared data\n"
               "  Dim d(3)\n"
               "  !Require{d(0)=0}\n"
               "!EndTestBatchInit\n"
               "!TestCase{frames_max}   ;comment with : colon and \"quote\n"
               "  For i=1 To 3\n"
               "    If i=2\n"
               "      !Check{i}\n"
               "    EndIf\n"
               "    If i=3 Then NPrint i\n"
               "  Next\n"
               "  While x<2 : x+1 : Wend\n"
               "  Repeat : x-1 : Until x=0\n"
               "  Select x\n"
               "    Case 0 : !Twice{!Check{1}}\n"
               "  End Select\n"
               "  If x Then For j=1 To 2 : Next\n"
               "!EndTestCase;x\n"
               "CNIF #width=16\n"
               "!TestCase{inside_cnif}\n"
               "  CNIF 1 : x=1 : CELSE : x=2 : CEND\n"
               "!EndTestCase\n"
               "CEND\n"
               "!TestBatchEnd\n"
               "  helper{1}\n"
               "!EndTestBatchEnd\n")
          == "");
}

TEST_CASE("testform: block macros on lines of their own", "[testform]")
{
    refused("!TestCase{x}:y=5\n!EndTestCase\n",
            "x.test.bas:1: !TestCase must stand on a line of its own; only a comment may follow it");
    refused("!TestCase{x}\ny=5 : !EndTestCase\n",
            "x.test.bas:2: !EndTestCase must stand on a line of its own; only a comment may follow it");
    refused("!TestCase{x}\nIf c Then !EndTestCase\n!EndTestCase\n",
            "x.test.bas:2: !EndTestCase must stand on a line of its own; only a comment may follow it");
    refused("!TestBatchInit x=1\n!EndTestBatchInit\n" + ok_case,
            "x.test.bas:1: !TestBatchInit must stand on a line of its own; only a comment may follow it");
    // blanks before, a comment behind: fine; in a string or a comment: no macro
    CHECK(form("  !TestCase{x}  ; a comment\n  a$=\"!EndTestCase\" ; !TestCase{y}\n\t!EndTestCase\n") == "");
    refused("Macro M\n!TestCase{q}\nEnd Macro\n" + ok_case, "x.test.bas:2: a block macro inside the macro M");
}

TEST_CASE("testform: pairing in source order", "[testform]")
{
    // the second of three cases has no end: named at the third
    refused("!TestCase{a}\n!EndTestCase\n!TestCase{b}\nx=1\n!TestCase{c}\n!EndTestCase\n",
            "x.test.bas:5: the case b (line 3) has no !EndTestCase");
    // an end without a case
    refused("!EndTestCase\n" + ok_case, "x.test.bas:1: !EndTestCase without a case before it");
    // an extra end does not cancel a missing one
    refused("!TestCase{a}\n!EndTestCase\n!EndTestCase\n!TestCase{b}\n!TestCase{c}\n!EndTestCase\n",
            "x.test.bas:3: !EndTestCase without a case before it\n"
            "x.test.bas:5: the case b (line 4) has no !EndTestCase");
    // the last case without its end
    refused("!TestCase{a}\n!EndTestCase\n!TestCase{b}\nx=1\n", "x.test.bas:3: the case b (line 3) has no !EndTestCase");
    // the wrong closing macro
    refused("!TestBatchInit\n!EndTestCase\n" + ok_case, "x.test.bas:2: !EndTestCase closes the batch init (line 1)");
    refused("!TestCase{a}\n!EndTestBatchEnd\n", "x.test.bas:2: !EndTestBatchEnd closes the case a (line 1)");
    refused(ok_case + "!EndTestBatchInit\n", "x.test.bas:3: !EndTestBatchInit without a batch init before it");
    refused("!TestBatchInit\n" + ok_case,
            "x.test.bas:2: the batch init (line 1) has no !EndTestBatchInit");
}

TEST_CASE("testform: the order of the blocks", "[testform]")
{
    refused("x=1\n", "x.test.bas:1: executable code outside the blocks: x=1\nx.test.bas:1: the source has no case");
    refused("; only a comment\n", "x.test.bas:1: the source has no case");
    refused(ok_case + "!TestBatchInit\n!EndTestBatchInit\n",
            "x.test.bas:3: the batch init must stand before the first case");
    refused("!TestBatchInit\n!EndTestBatchInit\n!TestBatchInit\n!EndTestBatchInit\n" + ok_case,
            "x.test.bas:3: a second batch init (the first in line 1)");
    refused("!TestBatchEnd\n!EndTestBatchEnd\n" + ok_case, "x.test.bas:3: a case after the batch end (line 1)");
    refused(ok_case + "!TestBatchEnd\n!EndTestBatchEnd\n!TestBatchEnd\n!EndTestBatchEnd\n",
            "x.test.bas:5: a second batch end (the first in line 3)");
}

TEST_CASE("testform: nothing crosses a block", "[testform]")
{
    // a case that starts inside a For loop and ends after its Next
    refused("For i=1 To 2\n!TestCase{a}\nNext\n!EndTestCase\n",
            "x.test.bas:1: executable code outside the blocks: For i=1 To 2\n"
            "x.test.bas:3: the Next closes a structure opened outside the case a (line 2)");
    refused("!TestBatchInit\nFor i=1 To 2\n!EndTestBatchInit\n!TestCase{a}\nNext\n!EndTestCase\n",
            "x.test.bas:3: the batch init (line 1) ends inside the For of line 2\n"
            "x.test.bas:5: the Next closes a structure opened outside the case a (line 4)");
    refused("!TestCase{a}\nWhile x\n!EndTestCase\n", "x.test.bas:3: the case a (line 1) ends inside the While of line 2");
    refused("!TestCase{a}\nSelect x\nCase 1\n!EndTestCase\n",
            "x.test.bas:4: the case a (line 1) ends inside the Select of line 2");
    refused("!TestCase{a}\nIf x\n!EndTestCase\n", "x.test.bas:3: the case a (line 1) ends inside the If of line 2");
    refused("!TestCase{a}\nEnd If\n!EndTestCase\n", "x.test.bas:2: the End If closes a structure opened outside the case a (line 1)");
    refused("!TestCase{a}\nUntil x\n!EndTestCase\n", "x.test.bas:2: the Until closes a structure opened outside the case a (line 1)");
    // conditional compilation that cuts a case
    refused("CNIF 0\n!TestCase{b}\nCEND\n!EndTestCase\n",
            "x.test.bas:4: conditional compilation cuts the case b (line 2): a CNIF, CSIF, CELSE or CEND inside it "
            "has no pair inside it");
    refused("!TestCase{b}\nCNIF 1\n!EndTestCase\nCEND\n",
            "x.test.bas:3: conditional compilation cuts the case b (line 1): a CNIF, CSIF, CELSE or CEND inside it "
            "has no pair inside it");
    refused("CNIF 1\n!TestCase{b}\nCELSE\n!EndTestCase\nCEND\n",
            "x.test.bas:4: conditional compilation cuts the case b (line 2): a CNIF, CSIF, CELSE or CEND inside it "
            "has no pair inside it");
}

TEST_CASE("testform: executable code outside the blocks", "[testform]")
{
    refused("Dim a(5)\n" + ok_case, "x.test.bas:1: executable code outside the blocks: Dim a(5)");
    refused(ok_case + "LoadShape 0,\"x\"\n", "x.test.bas:3: executable code outside the blocks: LoadShape 0,\"x\"");
    refused(ok_case + ".helper\nNPrint 1 : Return\n",
            "x.test.bas:4: executable code outside the blocks: NPrint 1\n"
            "x.test.bas:4: executable code outside the blocks: Return");
    refused("INCLUDE \"x.bb2\"\n" + ok_case, "x.test.bas:1: executable code outside the blocks: INCLUDE \"x.bb2\"");
    refused("!Twice{x}\n" + ok_case, "x.test.bas:1: executable code outside the blocks: !Twice{x}");
}

TEST_CASE("testform: case names", "[testform]")
{
    refused("!TestCase{Bad}\n!EndTestCase\n", "x.test.bas:1: the case name \"Bad\" does not match [a-z][a-z0-9_]*");
    refused("!TestCase{1x}\n!EndTestCase\n", "x.test.bas:1: the case name \"1x\" does not match [a-z][a-z0-9_]*");
    refused("!TestCase{a-b}\n!EndTestCase\n", "x.test.bas:1: the case name \"a-b\" does not match [a-z][a-z0-9_]*");
    refused("!TestCase{_init}\n!EndTestCase\n",
            "x.test.bas:1: the case name \"_init\" does not match [a-z][a-z0-9_]*");
    refused("!TestCase{a__b}\n!EndTestCase\n", "x.test.bas:1: the case name a__b holds \"__\" (the driver's labels)");
    refused("!TestCase{x_originals}\n!EndTestCase\n",
            "x.test.bas:1: the case name x_originals ends in _originals (the name of a registered test's side)");
    refused("!TestCase{free}\n!EndTestCase\n", "x.test.bas:1: the case name free is the token Free");
    refused("!TestCase{nprint}\n!EndTestCase\n", "x.test.bas:1: the case name nprint is the token NPrint");
    refused("!TestCase\n!EndTestCase\n", "x.test.bas:1: !TestCase needs the name of the case in braces");
    refused("!TestCase{}\n!EndTestCase\n", "x.test.bas:1: !TestCase needs the name of the case in braces");
    refused(ok_case + ok_case, "x.test.bas:3: the case name a is used twice (line 1)");
    refused("!TestBatchInit\n!EndTestBatchInit\n!TestCase{init}\n!EndTestCase\n",
            "x.test.bas:3: the case name init is the namespace of the batch init");
    // names that are fine: a token as a part of the name, digits, _
    CHECK(form("!TestCase{free_list}\n!EndTestCase\n!TestCase{shape2}\n!EndTestCase\n!TestCase{init}\n!EndTestCase\n")
          == "");
}
