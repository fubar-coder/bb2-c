/*
 * systest_mathstr - the arithmetic and string system libraries
 *   strings2lib, ffplib, strcomplib, floatquicklib, modlib, maxlenlib
 * (plus memlib, ldivlib, lmullib, which they call) on the Amiga: one set
 * (the original from acidlibs or the C build, as LIBSET says), observed
 * as cases (sysload.h; the expected data
 * libs/tests/expected/systest_mathstr.txt).
 *
 *   systest_mathstr DIR
 *
 * Every routine is called with the inputs of the vector (all other
 * registers filled with markers); written are the results (d0, CCR where
 * it is a result, a3, the bytes the routine removes from the stack,
 * memory contents) and the kept registers of every call (KEEPS: the set
 * the original keeps in all calls of the routine).  Many vectors: edge
 * values and pseudo random ones (fixed seed).
 */
#include "sysload.h"

/* ------------------------------------------------- call with stack args */

/* callstk(fn, r, args, nargs, popped): push args[0..nargs-1] (args[0]
   first, i.e. deepest), load d0-d7/a0-a6 from r, call fn, store all
   registers back.  *popped = bytes the routine removed from the stack
   itself.  Returns the CCR (N=8, Z=4, V=2, C=1).  The stack pointer is
   restored from the frame, so a wrong pop cannot crash the test. */
ULONG stk_frame, stk_regs, stk_args;
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _callstk\n"
"_callstk:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"    /* 44: fn 48, r 52, args 56, nargs 60, popped 64 */
"   move.l  sp,_stk_frame\n"
"   move.l  52(sp),_stk_regs\n"
"   move.l  56(sp),a0\n"
"   move.l  60(sp),d0\n"
"   bra.s   1f\n"
"0: move.l  (a0)+,-(sp)\n"
"1: dbf     d0,0b\n"
"   move.l  sp,_stk_args\n"
"   pea     2f\n"
"   move.l  _stk_frame,a0\n"
"   move.l  48(a0),-(sp)\n"
"   move.l  52(a0),a1\n"
"   movem.l (a1),d0-d7/a0-a6\n"
"   rts\n"
"2: scs     -(sp)\n"
"   seq     -(sp)\n"
"   smi     -(sp)\n"
"   svs     -(sp)\n"
"   move.l  a6,-(sp)\n"
"   move.l  _stk_regs,a6\n"
"   movem.l d0-d7/a0-a5,(a6)\n"
"   move.l  (sp)+,56(a6)\n"
"   moveq   #0,d0\n"
"   tst.b   (sp)+\n"
"   beq.s   3f\n"
"   or.b    #2,d0\n"
"3: tst.b   (sp)+\n"
"   beq.s   4f\n"
"   or.b    #8,d0\n"
"4: tst.b   (sp)+\n"
"   beq.s   5f\n"
"   or.b    #4,d0\n"
"5: tst.b   (sp)+\n"
"   beq.s   6f\n"
"   or.b    #1,d0\n"
"6: move.l  sp,d1\n"
"   sub.l   _stk_args,d1\n"
"   move.l  _stk_frame,sp\n"
"   move.l  64(sp),a0\n"
"   move.l  d1,(a0)\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);

/* error check routine (first !subs field) of entry idx */
static UBYTE *entry_err1(struct lib *l, int idx)
{
    UBYTE *p = l->code + 0x30;
    int i = 0;
    while (!(W(p) & 0x8000)) {
        p += 6;
        while (W(p)) p += 4;
        p += 2;
        if (i == idx)
            return (UBYTE *)L(p);
        p += 12;
        i++;
    }
    return NULL;
}

/* ------------------------------------------------------------ framework */

/* the call of the case (ROUT and VEC are sysload.h's key parts) */
static struct regs R, IN;
static ULONG ARGS[4], POP;
static int NARGS;
static UWORD CCR;

#define E(lib, i) entry_code(getlib(&loaded, lib), i)

/* call fn; `setup` may use R, ARGS, NARGS.  Every value of this program
   is a number (a result, a flag, an offset), written with OBS as it is */
#define DO1(fn, setup) do {                                                 \
        markregs(&R); NARGS = 0;                                            \
        setup;                                                              \
        IN = R;                                                             \
        CCR = callstk(fn, &R, ARGS, NARGS, &POP);                           \
        preserve();                                                         \
    } while (0)

/* the kept case of the call and the bytes it removed from the stack */
static void preserve(void)
{
    obs_kept(&IN, &R);
    OBS("popped", POP);
}

static ULONG seed = 0x12345678;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* integer -> FFP bit pattern (mantissa truncated) for test vectors */
static ULONG int2ffp(LONG v)
{
    ULONG m, sign = 0;
    int e = 32;
    if (!v)
        return 0;
    if (v < 0) {
        sign = 0x80;
        m = -(ULONG)v;
    } else
        m = v;
    while (!(m & 0x80000000UL)) {
        m <<= 1;
        e--;
    }
    return (m & 0xffffff00UL) | sign | (0x40 + e);
}

/* ------------------------------------------------------------- ffplib */

static ULONG ffpbase, membase;

static void test_ffplib_init(void)
{
    struct Library *b;
    UWORD cnt;
    ROUT = "ffplib init";
    strcpy(VEC, "");
    b = OpenLibrary((STRPTR)"mathffp.library", 0);
    cnt = b->lib_OpenCnt;
    DO1(E("ffplib", 1), (void)0);
    ffpbase = R.d[0];
    OBS("base - mathffp", R.d[0] - (ULONG)b);    /* not the address itself */
    OBS("is mathffp", R.d[0] == (ULONG)b);
    check("ffplib init: open count +1", b->lib_OpenCnt == cnt + 1);
    CloseLibrary(b);
}

static void test_ffplib_finit(void)
{
    struct Library *b = (struct Library *)ffpbase;
    UWORD cnt = b->lib_OpenCnt;
    ROUT = "ffplib finit";
    DO1(E("ffplib", 2), R.a[1] = ffpbase);
    check("ffplib finit: open count -1", b->lib_OpenCnt == cnt - 1);
}

/* ------------------------------------------------------------- modlib */

static const WORD wvals[] = {
    0, 1, -1, 2, -2, 3, -3, 5, -5, 7, -7, 10, -10, 13, 100, -100, 127, -128,
    255, -255, 256, 1000, -1000, 12345, -12345, 32767, -32767, -32768
};
#define NW (sizeof wvals / sizeof wvals[0])

static const LONG lvals[] = {
    0, 1, -1, 2, -2, 3, -3, 7, -7, 10, -10, 100, -100, 255, 65535, 65536,
    -65536, 46341, 1000000, -1000000, 123456789, -123456789, 0x7fffffff,
    -0x7fffffff, (LONG)0x80000000UL, (LONG)0x80000001UL, 0x10000001, -3000000
};
#define NL (sizeof lvals / sizeof lvals[0])

static void test_modlib(void)
{
    unsigned i, j;
    static const char *names[] = { "domodbyte", "domodword" };
    int e;

    /* byte/word: d0.w MOD d1.w, high words garbage */
    for (e = 0; e < 2; e++) {
        ROUT = names[e];
        for (i = 0; i < NW; i++)
            for (j = 0; j < NW; j++) {
                ULONG a, b;
                if (!wvals[j])
                    continue;           /* DIVS by 0: exception */
                a = (rnd() & 0xffff0000UL) | (UWORD)wvals[i];
                b = (rnd() & 0xffff0000UL) | (UWORD)wvals[j];
                sprintf(VEC, "%ld,%ld", (LONG)wvals[i], (LONG)wvals[j]);
                DO1(E("modlib", e), R.d[0] = a; R.d[1] = b);
                OBS("d0", R.d[0]);
            }
        for (i = 0; i < 1500; i++) {
            ULONG a = rnd(), b = rnd();
            if (!(UWORD)b)
                b |= 1;
            if (i & 1)
                b = (b & 0xffff0000UL) | (UWORD)(WORD)((b & 0xf) - 8 ? (b & 0xf) - 8 : 3);
            sprintf(VEC, "$%08lX,$%08lX", a, b);
            DO1(E("modlib", e), R.d[0] = a; R.d[1] = b);
            OBS("d0", R.d[0]);
        }
    }

    /* quick: integer parts in the high words */
    ROUT = "domodquick";
    for (i = 0; i < NW; i++)
        for (j = 0; j < NW; j++) {
            ULONG a, b;
            if (!wvals[j])
                continue;
            a = (ULONG)(UWORD)wvals[i] << 16 | (rnd() & 0xffff);
            b = (ULONG)(UWORD)wvals[j] << 16 | (rnd() & 0xffff);
            sprintf(VEC, "$%08lX,$%08lX", a, b);
            DO1(E("modlib", 3), R.d[0] = a; R.d[1] = b);
            OBS("d0", R.d[0]);
        }
    for (i = 0; i < 1000; i++) {
        ULONG a = rnd(), b = rnd();
        if (!(b >> 16))
            b |= 0x10000;
        sprintf(VEC, "$%08lX,$%08lX", a, b);
        DO1(E("modlib", 3), R.d[0] = a; R.d[1] = b);
        OBS("d0", R.d[0]);
    }

    /* long (b = 0 is fine: ldivide gives $7FFFFFFF) */
    ROUT = "domodlong";
    for (i = 0; i < NL; i++)
        for (j = 0; j < NL; j++) {
            sprintf(VEC, "%ld,%ld", lvals[i], lvals[j]);
            DO1(E("modlib", 2), R.d[0] = lvals[i]; R.d[1] = lvals[j]);
            OBS("d0", R.d[0]);
        }
    for (i = 0; i < 2000; i++) {
        ULONG a = rnd(), b = rnd();
        if (i & 1)
            b >>= (rnd() & 31);
        if (i & 2)
            b = -b;
        sprintf(VEC, "$%08lX,$%08lX", a, b);
        DO1(E("modlib", 2), R.d[0] = a; R.d[1] = b);
        OBS("d0", R.d[0]);
    }

    /* float: SPFix of both, then long */
    ROUT = "domodfloat";
    for (i = 0; i < NL; i++)
        for (j = 0; j < NL; j++) {
            ULONG a = int2ffp(lvals[i]), b = int2ffp(lvals[j]);
            sprintf(VEC, "%ld,%ld", lvals[i], lvals[j]);
            DO1(E("modlib", 4), R.d[0] = a; R.d[1] = b;
                R.a[6] = ffpbase);
            OBS("d0", R.d[0]);
        }
    for (i = 0; i < 1000; i++) {
        /* fractional values: random mantissa, exponent 0..40 */
        ULONG a = (rnd() | 0x80000000UL) & 0xffffff00UL, b = (rnd() | 0x80000000UL) & 0xffffff00UL;
        a |= (rnd() & 0x80) | (0x40 + (rnd() % 41));
        b |= (rnd() & 0x80) | (0x40 + (rnd() % 41));
        if (i % 7 == 0)
            b = rnd();                  /* any pattern */
        sprintf(VEC, "$%08lX,$%08lX", a, b);
        DO1(E("modlib", 4), R.d[0] = a; R.d[1] = b;
            R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
    }
}

/* ------------------------------------------------------- floatquicklib */

static void test_floatquick(void)
{
    static const ULONG qv[] = {
        0, 1, 0xffffffff, 0x8000, 0xffff8000, 0x10000, 0xffff0000, 0x18000,
        0xfffe8000, 0x7fffffff, 0x80000000, 0x80000001, 0xffff, 0x10001,
        0x640000, 0xff9c0000, 0x7fff0000, 0x80010000, 0x00004000, 0x0000c000,
        0x12345678, 0xedcba988, 0x00000003, 0xfffffffd, 0x7fff8000
    };
    static const ULONG fv[] = {
        0, 0x80, 0x80000041, 0x800000c1, 0x80000040, 0x800000c0, 0xc0000041,
        0xc00000c1, 0x80000050, 0x800000d0, 0x80000051, 0x800000d1,
        0xffffff50, 0xffffffd0, 0xffffff4f, 0x80000030, 0x800000b0,
        0x80000001, 0xffffff7f, 0xffffffff, 0x41, 0xc1, 0xaaaaaa45, 0x55555545,
        0x80000031, 0xc90fdb42, 0xc90fdbc2, 0x80000052, 0x80000060
    };
    static ULONG got[3000];
    unsigned i, ng = 0;

    ROUT = "quicktofloat";
    for (i = 0; i < sizeof qv / sizeof qv[0] + 2000; i++) {
        ULONG q = i < sizeof qv / sizeof qv[0] ? qv[i] : rnd();
        if (i >= sizeof qv / sizeof qv[0] && (i & 1))
            q = (LONG)q >> (rnd() & 15);
        sprintf(VEC, "$%08lX", q);
        DO1(E("floatquicklib", 0), R.d[0] = q; R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
        if (ng < 3000)
            got[ng++] = R.d[0];
    }

    ROUT = "floattoquick";
    for (i = 0; i < sizeof fv / sizeof fv[0]; i++) {
        sprintf(VEC, "$%08lX", fv[i]);
        DO1(E("floatquicklib", 1), R.d[0] = fv[i]; R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
    }
    for (i = 0; i < NL; i++) {
        ULONG f = int2ffp(lvals[i]);
        sprintf(VEC, "%ld", lvals[i]);
        DO1(E("floatquicklib", 1), R.d[0] = f; R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
    }
    /* round trip: the results of quicktofloat (the vector is the number
       of that result: a result is no part of a key) */
    for (i = 0; i < ng; i++) {
        sprintf(VEC, "rt %u", i);
        DO1(E("floatquicklib", 1), R.d[0] = got[i]; R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
    }
    for (i = 0; i < 2000; i++) {
        ULONG f = rnd();
        if (i & 1)                      /* normalised, exponent near range */
            f = ((f | 0x80000000UL) & 0xffffff80UL) | (0x30 + (rnd() % 0x24));
        sprintf(VEC, "$%08lX", f);
        DO1(E("floatquicklib", 1), R.d[0] = f; R.a[6] = ffpbase);
        OBS("d0", R.d[0]);
    }
}

/* ---------------------------------------------------------- strcomplib */

struct str { const char *s; ULONG len; };
static const struct str strs[] = {
    { "", 0 }, { "a", 1 }, { "b", 1 }, { "ab", 2 }, { "abc", 3 }, { "abd", 3 },
    { "abc\0", 4 }, { "ABC", 3 }, { "\xff", 1 }, { "\x80", 1 }, { "\x7f", 1 },
    { "a\0b", 3 }, { "a\0c", 3 }, { "zzzzzzzz", 8 }, { "hello world", 11 },
    { "hello worle", 11 }, { "hello", 5 }, { "\xff\xff", 2 }, { "\0", 1 }
};
#define NS (sizeof strs / sizeof strs[0])

static UBYTE workbuf[256];
static ULONG selbuf[8];

static void test_strcomp(void)
{
    unsigned i, j;

    ROUT = "strcomp2";
    for (i = 0; i < NS; i++)
        for (j = 0; j < NS; j++) {
            sprintf(VEC, "%u,%u", i, j);
            DO1(E("strcomplib", 2), R.a[0] = (ULONG)strs[i].s; R.d[0] = strs[i].len;
                R.a[1] = (ULONG)strs[j].s; R.d[1] = strs[j].len);
            OBS("d0", R.d[0]);
            OBS("ccr", CCR);
        }
    /* odd lengths: negative / huge counts (signed BMI loop) */
    {
        static const ULONG ln[] = { 0, 1, 5, 16, 0xffffffff, 0xfffffffe,
                                    0x80000000, 0x80000001 };
        static const UBYTE b1[17] = "0123456789abcdef", b2[17] = "0123456789abcdeX";
        static const UBYTE *bs[3] = { b1, b2, b1 };
        int k;
        for (k = 0; k < 2; k++)
            for (i = 0; i < 8; i++)
                for (j = 0; j < 8; j++) {
                    if (i >= 6 && j >= 6)
                        continue;       /* both endless */
                    if (k == 0 && (i >= 6 || j >= 6))
                        continue;       /* equal strings: would run past */
                    sprintf(VEC, "len $%lX,$%lX k%d", ln[i], ln[j], k);
                    DO1(E("strcomplib", 2), R.a[0] = (ULONG)bs[k + 1];
                        R.d[0] = ln[i]; R.a[1] = (ULONG)bs[k];
                        R.d[1] = ln[j]);
                    OBS("d0", R.d[0]);
                    OBS("ccr", CCR);
                }
    }

    /* strcomp: left then right string at the end of the work area (a3) */
    ROUT = "strcomp";
    for (i = 0; i < NS; i++)
        for (j = 0; j < NS; j++) {
            sprintf(VEC, "%u,%u", i, j);
            DO1(E("strcomplib", 0),
                memset(workbuf, 0x55, sizeof workbuf);
                memcpy(workbuf + 4, strs[i].s, strs[i].len);
                memcpy(workbuf + 4 + strs[i].len, strs[j].s, strs[j].len);
                R.a[3] = (ULONG)workbuf + 4 + strs[i].len + strs[j].len;
                ARGS[0] = strs[i].len; ARGS[1] = strs[j].len; NARGS = 2);
            OBS("ccr", CCR);
            OBS("a3", R.a[3] - (ULONG)workbuf);
        }

    /* casestrcomp: d0 = CASE string, 4(a7) its length, 8(a7) SELECT */
    ROUT = "casestrcomp";
    for (i = 0; i < NS; i++)
        for (j = 0; j < NS; j++) {
            sprintf(VEC, "%u,%u", i, j);
            DO1(E("strcomplib", 1),
                selbuf[0] = strs[j].len;
                memcpy(&selbuf[1], strs[j].s, strs[j].len);
                R.d[0] = (ULONG)strs[i].s;
                ARGS[0] = (ULONG)&selbuf[1]; ARGS[1] = strs[i].len; NARGS = 2);
            OBS("d0", R.d[0]);
            OBS("ccr", CCR);
        }
}

/* ----------------------------------------------- strings2lib/maxlenlib */

static UBYTE src[1200];
static ULONG var[4];                    /* string variables */

static ULONG hdr(int v, int k)         /* -8 alloc'd, -4 len */
{
    return var[v] ? ((ULONG *)var[v])[k - 2] : 0xdeadbeef;
}

/* compare the string in var v with src+off, len */
static ULONG content_ok(int v, ULONG off, ULONG len)
{
    UBYTE *p = (UBYTE *)var[v];
    return p && memcmp(p, src + off, len) == 0 && p[len] == 0 && hdr(v, 1) == len;
}

static void assign(int global, int v, ULONG off, ULONG len)
{
    ULONG old, fits;
    ROUT = global ? "gstringass" : "stringass";
    sprintf(VEC, "var%d len %lu", v, len);
    old = var[v], fits = old && len <= hdr(v, 0);
    DO1(E("strings2lib", global),
        R.a[2] = (ULONG)&var[v]; R.a[3] = (ULONG)src + off;
        R.a[6] = membase; ARGS[0] = len; NARGS = 1);
    OBS("a3 advanced", R.a[3] - (ULONG)src - off);
    /* memory kept if the string fits (a new block may land at the same
       address, so only this direction is checked) */
    OBS("fits", fits);
    OBS("reused", !fits || var[v] == old);
    OBS("content", content_ok(v, off, len));
    OBS("alloc'd", hdr(v, 0));
    OBS("len", hdr(v, 1));
    OBS("a6", R.a[6] - membase);
}

static void maxlen(int global, int v, ULONG n)
{
    ROUT = global ? "domaxlen" : "domaxlenl";
    sprintf(VEC, "var%d maxlen %lu", v, n);
    DO1(E("maxlenlib", global ? 0 : 1),
        R.d[0] = (ULONG)&var[v]; R.d[1] = n; R.a[6] = membase);
    OBS("d0 = var", R.d[0] == var[v]);
    OBS("var set", var[v] != 0);
    OBS("alloc'd", hdr(v, 0));
    OBS("len", hdr(v, 1));
    OBS("empty", var[v] ? *(UBYTE *)var[v] : 0x1ff);
    OBS("a6", R.a[6] - membase);
}

static void test_strings(void)
{
    static const ULONG lens[] = { 5, 3, 0, 20, 20, 8, 7, 9, 16, 17, 1, 64, 255,
                                  1000, 2, 0, 0, 33, 24, 25, 15, 1100, 1 };
    unsigned i, k;
    int g;

    for (i = 0; i < sizeof src; i++)
        src[i] = (UBYTE)(i * 7 + 3);

    for (g = 0; g < 2; g++) {
        for (i = 0; i < sizeof lens / sizeof lens[0]; i++)
            assign(g, g, i * 3, lens[i]);
        /* string from scratch with length 0 */
        assign(g, 2 + g, 0, 0);
    }
    /* MaxLen, then strings into the buffer */
    for (g = 0; g < 2; g++) {
        int v = g ? 1 : 0;
        maxlen(g, v, 100);
        assign(g, v, 1, 50);
        assign(g, v, 2, 100);
        assign(g, v, 3, 101);
        maxlen(g, v, 0);
        maxlen(g, v, 0);
        maxlen(g, v, 7);
        assign(g, v, 4, 7);
        maxlen(g, v, 1);
        maxlen(g, v, 300);
        assign(g, v, 4, 0);
        for (k = 0; k < 20; k++) {
            ULONG n = rnd() % 600, l = rnd() % 600;
            maxlen(g, v, n);
            assign(g, v, k, l);
        }
    }

    /* error check: d1 >= 0 only (d1 < 0 is TRAP #0, the debugger) */
    ROUT = "maxlen echk1";
    {
        static const ULONG ns[] = { 0, 1, 100, 0x7fffffff };
        for (i = 0; i < 4; i++) {
            sprintf(VEC, "%lu", ns[i]);
            DO1(entry_err1(getlib(&loaded, "maxlenlib"), i & 1),
                R.d[0] = (ULONG)&var[3]; R.d[1] = ns[i]);
        }
    }
}

/* ------------------------------------------------------------------ main */

static const char *names[] = {
    "memlib", "ldivlib", "lmullib", "ffplib", "strings2lib", "strcomplib",
    "floatquicklib", "modlib", "maxlenlib", NULL
};

int main(int argc, char **argv)
{
    ULONG avail;

    if (argc < 2) {
        printf("usage: systest_mathstr DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("ffplib init", d2-d7|a2|a3);
    KEEPS("domodbyte", d1-d7|a0-a3|a6);
    KEEPS("domodword", d1-d7|a0-a3|a6);
    KEEPS("domodquick", d2-d7|a0-a3|a6);
    KEEPS("domodlong", d2-d7|a0-a3|a6);
    KEEPS("domodfloat", d2-d7|a0-a3|a6);
    KEEPS("quicktofloat", d2-d7|a0-a3|a6);
    KEEPS("floattoquick", d2-d7|a0-a3|a6);
    KEEPS("strcomp2", d2-d7|a2|a3|a6);
    KEEPS("strcomp", d0-d7|a2|a6);
    KEEPS("casestrcomp", d2-d7|a2|a3|a6);
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("stringass", d1-d7|a0|a2|a6);
    KEEPS("gstringass", d1-d7|a0|a2);
    KEEPS("domaxlen", d3-d7|a3);
    KEEPS("domaxlenl", d3-d7|a3|a6);
    KEEPS("maxlen echk1", d0-d7|a0-a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    KEEPS("ffplib finit", d2-d7|a1-a3);

    test_ffplib_init();
    test_modlib();
    test_floatquick();
    test_strcomp();

    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib init";
    DO1(E("memlib", 7), (void)0);
    membase = R.d[0];
    test_strings();
    ROUT = "memlib finit";
    DO1(E("memlib", 8), (void)0);
    check("strings: all memory back", AvailMem(MEMF_ANY) == avail);

    test_ffplib_finit();

    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
