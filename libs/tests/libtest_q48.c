/*
 * libtest_q48 - the 48.16 routines of compilerlib ($D603-$D60E, OpenSpec
 * change expr-tree-5-operand-typing, design decision 9), the routines
 * of its speed review (from $D60F: q16mod, q48mull, q48mulf; q48divw and q48divb cut
 * down, #1221) and the conversions of change type-system-rework (task
 * 4.3: q48sat, f2l, f2q from $D612) against
 * programs/blitz2/src/exprarith.c, compiled here for the m68k: every
 * result bit for bit as exprarith_q48 and the casts give it, and the
 * registers the contract keeps (libs/compilerlib/compilerlib.c, "Registers
 * of the 48.16 routines").  Called by libtest for compilerlib
 * (libtest.c), whose generic two-register loop leaves these routines out.
 *
 * Inputs: all pairs of a list of edge values (whole numbers, 16.16
 * values, wide pairs, the limits of the division's paths and pairs that
 * reach the rare steps of its digit loop), then pseudo random pairs
 * (fixed seed) of mixed magnitudes.  A difference is a failed check with
 * the routine, the inputs, the result and exprarith's value; the first
 * QMAXFAIL are reported.  The conversions of a Float (f2l, f2q): every
 * exponent with a list of mantissas and both signs, 0 and -0, then pseudo
 * random Floats; f2l in the Long range also against mathffp's SPFix (the
 * ROM's), whose value it keeps there for every Float in mathffp's form
 * (floats()).
 */
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "exprarith.h"

struct regs { ULONG d[8]; ULONG a[7]; };
extern UWORD callregs(APTR fn, struct regs *r, ULONG arg, ULONG pusharg);
void bl_test_check(const char *what, int ok);

int libtest_q48(APTR const *code, const UWORD *tok, int n);

#define QMAXFAIL 20
#define QROUNDS 2500

enum { Q_ADD, Q_SUB, Q_MUL, Q_DIV, Q_DIVQ, Q_DIVW, Q_DIVB, Q_MOD, Q_L2Q, Q_Q2Q, Q_FLOOR, Q_TRUNC, Q_Q16MOD, Q_Q48MULL, Q_Q48MULF,
       Q_Q48SAT, Q_F2L, Q_F2Q, Q_N };
static const char *const qname[Q_N] = {
    "q48add", "q48sub", "q48mul", "q48div", "q48divq", "q48divw", "q48divb", "q48mod",
    "l2q48", "q2q48", "q48floor", "q48trunc", "q16mod", "q48mull", "q48mulf",
    "q48sat", "f2l", "f2q"
};

static APTR qcode[Q_N];
static long fails;
static struct Library *ffpbase;             /* mathffp, for SPFix */

/* 64-bit edges: whole numbers and 16.16 values (narrow pairs), wide
   pairs, the limits of q48div's paths (a divisor below $10000, below
   2^32, wider; a low word 0 or not; 2 to 5 digits of the dividend) */
static const int64_t edges[] = {
    0, 1, 2, 3, 0x7fff, 0x8000, 0xffff, 0x10000, 0x10001, 0x18000, 0x28000, 0x30000,
    0x34000, 0xc8000, 0x7fff0000, 0x7fffffff, 0x80000000, 0xffffffff, 0xfffe0001,
    0x100000000, 0x186a08000, 0x186a00000, 0x7fffffff0000, 0x800000000000,
    0x1234567890abcdef, 0x0000ffff00010000, 0xffff00010000, 0x7fffffffffffffff,
    INT64_MIN, INT64_MIN + 1, -1, -0x10000, -0x18000, -0x80000000, -0x7fffffff,
    -0x186a08000, -0x7fffffff0000,
    /* pairs that reach the digit loop's rare steps (a quotient digit
       estimated $FFFF, with and without the carry of its remainder; two
       corrections) */
    0x60823baddd5b7986, (int64_t)0xffffffffd65a4432, (int64_t)0xe3a85a03dc354b4e, 0x8344bcb60000,
    (int64_t)0xb317ad2aaf7bc4c2, 0x3d0ea4af0000, 0x5b1b5df270cc056f, (int64_t)0xffffffff8e5682e5,
    0x41d5aba610180ca7, (int64_t)0xffffffffbbfaf843, (int64_t)0xffffffd1b34d0997,
    (int64_t)0xffffffffc5e60611, 0x7bb81b15240000, (int64_t)0xffffffffbdbd8881,
    0x7474f459ccf358f1, (int64_t)0xffffffffee470338,
};
#define NEDGES (sizeof edges / sizeof edges[0])

static ULONG seed;
static ULONG rnd32(void)
{
    seed = seed * 1103515245UL + 12345;
    ULONG hi = seed >> 16;
    seed = seed * 1103515245UL + 12345;
    return hi << 16 | (seed >> 16);
}

/* mixed magnitudes: any 64 bits, shifted down, a whole number, a narrow
   pair */
static int64_t rnd64(void)
{
    uint64_t v = (uint64_t)rnd32() << 32 | rnd32();
    ULONG k = rnd32();

    switch (k & 3) {
    case 0: return (int64_t)v;
    case 1: return (int64_t)v >> ((k >> 2) & 63);
    case 2: return ((int64_t)v >> ((k >> 2) & 63)) & ~(int64_t)0xffff;
    default: return (int32_t)(uint32_t)v;
    }
}

static struct regs mark(void)
{
    struct regs r = {{0}};
    int k;

    for (k = 0; k < 8; k++)
        r.d[k] = 0x5a5a0000UL | k;
    for (k = 0; k < 7; k++)
        if (k < 4 || k == 6)                /* as libtest: not a4, a5 */
            r.a[k] = 0xa5a50000UL | k;
    return r;
}

static void fail(int q, int64_t a, int64_t b, uint64_t got, uint64_t want, const char *what)
{
    char m[160];

    if (++fails > QMAXFAIL)
        return;
    sprintf(m, "%s %s: a $%08lX%08lX b $%08lX%08lX: $%08lX%08lX, exprarith $%08lX%08lX", qname[q], what,
            (ULONG)((uint64_t)a >> 32), (ULONG)a, (ULONG)((uint64_t)b >> 32), (ULONG)b,
            (ULONG)(got >> 32), (ULONG)got, (ULONG)(want >> 32), (ULONG)want);
    printf("FAIL %s\n", m);
    bl_test_check(m, 0);
}

/* the registers a call must keep: every one but d0 and d1, and d2 and d3
   for the routines with two operands */
static void kept(int q, int64_t a, int64_t b, const struct regs *in, const struct regs *out)
{
    int k, first = q <= Q_MOD || q == Q_Q48MULL || q == Q_Q48MULF ? 4 : 2;

    for (k = first; k < 8; k++)
        if (in->d[k] != out->d[k])
            fail(q, a, b, out->d[k], in->d[k], "a kept data register changed");
    for (k = 0; k < 7; k++)
        if (k != 4 && k != 5 && in->a[k] != out->a[k])
            fail(q, a, b, out->a[k], in->a[k], "a kept address register changed");
}

static uint64_t call2(int q, int64_t a, int64_t b)
{
    struct regs r = mark(), in;

    r.d[0] = (ULONG)a;
    r.d[1] = (ULONG)((uint64_t)a >> 32);
    r.d[2] = (ULONG)b;
    r.d[3] = (ULONG)((uint64_t)b >> 32);
    in = r;
    callregs(qcode[q], &r, 0, 0);
    kept(q, a, b, &in, &r);
    return (uint64_t)r.d[1] << 32 | r.d[0];
}

static const unsigned ops[] = { ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV, ETO_DIV, ETO_DIV, ETO_DIV, ETO_MOD };

static void pair(int64_t a, int64_t b)
{
    int q;

    for (q = Q_ADD; q <= Q_MOD; q++) {
        unsigned f = 0;

        if (!b && q >= Q_DIV)
            continue;           /* a / or Mod by 0: the division trap (type-system-rework D1/D7) */
        /* q48divw and q48divb cut down (#1221) */
        uint64_t want = (uint64_t)(q == Q_DIVW || q == Q_DIVB ? exprarith_q48_down : exprarith_q48)(ops[q], a, b, &f);
        uint64_t got = call2(q, a, b);

        if (got != want)
            fail(q, a, b, got, want, "value");
    }
    {
        unsigned f = 0;
        uint64_t want = (uint64_t)exprarith_q48_down(ETO_MUL, a, b, &f);
        uint64_t got = call2(Q_Q48MULF, a, b);

        if (got != want)
            fail(Q_Q48MULF, a, b, got, want, "value");
    }
}

/* a cast: v the value, d1 for the ones from a Long or Quick the mark */
static void casts(int64_t v)
{
    static const int cq[] = { Q_L2Q, Q_Q2Q, Q_FLOOR, Q_TRUNC, Q_Q48SAT };
    int i;

    for (i = 0; i < 5; i++) {
        int q = cq[i];
        struct regs r = mark(), in;
        unsigned f = 0;
        uint64_t got, want;

        r.d[0] = (ULONG)v;
        if (q == Q_FLOOR || q == Q_TRUNC || q == Q_Q48SAT)
            r.d[1] = (ULONG)((uint64_t)v >> 32);
        in = r;
        callregs(qcode[q], &r, 0, 0);
        kept(q, v, 0, &in, &r);
        switch (q) {
        case Q_L2Q:
            want = (uint64_t)exprarith_whole_q48((int32_t)(ULONG)v);
            got = (uint64_t)r.d[1] << 32 | r.d[0];
            break;
        case Q_Q2Q:
            want = (uint64_t)exprarith_q16_q48((int32_t)(ULONG)v);
            got = (uint64_t)r.d[1] << 32 | r.d[0];
            break;
        case Q_Q48SAT:
            if (r.d[1] != in.d[1])
                fail(q, v, 0, r.d[1], in.d[1], "d1 changed");
            got = (ULONG)r.d[0];
            want = (ULONG)exprarith_q48_quick(v, &f);
            break;
        default:
            if (r.d[1] != in.d[1])
                fail(q, v, 0, r.d[1], in.d[1], "d1 changed");
            got = (ULONG)r.d[0];
            want = (ULONG)(q == Q_FLOOR ? exprarith_q48_floor(v, ET_LONG, &f)
                                        : exprarith_q48_trunc(v, ET_LONG, &f));
            break;
        }
        if (got != want)
            fail(q, v, 0, got, want, "value");
        if (q == Q_FLOOR || q == Q_TRUNC) {
            /* a Word or a Byte: the low word of the Long, ext.l */
            int32_t w = (int16_t)(uint16_t)r.d[0];
            int32_t ww = q == Q_FLOOR ? exprarith_q48_floor(v, ET_WORD, &f) : exprarith_q48_trunc(v, ET_WORD, &f);
            int32_t wb = q == Q_FLOOR ? exprarith_q48_floor(v, ET_BYTE, &f) : exprarith_q48_trunc(v, ET_BYTE, &f);

            if (w != ww || w != wb)
                fail(q, v, 0, (uint32_t)w, (uint32_t)ww, "Word/Byte as ext.l of the Long");
        }
    }
}

/* the routines of the speed review with two 16.16 operands in d0 and d1
   (the contract of an operator: d0 the result, d1 changed), q48mull with
   a Long in d0 and a Quick in d2 (the 48.16 routines' contract): the low
   32 bits of a and b; q16mod not with b 0 (its caller's trap) */
static void two(int64_t a, int64_t b)
{
    struct regs r = mark(), in;
    int32_t x = (int32_t)(uint32_t)(uint64_t)a, y = (int32_t)(uint32_t)(uint64_t)b;
    unsigned f = 0;
    uint64_t got, want;

    r.d[0] = (ULONG)x;
    r.d[2] = (ULONG)y;
    in = r;
    callregs(qcode[Q_Q48MULL], &r, 0, 0);
    kept(Q_Q48MULL, x, y, &in, &r);
    got = (uint64_t)r.d[1] << 32 | r.d[0];
    want = (uint64_t)exprarith_q48(ETO_MUL, exprarith_whole_q48(x), exprarith_q16_q48(y), &f);
    if (got != want)
        fail(Q_Q48MULL, x, y, got, want, "value");
    r = mark();
    if (!y)
        return;
    r.d[0] = (ULONG)x;
    r.d[1] = (ULONG)y;
    in = r;
    callregs(qcode[Q_Q16MOD], &r, 0, 0);
    kept(Q_Q16MOD, x, y, &in, &r);
    got = (ULONG)r.d[0];
    want = (ULONG)exprarith_q16(ETO_MOD, x, y, &f);
    if (got != want)
        fail(Q_Q16MOD, x, y, got, want, "value");
}

/* a Float conversion: f2l (against exprarith and, in the Long range,
   SPFix; a Word and a Byte as the low word, ext.l) and f2q */
static void floats(ULONG v)
{
    struct regs r = mark(), in;
    unsigned f = 0;
    ULONG got, want;

    r.d[0] = v;
    in = r;
    callregs(qcode[Q_F2L], &r, 0, 0);
    kept(Q_F2L, v, 0, &in, &r);
    got = r.d[0];
    want = (ULONG)exprarith_ffp_whole(v, ET_LONG, &f);
    if (got != want)
        fail(Q_F2L, v, 0, got, want, "value");
    /* SPFix in the Long range, for a Float in mathffp's form (bit 31 of
       the mantissa and the exponent byte not 0, or the value 0): the ROM
       takes a low byte 0 as the value 0 and returns d0 as it is (a
       mantissa with exponent 0, below 2^-64: f2l gives 0), and saturates
       a mantissa 0 with a large exponent */
    if (!(f & EXA_OVF) && ffpbase && (!v || ((v & 0x80000000UL) && (v & 0x7f)))) {
        struct regs s = mark();

        s.d[0] = v;
        s.a[6] = (ULONG)ffpbase;
        callregs((APTR)((UBYTE *)ffpbase - 30), &s, 0, 0);
        if (s.d[0] != got)
            fail(Q_F2L, v, 0, got, s.d[0], "value in the Long range, not SPFix's (shown as exprarith)");
    }
    {
        int32_t w = (int16_t)(uint16_t)got;
        int32_t ww = exprarith_ffp_whole(v, ET_WORD, &f), wb = exprarith_ffp_whole(v, ET_BYTE, &f);

        if (w != ww || w != wb)
            fail(Q_F2L, v, 0, (uint32_t)w, (uint32_t)ww, "Word/Byte as ext.l of the Long");
    }
    r = mark();
    r.d[0] = v;
    in = r;
    callregs(qcode[Q_F2Q], &r, 0, 0);
    kept(Q_F2Q, v, 0, &in, &r);
    got = r.d[0];
    want = (ULONG)exprarith_ffp_quick(v, &f);
    if (got != want)
        fail(Q_F2Q, v, 0, got, want, "value");
}

/* mantissas (bits 31-8, bit 31 set) for every exponent */
static const ULONG mants[] = {
    0x80000000, 0x80000100, 0x80008000, 0xc0000000, 0xaaaaaa00, 0xffffff00, 0xfffffe00, 0x80ffff00,
    0xe6666600, 0x95029000, 0xffff0000, 0x80010000,
};
#define NMANTS (sizeof mants / sizeof mants[0])

/* Floats: 1e10, -14.4 (-144/10), 32767.75, 2^31 - 128 (the largest
   below 2^31), 2^31, -2^31, 0.5, 1/65536; a mantissa 0 with exponents
   and signs */
static const ULONG fedges[] = {
    0x9502f962, 0xe66666c4, 0xffff804f, 0xffffff5f, 0x80000060, 0x800000e0, 0x80000040, 0x80000031,
    0, 0x80, 0x7f, 0xff, 0x40, 0xc0,
};
#define NFEDGES (sizeof fedges / sizeof fedges[0])

int libtest_q48(APTR const *code, const UWORD *tok, int n)
{
    long tests = 0;
    unsigned x, y;
    int i, q;

    for (q = 0; q < Q_N; q++)
        qcode[q] = NULL;
    for (i = 0; i < n; i++)
        if (tok[i] >= 0xd603 && tok[i] < 0xd603 + Q_N)
            qcode[tok[i] - 0xd603] = code[i];
    for (q = 0; q < Q_N; q++)
        if (!qcode[q]) {
            char m[60];

            sprintf(m, "compilerlib: no routine %s ($%04X)", qname[q], 0xd603 + q);
            printf("FAIL %s\n", m);
            bl_test_check(m, 0);
            return 1;
        }
    fails = 0;
    seed = 0x48164816;
    for (x = 0; x < NEDGES; x++) {
        for (y = 0; y < NEDGES; y++, tests++) {
            pair(edges[x], edges[y]);
            two(edges[x], edges[y]);
        }
        casts(edges[x]);
    }
    for (x = 0; x < QROUNDS; x++, tests++) {
        int64_t a = rnd64(), b = rnd64();

        pair(a, b);
        two(a, b);
        casts(a);
    }
    ffpbase = OpenLibrary((STRPTR)"mathffp.library", 0);
    if (!ffpbase) {
        printf("FAIL no mathffp.library\n");
        bl_test_check("compilerlib: mathffp.library for SPFix", 0);
        fails++;
    }
    for (x = 0; x < 128; x++)
        for (y = 0; y < NMANTS; y++, tests += 2) {
            floats(mants[y] | x);
            floats(mants[y] | x | 0x80);
        }
    for (x = 0; x < NFEDGES; x++, tests++)
        floats(fedges[x]);
    for (x = 0; x < QROUNDS * 4; x++, tests++)
        floats(((rnd32() | 0x80000000UL) & 0xffffff00UL) | (rnd32() & 0xff));
    if (ffpbase)
        CloseLibrary(ffpbase);
    printf("%-12s %ld pairs, %ld differences\n", "48.16", tests, fails);
    if (!fails)
        bl_test_check("compilerlib 48.16 routines as exprarith", 1);
    return fails != 0;
}
