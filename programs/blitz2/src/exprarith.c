/*
 * exprarith.c - the arithmetic of the formats of rule P+ (exprarith.h).
 * No asm counterpart: the computations of the routines the compiled code
 * calls, in C for the compiler's constant folding and as the oracle of the
 * emulator tests; where each one is from: libs/acidlibs/ldivlib.c
 * (qdivide, ldivide: the original's restoring loop), lmullib.c and the
 * Quick multiplication of the port's libraries (qmul: the magnitudes'
 * product), modlib (domodword, domodlong, domodquick), libs/compilerlib
 * (mixmult, qqmult, lqdiv; #1178) and design decision 9 of
 * expr-tree-5-operand-typing (48.16).
 */
#include "exprarith.h"

/* ------------------------------------------------- 128-bit magnitudes */

struct u128 {
    uint64_t hi, lo;
};

static struct u128 mul64(uint64_t a, uint64_t b)
{
    uint64_t al = a & 0xffffffffu, ah = a >> 32, bl = b & 0xffffffffu, bh = b >> 32;
    uint64_t ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    uint64_t mid = (ll >> 32) + (lh & 0xffffffffu) + (hl & 0xffffffffu);
    struct u128 r;

    r.lo = (mid << 32) | (ll & 0xffffffffu);
    r.hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
    return r;
}

static struct u128 shr128(struct u128 a, unsigned n)
{
    if (n >= 64) {
        a.lo = a.hi >> (n - 64);
        a.hi = 0;
    } else if (n) {
        a.lo = (a.lo >> n) | (a.hi << (64 - n));
        a.hi >>= n;
    }
    return a;
}

/* a / d (d > 0) by shift and subtract: the quotient's low 128 bits */
static struct u128 div128(struct u128 a, uint64_t d)
{
    struct u128 q = { 0, 0 };
    uint64_t r = 0;
    int i;

    for (i = 127; i >= 0; i--) {
        uint64_t bit = i >= 64 ? (a.hi >> (i - 64)) & 1 : (a.lo >> i) & 1;
        int carry = (r >> 63) != 0;

        r = (r << 1) | bit;
        if (carry || r >= d) {
            r -= d;
            if (i >= 64)
                q.hi |= (uint64_t)1 << (i - 64);
            else
                q.lo |= (uint64_t)1 << i;
        }
    }
    return q;
}

static uint64_t mag(int64_t v)
{
    return v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v;
}

/* the magnitude m with sign neg as an int64_t modulo 2^64; EXA_OVF when
   it does not fit */
static int64_t signed64(struct u128 m, int neg, unsigned *flags)
{
    uint64_t v = m.lo;

    if (m.hi || v > (uint64_t)INT64_MAX + (neg ? 1u : 0u))
        *flags |= EXA_OVF;
    if (neg)
        v = (uint64_t)0 - v;
    return (int64_t)v;
}

/* floor(v / 65536) */
static int64_t floor16(int64_t v)
{
    int64_t q = v / 65536, r = v % 65536;

    return r < 0 ? q - 1 : q;
}

/* the bits of type t (a Byte and a Word as the low word, sign-extended:
   the byte path computes in words) */
static int32_t keep(int64_t w, unsigned t, unsigned *flags)
{
    int64_t lo = t == ET_BYTE ? -128 : t == ET_WORD ? -32768 : INT32_MIN;
    int64_t hi = t == ET_BYTE ? 127 : t == ET_WORD ? 32767 : INT32_MAX;

    if (w < lo || w > hi)
        *flags |= EXA_OVF;
    if (t == ET_BYTE || t == ET_WORD)
        return (int16_t)(uint16_t)(uint64_t)w;
    return (int32_t)(uint32_t)(uint64_t)w;
}

/* ------------------------------------------------- today's routines */

/* ldivlib's loop ('loop): the magnitudes; the divisor shifted right out
   of d1 into d3 from the top, then one quotient bit per step where d3
   fits the remainder; steps 32 (ldivide) or 48 (qdivide, the 16 fraction
   bits with the divisor's low bits lost: the last bit); quotient bits
   above bit 31 lost.  A divisor 0: the largest number with the dividend's
   sign (#200). */
static uint32_t divide(uint32_t d0, uint32_t d1, int steps, unsigned *flags)
{
    uint32_t d3 = 0, d4 = 0;
    int neg = 0, i;

    if (!d1) {
        *flags |= EXA_DIV0;
        return (int32_t)d0 >= 0 ? 0x7fffffffu : 0x80000001u;
    }
    if ((int32_t)d1 < 0) {
        d1 = 0u - d1;
        neg = 1;
    }
    if ((int32_t)d0 < 0) {
        d0 = 0u - d0;
        neg = !neg;
    }
    for (i = 0; i < steps; i++) {
        d4 <<= 1;
        d3 = (d3 >> 1) | ((d1 & 1) << 31);
        d1 >>= 1;
        if (!d1 && d3 && d0 >= d3) {
            d0 -= d3;
            d4 |= 1;
        }
    }
    return neg ? 0u - d4 : d4;
}

/* DIVS.W d1,d0: the quotient in the low word, the remainder in the high
   word; an overflow leaves d0 as it was; a divisor 0 traps (d0 kept) */
static uint32_t divs(uint32_t d0, int16_t d1, unsigned *flags)
{
    int64_t q, r;

    if (!d1) {
        *flags |= EXA_TRAP;
        return d0;
    }
    q = (int64_t)(int32_t)d0 / d1;
    r = (int64_t)(int32_t)d0 % d1;
    if (q > 32767 || q < -32768)
        return d0;
    return ((uint32_t)(uint16_t)r << 16) | (uint16_t)q;
}

/* domodword (domodbyte): EXT.L d0, DIVS d1,d0, SWAP d0, a negative
   remainder plus the divisor (in words) */
static int32_t modword(int32_t a, int16_t b, unsigned *flags)
{
    uint32_t d0 = divs((uint32_t)(int32_t)(int16_t)a, b, flags);

    d0 = (d0 << 16) | (d0 >> 16);
    if ((int16_t)d0 < 0)
        d0 = (d0 & 0xffff0000u) | (uint16_t)((uint16_t)d0 + (uint16_t)b);
    return (int16_t)(uint16_t)d0;
}

int32_t exprarith_whole(unsigned op, unsigned t, int32_t a, int32_t b, unsigned *flags)
{
    uint32_t x = (uint32_t)a, y = (uint32_t)b;

    switch (op) {
    case ETO_ADD:
        if (t == ET_BYTE || t == ET_WORD)
            return (int16_t)(uint16_t)(x + y);  /* add.w (a Word sum then ext.l) */
        return (int32_t)(x + y);
    case ETO_SUB:
        if (t == ET_BYTE || t == ET_WORD)
            return (int16_t)(uint16_t)(x - y);
        return (int32_t)(x - y);
    case ETO_MUL:
        if (t == ET_BYTE)
            return (int16_t)(uint16_t)(uint32_t)((int32_t)(int16_t)x * (int32_t)(int16_t)y);
        if (t == ET_WORD)
            return (int32_t)(int16_t)x * (int32_t)(int16_t)y;   /* muls.w */
        return (int32_t)(uint32_t)((uint64_t)x * y);
    case ETO_DIV:
        if (t == ET_BYTE || t == ET_WORD)
            return (int16_t)(uint16_t)divs((uint32_t)(int32_t)(int16_t)x, (int16_t)y, flags);
        return (int32_t)divide(x, y, 32, flags);
    case ETO_MOD:
        if (t == ET_BYTE || t == ET_WORD)
            return modword(a, (int16_t)y, flags);
        {
            /* domodlong: q = ldivide(a, b), a - q * b, + b if negative */
            unsigned f = 0;
            uint32_t q = divide(x, y, 32, &f), r = x - (uint32_t)((uint64_t)q * y);

            if ((int32_t)r < 0)
                r += y;
            return (int32_t)r;
        }
    default: return a;
    }
}

int32_t exprarith_q16(unsigned op, int32_t a, int32_t b, unsigned *flags)
{
    uint32_t x = (uint32_t)a, y = (uint32_t)b;

    switch (op) {
    case ETO_ADD: return (int32_t)(x + y);
    case ETO_SUB: return (int32_t)(x - y);
    case ETO_MUL: {
        /* qmul: bits 16..47 of the product of the magnitudes, signed */
        int neg = (a < 0) != (b < 0);
        uint32_t m = (uint32_t)(((uint64_t)(a < 0 ? 0u - x : x) * (b < 0 ? 0u - y : y)) >> 16);

        return (int32_t)(neg ? 0u - m : m);
    }
    case ETO_DIV: return (int32_t)divide(x, y, 48, flags);
    case ETO_MOD: {
        int32_t r;

        if (!b) {
            *flags |= EXA_TRAP;
            return a;
        }
        r = b == -1 ? 0 : a % b;
        if (r < 0)
            r = (int32_t)((uint32_t)r + y);
        return r;
    }
    default: return a;
    }
}

int32_t exprarith_qmod_today(int32_t a, int32_t b, unsigned *flags)
{
    /* the two SWAPs of domodquick, then domodword on the whole parts */
    return modword((int16_t)((uint32_t)a >> 16), (int16_t)((uint32_t)b >> 16), flags);
}

/* ------------------------------------------------------------- 48.16 */

int64_t exprarith_q48(unsigned op, int64_t a, int64_t b, int64_t div0, unsigned *flags)
{
    switch (op) {
    case ETO_ADD:
    case ETO_SUB: {
        uint64_t r = op == ETO_ADD ? (uint64_t)a + (uint64_t)b : (uint64_t)a - (uint64_t)b;

        if (op == ETO_ADD ? (b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)
                          : (b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b))
            *flags |= EXA_OVF;
        return (int64_t)r;
    }
    case ETO_MUL: return signed64(shr128(mul64(mag(a), mag(b)), 16), (a < 0) != (b < 0), flags);
    case ETO_DIV: {
        struct u128 n;

        if (!b) {
            *flags |= EXA_DIV0;
            return a < 0 ? -div0 : div0;
        }
        n.hi = mag(a) >> 48;
        n.lo = mag(a) << 16;
        return signed64(div128(n, mag(b)), (a < 0) != (b < 0), flags);
    }
    case ETO_MOD: {
        int64_t r;

        if (!b)
            return a;           /* as domodlong: the dividend */
        r = b == -1 ? 0 : a % b;
        if (r < 0)
            r = (int64_t)((uint64_t)r + (uint64_t)b);
        return r;
    }
    default: return a;
    }
}

int64_t exprarith_q48_down(unsigned op, int64_t a, int64_t b, int64_t div0, unsigned *flags)
{
    int64_t v = exprarith_q48(op, a, b, div0, flags);
    int neg = (a < 0) != (b < 0);

    if (op == ETO_MUL && neg && (mul64(mag(a), mag(b)).lo & 0xffff))
        v = (int64_t)((uint64_t)v - 1);         /* the cut bits not 0: one 1/65536 down */
    else if (op == ETO_DIV && neg && b) {
        /* the remainder of (|a| << 16) / |b| not 0 */
        struct u128 n, q, p;

        n.hi = mag(a) >> 48;
        n.lo = mag(a) << 16;
        q = div128(n, mag(b));
        p = mul64(q.lo, mag(b));
        if (p.lo != n.lo || p.hi + q.hi * mag(b) != n.hi)
            v = (int64_t)((uint64_t)v - 1);
    }
    return v;
}

int64_t exprarith_div0(unsigned want)
{
    switch (want) {
    case ET_QUICK: return 0x7fffffff;
    case ET_WORD: return (int64_t)32767 * 65536;
    case ET_BYTE: return (int64_t)127 * 65536;
    default: return (int64_t)0x7fffffff * 65536;
    }
}

int exprarith_narrow(int64_t v)
{
    return v == (int64_t)(int32_t)(uint32_t)(uint64_t)v;
}

/* ------------------------------------------------------------- casts */

int64_t exprarith_whole_q48(int32_t v)
{
    return (int64_t)v * 65536;
}

int64_t exprarith_q16_q48(int32_t v)
{
    return v;
}

int32_t exprarith_q48_q16(int64_t v, unsigned *flags)
{
    if (!exprarith_narrow(v))
        *flags |= EXA_OVF;
    return (int32_t)(uint32_t)(uint64_t)v;
}

int32_t exprarith_q48_floor(int64_t v, unsigned t, unsigned *flags)
{
    return keep(floor16(v), t, flags);
}

int32_t exprarith_q48_trunc(int64_t v, unsigned t, unsigned *flags)
{
    return keep(v / 65536, t, flags);
}

int32_t exprarith_q16_floor(int32_t v, unsigned t, unsigned *flags)
{
    return keep(floor16(v), t, flags);
}

int32_t exprarith_whole_q16(int32_t v, unsigned *flags)
{
    if (v < -32768 || v > 32767)
        *flags |= EXA_OVF;
    return (int32_t)((uint32_t)v << 16);
}

int32_t exprarith_q48_fit(int64_t v, int *quick, unsigned *flags)
{
    *quick = floor16(v) >= -32768 && floor16(v) <= 32767;
    if (*quick)
        return (int32_t)v;
    return keep(floor16(v), ET_LONG, flags);
}

/* ------------------------------------------------- the #1178 routines */

int32_t exprarith_mixed(unsigned op, int32_t a, int aq, int32_t b, int bq, unsigned *flags)
{
    int64_t x = aq ? a : (int64_t)a * 65536, y = bq ? b : (int64_t)b * 65536, r;

    switch (op) {
    case ETO_ADD: r = floor16(x + y); break;
    case ETO_SUB: r = floor16(x - y); break;
    case ETO_MUL: {
        /* floor(x * y / 2^32): the magnitudes' product, the floor of a
           negative one rounded away from zero */
        struct u128 p = mul64(mag(x), mag(y));
        int neg = (x < 0) != (y < 0);
        uint64_t q = shr128(p, 32).lo;

        if (neg && (p.lo & 0xffffffffu))
            q++;
        r = neg ? -(int64_t)q : (int64_t)q;
        break;
    }
    default:                    /* / */
        if (!y) {
            int64_t big = aq && !bq ? 32767 : 0x7fffffff;

            *flags |= EXA_DIV0;
            r = x < 0 ? -big : big;
        } else
            r = x / y;
        break;
    }
    if (r < INT32_MIN || r > INT32_MAX)
        *flags |= EXA_OVF;
    return (int32_t)(uint32_t)(uint64_t)r;
}
