/*
 * systest_system - functional comparison of the "system" group: wbstartuplib,
 * intlib, switchlib, runerrlib, debuglib (original against C build), on
 * the Amiga, inside a normal AmigaOS process.
 *
 *   systest_system DIR
 *
 * Each set (with memlib and exitslib, which the group calls) runs the
 * same scenario; the address independent results are written to a text
 * log per set and the logs are compared line by line.  Every call is also
 * checked with cmp_preserve().
 *
 * Not run here (they would leave the OS or need the debugger's traps):
 * switchlib goblitz/goblitzS (Forbid, display off), the debuglib trap
 * handlers / evaluate / execute, runerrlib finalscheck/retchk/nextchk/
 * casechk/bmodechk/amodechk/trapper and every failing check (TRAP #0).
 * They are assembler in the C build; their first opcode words are
 * compared here, the full instruction sequences with tools/asmcmp.py.
 */
#include <stdarg.h>
/* log lines are compared, a shortened FAIL message is fine */
#pragma GCC diagnostic ignored "-Wformat-truncation"
#include "sysload.h"
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>
#include <proto/mathffp.h>

struct GfxBase *GfxBase;
struct Library *MathBase;

/* ------------------------------------------------- helpers in assembler */

/* interrupt servers: count calls, return Z (go on with the chain) */
volatile ULONG cnt[3];
extern char server0[], server1[], server2[];
/* debuglib handlers: h6 records d0 and returns d1 = 42 */
volatile ULONG hcalls, hd0;
extern char handler6[], nullhandler[], stubrts[];
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _server0,_server1,_server2,_handler6,_nullhandler,_stubrts\n"
"_server0: addq.l #1,_cnt\n   moveq #0,d0\n   rts\n"
"_server1: addq.l #1,_cnt+4\n   moveq #0,d0\n   rts\n"
"_server2: addq.l #1,_cnt+8\n   moveq #0,d0\n   rts\n"
"_handler6: move.l d0,_hd0\n   addq.l #1,_hcalls\n   moveq #42,d1\n   rts\n"
"_nullhandler: rts\n"
"_stubrts: rts\n"
);

static const struct stub stubs[] = {
    { 0x5204, stubrts },                /* inputoutputlib (debuglib) */
    { 0x5d81, stubrts },                /* printlib (debuglib, unreferenced) */
    { 0, NULL }
};

/* ------------------------------------------------------------- scenario */

#define MAXCALLS 160

struct result {
    struct call c[MAXCALLS];
    int k;
    char log[8192];
    int len;
};

static struct result res[2];

static void LOG(struct result *rs, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rs->len += vsprintf(rs->log + rs->len, fmt, ap);
    va_end(ap);
    rs->log[rs->len++] = '\n';
    rs->log[rs->len] = 0;
}

static struct regs r;
static struct result *cur;

static UWORD call(APTR fn)
{
    struct call *c = &cur->c[cur->k++];
    c->in = r;
    c->ccr = callregs(fn, &r, 0, 0);
    c->out = r;
    return c->ccr;
}

/* set up registers: markers, then the inputs */
#define SETUP(stmts) do { markregs(&r); stmts; } while (0)

/* 6 frames, busy waiting on VPOSR bit 0 (V8): works under Forbid, so
   no other task can allocate memory during the memory balance checks */
static void waitframes(void)
{
    volatile UBYTE *vposr = (volatile UBYTE *)0xdff005;
    int i;
    for (i = 0; i < 6; i++) {
        while (!(*vposr & 1))
            ;
        while (*vposr & 1)
            ;
    }
}

/* the argument list of wbstartuplib */
static void logargs(struct result *rs, ULONG data)
{
    UBYTE *d = (UBYTE *)data;
    UBYTE *a;
    LOG(rs, "wb fromcli=%04x numargs=%ld", W(d), (LONG)L(d + 2));
    for (a = (UBYTE *)L(d + 6); a; a = (UBYTE *)L(a)) {
        char buf[100];
        UWORD len = W(a + 8);
        memcpy(buf, (UBYTE *)L(a + 4), len < 99 ? len : 99);
        buf[len < 99 ? len : 99] = 0;
        LOG(rs, "  arg len=%d lock=%lx text=<%s> term=%d", len, L(a + 10), buf,
            ((UBYTE *)L(a + 4))[len]);
    }
}

static void wbtest(struct set *s, struct result *rs)
{
    struct lib *l = getlib(s, "wbstartuplib");
    static const char *lines[] = {
        "  foo \"bar baz\" \"\" qux\n",
        "single\n",
        "x y",                          /* no LF: reads on behind the 0
                                           into what the earlier calls
                                           left in cliString (a program
                                           writes it only once: no bug) */
        "",
        "\"open quote\n",
    };
    int i;
    for (i = 0; i < 5; i++) {
        ULONG avail = AvailMem(MEMF_ANY);
        static char line[64];
        strcpy(line, lines[i]);
        /* i == 1: d7 not "bLtZ" -> FindTask, our process is a CLI one */
        SETUP(r.a[0] = (ULONG)line; r.d[7] = i == 1 ? 0 : 0x624c745a);
        call(entry_code(l, 1));                         /* init */
        logargs(rs, r.d[0]);
        SETUP((void)0);
        call(entry_code(l, 2));                         /* finit */
        LOG(rs, "wb memory back: %d", AvailMem(MEMF_ANY) == avail);
    }
}

static void inttest(struct set *s, struct result *rs)
{
    struct lib *l = getlib(s, "intlib");
    UBYTE *setint = entry_code(l, 0), *clrint = entry_code(l, 1);
    UBYTE *vwait = entry_code(l, 4);
    ULONG avail;

    Forbid();
    avail = AvailMem(MEMF_ANY);
    LOG(rs, "int prefixes %04x %04x %04x", W(setint), W(clrint), W(vwait));
    SETUP(r.a[5] = 0x12345678);
    call(entry_code(l, 8));                             /* init */
    SETUP(r.a[5] = 0);
    call(entry_code(l, 2));                             /* newint */
    LOG(rs, "newint a5=%08lx", r.a[5]);
    SETUP((void)0);
    call(entry_code(l, 3));                             /* oldint */

    /* Amiga mode SetInt: level 5 = VERTB */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server0);
    call(setint + 6);
    SETUP(r.d[0] = 0x105; r.d[1] = (ULONG)server1);
    call(setint + 6);
    cnt[0] = cnt[1] = cnt[2] = 0;
    waitframes();
    LOG(rs, "setint: s0 %d s1 %d", cnt[0] > 0, cnt[1] > 0);
    SETUP(r.d[0] = 5);
    call(clrint + 6);                                   /* only level word 5 */
    cnt[0] = cnt[1] = 0;
    waitframes();
    LOG(rs, "clrint 5: s0 %d s1 %d", cnt[0] > 0, cnt[1] > 0);

    /* Blitz mode SetInt: only listed until goblitz */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server2);
    call((APTR)L(setint + 2));
    cnt[2] = 0;
    waitframes();
    LOG(rs, "b_setint: s2 %d", cnt[2] > 0);
    SETUP((void)0);
    call(entry_code(l, 6));                             /* goblitz */
    cnt[2] = 0;
    waitframes();
    LOG(rs, "goblitz: s2 %d", cnt[2] > 0);
    SETUP((void)0);
    call(entry_code(l, 6));                             /* again: nothing */
    SETUP((void)0);
    call(entry_code(l, 7));                             /* goamiga */
    waitframes();
    cnt[2] = 0;
    waitframes();
    LOG(rs, "goamiga: s2 %d", cnt[2] > 0);
    SETUP(r.d[0] = 5);
    call((APTR)L(clrint + 2));                          /* b_clrint, not on */

    /* a server left in each list for cleanup */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server2);
    call((APTR)L(setint + 2));
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server0);
    call(setint + 6);
    SETUP((void)0);
    call(entry_code(l, 5));                             /* cleanup */
    cnt[0] = cnt[1] = cnt[2] = 0;
    waitframes();
    LOG(rs, "cleanup: s0 %d s1 %d s2 %d", cnt[0] > 0, cnt[1] > 0, cnt[2] > 0);
    LOG(rs, "int memory back: %d", AvailMem(MEMF_ANY) == avail);
    Permit();

    /* VWait both variants */
    SETUP(r.d[0] = 1; r.a[6] = (ULONG)GfxBase);
    call(vwait + 6);
    LOG(rs, "vwait d0=%08lx", r.d[0] & 0xffff0000);
    SETUP(r.d[0] = 1);
    call((APTR)L(vwait + 2));
    LOG(rs, "b_vwait d0=%08lx", r.d[0]);

}

static void switchtest(struct set *s, struct result *rs)
{
    struct lib *l = getlib(s, "switchlib");
    struct View *view = GfxBase->ActiView;
    struct Node *kbd;

    Forbid();
    kbd = FindName(&SysBase->DeviceList, (STRPTR)"keyboard.device");
    Permit();
    SETUP((void)0);
    call(entry_code(l, 8));                             /* init */
    LOG(rs, "switch init = keyboard.device: %d", r.d[0] == (ULONG)kbd);
    SETUP((void)0);
    call(entry_code(l, 3));                             /* OwnBlitter */
    SETUP((void)0);
    call(entry_code(l, 3));                             /* again: nothing */
    SETUP((void)0);
    call(entry_code(l, 4));                             /* DisownBlitter */
    SETUP((void)0);
    call(entry_code(l, 4));
    SETUP(r.d[0] = 0xabcd5555);
    call(entry_code(l, 7));                             /* goqamigaS */
    LOG(rs, "goqamigaS d0=%08lx view kept %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.d[0] = 0x12340000);
    call(entry_code(l, 1));                             /* goamiga */
    LOG(rs, "goamiga d0=%08lx view kept %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.d[0] = 0);
    call(entry_code(l, 2));                             /* goqamiga */
    LOG(rs, "goqamiga d0=%08lx view off %d", r.d[0], GfxBase->ActiView == NULL);
    SETUP(r.d[0] = 0xffffffff);
    call(entry_code(l, 2));                             /* again */
    LOG(rs, "goqamiga d0=%08lx", r.d[0]);
    SETUP(r.d[0] = 0);
    call(entry_code(l, 1));                             /* goamiga */
    LOG(rs, "goamiga d0=%08lx view back %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.a[0] = 0);
    call(entry_code(l, 5));                             /* setcoplist */
    SETUP((void)0);
    call(entry_code(l, 9));                             /* finit */
}

static ULONG vbr;

static void runerrtest(struct set *s, struct result *rs)
{
    struct lib *l = getlib(s, "runerrlib");
    struct Task *me = FindTask(NULL);
    APTR trap = me->tc_TrapCode;
    static UBYTE data[4] = { 'D', 3, 'D', 5 };
    UBYTE *code;

    SETUP(r.d[2] = 0x10000);
    call(entry_code(l, 31));                            /* init */
    code = me->tc_TrapCode;
    LOG(rs, "runerr trapcode set %d, trapper %04x %04x", code != trap, W(code), W(code + 2));
    SETUP((void)0);
    call(entry_code(l, 29));                            /* getvbr */
    vbr = r.a[0];
    LOG(rs, "getvbr %08lx", r.a[0]);
    SETUP(r.d[0] = 0xabcd1234);
    call(entry_code(l, 30));                            /* getcpu */
    LOG(rs, "getcpu %08lx", r.d[0]);

    /* value checks, all in range (a failure would TRAP #0) */
    SETUP(r.d[0] = 127); call(entry_code(l, 22));
    SETUP(r.d[0] = -128); call(entry_code(l, 22));
    SETUP(r.d[0] = 0x12340010); call(entry_code(l, 23));
    SETUP(r.d[0] = 0x0000ff80); call(entry_code(l, 23));
    SETUP(r.d[0] = 32767); call(entry_code(l, 24));
    SETUP(r.d[0] = -32768); call(entry_code(l, 24));
    SETUP(r.d[0] = 0x007fffff); call(entry_code(l, 25));
    SETUP(r.d[0] = 0xff800000); call(entry_code(l, 25));
    SETUP(r.d[0] = 0xC8000047 /* FFP 100 */; r.a[6] = (ULONG)MathBase); call(entry_code(l, 26));
    SETUP(r.d[0] = 0x800000C8 /* FFP -128 */; r.a[6] = (ULONG)MathBase); call(entry_code(l, 26));
    SETUP(r.d[0] = 0xEA60004F /* FFP 30000 */; r.a[6] = (ULONG)MathBase); call(entry_code(l, 27));
    SETUP(r.d[0] = 0xF42400D4 /* FFP -1e6 */; r.a[6] = (ULONG)MathBase); call(entry_code(l, 28));
    SETUP(r.d[0] = 65535); call(entry_code(l, 14));
    SETUP(r.d[1] = 0); call(entry_code(l, 15));
    SETUP(r.d[0] = 3; r.a[3] = (ULONG)data); call(entry_code(l, 16));
    LOG(rs, "datachk a3+%ld", r.a[3] - (ULONG)data);
    SETUP(r.d[0] = 0x105; r.a[3] = (ULONG)data + 2); call(entry_code(l, 16));
    LOG(rs, "datachk a3+%ld", r.a[3] - (ULONG)data);
    SETUP(r.a[2] = 1); call(entry_code(l, 21));
    SETUP((void)0); call(entry_code(l, 11));            /* gosuper */
    SETUP((void)0); call(entry_code(l, 8));             /* stackchk: skipped */
    SETUP((void)0); call(entry_code(l, 12));            /* gouser */
    SETUP((void)0); call(entry_code(l, 8));             /* stackchk: a7 ok */
    LOG(rs, "runerr checks passed");
    /* assembler routines: first opcode words */
    {
        static const int asmidx[] = { 0, 5, 6, 7, 17, 18, 19, 20, 9, 10 };
        int i;
        for (i = 0; i < 10; i++) {
            UBYTE *p = entry_code(l, asmidx[i]);
            LOG(rs, "runerr asm entry %d: %04x", asmidx[i], W(p));
        }
    }
    SETUP((void)0);
    call(entry_code(l, 32));                            /* finit */
    LOG(rs, "runerr trapcode restored %d", me->tc_TrapCode == trap);
}

/* fake token list of the editor */
static UBYTE tok1[] = { 0, 0, 0, 0, 0x00, 0x05, 'P', 'R', 'I', 'N', 'T', 0 };
static UBYTE tok2[] = { 0, 0, 0, 0, 0x01, 0x23, 'N', 'P', 'r', 'i', 'n', 't', 0 };
static UBYTE tok3[] = { 0, 0, 0, 0, 0x00, 0x07, 0 };            /* empty name */

static void debugtest(struct set *s, struct result *rs)
{
    struct lib *l = getlib(s, "debuglib");
    APTR *vec = (APTR *)vbr;
    APTR save[6];
    APTR tab1[8], tab2[8];
    static UWORD pflag;
    UBYTE *state;
    int i;
    static const UBYTE src[] = { 'A', 0x80, 0x05, 'B', 0x81, 0x23, 'C', 0xff, 0xff,
                                 'D', 0x80, 0x07, 'E', 0 };
    UBYTE dst[80];

    /* init sets TRAP #0-#5 in the VBR table: restore them at once */
    Disable();
    for (i = 0; i < 6; i++)
        save[i] = vec[32 + i];
    SETUP(r.d[0] = 0x100);              /* d0.b = 0: stop at once */
    call(entry_code(l, 10));
    for (i = 0; i < 6; i++) {
        UBYTE *h = vec[32 + i];
        /* only the opcode: the next word is the upper half of an absolute
           address in the library copy (depends on the memory layout) */
        LOG(rs, "trap %d handler %04x", i, W(h));
        vec[32 + i] = save[i];
    }
    CacheClearU();
    Enable();

    for (i = 0; i < 8; i++)
        tab1[i] = tab2[i] = nullhandler;
    tab1[6] = handler6;
    pflag = 0;
    SETUP(r.d[0] = (ULONG)&pflag; r.a[0] = (ULONG)tab1);
    call(entry_code(l, 2));                             /* dosethandler */
    state = (UBYTE *)r.a[1];
    LOG(rs, "sethandler old-state=%lx tokens=vec1 %d", r.d[0] - r.a[1], r.a[2] == (ULONG)save[1]);
    LOG(rs, "state oldmode %04x flags %08lx usertext <%s>", W(state), L(state + 2),
        (char *)L(state + 14));
    SETUP((void)0); call(entry_code(l, 1));             /* dostop */
    LOG(rs, "dostop flags %08lx <%s>", L(state + 2), (char *)L(state + 14));
    SETUP(r.a[0] = (ULONG)"a+b"); call(entry_code(l, 4));
    LOG(rs, "doevaluate flags %08lx <%s>", L(state + 2), (char *)L(state + 18));
    SETUP(r.a[0] = (ULONG)"x=1"); call(entry_code(l, 5));
    LOG(rs, "doexecute flags %08lx <%s>", L(state + 2), (char *)L(state + 22));
    SETUP((void)0); call(entry_code(l, 6));             /* doquit */
    SETUP((void)0); call(entry_code(l, 8));             /* doskip */
    LOG(rs, "quit/skip flags %08lx", L(state + 2));
    SETUP((void)0); call(entry_code(l, 0));             /* donewmode */
    hcalls = 0;
    SETUP(r.d[0] = 0x1234); call(entry_code(l, 9));     /* docall */
    LOG(rs, "docall calls %ld d0 %lx d1 %ld", hcalls, hd0, r.d[1]);
    SETUP(r.d[0] = (ULONG)&pflag; r.a[0] = (ULONG)tab2);
    call(entry_code(l, 2));
    LOG(rs, "sethandler old = tab1 %d", r.d[0] == (ULONG)tab1);

    /* token list (written into the state block like the editor's) */
    *(ULONG *)tok1 = (ULONG)tok2;
    *(ULONG *)tok2 = (ULONG)tok3;
    *(ULONG *)(state + 6) = (ULONG)tok1;
    memset(dst, 0x55, sizeof dst);
    SETUP(r.a[0] = (ULONG)src; r.a[1] = (ULONG)dst);
    call(entry_code(l, 3));                             /* dodetoke */
    LOG(rs, "detoke <%s> a0+%ld a1+%ld", dst, r.a[0] - (ULONG)src, r.a[1] - (ULONG)dst);
    for (i = 0; i <= 14; i++) {
        SETUP(r.a[0] = (ULONG)src; r.d[0] = 0x77770000 | i);
        call(entry_code(l, 7));                         /* dofindoff */
        LOG(rs, "findoff %d -> %08lx", i, r.d[0]);
    }
    *(ULONG *)(state + 6) = 0;
    SETUP((void)0); call(entry_code(l, 11));            /* finit */
    {
        UBYTE *p = entry_code(l, 9);
        LOG(rs, "docall code %04x %04x %04x", W(p), W(p + 4), W(p + 6));
    }
}

static void run(struct set *s, struct result *rs)
{
    cur = rs;
    rs->k = 0;
    SETUP((void)0);
    call(entry_code(getlib(s, "memlib"), 7));           /* memlib init */
    wbtest(s, rs);
    inttest(s, rs);
    switchtest(s, rs);
    runerrtest(s, rs);
    debugtest(s, rs);
    SETUP((void)0);
    call(entry_code(getlib(s, "memlib"), 8));           /* memlib finit */
    LOG(rs, "calls %d", rs->k);
}

static const char *names[] = {
    "memlib", "exitslib", "intlib", "switchlib", "runerrlib", "debuglib",
    "wbstartuplib", NULL
};

int main(int argc, char **argv)
{
    int i;
    char *a, *b;

    if (argc < 2) {
        printf("usage: systest_system DIR\n");
        return 20;
    }
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    MathBase = OpenLibrary((STRPTR)"mathffp.library", 0);
    if (!GfxBase || !MathBase)
        return 20;
    sysload_stubs = stubs;
    /* warm up the VERTB server chain and graphics (first use may allocate
       memory, which would spoil the memory balance of the first set) */
    {
        static struct Interrupt is;
        is.is_Code = (void (*)())server0;
        is.is_Node.ln_Type = NT_INTERRUPT;
        AddIntServer(5, &is);
        waitframes();
        RemIntServer(5, &is);
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (i = 0; i < 2; i++)
        run(&sets[i], &res[i]);

    /* compare the logs line by line */
    a = res[0].log;
    b = res[1].log;
    while (*a || *b) {
        char *ea = strchr(a, '\n'), *eb = strchr(b, '\n');
        char buf[600];
        if (!ea) ea = a + strlen(a);
        if (!eb) eb = b + strlen(b);
        *ea = 0;
        *eb = 0;
        snprintf(buf, sizeof buf, "orig: %s\n     C: %s", a, b);
        check(buf, tape_strsame(buf, a, b));
        a = ea + 1;
        b = eb + 1;
    }
    check("number of calls", tape_same("number of calls", res[0].k, res[1].k));
    for (i = 0; i < res[0].k && i < res[1].k; i++) {
        char buf[40];
        sprintf(buf, "call %d", i);
        cmp_preserve(buf, &res[0].c[i], &res[1].c[i]);
    }
    printf("%d checks, %d failures\n", checks, failures);
    printf("log (orig):\n");
    for (a = res[0].log; a < res[0].log + res[0].len; a += strlen(a) + 1)
        printf("  %s\n", a);
    CloseLibrary(MathBase);
    CloseLibrary((struct Library *)GfxBase);
    return failures ? 5 : 0;
}
