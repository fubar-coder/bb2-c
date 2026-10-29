/*
 * rwe rigfxlib - the C build of rigfxlib (userlib 65) against the CD
 * binary (BSS2 RIGFXLIB.OBJ), in the emulator (snitch test case
 * "rwe rigfxlib", argv[1] = the directory with the library files).
 *
 * Both libraries are loaded like the compiler does (sysload.h) with
 * memlib (SaveCMAP's globalloc/globfree), and every routine is called on
 * both with the same inputs (other registers = markers, word arguments
 * with markers in the upper word).  Compared are d0, the whole memory
 * the routines work on (palettes and bitmaps lie in one buffer per set,
 * filled the same, so that out-of-range colour numbers read and write the
 * same bytes), and every register the original keeps must be kept by the
 * C build.
 *
 * SaveCMAP calls RIAmosFuncLib's BSave, a library that is not ported: a
 * stub (sysload_stubs) takes its place for both sets, records the name,
 * the length and the file data and returns -1 (or 0).
 */
#include "sysload.h"

extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static int cur;
static struct regs R[2], IN[2];
static ULONG POP[2];
static const char *ROUT;
static char VEC[100];
static int prints;

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

/* the same bytes in both sets (a SAME of a memcmp would compare the
   result with itself) */
static void samebytes(const char *what, const void *o, const void *c, ULONG n)
{
    const UBYTE *a = o, *b = c;
    ULONG i;
    for (i = 0; i < n && a[i] == b[i]; i++)
        ;
    if (i < n) {
        char buf[60];
        snprintf(buf, sizeof buf, "%s (offset %lu)", what, i);
        fail(buf, a[i], b[i]);
    } else
        checks++;
}

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
    SAME("popped", POP[cur]);
}

#define DO2(fn, setup) do {                                                 \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callstk(fn, &R[cur], NULL, 0, &POP[cur]);                       \
        }                                                                   \
        preserve();                                                         \
    } while (0)

/* code of form `form` of entry idx; Amiga-only code addresses are odd */
static UBYTE *ent(int idx, int form)
{
    UBYTE *p = entry_form(getlib(&sets[cur], "rigfxlib"), idx, form);
    return (UBYTE *)((ULONG)p & ~1UL);
}

enum { PALRED, PALGREEN, PALBLUE, PALETTEINFO, AGAPALRED, AGAPALGREEN, AGAPALBLUE,
       PALADJUST, FILLPALETTE, AGAFILLPALETTE, COPYCOLOUR, CPUCLS, SAVECMAP,
       MERGEBITPLANES };

static ULONG seed = 0x13579bdf;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* word argument with a marker in the upper word */
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* ------------------------------------------------------------- memory */

#define MEMSZ 72000
#define PALA 36000                      /* data of palette A (PaletteInfo) */
#define PALB 40000                      /* data of palette B (the target) */
static UBYTE mem[2][MEMSZ];
static ULONG objA[2][4], objB[2][4];    /* palette objects: data, numcols.w */

static void fillmem(void)
{
    ULONG i, v;
    for (i = 0; i < MEMSZ; i += 4) {
        v = rnd();
        memcpy(&mem[0][i], &v, 4);
        memcpy(&mem[1][i], &v, 4);
    }
}

static void setpal(ULONG (*obj)[4], ULONG at, UWORD n, UWORD objn)
{
    int s;
    for (s = 0; s < 2; s++) {
        obj[s][0] = (ULONG)&mem[s][at];
        obj[s][1] = (ULONG)objn << 16;
        *(UWORD *)&mem[s][at] = n;
    }
}

static void samemem(void)
{
    samebytes("memory", mem[0], mem[1], MEMSZ);
}

/* ------------------------------------------------------------ BSave stub */

static ULONG bs_name[2], bs_len[2], bs_calls[2], bs_ret;
static UBYTE bs_data[2][1024];

static ULONG __attribute__((used)) st_bsave(ULONG name __asm("d0"), ULONG addr __asm("d1"),
                                            ULONG len __asm("d2"))
{
    bs_calls[cur]++;
    bs_name[cur] = name;
    bs_len[cur] = len;
    memset(bs_data[cur], 0, sizeof bs_data[cur]);
    memcpy(bs_data[cur], (void *)addr, len < sizeof bs_data[cur] ? len : sizeof bs_data[cur]);
    return bs_ret;
}

static const struct stub gfxstubs[] = {
    { (75 << 7) + 16, (APTR)st_bsave },     /* RIAmosFuncLib BSave */
    { 0, NULL }
};

/* --------------------------------------------------------------- tests */

static void colours(const char *when)
{
    static const ULONG cols[] = { 0, 1, 2, 7, 15, 16, 100, 2730, 2731, 5000, 0xffff,
                                  0x5A010003UL };
    static const int fns[] = { PALRED, PALGREEN, PALBLUE, AGAPALRED, AGAPALGREEN, AGAPALBLUE };
    static const char *fname[] = { "PalRed", "PalGreen", "PalBlue", "AGAPalRed", "AGAPalGreen",
                                   "AGAPalBlue" };
    unsigned i, f;

    for (f = 0; f < 6; f++)
        for (i = 0; i < sizeof cols / sizeof cols[0]; i++) {
            ROUT = fname[f];
            sprintf(VEC, "%s colour $%lX", when, cols[i]);
            DO2(ent(fns[f], 0), R[cur].d[0] = cols[i]);
            SAME("d0", R[cur].d[0]);
        }
}

static void test_colours(void)
{
    /* before PaletteInfo: address 0 (same for both) */
    ROUT = "PalRed";
    strcpy(VEC, "no PaletteInfo");
    DO2(ent(PALRED, 0), R[cur].d[0] = 1);
    SAME("d0", R[cur].d[0]);

    fillmem();
    setpal(objA, PALA, 16, 16);
    ROUT = "PaletteInfo";
    strcpy(VEC, "A");
    DO2(ent(PALETTEINFO, 0), R[cur].d[0] = 1; R[cur].a[0] = (ULONG)objA[cur]);
    colours("A");
    samemem();
}

static void paladjust(int form, ULONG ratio, ULONG start, ULONG end)
{
    ROUT = "PalAdjust";
    sprintf(VEC, "form %d ratio $%lX %lu..%lu", form, ratio, start & 0xffff, end & 0xffff);
    DO2(ent(PALADJUST, form), R[cur].d[0] = WA(2); R[cur].d[1] = ratio;
        R[cur].a[0] = (ULONG)objB[cur]; if (form) { R[cur].d[2] = start; R[cur].d[3] = end; });
    samemem();
}

static void test_paladjust(void)
{
    static const ULONG ratios[] = { 0, 0x8000, 0x10000, 0x18000, 0x20000, 0x0000ff00UL,
                                    0xffff8000UL, 0x01234567UL, 0x00007fffUL };
    unsigned i;

    fillmem();
    setpal(objA, PALA, 12, 12);
    setpal(objB, PALB, 20, 20);
    ROUT = "PaletteInfo";
    strcpy(VEC, "A");
    DO2(ent(PALETTEINFO, 0), R[cur].a[0] = (ULONG)objA[cur]);
    for (i = 0; i < sizeof ratios / sizeof ratios[0]; i++)
        paladjust(0, ratios[i], 0, 0);
    paladjust(1, 0x18000, WA(0), WA(11));
    paladjust(1, 0x8000, WA(2), WA(5));
    paladjust(1, 0x30000, WA(5), WA(5));
    paladjust(1, 0x10000, WA(0), WA(15));       /* past palette A's 12 colours */
    paladjust(1, 0x12000, WA(2731), WA(2731));  /* index before the palettes */
    paladjust(1, 0x9000, 7, 9);
}

static void fillp(int fn, int form, ULONG r, ULONG g, ULONG b, ULONG start, ULONG end)
{
    ROUT = fn == FILLPALETTE ? "FillPalette" : "AGAFillPalette";
    sprintf(VEC, "form %d $%lX,$%lX,$%lX %lX..%lX", form, r, g, b, start, end);
    DO2(ent(fn, form), R[cur].d[0] = WA(2); R[cur].d[1] = r; R[cur].d[2] = g; R[cur].d[3] = b;
        R[cur].a[0] = (ULONG)objB[cur]; if (form) { R[cur].d[4] = start; R[cur].d[5] = end; });
    samemem();
}

static void test_fillpalette(void)
{
    int fn;

    fillmem();
    setpal(objB, PALB, 9, 9);
    for (fn = FILLPALETTE; fn <= AGAFILLPALETTE; fn++) {
        fillp(fn, 0, WA(15), WA(7), WA(0), 0, 0);
        fillp(fn, 0, 0x5A01FFFFUL, 0x1234UL, 0xA5UL, 0, 0);
        fillp(fn, 1, WA(3), WA(0x88), WA(0xff), WA(1), WA(3));
        fillp(fn, 1, 1, 2, 3, WA(4), WA(4));
        fillp(fn, 1, 4, 5, 6, WA(3), WA(1));            /* end < start: nothing */
        fillp(fn, 1, 7, 8, 9, 0, 0);                    /* end 0: the last colour */
        fillp(fn, 1, 10, 11, 12, WA(2), 0x00010000UL);  /* end 0 in the word only */
        fillp(fn, 1, 13, 14, 15, WA(0), 0x00010000UL);
        fillp(fn, 1, 0x21, 0x43, 0x65, WA(5), WA(20));  /* past the palette */
    }
}

static void copycol(ULONG (*src)[4], ULONG (*dst)[4], ULONG cs, ULONG cd)
{
    ROUT = "CopyColour";
    sprintf(VEC, "%lX -> %lX", cs, cd);
    DO2(ent(COPYCOLOUR, 0), R[cur].d[0] = WA(1); R[cur].d[1] = WA(2); R[cur].d[2] = cs;
        R[cur].d[3] = cd; R[cur].a[0] = (ULONG)src[cur]; R[cur].a[1] = (ULONG)dst[cur]);
    SAME("d0", R[cur].d[0]);
    samemem();
}

static void test_copycolour(void)
{
    fillmem();
    setpal(objA, PALA, 8, 8);
    setpal(objB, PALB, 4, 5);
    copycol(objA, objB, WA(2), WA(3));
    copycol(objA, objB, WA(7), WA(0));
    copycol(objA, objB, WA(8), WA(0));          /* not below the colours */
    copycol(objA, objB, WA(0), WA(5));
    copycol(objA, objB, WA(0), WA(4));          /* object says 5, data 4 */
    copycol(objB, objA, 3, 7);
    copycol(objA, objA, 1, 6);
}

/* bitmap objects: bpr, rows, depth word, planes at offsets of mem */
static ULONG bm[2][16];

static void cpucls(UWORD bpr, UWORD rows, UWORD depth, const ULONG *offs)
{
    int s, i;

    fillmem();
    for (s = 0; s < 2; s++) {
        memset(bm[s], 0, sizeof bm[s]);
        bm[s][0] = (ULONG)bpr << 16 | rows;
        bm[s][1] = (ULONG)depth << 16;
        for (i = 0; i < depth && i < 8; i++)
            bm[s][2 + i] = (ULONG)&mem[s][offs[i]];
    }
    ROUT = "CPUCls";
    sprintf(VEC, "%ux%u depth %u at %lu", bpr, rows, depth, offs[0]);
    DO2(ent(CPUCLS, 0), R[cur].d[0] = 1; R[cur].a[0] = (ULONG)bm[cur]);
    samemem();
}

static void test_cpucls(void)
{
    static const ULONG offs1[] = { 1000, 3000, 5000 };
    static const ULONG offs2[] = { 2001, 7777, 9000 };

    cpucls(40, 10, 1, offs1);
    cpucls(41, 3, 2, offs1);
    cpucls(80, 2, 3, offs1);
    cpucls(6, 7, 1, offs1);             /* 42 bytes: one block, one word */
    cpucls(21, 3, 3, offs2);            /* 63: rest 23 - one byte stays */
    cpucls(40, 1, 2, offs2);            /* odd planes */
    cpucls(320 / 8, 256, 1, offs1);
}

static char cmapname[] = "RAM:rwe-cmap";

static void savecmap(ULONG (*obj)[4], ULONG ret)
{
    ROUT = "SaveCMAP";
    sprintf(VEC, "%u colours, BSave %ld", *(UWORD *)obj[0][0], (LONG)ret);
    bs_ret = ret;
    for (cur = 0; cur < 2; cur++) {
        bs_calls[cur] = bs_len[cur] = bs_name[cur] = 0;
        memset(bs_data[cur], 0, sizeof bs_data[cur]);
    }
    DO2(ent(SAVECMAP, 0), R[cur].d[0] = 3; R[cur].d[1] = (ULONG)cmapname;
        R[cur].a[0] = (ULONG)obj[cur]);
    SAME("d0", R[cur].d[0]);
    SAME("BSave calls", bs_calls[cur]);
    check("SaveCMAP: BSave called once", bs_calls[0] == 1);
    SAME("BSave name", bs_name[cur]);
    SAME("BSave length", bs_len[cur]);
    samebytes("file", bs_data[0], bs_data[1], sizeof bs_data[0]);
    samemem();
}

static void test_savecmap(void)
{
    ULONG zero[2][4] = { { 0 }, { 0 } };

    fillmem();
    setpal(objA, PALA, 5, 5);
    setpal(objB, PALB, 32, 32);
    savecmap(objA, -1);
    savecmap(objB, -1);
    savecmap(objB, 0);
    setpal(objB, PALB, 1, 1);
    savecmap(objB, 1234);
    ROUT = "SaveCMAP";
    strcpy(VEC, "no data");
    DO2(ent(SAVECMAP, 0), R[cur].d[1] = (ULONG)cmapname; R[cur].a[0] = (ULONG)zero[cur]);
    SAME("d0", R[cur].d[0]);
}

/* MergeBitplanes: objects of 64 bytes, the same constant contents */
static UBYTE mb[2][3][64];

static void merge(UWORD d1, UWORD d2, ULONG planes, UWORD f1, UWORD f2, UWORD fd)
{
    int s, k, i;

    for (s = 0; s < 2; s++)
        for (k = 0; k < 3; k++) {
            for (i = 0; i < 64; i++)
                mb[s][k][i] = (UBYTE)(k * 64 + i * 7 + 1);
            *(UWORD *)&mb[s][k][62] = k == 0 ? f1 : k == 1 ? f2 : fd;
            if (k < 2)
                *(UWORD *)&mb[s][k][4] = k ? d2 : d1;
        }
    ROUT = "MergeBitplanes";
    sprintf(VEC, "%u+%u planes %lX flags %u,%u,%u", d1, d2, planes, f1, f2, fd);
    DO2(ent(MERGEBITPLANES, 0), R[cur].d[0] = WA(0); R[cur].d[1] = WA(1); R[cur].d[2] = WA(2);
        R[cur].d[3] = planes; R[cur].a[0] = (ULONG)mb[cur][0]; R[cur].a[1] = (ULONG)mb[cur][1];
        R[cur].a[2] = (ULONG)mb[cur][2]);
    samebytes("objects", mb[0], mb[1], sizeof mb[0]);
}

static void test_merge(void)
{
    merge(2, 3, WA(4), 1, 1, 0);
    merge(2, 3, WA(5), 1, 1, 0);
    merge(2, 3, WA(6), 1, 1, 0);        /* too many */
    merge(2, 3, WA(1), 1, 1, 0);
    merge(2, 3, WA(2), 1, 1, 0);
    merge(2, 3, WA(3), 1, 1, 0);
    merge(2, 3, WA(0), 1, 1, 0);        /* 0: all of both */
    merge(3, 0, WA(3), 1, 1, 0);
    merge(0, 4, WA(2), 1, 1, 0);
    merge(2, 3, WA(4), 0, 1, 0);        /* src1 not "allocated" */
    merge(2, 3, WA(4), 1, 0, 0);
    merge(2, 3, WA(4), 1, 1, 1);        /* dest already has planes */
    merge(2, 3, WA(0xfffe), 1, 1, 0);   /* negative count */
    merge(0x7fff, 0x7fff, WA(3), 1, 1, 0);      /* sum negative as a word */
}

static const char *names[] = { "memlib", "rigfxlib", NULL };

int main(int argc, char **argv)
{
    ULONG avail;

    if (argc < 2) {
        printf("usage: rwe_rigfxlib DIR\n");
        return 20;
    }
    sysload_stubs = gfxstubs;
    if (!load_sets(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;

    test_colours();
    test_paladjust();
    test_fillpalette();
    test_copycolour();
    test_cpucls();
    test_merge();

    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib init";
    strcpy(VEC, "");
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 7, 0), &R[cur], NULL, 0, &POP[cur]);
    }
    test_savecmap();
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 8, 0), &R[cur], NULL, 0, &POP[cur]);
    }
    check("rigfxlib: all memory back", AvailMem(MEMF_ANY) == avail);

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
