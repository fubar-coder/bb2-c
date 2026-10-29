/*
 * systest_cmdlibs - functional comparison, original (acidlibs) against the
 * C build, of the basic command libraries
 *   memacclib, sortlib, mathtranslib, utillib, environlib
 * (plus memlib and strcomplib, which they call).
 *
 *   systest_cmdlibs DIR
 *
 * Same method as systest_mathstr: every routine is called on both sets
 * with the same inputs (other registers = markers); compared are d0, a3,
 * memory contents, and every register the original keeps must be kept by
 * the C build.  The command entries are found through their first form
 * (cmd_entry).  The BASIC level (compiler + program) is tested by
 * compilerlauf/corpus/tests/t09..t13.
 */
#include "sysload.h"
#include <proto/intuition.h>

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static int cur;
static struct regs R[2], IN[2];
static ULONG POP[2];
static UWORD CCR[2];
static const char *ROUT;
static char VEC[100];
static int prints;

#define MAXROUT 32
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
/* the bytes of both sets the same (SAME(what, memcmp(x[0], x[1], n)) would
   compare the one memcmp result with itself and never fail) */
#define SAMEBYTES(what, x, n) do {                                          \
        if (memcmp((x)[0], (x)[1], (n))) fail(what, 0, 1); else checks++;   \
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
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
    SAME("popped", POP[cur]);
}

#define DO2(fn, setup) do {                                                 \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            CCR[cur] = callstk(fn, &R[cur], NULL, 0, &POP[cur]);            \
        }                                                                   \
        preserve();                                                         \
    } while (0)

static ULONG seed = 0x2468ace1;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* code of entry idx (form 0 of a command, or a nullsub) */
static UBYTE *cmd_entry(struct lib *l, int idx)
{
    UBYTE *b = l->code, *p = b + 0x30;
    int i = 0;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2;
            code = (UBYTE *)L(p + 4);
            p += 12;
        } else {
            int f = 0;
            if (kind != 8) {
                while (W(p) != 0xffff) {        /* forms */
                    p += 2 + (W(p) & 0xff);
                    p = b + ((p - b + 1) & ~1);
                    while (W(p)) p += 4;
                    p += 2;
                    if (f++ == 0)
                        code = (UBYTE *)L(p + 4);
                    p += 12;
                }
                p += 2;
            }
            p += 6;                             /* next.l, num.w */
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
        if (i == idx)
            return code;
        i++;
    }
    return NULL;
}

#define E(lib, i) cmd_entry(getlib(&sets[cur], lib), i)

static ULONG membase[2];

/* ---------------------------------------------------------- memacclib */

static UBYTE mem[2][256], out[2][256];
static ULONG frame[2][4];

static void fillmem(void)
{
    int s, i;
    for (s = 0; s < 2; s++)
        for (i = 0; i < 256; i++) {
            mem[s][i] = (UBYTE)(i * 37 + 11);
            out[s][i] = 0xEE;
        }
    for (s = 0; s < 2; s++) {
        strcpy((char *)mem[s] + 100, "Hello, Peek$");
        mem[s][120] = 0;
    }
}

static void test_memacclib(void)
{
    static const ULONG types[] = { 1, 2, 3, 4, 5, 7, 0, 6, 8, 0x5A000001UL, 0x5A000002UL,
                                   0x00010001UL, 0xFFFF0007UL };
    static const ULONG lens[] = { 0, 1, 5, 12, 17 };
    unsigned i, j;

    fillmem();
    ROUT = "Peek";
    for (i = 0; i < sizeof types / sizeof types[0]; i++)
        for (j = 0; j < 3; j++) {
            ULONG at = j == 0 ? 100 : j == 1 ? 2 : 120;
            sprintf(VEC, "type $%lX at %lu", types[i], at);
            DO2(E("memacclib", 1), R[cur].d[0] = types[i]; R[cur].d[1] = (ULONG)&mem[cur][at];
                R[cur].a[3] = (ULONG)out[cur]);
            SAME("d0", R[cur].d[0]);
            SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
            SAMEBYTES("out", out, 256);
        }
    ROUT = "Peeks$";
    for (i = 0; i < sizeof lens / sizeof lens[0]; i++) {
        sprintf(VEC, "len %lu", lens[i]);
        DO2(E("memacclib", 2), R[cur].d[0] = (ULONG)&mem[cur][100]; R[cur].d[1] = lens[i];
            R[cur].a[3] = (ULONG)out[cur] + 3);
        SAME("d0", R[cur].d[0]);
        SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
        SAMEBYTES("out", out, 256);
    }
    ROUT = "Poke";
    for (i = 0; i < sizeof types / sizeof types[0]; i++) {
        ULONG data = rnd();
        sprintf(VEC, "type $%lX", types[i]);
        DO2(E("memacclib", 0), R[cur].d[0] = types[i]; R[cur].d[1] = (ULONG)&mem[cur][10];
            R[cur].d[2] = (types[i] & 0xffff) == 7 ? (ULONG)&mem[cur][100] : data;
            frame[cur][1] = 1 + i; R[cur].a[2] = (ULONG)&frame[cur][2]);
        SAMEBYTES("mem", mem, 256);
        SAME("mem[10..20] sum", mem[cur][10] + mem[cur][11] * 3 + mem[cur][12] * 5 + mem[cur][13] * 7
             + mem[cur][14] * 11 + mem[cur][20]);
    }
    /* Poke.s with length 0: only the 0 */
    strcpy(VEC, "string len 0");
    DO2(E("memacclib", 0), R[cur].d[0] = 7; R[cur].d[1] = (ULONG)&mem[cur][30];
        R[cur].d[2] = (ULONG)&mem[cur][100]; frame[cur][1] = 0; R[cur].a[2] = (ULONG)&frame[cur][2]);
    SAMEBYTES("mem", mem, 256);

    /* Call: moveq #7,d3 / move.l a0,d4 / rts */
    ROUT = "Call";
    strcpy(VEC, "");
    {
        static const UWORD code[] = { 0x7607, 0x2808, 0x4e75 };
        for (cur = 0; cur < 2; cur++)
            memcpy(&mem[cur][200], code, sizeof code);
        CacheClearU();
        DO2(E("memacclib", 3), R[cur].d[0] = (ULONG)&mem[cur][200]);
        SAME("d3", R[cur].d[3]);
        SAME("d4 = a0 = address", R[cur].d[4] == (ULONG)&mem[cur][200]);
    }
}

/* ------------------------------------------------------------ sortlib */

#define NSORT 48
static UBYTE arr[2][NSORT * 4 + 16];
static ULONG sortframe[2][4];
static ULONG ffpbase;
static UBYTE strpool[2][NSORT][16];

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

static void sort_one(UWORD type, int n, int down, int pattern)
{
    int size = type == 1 ? 1 : type == 2 ? 2 : 4;
    int s, i;

    for (s = 0; s < 2; s++)
        for (i = 0; i < (int)sizeof arr[s]; i++)
            arr[s][i] = (UBYTE)(0xA5 ^ i);      /* same guard behind the array */
    for (i = 0; i < n; i++) {
        ULONG v = pattern == 0 ? rnd() : pattern == 1 ? (ULONG)i : pattern == 2 ? (ULONG)(n - i)
                  : (rnd() & 3);
        for (s = 0; s < 2; s++) {
            UBYTE *p = arr[s] + i * size;
            switch (type) {
            case 1: *p = (UBYTE)v; break;
            case 2: *(UWORD *)p = (UWORD)v; break;
            case 5: *(ULONG *)p = int2ffp((LONG)(v & 0xffff) - 30000); break;
            case 7: {
                UBYTE *q = strpool[s][i] + 4;
                int len = v & 7;
                int k;
                for (k = 0; k < len; k++)
                    q[k] = "abAB z\x80\xff"[(v >> (8 + k * 3)) & 7];
                q[len] = 0;
                ((ULONG *)q)[-1] = len;
                *(ULONG *)p = (ULONG)q;
                break;
            }
            default: *(ULONG *)p = v; break;
            }
        }
    }
    sprintf(VEC, "type %u n %d %s pattern %d", type, n, down ? "down" : "up", pattern);
    DO2(E("sortlib", down ? 2 : 1), (void)0);
    DO2(E("sortlib", 0), R[cur].d[0] = (ULONG)arr[cur];
        sortframe[cur][1] = type; sortframe[cur][2] = (ULONG)(arr[cur] + n * size);
        R[cur].a[2] = (ULONG)&sortframe[cur][3];   /* -4 high, -6 type.w */
        R[cur].a[6] = ffpbase);
    if (type == 7) {
        int ok = 1;
        for (i = 0; i < n; i++) {
            const char *a = (const char *)*(ULONG *)(arr[0] + i * 4);
            const char *b = (const char *)*(ULONG *)(arr[1] + i * 4);
            if ((ULONG)a - (ULONG)strpool[0] != (ULONG)b - (ULONG)strpool[1])
                ok = 0;
        }
        SAME("same order", ok);
    } else
        SAMEBYTES("array", arr, sizeof arr[0]);
}

static void test_sortlib(void)
{
    static const UWORD types[] = { 1, 2, 3, 4, 5, 7, 0 };
    static const int ns[] = { 1, 2, 3, 7, 16, NSORT };
    unsigned t, k;
    int d, pat;

    ROUT = "sortlib init";
    strcpy(VEC, "");
    DO2(E("sortlib", 3), (void)0);
    ROUT = "Sort";
    for (t = 0; t < sizeof types / sizeof types[0]; t++)
        for (k = 0; k < sizeof ns / sizeof ns[0]; k++)
            for (d = 0; d < 2; d++)
                for (pat = 0; pat < 4; pat++) {
                    if (types[t] == 2 && ns[k] == 1)
                        continue;               /* pivot behind the array, see sortlib.c */
                    sort_one(types[t], ns[k], d, pat);
                }
    DO2(E("sortlib", 1), (void)0);
}

/* -------------------------------------------------------- mathtranslib */

static void test_mathtranslib(void)
{
    int i;
    ULONG base[2];

    ROUT = "mathtranslib init";
    strcpy(VEC, "");
    DO2(E("mathtranslib", 14), (void)0);
    for (cur = 0; cur < 2; cur++)
        base[cur] = R[cur].d[0];
    SAME("base", R[cur].d[0]);
    /* inline routines: `Dc $a000,len` + JSR lvo(a6); compare the bytes */
    ROUT = "mathtranslib inline";
    for (i = 0; i <= 13; i++) {
        sprintf(VEC, "entry %d", i);
        SAME("prefix+len", L(E("mathtranslib", i)));
        SAME("code", L(E("mathtranslib", i) + 4));
    }
    ROUT = "mathtranslib finit";
    strcpy(VEC, "");
    DO2(E("mathtranslib", 15), R[cur].a[1] = base[cur]);
}

/* ------------------------------------------------------------ utillib */

static ULONG lenframe[2][2];

static void test_utillib(void)
{
    static const char *pnames[] = { "bb2 systest port", "", "p", "another" };
    ULONG port[2][4];
    int i;

    ROUT = "utillib init";
    strcpy(VEC, "");
    DO2(E("utillib", 4), (void)0);
    SAME("*init = 0", *(ULONG *)R[cur].d[0]);

    ROUT = "TaskName$";
    for (i = 0; i < 2; i++) {
        sprintf(VEC, "task %s", i ? "FindTask(0)" : "0");
        for (cur = 0; cur < 2; cur++)
            memset(out[cur], 0xEE, 256);
        DO2(E("utillib", 3), R[cur].d[0] = i ? (ULONG)FindTask(NULL) : 0;
            R[cur].a[3] = (ULONG)out[cur]);
        SAME("d0", R[cur].d[0]);
        SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
        SAMEBYTES("out", out, 256);
    }

    ROUT = "CreateMsgPort";
    for (i = 0; i < 4; i++) {
        sprintf(VEC, "\"%s\"", pnames[i]);
        DO2(E("utillib", 0), R[cur].d[0] = (ULONG)pnames[i];
            lenframe[cur][0] = strlen(pnames[i]); R[cur].a[2] = (ULONG)&lenframe[cur][1]);
        for (cur = 0; cur < 2; cur++)
            port[cur][i] = R[cur].d[0];
        SAME("port != 0", R[cur].d[0] != 0);
        SAME("a3 = name", R[cur].a[3] == (ULONG)pnames[i]);
        SAME("type/pri/flags", *(ULONG *)(R[cur].d[0] + 8) & 0xffff0000UL);
        SAME("flags", *(UBYTE *)(R[cur].d[0] + 14));
        SAME("sigtask", *(ULONG *)(R[cur].d[0] + 16) == (ULONG)FindTask(NULL));
        SAME("name", *(ULONG *)(R[cur].d[0] + 10) ? strcmp((char *)*(ULONG *)(R[cur].d[0] + 10),
                                                           pnames[i]) + 1000 : 0);
        SAME("name size", *(ULONG *)(R[cur].d[0] + 10) ? *(ULONG *)(*(ULONG *)(R[cur].d[0] + 10) - 4) : 0);
        SAME("msglist", *(ULONG *)(R[cur].d[0] + 20) == R[cur].d[0] + 24
             && *(ULONG *)(R[cur].d[0] + 28) == R[cur].d[0] + 20);
        SAME("findport", *pnames[i] ? FindPort((STRPTR)pnames[i]) != NULL : 0);
    }
    SAME("sigbits differ", *(UBYTE *)(port[cur][0] + 15) - *(UBYTE *)(port[cur][1] + 15));

    ROUT = "DeleteMsgPort";
    strcpy(VEC, "middle");
    DO2(E("utillib", 1), R[cur].d[0] = port[cur][1]);
    SAME("links", *(ULONG *)(port[cur][0] - 8) == port[cur][2] - 8
         && *(ULONG *)(port[cur][2] - 4) == port[cur][0] - 8);
    strcpy(VEC, "public");
    DO2(E("utillib", 1), R[cur].d[0] = port[cur][0]);
    SAME("findport gone", FindPort((STRPTR)pnames[0]) == NULL);

    /* Wait: signal ourselves first */
    ROUT = "Wait";
    strcpy(VEC, "");
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        IN[cur] = R[cur];
        Signal(FindTask(NULL), SIGBREAKF_CTRL_F);
        CCR[cur] = callstk(E("utillib", 2), &R[cur], NULL, 0, &POP[cur]);
    }
    preserve();
    /* other pending signals of the test task may differ between the calls */
    SAME("d0 & CTRL_F", R[cur].d[0] & SIGBREAKF_CTRL_F);

    ROUT = "utillib finit";
    DO2(E("utillib", 5), (void)0);
    SAME("findport gone", FindPort((STRPTR)pnames[3]) == NULL);
}

/* ---------------------------------------------------------- environlib */

static void test_environlib(void)
{
    struct Library *ib = OpenLibrary((STRPTR)"intuition.library", 0);
    int i;

    ROUT = "environlib init";
    strcpy(VEC, "");
    DO2(E("environlib", 8), R[cur].a[6] = (ULONG)ib);
    for (i = 0; i < 8; i++) {
        ROUT = "environlib function";
        sprintf(VEC, "entry %d", i);
        DO2(E("environlib", i), (void)0);
        SAME("d0", R[cur].d[0]);
    }
    CloseLibrary(ib);
}

/* ------------------------------------------------------------------ main */

static const char *names[] = {
    "memlib", "strcomplib", "memacclib", "sortlib", "mathtranslib", "utillib",
    "environlib", NULL
};

int main(int argc, char **argv)
{
    int i;
    ULONG avail, after, dispatches;
    struct Library *ffp;

    if (argc < 2) {
        printf("usage: systest_cmdlibs DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    ffp = OpenLibrary((STRPTR)"mathffp.library", 0);
    ffpbase = (ULONG)ffp;

    test_memacclib();
    test_sortlib();
    test_mathtranslib();
    test_environlib();

    /* The memory balance is measured under Forbid(): AvailMem() counts the
       whole system, and other tasks allocate during the run (in Amiberry
       a task took 536 bytes, 496 of them back right after, in 3 of 30
       runs at this point, and 40 bytes a second later in every run).
       Under Forbid() only this task and the libraries it calls change the
       free memory, so the comparison stays exact.  Nothing in between
       waits (utillib's Wait gets its signal beforehand, check() prints
       only on a failure); a Wait would break the Forbid(), so the
       dispatch counter must not change either. */
    Forbid();
    dispatches = SysBase->DispCount;
    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib init";
    strcpy(VEC, "");
    DO2(cmd_entry(getlib(&sets[cur], "memlib"), 7), (void)0);
    for (cur = 0; cur < 2; cur++)
        membase[cur] = R[cur].d[0];
    test_utillib();
    ROUT = "memlib finit";
    DO2(cmd_entry(getlib(&sets[cur], "memlib"), 8), (void)0);
    after = AvailMem(MEMF_ANY);
    dispatches = SysBase->DispCount - dispatches;
    Permit();
    check("utillib: no task switch while measuring the memory", dispatches == 0);
    check("utillib: all memory back", after == avail);

    CloseLibrary(ffp);
    printf("%d checks, %d failures\n", checks, failures);
    printf("kept registers (bit 0-7 d0-d7, 8-14 a0-a6), all calls:\n");
    for (i = 0; i < nstat; i++)
        printf("  %-20s %5lu calls  orig $%04lX  C $%04lX\n", stat[i].name,
               stat[i].n, stat[i].ko, stat[i].kc);
    return failures ? 5 : 0;
}
