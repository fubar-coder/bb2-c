/*
 * systest - the system libraries memlib, datalib, chipbaselib,
 * strings1lib, staticslib, varslib, exitslib, clrlib (and banklib AllocMem,
 * which calls memlib) on the Amiga: one set
 * (the original from blitz2:acidlibs or the C build, as LIBSET says),
 * observed as cases (sysload.h; the expected data
 * libs/tests/expected/systest.txt).
 *
 *   systest DIR
 *
 * DIR contains NAME.orig (original) and NAME.obj (C build) for every
 * library below.  The set is loaded like the compiler does it (relocate,
 * resolve ALibJsr placeholders against the other libraries of the same
 * set), then the scenarios run and their observable results are written:
 *   - results that do not depend on addresses (values, flags, contents),
 *   - the kept registers of every call (KEEPS: the set the original keeps
 *     in all calls of the routine),
 *   - memory balance (what this task allocated and freed, rwemem.h) after alloc/free scenarios.
 * Routines that are assembler in the C build too (exitslib) are observed
 * by their opcode words.
 */
#include "sysload.h"
#include "rwe/rwemem.h"

/* ------------------------------------------------------------- scenarios */

/* clrloc in a procedure's frame: the stub does LINK a4,#-N, fills the
   frame with $55555555, calls clrfn (JSR, the frame right above the
   return address), copies the frame to clrframe and does UNLK a4; a
   clrloc that cleared the saved a4 above the frame loses a4 */
APTR clrfn;
ULONG clrframe[8];
static int zeros(const ULONG *p, int n)
{
    while (n--)
        if (*p++)
            return 0;
    return 1;
}
void clrstub4(void), clrstub32(void);
__asm__(
"   .text\n"
"   .even\n"
"_clrstub4:\n"
"   link    a4,#-4\n"
"   move.l  #0x55555555,-4(a4)\n"
"   pea     1f\n"
"   move.l  _clrfn,-(sp)\n"
"   rts\n"
"1: move.l  -4(a4),_clrframe\n"
"   unlk    a4\n"
"   rts\n"
"_clrstub32:\n"
"   link    a4,#-32\n"
"   move.l  #0x55555555,-32(a4)\n"
"   move.l  #0x55555555,-28(a4)\n"
"   move.l  #0x55555555,-24(a4)\n"
"   move.l  #0x55555555,-20(a4)\n"
"   move.l  #0x55555555,-16(a4)\n"
"   move.l  #0x55555555,-12(a4)\n"
"   move.l  #0x55555555,-8(a4)\n"
"   move.l  #0x55555555,-4(a4)\n"
"   pea     2f\n"
"   move.l  _clrfn,-(sp)\n"
"   rts\n"
"2: move.l  -32(a4),_clrframe\n"
"   move.l  -28(a4),_clrframe+4\n"
"   move.l  -24(a4),_clrframe+8\n"
"   move.l  -20(a4),_clrframe+12\n"
"   move.l  -16(a4),_clrframe+16\n"
"   move.l  -12(a4),_clrframe+20\n"
"   move.l  -8(a4),_clrframe+24\n"
"   move.l  -4(a4),_clrframe+28\n"
"   unlk    a4\n"
"   rts\n"
);

static ULONG gvars[4096];           /* fake globals (a5 region) */
static UBYTE cpsrc[70016], cpbuf[70016];    /* strings1lib copy */

static void run(struct set *s)
{
    struct lib *l;
    struct regs r, in;
    UBYTE *code;
    UWORD ccr = 0;
    int k = 0;

    /* one call of the routine `name`: its kept case */
#define CALL(name, fn, setup, arg, push) do {                               \
        ROUT = (name);                                                      \
        markregs(&r); setup;                                                \
        in = r;                                                             \
        ccr = callregs(fn, &r, arg, push);                                  \
        obs_kept(&in, &r); k++; } while (0)

    /* datalib: init(d0) then getstart */
    l = getlib(s, "datalib");
    CALL("datalib init", entry_code(l, 1), r.d[0] = 0x12345678, 0, 0);
    OBS("d0", r.d[0]);
    CALL("datalib getdstart", entry_code(l, 0), (void)0, 0, 0);
    OBS("d0", r.d[0]);

    /* chipbaselib: inline routine, execute behind the $A000 prefix */
    l = getlib(s, "chipbaselib");
    code = entry_code(l, 0);
    ROUT = "chipbaselib";
    OBS("inline length", W(code) == 0xa000 ? W(code + 2) : 0xffff);
    CALL("chipbaselib", code + 4, (void)0, 0, 0);
    OBS("d0", r.d[0]);

    /* memlib: init, local/global alloc+free, newmem/freelast */
    l = getlib(s, "memlib");
    {
        ULONG membase, m1, m2, g1;
        rwe_mem_start();
        CALL("memlib init", entry_code(l, 7), (void)0, 0, 0);
        membase = r.d[0];
        OBS("membase set", membase != 0);
        CALL("memlib localloc", entry_code(l, 0), r.d[0] = 100; r.d[1] = MEMF_PUBLIC | MEMF_CLEAR,
             0, 0);
        m1 = r.d[0];
        OBS("cleared memory", m1 != 0 && ((ULONG *)m1)[0] == 0 && ((ULONG *)m1)[24] == 0);
        CALL("memlib localloc (table)", (APTR)membase, r.d[0] = 50; r.d[1] = MEMF_PUBLIC, 0, 0);
        m2 = r.d[0];
        OBS("memory", m2 != 0);
        CALL("memlib newmem", entry_code(l, 4), (void)0, 0, 0);
        CALL("memlib globalloc", entry_code(l, 2), r.d[0] = 64; r.d[1] = MEMF_PUBLIC, 0, 0);
        g1 = r.d[0];
        CALL("memlib localloc", entry_code(l, 0), r.d[0] = 10; r.d[1] = MEMF_PUBLIC, 0, 0);
        CALL("memlib freelast", entry_code(l, 5), (void)0, 0, 0);     /* frees it */
        OBS("a list (not Z)", !(ccr & CCR_Z));
        CALL("memlib locfree", entry_code(l, 1), r.a[1] = m1; r.d[0] = 100, 0, 0);
        CALL("memlib globfree", entry_code(l, 3), r.a[1] = g1, 0, 0);
        CALL("memlib locfree", entry_code(l, 1), r.a[1] = m2, 0, 0);
        CALL("memlib freelast", entry_code(l, 5), (void)0, 0, 0);     /* global list */
        OBS("a list (not Z)", !(ccr & CCR_Z));
        CALL("memlib freelast", entry_code(l, 5), (void)0, 0, 0);     /* none left */
        OBS("no list (Z)", !!(ccr & CCR_Z));
        OBS("all memory back", rwe_mem_back());
        CALL("memlib init", entry_code(l, 7), (void)0, 0, 0);         /* again for the others */
        /* the flags after a successful allocation, with the caller's d2 = 0:
           Z clear, as third-party libraries test them with `beq` right
           after the call (#1059; freed by memlib finit) */
        CALL("memlib globalloc (d2 = 0)", entry_code(l, 2), r.d[0] = 16; r.d[1] = MEMF_PUBLIC;
             r.d[2] = 0, 0, 0);
        OBS("memory", r.d[0] != 0);
        OBS("not Z", !(ccr & CCR_Z));
        CALL("memlib localloc (d2 = 0)", entry_code(l, 0), r.d[0] = 16; r.d[1] = MEMF_PUBLIC;
             r.d[2] = 0, 0, 0);
        OBS("memory", r.d[0] != 0);
        OBS("not Z", !(ccr & CCR_Z));
    }

    /* banklib AllocMem: memlib globalloc, its flags too (#1059) */
    l = getlib(s, "banklib");
    CALL("banklib AllocMem (d2 = 0)", entry_code(l, 7), r.d[0] = 16; r.d[1] = MEMF_PUBLIC;
         r.d[2] = 0, 0, 0);
    OBS("memory", r.d[0] != 0);
    OBS("not Z", !(ccr & CCR_Z));

    /* strings1lib: init allocates the buffer (memlib globalloc) */
    l = getlib(s, "strings1lib");
    {
        static const UBYTE text[] = "Hello, Blitz Basic 2 string copy test!";
        UBYTE *buf;
        CALL("strings1lib init", entry_code(l, 2), r.d[0] = 256, 0, 0);
        buf = (UBYTE *)r.d[0];
        OBS("buffer", buf != NULL);
        sprintf(VEC, "len %lu", (ULONG)sizeof text - 1);
        CALL("strings1lib copy", entry_code(l, 0), r.a[0] = (ULONG)text; r.a[3] = (ULONG)buf,
             sizeof text - 1, 1);
        OBS("a3 advanced", r.a[3] - (ULONG)buf);
        OBS("copied", memcmp(buf, text, sizeof text) == 0);
        strcpy(VEC, "len 3");
        CALL("strings1lib copy", entry_code(l, 0), r.a[0] = (ULONG)text; r.a[3] = (ULONG)buf, 3, 1);
        OBS("a3 advanced", r.a[3] - (ULONG)buf);
        OBS("copied", buf[3] == 0 && buf[0] == 'H');
        VEC[0] = 0;
    }
    /* the edges of the copy loop (8 bytes per pass, then the rest): no
       character, one, an odd length below 8, whole passes, passes with a
       rest, a length above 16 bits */
    {
        static const ULONG lens[] = { 0, 1, 7, 8, 9, 15, 16, 17, 23, 64, 255, 70007 };
        unsigned i, j;
        for (j = 0; j < sizeof cpsrc; j++)
            cpsrc[j] = (UBYTE)(j * 7 + 3);
        for (i = 0; i < sizeof lens / sizeof lens[0]; i++) {
            ULONG n = lens[i];
            memset(cpbuf, 0x55, n + 8);
            sprintf(VEC, "len %lu", n);
            CALL("strings1lib copy", entry_code(l, 0), r.a[0] = (ULONG)cpsrc + 1;
                 r.a[3] = (ULONG)cpbuf + 2, n, 1);
            OBS("a3 advanced", r.a[3] - (ULONG)cpbuf - 2);
            OBS("copied", memcmp(cpbuf + 2, cpsrc + 1, n) == 0 && cpbuf[2 + n] == 0);
            OBS("untouched", cpbuf[0] == 0x55 && cpbuf[1] == 0x55 && cpbuf[3 + n] == 0x55);
        }
        VEC[0] = 0;
    }

    /* staticslib: allocate 3 statics into fake globals / locals */
    l = getlib(s, "staticslib");
    {
        static UWORD tab[] = { 0, 40, 4, 8, (UWORD)-8, 12 };
        UBYTE *g = (UBYTE *)gvars + 0x2000;
        memset(gvars, 0, sizeof gvars);
        CALL("staticslib globals", entry_code(l, 0), r.a[2] = (ULONG)tab; r.d[4] = 2;
             r.a[5] = (ULONG)g, 0, 0);
        OBS("three statics", *(ULONG *)(g + 0) && *(ULONG *)(g + 4) && *(ULONG *)(g - 8));
        OBS("cleared", ((UBYTE *)*(ULONG *)(g + 0))[39] == 0);
        memset(gvars, 0, sizeof gvars);
        CALL("staticslib locals", entry_code(l, 1), r.a[2] = (ULONG)tab; r.d[4] = 2;
             r.a[4] = (ULONG)g, 0, 0);
        OBS("three statics", *(ULONG *)(g + 0) && *(ULONG *)(g + 4) && *(ULONG *)(g - 8));
        /* the parameters of the procedure are in d0-d5: the original
           changes d2 and d4 (#1063) */
        OBS("d2 kept", r.d[2] == in.d[2]);
        OBS("d4 kept", r.d[4] == in.d[4]);
    }

    /* varslib: allocate globals, a5 = block+$8000, free */
    l = getlib(s, "varslib");
    {
        ULONG a5;
        rwe_mem_start();
        DONTCARE = 1UL << 13;                           /* a5: the result */
        CALL("varslib init", entry_code(l, 0), r.d[0] = 2000, 0, 0);
        DONTCARE = 0;
        a5 = r.a[5];
        OBS("a5 = d0", r.a[5] == r.d[0] && r.d[0] != 0);
        OBS("cleared", *(ULONG *)(r.a[5] - 0x8000) == 0);
        CALL("varslib finit", entry_code(l, 1), r.d[0] = 2000; r.a[5] = a5, 0, 0);
        OBS("all memory back", rwe_mem_back());
    }

    /* clrlib: clear a frame of 1 and of 8 longs */
    l = getlib(s, "clrlib");
    memset(clrframe, 0xaa, sizeof clrframe);
    CALL("clrlib clrloc", (APTR)clrstub4, clrfn = entry_code(l, 0), 0, 0);
    OBS("cleared 1", clrframe[0] == 0 && clrframe[1] == 0xaaaaaaaa);
    memset(clrframe, 0xaa, sizeof clrframe);
    CALL("clrlib clrloc", (APTR)clrstub32, clrfn = entry_code(l, 0), 0, 0);
    OBS("cleared 8", zeros(clrframe, 8));

    /* memlib finit: free everything */
    l = getlib(s, "memlib");
    CALL("memlib finit", entry_code(l, 8), (void)0, 0, 0);
#undef CALL
    ROUT = "";
    OBS("number of calls", k);
}

static const char *names[] = {
    "memlib", "datalib", "chipbaselib", "strings1lib", "staticslib", "varslib",
    "exitslib", "clrlib", "banklib", NULL
};

/* banklib's LoadBank calls fileiolib Exists, which this set does not load;
   the scenario only calls AllocMem, so the stub is never run */
static void stub_exists(void)
{
}

static const struct stub stubs[] = {
    { 0x3e15, (APTR)stub_exists },      /* fileiolib (124) Exists, TOK_exists */
    { 0, NULL }
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: systest DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("datalib init", d0-d7|a0-a3|a6);
    KEEPS("datalib getdstart", d1-d7|a0-a3|a6);
    KEEPS("chipbaselib", d1-d7|a0-a3|a6);
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("memlib localloc", d2-d7|a0-a3|a6);
    KEEPS("memlib localloc (table)", d2-d7|a0-a3|a6);
    /* compiler: newmem in the procedure prologue, with the parameters in
       d0-d5 (perf-analysis thunks.md 1.4) */
    KEEPS("memlib newmem", d0-d7|a2|a3|a6);
    /* third party: a0 and a1 across globalloc (RIGTMenuLib, QuickLib,
       EFMUILib, ElmoreIncLib, chunkylib and others), d2-d7/a2-a6
       (analyse-internal-token-callers.md) */
    KEEPS("memlib globalloc", d2-d7|a0-a3|a6);
    /* third party: a0 and a1 across globalloc (RIGTMenuLib, QuickLib,
       EFMUILib and others), the contract the port keeps for those callers */
    KEEPS("memlib globalloc (d2 = 0)", a0-a1);
    KEEPS("memlib localloc (d2 = 0)", a0-a1);
    /* banklib AllocMem keeps a0/a1 like globalloc */
    KEEPS("banklib AllocMem (d2 = 0)", a0-a1);
    /* compiler: freelast at the end of a procedure, d0 = the result of
       a Function; its Z flag is observed below */
    KEEPS("memlib freelast", d0|d2-d7|a2|a3|a6);
    KEEPS("memlib locfree", d1-d7|a0|a2|a3|a6);
    /* third party: d1 (GKDBaseLib, DBaseLib, Mildred) and a0 (Mildred,
       ElmoreIncLib, RIARexxLib, QuickLib) across globfree */
    KEEPS("memlib globfree", d1-d7|a0|a2|a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    KEEPS("strings1lib init", d2-d7|a0-a3|a6);
    KEEPS("strings1lib copy", d0-d7|a2|a6);
    /* compiler: clrloc in the procedure prologue, with the parameters in
       d0-d5 */
    KEEPS("clrlib clrloc", d0-d7|a2|a3|a6);
    KEEPS("staticslib globals", d3|d5-d7|a0|a1|a3|a6);
    /* compiler: localstat in the procedure prologue, with the parameters
       in d0-d5 (d4 pushed around it by the compiler); the original
       changes d2 and d4 (#1063), so they are not stated: the cases
       "d2 kept" and "d4 kept" carry them */
    KEEPS("staticslib locals", d0|d1|d3|d5-d7|a3|a6);
    KEEPS("varslib init", d2-d7|a0|a2|a3);
    KEEPS("varslib finit", d0|d2-d7|a2|a3);
    run(&loaded);

    /* assembler routines: the opcode words (the pc-relative displacements
       depend on the layout) */
    {
        UBYTE *q = entry_code(getlib(&loaded, "exitslib"), 0);
        UWORD w[4];
        w[0] = W(q);
        w[1] = W(q + 4);
        w[2] = W(q + 6);
        w[3] = W(q + 10);
        ROUT = "exitslib quit";
        OBSBYTES("instructions", w, sizeof w);
    }

    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
