/*
 * systest_cmdlibs - the basic command libraries
 *   memacclib, sortlib, mathtranslib, utillib, environlib, and the small
 *   entries of vallib, scrolllib, gameiolib, mouselib, collslib,
 *   shapeslib, fontlib, windowslib and blitzkeyslib
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
        /* the original loses a6 in TaskName$ of task 0 (#1060) */
        OBS("a6 kept", R.a[6] == IN.a[6]);
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
    OBS("a6 kept", R.a[6] == IN.a[6]);           /* the original loses it (#1060) */
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
    /* ExecVersion with the upper word of d0 set before the call: the
       original leaves it (a word result), the C build returns a
       zero-extended long, which the callers' long compare needs.  Both
       sets give the same d0.w; the zero extension is checked on the C
       build only (the original has no such contract) */
    ROUT = "ExecVersion";
    strcpy(VEC, "d0 upper word set");
    DO1(E("environlib", 6), R.d[0] = 0x5a5a0000);
    OBS("d0.w", R.d[0] & 0xffff);
    if (!strcmp(loaded.what, "C"))
        check("ExecVersion: the upper word of d0 is 0 although it was set", R.d[0] <= 0xffff);
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

/* ------------------------------------------------------- small entries */

/* code of form `form` of the command `name` (the names are the same in
   both sets; found by name, not by the entry's place in the table) */
static UBYTE *cmd_named(struct lib *l, const char *name, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2 + 12;
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
            if (!strcmp((const char *)p, name)) {
                check("cmd_named: the form exists", code != NULL);
                return (UBYTE *)((ULONG)code & ~1UL);
            }
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
    }
    check("cmd_named: the command exists", FALSE);
    return NULL;
}

#define N(lib, name, form) cmd_named(getlib(&loaded, lib), (name), (form))

/* the registers d2-d7 and a2 (kept(): bits 2-7 and 10) */
#define KEPT_D2D7A2 0x04fcUL

static UBYTE bmplanes[3][32];           /* a bitmap of 3 planes, 4 x 8 bytes */
static ULONG bmobj[16];                 /* its struct bb_bitmap, 64 bytes */
static UBYTE shobj[32];                 /* a struct bb_shape */

static void mkbitmap(void)
{
    UBYTE *b = (UBYTE *)bmobj;
    int i;
    memset(bmobj, 0, sizeof bmobj);
    memset(bmplanes, 0x5a, sizeof bmplanes);
    *(UWORD *)(b + 0) = 4;                      /* bpr */
    *(UWORD *)(b + 2) = 8;                      /* rows */
    *(UWORD *)(b + 4) = 3;                      /* flags 0, depth 3 */
    for (i = 0; i < 3; i++)
        *(UBYTE **)(b + 8 + 4 * i) = bmplanes[i];
    *(WORD *)(b + 48) = 2;                      /* origin x */
    *(WORD *)(b + 50) = 1;                      /* origin y */
    *(UWORD *)(b + 56) = 30;                    /* clip width */
    *(UWORD *)(b + 58) = 7;                     /* clip height */
}

/* The small entries (OpenSpec change libs-performance, group 6: assembler
   entries in the original's form): their entry conditions - the upper
   words of word arguments, every argument type, the clipping, the limits
   - against both sets.  Hardware-dependent results (Joyx ..., Rnd,
   DoColl) are taken as the emulator gives them, the rest exactly. */
static void test_smallentries(void)
{
    static const struct { ULONG x, y, c; } plots[] = {
        { 0, 0, 5 }, { 3, 2, 7 }, { 0xabcd000a, 0x12340003, 0x77770002 },
        { 27, 5, 0 }, { 17, 0, 0xffff }, { 26, 3, 0x80000001 }, { 9, 4, 0xfffe },
        { 28, 0, 1 }, { 0, 6, 1 }, { 0xfffe, 0, 1 }, { 0, 0xffff, 3 }, { 1, 1, 0x7fff } };
    static const ULONG qlim[][3] = {
        { 5, 0, 10 }, { (ULONG)-5, 0, 10 }, { 10, 0, 10 }, { 0, 0, 10 }, { 11, 0, 10 },
        { 0x80000000, 0, 0x7fffffff }, { 0x7fffffff, 0x80000000, 0x7fffffff },
        { 0xfff80000, 0xfff00000, 0x00100000 }, { 3, 10, 0 }, { 20, 10, 0 },
        { 0x00018000, 0x00010000, 0x00020000 } };
    static const struct { UWORD type; ULONG v; } nums[] = {
        { 1, 0x123456f0 }, { 1, 0x12345670 }, { 1, 0 }, { 0, 0xabcd0081 },
        { 2, 0x1234fff0 }, { 2, 0x12340010 }, { 2, 0 }, { 3, 0xfffffff0 },
        { 3, 0x00012345 }, { 3, 0 }, { 4, 0x00038000 }, { 4, 0xfffc8000 },
        { 4, 0xffff0000 }, { 4, 0x00004000 }, { 4, 0 }, { 4, 0x80000000 },
        { 5, 0xa0000042 }, { 5, 0xa00000c2 }, { 5, 0 }, { 5, 0xc0000041 },
        { 6, 0x00000007 }, { 8, 0xfffffffb } };
    static UWORD tframe[2];
    UBYTE *p;
    unsigned i, j;

    /* Plot, Point: scrolllib on a bitmap object made here (origin 2,1,
       clip 30 x 7) */
    mkbitmap();
    ROUT = "Plot";
    for (i = 0; i < sizeof plots / sizeof plots[0]; i++) {
        sprintf(VEC, "x $%lX y $%lX colour $%lX", plots[i].x, plots[i].y, plots[i].c);
        DO1(N("scrolllib", "Plot", 0), R.d[0] = plots[i].x; R.d[1] = plots[i].y;
            R.d[2] = plots[i].c; R.a[2] = (ULONG)bmobj);
        OBSBYTES("planes", bmplanes, sizeof bmplanes);
        /* the port keeps d2-d7 and a2 (MikesSpaceFXLib calls Plot from a
           loop); the original changes d2 and d3 */
        if (!strcmp(loaded.what, "C"))
            check("Plot: d2-d7 and a2 kept", (kept(&IN, &R) & KEPT_D2D7A2) == KEPT_D2D7A2);
    }
    ROUT = "Point";
    for (i = 0; i < sizeof plots / sizeof plots[0]; i++) {
        ULONG x = (plots[i].x & 0xffff) | 0x5a5a0000UL;
        sprintf(VEC, "x $%lX y $%lX", x, plots[i].y);
        DO1(N("scrolllib", "Point", 0), R.d[0] = x; R.d[1] = plots[i].y;
            R.a[2] = (ULONG)bmobj);
        OBS("d0", R.d[0]);
    }

    /* Joyx, Joyy, Joyr, Joyb: the hardware as the emulator has it; the
       port is a word (TST.w d0), the upper word is no part of it */
    {
        static const char *joys[] = { "Joyx", "Joyy", "Joyr", "Joyb" };
        static const ULONG ports[] = { 0, 1, 2, 0x12340000, 0xffff0001 };
        for (i = 0; i < 4; i++) {
            ROUT = joys[i];
            for (j = 0; j < sizeof ports / sizeof ports[0]; j++) {
                sprintf(VEC, "port $%lX", ports[j]);
                DO1(N("gameiolib", joys[i], 0), R.d[0] = ports[j]);
                OBS("d0", R.d[0]);
            }
        }
    }

    /* MouseX ... after PositionMouse (the speeds: their initial value) */
    ROUT = "PositionMouse";
    strcpy(VEC, "x $5A5A0123 y $A5A5FF80");
    DO1(N("mouselib", "PositionMouse", 0), R.d[0] = 0x5a5a0123; R.d[1] = 0xa5a5ff80);
    {
        static const char *mice[] = { "MouseX", "MouseY", "MouseXSpeed", "MouseYSpeed" };
        for (i = 0; i < 4; i++) {
            ROUT = mice[i];
            strcpy(VEC, "after PositionMouse");
            DO1(N("mouselib", mice[i], 0), (void)0);
            OBS("d0", R.d[0]);
        }
    }

    /* RawStatus with the upper word of d0 set (no key matrix: the library
       variables are 0, the bit is read from low memory, the same in both
       sets) */
    ROUT = "RawStatus";
    for (i = 0; i < 3; i++) {
        sprintf(VEC, "rawkey $5A5A%04X", i * 33);
        DO1(N("blitzkeyslib", "RawStatus", 0), R.d[0] = 0x5a5a0000UL | (i * 33));
        OBS("d0.w", R.d[0] & 0xffff);
    }

    /* QLimit: signed limits, q = low and q = high, low > high */
    ROUT = "QLimit";
    for (i = 0; i < sizeof qlim / sizeof qlim[0]; i++) {
        sprintf(VEC, "q $%lX low $%lX high $%lX", qlim[i][0], qlim[i][1], qlim[i][2]);
        DO1(N("vallib", "QLimit", 0), R.d[0] = qlim[i][0]; R.d[1] = qlim[i][1];
            R.d[2] = qlim[i][2]);
        OBS("d0", R.d[0]);
    }

    /* Rnd(Range): the range is an unsigned word; the result (hardware
       noise, not compared as a value) is below range << 16 */
    ROUT = "Rnd(Range)";
    {
        static const ULONG ranges[] = { 0, 1, 0x5a5a0064, 0xffff, 0x00010000 };
        for (i = 0; i < sizeof ranges / sizeof ranges[0]; i++) {
            ULONG r = ranges[i] & 0xffff;
            sprintf(VEC, "range $%lX", ranges[i]);
            DO1(N("vallib", "Rnd", 1), R.d[0] = ranges[i]);
            OBS("d0 in range", r ? R.d[0] < (r << 16) : R.d[0] == 0);
        }
    }

    /* Int, Sgn, Abs: every type, the upper bits of byte and word values;
       the type word at -2(a2), a6 = mathffp */
    {
        static const char *fns[] = { "Int", "Sgn", "Abs" };
        for (j = 0; j < 3; j++) {
            ROUT = fns[j];
            for (i = 0; i < sizeof nums / sizeof nums[0]; i++) {
                if (j < 2 && nums[i].type >= 6)
                    continue;                   /* intchk refuses them */
                sprintf(VEC, "type %u value $%lX", nums[i].type, nums[i].v);
                tframe[0] = nums[i].type;
                DO1(N("vallib", fns[j], 0), R.d[0] = nums[i].v;
                    R.a[2] = (ULONG)&tframe[1]; R.a[6] = ffpbase);
                OBS("d0", R.d[0]);
            }
        }
    }

    /* PColl on chosen values of colval, DoColl's variable: its address is
       the operand of DoColl's MOVE $DFF00E,colval, the same instruction in
       both sets.  DoColl itself reads CLXDAT as the emulator has it. */
    p = N("collslib", "DoColl", 0);
    ROUT = "DoColl";
    strcpy(VEC, "");
    DO1(p, (void)0);
    if (p && W(p) == 0x33f9 && L(p + 2) == 0xdff00eUL) {
        UWORD *colval = (UWORD *)L(p + 6);
        static const UWORD vals[] = { 0, 0xffff, 0x0022, 0x0044, 0x0088, 0x0110, 0x0002,
                                      0x0020, 0x8001, 0x7ffe };
        ROUT = "PColl";
        for (i = 0; i < sizeof vals / sizeof vals[0]; i++)
            for (j = 0; j < 9; j++) {
                ULONG ch = j < 8 ? 0x5a5a0000UL | j : 0x0000fffeUL;
                sprintf(VEC, "colval $%04X channel $%lX", vals[i], ch);
                *colval = vals[i];
                DO1(N("collslib", "PColl", 0), R.d[0] = ch);
                OBS("d0", R.d[0]);
            }
    } else
        NOT_CAPTURED("DoColl instruction", "not MOVE.w $DFF00E,abs.l");

    /* ShapeWidth, ShapeHeight (a0 = the shape), Handle (a3) */
    memset(shobj, 0xa5, sizeof shobj);
    *(UWORD *)shobj = 0x8123;
    *(UWORD *)(shobj + 2) = 0x0456;
    ROUT = "ShapeWidth";
    strcpy(VEC, "width $8123");
    DO1(N("shapeslib", "ShapeWidth", 0), R.a[0] = (ULONG)shobj);
    OBS("d0.w", R.d[0] & 0xffff);
    ROUT = "ShapeHeight";
    strcpy(VEC, "height $0456");
    DO1(N("shapeslib", "ShapeHeight", 0), R.a[0] = (ULONG)shobj);
    OBS("d0.w", R.d[0] & 0xffff);
    ROUT = "Handle";
    strcpy(VEC, "x $AAAA0010 y $BBBBFFF0");
    DO1(N("shapeslib", "Handle", 0), R.d[1] = 0xaaaa0010; R.d[2] = 0xbbbbfff0;
        R.a[3] = (ULONG)shobj);
    OBSBYTES("shape", shobj, sizeof shobj);

    /* Colour (it sets library variables only: the kept case), Locate on
       the output bitmap of BitMapOutput, read back with CursX and CursY */
    ROUT = "Colour";
    strcpy(VEC, "fg $5A5A0003");
    DO1(N("fontlib", "Colour", 0), R.d[0] = 0x5a5a0003);
    strcpy(VEC, "fg $5A5A0002 bg $A5A50001");
    DO1(N("fontlib", "Colour", 1), R.d[0] = 0x5a5a0002; R.d[1] = 0xa5a50001);
    mkbitmap();
    ROUT = "BitMapOutput";
    strcpy(VEC, "");
    DO1(N("fontlib", "BitMapOutput", 0), R.a[0] = (ULONG)bmobj);
    {
        static const ULONG locs[][2] = {
            { 0, 0 }, { 3, 0x00020000 }, { 0x5a5a0007, 0x00058000 }, { 0x1fff, 0x0003ffff },
            { 0xffff, 0xfffe0000 }, { 2, 0x2000a000 } };
        for (i = 0; i < sizeof locs / sizeof locs[0]; i++) {
            sprintf(VEC, "x $%lX y $%lX", locs[i][0], locs[i][1]);
            ROUT = "Locate";
            DO1(N("fontlib", "Locate", 0), R.d[0] = locs[i][0]; R.d[1] = locs[i][1]);
            OBSBYTES("cursor", (UBYTE *)bmobj + 40, 4);
            ROUT = "CursX";
            DO1(N("fontlib", "CursX", 0), (void)0);
            OBS("d0", R.d[0]);
            ROUT = "CursY";
            DO1(N("fontlib", "CursY", 0), (void)0);
            OBS("d0", R.d[0]);
        }
    }

    /* GadgetHit ... EventQualifier: the library's values without an
       event (the form of EMouseX, test_exceptions) */
    {
        static const char *evs[] = { "GadgetHit", "MenuHit", "ItemHit", "SubHit",
                                     "EventQualifier" };
        for (i = 0; i < 5; i++) {
            ROUT = evs[i];
            strcpy(VEC, "no event");
            DO1(N("windowslib", evs[i], 0), (void)0);
            OBS("d0.w", R.d[0] & 0xffff);
        }
    }
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
    "floatquicklib", "vallib", "iffiolib", "audiolib", "scrolllib", "gameiolib", "mouselib",
    "collslib", "shapeslib", "fontlib", NULL
};

/* the tokens of the libraries the small entries call and this program
   does not load (spriteslib, intlib, bitmaplib, ilbmifflib, palettelib:
   Pointer, Mouse, LoadShape ..., never called here): an RTS each */
static const UWORD stubrts[] = { 0x4e75 };
static struct stub stubs[5 * 16 + 1];

static void mkstubs(void)
{
    static const UWORD bases[] = { 112 << 7, 0xc100, 160 << 7, 161 << 7, 156 << 7 };
    int i, j, n = 0;
    for (i = 0; i < 5; i++)
        for (j = 0; j < 16; j++) {
            stubs[n].tok = (UWORD)(bases[i] + j);
            stubs[n++].code = (APTR)stubrts;
        }
    stubs[n].tok = 0;
    stubs[n].code = NULL;
    sysload_stubs = stubs;
}

int main(int argc, char **argv)
{
    struct Library *ffp;

    if (argc < 2) {
        printf("usage: systest_cmdlibs DIR\n");
        return 20;
    }
    mkstubs();
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
    /* the original loses a6 in TaskName$ and Wait (#1060): the rule's a6
       is not stated for them here, the cases "a6 kept" are the pair
       [orig] / [fixed #1060] */
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
    /* the small entries (test_smallentries); Plot's d2-d7/a2 are checked
       on the C build only (the original changes d2 and d3) */
    KEEPS_STATEMENT("Plot", none);
    KEEPS_FUNCTION("Point", none);
    KEEPS_FUNCTION("Joyx", none);
    KEEPS_FUNCTION("Joyy", none);
    KEEPS_FUNCTION("Joyr", none);
    KEEPS_FUNCTION("Joyb", none);
    KEEPS_STATEMENT("PositionMouse", none);
    KEEPS_FUNCTION("MouseX", none);
    KEEPS_FUNCTION("MouseY", none);
    KEEPS_FUNCTION("MouseXSpeed", none);
    KEEPS_FUNCTION("MouseYSpeed", none);
    KEEPS_FUNCTION("QLimit", none);
    KEEPS_FUNCTION("Int", none);
    KEEPS_FUNCTION("Sgn", none);
    KEEPS_FUNCTION("Abs", none);
    KEEPS_STATEMENT("DoColl", none);
    KEEPS_FUNCTION("PColl", none);
    KEEPS_FUNCTION("ShapeWidth", none);
    KEEPS_FUNCTION("ShapeHeight", none);
    KEEPS_STATEMENT("Handle", none);
    KEEPS_STATEMENT("Colour", none);
    KEEPS_STATEMENT("BitMapOutput", none);
    KEEPS_STATEMENT("Locate", none);
    KEEPS_FUNCTION("CursX", none);
    KEEPS_FUNCTION("CursY", none);
    KEEPS_FUNCTION("GadgetHit", none);
    KEEPS_FUNCTION("MenuHit", none);
    KEEPS_FUNCTION("ItemHit", none);
    KEEPS_FUNCTION("SubHit", none);
    KEEPS_FUNCTION("EventQualifier", none);
    ffp = OpenLibrary((STRPTR)"mathffp.library", 0);
    ffpbase = (ULONG)ffp;

    test_memacclib();
    test_sortlib();
    test_mathtranslib();
    test_environlib();
    test_exceptions();
    test_smallentries();

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
