// test_exprmatrix_model - the model of the expression matrix
// (lib/exprmatrix, model.c): the measured rows of
// docs/blitz2/expression-types.md as cases, each by the original's rules
// (blitz2 1.95, the CD's libraries) and by the rules with the fixes
// (#1177, #1178 variant C, #1179, #1186, #1187, #1188, #1189, the C libraries),
// and the flags of the question classes (design decision 7 of the OpenSpec
// change expression-type-matrix).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprmatrix.h"
}

// value sets of the rows: F and Q the Float/Quick table (V = -1.5, l = 3),
// M the measured examples (q = 1.5, l = 10), N the comparisons, A the
// matrix's value set A
static const char *const DESC =
    "values F f=-1.5 l=3 cb=2\n"
    "values FI f=-1.5 cb=3\n"
    "values Q q=-1.5 l=3 cb=2\n"
    "values QI q=-1.5 cb=3\n"
    "values M q=1.5 l=10 cb=2 cf=2.0\n"
    "values MI q=1.5 cb=10\n"
    "values N q=1.5 l=65537 cb=1\n"
    "values A b=3 w=1000 l=100000 q=1.5 f=2.5 cb=3 cw=1000 cl=100000 cf=1.5\n"
    "counts A b=2 w=2 l=2 q=2 f=2 cb=2 cw=2 cl=2 cf=2.5\n"
    "values B b=-3 f=-2.5 w=-20 l=-100000 cf=-1.5\n"
    "counts B b=3 w=2 cf=3.5\n"
    "values J f=-2.5\n"
    "counts J b=3\n"
    "block rows\n"
    // the Float column
    "cell F dl - f\ncell F dw - f\ncell F dl * f cb\ncell F dl / f cb\n"
    "cell F dl * l f\ncell F dl / l f\ncell F dl + l f\ncell FI i1 + f cb\n"
    // the Quick column
    "cell Q dl - q\ncell Q dw - q\ncell Q dl * q cb\ncell Q dl / q cb\n"
    "cell Q dl * l q\ncell Q dl / l q\ncell Q dl + l q\ncell QI i1 + q cb\n"
    // the measured examples
    "cell M dl * q cb\ncell M dl - l q\ncell M dl * cf q\ncell MI i1 * q cb\ncell MI i2 * q cb\n"
    "cell N dl > q cb\ncell N if > q l\n"
    // the fixed bugs and the libraries
    "cell A db + cf cf\ncell A dw + cf cf\ncell A db = cf cf\ncell A if - f\ncell A if + b f\n"
    "cell A dq * w w\ncell A print / q l\ncell A db AND cf b\n"
    "cell A print + w w\ncell A print + b b\ncell A print + b f\ncell A if > l q\n"
    "cell B dl ^ f b\ncell B df ^ f b\ncell B dl ^ b w\ncell B dl ^ f cf\ncell J i1 ^ f b\ncell A if ^ b b\n"
    "cell B dl ^ l b\ncell B dw ^ l b\n"
    // the question classes
    "cell A dl * l l\ncell A dq AND q w\ncell A dl MOD l q\ncell A dl ^ w cb\ncell A dl AND f b\n";

struct model {
    struct em_matrix m;

    model()
    {
        char *errors = nullptr;
        int r = em_parse(DESC, std::strlen(DESC), "rows.txt", &m, &errors);

        if (r)
            std::fprintf(stderr, "em_parse: %d\n%s", r, errors ? errors : "");
        std::free(errors);
    }
    ~model() { em_free(&m); }

    const struct em_cell *cell(const char *key) const
    {
        for (size_t i = 0; i < m.ncells; i++)
            if (!std::strcmp(m.cells[i].key, key))
                return &m.cells[i];
        std::fprintf(stderr, "no cell %s\n", key);
        return nullptr;
    }

    // the value of key by rules (checked: runtime error checking on)
    std::string value(const char *key, unsigned rules, int checked = 0, unsigned *flags = nullptr) const
    {
        const struct em_cell *c = cell(key);
        struct em_value v;

        if (!c)
            return "(no cell)";
        em_model(&m, c, rules, checked, &v);
        if (flags)
            *flags = v.flags;
        return v.text;
    }
};

static const unsigned ORIG = EM_RULES_ORIG, FIXED = EM_RULES_FIXED;

TEST_CASE("exprmatrix model: Float to whole numbers (SPFix, towards zero)", "[exprmatrix]")
{
    model md;

    // expression-types.md, "Converting to a whole number", the Float column
    for (unsigned rules : { ORIG, FIXED }) {
        CHECK(md.value("dl-neg-f-F", rules) == "1");
        CHECK(md.value("dw-neg-f-F", rules) == "1");
        CHECK(md.value("dl-mul-f-cb-F", rules) == "-3");
        CHECK(md.value("dl-div-f-cb-F", rules) == "0");
        CHECK(md.value("dl-mul-l-f-F", rules) == "-4");
        CHECK(md.value("dl-div-l-f-F", rules) == "-2");
        CHECK(md.value("dl-add-l-f-F", rules) == "1");
        // r.l=a(V+3): element 1 (here i*16+1), checked and unchecked
        CHECK(md.value("i1-add-f-cb-FI", rules) == "17");
        CHECK(md.value("i1-add-f-cb-FI", rules, 1) == "17");
    }
}

TEST_CASE("exprmatrix model: the Quick column, cut at fetch and variant C", "[exprmatrix]")
{
    model md;

    // the original: the cut (towards minus infinity) before the operation
    CHECK(md.value("dl-neg-q-Q", ORIG) == "2");
    CHECK(md.value("dw-neg-q-Q", ORIG) == "1");
    CHECK(md.value("dl-mul-q-cb-Q", ORIG) == "-4");
    CHECK(md.value("dl-div-q-cb-Q", ORIG) == "-1");
    CHECK(md.value("dl-mul-l-q-Q", ORIG) == "-6");
    CHECK(md.value("dl-div-l-q-Q", ORIG) == "-1");
    CHECK(md.value("dl-add-l-q-Q", ORIG) == "1");
    CHECK(md.value("i1-add-q-cb-QI", ORIG) == "17");
    // #1178 variant C: computed with the fraction; / towards zero, the
    // rest down ("#1178 rounding rule decided")
    CHECK(md.value("dl-neg-q-Q", FIXED) == "1");
    CHECK(md.value("dw-neg-q-Q", FIXED) == "1");
    CHECK(md.value("dl-mul-q-cb-Q", FIXED) == "-3");
    CHECK(md.value("dl-div-q-cb-Q", FIXED) == "0");
    CHECK(md.value("dl-mul-l-q-Q", FIXED) == "-5");
    CHECK(md.value("dl-div-l-q-Q", FIXED) == "-2");
    CHECK(md.value("dl-add-l-q-Q", FIXED) == "1");
    CHECK(md.value("i1-add-q-cb-QI", FIXED) == "17");
}

TEST_CASE("exprmatrix model: the measured examples", "[exprmatrix]")
{
    model md;

    CHECK(md.value("dl-mul-q-cb-M", ORIG) == "2");
    CHECK(md.value("dl-mul-q-cb-M", FIXED) == "3");
    CHECK(md.value("dl-sub-l-q-M", ORIG) == "9");
    CHECK(md.value("dl-sub-l-q-M", FIXED) == "8");
    CHECK(md.value("dl-mul-cf-q-M", ORIG) == "3");      // a Float is not cut
    CHECK(md.value("dl-mul-cf-q-M", FIXED) == "3");
    // a(q*10): element 10 in the original, 15 fixed
    CHECK(md.value("i1-mul-q-cb-MI", ORIG) == "161");
    CHECK(md.value("i1-mul-q-cb-MI", FIXED) == "241");
    // b(1,q*10): 15 unchecked (a Word: no cut), 10 checked (#1179)
    CHECK(md.value("i2-mul-q-cb-MI", ORIG) == "241");
    CHECK(md.value("i2-mul-q-cb-MI", ORIG, 1) == "161");
    CHECK(md.value("i2-mul-q-cb-MI", FIXED) == "241");
    CHECK(md.value("i2-mul-q-cb-MI", FIXED, 1) == "241");
    // comparisons (#1177): r.l=(q>1), If q>b with b.l=65537
    CHECK(md.value("dl-gt-q-cb-N", ORIG) == "0");
    CHECK(md.value("dl-gt-q-cb-N", EM_RULES_PORT) == "-1");
    CHECK(md.value("dl-gt-q-cb-N", FIXED) == "-1");
    CHECK(md.value("if-gt-q-l-N", ORIG) == "T");
    CHECK(md.value("if-gt-q-l-N", FIXED) == "F");
}

TEST_CASE("exprmatrix model: the fixed bugs and the libraries", "[exprmatrix]")
{
    model md;
    unsigned f = 0;

    // #1188: under a Byte destination 1.5+1.5 is 1+1; under a Word 3
    CHECK(md.value("db-add-cf-cf-A", ORIG) == "2");
    CHECK(md.value("db-add-cf-cf-A", FIXED) == "3");
    CHECK(md.value("dw-add-cf-cf-A", ORIG) == "3");
    // ... and refuses AND with it, as under a Word (a question, Q-C)
    CHECK(md.value("db-and-cf-b-A", ORIG) == "1");
    CHECK(md.value("db-and-cf-b-A", FIXED, 0, &f) == "!noncompile");
    CHECK((f & EM_F_BITQ));
    // #1186: the port as measured compared 1 with 1.5
    CHECK(md.value("db-eq-cf-cf-A", ORIG) == "-1");
    CHECK(md.value("db-eq-cf-cf-A", EM_RULES_PORT) == "0");
    CHECK(md.value("db-eq-cf-cf-A", EM_RULES_PORT | EM_R_1186) == "-1");
    CHECK(md.value("db-eq-cf-cf-A", FIXED) == "-1");
    // #1187: a Float condition of the original is what d1 holds
    CHECK(md.value("if-neg-f-A", ORIG, 0, &f) == "?");
    CHECK((f & EM_F_D1));
    CHECK(md.value("if-neg-f-A", FIXED) == "T");
    CHECK(md.value("if-add-b-f-A", FIXED) == "T");
    // #199: the CD's quickmult beyond the Quick range
    CHECK(md.value("dq-mul-w-w-A", ORIG) == "4240000F");
    CHECK(md.value("dq-mul-w-w-A", FIXED) == "42400000");
    // #234: a Quick that rounds to 0 ("-." with the CD's printlib)
    CHECK(md.value("print-div-q-l-A", ORIG) == "-.");
    CHECK(md.value("print-div-q-l-A", FIXED) == "0");
    // #1189: SPPow drops the sign of a negative base; fixed, an odd power
    // keeps it, an even one is positive, a fractional exponent has no rule
    CHECK(md.value("dl-pow-f-b-B", ORIG) == "15");
    CHECK(md.value("dl-pow-f-b-B", FIXED) == "-15");
    CHECK(md.value("df-pow-f-b-B", ORIG) == "?");
    CHECK(md.value("df-pow-f-b-B", FIXED, 0, &f) == "?-");
    CHECK((f & EM_F_POWSIGN));
    CHECK(md.value("dl-pow-b-w-B", FIXED, 0, &f) == "?");
    CHECK(!(f & EM_F_POWSIGN));
    md.value("dl-pow-f-cf-B", FIXED, 0, &f);
    CHECK((f & EM_F_POWNEG));
    md.value("dl-pow-f-cf-B", ORIG, 0, &f);
    CHECK(!(f & EM_F_POWNEG));
    // ... and a power outside the Long range saturates, not symmetrically:
    // 100000^3 is $7FFFFFFF, -(100000^3) $80000000 (0 as a Word or Byte)
    CHECK(md.value("dl-pow-l-b-B", ORIG) == "2147483647");
    CHECK(md.value("dl-pow-l-b-B", FIXED) == "-2147483648");
    CHECK(md.value("dw-pow-l-b-B", ORIG) == "-1");
    CHECK(md.value("dw-pow-l-b-B", FIXED) == "0");
    // ... and a negative power is outside the array
    CHECK(md.value("i1-pow-f-b-J", ORIG, 1) == "241");
    CHECK(md.value("i1-pow-f-b-J", FIXED, 1) == "!runtime");
    // #1187 and a power: the original's If tests d1, the fixed one the
    // power's value, which is not 0
    CHECK(md.value("if-pow-b-b-A", ORIG, 0, &f) == "?");
    CHECK((f & EM_F_D1));
    CHECK(md.value("if-pow-b-b-A", FIXED) == "T");
}

TEST_CASE("exprmatrix model: the flags of the question classes", "[exprmatrix]")
{
    model md;
    unsigned f = 0;

    CHECK(md.value("dl-mul-l-l-A", FIXED, 0, &f) == "1410065408");
    CHECK((f & EM_F_OVF));
    md.value("dq-and-q-w-A", FIXED, 0, &f);
    CHECK((f & EM_F_BITQ));
    md.value("dl-mod-l-q-A", FIXED, 0, &f);
    CHECK((f & EM_F_MODNF));
    md.value("dl-pow-w-cb-A", FIXED, 0, &f);
    CHECK((f & EM_F_POWINT));
    CHECK((f & EM_F_POW));
    CHECK(md.value("dl-and-f-b-A", ORIG) == "!noncompile");
    // an exact cell raises none
    CHECK(md.value("dl-mul-q-cb-M", FIXED, 0, &f) == "3");
    CHECK(f == 0);
}

// ----------------------------------------------------------------------------
// expression trees and the candidate rules of expr-tree-5 (task 2.1)

// the full product of the committed matrix (tests/corpus/tests-exprmatrix/
// matrix.txt) without the index blocks: the flow over a tree must give
// every cell the value of the cell's own model
static const char *const FULL =
    "values A b=3 w=1000 l=100000 q=1.5 f=2.5 s=AB cb=3 cw=1000 cl=100000 cf=1.5\n"
    "counts A b=2 w=2 l=2 q=2 f=2 cb=2 cw=2 cl=2 cf=2.5\n"
    "values B b=-3 w=-1000 l=-100000 q=-1.5 f=-2.5 cb=-3 cw=-1000 cl=-100000 cf=-1.5\n"
    "counts B b=3 w=3 l=3 q=3 f=3 cb=3 cw=3 cl=3 cf=3.5\n"
    "block f\n"
    "contexts db dw dl dq df pl pw pq pf if print\n"
    "binary + - * / MOD ^ AND OR EOR LSL LSR ASL ASR = <> < <= > >=\n"
    "unary - NOT\n"
    "kinds b w l q f cb cw cl cf\n"
    "sets A B\n";

struct expr {
    struct em_expr *x = nullptr;

    expr(const char *text, const char *vars)
    {
        char *err = nullptr;
        int r = em_expr_parse(text, vars, &x, &err);

        if (r)
            std::fprintf(stderr, "em_expr_parse %s: %s\n", text ? text : "", err ? err : "out of memory");
        std::free(err);
    }
    ~expr() { em_expr_free(x); }

    std::string value(enum em_ctx ctx, unsigned rules) const
    {
        struct em_exprval v;

        if (!x)
            return "(not parsed)";
        em_expr_eval(x, ctx, rules, &v);
        return v.v.text;
    }
    std::string typing(enum em_ctx ctx, unsigned rules) const
    {
        struct em_exprval v;

        if (!x)
            return "(not parsed)";
        em_expr_eval(x, ctx, rules, &v);
        return v.sig;
    }
};

static const unsigned NOW = EM_RULES_CANDBASE;

static unsigned cand(int n, bool scopeb = false)
{
    return NOW | EM_R_CAND(n) | (scopeb ? EM_R_SCOPEB : 0);
}

TEST_CASE("exprmatrix model: the flow over a tree is the model of every cell", "[exprmatrix][cand]")
{
    struct em_matrix m;
    char *errors = nullptr;
    size_t compared = 0;

    REQUIRE(em_parse(FULL, std::strlen(FULL), "full.txt", &m, &errors) == 0);
    for (unsigned rules : { (unsigned)EM_RULES_ORIG, NOW }) {
        for (size_t i = 0; i < m.ncells; i++) {
            const struct em_cell *c = &m.cells[i];
            struct em_expr *x = nullptr;
            struct em_value v;
            struct em_exprval t;

            if (em_cell_expr(&m, c, &x))
                continue;
            em_model(&m, c, rules, 0, &v);
            em_expr_eval(x, c->ctx, rules, &t);
            em_expr_free(x);
            compared++;
            if (std::strcmp(v.text, t.v.text))
                FAIL_CHECK(std::string(c->key) + ": cell " + v.text + ", tree " + t.v.text);
        }
    }
    CHECK(compared > 60000);
    em_free(&m);
}

TEST_CASE("exprmatrix model: expressions are parsed with blitz2's precedences", "[exprmatrix][cand]")
{
    // comparisons below + and *, AND below comparisons, & and MOD above *
    CHECK(expr("1+2*3", "").value(EM_DL, NOW) == "7");
    CHECK(expr("10-2-3", "").value(EM_DL, NOW) == "5");
    CHECK(expr("2*3>5 AND 1<2", "").value(EM_DL, NOW) == "-1");
    CHECK(expr("2*7 & 3", "").value(EM_DL, NOW) == "6");
    CHECK(expr("(1+2)*3", "").value(EM_DL, NOW) == "9");
    CHECK(expr("-a+b", "a.w=3 b.w=5").value(EM_DL, NOW) == "2");
    CHECK(expr("$10+%11", "").value(EM_DL, NOW) == "19");
    // the typing of each operation: q raw under the Long, the constant (k) a
    // Long, a mixed product
    CHECK(expr("q*2", "q=1.5").typing(EM_DL, NOW) == "mul Q>~ k>L m L;");

    struct em_expr *x = nullptr;
    char *err = nullptr;
    CHECK(em_expr_parse("a+", "a=1", &x, &err) == 1);
    CHECK(std::string(err).find("element expected") != std::string::npos);
    std::free(err);
    CHECK(em_expr_parse("a+zz", "a=1", &x, &err) == 1);
    CHECK(std::string(err) == "no value for the variable zz");
    std::free(err);
    CHECK(em_expr_parse("(a", "a=1", &x, &err) == 1);
    std::free(err);
}

TEST_CASE("exprmatrix model: today's values of the cases of the decision round", "[exprmatrix][cand]")
{
    // the measured port (cmp-compound-1-1/report.md, quick-arith-long.bas)
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, NOW) == "-1");
    CHECK(expr("q*1000>50000", "q=100").value(EM_DL, NOW) == "-1");
    CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, NOW) == "00000000");
    CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DF, NOW) == "800000C1");     // -1.0
    CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, NOW) == "00000000");
    CHECK(expr("q*2>2.4", "q=1.25").value(EM_DL, NOW) == "0");                // #1183
    CHECK(expr("-q<-1.25", "q=1.5").value(EM_DL, NOW) == "-1");
    CHECK(expr("1+1+1+q", "q=1.5").value(EM_PRINT, NOW) == "4");
    CHECK(expr("1+1+q", "q=1.5").value(EM_PRINT, NOW) == "3.5");
    CHECK(expr("w+v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, NOW) == "8");
    CHECK(expr("w-v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, NOW) == ".5");
    CHECK(expr("base+i*4", "base.l=2097152 i=10000").value(EM_DL, NOW) == "2137152");
}

TEST_CASE("exprmatrix model: the spec's scenarios under every candidate", "[exprmatrix][cand]")
{
    for (int n = EM_CAND_C1; n < EM_CAND_C7; n++)  // C7: its own case below
        for (bool b : { false, true }) {
            unsigned r = cand(n, b);

            INFO(em_cand_name(n), b ? " (b)" : " (a)");
            CHECK(expr("base+q*4", "base.l=2097152 q=1.5").value(EM_DL, r)
                  == expr("q*4+base", "base.l=2097152 q=1.5").value(EM_DL, r));
            CHECK(expr("w+v+q*2", "q=1.5 w.w=3 v.w=4").value(EM_DW, r)
                  == expr("q*2+w+v", "q=1.5 w.w=3 v.w=4").value(EM_DW, r));
            CHECK(expr("l+q*2>12", "l.l=10 q=1.5").value(EM_IF, r)
                  == expr("q*2+l>12", "l.l=10 q=1.5").value(EM_IF, r));
            CHECK(expr("q*2>2.4", "q=1.25").value(EM_DL, r) == "-1");
            CHECK(expr("q+q>2.4", "q=1.25").value(EM_DL, r) == "-1");
            CHECK(expr("-q<-1.25", "q=1.5").value(EM_DL, r) == "-1");
            CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, r) == "800000C1");
            CHECK(expr("1+1+1+q", "q=1.5").value(EM_PRINT, r) == "4.5");
        }
}

TEST_CASE("exprmatrix model: the count cases of #1183 by candidate and scope", "[exprmatrix][cand]")
{
    // r.l=(w+v>c) and r.l=(w-(-v)>c) with w=v=20000, c=0: the Word sum
    // wraps unless the left operand keeps the wanted Long (scope b) or the
    // sum is a Long (C3)
    for (int n = EM_CAND_C1; n < EM_CAND_C7; n++) {  // C7: its own case below
        INFO(em_cand_name(n));
        const char *a = n == EM_CAND_C3 ? "-1" : "0";
        CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, cand(n)) == a);
        CHECK(expr("w-(-v)>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, cand(n)) == a);
        CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, cand(n, true)) == "-1");
        // r.l=(q*1000>50000), q=100: the Quick product overflows in both
        // scopes, but for C6's 32.16 product of the Quick and the Word 1000
        const char *k = n == EM_CAND_C6 || n == EM_CAND_C7 ? "-1" : "0";
        CHECK(expr("q*1000>50000", "q=100").value(EM_DL, cand(n)) == k);
        CHECK(expr("q*1000>50000", "q=100").value(EM_DL, cand(n, true)) == k);
        // rq.q=(l1+1>40000), l1=40000: fixed by scope (a) only
        CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, cand(n)) == "FFFF0000");
        CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, cand(n, true)) == "00000000");
        // rf.f=(l1/3>3.2), rq.q=(l1/3>3.2), l1=10: the whole-number division
        // under (a), as If does
        CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DF, cand(n)) == "00000000");
        CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DF, cand(n, true)) == "800000C1");
        CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DQ, cand(n)) == "00000000");
        CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DQ, cand(n, true)) == "FFFF0000");
        // rb.b=(b1+b2>150): -1 in both
        CHECK(expr("b1+b2>150", "b1.b=100 b2.b=100").value(EM_DB, cand(n)) == "-1");
        CHECK(expr("b1+b2>150", "b1.b=100 b2.b=100").value(EM_DB, cand(n, true)) == "-1");
    }
}

TEST_CASE("exprmatrix model: what tells the candidates apart", "[exprmatrix][cand]")
{
    // C2: a Word sum stays a Word, so w+v+q keeps the fraction as w-v+q
    // does; no wanted Long reaches a Quick product (it overflows)
    CHECK(expr("w+v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, cand(EM_CAND_C1)) == "8");
    CHECK(expr("w+v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, cand(EM_CAND_C2)) == "8.5");
    CHECK(expr("q*1000", "q=100").value(EM_DL, cand(EM_CAND_C1)) == "100000");
    CHECK(expr("q*1000", "q=100").value(EM_DL, cand(EM_CAND_C2)) == "-31072");
    // C3: Bytes and Words are added in Long, no wrap; the Long then takes
    // a Quick's fraction away
    CHECK(expr("w+v", "w.w=20000 v.w=20000").value(EM_PRINT, cand(EM_CAND_C3)) == "40000");
    CHECK(expr("w+v", "w.w=20000 v.w=20000").value(EM_PRINT, cand(EM_CAND_C1)) == "-25536");
    CHECK(expr("w-v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, cand(EM_CAND_C3)) == "0");
    // C4: a constant with a fraction is a Quick beside a Long, so the
    // operation is exact and rounded (C1: a Float)
    CHECK(expr("l/1.5", "l.l=100000").value(EM_PRINT, cand(EM_CAND_C1)) == "66666.66");
    CHECK(expr("l/1.5", "l.l=100000").value(EM_PRINT, cand(EM_CAND_C4)) == "66666");
    // every candidate but C7 rounds each exact operation under a wanted
    // Long, so a chain of two Quicks and a Long still depends on the order;
    // C7 rounds once, at the consumer
    for (int n = EM_CAND_C1; n < EM_NCAND; n++) {
        INFO(em_cand_name(n));
        CHECK(expr("q+p+l", "q=1.5 p=1.5 l.l=10").value(EM_DL, cand(n)) == "13");
        CHECK(expr("l+q+p", "q=1.5 p=1.5 l.l=10").value(EM_DL, cand(n)) == (n == EM_CAND_C7 ? "13" : "12"));
    }
}

TEST_CASE("exprmatrix model: the typing of one operation by a candidate", "[exprmatrix][cand]")
{
    struct em_optype t;

    // a Long and a Quick under no wanted type: today l+q is a rounded Long,
    // q+l a Quick; every candidate gives both the exact Long
    for (int n = EM_CAND_C1; n < EM_NCAND; n++) {
        int path = n == EM_CAND_C7 ? '3' : 'm';    // C7: a 32.16 operation

        em_cand_cell(cand(n), EM_ADD, 0, 3, 4, &t);
        CHECK(t.result == 3);
        CHECK(t.path == path);
        em_cand_cell(cand(n), EM_ADD, 0, 4, 3, &t);
        CHECK(t.result == 3);
        CHECK(t.path == path);
    }
    // Word+Word: a Long in C1, a Word in C2
    em_cand_cell(cand(EM_CAND_C1), EM_ADD, 0, 2, 2, &t);
    CHECK(t.result == 3);
    em_cand_cell(cand(EM_CAND_C2), EM_ADD, 0, 2, 2, &t);
    CHECK(t.result == 2);
    // a Float and an AND: refused
    em_cand_cell(cand(EM_CAND_C1), EM_AND, 0, 5, 2, &t);
    CHECK(t.result == 0);
    CHECK(std::string(em_cand_name(EM_CAND_C4)) == "C4");
}

TEST_CASE("exprmatrix model: C5, the 32.16 product of a Quick and a whole number", "[exprmatrix][cand]")
{
    unsigned c5 = cand(EM_CAND_C5), c5e = cand(EM_CAND_C5) | EM_R_C5ERR;

    // cut: the consumer takes the whole part's low word, as the Quick wraps today
    CHECK(expr("q*w", "q=1.5 w.w=30000").value(EM_DW, c5) == "-20536");
    CHECK(expr("q*w", "q=1.5 w.w=30000").value(EM_DW, cand(EM_CAND_C1)) == "-20536");
    // with the overflow an error: the product 45000 fits no Word
    CHECK(expr("q*w", "q=1.5 w.w=30000").value(EM_DW, c5e) == "!runtime");
    CHECK(expr("q*w", "q=1.5 w.w=3000").value(EM_DW, c5e) == "4500");
    // a Long product beyond 32 bits under a wanted Long
    CHECK(expr("q*l", "q=1.5 l.l=2000000000").value(EM_DL, c5e) == "!runtime");
    CHECK(expr("q*l", "q=1.5 l.l=1000").value(EM_DL, c5e) == "1500");
    // the typing: a wide product
    CHECK(expr("q*w", "q=1.5 w.w=3").typing(EM_DW, c5) == "mul Q>~ W>W w L;");
}

TEST_CASE("exprmatrix model: C6, a whole constant beside a Quick by its value", "[exprmatrix][cand]")
{
    unsigned c6 = cand(EM_CAND_C6), c6e = cand(EM_CAND_C6) | EM_R_C5ERR;

    // q*1000 is a 32.16 product: exact under a wanted Long, in a comparison
    CHECK(expr("q*1000", "q=100").value(EM_DL, c6) == "100000");
    CHECK(expr("q*1000", "q=100").value(EM_DL, cand(EM_CAND_C5)) == "100000");
    CHECK(expr("q*1000>50000", "q=100").value(EM_DL, c6e) == "-1");
    // with no wanted type the product is a Quick: 100000 does not fit
    CHECK(expr("q*1000", "q=100").value(EM_PRINT, c6e) == "!runtime");
    CHECK(expr("q*1000", "q=10").value(EM_PRINT, c6) == "10000");
    // the constant is a Word, not a Quick
    CHECK(expr("q*1000", "q=1.5").typing(EM_PRINT, c6) == "mul Q>~ k>W w Q;");
    CHECK(expr("q*1000", "q=1.5").typing(EM_PRINT, cand(EM_CAND_C5)) == "mul Q>Q k>Q p Q;");
    // a constant with a fraction as in C1, and + as in C1
    CHECK(expr("q*0.5", "q=1.5").typing(EM_PRINT, c6) == expr("q*0.5", "q=1.5").typing(EM_PRINT, cand(EM_CAND_C1)));
    CHECK(expr("q+1000", "q=1.5").typing(EM_PRINT, c6) == expr("q+1000", "q=1.5").typing(EM_PRINT, cand(EM_CAND_C1)));
}

TEST_CASE("exprmatrix model: C7, the wanted type sets the intermediate format", "[exprmatrix][cand]")
{
    unsigned c7 = cand(EM_CAND_C7);

    // the chain clause: rounded once, at the consumer
    CHECK(expr("q+p+l", "q=1.5 p=1.5 l.l=10").value(EM_DL, c7) == "13");
    CHECK(expr("l+q+p", "q=1.5 p=1.5 l.l=10").value(EM_DL, c7) == "13");
    CHECK(expr("w+v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, c7) == "8.5");
    CHECK(expr("q+w+v", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, c7) == "8.5");
    // a Quick keeps its fraction with no wanted type
    CHECK(expr("w-v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, c7) == ".5");
    CHECK(expr("l/q", "q=1.5 l.l=10").value(EM_PRINT, c7) == "6.6667");
    // a wanted Word: 16.16, the Quick product wraps
    CHECK(expr("q*w", "q=1.5 w.w=30000").value(EM_DW, c7) == "-20536");
    // a / with a Long under a wanted Quick: 32.16
    CHECK(expr("l/q", "q=4 l.l=100000").value(EM_DQ, c7) == "61A80000");
    CHECK(expr("l/q", "q=4 l.l=100000").value(EM_DQ, cand(EM_CAND_C1)) == "E1A80000");  // the Long cut to 16.16 first
    // without a Quick: today's (C7), or C1's with its left operand by its
    // own operands (C7+, EM_R_SCOPEB)
    unsigned c7p = c7 | EM_R_SCOPEB;
    CHECK(expr("w+v", "w.w=20000 v.w=20000").value(EM_PRINT, c7) == "-25536");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, c7) == "-1");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, c7p) == "0");
    CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, c7) == "00000000");
    CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, c7p) == "800000C1");
    CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, c7p) == "FFFF0000");
    // with a Quick: a comparison's operands have no wanted type
    for (unsigned r : { c7, c7p }) {
        CHECK(expr("q*1000>50000", "q=100").value(EM_DL, r) == "-1");
        CHECK(expr("q*2>2.4", "q=1.25").value(EM_DL, r) == "-1");
        CHECK(expr("q+q>2.4", "q=1.25").value(EM_DL, r) == "-1");
        CHECK(expr("-q<-1.25", "q=1.5").value(EM_DL, r) == "-1");
        CHECK(expr("1+1+1+q", "q=1.5").value(EM_PRINT, r) == "4.5");
        CHECK(expr("base+q*4", "base.l=2097152 q=1.5").value(EM_DL, r) == "2097158");
        CHECK(expr("w+v+q*2", "q=1.5 w.w=3 v.w=4").value(EM_DW, r)
              == expr("q*2+w+v", "q=1.5 w.w=3 v.w=4").value(EM_DW, r));
    }
    // Mod on 32.16: a - b*trunc(a/b)
    CHECK(expr("q MOD l", "q=-7.5 l.l=2").value(EM_PRINT, c7) == "-1.5");
    // the typing: a 32.16 and a 16.16 operation
    CHECK(expr("q*w", "q=1.5 w.w=3").typing(EM_DL, c7) == "mul Q>~ W>~ 3 L;");
    CHECK(expr("q*p", "q=1.5 p=2").typing(EM_DQ, c7) == "mul Q>Q Q>Q 1 Q;");
}

TEST_CASE("exprmatrix model: C7Kx, a fraction constant a Quick only when exact", "[exprmatrix][cand]")
{
    unsigned lng = cand(EM_CAND_C7) | EM_R_C7K | EM_R_C7KLONG, x = lng | EM_R_C7KEXACT;

    // 0.001 and 0.1 in 16.16 lose precision beside a Long; kept as Floats
    CHECK(expr("l*0.001", "l.l=1000000").value(EM_PRINT, lng) == "991");
    CHECK(expr("l*0.001", "l.l=1000000").value(EM_PRINT, x) == "1000");
    CHECK(expr("l*0.1", "l.l=1000000").value(EM_DL, x) == "100000");
    // 0.75 is exact in 16.16: a Quick, as in C7K-long
    CHECK(expr("l*0.75", "l.l=1000").typing(EM_DL, x) == expr("l*0.75", "l.l=1000").typing(EM_DL, lng));
    // beside a Quick a Quick, as today
    CHECK(expr("q*0.1", "q=10").value(EM_DQ, x) == expr("q*0.1", "q=10").value(EM_DQ, lng));
    CHECK(expr("b AND 1.5", "b.b=3").value(EM_DB, x) == "1");
    // a Long in a 16.16 operation under a wanted Word is cut first in C7 and
    // C7K-long; C7Kx computes every operation with a Long in 32.16
    CHECK(expr("l*q", "l.l=100000 q=0.25").value(EM_DW, lng) == "-7768");
    CHECK(expr("l*q", "l.l=100000 q=0.25").value(EM_DW, x) == "25000");
    CHECK(expr("l*q", "l.l=100000 q=0.25").value(EM_DW, NOW) == "25000");
}

TEST_CASE("exprmatrix model: C7K, constants by their smallest type", "[exprmatrix][cand]")
{
    unsigned k = cand(EM_CAND_C7) | EM_R_C7K, own = k, lng = k | EM_R_C7KLONG, dst = k | EM_R_C7KDEST;

    // the Word sum in a comparison: wraps in its own type, not under at
    // least a Long, under the destination's width by the destination
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, own) == "0");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, lng) == "-1");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, dst) == "-1");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DW, dst) == "0");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_IF, dst) == "F");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_IF, lng) == "T");
    // the comparison's operands by their own: #1183's cases fixed in all three
    for (unsigned r : { own, lng, dst }) {
        CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, r) == "800000C1");
        CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, r) == "FFFF0000");
        CHECK(expr("q*1000>50000", "q=100").value(EM_DL, r) == "-1");
        CHECK(expr("q*2>2.4", "q=1.25").value(EM_DL, r) == "-1");
        CHECK(expr("b1+b2>150", "b1.b=100 b2.b=100").value(EM_DB, r) == "-1");
        CHECK(expr("1+1+1+q", "q=1.5").value(EM_PRINT, r) == "4.5");
        CHECK(expr("q+p+l", "q=1.5 p=1.5 l.l=10").value(EM_DL, r) == expr("l+q+p", "q=1.5 p=1.5 l.l=10").value(EM_DL, r));
    }
}

// ------------------------------------------------- the decided rule P+

// P+ (exprmatrix.h, EM_RULES_DECIDED; task 2.2 of expr-tree-5-operand-typing,
// the analysis expr-tree-5-analysis/report.md with the user's answers F1-F7)
static const unsigned DEC = EM_RULES_DECIDED;

// the value of text under ctx by P+ and by today's port (NOW)
static void same_as_today(const char *text, const char *vars, enum em_ctx ctx)
{
    INFO(text, " ", vars);
    CHECK(expr(text, vars).value(ctx, DEC) == expr(text, vars).value(ctx, NOW));
}

TEST_CASE("exprmatrix model: P+, the count cases of #1183", "[exprmatrix][cand][decided]")
{
    // two of the eight change, both fixes: the Long sum and the Long
    // constant compared as Longs
    CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, DEC) == "FFFF0000");
    CHECK(expr("l1+1>40000", "l1.l=40000").value(EM_DQ, NOW) == "00000000");
    CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, DEC) == "800000C1");
    CHECK(expr("100000001>l3", "l3.l=100000000").value(EM_DF, NOW) == "00000000");
    // the other six keep today's value
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, DEC) == "-1");
    CHECK(expr("w-(-v)>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DL, DEC) == "-1");
    CHECK(expr("q*1000>50000", "q=100").value(EM_DL, DEC) == "-1");
    CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DF, DEC) == "800000C1");
    CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DQ, DEC) == "FFFF0000");
    CHECK(expr("b1+b2>150", "b1.b=100 b2.b=100").value(EM_DB, DEC) == "-1");
    same_as_today("w+v>c", "w.w=20000 v.w=20000 c.w=0", EM_DL);
    same_as_today("w-(-v)>c", "w.w=20000 v.w=20000 c.w=0", EM_DL);
    same_as_today("q*1000>50000", "q=100", EM_DL);
    same_as_today("l1/3>3.2", "l1.l=10", EM_DF);
    same_as_today("l1/3>3.2", "l1.l=10", EM_DQ);
    same_as_today("b1+b2>150", "b1.b=100 b2.b=100", EM_DB);
    // the Word sum in a comparison at least a Long: If and a Word destination too
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_IF, DEC) == "T");
    CHECK(expr("w+v>c", "w.w=20000 v.w=20000 c.w=0").value(EM_DW, DEC) == "-1");
    // change 8: the comparison's common format reaches the whole-number
    // division (today 0 and false)
    CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_DL, DEC) == "-1");
    CHECK(expr("l1/3>3.2", "l1.l=10").value(EM_IF, DEC) == "T");
}

TEST_CASE("exprmatrix model: P+, the spec's scenarios", "[exprmatrix][cand][decided]")
{
    CHECK(expr("base+q*4", "base.l=2097152 q=1.5").value(EM_DL, DEC) == "2097158");
    CHECK(expr("q*4+base", "base.l=2097152 q=1.5").value(EM_DL, DEC) == "2097158");
    CHECK(expr("w+v+q*2", "q=1.5 w.w=3 v.w=4").value(EM_DW, DEC) == "10");
    CHECK(expr("q*2+w+v", "q=1.5 w.w=3 v.w=4").value(EM_DW, DEC) == "10");
    CHECK(expr("l+q*2>12", "l.l=10 q=1.5").value(EM_IF, DEC) == "T");
    CHECK(expr("q*2+l>12", "l.l=10 q=1.5").value(EM_IF, DEC) == "T");
    CHECK(expr("q*2>2.4", "q=1.25").value(EM_DL, DEC) == "-1");
    CHECK(expr("q+q>2.4", "q=1.25").value(EM_DL, DEC) == "-1");
    CHECK(expr("-q<-1.25", "q=1.5").value(EM_DL, DEC) == "-1");
    CHECK(expr("1+1+1+q", "q=1.5").value(EM_PRINT, DEC) == "4.5");
    CHECK(expr("w+v+q", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, DEC) == "8.5");
    CHECK(expr("q+w+v", "q=1.5 w.w=3 v.w=4").value(EM_PRINT, DEC) == "8.5");
    // the If forms change 8 changes (the analysis, section 4)
    CHECK(expr("w/2>q", "w.w=3 q=1.2").value(EM_IF, DEC) == "T");
    CHECK(expr("w/2*2<>x", "w.w=3 x=3").value(EM_IF, DEC) == "F");
    CHECK(expr("w/2*2=q", "w.w=3 q=2").value(EM_IF, DEC) == "F");
    CHECK(expr("w/2*2=w", "w.w=3").value(EM_IF, DEC) == "F");
}

TEST_CASE("exprmatrix model: P+ keeps Quick as a whole number (fixed point 3)", "[exprmatrix][cand][decided]")
{
    same_as_today("Not h", "h=-1", EM_DQ);
    same_as_today("Not h", "h=-1", EM_IF);
    same_as_today("Not h", "h=0", EM_DQ);
    same_as_today("Not q", "q=0", EM_IF);
    same_as_today("q=-1", "q=-1", EM_IF);
    same_as_today("(q AND 1)=1", "q=3", EM_IF);
    same_as_today("(q AND 1)=1", "q=1.5", EM_IF);
    same_as_today("h AND g", "h=-1 g=-1", EM_IF);
    same_as_today("q AND 6", "q=7", EM_DQ);
    same_as_today("flag=(a>b)", "flag=-1 a=2 b=1", EM_IF);
    same_as_today("(flag=-1)*5", "flag=-1", EM_DW);
    same_as_today("x-(a>b)", "x=10 a=2 b=1", EM_DL);
    same_as_today("x*(y>0)+1", "x=3 y=1", EM_DQ);
    same_as_today("(x AND 3)=q", "x=7 q=3", EM_IF);
    same_as_today("q LSL 2", "q=1.5", EM_DL);
    same_as_today("q ASR 1", "q=-3", EM_DW);
    same_as_today("q*4+base", "q=3 base.l=100000", EM_DL);
    same_as_today("q MOD 2=1", "q=7", EM_IF);
    CHECK(expr("(flag=-1)*5", "flag=-1").value(EM_DW, DEC) == "-5");
    CHECK(expr("Not h", "h=-1").value(EM_IF, DEC) == "F");
    // a Long and a Quick under AND: the Long operation in both orders
    CHECK(expr("q AND l", "l.l=65536 q=-1").value(EM_IF, DEC) == "T");
    CHECK(expr("l AND q", "l.l=65536 q=-1").value(EM_IF, DEC) == "T");
}

TEST_CASE("exprmatrix model: P+ on the conflicts of the analysis", "[exprmatrix][cand][decided]")
{
    // 1-4: a / under a wanted Long whole at once (F3), the top-level / of
    // a Word destination rounded down, Mod by today's sign rule (F4)
    CHECK(expr("i/2*2", "i=7").value(EM_DL, DEC) == "6");
    CHECK(expr("q/2+0", "q=-3").value(EM_DL, DEC) == "-1");
    CHECK(expr("q/2", "q=-3").value(EM_DW, DEC) == "-2");
    CHECK(expr("q MOD 2", "q=-7").value(EM_DL, DEC) == "1");
    // 5-8: a constant with a point beside whole numbers or under a wanted
    // Long a Float as in the original (F2)
    CHECK(expr("l*0.1", "l.l=100").value(EM_DL, DEC) == "10");
    CHECK(expr("l*0.001", "l.l=1000000").value(EM_PRINT, DEC) == "1000");
    CHECK(expr("w*0.1", "w.w=100").value(EM_DW, DEC) == "10");
    CHECK(expr("q*1.1", "q=10").value(EM_DL, DEC) == "11");
    CHECK(expr("l*0.5", "l.l=-3").value(EM_DL, DEC) == "-1");
    CHECK(expr("1.5-l", "l.l=3").value(EM_DL, DEC) == "-1");
    CHECK(expr("l+0.5", "l.l=-3").value(EM_DL, DEC) == "-2");
    CHECK(expr("l/1.5", "l.l=-10").value(EM_PRINT, DEC) == "-6.666666");
    CHECK(expr("l*0.5", "l.l=-3").value(EM_PRINT, DEC) == "-1.5");
    // 9, 10: a Long in a 16.16 operation under a Word computed wide; the
    // Quick product of #1178's decision wraps as today
    CHECK(expr("l*q", "l.l=100000 q=0.25").value(EM_DW, DEC) == "25000");
    CHECK(expr("q*40000", "q=1.5").value(EM_DQ, DEC) == "EA600000");
    CHECK(expr("q*40000", "q=1.5").value(EM_DW, DEC) == "-5536");
    // 11: a boolean times a Long is a Long operation (change 5)
    CHECK(expr("l*(a>b)", "l.l=100000 a=2 b=1").value(EM_PRINT, DEC) == "-100000");
    CHECK(expr("(a>b)*100000", "a=2 b=1").value(EM_PRINT, DEC) == "-100000");
    // 16, change 7: a whole-number subtree in the fractional format of its
    // expression, in both orders
    CHECK(expr("q+w/2", "q=0.5 w.w=3").value(EM_PRINT, DEC) == "2");
    CHECK(expr("w/2+q", "q=0.5 w.w=3").value(EM_PRINT, DEC) == "2");
    same_as_today("(q LSL 3)+8-w/2", "q=10 w.w=91", EM_DW);
    // F5: a Quick and a small whole number without a Long in 16.16 as the
    // original (the product wraps)
    CHECK(expr("i*300", "i=300").value(EM_PRINT, DEC) == "24464");
    CHECK(expr("q+20000", "q=20000").value(EM_PRINT, DEC) == "-25536");
    // under a wanted Quick a constant with a point is a Quick (F2)
    CHECK(expr("l*1.1", "l.l=10").value(EM_DQ, DEC) == "000AFFFA");
    // F7: a Quick rounded down before a shift, as today
    CHECK(expr("q ASL 4", "q=1.5").value(EM_DL, DEC) == "16");
}

TEST_CASE("exprmatrix model: P+ in both orders of the operands", "[exprmatrix][cand][decided]")
{
    // conflicts 12-15 of the analysis: the same value in both orders; at
    // an overflow the same value as long as no intermediate leaves its
    // format (the user's reading of fixed point 1)
    static const struct {
        const char *a, *b, *vars;
        enum em_ctx ctx;
        const char *v;
    } pairs[] = {
        { "(l+q+p) AND 255", "(q+p+l) AND 255", "l.l=10 q=1.5 p=1.5", EM_DL, "13" },
        { "(l+q+p) LSL 1", "(q+p+l) LSL 1", "l.l=10 q=1.5 p=1.5", EM_DL, "26" },
        { "NOT (l+q+p)", "NOT (q+p+l)", "l.l=10 q=1.5 p=1.5", EM_DL, "-14" },
        { "l+q+f", "f+q+l", "l.l=10 q=1.5 f.f=0.5", EM_DL, "12" },
        { "w+v+q", "q+w+v", "w.w=20000 v.w=20000 q=0.5", EM_PRINT, "-25535.5" },
        { "w+1+q", "q+w+1", "w.w=32767 q=0.5", EM_PRINT, "-32767.5" },
        { "q*q2*l", "l*q*q2", "q=300 q2=300 l.l=2", EM_PRINT, "180000" },
        { "base+q*4", "q*4+base", "base.l=2097152 q=1.5", EM_DL, "2097158" },
        { "q+w/2", "w/2+q", "q=0.5 w.w=3", EM_PRINT, "2" },
    };

    for (const auto &p : pairs) {
        INFO(p.a, " / ", p.b);
        CHECK(expr(p.a, p.vars).value(p.ctx, DEC) == p.v);
        CHECK(expr(p.b, p.vars).value(p.ctx, DEC) == p.v);
    }
}

TEST_CASE("exprmatrix model: P+, a constant with a point is never integral", "[exprmatrix][cand][decided]")
{
    // l1/3.0: the fractional division in every context ("3.0 ist nie ein
    // integraler Typ")
    CHECK(expr("l1/3.0>3.2", "l1.l=10").value(EM_DL, DEC) == "-1");
    CHECK(expr("l1/3.0>3.2", "l1.l=10").value(EM_DF, DEC) == "800000C1");
    CHECK(expr("l1/3.0>3.2", "l1.l=10").value(EM_DQ, DEC) == "FFFF0000");
    CHECK(expr("l1/3.0>3.2", "l1.l=10").value(EM_IF, DEC) == "T");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_DL, DEC) == "3");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_DF, DEC) == "D5555542");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_PRINT, DEC) == "3.333333");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_DQ, DEC) == "00035555");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_DW, DEC) == "3");
    CHECK(expr("l1/3.0", "l1.l=10").value(EM_DB, DEC) == "3");
    // F6: also under a Byte type (today 2.0 is the whole Byte there,
    // #1188's row): the value stays, the operation is a Float one
    CHECK(expr("b/2.0", "b.b=3").value(EM_DB, DEC) == "1");
    CHECK(expr("b/2.0", "b.b=3").typing(EM_DB, NOW) == "div B>B k>B p B;");
    CHECK(expr("b/2.0", "b.b=3").typing(EM_DB, DEC) == "div B>F k>F p F;");
    CHECK(expr("b/2", "b.b=3").typing(EM_DB, DEC) == "div B>B k>B p B;");
    // a bit operator takes its whole part, a shift count refuses it (the
    // case below)
    CHECK(expr("b AND 2.0", "b.b=3").value(EM_DB, NOW) == "2");
    CHECK(expr("b AND 2.0", "b.b=3").value(EM_DB, DEC) == "2");
    CHECK(expr("b LSL 2.0", "b.b=3").value(EM_DB, DEC) == "!noncompile");
    // beside a Quick a Quick
    CHECK(expr("q*3.0", "q=1.5").value(EM_DQ, DEC) == "00048000");
    CHECK(expr("q+3.0", "q=1.5").value(EM_PRINT, DEC) == "4.5");
}

// a 16.16 value of raw bits as decimal text (exact: 1/65536 = 5^16/10^16)
static std::string q16text(int32_t raw)
{
    uint32_t m = raw < 0 ? (uint32_t)-(int64_t)raw : (uint32_t)raw;
    char s[48];

    std::snprintf(s, sizeof s, "%s%u.%016llu", raw < 0 ? "-" : "", m >> 16,
                  (unsigned long long)(m & 0xffff) * 152587890625ull);
    return s;
}

TEST_CASE("exprmatrix model: P+ computes 16.16 as today's routines, bit for bit", "[exprmatrix][cand][decided]")
{
    // qmul and qdivide (its last bit, $7FFFFFFF for a divisor 0 or its
    // negative for a negative dividend, #200) are today's where P+ keeps
    // 16.16: the same text as today's port for random Quicks
    uint32_t seed = 12345;
    auto next = [&seed] {
        seed = seed * 1103515245u + 12345u;
        return seed;
    };
    int differ = 0;

    for (int i = 0; i < 400; i++) {
        int32_t a = (int32_t)next() >> (next() % 16), b = (int32_t)next() >> (next() % 24);

        if (i % 50 == 0)
            b = 0;
        std::string vars = "q=" + q16text(a) + " p=" + q16text(b);
        for (const char *t : { "q/p", "q*p", "q+p", "q-p", "q/p*p", "(q+p)/p" })
            for (enum em_ctx ctx : { EM_DQ, EM_DW, EM_PRINT }) {
                std::string x = expr(t, vars.c_str()).value(ctx, DEC), y = expr(t, vars.c_str()).value(ctx, NOW);

                if (x != y && differ++ < 10)
                    FAIL_CHECK(std::string(t) + " " + vars + ": " + x + ", today " + y);
            }
    }
    CHECK(differ == 0);
    CHECK(expr("q/p", "q=1 p=0").value(EM_DQ, DEC) == "7FFFFFFF");
    CHECK(expr("q/p", "q=-1 p=0").value(EM_DQ, DEC) == "80000001");
    CHECK(expr("q/p", "q=-7.25 p=0.3").value(EM_DQ, DEC) == "FFE7D508");
    // a wider division by 0 as lqdiv and longdiv: the largest number with
    // the dividend's sign
    CHECK(expr("l/q", "l.l=-100 q=0").value(EM_DL, DEC) == "-2147483647");
    CHECK(expr("l/q", "l.l=-100 q=0").value(EM_DL, NOW) == "-2147483647");
    CHECK(expr("q/p", "q=1 p=0").value(EM_DL, NOW) == "2147483647");
}

TEST_CASE("exprmatrix model: today's division by 0 and 2.0 under a Byte", "[exprmatrix][cand]")
{
    // the port's libraries: the dividend's sign (#200); the CD's: $7FFFFFFF
    CHECK(expr("q/p", "q=-1 p=0").value(EM_DQ, NOW) == "80000001");
    CHECK(expr("q/p", "q=-1 p=0").value(EM_DQ, EM_RULES_ORIG) == "7FFFFFFF");
    // the mixed routines of #1178 (lqdiv, longdiv): q/l the Quick quotient
    CHECK(expr("l/q", "l.l=100 q=0").value(EM_DL, NOW) == "2147483647");
    CHECK(expr("q/l", "l.l=0 q=-1").value(EM_DL, NOW) == "-32767");
    // 2.0 under a Byte type is the whole Byte (exprtype.c, consttype2)
    CHECK(expr("b AND 2.0", "b.b=3").value(EM_DB, NOW) == "2");
    CHECK(expr("b+1.5", "b.b=3").value(EM_DB, NOW) == "4");
}

TEST_CASE("exprmatrix model: P+, a bit operator inside a comparison with a Float", "[exprmatrix][cand][decided]")
{
    // change 8 reaches the Float format into both sides; a bit operator,
    // shift or Not there is typed by its own operands and its whole result
    // taken as a Float, so what compiles today still compiles
    CHECK(expr("(b AND 1)>f", "b.b=3 f.f=0.5").value(EM_IF, DEC) == "T");
    CHECK(expr("(b AND 1)=2.5", "b.b=3").value(EM_DB, DEC) == "0");
    CHECK(expr("2.0=(b AND 1)", "b.b=3").value(EM_IF, DEC) == "F");
    CHECK(expr("(l LSL 1)>2.5", "l.l=3").value(EM_IF, DEC) == "T");
    CHECK(expr("(NOT b)<1.5", "b.b=3").value(EM_IF, DEC) == "T");
    CHECK(expr("(b AND 1)+0.5>1", "b.b=3").value(EM_IF, DEC) == "T");
    // a wanted Float stays refused, as today
    CHECK(expr("b AND 1", "b.b=3").value(EM_DF, DEC) == "!noncompile");
}


TEST_CASE("exprmatrix model: P+, bit operators take the whole part of a fraction", "[exprmatrix][cand][decided]")
{
    // "expr-tree-5: bit operators and shifts with fractions; Quick /0;
    // Mod /0" (2026-11-25): AND, OR, EOR and NOT take a Quick or a constant
    // with a fraction on either side, the fraction cut (rounded down)
    for (enum em_ctx ctx : { EM_DB, EM_DW, EM_DL, EM_PRINT }) {
        INFO(em_ctx_name(ctx));
        CHECK(expr("b AND 1.5", "b.b=3").value(ctx, DEC) == "1");
        CHECK(expr("1.5 AND b", "b.b=3").value(ctx, DEC) == "1");
        CHECK(expr("b AND 1", "b.b=3").value(ctx, DEC) == "1");
        CHECK(expr("b AND 2.0", "b.b=3").value(ctx, DEC) == "2");
        CHECK(expr("2.0 AND b", "b.b=3").value(ctx, DEC) == "2");
        CHECK(expr("w OR 2.5", "w.w=4").value(ctx, DEC) == "6");
        CHECK(expr("2.5 EOR l", "l.l=7").value(ctx, DEC) == "5");
        CHECK(expr("NOT 1.5", "").value(ctx, DEC) == "-2");
    }
    CHECK(expr("b AND 1.5", "b.b=3").value(EM_DQ, DEC) == "00010000");
    CHECK(expr("1.5 AND b", "b.b=3").value(EM_DQ, DEC) == "00010000");
    CHECK(expr("b AND 1.5", "b.b=3").value(EM_IF, DEC) == "T");
    CHECK(expr("q AND 1.5", "q=3.5").value(EM_DQ, DEC) == "00010000");
    CHECK(expr("q AND 1.5", "q=3.5").value(EM_DL, DEC) == "1");
    CHECK(expr("q AND 2.0", "q=3.5").value(EM_DQ, DEC) == "00020000");
    CHECK(expr("b AND -1.5", "b.b=7").value(EM_DL, DEC) == "6");     // -1.5 rounded down: -2
    CHECK(expr("NOT 1.5", "").value(EM_DQ, DEC) == "FFFE0000");
    // in an index (#1179's refusal lifted)
    CHECK(expr("q AND 1.5", "q=3").value(EM_I1, DEC) == "1");
    CHECK(expr("q AND 1.5", "q=3").value(EM_I2, DEC) == "1");
    // today refused under a whole-number type
    CHECK(expr("b AND 1.5", "b.b=3").value(EM_DB, NOW) == "!noncompile");
    // a Float variable stays refused
    CHECK(expr("l AND f", "l.l=7 f.f=1.5").value(EM_DL, DEC) == "!noncompile");
}

TEST_CASE("exprmatrix model: P+, shift counts", "[exprmatrix][cand][decided]")
{
    // a constant with a point as a count is always refused; a variable or
    // expression of any whole or Quick type is the count, a Quick cut to
    // its whole part, as the original (shift-count/report.md)
    for (const char *t : { "b LSL 2.0", "b LSL 2.5", "q LSL 2.5", "q LSL 2.0", "x LSL 2.0", "l LSL 2.0", "b LSL (1+1.0)" })
        for (enum em_ctx ctx : { EM_DB, EM_DW, EM_DL, EM_DQ, EM_PRINT }) {
            INFO(t, " ", em_ctx_name(ctx));
            CHECK(expr(t, "b.b=3 q=1.5 x.w=3 l.l=3").value(ctx, DEC) == "!noncompile");
        }
    CHECK(expr("b LSL 2.0", "b.b=3").value(EM_DB, NOW) == "12");    // today and the original
    static const char *const V = "x.w=3 l.l=100000 b.b=3 q=1.5 n.w=1 q2=2.0 qh=2.5 qn=2.99";
    static const struct {
        const char *e;
        enum em_ctx ctx;
        const char *v;
    } counts[] = {
        { "x LSL q2", EM_PRINT, "12" },    { "x LSL qh", EM_PRINT, "12" },   { "x LSL qn", EM_PRINT, "12" },
        { "l LSL q2", EM_PRINT, "400000" }, { "l LSL qh", EM_PRINT, "400000" }, { "q LSL 2", EM_PRINT, "6" },
        { "q LSL n", EM_PRINT, "3" },      { "q LSL qh", EM_PRINT, "6" },    { "b LSL q2", EM_PRINT, "12" },
        { "b LSL qh", EM_PRINT, "12" },    { "x LSL (n+1)", EM_PRINT, "12" }, { "x LSL (qh*2)", EM_PRINT, "96" },
        { "x LSR qh", EM_PRINT, ".75" },   { "l LSR qh", EM_PRINT, "25000" }, { "l ASR qh", EM_PRINT, "25000" },
        { "q ASR qh", EM_PRINT, ".375" },  { "x LSL qh", EM_DL, "12" },      { "l LSL qh", EM_DL, "400000" },
        { "x LSL qh", EM_DW, "12" },       { "x LSL qh", EM_DQ, "000C0000" }, { "q LSL qh", EM_DL, "4" },
    };

    for (const auto &c : counts) {
        INFO(c.e, " ", em_ctx_name(c.ctx));
        CHECK(expr(c.e, V).value(c.ctx, DEC) == c.v);
        CHECK(expr(c.e, V).value(c.ctx, NOW) == c.v);
    }
}

TEST_CASE("exprmatrix model: P+, division by 0 in the destination's format", "[exprmatrix][cand][decided]")
{
    // the largest value in the destination's format with the dividend's
    // sign: a Quick destination $7FFFFFFF as today
    CHECK(expr("l/q", "l.l=100 q=0").value(EM_DQ, DEC) == "7FFFFFFF");
    CHECK(expr("l/q", "l.l=-100 q=0").value(EM_DQ, DEC) == "80000001");
    CHECK(expr("(l+q)/p", "l.l=100 q=0.5 p=0").value(EM_DQ, DEC) == "7FFFFFFF");
    CHECK(expr("l/q", "l.l=100 q=0").value(EM_DL, DEC) == "2147483647");
    CHECK(expr("l/q", "l.l=-100 q=0").value(EM_PRINT, DEC) == "-2147483647");
    CHECK(expr("l/q", "l.l=100 q=0").value(EM_DW, DEC) == "32767");
    CHECK(expr("l/q", "l.l=-100 q=0").value(EM_DW, DEC) == "-32767");
    CHECK(expr("l/q", "l.l=100 q=0").value(EM_DB, DEC) == "127");
}

TEST_CASE("exprmatrix model: Mod by 0 as today per type", "[exprmatrix][cand][decided]")
{
    // "Wie heute je Typ": Byte, Word and Quick trap (DIVS.W by 0), Long and
    // Float give the dividend; P+ keeps it (a 16.16 Mod traps, a 48.16 one
    // gives the dividend)
    for (unsigned r : { NOW, DEC }) {
        CHECK(expr("b MOD z", "b.b=7 z.b=0").value(EM_DB, r) == "!trap");
        CHECK(expr("w MOD z", "w.w=7 z.w=0").value(EM_DW, r) == "!trap");
        CHECK(expr("q MOD p", "q=7.5 p=0").value(EM_DQ, r) == "!trap");
        CHECK(expr("l MOD z", "l.l=-7 z.l=0").value(EM_DL, r) == "-7");
        CHECK(expr("f MOD g", "f.f=7.5 g.f=0").value(EM_DL, r) == "7");
        CHECK(expr("l MOD q", "l.l=100000 q=0").value(EM_DL, r) == "100000");
        CHECK(expr("w/z", "w.w=7 z.w=0").value(EM_DW, r) == "!trap");
    }
    // a Quick divisor below 1: today its whole part 0 traps, P+'s exact
    // Mod has a value
    CHECK(expr("q MOD p", "q=7.5 p=0.5").value(EM_DQ, NOW) == "!trap");
    CHECK(expr("q MOD p", "q=7.5 p=0.5").value(EM_DQ, DEC) == "00000000");
}

TEST_CASE("exprmatrix model: P+, a constant with a point left of a shift is a Quick", "[exprmatrix][cand][decided]")
{
    // "expr-tree-5: division by 0 per format, q Mod 0.5, fraction left of a
    // shift" (2026-11-25): "Wie eine Quick-Variable" - its 16.16 value is
    // shifted, under a Long rounded down first (F7); the same as a Quick
    // variable of that value
    static const struct {
        const char *e;
        enum em_ctx ctx;
        const char *v;
    } cases[] = {
        { "1.5 LSL 2", EM_DB, "6" },        { "1.5 LSL 2", EM_DW, "6" },     { "1.5 LSL 2", EM_DL, "4" },
        { "1.5 LSL 2", EM_DQ, "00060000" }, { "1.5 LSL 2", EM_PRINT, "6" },  { "2.0 LSL 1", EM_DB, "4" },
        { "2.0 LSL 1", EM_DL, "4" },        { "2.0 LSL 1", EM_PRINT, "4" },  { "1.5 LSR 1", EM_DQ, "0000C000" },
        { "1.5 LSR 1", EM_PRINT, ".75" },   { "1.5 LSL qh", EM_PRINT, "6" }, { "1.5 LSL qh", EM_DL, "4" },
        { "1.5 LSL qh", EM_DB, "6" },       { "-1.5 ASR 1", EM_PRINT, "-.75" }, { "(1.5 LSL 2)+1", EM_DB, "7" },
    };

    for (const auto &k : cases) {
        INFO(k.e, " ", em_ctx_name(k.ctx));
        CHECK(expr(k.e, "qh=2.5").value(k.ctx, DEC) == k.v);
    }
    // as a Quick variable of the same value
    for (enum em_ctx ctx : { EM_DB, EM_DW, EM_DL, EM_DQ, EM_PRINT }) {
        INFO(em_ctx_name(ctx));
        CHECK(expr("1.5 LSL 2", "").value(ctx, DEC) == expr("c LSL 2", "c=1.5").value(ctx, DEC));
        CHECK(expr("1.5 LSR 1", "").value(ctx, DEC) == expr("c LSR 1", "c=1.5").value(ctx, DEC));
    }
    // today: refused under Byte (#1188), the original 4
    CHECK(expr("1.5 LSL 2", "").value(EM_DB, NOW) == "!noncompile");
    // the count stays a whole number
    CHECK(expr("1.5 LSL 2.0", "").value(EM_DL, DEC) == "!noncompile");
}

TEST_CASE("exprmatrix model: the type so far of an evaluated cell", "[exprmatrix]")
{
    model md;
    struct em_value v;

    // Word+Word is a Long (doplusw), Byte+Byte a Word (doplusb), a Byte and
    // a Float a Float, a comparison a Word
    const struct { const char *key; int type; } want[] = {
        { "print-add-w-w-A", 3 }, { "print-add-b-b-A", 2 }, { "print-add-b-f-A", 5 }, { "if-gt-l-q-A", 2 },
        { "dl-and-f-b-A", 0 },  /* refused: no type */
        { "i1-add-f-cb-FI", 0 }, /* an index cell: no type */
    };

    for (const auto &w : want) {
        const struct em_cell *c = md.cell(w.key);

        REQUIRE(c != nullptr);
        em_model(&md.m, c, EM_RULES_PORTNOW, 0, &v);
        CHECK(v.type == w.type);
    }
}
