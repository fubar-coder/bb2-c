/*
 * systest - functional comparison of the system libraries: original
 * (blitz2:acidlibs) against the C build, on the Amiga.
 *
 *   systest DIR
 *
 * DIR contains NAME.orig (original) and NAME.obj (C build) for every
 * library below.  Each set is loaded like the compiler does it (relocate,
 * resolve ALibJsr placeholders against the other libraries of the same
 * set), then the same scenarios run on both sets and their observable
 * results are compared:
 *   - results that do not depend on addresses (values, flags, contents),
 *   - every register the original preserves must be preserved by the C
 *     build too (the C build may preserve more),
 *   - memory balance (AvailMem) after alloc/free scenarios.
 * Routines that are assembler in the C build too (exitslib, clrlib's
 * thunk prefix ...) are compared byte for byte instead.
 */
#include "sysload.h"

/* ------------------------------------------------------------- scenarios */

static ULONG gvars[4096];           /* fake globals (a5 region) */

struct result {                     /* per set */
    struct call c[24];
    ULONG v[24];
};

static void run(struct set *s, struct result *res)
{
    struct lib *l;
    struct regs r;
    UBYTE *code;
    int k = 0, vi = 0;

#define CALL(fn, setup, arg, push) do {                                     \
        markregs(&r); setup;                                                \
        res->c[k].in = r;                                                   \
        res->c[k].ccr = callregs(fn, &r, arg, push);                        \
        res->c[k].out = r; k++; } while (0)

    /* datalib: init(d0) then getstart */
    l = getlib(s, "datalib");
    CALL(entry_code(l, 1), r.d[0] = 0x12345678, 0, 0);
    res->v[vi++] = r.d[0];
    CALL(entry_code(l, 0), (void)0, 0, 0);
    res->v[vi++] = r.d[0];

    /* chipbaselib: inline routine, execute behind the $A000 prefix */
    l = getlib(s, "chipbaselib");
    code = entry_code(l, 0);
    res->v[vi++] = W(code) == 0xa000 ? W(code + 2) : 0xffff;
    CALL(code + 4, (void)0, 0, 0);
    res->v[vi++] = r.d[0];

    /* memlib: init, local/global alloc+free, newmem/freelast */
    l = getlib(s, "memlib");
    {
        ULONG avail = AvailMem(MEMF_ANY), membase, m1, m2, g1;
        CALL(entry_code(l, 7), (void)0, 0, 0);          /* init */
        membase = r.d[0];
        res->v[vi++] = membase != 0;
        CALL(entry_code(l, 0), r.d[0] = 100; r.d[1] = MEMF_PUBLIC | MEMF_CLEAR, 0, 0);
        m1 = r.d[0];
        res->v[vi++] = m1 != 0 && ((ULONG *)m1)[0] == 0 && ((ULONG *)m1)[24] == 0;
        CALL((APTR)membase, r.d[0] = 50; r.d[1] = MEMF_PUBLIC, 0, 0); /* via table */
        m2 = r.d[0];
        res->v[vi++] = m2 != 0;
        CALL(entry_code(l, 4), (void)0, 0, 0);          /* newmem */
        CALL(entry_code(l, 2), r.d[0] = 64; r.d[1] = MEMF_PUBLIC, 0, 0);  /* globalloc */
        g1 = r.d[0];
        CALL(entry_code(l, 0), r.d[0] = 10; r.d[1] = MEMF_PUBLIC, 0, 0);  /* local in new list */
        CALL(entry_code(l, 5), (void)0, 0, 0);          /* freelast: frees it */
        res->v[vi++] = !(res->c[k - 1].ccr & CCR_Z);    /* there was a list */
        CALL(entry_code(l, 1), r.a[1] = m1; r.d[0] = 100, 0, 0);          /* locfree */
        CALL(entry_code(l, 3), r.a[1] = g1, 0, 0);      /* globfree */
        CALL(entry_code(l, 1), r.a[1] = m2, 0, 0);      /* locfree */
        CALL(entry_code(l, 5), (void)0, 0, 0);          /* freelast: global list */
        res->v[vi++] = !(res->c[k - 1].ccr & CCR_Z);
        CALL(entry_code(l, 5), (void)0, 0, 0);          /* none left */
        res->v[vi++] = !!(res->c[k - 1].ccr & CCR_Z);
        res->v[vi++] = AvailMem(MEMF_ANY) == avail;     /* all memory back */
        CALL(entry_code(l, 7), (void)0, 0, 0);          /* init again for the others */
    }

    /* strings1lib: init allocates the buffer (memlib globalloc) */
    l = getlib(s, "strings1lib");
    {
        static const UBYTE text[] = "Hello, Blitz Basic 2 string copy test!";
        UBYTE *buf;
        CALL(entry_code(l, 2), r.d[0] = 256, 0, 0);     /* init */
        buf = (UBYTE *)r.d[0];
        res->v[vi++] = buf != NULL;
        CALL(entry_code(l, 0), r.a[0] = (ULONG)text; r.a[3] = (ULONG)buf, sizeof text - 1, 1);
        res->v[vi++] = r.a[3] - (ULONG)buf;             /* a3 advanced by len */
        res->v[vi++] = memcmp(buf, text, sizeof text) == 0;
        CALL(entry_code(l, 0), r.a[0] = (ULONG)text; r.a[3] = (ULONG)buf, 3, 1);
        res->v[vi++] = r.a[3] - (ULONG)buf;
        res->v[vi++] = buf[3] == 0 && buf[0] == 'H';
    }

    /* staticslib: allocate 3 statics into fake globals / locals */
    l = getlib(s, "staticslib");
    {
        static UWORD tab[] = { 0, 40, 4, 8, (UWORD)-8, 12 };
        UBYTE *g = (UBYTE *)gvars + 0x2000;
        memset(gvars, 0, sizeof gvars);
        CALL(entry_code(l, 0), r.a[2] = (ULONG)tab; r.d[4] = 2; r.a[5] = (ULONG)g, 0, 0);
        res->v[vi++] = *(ULONG *)(g + 0) && *(ULONG *)(g + 4) && *(ULONG *)(g - 8);
        res->v[vi++] = ((UBYTE *)*(ULONG *)(g + 0))[39] == 0;
        memset(gvars, 0, sizeof gvars);
        CALL(entry_code(l, 1), r.a[2] = (ULONG)tab; r.d[4] = 2; r.a[4] = (ULONG)g, 0, 0);
        res->v[vi++] = *(ULONG *)(g + 0) && *(ULONG *)(g + 4) && *(ULONG *)(g - 8);
    }

    /* varslib: allocate globals, a5 = block+$8000, free */
    l = getlib(s, "varslib");
    {
        ULONG avail = AvailMem(MEMF_ANY);
        CALL(entry_code(l, 0), r.d[0] = 2000, 0, 0);
        res->v[vi++] = r.a[5] == r.d[0] && r.d[0] != 0;
        res->v[vi++] = *(ULONG *)(r.a[5] - 0x8000) == 0;   /* cleared */
        CALL(entry_code(l, 1), r.d[0] = 2000; r.a[5] = res->c[k - 1].out.a[5], 0, 0);
        res->v[vi++] = AvailMem(MEMF_ANY) == avail;
    }

    /* memlib finit: free everything */
    l = getlib(s, "memlib");
    CALL(entry_code(l, 8), (void)0, 0, 0);
#undef CALL
    res->v[23] = k;                 /* number of calls */
}

static const char *names[] = {
    "memlib", "datalib", "chipbaselib", "strings1lib", "staticslib", "varslib",
    "exitslib", "clrlib", NULL
};

int main(int argc, char **argv)
{
    static struct result res[2];
    int s, i;

    if (argc < 2) {
        printf("usage: systest DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++)
        run(&sets[s], &res[s]);

    /* compare results */
    for (i = 0; i < 23; i++) {
        char buf[80];
        sprintf(buf, "value %d: orig $%08lX, C $%08lX", i, res[0].v[i], res[1].v[i]);
        check(buf, res[0].v[i] == res[1].v[i]);
    }
    check("number of calls", res[0].v[23] == res[1].v[23]);
    for (i = 0; i < (int)res[0].v[23]; i++) {
        char buf[40];
        sprintf(buf, "call %d", i);
        cmp_preserve(buf, &res[0].c[i], &res[1].c[i]);
    }

    /* assembler routines: same bytes as the original */
    {
        struct lib *o = getlib(&sets[0], "exitslib"), *c = getlib(&sets[1], "exitslib");
        UBYTE *qo = entry_code(o, 0), *qc = entry_code(c, 0);
        /* opcode words; the pc-relative displacements depend on layout */
        check("exitslib quit: same instructions",
              W(qo) == W(qc) && W(qo + 4) == W(qc + 4) && W(qo + 6) == W(qc + 6) &&
              W(qo + 10) == W(qc + 10));
    }

    printf("%d checks, %d failures\n", checks, failures);
    for (i = 0; i < 23; i++)
        printf("  v%-2d $%08lX\n", i, res[0].v[i]);
    return failures ? 5 : 0;
}
