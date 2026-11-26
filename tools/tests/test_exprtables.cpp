// test_exprtables - the readable tables of the type rules of an expression
// (lib/exprtype_tables): every cell of the tables by today's rules from
// exprtype.c is the cell from the rows of its committed table
// programs/blitz2/tests/host/exprtype.tsv (each decision the tables take
// has its row there), the page from both is the same, a changed row
// changes the page and a missing one makes none; cells named in
// docs/blitz2/expression-types.md; the comparison of a committed page.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprtype_tables.h"
}

namespace {

std::string readfile(const char *path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream s;

    s << in.rdbuf();
    return s.str();
}

struct tsvset {
    ett_tsv *t = nullptr;
    ett_prims p{};
    ett_rules r{};

    explicit tsvset(const std::string &text)
    {
        char *errors = nullptr;

        REQUIRE(ett_tsv_load(text.data(), text.size(), "exprtype.tsv", &t, &errors) == 0);
        ett_tsv_prims(t, &p);
        ett_rules_flow(&r, &p);
    }
    ~tsvset() { ett_tsv_free(t); }
    tsvset(const tsvset &) = delete;
    tsvset &operator=(const tsvset &) = delete;
};

struct today {
    ett_prims p{};
    ett_rules r{};

    today()
    {
        ett_prims_exprtype(&p);
        ett_rules_flow(&r, &p);
    }
};

std::string page(const ett_rules &r, ett_stats *st = nullptr)
{
    char *error = nullptr;
    char *p = ett_page(&r, st, &error);
    std::string s;

    if (!p) {
        FAIL_CHECK(error ? error : "no page");
        std::free(error);
        return s;
    }
    s = p;
    std::free(p);
    return s;
}

std::string cellkey(unsigned op, unsigned tsf, unsigned l, unsigned r, const char *facts)
{
    return std::string(ett_tname(l)) + " " + ett_oname(op) + " " + ett_tname(r) + " under " + ett_tname(tsf) + facts;
}

bool same(const ett_cell &a, const ett_cell &b)
{
    return a.result == b.result && a.l.as == b.l.as && a.l.how == b.l.how && a.r.as == b.r.as && a.r.how == b.r.how
           && a.exact == b.exact && a.wordop == b.wordop && a.byteext == b.byteext;
}

ett_cell cellof(const ett_rules &r, unsigned op, unsigned tsf, unsigned l, unsigned rt, const ett_facts *x = nullptr)
{
    ett_cell c{};

    REQUIRE(r.cell(r.ctx, op, tsf, l, rt, x, &c) == 0);
    return c;
}

} // namespace

TEST_CASE("exprtables: every cell from exprtype.c is the cell from exprtype.tsv", "[exprtables]")
{
    tsvset tsv(readfile(EXPRTYPE_TSV));
    today now;
    const ett_facts none{}, rcmp{ 1, 0, 0 }, words{ 0, 1, 0 }, b978{ 0, 0, 1 };
    // the cells the page shows: every operation; a comparison as the right
    // operand (a Word); two Byte or Word values in a later index (a Long);
    // a Byte value and a constant from 128 to 255 compared under a Word
    // (exprtype.tsv has the facts of #978 for two Words only)
    const struct {
        const ett_facts *x;
        const char *name;
    } facts[] = { { &none, "" }, { &rcmp, " (a comparison right)" }, { &words, " (a later index)" },
                  { &b978, " (a constant 128-255 right)" } };
    size_t cells = 0;

    for (unsigned op = 0; op < ETO_COUNT; op++)
        for (unsigned tsf = ET_NONE; tsf <= ET_FLOAT; tsf++)
            for (unsigned l = ET_BYTE; l <= ET_FLOAT; l++)
                for (unsigned r = ET_BYTE; r <= ET_FLOAT; r++)
                    for (const auto &f : facts) {
                        ett_cell a{}, b{};

                        if ((f.x->rcmp && r != ET_WORD)
                            || (f.x->words && (tsf != ET_LONG || l > ET_WORD || r > ET_WORD))
                            || (f.x->byte978 && (tsf != ET_WORD || l != ET_BYTE || r != ET_WORD)))
                            continue;
                        REQUIRE(now.r.cell(now.r.ctx, op, tsf, l, r, f.x, &a) == 0);
                        if (tsv.r.cell(tsv.r.ctx, op, tsf, l, r, f.x, &b)) {
                            FAIL_CHECK("no row for " + cellkey(op, tsf, l, r, f.name) + ": "
                                       + ett_tsv_first_missing(tsv.t));
                            continue;
                        }
                        cells++;
                        if (!same(a, b))
                            FAIL_CHECK(cellkey(op, tsf, l, r, f.name) + ": the cells differ");
                    }
    for (unsigned tsf = ET_NONE; tsf <= ET_FLOAT; tsf++)
        for (unsigned own = ET_BYTE; own <= ET_FLOAT; own++) {
            ett_elem a{}, b{};

            REQUIRE(now.r.element(now.r.ctx, tsf, own, &a) == 0);
            REQUIRE(tsv.r.element(tsv.r.ctx, tsf, own, &b) == 0);
            CHECK(std::memcmp(&a, &b, sizeof a) == 0);
        }
    CHECK(ett_tsv_missing(tsv.t) == 0);
    std::printf("exprtype.tsv: %zu rows, %zu of them behind %zu cells\n", ett_tsv_rows(tsv.t), ett_tsv_used(tsv.t),
                cells);
    CHECK(cells == ETO_COUNT * (6 * 5 * 5 + 6 * 5 + 4 + 1));
}

TEST_CASE("exprtables: the page from exprtype.tsv is the page from exprtype.c", "[exprtables]")
{
    tsvset tsv(readfile(EXPRTYPE_TSV));
    today now;
    ett_stats st{};
    std::string a = page(now.r, &st), b = page(tsv.r);

    REQUIRE(!a.empty());
    CHECK(a == b);
    CHECK(ett_tsv_missing(tsv.t) == 0);
    // the decisions behind the page: rows of every kind but optype (the
    // results of binop include it)
    CHECK(ett_tsv_used(tsv.t) > 500);
    for (unsigned g = 0; g < ETG_COUNT; g++) {
        std::printf("group %u: %u tables, %u rows, %u cells, %u differ swapped\n", g, st.tables[g], st.rows[g],
                    st.cells[g], st.swapped[g]);
        CHECK(st.tables[g] > 0);
        CHECK(st.rows[g] == 25 * st.tables[g]);
        CHECK(st.cells[g] == 6 * st.rows[g]);
    }
}

TEST_CASE("exprtables: a changed row of exprtype.tsv changes the page", "[exprtables]")
{
    std::string text = readfile(EXPRTYPE_TSV);
    // l+l: a Long - recorded as a Float
    const std::string from = "binop\t+\tL\tL\t00000000\tplain\t-\t00\tL>L\t-\t00\tL\tL\t000\n";
    size_t at = text.find(from);

    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), "binop\t+\tL\tL\t00000000\tplain\t-\t00\tL>L\t-\t00\tL\tF\t000\n");
    tsvset tsv(text);
    today now;
    std::string want = page(now.r), got = page(tsv.r);
    char *report = nullptr;

    REQUIRE(ett_page_compare(want.data(), want.size(), got.data(), got.size(), "page.md", &report) == 1);
    REQUIRE(report != nullptr);
    CHECK(std::strstr(report, "page.md:") != nullptr);
    CHECK(std::strstr(report, "L L \xe2\x86\x92 F") != nullptr);
    std::free(report);
}

TEST_CASE("exprtables: a decision without its row in exprtype.tsv makes no page", "[exprtables]")
{
    std::string text = readfile(EXPRTYPE_TSV);
    const std::string row = "element\tL\tQ\t-\tL\t100\n";
    size_t at = text.find(row);
    char *error = nullptr;

    REQUIRE(at != std::string::npos);
    text.erase(at, row.size());
    tsvset tsv(text);
    char *p = ett_page(&tsv.r, nullptr, &error);
    CHECK(p == nullptr);
    REQUIRE(error != nullptr);
    CHECK(std::strstr(error, "no decision") != nullptr);
    CHECK(ett_tsv_missing(tsv.t) > 0);
    CHECK(std::string(ett_tsv_first_missing(tsv.t)) == "element\tL\tQ");
    std::free(p);
    std::free(error);
}

TEST_CASE("exprtables: a malformed row of exprtype.tsv is named", "[exprtables]")
{
    const std::string text = "# a comment\nbinop\t+\n" "frob\tx\ty\n";
    ett_tsv *t = nullptr;
    char *errors = nullptr;

    CHECK(ett_tsv_load(text.data(), text.size(), "x.tsv", &t, &errors) == 1);
    CHECK(t == nullptr);
    REQUIRE(errors != nullptr);
    CHECK(std::strstr(errors, "x.tsv:2: a row without its result") != nullptr);
    CHECK(std::strstr(errors, "x.tsv:3: unknown row") != nullptr);
    std::free(errors);
}

TEST_CASE("exprtables: cells named in expression-types.md", "[exprtables]")
{
    today now;
    const ett_rules &r = now.r;
    ett_cell c;

    // r.l=q*2 (a Quick and a Byte value under a Long): the Quick with its
    // fraction, the result a Long rounded at the operator (#1178)
    c = cellof(r, ETO_MUL, ET_LONG, ET_QUICK, ET_BYTE);
    CHECK(c.l.as == ET_QUICK);
    CHECK(c.l.how == ETT_RAW);
    CHECK(c.result == ET_LONG);
    // Word+Word is a Long, Byte+Byte a Word (doplusw, doplusb)
    CHECK(cellof(r, ETO_ADD, ET_NONE, ET_WORD, ET_WORD).result == ET_LONG);
    c = cellof(r, ETO_ADD, ET_NONE, ET_BYTE, ET_BYTE);
    CHECK(c.l.as == ET_BYTE);
    CHECK(c.result == ET_WORD);
    // a Quick under a Long before AND: rounded down (#1178), the Long
    // operation
    c = cellof(r, ETO_AND, ET_LONG, ET_QUICK, ET_LONG);
    CHECK(c.l.as == ET_LONG);
    CHECK(c.l.how == ETT_KEEP);
    CHECK(c.result == ET_LONG);
    // a Float under AND: Illegal Operator for Type
    CHECK(cellof(r, ETO_AND, ET_NONE, ET_LONG, ET_FLOAT).result == 0);
    // r.l=(q>l): the Quick its own, the Long against it exactly (#1177)
    c = cellof(r, ETO_GT, ET_LONG, ET_QUICK, ET_LONG);
    CHECK(c.l.as == ET_QUICK);
    CHECK(c.r.as == ET_LONG);
    CHECK(c.exact == 1);
    CHECK(c.result == ET_WORD);
    // rf.f=(l2=l3): two Longs compared as Longs under a Float (#1177)
    c = cellof(r, ETO_EQ, ET_FLOAT, ET_LONG, ET_LONG);
    CHECK(c.l.as == ET_LONG);
    CHECK(c.r.as == ET_LONG);
    // rl.l=lw AND (l1=l1): the boolean up to the Long (#1182)
    const ett_facts rcmp{ 1, 0, 0 };
    c = cellof(r, ETO_AND, ET_LONG, ET_LONG, ET_WORD, &rcmp);
    CHECK(c.r.as == ET_LONG);
    CHECK(c.result == ET_LONG);
    // a.b=$90 : If a=$90 (#978)
    const ett_facts b978{ 0, 0, 1 };
    CHECK(cellof(r, ETO_EQ, ET_WORD, ET_BYTE, ET_WORD, &b978).byteext == ETB_RIGHT);
    CHECK(cellof(r, ETO_LT, ET_WORD, ET_BYTE, ET_WORD, &b978).byteext == ETB_NONE);
    // g(1,y*w): two Word values in a later index (#1179)
    const ett_facts words{ 0, 1, 0 };
    CHECK(cellof(r, ETO_MUL, ET_LONG, ET_WORD, ET_WORD, &words).wordop == 1);
    // rb.b=(1.5+1.5): a constant with a fraction under a Byte is a Float
    // (#1188), 2.0 the whole number (#1186)
    exprtype_constfacts k{};
    ett_elem e{};
    k.point = k.fix = k.qfit = 1;
    k.size = ET_BYTE;
    REQUIRE(r.constant(r.ctx, ET_BYTE, &k, &e) == 0);
    CHECK(e.result == ET_FLOAT);
    k.whole = 1;
    REQUIRE(r.constant(r.ctx, ET_BYTE, &k, &e) == 0);
    CHECK(e.result == ET_BYTE);
    CHECK(e.litcut == 1);
    // a Quick under a Long is held with its fraction (#1178)
    REQUIRE(r.element(r.ctx, ET_LONG, ET_QUICK, &e) == 0);
    CHECK(e.raw == 1);
    CHECK(e.result == ET_LONG);
}

TEST_CASE("exprtables: the page shows the exceptions with their issues", "[exprtables]")
{
    today now;
    std::string p = page(now.r);

    for (const char *issue : { "#978", "#1177", "#1178", "#1179", "#1182", "#1183", "#1186", "#1188", "#1209" })
        CHECK(p.find(issue) != std::string::npos);
    CHECK(p.find("](expression-types.md") != std::string::npos);
    // one table per group at least, the legend's marks used
    CHECK(p.find("### `+`") != std::string::npos);
    CHECK(p.find("Q~") != std::string::npos);
    CHECK(p.find("illegal") != std::string::npos);
}

TEST_CASE("exprtables: the comparison of a committed page", "[exprtables]")
{
    const std::string got = "a\nb\nc\n";
    char *report = nullptr;

    CHECK(ett_page_compare(got.data(), got.size(), got.data(), got.size(), "p.md", &report) == 0);
    CHECK(report == nullptr);
    const std::string changed = "a\nB\nc\n";
    REQUIRE(ett_page_compare(changed.data(), changed.size(), got.data(), got.size(), "p.md", &report) == 1);
    CHECK(std::strstr(report, "p.md:2: differs from the generated page: B\n  generated: b\n") != nullptr);
    CHECK(std::strstr(report, "p.md: lines that differ from the generated page: 1\n") != nullptr);
    std::free(report);
    const std::string shorter = "a\nb\n";
    REQUIRE(ett_page_compare(shorter.data(), shorter.size(), got.data(), got.size(), "p.md", &report) == 1);
    CHECK(std::strstr(report, "p.md:3: missing: c\n") != nullptr);
    std::free(report);
    const std::string longer = "a\nb\nc\nd\n";
    REQUIRE(ett_page_compare(longer.data(), longer.size(), got.data(), got.size(), "p.md", &report) == 1);
    CHECK(std::strstr(report, "p.md:4: not in the generated page: d\n") != nullptr);
    std::free(report);
}

// ----------------------------------------------------------------------------
// the candidate rules of expr-tree-5 (task 2.1): the same tables, marked

extern "C" {
#include "exprmatrix.h"
}

namespace {

int mark_wmul(void *, unsigned op, unsigned want, unsigned l, unsigned r)
{
    return op == ETO_MUL && want == ET_WORD && l == ET_WORD && r == ET_WORD;
}

int beside(void *ctx, unsigned want, unsigned partner, const char *text)
{
    return em_cand_consttype(*static_cast<uint64_t *>(ctx), (int)want, (int)partner, text);
}

std::string candpage(int n, struct ett_stats *st, int (*mark)(void *, unsigned, unsigned, unsigned, unsigned))
{
    struct ett_prims p;
    struct ett_rules base, r;
    struct ett_cand c;
    uint64_t rules = EM_RULES_CANDBASE | EM_R_CAND(n);
    char *error = nullptr;

    ett_prims_exprtype(&p);
    ett_rules_flow(&base, &p);
    c.rules = rules;
    c.base = &base;
    ett_rules_cand(&r, &c);
    struct ett_pageopts o = { "Candidate", "About.\n", "## Notes\n", &base, mark, nullptr, beside, &rules, nullptr, nullptr, nullptr, nullptr, nullptr };
    char *page = ett_page_opts(&r, &o, st, &error);
    REQUIRE(page != nullptr);
    std::string s = page;
    std::free(page);
    return s;
}

}  // namespace

TEST_CASE("exprtables: a candidate's tables beside today's", "[exprtables][cand]")
{
    struct ett_stats st, st2;
    std::string c1 = candpage(EM_CAND_C1, &st, mark_wmul), c2 = candpage(EM_CAND_C2, &st2, nullptr);

    // its own heading and notes, not today's
    CHECK(c1.rfind("# Candidate\n\nAbout.\n", 0) == 0);
    CHECK(c1.find("## The exceptions and their issues") == std::string::npos);
    CHECK(c1.find("## Notes") != std::string::npos);
    CHECK(c1.find("## Constants beside an operand") != std::string::npos);
    // order independence: no cell differs from its swapped operation in
    // the arithmetic and the comparisons
    CHECK(st.swapped[ETG_ARITH] == 0);
    CHECK(st.swapped[ETG_COMPARE] == 0);
    CHECK(st2.swapped[ETG_ARITH] == 0);
    // today's order-dependent cells are among the changed ones: l+q and q+l
    // under no wanted type are one exact Long
    CHECK(st.changed[ETG_ARITH] > 0);
    CHECK(c1.find("| Q | L | Q~ L \xe2\x86\x92 L \xe2\x97\x86 |") != std::string::npos);
    // the mark: Word*Word under a wanted Word (W W W, one cell)
    CHECK(st.marked[ETG_ARITH] == 1);
    CHECK(st.marked[ETG_COMPARE] == 0);
    // C2: Word+Word a Word
    CHECK(c2.find("| W | W | W W \xe2\x86\x92 W") != std::string::npos);
    // today's page is untouched by the options
    struct ett_prims p;
    struct ett_rules r;
    char *error = nullptr;
    ett_prims_exprtype(&p);
    ett_rules_flow(&r, &p);
    char *today = ett_page(&r, nullptr, &error);
    REQUIRE(today != nullptr);
    CHECK(std::string(today).find("\xe2\x97\x86") == std::string::npos);
    std::free(today);
}
