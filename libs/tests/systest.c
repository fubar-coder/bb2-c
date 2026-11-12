/*
 * systest - the system libraries memlib, datalib, chipbaselib,
 * strings1lib, staticslib, varslib, exitslib, clrlib on the Amiga: one set
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
 *   - memory balance (AvailMem) after alloc/free scenarios.
 * Routines that are assembler in the C build too (exitslib) are observed
 * by their opcode words.
 */
#include "sysload.h"

/* ------------------------------------------------------------- scenarios */

static ULONG gvars[4096];           /* fake globals (a5 region) */

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
        ULONG avail = AvailMem(MEMF_ANY), membase, m1, m2, g1;
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
        OBS("all memory back", AvailMem(MEMF_ANY) == avail);
        CALL("memlib init", entry_code(l, 7), (void)0, 0, 0);         /* again for the others */
    }

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
    }

    /* varslib: allocate globals, a5 = block+$8000, free */
    l = getlib(s, "varslib");
    {
        ULONG avail = AvailMem(MEMF_ANY), a5;
        DONTCARE = 1UL << 13;                           /* a5: the result */
        CALL("varslib init", entry_code(l, 0), r.d[0] = 2000, 0, 0);
        DONTCARE = 0;
        a5 = r.a[5];
        OBS("a5 = d0", r.a[5] == r.d[0] && r.d[0] != 0);
        OBS("cleared", *(ULONG *)(r.a[5] - 0x8000) == 0);
        CALL("varslib finit", entry_code(l, 1), r.d[0] = 2000; r.a[5] = a5, 0, 0);
        OBS("all memory back", AvailMem(MEMF_ANY) == avail);
    }

    /* memlib finit: free everything */
    l = getlib(s, "memlib");
    CALL("memlib finit", entry_code(l, 8), (void)0, 0, 0);
#undef CALL
    ROUT = "";
    OBS("number of calls", k);
}

static const char *names[] = {
    "memlib", "datalib", "chipbaselib", "strings1lib", "staticslib", "varslib",
    "exitslib", "clrlib", NULL
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: systest DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("datalib init", d0-d7|a0-a3|a6);
    KEEPS("datalib getdstart", d1-d7|a0-a3|a6);
    KEEPS("chipbaselib", d1-d7|a0-a3|a6);
    KEEPS("memlib init", d1-d7|a0-a3|a6);
    KEEPS("memlib localloc", d2-d7|a0-a3|a6);
    KEEPS("memlib localloc (table)", d2-d7|a0-a3|a6);
    KEEPS("memlib newmem", d0-d7|a0-a3|a6);
    KEEPS("memlib globalloc", d2-d7|a0-a3|a6);
    KEEPS("memlib freelast", d0-d7|a0-a3|a6);
    KEEPS("memlib locfree", d1-d7|a0|a2|a3|a6);
    KEEPS("memlib globfree", d1-d7|a0|a2|a3|a6);
    KEEPS("memlib finit", d0-d7|a0-a3|a6);
    KEEPS("strings1lib init", d2-d7|a0-a3|a6);
    KEEPS("strings1lib copy", d0-d7|a1|a2|a6);
    KEEPS("staticslib globals", d3|d5-d7|a0|a1|a3|a6);
    KEEPS("staticslib locals", d0|d1|d3|d5-d7|a0|a1|a3|a6);
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
