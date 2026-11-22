/*
 * model.c - the typing and value model of the expression matrix (OpenSpec
 * change expression-type-matrix, design decision 7): the value a cell
 * prints, by the original's rules (blitz2 1.95 with the CD's libraries)
 * or by the rules with the fixes (exprmatrix.h, em_model).
 *
 * The model follows the code blitz2 emits for the cell (eval5, varcont,
 * consttype2, the operator handlers of eval.c and libcall.c, with their
 * blitz2.s lines there): one 32-bit register per operand, the type so
 * far, the conversions of convtable and the m68k semantics of each
 * instruction (add.w of a Byte, DIVS overflow, Scc/ext.w); the library
 * routines the code calls are modelled from their sources
 * (bblibsrc/lmullib, ldivlib, modlib, floatquicklib, printlib on the
 * Aminet source disk; libs/acidlibs for the port's C libraries), and
 * mathffp (the ROM library the Float code calls) by an emulation of
 * Motorola FFP (rounded half up after truncating to 32 bits).  SPPow
 * (mathtrans, the ^ operator) is not modelled: its result is only known
 * where a whole number is taken from it far from a step (EM_F_POW
 * otherwise).
 *
 * Beside the register value every operand carries its ideal value (exact
 * rational arithmetic, rounded only where the type demands it: a whole
 * number type rounds a division towards zero and everything else down, a
 * Float is truncated to a whole number), and the model raises a flag
 * (EM_F_*) where the computed value is not the ideal one or where a
 * question class of design decision 7 applies.  Each rule of the fixed
 * set names its entry in task-decisions.md (heading, date).
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exprmatrix.h"

enum { TB = 1, TW = 2, TL = 3, TQ = 4, TF = 5, TS = 7 };

/* ------------------------------------------------------------------ rationals */

/* an exact value n/d (d > 0) with a double beside it; ok = 0: not exact
   (the double only) */
struct rat {
    __int128 n, d;
    double x;
    int ok;
};

static __int128 gcd128(__int128 a, __int128 b)
{
    if (a < 0)
        a = -a;
    while (b) {
        __int128 t = a % b;
        a = b;
        b = t < 0 ? -t : t;
    }
    return a ? a : 1;
}

static const __int128 RAT_LIMIT = (__int128)1 << 100;

static struct rat rat_make(__int128 n, __int128 d)
{
    struct rat r = { 0, 1, 0, 1 };
    __int128 g;

    if (d < 0) {
        n = -n;
        d = -d;
    }
    g = gcd128(n, d);
    r.n = n / g;
    r.d = d / g;
    r.x = (double)r.n / (double)r.d;
    if (r.n > RAT_LIMIT || r.n < -RAT_LIMIT || r.d > RAT_LIMIT)
        r.ok = 0;
    return r;
}

static struct rat rat_int(int64_t v)
{
    return rat_make(v, 1);
}

static struct rat rat_dbl(double x)
{
    struct rat r = { 0, 1, x, 0 };

    return r;
}

static struct rat rat_op(struct rat a, struct rat b, char op)
{
    if (!a.ok || !b.ok) {
        switch (op) {
        case '+': return rat_dbl(a.x + b.x);
        case '-': return rat_dbl(a.x - b.x);
        case '*': return rat_dbl(a.x * b.x);
        default: return rat_dbl(a.x / b.x);
        }
    }
    switch (op) {
    case '+': return rat_make(a.n * b.d + b.n * a.d, a.d * b.d);
    case '-': return rat_make(a.n * b.d - b.n * a.d, a.d * b.d);
    case '*': return rat_make(a.n * b.n, a.d * b.d);
    default: return rat_make(a.n * b.d, a.d * b.n);
    }
}

static int rat_isint(struct rat a)
{
    return a.ok ? a.d == 1 : a.x == floor(a.x);
}

static int rat_sign(struct rat a)
{
    if (a.ok)
        return a.n > 0 ? 1 : a.n < 0 ? -1 : 0;
    return a.x > 0 ? 1 : a.x < 0 ? -1 : 0;
}

static int rat_cmp(struct rat a, struct rat b)
{
    return rat_sign(rat_op(a, b, '-'));
}

/* rounded down, towards zero */
static struct rat rat_floor(struct rat a)
{
    __int128 q;

    if (!a.ok)
        return rat_dbl(floor(a.x));
    q = a.n / a.d;
    if (a.n % a.d && a.n < 0)
        q--;
    return rat_make(q, 1);
}

static struct rat rat_trunc(struct rat a)
{
    if (!a.ok)
        return rat_dbl(trunc(a.x));
    return rat_make(a.n / a.d, 1);
}

/* the whole number (exact or the double rounded) */
static int64_t rat_whole(struct rat a)
{
    if (a.ok)
        return (int64_t)(a.n / a.d);
    return (int64_t)a.x;
}

/* the multiple of 1/65536 towards zero */
static struct rat rat_q16(struct rat a)
{
    return rat_op(rat_trunc(rat_op(a, rat_int(65536), '*')), rat_int(65536), '/');
}

/* a decimal literal or value text */
static struct rat rat_text(const char *s)
{
    __int128 n = 0, d = 1;
    int neg = 0, frac = 0;

    if (*s == '-') {
        neg = 1;
        s++;
    }
    for (; *s; s++) {
        if (*s == '.') {
            frac = 1;
            continue;
        }
        n = n * 10 + (*s - '0');
        if (frac)
            d *= 10;
    }
    return rat_make(neg ? -n : n, d);
}

/* ------------------------------------------------------------- Motorola FFP */

/* FFP: mantissa in bits 31-8 (bit 31 set unless 0), sign bit 7, exponent
   bits 6-0 excess 64; the value is mantissa/2^24 * 2^(exp-64).  The
   operations truncate the exact result to 32 bits and round it to 24
   bits, half up (checked against the measured raw Float values of the
   matrix: no difference). */
static int ffp_zero(uint32_t f)
{
    return !(f & 0xffffff00u);
}

/* m * 2^e (m > 0, any width up to 64 bits), negative: the sign */
static uint32_t ffp_pack(uint64_t m, int e, int neg, int sticky)
{
    int exp;
    uint64_t m32;

    if (!m)
        return 0;
    while (!(m & 0x8000000000000000ull)) {
        m <<= 1;
        e--;
    }
    /* m is now 1.xxx * 2^63: value = m * 2^e = (m/2^64) * 2^(e+64) */
    /* truncated to 32 bits, then $80 added below the 24-bit mantissa */
    m32 = m >> 32;
    (void)sticky;                           /* the bits below the 32 do not count */
    if ((m32 & 0xff) >= 0x80) {
        m32 = (m32 & 0xffffff00u) + 0x100;
        if (m32 > 0xffffffffull) {
            m32 = 0x80000000u;
            e++;
        }
    } else
        m32 &= 0xffffff00u;
    exp = e + 64 + 64;                      /* e+64 is the binary exponent, 64 the bias */
    if (exp > 127)
        return 0xffffff7fu | (neg ? 0x80 : 0);
    if (exp < 0)
        return 0;
    return (uint32_t)m32 | (neg ? 0x80u : 0) | (uint32_t)exp;
}

/* f = m * 2^e with m the 24-bit mantissa */
static void ffp_unpack(uint32_t f, uint64_t *m, int *e, int *neg)
{
    *m = f >> 8;
    *e = (int)(f & 0x7f) - 64 - 24;
    *neg = (f & 0x80) != 0;
}

static uint32_t ffp_flt(int32_t v)
{
    if (!v)
        return 0;
    return ffp_pack(v < 0 ? (uint64_t)(-(int64_t)v) : (uint64_t)v, 0, v < 0, 0);
}

/* towards zero; out of range: $7FFFFFFF / $80000000 (*ovf set) */
static int32_t ffp_fix(uint32_t f, int *ovf)
{
    uint64_t m;
    int e, neg;

    if (ovf)
        *ovf = 0;
    if (ffp_zero(f))
        return 0;
    ffp_unpack(f, &m, &e, &neg);
    if (e >= 0) {
        if (e > 8 || (m << e) > 0x7fffffffull + (neg ? 1 : 0)) {
            if (ovf)
                *ovf = 1;
            return neg ? INT32_MIN : INT32_MAX;
        }
        m <<= e;
    } else
        m = e <= -40 ? 0 : m >> -e;
    return neg ? (int32_t)-(int64_t)m : (int32_t)m;
}

static uint32_t ffp_neg(uint32_t f)
{
    return ffp_zero(f) ? 0 : f ^ 0x80;
}

/* a + b (sub: a - b) */
static uint32_t ffp_add(uint32_t a, uint32_t b, int sub)
{
    uint64_t ma, mb;
    int ea, eb, na, nb, sticky = 0;
    __int128 sa, sb, s;
    int d;

    if (sub)
        b = ffp_neg(b);
    if (ffp_zero(a))
        return ffp_zero(b) ? 0 : b;
    if (ffp_zero(b))
        return a;
    ffp_unpack(a, &ma, &ea, &na);
    ffp_unpack(b, &mb, &eb, &nb);
    /* align at 2^(min) with 40 guard bits */
    if (ea < eb) {
        uint64_t tm = ma; int te = ea, tn = na;
        ma = mb; ea = eb; na = nb;
        mb = tm; eb = te; nb = tn;
    }
    d = ea - eb;
    sa = (__int128)ma << 40;
    if (d >= 64) {
        sb = 0;
        sticky = 1;
    } else {
        sb = ((__int128)mb << 40) >> d;
        if (((((__int128)mb << 40) >> d) << d) != ((__int128)mb << 40))
            sticky = 1;
    }
    s = (na ? -sa : sa) + (nb ? -sb : sb);
    if (!s)
        return 0;
    {
        int neg = s < 0;
        unsigned __int128 u = (unsigned __int128)(neg ? -s : s);
        int e = ea - 40;

        while (u >> 64) {
            if (u & 1)
                sticky = 1;
            u >>= 1;
            e++;
        }
        return ffp_pack((uint64_t)u, e, neg, sticky);
    }
}

static uint32_t ffp_mul(uint32_t a, uint32_t b)
{
    uint64_t ma, mb;
    int ea, eb, na, nb;

    if (ffp_zero(a) || ffp_zero(b))
        return 0;
    ffp_unpack(a, &ma, &ea, &na);
    ffp_unpack(b, &mb, &eb, &nb);
    return ffp_pack(ma * mb, ea + eb, na != nb, 0);
}

static uint32_t ffp_div(uint32_t a, uint32_t b)
{
    uint64_t ma, mb, q;
    int ea, eb, na, nb;
    unsigned __int128 num;

    if (ffp_zero(a) || ffp_zero(b))
        return 0;
    ffp_unpack(a, &ma, &ea, &na);
    ffp_unpack(b, &mb, &eb, &nb);
    num = (unsigned __int128)ma << 40;
    q = (uint64_t)(num / mb);
    return ffp_pack(q, ea - 40 - eb, na != nb, (num % mb) != 0);
}

static struct rat ffp_rat(uint32_t f)
{
    uint64_t m;
    int e, neg;
    __int128 n, d = 1;

    if (ffp_zero(f))
        return rat_int(0);
    ffp_unpack(f, &m, &e, &neg);
    n = (__int128)m;
    if (e >= 0)
        n <<= e;
    else
        d <<= -e;
    return rat_make(neg ? -n : n, d);
}

static int ffp_cmp(uint32_t a, uint32_t b)
{
    return rat_cmp(ffp_rat(a), ffp_rat(b));
}

/* the FFP value of an exact rational (rounded) */
static uint32_t ffp_of(struct rat a)
{
    uint32_t n, d;

    if (!a.ok || a.n > INT32_MAX || a.n < INT32_MIN || a.d > INT32_MAX)
        return 0;
    n = ffp_flt((int32_t)a.n);
    if (a.d == 1)
        return n;
    d = ffp_flt((int32_t)a.d);
    return ffp_div(n, d);
}

/* ---------------------------------------------------------- library routines */

/* bblibsrc/lmullib lmultiply (CD): magnitudes, the cross products added
   SWAPped (#199); C library: the product modulo 2^32 (Fixed #199) */
static uint32_t lib_lmul(uint32_t a, uint32_t b, int cd)
{
    int neg = 0;
    uint32_t d0, d3, d4;

    if (!cd)
        return (uint32_t)((uint64_t)a * b);
    if ((int32_t)a < 0) {
        a = -a;
        neg = 1;
    }
    if ((int32_t)b < 0) {
        b = -b;
        neg = !neg;
    }
    d0 = (a & 0xffff) * (b & 0xffff);
    d3 = (a >> 16) * (b & 0xffff);
    d3 = (d3 << 16) | (d3 >> 16);
    d0 += d3;
    d4 = (a & 0xffff) * (b >> 16);
    d4 = (d4 << 16) | (d4 >> 16);
    d0 += d4;
    return neg ? -d0 : d0;
}

/* qmultiply: CD the high product added SWAPped (#199); C: bits 16..47 of
   the product of the magnitudes (both truncate the low product) */
static uint32_t lib_qmul(uint32_t a, uint32_t b, int cd)
{
    int neg = 0;
    uint32_t d0, d3, d4;

    if ((int32_t)a < 0) {
        a = -a;
        neg = 1;
    }
    if ((int32_t)b < 0) {
        b = -b;
        neg = !neg;
    }
    if (!cd) {
        uint64_t p = (uint64_t)a * b;

        d0 = (uint32_t)(p >> 16);
        return neg ? -d0 : d0;
    }
    d0 = ((a & 0xffff) * (b & 0xffff)) >> 16;
    d3 = (a >> 16) * (b & 0xffff);
    d0 += d3;
    d4 = (a & 0xffff) * (b >> 16);
    d0 += d4;
    d3 = (a >> 16) * (b >> 16);
    d3 = (d3 << 16) | (d3 >> 16);
    d0 += d3;
    return neg ? -d0 : d0;
}

/* bblibsrc/ldivlib divide (ldivide: 32 rounds, qdivide: 48), as the
   instructions run */
static uint32_t lib_divide(uint32_t d0, uint32_t d1, int count)
{
    uint32_t d3 = 0, d4 = 0;
    uint16_t d2 = 0;
    uint32_t xbit;
    int d5 = count;

    if (!d1)
        return 0x7fffffff;
    if ((int32_t)d1 < 0) {
        d1 = -d1;
        d2 = 0xffff;
    }
    if ((int32_t)d0 < 0) {
        d0 = -d0;
        d2 = (uint16_t)~d2;
    }
    for (;;) {
        d4 <<= 1;                           /* LSL.l #1,d4 */
        xbit = d1 & 1;                      /* LSR.l #1,d1: X = C = bit 0 */
        d1 >>= 1;
        d3 = (d3 >> 1) | (xbit << 31);      /* ROXR.l #1,d3 */
        if (d1 == 0) {                      /* BEQ 'skip6 (Z of the LSR) */
            if (d3 && d0 >= d3) {           /* 'skip6: BEQ 'skip3 / CMP.l d3,d0:BCS */
                d0 -= d3;
                d4 = (d4 & 0xffff0000u) | (uint16_t)(d4 + 1);   /* ADDQ #1,d4 (.w) */
            }
        }
        if (--d5 < 0)                       /* DBF d5 */
            break;
    }
    if (d2)
        d4 = -d4;
    return d4;
}

/* DIVS.w d1,d0: quotient in the low word, remainder in the high word;
   an overflow leaves d0 unchanged */
static uint32_t m68k_divs(uint32_t d0, uint32_t d1)
{
    int32_t a = (int32_t)d0;
    int16_t b = (int16_t)d1;
    int64_t q, r;

    if (!b)
        return d0;
    q = (int64_t)a / b;
    r = (int64_t)a % b;
    if (q > 32767 || q < -32768)
        return d0;
    return ((uint32_t)(uint16_t)r << 16) | (uint16_t)q;
}

/* modlib domodword (and domodbyte; domodquick after the two SWAPs):
   EXT.l d0:DIVS d1,d0:SWAP d0:TST d0:BPL:ADD d1,d0 */
static uint32_t lib_modw(uint32_t d0, uint32_t d1)
{
    d0 = (uint32_t)(int32_t)(int16_t)d0;
    d0 = m68k_divs(d0, d1);
    d0 = (d0 << 16) | (d0 >> 16);
    if ((int16_t)d0 < 0)
        d0 = (d0 & 0xffff0000u) | (uint16_t)((uint16_t)d0 + (uint16_t)d1);
    return d0;
}

/* domodlong: q = ldivide(a,b), a - lmultiply(q,b), + b if negative */
static uint32_t lib_modl(uint32_t a, uint32_t b, int cd)
{
    uint32_t q = lib_divide(a, b, 31);
    uint32_t r = a - lib_lmul(q, b, cd);

    if ((int32_t)r < 0)
        r += b;
    return r;
}

/* floatquicklib quicktofloat (both libraries: the C library's is the
   same, libs/acidlibs/floatquicklib.c) */
static uint32_t lib_qtof(uint32_t q)
{
    uint32_t t = -q, x, fr, in;
    int neg;

    if ((int32_t)t < 0) {
        x = q;
        neg = 0;
    } else {
        x = t;
        neg = 1;
    }
    fr = ffp_div(ffp_flt((int32_t)(x & 0xffff)), 0x80000051u);
    in = ffp_flt((int32_t)(x >> 16));
    x = ffp_add(in, fr, 0);
    if (neg)
        x |= 0x80;                          /* BSET #7: also a "-0" ($80) */
    return x;
}

/* floattoquick: the integer part (SPFix) in the high word, the fraction
   *65536 truncated in the low word; a negative value negated */
static uint32_t lib_ftoq(uint32_t f)
{
    int neg = (f & 0x80) != 0;
    uint32_t a = f & ~0x80u, d2;
    int32_t in;
    uint32_t fr;

    in = ffp_fix(a, NULL);
    d2 = (uint32_t)(uint16_t)in << 16;
    fr = ffp_add(a, ffp_flt(in), 1);
    fr = ffp_mul(fr, 0x80000051u);
    d2 |= (uint16_t)ffp_fix(fr, NULL);
    return neg ? -d2 : d2;
}

/* ------------------------------------------------------------- print formats */

static void print_long(char *out, int32_t v)
{
    snprintf(out, EM_TEXT_MAX, "%ld", (long)v);
}

/* printlib pquick2 / 'quick3; cd: the CD's library (#233, #234 not fixed) */
static void print_quick(char *out, uint32_t value, int cd)
{
    char fr[8] = "", ip[8];
    int neg = (int32_t)value < 0;
    uint32_t v = neg ? -value : value;
    uint32_t frac = v & 0xffff;
    uint32_t in;

    if (frac) {
        frac *= 10000;
        frac = (frac & 0xffff) >= 0x8000 ? (frac >> 16) + 1 : frac >> 16;
        if (!cd && frac >= 10000) {
            frac = 0;
            v += 0x10000;
        }
        snprintf(fr, sizeof fr, ".%04u", (unsigned)(frac % 10000));
        {
            size_t n = strlen(fr);

            if (cd) {
                /* 'f0l stops only at a digit: ".0000" leaves "." */
                while (n > 1 && fr[n - 1] == '0')
                    n--;
            } else {
                while (fr[n - 1] == '0')
                    n--;
                if (fr[n - 1] == '.')
                    n--;
            }
            fr[n] = 0;
        }
    }
    in = (v >> 16) & 0xffff;
    if (in || !fr[0])
        snprintf(ip, sizeof ip, "%u", (unsigned)in);
    else
        ip[0] = 0;
    if (!ip[0] && !fr[0])
        strcpy(ip, "0");
    snprintf(out, EM_TEXT_MAX, "%s%s%s", neg && (cd || strcmp(ip, "0") || fr[0]) ? "-" : "", ip, fr);
}

/* printlib ffptoascii (FloatMode 0), as libs/acidlibs/printlib.c
   translates it from the binary */
static const struct { int16_t e; uint32_t m; } tens[48] = {
    { 0x0040, 0x8ac72305 }, { 0x003c, 0xde0b6b3a }, { 0x0039, 0xb1a2bc2f }, { 0x0036, 0x8e1bc9bf },
    { 0x0032, 0xe35fa932 }, { 0x002f, 0xb5e620f5 }, { 0x002c, 0x9184e72a }, { 0x0028, 0xe8d4a510 },
    { 0x0025, 0xba43b740 }, { 0x0022, 0x9502f900 }, { 0x001e, 0xee6b2800 }, { 0x001b, 0xbebc2000 },
    { 0x0018, 0x98968000 }, { 0x0014, 0xf4240000 }, { 0x0011, 0xc3500000 }, { 0x000e, 0x9c400000 },
    { 0x000a, 0xfa000000 }, { 0x0007, 0xc8000000 }, { 0x0004, 0xa0000000 }, { 0x0001, 0x80000000 },
    { -3, 0xcccccccd }, { -6, 0xa3d70a3d }, { -9, 0x83126e98 }, { -13, 0xd1b71759 },
    { -16, 0xa7c5ac47 }, { -19, 0x8637bd06 }, { -23, 0xd6bf94d6 }, { -26, 0xabcc7712 },
    { -29, 0x89705f41 }, { -33, 0xdbe6fecf }, { -36, 0xafebff0c }, { -39, 0x8cbccc09 },
    { -43, 0xe12e1342 }, { -46, 0xb424dc35 }, { -49, 0x901d7cf7 }, { -53, 0xe69594bf },
    { -56, 0xb877aa32 }, { -59, 0x9392ee8f }, { -63, 0xec1e4a7e }, { -66, 0xbce50865 },
    { -69, 0x971da050 }, { -73, 0xf1c90081 }, { -76, 0xc16d9a01 }, { -79, 0x9abe14cd },
    { -83, 0xf79687ae }, { -86, 0xc6120625 }, { -89, 0x9e74d1b8 }, { -93, 0xfd87b5f3 },
};

static void print_float(char *res, uint32_t value)
{
    char buf1[16], fixbuf[52];
    char *buf, *out, *start;
    int ten = 19;
    int8_t b;
    int16_t bexp, dexp;
    uint32_t mant;

    memset(buf1, 0, sizeof buf1);
    if (!(uint8_t)value)
        value = 65;
    buf1[0] = '+';
    buf1[1] = '.';
    b = (int8_t)(uint8_t)value;
    if ((uint8_t)b & 0x80)
        buf1[0] += 2;
    b = (int8_t)(uint8_t)(((uint8_t)b + (uint8_t)b) ^ 0x80);
    mant = (value & ~0xffu) | 0x80;
    bexp = (int16_t)(b >> 1);
    dexp = 1;
    if (bexp < tens[ten].e) {
        do {
            ten++;
            dexp--;
        } while (ten < 47 && bexp < tens[ten].e);
        if (bexp != tens[ten].e)
            goto found;
    } else if (bexp > tens[ten].e) {
        do {
            ten--;
            dexp++;
        } while (ten > 0 && bexp > tens[ten].e);
        if (bexp != tens[ten].e)
            goto next;
    }
    if (mant >= tens[ten].m)
        goto found;
next:
    ten++;
    dexp--;
found:
    out = buf1 + 2;
    if ((int32_t)mant >= 0 || !(tens[ten].m & 0xff))
        mant &= ~0xffu;
    for (int i = 0; i < 7; i++) {
        uint16_t sh = (uint16_t)(bexp - tens[ten].e);
        uint32_t power = tens[ten].m;
        uint16_t count = 9;
        int borrow;

        if (ten < 47)
            ten++;
        sh &= 63;
        power = sh >= 32 ? 0 : power >> sh;
        for (;;) {
            borrow = mant < power;
            mant -= power;
            if (borrow || --count == 0xffff)
                break;
        }
        if (!borrow)
            count &= 0xff00;
        mant += power;
        *out++ = (char)((uint8_t)-(uint8_t)((uint8_t)count - 9) | '0');
    }
    *out++ = '0';
    if (dexp < 8 || (uint16_t)dexp >= 0xfff8) {
        fixbuf[25] = '0';
        for (int i = 0; i < 24; i++) {
            fixbuf[i] = '0';
            fixbuf[26 + i] = '0';
        }
        fixbuf[50] = fixbuf[51] = 0;
        fixbuf[24] = '.';
        buf = fixbuf + 24;
        if (dexp < 0)
            buf++;
        buf -= dexp;
        out = buf1 + 2;
        start = buf;
        for (int i = 0; i < 8; i++) {
            if (*buf == '.')
                buf++;
            *buf++ = *out++;
        }
        if (buf >= fixbuf + 25) {
            for (;;) {
                if (*--buf != '0')
                    break;
                *buf = 0;
            }
            if (*buf != '.') {
                *++buf = 0;
                goto sign;
            }
            if (!(buf > start)) {
                strcpy(res, "0");
                return;
            }
        }
        fixbuf[24] = 0;
sign:
        buf = start;
        if (buf >= fixbuf + 24)
            buf = fixbuf + 24;
        *--buf = buf1[0];
        if (*buf != '-')
            buf++;
        snprintf(res, EM_TEXT_MAX, "%s", buf);
        return;
    }
    buf = buf1;
    buf[1] = buf[2];
    buf[2] = '.';
    while (*--out == '0')
        ;
    if (*out++ == '.') {
        out--;
        if (buf[1] == '0')
            goto term;
    }
    dexp--;
    *out++ = 'E';
    *out++ = '+';
    if (dexp < 0) {
        out[-1] = '-';
        dexp = -dexp;
    }
    if (dexp / 10)
        *out++ = (char)(dexp / 10 + '0');
    *out++ = (char)(dexp % 10 + '0');
term:
    *out = 0;
    if (*buf != '-')
        buf++;
    snprintf(res, EM_TEXT_MAX, "%s", buf);
}

/* ------------------------------------------------------------------- the model */

/* an operand in its register */
struct reg {
    uint32_t v;
    int t;                      /* its type (TB..TF, TS) */
    int raw;                    /* EM_R_1178: a raw Quick under a Long type so far */
    struct rat id;              /* the ideal value */
    int isvar;                  /* a variable (one element for cmpownleft) */
    int conv_from, conv_to;     /* the conversion varcont made (0: none) */
    uint32_t conv_v;            /* the value before it */
    unsigned conv_flags;        /* the flags before it */
    int litcut;                 /* a fraction literal cut to a whole number (#1188) */
    struct rat lit;             /* the literal */
    int isconst;
    char s[16];                 /* a string */
};

struct ev {
    unsigned rules;
    int checked;
    unsigned flags;
    int illegal;
    int cd;                     /* the CD's libraries */
    struct rat pow;             /* the ideal of the last ^ */
    uint64_t powsig;            /* its two Float operands */
    int powknown;
};

static int fits(struct rat a, int t)
{
    switch (t) {
    case TB: return rat_cmp(a, rat_int(-128)) >= 0 && rat_cmp(a, rat_int(127)) <= 0;
    case TW: return rat_cmp(a, rat_int(-32768)) >= 0 && rat_cmp(a, rat_int(32767)) <= 0;
    case TL: return rat_cmp(a, rat_int(INT32_MIN)) >= 0 && rat_cmp(a, rat_int(INT32_MAX)) <= 0;
    case TQ: return rat_cmp(a, rat_int(-32768)) >= 0 && rat_cmp(a, rat_int(32768)) < 0;
    default: return 1;
    }
}

/* the ideal rounded to type t (a whole number: floor; Float: trunc) */
static struct rat ideal_to(struct rat a, int from, int to)
{
    if (to == TF || to == TS)
        return a;
    if (to == TQ)
        return from == TF ? rat_q16(a) : a;
    return from == TF ? rat_trunc(a) : rat_floor(a);
}

/* the register's value as a rational, in its type */
static struct rat reg_value(const struct reg *r)
{
    if (r->raw || r->t == TQ)
        return rat_make((int32_t)r->v, 65536);
    switch (r->t) {
    case TB:
    case TW: return rat_int((int16_t)r->v);
    case TL: return rat_int((int32_t)r->v);
    case TF: return ffp_rat(r->v);
    default: return rat_int(0);
    }
}

/* convtype: from -> to on r (blitz2.s:14693 convtable, eval.c) */
static void conv(struct ev *e, struct reg *r, int from, int to)
{
    uint32_t v = r->v;
    int ovf = 0;

    if (from == to)
        return;
    if (r->raw) {
        /* EM_R_1178: a raw Quick is converted from Quick, with its fraction
           ("#1178 direction: variant C", 2026-11-21; design decision 5 of
           quick-long-arithmetic: to Float by quicktofloat; to a whole
           number rounded down, "#1178 rounding rule decided") */
        from = TQ;
        r->raw = 0;
    }
    switch (from) {
    case TB:
    case TW:
        if (to == TL)
            v = (uint32_t)(int32_t)(int16_t)v;
        else if (to == TQ)
            v = v << 16;
        else if (to == TF)
            v = ffp_flt((int16_t)v);
        break;
    case TL:
        if (to == TQ)
            v = v << 16;
        else if (to == TF)
            v = ffp_flt((int32_t)v);
        break;
    case TQ:
        if (to == TB || to == TW)
            v = v >> 16;
        else if (to == TL)
            v = (uint32_t)(int32_t)(int16_t)(v >> 16);
        else if (to == TF)
            v = lib_qtof(v);
        break;
    case TF:
        if (to == TQ)
            v = lib_ftoq(v);
        else
            v = (uint32_t)ffp_fix(v, &ovf);
        break;
    }
    r->id = ideal_to(r->id, from, to);
    if (!fits(r->id, to) || ovf)
        e->flags |= EM_F_OVF;
    r->v = v;
    r->t = to;
}

static int kind_type(enum em_kind k)
{
    switch (k) {
    case EM_B: return TB;
    case EM_W: return TW;
    case EM_L: return TL;
    case EM_Q: return TQ;
    case EM_F: return TF;
    case EM_S: return TS;
    default: return 0;
    }
}

/* the bits of a variable's value in its type */
static uint32_t var_bits(int t, const char *text)
{
    struct rat a = rat_text(text);

    switch (t) {
    case TB:
    case TW:
    case TL: return (uint32_t)(int32_t)rat_whole(a);
    case TQ: return (uint32_t)(int32_t)rat_whole(rat_floor(rat_op(a, rat_int(65536), '*')));
    case TF: return ffp_of(a);
    default: return 0;
    }
}

/* consttype2 (libcall.c, blitz2.s:10913): the literal text (no sign) sized
   to the type so far *T */
static void constant(struct ev *e, const char *text, int *T, struct reg *r)
{
    struct rat a = rat_text(text);
    int isfloat = strchr(text, '.') != NULL;
    int t = *T;
    int32_t iv = isfloat ? 0 : (int32_t)rat_whole(a);
    uint32_t ffp = 0;

    r->isconst = 1;
    r->id = a;
    r->lit = a;
    if (isfloat) {
        /* fetchpoint2: whole + digits/10^n, in FFP */
        const char *p = strchr(text, '.');
        int32_t whole = (int32_t)strtol(text, NULL, 10), digits = 0, div = 1;

        for (p++; *p; p++) {
            digits = digits * 10 + (*p - '0');
            div *= 10;
        }
        ffp = ffp_flt(whole);
        if (digits)
            ffp = ffp_add(ffp_div(ffp_flt(digits), ffp_flt(div)), ffp, 0);
    }
    if (t == 0) {
        if (isfloat)
            goto dofloat;
        t = TB;
        goto byte;
    }
    switch (t) {
    case TB: goto byte;
    case TW:
        if (isfloat)
            goto dofloat;
        goto over1;
    case TL:
        if (isfloat)
            goto dofloat;
        r->v = (uint32_t)iv;
        goto done;
    case TQ:
        if (isfloat) {
            int32_t w = ffp_fix(ffp, NULL);
            uint32_t frac = ffp_add(ffp, ffp_flt(w), 1);

            if (w > 32767 || w < -32768)
                goto dofloat;
            frac = (uint32_t)ffp_fix(ffp_mul(frac, 0x80000051u), NULL);
            r->v = ((uint32_t)(uint16_t)w << 16) | (uint16_t)frac;
            goto done;
        }
        if (iv > 32767 || iv < -32768) {
            t = TL;
            r->v = (uint32_t)iv;
            goto done;
        }
        r->v = (uint32_t)iv << 16;
        goto done;
    default:
        if (!isfloat)
            ffp = ffp_flt(iv);
        goto dofloat;
    }
byte:
    if (isfloat) {
        /* BUG(orig) (#1188): the Byte path cuts a fraction literal
           (makeint); EM_R_1188: it keeps the fraction as the Word path does
           (compiler-byte-fraction-const: "the constant keeps its fraction in
           every context"; task-coordinator-decisions.md 2026-11-22 "#1186
           fixed; #1188 found") */
        if (e->rules & EM_R_1188)
            goto dofloat;
        iv = ffp_fix(ffp, NULL);
        r->litcut = 1;
    }
    if (iv <= 127 && iv >= -128) {
        r->v = (uint32_t)iv;
        goto done;
    }
over1:
    if (isfloat)
        iv = ffp_fix(ffp, NULL);
    t = TW;
    if (iv <= 32767 && iv >= -32768) {
        r->v = (uint32_t)iv;
        goto done;
    }
    t = TL;
    r->v = (uint32_t)iv;
    goto done;
dofloat:
    t = TF;
    r->v = ffp;
done:
    *T = t;
    r->t = t;
}

/* negate (eval.c, blitz2.s:9667): neg.w, neg.l or SPNeg by the type */
static void negate(struct reg *r)
{
    if (r->t == TF)
        r->v = ffp_neg(r->v);
    else if (r->t >= TL || r->raw)
        r->v = -r->v;
    else
        r->v = (r->v & 0xffff0000u) | (uint16_t)-(uint16_t)r->v;
    r->id = rat_op(rat_int(0), r->id, '-');
    r->lit = rat_op(rat_int(0), r->lit, '-');
}

/* an element: a variable (varcont) or a constant (a negative one negated
   after consttype2, as getelement2 / negate do) */
static void fetch(struct ev *e, enum em_kind k, const char *text, int *T, struct reg *r)
{
    int vt = kind_type(k);

    memset(r, 0, sizeof *r);
    if (k == EM_S) {
        snprintf(r->s, sizeof r->s, "%s", text);
        r->t = TS;
        /* varcont: a string is a string whatever the type so far */
        *T = TS;
        return;
    }
    if (!vt) {
        int neg = text[0] == '-';

        constant(e, neg ? text + 1 : text, T, r);
        if (neg)
            negate(r);
        return;
    }
    r->isvar = 1;
    r->t = vt;
    r->v = var_bits(vt, text);
    if (vt == TB)
        r->v = (uint32_t)(int32_t)(int8_t)r->v;
    else if (vt == TW)
        r->v = (uint32_t)(int32_t)(int16_t)r->v;
    r->id = rat_text(text);
    if (vt == TQ)
        r->id = rat_make((int32_t)r->v, 65536);
    else if (vt == TF)
        r->id = ffp_rat(r->v);
    /* varcont (eval.c, blitz2.s:10473) */
    if (!*T) {
        *T = vt;
        return;
    }
    if (*T == TL && vt == TQ) {
        r->conv_from = TQ;
        r->conv_to = TL;
        r->conv_v = r->v;
        r->conv_flags = e->flags;
        if (e->rules & EM_R_1178) {
            /* no cut at fetch: "#1178 direction: variant C" (2026-11-21),
               "Reihenfolge wie heute reicht, Operationen exakt rechnen" */
            r->raw = 1;
            r->t = TL;
            return;
        }
        /* the cut (#1178, open) */
        conv(e, r, TQ, TL);
        return;
    }
    if (*T > vt) {
        r->conv_from = vt;
        r->conv_to = *T;
        r->conv_v = r->v;
        r->conv_flags = e->flags;
        conv(e, r, vt, *T);
        r->t = *T;
        return;
    }
    *T = vt;
}

/* the raw Quick of a register cut to Long (floor), as varcont did */
static void cutraw(struct ev *e, struct reg *r)
{
    if (r->raw) {
        r->raw = 0;
        r->t = TQ;
        conv(e, r, TQ, TL);
    }
}

static int is_bitop(enum em_op o)
{
    return o == EM_AND || o == EM_OR || o == EM_EOR || o == EM_LSL || o == EM_LSR || o == EM_ASL
           || o == EM_ASR || o == EM_NOT;
}

/* a comparison by Scc of type t: -1/0 in the low word (ext.w), the high
   word of d0 kept */
static void scc(struct reg *a, int c, enum em_op op)
{
    int r = 0;

    switch (op) {
    case EM_EQ: r = c == 0; break;
    case EM_NE: r = c != 0; break;
    case EM_LT: r = c < 0; break;
    case EM_LE: r = c <= 0; break;
    case EM_GT: r = c > 0; break;
    default: r = c >= 0; break;
    }
    a->v = (a->v & 0xffff0000u) | (r ? 0xffff : 0);
    a->t = TW;
    a->raw = 0;
    a->id = rat_int(r ? -1 : 0);
}

static int cmp_typed(const struct reg *a, const struct reg *b, int t)
{
    switch (t) {
    case TB: return (int8_t)a->v < (int8_t)b->v ? -1 : (int8_t)a->v > (int8_t)b->v;
    case TW: return (int16_t)a->v < (int16_t)b->v ? -1 : (int16_t)a->v > (int16_t)b->v;
    case TL:
    case TQ: return (int32_t)a->v < (int32_t)b->v ? -1 : (int32_t)a->v > (int32_t)b->v;
    case TF: return ffp_cmp(a->v, b->v);
    case TS: return strcmp(a->s, b->s) < 0 ? -1 : strcmp(a->s, b->s) > 0;
    default: return 0;
    }
}

/* the ideal of a operation on the ideals (whole-number result types round:
   a division towards zero, the rest down; Quick: * and / to 1/65536
   towards zero) */
static struct rat ideal_op(enum em_op op, struct rat a, struct rat b, int t)
{
    struct rat r;

    switch (op) {
    case EM_ADD: r = rat_op(a, b, '+'); break;
    case EM_SUB: r = rat_op(a, b, '-'); break;
    case EM_MUL: r = rat_op(a, b, '*'); break;
    case EM_DIV:
        r = rat_op(a, b, '/');
        if (t == TQ)
            return rat_q16(r);
        return t == TF ? r : rat_trunc(r);
    default: return a;
    }
    if (t == TQ && op == EM_MUL)
        return rat_q16(r);
    return t == TF || t == TQ ? r : rat_floor(r);
}

/* the operator op on a (left, result) and b in type t (doop and the
   handlers of eval.c) */
static void doop(struct ev *e, enum em_op op, struct reg *a, struct reg *b, int t)
{
    uint32_t x = a->v, y = b->v;
    int rt = t;
    struct rat id = a->id;
    int checkid = 1;

    if (t == TS) {
        if (op != EM_ADD) {
            scc(a, cmp_typed(a, b, TS), op);
            return;
        }
        strncat(a->s, b->s, sizeof a->s - strlen(a->s) - 1);
        return;
    }
    switch (op) {
    case EM_ADD:
        if (t == TB) {
            x = (x & 0xffff0000u) | (uint16_t)(x + y);
            rt = TW;
        } else if (t == TW) {
            x = (uint32_t)(int32_t)(int16_t)(x + y);
            rt = TL;
        } else if (t == TF)
            x = ffp_add(x, y, 0);
        else
            x += y;
        id = ideal_op(op, a->id, b->id, rt);
        break;
    case EM_SUB:
        if (t <= TW)
            x = (x & 0xffff0000u) | (uint16_t)(x - y);
        else if (t == TF)
            x = ffp_add(x, y, 1);
        else
            x -= y;
        id = ideal_op(op, a->id, b->id, rt);
        break;
    case EM_MUL:
        if (t <= TW) {
            x = (uint32_t)((int32_t)(int16_t)x * (int32_t)(int16_t)y);
            rt = t + 1;
        } else if (t == TL)
            x = lib_lmul(x, y, e->cd);
        else if (t == TQ)
            x = lib_qmul(x, y, e->cd);
        else
            x = ffp_mul(x, y);
        id = ideal_op(op, a->id, b->id, rt);
        break;
    case EM_DIV:
        if (t <= TW) {
            if (t == TB)
                y = (uint32_t)(int32_t)(int16_t)y;
            x = m68k_divs((uint32_t)(int32_t)(int16_t)x, y);
        } else if (t == TL)
            x = lib_divide(x, y, 31);
        else if (t == TQ)
            x = lib_divide(x, y, 47);
        else
            x = ffp_div(x, y);
        id = ideal_op(op, a->id, b->id, rt);
        break;
    case EM_MOD:
        if (t == TL || t == TF)
            e->flags |= EM_F_MODLF;    /* not folded as a constant (nocando) */
        if (t <= TW)
            x = lib_modw(x, y);
        else if (t == TL)
            x = lib_modl(x, y, e->cd);
        else if (t == TQ) {
            x = lib_modw((x << 16) | (x >> 16), (y << 16) | (y >> 16));
            rt = TW;
        } else {
            x = lib_modl((uint32_t)ffp_fix(x, NULL), (uint32_t)ffp_fix(y, NULL), e->cd);
            rt = TL;
        }
        /* the ideal: a whole-number MOD of non-negative values; else a
           question (Q-D, design decision 7) */
        if (rat_sign(a->id) < 0 || rat_sign(b->id) < 0 || !rat_isint(a->id) || !rat_isint(b->id))
            e->flags |= EM_F_MODNF;
        if (rat_isint(a->id) && rat_isint(b->id) && rat_sign(b->id) > 0 && rat_sign(a->id) >= 0)
            id = rat_int(rat_whole(a->id) % rat_whole(b->id));
        else
            checkid = 0;
        break;
    case EM_AND:
    case EM_OR:
    case EM_EOR:
        if (t == TF) {
            e->illegal = 1;
            return;
        }
        {
            uint32_t r = op == EM_AND ? x & y : op == EM_OR ? x | y : x ^ y;
            uint32_t mask = t == TB ? 0xff : t == TW ? 0xffff : 0xffffffffu;

            x = (x & ~mask) | (r & mask);
        }
        /* whole operands, also as Quicks (their low words 0): exact */
        if (rat_isint(a->id) && rat_isint(b->id)) {
            int64_t p = rat_whole(a->id), q = rat_whole(b->id);

            id = rat_int(op == EM_AND ? (p & q) : op == EM_OR ? (p | q) : (p ^ q));
        } else
            checkid = 0;
        break;
    case EM_LSL:
    case EM_ASL:
    case EM_LSR:
    case EM_ASR:
        if (t == TF) {
            e->illegal = 1;
            return;
        }
        /* shpoke: the count to Word, the value to Long unless a Quick */
        conv(e, b, t, TW);
        if (t != TQ) {
            conv(e, a, t, TL);
            rt = TL;
        }
        x = a->v;
        {
            unsigned n = b->v & 63;

            if (op == EM_LSR)
                x = n >= 32 ? 0 : x >> n;
            else if (op == EM_ASR)
                x = n >= 32 ? ((int32_t)x < 0 ? 0xffffffffu : 0) : (uint32_t)((int32_t)x >> n);
            else
                x = n >= 32 ? 0 : x << n;
            if (rat_isint(a->id) && rat_isint(b->id) && t != TQ) {
                int64_t p = rat_whole(a->id);

                if (op == EM_LSR)
                    id = rat_int((int64_t)((uint32_t)p >> (n & 31)));
                else if (op == EM_ASR)
                    id = rat_int(p >> (n & 31));
                else
                    id = rat_int(p * ((int64_t)1 << (n & 31)));
            } else if (t == TQ && rat_isint(b->id) && n < 32) {
                /* a Quick shifted as raw 16.16 bits: times or divided by 2^n
                   (down to 1/65536); a negative one shifted right logically
                   is no such value */
                struct rat p2 = rat_int((int64_t)1 << n);

                if (op == EM_LSL || op == EM_ASL)
                    id = rat_op(a->id, p2, '*');
                else
                    id = rat_op(rat_floor(rat_op(rat_op(a->id, rat_int(65536), '*'), p2, '/')), rat_int(65536), '/');
            } else
                checkid = 0;
        }
        break;
    case EM_POW:
        /* dopow: both to Float, mathtrans SPPow (not modelled) */
        if (a->raw)
            conv(e, a, TQ, TF);
        else
            conv(e, a, t, TF);
        if (b->raw)
            conv(e, b, TQ, TF);
        else
            conv(e, b, t, TF);
        rt = TF;
        x = 0;
        e->flags |= EM_F_POW;
        e->powsig = (uint64_t)a->v << 32 | b->v;
        /* SPPow of the two Float values as they are (a fraction constant
           cut under a Byte type stays cut); measured: the sign of the base
           is dropped ((-2.5)^3 gives 15.6, BUG(orig) (#1189)).  Where no
           issue changes it, the original's measured value is the cell's
           baseline, also SPPow's precision (1000^2 gives 999999): a
           regression check, not a decision that the value is right
           ("Matrix questions Q-A, Q-C, Q-D, Q-E", task-decisions.md,
           2026-11-22, Q-E: "Original, Vorzeichen per #1189 (Recommended)",
           and the "Correction by the user" there) */
        {
            struct rat x1 = ffp_rat(a->v), y1 = ffp_rat(b->v);
            int neg = rat_sign(x1) < 0;

            if (neg)
                x1 = rat_op(rat_int(0), x1, '-');
            if (rat_isint(y1) && x1.ok && rat_whole(y1) >= 0 && rat_whole(y1) <= 8) {
                id = rat_int(1);
                for (int64_t i = 0; i < rat_whole(y1); i++)
                    id = rat_op(id, x1, '*');
            } else
                id = rat_dbl(pow(x1.x, y1.x));
            if (neg && (e->rules & EM_R_1189)) {
                /* EM_R_1189 (issue #1189, the same entry: "the negative-base
                   cells belong to #1189"): a whole exponent gives the signed
                   power, SPPow's magnitude with the sign of an odd power; a
                   fractional exponent has no rule yet (the issue: "to be
                   decided"): a question */
                if (!rat_isint(y1))
                    e->flags |= EM_F_POWNEG;
                else if (rat_whole(y1) & 1) {
                    id = rat_op(rat_int(0), id, '-');
                    e->flags |= EM_F_POWSIGN;
                }
            }
        }
        e->pow = id;
        e->powknown = 1;
        checkid = 0;
        break;
    default:
        /* a comparison in type t */
        if (t == TF || t <= TQ) {
            scc(a, cmp_typed(a, b, t), op);
            return;
        }
        e->illegal = 1;
        return;
    }
    a->v = x;
    a->t = rt;
    a->raw = 0;
    a->id = id;
    /* qdivide's loop gives a quotient up to 1/65536 above the one towards
       zero (quick-long-arithmetic, design decision 3, "l/q"): the Quick
       division of its type, no overflow */
    if (checkid && op == EM_DIV && rt == TQ && fits(id, rt)) {
        struct rat d = rat_op(reg_value(a), id, '-');

        if (rat_cmp(d, rat_make(1, 65536)) <= 0 && rat_cmp(d, rat_make(-1, 65536)) >= 0)
            checkid = 0;
    }
    if (checkid && rt != TF && (!fits(id, rt) || rat_cmp(reg_value(a), id) != 0)) {
        if (is_bitop(op) && !(e->flags & EM_F_OVF))
            e->flags |= EM_F_BITQ;
        else
            e->flags |= EM_F_OVF;
    }
    if (rt == TF && checkid && id.ok && rat_sign(id) && fabs(ffp_rat(a->v).x / id.x - 1) > 1e-6)
        e->flags |= EM_F_OVF;
}

/* EM_R_1178: an operation with a raw Quick under a Long type so far,
   computed with the fraction (design decision 3 of quick-long-arithmetic):
   a division towards zero, + - * down ("#1178 rounding rule decided",
   2026-11-21: "Zwischenlösung: / gegen Null (Recommended)"); a Long
   result beyond 32 bits wraps as longmult ("quick-long-arithmetic: first
   open questions", 2026-11-21: "Wie longmult (Recommended)") */
static void mixed(struct reg *a, struct reg *b, enum em_op op)
{
    struct rat x = a->raw || a->t == TQ ? rat_make((int32_t)a->v, 65536) : rat_int((int32_t)a->v);
    struct rat y = b->raw || b->t == TQ ? rat_make((int32_t)b->v, 65536) : rat_int((int32_t)b->v);
    struct rat r;

    switch (op) {
    case EM_ADD: r = rat_floor(rat_op(x, y, '+')); break;
    case EM_SUB: r = rat_floor(rat_op(x, y, '-')); break;
    case EM_MUL: r = rat_floor(rat_op(x, y, '*')); break;
    default: r = rat_trunc(rat_op(x, y, '/')); break;
    }
    a->v = (uint32_t)(int64_t)(r.ok ? (int64_t)r.n : (int64_t)r.x);
    a->t = TL;
    a->raw = 0;
    a->id = r;
}

static int ctx_type(enum em_ctx x, int checked, unsigned rules)
{
    static const int t[] = { TB, TW, TL, TQ, TF, TL, TW, TQ, TF, 0, TL, TW, 0 };

    if (x == EM_I2 && (checked || (rules & EM_R_1179)))
        /* arrevalchk: Long with runtime error checking; EM_R_1179: every
           index a Long ("#1179, CRC findings, trailers, merge", 2026-11-21:
           "Keine Prüfung im Release (Recommended)", direction A) */
        return TL;
    return t[x];
}

/* the expression of the cell, its result in *r with its type; 0, or 1
   when the compiler refuses it */
static int expression(struct ev *e, const struct em_set *s, const struct em_cell *c, int asked, struct reg *r)
{
    int T = asked;
    struct reg b;

    if (c->right < 0) {
        fetch(e, c->left, s->value[c->left], &T, r);
        if (c->op == EM_NEG) {
            if (r->t == TS)
                return 1;
            negate(r);
        } else {
            /* notit (a refused one is a question too: NOT 1.5 under a Byte
               type compiles until #1188's fix makes the constant a Float) */
            if (r->t == TQ || c->left == EM_Q || c->left == EM_F || c->left == EM_CF)
                e->flags |= EM_F_BITQ;
            if (r->t >= TF)
                return 1;
            cutraw(e, r);
            if (r->t >= TL)
                r->v = ~r->v;
            else
                r->v = (r->v & 0xffff0000u) | (uint16_t)~r->v;
            r->id = rat_op(rat_int(-1), r->id, '-');
        }
    } else {
        int cmp = em_op_is_compare(c->op), left;
        enum em_kind rk = (enum em_kind)c->right;
        /* the right operand of a shift and of ^ is the count */
        const char *rtext = c->op == EM_POW || (c->op >= EM_LSL && c->op <= EM_ASR) ? s->count[rk] : s->value[rk];

        fetch(e, c->left, s->value[c->left], &T, r);
        if (cmp && (e->rules & EM_R_1177)) {
            /* cmpownleft (#1177: "quick-compare-types (#1177): open
               questions", 2026-11-21, "(a), Rest zu #1180"): a one-element
               left operand gets its own type back */
            if (r->isvar && r->conv_to
                && ((r->conv_from == TQ && r->conv_to == TL) || (r->conv_from <= TL && r->conv_to == TQ)
                    || (r->conv_from <= TQ && r->conv_to == TF))) {
                r->v = r->conv_v;
                e->flags = r->conv_flags;   /* its overflow is gone with it */
                r->t = r->conv_from;
                r->raw = 0;
                r->id = rat_text(s->value[c->left]);
                if (r->t == TQ)
                    r->id = rat_make((int32_t)r->v, 65536);
                T = r->t;
            }
            if (r->litcut && (e->rules & EM_R_1186)) {
                /* fraclitfloat (#1186, a port defect fixed after the
                   measurement): the cut literal is a Float again */
                r->v = ffp_of(r->lit);
                r->t = T = TF;
                r->litcut = 0;
                r->id = r->lit;
            }
        }
        left = T;
        if (cmp && (e->rules & EM_R_1177))
            T = 0;
        fetch(e, rk, rtext, &T, &b);
        if (cmp && (e->rules & EM_R_1177)) {
            /* compareop: a Long and a Quick exactly, else the lower one up
               ("#1177: right operand as planned; Float destination
               measured first", 2026-11-21) */
            int right = T;

            if (r->raw)
                r->t = left = TQ, r->raw = 0;
            if ((left == TL && right == TQ) || (left == TQ && right == TL)) {
                struct rat x = left == TQ ? rat_make((int32_t)r->v, 65536) : rat_int((int32_t)r->v);
                struct rat y = right == TQ ? rat_make((int32_t)b.v, 65536) : rat_int((int32_t)b.v);

                scc(r, rat_cmp(x, y), c->op);
            } else {
                if (left && right < left) {
                    conv(e, &b, right, left);
                    right = left;
                }
                if (left)
                    conv(e, r, left, right);
                if (right == TS && left != TS)
                    return 1;
                doop(e, c->op, r, &b, right);
            }
        } else {
            int t2 = T;

            if ((e->rules & EM_R_1178) && t2 == TL
                && (c->op == EM_ADD || c->op == EM_SUB || c->op == EM_MUL || c->op == EM_DIV)
                && (r->raw || b.raw || ((c->op == EM_MUL || c->op == EM_DIV) && left == TQ))) {
                /* the mixed path; with a Quick left whose Long right
                   operand set the type (q*100000 with no type so far):
                   "#1178 d refined; e questioned", 2026-11-21: "Bei "*"
                   oder "/" brauchen wir für optimale Genauigkeit die neuen
                   Routinen" (+ and - keep the late conversion) */
                if (r->t != TQ && !r->raw && left != TL)
                    conv(e, r, left, TL);
                if (b.t != TQ && !b.raw && b.t != TL)
                    conv(e, &b, b.t, TL);
                mixed(r, &b, c->op);
            } else {
                /* a raw Quick: cut for every other operator (today's value,
                   design decision 3 of quick-long-arithmetic), with its
                   fraction to a Float and to ^ (decision 5) */
                if (c->op != EM_POW) {
                    if (r->raw && t2 != TF)
                        cutraw(e, r);
                    if (b.raw)
                        cutraw(e, &b);
                }
                if (left && left != t2 && !(r->raw && c->op == EM_POW))
                    conv(e, r, left, t2);
                if (cmp && t2 == TS && left != TS)
                    return 1;
                if (is_bitop(c->op)
                    && (c->left == EM_Q || c->left == EM_F || c->left == EM_CF || rk == EM_Q
                        || rk == EM_F || rk == EM_CF))
                    e->flags |= EM_F_BITQ;
                /* a fraction operand of MOD is cut first: Q-D, whatever the
                   cut leaves */
                if (c->op == EM_MOD
                    && (c->left == EM_Q || c->left == EM_F || c->left == EM_CF || rk == EM_Q || rk == EM_F
                        || rk == EM_CF))
                    e->flags |= EM_F_MODNF;
                if (c->op == EM_POW
                    && (c->left != EM_Q && c->left != EM_F && c->left != EM_CF && rk != EM_Q && rk != EM_F
                        && rk != EM_CF))
                    e->flags |= EM_F_POWINT;
                doop(e, c->op, r, &b, t2);
                if (e->illegal)
                    return 1;
            }
        }
    }
    /* maineval2: a raw Quick rounded down, then the type asked for */
    if (r->raw)
        cutraw(e, r);
    if (asked && r->t != asked) {
        if (r->t == TS || asked == TS)
            return 1;
        conv(e, r, r->t, asked);
    }
    return 0;
}

/* the text of a value of type t as Print gives it */
static void print_typed(struct ev *e, char *out, const struct reg *r)
{
    switch (r->t) {
    case TB: print_long(out, (int8_t)r->v); break;
    case TW: print_long(out, (int16_t)r->v); break;
    case TL: print_long(out, (int32_t)r->v); break;
    case TQ: print_quick(out, r->v, e->cd); break;
    case TF: print_float(out, r->v); break;
    default: snprintf(out, EM_TEXT_MAX, "%s", r->s); break;
    }
}

/* a pow result converted to a whole number: known when the ideal is far
   enough from a step (SPPow's error is below 1e-6) */
static int pow_whole(struct ev *e, int asked, int64_t *v)
{
    double x = e->pow.x;

    if (!e->powknown || asked == TF || asked == TQ || asked == 0)
        return 0;
    if (fabs(x - round(x)) < 1e-4 * (fabs(x) > 1 ? fabs(x) : 1))
        return 0;
    if (fabs(x) > 2147483647.0) {
        *v = x < 0 ? INT32_MIN : INT32_MAX;
        return 1;
    }
    *v = (int64_t)x;
    return 1;
}

/* the index of an index cell (asked Long or Word), 0 or 1 refused */
static int index_value(struct ev *e, const struct em_set *s, const struct em_cell *c, int checked, int64_t *idx,
                       int *known)
{
    struct reg r;
    int asked = ctx_type(c->ctx, checked, e->rules);

    e->checked = checked;
    if (expression(e, s, c, asked, &r))
        return 1;
    *known = 1;
    if (e->flags & EM_F_POW) {
        int64_t v = 0;

        *known = pow_whole(e, asked, &v);
        if (!*known && (e->flags & EM_F_POWSIGN) && e->pow.x < -1.0001) {
            /* EM_R_1189: a negative power below -1 is outside the array
               whatever SPPow's last bits are */
            *known = 1;
            v = -1;
        }
        r.v = (uint32_t)v;
    }
    *idx = asked == TW ? (int64_t)(uint16_t)r.v : (int64_t)(int32_t)r.v;
    return 0;
}

void em_model(const struct em_matrix *m, const struct em_cell *c, unsigned rules, int checked, struct em_value *out)
{
    const struct em_set *s = &m->sets[c->set];
    struct ev e = { 0 };
    struct reg r;
    int asked;
    int ok;

    memset(out, 0, sizeof *out);
    e.rules = rules;
    e.cd = (rules & EM_R_CDLIB) != 0;
    if (c->ctx == EM_I1 || c->ctx == EM_I2) {
        int64_t ic = 0, iu = 0;
        int kc = 0, ku = 0, refc;
        struct ev u = { 0 };

        /* the checked build: an element, or "Array Subscript out of Range" */
        refc = index_value(&e, s, c, 1, &ic, &kc);
        if (checked) {
            out->flags = e.flags;
            out->sig = e.powsig;
            if (refc)
                strcpy(out->text, "!noncompile");
            else if (!kc)
                strcpy(out->text, "?");
            else if (ic < 0 || ic > m->dim)
                strcpy(out->text, "!runtime");
            else
                snprintf(out->text, EM_TEXT_MAX, "%lld", (long long)(ic * 16 + 1));
            return;
        }
        /* unchecked: the cells outside in the checked build are left out
           (design decision 8); a later index is a Word (#1179); a cell the
           checked build refuses is outside when it reads no element */
        u.rules = rules;
        u.cd = e.cd;
        if (index_value(&u, s, c, 0, &iu, &ku)) {
            strcpy(out->text, "!noncompile");
            out->flags = u.flags;
            return;
        }
        out->flags = u.flags | (refc ? 0 : e.flags);
        out->sig = u.powsig;
        if (!refc && !kc)
            strcpy(out->text, "?");
        else if (!refc && (ic < 0 || ic > m->dim))
            strcpy(out->text, "!outside");
        else if (!ku)
            strcpy(out->text, "?");
        else if (c->ctx == EM_I1 ? iu < 0 || iu > m->dim : (iu & 0xffff) > m->dim)
            strcpy(out->text, "!garbage");
        else
            snprintf(out->text, EM_TEXT_MAX, "%lld", (long long)((c->ctx == EM_I1 ? iu : (iu & 0xffff)) * 16 + 1));
        return;
    }
    asked = ctx_type(c->ctx, checked, rules);
    if (c->ctx == EM_IF || c->ctx == EM_PRINT)
        asked = 0;
    ok = !expression(&e, s, c, asked, &r);
    out->flags = e.flags;
    out->sig = e.powsig;
    if (!ok) {
        strcpy(out->text, "!noncompile");
        return;
    }
    if (e.flags & EM_F_POW) {
        int64_t v = 0;

        if (c->ctx == EM_IF && (rules & EM_R_1187) && e.powknown && fabs(e.pow.x) > 1e-30) {
            /* EM_R_1187: the Float condition tests the power's value, which
               is not 0 whatever SPPow's last bits are */
            strcpy(out->text, "T");
            return;
        }
        if (c->ctx == EM_IF && !(rules & EM_R_1187))
            out->flags |= EM_F_D1;      /* BUG(orig) (#1187): what d1 holds */
        if (!pow_whole(&e, asked, &v)) {
            strcpy(out->text, e.flags & EM_F_POWSIGN ? "?-" : "?");
            return;
        }
        r.v = (uint32_t)v;
    }
    switch (c->ctx) {
    case EM_DB: print_long(out->text, (int8_t)r.v); break;
    case EM_DW: case EM_PW: print_long(out->text, (int16_t)r.v); break;
    case EM_DL: case EM_PL: print_long(out->text, (int32_t)r.v); break;
    case EM_DQ: case EM_DF: case EM_PQ: case EM_PF:
        snprintf(out->text, EM_TEXT_MAX, "%08X", (unsigned)r.v);
        break;
    case EM_IF: {
        int t;

        switch (r.t) {
        case TB: t = (uint8_t)r.v != 0; break;
        case TW: t = (uint16_t)r.v != 0; break;
        case TF:
            if (!(rules & EM_R_1187)) {
                /* BUG(orig) (#1187): SPTst tests d1, what the code before
                   left there */
                out->flags |= EM_F_D1;
                strcpy(out->text, "?");
                return;
            }
            /* EM_R_1187 (fixed, task-coordinator-decisions.md 2026-11-22
               "Float conditions test d1"): the value is tested */
            t = !ffp_zero(r.v);
            break;
        default: t = r.v != 0; break;
        }
        strcpy(out->text, t ? "T" : "F");
        break;
    }
    default:
        print_typed(&e, out->text, &r);
        break;
    }
}
