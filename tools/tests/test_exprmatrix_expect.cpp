// test_exprmatrix_expect - the committed tests of the expression matrix
// (lib/exprmatrix, expect.c): a small description with a cell of every
// context and of every kind of test (an agreement, a baseline, a fixed and
// an open cell, one of two issues' values, a refused one, one refused
// after a fix, an index cell unchecked and checked) gives the tests of the
// BASIC test model with their names, cases, checks, settings and
// deviations, and the AmiBlitz3 cases of the agreement and fixed sources;
// the cells of an issue fixed after the matrix stay in their bug-orig
// tests with fixed-orig (T1); a value the model does not know needs its
// measured row; the generator's own files are recognised by their path.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprmatrix.h"
}

static const char *const DESC =
    "cells-per-source 56\n"
    "values A b=3 w=1000 l=100000 q=1.5 f=2.5 cb=3 cw=1000 cl=100000 cf=1.5\n"
    "counts A b=2 w=2 l=2 q=2 f=2 cb=2 cw=2 cl=2 cf=2.5\n"
    "values B b=-3 w=-1000 l=-100000 q=-1.5 f=-2.5 cb=-3 cw=-1000 cl=-100000 cf=-1.5\n"
    "counts B b=3 w=3 l=3 q=3 f=3 cb=3 cw=3 cl=3 cf=3.5\n"
    "values I b=3 w=20 l=30 q=1.5 f=2.5 cb=3 cw=200 cl=40000 cf=1.5\n"
    "block t\n"
    "cell A dl + w l\ncell A dl * l l\ncell A db < q l\ncell A dl * l q\ncell A db AND cf b\n"
    "cell A dl AND f b\ncell I i1 * q w\ncell B dl ^ f b\ncell B df ^ f b\ncell A db MOD cb cb\n"
    "cell A db MOD cb cl\ncell A dl MOD cl cb\ncell A dl MOD l cb\ncell B dl ^ q b\n"
    "cell A dq * q q\ncell A pl + w l\ncell A pq + q l\ncell A if - q q\ncell A if < q l\n"
    "cell I i2 + b w\ncell I i2 * l cl\ncell A print + w l\ncell A print / q l\n"
    // the named cells of expr-tree-5: another value, compiles after the change, refused after it
    "cell A db / q l\ncell A dl AND l cf\ncell A dq LSL q cf\ncell A dq LSL q cb\n";

static const char *const HEAD = "key\tsource\tc210\tc195\tportcd\tclib\tab3\tc210_on\tc195_on\tportcd_on\tclib_on\n";
// the Float powers the model does not compute: 1.95's bits of (-2.5)^3, and
// the whole number of (-1.5)^3 (a Quick base, #1178 and #1189)
static const char *const POW = "df-pow-f-b-B\t-\tF9FFFB44\tF9FFFB44\tF9FFFB44\tF9FFFB44\tF9FFFB44\t\t\t\t\n"
                               "dl-pow-q-b-B\t-\t8\t8\t8\t8\t8\t\t\t\t\n";

struct gen {
    struct em_matrix m;
    std::map<std::string, std::string> files;
    std::string report, errors;
    int rc = -9;

    explicit gen(const std::string &measured, uint64_t port = EM_RULES_PORTNOW)
    {
        char *e = nullptr, *rep = nullptr;
        struct em_table t;
        struct em_file *f = nullptr;
        size_t n = 0;

        if (em_parse(DESC, std::strlen(DESC), "m.txt", &m, &e)) {
            std::fprintf(stderr, "em_parse: %s", e ? e : "");
            std::free(e);
            return;
        }
        if (em_table_parse(measured.data(), measured.size(), "measured.tsv", &t, &e)) {
            std::fprintf(stderr, "em_table_parse: %s", e ? e : "");
            std::free(e);
            return;
        }
        rc = em_expect_port(&m, "m.txt", &t, port, &f, &n, &rep, &e);
        for (size_t i = 0; i < n; i++)
            files[f[i].path] = f[i].text;
        if (rep)
            report = rep;
        if (e)
            errors = e;
        em_files_free(f, n);
        std::free(rep);
        std::free(e);
        em_table_free(&t);
    }
    ~gen() { em_free(&m); }

    std::string file(const std::string &path) const
    {
        auto it = files.find(path);
        return it == files.end() ? std::string("(no file)") : it->second;
    }
    bool has(const std::string &path, const std::string &text) const
    {
        return file(path).find(text) != std::string::npos;
    }
};

// AmiBlitz3 refuses a cell 1.95 compiles (measured: a fraction constant
// shifted under a Byte type; here made up for an agreement cell)
static const char *const AB3REF = "dl-mod-l-cb-A\t-\t1\t1\t1\t1\t!noncompile:Illegal operator for this type\t\t\t\t\n";

static const std::string SRC = "tests/corpus/tests-exprmatrix/", COR = "tests/bug-orig/exprmatrix/",
                         BUG = "tests/bug-orig/compiler/", REG = "tests/bug-orig/regression/",
                         AB3 = "tests/ab3/cli/exprmatrix/";

// the line of text in the file (1 = the first), 0: not there
static int line_of(const std::string &file, const std::string &text)
{
    size_t at = file.find(text);
    int line = 1;

    if (at == std::string::npos)
        return 0;
    for (size_t i = 0; i < at; i++)
        line += file[i] == '\n';
    return line;
}

TEST_CASE("exprmatrix expect: a cell per context in its model line", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW);

    REQUIRE(g.rc == 0);
    // a destination: the value checked under the cell's key, in the case
    // "cells" with its variables; a Quick destination by its raw bits
    {
        std::string b = g.file(COR + "dl-a-agree-01.test.bas");
        CHECK(b.find("; agreement cells") != std::string::npos);
        CHECK(b.find("!TestCase{cells}\n  vw.w=1000 : vl.l=100000\n  rl.l=0\n") != std::string::npos);
        CHECK(b.find("  rl=vw + vl : !CheckEq{rl,101000,\"dl-add-w-l-A\"}\n") != std::string::npos);
        CHECK(b.find("!EndTestCase\n") != std::string::npos);
        CHECK(b.find("NPrint") == std::string::npos);
    }
    CHECK(g.has(COR + "dq-a-agree-01.test.bas", "  rq=vq * vq : !CheckEq{Peek.l(&rq),$00024000,\"dq-mul-q-q-A\"}\n"));
    // a comparison in a destination keeps its parentheses
    CHECK(g.has(COR + "db-a-fix1177-01.test.bas", "  rb=(vq < vl) : !CheckEq{rb,-1,\"db-lt-q-l-A\"}\n"));
    // a parameter: the statement stores it in a global the case checks
    CHECK(g.has(COR + "pl-a-agree-01.test.bas",
                "DEFTYPE .l gl\nStatement pl{x.l}\n  SHARED gl\n  gl=x\nEnd Statement\n!TestCase{cells}\n"));
    CHECK(g.has(COR + "pl-a-agree-01.test.bas", "  pl{vw + vl} : !CheckEq{gl,101000,\"pl-add-w-l-A\"}\n"));
    CHECK(g.has(COR + "pq-a-base-01.test.bas", "  pq{vq + vl} : !CheckEq{Peek.l(&gq),$86A18000,\"pq-add-q-l-A\"}\n"));
    // If: 1 or 0
    CHECK(g.has(COR + "if-a-agree-01.test.bas", "  If vq - vq Then t=1 Else t=0\n  !CheckEq{t,0,\"if-sub-q-q-A\"}\n"));
    // an index unchecked: read into a Long; checked: behind SetErr, -1 the
    // runtime error, ClrErr at the end, runtime error checking on
    {
        std::string b = g.file(COR + "i2-i-c-agree-01.test.bas");
        CHECK(b.find("  vv=0 : SetErr : vv=-1 : Goto mx2 : End SetErr\n  vv=ja(1,vl * 40000)\n  mx2:\n"
                     "  !CheckEq{vv,-1,\"i2-mul-l-cl-I\"}\n  ClrErr\n!EndTestCase\n")
              != std::string::npos);
    }
    CHECK(g.has(COR + "i2-i-c-agree-01.test.toml", "runtime-errors = \"on\"\n"));
    CHECK(!g.has(COR + "i2-i-c-agree-01.test.toml", "isolate"));
    CHECK(g.has(BUG + "compiler-expr-1178-i1-i-u-01.test.bas", "  vv=ia(vq * vw) : !CheckEq{vv,481,\"i1-mul-q-w-I\"}\n"));
    CHECK(!g.has(BUG + "compiler-expr-1178-i1-i-u-01.test.bas", "SetErr"));
    // the unchecked index cells share sessions as the others (bundled)
    CHECK(!g.has(BUG + "compiler-expr-1178-i1-i-u-01.test.toml", "isolate"));
    CHECK(!g.has(COR + "i2-i-u-agree-01.test.toml", "isolate"));
    // Print: the lines printed, judged by the output
    CHECK(g.has(COR + "print-a-agree-01.test.bas", "  NPrint \"print-add-w-l-A \",vw + vl\n"));
    CHECK(g.has(COR + "print-a-agree-01.test.toml", "\n[asserts]\noutput = \"== \\\"print-add-w-l-A 101000\\n\\\"\"\n"));
    // an agreement source: no deviation, no fixed-orig
    CHECK(!g.has(COR + "dl-a-agree-01.test.toml", "deviations"));
    CHECK(!g.has(COR + "dl-a-agree-01.test.toml", "fixed-orig"));
    // every line fits TED (at most 128 characters)
    for (const auto &[path, text] : g.files) {
        if (path.size() > 5 && path.compare(path.size() - 5, 5, ".toml") == 0)
            continue;
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find('\n', start);
            if (end == std::string::npos)
                end = text.size();
            CHECK(end - start <= 128);
            start = end + 1;
        }
    }
    CHECK(g.files.count(SRC + "left-out.txt") == 1);
    CHECK(g.report.find("files ") != std::string::npos);
}

TEST_CASE("exprmatrix expect: deviations and fixed-orig", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW);

    REQUIRE(g.rc == 0);
    // a compiler fix in the corpus: the original's value in both cells of
    // 2.10 (both library sets), fixed-orig
    CHECK(g.file(COR + "db-a-fix1177-01.test.toml")
          == "# db-a-fix1177-01.test.bas: generated by exprmatrix expect from m.txt (tools/exprmatrix/README.md); "
             "do not edit\nfixed-orig = 1177\n\n[deviations.\"compiler=orig210\"]\n"
             "\"check.cells.db-lt-q-l-A\" = \"== fail\"\n\"check.cells.db-lt-q-l-A.actual\" = \"== 0\"\n");
    // a library fix (#234): the cells with the CD's libraries
    CHECK(g.has(COR + "print-a-fix234-01.test.toml", "fixed-orig = 234\n"));
    CHECK(g.has(COR + "print-a-fix234-01.test.toml",
                "\n[deviations.\"libs=cd\"]\noutput = \"== \\\"print-div-q-l-A -.\\n\\\"\"\n"));
    // a compiler fix in the bug-orig area: 1.95, the original's value
    CHECK(g.has(BUG + "compiler-expr-1178-dl-a-01.test.toml",
                "fixed-orig = 1178\n\n[deviations.\"compiler=orig195\"]\n\"check.cells.dl-mul-l-q-A\" = \"== fail\"\n"
                "\"check.cells.dl-mul-l-q-A.actual\" = \"== 100000\"\n"));
    CHECK(g.has(BUG + "compiler-expr-1178-dl-a-01.test.bas", "BUG(orig) (#1178)"));
    CHECK(g.has(BUG + "compiler-expr-1178-dl-a-01.test.bas", "; cells: dl-mul-l-q-A\n"));
    CHECK(!g.has(BUG + "compiler-expr-1178-dl-a-01.test.bas", "@fixed-orig"));
    // a cell whose fix is in the library (#1189 with a Float base): the CD's
    // libraries, the raw bits as a signed Long; the check has the bits with
    // the sign of the odd power
    CHECK(g.has(BUG + "compiler-expr-1189-df-b-01.test.bas", "!CheckEq{Peek.l(&rf),$F9FFFBC4,\"df-pow-f-b-B\"}"));
    CHECK(g.has(BUG + "compiler-expr-1189-df-b-01.test.toml",
                "\n[deviations.\"libs=cd\"]\n\"check.cells.df-pow-f-b-B\" = \"== fail\"\n"
                "\"check.cells.df-pow-f-b-B.actual\" = \"== -100664508\"\n"));
    // two issues: fixed-orig of the last of them
    CHECK(g.has(BUG + "compiler-expr-1178-1189-dl-b-01.test.toml", "fixed-orig = 1189\n"));
    CHECK(g.has(BUG + "compiler-expr-1178-1189-dl-b-01.test.bas", "!CheckEq{rl,-3,\"dl-pow-q-b-B\"}"));
    // no file of the old form
    for (const auto &[path, text] : g.files) {
        CHECK(path.find(".expect") == std::string::npos || path.compare(0, AB3.size(), AB3) == 0);
        CHECK(path.find(".out") == std::string::npos);
        CHECK(text.find("@fixed-orig") == std::string::npos);
        CHECK(text.find("@pass") == std::string::npos);
    }
}

TEST_CASE("exprmatrix expect: the refused cells as data sources", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW);

    REQUIRE(g.rc == 0);
    // refused by the originals and the port: a regression test per
    // operator, a data source per operand classes, its error at its line
    {
        std::string d = g.file(REG + "compiler-expr-refused-and.test/F-I.bas");
        int line = line_of(d, "rl=vf AND vb");

        REQUIRE(line > 0);
        CHECK(g.has(REG + "compiler-expr-refused-and.test.toml", "\n[data]\n\"F-I.bas\" = \"basic-compile\"\n"));
        CHECK(g.has(REG + "compiler-expr-refused-and.test.toml",
                    "\n[asserts]\n\"data.F-I.bas\" = \"== \\\"error " + std::to_string(line)
                        + " Illegal Operator for Type\\\"\"\n"));
        CHECK(!g.has(REG + "compiler-expr-refused-and.test.toml", "deviations"));
    }
    CHECK(g.has(REG + "compiler-expr-refused-and.test.bas", "!TestCase{cells}\n  !Check{1,\"driver\"}\n!EndTestCase\n"));
    // refused after #1188's fix: a bug-orig test per issue set, the
    // original's compile its deviation
    CHECK(g.has(BUG + "compiler-expr-1188-refused.test.bas", "BUG(orig) (#1188)"));
    CHECK(g.files.count(BUG + "compiler-expr-1188-refused.test/and-C-I.bas") == 1);
    CHECK(g.has(BUG + "compiler-expr-1188-refused.test.toml", "fixed-orig = 1188\n"));
    CHECK(g.has(BUG + "compiler-expr-1188-refused.test.toml",
                "\n[deviations.\"compiler=orig195\"]\n\"data.and-C-I.bas\" = \"== ok\"\n"));
}

TEST_CASE("exprmatrix expect: the AmiBlitz3 cases", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW + AB3REF);

    REQUIRE(g.rc == 0);
    // an agreement source: the cells AmiBlitz3 compiles, the port's output,
    // no file of AmiBlitz3's own where it gives the same
    {
        std::string b = g.file(AB3 + "dl-A-agree-01.ab3");
        CHECK(b.find("dl-add-w-l-A") != std::string::npos);
        CHECK(b.find("\"dl-mod-l-cb-A") == std::string::npos);
        // the header as one text (its lines are wrapped)
        for (size_t at; (at = b.find("\n; ")) != std::string::npos;)
            b.replace(at, 3, " ");
        CHECK(b.find("2 of its 3 cells, the 1 AmiBlitz3 refuses left out") != std::string::npos);
    }
    CHECK(g.file(AB3 + "dl-A-agree-01.expect")
          == "[compile]\nrc=0\n[run]\ndl-add-w-l-A 101000\ndl-mod-cl-cb-A 1\nrc=0\n");
    CHECK(g.file(AB3 + "dl-A-agree-01.ref.expect") == "(no file)");
    // a fixed source: AmiBlitz3 has the original's value (#1177)
    CHECK(g.file(AB3 + "db-A-fix1177-01.expect") == "[compile]\nrc=0\n[run]\ndb-lt-q-l-A -1\nrc=0\n");
    CHECK(g.file(AB3 + "db-A-fix1177-01.ref.expect") == "[compile]\nrc=0\n[run]\ndb-lt-q-l-A 0\nrc=0\n");
    // no case of a baseline or a checked source
    CHECK(g.file(AB3 + "dl-A-base-01.ab3") == "(no file)");
    CHECK(g.file(AB3 + "i2-I-c-agree-01.ab3") == "(no file)");
    // the case list for tests/ab3
    {
        std::string c = g.file(AB3 + "cases.cmake");
        CHECK(c.find("ab3_cli_case(dl-A-agree-01 dl-A-agree-01.ab3 DIR exprmatrix ARGS -r REF)\n") != std::string::npos);
        CHECK(c.find("ab3_cli_case(db-A-fix1177-01 db-A-fix1177-01.ab3 DIR exprmatrix ARGS -r REF REF_EXPECT "
                     "db-A-fix1177-01.ref.expect)\n")
              != std::string::npos);
        CHECK(c.find("dl-A-base-01") == std::string::npos);
    }
    CHECK(g.report.find("AmiBlitz3 cases ") != std::string::npos);
}

// T1 (quick-long-arithmetic, design decision 13): the cells of an issue
// fixed after the matrix stay in their bug-orig tests, with fixed-orig
TEST_CASE("exprmatrix expect: an issue fixed after the matrix", "[exprmatrix]")
{
    gen now(std::string(HEAD) + POW, EM_RULES_MATRIX);
    gen one(std::string(HEAD) + POW, EM_RULES_MATRIX | EM_R_1178);
    gen two(std::string(HEAD) + POW, EM_RULES_MATRIX | EM_R_1178 | EM_R_1189);
    gen ref(std::string(HEAD) + POW, EM_RULES_MATRIX | EM_R_1188);
    const std::string t = BUG + "compiler-expr-1178-dl-a-01", tq = BUG + "compiler-expr-1178-1189-dl-b-01";

    REQUIRE(now.rc == 0);
    REQUIRE(one.rc == 0);
    REQUIRE(two.rc == 0);
    REQUIRE(ref.rc == 0);
    // the port when the matrix was committed: no bug-orig test is marked
    // (an open bug: the deviations, no fixed-orig; the port with the CD's
    // libraries gives the original's value, so they are those of libs=cd)
    for (const auto &[path, text] : now.files)
        if (path.compare(0, BUG.size(), BUG) == 0)
            CHECK(text.find("fixed-orig") == std::string::npos);
    CHECK(now.has(t + ".test.toml", "[deviations.\"libs=cd\"]\n"));
    CHECK(one.has(t + ".test.toml", "[deviations.\"compiler=orig195\"]\n"));
    // one issue fixed: the same test with fixed-orig and the same source;
    // no corpus test of it
    CHECK(one.has(t + ".test.toml", "do not edit\nfixed-orig = 1178\n"));
    CHECK(one.file(t + ".test.bas") == now.file(t + ".test.bas"));
    // (the named cells of expr-tree-5 depend on the port's rules, not on the fix)
    auto others = [](const gen &g) {
        size_t k = 0;

        for (const auto &[path, text] : g.files)
            k += path.find("compiler-expr-1216") == std::string::npos;
        return k;
    };
    CHECK(others(one) == others(now));
    for (const auto &[path, text] : one.files)
        CHECK(path.find("fix1178") == std::string::npos);
    // a test of two issues, one fixed: kept, not marked
    CHECK(one.file(tq + ".test.bas") == now.file(tq + ".test.bas"));
    CHECK(!one.has(tq + ".test.toml", "fixed-orig"));
    // both fixed: marked by the last of them
    CHECK(two.has(tq + ".test.toml", "fixed-orig = 1189\n"));
    CHECK(two.has(t + ".test.toml", "fixed-orig = 1178\n"));
    // a cell refused after the fix
    CHECK(ref.has(BUG + "compiler-expr-1188-refused.test.toml", "fixed-orig = 1188\n"));
    CHECK(ref.has(BUG + "compiler-expr-1188-refused.test.toml", " Illegal Operator for Type\\\"\"\n"));
    CHECK(!now.has(BUG + "compiler-expr-1188-refused.test.toml", "fixed-orig"));
}

TEST_CASE("exprmatrix expect: a value the model does not know", "[exprmatrix]")
{
    gen g(HEAD);

    CHECK(g.rc == 1);
    CHECK(g.errors.find("df-pow-f-b-B: the model does not know its value and the measured table has no row")
          != std::string::npos);
    CHECK(g.files.empty());
}

TEST_CASE("exprmatrix expect: the files of its own", "[exprmatrix]")
{
    CHECK(em_expect_owns("tests/corpus/tests-exprmatrix/left-out.txt"));
    CHECK(!em_expect_owns("tests/corpus/tests-exprmatrix/matrix.txt"));
    CHECK(!em_expect_owns("tests/corpus/tests-exprmatrix/measured.tsv"));
    CHECK(em_expect_owns("tests/bug-orig/exprmatrix/dl-a-agree-01.test.bas"));
    CHECK(em_expect_owns("tests/bug-orig/exprmatrix/dl-a-agree-01.test.toml"));
    CHECK(!em_expect_owns("tests/bug-orig/exprmatrix/tests.toml"));
    CHECK(em_expect_owns("tests/bug-orig/compiler/compiler-expr-1178-dl-a-01.test.bas"));
    CHECK(em_expect_owns("tests/bug-orig/compiler/compiler-expr-1178-dl-a-01.test.toml"));
    CHECK(em_expect_owns("tests/bug-orig/compiler/compiler-expr-1188-refused.test/and-C-I.bas"));
    CHECK(!em_expect_owns("tests/bug-orig/compiler/compiler-expr-1188-refused.test/sub/and-C-I.bas"));
    CHECK(!em_expect_owns("tests/bug-orig/compiler/compiler-float-condition.test.bas"));
    CHECK(!em_expect_owns("tests/bug-orig/compiler/compiler-fold-buffer-full.test/child.bas"));
    CHECK(em_expect_owns("tests/bug-orig/regression/compiler-expr-refused-and.test.bas"));
    CHECK(em_expect_owns("tests/bug-orig/regression/compiler-expr-refused-and.test/F-I.bas"));
    CHECK(!em_expect_owns("tests/bug-orig/regression/compiler-expr-depth-d7.test.bas"));
    CHECK(em_expect_owns("tests/ab3/cli/exprmatrix/dl-A-agree-01.ab3"));
    CHECK(em_expect_owns("tests/ab3/cli/exprmatrix/cases.cmake"));
    CHECK(!em_expect_owns("tests/ab3/cli/atoi.ab3"));
}

TEST_CASE("exprmatrix expect: the named cells of the decided rule", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW);

    REQUIRE(g.rc == 0);
    // the named cells (the decided rule's value differs from the port's) as
    // bug-orig tests of #1216, open: no fixed-orig, the original's value the
    // deviation
    CHECK(g.has(SRC + "named-cells.tsv", "db-div-q-l-A\tdb\tarith\tvalue\t-1\t0\n"));
    const std::string v = BUG + "compiler-expr-1216-db-a-01";

    CHECK(g.has(v + ".test.bas", "BUG(orig) (#1216)"));
    CHECK(g.has(v + ".test.bas", "!CheckEq{rb,0,\"db-div-q-l-A\"}"));
    CHECK(g.has(v + ".test.toml", "\"check.cells.db-div-q-l-A.actual\" = \"== -1\"\n"));
    CHECK(!g.has(v + ".test.toml", "fixed-orig"));
    // a cell that compiles after the change: the whole source is refused by
    // the port and the original, the compile error the deviation
    const std::string n = BUG + "compiler-expr-1216-dl-a-now-01";

    CHECK(g.has(n + ".test.bas", "!CheckEq{rl,0,\"dl-and-l-cf-A\"}"));
    CHECK(g.has(n + ".test.toml", "compile = \"contains \\\"Illegal Operator for Type\\\"\"\n"));
    CHECK(g.has(n + ".test.toml", "rc = \"not exists\"\n"));
    CHECK(!g.has(n + ".test.toml", "fixed-orig"));
    // a cell refused after the change: a data source, the original's compile the deviation
    const std::string r = BUG + "compiler-expr-1216-refused";

    CHECK(g.has(r + ".test.toml", "\"data.dq-lsl-Q-C.bas\" = \"== \\\"error 7 Illegal Operator for Type\\\"\"\n"));
    CHECK(g.has(r + ".test.toml", "\"data.dq-lsl-Q-C.bas\" = \"== ok\"\n"));
    CHECK(g.has(r + ".test/dq-lsl-Q-C.bas", "rq=vq LSL 2.5"));
    // a cell the original gives a value (the port refuses it): left out, with its reason
    CHECK(g.has(SRC + "left-out.txt", "db-and-cf-b-A\tnamed cell, compiles now: the port refuses it, the original "
                                      "gives 1"));
    CHECK(g.report.find("named cells in tests: with another value 1, compiling after the change 1, refused after "
                        "the change 1; left out 1") != std::string::npos);
    // with the decided rule as the port's rules there is no named cell and no test
    gen d(std::string(HEAD) + POW, EM_RULES_DECIDED);

    REQUIRE(d.rc == 0);
    for (const auto &[path, text] : d.files)
        CHECK(path.find("compiler-expr-1216") == std::string::npos);
    CHECK(d.report.find("named cells of the decided rule 0\n") != std::string::npos);
}
