/*
 * libtest - the routines of one build of a Blitz library on the Amiga:
 * the original (extracted from blitz2:acidlibs) or the C build, as the
 * run's LIBSET says (libs_tests.cpp).  The file is loaded with LoadSeg;
 * the code addresses come from the relocated !subs fields of the token
 * table.  Every nullsub/form routine is called with the same register
 * inputs and its d0 is written as a case (capture.h): the key is the
 * routine, "d0" and the inputs; the expected data
 * (libs/tests/expected/libtest_NAME.txt) holds them as block cases
 * (design D6 of the openspec change libs-tests-capture: 41 458 random
 * 32-bit results are as hex nobody reads, and nearly incompressible).
 *
 *   libtest LIB DIR NAME [ROUNDS]     the cases to DIR/NAME.txt
 *
 * Inputs: all pairs of a list of edge values in d0/d1, plus ROUNDS
 * (default 20000) pseudo random pairs (fixed seed, reproducible), as
 * block cases; for ldivlib then the limits of its paths as plain cases
 * (check "limit d0") and the steps of its normalisation ("norm d0").
 * Output: one line per routine; exit 0 if the library loads and its
 * cases are written.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "capture.h"

/* registers for callregs() - layout of movem.l d0-d7/a0-a6 */
struct regs { ULONG d[8]; ULONG a[7]; };
/* the shared register call thunk (callregs.c): loads d0-d7/a0-a6, calls,
   stores them back.  Library routines may change every register except
   a4/a5/a7. */
extern UWORD callregs(APTR fn, struct regs *r, ULONG arg, ULONG pusharg);

#define MAXR 64

struct lib {
    BPTR seg;
    UBYTE *base;                /* hunk start = library header */
    int n;                      /* routines found */
    APTR code[MAXR];            /* routine address per entry/form */
    char what[MAXR][24];
    ULONG lost[MAXR];           /* compilerlib: the registers changed */
};

static UWORD W(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static ULONG L(const UBYTE *p) { return (ULONG)W(p) << 16 | W(p + 2); }
static const UBYTE *even(const UBYTE *p) { return (const UBYTE *)(((ULONG)p + 1) & ~1UL); }

/* collect the code addresses from the token table (layout: blitz2.s fixlibs);
   only nullsubs and statement/function forms have code */
static int scan(struct lib *l)
{
    const UBYTE *p = l->base + 0x30;
    UWORD first = W(l->base + 0x0e);
    int tok = 0;

    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        p += 6;
        if (kind == 8 || (kind & 4))
            return -1;          /* names / OS calls: not needed here */
        if ((kind & 0xf) == 0) {                    /* nullsub */
            while (W(p)) p += 4;
            p += 2;
            if (l->n < MAXR && L(p + 4)) {
                l->code[l->n] = (APTR)L(p + 4);
                sprintf(l->what[l->n++], "$%04X", first + tok);
            }
            p += 12;
        } else {
            int form = 0;
            while (W(p) != 0xffff) {
                p = even(p + 2 + (W(p) & 0xff));
                while (W(p)) p += 4;
                p += 2;
                if (l->n < MAXR && L(p + 4)) {
                    l->code[l->n] = (APTR)L(p + 4);
                    sprintf(l->what[l->n++], "#%d form %d", tok, form);
                }
                p += 12;
                form++;
            }
            p += 2 + 6;
            while (*p++) ;
            while (*p++) ;
            p = even(p);
        }
        tok++;
    }
    return l->n;
}

static int load(struct lib *l, const char *name)
{
    l->seg = LoadSeg((STRPTR)name);
    if (!l->seg) {
        printf("cannot LoadSeg %s\n", name);
        return 0;
    }
    l->base = (UBYTE *)BADDR(l->seg) + 4;
    l->n = 0;
    if (scan(l) <= 0) {
        printf("%s: no routines found\n", name);
        UnLoadSeg(l->seg);
        return 0;
    }
    return 1;
}

static const ULONG edges[] = {
    0, 1, 2, 3, 7, 10, 255, 256, 0x7fff, 0x8000, 0xffff, 0x10000, 0x10001,
    0x12345, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff, 0xfffffffe,
    0xffff0000, 0x8000ffff, 0x00018000, 0x40000000, 0x3fffffff, 1000000,
    0xfff0bdc0, 0x00010000 * 3 / 2,
};
#define NEDGES (sizeof edges / sizeof edges[0])

/* ldivlib (plain cases after the blocks, every sign): the divisors at
   the limits of its paths - DIVU for a divisor below $10000 (Long; Quick
   below 1.0, $4000 = 0.25, $C000 = 0.75), DIVU by the high word for a
   Quick divisor without a fraction (3.0, 10.0, $7FFF0000), the loop for
   the others (3.25, $10001, $1FFFF, $7FFFFFFF, $80000000) - and Quick
   fractions in the dividends (12.5 = $C8000, $7FFF8000) */
static const ULONG dividends[] = {
    0, 1, 0xffff, 0x12345678, 0x7fffffff, 0x80000000, 0x000c8000,
    0x7fff8000,
};
#define NDIVIDENDS (sizeof dividends / sizeof dividends[0])
static const ULONG divisors[] = {
    1, 3, 0x7fff, 0x8000, 0xffff, 0x10000, 0x10001, 0x1ffff, 0x4000,
    0xc000, 0x30000, 0x34000, 0xa0000, 0x7fff0000, 0x7fffffff, 0x80000000,
};
#define NDIVISORS (sizeof divisors / sizeof divisors[0])

static ULONG seed;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    ULONG hi = seed >> 16;
    seed = seed * 1103515245UL + 12345;
    ULONG v = hi << 16 | (seed >> 16);
    switch (v & 3) {            /* mix in small and medium numbers */
    case 0: return v;
    case 1: return (LONG)v >> 12;
    case 2: return (LONG)v >> 20;
    default: return (LONG)v >> 26;
    }
}

/* compilerlib (#1178): the registers d2-d7, a0-a3 and a6 get marks before
   each call, and the ones a call changed are collected in lost (bit 0-7
   d0-d7, 8-14 a0-a6): its contract is longmult's and longdiv's */
static int marks;
static ULONG lost;

static ULONG call(APTR fn, ULONG d0, ULONG d1)
{
    struct regs r = {{0}}, in;
    int k;

    if (marks) {
        for (k = 2; k < 8; k++)
            r.d[k] = 0x5a5a0000UL | k;
        for (k = 0; k < 7; k++)
            if (k < 4 || k == 6)
                r.a[k] = 0xa5a50000UL | k;
    }
    r.d[0] = d0;
    r.d[1] = d1;
    in = r;
    callregs(fn, &r, 0, 0);
    if (marks) {
        for (k = 2; k < 8; k++)
            if (r.d[k] != in.d[k])
                lost |= 1UL << k;
        for (k = 0; k < 7; k++)
            if (r.a[k] != in.a[k])
                lost |= 1UL << (8 + k);
    }
    return r.d[0];
}

int main(int argc, char **argv)
{
    static struct lib a;
    long rounds = argc > 4 ? atol(argv[4]) : 20000;
    char vec[24];
    int i;

    if (argc < 4) {
        printf("usage: libtest LIB DIR NAME [ROUNDS]\n");
        return 20;
    }
    if (!load(&a, argv[1]))
        return 20;
    seed = 0x1234567;           /* the same inputs in every run */
    marks = W(a.base + 0x0e) == 0xd600;
    cap_begin(argv[2], argv[3]);
    cap_num("routine count", NULL, a.n);
    cap_blocks(1);
    for (i = 0; i < a.n; i++) {
        long tests = 0;
        unsigned x, y;
        ULONG d0, d1;
        cap_routine(a.what[i]);
        lost = 0;
        for (x = 0; x < NEDGES * NEDGES + rounds; x++) {
            if (x < NEDGES * NEDGES) {
                y = x / NEDGES;
                d0 = edges[x % NEDGES];
                d1 = edges[y];
                if (x % 2) d0 = -d0;            /* both signs */
            } else {
                d0 = rnd();
                d1 = rnd();
            }
            sprintf(vec, "$%08lX $%08lX", d0, d1);
            cap_num("d0", vec, call(a.code[i], d0, d1));
            tests++;
        }
        printf("%-12s %ld tests\n", a.what[i], tests);
        if (marks)
            a.lost[i] = lost;
    }
    cap_blocks(0);
    if (marks)                          /* compilerlib: the kept registers */
        for (i = 0; i < a.n; i++) {
            cap_routine(a.what[i]);
            cap_num("lost registers", NULL, a.lost[i]);
        }
    /* ldivlib: its paths' limits; compilerlib (#1178) the same inputs */
    if (W(a.base + 0x0e) == 0xcb00 || marks) {
        for (i = 0; i < a.n; i++) {
            unsigned x, y, sg;
            cap_routine(a.what[i]);
            for (x = 0; x < NDIVIDENDS; x++)
                for (y = 0; y < NDIVISORS; y++)
                    for (sg = 0; sg < 4; sg++) {
                        ULONG d0 = sg & 1 ? -dividends[x] : dividends[x];
                        ULONG d1 = sg & 2 ? -divisors[y] : divisors[y];
                        sprintf(vec, "$%08lX $%08lX", d0, d1);
                        cap_num("limit d0", vec, call(a.code[i], d0, d1));
                    }
            /* the loop's normalisation: a divisor with its top bit at
               every place from 16 to 31, (1 << m) + 1 and (2 << m) - 1
               (fraction bits for Quick), both signs of the dividend */
            for (x = 0; x < NDIVIDENDS; x++)
                for (y = 16; y < 32; y++)
                    for (sg = 0; sg < 4; sg++) {
                        ULONG d0 = sg & 1 ? -dividends[x] : dividends[x];
                        ULONG d1 = sg & 2 ? (2UL << y) - 1 : (1UL << y) + 1;
                        if ((sg & 2) && y == 31)
                            continue;           /* $FFFFFFFF: -1 */
                        sprintf(vec, "$%08lX $%08lX", d0, d1);
                        cap_num("norm d0", vec, call(a.code[i], d0, d1));
                    }
        }
    }
    UnLoadSeg(a.seg);
    return cap_end() ? 5 : 0;
}
