/*
 * rwe ridisasmlib - the C build of the RWE userlib ridisasmlib
 * (libs/userlibs/rwe/ridisasmlib.c) or the reference build of its source
 * rwelibsrc/Disasmlib/Disasm-library.bb2 (ridisasmlib.orig), one set as
 * LIBSET says, observed as cases (rwecmp.h; the expected
 * data libs/tests/expected/rwe_ridisasmlib.txt).
 *
 *   ridisasmlib DIR     (DIR: ridisasmlib .obj/.orig)
 *
 * The set disassembles a code buffer at a fixed address (the PC relative
 * targets in the text are absolute, so every run gives the same text)
 * into a string area (a3, filled with $AA before every call); observed
 * after every call are d0 (34 or 5), how far a3 moved, the string area
 * (40 bytes: the string and what is behind it), InstrSize and the kept
 * registers.  The cases of the sweeps over the instruction words (about
 * 3 million) are written as block cases (design D6 of the openspec change
 * libs-tests-capture: LIBS_CAP_DUMP=BLOCK:N writes the observations of
 * block N); init, the first InstrSize, the odd address and a3 and finit
 * are plain cases.
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
#define CODEADDR 0x47000000UL           /* in the Zorro III RAM of default.uae */
#define SAREA 48
static UWORD *code;
static UBYTE *sarea;
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
    calls++;
    memset(sarea, 0xaa, SAREA);
    ROUT = "DisAssemble$";
    DO1(ENT(LIB, E_DIS, 0), R.d[0] = (ULONG)code + off; R.a[3] = (ULONG)sarea + aoff);
    OBS("d0", R.d[0]);
    OBS("a3 moved", R.a[3] - (ULONG)sarea);
    OBSBYTES("string", sarea, 40);
    ROUT = "InstrSize";
    DO1(ENT(LIB, E_SIZE, 0), (void)0);
    OBS("d0", R.d[0]);
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
    printf("%s: %lu calls so far\n", what, calls);
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
    printf("%s: %lu calls so far\n", what, calls);
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

    if (argc < 2) {
        printf("usage: ridisasmlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("InstrSize (before DisAssemble$)", d1-d7|a0-a3|a6);
    KEEPS("DisAssemble$", a2|a6);
    KEEPS("InstrSize", d1-d7|a0-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    /* the code buffer at a fixed address: the PC relative targets in the
       text are absolute, and the expected data needs the same text in
       every run; any free block if that one is taken.  Outside
       the memory balance (rwe_mem_start does not count AllocAbs) */
    code = AllocAbs(CODEWORDS * 2, (APTR)CODEADDR);
    if (!code)
        code = AllocMem(CODEWORDS * 2, MEMF_PUBLIC);
    if (code)
        memset(code, 0, CODEWORDS * 2);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    sarea = AllocMem(SAREA, MEMF_PUBLIC);

    strcpy(VEC, "");
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    ROUT = "InstrSize (before DisAssemble$)";
    DO1(ENT(LIB, E_SIZE, 0), (void)0);
    OBS("d0", R.d[0]);

    OBS_BLOCKS(1);

    allops("all first words, brief extensions", brief, 14);
    allops("all first words, full format extensions", full, 14);
    allwords("MOVE.l (bd,a0,xn),(bd,a0,xn): source extension", move2, 11, 1, 1);
    allwords("MOVE.l (bd,a0,xn),(bd,a0,xn): destination extension", move2, 11, 6, 3);
    allwords("MOVE.l (pc,xn),d0 forms", pcx, 7, 1, 2);
    allwords("MOVEM -(a7)", movem1, 2, 1, 1);
    allwords("MOVEM (a7)+", movem2, 2, 1, 3);
    allwords("MOVEM (a0)", movem3, 2, 1, 3);
    allwords("MOVEC", movec, 2, 1, 3);
    OBS_BLOCKS(0);

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
    DO1(ENT(LIB, E_FINIT, 0), (void)0);

    FreeMem(sarea, SAREA);
    rwe_mem_check(LIB, avail, TRUE);
    FreeMem(code, CODEWORDS * 2);
    printf("ridisasmlib: %lu DisAssemble$ calls\n", calls);
    return rwe_done(LIB);
}
