/*
 * systest_cmdlibs - the basic command libraries
 *   memacclib, sortlib, mathtranslib, utillib, environlib, and the small
 *   entries of vallib, scrolllib, gameiolib, mouselib, collslib,
 *   shapeslib, fontlib, windowslib and blitzkeyslib; the rectangle tests
 *   of collslib, Frac, Val, BlitzKey and Inkey$ (editlib), Print on a bitmap
 *   (printlib, fontlib), Str$ and UStr$ (printlib), Instr, Left$, UnLeft$,
 *   Right$, UnRight$, Mid$, Hex$, Bin$, Cvi, Cvl and Cvq (stringfunclib),
 *   Sound and LoopSound (audiolib), the list commands (linklistlib)
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
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfxb
#include <proto/graphics.h>
#undef __NOLIBBASE__

static struct Library *gfxb;            /* graphics.library (test_print) */

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
/* the long arrays (201 elements, the length of the benchmark's Sort) */
#define NBIG 201
static UBYTE bigarr[NBIG * 4 + 16];
static UBYTE bigpool[NBIG][16];

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

/* sort n elements of `type` in arr (asize bytes, the strings in pool);
   pattern 0 random, 1 ascending, 2 descending, 3 four values, 4 all
   equal (the order of equal strings shows which slots the sort swaps) */
static void sort_in(UWORD type, int n, int down, int pattern, UBYTE *arr, size_t asize,
                    UBYTE (*strpool)[16], size_t poolsize)
{
    int size = type == 1 ? 1 : type == 2 ? 2 : 4;
    int i;

    for (i = 0; i < (int)asize; i++)
        arr[i] = (UBYTE)(0xA5 ^ i);             /* a guard behind the array */
    for (i = 0; i < n; i++) {
        ULONG v = pattern == 0 ? rnd() : pattern == 1 ? (ULONG)i : pattern == 2 ? (ULONG)(n - i)
                  : pattern == 3 ? (rnd() & 3) : 0x00ab2305UL;
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
        UBYTE ord[NBIG];
        for (i = 0; i < n; i++) {
            ULONG off = *(ULONG *)(arr + i * 4) - (ULONG)strpool;
            ord[i] = off < poolsize && off % 16 == 4 ? (UBYTE)(off / 16) : 0xff;
        }
        OBSBYTES("order", ord, n);
    } else
        OBSBYTES("array", arr, asize);
}

static void sort_one(UWORD type, int n, int down, int pattern)
{
    sort_in(type, n, down, pattern, arr, sizeof arr, strpool, sizeof strpool);
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
    /* every element type on 201 elements (OpenSpec change
       libs-performance, task 7.3: one scan loop per type), also all equal;
       the seed is kept, so the cases after these see the same numbers */
    {
        ULONG keep = seed;

        for (t = 0; t < sizeof types / sizeof types[0]; t++)
            for (d = 0; d < 2; d++)
                for (pat = 0; pat < 5; pat++)
                    sort_in(types[t], NBIG, d, pat, bigarr, sizeof bigarr, bigpool,
                            sizeof bigpool);
        seed = keep;
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

/* the registers d4-d7 and a2 (kept(): bits 4-7 and 10) */
#define KEPT_D4D7A2 0x04f0UL

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
        /* the port keeps d4-d7 and a2; it changes d2 and d3 like the
           original, so no caller can rely on them (MikesSpaceFXLib saves
           d0-d2 itself around the call, nothing else of it is live) */
        if (!strcmp(loaded.what, "C"))
            check("Plot: d4-d7 and a2 kept", (kept(&IN, &R) & KEPT_D4D7A2) == KEPT_D4D7A2);
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
        /* SColl for every two channels (also of one pair, and equal,
           which the error check refuses): the bits of colval it reports,
           one call per bit, then the call with every bit set; junk in
           the upper words.  #110: the original tests the wrong bit for
           a pair other than 0 first and pair 0 second. */
        ROUT = "SColl";
        for (i = 0; i < 8; i++)
            for (j = 0; j < 8; j++) {
                ULONG hits = 0, k;

                sprintf(VEC, "channels $5A5A000%u $A5A5000%u", i, j);
                for (k = 0; k < 16; k++) {
                    *colval = (UWORD)(1u << k);
                    markregs(&R);
                    R.d[0] = 0x5a5a0000UL | i;
                    R.d[1] = 0xa5a50000UL | j;
                    callstk(N("collslib", "SColl", 0), &R, NULL, 0, &POP);
                    if ((UWORD)R.d[0] == 0xffff)
                        hits |= 1UL << k;
                    else if ((UWORD)R.d[0] != 0)
                        hits |= 0x10000UL;      /* neither 0 nor -1 */
                }
                OBS("hit bits", hits);
                *colval = 0xffff;
                DO1(N("collslib", "SColl", 0), R.d[0] = 0x5a5a0000UL | i;
                    R.d[1] = 0xa5a50000UL | j);
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

/* Volume Channelmask,Vol1[,Vol2...]: Paula's volume registers are
   write-only, so every absolute address $DFF0A0-$DFF0DF in the code of
   the routine (up to its first RTS: MOVE.l #$DFF0A0,a0 in the original,
   one address per channel in the C build) is patched to a RAM copy,
   filled with a pattern before each call, and restored afterwards (the
   method of systest_tracker).  Every mask, with junk in the upper word
   and in bits 4-15 (which the error check refuses).  Not observed: a2,
   which the original lowers by the volumes it pops and the C build by
   all but the last (docs/libs/deviations.md: the compiler does not read
   a2 after a command). */
static void test_volume(void)
{
    static UBYTE fake[0x40];
    static UWORD vols[6] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666 };
    static const ULONG masks[] = { 0x5a5a0000, 0x5a5a0001, 0x5a5a0002, 0x5a5a0003,
                                   0x5a5a0004, 0x5a5a0005, 0x5a5a0006, 0x5a5a0007,
                                   0x5a5a0008, 0x5a5a0009, 0x5a5a000a, 0x5a5a000b,
                                   0x5a5a000c, 0x5a5a000d, 0x5a5a000e, 0x5a5a000f,
                                   0x0000fff0, 0x0000a5a5 };
    ULONG at[8];
    UBYTE *p = N("audiolib", "Volume", 0);
    unsigned i, n = 0;

    ROUT = "Volume";
    for (i = 0; p && i + 4 <= 80 && W(p + i) != 0x4e75; i += 2) {
        ULONG v = L(p + i);

        if (v >= 0xdff0a0 && v < 0xdff0e0 && n < 8) {
            at[n++] = i;
            p[i] = (UBYTE)((ULONG)fake >> 24);  /* the same offset in fake */
            p[i + 1] = (UBYTE)((ULONG)fake >> 16);
            p[i + 2] = (UBYTE)(((ULONG)fake + v - 0xdff0a0) >> 8);
            p[i + 3] = (UBYTE)((ULONG)fake + v - 0xdff0a0);
            i += 2;
        }
    }
    if (!n) {
        NOT_CAPTURED("audio register addresses", "none in the routine's code");
        return;
    }
    CacheClearU();
    for (i = 0; i < sizeof masks / sizeof masks[0]; i++) {
        ULONG k;

        for (k = 0; k < sizeof fake; k++)
            fake[k] = (UBYTE)(0xa5 ^ k);
        sprintf(VEC, "mask $%lX", masks[i]);
        DO1(p, R.d[0] = masks[i]; R.a[2] = (ULONG)&vols[6]);
        OBSBYTES("audio registers", fake, sizeof fake);
    }
    for (i = 0; i < n; i++) {                   /* restore the code */
        ULONG v = 0xdff0a0 + (L(p + at[i]) - (ULONG)fake);

        p[at[i]] = 0;
        p[at[i] + 1] = 0xdf;
        p[at[i] + 2] = (UBYTE)(v >> 8);
        p[at[i] + 3] = (UBYTE)v;
    }
    CacheClearU();
}

/* Sound and LoopSound Sound#,Channelmask[,Vol1[,Vol2...]]: as for Volume,
   every absolute address $DFF090-$DFF0DF (DMACON and the audio registers)
   in the code of the routine is patched to a RAM copy, and restored
   afterwards.  The code is the one from the entry up to its first RTS and
   the routines it calls there with JSR d16(pc) or BSR (two levels): the
   original's MOVE.l #$DFF0A0,a0 of dosnd and the DMACON write of
   dochansoff, the C build's absolute registers.  The routines wait for the
   real VHPOSR (24 lines a call).  Observed: DMACON (the last write: the
   mask with bit 15, DMA on) and the audio registers after the call, the
   location written as its offset from the sample (the sound's data and
   its repeat part are in `samp`).  The start sets location, length,
   period and volume (64 or the next given one), the repeat part then
   location and length again: a zero word in chip memory, chzero, for
   Sound (0 here: the library's init is not run), the sound's repeat part
   for LoopSound.  Every mask with junk in the upper word (bits 4-15 are
   refused by the error check), each form with volumes with 0, 1, 2 and
   4 of them.  Not observed: a2 (as for Volume). */
static UBYTE sndfake[0x50];
static UBYTE *sndat[64];
static unsigned nsndat;

static void sndpatch(UBYTE *p, struct lib *l, int depth)
{
    unsigned i;

    for (i = 0; p && i + 4 <= 1024 && W(p + i) != 0x4e75; i += 2) {
        ULONG a = L(p + i);
        UWORD w = W(p + i);
        UBYTE *t = NULL;

        if (a >= 0xdff090 && a < 0xdff0e0) {
            if (nsndat == sizeof sndat / sizeof sndat[0]) {
                check("Sound: room for every patched register address", FALSE);
                return;
            }
            sndat[nsndat++] = p + i;
            a = (ULONG)sndfake + a - 0xdff090;
            p[i] = (UBYTE)(a >> 24);
            p[i + 1] = (UBYTE)(a >> 16);
            p[i + 2] = (UBYTE)(a >> 8);
            p[i + 3] = (UBYTE)a;
            i += 2;
            continue;
        }
        if (w == 0x4eba || w == 0x6100)         /* JSR d16(pc), BSR.w */
            t = p + i + 2 + (WORD)W(p + i + 2);
        else if ((w & 0xff00) == 0x6100 && (w & 0xff) != 0xff)
            t = p + i + 2 + (BYTE)(w & 0xff);   /* BSR.s */
        if (t && depth > 0 && t >= l->code && t < l->code + l->size)
            sndpatch(t, l, depth - 1);
    }
}

static void test_sound(void)
{
    static UBYTE seen[0x40];
    static UWORD vols[6] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666 };
    static UWORD samp[16];
    static ULONG snd[4];
    static const char *names[] = { "Sound", "LoopSound" };
    static const ULONG nvols[] = { 0x5a5a0000, 0x5a5a0001, 0x5a5a0002, 0x5a5a0004 };
    struct lib *l = getlib(&loaded, "audiolib");
    unsigned c, form, i, k, v, nv;

    snd[0] = (ULONG)samp;                       /* data */
    snd[1] = 0x01240010;                        /* period, length in words */
    snd[2] = (ULONG)&samp[4];                   /* repeat */
    snd[3] = 0x00060000;                        /* repeat length, mark */
    for (c = 0; c < 2; c++)
        for (form = 0; form < 2; form++) {
            UBYTE *p = N("audiolib", names[c], (int)form);

            ROUT = names[c];
            nsndat = 0;
            sndpatch(p, l, 2);
            if (!nsndat) {
                strcpy(VEC, form ? "with volumes" : "mask only");
                NOT_CAPTURED("audio register addresses", "none in the routine's code");
                continue;
            }
            CacheClearU();
            for (i = 0; i < 16; i++)
                for (nv = 0; nv < (form ? 4u : 1u); nv++) {
                    for (k = 0; k < sizeof sndfake; k++)
                        sndfake[k] = (UBYTE)(0xa5 ^ k);
                    if (form)
                        sprintf(VEC, "mask $%lX, %lu volumes", 0x5a5a0000UL | i,
                                nvols[nv] & 0xffff);
                    else
                        sprintf(VEC, "mask $%lX", 0x5a5a0000UL | i);
                    DO1(p, R.d[1] = 0x5a5a0000UL | i; R.a[3] = (ULONG)snd;
                        if (form) { R.d[7] = nvols[nv]; R.a[2] = (ULONG)&vols[6]; });
                    OBS("DMACON", W(sndfake + 6));
                    memcpy(seen, sndfake + 0x10, sizeof seen);
                    for (k = 0; k < 4; k++) {          /* AUDxLC: offset */
                        v = L(seen + 16 * k);
                        if (v >= (ULONG)samp && v < (ULONG)samp + sizeof samp)
                            v -= (ULONG)samp;
                        else if (v)
                            continue;           /* not written */
                        seen[16 * k] = (UBYTE)(v >> 24);
                        seen[16 * k + 1] = (UBYTE)(v >> 16);
                        seen[16 * k + 2] = (UBYTE)(v >> 8);
                        seen[16 * k + 3] = (UBYTE)v;
                    }
                    OBSBYTES("audio registers", seen, sizeof seen);
                }
            for (i = 0; i < nsndat; i++) {          /* restore the code */
                UBYTE *q = sndat[i];
                ULONG a = 0xdff090 + (L(q) - (ULONG)sndfake);

                q[0] = 0;
                q[1] = 0xdf;
                q[2] = (UBYTE)(a >> 8);
                q[3] = (UBYTE)a;
            }
            CacheClearU();
        }
}

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
    test_volume();
    test_sound();
}

/* ------------------------------------------------------- linklistlib */

/* A Dim List array of five items, built here as arrayslib's newlist
   does: the 32 bytes of the header (current, the used and the free
   MinList, "lIsT") in front of the array base, the items (a node and a
   long) behind it.  Each command is called on every list state of used
   items 0, 1 and 3 (in the order 0, 1, 2), no current item or each one
   as current, and 2 or no free items (3, 4); the state afterwards is
   written as text: the current item, then the next/prev link of every
   node ("U"/"u" the head and tail node of the used list, "F"/"f" those of
   the free list, a digit an item, "-" a null link, "?" a link that is
   none of them: an unlinked item's), so no address is written.  Then a
   traversal with NextItem and with PrevItem from no current item, and
   KillItem of the current item until the list is empty. */
struct listarr {
    ULONG current;
    ULONG uhead, utail, utailpred;
    ULONG fhead, ftail, ftailpred;
    ULONG id;
    ULONG item[5][3];                   /* next, prev, the element */
};
static struct listarr larr;

#define LBASE ((ULONG)&larr.item[0][0])

static char lsym(ULONG p)
{
    int i;

    if (!p)
        return '-';
    if (p == (ULONG)&larr.uhead)
        return 'U';
    if (p == (ULONG)&larr.utail)
        return 'u';
    if (p == (ULONG)&larr.fhead)
        return 'F';
    if (p == (ULONG)&larr.ftail)
        return 'f';
    for (i = 0; i < 5; i++)
        if (p == (ULONG)&larr.item[i][0])
            return (char)('0' + i);
    return '?';
}

/* link the nodes `seq` (n of them; -1 = the head and tail nodes) into the
   list whose head node is at `head` */
static void llink(ULONG *head, const int *seq, int n)
{
    ULONG prev = (ULONG)head, node;
    int i;

    head[1] = 0;
    for (i = 0; i < n; i++) {
        node = (ULONG)&larr.item[seq[i]][0];
        *(ULONG *)prev = node;
        larr.item[seq[i]][1] = prev;
        prev = node;
    }
    *(ULONG *)prev = (ULONG)&head[1];
    head[2] = prev;
}

static void lset(int nused, int nfree, int cur)
{
    static const int used[] = { 0, 1, 2 }, fr[] = { 3, 4 };
    int i;

    memset(&larr, 0xee, sizeof larr);
    for (i = 0; i < 5; i++)
        larr.item[i][2] = 0x11111111UL * (ULONG)(i + 1);
    llink(&larr.uhead, used, nused);
    llink(&larr.fhead, fr, nfree);
    larr.current = cur < 0 ? 0 : (ULONG)&larr.item[cur][0];
    larr.id = 0x6c497354UL;                    /* "lIsT" */
}

static void lstate(void)
{
    char s[64], *p = s;
    int i;

    *p++ = lsym(larr.current);
    *p++ = ' ';
    *p++ = lsym(larr.uhead);
    *p++ = lsym(larr.utail);
    *p++ = lsym(larr.utailpred);
    *p++ = ' ';
    *p++ = lsym(larr.fhead);
    *p++ = lsym(larr.ftail);
    *p++ = lsym(larr.ftailpred);
    for (i = 0; i < 5; i++) {
        *p++ = ' ';
        *p++ = lsym(larr.item[i][0]);
        *p++ = lsym(larr.item[i][1]);
    }
    *p = 0;
    OBSSTR("list", s);
    OBS("id", larr.id);
    for (i = 0; i < 5; i++)
        if (larr.item[i][2] != 0x11111111UL * (ULONG)(i + 1))
            OBS("element changed", i);
}

static void test_lists(void)
{
    static const struct { const char *name; int fn, needcur; } cmds[] = {
        { "FirstItem", 1, 0 }, { "LastItem", 1, 0 }, { "NextItem", 1, 0 },
        { "PrevItem", 1, 0 }, { "AddItem", 1, 0 }, { "AddFirst", 1, 0 },
        { "AddLast", 1, 0 }, { "KillItem", 0, 1 }, { "ClearList", 0, 0 },
        { "ResetList", 0, 0 } };
    static const int nuseds[] = { 0, 1, 3 }, nfrees[] = { 2, 0 };
    unsigned c, u, f;
    int cur;

    for (c = 0; c < sizeof cmds / sizeof cmds[0]; c++) {
        UBYTE *p = N("linklistlib", cmds[c].name, 0);

        ROUT = cmds[c].name;
        for (u = 0; u < 3; u++)
            for (f = 0; f < 2; f++)
                for (cur = cmds[c].needcur ? 0 : -1; cur < nuseds[u]; cur++) {
                    sprintf(VEC, "used %d free %d current %d", nuseds[u], nfrees[f], cur);
                    lset(nuseds[u], nfrees[f], cur);
                    DO1(p, R.d[0] = LBASE);
                    if (cmds[c].fn)
                        OBS("d0", R.d[0]);
                    lstate();
                }
    }
    /* a whole traversal with NextItem and with PrevItem from ResetList,
       and KillItem of the current item until the list is empty */
    {
        static const char *walk[] = { "NextItem", "PrevItem" };
        int i, k;

        for (k = 0; k < 2; k++) {
            UBYTE *p = N("linklistlib", walk[k], 0);

            ROUT = walk[k];
            lset(3, 2, -1);
            for (i = 0; i < 5; i++) {
                sprintf(VEC, "walk step %d", i);
                DO1(p, R.d[0] = LBASE);
                OBS("d0", R.d[0]);
                lstate();
            }
        }
        ROUT = "KillItem";
        lset(3, 2, 2);
        for (i = 0; i < 3; i++) {
            sprintf(VEC, "kill until empty, step %d", i);
            DO1(N("linklistlib", "KillItem", 0), R.d[0] = LBASE);
            lstate();
        }
    }
}

/* ------------------------------------------------------- collisions */

/* ShapesHit, ShapeSpriteHit, SpritesHit and RectsHit: the second object
   at offsets around the first one's edges (each edge at -1, 0 and +1),
   junk in the upper words of the coordinates.  The objects: a shape
   16x12 with the handle (3,-2) and one 9x20 with (1,2); a sprite 16 wide,
   10 high (+4 = height + 1) with (2,1) and one 8x24 with (0,5) */
static const WORD hitoffs[] = { -40, -16, -15, -14, -12, -11, -10, -5, 0, 3, 12, 13, 14,
                                15, 16, 17, 40 };

static void test_hits(void)
{
    static UBYTE sha[32], shb[32], spa[16], spb[16];
    static UWORD rstk[3];
    static const struct { const char *name; UBYTE *a, *b; } kinds[] = {
        { "ShapesHit", sha, shb }, { "ShapeSpriteHit", sha, spb },
        { "SpritesHit", spa, spb } };
    unsigned i, j, k;

    memset(sha, 0xa5, sizeof sha);
    memset(shb, 0x5a, sizeof shb);
    memset(spa, 0xa5, sizeof spa);
    memset(spb, 0x5a, sizeof spb);
    *(UWORD *)(sha + 0) = 16; *(UWORD *)(sha + 2) = 12;     /* width, height */
    *(WORD *)(sha + 10) = 3; *(WORD *)(sha + 12) = -2;      /* handle */
    *(UWORD *)(shb + 0) = 9; *(UWORD *)(shb + 2) = 20;
    *(WORD *)(shb + 10) = 1; *(WORD *)(shb + 12) = 2;
    *(UWORD *)(spa + 4) = 11; spa[9] = 16;                  /* height + 1, width */
    *(UWORD *)(spa + 12) = 2; *(UWORD *)(spa + 14) = 1;     /* handle */
    *(UWORD *)(spb + 4) = 25; spb[9] = 8;
    *(UWORD *)(spb + 12) = 0; *(UWORD *)(spb + 14) = 5;
    for (k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
        ROUT = kinds[k].name;
        for (i = 0; i < sizeof hitoffs / sizeof hitoffs[0]; i++)
            for (j = 0; j < sizeof hitoffs / sizeof hitoffs[0]; j++) {
                ULONG up = (i + j) & 1 ? 0x5a5a0000UL : 0;
                sprintf(VEC, "dx %d dy %d up $%lX", hitoffs[i], hitoffs[j], up);
                DO1(N("collslib", kinds[k].name, 0),
                    R.d[1] = up | 100; R.d[2] = up | 50;
                    R.d[4] = up | (UWORD)(100 + hitoffs[i]);
                    R.d[5] = up | (UWORD)(50 + hitoffs[j]);
                    R.a[0] = (ULONG)kinds[k].a; R.a[1] = (ULONG)kinds[k].b);
                OBS("d0", R.d[0]);
            }
        /* signed word compares at the edge of the word range */
        sprintf(VEC, "x $7FF8 against $8004");
        DO1(N("collslib", kinds[k].name, 0),
            R.d[1] = 0x7ff8; R.d[2] = 50; R.d[4] = 0x8004; R.d[5] = 50;
            R.a[0] = (ULONG)kinds[k].a; R.a[1] = (ULONG)kinds[k].b);
        OBS("d0", R.d[0]);
    }
    /* RectsHit: 16x12 at (100,50) against 9x20; Width2/Height2 on the a2
       stack (-2: width, -4: height) */
    ROUT = "RectsHit";
    for (i = 0; i < sizeof hitoffs / sizeof hitoffs[0]; i++)
        for (j = 0; j < sizeof hitoffs / sizeof hitoffs[0]; j++) {
            ULONG up = (i + j) & 1 ? 0xa5a50000UL : 0;
            sprintf(VEC, "dx %d dy %d up $%lX", hitoffs[i], hitoffs[j], up);
            rstk[0] = 20;                                   /* height2 */
            rstk[1] = 9;                                    /* width2 */
            DO1(N("collslib", "RectsHit", 0),
                R.d[0] = up | 100; R.d[1] = up | 50; R.d[2] = up | 16; R.d[3] = up | 12;
                R.d[4] = up | (UWORD)(100 + hitoffs[i]);
                R.d[5] = up | (UWORD)(50 + hitoffs[j]);
                R.a[2] = (ULONG)&rstk[2]);
            OBS("d0", R.d[0]);
        }
    sprintf(VEC, "negative sizes");
    rstk[0] = (UWORD)-3;
    rstk[1] = (UWORD)-4;
    DO1(N("collslib", "RectsHit", 0),
        R.d[0] = 10; R.d[1] = 10; R.d[2] = (UWORD)-2; R.d[3] = (UWORD)-2;
        R.d[4] = 12; R.d[5] = 12; R.a[2] = (ULONG)&rstk[2]);
    OBS("d0", R.d[0]);
}

/* ------------------------------------------------------------ Frac */

/* Frac takes any numeric type (the type word at -2(a2)); the original
   took a Float, the compiler converted the argument first: a Quick with
   floatquicklib's qutofl ($D300), a byte, word or long with SPFlt (sign
   extended).  For the original set the case makes that conversion, then
   calls Frac; for the C build it calls Frac with the value and its type.
   Floats: specials and pseudo-random values of both signs with exponents
   from 2^-9 to 2^31 (mathffp of the ROM computes, the same in both sets).
   Quick: integer parts below 256 (exact in both), and from 256 on, where
   qutofl's 24-bit mantissa rounds low fraction bits away: the pairs
   [orig] / [fixed #1064] (the port gives the exact fraction).  The
   original's qutofl made the Quick 0 the FFP "-0" $00000080 (#146, fixed
   in the port's qutofl): the pair [orig] / [fixed #146]. */
static UWORD fracframe[2];

static void frac_case(UWORD type, ULONG v)
{
    ULONG d0 = v;

    if (strcmp(loaded.what, "C") != 0 && type != 5) {
        struct regs c;
        ULONG popped;

        markregs(&c);
        if (type == 4) {
            c.d[0] = v;
            c.a[6] = ffpbase;
            callstk(find_token(&loaded, 0xd300, 0), &c, NULL, 0, &popped);
        } else {
            c.d[0] = type == 1 ? (ULONG)(LONG)(BYTE)v : type == 2 ? (ULONG)(LONG)(WORD)v : v;
            c.a[6] = ffpbase;
            callstk((APTR)(ffpbase - 36), &c, NULL, 0, &popped);        /* SPFlt */
        }
        d0 = c.d[0];
    }
    if (type == 5)
        sprintf(VEC, "ffp $%08lX", v);
    else
        sprintf(VEC, "type %u value $%08lX", type, v);
    fracframe[0] = type;
    DO1(N("vallib", "Frac", 0), R.d[0] = d0; R.a[2] = (ULONG)&fracframe[1];
        R.a[6] = ffpbase);
    OBS("d0", R.d[0]);
}

static void test_frac(void)
{
    static const ULONG specials[] = {
        0, 0x80000041UL, 0x800000c1UL, 0xc0000041UL, 0xc00000c1UL, 0x8000003fUL,
        0x800000bfUL, 0xb0000042UL, 0xb00000c2UL, 0xffffff60UL, 0xffffffe0UL,
        0xffffff7fUL, 0x80000001UL, 0xaaaaaa58UL, 0xaaaaaad8UL };
    static const ULONG quicks[] = {
        0, 0x00010000UL, 0xffff0000UL, 0x00008000UL, 0xffff8000UL, 0x00000001UL,
        0xffffffffUL, 0x0000ffffUL, 0xffff0001UL, 0x000c8000UL, 0xfff38000UL,
        0x00ff0001UL, 0xff00ffffUL, 0x0100c000UL, 0x01000001UL, 0xfeffffffUL,
        0x7fffffffUL, 0x80000000UL, 0x80000001UL, 0x7530000aUL, 0x12345678UL,
        0xedcba988UL };
    static const struct { UWORD type; ULONG v; } ints[] = {
        { 1, 0x123456f0 }, { 1, 0x12345670 }, { 1, 0 }, { 2, 0x1234fff0 },
        { 2, 0x12340010 }, { 2, 0 }, { 3, 0xfffffff0 }, { 3, 0x00012345 },
        { 3, 0 }, { 3, 0x7fffffff }, { 3, 0x80000000 } };
    unsigned i;

    ROUT = "Frac";
    for (i = 0; i < sizeof specials / sizeof specials[0]; i++)
        frac_case(5, specials[i]);
    seed = 0x13579bdf;
    for (i = 0; i < 64; i++) {
        ULONG m = rnd() | 0x80000000UL;
        ULONG f = (m & 0xffffff00UL) | (0x37 + i % 0x29) | (i & 1 ? 0x80 : 0);
        frac_case(5, f);
    }
    for (i = 0; i < sizeof quicks / sizeof quicks[0]; i++)
        frac_case(4, quicks[i]);
    for (i = 0; i < 32; i++) {          /* integer parts of 0 .. 2^15 */
        ULONG q = rnd() >> (i % 16 + 1);
        frac_case(4, i & 1 ? (ULONG)-(LONG)q : q);
    }
    for (i = 0; i < sizeof ints / sizeof ints[0]; i++)
        frac_case(ints[i].type, ints[i].v);
}

/* ------------------------------------------------------------------ Val */

/* Val(String$): d0 = the string, its length at -4(a2), a6 = mathffp.
   Every path of doval2: leading spaces, decimal with and without a
   fraction, "$" hex, "%" binary, "-", the end of the count in each loop,
   characters that end a number, integer overflow (mod 2^32).  The byte
   behind the string is set (`trail`): hex, binary and negative numbers
   read it (#302, open), the others must not.  Two fixed bugs are pairs
   [orig] / [fixed #N]: more than 9 fraction digits (#304) and a length
   above 32768 (#303). */
static ULONG valframe[1];
static UBYTE valbuf[40002];

static void val_case(const char *desc, const char *s, ULONG len, UBYTE trail)
{
    if (s)
        memcpy(valbuf, s, len);
    valbuf[len] = trail;
    sprintf(VEC, "%s trail $%02X", desc, trail);
    valframe[0] = len;
    DO1(N("vallib", "Val", 0), R.d[0] = (ULONG)valbuf; R.a[2] = (ULONG)&valframe[1];
        R.a[6] = ffpbase);
    OBS("d0", R.d[0]);
    OBS("a3 - string", R.a[3] - (ULONG)valbuf);
}

static void test_val(void)
{
    static const char *strs[] = {
        "", " ", "   ", "0", "7", "12345", "  42", " 9 ", "4294967295", "4294967296",
        "99999999999", "2147483648", "12a", "1e5", "+5", "1.5", "3.14159", ".5", "12.",
        ".", "0.000001", "1.123456789", "123.456.7", "1. 5", "-12", "-.25", "- 3",
        "--1", "-", "-0", "$1aF", "$FFFFFFFF", "$123456789", "$", "$g", "$G1", "$@",
        "$/", "$fF0", "%1011", "%", "%2", "%10102", "%11111111111111111111111111111111",
        "-$10", "-%1", " $A", " %1", "\t5", "5\t", "0123", "00.0100" };
    static const UBYTE trails[] = { 0, '9', '.' };
    unsigned i, j;
    char d[64];

    ROUT = "Val";
    for (i = 0; i < sizeof strs / sizeof strs[0]; i++)
        for (j = 0; j < sizeof trails / sizeof trails[0]; j++) {
            sprintf(d, "\"%s\"", strs[i]);
            val_case(d, strs[i], strlen(strs[i]), trails[j]);
        }
    /* the count ends inside a number: the first characters of a longer
       string (the rest is the byte behind and must not count) */
    val_case("\"12345\" length 3", "12345", 3, '4');
    val_case("\"1.25\" length 3", "1.25", 3, '5');
    val_case("\"$12\" length 2", "$12", 2, '2');
    val_case("\"%11\" length 2", "%11", 2, '1');
    val_case("\"-12\" length 2", "-12", 2, '2');
    /* #304: 10 and 13 fraction digits */
    val_case("\"0.1234567891\"", "0.1234567891", 12, 0);
    val_case("\"2.0000000001234\"", "2.0000000001234", 15, 0);
    /* #303: 40000 characters (the original counts the low word, $9C40,
       which is negative: 0) */
    memset(valbuf, '7', 40000);
    val_case("40000 x \"7\"", NULL, 40000, 0);
    memset(valbuf, ' ', 39999);
    valbuf[39999] = '3';
    val_case("39999 spaces and \"3\"", NULL, 40000, 0);
}

/* ------------------------------------------- BlitzKey, Inkey$ */

/* keyboard.device as blitzkeyslib reads it, a copy in the test's memory:
   the read index at +$70, the write index at +$72, 32 events {code.w,
   qualifier.w} from +$74; the init takes its address (a0) */
static UWORD kbd[0x140 / 2];

static void kbd_put(UWORD rd, UWORD wr, const UWORD *ev, unsigned n)
{
    unsigned i;
    memset(kbd, 0, sizeof kbd);
    kbd[0x70 / 2] = rd;
    kbd[0x72 / 2] = wr;
    for (i = 0; i < n && i < 4; i++) {        /* behind the read index */
        UWORD at = (rd + 4 + 4 * i) & 127;
        kbd[(0x74 + at) / 2] = ev[2 * i];
        kbd[(0x76 + at) / 2] = ev[2 * i + 1];
    }
}

static void test_keys(void)
{
    /* n events after the read index (the first one 4 behind it), the write
       index 4 behind the last one: BlitzKey stops where its next position
       is the write index; more than 4 events: the rest are zeros */
    static const struct { const char *vec; UWORD rd; UWORD n; UWORD ev[8]; } bufs[] = {
        { "empty", 0x10, 0, { 0 } },
        { "a", 0x10, 1, { 0x20, 0 } },
        { "shift a", 0x20, 1, { 0x20, 1 } },
        { "caps a", 0x30, 1, { 0x20, 4 } },
        { "ctrl a", 0x40, 1, { 0x20, 8 } },
        { "key up then b", 0x50, 2, { 0xa0, 0, 0x35, 0 } },
        { "two key ups", 0x60, 2, { 0xa0, 0, 0xb5, 0 } },
        { "$7D", 0x70, 1, { 0x7d, 0 } },
        { "wrap at 127: 1", 0x7c, 1, { 0x01, 0 } },
        { "return", 0x08, 1, { 0x44, 0 } },
        { "f1 (no character)", 0x0c, 1, { 0x50, 0 } },
        { "read = write: 31 events", 0x18, 31, { 0x21, 0 } },
    };
    static UBYTE sbuf[8];
    unsigned i;

    /* the tables of qconvertkey (console.device, the default keymap) */
    ROUT = "rawkeylib init";
    strcpy(VEC, "");
    DO1(cmd_entry(getlib(&loaded, "rawkeylib"), 3), (void)0);
    ROUT = "blitzkeyslib init";
    strcpy(VEC, "keyboard data");
    DO1(cmd_entry(getlib(&loaded, "blitzkeyslib"), 6), R.a[0] = (ULONG)kbd);
    for (i = 0; i < sizeof bufs / sizeof bufs[0]; i++) {
        kbd_put(bufs[i].rd, (bufs[i].rd + 4 * (bufs[i].n + 1)) & 127, bufs[i].ev, bufs[i].n);
        ROUT = "BlitzKey";
        strcpy(VEC, bufs[i].vec);
        DO1(N("blitzkeyslib", "BlitzKey", 0), R.d[0] = 0x5a5a0000UL; R.d[1] = 0xa5a50000UL);
        OBS("d0", R.d[0]);
        OBS("read index", kbd[0x70 / 2]);
        ROUT = "BlitzQualifier";
        DO1(N("blitzkeyslib", "BlitzQualifier", 0), (void)0);
        OBS("d0.w", R.d[0] & 0xffff);
    }
    /* Inkey$ through the input handler of BlitzKeys On (inputoutputlib:
       Inkey$ -> dogetsingle -> BlitzKey) */
    ROUT = "BlitzKeys";
    strcpy(VEC, "On");
    DO1(N("blitzkeyslib", "BlitzKeys", 0), R.d[0] = 1);
    for (i = 0; i < sizeof bufs / sizeof bufs[0]; i++) {
        kbd_put(bufs[i].rd, (bufs[i].rd + 4 * (bufs[i].n + 1)) & 127, bufs[i].ev, bufs[i].n);
        memset(sbuf, 0xee, sizeof sbuf);
        ROUT = "Inkey$";
        strcpy(VEC, bufs[i].vec);
        DO1(N("editlib", "Inkey$", 0), R.a[3] = (ULONG)sbuf);
        OBS("d0", R.d[0]);
        OBS("a3 - string", R.a[3] - (ULONG)sbuf);
        OBSBYTES("string", sbuf, sizeof sbuf);
        OBS("read index", kbd[0x70 / 2]);
    }
    ROUT = "rawkeylib finit";
    strcpy(VEC, "");
    DO1(cmd_entry(getlib(&loaded, "rawkeylib"), 4), (void)0);
}

/* -------------------------------------------------- Print on a bitmap */

/* Print and the output handler of BitMapOutput (fontlib bitout, bitout2):
   strings and numbers on a bitmap of 3 planes, 5 bytes x 20 rows, with
   topaz 8 of the ROM; every colour pair, a new line (10), the right edge
   (a new line), the bottom (nothing drawn), characters outside the
   font */
static UBYTE pplanes[3][100];
static ULONG pbm[16];                   /* struct bb_bitmap */

/* a long on the a2 stack of words */
#define STKL(i, v) (stk[i] = (UWORD)((ULONG)(v) >> 16), stk[(i) + 1] = (UWORD)(ULONG)(v))

static void test_print(void)
{
    static const struct TextAttr topaz = { (STRPTR)"topaz.font", 8, 0, 0 };
    static ULONG fontobj;
    static const char *strs[] = { "SCORE", "Ab", "a\nb", "123456789", "\x7f\x1f\x80 ~",
                                  "x" };
    static const ULONG longs[] = { 0, 1, 123456, (ULONG)-7, 0x7fffffffUL, 0x80000000UL };
    struct TextFont *font = NULL;
    UBYTE *b = (UBYTE *)pbm;
    UWORD stk[8];
    unsigned i, fg, bg;

    gfxb = OpenLibrary((STRPTR)"graphics.library", 0);
    if (gfxb)
        font = OpenFont((struct TextAttr *)&topaz);
    if (!font) {
        check("topaz 8", FALSE);
        return;
    }
    fontobj = (ULONG)font;
    ROUT = "BlitzFont _use";
    strcpy(VEC, "topaz 8");
    DO1(cmd_entry(getlib(&loaded, "fontlib"), 10), R.a[3] = (ULONG)&fontobj);
    memset(pbm, 0, sizeof pbm);
    *(UWORD *)(b + 0) = 5;                      /* bpr */
    *(UWORD *)(b + 2) = 20;                     /* rows */
    *(UWORD *)(b + 4) = 3;                      /* depth */
    for (i = 0; i < 3; i++)
        *(UBYTE **)(b + 8 + 4 * i) = pplanes[i];
    ROUT = "BitMapOutput";
    strcpy(VEC, "5 x 20");
    DO1(N("fontlib", "BitMapOutput", 0), R.a[0] = (ULONG)pbm);
    for (i = 0; i < sizeof strs / sizeof strs[0]; i++)
        for (fg = 0; fg < 8; fg += i ? 7 : 1)
            for (bg = 0; bg < 8; bg += i ? 7 : 3) {
                ULONG len = strlen(strs[i]);
                memset(pplanes, 0x3c, sizeof pplanes);
                ROUT = "Colour";
                sprintf(VEC, "fg %u bg %u", fg, bg);
                DO1(N("fontlib", "Colour", 1), R.d[0] = fg; R.d[1] = bg);
                ROUT = "Locate";
                DO1(N("fontlib", "Locate", 0), R.d[0] = 1; R.d[1] = 0x8000);
                ROUT = "Print";
                sprintf(VEC, "string %u fg %u bg %u", i, fg, bg);
                /* the a2 stack: address, length, type 7 */
                STKL(0, (ULONG)strs[i]);
                STKL(2, len);
                stk[4] = 7;
                DO1(N("printlib", "Print", 0), R.d[7] = 1; R.a[2] = (ULONG)&stk[5];
                    R.a[6] = ffpbase);
                OBSBYTES("plane 0", pplanes[0], 100);
                OBSBYTES("plane 1", pplanes[1], 100);
                OBSBYTES("plane 2", pplanes[2], 100);
                OBSBYTES("cursor", b + 40, 4);
            }
    /* the bottom: nothing drawn, the cursor moves */
    memset(pplanes, 0x3c, sizeof pplanes);
    ROUT = "Locate";
    strcpy(VEC, "bottom");
    DO1(N("fontlib", "Locate", 0), R.d[0] = 3; R.d[1] = 0x20000);
    ROUT = "Print";
    STKL(0, (ULONG)"ABCD");
    STKL(2, 4);
    stk[4] = 7;
    DO1(N("printlib", "Print", 0), R.d[7] = 1; R.a[2] = (ULONG)&stk[5]; R.a[6] = ffpbase);
    OBSBYTES("plane 0", pplanes[0], 100);
    OBSBYTES("cursor", b + 40, 4);
    /* numbers (Long), two arguments in one Print */
    ROUT = "Colour";
    strcpy(VEC, "fg 5 bg 2");
    DO1(N("fontlib", "Colour", 1), R.d[0] = 5; R.d[1] = 2);
    for (i = 0; i < sizeof longs / sizeof longs[0]; i++) {
        memset(pplanes, 0x3c, sizeof pplanes);
        ROUT = "Locate";
        sprintf(VEC, "long $%lX", longs[i]);
        DO1(N("fontlib", "Locate", 0), R.d[0] = 0; R.d[1] = 0);
        ROUT = "Print";
        /* two arguments: the long, then the word 12 */
        STKL(0, longs[i]);
        stk[2] = 3;
        stk[3] = 12;
        stk[4] = 2;
        DO1(N("printlib", "Print", 0), R.d[7] = 2; R.a[2] = (ULONG)&stk[5]; R.a[6] = ffpbase);
        OBSBYTES("plane 0", pplanes[0], 100);
        OBSBYTES("plane 1", pplanes[1], 100);
        OBSBYTES("plane 2", pplanes[2], 100);
        OBSBYTES("cursor", b + 40, 4);
    }
    /* the other types, each followed by the word 12: a byte and a word
       (popped as words, the upper byte of the byte set), a quick, a float,
       and type 6 (printed as a long) */
    {
        static const struct { UWORD type; ULONG v; } others[] = {
            { 1, 0x5afd }, { 2, 0xfff6 }, { 4, 0x00018000 }, { 5, 0xa0000042 }, { 6, 77 } };
        for (i = 0; i < sizeof others / sizeof others[0]; i++) {
            memset(pplanes, 0x3c, sizeof pplanes);
            ROUT = "Locate";
            sprintf(VEC, "type %u value $%lX", others[i].type, others[i].v);
            DO1(N("fontlib", "Locate", 0), R.d[0] = 0; R.d[1] = 0);
            ROUT = "Print";
            if (others[i].type <= 2) {
                stk[0] = 0x5a5a;
                stk[1] = (UWORD)others[i].v;
            } else
                STKL(0, others[i].v);
            stk[2] = others[i].type;
            stk[3] = 12;
            stk[4] = 2;
            DO1(N("printlib", "Print", 0), R.d[7] = 2; R.a[2] = (ULONG)&stk[5];
                R.a[6] = ffpbase);
            OBSBYTES("plane 0", pplanes[0], 100);
            OBSBYTES("plane 1", pplanes[1], 100);
            OBSBYTES("plane 2", pplanes[2], 100);
            OBSBYTES("cursor", b + 40, 4);
        }
    }
    /* one character through the handler's entry 0 (dooutsingle) */
    memset(pplanes, 0x3c, sizeof pplanes);
    ROUT = "Locate";
    strcpy(VEC, "character Q");
    DO1(N("fontlib", "Locate", 0), R.d[0] = 4; R.d[1] = 0x10000);
    ROUT = "dooutsingle";
    DO1(cmd_entry(getlib(&loaded, "inputoutputlib"), 6), R.d[0] = 'Q');
    OBSBYTES("plane 0", pplanes[0], 100);
    OBSBYTES("plane 2", pplanes[2], 100);
    OBSBYTES("cursor", b + 40, 4);
    CloseFont(font);
    CloseLibrary(gfxb);
}

/* --------------------------------------------------------- Str$, UStr$ */

/* Str$(Expression) and UStr$: d0 = the value, its type at -2(a2); the
   characters go to a3 (the buffer, filled with $5A first), d0 = their
   number.  Every numeric type, without and with a Format string (the
   formats and values avoid the Format bugs fixed in the port, #235-#238,
   which have their bug-orig tests). */
static UBYTE sbuf2[64];
static UWORD strframe[1];
static const char *strformat = "";             /* the Format string set */

static void str_case(const char *rout, UWORD type, ULONG v)
{
    ROUT = rout;
    sprintf(VEC, "Format \"%s\" type %u value $%08lX", strformat, type, v);
    memset(sbuf2, 0x5a, sizeof sbuf2);
    strframe[0] = type;
    DO1(N("printlib", rout, 0), R.d[0] = v; R.a[2] = (ULONG)&strframe[1];
        R.a[3] = (ULONG)sbuf2; R.a[6] = ffpbase);
    OBS("d0", R.d[0]);
    OBS("a3 - buffer", R.a[3] - (ULONG)sbuf2);
    OBSBYTES("buffer", sbuf2, 24);
}

static void format_set(const char *f)
{
    static ULONG fframe[1];

    ROUT = "Format";
    sprintf(VEC, "\"%s\"", f);
    fframe[0] = strlen(f);
    DO1(N("printlib", "Format", 0), R.d[0] = (ULONG)f; R.a[2] = (ULONG)&fframe[1]);
    strformat = f;
}

static void test_strstr(void)
{
    static const struct { UWORD type; ULONG v; } vals[] = {
        { 1, 0 }, { 1, 0x5a5a007f }, { 1, 0x5a5a0080 }, { 1, 0x123456fd },
        { 2, 0 }, { 2, 0x5a5a7fff }, { 2, 0x5a5a8000 }, { 2, 0x1234abcd }, { 2, 0x0000fff6 },
        { 3, 0 }, { 3, 1 }, { 3, 9 }, { 3, 10 }, { 3, 123456 }, { 3, (ULONG)-7 },
        { 3, 999999999 }, { 3, 1000000000 }, { 3, 0x7fffffff }, { 3, 0x80000000 },
        { 3, 0x80000001 }, { 3, 4000000000UL }, { 6, 42 },
        { 4, 0 }, { 4, 0x00018000 }, { 4, 0xffffc000 }, { 4, 0x00010001 }, { 4, 0xffff0000 },
        { 4, 0x00000001 }, { 4, 0x7fffffff }, { 4, 0x80000000 }, { 4, 0x0003c000 },
        { 4, 0x00004000 }, { 4, 0xfffe8000 },
        { 5, 0 }, { 5, 0x80000041 }, { 5, 0xc00000c1 }, { 5, 0xa0000042 }, { 5, 0x98968058 },
        { 5, 0x98968098 }, { 5, 0x83126e38 }, { 5, 0xc90fdb42 }, { 5, 0xf423f854 },
        { 5, 0x8000001f }, { 5, 0xffffff7f } };
    static const char *formats[] = { "####", "0000.00", "+###.#", "##,###" };
    static const struct { UWORD type; ULONG v; } fvals[] = {
        { 3, 12 }, { 3, 4321 }, { 2, 0x00000007 }, { 4, 0x000c8000 }, { 4, 0x00030000 },
        { 5, 0xd0000043 }, { 5, 0xa0000042 } };
    unsigned i, j;

    for (i = 0; i < sizeof vals / sizeof vals[0]; i++)
        str_case("Str$", vals[i].type, vals[i].v);
    for (j = 0; j < sizeof formats / sizeof formats[0]; j++) {
        format_set(formats[j]);
        for (i = 0; i < sizeof fvals / sizeof fvals[0]; i++) {
            str_case("Str$", fvals[i].type, fvals[i].v);
            str_case("UStr$", fvals[i].type, fvals[i].v);
        }
    }
    format_set("");
    str_case("UStr$", 3, 123456);
    str_case("UStr$", 5, 0xa0000042);
}

/* ----------------------------------------------------------------- Instr */

/* Instr(String$,Findstring$[,Startpos]): d0 = String$, d1 = Findstring$,
   d2 = Startpos, the lengths at -4(a2) (String$) and -8(a2); a3 =
   String$ afterwards.  CaseSense On and Off (Off compares "a".."z" as
   upper case only: "`", "{", "@" and "[" stay), matches at the start, in
   the middle, at the end, cut off by the end, the empty strings, and
   Startpos from 1 to behind the string (Startpos <= 0 and near $7FFFFFFF
   are the fixed bugs #287 and #1030, with bug-orig tests). */
static ULONG instrframe[2];

static void instr_case(const char *cs, const char *s, const char *f, LONG start)
{
    if (start)
        sprintf(VEC, "%s \"%s\" \"%s\" from %ld", cs, s, f, start);
    else
        sprintf(VEC, "%s \"%s\" \"%s\"", cs, s, f);
    instrframe[0] = strlen(f);
    instrframe[1] = strlen(s);
    DO1(N("stringfunclib", "Instr", start ? 1 : 0), R.d[0] = (ULONG)s; R.d[1] = (ULONG)f;
        if (start) R.d[2] = start; R.a[2] = (ULONG)&instrframe[2]);
    OBS("d0", R.d[0]);
    OBS("a3 - string", R.a[3] - (ULONG)s);
}

static void test_instr(void)
{
    static const char *hay[] = { "Hello, World of Blitz", "", "aaab", "abababc", "a{z`@[",
                                 "BLITZ blitz" };
    static const char *needles[] = { "Blitz", "blitz", "BLITZ", "", "H", "z", "o", "World",
                                     "WORLD", "of B", "xyz", "Blitzz", "aab", "abc", "bab",
                                     "A{Z", "`@", "@[", "[", "{", "a", "Hello, World of Blitz",
                                     "Hello, World of Blitz!" };
    static const LONG starts[] = { 1, 2, 5, 13, 17, 18, 21, 22, 23, 100 };
    unsigned c, i, j, k;

    for (c = 0; c < 2; c++) {
        ROUT = "CaseSense";
        sprintf(VEC, "%s", c ? "Off" : "On");
        DO1(N("stringfunclib", "CaseSense", 0), R.d[0] = c ? 0 : 0xffff);
        ROUT = "Instr";
        for (i = 0; i < sizeof hay / sizeof hay[0]; i++)
            for (j = 0; j < sizeof needles / sizeof needles[0]; j++) {
                instr_case(c ? "Off" : "On", hay[i], needles[j], 0);
                if (i == 0 || i == 3)
                    for (k = 0; k < sizeof starts / sizeof starts[0]; k++)
                        instr_case(c ? "Off" : "On", hay[i], needles[j], starts[k]);
            }
    }
}

/* ---------------------------------- Left$ Right$ Mid$ Hex$ Bin$ Cvi Cvl */

/* The string functions that cut or move a string in place: d0 = String$
   (in the string work area), d1 (and d2) the numbers, the length at
   -4(a2), a3 one byte past the argument's 0 on entry; the result starts
   at String$, d0 = its length, a3 = its end.  The string lies at an even
   and at an odd address; its lengths are 0, 1, 5, 8, 21 and 70 (the long
   copies of Mid$/Right$), the numbers the boundaries of each function.
   Not here: Right$ with Len-Length >= 32768 (#285) and Mid$ with
   start-1+length above $7FFFFFFF (#286), the fixed bugs with bug-orig
   tests. */
static ULONG strbuf[40];                /* 160 bytes, long aligned */
static ULONG cutframe[1];

static const char strtext[] =
    "Hello, World of Blitz! 0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMN";

/* a fresh string of `len` characters at strbuf + odd; returns it */
static UBYTE *strset(ULONG len, int odd)
{
    UBYTE *b = (UBYTE *)strbuf, *s = b + odd;
    ULONG i;

    for (i = 0; i < sizeof strbuf; i++)
        b[i] = (UBYTE)(0xA0 + i);
    memcpy(s, strtext, len);
    s[len] = 0;
    cutframe[0] = len;
    return s;
}

/* the result: d0, a3 and the bytes of the argument (with its 0 and one
   byte behind it) */
static void strobs(const UBYTE *s, ULONG len)
{
    OBS("d0", R.d[0]);
    OBS("a3 - string", R.a[3] - (ULONG)s);
    OBSBYTES("string", s, len + 2);
}

static void strcut_case(const char *name, int form, ULONG len, int odd, LONG n1, LONG n2)
{
    UBYTE *s = strset(len, odd);

    ROUT = name;
    if (form == 0 && !strcmp(name, "Mid$"))
        sprintf(VEC, "len %lu %s %ld,%ld", len, odd ? "odd" : "even", n1, n2);
    else
        sprintf(VEC, "len %lu %s %ld", len, odd ? "odd" : "even", n1);
    DO1(N("stringfunclib", name, form), R.d[0] = (ULONG)s; R.d[1] = (ULONG)n1;
        if (form == 0 && !strcmp(name, "Mid$")) R.d[2] = (ULONG)n2;
        R.a[2] = (ULONG)&cutframe[1]; R.a[3] = (ULONG)s + len + 1);
    strobs(s, len);
}

/* add v to the list l of *n values unless it is there */
static void addval(LONG *l, unsigned *n, LONG v)
{
    unsigned i;

    for (i = 0; i < *n; i++)
        if (l[i] == v)
            return;
    l[(*n)++] = v;
}

static void test_strcut(void)
{
    static const ULONG lens[] = { 0, 1, 5, 8, 21, 70 };
    static const LONG fixed[] = { -1, 0, 1, 2, 7, 8, 9, 40 };
    LONG counts[16], starts[16], mlens[16];
    unsigned nc, ns, nm, i, j, k;
    int odd;

    for (odd = 0; odd < 2; odd++)
        for (i = 0; i < sizeof lens / sizeof lens[0]; i++) {
            LONG len = (LONG)lens[i];

            /* Left$, Right$: the fixed counts, Len-1 to Len+1 and the
               extremes; UnLeft$, UnRight$ (Len-n): no n that overflows */
            nc = 0;
            for (j = 0; j < sizeof fixed / sizeof fixed[0]; j++)
                addval(counts, &nc, fixed[j]);
            addval(counts, &nc, len - 1);
            addval(counts, &nc, len);
            addval(counts, &nc, len + 1);
            for (j = 0; j < nc; j++) {
                strcut_case("Left$", 0, lens[i], odd, counts[j], 0);
                strcut_case("Right$", 0, lens[i], odd, counts[j], 0);
                strcut_case("UnLeft$", 0, lens[i], odd, counts[j], 0);
                strcut_case("UnRight$", 0, lens[i], odd, counts[j], 0);
            }
            strcut_case("Left$", 0, lens[i], odd, 0x7fffffffL, 0);
            strcut_case("Left$", 0, lens[i], odd, (LONG)0x80000000UL, 0);
            strcut_case("Right$", 0, lens[i], odd, 0x7fffffffL, 0);
            strcut_case("Right$", 0, lens[i], odd, (LONG)0x80000000UL, 0);

            /* Mid$: the start before, at, inside and behind the string;
               Mid$(s,start) computes Len-start+1: no start that
               overflows */
            ns = 0;
            addval(starts, &ns, -1);
            addval(starts, &ns, 0);
            addval(starts, &ns, 1);
            addval(starts, &ns, 2);
            addval(starts, &ns, 3);
            addval(starts, &ns, len - 1);
            addval(starts, &ns, len);
            addval(starts, &ns, len + 1);
            addval(starts, &ns, len + 2);
            addval(starts, &ns, 0x7fffffffL);
            for (j = 0; j < ns; j++) {
                LONG st = starts[j], st0 = st > 0 ? st - 1 : 0;

                strcut_case("Mid$", 1, lens[i], odd, st, 0);
                /* the lengths: the fixed ones, the rest of the string
                   and one more; not (#286) start-1+length above
                   $7FFFFFFF with the start inside the string */
                nm = 0;
                for (k = 0; k < sizeof fixed / sizeof fixed[0]; k++)
                    addval(mlens, &nm, fixed[k]);
                addval(mlens, &nm, len - st0);
                addval(mlens, &nm, len - st0 + 1);
                if (st0 == 0 || st0 > len)
                    addval(mlens, &nm, 0x7fffffffL);
                for (k = 0; k < nm; k++)
                    strcut_case("Mid$", 0, lens[i], odd, st, mlens[k]);
            }
            strcut_case("Mid$", 0, lens[i], odd, (LONG)0x80000000UL, 5);
        }
}

/* Hex$(Expression), Bin$(Expression): d0 = the value, written at a3 (an
   even and an odd address); d0 = 8 or 32, a3 behind the digits */
static void test_hexbin(void)
{
    static const ULONG vals[] = { 0, 1, 9, 10, 15, 16, 0x0001e240UL, 0x12345678UL,
                                  0x9abcdef0UL, 0xdeadbeefUL, 0x7fffffffUL, 0x80000000UL,
                                  0xa5a5a5a5UL, 0x5a5a5a5aUL, 0x0f0f0f0fUL, 0xffffffffUL };
    static const char *fn[] = { "Hex$", "Bin$" };
    unsigned i, j, n;
    int odd;
    UBYTE *b = (UBYTE *)strbuf;

    for (n = 0; n < 2; n++)
        for (odd = 0; odd < 2; odd++)
            for (i = 0; i < sizeof vals / sizeof vals[0] + 4; i++) {
                ULONG v = i < sizeof vals / sizeof vals[0] ? vals[i] : rnd();
                for (j = 0; j < 40; j++)
                    b[j] = (UBYTE)(0xA0 + j);
                ROUT = fn[n];
                sprintf(VEC, "$%08lx %s", v, odd ? "odd" : "even");
                DO1(N("stringfunclib", fn[n], 0), R.d[0] = v; R.a[3] = (ULONG)b + odd);
                OBS("d0", R.d[0]);
                OBS("a3 - start", R.a[3] - ((ULONG)b + odd));
                OBSBYTES("digits", b, 36);
            }
}

/* Cvi(String$), Cvl(String$), Cvq(String$): d0 = the string, its length
   at -4(a2); the first 2 or 4 bytes big endian, a3 = String$.  Cvi keeps
   the upper word of d0 (the string address's: observed as a flag).  The
   strings shorter than the 2 or 4 bytes have zeros behind them, so the
   original's read past the end (#288) gives the same as the fix. */
static void test_cv(void)
{
    static const char *fn[] = { "Cvi", "Cvl", "Cvq" };
    static const UBYTE bytes[] = { 0x81, 0x7f, 0x00, 0xff, 0x12, 0xfe, 0x01, 0x80 };
    UBYTE *b = (UBYTE *)strbuf;
    unsigned n, len, j;
    int odd;

    for (n = 0; n < 3; n++)
        for (odd = 0; odd < 2; odd++)
            for (len = 0; len <= 6; len++)
                for (j = 0; j < 3; j++) {
                    UBYTE *s = b + 2 + odd;
                    unsigned k;
                    memset(b, 0, 16);
                    for (k = 0; k < len; k++)
                        s[k] = bytes[(k + j * 3) & 7];
                    cutframe[0] = len;
                    ROUT = fn[n];
                    sprintf(VEC, "len %u %s bytes %u", len, odd ? "odd" : "even", j);
                    DO1(N("stringfunclib", fn[n], 0), R.d[0] = (ULONG)s;
                        R.a[2] = (ULONG)&cutframe[1]; R.a[3] = (ULONG)s + len + 1);
                    if (n == 0) {
                        OBS("d0.w", R.d[0] & 0xffff);
                        OBS("d0 upper word = the string's", (R.d[0] >> 16) == ((ULONG)s >> 16));
                    } else
                        OBS("d0", R.d[0]);
                    OBS("a3 - string", R.a[3] - (ULONG)s);
                }
}

/* ------------------------------------------------------------------ main */

static const char *names[] = {
    "memlib", "strcomplib", "memacclib", "sortlib", "mathtranslib", "utillib",
    "environlib", "maxslib", "inputoutputlib", "rawkeylib", "windowslib", "blitzkeyslib",
    "floatquicklib", "vallib", "iffiolib", "audiolib", "scrolllib", "gameiolib", "mouselib",
    "collslib", "shapeslib", "fontlib", "editlib", "printlib", "stringfunclib", "linklistlib",
    NULL
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
    KEEPS_STATEMENT("Volume", none);
    KEEPS_STATEMENT("Sound", none);
    KEEPS_STATEMENT("LoopSound", none);
    /* test_lists */
    KEEPS_FUNCTION("FirstItem", none);
    KEEPS_FUNCTION("LastItem", none);
    KEEPS_FUNCTION("NextItem", none);
    KEEPS_FUNCTION("PrevItem", none);
    KEEPS_FUNCTION("AddItem", none);
    KEEPS_FUNCTION("AddFirst", none);
    KEEPS_FUNCTION("AddLast", none);
    KEEPS_STATEMENT("KillItem", none);
    KEEPS_STATEMENT("ClearList", none);
    KEEPS_STATEMENT("ResetList", none);
    /* the small entries (test_smallentries); Plot's d4-d7/a2 are checked
       on the C build only (both change d2 and d3) */
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
    KEEPS_FUNCTION("SColl", none);
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
    /* test_hits, test_frac, test_keys, test_print */
    KEEPS_FUNCTION("ShapesHit", none);
    KEEPS_FUNCTION("ShapeSpriteHit", none);
    KEEPS_FUNCTION("SpritesHit", none);
    KEEPS_FUNCTION("RectsHit", none);
    KEEPS_FUNCTION("Frac", none);
    /* Val leaves a3 at the string (MOVE.l d0,a3), the a3 case */
    KEEPS_STRFUNCTION("Val", none);
    KEEPS("rawkeylib init", none);
    KEEPS("rawkeylib finit", none);
    KEEPS("blitzkeyslib init", d2-d7|a2-a3|a6);
    /* the input handler entry keeps d1/a0-a1 (inputoutputlib passes every
       register on) */
    KEEPS_FUNCTION("BlitzKey", d1|a0|a1);
    KEEPS_FUNCTION("BlitzQualifier", none);
    KEEPS_STATEMENT("BlitzKeys", none);
    KEEPS_STRFUNCTION("Inkey$", none);
    KEEPS("BlitzFont _use", d0-d7|a0-a3|a6);
    KEEPS_STATEMENT("Print", none);
    KEEPS("dooutsingle", d1-d7|a0-a3|a6);
    /* test_strstr, test_instr */
    KEEPS_STRFUNCTION("Str$", none);
    KEEPS_STRFUNCTION("UStr$", none);
    KEEPS_STATEMENT("Format", none);
    KEEPS_STATEMENT("CaseSense", none);
    KEEPS_STRFUNCTION("Instr", none);
    /* test_strcut, test_hexbin, test_cv; Cvi, Cvl and Cvq leave a3 at the
       string (MOVE.l d0,a3), the a3 case */
    KEEPS_STRFUNCTION("Left$", none);
    KEEPS_STRFUNCTION("UnLeft$", none);
    KEEPS_STRFUNCTION("Right$", none);
    KEEPS_STRFUNCTION("UnRight$", none);
    KEEPS_STRFUNCTION("Mid$", none);
    KEEPS_STRFUNCTION("Hex$", none);
    KEEPS_STRFUNCTION("Bin$", none);
    KEEPS_STRFUNCTION("Cvi", none);
    KEEPS_STRFUNCTION("Cvl", none);
    KEEPS_STRFUNCTION("Cvq", none);
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
    test_lists();
    test_hits();
    test_frac();
    test_val();
    test_keys();
    test_print();
    test_strstr();
    test_instr();
    test_strcut();
    test_hexbin();
    test_cv();

    CloseLibrary(ffp);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
