/*
 * systest_cmdlibs - the basic command libraries
 *   memacclib, sortlib, mathtranslib, utillib, environlib
 * (plus memlib and strcomplib, which they call) on the Amiga: one set (the
 * original from acidlibs or the C build, as LIBSET says), observed as
 * cases (sysload.h; the expected data
 * libs/tests/expected/systest_cmdlibs.txt).
 *
 *   systest_cmdlibs DIR
 *
 * Same method as systest_mathstr: every routine is called with the inputs
 * of the vector (other registers = markers); written are d0, a3, memory
 * contents and the kept registers of every call (KEEPS: the set the
 * original keeps in all calls of the routine).  The command entries are
 * found through their first form (cmd_entry).  The BASIC level (compiler
 * + program) is tested by compilerlauf/corpus/tests/t09..t13.
 */
#include "sysload.h"
#include "rwe/rwemem.h"
#include <proto/intuition.h>

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

/* the call of the case (ROUT and VEC are sysload.h's key parts) */
static struct regs R, IN;
static ULONG POP;
static UWORD CCR;

/* the kept case of the call and the bytes it removed from the stack */
static void preserve(void)
{
    obs_kept(&IN, &R);
    OBS("popped", POP);
}

/* call fn; `setup` may use R.  Every value is written with OBS as it is:
   a number, a flag or an offset */
#define DO1(fn, setup) do {                                                 \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        CCR = callstk(fn, &R, NULL, 0, &POP);                               \
        preserve();                                                         \
    } while (0)

static ULONG seed = 0x2468ace1;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* code of form `form` of entry idx (a command, or a nullsub: form 0),
   even (bit 0 marks an Amiga mode only routine) */
static UBYTE *cmd_form(struct lib *l, int idx, int form)
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
                    if (f++ == form)
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
            return (UBYTE *)((ULONG)code & ~1UL);
        i++;
    }
    return NULL;
}

/* code of entry idx (form 0 of a command, or a nullsub) */
static UBYTE *cmd_entry(struct lib *l, int idx)
{
    return cmd_form(l, idx, 0);
}

#define E(lib, i) cmd_entry(getlib(&loaded, lib), i)

static ULONG membase;

/* ---------------------------------------------------------- memacclib */

static UBYTE mem[256], out[256];
static ULONG frame[4];

static void fillmem(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        mem[i] = (UBYTE)(i * 37 + 11);
        out[i] = 0xEE;
    }
    strcpy((char *)mem + 100, "Hello, Peek$");
    mem[120] = 0;
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
            /* Peek.s: a3 is the end of the string result (observed) */
            DONTCARE = (types[i] & 0xffff) == 7 ? 1UL << 11 : 0;
            DO1(E("memacclib", 1), R.d[0] = types[i]; R.d[1] = (ULONG)&mem[at];
                R.a[3] = (ULONG)out);
            DONTCARE = 0;
            OBS("d0", R.d[0]);
            OBS("a3", R.a[3] - (ULONG)out);
            OBSBYTES("out", out, 256);
        }
    ROUT = "Peeks$";
    for (i = 0; i < sizeof lens / sizeof lens[0]; i++) {
        sprintf(VEC, "len %lu", lens[i]);
        DO1(E("memacclib", 2), R.d[0] = (ULONG)&mem[100]; R.d[1] = lens[i];
            R.a[3] = (ULONG)out + 3);
        OBS("d0", R.d[0]);
        OBS("a3", R.a[3] - (ULONG)out);
        OBSBYTES("out", out, 256);
    }
    ROUT = "Poke";
    for (i = 0; i < sizeof types / sizeof types[0]; i++) {
        ULONG data = rnd();
        sprintf(VEC, "type $%lX", types[i]);
        DO1(E("memacclib", 0), R.d[0] = types[i]; R.d[1] = (ULONG)&mem[10];
            R.d[2] = (types[i] & 0xffff) == 7 ? (ULONG)&mem[100] : data;
            frame[1] = 1 + i; R.a[2] = (ULONG)&frame[2]);
        OBSBYTES("mem", mem, 256);
        OBS("mem[10..20] sum", mem[10] + mem[11] * 3 + mem[12] * 5 + mem[13] * 7
             + mem[14] * 11 + mem[20]);
    }
    /* Poke.s with length 0: only the 0 */
    strcpy(VEC, "string len 0");
    DO1(E("memacclib", 0), R.d[0] = 7; R.d[1] = (ULONG)&mem[30];
        R.d[2] = (ULONG)&mem[100]; frame[1] = 0; R.a[2] = (ULONG)&frame[2]);
    OBSBYTES("mem", mem, 256);

    /* Call: moveq #7,d3 / move.l a0,d4 / rts */
    ROUT = "Call";
    strcpy(VEC, "");
    {
        static const UWORD code[] = { 0x7607, 0x2808, 0x4e75 };
        memcpy(&mem[200], code, sizeof code);
        CacheClearU();
        DO1(E("memacclib", 3), R.d[0] = (ULONG)&mem[200]);
        OBS("d3", R.d[3]);
        OBS("d4 = a0 = address", R.d[4] == (ULONG)&mem[200]);
    }
}

/* ------------------------------------------------------------ sortlib */

#define NSORT 48
static UBYTE arr[NSORT * 4 + 16];
static ULONG sortframe[4];
static ULONG ffpbase;
static UBYTE strpool[NSORT][16];

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
    int i;

    for (i = 0; i < (int)sizeof arr; i++)
        arr[i] = (UBYTE)(0xA5 ^ i);             /* a guard behind the array */
    for (i = 0; i < n; i++) {
        ULONG v = pattern == 0 ? rnd() : pattern == 1 ? (ULONG)i : pattern == 2 ? (ULONG)(n - i)
                  : (rnd() & 3);
        UBYTE *p = arr + i * size;
        switch (type) {
        case 1: *p = (UBYTE)v; break;
        case 2: *(UWORD *)p = (UWORD)v; break;
        case 5: *(ULONG *)p = int2ffp((LONG)(v & 0xffff) - 30000); break;
        case 7: {
            UBYTE *q = strpool[i] + 4;
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
    sprintf(VEC, "type %u n %d %s pattern %d", type, n, down ? "down" : "up", pattern);
    DO1(E("sortlib", down ? 2 : 1), (void)0);
    DO1(E("sortlib", 0), R.d[0] = (ULONG)arr;
        sortframe[1] = type; sortframe[2] = (ULONG)(arr + n * size);
        R.a[2] = (ULONG)&sortframe[3];   /* -4 high, -6 type.w */
        R.a[6] = ffpbase);
    if (type == 7) {
        /* the order: for every slot of the sorted array the number of the
           string it points to ($FF: no string of the pool) */
        UBYTE ord[NSORT];
        for (i = 0; i < n; i++) {
            ULONG off = *(ULONG *)(arr + i * 4) - (ULONG)strpool;
            ord[i] = off < sizeof strpool && off % 16 == 4 ? (UBYTE)(off / 16) : 0xff;
        }
        OBSBYTES("order", ord, n);
    } else
        OBSBYTES("array", arr, sizeof arr);
}

static void test_sortlib(void)
{
    static const UWORD types[] = { 1, 2, 3, 4, 5, 7, 0 };
    static const int ns[] = { 1, 2, 3, 7, 16, NSORT };
    unsigned t, k;
    int d, pat;

    ROUT = "sortlib init";
    strcpy(VEC, "");
    DO1(E("sortlib", 3), (void)0);
    ROUT = "Sort";
    for (t = 0; t < sizeof types / sizeof types[0]; t++)
        for (k = 0; k < sizeof ns / sizeof ns[0]; k++)
            for (d = 0; d < 2; d++)
                for (pat = 0; pat < 4; pat++) {
                    if (types[t] == 2 && ns[k] == 1)
                        continue;               /* pivot behind the array, see sortlib.c */
                    sort_one(types[t], ns[k], d, pat);
                }
    DO1(E("sortlib", 1), (void)0);
}

/* -------------------------------------------------------- mathtranslib */

static void test_mathtranslib(void)
{
    int i;
    ULONG base, mt;
    struct Library *hold;

    ROUT = "mathtranslib init";
    strcpy(VEC, "");
    DO1(E("mathtranslib", 14), (void)0);
    base = R.d[0];
    /* relative to mathtrans.library's base: not the address itself */
    Forbid();
    mt = (ULONG)FindName(&SysBase->LibList, (CONST_STRPTR)"mathtrans.library");
    Permit();
    OBS("base - mathtrans.library", R.d[0] - mt);
    /* inline routines: `Dc $a000,len` + JSR lvo(a6); compare the bytes */
    ROUT = "mathtranslib inline";
    for (i = 0; i <= 13; i++) {
        sprintf(VEC, "entry %d", i);
        OBS("prefix+len", L(E("mathtranslib", i)));
        OBS("code", L(E("mathtranslib", i) + 4));
    }
    /* the test holds mathtrans.library open itself: neither finit's
       CloseLibrary is the last one, which on Workbench 3.1 (mathtrans.library
       37.1 from LIBS:) goes another way through the library's Close and
       leaves another a1 - it would depend on the order of the two calls */
    ROUT = "mathtranslib finit";
    strcpy(VEC, "");
    hold = OpenLibrary((CONST_STRPTR)"mathtrans.library", 0);
    check("mathtranslib: mathtrans.library held open", hold != NULL);
    DO1(E("mathtranslib", 15), R.a[1] = base);
    if (hold)
        CloseLibrary(hold);
}

/* ------------------------------------------------------------ utillib */

static ULONG lenframe[2];

static void test_utillib(void)
{
    static const char *pnames[] = { "bb2 systest port", "", "p", "another" };
    ULONG port[4];
    int i;

    ROUT = "utillib init";
    strcpy(VEC, "");
    DO1(E("utillib", 4), (void)0);
    OBS("*init = 0", *(ULONG *)R.d[0]);

    ROUT = "TaskName$";
    for (i = 0; i < 2; i++) {
        sprintf(VEC, "task %s", i ? "FindTask(0)" : "0");
        memset(out, 0xEE, 256);
        DO1(E("utillib", 3), R.d[0] = i ? (ULONG)FindTask(NULL) : 0;
            R.a[3] = (ULONG)out);
        OBS("d0", R.d[0]);
        OBS("a3", R.a[3] - (ULONG)out);
        OBSBYTES("out", out, 256);
    }

    ROUT = "CreateMsgPort";
    for (i = 0; i < 4; i++) {
        sprintf(VEC, "\"%s\"", pnames[i]);
        DONTCARE = 1UL << 11;           /* a3 = the name: observed below */
        DO1(E("utillib", 0), R.d[0] = (ULONG)pnames[i];
            lenframe[0] = strlen(pnames[i]); R.a[2] = (ULONG)&lenframe[1]);
        DONTCARE = 0;
        port[i] = R.d[0];
        OBS("port != 0", R.d[0] != 0);
        OBS("a3 = name", R.a[3] == (ULONG)pnames[i]);
        OBS("type/pri/flags", *(ULONG *)(R.d[0] + 8) & 0xffff0000UL);
        OBS("flags", *(UBYTE *)(R.d[0] + 14));
        OBS("sigtask", *(ULONG *)(R.d[0] + 16) == (ULONG)FindTask(NULL));
        OBS("name", *(ULONG *)(R.d[0] + 10) ? strcmp((char *)*(ULONG *)(R.d[0] + 10),
                                                           pnames[i]) + 1000 : 0);
        OBS("name size", *(ULONG *)(R.d[0] + 10) ? *(ULONG *)(*(ULONG *)(R.d[0] + 10) - 4) : 0);
        OBS("msglist", *(ULONG *)(R.d[0] + 20) == R.d[0] + 24
             && *(ULONG *)(R.d[0] + 28) == R.d[0] + 20);
        OBS("findport", *pnames[i] ? FindPort((STRPTR)pnames[i]) != NULL : 0);
    }
    OBS("sigbits differ", *(UBYTE *)(port[0] + 15) - *(UBYTE *)(port[1] + 15));

    ROUT = "DeleteMsgPort";
    strcpy(VEC, "middle");
    DO1(E("utillib", 1), R.d[0] = port[1]);
    OBS("links", *(ULONG *)(port[0] - 8) == port[2] - 8
         && *(ULONG *)(port[2] - 4) == port[0] - 8);
    strcpy(VEC, "public");
    DO1(E("utillib", 1), R.d[0] = port[0]);
    OBS("findport gone", FindPort((STRPTR)pnames[0]) == NULL);

    /* Wait: signal ourselves first */
    ROUT = "Wait";
    strcpy(VEC, "");
    markregs(&R);
    IN = R;
    Signal(FindTask(NULL), SIGBREAKF_CTRL_F);
    CCR = callstk(E("utillib", 2), &R, NULL, 0, &POP);
    preserve();
    /* other pending signals of the test task may differ between the calls */
    OBS("d0 & CTRL_F", R.d[0] & SIGBREAKF_CTRL_F);

    ROUT = "utillib finit";
    DO1(E("utillib", 5), (void)0);
    OBS("findport gone", FindPort((STRPTR)pnames[3]) == NULL);
}

/* ---------------------------------------------------------- environlib */

static void test_environlib(void)
{
    /* the functions in the order of the token table: their result is a
       long, or a word (the upper word of d0 is undefined: only d0.w is
       compared) */
    static const struct { const char *name; char word; } fn[] = {
        { "WBWidth", 0 }, { "WBHeight", 0 }, { "WBDepth", 1 }, { "WBViewMode", 1 },
        { "Processor", 1 }, { "cr_vbr", 0 }, { "ExecVersion", 1 }, { "WBExtViewMode", 1 } };
    struct Library *ib = OpenLibrary((STRPTR)"intuition.library", 0);
    int i;

    ROUT = "environlib init";
    strcpy(VEC, "");
    DO1(E("environlib", 8), R.a[6] = (ULONG)ib);
    for (i = 0; i < 8; i++) {
        ROUT = fn[i].name;
        /* ExecVersion: called with the upper word of d0 0, as ElmoreSysLib
           compares the result as a long (CMP.l #36,d0); the C build returns
           a zero-extended long */
        DO1(E("environlib", i), if (i == 6) R.d[0] = 0);
        if (fn[i].word && i != 6)
            OBS("d0.w", R.d[0] & 0xffff);
        else
            OBS("d0", R.d[0]);
    }
    CloseLibrary(ib);
}

/* --------------------------------------- exceptions of the register rule */

/* The entries that keep registers beyond the rule because a known caller
   needs them (docs/libs/deviations.md, "Register preservation"): their
   KEEPS state exactly those registers.  The library variables are in the
   libraries themselves; without an event or a key matrix the entries read
   their initial values (EventWindow: no window, -1). */
static void test_exceptions(void)
{
    static const struct { const char *name; int idx; } getter[] = {
        { "EMouseX", 50 }, { "EMouseY", 51 }, { "EventCode", 63 } };
    unsigned i;

    for (i = 0; i < sizeof getter / sizeof getter[0]; i++) {
        ROUT = getter[i].name;
        strcpy(VEC, "");
        DO1(E("windowslib", getter[i].idx), (void)0);
        OBS("d0.w", R.d[0] & 0xffff);
    }
    ROUT = "EventWindow";
    DO1(E("windowslib", 4), (void)0);
    OBS("d0.w", R.d[0] & 0xffff);
    ROUT = "RawStatus";
    for (i = 0; i < 2; i++) {
        sprintf(VEC, "rawkey %u", i * 69);
        DO1(E("blitzkeyslib", 1), R.d[0] = i * 69);
    }
    /* Rnd: hardware noise, the result is not compared */
    ROUT = "Rnd";
    strcpy(VEC, "");
    DO1(cmd_form(getlib(&loaded, "vallib"), 6, 0), (void)0);
    ROUT = "Rnd(Range)";
    strcpy(VEC, "range 100");
    DO1(cmd_form(getlib(&loaded, "vallib"), 6, 1), R.d[0] = 100);
}

/* ---------------------------------------------------------- audiolib */

/* InitSound Sound#,Length: an empty sound in chip memory.  The Sound
   object comes in a3 (the compiler's !libs {#audiolib,$1380}); the memory
   is memlib's (freed by its finit).  It keeps a0 (#1058). */
static void test_audiolib(void)
{
    static UBYTE snd[16];

    ROUT = "memlib init";
    strcpy(VEC, "audiolib");
    DO1(cmd_entry(getlib(&loaded, "memlib"), 7), (void)0);
    ROUT = "InitSound";
    strcpy(VEC, "sound 0, 64 bytes");
    memset(snd, 0, sizeof snd);
    DO1(E("audiolib", 7), R.d[0] = 0; R.d[1] = 64; R.a[3] = (ULONG)snd);
    OBS("data != 0", L(snd) != 0);
    OBS("period, length", L(snd + 4));
    OBS("repeat - data", L(snd + 8) - L(snd));
    OBS("repeat length, mark", L(snd + 12));
    ROUT = "memlib finit";
    strcpy(VEC, "audiolib");
    DO1(cmd_entry(getlib(&loaded, "memlib"), 8), (void)0);
}

/* ------------------------------------------------------------------ main */

static const char *names[] = {
    "memlib", "strcomplib", "memacclib", "sortlib", "mathtranslib", "utillib",
    "environlib", "maxslib", "inputoutputlib", "rawkeylib", "windowslib", "blitzkeyslib",
    "floatquicklib", "vallib", "iffiolib", "audiolib", NULL
};

int main(int argc, char **argv)
{
    struct Library *ffp;

    if (argc < 2) {
        printf("usage: systest_cmdlibs DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS_FUNCTION("Peek", none);
    KEEPS_STRFUNCTION("Peeks$", none);
    KEEPS_STATEMENT("Poke", none);
    KEEPS_STATEMENT("Call", none);
    KEEPS("sortlib init", d0-d7|a0-a3|a6);
    KEEPS_STATEMENT("Sort", none);
    KEEPS("mathtranslib init", d2-d7|a2|a3);
    KEEPS("mathtranslib finit", d2-d7|a1-a3);
    KEEPS("environlib init", d3-d7|a2|a3);
    /* exceptions: the CD program WBDEMO.BB2 holds a0/a1 across WBWidth,
       WBHeight and WBDepth */
    KEEPS_FUNCTION("WBWidth", a0|a1);
    KEEPS_FUNCTION("WBHeight", a0|a1);
    KEEPS_FUNCTION("WBDepth", a0|a1);
    KEEPS_FUNCTION("WBViewMode", none);
    KEEPS_FUNCTION("Processor", none);
    KEEPS_FUNCTION("cr_vbr", none);
    KEEPS_FUNCTION("ExecVersion", none);
    KEEPS_FUNCTION("WBExtViewMode", none);
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("utillib init", d1-d7|a0-a3|a6);
    /* the original loses a6 in TaskName$ and Wait: the rule's a6 is not
       stated for them (an original bug to decide, see the report of
       libs-performance group 2) */
    KEEPS("TaskName$", none);
    KEEPS_FUNCTION("CreateMsgPort", none);
    KEEPS_STATEMENT("DeleteMsgPort", none);
    KEEPS("Wait", none);
    KEEPS("utillib finit", d1-d7|a1-a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    /* exceptions (test_exceptions): wzlib holds a0 across EMouseX, EMouseY
       and EventCode and combines EMouseX<<16|EMouseY, and a1 across
       EventWindow; RILesDebugLib a1 across RawStatus; the CD programs
       SQUIRT2_TO and SQUIRT3_TO a0/a1 across Rnd */
    KEEPS_FUNCTION("EMouseX", a0|d0u);
    KEEPS_FUNCTION("EMouseY", a0|d0u);
    KEEPS_FUNCTION("EventCode", a0|d0u);
    KEEPS_FUNCTION("EventWindow", a1);
    KEEPS_FUNCTION("RawStatus", a1);
    KEEPS_FUNCTION("Rnd", a0|a1);
    KEEPS_FUNCTION("Rnd(Range)", a0|a1);
    /* exception: ElmoreIncLib's IncSound reads (a0) after InitSound */
    KEEPS_STATEMENT("InitSound", a0);
    ffp = OpenLibrary((STRPTR)"mathffp.library", 0);
    ffpbase = (ULONG)ffp;

    test_memacclib();
    test_sortlib();
    test_mathtranslib();
    test_environlib();
    test_exceptions();

    /* The memory balance is the task's own (rwemem.h: exec's AllocMem/FreeMem
       are counted for this task only): AvailMem() counts the whole system,
       and other tasks allocate and free while this runs.  The cases of this
       part stay in capture.c's 64 KB buffer: it is flushed after every 64 KB
       of the program's output, and this part is written between 92 and
       95 KB. */
    rwe_mem_start();
    ROUT = "memlib init";
    strcpy(VEC, "");
    DO1(cmd_entry(getlib(&loaded, "memlib"), 7), (void)0);
    membase = R.d[0];
    test_utillib();
    ROUT = "memlib finit";
    DO1(cmd_entry(getlib(&loaded, "memlib"), 8), (void)0);
    check("utillib: all memory back", rwe_mem_back());
    test_audiolib();

    CloseLibrary(ffp);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
