// test_exprarith - the arithmetic of the formats of rule P+ (programs/
// blitz2/src/exprarith.c; change expr-tree-5-operand-typing, task 3.4):
// every routine against the values of the expression matrix's model
// (tools/lib/exprmatrix/model.c, its own implementation of the 68k
// routines): 16.16 (qmul, qdivide with its last bit, the exact Mod, today's
// Quick Mod), 48.16 (a Long beside a Quick under P+, EM_RULES_DECIDED,
// with the consumer's rounding), the routines of #1178 (today's rules) and
// the whole-number operations, for random operands and the edge values
// (a divisor 0, the largest numbers); and the casts and the fast-path test
// on their own.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprarith.h"
#include "exprmatrix.h"
}

namespace {

const unsigned DEC = EM_RULES_DECIDED, NOW = EM_RULES_CANDBASE;

// the value text the model gives for text under ctx by rules
std::string model(const char *text, const std::string &vars, enum em_ctx ctx, unsigned rules)
{
    struct em_expr *x = nullptr;
    char *err = nullptr;
    struct em_exprval v;

    if (em_expr_parse(text, vars.c_str(), &x, &err)) {
        std::string e = std::string("(parse: ") + (err ? err : "out of memory") + ")";
        std::free(err);
        return e;
    }
    em_expr_eval(x, ctx, rules, &v);
    em_expr_free(x);
    return v.v.text;
}

// a 16.16 value of raw bits as exact decimal text (1/65536 = 5^16/10^16)
std::string q16text(int32_t raw)
{
    uint32_t m = raw < 0 ? (uint32_t)-(int64_t)raw : (uint32_t)raw;
    char s[48];

    std::snprintf(s, sizeof s, "%s%u.%016llu", raw < 0 ? "-" : "", m >> 16,
                  (unsigned long long)(m & 0xffff) * 152587890625ull);
    return s;
}

std::string hex(int32_t v)
{
    char s[16];

    std::snprintf(s, sizeof s, "%08X", (unsigned)v);
    return s;
}

std::string dec(int32_t v)
{
    return std::to_string(v);
}

// the model's text of a value r of a destination (DB DW DL: decimal of the
// type's bits; DQ: the raw bits), a trap "!trap"
std::string dest(enum em_ctx ctx, int32_t r, unsigned flags)
{
    if (flags & EXA_TRAP)
        return "!trap";
    switch (ctx) {
    case EM_DB: return dec((int8_t)r);
    case EM_DW: return dec((int16_t)r);
    case EM_DL: return dec(r);
    default: return hex(r);
    }
}

struct rng {
    uint32_t s = 20261125;
    uint32_t next()
    {
        s = s * 1103515245u + 12345u;
        return s ^ (s >> 15);
    }
    // a Quick's bits of any size, now and then a whole one or 0
    int32_t quick(int i)
    {
        int32_t v = (int32_t)next() >> (next() % 24);

        if (i % 37 == 0)
            return 0;
        if (i % 11 == 0)
            v &= ~0xffff;
        return v;
    }
    int32_t whole(int bits)
    {
        return (int32_t)next() >> (32 - bits);
    }
};

const char *opname(unsigned op)
{
    switch (op) {
    case ETO_ADD: return "+";
    case ETO_SUB: return "-";
    case ETO_MUL: return "*";
    case ETO_DIV: return "/";
    default: return " MOD ";
    }
}

} // namespace

TEST_CASE("exprarith: 16.16 as today's routines (qmul, qdivide, + -)", "[exprarith]")
{
    rng g;
    int differ = 0;

    for (int i = 0; i < 1500; i++) {
        int32_t a = g.quick(i + 1), b = g.quick(i);
        std::string vars = "q=" + q16text(a) + " p=" + q16text(b);

        for (unsigned op : { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV }) {
            std::string text = std::string("q") + opname(op) + "p";
            unsigned f = 0;
            std::string mine = hex(exprarith_q16(op, a, b, &f));

            for (unsigned rules : { DEC, NOW }) {
                std::string m = model(text.c_str(), vars, EM_DQ, rules);

                if (m != mine && differ++ < 10)
                    FAIL_CHECK(text + " " + vars + ": the model " + m + ", exprarith " + mine);
            }
        }
    }
    CHECK(differ == 0);
    // qdivide's last bit: 1/3.25 is $00004EC5 (exact $00004EC4)
    unsigned f = 0;
    CHECK(exprarith_q16(ETO_DIV, 0x10000, 0x34000, &f) == 0x4ec5);
    // a divisor 0: the largest number with the dividend's sign (#200)
    CHECK(exprarith_q16(ETO_DIV, 0x10000, 0, &f) == 0x7fffffff);
    CHECK(exprarith_q16(ETO_DIV, -0x10000, 0, &f) == (int32_t)0x80000001);
    CHECK((f & EXA_DIV0) != 0);
}

TEST_CASE("exprarith: the Quick Mod, exact (P+) and today's (domodquick)", "[exprarith]")
{
    rng g;
    int differ = 0;

    for (int i = 0; i < 1500; i++) {
        int32_t a = g.quick(i + 1), b = g.quick(i);
        std::string vars = "q=" + q16text(a) + " p=" + q16text(b);
        unsigned f = 0, ft = 0;
        int32_t exact = exprarith_q16(ETO_MOD, a, b, &f);
        int32_t today = exprarith_qmod_today(a, b, &ft);
        std::string m = model("q MOD p", vars, EM_DQ, DEC), n = model("q MOD p", vars, EM_DQ, NOW);
        // today's Mod gives a Word, the destination takes it as a Quick
        std::string mine = dest(EM_DQ, exact, f), mine_now = dest(EM_DQ, (int32_t)((uint32_t)today << 16), ft);

        if (m != mine && differ++ < 10)
            FAIL_CHECK("q MOD p " + vars + ": the model (P+) " + m + ", exprarith " + mine);
        if (n != mine_now && differ++ < 10)
            FAIL_CHECK("q MOD p " + vars + ": the model (today) " + n + ", exprarith " + mine_now);
    }
    CHECK(differ == 0);
    unsigned f = 0;
    // q Mod 0.5 exact (today it traps: the divisor's whole part is 0)
    CHECK(exprarith_q16(ETO_MOD, 0x78000, 0x8000, &f) == 0);
    CHECK(f == 0);
    exprarith_qmod_today(0x78000, 0x8000, &f);
    CHECK(f == EXA_TRAP);
    // today's sign rule: -7 Mod 2 is 1
    f = 0;
    CHECK(exprarith_q16(ETO_MOD, -7 * 65536, 2 * 65536, &f) == 65536);
}

TEST_CASE("exprarith: 48.16 with the consumer's rounding (P+)", "[exprarith]")
{
    // a Long and a Quick under P+: 48.16, under a wanted Long rounded down
    // (a / towards zero), under a wanted Quick, Word or Byte computed 48.16
    // too (a Long takes part) and then the 16.16 value or the whole part
    rng g;
    int differ = 0, compared = 0;

    for (int i = 0; i < 1200; i++) {
        int32_t l = g.whole(1 + (int)(g.next() % 24)), q = g.quick(i);
        std::string vars = "l.l=" + dec(l) + " q=" + q16text(q);

        if (i % 53 == 0)
            l = 0;
        vars = "l.l=" + dec(l) + " q=" + q16text(q);
        for (unsigned op : { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV, ETO_MOD })
            for (int order = 0; order < 2; order++) {
                std::string text = order ? std::string("q") + opname(op) + "l" : std::string("l") + opname(op) + "q";
                int64_t x = exprarith_whole_q48(l), y = exprarith_q16_q48(q);

                if (order)
                    std::swap(x, y);
                for (enum em_ctx ctx : { EM_DL, EM_DQ, EM_DW, EM_DB }) {
                    unsigned want = ctx == EM_DL ? ET_LONG : ctx == EM_DQ ? ET_QUICK : ctx == EM_DW ? ET_WORD : ET_BYTE;
                    unsigned f = 0;
                    int64_t v = exprarith_q48(op, x, y, exprarith_div0(want), &f);
                    int32_t r;

                    if (want == ET_QUICK)
                        r = exprarith_q48_q16(v, &f);
                    else if (op == ETO_DIV && want == ET_LONG)
                        r = exprarith_q48_trunc(v, ET_LONG, &f);
                    else
                        r = exprarith_q48_floor(v, ET_LONG, &f);
                    std::string m = model(text.c_str(), vars, ctx, DEC), mine = dest(ctx, r, 0);

                    compared++;
                    if (m != mine && differ++ < 10)
                        FAIL_CHECK(text + " " + vars + " " + em_ctx_name(ctx) + ": the model " + m + ", exprarith "
                                   + mine);
                }
            }
    }
    CHECK(differ == 0);
    CHECK(compared > 40000);
    // a division by 0 in the wanted type's format with the dividend's sign
    unsigned f = 0;
    CHECK(exprarith_q48(ETO_DIV, -100 * 65536, 0, exprarith_div0(ET_WORD), &f) == -(int64_t)32767 * 65536);
    CHECK(exprarith_q48(ETO_DIV, 100 * 65536, 0, exprarith_div0(ET_QUICK), &f) == 0x7fffffff);
    CHECK((f & EXA_DIV0) != 0);
    // a Mod by 0: the dividend
    CHECK(exprarith_q48(ETO_MOD, 100000LL * 65536, 0, 0, &f) == 100000LL * 65536);
    // the product keeps its 48 bits (q*q2*l: 300*300*2 = 180000)
    f = 0;
    int64_t p = exprarith_q48(ETO_MUL, exprarith_q16_q48(300 << 16), exprarith_q16_q48(300 << 16), 0, &f);
    p = exprarith_q48(ETO_MUL, p, exprarith_whole_q48(2), 0, &f);
    CHECK(exprarith_q48_floor(p, ET_LONG, &f) == 180000);
    CHECK(f == 0);
}

TEST_CASE("exprarith: 48.16 chains keep a 48-bit whole part (P+)", "[exprarith]")
{
    // "expr-tree-5 group 3 findings: four rule points of P+", 4: an
    // intermediate beyond 32 bits keeps its whole part; the chains with a
    // Long and Quicks under a wanted Long (a / towards zero at the
    // operator), Word and Quick (computed 48.16: a Long takes part)
    rng g;
    int differ = 0, compared = 0;

    for (int i = 0; i < 1500; i++) {
        int32_t l = g.whole(1 + (int)(g.next() % 31)), l2 = g.whole(1 + (int)(g.next() % 20)), q = g.quick(i),
                p = g.quick(i + 5);

        if (i % 47 == 0)
            l2 = 0;
        std::string vars = "l.l=" + dec(l) + " l2.l=" + dec(l2) + " q=" + q16text(q) + " p=" + q16text(p);
        for (enum em_ctx ctx : { EM_DL, EM_DW, EM_DQ }) {
            unsigned want = ctx == EM_DL ? ET_LONG : ctx == EM_DW ? ET_WORD : ET_QUICK;
            int64_t L = exprarith_whole_q48(l), L2 = exprarith_whole_q48(l2), Q = exprarith_q16_q48(q),
                    P = exprarith_q16_q48(p), d0 = exprarith_div0(want);
            unsigned f = 0;
            // a / under a wanted Long: a whole number at once
            auto div = [&](int64_t a, int64_t b) {
                int64_t v = exprarith_q48(ETO_DIV, a, b, d0, &f);
                return want == ET_LONG ? exprarith_whole_q48(exprarith_q48_trunc(v, ET_LONG, &f)) : v;
            };
            auto take = [&](int64_t v, bool topdiv) {
                if (want == ET_QUICK)
                    return exprarith_q48_q16(v, &f);
                return topdiv && want == ET_LONG ? exprarith_q48_trunc(v, ET_LONG, &f)
                                                 : exprarith_q48_floor(v, ET_LONG, &f);
            };
            const struct {
                const char *text;
                int32_t r;
            } forms[] = {
                { "l*q/l2", take(div(exprarith_q48(ETO_MUL, L, Q, 0, &f), L2), true) },
                { "l*q*p", take(exprarith_q48(ETO_MUL, exprarith_q48(ETO_MUL, L, Q, 0, &f), P, 0, &f), false) },
                { "l*q-l2*p",
                  take(exprarith_q48(ETO_SUB, exprarith_q48(ETO_MUL, L, Q, 0, &f), exprarith_q48(ETO_MUL, L2, P, 0, &f),
                                     0, &f),
                       false) },
                { "(l+q)*p", take(exprarith_q48(ETO_MUL, exprarith_q48(ETO_ADD, L, Q, 0, &f), P, 0, &f), false) },
                { "l/q+l2", take(exprarith_q48(ETO_ADD, div(L, Q), L2, 0, &f), false) },
                { "(l*q) MOD l2", take(exprarith_q48(ETO_MOD, exprarith_q48(ETO_MUL, L, Q, 0, &f), L2, 0, &f), false) },
            };

            for (const auto &fm : forms) {
                std::string m = model(fm.text, vars, ctx, DEC), mine = dest(ctx, fm.r, 0);

                compared++;
                if (m != mine && differ++ < 10)
                    FAIL_CHECK(std::string(fm.text) + " " + vars + " " + em_ctx_name(ctx) + ": the model " + m
                               + ", exprarith " + mine);
            }
        }
    }
    CHECK(differ == 0);
    CHECK(compared > 25000);
    CHECK(model("l*q/l2", "l.l=2000000000 q=2 l2.l=4", EM_DL, DEC) == "1000000000");
}

TEST_CASE("exprarith: no wanted type takes a Quick when it fits, else the whole part", "[exprarith]")
{
    unsigned f = 0;
    int quick = 0;

    CHECK(exprarith_q48_fit(exprarith_whole_q48(180000), &quick, &f) == 180000);
    CHECK(quick == 0);
    CHECK(model("l*q", "l.l=120000 q=1.5", EM_PRINT, DEC) == "180000");
    CHECK(exprarith_q48_fit(exprarith_q48(ETO_MUL, exprarith_whole_q48(-3), 0x8000, 0, &f), &quick, &f)
          == (int32_t)0xfffe8000);
    CHECK(quick == 1);
    // rounded down: -100000.5 is -100001
    CHECK(exprarith_q48_fit(exprarith_whole_q48(-100000) - 0x8000, &quick, &f) == -100001);
    CHECK(model("l-q", "l.l=-100000 q=0.5", EM_PRINT, DEC) == "-100001");
}

TEST_CASE("exprarith: the routines of #1178 (today's rules)", "[exprarith]")
{
    // today: a Quick under a Long type so far by mixmult, qqmult, lqdiv,
    // longdiv (mixedop): the cells r.l=l*q, q*l, l/q, q/l, q*p, l+q
    rng g;
    int differ = 0;
    static const struct {
        const char *text;
        unsigned op;
        int lq, rq;
    } forms[] = {
        { "l*q", ETO_MUL, 0, 1 }, { "q*l", ETO_MUL, 1, 0 }, { "l/q", ETO_DIV, 0, 1 }, { "q/l", ETO_DIV, 1, 0 },
        { "q*p", ETO_MUL, 1, 1 }, { "q/p", ETO_DIV, 1, 1 }, { "l+q", ETO_ADD, 0, 1 }, { "q-l", ETO_SUB, 1, 0 },
    };

    for (int i = 0; i < 1500; i++) {
        int32_t l = g.whole(1 + (int)(g.next() % 31)), q = g.quick(i), p = g.quick(i + 3);

        if (i % 41 == 0)
            l = 0;
        std::string vars = "l.l=" + dec(l) + " q=" + q16text(q) + " p=" + q16text(p);
        for (const auto &fm : forms) {
            int32_t a = fm.lq ? q : l, b = fm.rq ? (fm.lq ? p : q) : l;
            unsigned f = 0;
            std::string mine = dec(exprarith_mixed(fm.op, a, fm.lq, b, fm.rq, &f));
            std::string m = model(fm.text, vars, EM_DL, NOW);

            if (m != mine && differ++ < 10)
                FAIL_CHECK(std::string(fm.text) + " " + vars + ": the model " + m + ", exprarith " + mine);
        }
    }
    CHECK(differ == 0);
    unsigned f = 0;
    CHECK(exprarith_mixed(ETO_DIV, -100, 0, 0, 1, &f) == -2147483647);
    CHECK(exprarith_mixed(ETO_DIV, -65536, 1, 0, 0, &f) == -32767);
}

TEST_CASE("exprarith: whole numbers as today's code", "[exprarith]")
{
    // Byte, Word and Long operations in their own type (a wanted type of
    // the operands' size; Print shows the handler's result type)
    rng g;
    int differ = 0;
    static const struct {
        unsigned t;
        const char *a, *b;
        enum em_ctx ctx;
    } kinds[] = {
        { ET_BYTE, "b", "c", EM_DB }, { ET_WORD, "w", "v", EM_DW }, { ET_LONG, "l", "m", EM_DL },
        { ET_WORD, "w", "v", EM_PRINT }, { ET_BYTE, "b", "c", EM_PRINT },
    };

    for (int i = 0; i < 1500; i++) {
        for (const auto &k : kinds) {
            int bits = k.t == ET_BYTE ? 8 : k.t == ET_WORD ? 16 : 32;
            int32_t a = g.whole(bits), b = g.whole(1 + (int)(g.next() % bits));
            char ty = k.t == ET_BYTE ? 'b' : k.t == ET_WORD ? 'w' : 'l';

            if (i % 29 == 0)
                b = 0;
            std::string vars = std::string(k.a) + "." + ty + "=" + dec(a) + " " + k.b + "." + ty + "=" + dec(b);
            for (unsigned op : { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV, ETO_MOD }) {
                std::string text = std::string(k.a) + opname(op) + k.b;
                unsigned f = 0;
                int32_t r = exprarith_whole(op, k.t, a, b, &f);
                // Print shows a Byte result (- / Mod of Bytes) by its low byte
                bool byte = k.t == ET_BYTE && op != ETO_ADD && op != ETO_MUL;
                std::string mine = k.ctx != EM_PRINT ? dest(k.ctx, r, f)
                                   : (f & EXA_TRAP) ? "!trap"
                                                    : dec(byte ? (int8_t)r : r);

                for (unsigned rules : { DEC, NOW }) {
                    std::string m = model(text.c_str(), vars, k.ctx, rules);

                    if (m != mine && differ++ < 10)
                        FAIL_CHECK(text + " " + vars + " " + em_ctx_name(k.ctx) + ": the model " + m
                                   + ", exprarith " + mine);
                }
            }
        }
    }
    CHECK(differ == 0);
    unsigned f = 0;
    // add.w then ext.l: the Word sum wraps
    CHECK(exprarith_whole(ETO_ADD, ET_WORD, 20000, 20000, &f) == -25536);
    // a Long division by 0: the largest number with the dividend's sign
    CHECK(exprarith_whole(ETO_DIV, ET_LONG, -5, 0, &f) == -2147483647);
    CHECK(exprarith_whole(ETO_MOD, ET_LONG, -7, 0, &f) == -7);
    CHECK(exprarith_whole(ETO_MOD, ET_LONG, -7, 2, &f) == 1);
    f = 0;
    exprarith_whole(ETO_DIV, ET_WORD, 7, 0, &f);
    CHECK(f == EXA_TRAP);
}

TEST_CASE("exprarith: the casts and the fast-path test", "[exprarith]")
{
    unsigned f = 0;

    CHECK(exprarith_narrow(exprarith_q16_q48((int32_t)0x80000000)));
    CHECK(exprarith_narrow(exprarith_whole_q48(-32768)));
    CHECK(!exprarith_narrow(exprarith_whole_q48(32768)));
    CHECK(!exprarith_narrow(exprarith_whole_q48(100000)));
    CHECK(exprarith_q48_q16(exprarith_whole_q48(5), &f) == 5 << 16);
    CHECK(f == 0);
    exprarith_q48_q16(exprarith_whole_q48(40000), &f);
    CHECK(f == EXA_OVF);
    f = 0;
    CHECK(exprarith_q48_floor(-0x18000, ET_LONG, &f) == -2);
    CHECK(exprarith_q48_trunc(-0x18000, ET_LONG, &f) == -1);
    CHECK(exprarith_q48_floor(exprarith_whole_q48(300), ET_BYTE, &f) == 300);   // the low word
    CHECK(f == EXA_OVF);
    f = 0;
    CHECK(exprarith_q16_floor(-0x8000, ET_WORD, &f) == -1);
    CHECK(exprarith_whole_q16(-3, &f) == (int32_t)0xfffd0000);
    CHECK(f == 0);
    exprarith_whole_q16(32768, &f);
    CHECK(f == EXA_OVF);
}
