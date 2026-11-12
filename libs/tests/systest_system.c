/*
 * systest_system - the "system" group wbstartuplib, intlib, switchlib,
 * runerrlib, debuglib on the Amiga, inside a normal AmigaOS process: one
 * set (the original or the C build, as LIBSET says), observed as cases
 * (sysload.h; the expected data
 * libs/tests/expected/systest_system.txt).
 *
 *   systest_system DIR
 *
 * The set (with memlib and exitslib, which the group calls) runs the
 * scenario; every address independent result is one text case (LOG: a
 * name as the check, the text as the value), and every call writes its
 * kept registers (KEEPS: the set the original keeps in all calls of the
 * routine).
 *
 * Not run here (they would leave the OS or need the debugger's traps):
 * switchlib goblitz/goblitzS (Forbid, display off), the debuglib trap
 * handlers / evaluate / execute, runerrlib finalscheck/retchk/nextchk/
 * casechk/bmodechk/amodechk/trapper and every failing check (TRAP #0).
 * They are assembler in the C build; their first opcode words are
 * written here, the full instruction sequences compared with
 * tools/asmcmp.py.
 */
#include <stdarg.h>
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

static int ncalls;

/* one result as the case |what|VEC: the text as its value.  The cases go
   to capture.c's 64 KB buffer, which this program does not fill, so no
   Write happens under the Forbid of inttest */
static void LOG(const char *what, const char *fmt, ...)
{
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    ROUT = "";
    OBSSTR(what, buf);
}

static struct regs r;

/* one call of the routine `name` (a literal: KEEPS matches it by text):
   its kept case name|kept|VEC */
static UWORD call(const char *name, APTR fn)
{
    struct regs in = r;
    UWORD ccr;
    ROUT = name;
    ccr = callregs(fn, &r, 0, 0);
    obs_kept(&in, &r);
    ROUT = "";
    ncalls++;
    return ccr;
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
static void logargs(ULONG data)
{
    UBYTE *d = (UBYTE *)data;
    UBYTE *a;
    LOG("wb args", "wb fromcli=%04x numargs=%ld", W(d), (LONG)L(d + 2));
    for (a = (UBYTE *)L(d + 6); a; a = (UBYTE *)L(a)) {
        char buf[100];
        UWORD len = W(a + 8);
        memcpy(buf, (UBYTE *)L(a + 4), len < 99 ? len : 99);
        buf[len < 99 ? len : 99] = 0;
        LOG("wb arg", "  arg len=%d lock=%lx text=<%s> term=%d", len, L(a + 10), buf,
            ((UBYTE *)L(a + 4))[len]);
    }
}

static void wbtest(struct set *s)
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
        sprintf(VEC, "line %d", i);
        SETUP(r.a[0] = (ULONG)line; r.d[7] = i == 1 ? 0 : 0x624c745a);
        call("wbstartup init", entry_code(l, 1));
        logargs(r.d[0]);
        SETUP((void)0);
        call("wbstartup finit", entry_code(l, 2));
        LOG("wb memory back", "wb memory back: %d", AvailMem(MEMF_ANY) == avail);
    }
    VEC[0] = 0;
}

static void inttest(struct set *s)
{
    struct lib *l = getlib(s, "intlib");
    UBYTE *setint = entry_code(l, 0), *clrint = entry_code(l, 1);
    UBYTE *vwait = entry_code(l, 4);
    ULONG avail;

    Forbid();
    avail = AvailMem(MEMF_ANY);
    LOG("int prefixes", "int prefixes %04x %04x %04x", W(setint), W(clrint), W(vwait));
    SETUP(r.a[5] = 0x12345678);
    call("int init", entry_code(l, 8));
    SETUP(r.a[5] = 0);
    DONTCARE = 1UL << 13;                               /* a5: the result */
    call("newint", entry_code(l, 2));
    DONTCARE = 0;
    LOG("newint a5", "newint a5=%08lx", r.a[5]);
    SETUP((void)0);
    call("oldint", entry_code(l, 3));

    /* Amiga mode SetInt: level 5 = VERTB */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server0);
    call("setint", setint + 6);
    SETUP(r.d[0] = 0x105; r.d[1] = (ULONG)server1);
    call("setint", setint + 6);
    cnt[0] = cnt[1] = cnt[2] = 0;
    waitframes();
    LOG("setint", "setint: s0 %d s1 %d", cnt[0] > 0, cnt[1] > 0);
    SETUP(r.d[0] = 5);
    call("clrint", clrint + 6);                         /* only level word 5 */
    cnt[0] = cnt[1] = 0;
    waitframes();
    LOG("clrint 5", "clrint 5: s0 %d s1 %d", cnt[0] > 0, cnt[1] > 0);

    /* Blitz mode SetInt: only listed until goblitz */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server2);
    call("b_setint", (APTR)L(setint + 2));
    cnt[2] = 0;
    waitframes();
    LOG("b_setint", "b_setint: s2 %d", cnt[2] > 0);
    SETUP((void)0);
    call("int goblitz", entry_code(l, 6));
    cnt[2] = 0;
    waitframes();
    LOG("int goblitz", "goblitz: s2 %d", cnt[2] > 0);
    SETUP((void)0);
    call("int goblitz", entry_code(l, 6));              /* again: nothing */
    SETUP((void)0);
    call("int goamiga", entry_code(l, 7));
    waitframes();
    cnt[2] = 0;
    waitframes();
    LOG("int goamiga", "goamiga: s2 %d", cnt[2] > 0);
    SETUP(r.d[0] = 5);
    call("b_clrint", (APTR)L(clrint + 2));              /* not on */

    /* a server left in each list for cleanup */
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server2);
    call("b_setint", (APTR)L(setint + 2));
    SETUP(r.d[0] = 5; r.d[1] = (ULONG)server0);
    call("setint", setint + 6);
    SETUP((void)0);
    call("int cleanup", entry_code(l, 5));
    cnt[0] = cnt[1] = cnt[2] = 0;
    waitframes();
    LOG("cleanup", "cleanup: s0 %d s1 %d s2 %d", cnt[0] > 0, cnt[1] > 0, cnt[2] > 0);
    LOG("int memory back", "int memory back: %d", AvailMem(MEMF_ANY) == avail);
    Permit();

    /* VWait both variants */
    SETUP(r.d[0] = 1; r.a[6] = (ULONG)GfxBase);
    call("vwait", vwait + 6);
    LOG("vwait d0", "vwait d0=%08lx", r.d[0] & 0xffff0000);
    SETUP(r.d[0] = 1);
    call("b_vwait", (APTR)L(vwait + 2));
    LOG("b_vwait d0", "b_vwait d0=%08lx", r.d[0]);

}

static void switchtest(struct set *s)
{
    struct lib *l = getlib(s, "switchlib");
    struct View *view = GfxBase->ActiView;
    struct Node *kbd;

    Forbid();
    kbd = FindName(&SysBase->DeviceList, (STRPTR)"keyboard.device");
    Permit();
    SETUP((void)0);
    call("switch init", entry_code(l, 8));
    LOG("switch init", "switch init = keyboard.device: %d", r.d[0] == (ULONG)kbd);
    SETUP((void)0);
    call("OwnBlitter", entry_code(l, 3));
    SETUP((void)0);
    call("OwnBlitter", entry_code(l, 3));               /* again: nothing */
    SETUP((void)0);
    call("DisownBlitter", entry_code(l, 4));
    SETUP((void)0);
    call("DisownBlitter", entry_code(l, 4));
    SETUP(r.d[0] = 0xabcd5555);
    call("goqamigaS", entry_code(l, 7));
    LOG("goqamigaS", "goqamigaS d0=%08lx view kept %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.d[0] = 0x12340000);
    call("goamiga", entry_code(l, 1));
    LOG("goamiga", "goamiga d0=%08lx view kept %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.d[0] = 0);
    call("goqamiga", entry_code(l, 2));
    LOG("goqamiga", "goqamiga d0=%08lx view off %d", r.d[0], GfxBase->ActiView == NULL);
    SETUP(r.d[0] = 0xffffffff);
    call("goqamiga", entry_code(l, 2));                 /* again */
    LOG("goqamiga again", "goqamiga d0=%08lx", r.d[0]);
    SETUP(r.d[0] = 0);
    call("goamiga", entry_code(l, 1));
    LOG("goamiga back", "goamiga d0=%08lx view back %d", r.d[0], GfxBase->ActiView == view);
    SETUP(r.a[0] = 0);
    call("setcoplist", entry_code(l, 5));
    SETUP((void)0);
    call("switch finit", entry_code(l, 9));
}

static ULONG vbr;

static void runerrtest(struct set *s)
{
    struct lib *l = getlib(s, "runerrlib");
    struct Task *me = FindTask(NULL);
    APTR trap = me->tc_TrapCode;
    static UBYTE data[4] = { 'D', 3, 'D', 5 };
    UBYTE *code;

    SETUP(r.d[2] = 0x10000);
    call("runerr init", entry_code(l, 31));
    code = me->tc_TrapCode;
    LOG("runerr trapcode set", "runerr trapcode set %d, trapper %04x %04x", code != trap,
        W(code), W(code + 2));
    SETUP((void)0);
    call("getvbr", entry_code(l, 29));
    vbr = r.a[0];
    LOG("getvbr", "getvbr %08lx", r.a[0]);
    SETUP(r.d[0] = 0xabcd1234);
    call("getcpu", entry_code(l, 30));
    LOG("getcpu", "getcpu %08lx", r.d[0]);

    /* value checks, all in range (a failure would TRAP #0) */
    SETUP(r.d[0] = 127); call("runerr 22", entry_code(l, 22));
    SETUP(r.d[0] = -128); call("runerr 22", entry_code(l, 22));
    SETUP(r.d[0] = 0x12340010); call("runerr 23", entry_code(l, 23));
    SETUP(r.d[0] = 0x0000ff80); call("runerr 23", entry_code(l, 23));
    SETUP(r.d[0] = 32767); call("runerr 24", entry_code(l, 24));
    SETUP(r.d[0] = -32768); call("runerr 24", entry_code(l, 24));
    SETUP(r.d[0] = 0x007fffff); call("runerr 25", entry_code(l, 25));
    SETUP(r.d[0] = 0xff800000); call("runerr 25", entry_code(l, 25));
    SETUP(r.d[0] = 0xC8000047 /* FFP 100 */; r.a[6] = (ULONG)MathBase);
    call("runerr 26", entry_code(l, 26));
    SETUP(r.d[0] = 0x800000C8 /* FFP -128 */; r.a[6] = (ULONG)MathBase);
    call("runerr 26", entry_code(l, 26));
    SETUP(r.d[0] = 0xEA60004F /* FFP 30000 */; r.a[6] = (ULONG)MathBase);
    call("runerr 27", entry_code(l, 27));
    SETUP(r.d[0] = 0xF42400D4 /* FFP -1e6 */; r.a[6] = (ULONG)MathBase);
    call("runerr 28", entry_code(l, 28));
    SETUP(r.d[0] = 65535); call("runerr 14", entry_code(l, 14));
    SETUP(r.d[1] = 0); call("runerr 15", entry_code(l, 15));
    SETUP(r.d[0] = 3; r.a[3] = (ULONG)data); call("datachk", entry_code(l, 16));
    LOG("datachk a3", "datachk a3+%ld", r.a[3] - (ULONG)data);
    SETUP(r.d[0] = 0x105; r.a[3] = (ULONG)data + 2); call("datachk", entry_code(l, 16));
    LOG("datachk a3", "datachk a3+%ld", r.a[3] - (ULONG)data);
    SETUP(r.a[2] = 1); call("runerr 21", entry_code(l, 21));
    SETUP((void)0); call("gosuper", entry_code(l, 11));
    SETUP((void)0); call("stackchk", entry_code(l, 8));    /* skipped */
    SETUP((void)0); call("gouser", entry_code(l, 12));
    SETUP((void)0); call("stackchk", entry_code(l, 8));    /* a7 ok */
    LOG("runerr checks passed", "runerr checks passed");
    /* assembler routines: first opcode words */
    {
        static const int asmidx[] = { 0, 5, 6, 7, 17, 18, 19, 20, 9, 10 };
        int i;
        for (i = 0; i < 10; i++) {
            UBYTE *p = entry_code(l, asmidx[i]);
            sprintf(VEC, "entry %d", asmidx[i]);
            LOG("runerr asm entry", "runerr asm entry %d: %04x", asmidx[i], W(p));
        }
        VEC[0] = 0;
    }
    SETUP((void)0);
    call("runerr finit", entry_code(l, 32));
    LOG("runerr trapcode restored", "runerr trapcode restored %d", me->tc_TrapCode == trap);
}

/* fake token list of the editor */
static UBYTE tok1[] = { 0, 0, 0, 0, 0x00, 0x05, 'P', 'R', 'I', 'N', 'T', 0 };
static UBYTE tok2[] = { 0, 0, 0, 0, 0x01, 0x23, 'N', 'P', 'r', 'i', 'n', 't', 0 };
static UBYTE tok3[] = { 0, 0, 0, 0, 0x00, 0x07, 0 };            /* empty name */

static void debugtest(struct set *s)
{
    struct lib *l = getlib(s, "debuglib");
    APTR *vec = (APTR *)vbr;
    APTR save[6];
    APTR tab1[8], tab2[8];
    static UWORD pflag;
    UBYTE *state;
    UWORD hw[6];
    int i;
    static const UBYTE src[] = { 'A', 0x80, 0x05, 'B', 0x81, 0x23, 'C', 0xff, 0xff,
                                 'D', 0x80, 0x07, 'E', 0 };
    UBYTE dst[80];

    /* init sets TRAP #0-#5 in the VBR table: restore them at once (the
       cases are written after Enable) */
    Disable();
    for (i = 0; i < 6; i++)
        save[i] = vec[32 + i];
    SETUP(r.d[0] = 0x100);              /* d0.b = 0: stop at once */
    {
        struct regs in = r;
        callregs(entry_code(l, 10), &r, 0, 0);
        for (i = 0; i < 6; i++) {
            /* only the opcode: the next word is the upper half of an
               absolute address in the library copy (depends on the memory
               layout) */
            hw[i] = W((UBYTE *)vec[32 + i]);
            vec[32 + i] = save[i];
        }
        CacheClearU();
        Enable();
        ROUT = "debug init";
        obs_kept(&in, &r);
        ROUT = "";
        ncalls++;
    }
    for (i = 0; i < 6; i++) {
        sprintf(VEC, "trap %d", i);
        LOG("trap handler", "trap %d handler %04x", i, hw[i]);
    }
    VEC[0] = 0;

    for (i = 0; i < 8; i++)
        tab1[i] = tab2[i] = nullhandler;
    tab1[6] = handler6;
    pflag = 0;
    SETUP(r.d[0] = (ULONG)&pflag; r.a[0] = (ULONG)tab1);
    call("dosethandler", entry_code(l, 2));
    state = (UBYTE *)r.a[1];
    LOG("sethandler", "sethandler old-state=%lx tokens=vec1 %d", r.d[0] - r.a[1],
        r.a[2] == (ULONG)save[1]);
    LOG("state", "state oldmode %04x flags %08lx usertext <%s>", W(state), L(state + 2),
        (char *)L(state + 14));
    SETUP((void)0); call("dostop", entry_code(l, 1));
    LOG("dostop", "dostop flags %08lx <%s>", L(state + 2), (char *)L(state + 14));
    SETUP(r.a[0] = (ULONG)"a+b"); call("doevaluate", entry_code(l, 4));
    LOG("doevaluate", "doevaluate flags %08lx <%s>", L(state + 2), (char *)L(state + 18));
    SETUP(r.a[0] = (ULONG)"x=1"); call("doexecute", entry_code(l, 5));
    LOG("doexecute", "doexecute flags %08lx <%s>", L(state + 2), (char *)L(state + 22));
    SETUP((void)0); call("doquit", entry_code(l, 6));
    SETUP((void)0); call("doskip", entry_code(l, 8));
    LOG("quit/skip", "quit/skip flags %08lx", L(state + 2));
    SETUP((void)0); call("donewmode", entry_code(l, 0));
    hcalls = 0;
    SETUP(r.d[0] = 0x1234); call("docall", entry_code(l, 9));
    LOG("docall", "docall calls %ld d0 %lx d1 %ld", hcalls, hd0, r.d[1]);
    SETUP(r.d[0] = (ULONG)&pflag; r.a[0] = (ULONG)tab2);
    call("dosethandler", entry_code(l, 2));
    LOG("sethandler old", "sethandler old = tab1 %d", r.d[0] == (ULONG)tab1);

    /* token list (written into the state block like the editor's) */
    *(ULONG *)tok1 = (ULONG)tok2;
    *(ULONG *)tok2 = (ULONG)tok3;
    *(ULONG *)(state + 6) = (ULONG)tok1;
    memset(dst, 0x55, sizeof dst);
    SETUP(r.a[0] = (ULONG)src; r.a[1] = (ULONG)dst);
    call("dodetoke", entry_code(l, 3));
    LOG("detoke", "detoke <%s> a0+%ld a1+%ld", dst, r.a[0] - (ULONG)src, r.a[1] - (ULONG)dst);
    for (i = 0; i <= 14; i++) {
        sprintf(VEC, "%d", i);
        SETUP(r.a[0] = (ULONG)src; r.d[0] = 0x77770000 | i);
        call("dofindoff", entry_code(l, 7));
        LOG("findoff", "findoff %d -> %08lx", i, r.d[0]);
    }
    VEC[0] = 0;
    *(ULONG *)(state + 6) = 0;
    SETUP((void)0); call("debug finit", entry_code(l, 11));
    {
        UBYTE *p = entry_code(l, 9);
        LOG("docall code", "docall code %04x %04x %04x", W(p), W(p + 4), W(p + 6));
    }
}

static void run(struct set *s)
{
    SETUP((void)0);
    call("memlib init", entry_code(getlib(s, "memlib"), 7));
    wbtest(s);
    inttest(s);
    switchtest(s);
    runerrtest(s);
    debugtest(s);
    SETUP((void)0);
    call("memlib finit", entry_code(getlib(s, "memlib"), 8));
    LOG("calls", "calls %d", ncalls);
}

static const char *names[] = {
    "memlib", "exitslib", "intlib", "switchlib", "runerrlib", "debuglib",
    "wbstartuplib", NULL
};

int main(int argc, char **argv)
{
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
       memory, which would spoil the memory balance of the scenario) */
    {
        static struct Interrupt is;
        is.is_Code = (void (*)())server0;
        is.is_Node.ln_Type = NT_INTERRUPT;
        AddIntServer(5, &is);
        waitframes();
        RemIntServer(5, &is);
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("wbstartup init", d1-d7|a1-a3|a6);
    KEEPS("wbstartup finit", d2-d7);
    KEEPS("int init", d1-d7|a1-a3|a6);
    KEEPS("newint", d0-d7|a0-a3|a6);
    KEEPS("oldint", d0-d7|a0-a3|a6);
    KEEPS("setint", d2-d7|a0-a3|a6);
    KEEPS("clrint", d1-d7|a0-a3|a6);
    KEEPS("b_setint", d0-d7|a0-a3|a6);
    KEEPS("int goblitz", d0-d7|a0-a3|a6);
    KEEPS("int goamiga", d0-d7|a0-a3|a6);
    KEEPS("b_clrint", d1-d7|a0-a3|a6);
    KEEPS("int cleanup", d3-d7);
    KEEPS("vwait", d3-d7|a2|a3|a6);
    KEEPS("b_vwait", d1-d7|a0-a3|a6);
    KEEPS("switch init", d2-d7|a2|a3);
    KEEPS("OwnBlitter", d0-d7|a0-a3|a6);
    KEEPS("DisownBlitter", d0-d7|a0-a3|a6);
    KEEPS("goqamigaS", d1-d7|a0-a3|a6);
    KEEPS("goamiga", d1-d7|a0-a3|a6);
    KEEPS("goqamiga", d1-d7|a0-a3|a6);
    KEEPS("setcoplist", d0-d7|a0-a3|a6);
    KEEPS("switch finit", d2-d7|a2|a3);
    KEEPS("runerr init", d3-d7|a1-a3);
    KEEPS("getvbr", d0-d7|a1-a3|a6);
    KEEPS("getcpu", d1-d7|a0-a3|a6);
    KEEPS("runerr 22", d0-d7|a0-a3|a6);
    KEEPS("runerr 23", d0-d7|a0-a3|a6);
    KEEPS("runerr 24", d0-d7|a0-a3|a6);
    KEEPS("runerr 25", d0-d7|a0-a3|a6);
    KEEPS("runerr 26", d0-d7|a0-a3|a6);
    KEEPS("runerr 27", d0-d7|a0-a3|a6);
    KEEPS("runerr 28", d0-d7|a0-a3|a6);
    KEEPS("runerr 14", d0-d7|a0-a3|a6);
    KEEPS("runerr 15", d0-d7|a0-a3|a6);
    KEEPS("datachk", d0|d2-d7|a0-a2|a6);
    KEEPS("runerr 21", d0-d7|a0-a3|a6);
    KEEPS("gosuper", d0-d7|a0-a3|a6);
    KEEPS("stackchk", d0-d7|a0-a3|a6);
    KEEPS("gouser", d0-d7|a0-a3|a6);
    KEEPS("runerr finit", d0-d7|a1-a3|a6);
    KEEPS("debug init", d0-d7|a1-a3|a6);
    KEEPS("dosethandler", d1-d7|a3|a6);
    KEEPS("dostop", d0-d7|a0-a3|a6);
    KEEPS("doevaluate", d0-d7|a0-a3|a6);
    KEEPS("doexecute", d0-d7|a0-a3|a6);
    KEEPS("doquit", d0-d7|a0-a3|a6);
    KEEPS("doskip", d0-d7|a0-a3|a6);
    KEEPS("donewmode", d0-d7|a0-a3|a6);
    KEEPS("docall", d0|d2-d7|a0|a2|a3|a6);
    KEEPS("dodetoke", d2-d7|a3|a6);
    KEEPS("dofindoff", d4-d7|a1|a3|a6);
    KEEPS("debug finit", d0-d7|a0-a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    run(&loaded);

    OBS("number of calls", ncalls);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    CloseLibrary(MathBase);
    CloseLibrary((struct Library *)GfxBase);
    return failures ? 5 : 0;
}
