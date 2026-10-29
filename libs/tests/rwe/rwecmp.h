/*
 * rwecmp.h - the comparison framework of the RWE userlib tests
 * (libs/tests/rwe/NAME.c, header only; the method of systest_cmdlibs.c):
 * every routine is called on both sets - sets[0] the CD original
 * (NAME.orig), sets[1] the C build (NAME.obj), loaded and linked by
 * sysload.h - with the same inputs, the other registers markers; the test
 * compares results and memory (SAME), and every register the original
 * keeps must be kept by the C build (preserve, after every DO2).
 *
 *   ENT(lib, idx, form)   code of form `form` of entry `idx` of `lib`
 *   DO2(fn, setup)        call fn on both sets; `setup` sets R[cur] ...
 *   DO2S(fn, setup)       the same with stack arguments ARGS[cur][0..NARGS-1]
 *   SAME(what, expr)      expr (evaluated with cur = 0 and 1) equal
 *   rwe_done()            summary, the exit status
 */
#ifndef RWECMP_H
#define RWECMP_H

#include "../sysload.h"

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

static int cur;                         /* set index in setup/compare */
static struct regs R[2], IN[2];
static ULONG ARGS[2][6], POP[2];
static int NARGS;
static UWORD CCR[2];
static const char *ROUT;                /* routine under test */
static char VEC[100];                   /* current case, for messages */
static int prints;

#define ENT(lib, idx, form) entry_form(getlib(&sets[cur], lib), idx, form)

#define MAXROUT 40
static struct { const char *name; ULONG ko, kc; ULONG n; } stat[MAXROUT];
static int nstat;

static void note_kept(void)
{
    int i;
    ULONG ko = kept(&IN[0], &R[0]), kc = kept(&IN[1], &R[1]);
    for (i = 0; i < nstat; i++)
        if (stat[i].name == ROUT)
            break;
    if (i == nstat) {
        if (nstat == MAXROUT)
            return;
        stat[nstat].name = ROUT;
        stat[nstat].ko = stat[nstat].kc = 0x7fff;
        stat[nstat].n = 0;
        nstat++;
    }
    stat[i].ko &= ko;
    stat[i].kc &= kc;
    stat[i].n++;
}

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

/* a check on each set separately (results that may differ between the
   sets, e.g. random numbers): `expr` must hold for both */
#define BOTH(what, expr) do {                                               \
        for (cur = 0; cur < 2; cur++)                                       \
            if (!(expr)) fail(what, cur == 0, cur == 1); else checks++;     \
    } while (0)

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    note_kept();
    sprintf(buf, "%.38s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
    SAME("popped", POP[cur]);
}

#define DO2S(fn, setup) do {                                                \
        for (cur = 0; cur < 2; cur++) {                                     \
            APTR fn_;                                                       \
            markregs(&R[cur]);                                              \
            NARGS = 0;                                                      \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            fn_ = (fn);                                                     \
            if (!fn_) {                                                     \
                check("entry missing", FALSE);                              \
                continue;                                                   \
            }                                                               \
            CCR[cur] = callstk(fn_, &R[cur], ARGS[cur], NARGS, &POP[cur]);  \
        }                                                                   \
        preserve();                                                         \
    } while (0)
#define DO2(fn, setup) DO2S(fn, setup)

static ULONG rwe_seed = 0x13579bdf;
static __attribute__((unused)) ULONG rnd(void)
{
    rwe_seed = rwe_seed * 1103515245UL + 12345;
    return (rwe_seed >> 16) | ((rwe_seed * 69069UL + 1) & 0xffff0000UL);
}

static int rwe_done(const char *name)
{
    int i;
    printf("%s: %d checks, %d failures\n", name, checks, failures);
    printf("kept registers (bit 0-7 d0-d7, 8-14 a0-a6), all calls:\n");
    for (i = 0; i < nstat; i++)
        printf("  %-24s %5lu calls  orig $%04lX  C $%04lX\n", stat[i].name,
               stat[i].n, stat[i].ko, stat[i].kc);
    return failures ? 5 : 0;
}

#endif
