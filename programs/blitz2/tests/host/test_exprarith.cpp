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
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprarith.h"
#include "exprmatrix.h"
}

namespace {

const uint64_t DEC = EM_RULES_DECIDED, NOW = EM_RULES_CANDBASE;

// the value text the model gives for text under ctx by rules
std::string model(const char *text, const std::string &vars, enum em_ctx ctx, uint64_t rules)
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

            for (uint64_t rules : { DEC, NOW }) {
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

                for (uint64_t rules : { DEC, NOW }) {
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

// ------------------------------------------------- the fast paths (task 4.3)
// The inline code of eval.c's p5fast step by step, on the 68000's word
// operations (DIVU, MULU of words, swap), against exprarith_q48: a 16.16
// value divided by a whole constant 2 <= |d| <= 65535 (P5F_NDIV), times a
// constant below 1 (P5F_NMULF), times a Word (P5F_WMUL).

namespace {

uint32_t mag32(int32_t a)
{
    return a < 0 ? 0u - (uint32_t)a : (uint32_t)a;
}

// P5F_NDIV: two DIVU of |A| by d, the sign, under a wanted Long the cut
// towards zero
int32_t fast_ndiv(int32_t a, int32_t d, bool cut)
{
    uint32_t s = mag32(a), dm = mag32(d), t;
    uint32_t hi = s >> 16, lo = s & 0xffff, q1, q2;

    t = hi;                                     // moveq #0 : move.w
    q1 = (t / dm) & 0xffff;                     // divu.w: t = r1:q1
    t = ((t % dm) << 16) | lo;                  // move.w, swap, move.w: r1:lo
    q2 = (t / dm) & 0xffff;
    s = (q1 << 16) | q2;
    if ((a < 0) != (d < 0))
        s = 0u - s;
    int32_t r = (int32_t)s;
    if (cut && r < 0 && (r & 0xffff)) {
        r = (int32_t)((uint32_t)r + 0x10000);
    }
    if (cut)
        r = (int32_t)((uint32_t)r & 0xffff0000u);
    return r;
}

// P5F_NMULF: (al * |X|) >> 16 + ah * |X|, the sign
int32_t fast_nmulf(int32_t a, int32_t x)
{
    uint32_t s = mag32(a), xm = mag32(x) & 0xffff;
    uint32_t t = ((s & 0xffff) * xm) >> 16;
    uint32_t r = (s >> 16) * xm + t;

    if ((int32_t)((uint32_t)a ^ (uint32_t)x) < 0)
        r = 0u - r;
    return (int32_t)r;
}

// P5F_WMUL: ah * |w| << 16 + al * |w| in 64 bits (the carry by addx), the
// sign of ah eor w applied in 64 bits
int64_t fast_wmul(int32_t a, int16_t w)
{
    uint32_t s = mag32(a);
    uint16_t wm = (uint16_t)(w < 0 ? -(int32_t)w : w);
    uint32_t t = (s >> 16) * wm, lo = (s & 0xffff) * wm;
    uint32_t hi = t >> 16, sh = t << 16;
    uint32_t l2 = lo + sh;

    hi += l2 < lo;
    uint64_t v = ((uint64_t)hi << 32) | l2;
    if ((int16_t)((uint16_t)((uint32_t)a >> 16) ^ (uint16_t)w) < 0)
        v = 0 - v;
    return (int64_t)v;
}

int64_t q48(unsigned op, int64_t a, int64_t b)
{
    unsigned f = 0;

    return exprarith_q48(op, a, b, exprarith_div0(ET_QUICK), &f);
}

// the 48.16 code's cut towards zero of a quotient (part 3)
int64_t q48cut(int64_t v)
{
    if (v < 0 && (v & 0xffff))
        v += 0x10000;
    return v & ~(int64_t)0xffff;
}

uint32_t rnd32(uint64_t &s)
{
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (uint32_t)(s >> 32);
}

// the 16.16 values of the test: every whole part with the fractions 0, 1,
// $7FFF, $8000, $FFFF and four random ones
template <typename F> bool each_quick(uint64_t &seed, F f)
{
    uint32_t frac[9] = { 0, 1, 0x7fff, 0x8000, 0xffff, 0, 0, 0, 0 };

    for (int i = 5; i < 9; i++)
        frac[i] = rnd32(seed) & 0xffff;
    for (uint32_t w = 0; w < 0x10000; w++)
        for (uint32_t fr : frac)
            if (!f((int32_t)((w << 16) | fr)))
                return false;
    return true;
}

} // namespace

TEST_CASE("exprarith: the fast paths of p5fast equal q48div and q48mul", "[exprarith]")
{
    uint64_t seed = 0x4f3ull;
    const int32_t divs[] = { 2, 3, 10, -2, -3, 7, 100, 255, 256, 4096, 32767, 32768, 65535, -65535 };

    for (int32_t d : divs)
        for (int cut = 0; cut < 2; cut++) {
            int32_t bad = 0;
            bool ok = each_quick(seed, [&](int32_t a) {
                int64_t v = q48(ETO_DIV, exprarith_q16_q48(a), exprarith_whole_q48(d));
                if (cut)
                    v = q48cut(v);
                if (v != (int64_t)fast_ndiv(a, d, cut)) {
                    bad = a;
                    return false;
                }
                return true;
            });
            INFO("d ", d, " cut ", cut, " a ", bad);
            CHECK(ok);
        }
    // a constant below 1: its 16.16 value |X| < 65536 (0.5, -0.5, the
    // smallest, the largest, random)
    int32_t xs[24] = { 32768, -32768, 1, -1, 65535, -65535, 16384, 6554, -6553, 0 };
    for (int i = 10; i < 24; i++) {
        int32_t m = (int32_t)(rnd32(seed) % 65535) + 1;
        xs[i] = (rnd32(seed) & 1) ? -m : m;
    }
    for (int32_t x : xs) {
        int32_t bad = 0;
        bool ok = each_quick(seed, [&](int32_t a) {
            if (q48(ETO_MUL, exprarith_q16_q48(a), exprarith_q16_q48(x)) != (int64_t)fast_nmulf(a, x)) {
                bad = a;
                return false;
            }
            return true;
        });
        INFO("x ", x, " a ", bad);
        CHECK(ok);
    }
    // a planted error is seen: the sign of a negative divisor dropped
    CHECK(q48(ETO_DIV, exprarith_q16_q48(-0x30000), exprarith_whole_q48(-2)) != (int64_t)fast_ndiv(-0x30000, 2, false));
}

TEST_CASE("exprarith: the Word fast path of p5fast equals q48mul", "[exprarith]")
{
    uint64_t seed = 0x5a1ull;

    // a Word: every Word for a set of 16.16 values, and random pairs
    int32_t qs[16] = { 0, 1, -1, 0x7fffffff, (int32_t)0x80000000u, 0x8000, -0x8000, 0x18000, -0x18000,
                       0x7fff8000, (int32_t)0x80008000u, 0x10000, -0x10000, 0xffff, -0xffff, 0x12345678 };
    for (int32_t a : qs) {
        int32_t bad = 0;
        bool ok = true;
        for (int32_t w = -32768; w <= 32767 && ok; w++)
            if (q48(ETO_MUL, exprarith_q16_q48(a), exprarith_whole_q48(w)) != fast_wmul(a, (int16_t)w)) {
                bad = w;
                ok = false;
            }
        INFO("a ", a, " w ", bad);
        CHECK(ok);
    }
    bool ok = true;
    for (int i = 0; i < 2000000 && ok; i++) {
        int32_t a = (int32_t)rnd32(seed);
        int16_t w = (int16_t)rnd32(seed);
        ok = q48(ETO_MUL, exprarith_q16_q48(a), exprarith_whole_q48(w)) == fast_wmul(a, w);
    }
    CHECK(ok);
    // a planted error is seen: the carry of the low part dropped
    CHECK(q48(ETO_MUL, exprarith_q16_q48(0x7fffffff), exprarith_whole_q48(32767)) != fast_wmul(0x7fffffff, 32767) - ((int64_t)1 << 32));
}

// ------------------------------------------ a whole divisor at run time (task 4.3)
// eval.c's p5wholediv step by step: a 48.16 dividend's 64-bit magnitude
// divided by the whole part |m| <= 32768 of a 16.16 divisor with four DIVU
// (a word quotient per digit: an overflow fails), the sign of a eor b,
// against exprarith_q48's division.

namespace {

bool fast_wdiv(int64_t n, int32_t mraw, int64_t *out)
{
    uint32_t sign = ((uint32_t)((uint64_t)n >> 32) ^ (uint32_t)mraw) >> 31;
    uint64_t u = n < 0 ? 0 - (uint64_t)n : (uint64_t)n, q = 0;
    int32_t m = mraw >> 16;
    uint32_t mm = (uint32_t)(m < 0 ? -m : m), r = 0;

    for (int k = 3; k >= 0; k--) {
        uint32_t cur = (r << 16) | (uint32_t)((u >> (16 * k)) & 0xffff);
        if (cur / mm > 0xffff)
            return false;                       // DIVU's overflow
        q = (q << 16) | (cur / mm);
        r = cur % mm;
    }
    if (sign)
        q = 0 - q;
    *out = (int64_t)q;
    return true;
}

} // namespace

TEST_CASE("exprarith: a whole divisor at run time equals q48div", "[exprarith]")
{
    uint64_t seed = 0x77aull;
    std::vector<int64_t> ns = { 0, 1, -1, 0x8000, -0x8000, 0x10000, -0x10000, 0x7fffffff, -0x7fffffffLL - 1,
                                0x7fffffffLL << 16, -(0x7fffffffLL << 16), (int64_t)0x00007fffffffffffLL << 16,
                                -((int64_t)0x00007fffffffffffLL << 16), 0x7fffffffffffffffLL, -0x7fffffffffffffffLL,
                                0x123456789abcLL, -0x123456789abcLL, 0xffffffffLL, -0xffffffffLL };
    for (int i = 0; i < 40; i++) {
        uint64_t v = ((uint64_t)rnd32(seed) << 32) | rnd32(seed);
        ns.push_back((int64_t)v >> (rnd32(seed) % 40));
        ns.push_back((int64_t)(int32_t)rnd32(seed));
    }
    bool ok = true;
    int64_t badn = 0;
    int32_t badm = 0;
    for (int32_t m = -32768; m <= 32767 && ok; m++) {
        if (!m)
            continue;
        int32_t mraw = (int32_t)((uint32_t)m << 16);
        for (int64_t n : ns) {
            int64_t got;
            if (!fast_wdiv(n, mraw, &got) || got != q48(ETO_DIV, n, (int64_t)mraw)) {
                ok = false;
                badn = n;
                badm = m;
                break;
            }
        }
    }
    INFO("n ", badn, " m ", badm);
    CHECK(ok);
    for (int i = 0; i < 2000000 && ok; i++) {
        uint64_t v = ((uint64_t)rnd32(seed) << 32) | rnd32(seed);
        int64_t n = (int64_t)v >> (rnd32(seed) % 48 + 1);
        int32_t m = (int32_t)(rnd32(seed) & 0xffff) - 32768;
        int64_t got;
        if (!m)
            continue;
        int32_t mraw = (int32_t)((uint32_t)m << 16);
        ok = fast_wdiv(n, mraw, &got) && got == q48(ETO_DIV, n, (int64_t)mraw);
    }
    CHECK(ok);
    // a planted error is seen: the sign of the divisor dropped
    int64_t got = 0;
    CHECK(fast_wdiv(-0x30000, 2 << 16, &got));
    CHECK(got != q48(ETO_DIV, -0x30000, (int64_t)(-2 * 65536)));
}
