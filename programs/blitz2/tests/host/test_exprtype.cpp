// test_exprtype - the type decisions of an expression (programs/blitz2/src/
// exprtype.c; issue #1180): every decision of
// the functions against the committed table exprtype.tsv (written by this
// program's generator mode after a byte-identity check had
// passed, so it records the compiler's behaviour as proved on all sources),
// the same table with one row changed (must fail), a few decisions named in
// docs/blitz2/expression-types.md, and the two-operand cells of the
// expression matrix (tests/corpus/tests-exprmatrix/matrix.txt) against the
// matrix's model (tools/lib/exprmatrix/model.c, the port's rules): a cell
// the functions refuse ("Illegal Operator for Type") is one the model
// refuses, the value the model prints in the Print context fits the type
// the functions give, and the type so far the model gives a Print or If
// cell (em_value.type) is the result type of the functions (a planted wrong
// type that keeps the value fails).
//
// Generator mode: EXPRTYPE_TSV_WRITE=FILE test_exprtype writes the table to
// FILE (the test "the generator mode"; the others still run).
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
#include "exprmatrix.h"
#include "exprtype.h"
}

namespace {

const char *tname(unsigned t)
{
    static const char *const n[ET_NTYPES] = { "-", "B", "W", "L", "Q", "F", "A", "S" };
    return t < ET_NTYPES ? n[t] : "?";
}

const char *oname(unsigned op)
{
    static const char *const n[ETO_COUNT + 1] = {
        "+",   "-",   "*",   "/",   "&",  "|",  "AND",    "OR",     "EOR",    "LSL",    "ASL",    "LSR",   "ASR", "MOD",
        "^",   "=",   "<>",  "<",   "<=", ">",  ">=",     "BITTST", "BITSET", "BITCLR", "BITCHG", "END",
    };
    return op <= ETO_COUNT ? n[op] : "?";
}

const char *pname(unsigned p)
{
    static const char *const n[] = { "plain", "compare", "cmplq", "mixed" };
    return p < 4 ? n[p] : "?";
}

std::string conv(unsigned from, unsigned to)
{
    return from ? std::string(tname(from)) + ">" + tname(to) : "-";
}

char bit(unsigned v)
{
    return v ? '1' : '0';
}

// one row: the fields separated by tabs
std::string row(std::initializer_list<std::string> f)
{
    std::string r;

    for (const std::string &s : f) {
        if (!r.empty())
            r += '\t';
        r += s;
    }
    return r + "\n";
}

std::string binrow(unsigned op, unsigned l, unsigned r, const exprtype_binfacts &f)
{
    exprtype_bin d = exprtype_binop(op, l, r, &f);
    std::string facts = { bit(f.lraw), bit(f.rraw), bit(f.rcmp), bit(f.words),
                          bit(f.lbyte), bit(f.lconst), bit(f.rbyte), bit(f.rconst) };

    return row({ "binop", oname(op), tname(l), tname(r), facts, pname(d.path), conv(d.rup_from, d.rup_to),
                 std::string{ bit(d.lcut), bit(d.rcut) }, conv(d.lconv_from, d.lconv_to),
                 conv(d.rconv_from, d.rconv_to), std::string{ bit(d.lq), bit(d.rq) }, tname(d.handler),
                 tname(d.result), std::string{ bit(d.cmpres), bit(d.wordop), char('0' + d.byteext) } });
}

// the table of every decision (exprtype.tsv)
std::string table()
{
    std::string t =
        "# exprtype.tsv - every type decision of programs/blitz2/src/exprtype.c (test_exprtype.cpp;\n"
        "# EXPRTYPE_TSV_WRITE=FILE test_exprtype writes it).  Types: - (none) B W L Q F A (Addr) S;\n"
        "# a conversion FROM>TO or -; flags are 0/1 digits.  Rows:\n"
        "# optype OP T RESULT\n"
        "# cutsraw OP CUT\n"
        "# left OP LEFT LRAW -> LEFT LRAW OWNLEFT COMPARE\n"
        "# ownleft FROM TO DROPPED\n"
        "# binop OP LEFT RIGHT FACTS(lraw rraw rcmp words lbyte lconst rbyte rconst)\n"
        "#   -> PATH RUP CUT(l r) LCONV RCONV LQRQ HANDLER RESULT FLAGS(cmpres wordop byteext)\n"
        "# element TSF OWN -> CONV RESULT FLAGS(raw ext wordval)\n"
        "# const TSF FACTS(point fix whole data qfit) SIZE -> RESULT LITCUT\n";

    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned ty = 0; ty < ET_NTYPES; ty++)
            t += row({ "optype", oname(op), tname(ty), tname(exprtype_optype(op, ty)) });
    for (unsigned op = 0; op <= ETO_COUNT; op++)
        t += row({ "cutsraw", oname(op), std::string(1, bit(exprtype_cutsraw(op))) });
    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned l = 0; l < ET_NTYPES; l++)
            for (int raw = 0; raw <= (l == ET_LONG); raw++) {
                exprtype_left x = exprtype_leftop(op, l, raw);
                t += row({ "left", oname(op), tname(l), std::string(1, bit(raw)), tname(x.left),
                           std::string(1, bit(x.lraw)), std::string(1, bit(x.ownleft)),
                           std::string(1, bit(x.compare)) });
            }
    for (unsigned from = 0; from < ET_NTYPES; from++)
        for (unsigned to = 0; to < ET_NTYPES; to++)
            t += row({ "ownleft", tname(from), tname(to), std::string(1, bit(exprtype_ownleft(from, to))) });
    // the types with the raw and the comparison facts
    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned l = 0; l < ET_NTYPES; l++)
            for (unsigned r = 0; r < ET_NTYPES; r++)
                for (int lraw = 0; lraw <= (l == ET_LONG); lraw++)
                    for (int rraw = 0; rraw <= (r == ET_LONG); rraw++)
                        for (int rcmp = 0; rcmp <= 1; rcmp++) {
                            exprtype_binfacts f{};
                            f.lraw = (unsigned char)lraw;
                            f.rraw = (unsigned char)rraw;
                            f.rcmp = (unsigned char)rcmp;
                            t += binrow(op, l, r, f);
                        }
    // two Word values of a later index (#1179)
    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned l = 0; l < ET_NTYPES; l++)
            for (unsigned r = 0; r < ET_NTYPES; r++) {
                exprtype_binfacts f{};
                f.words = 1;
                t += binrow(op, l, r, f);
            }
    // the byte values and constants (#978)
    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned b = 1; b < 16; b++) {
            exprtype_binfacts f{};
            f.lbyte = b & 1;
            f.lconst = (b >> 1) & 1;
            f.rbyte = (b >> 2) & 1;
            f.rconst = (b >> 3) & 1;
            t += binrow(op, ET_WORD, ET_WORD, f);
        }
    for (unsigned tsf = 0; tsf < ET_NTYPES; tsf++)
        for (unsigned own = 0; own < ET_NTYPES; own++) {
            exprtype_elem e = exprtype_element(tsf, own, nullptr);
            t += row({ "element", tname(tsf), tname(own), conv(e.conv_from, e.conv_to), tname(e.result),
                       std::string{ bit(e.raw), bit(e.ext), bit(e.wordval) } });
        }
    for (unsigned tsf = 0; tsf < ET_NTYPES; tsf++)
        for (unsigned facts = 0; facts < 32; facts++) {
            if (!(facts & 1) && facts)
                continue;       // a whole number has no other fact
            for (unsigned size = ET_BYTE; size <= ET_LONG; size++) {
                exprtype_constfacts k{};
                k.point = facts & 1;
                k.fix = (facts >> 1) & 1;
                k.whole = (facts >> 2) & 1;
                k.data = (facts >> 3) & 1;
                k.qfit = (facts >> 4) & 1;
                k.size = (unsigned char)size;
                exprtype_elem e = exprtype_element(tsf, 0, &k);
                t += row({ "const", tname(tsf),
                           std::string{ bit(k.point), bit(k.fix), bit(k.whole), bit(k.data), bit(k.qfit) },
                           tname(size), tname(e.result), std::string(1, bit(e.litcut)) });
            }
        }
    return t;
}

std::vector<std::string> lines(const std::string &s)
{
    std::vector<std::string> v;
    std::istringstream in(s);
    std::string l;

    while (std::getline(in, l))
        v.push_back(l);
    return v;
}

// the rows of want that got does not have, as messages (at most 10, then
// the count)
std::vector<std::string> differences(const std::string &want, const std::string &got)
{
    std::vector<std::string> w = lines(want), g = lines(got), out;
    size_t n = 0;

    for (size_t i = 0; i < w.size() || i < g.size(); i++) {
        const std::string a = i < w.size() ? w[i] : "(none)", b = i < g.size() ? g[i] : "(none)";

        if (a == b)
            continue;
        if (++n <= 10)
            out.push_back("line " + std::to_string(i + 1) + ": table '" + a + "', function '" + b + "'");
    }
    if (n > 10)
        out.push_back(std::to_string(n) + " rows differ");
    return out;
}

std::string readfile(const char *path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream s;

    s << in.rdbuf();
    return s.str();
}

// ------------------------------------------------- the expression matrix

unsigned constsize(long v)
{
    if (v >= -128 && v <= 127)
        return ET_BYTE;
    if (v >= -32768 && v <= 32767)
        return ET_WORD;
    return ET_LONG;
}

// the facts consttype2 finds for the matrix's literal text (its sign is
// the negation, not part of the literal: litneg); the values of the matrix
// are far from the edges of the sizes and of the Quick
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

unsigned kindtype(em_kind k)
{
    switch (k) {
    case EM_B: return ET_BYTE;
    case EM_W: return ET_WORD;
    case EM_L: return ET_LONG;
    case EM_Q: return ET_QUICK;
    case EM_F: return ET_FLOAT;
    default: return 0;
    }
}

unsigned emop(em_op o)
{
    switch (o) {
    case EM_ADD: return ETO_ADD;
    case EM_SUB: return ETO_SUB;
    case EM_MUL: return ETO_MUL;
    case EM_DIV: return ETO_DIV;
    case EM_MOD: return ETO_MOD;
    case EM_POW: return ETO_POW;
    case EM_AND: return ETO_AND;
    case EM_OR: return ETO_OR;
    case EM_EOR: return ETO_EOR;
    case EM_LSL: return ETO_LSL;
    case EM_LSR: return ETO_LSR;
    case EM_ASL: return ETO_ASL;
    case EM_ASR: return ETO_ASR;
    case EM_EQ: return ETO_EQ;
    case EM_NE: return ETO_NE;
    case EM_LT: return ETO_LT;
    case EM_LE: return ETO_LE;
    case EM_GT: return ETO_GT;
    case EM_GE: return ETO_GE;
    default: return ETO_END;
    }
}

// the type so far an expression of the context starts with (the port: a
// later index is a Long, #1179)
unsigned ctxstart(em_ctx x)
{
    static const unsigned t[EM_NCTX] = { ET_BYTE, ET_WORD,  ET_LONG, ET_QUICK, ET_FLOAT, ET_LONG, ET_WORD,
                                         ET_QUICK, ET_FLOAT, 0,      ET_LONG,  ET_LONG,  0 };
    return t[x];
}

struct operand {
    exprtype_elem e;
    bool var;
};

operand element(unsigned tsf, em_kind k, const char *text)
{
    operand o{};

    o.var = k <= EM_F;
    if (o.var)
        o.e = exprtype_element(tsf, kindtype(k), nullptr);
    else {
        exprtype_constfacts c = constfacts(text);
        o.e = exprtype_element(tsf, 0, &c);
    }
    return o;
}

// the type of the cell's result by the functions, as eval5 asks them for
// one operator (0: refused)
unsigned celltype(const em_matrix &m, const em_cell &c)
{
    const em_set &s = m.sets[c.set];
    unsigned op = emop(c.op);
    em_kind rk = (em_kind)c.right;
    const char *rtext = c.op == EM_POW || (c.op >= EM_LSL && c.op <= EM_ASR) ? s.count[rk] : s.value[rk];
    operand l = element(ctxstart(c.ctx), c.left, s.value[c.left]), r;
    exprtype_left pre;
    exprtype_binfacts f{};
    unsigned left;
    int lraw;

    if (!l.e.result)
        return 0;
    // the consumer of the left operand is the operator
    lraw = l.e.raw && !exprtype_cutsraw(op);
    pre = exprtype_leftop(op, l.e.result, lraw);
    left = pre.left;
    if (pre.ownleft && l.var && l.e.conv_from && exprtype_ownleft(l.e.conv_from, l.e.conv_to))
        left = l.e.conv_from;   // cmpownleft
    if (pre.ownleft && l.e.litcut)
        left = ET_FLOAT;        // fraclitfloat (#1186)
    r = element(pre.compare ? 0 : left, rk, rtext);
    if (!r.e.result)
        return 0;
    f.lraw = (unsigned char)pre.lraw;
    // the consumer of the right operand is the pending operator
    f.rraw = r.e.raw && !exprtype_cutsraw(op);
    return exprtype_binop(op, left, r.e.result, &f).result;
}

// the text is a whole number of type t
bool fitswhole(const char *text, unsigned t)
{
    char *end;
    long v = std::strtol(text, &end, 10);

    if (end == text || *end)
        return false;
    switch (t) {
    case ET_BYTE: return v >= -128 && v <= 127;
    case ET_WORD: return v >= -32768 && v <= 32767;
    default: return v >= INT32_MIN && v <= INT32_MAX;
    }
}

struct matrixcheck {
    size_t cells = 0, refused = 0, printed = 0, typed = 0;
    std::vector<std::string> mismatches;
};

matrixcheck checkmatrix(unsigned (*plant)(unsigned) = nullptr)
{
    matrixcheck r;
    std::string text = readfile(EXPRMATRIX_TXT);
    em_matrix m{};
    char *errors = nullptr;

    REQUIRE(!text.empty());
    REQUIRE(em_parse(text.data(), text.size(), EXPRMATRIX_TXT, &m, &errors) == 0);
    for (size_t i = 0; i < m.ncells; i++) {
        const em_cell &c = m.cells[i];
        em_value v;
        unsigned t;
        bool refused;

        if (c.right < 0 || c.left == EM_S || c.right == EM_S)
            continue;           // the two-operand cells of numbers
        r.cells++;
        t = celltype(m, c);
        if (plant && t)
            t = plant(t);
        em_model(&m, &c, EM_RULES_PORTNOW, c.ctx == EM_I1 || c.ctx == EM_I2, &v);
        refused = !std::strncmp(v.text, "!noncompile", 11);
        r.refused += refused;
        if (refused != !t) {
            r.mismatches.push_back(std::string(c.key) + ": the model " + v.text + ", the functions " + tname(t));
            continue;
        }
        if (t && v.type && (c.ctx == EM_PRINT || c.ctx == EM_IF)) {
            r.typed++;
            if ((unsigned)v.type != t)
                r.mismatches.push_back(std::string(c.key) + ": the model gives the type " + tname(v.type)
                                       + ", the functions " + tname(t));
        }
        if (c.ctx != EM_PRINT || !t || v.text[0] == '?')
            continue;
        // Print shows the value in the type of the result: a whole number
        // fits its type, a fraction is a Quick or a Float
        r.printed++;
        if (t <= ET_LONG ? !fitswhole(v.text, t) : (t == ET_QUICK && std::fabs(std::atof(v.text)) > 32768.0))
            r.mismatches.push_back(std::string(c.key) + ": the model prints " + v.text + ", the functions give "
                                   + tname(t));
    }
    em_free(&m);
    return r;
}

} // namespace

TEST_CASE("exprtype: the generator mode", "[exprtype]")
{
    const char *path = std::getenv("EXPRTYPE_TSV_WRITE");

    if (path) {
        std::ofstream out(path, std::ios::binary);
        out << table();
        REQUIRE(out.good());
    }
}

TEST_CASE("exprtype: every decision as the table records it", "[exprtype]")
{
    std::string want = readfile(EXPRTYPE_TSV);

    REQUIRE(!want.empty());
    for (const std::string &d : differences(want, table()))
        FAIL_CHECK(d);
}

TEST_CASE("exprtype: a changed row of the table fails", "[exprtype]")
{
    std::string got = table(), want = got;
    // the row of w+w: a Long ("now a long", doplusw) - recorded as a Word
    const std::string from = "binop\t+\tW\tW\t00000000\tplain\t-\t00\tW>W\t-\t00\tW\tL\t000\n";
    size_t at = want.find(from);

    REQUIRE(at != std::string::npos);
    want.replace(at, from.size(), "binop\t+\tW\tW\t00000000\tplain\t-\t00\tW>W\t-\t00\tW\tW\t000\n");
    std::vector<std::string> d = differences(want, got);
    REQUIRE(d.size() == 1);
    CHECK(d[0].find("function 'binop\t+\tW\tW") != std::string::npos);
}

TEST_CASE("exprtype: decisions named in expression-types.md", "[exprtype]")
{
    exprtype_binfacts f{};

    // r.l=q*2: the Quick held raw under the Long, the operation mixed (#1178)
    exprtype_elem q = exprtype_element(ET_LONG, ET_QUICK, nullptr);
    CHECK(q.raw == 1);
    CHECK(q.result == ET_LONG);
    f.lraw = 1;
    exprtype_bin d = exprtype_binop(ETO_MUL, ET_LONG, ET_LONG, &f);
    CHECK(d.path == ETP_MIXED);
    CHECK(d.result == ET_LONG);
    // Word+Word is a Long, Byte+Byte a Word (doplusw, doplusb)
    f = {};
    CHECK(exprtype_binop(ETO_ADD, ET_WORD, ET_WORD, &f).result == ET_LONG);
    CHECK(exprtype_binop(ETO_ADD, ET_BYTE, ET_BYTE, &f).result == ET_WORD);
    // a shift of a Byte gives a Long (shpoke), of a Quick a Quick
    CHECK(exprtype_binop(ETO_LSL, ET_BYTE, ET_BYTE, &f).result == ET_LONG);
    CHECK(exprtype_binop(ETO_LSL, ET_QUICK, ET_QUICK, &f).result == ET_QUICK);
    // a Float under AND: Illegal Operator for Type
    CHECK(exprtype_binop(ETO_AND, ET_LONG, ET_FLOAT, &f).result == 0);
    // r.l=(q>1): the raw Quick a Quick, the right operand from type 0, the
    // Byte constant up to the Quick, the boolean a Word (#1177)
    exprtype_left pre = exprtype_leftop(ETO_GT, ET_LONG, 1);
    CHECK(pre.left == ET_QUICK);
    CHECK(pre.lraw == 0);
    CHECK(pre.compare == 1);
    d = exprtype_binop(ETO_GT, ET_QUICK, ET_BYTE, &f);
    CHECK(d.path == ETP_COMPARE);
    CHECK(d.rup_from == ET_BYTE);
    CHECK(d.rup_to == ET_QUICK);
    CHECK(d.result == ET_WORD);
    // a Long and a Quick compared exactly (cmplq)
    CHECK(exprtype_binop(ETO_LT, ET_LONG, ET_QUICK, &f).path == ETP_CMPLQ);
    // rl.l=lw AND (l1=l1): the boolean up to the Long (#1182)
    f.rcmp = 1;
    d = exprtype_binop(ETO_AND, ET_LONG, ET_WORD, &f);
    CHECK(d.rup_from == ET_WORD);
    CHECK(d.rup_to == ET_LONG);
    CHECK(d.handler == ET_LONG);
    CHECK(d.cmpres == 0);
    // a whole number beyond a Word under a Quick is a Long ("sic"), and
    // q*100000 then takes the Quick with its fraction (#1178)
    exprtype_constfacts k{};
    k.size = ET_LONG;
    CHECK(exprtype_element(ET_QUICK, 0, &k).result == ET_LONG);
    f = {};
    d = exprtype_binop(ETO_MUL, ET_QUICK, ET_LONG, &f);
    CHECK(d.path == ETP_MIXED);
    CHECK(d.lq == 1);
    CHECK(d.result == ET_LONG);
    // rb.b=(1.5+1.5): a fraction literal under a Byte is a Float (#1188);
    // 2.0 the whole number (litcut, #1186); Data.b 1.5 cut
    k = {};
    k.point = k.fix = 1;
    k.size = ET_BYTE;
    CHECK(exprtype_element(ET_BYTE, 0, &k).result == ET_FLOAT);
    k.whole = 1;
    CHECK(exprtype_element(ET_BYTE, 0, &k).result == ET_BYTE);
    CHECK(exprtype_element(ET_BYTE, 0, &k).litcut == 1);
    k.whole = 0;
    k.data = 1;
    CHECK(exprtype_element(ET_BYTE, 0, &k).result == ET_BYTE);
}

TEST_CASE("exprtype: the two-operand cells of the expression matrix against its model", "[exprtype]")
{
    matrixcheck r = checkmatrix();

    std::printf("expression matrix: %zu two-operand cells, %zu refused, %zu printed values and %zu types checked\n",
                r.cells, r.refused, r.printed, r.typed);
    // the cells of matrix.txt: two operands, no string
    CHECK(r.cells > 4000);
    CHECK(r.refused > 0);
    CHECK(r.printed > 0);
    // the type so far of the Print and If cells (the destination contexts convert to theirs)
    CHECK(r.typed > 4000);
    for (const std::string &m : r.mismatches)
        FAIL_CHECK(m);
}

TEST_CASE("exprtype: a wrong result type that keeps the value fails the type check", "[exprtype]")
{
    // a Long result given as a Word: the value the model prints still fits
    matrixcheck r = checkmatrix([](unsigned t) -> unsigned { return t == ET_LONG ? (unsigned)ET_WORD : t; });
    size_t types = 0;

    for (const std::string &m : r.mismatches)
        if (m.find("the model gives the type L, the functions W") != std::string::npos)
            types++;
    CHECK(types > 100);
}
