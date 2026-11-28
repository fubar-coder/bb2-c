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
 * set names its entry in ~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md (heading, date).
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
    {
        /* exact while the products fit 128 bits, else the double (#1232: a
           product of two denominators near RAT_LIMIT wrapped to 0, and a
           later division by it raised SIGFPE) */
        __int128 p, q, s;
        int ovf = 0;

        switch (op) {
        case '+':
        case '-':
            ovf |= __builtin_mul_overflow(a.n, b.d, &p);
            ovf |= __builtin_mul_overflow(b.n, a.d, &q);
            ovf |= op == '+' ? __builtin_add_overflow(p, q, &s) : __builtin_sub_overflow(p, q, &s);
            ovf |= __builtin_mul_overflow(a.d, b.d, &q);
            break;
        case '*':
            ovf |= __builtin_mul_overflow(a.n, b.n, &s);
            ovf |= __builtin_mul_overflow(a.d, b.d, &q);
            break;
        default:
            if (!b.n)
                return rat_dbl(a.x / b.x);      /* by zero (an expression's value): not exact */
            ovf |= __builtin_mul_overflow(a.n, b.d, &s);
            ovf |= __builtin_mul_overflow(a.d, b.n, &q);
            break;
        }
        if (ovf) {
            switch (op) {
            case '+': return rat_dbl(a.x + b.x);
            case '-': return rat_dbl(a.x - b.x);
            case '*': return rat_dbl(a.x * b.x);
            default: return rat_dbl(a.x / b.x);
            }
        }
        return rat_make(s, q);
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

/* the multiple of 1/65536 rounded down (EM_R_1221) */
static struct rat rat_q16dn(struct rat a)
{
    return rat_op(rat_floor(rat_op(a, rat_int(65536), '*')), rat_int(65536), '/');
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
   instructions run; a divisor 0 gives $7FFFFFFF (cd: the CD's library),
   in the port's library the largest number with the dividend's sign
   (#200, libs/acidlibs/ldivlib.c) */
static uint32_t lib_divide(uint32_t d0, uint32_t d1, int count, int cd)
{
    uint32_t d3 = 0, d4 = 0;
    uint16_t d2 = 0;
    uint32_t xbit;
    int d5 = count;

    if (!d1)
        return cd || (int32_t)d0 >= 0 ? 0x7fffffffu : 0x80000001u;
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
    uint32_t q = lib_divide(a, b, 31, cd);
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
    int wide;                   /* C5/C6: a 32.16 product with no wanted type: exact in a comparison */
    struct rat wid;             /* its exact value */
    int wideovf;                /* it does not fit the Quick or Long it is anywhere else */
    char s[16];                 /* a string */
};

struct ev {
    uint64_t rules;
    int checked;
    unsigned flags;
    int illegal;
    int cd;                     /* the CD's libraries */
    struct rat pow;             /* the ideal of the last ^ */
    uint64_t powsig;            /* its two Float operands */
    int powknown;
    int c5ovf;                  /* C5: a 32.16 product beyond its consumer (EM_R_C5ERR) */
    int divtrap;                /* a DIVS.W by 0 ran: the CPU's division trap, no value */
    int wholeconst;             /* EM_R_1224: flow of a constquick expression, wholesub a constant */
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
           every context"; ~/.local/share/bb2-c-work/decisions/rounds/quick/coordinator-decisions.md 2026-11-22 "#1186
           fixed; #1188 found"); a whole value written with a point (2.0)
           stays the whole Byte, as exprtype.c's consttype2 */
        if ((e->rules & EM_R_1188)
            && (!rat_isint(a) || rat_cmp(a, rat_int(INT32_MAX)) > 0 || rat_cmp(a, rat_int(INT32_MIN)) < 0))
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
            if (!(int16_t)y)
                e->divtrap = 1;     /* DIVS.W by 0 */
            x = m68k_divs((uint32_t)(int32_t)(int16_t)x, y);
        } else if (t == TL)
            x = lib_divide(x, y, 31, e->cd);
        else if (t == TQ)
            x = lib_divide(x, y, 47, e->cd);
        else {
            if (ffp_zero(y))
                e->divtrap = 1;     /* SPDiv by 0: the division trap in both compilers (#1232) */
            x = ffp_div(x, y);
        }
        id = ideal_op(op, a->id, b->id, rt);
        break;
    case EM_MOD:
        if (t == TL || t == TF)
            e->flags |= EM_F_MODLF;    /* not folded as a constant (nocando) */
        if (t <= TW) {
            if (!(int16_t)y)
                e->divtrap = 1;     /* modlib's DIVS.W by 0 */
            x = lib_modw(x, y);
        } else if (t == TL)
            x = lib_modl(x, y, e->cd);
        else if (t == TQ) {
            if (!(int16_t)(y >> 16))
                e->divtrap = 1;     /* the whole part of the divisor 0 */
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
            /* a Byte lies in a register as a sign-extended Word: the original's
               byte operation (and.b, or.b, eor.b) keeps bits 8-15 of the left
               operand (-2 AND 0 is $FF00), which a widening takes as part of
               the value; EM_R_1218: the Word operation (#1218, #1217) */
            uint32_t mask = t == TB && !(e->rules & EM_R_1218) ? 0xff : t <= TW ? 0xffff : 0xffffffffu;

            x = (x & ~mask) | (r & mask);
        }
        /* EM_R_1209: under a Quick type so far on the whole parts, the
           fraction bits dropped, the result a Quick (quickwhole: clr.w
           after the operation; "Bit operators on a Quick: Word by
           dropping the fraction" and "... scope, result type, shifts,
           order", 2026-11-24: "(1) Drop the fraction bits", "Q: stays
           Quick") */
        if (t == TQ && (e->rules & EM_R_1209))
            x &= 0xffff0000u;
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
           ("Matrix questions Q-A, Q-C, Q-D, Q-E", ~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md,
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
                   fractional exponent keeps the original's value as its
                   baseline ("Matrix: last Q-F and Q-E cells", 2026-11-22:
                   "Baseline wie heute (Recommended)") */
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
   open questions", 2026-11-21: "Wie longmult (Recommended)"); a divisor
   0 as the routines the compiler calls (eval.c, mixedop): lqdiv for l/q
   and longdiv for q/p the largest number with the dividend's sign
   (longdiv of the CD's library $7FFFFFFF), q/l longdiv's quotient taken
   as a Quick, so +-32767 */
static void mixed(struct ev *e, struct reg *a, struct reg *b, enum em_op op)
{
    struct rat x = a->raw || a->t == TQ ? rat_make((int32_t)a->v, 65536) : rat_int((int32_t)a->v);
    struct rat y = b->raw || b->t == TQ ? rat_make((int32_t)b->v, 65536) : rat_int((int32_t)b->v);
    struct rat r;

    switch (op) {
    case EM_ADD: r = rat_floor(rat_op(x, y, '+')); break;
    case EM_SUB: r = rat_floor(rat_op(x, y, '-')); break;
    case EM_MUL: r = rat_floor(rat_op(x, y, '*')); break;
    default:
        if (!rat_sign(y)) {
            int lq = a->raw || a->t == TQ, rq = b->raw || b->t == TQ;
            int neg = rat_sign(x) < 0 && (!e->cd || (rq && !lq));
            int64_t big = lq && !rq ? 32767 : 0x7fffffff;

            e->flags |= EM_F_OVF;
            r = rat_int(neg ? -big : big);
        } else
            r = rat_trunc(rat_op(x, y, '/'));
        break;
    }
    a->v = (uint32_t)(int64_t)(r.ok ? (int64_t)r.n : (int64_t)r.x);
    a->t = TL;
    a->raw = 0;
    a->id = r;
}

static int ctx_type(enum em_ctx x, int checked, uint64_t rules)
{
    static const int t[] = { TB, TW, TL, TQ, TF, TL, TW, TQ, TF, 0, TL, TW, 0 };

    if (x == EM_I2 && (checked || (rules & EM_R_1179)))
        /* arrevalchk: Long with runtime error checking; EM_R_1179: every
           index a Long ("#1179, CRC findings, trailers, merge", 2026-11-21:
           "Keine Prüfung im Release (Recommended)", direction A) */
        return TL;
    return t[x];
}

/* a candidate rule set (exprmatrix.h, EM_R_CAND): the cell as a tree */
static int cand_expression(struct ev *e, const struct em_set *s, const struct em_cell *c, int asked, struct reg *r);

/* the expression of the cell, its result in *r with its type; 0, or 1
   when the compiler refuses it */
static int expression(struct ev *e, const struct em_set *s, const struct em_cell *c, int asked, struct reg *r)
{
    int T = asked;
    struct reg b;

    /* the candidates of expr-tree-5 type numbers only: a string cell keeps
       today's rules */
    if ((e->rules & (EM_R_CANDMASK | EM_R_TSR)) && c->left != EM_S && c->right != EM_S)
        return cand_expression(e, s, c, asked, r);
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
            /* EM_R_1209: Not of the whole part of a Quick (as AND, OR
               and EOR in doop) */
            if (r->t == TQ && (e->rules & EM_R_1209))
                r->v &= 0xffff0000u;
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
                mixed(e, r, &b, c->op);
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
   enough from a step (SPPow's error is below 1e-6), or far enough outside
   the Long range for SPFix to saturate whatever SPPow's last bits are:
   $7FFFFFFF above, $80000000 below (not symmetric: -(100000^3) is
   -2147483648 as a Long and 0 as a Word, EM_R_1189) */
static int pow_whole(struct ev *e, int asked, int64_t *v)
{
    double x = e->pow.x;

    if (e->powknown && asked == TQ && (e->rules & EM_R_PTYPED) && x > 2147483648.0 * (1 + 1e-4)) {
        /* P+ types ^'s operands by their own types (a Long base is no raw
           Quick), and the Float far above the Quick range goes to a Quick by
           lib_ftoq, whose whole part and fraction both saturate: $FFFFFFFF
           whatever SPPow's last bits are (expr-tree-5 4.1, "the `^` cells",
           rounds/tree/decisions.md 2026-11-26) */
        *v = 0xFFFFFFFF;
        return 1;
    }
    if (!e->powknown || asked == TF || asked == TQ || asked == 0)
        return 0;
    if (fabs(x) > 2147483648.0 * (1 + 1e-4)) {
        *v = x < 0 ? INT32_MIN : INT32_MAX;
        return 1;
    }
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

void em_model(const struct em_matrix *m, const struct em_cell *c, uint64_t rules, int checked, struct em_value *out)
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
    out->type = r.t;
    if (e.c5ovf && (rules & EM_R_C5ERR)) {
        strcpy(out->text, "!runtime");  /* C5 with its overflow an error */
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
            /* EM_R_1187 (fixed, ~/.local/share/bb2-c-work/decisions/rounds/quick/coordinator-decisions.md 2026-11-22
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

/* ======================================================= expression trees */

/*
 * Expressions as trees (exprmatrix.h, em_expr_parse): the flow of today's
 * rules over a whole expression (flow: the type so far from left to
 * right, as expression() for one operation), and the candidate rules of
 * the decision round of expr-tree-5-operand-typing (cand: the operand
 * typing).
 */

enum { TN_VAR = 1, TN_CONST, TN_BIN, TN_NEG, TN_NOT };

enum { EM_EXPR_NODES = 256 };

struct em_tnode {
    int kind;
    enum em_op op;              /* TN_BIN */
    int t;                      /* TN_VAR: TB..TF */
    char text[40];              /* TN_VAR: its value; TN_CONST: the literal without a sign */
    struct em_tnode *l, *r;     /* TN_BIN; TN_NEG, TN_NOT: l */
};

struct em_expr {
    struct em_tnode node[EM_EXPR_NODES];
    int n;
    struct em_tnode *root;
};

struct pvar {
    char name[32];
    int t;
    char value[40];
};

struct parser {
    const char *p;
    struct em_expr *x;
    struct pvar vars[32];
    int nvars;
    char err[160];
};

static struct em_tnode *pnew(struct parser *ps, int kind)
{
    struct em_tnode *n;

    if (ps->x->n >= EM_EXPR_NODES) {
        snprintf(ps->err, sizeof ps->err, "more than %d nodes", EM_EXPR_NODES);
        return NULL;
    }
    n = &ps->x->node[ps->x->n++];
    memset(n, 0, sizeof *n);
    n->kind = kind;
    return n;
}

static void pskip(struct parser *ps)
{
    while (*ps->p == ' ' || *ps->p == '\t')
        ps->p++;
}

static int pidc(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static int ptype(char c)
{
    switch (c | 0x20) {
    case 'b': return TB;
    case 'w': return TW;
    case 'l': return TL;
    case 'q': return TQ;
    case 'f': return TF;
    default: return 0;
    }
}

/* a keyword at p (any case), not followed by a name character */
static int pkw(struct parser *ps, const char *kw)
{
    size_t n = strlen(kw);

    for (size_t i = 0; i < n; i++)
        if ((ps->p[i] | 0x20) != (kw[i] | 0x20))
            return 0;
    if (pidc(ps->p[n]))
        return 0;
    ps->p += n;
    return 1;
}

/* the binary operator at p and its precedence (blitz2.s:14704 operators);
   0 none.  peek: p stays */
static int pbinop(struct parser *ps, enum em_op *op, int peek)
{
    static const struct {
        const char *t;
        enum em_op op;
        int prec;
        int kw;
    } ops[] = {
        { "AND", EM_AND, 1000, 1 }, { "OR", EM_OR, 1000, 1 },   { "EOR", EM_EOR, 1000, 1 },
        { "MOD", EM_MOD, 5000, 1 }, { "LSL", EM_LSL, 6000, 1 }, { "ASL", EM_ASL, 6000, 1 },
        { "LSR", EM_LSR, 6000, 1 }, { "ASR", EM_ASR, 6000, 1 }, { "<>", EM_NE, 2000, 0 },
        { "><", EM_NE, 2000, 0 },   { "<=", EM_LE, 2000, 0 },   { "=<", EM_LE, 2000, 0 },
        { ">=", EM_GE, 2000, 0 },   { "=>", EM_GE, 2000, 0 },   { "=", EM_EQ, 2000, 0 },
        { "<", EM_LT, 2000, 0 },    { ">", EM_GT, 2000, 0 },    { "+", EM_ADD, 3000, 0 },
        { "-", EM_SUB, 3000, 0 },   { "*", EM_MUL, 4000, 0 },   { "/", EM_DIV, 4000, 0 },
        { "&", EM_AND, 5000, 0 },   { "|", EM_OR, 5000, 0 },    { "^", EM_POW, 7000, 0 },
    };
    const char *save;

    pskip(ps);
    save = ps->p;
    for (size_t i = 0; i < sizeof ops / sizeof ops[0]; i++) {
        size_t n = strlen(ops[i].t);
        int hit = ops[i].kw ? pkw(ps, ops[i].t) : !strncmp(ps->p, ops[i].t, n);

        if (hit) {
            if (!ops[i].kw)
                ps->p += n;
            *op = ops[i].op;
            if (peek)
                ps->p = save;
            return ops[i].prec;
        }
    }
    return 0;
}

static struct em_tnode *pexpr(struct parser *ps, int minprec);

static struct em_tnode *pnumber(struct parser *ps)
{
    struct em_tnode *n = pnew(ps, TN_CONST);
    size_t k = 0;

    if (!n)
        return NULL;
    if (*ps->p == '$' || *ps->p == '%') {
        int base = *ps->p == '$' ? 16 : 2;
        char *end;
        unsigned long v = strtoul(ps->p + 1, &end, base);
        int32_t s = (int32_t)(uint32_t)v;

        ps->p = end;
        if (s < 0) {
            /* bit 31 set: the Long it is, as a negated literal */
            struct em_tnode *m = pnew(ps, TN_NEG);

            if (!m)
                return NULL;
            snprintf(n->text, sizeof n->text, "%lld", -(long long)s);
            m->l = n;
            return m;
        }
        snprintf(n->text, sizeof n->text, "%ld", (long)s);
        return n;
    }
    if (*ps->p == '.')
        n->text[k++] = '0';
    while (((*ps->p >= '0' && *ps->p <= '9') || *ps->p == '.') && k + 1 < sizeof n->text)
        n->text[k++] = *ps->p++;
    n->text[k] = 0;
    return n;
}

static struct em_tnode *pelement(struct parser *ps)
{
    struct em_tnode *n;

    pskip(ps);
    if (*ps->p == '-') {
        ps->p++;
        n = pnew(ps, TN_NEG);
        if (!n || !(n->l = pelement(ps)))
            return NULL;
        return n;
    }
    if (pkw(ps, "NOT")) {
        n = pnew(ps, TN_NOT);
        if (!n || !(n->l = pexpr(ps, 0)))
            return NULL;
        return n;
    }
    if (*ps->p == '(') {
        ps->p++;
        n = pexpr(ps, 0);
        if (!n)
            return NULL;
        pskip(ps);
        if (*ps->p != ')') {
            snprintf(ps->err, sizeof ps->err, "')' expected at \"%.40s\"", ps->p);
            return NULL;
        }
        ps->p++;
        return n;
    }
    if ((*ps->p >= '0' && *ps->p <= '9') || *ps->p == '.' || *ps->p == '$' || *ps->p == '%')
        return pnumber(ps);
    if (pidc(*ps->p)) {
        char name[32];
        size_t k = 0;
        int t = 0;

        while (pidc(*ps->p) && k + 1 < sizeof name)
            name[k++] = *ps->p++;
        name[k] = 0;
        if (*ps->p == '.' && ptype(ps->p[1]) && !pidc(ps->p[2])) {
            t = ptype(ps->p[1]);
            ps->p += 2;
        }
        for (int i = 0; i < ps->nvars; i++)
            if (!strcmp(ps->vars[i].name, name)) {
                n = pnew(ps, TN_VAR);
                if (!n)
                    return NULL;
                n->t = t ? t : ps->vars[i].t;
                snprintf(n->text, sizeof n->text, "%s", ps->vars[i].value);
                return n;
            }
        snprintf(ps->err, sizeof ps->err, "no value for the variable %s", name);
        return NULL;
    }
    snprintf(ps->err, sizeof ps->err, "an element expected at \"%.40s\"", ps->p);
    return NULL;
}

/* precedence climbing (exprtree_climb): the operators that bind tighter
   than minprec; along a chain of one precedence from left to right */
static struct em_tnode *pexpr(struct parser *ps, int minprec)
{
    struct em_tnode *l = pelement(ps);
    enum em_op op;
    int prec;

    while (l && (prec = pbinop(ps, &op, 1)) > minprec) {
        struct em_tnode *n;

        pbinop(ps, &op, 0);
        n = pnew(ps, TN_BIN);
        if (!n)
            return NULL;
        n->op = op;
        n->l = l;
        n->r = pexpr(ps, prec);
        if (!n->r)
            return NULL;
        l = n;
    }
    return l;
}

int em_expr_parse(const char *text, const char *vars, struct em_expr **out, char **error)
{
    struct parser ps;
    struct em_expr *x = calloc(1, sizeof *x);
    const char *v = vars ? vars : "";

    *out = NULL;
    *error = NULL;
    if (!x)
        return -1;
    memset(&ps, 0, sizeof ps);
    ps.x = x;
    for (;;) {
        struct pvar *pv;
        size_t k = 0;

        while (*v == ' ' || *v == '\t')
            v++;
        if (!*v)
            break;
        if (ps.nvars >= (int)(sizeof ps.vars / sizeof ps.vars[0])) {
            snprintf(ps.err, sizeof ps.err, "too many variables");
            goto fail;
        }
        pv = &ps.vars[ps.nvars++];
        pv->t = TQ;
        while (pidc(*v) && k + 1 < sizeof pv->name)
            pv->name[k++] = *v++;
        pv->name[k] = 0;
        if (*v == '.') {
            pv->t = ptype(v[1]);
            v += 2;
        }
        if (!k || !pv->t || *v != '=') {
            snprintf(ps.err, sizeof ps.err, "NAME[.T]=VALUE expected in the values at \"%.40s\"", v);
            goto fail;
        }
        v++;
        k = 0;
        while (*v && *v != ' ' && *v != '\t' && k + 1 < sizeof pv->value)
            pv->value[k++] = *v++;
        pv->value[k] = 0;
    }
    ps.p = text;
    x->root = pexpr(&ps, 0);
    if (x->root) {
        pskip(&ps);
        if (*ps.p) {
            snprintf(ps.err, sizeof ps.err, "an operator expected at \"%.40s\"", ps.p);
            x->root = NULL;
        }
    }
    if (!x->root)
        goto fail;
    *out = x;
    return 0;
fail:
    free(x);
    *error = malloc(strlen(ps.err) + 1);
    if (!*error)
        return -1;
    strcpy(*error, ps.err);
    return 1;
}

void em_expr_free(struct em_expr *x)
{
    free(x);
}

/* ------------------------------------------------------------ the typing */

/* the typing of the operations of an expression (em_exprval) */
struct trec {
    char *sig;
    size_t cap, len;
    struct em_optype top;
    int nops;
    char *cells;                /* "OP WANT LFROM RFROM;" per operation */
    size_t ccap, clen;
    int curw;                   /* the wanted type (type so far) at the operation */
    int consumer;               /* em_exprval.consumer */
};

static char tlet(int t, int raw)
{
    if (raw)
        return '~';
    if (t < 0)
        return 'k';             /* a constant */
    return "-BWLQFAS"[t <= 7 ? t : 0];
}

static int is_consttree(const struct em_tnode *n);

static void trecord(struct trec *tr, enum em_op op, int lfrom, int las, int lraw, int rfrom, int ras, int rraw,
                    int path, int result)
{
    char b[48];
    int k;

    if (!tr || (lfrom < 0 && rfrom < 0))
        return;                 /* two constants: folded, no operation */
    tr->top.op = (int)op;
    tr->top.lfrom = lfrom;
    tr->top.las = las;
    tr->top.lraw = lraw;
    tr->top.rfrom = rfrom;
    tr->top.ras = ras;
    tr->top.rraw = rraw;
    tr->top.path = path;
    tr->top.result = result;
    tr->nops++;
    k = snprintf(b, sizeof b, "%s %c>%c %c>%c %c %c;", em_op_name(op), tlet(lfrom, 0), tlet(las, lraw),
                 tlet(rfrom, 0), tlet(ras, rraw), path, tlet(result, 0));
    if (k > 0 && tr->sig && tr->len + (size_t)k + 1 < tr->cap) {
        memcpy(tr->sig + tr->len, b, (size_t)k + 1);
        tr->len += (size_t)k;
    }
    k = snprintf(b, sizeof b, "%s %c %c %c;", em_op_name(op), tlet(tr->curw, 0), tlet(lfrom, 0), tlet(rfrom, 0));
    if (k > 0 && tr->cells && tr->clen + (size_t)k + 1 < tr->ccap) {
        memcpy(tr->cells + tr->clen, b, (size_t)k + 1);
        tr->clen += (size_t)k;
    }
}

static enum em_kind tkind(int t)
{
    switch (t) {
    case TB: return EM_B;
    case TW: return EM_W;
    case TL: return EM_L;
    case TQ: return EM_Q;
    case TF: return EM_F;
    default: return EM_S;
    }
}

/* the own type of an operand register before the operation converts it */
static int fromtype(const struct reg *r)
{
    if (r->isvar && r->conv_from)
        return r->conv_from;
    return r->raw ? TQ : r->t;
}

/* NOT on r (notit, as expression() for the unary NOT); 1 refused */
static int notop(struct ev *e, struct reg *r, int fraction)
{
    if (r->t == TQ || fraction)
        e->flags |= EM_F_BITQ;
    if (r->t >= TF)
        return 1;
    cutraw(e, r);
    if (r->t >= TL)
        r->v = ~r->v;
    else
        r->v = (r->v & 0xffff0000u) | (uint16_t)~r->v;
    if (r->t == TQ && (e->rules & EM_R_1209))
        r->v &= 0xffff0000u;
    r->id = rat_op(rat_int(-1), r->id, '-');
    return 0;
}

/* a register that is no single element any more */
static void plainreg(struct reg *r)
{
    r->isvar = 0;
    r->conv_from = r->conv_to = 0;
    r->litcut = 0;
}

static int frac_operand(const struct em_tnode *n)
{
    return (n->kind == TN_VAR && n->t >= TQ) || (n->kind == TN_CONST && strchr(n->text, '.'));
}

/* ----------------------------------------------- today's rules: the flow */

static struct rat const_ideal(const struct em_tnode *n, int modx);
static struct rat c7wrap(struct ev *e, struct rat x, int bits);
static uint32_t q16bits(struct rat x);
static int wholesub(const struct em_tnode *n);

/* EM_R_1223: a Mod of two constants with a fraction in an operand, in the
   Quick or Float T it is computed in, is the exact Mod with today's sign
   rule (a - b*trunc(a/b), plus b when negative), as with variables; r
   takes it (0: not such a Mod) */
static int modexact(struct ev *e, const struct em_tnode *n, int T, struct reg *r)
{
    struct rat x = const_ideal(n->l, 1), y = const_ideal(n->r, 1), v;

    if (!rat_sign(y) || (rat_isint(x) && rat_isint(y)))
        return 0;
    v = rat_op(x, rat_op(y, rat_trunc(rat_op(x, y, '/')), '*'), '-');
    if (rat_sign(v) < 0)
        v = rat_op(v, y, '+');
    if (rat_isint(v)) {
        /* a whole result keeps the handler's type (a Word, a Float's a Long) */
        r->v = (uint32_t)(int32_t)rat_whole(v);
        r->t = T == TQ ? TW : TL;
    } else if (T == TQ) {
        v = c7wrap(e, v, 16);
        r->v = q16bits(v);
        r->t = T;
    } else {
        r->v = ffp_of(v);
        r->t = T;
    }
    r->id = v;
    r->raw = 0;
    return 1;
}

/* EM_R_1227: in the range of a 16.16 value whose fraction is cut up
   (-32768 .. 32767) */
static int quickrange(struct rat a)
{
    return a.ok && rat_cmp(a, rat_int(-32768)) >= 0 && rat_cmp(a, rat_int(32767)) <= 0;
}

/* EM_R_1227: the operation n of a subtree of constants, its left operand
   of type left and its right one of the whole type right below it, is
   computed in left: +, -, *, / or Mod with a Float on the left, or a
   Quick and a Byte or Word (a Mod's Word), or a Quick and a Long where the
   right operand and the result are in the Quick range; not a / or Mod by
   0, which stays as before */
static int constraise(const struct em_tnode *n, int left, int right)
{
    if (!is_consttree(n) || !(n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL || n->op == EM_DIV
                              || n->op == EM_MOD)
        || !right || right >= left)
        return 0;
    if ((n->op == EM_DIV || n->op == EM_MOD) && !rat_sign(const_ideal(n->r, 1)))
        return 0;
    if (left == TF || (left == TQ && right <= TW))
        return 1;
    return left == TQ && right == TL && quickrange(const_ideal(n->r, 1)) && quickrange(const_ideal(n, 1));
}

/* the node n with the type so far *T (eval5 over the tree: the left
   operand, then the right one at the type so far after it); 0, 1 refused */
static int flow(struct ev *e, const struct em_tnode *n, int *T, struct reg *r, struct trec *tr)
{
    struct reg b;
    enum em_op op;
    int cmp, left, lfrom, rfrom, rcmp, w0;

    switch (n->kind) {
    case TN_VAR:
        fetch(e, tkind(n->t), n->text, T, r);
        return 0;
    case TN_CONST:
        fetch(e, EM_CB, n->text, T, r);
        return 0;
    case TN_NEG:
        if (n->l->kind == TN_CONST) {
            char t[48];

            snprintf(t, sizeof t, "-%s", n->l->text);
            fetch(e, EM_CB, t, T, r);
            return 0;
        }
        if (flow(e, n->l, T, r, tr) || r->t == TS)
            return 1;
        negate(r);
        plainreg(r);
        return 0;
    case TN_NOT:
        if (flow(e, n->l, T, r, tr) || notop(e, r, frac_operand(n->l)))
            return 1;
        *T = r->t;
        plainreg(r);
        return 0;
    default:
        break;
    }
    if (e->wholeconst && wholesub(n)) {
        /* EM_R_1224: in the variable form a constant of its value */
        char t[48];

        snprintf(t, sizeof t, "%lld", (long long)rat_whole(const_ideal(n, 1)));
        fetch(e, EM_CB, t, T, r);
        return 0;
    }
    op = n->op;
    w0 = *T;
    cmp = em_op_is_compare(op);
    rcmp = n->r->kind == TN_BIN && em_op_is_compare(n->r->op);
    if (flow(e, n->l, T, r, tr))
        return 1;
    lfrom = is_consttree(n->l) ? -1 : fromtype(r);  /* -1: a constant (its own type is the one it takes) */
    if (cmp && (e->rules & EM_R_1177) && n->l->kind == TN_VAR) {
        /* cmpownleft (#1177), as expression() */
        if (r->isvar && r->conv_to
            && ((r->conv_from == TQ && r->conv_to == TL) || (r->conv_from <= TL && r->conv_to == TQ)
                || (r->conv_from <= TQ && r->conv_to == TF))) {
            r->v = r->conv_v;
            e->flags = r->conv_flags;
            r->t = r->conv_from;
            r->raw = 0;
            r->id = rat_text(n->l->text);
            if (r->t == TQ)
                r->id = rat_make((int32_t)r->v, 65536);
            *T = r->t;
        }
    }
    if (cmp && (e->rules & EM_R_1177) && r->litcut && (e->rules & EM_R_1186)) {
        r->v = ffp_of(r->lit);
        r->t = *T = TF;
        r->litcut = 0;
        r->id = r->lit;
    }
    left = *T;
    if (cmp && (e->rules & EM_R_1177))
        *T = 0;
    if (flow(e, n->r, T, &b, tr))
        return 1;
    rfrom = is_consttree(n->r) ? -1 : fromtype(&b);
    if (tr)
        tr->curw = w0;          /* the type so far at the left operand (its cell) */
    if (cmp && (e->rules & EM_R_1177)) {
        int right = *T;

        if (r->raw)
            r->t = left = TQ, r->raw = 0;
        if ((left == TL && right == TQ) || (left == TQ && right == TL)) {
            struct rat x = left == TQ ? rat_make((int32_t)r->v, 65536) : rat_int((int32_t)r->v);
            struct rat y = right == TQ ? rat_make((int32_t)b.v, 65536) : rat_int((int32_t)b.v);

            scc(r, rat_cmp(x, y), op);
            trecord(tr, op, lfrom, left, 0, rfrom, right, 0, 'x', TW);
        } else {
            if (left && right < left) {
                conv(e, &b, right, left);
                right = left;
            }
            if (left)
                conv(e, r, left, right);
            if (right == TS && left != TS)
                return 1;
            doop(e, op, r, &b, right);
            trecord(tr, op, lfrom, right, 0, rfrom, right, 0, 'c', TW);
        }
    } else {
        int t2 = *T;
        int lr = r->raw, rr = b.raw;

        if (rcmp && left > t2 && t2) {
            /* #1182 (fixed): the comparison's boolean is converted up to
               the left operand's type */
            conv(e, &b, t2, left);
            t2 = left;
        } else if ((e->rules & EM_R_1227) && constraise(n, left, t2)) {
            /* EM_R_1227: a whole-number result of the right operation (a
               Mod's Word in a Quick, its Long in a Float) converted up to
               the left operand's Quick or Float, not the left operand down */
            conv(e, &b, t2, left);
            t2 = left;
        }
        if ((e->rules & EM_R_1178) && t2 == TL && (op == EM_ADD || op == EM_SUB || op == EM_MUL || op == EM_DIV)
            && (r->raw || b.raw || ((op == EM_MUL || op == EM_DIV) && left == TQ))) {
            int lq = lr || r->t == TQ, rq = rr || b.t == TQ;

            if (r->t != TQ && !r->raw && left != TL)
                conv(e, r, left, TL);
            if (b.t != TQ && !b.raw && b.t != TL)
                conv(e, &b, b.t, TL);
            trecord(tr, op, lfrom, lq ? TQ : TL, lq, rfrom, rq ? TQ : TL, rq, 'm', TL);
            mixed(e, r, &b, op);
        } else {
            if (op != EM_POW) {
                if (r->raw && t2 != TF)
                    cutraw(e, r);
                if (b.raw)
                    cutraw(e, &b);
            }
            if (left && left != t2 && !(r->raw && op == EM_POW))
                conv(e, r, left, t2);
            if (is_bitop(op) && (frac_operand(n->l) || frac_operand(n->r)))
                e->flags |= EM_F_BITQ;
            if (op == EM_MOD && (frac_operand(n->l) || frac_operand(n->r)))
                e->flags |= EM_F_MODNF;
            if (op == EM_POW && !frac_operand(n->l) && !frac_operand(n->r))
                e->flags |= EM_F_POWINT;
            if (!(op == EM_MOD && (e->rules & EM_R_1223) && (t2 == TQ || t2 == TF) && is_consttree(n)
                  && modexact(e, n, t2, r)))
                doop(e, op, r, &b, t2);
            if (e->illegal)
                return 1;
            trecord(tr, op, lfrom, t2, 0, rfrom, t2, 0, 'p', r->t);
        }
    }
    *T = r->t;
    plainreg(r);
    return 0;
}

/* ------------------------------------------------ the candidate rules (Q4) */

/*
 * The candidate rules of open question 1 of expr-tree-5-operand-typing
 * (task 2.1, ~/.local/share/bb2-c-work/expr-tree-5-rules.md): a typing
 * of each operation by its operator, the types of its two operands and
 * the wanted type W, which reaches every operand from the top (the
 * destination's or parameter's type; none for If and Print and for the
 * operands of a comparison).  Common to all four:
 *
 * - A variable or function value keeps its own type until its operation
 *   converts it; a subtree of constants only is folded into a constant.
 * - Under a wanted Float or Quick every operation is a Float or Quick one
 *   (the type so far of today's rules where it starts at the destination).
 * - Whole-number operands under a whole-number or no W: the operation in
 *   the highest of W and the two types.
 * - A Long and a Quick (no Float): computed exactly with the fraction and
 *   rounded down to a Long at the operator (#1178 variant C, mixed()).
 * - A comparison compares its two operands, each typed by its own
 *   operands, exactly (a Long and a Quick) or the lower one up; with
 *   EM_R_SCOPEB the left one only when a Quick or Float is inside it or it
 *   is a Long constant under a wanted Float, else under W.
 *
 * C1 "symmetric flow": constants take the type the flow would give them
 *   second (the higher of W and the other operand; a raw Quick under a
 *   Long leaves it Long); W reaches every Quick as today: under a wanted
 *   Long every operation with a Quick is exact (variant C), under a
 *   wanted Byte or Word a Quick operation stays Quick; Word+Word as today
 *   (add.w, then a Long).
 * C2 "own types": constants take the other operand's type (a fraction a
 *   Quick, or a Float beside one); a wanted whole number does not reach a
 *   Quick (Quick operations stay Quick, only a Long and a Quick are
 *   exact); Word+Word and Word-Word are a Word, Word*Word a Long.
 * C3 "value classes": constants by their value (Byte, Word, Long; a
 *   fraction a Quick, a Float beside a Float or under a wanted Float);
 *   under a wanted whole number an operation of a Quick and a whole number
 *   is exact, Quick with Quick stays Quick; + - * of Bytes and Words are
 *   computed in Long (no wrap).
 * C4 "integral and fractional numbers" (the user, 2026-11-24): C1 with
 *   constants as generic numbers: a whole value takes the other operand's
 *   type if it fits (beside a Quick under a wanted whole number a Long:
 *   the Quick-times-Long operation), a value with a fraction is a Quick
 *   (a Float beside a Float or under a wanted Float).
 * C5 "32.16 products" (the user, 2026-11-24, "Q*W and Q*L with a 32.16
 *   result"): C1, and a Quick times a Byte, Word or Long value is an exact
 *   32.16 product its consumer converts (EM_R_C5ERR: its overflow an error).
 * C6 (task 2.1a, the planning agent's P2): C5 with a whole constant beside
 *   a Quick in a * by its value (Byte, Word, Long: C3's rule), so q*1000
 *   is a 32.16 product too.
 * C7 (task 2.1a, the user, "C7: the wanted type sets the intermediate
 *   format"): see c7val; otherwise (no Quick, a Float) C1.
 * P+ the decided rule (task 2.2; exprmatrix.h, EM_RULES_DECIDED): C7Kx
 *   with the parts EM_R_PTYPED .. EM_R_PQDIV, marked "P" below (the
 *   analysis's changes 1-8; pconstfloat, pfloat, haslongw, c7regf and the
 *   P branches of cand, coperands, c7val and c7root).  The model stays an
 *   oracle of its own: the compiler's typing pass (exprtyping.c) and
 *   arithmetic (exprarith.c) are checked against it, not built from it.
 */

static const char *const cand_names[EM_NCAND] = { "flow", "C1", "C2", "C3", "C4", "C5", "C6", "C7" };
static const char *const cand_titles[EM_NCAND] = {
    "today's rules (the flow of the type so far)", "symmetric flow", "own types", "value classes",
    "integral and fractional numbers", "32.16 products", "32.16 products, whole constants by value",
    "the wanted type sets the intermediate format",
};

const char *em_cand_name(int n)
{
    return n >= 0 && n < EM_NCAND ? cand_names[n] : "?";
}

const char *em_cand_title(int n)
{
    return n >= 0 && n < EM_NCAND ? cand_titles[n] : "?";
}

struct cev {
    struct ev *e;
    int cand;
    int scopeb;
    struct trec *tr;
    int inmul;                  /* C6: the constant being typed is an operand of a * */
    int c7k;                    /* C7K: constants by their smallest type, comparisons by their operands */
    int cmpmode;                /* C7K: whole numbers in a comparison: 0 own, 1 at least Long, 2 the destination's */
    int rootw;                  /* the wanted type of the whole expression */
    int cmpmin;                 /* the least width of a whole-number operation here (0: none) */
    int c7kexact;               /* C7Kx: a fraction constant a Quick only if it is exact in 16.16 */
    /* the parts of the decided rule P+ (exprmatrix.h, EM_RULES_DECIDED) */
    int ptyped;                 /* EM_R_PTYPED */
    int pconst;                 /* EM_R_PCONST */
    int pmod;                   /* EM_R_PMOD */
    int pnarrow;                /* EM_R_PNARROW */
    int pdivl;                  /* EM_R_PDIVL */
    int preach;                 /* EM_R_PREACH */
    int pcmp;                   /* EM_R_PCMP */
    int pqdiv;                  /* EM_R_PQDIV */
    int pbits;                  /* EM_R_PBITS */
    int ptake;                  /* EM_R_PTAKE */
    int pmodx;                  /* EM_R_PMODX */
    int pbitk;                  /* EM_R_PBITK */
    int p48;                    /* EM_R_P48 */
    int pcmpw;                  /* EM_R_PCMPW */
    int p1221;                  /* EM_R_1221: a 48.16 * or / under a wanted Byte, Word or Long cut down */
    int p1222;                  /* EM_R_1222: the top / under a wanted Byte or Word towards zero, a
                                   constant with a point a Float under them */
    int p1223;                  /* EM_R_1223: a Mod of constants with a fraction exact */
    int p1224;                  /* EM_R_1224: a larger constant expression with such a Mod a Quick */
    const struct em_tnode *root; /* the expression's root (EM_R_1222) */
    int bittaker;               /* EM_R_PTAKE: the operand being typed is taken by a bit operator,
                                   shift or Not */
    int inbit;                  /* the constant being typed: 1 an operand of AND, OR, EOR or NOT,
                                   2 the count of a shift, 3 the shifted operand (EM_R_PBITS) */
    int regionq;                /* P: a Quick variable is in the expression or comparison side being typed */
    int fracwhole;              /* P: a whole-number subtree here is computed in the fractional format */
    int reachf;                 /* P: the expression is typed under Float (a Float in it, change 7) */
    int fmtf;                   /* P: the Float typed under is a reached format (changes 7, 8), not a wanted type */
};

static int tmax(int a, int b)
{
    return a > b ? a : b;
}

static int is_consttree(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_CONST: return 1;
    case TN_NEG: return is_consttree(n->l);
    case TN_BIN: return n->op != EM_POW && is_consttree(n->l) && is_consttree(n->r);
    default: return 0;
    }
}

static int has_qf(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_VAR: return n->t >= TQ;
    case TN_CONST: return strchr(n->text, '.') != NULL;
    case TN_BIN: return has_qf(n->l) || has_qf(n->r);
    default: return has_qf(n->l);
    }
}

/* the value fits type t as a constant of it */
static int cfits(struct rat a, int t)
{
    switch (t) {
    case TB: return rat_cmp(a, rat_int(-128)) >= 0 && rat_cmp(a, rat_int(127)) <= 0;
    case TW: return rat_cmp(a, rat_int(-32768)) >= 0 && rat_cmp(a, rat_int(32767)) <= 0;
    case TL: return rat_cmp(a, rat_int(INT32_MIN)) >= 0 && rat_cmp(a, rat_int(INT32_MAX)) <= 0;
    case TQ: return rat_cmp(a, rat_int(-32768)) >= 0 && rat_cmp(a, rat_int(32767)) <= 0;
    default: return 1;
    }
}

/* the type a constant of value a is fetched under (consttype2 then sizes
   it) beside an operand of type pt (0: none) under the wanted type W */
static int consttarget(int cand, struct rat a, int W, int pt)
{
    int whole = rat_isint(a);

    if (!pt)
        return W;
    switch (cand) {
    case EM_CAND_C1:
    case EM_CAND_C5:
        return W == TL && pt == TQ ? TL : tmax(W, pt);
    case EM_CAND_C2:
        if (whole)
            return pt == TF || cfits(a, pt) ? pt : 0;
        return pt == TF ? TF : cfits(a, TQ) ? TQ : TF;
    case EM_CAND_C3:
        if (whole)
            return 0;
        return pt == TF || W == TF ? TF : cfits(a, TQ) ? TQ : TF;
    default:                    /* C4 */
        if (whole) {
            if (pt == TQ && W >= TB && W <= TL)
                return TL;
            return pt == TF || cfits(a, pt) ? pt : 0;
        }
        return pt == TF || W == TF ? TF : cfits(a, TQ) ? TQ : TF;
    }
}

/* a subtree of constants: folded by today's rules under the type it
   takes (exprtree's fold), its value a constant of that type */
/* the exact value of a subtree of constants (a whole-number operator on
   the whole parts; a division by zero: 0); a Mod by today's sign rule, as
   the code computes it (the divisor added to a negative remainder: -6 Mod 4
   is 2), with modx (EM_R_1223) exact where an operand has a fraction; a
   comparison -1 whatever it compares, or with cmp (EM_R_PBITK: "Auch nach
   Regel falten", "expr-tree-5 group 3: three readings of the four rule
   points") its value, -1 or 0 */
static struct rat const_ideal2(const struct em_tnode *n, int cmp, int modx)
{
    struct rat a, b;

    if (n->kind == TN_CONST)
        return rat_text(n->text);
    if (n->kind == TN_NEG)
        return rat_op(rat_int(0), const_ideal2(n->l, cmp, modx), '-');
    a = const_ideal2(n->l, cmp, modx);
    b = const_ideal2(n->r, cmp, modx);
    if (cmp && em_op_is_compare(n->op)) {
        int d = rat_cmp(a, b), t;

        switch (n->op) {
        case EM_EQ: t = d == 0; break;
        case EM_NE: t = d != 0; break;
        case EM_LT: t = d < 0; break;
        case EM_LE: t = d <= 0; break;
        case EM_GT: t = d > 0; break;
        default: t = d >= 0; break;
        }
        return rat_int(t ? -1 : 0);
    }
    switch (n->op) {
    case EM_ADD: return rat_op(a, b, '+');
    case EM_SUB: return rat_op(a, b, '-');
    case EM_MUL: return rat_op(a, b, '*');
    case EM_DIV: return rat_sign(b) ? rat_op(a, b, '/') : rat_int(0);
    default: {
        int64_t x = rat_whole(rat_floor(a)), y = rat_whole(rat_floor(b));

        switch (n->op) {
        case EM_AND: return rat_int(x & y);
        case EM_OR: return rat_int(x | y);
        case EM_EOR: return rat_int(x ^ y);
        case EM_MOD:
            if (modx && rat_sign(b) && (!rat_isint(a) || !rat_isint(b))) {
                /* EM_R_1223: exact, today's sign rule (a - b*trunc(a/b), plus b when negative) */
                struct rat v = rat_op(a, rat_op(b, rat_trunc(rat_op(a, b, '/')), '*'), '-');

                return rat_sign(v) < 0 ? rat_op(v, b, '+') : v;
            }
            if (!y)
                return rat_int(0);
            /* today's sign rule, as the code (domodword, domodlong; #1228) */
            return rat_int(x % y < 0 ? x % y + y : x % y);
        case EM_LSL: case EM_ASL: return rat_int(y >= 0 && y < 32 ? (int64_t)((uint64_t)x << y) : 0);
        case EM_LSR: case EM_ASR: return rat_int(y >= 0 && y < 32 ? x >> y : 0);
        default: return rat_int(-1);    /* a comparison: true or false, whole */
        }
    }
    }
}

static struct rat const_ideal(const struct em_tnode *n, int modx)
{
    return const_ideal2(n, 0, modx);
}

static int has_qvar(const struct em_tnode *n);
static int pfloat(struct cev *c, const struct em_tnode *n, int W);
static int c7reg(struct cev *c, const struct em_tnode *n, int W, struct reg *r);
static int haslong(struct cev *c, const struct em_tnode *n);
static int haslongw(struct cev *c, const struct em_tnode *n, int W);
static int c7regf(struct cev *c, const struct em_tnode *n, int W, int frac, struct reg *r);
static struct rat c7wrap(struct ev *e, struct rat x, int bits);
static uint32_t q16bits(struct rat x);
static int cpoint(const struct em_tnode *n);

/* P, change 2 (EM_R_PCONST; "expr-tree-5 Q4 (P+ round 2)", F2): is a
   constant with a point a Float here, as in the original: under a wanted
   Float, beside a Float or beyond the Quick range always; under a wanted
   Quick never; under a wanted Long always (EM_R_1222: under a wanted
   Byte, Word or Long); else unless a Quick is beside it or in its
   expression (regionq) */
static int pconstfloat(struct cev *c, struct rat a, int W, int pt)
{
    if (W == TF || pt == TF || !cfits(a, TQ))
        return 1;
    if (W == TQ)
        return 0;
    if (W == TL || (c->p1222 && (W == TB || W == TW)))
        return 1;
    return !(pt == TQ || c->regionq);
}

/* EM_R_PBITK: a subtree of constants with AND, OR or EOR on an operand with
   a point or a fraction (a comparison in it evaluated: cideal) */
static int bitfrac(const struct cev *c, const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_BIN:
        if ((n->op == EM_AND || n->op == EM_OR || n->op == EM_EOR)
            && (cpoint(n->l) || cpoint(n->r) || !rat_isint(const_ideal(n->l, c->p1223)) || !rat_isint(const_ideal(n->r, c->p1223))))
            return 1;
        return bitfrac(c, n->l) || bitfrac(c, n->r);
    case TN_NEG: return bitfrac(c, n->l);
    default: return 0;
    }
}

/* the exact value of a constant subtree under the rules of c (EM_R_PBITK:
   its comparisons evaluated) */
static struct rat cideal(const struct cev *c, const struct em_tnode *n)
{
    return const_ideal2(n, c->pbitk, c->p1223);
}

/* EM_R_1223: a Mod of two constants with a fraction or a point in an
   operand is a Quick under a whole destination, rounded down as the
   variable form (the user's decision of 2026-11-26, "B: like the variable
   form"), not the Float of a constant with a point (#1222) */
static int constmodq(const struct em_tnode *n)
{
    struct rat a, b;

    if (n->kind == TN_NEG)
        return constmodq(n->l);
    if (n->kind != TN_BIN || n->op != EM_MOD)
        return 0;
    a = const_ideal(n->l, 1);
    b = const_ideal(n->r, 1);
    return a.ok && b.ok && rat_sign(b) && (cpoint(n) || !rat_isint(a) || !rat_isint(b));
}

static int hasdiv(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_BIN: return n->op == EM_DIV || hasdiv(n->l) || hasdiv(n->r);
    case TN_NEG:
    case TN_NOT: return hasdiv(n->l);
    default: return 0;
    }
}

/* EM_R_1224: an operation of constants without a point and without a /
   whose value is whole (7 Mod 3, 2*3): in the variable form a constant of
   its value beside the Quick */
static int wholesub(const struct em_tnode *n)
{
    struct rat a;

    if (n->kind != TN_BIN || !is_consttree(n) || cpoint(n) || hasdiv(n))
        return 0;
    a = const_ideal(n, 1);
    return a.ok && rat_isint(a);
}

/* EM_R_1224: n is +, - or * (or a negation) over whole constants without
   a point, wholesub operations of them and Mods of constmodq, one of them
   at least (*mod) */
static int constmodtree(const struct em_tnode *n, int *mod)
{
    switch (n->kind) {
    case TN_CONST: return !cpoint(n);
    case TN_NEG: return constmodtree(n->l, mod);
    case TN_BIN:
        if (constmodq(n))
            return *mod = 1;
        if (wholesub(n))
            return 1;
        return (n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL) && constmodtree(n->l, mod)
               && constmodtree(n->r, mod);
    default: return 0;
    }
}

/* the constant subtree n is a Quick under a whole destination, rounded
   down as its variable form (each Mod of constmodq one of Quick
   variables), an expression of a Quick without a Float: EM_R_1223 a Mod of
   constmodq, EM_R_1224 a larger expression of constmodtree */
static int constquick(const struct cev *c, const struct em_tnode *n)
{
    int mod = 0;

    if (constmodq(n))
        return 1;
    return c->p1224 && constmodtree(n, &mod) && mod;
}

/* a subtree of constants: one constant of its exact value, of the type the
   rule gives it (exprtree's fold; alone, or a fraction folded under a
   whole number, by today's fold in that type) */
static int cconst(struct cev *c, const struct em_tnode *n, int W, int pt, struct reg *r)
{
    int mq = c->p1223 && W >= TB && W <= TL && constquick(c, n);

    if (mq)
        W = TQ;
    struct rat a = cideal(c, n);
    int cn = c->cand, T, tt;

    /* C6: a whole constant beside a Quick in a * by its value (C3's rule),
       else C1's */
    if (c->c7k)
        cn = EM_CAND_C3;        /* C7K: a whole constant by its smallest type, a fraction a Quick */
    else if (cn == EM_CAND_C6)
        cn = c->inmul && pt == TQ && rat_isint(a) ? EM_CAND_C3 : EM_CAND_C1;
    tt = consttarget(cn, a, W, pt);
    /* C7Kx: a fraction that a Quick cannot hold exactly (0.1, 0.001) stays a
       Float beside a whole number, as today; beside a Quick a Quick */
    if (c->c7kexact && tt == TQ && pt && pt != TQ && !rat_isint(a) && !rat_isint(rat_op(a, rat_int(65536), '*')))
        tt = TF;
    if (c->pconst && pt && a.ok && (!rat_isint(a) || cpoint(n)))
        tt = pconstfloat(c, a, W, pt) ? TF : TQ;
    /* EM_R_PBITS ("expr-tree-5: bit operators and shifts with fractions;
       Quick /0; Mod /0", 2026-11-25): a constant with a point or a
       fraction as an operand of AND, OR, EOR or NOT is its whole part,
       rounded down (as #1209 takes a Quick), a whole number of its
       smallest type: b AND 1.5 = 1.5 AND b = b AND 1; as the count of a
       shift it is refused ("Immer ablehnen") */
    int bitwhole = 0, inbit = c->inbit;

    c->inbit = 0;
    if (c->pbits && inbit == 3) {
        /* the shifted operand ("expr-tree-5: division by 0 per format, q
           Mod 0.5, fraction left of a shift", 2026-11-25: "Wie eine
           Quick-Variable"): a constant with a point is a Quick, its 16.16
           value shifted (1.5 LSL 2 = 6; under a Long rounded down first,
           F7) */
        if (a.ok && (cpoint(n) || !rat_isint(a)) && cfits(a, TQ))
            tt = TQ;
    } else if (c->pbits && inbit && a.ok && (cpoint(n) || !rat_isint(a))) {
        if (inbit == 2) {
            c->e->illegal = 1;
            return 1;
        }
        a = rat_floor(a);
        if (cfits(a, TL)) {
            tt = cfits(a, TB) ? TB : cfits(a, TW) ? TW : TL;
            bitwhole = 1;
        }
    }
    if (c->pbitk && !bitwhole && inbit != 3 && bitfrac(c, n)) {
        /* EM_R_PBITK ("Wie mit Variable", "expr-tree-5 group 3 findings:
           four rule points of P+"): a bit operator of constants with a
           fraction folds as b AND 1.5 takes it, the fraction cut (the
           whole parts in const_ideal); refused under a wanted Float as the
           bit operator with a variable is */
        if (W == TF && !c->fmtf) {
            c->e->illegal = 1;
            return 1;
        }
        a = cideal(c, n);
        tt = !pt ? W : rat_isint(a) ? 0 : pconstfloat(c, a, W, pt) ? TF : TQ;
        bitwhole = 1;
    }
    char text[48];

    T = tt;
    if (!bitwhole && (n->kind == TN_CONST || !pt || !a.ok || (!rat_isint(a) && tt != TQ && tt != TF))) {
        int k;

        c->e->wholeconst = mq && c->p1224;
        k = flow(c->e, n, &T, r, NULL);
        c->e->wholeconst = 0;
        if (k)
            return 1;
    } else {
        int neg = rat_sign(a) < 0;
        struct rat m = neg ? rat_op(rat_int(0), a, '-') : a;

        if (rat_isint(m))
            snprintf(text, sizeof text, "%lld", (long long)rat_whole(m));
        else {
            /* the fraction in six decimals (fetchpoint2 reads them) */
            struct rat f = rat_op(m, rat_floor(m), '-');
            long long d = (long long)rat_whole(rat_floor(rat_op(f, rat_int(1000000), '*')));

            snprintf(text, sizeof text, "%lld.%06lld", (long long)rat_whole(rat_floor(m)), d);
        }
        memset(r, 0, sizeof *r);
        constant(c->e, text, &T, r);
        if (neg)
            negate(r);
    }
    plainreg(r);
    r->isconst = 1;
    return 0;
}

static int cand(struct cev *c, const struct em_tnode *n, int W, int pt, struct reg *r);

/* EM_R_PBITS: a subtree of constants with a shift of a constant with a
   point: not folded as one constant, the shifted constant is a Quick */
static int shiftq(const struct cev *c, const struct em_tnode *n)
{
    if (!c->pbits)
        return 0;
    switch (n->kind) {
    case TN_BIN:
        if (n->op >= EM_LSL && n->op <= EM_ASR && cpoint(n->l))
            return 1;
        return shiftq(c, n->l) || shiftq(c, n->r);
    case TN_NEG:
    case TN_NOT: return shiftq(c, n->l);
    default: return 0;
    }
}

/* C7K: the least width of a whole-number operation in the operands of a
   comparison (with no wanted type there): none, a Long, or the
   destination's whole-number width (a Quick or Float destination, or none:
   none) */
static int c7kmin(const struct cev *c)
{
    if (!c->c7k || !c->cmpmode)
        return 0;
    if (c->cmpmode == 1)
        return TL;
    return c->rootw >= TB && c->rootw <= TL ? c->rootw : 0;
}

/* a 32.16 product (C5, C6) taken as the Quick or Long it is: its overflow */
static void wideok(struct ev *e, struct reg *r)
{
    if (r->wide && r->wideovf) {
        e->c5ovf = 1;
        e->flags |= EM_F_OVF;
    }
    r->wide = 0;
}

/* the operands of n: a constant one after the other, which it is typed by */
static int coperands(struct cev *c, const struct em_tnode *n, int WL, int WR, struct reg *r, struct reg *b)
{
    int lc = is_consttree(n->l), rc = is_consttree(n->r);
    int shift = n->op >= EM_LSL && n->op <= EM_ASR, bit = is_bitop(n->op) && !shift;

    c->inbit = 0;
    c->bittaker = 0;           /* EM_R_PTAKE: set for the operands of a bit operator or shift */
    if (em_op_is_compare(n->op) && (c->ptyped || c->pconst)) {
        int save = c->regionq, k;

        if (lc && !rc) {
            c->regionq = has_qvar(n->r);
            k = cand(c, n->r, WR, 0, b);
            if (!k) {
                c->regionq = 0;
                c->bittaker = 0;
                k = cand(c, n->l, WL, b->t, r);
            }
        } else {
            c->regionq = has_qvar(n->l);
            k = cand(c, n->l, WL, 0, r);
            if (!k) {
                c->regionq = has_qvar(n->r);
                c->bittaker = 0;
                k = cand(c, n->r, WR, rc ? r->t : 0, b);
            }
        }
        c->regionq = save;
        return k;
    }
    if (lc && !rc) {
        c->bittaker = is_bitop(n->op);
        if (cand(c, n->r, WR, 0, b))
            return 1;
        /* a shifted constant is not typed by the count */
        c->inmul = n->op == EM_MUL;
        c->inbit = shift ? 3 : bit;     /* a shifted constant: EM_R_PBITS */
        c->bittaker = is_bitop(n->op);
        return cand(c, n->l, WL, n->op >= EM_LSL && n->op <= EM_ASR ? 0 : b->t, r);
    }
    c->inbit = shift && lc ? 3 : 0;
    c->bittaker = is_bitop(n->op);
    if (cand(c, n->l, WL, 0, r))
        return 1;
    c->inmul = n->op == EM_MUL;
    c->inbit = rc ? (shift ? 2 : bit) : 0;
    c->bittaker = is_bitop(n->op);
    return cand(c, n->r, WR, rc ? r->t : 0, b);
}

static int cmixed(int cand, int W, int lt, int rt)
{
    int wl = W >= TB && W <= TL, lng = lt == TL || rt == TL;

    switch (cand) {
    case EM_CAND_C1:
    case EM_CAND_C4:
    case EM_CAND_C5:
    case EM_CAND_C6: return W == TL || lng;
    case EM_CAND_C2: return lng;
    default: return (wl && !(lt == TQ && rt == TQ)) || lng;
    }
}

static int cand(struct cev *c, const struct em_tnode *n, int W, int pt, struct reg *r)
{
    struct ev *e = c->e;
    struct reg b;
    enum em_op op;
    int lt, rt, T, lfrom, rfrom;

    if (is_consttree(n) && !shiftq(c, n))
        return cconst(c, n, W, pt, r);
    if (W == TF && c->fmtf
        && (n->kind == TN_NOT
            || (n->kind == TN_BIN && (is_bitop(n->op) || (n->op >= EM_LSL && n->op <= EM_ASR))))) {
        /* P (changes 5-8): a Float format reached from a comparison or an
           expression with a Float is no wanted Float for a bit operator,
           shift or Not: its operands are typed by their own (rounded down
           where it takes them, as today) and its whole result is taken
           into the Float format; a wanted Float (rf.f=b AND 1) stays
           refused */
        int save = c->fmtf, k;

        c->fmtf = 0;
        k = cand(c, n, 0, pt, r);
        c->fmtf = save;
        if (k || r->t == TS)
            return 1;
        wideok(e, r);
        if (r->raw)
            cutraw(e, r);
        if (r->t != TF)
            conv(e, r, r->t, TF);
        plainreg(r);
        return 0;
    }
    if (c->ptyped && c->cand == EM_CAND_C1 && c->reachf && W == TF && has_qvar(n)
        && (n->kind == TN_NEG
            || (n->kind == TN_BIN
                && (em_op_is_compare(n->op) || n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL
                    || n->op == EM_DIV || n->op == EM_MOD)))
        && !pfloat(c, n, 0))
        return c7regf(c, n, 0, 1, r);
    if (c->ptyped && c->cand == EM_CAND_C1 && W != TF && W != TS && has_qvar(n)
        && (n->kind == TN_NEG
            || (n->kind == TN_BIN
                && (em_op_is_compare(n->op) || n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL
                    || n->op == EM_DIV || n->op == EM_MOD)))
        && !pfloat(c, n, W))
        return c7reg(c, n, W, r);
    switch (n->kind) {
    case TN_VAR:
        T = 0;
        fetch(e, tkind(n->t), n->text, &T, r);
        return 0;
    case TN_NEG:
        if (cand(c, n->l, W, pt, r) || r->t == TS)
            return 1;
        wideok(e, r);
        negate(r);
        plainreg(r);
        return 0;
    case TN_NOT:
        c->inbit = is_consttree(n->l);
        c->bittaker = 1;
        if (cand(c, n->l, W, 0, r))
            return 1;
        wideok(e, r);
        /* NOT in the higher of W and its operand's type (refused as a
           Float); a Quick under a wanted Long rounded down, as today */
        if (r->t == TQ && W == TL && c->cand != EM_CAND_C2)
            conv(e, r, TQ, TL);
        else if (W > r->t && W != TS)
            conv(e, r, r->t, W);
        if (notop(e, r, frac_operand(n->l)))
            return 1;
        plainreg(r);
        return 0;
    default:
        break;
    }
    op = n->op;
    if (em_op_is_compare(op)) {
        int WL = 0;

        if (c->scopeb && !has_qf(n->l)
            && !(is_consttree(n->l) && W == TF && n->l->kind == TN_CONST && !cfits(rat_text(n->l->text), TW)))
            WL = W;
        {
            int save = c->cmpmin, k;

            c->cmpmin = c7kmin(c);
            if (c->pcmp && pfloat(c, n, 0)) {
                /* P: the comparison's common format is Float: it reaches both operands */
                const struct em_tnode *o[2] = { n->l, n->r };
                struct reg *rr[2] = { r, &b };
                int i;

                k = 0;
                for (i = 0; i < 2 && !k; i++) {
                    int sq = c->regionq;

                    c->regionq = has_qvar(o[i]);
                    c->bittaker = 0;
                    if (!is_consttree(o[i]) && has_qvar(o[i]) && !pfloat(c, o[i], 0))
                        k = c7regf(c, o[i], 0, 1, rr[i]);
                    else {
                        int sf = c->fmtf;

                        c->fmtf = 1;
                        k = cand(c, o[i], TF, is_consttree(o[i]) ? TF : 0, rr[i]);
                        c->fmtf = sf;
                    }
                    c->regionq = sq;
                }
            } else
                k = coperands(c, n, WL, 0, r, &b);
            c->cmpmin = save;
            if (k)
                return 1;
        }
        if (c->tr)
            c->tr->curw = W;    /* the wanted type at the comparison (its cell) */
        lt = r->t;
        rt = b.t;
        if (lt == TS || rt == TS)
            return 1;
        if (r->wide || b.wide) {
            /* C5, C6: the 32.16 product compared exactly */
            struct rat x = r->wide ? r->wid : reg_value(r), y = b.wide ? b.wid : reg_value(&b);

            if (lt == TF || rt == TF) {
                wideok(e, r);
                wideok(e, &b);
            } else {
                trecord(c->tr, op, is_consttree(n->l) ? -1 : lt, lt, 0, is_consttree(n->r) ? -1 : rt, rt, 0, 'x',
                        TW);
                scc(r, rat_cmp(x, y), op);
                r->wide = 0;
                plainreg(r);
                return 0;
            }
        }
        if ((lt == TL && rt == TQ) || (lt == TQ && rt == TL)) {
            struct rat x = reg_value(r), y = reg_value(&b);

            trecord(c->tr, op, is_consttree(n->l) ? -1 : lt, lt, 0, is_consttree(n->r) ? -1 : rt, rt, 0, 'x', TW);
            scc(r, rat_cmp(x, y), op);
        } else {
            T = tmax(lt, rt);
            conv(e, r, lt, T);
            conv(e, &b, rt, T);
            doop(e, op, r, &b, T);
            trecord(c->tr, op, is_consttree(n->l) ? -1 : lt, T, 0, is_consttree(n->r) ? -1 : rt, T, 0, 'c', TW);
        }
        plainreg(r);
        return 0;
    }
    if (coperands(c, n, W, W, r, &b))
        return 1;
    wideok(e, r);
    wideok(e, &b);
    if (c->tr)
        c->tr->curw = W;        /* the wanted type at the operation (its cell) */
    lt = r->t;
    lfrom = is_consttree(n->l) ? -1 : lt;
    rt = b.t;
    rfrom = is_consttree(n->r) ? -1 : rt;
    if (lt == TS || rt == TS)
        return 1;               /* strings are not typed by the candidates */
    /* a constant too large for a Quick under a wanted Quick is a Long, as
       today, and the operation then one under a Long */
    if (W == TQ
        && ((is_consttree(n->l) && !cfits(cideal(c, n->l), TQ))
            || (is_consttree(n->r) && !cfits(cideal(c, n->r), TQ))))
        W = TL;
    if ((c->cand == EM_CAND_C5 || c->cand == EM_CAND_C6) && op == EM_MUL && W != TF && lt != TF && rt != TF && (lt == TQ) != (rt == TQ)) {
        /* C5 ("Q*W and Q*L with a 32.16 result", 2026-11-24): the exact
           product of a Quick and a whole number as a 32.16 value (48.16
           for a Long: the overflow of its 32-bit whole part is seen), taken
           by its consumer: a wanted whole number its whole part (rounded
           down; a Word its low word), a wanted Quick or none 16.16 (none
           with a Long operand: the whole part, a Long) */
        struct rat x = reg_value(r), y = reg_value(&b), p = rat_op(x, y, '*');
        int ct = W >= TB && W <= TL ? TL : W == TQ ? TQ : (lt == TL || rt == TL) ? TL : TQ;
        int ovf;

        if (ct == TL) {
            struct rat f = rat_floor(p);

            ovf = !cfits(f, TL) || (W >= TB && W <= TW && !cfits(f, W));
            r->v = (uint32_t)(int64_t)rat_whole(f);
            r->id = f;
        } else {
            struct rat q = rat_q16(p);

            ovf = !fits(p, TQ);
            r->v = (uint32_t)(int64_t)rat_whole(rat_trunc(rat_op(p, rat_int(65536), '*')));
            r->id = q;
        }
        r->t = ct;
        r->raw = 0;
        trecord(c->tr, op, lfrom, lt, lt == TQ, rfrom, rt, rt == TQ, 'w', ct);
        plainreg(r);
        if (!W && cfits(rat_floor(p), TL)) {
            /* no wanted type: a comparison compares the 32.16 value exactly
               (its consumer); anywhere else it is the Quick or Long (wideok) */
            r->wide = 1;
            r->wid = p;
            r->wideovf = ovf;
        } else if (ovf) {
            e->c5ovf = 1;
            e->flags |= EM_F_OVF;
        }
        return 0;
    }
    if (op == EM_ADD || op == EM_SUB || op == EM_MUL || op == EM_DIV) {
        if (W == TF || lt == TF || rt == TF)
            T = TF;
        else if (W == TQ)
            T = TQ;
        else if (lt == TQ || rt == TQ) {
            if (cmixed(c->cand, W, lt, rt)) {
                if (lt != TQ && lt != TL)
                    conv(e, r, lt, TL);
                if (rt != TQ && rt != TL)
                    conv(e, &b, rt, TL);
                trecord(c->tr, op, lfrom, lt == TQ ? TQ : TL, lt == TQ, rfrom, rt == TQ ? TQ : TL, rt == TQ, 'm',
                        TL);
                mixed(e, r, &b, op);
                plainreg(r);
                return 0;
            }
            T = TQ;
        } else {
            T = tmax(W, tmax(lt, rt));
            if (c->cand == EM_CAND_C3 && T <= TW && (op == EM_ADD || op == EM_SUB))
                T = TL;
            if (c->cmpmin > T)
                T = c->cmpmin;  /* C7K: a whole-number operation in a comparison */
        }
    } else if (op == EM_POW) {
        /* dopow: both to Float, through a higher wanted Quick or whole
           number first as today */
        int tt = tmax(W, tmax(lt, rt));

        if (tt == TQ && W != TQ && (lt == TL || rt == TL))
            tt = TF;            /* a Long and a Quick: both straight to Float, as a raw Quick */
        if (tt > lt) {
            conv(e, r, lt, tt);
            lt = tt;
        }
        if (tt > rt) {
            conv(e, &b, rt, tt);
            rt = tt;
        }
        T = TF;
    } else {
        /* the bit operators, Mod and the shifts (a Quick count makes the
           shift a Quick one, as today) */
        int rr = rt;

        if (op >= EM_LSL && op <= EM_ASR) {
            /* the count raises a Byte or Word to a Quick, as today; under a
               Long it is rounded down */
            rr = rt == TF ? TF : rt == TQ && tmax(W, lt) < TL ? TQ : 0;
        }
        if (W == TF)
            T = TF;             /* refused but for Mod, as today */
        else if (W != TQ && !(op >= EM_LSL && op <= EM_ASR) && ((lt == TL && rr == TQ) || (lt == TQ && rr == TL)))
            T = TL;             /* the Quick rounded down, as a Long and a Quick add */
        else
            T = tmax(W, tmax(lt, rr));
        if (T == TQ && W == TL && c->cand != EM_CAND_C2)
            T = TL;             /* the Quick rounded down, as under today's Long */
        if (is_bitop(op) && (frac_operand(n->l) || frac_operand(n->r) || lt >= TQ || rt >= TQ))
            e->flags |= EM_F_BITQ;
        if (op == EM_MOD && (frac_operand(n->l) || frac_operand(n->r)))
            e->flags |= EM_F_MODNF;
    }
    if (op == EM_POW && !frac_operand(n->l) && !frac_operand(n->r))
        e->flags |= EM_F_POWINT;
    /* both in T (^: dopow takes them as Floats) */
    conv(e, r, lt, T);
    conv(e, &b, rt, T);
    if (c->pmodx && op == EM_MOD && (T == TQ || T == TF)
        && ((lt >= TQ && lt <= TF) || (rt >= TQ && rt <= TF))) {
        /* EM_R_PMODX ("Exakt bei jedem Bruch", "expr-tree-5 group 3
           findings: four rule points of P+"): an operand with a fraction
           makes Mod exact with today's sign rule, in the Quick or Float it
           is computed in; a divisor 0 as today per type (a Quick traps, a
           Float gives the dividend) */
        struct rat x = reg_value(r), y = reg_value(&b), v;

        if (!rat_sign(y)) {
            e->flags |= EM_F_OVF;
            if (T == TQ)
                e->divtrap = 1;
            v = x;
        } else
            v = rat_op(x, rat_op(y, rat_trunc(rat_op(x, y, '/')), '*'), '-');
        if (rat_sign(v) < 0)
            v = rat_op(v, y, '+');
        if (T == TQ) {
            v = c7wrap(e, v, 16);
            r->v = q16bits(v);
        } else
            r->v = ffp_of(v);
        r->id = v;
        r->t = T;
        r->raw = 0;
    } else
        doop(e, op, r, &b, T);
    if (e->illegal)
        return 1;
    if (c->cand == EM_CAND_C2 && (op == EM_ADD || op == EM_SUB) && T == TW)
        r->t = TW;              /* add.w: the Word sum stays a Word */
    trecord(c->tr, op, lfrom, op == EM_POW ? lt : T, 0, rfrom, op == EM_POW ? rt : T, 0, 'p', r->t);
    plainreg(r);
    return 0;
}

/* ------------------------------------------------ C7: the wanted type sets the format */

/*
 * C7 ("C7: the wanted type sets the intermediate format", 2026-11-24): in
 * an expression with a Quick the wanted type sets the format of every
 * intermediate result, rounded once at the consumer: a wanted Long 32.16
 * (48.16 in a register pair); a wanted Quick, Word or Byte 16.16, but the
 * subtree of a / or Mod with a Long in it 32.16; no wanted type (If,
 * Print, the operands of a comparison) 32.16 where a Quick or Quick
 * subtree meets a whole value or constant, 16.16 for Quick with Quick.
 * An expression without a Quick, and one with a Float or under a wanted
 * Float, is today's (the flow).  * and / are cut to 1/65536 towards zero
 * (as qmul and qdivide), Mod is a - b*trunc(a/b); a result beyond the
 * format wraps (EM_F_OVF).  The consumer: a whole number rounded down (a
 * / at the top towards zero, "#1178 rounding rule decided"), a Quick the
 * 16.16 value, no wanted type the Quick when it fits, else the whole part.
 */

static int has_qvar(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_VAR: return n->t == TQ;
    case TN_CONST: return 0;
    case TN_BIN: return has_qvar(n->l) || has_qvar(n->r);
    default: return has_qvar(n->l);
    }
}

static int has_pow(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_VAR:
    case TN_CONST: return 0;
    case TN_BIN: return n->op == EM_POW || has_pow(n->l) || has_pow(n->r);
    default: return has_pow(n->l);
    }
}

static int has_t(const struct em_tnode *n, int t)
{
    switch (n->kind) {
    case TN_VAR: return n->t == t;
    case TN_CONST: return 0;    /* a constant with a fraction is exact in the format, no Float */
    case TN_BIN: return has_t(n->l, t) || has_t(n->r, t);
    default: return has_t(n->l, t);
    }
}

/* x wrapped into a format of whole part bits wide (16: 16.16, 32: 32.16) */
static struct rat c7wrap(struct ev *e, struct rat x, int bits)
{
    struct rat w = rat_floor(x);
    __int128 lim = (__int128)1 << (bits - 1);

    if (!x.ok)
        return x;
    if (w.n >= lim || w.n < -lim) {
        __int128 m = (__int128)1 << bits, k = (w.n + lim) / m;

        if (w.n + lim < 0 && (w.n + lim) % m)
            k--;
        x = rat_op(x, rat_make(k * m, 1), '-');
        e->flags |= EM_F_OVF;
    }
    return x;
}

/* the whole-part bits of the format f of c7val (16: 16.16; 32: the wide
   format, 32.16, with EM_R_P48 48.16 as design decision 9: "Modell auf 48
   Bit", "expr-tree-5 group 3 findings: four rule points of P+") */
static int c7bits(const struct cev *c, int f)
{
    return f == 32 && c->p48 ? 48 : f;
}

/* the bits of a 16.16 value x (on the 1/65536 grid, in the format) */
static uint32_t q16bits(struct rat x)
{
    return (uint32_t)(int32_t)rat_whole(rat_floor(rat_op(x, rat_int(65536), '*')));
}

static int c7val(struct cev *c, const struct em_tnode *n, int W, int force, struct rat *out, int *fmt);
static uint32_t pc_float(const char *text);

/* a subtree without a Quick: its value by C1 (a whole number, a constant) */
static int c7whole(struct cev *c, const struct em_tnode *n, int W, struct rat *out, int *t)
{
    struct reg r;
    int save = c->cand;

    if (is_consttree(n) && !shiftq(c, n)) {
        *out = cideal(c, n);
        if (!rat_isint(*out) && n->kind == TN_CONST && cfits(*out, TQ))
            /* a literal with a point read as a Quick: the FFP value of its
               text cut down, as both compilers read it (the emulator, #1232:
               x.q=q+30000.3 $75314D00) */
            *out = rat_q16dn(ffp_rat(pc_float(n->text)));
        else if (!rat_isint(*out))
            *out = rat_q16(*out);       /* a fraction on the 1/65536 grid of the format */
        *t = -1;
        return 0;
    }
    c->cand = EM_CAND_C1;
    if (cand(c, n, W, 0, &r)) {
        c->cand = save;
        return 1;
    }
    c->cand = save;
    wideok(c->e, &r);
    *out = reg_value(&r);
    *t = r.t;
    return 0;
}

/* NOT, the bit operators, the shifts and ^ with a Quick inside: C1's (today's
   rules on its operands), the whole subtree */
static int c7other(struct cev *c, const struct em_tnode *n, int W, struct rat *out, int *fmt)
{
    struct reg r;
    int save = c->cand, sf = c->fracwhole;

    c->cand = EM_CAND_C1;
    c->fracwhole = 0;
    if (cand(c, n, W, 0, &r)) {
        c->cand = save;
        c->fracwhole = sf;
        return 1;
    }
    c->cand = save;
    c->fracwhole = sf;
    wideok(c->e, &r);
    if (r.raw)
        cutraw(c->e, &r);
    *out = reg_value(&r);
    *fmt = r.t == TQ ? 16 : r.t == TF ? 32 : 0;
    return 0;
}

static int c7side(struct cev *c, const struct em_tnode *n, int W, int force, struct rat *out, int *fmt, int *from)
{
    if (has_qvar(n)
        || (c->fracwhole && !is_consttree(n)
            && (n->kind == TN_NEG
                || (n->kind == TN_BIN
                    && (n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL || n->op == EM_DIV || n->op == EM_MOD))))) {
        *from = TQ;
        return c7val(c, n, W, force, out, fmt);
    }
    *fmt = 0;
    return c7whole(c, n, W, out, from);
}

static int c7val(struct cev *c, const struct em_tnode *n, int W, int force, struct rat *out, int *fmt)
{
    struct ev *e = c->e;
    struct rat x, y, v;
    int fl, fr, lf, rf, f;
    enum em_op op;

    switch (n->kind) {
    case TN_VAR:
        *out = rat_make((int32_t)var_bits(TQ, n->text), 65536);
        *fmt = 16;
        return 0;
    case TN_NEG:
        if (c->ptyped ? c7side(c, n->l, W, force, &x, fmt, &lf) : c7val(c, n->l, W, force, &x, fmt))
            return 1;
        *out = c7wrap(e, rat_op(rat_int(0), x, '-'), *fmt ? c7bits(c, *fmt) : 32);
        return 0;
    case TN_NOT:
        return c7other(c, n, W, out, fmt);
    default:
        break;
    }
    op = n->op;
    if (!em_op_is_compare(op) && op != EM_ADD && op != EM_SUB && op != EM_MUL && op != EM_DIV && op != EM_MOD)
        return c7other(c, n, W, out, fmt);
    if (em_op_is_compare(op)) {
        /* both operands with no wanted type; compared exactly */
        int save = c->cmpmin, k;

        c->cmpmin = c7kmin(c);
        {
            int sq = c->regionq, fc = c->pnarrow ? (haslong(c, n->l) || haslong(c, n->r)) : 0, sf = c->fracwhole;

            c->fracwhole = c->pcmp;
            c->regionq = has_qvar(n->l);
            k = c7side(c, n->l, 0, fc, &x, &fl, &lf);
            c->regionq = has_qvar(n->r);
            k = k || c7side(c, n->r, 0, fc, &y, &fr, &rf);
            c->regionq = sq;
            c->fracwhole = sf;
        }
        c->cmpmin = save;
        if (k)
            return 1;
        trecord(c->tr, op, is_consttree(n->l) ? -1 : lf, lf, fl == 32, is_consttree(n->r) ? -1 : rf, rf, fr == 32,
                'x', TW);
        {
            int d = rat_cmp(x, y), t;

            switch (op) {
            case EM_EQ: t = d == 0; break;
            case EM_NE: t = d != 0; break;
            case EM_LT: t = d < 0; break;
            case EM_LE: t = d <= 0; break;
            case EM_GT: t = d > 0; break;
            default: t = d >= 0; break;
            }
            *out = rat_int(t ? -1 : 0);
        }
        *fmt = 0;
        return 0;
    }
    if (W >= TB && W <= TQ && (op == EM_DIV || op == EM_MOD || c->c7kexact) && (c->ptyped ? haslongw(c, n, W) : has_t(n, TL)))
        force = 1;              /* C7Kx: every operation with a Long in it in 32.16 */
    if (c7side(c, n->l, W, force, &x, &fl, &lf) || c7side(c, n->r, W, force, &y, &fr, &rf))
        return 1;
    if (force || W == TL)
        f = 32;
    else if (W >= TB && W <= TQ)
        f = 16;
    else if (c->pnarrow)
        f = 16;                 /* P: no wanted type and no Long in the expression: 16.16 as the original */
    else if (c->ptyped && !fl && !fr) {
        /* P: two whole numbers (a boolean and a whole value): Long with a Long */
        int ll = lf == TL || (lf == -1 && !cfits(x, TW)), rl = rf == TL || (rf == -1 && !cfits(y, TW));

        f = ll || rl ? 32 : 16;
    } else
        f = fl == 32 || fr == 32 || (fl && !fr) || (!fl && fr) ? 32 : 16;
    /* both operands in the format (a Long beyond it cut into a 16.16) */
    x = c7wrap(e, x, c7bits(c, f));
    y = c7wrap(e, y, c7bits(c, f));
    switch (op) {
    case EM_ADD: v = rat_op(x, y, '+'); break;
    case EM_SUB: v = rat_op(x, y, '-'); break;
    case EM_MUL:
        /* EM_R_1221: the cut at 1/65536 in the direction of the final
           rounding, down under a wanted whole number */
        v = c->p1221 && f == 32 && W >= TB && W <= TL ? rat_q16dn(rat_op(x, y, '*')) : rat_q16(rat_op(x, y, '*'));
        break;
    case EM_DIV:
        if (c->p1222 && f == 16 && n == c->root && (W == TB || W == TW) && rat_sign(y)
            && (q16bits(y) & 0xffff)) {
            /* EM_R_1222: at the top a divisor with a fraction, the exact
               quotient towards zero (the code's Long division of the 16.16
               bits; beyond the 16.16 range its low bits) */
            v = rat_trunc(rat_op(x, y, '/'));
        } else if (c->pqdiv && f == 16) {
            /* EM_R_PQDIV: qdivide as it runs, its last bit (up to 1/65536
               above the quotient towards zero) and its $7FFFFFFF for a
               divisor 0 */
            uint32_t q = lib_divide(q16bits(x), q16bits(y), 47, e->cd);

            if (!rat_sign(y))
                e->flags |= EM_F_OVF;
            v = rat_make((int32_t)q, 65536);
        } else if (!rat_sign(y)) {
            e->flags |= EM_F_OVF;
            /* EM_R_PQDIV: the largest value in the format of the wanted
               type with the dividend's sign ("expr-tree-5: bit operators
               and shifts with fractions; Quick /0; Mod /0": "Größter Wert
               des Ziels"): a Quick $7FFFFFFF as qdivide, a Word 32767, a
               Byte 127, a Long or none the largest Long, as lqdiv and
               longdiv (#200); else 0 */
            if (!c->pqdiv)
                v = rat_int(0);
            else {
                v = W == TQ ? rat_make(0x7fffffff, 65536)
                    : W == TW ? rat_int(32767) : W == TB ? rat_int(127) : rat_int(0x7fffffff);
                if (rat_sign(x) < 0)
                    v = rat_op(rat_int(0), v, '-');
            }
        } else if (c->p1221 && f == 32 && W >= TB && W <= TL && !(c->pdivl && W == TL)
                   && !(c->p1222 && n == c->root))
            v = rat_q16dn(rat_op(x, y, '/'));   /* EM_R_1221, as * (a Long: whole towards zero below) */
        else
            v = rat_q16(rat_op(x, y, '/'));
        if (c->pdivl && W == TL)
            v = rat_trunc(v);   /* P: a / under a wanted Long gives a whole number, as today (#1178) */
        break;
    default:                    /* Mod: a - b*trunc(a/b) */
        if (!rat_sign(y)) {
            e->flags |= EM_F_OVF;
            v = rat_int(0);
            if (c->pmod) {
                /* EM_R_PMOD, a divisor 0 as today per type ("Wie heute je
                   Typ"): a 16.16 Mod traps as domodquick's DIVS.W, a
                   48.16 one gives the dividend as domodlong */
                if (f == 16)
                    e->divtrap = 1;
                else
                    v = x;
            }
        } else
            v = rat_op(x, rat_op(y, rat_trunc(rat_op(x, y, '/')), '*'), '-');
        if (c->pmod && rat_sign(v) < 0)
            v = rat_op(v, y, '+');      /* P: as today's Mod (domodlong: + the divisor if negative) */
        break;
    }
    trecord(c->tr, op, is_consttree(n->l) ? -1 : lf, TQ, f == 32, is_consttree(n->r) ? -1 : rf, TQ, f == 32,
            f == 32 ? '3' : '1', f == 32 ? TL : TQ);
    *out = c7wrap(e, v, c7bits(c, f));
    *fmt = f;
    return 0;
}

static int cpoint(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_VAR: return 0;
    case TN_CONST: return strchr(n->text, '.') != NULL;
    case TN_BIN: return cpoint(n->l) || cpoint(n->r);
    default: return cpoint(n->l);
    }
}

/* pfloat of an operand: EM_R_PCMPW ("Vergleichsergebnis ganzzahlig"): a
   comparison's result is a whole number, also when it compares in Float */
static int pfloatop(struct cev *c, const struct em_tnode *n, int W)
{
    if (c->pcmpw && n->kind == TN_BIN && em_op_is_compare(n->op))
        return 0;
    return pfloat(c, n, W);
}

/* P: does the subtree under the wanted type W hold a Float value (a Float
   variable, a ^, a fraction constant that is a Float there) */
static int pfloat(struct cev *c, const struct em_tnode *n, int W)
{
    switch (n->kind) {
    case TN_VAR: return n->t == TF;
    case TN_CONST: {
        struct rat a = rat_text(n->text);

        if (!c->pconst || !a.ok || (rat_isint(a) && !strchr(n->text, 0x2e)))
            return 0;
        return pconstfloat(c, a, W, 0);
    }
    case TN_BIN:
        if (n->op == EM_POW)
            return 1;
        if (em_op_is_compare(n->op)) {
            int sq = c->regionq, k;

            c->regionq = has_qvar(n->l);
            k = pfloatop(c, n->l, 0);
            c->regionq = has_qvar(n->r);
            k = k || pfloatop(c, n->r, 0);
            c->regionq = sq;
            /* a constant alone beside the other operand takes its class */
            if (is_consttree(n->l) && !has_t(n->r, TF) && !has_pow(n->r) && has_qvar(n->r))
                k = pfloatop(c, n->r, 0);
            if (is_consttree(n->r) && !has_t(n->l, TF) && !has_pow(n->l) && has_qvar(n->l)) {
                c->regionq = 1;
                k = pfloatop(c, n->l, 0);
                c->regionq = sq;
            }
            return k;
        }
        return pfloatop(c, n->l, W) || pfloatop(c, n->r, W);
    default: return pfloatop(c, n->l, W);
    }
}

/* the type of a subtree by a dry run of C1 (no trace, no flags) */
static int drytype(struct cev *c, const struct em_tnode *n, int W)
{
    struct ev save = *c->e;
    struct trec *tr = c->tr;
    int cn = c->cand, k, t;
    struct reg r;

    c->tr = NULL;
    c->cand = EM_CAND_C1;
    k = cand(c, n, W, 0, &r);
    t = k ? 0 : r.t;
    c->cand = cn;
    c->tr = tr;
    *c->e = save;
    return t;
}

/* P: a Long value takes part in the arithmetic of the subtree under the
   wanted type W (a Long variable, a constant beyond a Word, a whole-number
   or bit-operator subexpression whose type is Long there) */
static int bigconst(const struct em_tnode *n)
{
    switch (n->kind) {
    case TN_VAR: return 0;
    case TN_CONST: {
        struct rat a = rat_text(n->text);

        return a.ok && rat_isint(a) && !cfits(a, TW);
    }
    case TN_BIN: return bigconst(n->l) || bigconst(n->r);
    default: return bigconst(n->l);
    }
}

static int haslongw(struct cev *c, const struct em_tnode *n, int W)
{
    if (is_consttree(n)) {
        struct rat a = cideal(c, n);

        return a.ok && rat_isint(a) && !cfits(a, TW);
    }
    if (!has_qvar(n)) {
        if (n->kind == TN_VAR)
            return n->t == TL;
        if (n->kind == TN_NEG)
            return haslongw(c, n->l, W);
        /* Word+Word is a Long only by name (add.w): a Long there only in a
           comparison, where whole-number operations are at least Long */
        if (n->kind == TN_BIN && (n->op == EM_ADD || n->op == EM_SUB) && c->cmpmin != TL)
            return haslongw(c, n->l, W) || haslongw(c, n->r, W);
        return has_t(n, TL) || bigconst(n) || drytype(c, n, W) == TL;
    }
    switch (n->kind) {
    case TN_VAR: return 0;
    case TN_NEG: return haslongw(c, n->l, W);
    case TN_BIN:
        if (em_op_is_compare(n->op))
            return 0;
        if (n->op == EM_ADD || n->op == EM_SUB || n->op == EM_MUL || n->op == EM_DIV || n->op == EM_MOD)
            return haslongw(c, n->l, W) || haslongw(c, n->r, W);
        return drytype(c, n, W) == TL;
    default: return drytype(c, n, W) == TL;
    }
}

static int haslong(struct cev *c, const struct em_tnode *n)
{
    return haslongw(c, n, 0);
}

/* P: a subtree with a Quick and no Float inside a bit operator, a shift, Not
   or an expression with a Float: by C7 under W, then the Quick it fits or its
   whole part as a Long; EM_R_PTAKE: a 16.16 value the Quick, a 48.16 one
   its whole part as a Long under a bit operator, shift or Not (bittaker),
   else the value as a Float */
static int c7reg(struct cev *c, const struct em_tnode *n, int W, struct reg *r)
{
    return c7regf(c, n, W, 0, r);
}

static int c7regf(struct cev *c, const struct em_tnode *n, int W, int frac, struct reg *r)
{
    struct rat v;
    int fmt, cn = c->cand, k, sf = c->fracwhole, bit = c->bittaker;

    c->cand = EM_CAND_C7;
    c->fracwhole = frac || (c->preach && W != TL && !(n->kind == TN_BIN && em_op_is_compare(n->op)));
    k = c7val(c, n, W, c->pnarrow && !W && !(n->kind == TN_BIN && em_op_is_compare(n->op)) ? haslong(c, n) : 0, &v, &fmt);
    c->cand = cn;
    c->fracwhole = sf;
    if (k)
        return 1;
    memset(r, 0, sizeof *r);
    if (n->kind == TN_BIN && em_op_is_compare(n->op)) {
        r->t = TW;
        r->v = (uint32_t)(int32_t)rat_whole(v);
        r->id = v;
    } else if (c->ptake && fmt == 32 && !bit) {
        /* EM_R_PTAKE ("Ganzer Teil als Long"): a 48.16 value taken by a
           comparison under a wanted Float or beside a Float: straight to
           Float, by the operation, not by the value */
        r->t = TF;
        r->v = ffp_of(v);
        r->id = v;
    } else if (c->ptake ? fmt != 32 : fits(v, TQ)) {
        /* a 16.16 value stays the Quick it is; without EM_R_PTAKE a 48.16
           one too when it fits a Quick */
        r->t = TQ;
        r->v = (uint32_t)(int32_t)rat_whole(rat_floor(rat_op(v, rat_int(65536), '*')));
        r->id = v;
    } else {
        v = c7wrap(c->e, rat_floor(v), 32);
        r->t = TL;
        r->v = (uint32_t)(int32_t)rat_whole(v);
        r->id = v;
    }
    plainreg(r);
    return 0;
}

/* the expression by C7 under the wanted type asked into *r; 0, 1 refused;
   -1: not C7's (no Quick, a Float), the caller takes C1 */
static int c7root(struct cev *c, const struct em_tnode *root, int asked, struct reg *r)
{
    struct ev *e = c->e;
    struct rat v;
    int fmt;
    /* part 3: a / at the top towards zero, under a wanted Long (EM_R_1222:
       also under a Byte or Word) */
    int topdiv = root->kind == TN_BIN && root->op == EM_DIV
                 && (!(c->ptyped && asked != TL) || (c->p1222 && (asked == TB || asked == TW)));

    c->scopeb = 0;              /* a comparison's operands: no wanted type, both scopes */
    c->root = root;
    c->regionq = has_qvar(root);
    if (!has_qvar(root) || asked == TF)
        return -1;
    if (c->ptyped || c->pconst ? pfloat(c, root, asked) : (has_t(root, TF) || has_pow(root)))
        return -1;          /* without a Quick, with a Float or ^ (a Float): today's */
    c->fracwhole = c->preach && asked != TL && !(root->kind == TN_BIN && em_op_is_compare(root->op));
    if (c7val(c, root, asked, c->pnarrow && !asked && !(root->kind == TN_BIN && em_op_is_compare(root->op)) ? haslong(c, root) : 0, &v, &fmt))
        return 1;
    memset(r, 0, sizeof *r);
    if (c->tr)
        c->tr->consumer = root->kind == TN_BIN && em_op_is_compare(root->op) ? 'c'
                          : asked == TQ ? 'q'
                          : !asked ? 'z'
                          : topdiv ? 't' : 'f';
    if (root->kind == TN_BIN && em_op_is_compare(root->op)) {
        r->t = TW;
        r->v = (uint32_t)(int32_t)rat_whole(v);
        r->id = v;
        return 0;
    }
    if (asked == TQ || (!asked && fits(v, TQ))) {
        struct rat q = c7wrap(e, v, 16);

        r->t = TQ;
        r->v = (uint32_t)(int32_t)rat_whole(rat_floor(rat_op(q, rat_int(65536), '*')));
        r->id = q;
        return 0;
    }
    /* a whole number: once, at the consumer */
    v = topdiv ? rat_trunc(v) : rat_floor(v);
    v = c7wrap(e, v, 32);
    r->t = TL;
    r->v = (uint32_t)(int32_t)rat_whole(v);
    r->id = v;
    return 0;
}

/* ------------------------------------------------------- the evaluation */

/* ============================================ the rules of type-system-rework */

/*
 * EM_R_TSR (exprmatrix.h, EM_RULES_TSR): the rules of the OpenSpec change
 * type-system-rework, written from its spec delta
 * (openspec/changes/type-system-rework/specs/compiler/expression-types/
 * spec.md) and not from the compiler.  Each rule names its decision as
 * the rule table of the change's design.md does, by the heading of the
 * user's answer in ~/.local/share/bb2-c-work/decisions/rounds/tree/
 * decisions.md; of 2026-11-27 unless named, cited below by these short
 * names:
 *
 *   direction  "Rework of the whole type system (direction)"
 *   round 1    "Type system rework, round 1: width, constants, whole
 *              division, conversion"
 *   round 2    "Type system rework, round 2: bit operators, shifts,
 *              overflow, no destination"
 *   round 3    "Type system rework, round 3: comparisons, point constants,
 *              the shift exception, AB3"
 *   round 4/5  "Type system rework, round 4: the shift exception's reach;
 *              planning", "..., round 5: LSR vs LSL in the shift exception"
 *   Q9 Q6 Q8   "type-system-rework Q9, Q12, Q6, Q8"
 *   Q1 Q2 Q4   "type-system-rework Q1, Q2, Q4, Q7"
 *   Q5 Q13-Q17 "type-system-rework Q3, Q5, Q10, Q13-Q15, Q17"
 *   widths     "type-system-rework: shift widths"
 *   Q12 X      "type-system-rework Q12 classes", class X
 *   N1 N2      "type-system-rework N1, N2"
 *   I1-I9      "type-system-rework I1–I3", "... I4" ... "... I8",
 *              "type-system-rework I9 and exception warnings"
 *   item 29    "type-system-rework conflict item 29 (destination influence)"
 *   D2 D3 D4   "2026-11-28 type-system-rework D2, D3, D4 (readings of the
 *              model)"
 *   D1/D7 D5   "2026-11-28 type-system-rework D1/D7 (division and Mod by a
 *              run-time 0), D5"
 *   D6         "2026-11-28 type-system-rework D6 (width of NOT)"
 *   P1         the planner's P1 in rounds/tree/coordinator-decisions.md,
 *              "2026-11-28 type-system-rework task 1.3: the planner's
 *              Plan/Monitor choices"
 *
 * Every node gets its class from its operator and its operands
 * (tsr_class): whole (a width of 8, 16 or 32 bits), 48.16 or Float; the
 * destination only converts the finished value (tsr_convert), except for
 * the named exceptions, which reach a node from the root (TSR_C1, TSR_E)
 * or concern the root's operation (D, J) ("direction"; conflict item 29
 * "Richtung + Ausnahmeliste": a closed list).  A value (struct tv) is
 * exact in its class: a whole number in its width, a 48.16 number as
 * value * 65536 in 64 bits (it wraps beyond 48 integer bits), a Float in
 * Motorola FFP.  A / or Mod by a run-time 0 raises the division trap for
 * every class (D1/D7); a constant divisor 0 is refused.
 */

enum { K_W = 1, K_Q = TQ, K_F = TF, K_P = 6 };      /* K_P: a constant with a point, its class not decided yet */

/* how a node is reached from the root */
enum {
    TSR_ROOT = 1,               /* the whole expression */
    TSR_C1 = 2,                 /* only through + - * / and the unary minus (the whole-division exception) */
    TSR_E = 4                   /* only through + - (the shift exception) */
};

/* what a node's parent tells it about the other operand of its operator */
enum {
    SIB_NONE = 0,               /* no other operand (the root, a unary operator, a shift count) */
    SIB_PROBE = 8,              /* the class of the node by itself, for the parent to decide */
    SIB_SHIFTL = 9,             /* the left operand of a shift */
    SIB_FTOP = 10               /* an operand of the top operation computed in Float (D) */
    /* else K_W, K_Q, K_F: the decided class of the other operand */
};

struct tty {
    int k, n;                   /* the class, the width of a whole number */
    int refused;                /* "Illegal Operator for Type" (or a constant divisor 0, tsr_eval) */
    int lsib, rsib, lreach, rreach;     /* what the operands are told */
    int ftop;                   /* D: the top operation under a Float destination in Float */
    int div;                    /* a whole / that keeps its fraction beside a Quick (C3) when told so */
    int shiftx;                 /* E: a right shift with 16 fraction bits */
    int jwrap;                  /* J: the top left shift converted to the Quick wrapped */
};

struct tv {
    int k, n;
    int64_t v;                  /* K_W: the value in its width; K_Q: value * 65536 */
    uint32_t f;                 /* K_F */
    int jwrap;
};

struct tsr {
    struct ev *e;
    int dest;                   /* TB..TF; 0: none (Print, If) */
    double powscale;            /* ^: SPPow's result scaled (its last bits are not modelled) */
    int powzero;                /* ^: a result below FFP's range 0 */
    int haspow, powtiny, powunknown;
};

static int64_t wrapw(int64_t v, int n)
{
    return n == 8 ? (int8_t)v : n == 16 ? (int16_t)v : (int32_t)(uint32_t)v;
}

/* whole operations one step wider: Byte to Word, Word to Long, Long stays
   ("round 1 (width)", "Q6 Bestätigen, beides": the unary minus too) */
static int wider(int n)
{
    return n == 8 ? 16 : 32;
}

static int is_shift(enum em_op o)
{
    return o >= EM_LSL && o <= EM_ASR;
}

static int is_pointconst(const struct em_tnode *n)
{
    return n->kind == TN_CONST && strchr(n->text, '.') != NULL;
}

/* a constant with a point, also under unary minuses (which do not change
   its reading, D3); else NULL */
static const struct em_tnode *pc_node(const struct em_tnode *n)
{
    while (n->kind == TN_NEG)
        n = n->l;
    return is_pointconst(n) ? n : NULL;
}

/* a constant with a point exact in 16.16 (a literal has no sign) */
static int pc_exact(const char *text)
{
    struct rat a = rat_text(text);

    return a.ok && (a.n * 65536) % a.d == 0 && a.n < 32768 * a.d;
}

/* a constant with a point read as a Quick fits the Quick range */
static int pc_fits(const char *text)
{
    struct rat a = rat_text(text);

    return a.ok && a.n < 32768 * a.d;
}

/* the width of a whole constant: its smallest type, DEFTYPE has no say
   ("round 1: (c) kleinster Typ") */
static int wc_width(const char *text)
{
    struct rat a = rat_text(text);
    int64_t v = (int32_t)(uint32_t)(uint64_t)(a.ok ? (int64_t)a.n : 0);

    return v >= -128 && v <= 127 ? 8 : v >= -32768 && v <= 32767 ? 16 : 32;
}

static int arith_op(enum em_op o)
{
    return o == EM_ADD || o == EM_SUB || o == EM_MUL || o == EM_DIV || o == EM_MOD;
}

/* the class of n reached by reach, told sib about the other operand */
static void tsr_class(const struct tsr *t, const struct em_tnode *n, int reach, int sib, struct tty *o)
{
    struct tty a, b;

    memset(o, 0, sizeof *o);
    switch (n->kind) {
    case TN_VAR:
        if (n->t == TQ || n->t == TF)
            o->k = n->t;
        else {
            o->k = K_W;
            o->n = n->t == TB ? 8 : n->t == TW ? 16 : 32;
        }
        return;
    case TN_CONST:
        if (!strchr(n->text, '.')) {
            o->k = K_W;
            o->n = wc_width(n->text);
        } else if (sib == SIB_FTOP)
            o->k = K_F;         /* D: read as the Float of the written value ("Q12 D", I2 A) */
        else if (pc_exact(n->text) || (sib == K_Q && pc_fits(n->text)))
            /* a Quick when exact in 16.16 or beside a Quick ("Q8 Quick nur
               wenn exakt", round 3); left of a shift with no Quick beside it
               (-0.1 LSL 2) not exact it is a Float, which the shift refuses
               (spec "A constant with a point is read by its operands") */
            o->k = K_Q;
        else
            o->k = sib == SIB_PROBE ? K_P : K_F;
        return;
    case TN_NEG:
        /* C1 reaches through the unary minus (I4 B), D and E do not (D2); a
           constant with a point under it is read as without it, beside a
           Quick a Quick (D3); a whole / under it keeps its fraction beside a
           Quick as without it (decision "2026-11-28 type-system-rework C3
           through a unary minus": NPrint q+-(w/2) -1) */
        o->lreach = reach & TSR_C1;
        o->lsib = pc_node(n->l) || sib == K_Q ? sib : SIB_NONE;
        tsr_class(t, n->l, o->lreach, o->lsib, &a);
        o->refused = a.refused;
        o->k = a.k;
        o->div = a.div;
        if (a.k == K_W)
            o->n = wider(a.n);
        return;
    case TN_NOT:
        /* bit operators take whole operands, a Quick rounded down; a Float
           refused ("round 2 (option 1)", "Q17 Weiter verweigern"); one step
           wider than a whole operand, of a Quick a Long (D6) */
        tsr_class(t, n->l, 0, SIB_NONE, &a);
        o->refused = a.refused || a.k == K_F;
        o->k = K_W;
        o->n = a.k == K_W ? wider(a.n) : 32;
        return;
    default:
        break;
    }
    if (is_shift(n->op)) {
        const struct em_tnode *c = n->r;

        /* a shift keeps the type of its left operand; the count is whole, a
           constant with a point refused ("round 2", "Q15 48.16", "Q14 Eigene
           Breite", "Q12 A LSL immer Long") */
        o->lsib = SIB_SHIFTL;
        tsr_class(t, n->l, 0, SIB_SHIFTL, &a);
        tsr_class(t, c, 0, SIB_NONE, &b);
        while (c->kind == TN_NEG)
            c = c->l;
        o->refused = a.refused || b.refused || a.k == K_F || b.k == K_F || is_pointconst(c);
        if (a.k == K_Q) {
            o->k = K_Q;
            /* J for a Quick left operand (I6 B) */
            o->jwrap = (n->op == EM_LSL || n->op == EM_ASL) && t->dest == TQ && (reach & TSR_ROOT);
        } else if ((n->op == EM_LSR || n->op == EM_ASR) && t->dest == TQ && (reach & TSR_E)) {
            /* the shift exception: at the top or through + - under a Quick
               destination ("round 3-5", Q13, "N1 16.16, außerhalb 48.16",
               "Q12 E Shift durch + -", I9 A) */
            o->k = K_Q;
            o->shiftx = 1;
        } else if (n->op == EM_LSL || n->op == EM_ASL) {
            o->k = K_W;
            o->n = 32;
            /* J: the top left shift under a Quick destination wraps ("Q12 J
               LSL unter Quick umbrechen", I5 A: only as the whole expression) */
            o->jwrap = t->dest == TQ && (reach & TSR_ROOT);
        } else {
            o->k = K_W;
            o->n = a.n;
        }
        return;
    }
    {
        int r = 0, ka, kb;

        if ((reach & TSR_C1) && (arith_op(n->op) && n->op != EM_MOD))
            r |= TSR_C1;
        if ((reach & TSR_E) && (n->op == EM_ADD || n->op == EM_SUB))
            r |= TSR_E;
        o->lreach = o->rreach = r;
        tsr_class(t, n->l, r, SIB_PROBE, &a);
        tsr_class(t, n->r, r, SIB_PROBE, &b);
        o->refused = a.refused || b.refused;
        ka = a.k;
        kb = b.k;
        /* a constant with a point beside a Quick-class operand is a Quick, else
           a Float (Q8; I1 A: a Quick subexpression or a Quick constant is of
           Quick class); two undecided ones are Floats */
        if (ka == K_P)
            ka = kb == K_Q && pc_fits(pc_node(n->l)->text) ? K_Q : K_F;
        if (kb == K_P)
            kb = ka == K_Q && pc_fits(pc_node(n->r)->text) ? K_Q : K_F;
        /* a whole / beside a Quick-class operand keeps its fraction ("Q12
           C3", I1 A); a Float does not; at the arithmetic operators, Mod, ^
           and the comparisons, not at the bit operators (D4) */
        if (n->op != EM_AND && n->op != EM_OR && n->op != EM_EOR) {
            if (a.div && kb == K_Q)
                ka = K_Q;
            if (b.div && ka == K_Q)
                kb = K_Q;
        }
        o->lsib = kb;
        o->rsib = ka;
        if (a.div && ka == K_W)
            o->lsib = SIB_NONE; /* a whole / that stays whole is not told of a Quick */
        if (b.div && kb == K_W)
            o->rsib = SIB_NONE;
        /* D: under a Float destination the top operation with a Quick operand
           in Float, a constant with a point there read as a Float ("Q12 D
           Float reicht oben hinein", I2 A); only an arithmetic operation (+ -
           * /, Mod, ^), not a comparison (D2) nor a bit operator, whose Float
           operand is refused */
        if (t->dest == TF && (reach & TSR_ROOT) && (arith_op(n->op) || n->op == EM_POW)
            && (ka == K_Q || kb == K_Q)) {
            o->ftop = 1;
            if (pc_node(n->l))
                o->lsib = SIB_FTOP;
            if (pc_node(n->r))
                o->rsib = SIB_FTOP;
        }
        if (n->op == EM_POW) {
            o->k = K_F;         /* ^ always in Float ("direction (upcasting)") */
            return;
        }
        if (em_op_is_compare(n->op)) {
            o->k = K_W;         /* the boolean a Word ("Q5 Word, wie Original") */
            o->n = 16;
            return;
        }
        if (n->op == EM_AND || n->op == EM_OR || n->op == EM_EOR) {
            o->refused |= ka == K_F || kb == K_F;
            o->k = K_W;
            o->n = wider(tmax(ka == K_W ? a.n : 32, kb == K_W ? b.n : 32));
            return;
        }
        if (o->ftop || ka == K_F || kb == K_F)
            o->k = K_F;         /* a Float operand: Float ("direction (upcasting)") */
        else if (ka == K_Q || kb == K_Q)
            o->k = K_Q;         /* a Quick operand: 48.16 ("round 1 (width)") */
        else if (n->op == EM_DIV && (reach & TSR_C1) && (t->dest == TQ || t->dest == TF))
            /* the whole-division exception: at the top or through + - * / and
               the unary minus under a Quick or Float destination, a 48.16
               quotient or one computed in Float (Q9, "Q12 C1 Ja, für alle",
               I3 A, I4 B) */
            o->k = t->dest;
        else if (n->op == EM_DIV && sib == K_Q)
            o->k = K_Q;         /* C3, told by the parent */
        else {
            o->k = K_W;
            o->n = wider(tmax(a.n, b.n));
            o->div = n->op == EM_DIV;
        }
    }
}

/* FFP of a double (truncated to 32 bits, rounded half up as ffp_pack) */
static uint32_t ffp_dbl(double x)
{
    int e;
    double m;

    if (x == 0 || x != x)
        return 0;
    m = frexp(fabs(x), &e);
    return ffp_pack((uint64_t)ldexp(m, 63), e - 63, x < 0, 0);
}

/* the whole part (rounded down) of a whole or 48.16 value, the low 32 bits */
static int64_t tv_whole(const struct tv *a)
{
    if (a->k == K_W)
        return a->v;
    return (int32_t)(uint32_t)(uint64_t)(a->v >> 16);
}

/* as 48.16 (value * 65536) */
static __int128 tv_q(const struct tv *a)
{
    return a->k == K_W ? (__int128)a->v * 65536 : (__int128)a->v;
}

/* as a Float, a Quick or 48.16 with its fraction ("direction") */
static uint32_t tv_f(const struct tv *a)
{
    if (a->k == K_F)
        return a->f;
    if (a->k == K_W)
        return ffp_flt((int32_t)a->v);
    if (a->v >= INT32_MIN && a->v <= INT32_MAX)
        return lib_qtof((uint32_t)a->v);
    return ffp_pack(a->v < 0 ? -(uint64_t)a->v : (uint64_t)a->v, -16, a->v < 0, 0);
}

static __int128 floordiv128(__int128 a, __int128 b)
{
    __int128 q = a / b;

    if ((a % b) && ((a < 0) != (b < 0)))
        q--;
    return q;
}

/* the Float of a written constant with a point: fetchpoint2's whole +
   digits / 10^n in FFP, more digits than a Long holds by the exact value */
static uint32_t pc_float(const char *text)
{
    const char *p = strchr(text, '.');
    int32_t whole = (int32_t)strtol(text, NULL, 10), digits = 0, div = 1;
    uint32_t f;

    if (strlen(p + 1) > 9 || strtod(text, NULL) > 2147483647.0)
        return ffp_dbl(strtod(text, NULL));
    for (p++; *p; p++) {
        digits = digits * 10 + (*p - '0');
        div *= 10;
    }
    f = ffp_flt(whole);
    if (digits)
        f = ffp_add(ffp_div(ffp_flt(digits), ffp_flt(div)), f, 0);
    return f;
}

/* the FFP of an exact value (a variable's) */
static uint32_t rat_ffp(struct rat a)
{
    if (a.ok && a.n <= INT32_MAX && a.n >= INT32_MIN && a.d <= INT32_MAX)
        return ffp_of(a);
    return ffp_dbl(a.x);
}

static int tsr_eval(struct tsr *t, const struct em_tnode *n, int reach, int sib, struct tv *o);

static int tsr_shift(struct tsr *t, const struct em_tnode *n, const struct tty *ty, struct tv *o)
{
    struct tv a, b;
    int cnt;

    if (tsr_eval(t, n->l, 0, SIB_SHIFTL, &a) || tsr_eval(t, n->r, 0, SIB_NONE, &b))
        return 1;
    cnt = (int)(tv_whole(&b) & 63);     /* a Quick count rounded down; the 68000's count modulo 64 */
    if (a.k == K_Q) {
        /* a Quick: LSL, ASL, ASR in 48.16, LSR on the 16.16 bits (Q15) */
        if (n->op == EM_LSR)
            o->v = (int32_t)(cnt >= 32 ? 0 : (uint32_t)a.v >> cnt);
        else if (n->op == EM_ASR)
            o->v = a.v >> cnt;
        else
            o->v = (int64_t)((uint64_t)a.v << cnt);
        return 0;
    }
    if (ty->shiftx) {
        /* the shift exception: the value with 16 fraction bits, on the 16.16
           bits inside the Quick range, in 48.16 beyond it (N1) */
        int64_t raw = a.v * 65536;

        if (a.v >= -32768 && a.v <= 32767) {
            if (n->op == EM_LSR)
                o->v = (int32_t)(cnt >= 32 ? 0 : (uint32_t)raw >> cnt);
            else
                o->v = (int32_t)raw >> (cnt > 31 ? 31 : cnt);
        } else if (n->op == EM_LSR)
            o->v = (int64_t)((uint64_t)raw >> cnt);
        else
            o->v = raw >> cnt;
        return 0;
    }
    if (n->op == EM_LSL || n->op == EM_ASL)
        o->v = (int32_t)(cnt >= 32 ? 0 : (uint32_t)a.v << cnt);       /* as a Long (Q12 A) */
    else if (n->op == EM_LSR) {
        /* in its own width (Q14) */
        uint32_t m = a.n == 8 ? 0xffu : a.n == 16 ? 0xffffu : 0xffffffffu;

        o->v = wrapw(cnt >= a.n ? 0 : ((uint32_t)a.v & m) >> cnt, a.n);
    } else
        o->v = wrapw(a.v >> (cnt > 63 ? 63 : cnt), a.n);
    return 0;
}

/* ^ in Float; SPPow (mathtrans) is not modelled: its result is known only
   where every value near it gives the same result (tsr_value) */
static void tsr_pow(struct tsr *t, const struct tv *a, const struct tv *b, struct tv *o)
{
    double x = ffp_rat(tv_f(a)).x, y = ffp_rat(tv_f(b)).x, r;

    t->haspow = 1;
    if ((x < 0 && y != floor(y)) || (x == 0 && y <= 0) || fabs(y) > 1e9) {
        t->powunknown = 1;
        o->f = 0;
        return;
    }
    r = pow(fabs(x), y);
    if (x < 0 && fmod(fabs(y), 2) == 1)
        r = -r;                 /* a negative base and a whole exponent keep the sign (#1189) */
    r *= t->powscale;
    if (!isfinite(r) || fabs(r) > 9.2e18) {
        t->powunknown = 1;
        o->f = 0;
        return;
    }
    if (r != 0 && fabs(r) < 5.5e-20) {
        t->powtiny = 1;
        if (t->powzero)
            r = 0;
    }
    o->f = ffp_dbl(r);
}

static int tsr_eval(struct tsr *t, const struct em_tnode *n, int reach, int sib, struct tv *o)
{
    struct tty ty;
    struct tv a, b;

    tsr_class(t, n, reach, sib, &ty);
    memset(o, 0, sizeof *o);
    if (ty.refused)
        return 1;
    o->k = ty.k;
    o->n = ty.n;
    o->jwrap = ty.jwrap;
    switch (n->kind) {
    case TN_VAR: {
        uint32_t bits = var_bits(n->t, n->text);

        if (ty.k == K_W)
            o->v = wrapw((int32_t)bits, ty.n);
        else if (ty.k == K_Q)
            o->v = (int32_t)bits;
        else
            o->f = rat_ffp(rat_text(n->text));
        return 0;
    }
    case TN_CONST: {
        struct rat x = rat_text(n->text);

        if (ty.k == K_W)
            o->v = (int32_t)(uint32_t)(uint64_t)(x.ok ? (int64_t)x.n : 0);
        else if (ty.k == K_Q) {
            /* the FFP value of its text cut down to 1/65536, as a Quick
               literal (Q8, P1: x.q=q+30000.3 $75314D00) */
            struct rat f = rat_floor(rat_op(ffp_rat(pc_float(n->text)), rat_int(65536), '*'));

            o->v = f.ok ? (int64_t)f.n : (int64_t)floor(f.x);
        } else
            o->f = pc_float(n->text);
        return 0;
    }
    case TN_NEG:
        if (tsr_eval(t, n->l, ty.lreach, ty.lsib, &a))
            return 1;
        if (a.k == K_W)
            o->v = wrapw(-a.v, ty.n);
        else if (a.k == K_Q)
            o->v = (int64_t)(0 - (uint64_t)a.v);
        else
            o->f = ffp_neg(a.f);
        return 0;
    case TN_NOT:
        if (tsr_eval(t, n->l, 0, SIB_NONE, &a))
            return 1;
        o->v = wrapw(~tv_whole(&a), ty.n);
        return 0;
    default:
        break;
    }
    if (is_shift(n->op))
        return tsr_shift(t, n, &ty, o);
    if (tsr_eval(t, n->l, ty.lreach, ty.lsib, &a) || tsr_eval(t, n->r, ty.rreach, ty.rsib, &b))
        return 1;
    if ((n->op == EM_DIV || n->op == EM_MOD) && (b.k == K_F ? ffp_zero(b.f) : b.v == 0)) {
        if (is_consttree(n->r))
            return 1;           /* a constant divisor 0: "Division by Zero" */
        /* a run-time divisor 0: the division trap for every class (D1/D7) */
        t->e->divtrap = 1;
        return 0;
    }
    if (n->op == EM_POW) {
        tsr_pow(t, &a, &b, o);
        return 0;
    }
    if (em_op_is_compare(n->op)) {
        /* comparisons on the full value, the wider of the two (round 3) */
        int c;

        if (a.k == K_F || b.k == K_F)
            c = ffp_cmp(tv_f(&a), tv_f(&b));
        else {
            __int128 x = tv_q(&a), y = tv_q(&b);

            c = (x > y) - (x < y);
        }
        switch (n->op) {
        case EM_EQ: c = c == 0; break;
        case EM_NE: c = c != 0; break;
        case EM_LT: c = c < 0; break;
        case EM_LE: c = c <= 0; break;
        case EM_GT: c = c > 0; break;
        default: c = c >= 0; break;
        }
        o->v = c ? -1 : 0;
        return 0;
    }
    if (n->op == EM_AND || n->op == EM_OR || n->op == EM_EOR) {
        uint32_t x = (uint32_t)tv_whole(&a), y = (uint32_t)tv_whole(&b);

        o->v = wrapw(n->op == EM_AND ? x & y : n->op == EM_OR ? x | y : x ^ y, ty.n);
        return 0;
    }
    if (ty.k == K_F) {
        uint32_t x = tv_f(&a), y = tv_f(&b);

        switch (n->op) {
        case EM_ADD: o->f = ffp_add(x, y, 0); break;
        case EM_SUB: o->f = ffp_add(x, y, 1); break;
        case EM_MUL: o->f = ffp_mul(x, y); break;
        case EM_DIV: o->f = ffp_div(x, y); break;
        default: {
            /* floor Mod ("Q2 Floor") */
            struct rat rx = ffp_rat(x), ry = ffp_rat(y);

            o->f = rat_ffp(rat_op(rx, rat_op(ry, rat_floor(rat_op(rx, ry, '/')), '*'), '-'));
            break;
        }
        }
        return 0;
    }
    if (ty.k == K_Q) {
        /* 48.16, a product or quotient cut down to 1/65536 ("Q1 Immer
           abrunden") */
        __int128 x = tv_q(&a), y = tv_q(&b), r;

        switch (n->op) {
        case EM_ADD: r = x + y; break;
        case EM_SUB: r = x - y; break;
        case EM_MUL: r = floordiv128(x * y, 65536); break;
        case EM_DIV: r = floordiv128(x * 65536, y); break;
        default: r = x - y * floordiv128(x, y); break;
        }
        if (r > INT64_MAX || r < INT64_MIN)
            t->e->flags |= EM_F_OVF;
        o->v = (int64_t)(uint64_t)(unsigned __int128)r;
        return 0;
    }
    {
        int64_t x = a.v, y = b.v, r;

        switch (n->op) {
        case EM_ADD: r = x + y; break;
        case EM_SUB: r = x - y; break;
        case EM_MUL: r = x * y; break;
        case EM_DIV: r = x / y; break;     /* towards zero ("round 1: Auf null hin") */
        default:
            /* floor Mod ("Q2 Floor") */
            r = x % y;
            if (r && ((r < 0) != (y < 0)))
                r += y;
            break;
        }
        if (wrapw(r, ty.n) != r)
            t->e->flags |= EM_F_OVF;
        o->v = wrapw(r, ty.n);
    }
    return 0;
}

/* saturated to a signed range of bits */
static int64_t sat(__int128 v, int bits)
{
    __int128 hi = ((__int128)1 << (bits - 1)) - 1;

    return (int64_t)(v > hi ? hi : v < -hi - 1 ? -hi - 1 : v);
}

/* the conversion of the root's value to the destination (asked TB..TF, 0
   none) into r: to a whole number a Quick rounded down, a Float towards
   zero, the low bits kept; to a Quick rounded down, saturated; without a
   destination a 48.16 value a Quick, a Long or a Float decided at run time
   ("round 1", "Q12 B Float auf null hin", "round 2 Umbrechen", I7 A, I8 A;
   "round 2 (counter-proposal)", "round 3 Ja, so") */
static void tsr_convert(struct tsr *t, const struct tv *a, struct reg *r)
{
    int asked = t->dest, bits;

    memset(r, 0, sizeof *r);
    if (asked == TF) {
        r->t = TF;
        r->v = tv_f(a);
        r->id = ffp_rat(r->v);
        return;
    }
    if (asked == TQ) {
        __int128 q;

        r->t = TQ;
        if (a->k == K_F) {
            struct rat x = rat_floor(rat_op(ffp_rat(a->f), rat_int(65536), '*'));

            q = x.ok ? x.n : (__int128)x.x;
        } else
            q = tv_q(a);
        if (a->jwrap)
            r->v = (uint32_t)(uint64_t)q;       /* J: wrapped */
        else {
            r->v = (uint32_t)sat(q, 32);
            if ((int64_t)(int32_t)r->v != q)
                t->e->flags |= EM_F_OVF;
        }
        r->id = rat_make((int32_t)r->v, 65536);
        return;
    }
    if (asked == TB || asked == TW || asked == TL) {
        __int128 w;

        bits = asked == TB ? 8 : asked == TW ? 16 : 32;
        r->t = asked;
        if (a->k == K_W)
            w = a->v;
        else if (a->k == K_Q)
            w = a->v >> 16;
        else {
            struct rat x = rat_trunc(ffp_rat(a->f));

            w = x.ok ? x.n : (__int128)x.x;
        }
        r->v = (uint32_t)(uint64_t)wrapw((int64_t)(uint64_t)(unsigned __int128)w, bits);
        if ((__int128)wrapw((int64_t)(uint64_t)(unsigned __int128)w, bits) != w)
            t->e->flags |= EM_F_OVF;
        r->id = rat_int(wrapw((int64_t)r->v, bits));
        return;
    }
    /* no destination (Print, If, the "any" parameters) */
    if (a->k == K_W) {
        r->t = TL;
        r->v = (uint32_t)a->v;
    } else if (a->k == K_F) {
        r->t = TF;
        r->v = a->f;
    } else if (a->v >= INT32_MIN && a->v <= INT32_MAX) {
        r->t = TQ;
        r->v = (uint32_t)a->v;
    } else if (!(a->v & 0xffff) && (a->v >> 16) >= INT32_MIN && (a->v >> 16) <= INT32_MAX) {
        r->t = TL;
        r->v = (uint32_t)(a->v >> 16);
    } else {
        r->t = TF;
        r->v = tv_f(a);
    }
}

/* one evaluation of the root: 0, 1 refused */
static int tsr_run(struct tsr *t, const struct em_tnode *root, struct reg *r)
{
    struct tv a;

    if (tsr_eval(t, root, TSR_ROOT | TSR_C1 | TSR_E, SIB_NONE, &a))
        return 1;
    tsr_convert(t, &a, r);
    return 0;
}

/* the value of root under the destination asked by the rules of
   type-system-rework: 0, 1 refused.  With a ^ the expression is evaluated
   again with SPPow's result 1e-4 smaller and larger (and 0 where it is below
   FFP's range): the value is known only where all of them agree, else
   EM_F_POW ("?") */
static int tsr_value(struct ev *e, const struct em_tnode *root, int asked, struct reg *r)
{
    struct tsr t = { .e = e, .dest = asked, .powscale = 1 };
    static const double scale[] = { 1 - 1e-4, 1 + 1e-4 };
    int trap;

    if (tsr_run(&t, root, r))
        return 1;
    trap = e->divtrap;
    if (!t.haspow)
        return 0;
    for (int z = 0; z <= t.powtiny && !t.powunknown; z++)
        for (size_t i = 0; i < sizeof scale / sizeof scale[0] && !t.powunknown; i++) {
            struct reg o;

            t.powscale = scale[i];
            t.powzero = z;
            e->divtrap = 0;
            if (tsr_run(&t, root, &o) || o.t != r->t || o.v != r->v || e->divtrap != trap)
                t.powunknown = 1;
        }
    e->divtrap = trap;
    if (t.powunknown) {
        e->flags |= EM_F_POW;
        e->powknown = 0;
    }
    return 0;
}

static int ctx_want(enum em_ctx x, uint64_t rules)
{
    if (x == EM_IF || x == EM_PRINT)
        return 0;
    return ctx_type(x, 0, rules);
}

/* the expression root under ctx by rules: its value in *r; 0, 1 refused */
static int tree_value(struct ev *e, const struct em_tnode *root, int asked, uint64_t rules, struct reg *r,
                      struct trec *tr)
{
    int cn = (int)((rules & EM_R_CANDMASK) >> 9);

    if (rules & EM_R_TSR)
        return tsr_value(e, root, asked, r);
    if (cn) {
        struct cev c = { .e = e, .cand = cn, .scopeb = (rules & EM_R_SCOPEB) != 0, .tr = tr, .rootw = asked };
        int k;

        if (cn == EM_CAND_C7 && (rules & EM_R_C7K)) {
            c.c7k = 1;
            c.cmpmode = rules & EM_R_C7KLONG ? 1 : rules & EM_R_C7KDEST ? 2 : 0;
            c.c7kexact = (rules & EM_R_C7KEXACT) != 0;
            c.ptyped = (rules & EM_R_PTYPED) != 0;
            c.pconst = (rules & EM_R_PCONST) != 0;
            c.pmod = (rules & EM_R_PMOD) != 0;
            c.pnarrow = (rules & EM_R_PNARROW) != 0;
            c.pdivl = (rules & EM_R_PDIVL) != 0;
            c.preach = (rules & EM_R_PREACH) != 0;
            c.pcmp = (rules & EM_R_PCMP) != 0;
            c.pqdiv = (rules & EM_R_PQDIV) != 0;
            c.pbits = (rules & EM_R_PBITS) != 0;
            c.ptake = (rules & EM_R_PTAKE) != 0;
            c.pmodx = (rules & EM_R_PMODX) != 0;
            c.pbitk = (rules & EM_R_PBITK) != 0;
            c.p48 = (rules & EM_R_P48) != 0;
            c.pcmpw = (rules & EM_R_PCMPW) != 0;
            c.p1221 = (rules & EM_R_1221) != 0;
            c.p1222 = (rules & EM_R_1222) != 0;
            c.p1223 = (rules & EM_R_1223) != 0;
            c.p1224 = (rules & EM_R_1224) != 0;
            rules |= EM_R_SCOPEB;   /* expressions without a Quick: C1's, operands typed by their own */
        }
        k = cn == EM_CAND_C7 ? c7root(&c, root, asked, r) : -1;

        if (k > 0)
            return 1;
        if (k < 0 && cn == EM_CAND_C7 && !(rules & EM_R_SCOPEB)) {
            /* C7: an expression without a Quick (or with a Float) is
               today's, byte for byte */
            int T = asked;

            if (flow(e, root, &T, r, tr))
                return 1;
        } else if (k < 0) {
            /* C7 with EM_R_SCOPEB ("C7+"): such an expression by C1,
               a comparison's left operand by its own operands */
            if (cn == EM_CAND_C7) {
                c.cand = EM_CAND_C1;
                c.scopeb = 0;
            }
            if (c.preach && asked != TL && !(c.p1222 && (asked == TB || asked == TW)) && asked != TF && asked != TS
                && has_qvar(root) && pfloat(&c, root, asked)
                && !(root->kind == TN_BIN && em_op_is_compare(root->op))) {
                c.reachf = 1;
                c.fmtf = 1;
                if (cand(&c, root, TF, 0, r))
                    return 1;
            } else if (cand(&c, root, asked, 0, r))
                return 1;
            wideok(e, r);
        }
    } else {
        int T = asked;

        if (flow(e, root, &T, r, tr))
            return 1;
    }
    if (r->raw)
        cutraw(e, r);
    if (asked && r->t != asked) {
        if (r->t == TS || asked == TS)
            return 1;
        conv(e, r, r->t, asked);
    }
    return 0;
}

/* the text of the value r of an expression under ctx (em_model's) */
static void value_text(struct ev *e, enum em_ctx ctx, uint64_t rules, int asked, struct reg *r,
                       struct em_value *out)
{
    if (e->flags & EM_F_POW) {
        int64_t v = 0;

        if (ctx == EM_IF && (rules & EM_R_1187) && e->powknown && fabs(e->pow.x) > 1e-30) {
            strcpy(out->text, "T");     /* EM_R_1187, as em_model */
            return;
        }
        if (ctx == EM_IF && !(rules & EM_R_1187))
            out->flags |= EM_F_D1;
        if (!pow_whole(e, asked, &v)) {
            strcpy(out->text, e->flags & EM_F_POWSIGN ? "?-" : "?");
            return;
        }
        r->v = (uint32_t)v;
    }
    switch (ctx) {
    case EM_DB: print_long(out->text, (int8_t)r->v); break;
    case EM_I2:
        /* EM_R_TSR: every index a Long */
        print_long(out->text, (rules & EM_R_TSR) ? (int32_t)r->v : (int16_t)r->v);
        break;
    case EM_DW: case EM_PW: print_long(out->text, (int16_t)r->v); break;
    case EM_DL: case EM_PL: case EM_I1: print_long(out->text, (int32_t)r->v); break;
    case EM_DQ: case EM_DF: case EM_PQ: case EM_PF:
        snprintf(out->text, EM_TEXT_MAX, "%08X", (unsigned)r->v);
        break;
    case EM_IF: {
        int t;

        switch (r->t) {
        case TB: t = (uint8_t)r->v != 0; break;
        case TW: t = (uint16_t)r->v != 0; break;
        case TF:
            if (!(rules & EM_R_1187)) {
                out->flags |= EM_F_D1;
                strcpy(out->text, "?");
                return;
            }
            t = !ffp_zero(r->v);
            break;
        default: t = r->v != 0; break;
        }
        strcpy(out->text, t ? "T" : "F");
        break;
    }
    default:
        print_typed(e, out->text, r);
        break;
    }
}

void em_expr_eval(const struct em_expr *x, enum em_ctx ctx, uint64_t rules, struct em_exprval *out)
{
    struct ev e = { 0 };
    struct reg r;
    struct trec tr = { 0 };
    int asked = ctx_want(ctx, rules);

    memset(out, 0, sizeof *out);
    out->top.op = -1;
    tr.sig = out->sig;
    tr.cells = out->cells;
    tr.ccap = sizeof out->cells;
    tr.cap = sizeof out->sig;
    e.rules = rules;
    e.cd = (rules & EM_R_CDLIB) != 0;
    memset(&r, 0, sizeof r);
    if (tree_value(&e, x->root, asked, rules, &r, &tr)) {
        strcpy(out->v.text, "!noncompile");
        out->v.flags = e.flags;
    } else if (e.divtrap) {
        strcpy(out->v.text, "!trap");
        out->v.flags = e.flags;
    } else if (e.c5ovf && (rules & EM_R_C5ERR)) {
        strcpy(out->v.text, "!runtime");
        out->v.flags = e.flags;
    } else {
        out->v.flags = e.flags;
        out->v.sig = e.powsig;
        value_text(&e, ctx, rules, asked, &r, &out->v);
    }
    out->top = tr.top;
    if (!tr.nops)
        out->top.op = -1;
    out->nops = tr.nops;
    out->consumer = tr.consumer;
}

/* the cell's expression as a tree (a constant with a sign: NEG over it) */
static void cell_tree(const struct em_set *s, const struct em_cell *c, struct em_expr *x)
{
    struct em_tnode *n;
    int k = 0;

    memset(x, 0, sizeof *x);
    for (int side = 0; side < (c->right < 0 ? 1 : 2); side++) {
        enum em_kind kd = side ? (enum em_kind)c->right : c->left;
        const char *text = side && (c->op == EM_POW || (c->op >= EM_LSL && c->op <= EM_ASR)) ? s->count[kd]
                                                                                            : s->value[kd];
        struct em_tnode *leaf = &x->node[k++];

        leaf->kind = kind_type(kd) ? TN_VAR : TN_CONST;
        leaf->t = kind_type(kd);
        if (leaf->kind == TN_CONST && text[0] == '-') {
            snprintf(leaf->text, sizeof leaf->text, "%s", text + 1);
            n = &x->node[k++];
            n->kind = TN_NEG;
            n->l = leaf;
            leaf = n;
        } else
            snprintf(leaf->text, sizeof leaf->text, "%s", text);
        if (side)
            x->root->r = leaf;
        else
            x->root = leaf;
        if (!side && c->right >= 0) {
            n = &x->node[k++];
            n->kind = TN_BIN;
            n->op = c->op;
            n->l = x->root;
            x->root = n;
        }
    }
    if (c->right < 0) {
        n = &x->node[k++];
        n->kind = c->op == EM_NEG ? TN_NEG : TN_NOT;
        n->l = x->root;
        x->root = n;
    }
    x->n = k;
}

/* expression() for a candidate rule set: the cell as a tree */
static int cand_expression(struct ev *e, const struct em_set *s, const struct em_cell *c, int asked, struct reg *r)
{
    struct em_expr x;

    cell_tree(s, c, &x);
    return tree_value(e, x.root, asked, e->rules, r, NULL);
}

int em_cell_expr(const struct em_matrix *m, const struct em_cell *c, struct em_expr **out)
{
    *out = NULL;
    if (c->left == EM_S || c->right == EM_S)
        return 1;
    *out = malloc(sizeof **out);
    if (!*out)
        return -1;
    cell_tree(&m->sets[c->set], c, *out);
    return 0;
}

void em_cand_cell(uint64_t rules, enum em_op op, int want, int l, int r, struct em_optype *out)
{
    static const enum em_ctx ctxof[] = { EM_PRINT, EM_DB, EM_DW, EM_DL, EM_DQ, EM_DF };
    struct em_expr x;
    struct ev e = { 0 };
    struct reg rg;
    struct trec tr = { 0 };

    memset(&x, 0, sizeof x);
    x.node[0].kind = TN_VAR;
    x.node[0].t = l;
    strcpy(x.node[0].text, "1");
    x.node[1].kind = TN_VAR;
    x.node[1].t = r;
    strcpy(x.node[1].text, op == EM_POW || (op >= EM_LSL && op <= EM_ASR) ? "2" : "1");
    x.node[2].kind = TN_BIN;
    x.node[2].op = op;
    x.node[2].l = &x.node[0];
    x.node[2].r = &x.node[1];
    x.root = &x.node[2];
    e.rules = rules;
    memset(&rg, 0, sizeof rg);
    tr.top.op = -1;
    if (tree_value(&e, x.root, ctx_want(ctxof[want >= 0 && want <= TF ? want : 0], rules), rules, &rg, &tr))
        tr.top.result = 0;
    *out = tr.top;
    if (e.illegal)
        out->result = 0;
}

int em_cand_consttype(uint64_t rules, int want, int partner, const char *text)
{
    struct ev e = { 0 };
    struct reg r;
    int cn = (int)((rules & EM_R_CANDMASK) >> 9);
    int T = cn ? consttarget(cn == EM_CAND_C6 || cn == EM_CAND_C7 ? EM_CAND_C1 : cn, rat_text(text), want, partner)
               : want;

    e.rules = rules;
    memset(&r, 0, sizeof r);
    constant(&e, text, &T, &r);
    return T;
}
