/*
 * libtest - compare the routines of two builds of a Blitz library on the
 * Amiga: the original (e.g. extracted from blitz2:acidlibs) and the C
 * build.  Both files are loaded with LoadSeg; the code addresses come from
 * the relocated !subs fields of the token table.  Every nullsub/form
 * routine is called with the same register inputs, d0 is compared.
 *
 *   libtest ORIG.obj NEW.obj [ROUNDS]
 *
 * Inputs: all pairs of a list of edge values in d0/d1, plus ROUNDS
 * (default 20000) pseudo random pairs (fixed seed, reproducible).
 * Output: one line per routine; exit 0 if all results are equal.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>

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

static ULONG seed = 0x1234567;
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

static ULONG call(APTR fn, ULONG d0, ULONG d1)
{
    struct regs r = {{0}};
    r.d[0] = d0;
    r.d[1] = d1;
    callregs(fn, &r, 0, 0);
    return r.d[0];
}

int main(int argc, char **argv)
{
    static struct lib a, b;
    long rounds = argc > 3 ? atol(argv[3]) : 20000;
    int i, bad = 0;

    if (argc < 3) {
        printf("usage: libtest ORIG.obj NEW.obj [ROUNDS]\n");
        return 20;
    }
    if (!load(&a, argv[1]) || !load(&b, argv[2]))
        return 20;
    if (a.n != b.n) {
        printf("routine count differs: %d / %d\n", a.n, b.n);
        return 10;
    }
    for (i = 0; i < a.n; i++) {
        long tests = 0, diffs = 0;
        unsigned x, y;
        ULONG d0, d1, ra, rb;
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
            ra = call(a.code[i], d0, d1);
            rb = call(b.code[i], d0, d1);
            tests++;
            if (ra != rb) {
                if (diffs < 5)
                    printf("  %s: d0=$%08lX d1=$%08lX -> orig $%08lX, C $%08lX\n",
                           a.what[i], d0, d1, ra, rb);
                diffs++;
            }
        }
        printf("%s %-12s %ld tests, %ld differences\n",
               diffs ? "FAIL" : "ok  ", a.what[i], tests, diffs);
        if (diffs)
            bad = 1;
    }
    UnLoadSeg(a.seg);
    UnLoadSeg(b.seg);
    return bad ? 5 : 0;
}
