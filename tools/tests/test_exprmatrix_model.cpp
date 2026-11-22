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
    "values B b=-3 f=-2.5 w=-20 cf=-1.5\n"
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
    "cell B dl ^ f b\ncell B df ^ f b\ncell B dl ^ b w\ncell B dl ^ f cf\ncell J i1 ^ f b\n"
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
    // ... and a negative power is outside the array
    CHECK(md.value("i1-pow-f-b-J", ORIG, 1) == "241");
    CHECK(md.value("i1-pow-f-b-J", FIXED, 1) == "!runtime");
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
