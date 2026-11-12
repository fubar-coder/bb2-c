/*
 * rwe rigfxlib - rigfxlib (userlib 65): the C build or the CD binary
 * (BSS2 RIGFXLIB.OBJ), one set as LIBSET says, observed as cases
 * (sysload.h; the expected data
 * libs/tests/expected/rwe_rigfxlib.txt), in the emulator (snitch test
 * case "rwe rigfxlib", argv[1] = the directory with the library files).
 *
 * The library is loaded like the compiler does (sysload.h) with memlib
 * (SaveCMAP's globalloc/globfree), and every routine is called with the
 * inputs (other registers = markers, word arguments with markers in the
 * upper word).  Observed are d0, the whole memory the routines work on
 * (palettes and bitmaps lie in one buffer, filled the same before each
 * test, so that out-of-range colour numbers read and write the same
 * bytes), and the kept registers.
 *
 * SaveCMAP calls RIAmosFuncLib's BSave (form 1): the library is loaded
 * too - the C build with the C set, the AmiBlitz3 binary (the reference
 * of the port; its token list is the one of the BSS2 binary the original
 * rigfxlib was built against) with the original - and initialised after
 * memlib; the file it writes in RAM: is read back and observed, with the
 * FORM header and length checked, and one into a missing directory
 * (BSave fails: 0).
 */
#include "sysload.h"
#include "amosfunc.h"
#include "rwemem.h"

extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static struct regs R, IN;
static ULONG POP;

#define DO1(fn, setup) do {                                                 \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callstk(fn, &R, NULL, 0, &POP);                                     \
        obs_kept(&IN, &R);                                                  \
        OBS("popped", POP);                                                 \
    } while (0)

/* code of form `form` of entry idx; Amiga-only code addresses are odd */
static UBYTE *ent(int idx, int form)
{
    UBYTE *p = entry_form(getlib(&loaded, "rigfxlib"), idx, form);
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
static UBYTE mem[MEMSZ];
static ULONG objA[4], objB[4];          /* palette objects: data, numcols.w */

static void fillmem(void)
{
    ULONG i, v;
    for (i = 0; i < MEMSZ; i += 4) {
        v = rnd();
        memcpy(&mem[i], &v, 4);
    }
}

static void setpal(ULONG *obj, ULONG at, UWORD n, UWORD objn)
{
    obj[0] = (ULONG)&mem[at];
    obj[1] = (ULONG)objn << 16;
    *(UWORD *)&mem[at] = n;
}

/* ------------------------------------------------------- the CMAP files */

/* the file SaveCMAP wrote through RIAmosFuncLib's BSave (-1: none) */
static LONG bs_len;
static UBYTE bs_data[1024];

static void readfile(const char *name)
{
    BPTR f = Open((STRPTR)name, MODE_OLDFILE);

    memset(bs_data, 0, sizeof bs_data);
    bs_len = -1;
    if (f) {
        bs_len = Read(f, bs_data, sizeof bs_data);
        Close(f);
    }
}

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
            DO1(ent(fns[f], 0), R.d[0] = cols[i]);
            OBS("d0", R.d[0]);
        }
}

static void test_colours(void)
{
    /* before PaletteInfo: address 0 (same for both) */
    ROUT = "PalRed";
    strcpy(VEC, "no PaletteInfo");
    DO1(ent(PALRED, 0), R.d[0] = 1);
    OBS("d0", R.d[0]);

    fillmem();
    setpal(objA, PALA, 16, 16);
    ROUT = "PaletteInfo";
    strcpy(VEC, "A");
    DO1(ent(PALETTEINFO, 0), R.d[0] = 1; R.a[0] = (ULONG)objA);
    colours("A");
    OBSBYTES("memory", mem, MEMSZ);
}

static void paladjust(int form, ULONG ratio, ULONG start, ULONG end)
{
    ROUT = "PalAdjust";
    sprintf(VEC, "form %d ratio $%lX %lu..%lu", form, ratio, start & 0xffff, end & 0xffff);
    DO1(ent(PALADJUST, form), R.d[0] = WA(2); R.d[1] = ratio;
        R.a[0] = (ULONG)objB; if (form) { R.d[2] = start; R.d[3] = end; });
    OBSBYTES("memory", mem, MEMSZ);
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
    DO1(ent(PALETTEINFO, 0), R.a[0] = (ULONG)objA);
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
    DO1(ent(fn, form), R.d[0] = WA(2); R.d[1] = r; R.d[2] = g; R.d[3] = b;
        R.a[0] = (ULONG)objB; if (form) { R.d[4] = start; R.d[5] = end; });
    OBSBYTES("memory", mem, MEMSZ);
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

static void copycol(ULONG *src, ULONG *dst, ULONG cs, ULONG cd)
{
    ROUT = "CopyColour";
    sprintf(VEC, "%lX -> %lX", cs, cd);
    DO1(ent(COPYCOLOUR, 0), R.d[0] = WA(1); R.d[1] = WA(2); R.d[2] = cs;
        R.d[3] = cd; R.a[0] = (ULONG)src; R.a[1] = (ULONG)dst);
    OBS("d0", R.d[0]);
    OBSBYTES("memory", mem, MEMSZ);
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
static ULONG bm[16];

static void cpucls(UWORD bpr, UWORD rows, UWORD depth, const ULONG *offs)
{
    int i;

    fillmem();
    memset(bm, 0, sizeof bm);
    bm[0] = (ULONG)bpr << 16 | rows;
    bm[1] = (ULONG)depth << 16;
    for (i = 0; i < depth && i < 8; i++)
        bm[2 + i] = (ULONG)&mem[offs[i]];
    ROUT = "CPUCls";
    sprintf(VEC, "%ux%u depth %u at %lu", bpr, rows, depth, offs[0]);
    DO1(ent(CPUCLS, 0), R.d[0] = 1; R.a[0] = (ULONG)bm);
    OBSBYTES("memory", mem, MEMSZ);
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

/* the file (BSave opens with MODE_NEWFILE); "RAM:rwe-nodir/..." cannot be
   opened: BSave returns 0 */
static char cmapname[40];

static void setname(const char *dir)
{
    sprintf(cmapname, "RAM:%s%scmap", dir, *dir ? "/" : "");
    DeleteFile((STRPTR)cmapname);
}

static void savecmap(ULONG *obj, const char *dir)
{
    ULONG n = *(UWORD *)obj[0];

    ROUT = "SaveCMAP";
    sprintf(VEC, "%lu colours, %s", n, dir);
    setname(dir);
    DO1(ent(SAVECMAP, 0), R.d[0] = 3; R.d[1] = (ULONG)cmapname;
        R.a[0] = (ULONG)obj);
    OBS("d0", R.d[0]);
    readfile(cmapname);
    OBS("file length", bs_len);
    if (R.d[0])                                 /* written: FORM, the length */
        check("SaveCMAP: the file", bs_len == (LONG)(n * 3 + 48)
              && !memcmp(bs_data, "FORM", 4));
    else
        check("SaveCMAP: no file", bs_len == -1);
    OBSBYTES("file", bs_data, sizeof bs_data);
    OBSBYTES("memory", mem, MEMSZ);
    DeleteFile((STRPTR)cmapname);
}

static void test_savecmap(void)
{
    ULONG zero[4] = { 0 };

    fillmem();
    setpal(objA, PALA, 5, 5);
    setpal(objB, PALB, 32, 32);
    savecmap(objA, "");
    savecmap(objB, "");
    savecmap(objB, "rwe-nodir");                /* BSave: 0 */
    setpal(objB, PALB, 1, 1);
    savecmap(objB, "");
    ROUT = "SaveCMAP";
    strcpy(VEC, "no data");
    setname("");
    DO1(ent(SAVECMAP, 0), R.d[1] = (ULONG)cmapname; R.a[0] = (ULONG)zero);
    OBS("d0", R.d[0]);
    readfile(cmapname);
    OBS("no file", bs_len);
}

/* MergeBitplanes: objects of 64 bytes, constant contents */
static UBYTE mb[3][64];

static void merge(UWORD d1, UWORD d2, ULONG planes, UWORD f1, UWORD f2, UWORD fd)
{
    int k, i;

    for (k = 0; k < 3; k++) {
        for (i = 0; i < 64; i++)
            mb[k][i] = (UBYTE)(k * 64 + i * 7 + 1);
        *(UWORD *)&mb[k][62] = k == 0 ? f1 : k == 1 ? f2 : fd;
        if (k < 2)
            *(UWORD *)&mb[k][4] = k ? d2 : d1;
    }
    ROUT = "MergeBitplanes";
    sprintf(VEC, "%u+%u planes %lX flags %u,%u,%u", d1, d2, planes, f1, f2, fd);
    DO1(ent(MERGEBITPLANES, 0), R.d[0] = WA(0); R.d[1] = WA(1); R.d[2] = WA(2);
        R.d[3] = planes; R.a[0] = (ULONG)mb[0]; R.a[1] = (ULONG)mb[1];
        R.a[2] = (ULONG)mb[2]);
    OBSBYTES("objects", mb, sizeof mb);
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

static const char *names[] = { "memlib", "rigfxlib", "riamosfunclib", NULL };

int main(int argc, char **argv)
{
    ULONG avail;

    if (argc < 2) {
        printf("usage: rwe_rigfxlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("PalRed", d1-d7|a1-a3|a6);
    KEEPS("PalGreen", d1-d7|a1-a3|a6);
    KEEPS("PalBlue", d1-d7|a1-a3|a6);
    KEEPS("AGAPalRed", d1-d7|a1-a3|a6);
    KEEPS("AGAPalGreen", d1-d7|a1-a3|a6);
    KEEPS("AGAPalBlue", d1-d7|a1-a3|a6);
    KEEPS("PaletteInfo", d0-d7|a1-a3|a6);
    KEEPS("PalAdjust", d4|a2|a3|a6);
    KEEPS("FillPalette", d0|d6|a1-a3|a6);
    KEEPS("AGAFillPalette", d0|d6|a1-a3|a6);
    KEEPS("CopyColour", d1|d4-d7|a2|a3|a6);
    KEEPS("CPUCls", a6);
    KEEPS("SaveCMAP", d4-d6|a2|a3);
    KEEPS("MergeBitplanes", d2|d4-d7|a3|a6);

    test_colours();
    test_paladjust();
    test_fillpalette();
    test_copycolour();
    test_cpucls();
    test_merge();

    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    strcpy(VEC, "");
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), 7, 0), &R, NULL, 0, &POP);
    markregs(&R);                       /* RIAmosFuncLib: dos.library */
    callstk(entry_form(getlib(&loaded, "riamosfunclib"), AMOS_E_INIT, 0), &R, NULL, 0, &POP);
    test_savecmap();
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "riamosfunclib"), AMOS_E_FINIT, 0), &R, NULL, 0, &POP);
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), 8, 0), &R, NULL, 0, &POP);
    rwe_mem_check("rigfxlib", avail, TRUE);

    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
