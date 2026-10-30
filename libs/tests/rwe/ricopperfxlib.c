/*
 * rwe ricopperfxlib - the C build of the RWE userlib ricopperfxlib
 * (libs/userlibs/rwe/ricopperfxlib.c) against the CD binary BSS2
 * RICOPPERFXLIB.OBJ (ricopperfxlib.orig).
 *
 *   ricopperfxlib DIR     (DIR: ricopperfxlib.obj and .orig)
 *
 * Every command is called on both with the same inputs (other registers
 * = markers, the upper words of word arguments garbage): compared are
 * d0 of the functions, the custom copper space (a buffer per set, filled
 * with a pattern, compared as a whole), the state block of
 * CopperInfoBlock (pointers relative to the set's buffer/coplist) and
 * every register the original keeps.  Covered: all 14 commands and both
 * forms of CopperReset, before any reset (no custom space), after
 * CopperReset and CopperResetAdr, colour splits crossing line 256,
 * falling/rising/negative colour values, random splits.
 * Not covered: the copper actually running the lists (nothing here is
 * displayed - the lists are only built in memory), DoColSplit/RedoColSplit
 * with numlines 0 (divide-by-zero exception) or negative (65536 lines),
 * RedoColSplit with a ccoffset above $FFFF (writes 64 KB further on).
 */
#include "sysload.h"

static const char *names[] = { "ricopperfxlib", NULL };

/* entries of the table */
enum { DOCOLSPLIT, REDOCOLSPLIT, COPPERRESET, COPPEREND, INFOBLOCK, COMMAND, MOVE,
       WAIT, SKIP, GETCCOFFSET, AGACOL, RESETADR, BPLCON3, FINIT, INIT };

static int cur;
static struct regs R[2], IN[2];
static const char *ROUT;
static char VEC[100];
static int prints;

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

/* a comparison of the data of both sets (once, not per set) */
#define BOTH(what, cond) do {                                               \
        if (!(cond)) fail(what, 0, 1); else checks++;                       \
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
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
}

/* code of entry idx, form `form` (bit 0 = Amiga mode only cleared) */
static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(getlib(&sets[cur], names[0]), idx, form) & ~1UL);
}

#define DO2(idx, form, setup) do {                                          \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callregs(ent(idx, form), &R[cur], 0, 0);                        \
        }                                                                   \
        preserve();                                                         \
    } while (0)

static ULONG seed = 0x2468ace1;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* a word argument: the upper word is whatever the register held */
#define WARG(v) (0xA5A50000UL | (UWORD)(v))

#define BUFSIZE 8192
static UBYTE *buf[2];
static ULONG coplist[2][8];             /* +28 = custom space */
static UBYTE *info[2];
static LONG cols[2][6];

static void fillbuf(void)
{
    int s, i;
    for (s = 0; s < 2; s++)
        for (i = 0; i < BUFSIZE; i++)
            buf[s][i] = (UBYTE)(0x5A ^ i);
}

/* pointer of the state block relative to the set's objects */
static ULONG relptr(ULONG p)
{
    if (!p)
        return 0;
    if (p == (ULONG)coplist[cur])
        return 1;
    if (p >= (ULONG)buf[cur] && p < (ULONG)buf[cur] + BUFSIZE)
        return 0x10000 + p - (ULONG)buf[cur];
    return 0xdead0000;
}

static void same_state(void)
{
    BOTH("custom space", memcmp(buf[0], buf[1], BUFSIZE) == 0);
    SAME("coplistaddr", relptr(*(ULONG *)info[cur]));
    SAME("customaddr", relptr(*(ULONG *)(info[cur] + 4)));
    SAME("startline/ccoffset", *(ULONG *)(info[cur] + 8));
    SAME("bplcon3LO/HI", *(ULONG *)(info[cur] + 12));
    SAME("colreg/lo_bplcon3", *(ULONG *)(info[cur] + 16));
}

static void setcols(LONG r0, LONG g0, LONG b0, LONG r1, LONG g1, LONG b1)
{
    for (cur = 0; cur < 2; cur++) {
        cols[cur][0] = r0; cols[cur][1] = g0; cols[cur][2] = b0;
        cols[cur][3] = r1; cols[cur][4] = g1; cols[cur][5] = b1;
    }
}

static void colsplit(ULONG lines, ULONG reg)
{
    ROUT = "DoColSplit";
    sprintf(VEC, "lines %lu reg %lu (%ld,%ld,%ld)-(%ld,%ld,%ld)", lines, reg, cols[0][0],
            cols[0][1], cols[0][2], cols[0][3], cols[0][4], cols[0][5]);
    DO2(DOCOLSPLIT, 0, R[cur].d[0] = (ULONG)cols[cur]; R[cur].d[1] = WARG(lines);
        R[cur].d[2] = WARG(reg));
    /* d0 = the red step, or cols_adr without a custom space */
    SAME("d0", R[cur].d[0] == (ULONG)cols[cur] ? 1 : R[cur].d[0]);
    same_state();
}

static void redosplit(ULONG lines, ULONG off)
{
    ROUT = "RedoColSplit";
    sprintf(VEC, "lines %lu offset %lu (%ld,%ld,%ld)-(%ld,%ld,%ld)", lines, off, cols[0][0],
            cols[0][1], cols[0][2], cols[0][3], cols[0][4], cols[0][5]);
    DO2(REDOCOLSPLIT, 0, R[cur].d[0] = (ULONG)cols[cur]; R[cur].d[1] = WARG(lines);
        R[cur].d[2] = off);
    same_state();
}

static void reset(ULONG line, int withoff, ULONG off)
{
    ROUT = "CopperReset";
    sprintf(VEC, "line %lu off %lu form %d", line, off, withoff);
    DO2(COPPERRESET, withoff, R[cur].d[0] = WARG(3); R[cur].d[1] = WARG(line);
        R[cur].d[2] = WARG(off); R[cur].a[0] = (ULONG)coplist[cur]);
    same_state();
}

static void two(int idx, const char *name, ULONG a, ULONG b)
{
    ROUT = name;
    sprintf(VEC, "$%lX,$%lX", a, b);
    DO2(idx, 0, R[cur].d[0] = WARG(a); R[cur].d[1] = WARG(b));
    same_state();
}

static void agacol(ULONG c, ULONG r, ULONG g, ULONG b)
{
    ROUT = "CopperAGACol";
    sprintf(VEC, "%lu,$%lX,$%lX,$%lX", c, r, g, b);
    DO2(AGACOL, 0, R[cur].d[0] = WARG(c); R[cur].d[1] = WARG(r); R[cur].d[2] = WARG(g);
        R[cur].d[3] = WARG(b));
    SAME("d0", R[cur].d[0]);
    same_state();
}

static void nullary(int idx, const char *name, int result)
{
    ROUT = name;
    strcpy(VEC, "");
    DO2(idx, 0, (void)0);
    if (result)
        SAME("d0", R[cur].d[0]);
    same_state();
}

static void commands(void)
{
    static const UWORD xs[] = { 0, 1, 2, 3, 100, 226, 227, 254, 255, 498, 500, 0x7fff, 0x8000,
                                0xfffe, 0xffff };
    unsigned i;

    two(MOVE, "CopperMove", 0x181, 0x123);
    two(MOVE, "CopperMove", 0x0fe, 0xffff);
    for (i = 0; i < sizeof xs / sizeof xs[0]; i++) {
        two(WAIT, "CopperWait", xs[i], 0x2c + i);
        two(SKIP, "CopperSkip", xs[i], 0xff - i);
    }
    two(WAIT, "CopperWait", 40, 0x1ff);
    two(COMMAND, "CopperCommand", 0x1fe, 0);
    two(COMMAND, "CopperCommand", 0x4321, 0x8765);
    agacol(37, 0x12, 0x34, 0x56);
    agacol(200, 0xff, 0x80, 0x01);
    agacol(255, 0x1ff, 0xfedc, 0x8000);
    agacol(0x1234, 0, 0, 0);
    nullary(COPPEREND, "CopperEnd", 0);
    nullary(GETCCOFFSET, "GetCCOffset", 1);
}

int main(int argc, char **argv)
{
    int i;

    if (argc < 2) {
        printf("usage: ricopperfxlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (cur = 0; cur < 2; cur++) {
        buf[cur] = AllocMem(BUFSIZE, MEMF_PUBLIC);
        coplist[cur][7] = (ULONG)buf[cur];
    }
    fillbuf();
    setcols(0, 0, 0, 255, 128, 64);

    ROUT = "init";
    strcpy(VEC, "");
    DO2(INIT, 0, (void)0);
    ROUT = "CopperInfoBlock";
    DO2(INFOBLOCK, 0, (void)0);
    for (cur = 0; cur < 2; cur++)
        info[cur] = (UBYTE *)R[cur].d[0];
    SAME("block in the library",
         R[cur].d[0] >= (ULONG)sets[cur].lib[0].code
         && R[cur].d[0] < (ULONG)sets[cur].lib[0].code + sets[cur].lib[0].size);
    if (failures)
        return 5;
    same_state();

    /* no custom space yet: nothing is written */
    nullary(GETCCOFFSET, "GetCCOffset (no coplist)", 1);
    commands();
    colsplit(10, 5);
    redosplit(10, 0);

    /* CopperReset coplist,startline */
    reset(40, 0, 0);
    nullary(GETCCOFFSET, "GetCCOffset", 1);
    ROUT = "ColSplitBplcon3";
    strcpy(VEC, "$22");
    DO2(BPLCON3, 0, R[cur].d[0] = 0xA5A5A522UL);
    same_state();
    colsplit(20, 37);
    commands();
    setcols(200, 10, 255, 30, 250, 0);
    redosplit(20, 0);
    nullary(GETCCOFFSET, "GetCCOffset", 1);

    /* CopperReset coplist,startline,ccoffset: across line 256 */
    reset(250, 1, 300);
    setcols(255, 255, 255, 0, 0, 0);
    colsplit(12, 255);
    setcols(-100, 300, 7, 100, -300, 7);
    redosplit(12, 300);
    colsplit(1, 0);
    colsplit(7, 31);
    nullary(GETCCOFFSET, "GetCCOffset", 1);
    reset(0, 1, 2);                     /* line 0: the line 256 wait */
    setcols(1, 2, 3, 4, 5, 6);
    colsplit(3, 64);
    ROUT = "ColSplitBplcon3";
    strcpy(VEC, "$FF");
    DO2(BPLCON3, 0, R[cur].d[0] = 0xff);
    colsplit(5, 128);

    for (i = 0; i < 40; i++) {
        ULONG line = rnd() & 0x1ff, off = rnd() % 200, n = 1 + rnd() % 40, reg = rnd() & 0xff;
        LONG c[6];
        int k;
        for (k = 0; k < 6; k++)
            c[k] = (i & 3) == 3 ? (LONG)(rnd() & 0xfff) - 0x800 : (LONG)(rnd() & 0xff);
        fillbuf();
        reset(line, 1, off);
        setcols(c[0], c[1], c[2], c[3], c[4], c[5]);
        colsplit(n, reg);
        setcols(c[3], c[4], c[5], c[0], c[1], c[2]);
        redosplit(n, off);
        agacol(rnd() & 0xff, rnd() & 0xff, rnd() & 0xff, rnd() & 0xff);
    }

    /* CopperResetAdr: works, but GetCCOffset says -1 (BUG(orig) (#593)) */
    fillbuf();
    ROUT = "CopperResetAdr";
    strcpy(VEC, "buf+1000,7");
    DO2(RESETADR, 0, R[cur].d[0] = (ULONG)buf[cur] + 1000; R[cur].d[1] = WARG(7));
    same_state();
    nullary(GETCCOFFSET, "GetCCOffset (after CopperResetAdr)", 1);
    commands();
    setcols(9, 99, 199, 0, 255, 255);
    colsplit(16, 3);
    redosplit(16, 0);

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(FINIT, 0, (void)0);
    same_state();

    for (cur = 0; cur < 2; cur++)
        FreeMem(buf[cur], BUFSIZE);
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
