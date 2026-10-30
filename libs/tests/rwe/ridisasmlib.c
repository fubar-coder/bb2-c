/*
 * rwe ridisasmlib - the C build of the RWE userlib ridisasmlib
 * (libs/userlibs/rwe/ridisasmlib.c) against the reference build of its
 * source rwelibsrc/Disasmlib/Disasm-library.bb2 (ridisasmlib.orig),
 * method of rwecmp.h.
 *
 *   ridisasmlib DIR     (DIR: ridisasmlib .obj/.orig)
 *
 * Both sets disassemble the same code buffer (the same addresses: the
 * PC relative targets are equal), each into its own string area (a3,
 * filled with $AA before every call); compared after every call are d0
 * (34 or 5), how far a3 moved, the string area (40 bytes: the string and
 * what is behind it), InstrSize and the registers the original keeps.
 * Covered: InstrSize before the first call; every first instruction word
 * $0000-$FFFF, twice: with brief extension words behind it and with
 * full format ones (memory indirect, long displacements); the extension
 * words of MOVE.l (bd,a0,xn),(bd,a0,xn) (every source one, every third
 * destination one; the longest lines: 86 bytes, BUG(orig) (#616) - the string
 * is cut after 34 columns) and every second one of MOVE.l (pc,xn),d0 /
 * (bd,pc,xn); MOVEM's register lists (every mask for -(a7), every third
 * for (a7)+ and (a0); BUG(orig) (#615): an empty list still gets a comma);
 * displacements 0 (BUG(orig) (#614): the bytes at address 8 instead of "0");
 * MOVEC with every third control register word (all 4096 codes); an odd
 * instruction address and an odd a3 (the emulator has a 68020; on a
 * 68000 both are address errors, BUG(orig) (#617)); init and finit (RTS).
 * Not covered: the 68000 address errors; the steps leave out some
 * extension words (time: about 380000 calls take 20 s).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ridisasmlib"

enum { E_DIS, E_SIZE, E_FINIT, E_INIT };

#define CODEWORDS 32
#define SAREA 48
static UWORD *code;
static UBYTE *sarea[2];
static ULONG calls;

/* VEC without sprintf (millions of calls): "w0 w1 w2" in hex */
static void setvec(int n)
{
    static const char hex[] = "0123456789ABCDEF";
    char *p = VEC;
    int i, k;

    for (i = 0; i < n; i++) {
        for (k = 12; k >= 0; k -= 4)
            *p++ = hex[(code[i] >> k) & 15];
        *p++ = ' ';
    }
    *p = 0;
}

/* DisAssemble$(code + off) with a3 = string area + aoff, then InstrSize */
static void dis(ULONG off, ULONG aoff)
{
    const UBYTE *s[2];

    calls++;
    for (cur = 0; cur < 2; cur++)
        memset(sarea[cur], 0xaa, SAREA);
    ROUT = "DisAssemble$";
    DO2(ENT(LIB, E_DIS, 0), R[cur].d[0] = (ULONG)code + off;
        R[cur].a[3] = (ULONG)sarea[cur] + aoff);
    SAME("d0", R[cur].d[0]);
    SAME("a3 moved", R[cur].a[3] - (ULONG)sarea[cur]);
    for (cur = 0; cur < 2; cur++)
        s[cur] = sarea[cur];
    if (memcmp(s[0], s[1], 40) && prints < 60)
        printf("  orig \"%.34s\"\n  C    \"%.34s\"\n", (const char *)s[0], (const char *)s[1]);
    SAMEBYTES("string", s, 40);
    ROUT = "InstrSize";
    DO2(ENT(LIB, E_SIZE, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void fill(const UWORD *ext, int n)
{
    int i;

    for (i = 0; i < n && i + 1 < CODEWORDS; i++)
        code[i + 1] = ext[i];
}

/* every first word with the extension words `ext` */
static void allops(const char *what, const UWORD *ext, int n)
{
    ULONG op;

    fill(ext, n);
    for (op = 0; op < 0x10000; op++) {
        code[0] = (UWORD)op;
        setvec(4);
        dis(0, 0);
    }
    printf("%s: %lu calls, %d failures so far\n", what, calls, failures);
}

/* the instruction words op[0..nop-1], word `at` = every value in steps
   of `step` (the others: ext) */
static void allwords(const char *what, const UWORD *op, int nop, int at, ULONG step)
{
    ULONG w;
    int i;

    for (w = 0; w < 0x10000; w += step) {
        for (i = 0; i < nop; i++)
            code[i] = op[i];
        code[at] = (UWORD)w;
        setvec(nop);
        dis(0, 0);
    }
    printf("%s: %lu calls, %d failures so far\n", what, calls, failures);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    /* brief extension words, then numbers */
    static const UWORD brief[] = { 0x8a26, 0x7ff3, 0x9abc, 0xdef0, 0x1357, 0x9bdf, 0x8000,
                                   0x0001, 0xffff, 0x0123, 0x4567, 0x89ab, 0xcdef, 0x0000 };
    /* full format: pre-indexed, long bd and od, negative numbers */
    static const UWORD full[] = { 0xd933, 0x8000, 0x0000, 0xffff, 0xfffe, 0x2d36, 0x8000,
                                  0x0000, 0x8000, 0x0000, 0x7fff, 0xffff, 0x1234, 0x5678 };
    /* MOVE.l (bd,a0,xn),(bd,a0,xn): ext, bd.l, od.l, ext, bd.l, od.l */
    static const UWORD move2[] = { 0x21b0, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
                                   0xd937, 0x8000, 0x0000, 0x8000, 0x0000 };
    static const UWORD pcx[] = { 0x203b, 0x0000, 0xffff, 0xfff0, 0x8000, 0x0000, 0x1234 };
    static const UWORD movem1[] = { 0x48e7, 0x0000 };
    static const UWORD movem2[] = { 0x4cdf, 0x0000 };
    static const UWORD movem3[] = { 0x4890, 0x0000 };
    static const UWORD movec[] = { 0x4e7a, 0x0000 };
    ULONG avail;
    int s;

    if (argc < 2) {
        printf("usage: ridisasmlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    code = AllocMem(CODEWORDS * 2, MEMF_PUBLIC | MEMF_CLEAR);
    for (s = 0; s < 2; s++)
        sarea[s] = AllocMem(SAREA, MEMF_PUBLIC);

    strcpy(VEC, "");
    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    ROUT = "InstrSize (before DisAssemble$)";
    DO2(ENT(LIB, E_SIZE, 0), (void)0);
    SAME("d0", R[cur].d[0]);

    allops("all first words, brief extensions", brief, 14);
    allops("all first words, full format extensions", full, 14);
    allwords("MOVE.l (bd,a0,xn),(bd,a0,xn): source extension", move2, 11, 1, 1);
    allwords("MOVE.l (bd,a0,xn),(bd,a0,xn): destination extension", move2, 11, 6, 3);
    allwords("MOVE.l (pc,xn),d0 forms", pcx, 7, 1, 2);
    allwords("MOVEM -(a7)", movem1, 2, 1, 1);
    allwords("MOVEM (a7)+", movem2, 2, 1, 3);
    allwords("MOVEM (a0)", movem3, 2, 1, 3);
    allwords("MOVEC", movec, 2, 1, 3);

    /* an odd instruction address, an odd a3 (68020: no address error) */
    fill(brief, 14);
    code[0] = 0x4e75;
    code[1] = 0x2030;                   /* at +1: $75 $20 ... */
    strcpy(VEC, "odd address");
    dis(1, 0);
    code[0] = 0x2030;
    code[1] = 0x8a26;
    strcpy(VEC, "odd a3");
    dis(0, 1);

    strcpy(VEC, "");
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);

    FreeMem(code, CODEWORDS * 2);
    for (s = 0; s < 2; s++)
        FreeMem(sarea[s], SAREA);
    rwe_mem_check(LIB, avail, TRUE);
    printf("ridisasmlib: %lu DisAssemble$ calls\n", calls);
    return rwe_done(LIB);
}
