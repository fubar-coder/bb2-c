/*
 * systest_runtime - functional comparison of the "runtime" group:
 * arrayslib, maxslib, errtraplib, rundebuglib (original vs C build).
 *
 *   systest_runtime DIR
 *
 * DIR contains NAME.orig and NAME.obj of memlib, exitslib (dependencies:
 * memlib for arrayslib/maxslib, exitslib for rundebuglib's End) and of the
 * group's libraries.  runerrlib's getvbr ($C31D, used by rundebuglib) is
 * a stub that returns the real VBR.
 *
 * Every scenario runs on both sets; compared are results that do not
 * depend on addresses and the register preservation of every call.
 *
 * Not run: errtraplib dofail with a pending error and the trap without a
 * SetErr handler (both give the trap to the system's TRAP #0 vector),
 * rundebuglib TRAP #0 (requester + End) - for these only the first
 * instruction words of the handlers are compared.
 */
#include "sysload.h"
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>

/* --------------------------------------------------------- asm helpers */

/* stub for runerrlib getvbr: a0 = VBR */
ULONG realvbr;
extern char stub_getvbr[];
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _stub_getvbr\n"
"_stub_getvbr:\n"
"   move.l  _realvbr,a0\n"
"   rts\n"
);

static const struct stub stubs[] = {
    { 0xc31d, stub_getvbr },
    { 0, NULL }
};

/* trytrap(dosettrap, msg, num): SetErr with the label behind the TRAP,
   then TRAP #0 with d0 = msg, d1 = num.  1 = the handler got it */
extern LONG trytrap(APTR settrap, ULONG msg, ULONG num);
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _trytrap\n"
"_trytrap:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"        /* 44: settrap 48, msg 52, num 56 */
"   move.l  48(sp),a0\n"
"   move.l  #9f,d0\n"
"   jsr     (a0)\n"
"   move.l  52(sp),d0\n"
"   move.l  56(sp),d1\n"
"   trap    #0\n"
"   moveq   #-1,d0\n"
"   bra.s   8f\n"
"9: moveq   #1,d0\n"
"8: movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);

/* trapN(): TRAP #N followed by 8 x ADDQ #1,d2 (2 bytes each):
   result = 8 - skipped bytes / 2 */
#define ADDQ8 "   addq.w  #1,d2\n   addq.w  #1,d2\n   addq.w  #1,d2\n   addq.w  #1,d2\n" \
              "   addq.w  #1,d2\n   addq.w  #1,d2\n   addq.w  #1,d2\n   addq.w  #1,d2\n"
#define TRAPFN(n)                                                           \
    "   .globl  _trap" #n "\n_trap" #n ":\n"                                \
    "   move.l  d2,-(sp)\n   moveq   #0,d2\n   trap    #" #n "\n" ADDQ8     \
    "   move.l  d2,d0\n   move.l  (sp)+,d2\n   rts\n"
extern ULONG trap1(void), trap2(void), trap3(void), trap4(void), trap5(void);
__asm__("   .text\n   .even\n" TRAPFN(1) TRAPFN(2) TRAPFN(3) TRAPFN(4) TRAPFN(5));

static ULONG readvbr(void)
{
    ULONG v = 0;
    if (SysBase->AttnFlags & AFF_68010) {
        APTR ssp = SuperState();
        register ULONG r __asm("d2");
        __asm volatile(".short 0x4e7a,0x2801" : "=d"(r));  /* MOVEC vbr,d2 */
        v = r;
        UserState(ssp);
    }
    return v;
}

/* ------------------------------------------------------------- scenarios */

static int verbose;
static ULONG gvars[4096], lvars[4096];  /* fake globals (a5) / locals (a4) */

#define MAXC 96
#define MAXV 192

struct result {                         /* per set */
    struct call c[MAXC];
    const char *cname[MAXC];
    ULONG v[MAXV];
    const char *vname[MAXV];
    int nc, nv;
};

static BOOL inlib(struct lib *l, ULONG p)
{
    return p >= (ULONG)l->code && p < (ULONG)l->code + l->size;
}

static BOOL zero(ULONG p, ULONG n)
{
    ULONG i;
    for (i = 0; i < n; i++)
        if (((UBYTE *)p)[i])
            return FALSE;
    return TRUE;
}

/* the free list of a list made by newlist: offsets from the header */
static ULONG listsig(UBYTE *h)
{
    ULONG sum = 0, n = 0, prev = (ULONG)(h + 16);
    ULONG *node = *(ULONG **)(h + 16);
    BOOL links = TRUE;
    while (node && node[0]) {           /* the tail node (h+20) has next 0 */
        links &= node[1] == prev;
        sum = sum * 31 + ((ULONG)node - (ULONG)h);
        prev = (ULONG)node;
        node = (ULONG *)node[0];
        n++;
        if (n > 1000)
            break;
    }
    return links ? (n << 24) ^ sum : 0xdead0000 | n;
}

static void run(struct set *s, struct result *res)
{
    struct lib *l, *mem = getlib(s, "memlib");
    struct regs r;
    UBYTE *g = (UBYTE *)gvars + 0x2000, *lo = (UBYTE *)lvars + 0x2000;
    ULONG avail0 = AvailMem(MEMF_ANY), a1;
    int i;

#define CALL(name, fn, setup) do {                                          \
        markregs(&r); r.a[5] = (ULONG)g; r.a[4] = (ULONG)lo; setup;         \
        res->c[res->nc].in = r; if (verbose) printf("%s call %s\n", s->what, name); \
        res->cname[res->nc] = name;                                         \
        res->c[res->nc].ccr = callregs(fn, &r, 0, 0);                       \
        res->c[res->nc].out = r; res->nc++; } while (0)
#define V(name, val) do { if (verbose) printf("%s %s\n", s->what, name); res->vname[res->nv] = name; res->v[res->nv++] = (val); } while (0)

    memset(gvars, 0, sizeof gvars);
    memset(lvars, 0, sizeof lvars);
    CALL("memlib init", entry_code(mem, 7), (void)0);

    /* ---------------- arrayslib */
    l = getlib(s, "arrayslib");
    {
        static const ULONG mul[][2] = {
            { 4, 9 }, { 1, 0 }, { 100, 99 }, { 2, 0xffff }, { 0x12345, 3 },
            { 0x10000, 0x7fff }, { 0x7ffff, 0xfffe }, { 0xffffffff, 1 },
            { 0x00020000, 0x40000 }, { 0xabcd1234, 0x1ff }, { 0, 5 },
            { 0x12345678, 0xffff0003 },
        };
        for (i = 0; i < (int)(sizeof mul / sizeof mul[0]); i++) {
            CALL("arrmult", entry_code(l, 2), r.d[0] = mul[i][0]; r.d[1] = mul[i][1]);
            V("arrmult d0", r.d[0]);
        }

        /* newarr: ptr at +4, size at +8; then again (frees the old one) */
        *(ULONG *)(g + 8) = 100;
        CALL("newarr", entry_code(l, 0), r.d[2] = 4; r.d[3] = 8);
        V("newarr d0 = ptr", r.d[0] && r.d[0] == *(ULONG *)(g + 4));
        V("newarr cleared", zero(*(ULONG *)(g + 4), 100));
        a1 = AvailMem(MEMF_ANY);
        memset((void *)*(ULONG *)(g + 4), 0x55, 100);
        CALL("newarr again", entry_code(l, 0), r.d[2] = 4; r.d[3] = 8);
        V("newarr again cleared", r.d[0] == *(ULONG *)(g + 4) && zero(r.d[0], 100));
        V("newarr again: old one freed", AvailMem(MEMF_ANY) == a1);

        /* newlocarr: negative offsets from a4 */
        *(ULONG *)(lo - 8) = 64;
        CALL("newlocarr", entry_code(l, 1), r.d[2] = 0xfffc; r.d[3] = 0xfff8);
        V("newlocarr d0 = ptr", r.d[0] && r.d[0] == *(ULONG *)(lo - 4));
        V("newlocarr cleared", zero(r.d[0], 64));
        a1 = AvailMem(MEMF_ANY);
        CALL("newlocarr again", entry_code(l, 1), r.d[2] = 0xfffc; r.d[3] = 0xfff8);
        V("newlocarr again: old one freed", AvailMem(MEMF_ANY) == a1);
        V("newlocarr: a5 untouched", *(ULONG *)(g - 4) == 0);

        /* newlist: 10 items of 12 bytes */
        *(ULONG *)(g + 16) = 120;
        CALL("newlist", entry_code(l, 3), r.d[2] = 12; r.d[3] = 16; r.d[4] = 12);
        {
            UBYTE *it = (UBYTE *)*(ULONG *)(g + 12), *h = it - 32;
            ULONG *hl = (ULONG *)h;
            V("newlist hdr 0", hl[0]);
            V("newlist hdr 4", hl[1] - (ULONG)h);
            V("newlist hdr 8", hl[2]);
            V("newlist hdr 12", hl[3] - (ULONG)h);
            V("newlist hdr 16", hl[4] - (ULONG)h);
            V("newlist hdr 20", hl[5]);
            V("newlist hdr 24", hl[6] - (ULONG)h);
            V("newlist id", hl[7]);
            V("newlist free list", listsig(h));
        }
        a1 = AvailMem(MEMF_ANY);
        CALL("newlist again", entry_code(l, 3), r.d[2] = 12; r.d[3] = 16; r.d[4] = 12);
        V("newlist again: old one freed", AvailMem(MEMF_ANY) == a1);
        V("newlist again", listsig((UBYTE *)*(ULONG *)(g + 12) - 32));

        /* newloclist: 40 bytes, items of 10 -> 4 items (the loop links
           an item while it starts below the end and writes 8 bytes of it:
           sizes must leave room for that) */
        *(ULONG *)(lo + 0x20) = 40;
        CALL("newloclist", entry_code(l, 4), r.d[2] = 0xfff0; r.d[3] = 0x20; r.d[4] = 10);
        {
            UBYTE *h = (UBYTE *)*(ULONG *)(lo - 0x10) - 32;
            V("newloclist id", ((ULONG *)h)[7]);
            V("newloclist free list", listsig(h));
            V("newloclist tailpred", ((ULONG *)h)[6] - (ULONG)h);
        }
    }

    /* ---------------- maxslib */
    l = getlib(s, "maxslib");
    {
        static UWORD tab[] = { 0x40, 0, 40, (UWORD)-0x40, 0, 100, 0x44, 0, 8 };
        static const UBYTE text[] = "Blitz Basic 2";
        ULONG p;

        CALL("setmaxs", entry_code(l, 0), r.a[2] = (ULONG)tab; r.d[4] = 2);
        V("setmaxs slots", *(ULONG *)(g + 0x40) && *(ULONG *)(g - 0x40) &&
          *(ULONG *)(g + 0x44));
        V("setmaxs cleared", zero(*(ULONG *)(g + 0x40), 40) &&
          zero(*(ULONG *)(g - 0x40), 100) && zero(*(ULONG *)(g + 0x44), 8));

        a1 = AvailMem(MEMF_ANY);
        CALL("allocst", entry_code(l, 1), r.d[0] = (ULONG)text; r.d[1] = 11);
        p = r.d[0];
        V("allocst length", p ? *(ULONG *)(p - 4) : 0xffffffff);
        V("allocst text", p && memcmp((void *)p, text, 11) == 0 && ((UBYTE *)p)[11] == 0);
        CALL("freest", entry_code(l, 2), r.d[0] = p);
        V("freest: freed", AvailMem(MEMF_ANY) == a1);
        CALL("allocst 0", entry_code(l, 1), r.d[0] = (ULONG)text; r.d[1] = 0);
        V("allocst 0 d0", r.d[0]);
        V("allocst 0: nothing allocated", AvailMem(MEMF_ANY) == a1);
        CALL("freest 0", entry_code(l, 2), r.d[0] = 0);
        V("freest 0 d0", r.d[0]);
    }

    /* ---------------- errtraplib (real TRAP #0 vector) */
    l = getlib(s, "errtraplib");
    {
        ULONG *vbr = (ULONG *)realvbr, trap0 = vbr[32], sp;

        CALL("errtrap init", entry_code(l, 6), (void)0);
        V("errtrap init: TRAP #0 in lib", inlib(l, vbr[32]));
        V("errtrap newtrap first word", W((UBYTE *)vbr[32]));
        CALL("dofail (no error)", entry_code(l, 5), (void)0);
        V("dofail d0", r.d[0]);
        CALL("doreadern", entry_code(l, 0), (void)0);
        V("ern before", r.d[0]);
        V("SetErr + TRAP #0 caught", trytrap(entry_code(l, 1), (ULONG)"error", 0x8123));
        CALL("doreadern", entry_code(l, 0), (void)0);
        V("ern after", r.d[0]);
        V("SetErr + TRAP #0 caught again", trytrap(entry_code(l, 1), (ULONG)"error", 7));
        CALL("doreadern", entry_code(l, 0), (void)0);
        V("ern after 2", r.d[0]);
        CALL("doclrerr", entry_code(l, 2), (void)0);
        CALL("dosettrap", entry_code(l, 1), r.d[0] = 0x1234);
        CALL("doclrerr", entry_code(l, 2), (void)0);
        CALL("dogetsp", entry_code(l, 3), (void)0);
        sp = r.d[0];
        V("dogetsp (same stack depth)", sp);
        V("dogetsp a0", r.a[0] == sp);
        CALL("doputsp", entry_code(l, 4), r.d[0] = sp);
        V("doputsp returned", 1);
        CALL("errtrap finit", entry_code(l, 7), (void)0);
        V("errtrap finit: TRAP #0 restored", vbr[32] == trap0);
    }

    /* ---------------- rundebuglib (real TRAP #0..#5 vectors) */
    l = getlib(s, "rundebuglib");
    {
        ULONG *vbr = (ULONG *)realvbr, old[8];
        struct Library *ib = OpenLibrary((STRPTR)"intuition.library", 0);
        UWORD cnt = ib->lib_OpenCnt;
        BOOL in = TRUE;

        for (i = 0; i < 8; i++)
            old[i] = vbr[31 + i];
        CALL("rundebug init", entry_code(l, 0), (void)0);
        V("rundebug init: intuition open count +", (UWORD)(ib->lib_OpenCnt - cnt));
        for (i = 0; i < 6; i++)
            in &= inlib(l, vbr[32 + i]);
        V("rundebug init: TRAP #0-#5 in lib", in);
        V("rundebug init: others kept", vbr[31] == old[0] && vbr[38] == old[7]);
        for (i = 0; i < 6; i++)
            V("rundebug handler first word", W((UBYTE *)vbr[32 + i]));
        V("TRAP #0 handler 2nd word", W((UBYTE *)vbr[32] + 2));
        V("TRAP #1 (ctrlc 0: skip 8)", trap1());
        V("TRAP #2 (skip 6)", trap2());
        V("TRAP #3", trap3());
        V("TRAP #4 (skip 8)", trap4());
        V("TRAP #5", trap5());
        CALL("rundebug finit", entry_code(l, 1), (void)0);
        in = TRUE;
        for (i = 0; i < 8; i++)
            in &= vbr[31 + i] == old[i];
        V("rundebug finit: vectors restored", in);
        V("rundebug finit: intuition open count +", (UWORD)(ib->lib_OpenCnt - cnt));
        CloseLibrary(ib);
    }

    /* memlib finit: free everything */
    CALL("memlib finit", entry_code(mem, 8), (void)0);
    V("all memory back", AvailMem(MEMF_ANY) == avail0);
#undef CALL
#undef V
}

static const char *names[] = {
    "memlib", "exitslib", "arrayslib", "maxslib", "errtraplib", "rundebuglib", NULL
};

int main(int argc, char **argv)
{
    static struct result res[2];
    int s, i;

    if (argc < 2) {
        printf("usage: systest_runtime DIR [trace]\n");
        return 20;
    }
    verbose = argc > 2;                 /* progress trace for bisecting a crash */
    setvbuf(stdout, NULL, _IONBF, 0);
    realvbr = readvbr();
    printf("vbr $%08lX\n", realvbr);
    sysload_stubs = stubs;
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++)
        run(&sets[s], &res[s]);

    check("number of values", tape_same("number of values", res[0].nv, res[1].nv));
    check("number of calls", tape_same("number of calls", res[0].nc, res[1].nc));
    for (i = 0; i < res[0].nv && i < res[1].nv; i++) {
        char buf[120];
        sprintf(buf, "v%d %s: orig $%08lX, C $%08lX", i, res[0].vname[i],
                res[0].v[i], res[1].v[i]);
        check(buf, tape_same(buf, res[0].v[i], res[1].v[i]));
    }
    for (i = 0; i < res[0].nc && i < res[1].nc; i++) {
        char buf[80];
        sprintf(buf, "call %d %s", i, res[0].cname[i]);
        cmp_preserve(buf, &res[0].c[i], &res[1].c[i]);
    }

    printf("%d checks, %d failures (%d values, %d calls)\n", checks, failures,
           res[0].nv, res[0].nc);
    for (i = 0; i < res[0].nv; i++)
        printf("  v%-3d $%08lX %s\n", i, res[0].v[i], res[0].vname[i]);
    return failures ? 5 : 0;
}
