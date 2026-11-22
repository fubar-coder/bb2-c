// test_exprmatrix_expect - the committed tests of the expression matrix
// (lib/exprmatrix, expect.c): a small description with a cell of every
// kind of file (an agreement, a baseline, a fixed and an open cell, one of
// two issues' values, a refused one, one refused after a fix, an index
// cell unchecked and checked) gives the files with their names, headers
// and expected outputs, and the AmiBlitz3 cases of the agreement and
// fixed sources; a value the model does not know needs its measured row;
// the generator's own files are recognised by their path.
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
    "cell A db MOD cb cl\ncell A dl MOD cl cb\ncell A dl MOD l cb\n";

static const char *const HEAD = "key\tsource\tc210\tc195\tportcd\tclib\tab3\tc210_on\tc195_on\tportcd_on\tclib_on\n";
// the Float power the model does not compute: 1.95's bits of (-2.5)^3
static const char *const POW = "df-pow-f-b-B\t-\tF9FFFB44\tF9FFFB44\tF9FFFB44\tF9FFFB44\tF9FFFB44\t\t\t\t\n";

struct gen {
    struct em_matrix m;
    std::map<std::string, std::string> files;
    std::string report, errors;
    int rc = -9;

    explicit gen(const std::string &measured)
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
        rc = em_expect(&m, "m.txt", &t, &f, &n, &rep, &e);
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

static const std::string SRC = "tests/corpus/tests-exprmatrix/", EXP = "tests/corpus/expect/corpus_exprmatrix/",
                         EXPR = "tests/corpus/expect/corpus_exprmatrix_runerr/", BUG = "tests/bug-orig/compiler/",
                         REG = "tests/bug-orig/regression/", AB3 = "tests/ab3/cli/exprmatrix/";

TEST_CASE("exprmatrix expect: the files of every kind", "[exprmatrix]")
{
    gen g(std::string(HEAD) + POW);

    REQUIRE(g.rc == 0);
    // agreement and baseline cells: corpus sources with one expected file
    CHECK(g.file(EXP + "dl-A-agree-01.out") == "dl-add-w-l-A 101000\ndl-mod-cl-cb-A 1\ndl-mod-l-cb-A 1\nrc=0\n");
    CHECK(g.has(SRC + "dl-A-agree-01.bas", "; agreement cells"));
    CHECK(g.has(EXP + "dl-A-base-01.out", "dl-mul-l-l-A 1410065408\n"));
    CHECK(g.has(SRC + "dl-A-base-01.bas", "; baseline cells (the original's measured value)"));
    // the folded constant MOD (a Byte one) after the Long one (#1049); a
    // Long constant MOD is not folded and keeps its place
    {
        std::string b = g.file(SRC + "db-A-agree-01.bas");
        CHECK(b.find("db-mod-cb-cb-A") != std::string::npos);
        CHECK(b.find("db-mod-cb-cl-A") < b.find("db-mod-cb-cb-A"));
    }
    // a fixed cell (#1177 in the port): the pair
    CHECK(g.file(EXP + "db-A-fix1177-01.orig.out") == "db-lt-q-l-A 0\nrc=0\n");
    CHECK(g.file(EXP + "db-A-fix1177-01.fixed-1177.out") == "db-lt-q-l-A -1\nrc=0\n");
    CHECK(g.file(EXP + "db-A-fix1177-01.out") == "(no file)");
    // open cells: bug-orig tests with the expected values
    CHECK(g.has(BUG + "compiler-expr-1178-dl-A-01.bas", "BUG(orig) (#1178)"));
    CHECK(g.has(BUG + "compiler-expr-1178-dl-A-01.bas", "; cells: dl-mul-l-q-A\n"));
    CHECK(g.file(BUG + "compiler-expr-1178-dl-A-01.expect") == "[compile]\nok\n[run]\ndl-mul-l-q-A 150000\nrc=0\n");
    CHECK(g.has(BUG + "compiler-expr-1189-dl-B-01.expect", "dl-pow-f-b-B -15\n"));
    // ... the measured Float bits with the sign of the odd power
    CHECK(g.has(BUG + "compiler-expr-1189-df-B-01.expect", "df-pow-f-b-B F9FFFBC4\n"));
    // an index cell, unchecked and checked (SetErr, runtime error checking)
    CHECK(g.has(BUG + "compiler-expr-1178-i1-I-u-01.expect", "i1-mul-q-w-I 481\n"));
    CHECK(g.has(BUG + "compiler-expr-1178-i1-I-c-01.bas", "; @pass runtime_errors=on\n"));
    CHECK(g.has(BUG + "compiler-expr-1178-i1-I-c-01.bas",
                "SetErr : NPrint \"i1-mul-q-w-I !runtime\" : Goto mx1 : End SetErr\n"));
    CHECK(!g.has(BUG + "compiler-expr-1178-i1-I-u-01.bas", "SetErr"));
    // a refused cell: one representative, its line in the expectation
    {
        std::string b = g.file(REG + "compiler-expr-refused-and-F-I.bas");
        size_t at = b.find("rl=vf AND vb");
        REQUIRE(at != std::string::npos);
        int line = 1;
        for (size_t i = 0; i < at; i++)
            line += b[i] == '\n';
        CHECK(g.file(REG + "compiler-expr-refused-and-F-I.expect")
              == "[compile]\nerror " + std::to_string(line) + " Illegal Operator for Type\n");
    }
    // a cell refused after #1188's fix: a bug-orig test that expects the refusal
    CHECK(g.has(BUG + "compiler-expr-1188-refused-and-C-I.expect", "Illegal Operator for Type"));
    CHECK(g.has(BUG + "compiler-expr-1188-refused-and-C-I.bas", "BUG(orig) (#1188)"));
    // every header line fits TED (at most 128 characters)
    for (const auto &[path, text] : g.files) {
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
    CHECK(g.file(AB3 + "i1-I-c-agree-01.ab3") == "(no file)");
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
    CHECK(em_expect_owns("tests/corpus/tests-exprmatrix/dl-A-agree-01.bas"));
    CHECK(em_expect_owns("tests/corpus/tests-exprmatrix/left-out.txt"));
    CHECK(!em_expect_owns("tests/corpus/tests-exprmatrix/matrix.txt"));
    CHECK(!em_expect_owns("tests/corpus/tests-exprmatrix/measured.tsv"));
    CHECK(em_expect_owns("tests/corpus/expect/corpus_exprmatrix/dl-A-agree-01.out"));
    CHECK(em_expect_owns("tests/corpus/expect/corpus_exprmatrix_runerr/i1-I-c-agree-01.out"));
    CHECK(!em_expect_owns("tests/corpus/expect/corpus/t01_int.out"));
    CHECK(em_expect_owns("tests/bug-orig/compiler/compiler-expr-1178-dl-A-01.bas"));
    CHECK(em_expect_owns("tests/bug-orig/compiler/compiler-expr-1178-dl-A-01.expect"));
    CHECK(!em_expect_owns("tests/bug-orig/compiler/compiler-expr-1178-dl-A-01.orig"));
    CHECK(!em_expect_owns("tests/bug-orig/compiler/compiler-float-condition.bas"));
    CHECK(em_expect_owns("tests/bug-orig/regression/compiler-expr-refused-and-F-I.bas"));
    CHECK(!em_expect_owns("tests/bug-orig/regression/compiler-expr-depth-d7.bas"));
    CHECK(em_expect_owns("tests/ab3/cli/exprmatrix/dl-A-agree-01.ab3"));
    CHECK(em_expect_owns("tests/ab3/cli/exprmatrix/cases.cmake"));
    CHECK(!em_expect_owns("tests/ab3/cli/atoi.ab3"));
}
