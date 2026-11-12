/*
 * rwe rievallib - RI Eval Lib: the C build or the CD original (BSS2
 * RIEVALLIB.OBJ, variant rwe-1999-eval), one set as LIBSET says, observed
 * as cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_rievallib.txt).
 *
 *   rievallib DIR     (DIR with rievallib .obj/.orig)
 *
 * The string argument is in a buffer (ConvToPostFix lower-cases
 * it), the rest of the buffer 0 or a given tail (the reads behind the
 * end); observed are the string, the output buffer, d0, a3 and the kept
 * registers, and after every PFEvaluate PFErrorType, PFErrorText (at a3)
 * and PFIndirectAddr (an address in the target as its offset).
 *
 * Covered: the start state (no error, no text, indirect address 0);
 * init/finit; ConvToPostFix with every operator and precedence, brackets
 * (too many either way), signs (start, after an operator or "(", after a
 * space), hex/binary/registers/indirection operands, upper case, dropped
 * characters, "." at the end with the scan behind it (BUG(orig) (#631)), 200
 * nested brackets and 150 nested operators (stack chunks); PFEvaluate
 * with every operator (signed/unsigned compares, overflowing products,
 * the division's signs, division by 0, powers 0, 1, 0^n, a 65538
 * exponent), decimal/hex/binary/negative numbers and their quirks,
 * registers d0-d7/a0-a7/pc and odd indexes without and with PFRegisters,
 * a register at the string end (the read behind it), {address} with .b,
 * .w, .l, a bad size, no "}", {register} with a stale d1 and with d1 =
 * "}", too few arguments, 130 values (the byte count), 300 values with
 * operators and pushes across the chunks, an empty string, spaces only
 * (the return address as result, BUG(orig) (#644)), two results.
 * Not covered: an indirection at an odd address (address error).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"

#define LIB "rievallib"
enum { E_CTP, E_EVAL, E_REGS, E_ETYPE, E_ETEXT, E_IADDR, E_FINIT, E_INIT };

#define BIG 1400
static UBYTE in[BIG];                /* the string argument */
static UBYTE out[2 * BIG + 16];      /* ConvToPostFix buffer, PFErrorText */
static ULONG block[260];             /* the register block */
static ULONG target[2];              /* what {address} reads */
static char gen[BIG];                   /* generated strings */
static ULONG D1;                        /* d1 for PFEvaluate (0: the marker) */

/* the string (n bytes, 0s included) into in, the rest 0 */
static void setin(const char *s, ULONG n)
{
    memset(in, 0, BIG);
    memcpy(in, s, n);
}

/* an address in the set's target as an offset, else the value */
static ULONG addr(ULONG a)
{
    ULONG t = (ULONG)target;
    return a >= t && a < t + sizeof target ? a - t + 0x40000000UL : a;
}

static void ctp_n(const char *s, ULONG n)
{
    ROUT = "ConvToPostFix";
    snprintf(VEC, sizeof VEC, "\"%.80s\"", s);
    DO1(ENT(LIB, E_CTP, 0), setin(s, n); memset(out, 0xee, sizeof out);
        R.d[0] = (ULONG)in; R.d[1] = (ULONG)out);
    OBSBYTES("string", in, BIG);
    OBSBYTES("buffer", out, sizeof out);
    OBS("a3", R.a[3] - (ULONG)in);
}

static void ctp(const char *s)
{
    ctp_n(s, strlen(s) + 1);
}

/* PFErrorType, PFErrorText, PFIndirectAddr after a call */
static void state(void)
{
    const char *rout = ROUT;

    ROUT = "PFErrorType";
    DO1(ENT(LIB, E_ETYPE, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "PFErrorText";
    DO1(ENT(LIB, E_ETEXT, 0), memset(out, 0xee, 64); R.a[3] = (ULONG)out + 4);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)out);
    OBSBYTES("text", out, 64);
    ROUT = "PFIndirectAddr";
    DO1(ENT(LIB, E_IADDR, 0), (void)0);
    OBS("d0", addr(R.d[0]));
    ROUT = rout;
}

/* PFEvaluate on src (n bytes); `plain`: src is the vector's string itself
   (else it holds the target's address, and the string is not observed) */
static void ev_one(const char *name, const char *src, ULONG n, int plain)
{
    ROUT = "PFEvaluate";
    snprintf(VEC, sizeof VEC, "\"%.80s\"%s", name, D1 ? " d1 '}'" : "");
    DO1(ENT(LIB, E_EVAL, 0), setin(src, n);
        R.d[0] = (ULONG)in; if (D1) R.d[1] = D1);
    /* the result; spaces only give the return address (BUG(orig) #644) */
    OBSPTR("d0", addr(R.d[0]), NULL, 0);
    if (plain)
        OBSBYTES("string", in, BIG);
    state();
}

/* PFEvaluate on n bytes of s */
static void ev_n(const char *s, ULONG n)
{
    ev_one(s, s, n, 1);
}

/* PFEvaluate on s with %T = the address of the target (decimal), %H the
   same in hex (8 digits) */
static void ev(const char *s)
{
    static char buf[120];
    const char *p;
    char *q = buf;

    if (!strchr(s, '%') || strlen(s) > 60) {
        ev_n(s, strlen(s) + 1);
        return;
    }
    for (p = s; *p; p++)
        if (p[0] == '%' && (p[1] == 'T' || p[1] == 'H')) {
            q += sprintf(q, p[1] == 'T' ? "%lu" : "%08lx", (ULONG)target);
            p++;
        } else
            *q++ = *p;
    *q = 0;
    ev_one(s, buf, strlen(buf) + 1, 0);
}

/* gen = n times `item` then m times `op` */
static const char *repeat(int n, const char *item, int m, const char *op)
{
    char *q = gen;
    int i;
    for (i = 0; i < n; i++)
        q += sprintf(q, "%s", item);
    for (i = 0; i < m; i++)
        q += sprintf(q, "%s", op);
    *q = 0;
    return gen;
}

static void convtopostfix(void)
{
    static const char *const v[] = {
        "2+6", "1+2*3", "(1+2)*3", "1*2+3", "2^3^2", "10/3-1", "1-2-3", "1=1&2|3",
        "1<2>3", "1<2+3*4^5", "8/(4/2)", "-3+4", "3 -4", "3- -4", "3-(-4)", "-(-1)",
        "(-1)", "4*-2", "3 - 4", "2-(3)", "$FF&%1010|7", "A0+D1*PC", "a7-d0", "{1000}.w+2",
        "{a0}.b*{$10}.l", "abcdef+1", "  12x+3", "1+2)", "((1+2)", ")(", "1+2)*3",
        "", "   ", "-", "*", "3-", "1++2", "1 2 3", "1,2;3", "Zz+yY", "c+p+$",
        "9.9+1", "%101.", "(", ")",
    };
    unsigned i;

    for (i = 0; i < sizeof v / sizeof v[0]; i++)
        ctp(v[i]);
    /* "." at the end: the 0 is copied, the scan goes on behind it */
    ctp_n("1.\0(((2\0", 9);
    ctp_n("x+a.\0-5*(3\0", 12);
    /* nested: 200 brackets, 150 operators deep (stack chunks) */
    memset(gen, '(', 200);
    gen[200] = '1';
    memset(gen + 201, ')', 200);
    gen[401] = 0;
    ctp(gen);
    ctp(repeat(150, "1^(", 0, ""));
    ctp(repeat(150, "2-3*(", 150, ")"));
    ctp(repeat(300, "1+", 1, "1"));
    ctp(repeat(200, "(1+", 0, ""));
}

static void evaluate(void)
{
    static const char *const v[] = {
        /* the operators */
        "2 6 +", "1 2 3 * +", "10 3 -", "3 10 -", "5 5 =", "5 6 =", "12 10 &", "12 3 |",
        "1 2 <", "2 1 <", "2 2 <", "-1 1 <", "1 -1 <", "1 2 >", "2 1 >", "-1 1 >",
        "1 -1 >", "7 2 /", "-7 2 /", "7 -2 /", "-7 -2 /", "100 7 /", "0 5 /", "5 0 /",
        "0 0 /", "1000000 3 /", "2147483647 1 /", "-2147483648 1 /", "-2147483648 -1 /",
        "6 7 *", "-6 7 *", "6 -7 *", "-6 -7 *", "65536 3 *", "70000 70000 *",
        "100000 100000 *", "-65536 65536 *", "123456 789 *", "2 10 ^", "3 0 ^", "3 1 ^",
        "0 5 ^", "-3 3 ^", "2 31 ^", "2 -1 ^", "3 65538 ^", "7 1 2 + *",
        /* numbers */
        "$ff $10 +", "$FF", "$7fffffff", "$123456789", "%1010 1 +", "%2", "$g", "-5",
        "-$10", "-%1", "12x", "4294967295", "99999999999", "- 3", "3 -", "-", "+",
        "1 +", "1 2", "1 2 3 + ", "  42  ", "", "   ", "x", "(1 2 +)", "1 2 + 3",
        /* registers without a block */
        "d0", "1 pc +",
    };
    static const char *const r[] = {
        "d0", "d1 d2 +", "d7", "a0", "a7", "pc", "d9", "az", "a", "d", "p", "pz",
        "-d3", "d1 -a2 *", "{1000}", "{%T}", "{%T}.b", "{%T}.w", "{%T}.l", "{%T}.x",
        "{%T}.", "{%T", "{$%H}", "{$%H}.w", "{%T} {d0}", "{d0}", "{d0}.w", "{a0}",
        "-{1000}", "1 {%T}.b +", "1 {%T}.b",
    };
    unsigned i;

    for (i = 0; i < sizeof v / sizeof v[0]; i++)
        ev(v[i]);

    ROUT = "PFRegisters";
    strcpy(VEC, "block");
    DO1(ENT(LIB, E_REGS, 0), R.d[0] = (ULONG)block);
    for (i = 0; i < sizeof r / sizeof r[0]; i++)
        ev(r[i]);
    /* {register} with d1 = '}' (the stale d1 the check sees) */
    D1 = 0x5a01127d;
    ev("{d0}");
    ev("{d0}.w");
    ev("{a1}");
    ev("5 {d0} +");
    D1 = 0;
    /* a register at the end: index (0 - '0' + 8) * 4, then behind the 0 */
    ev_n("1 a\0" "9 +\0", 9);
    ev_n("d\0" "\0", 4);
    /* the value count is a byte: 130 values, 300 values */
    ev(repeat(130, "1 ", 1, "+"));
    ev(repeat(129, "1 ", 2, "+ "));
    ev(repeat(300, "2 ", 40, "+ "));
    ev(repeat(300, "1 ", 50, "+ "));
    {
        char *q = gen;
        int k;
        for (k = 0; k < 300; k++)
            q += sprintf(q, "2 ");
        for (k = 0; k < 40; k++)
            q += sprintf(q, "+ ");
        for (k = 0; k < 50; k++)
            q += sprintf(q, "%d ", k + 1);
        for (k = 0; k < 50; k++)
            q += sprintf(q, "* ");
        *q = 0;
    }
    ev(gen);
    /* deep but right: 1 2 3 ... + + + */
    {
        char *q = gen;
        int k;
        for (k = 1; k <= 120; k++)
            q += sprintf(q, "%d ", k);
        for (k = 1; k < 120; k++)
            q += sprintf(q, "+ ");
        *q = 0;
    }
    ev(gen);

    ROUT = "PFRegisters";
    strcpy(VEC, "0");
    DO1(ENT(LIB, E_REGS, 0), R.d[0] = 0);
    ev("d0");
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int i;

    if (argc < 2) {
        printf("usage: rievallib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    KEEPS("ConvToPostFix", d3|d5|d6|a6);
    KEEPS("PFEvaluate", d4-d7|a3|a6);
    KEEPS("PFRegisters", d0-d7|a0-a3|a6);
    KEEPS("PFErrorType", d1-d7|a0-a3|a6);
    KEEPS("PFErrorText", d1-d7|a1|a2|a6);
    KEEPS("PFIndirectAddr", d1-d7|a0-a3|a6);
    for (i = 0; i < 260; i++)
        block[i] = 0x01000000UL * (i & 0x7f) + i * 7 + 3;
    block[0] = (ULONG)target;                   /* d0 */
    block[8] = (ULONG)&target[1];               /* a0 */
    block[9] = (ULONG)target + 2;               /* a1 */
    target[0] = 0x89abcdefUL;
    target[1] = 0x12345678UL;
    /* the results (d0) are observed by the tests; PFEvaluate's d0 is the
       input string by chance only */
    DONTCARE = 1;

    strcpy(VEC, "start");
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    state();
    ROUT = "finit";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);

    convtopostfix();
    evaluate();
    return rwe_done(LIB);
}
