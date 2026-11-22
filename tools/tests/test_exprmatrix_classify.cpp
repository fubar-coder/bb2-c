// test_exprmatrix_classify - the classes of the expression matrix
// (lib/exprmatrix, classify.c): a recorded small cell table (rows of the
// measurement of task 1.3, one cell made up for the class "new") gives
// every class at least once; a broken output line, a cell measured twice
// or not at all, and a key that is no cell are refused.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprmatrix.h"
}

static const char *const DESC =
    "values A b=3 w=1000 l=100000 q=1.5 f=2.5 cb=3 cw=1000 cl=100000 cf=1.5\n"
    "counts A b=2 w=2 l=2 q=2 f=2 cb=2 cw=2 cl=2 cf=2.5\n"
    "values I b=3 w=20 l=30 q=1.5 f=2.5 cb=3 cw=200 cl=40000 cf=1.5\n"
    "block t\n"
    "cell A dl + w l\ncell A db < q l\ncell A dl * l q\ncell A db = cf cf\ncell A if - f\n"
    "cell A dl * l l\ncell A dl AND f b\ncell A dl + b b\ncell A db + cf q\ncell I i1 * q w\n"
    "cell A dl MOD l q\ncell A dl ^ w cb\ncell A dl AND q w\n";

static const char *const HEAD = "key\tsource\tc210\tc195\tportcd\tclib\tab3\tc210_on\tc195_on\tportcd_on\tclib_on\n";

// the measured rows (task 1.3; dl-add-b-b-A made up: 7 instead of 6)
static const char *const ROWS =
    "dl-add-w-l-A\tt-01\t101000\t101000\t101000\t101000\t101000\t\t\t\t\n"
    "db-lt-q-l-A\tt-01\t0\t0\t-1\t-1\t0\t\t\t\t\n"
    "dl-mul-l-q-A\tt-01\t100000\t100000\t100000\t100000\t100000\t\t\t\t\n"
    "db-eq-cf-cf-A\tt-01\t-1\t-1\t0\t0\t-1\t\t\t\t\n"
    "if-neg-f-A\tt-01\tF\tF\tF\tF\tF\t\t\t\t\n"
    "dl-mul-l-l-A\tt-01\t1410065408\t1410065408\t1410065408\t1410065408\t1410065408\t\t\t\t\n"
    "dl-and-f-b-A\tt-01\t!noncompile:Illegal Operator for Type\t!noncompile:Illegal Operator for Type\t"
    "!noncompile:Illegal Operator for Type\t!noncompile:Illegal Operator for Type\t"
    "!noncompile:Illegal operator for this type\t\t\t\t\n"
    "dl-add-b-b-A\tt-01\t7\t7\t7\t7\t7\t\t\t\t\n"
    "db-add-cf-q-A\tt-01\t2\t2\t2\t2\t3\t\t\t\t\n"
    "i1-mul-q-w-I\tt-02\t321\t321\t321\t321\t321\t321\t321\t321\t321\n"
    "dl-mod-l-q-A\tt-01\t0\t0\t0\t0\t0\t\t\t\t\n"
    "dl-pow-w-cb-A\tt-01\t999999\t999999\t999999\t999999\t999999\t\t\t\t\n"
    "dl-and-q-w-A\tt-01\t0\t0\t0\t0\t0\t\t\t\t\n";

struct run {
    struct em_matrix m;
    struct em_table t;
    struct em_classified c;
    int parse = -9, cls = -9;
    std::string errors;

    explicit run(const std::string &table)
    {
        char *e = nullptr;

        std::memset(&c, 0, sizeof c);
        std::memset(&t, 0, sizeof t);
        if (em_parse(DESC, std::strlen(DESC), "t.txt", &m, &e)) {
            std::fprintf(stderr, "em_parse: %s", e ? e : "");
            std::free(e);
            return;
        }
        parse = em_table_parse(table.data(), table.size(), "cells.tsv", &t, &e);
        if (parse) {
            errors = e ? e : "";
            std::free(e);
            return;
        }
        cls = em_classify(&m, &t, &c, &e);
        if (cls)
            errors = e ? e : "";
        std::free(e);
    }
    ~run()
    {
        em_classified_free(&c);
        em_table_free(&t);
        em_free(&m);
    }

    // the class field (and the question class) of a key in the cell table
    std::string cls_of(const char *key) const
    {
        std::string tab = c.table ? c.table : "", k = std::string("\n") + key + "\t";
        size_t at = tab.find(k);

        if (at == std::string::npos)
            return "(none)";
        at += k.size();
        size_t t1 = tab.find('\t', at), t2 = tab.find('\t', t1 + 1), t3 = tab.find('\t', t2 + 1);
        std::string cl = tab.substr(at, t1 - at), q = tab.substr(t2 + 1, t3 - t2 - 1);

        return q.empty() ? cl : cl + " " + q;
    }
};

TEST_CASE("exprmatrix classify: every class", "[exprmatrix]")
{
    run r(std::string(HEAD) + ROWS);

    REQUIRE(r.parse == 0);
    REQUIRE(r.cls == 0);
    CHECK(r.cls_of("dl-add-w-l-A") == "agree");
    CHECK(r.cls_of("db-lt-q-l-A") == "fixed-1177");
    CHECK(r.cls_of("dl-mul-l-q-A") == "open-1178");
    CHECK(r.cls_of("db-eq-cf-cf-A") == "port");
    CHECK(r.cls_of("if-neg-f-A") == "fixed-1187");
    CHECK(r.cls_of("dl-mul-l-l-A") == "question Q-A");
    CHECK(r.cls_of("dl-and-q-w-A") == "question Q-C");
    CHECK(r.cls_of("dl-mod-l-q-A") == "question Q-D");
    CHECK(r.cls_of("dl-pow-w-cb-A") == "question Q-E");
    CHECK(r.cls_of("db-add-cf-q-A") == "question Q-F");
    CHECK(r.cls_of("dl-and-f-b-A") == "noncompile");
    CHECK(r.cls_of("dl-add-b-b-A") == "new");
    CHECK(r.cls_of("i1-mul-q-w-I") == "open-1178");
    // the made-up cell is the one mismatch of the original's rules
    CHECK(r.c.mismatches == 1);
    CHECK(std::string(r.c.report).find("dl-add-b-b-A\tmodel 6\t1.95 7") != std::string::npos);
    CHECK(std::string(r.c.report).find("class agree 1\n") != std::string::npos);
    // the questions: one section per class, with its examples
    std::string q = r.c.questions;
    CHECK(q.find("## Q-A ") != std::string::npos);
    CHECK(q.find("## Q-F ") != std::string::npos);
    CHECK(q.find("## Q-B ") == std::string::npos);
    CHECK(q.find("| `dl-mul-l-l-A` | 1410065408 |") != std::string::npos);
}

TEST_CASE("exprmatrix classify: broken tables", "[exprmatrix]")
{
    // a broken output line: too few fields
    {
        run r(std::string(HEAD) + ROWS + "dl-add-w-w-A\tt-01\t2000\n");
        CHECK(r.parse == 1);
        CHECK(r.errors.find("cells.tsv:15: 3 fields, wanted 11") != std::string::npos);
    }
    // a value no compiler prints
    {
        std::string rows = ROWS;
        rows.replace(rows.find("101000\t101000\t101000\t101000\t101000"), 6, "!bogus");
        run r(std::string(HEAD) + rows);
        CHECK(r.parse == 1);
        CHECK(r.errors.find("cells.tsv:2: c210: bad value \"!bogus\"") != std::string::npos);
    }
    // a value with a blank in it
    {
        std::string rows = ROWS;
        rows.replace(rows.find("\t7\t7"), 2, "\t7 x");
        run r(std::string(HEAD) + rows);
        CHECK(r.parse == 1);
        CHECK(r.errors.find("bad value \"7 x\"") != std::string::npos);
    }
    // a key that is no cell, a cell measured twice, a cell not measured
    {
        std::string rows = ROWS;
        rows.erase(rows.find("dl-and-q-w-A"));
        rows += "dl-sub-w-l-A\tt-01\t1\t1\t1\t1\t1\t\t\t\t\n";
        rows += "dl-add-w-l-A\tt-01\t101000\t101000\t101000\t101000\t101000\t\t\t\t\n";
        run r(std::string(HEAD) + rows);
        CHECK(r.parse == 0);
        CHECK(r.cls == 1);
        CHECK(r.errors.find("dl-sub-w-l-A: not a cell of the description") != std::string::npos);
        CHECK(r.errors.find("dl-add-w-l-A: measured twice") != std::string::npos);
        CHECK(r.errors.find("dl-and-q-w-A: not measured") != std::string::npos);
    }
}
