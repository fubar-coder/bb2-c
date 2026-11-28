// test_exprfold - the constant folding of whole-number and Quick operations
// in C (programs/blitz2/src/exprfold.c; change expr-tree-5-operand-typing,
// task 3.6): single operations against values the 68000 code gives (the
// handlers of eval.c, worked out by hand, among them the Byte AND, OR and
// EOR of #1217 and #1218, the overflows of DIVS.W and of add.w, the shifts
// beyond 31, the exact comparison of a Long with a Quick, the raw Quick
// under a Long), what is not folded here (a Float, a word that is not
// known, the trap of a Quick Mod, lmullib's products, which eval.c leaves to
// the library loaded), and every two-operand cell of the
// expression matrix with two constants (tests/corpus/tests-exprmatrix/
// matrix.txt) in the Print context against the matrix's model with the
// port's rules (tools/lib/exprmatrix/model.c, EM_RULES_PORTNOW): the
// decision of exprtype_binop, the fold, the value as Print shows it; a
// planted wrong fold fails.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprfold.h"
#include "exprmatrix.h"
#include "exprtype.h"
}

namespace {

exprfold_reg L(uint32_t v)
{
    exprfold_reg r{};

    exprfold_setl(&r, v);
    return r;
}

exprfold_reg W(uint16_t v)
{
    exprfold_reg r{};

    exprfold_setw(&r, v);
    return r;
}

exprfold_reg Q(double v)
{
    return L((uint32_t)(int32_t)std::lround(v * 65536.0));
}

// the operation op of two operands of the types lt and rt as eval5 types
// it (no facts: two constants); *ok 0 when it is not folded here
uint32_t fold(unsigned op, unsigned lt, exprfold_reg l, unsigned rt, exprfold_reg r, int *ok = nullptr,
              unsigned *type = nullptr)
{
    exprtype_binfacts f{};
    exprtype_bin d = exprtype_binop(op, lt, rt, &f);
    uint32_t v = 0xdeadbeef;
    int done = exprfold_binop(op, &d, lt, &l, &r) && exprfold_value(&l, d.result, &v);

    if (ok)
        *ok = done;
    else
        REQUIRE(done);
    if (type)
        *type = d.result;
    return v;
}

const char *tname(unsigned t)
{
    static const char *const n[ET_NTYPES] = { "-", "B", "W", "L", "Q", "F", "A", "S" };
    return t < ET_NTYPES ? n[t] : "?";
}

std::string readfile(const char *path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream s;

    s << in.rdbuf();
    return s.str();
}

// ------------------------------------------------- the expression matrix
// (the helpers of test_exprtype.cpp: the facts of a literal, the operators)

unsigned constsize(long v)
{
    if (v >= -128 && v <= 127)
        return ET_BYTE;
    if (v >= -32768 && v <= 32767)
        return ET_WORD;
    return ET_LONG;
}

exprtype_constfacts constfacts(const char *text)
{
    exprtype_constfacts k{};

    if (*text == '-')
        text++;
    if (std::strpbrk(text, ".eE")) {
        double v = std::strtod(text, nullptr), w = std::trunc(v);

        k.point = 1;
        k.fix = v < 2147483648.0;
        k.whole = v == w;
        k.size = (unsigned char)constsize((long)w);
        k.qfit = k.fix && v < 32768.0;
    } else
        k.size = (unsigned char)constsize(std::strtol(text, nullptr, 10));
    return k;
}

unsigned emop(em_op o)
{
    static const unsigned t[] = { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV, ETO_MOD, ETO_POW, ETO_AND,
                                  ETO_OR,  ETO_EOR, ETO_LSL, ETO_LSR, ETO_ASL, ETO_ASR, ETO_EQ,
                                  ETO_NE,  ETO_LT,  ETO_LE,  ETO_GT,  ETO_GE };
    return o < EM_NEG ? t[o] : (unsigned)ETO_END;
}

// the constant text as consttype2 leaves it in its register under the type
// so far tsf: *t its type; 0 when this test does not know its bits (a
// Float, a literal with a point made a whole number)
int constant(unsigned tsf, const char *text, exprfold_reg *r, unsigned *t)
{
    exprtype_constfacts c = constfacts(text);
    exprtype_elem e = exprtype_element(tsf, 0, &c);
    double v = std::strtod(text, nullptr);

    *t = e.result;
    if (!e.result || e.conv_from || e.litcut || e.result == ET_FLOAT)
        return 0;
    if (e.result == ET_QUICK) {
        *r = Q(v);
        return 1;
    }
    if (c.point)
        return 0;
    *r = e.result == ET_LONG ? L((uint32_t)(int32_t)v) : W((uint16_t)(int16_t)v);
    return 1;
}

// printlib's Quick (the model's print_quick for the C libraries)
std::string quicktext(uint32_t value)
{
    char fr[8] = "", ip[8], out[32];
    int neg = (int32_t)value < 0;
    uint32_t v = neg ? 0u - value : value, frac = v & 0xffff, in;

    if (frac) {
        frac *= 10000;
        frac = (frac & 0xffff) >= 0x8000 ? (frac >> 16) + 1 : frac >> 16;
        if (frac >= 10000) {
            frac = 0;
            v += 0x10000;
        }
        std::snprintf(fr, sizeof fr, ".%04u", (unsigned)(frac % 10000));
        size_t n = std::strlen(fr);
        while (fr[n - 1] == '0')
            n--;
        if (fr[n - 1] == '.')
            n--;
        fr[n] = 0;
    }
    in = (v >> 16) & 0xffff;
    if (in || !fr[0])
        std::snprintf(ip, sizeof ip, "%u", (unsigned)in);
    else
        ip[0] = 0;
    if (!ip[0] && !fr[0])
        std::strcpy(ip, "0");
    std::snprintf(out, sizeof out, "%s%s%s", neg && (std::strcmp(ip, "0") || fr[0]) ? "-" : "", ip, fr);
    return out;
}

// the folded value v of type t as Print shows it
std::string printed(uint32_t v, unsigned t)
{
    switch (t) {
    case ET_BYTE: return std::to_string((int8_t)(uint8_t)v);
    case ET_WORD: return std::to_string((int16_t)(uint16_t)v);
    case ET_QUICK: return quicktext(v);
    default: return std::to_string((int32_t)v);
    }
}

struct matrixcheck {
    size_t cells = 0, folded = 0;
    std::vector<std::string> mismatches;
};

// every two-operand cell of two constants in the Print context; plant
// changes a folded value
matrixcheck checkmatrix(uint32_t (*plant)(unsigned op, unsigned t, uint32_t v) = nullptr)
{
    matrixcheck res;
    std::string text = readfile(EXPRMATRIX_TXT);
    em_matrix m{};
    char *errors = nullptr;

    REQUIRE(!text.empty());
    REQUIRE(em_parse(text.data(), text.size(), EXPRMATRIX_TXT, &m, &errors) == 0);
    for (size_t i = 0; i < m.ncells; i++) {
        const em_cell &c = m.cells[i];
        const em_set &s = m.sets[c.set];
        exprfold_reg l{}, r{};
        exprtype_binfacts f{};
        exprtype_bin d;
        exprtype_left pre;
        unsigned op, lt, rt, left;
        uint32_t v;
        em_value want;

        if (c.ctx != EM_PRINT || c.right < EM_CB || c.left < EM_CB)
            continue;
        op = emop(c.op);
        const char *rtext = c.op == EM_POW || (c.op >= EM_LSL && c.op <= EM_ASR) ? s.count[c.right] : s.value[c.right];
        res.cells++;
        if (!constant(0, s.value[c.left], &l, &lt))
            continue;
        pre = exprtype_leftop(op, lt, 0);
        left = pre.left;
        if (left != lt && !exprfold_conv(&l, lt, left))
            continue;
        if (!constant(pre.compare ? 0 : left, rtext, &r, &rt))
            continue;
        d = exprtype_binop(op, left, rt, &f);
        if (!d.result || !exprfold_binop(op, &d, left, &l, &r) || !exprfold_value(&l, d.result, &v))
            continue;
        // a Mod by a negative divisor: modlib's floor Mod (#1197, task 4.7
        // of type-system-rework) where the model's rules keep today's sign
        // rule; its own case "Mod is the floor Mod"
        if (op == ETO_MOD && rtext[0] == '-')
            continue;
        res.folded++;
        if (plant)
            v = plant(op, d.result, v);
        em_model(&m, &c, EM_RULES_PORTNOW, 0, &want);
        if (printed(v, d.result) != want.text)
            res.mismatches.push_back(std::string(c.key) + ": the fold " + printed(v, d.result) + " ("
                                     + tname(d.result) + "), the model " + want.text);
    }
    em_free(&m);
    return res;
}

} // namespace

TEST_CASE("exprfold: whole numbers as the handlers compute them", "[exprfold]")
{
    unsigned t;

    // a Byte sum is a Word (doplusb), a Word sum a Long after add.w, ext.l
    CHECK(fold(ETO_ADD, ET_BYTE, W(100), ET_BYTE, W(100), nullptr, &t) == 200);
    CHECK(t == ET_WORD);
    CHECK(fold(ETO_ADD, ET_WORD, W(20000), ET_WORD, W(20000), nullptr, &t) == (uint32_t)-25536);
    CHECK(t == ET_LONG);
    CHECK(fold(ETO_SUB, ET_WORD, W(0x8000), ET_WORD, W(1)) == 0x7fff);
    // muls.w: the Word product is a Long
    CHECK(fold(ETO_MUL, ET_WORD, W(-300), ET_WORD, W(300), nullptr, &t) == (uint32_t)-90000);
    CHECK(t == ET_LONG);
    // lmultiply modulo 2^32 (#199 fixed)
    CHECK(fold(ETO_MUL, ET_LONG, L(196608), ET_LONG, L(30000)) == 1603272704u);
    // DIVS.W: a Word -32768 / -1 overflows and keeps the dividend
    CHECK(fold(ETO_DIV, ET_WORD, W(0x8000), ET_WORD, W(0xffff)) == 0x8000);
    CHECK(fold(ETO_DIV, ET_BYTE, W((uint16_t)-7), ET_BYTE, W(2)) == (uint16_t)-3);
    CHECK(fold(ETO_DIV, ET_LONG, L((uint32_t)-100001), ET_LONG, L(10)) == (uint32_t)-10000);
    // domodword: a negative remainder plus the divisor
    CHECK(fold(ETO_MOD, ET_WORD, W((uint16_t)-7), ET_WORD, W(3)) == 2);
}

TEST_CASE("exprfold: the Byte AND, OR and EOR are sign-extended bytes (#1217, #1218)", "[exprfold]")
{
    unsigned t;

    CHECK(fold(ETO_AND, ET_BYTE, W(0xfffe), ET_BYTE, W(0), nullptr, &t) == 0);
    CHECK(t == ET_BYTE);
    CHECK(fold(ETO_OR, ET_BYTE, W(3), ET_BYTE, W(0xfffe)) == 0xffff);
    CHECK(fold(ETO_EOR, ET_BYTE, W(0xfffe), ET_BYTE, W(1)) == 0xffff);
    CHECK(fold(ETO_AMP, ET_WORD, W(0xff00), ET_WORD, W(0x0ff0)) == 0x0f00);
    CHECK(fold(ETO_BAR, ET_LONG, L(0x10000), ET_LONG, L(1)) == 0x10001);
    // a Quick: the whole parts (#1209)
    CHECK(fold(ETO_OR, ET_QUICK, Q(1.5), ET_QUICK, Q(2.5)) == 0x30000);
}

TEST_CASE("exprfold: the shifts and the bit operators", "[exprfold]")
{
    unsigned t;

    CHECK(fold(ETO_LSL, ET_BYTE, W(3), ET_BYTE, W(2), nullptr, &t) == 12);
    CHECK(t == ET_LONG);
    CHECK(fold(ETO_LSL, ET_LONG, L(1), ET_BYTE, W(33)) == 0);       // the count modulo 64
    CHECK(fold(ETO_LSL, ET_LONG, L(1), ET_BYTE, W(65)) == 2);
    CHECK(fold(ETO_ASR, ET_LONG, L((uint32_t)-8), ET_BYTE, W(40)) == 0xffffffffu);
    CHECK(fold(ETO_LSR, ET_WORD, W(0xffff), ET_WORD, W(4)) == 0x0fffffffu);     // ext.l first
    CHECK(fold(ETO_LSL, ET_QUICK, Q(1.5), ET_QUICK, Q(1.0), nullptr, &t) == 0x30000);
    CHECK(t == ET_QUICK);
    CHECK(fold(ETO_BITTST, ET_BYTE, W(5), ET_BYTE, W(2), nullptr, &t) == 0xffff);
    CHECK(t == ET_WORD);
    CHECK(fold(ETO_BITTST, ET_BYTE, W(5), ET_BYTE, W(1)) == 0);
    CHECK(fold(ETO_BITSET, ET_LONG, L(0), ET_BYTE, W(33)) == 2);    // the bit modulo 32
    CHECK(fold(ETO_BITCLR, ET_LONG, L(0xff), ET_BYTE, W(0)) == 0xfe);
    CHECK(fold(ETO_BITCHG, ET_WORD, W(0xffff), ET_WORD, W(31)) == 0x7fffffffu);
}

TEST_CASE("exprfold: the comparisons", "[exprfold]")
{
    unsigned t;

    CHECK(fold(ETO_LT, ET_WORD, W(0xffff), ET_WORD, W(1), nullptr, &t) == 0xffff);
    CHECK(t == ET_WORD);
    CHECK(fold(ETO_GE, ET_LONG, L(100000), ET_LONG, L(100001)) == 0);
    // a Long and a Quick exactly (cmplq, #1177)
    CHECK(fold(ETO_LT, ET_LONG, L(2), ET_QUICK, Q(2.5)) == 0xffff);
    CHECK(fold(ETO_EQ, ET_LONG, L(2), ET_QUICK, Q(2.5)) == 0);
    CHECK(fold(ETO_EQ, ET_QUICK, Q(3.0), ET_LONG, L(3)) == 0xffff);
    CHECK(fold(ETO_GT, ET_QUICK, Q(-1.5), ET_LONG, L(-2)) == 0xffff);
    CHECK(fold(ETO_LE, ET_LONG, L(100000), ET_QUICK, Q(1.5)) == 0);
}

TEST_CASE("exprfold: Quick operations and a raw Quick under a Long (#1178)", "[exprfold]")
{
    exprfold_reg l = L(5), r = Q(-1.5);
    exprtype_bin d{};
    uint32_t v;

    CHECK(fold(ETO_MUL, ET_QUICK, Q(1.5), ET_QUICK, Q(1.5)) == 0x24000);
    CHECK(fold(ETO_DIV, ET_QUICK, Q(1.0), ET_QUICK, Q(3.0)) == 0x5555);     // qdivide's last bit
    CHECK(fold(ETO_MOD, ET_QUICK, Q(7.5), ET_QUICK, Q(2.0)) == 1);          // domodquick: a Word
    // the operation mixedop writes: 5 + -1.5 rounded down, 5 - -1.5
    d.path = ETP_MIXED;
    d.rq = 1;
    d.result = ET_LONG;
    REQUIRE(exprfold_binop(ETO_ADD, &d, ET_LONG, &l, &r));
    REQUIRE(exprfold_value(&l, ET_LONG, &v));
    CHECK(v == 3);
    l = L(5);
    r = Q(-1.5);
    REQUIRE(exprfold_binop(ETO_SUB, &d, ET_LONG, &l, &r));
    REQUIRE(exprfold_value(&l, ET_LONG, &v));
    CHECK(v == 6);
}

TEST_CASE("exprfold: the minus sign, Not and the conversions", "[exprfold]")
{
    exprfold_reg r = W(0x8000);

    REQUIRE(exprfold_neg(&r, ET_WORD, 1));
    CHECK((r.v & 0xffff) == 0x8000);
    r = Q(3.0);
    REQUIRE(exprfold_not(&r, ET_QUICK));
    CHECK(r.v == 0xfffc0000u);                  // Not of the whole part (#1209)
    r = Q(-1.5);
    REQUIRE(exprfold_conv(&r, ET_QUICK, ET_LONG));
    CHECK(r.v == 0xfffffffeu);                  // rounded down
    r = L(100000);
    REQUIRE(exprfold_conv(&r, ET_LONG, ET_QUICK));
    CHECK(r.v == 0x86a00000u);                  // its low word in the high word
    r = W(0xffff);
    REQUIRE(exprfold_conv(&r, ET_WORD, ET_LONG));
    CHECK(r.v == 0xffffffffu);
}

TEST_CASE("exprfold: what is not folded here", "[exprfold]")
{
    int ok;
    exprfold_reg r{};

    // a Float, ^
    fold(ETO_ADD, ET_FLOAT, L(0x80000041), ET_FLOAT, L(0x80000041), &ok);
    CHECK(!ok);
    fold(ETO_POW, ET_WORD, W(2), ET_WORD, W(3), &ok);
    CHECK(!ok);
    CHECK(!exprfold_conv(&r, ET_WORD, ET_FLOAT));
    // a Long whose high word is not known (a move.w)
    fold(ETO_ADD, ET_LONG, W(1), ET_LONG, L(1), &ok);
    CHECK(!ok);
    // a division by 0: "Division by Zero" before the fold
    fold(ETO_DIV, ET_WORD, W(5), ET_WORD, W(0), &ok);
    CHECK(!ok);
}

TEST_CASE("exprfold: Mod is the floor Mod", "[exprfold]")
{
    int ok;

    // #1197: the divisor added to a remainder that is not 0 and of the
    // other sign, as modlib's domodword (today's rule: 1, -4, -4, 2)
    CHECK((int16_t)fold(ETO_MOD, ET_WORD, W(7), ET_WORD, W(0xfffd), &ok) == -2);
    CHECK(ok);
    CHECK((int16_t)fold(ETO_MOD, ET_WORD, W(0xfff9), ET_WORD, W(0xfffd), &ok) == -1);
    CHECK(ok);
    CHECK((int16_t)fold(ETO_MOD, ET_BYTE, W(0xfff9), ET_BYTE, W(0xfffd), &ok) == -1);
    CHECK(ok);
    CHECK((int16_t)fold(ETO_MOD, ET_WORD, W(0xfff9), ET_WORD, W(3), &ok) == 2);
    CHECK(ok);
}

TEST_CASE("exprfold: a Quick Mod by a divisor below 1", "[exprfold]")
{
    int ok;

    // #1219: folded, no DIVS.W by 0; the exact Mod's whole part as the
    // Word of the handler (the typing pass writes the exact value itself)
    CHECK(fold(ETO_MOD, ET_QUICK, Q(5.0), ET_QUICK, Q(0.5), &ok) == 0);
    CHECK(ok);
    CHECK(fold(ETO_MOD, ET_QUICK, Q(5.75), ET_QUICK, Q(0.5), &ok) == 0);
    CHECK(ok);
}

TEST_CASE("exprfold: lmullib's products are left to the library loaded", "[exprfold]")
{
    exprtype_binfacts f{};
    exprtype_bin d = exprtype_binop(ETO_MUL, ET_LONG, ET_LONG, &f);

    CHECK(exprfold_libdep(ETO_MUL, &d));
    d.wordop = 1;               // #1179: muls.w
    CHECK(!exprfold_libdep(ETO_MUL, &d));
    d = exprtype_binop(ETO_MUL, ET_QUICK, ET_QUICK, &f);
    CHECK(exprfold_libdep(ETO_MUL, &d));
    d = exprtype_binop(ETO_MUL, ET_WORD, ET_WORD, &f);
    CHECK(!exprfold_libdep(ETO_MUL, &d));
    d = exprtype_binop(ETO_DIV, ET_QUICK, ET_QUICK, &f);
    CHECK(!exprfold_libdep(ETO_DIV, &d));
}

TEST_CASE("exprfold: the matrix's cells of two constants against the model", "[exprfold]")
{
    matrixcheck r = checkmatrix();

    for (const std::string &m : r.mismatches)
        FAIL_CHECK(m);
    CHECK(r.cells == 608);
    CHECK(r.folded > 300);
    std::printf("exprfold: %zu cells of two constants, %zu folded here\n", r.cells, r.folded);
}

TEST_CASE("exprfold: a planted wrong fold fails", "[exprfold]")
{
    // an AND with its lowest bit flipped
    matrixcheck r = checkmatrix([](unsigned op, unsigned, uint32_t v) { return op == ETO_AND ? v ^ 1u : v; });
    CHECK(!r.mismatches.empty());
    // a shift by one too few
    r = checkmatrix([](unsigned op, unsigned, uint32_t v) { return op == ETO_LSL ? v >> 1 : v; });
    CHECK(!r.mismatches.empty());
}
