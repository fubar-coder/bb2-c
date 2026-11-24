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
            std::fprintf(stderr, "em_expr_parse %s: %s\n", text, err ? err : "out of memory");
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
