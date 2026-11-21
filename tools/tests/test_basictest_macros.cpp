// test_basictest_macros - the macro text of the Blitz test drivers
// (shared/basictest/macros.bas, spec testing/basic-test-interface): it
// defines every macro of the spec and no other, the blocks as subroutines
// whose opening macro refers with Dc.l to the label of its closing macro
// (tc_end_`@ for a case, the fixed labels tc__init_end and tc__final_end),
// and the check macros that call the library routines with K first and in
// the spec's argument order.
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>

namespace {

using body = std::vector<std::string>;

// the macros of the file: name -> lines of the body without leading blanks
std::map<std::string, body> macros()
{
    std::ifstream in(BASICTEST_MACROS, std::ios::binary);
    REQUIRE(in.good());
    std::map<std::string, body> m;
    std::string line, open;
    bool inside = false;
    while (std::getline(in, line)) {
        REQUIRE(line.find('\r') == std::string::npos);
        if (!inside) {
            if (line.empty() || line[0] == ';')
                continue;
            REQUIRE(line.starts_with("Macro "));
            open = line.substr(6);
            REQUIRE(!m.contains(open));
            m[open];
            inside = true;
            continue;
        }
        if (line == "End Macro") {
            inside = false;
            continue;
        }
        size_t i = line.find_first_not_of(' ');
        REQUIRE(i != std::string::npos);
        m[open].push_back(line.substr(i));
    }
    REQUIRE(!inside);
    return m;
}

// the arguments of a call "NAME a,b,c" (no comma inside an argument here)
std::vector<std::string> args(const std::string &call, const std::string &name)
{
    REQUIRE(call.starts_with(name + " "));
    std::vector<std::string> a;
    std::stringstream s(call.substr(name.size() + 1));
    std::string x;
    while (std::getline(s, x, ','))
        a.push_back(x);
    return a;
}

} // namespace

TEST_CASE("macros.bas defines the macros of the spec and no other", "[basictest]")
{
    std::set<std::string> want = { "TestBatchInit", "EndTestBatchInit", "TestCase",
                                   "EndTestCase",   "TestBatchEnd",     "EndTestBatchEnd",
                                   "Check",         "CheckEq",          "CheckStrEq",
                                   "Require",       "ExpectError" };
    std::set<std::string> have;
    for (const auto &[name, b] : macros()) {
        have.insert(name);
        CHECK(!b.empty());
    }
    CHECK(have == want);
}

TEST_CASE("macros.bas: the text of every macro", "[basictest]")
{
    std::map<std::string, body> want = {
        { "TestBatchInit", { "Dc.l tc__init_end", ".tc__init", "If TLInit=0 Then Return" } },
        { "EndTestBatchInit", { "TLBlockEnd : Return", ".tc__init_end" } },
        { "TestCase", { "Dc.l tc_end_`@", ".tc_`1", "If TLCase(\"`1\")=0 Then Return" } },
        { "EndTestCase", { "TLBlockEnd : Return", ".tc_end_`@" } },
        { "TestBatchEnd", { "Dc.l tc__final_end", ".tc__final", "If TLFinal=0 Then Return" } },
        { "EndTestBatchEnd", { "TLBlockEnd : Return", ".tc__final_end" } },
        { "Check", { "TLCheck `1,-1,(`2)<>0" } },
        { "CheckEq", { "TLCheck `1,`3,`2" } },
        { "CheckStrEq", { "TLCheckStr `1,`3,`2" } },
        { "Require", { "TLRequire `1,`2" } },
        { "ExpectError", { "TLExpectError `1" } },
    };
    auto m = macros();
    for (const auto &[name, b] : want) {
        CAPTURE(name);
        REQUIRE(m.contains(name));
        CHECK(m[name] == b);
    }
}

TEST_CASE("macros.bas: a block refers to the label of its closing macro", "[basictest]")
{
    auto m = macros();
    const struct {
        const char *open, *close, *label, *end, *routine;
    } blocks[] = {
        { "TestBatchInit", "EndTestBatchInit", ".tc__init", "tc__init_end", "TLInit=0" },
        { "TestCase", "EndTestCase", ".tc_`1", "tc_end_`@", "TLCase(\"`1\")=0" },
        { "TestBatchEnd", "EndTestBatchEnd", ".tc__final", "tc__final_end", "TLFinal=0" },
    };
    for (const auto &b : blocks) {
        CAPTURE(b.open);
        const body &o = m[b.open], &c = m[b.close];
        REQUIRE(o.size() == 3u);
        REQUIRE(c.size() == 2u);
        // the reference first, then the label the driver's Gosub calls
        CHECK(o[0] == std::string("Dc.l ") + b.end);
        CHECK(o[1] == b.label);
        // the block returns at once when the library says it does not run;
        // the If ... Then is the last statement (it takes the rest of the line)
        CHECK(o[2] == std::string("If ") + b.routine + " Then Return");
        // the closing macro ends the block in the library, returns, and
        // defines the label the reference needs
        CHECK(c[0] == "TLBlockEnd : Return");
        CHECK(c[1] == std::string(".") + b.end);
    }
}

TEST_CASE("macros.bas: the checks call the library with K first", "[basictest]")
{
    auto m = macros();
    // !CheckEq{K,ACTUAL,EXPECTED} -> TLCheck K,expected,actual
    auto a = args(m["CheckEq"].at(0), "TLCheck");
    CHECK(a == std::vector<std::string>{ "`1", "`3", "`2" });
    // !CheckStrEq{K,ACTUAL$,EXPECTED$} -> TLCheckStr K,expected$,actual$
    a = args(m["CheckStrEq"].at(0), "TLCheckStr");
    CHECK(a == std::vector<std::string>{ "`1", "`3", "`2" });
    // !Check{K,COND} -> TLCheck K,-1,(COND)<>0: ok if COND is not 0
    a = args(m["Check"].at(0), "TLCheck");
    CHECK(a == std::vector<std::string>{ "`1", "-1", "(`2)<>0" });
    // !Require{K,COND} -> TLRequire K,cond
    a = args(m["Require"].at(0), "TLRequire");
    CHECK(a == std::vector<std::string>{ "`1", "`2" });
    // !ExpectError{K} -> TLExpectError K
    a = args(m["ExpectError"].at(0), "TLExpectError");
    CHECK(a == std::vector<std::string>{ "`1" });
    for (const char *name : { "Check", "CheckEq", "CheckStrEq", "Require", "ExpectError" })
        CHECK(m[name].size() == 1u);
}

TEST_CASE("macros.bas: labels with __ are the driver's fixed ones", "[basictest]")
{
    std::set<std::string> fixed = { "tc__init", "tc__init_end", "tc__final", "tc__final_end" };
    for (const auto &[name, b] : macros()) {
        for (const std::string &l : b) {
            size_t p = 0;
            while ((p = l.find("tc_", p)) != std::string::npos) {
                size_t e = l.find_first_not_of(
                    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_`@", p);
                std::string label = l.substr(p, e == std::string::npos ? e : e - p);
                CAPTURE(name, label);
                if (label.find("__") != std::string::npos)
                    CHECK(fixed.contains(label));
                p += label.size();
            }
        }
    }
}
