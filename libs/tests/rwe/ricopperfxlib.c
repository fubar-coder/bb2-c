/*
 * rwe ricopperfxlib - the RWE userlib ricopperfxlib: the C build
 * (libs/userlibs/rwe/ricopperfxlib.c) or the CD binary BSS2
 * RICOPPERFXLIB.OBJ (ricopperfxlib.orig), one set as LIBSET says, observed
 * as cases (sysload.h; the expected data
 * libs/tests/expected/rwe_ricopperfxlib.txt).
 *
 *   ricopperfxlib DIR     (DIR: ricopperfxlib.obj and .orig)
 *
 * Every command is called (other registers = markers, the upper words of
 * word arguments garbage): observed are d0 of the functions, the custom
 * copper space (a buffer filled with a pattern, observed as a whole), the
 * state block of CopperInfoBlock (pointers relative to the buffer/coplist)
 * and the kept registers (KEEPS).  Covered: all 14 commands and both
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

static struct regs R, IN;

/* code of entry idx, form `form` (bit 0 = Amiga mode only cleared) */
static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(getlib(&loaded, names[0]), idx, form) & ~1UL);
}

/* call entry idx, form `form`: markers in all registers, `setup` sets R,
   then the kept case of ROUT */
#define DO1(idx, form, setup) do {                                          \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callregs(ent(idx, form), &R, 0, 0);                                 \
        obs_kept(&IN, &R);                                                  \
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
static UBYTE *buf;
static ULONG coplist[8];                /* +28 = custom space */
static UBYTE *info;
static LONG cols[6];

static void fillbuf(void)
{
    int i;
    for (i = 0; i < BUFSIZE; i++)
        buf[i] = (UBYTE)(0x5A ^ i);
}

/* pointer of the state block relative to the objects */
static ULONG relptr(ULONG p)
{
    if (!p)
        return 0;
    if (p == (ULONG)coplist)
        return 1;
    if (p >= (ULONG)buf && p < (ULONG)buf + BUFSIZE)
        return 0x10000 + p - (ULONG)buf;
    return 0xdead0000;
}

static void same_state(void)
{
    OBSBYTES("custom space", buf, BUFSIZE);
    OBS("coplistaddr", relptr(*(ULONG *)info));
    OBS("customaddr", relptr(*(ULONG *)(info + 4)));
    OBS("startline/ccoffset", *(ULONG *)(info + 8));
    OBS("bplcon3LO/HI", *(ULONG *)(info + 12));
    OBS("colreg/lo_bplcon3", *(ULONG *)(info + 16));
}

static void setcols(LONG r0, LONG g0, LONG b0, LONG r1, LONG g1, LONG b1)
{
    cols[0] = r0; cols[1] = g0; cols[2] = b0;
    cols[3] = r1; cols[4] = g1; cols[5] = b1;
}

static void colsplit(ULONG lines, ULONG reg)
{
    ROUT = "DoColSplit";
    sprintf(VEC, "lines %lu reg %lu (%ld,%ld,%ld)-(%ld,%ld,%ld)", lines, reg, cols[0],
            cols[1], cols[2], cols[3], cols[4], cols[5]);
    DO1(DOCOLSPLIT, 0, R.d[0] = (ULONG)cols; R.d[1] = WARG(lines);
        R.d[2] = WARG(reg));
    /* d0 = the red step, or cols_adr without a custom space */
    OBS("d0", R.d[0] == (ULONG)cols ? 1 : R.d[0]);
    same_state();
}

static void redosplit(ULONG lines, ULONG off)
{
    ROUT = "RedoColSplit";
    sprintf(VEC, "lines %lu offset %lu (%ld,%ld,%ld)-(%ld,%ld,%ld)", lines, off, cols[0],
            cols[1], cols[2], cols[3], cols[4], cols[5]);
    DO1(REDOCOLSPLIT, 0, R.d[0] = (ULONG)cols; R.d[1] = WARG(lines);
        R.d[2] = off);
    same_state();
}

static void reset(ULONG line, int withoff, ULONG off)
{
    ROUT = "CopperReset";
    sprintf(VEC, "line %lu off %lu form %d", line, off, withoff);
    DO1(COPPERRESET, withoff, R.d[0] = WARG(3); R.d[1] = WARG(line);
        R.d[2] = WARG(off); R.a[0] = (ULONG)coplist);
    same_state();
}

static void two(int idx, const char *name, ULONG a, ULONG b)
{
    ROUT = name;
    sprintf(VEC, "$%lX,$%lX", a, b);
    DO1(idx, 0, R.d[0] = WARG(a); R.d[1] = WARG(b));
    same_state();
}

static void agacol(ULONG c, ULONG r, ULONG g, ULONG b)
{
    ROUT = "CopperAGACol";
    sprintf(VEC, "%lu,$%lX,$%lX,$%lX", c, r, g, b);
    DO1(AGACOL, 0, R.d[0] = WARG(c); R.d[1] = WARG(r); R.d[2] = WARG(g);
        R.d[3] = WARG(b));
    OBS("d0", R.d[0]);
    same_state();
}

static void nullary(int idx, const char *name, int result)
{
    ROUT = name;
    strcpy(VEC, "");
    DO1(idx, 0, (void)0);
    if (result)
        OBS("d0", R.d[0]);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("CopperInfoBlock", d1-d7|a1-a3|a6);
    KEEPS("GetCCOffset (no coplist)", d1-d7|a0-a3|a6);
    KEEPS("CopperMove", d1-d5|a1-a3|a6);
    KEEPS("CopperWait", d2-d5|a1-a3|a6);
    KEEPS("CopperSkip", d2-d5|a1-a3|a6);
    KEEPS("CopperCommand", d0-d5|a1-a3|a6);
    KEEPS("CopperAGACol", d4|a1-a3|a6);
    KEEPS("CopperEnd", d0-d5|a1-a3|a6);
    KEEPS("GetCCOffset", d1-d7|a0-a3|a6);
    KEEPS("DoColSplit", a3|a6);
    KEEPS("RedoColSplit", a3|a6);
    KEEPS("CopperReset", d0|d1|d3-d7|a1-a3|a6);
    KEEPS("ColSplitBplcon3", d0-d7|a0-a3|a6);
    KEEPS("CopperResetAdr", d0-d7|a0-a3|a6);
    KEEPS("GetCCOffset (after CopperResetAdr)", d1-d7|a0-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    buf = AllocMem(BUFSIZE, MEMF_PUBLIC);
    coplist[7] = (ULONG)buf;
    fillbuf();
    setcols(0, 0, 0, 255, 128, 64);

    ROUT = "init";
    strcpy(VEC, "");
    DO1(INIT, 0, (void)0);
    ROUT = "CopperInfoBlock";
    DO1(INFOBLOCK, 0, (void)0);
    info = (UBYTE *)R.d[0];
    OBS("block in the library",
        R.d[0] >= (ULONG)loaded.lib[0].code
        && R.d[0] < (ULONG)loaded.lib[0].code + loaded.lib[0].size);
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
    DO1(BPLCON3, 0, R.d[0] = 0xA5A5A522UL);
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
    DO1(BPLCON3, 0, R.d[0] = 0xff);
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
    DO1(RESETADR, 0, R.d[0] = (ULONG)buf + 1000; R.d[1] = WARG(7));
    same_state();
    nullary(GETCCOFFSET, "GetCCOffset (after CopperResetAdr)", 1);
    commands();
    setcols(9, 99, 199, 0, 255, 255);
    colsplit(16, 3);
    redosplit(16, 0);

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(FINIT, 0, (void)0);
    same_state();

    FreeMem(buf, BUFSIZE);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
