/*
 * rwe rifnslib - the FNS library: the C build or the CD original
 * (BSS2 RIFNSLIB.OBJ), one set as LIBSET says, observed as cases
 * (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_rifnslib.txt).
 *
 *   rifnslib DIR     (DIR with rifnslib .obj or .orig)
 *
 * Text is printed into a bitmap in memory (one buffer with the
 * planes and guards): observed after every call are
 * the whole buffer, FNSPrint's end position (the words at a0), d0 and the
 * kept registers.  Fonts: the built-in one (slot 0) and
 * fonts made here, 16, 32, 48 and 64 pixels wide (random glyphs, widths
 * 0..width).  RIAmosFuncLib is not loaded: stubs (sysload_stubs) take
 * the place of BLoad, Start and Erase and record the calls.
 *
 * Covered: FNSOutput both forms, depths 1..8; FNSClip/FNSClipOutput;
 * FNSPrint/FNSShadowPrint all forms with all 16 prefs (centred, bold,
 * underlined, right aligned), colours, chr$(1) colour changes, tabs
 * (FNSSetTab), new lines (also leading and double ones: the binary's
 * missing guard), zero-width characters, codes from 160 (end), x on
 * every bit position, near xmax, below xmin, rows clipped at ymin/ymax
 * (the 48/64 pixel BUG(orig) (#656)), an empty text (underline 1 pixel, the
 * centring by -32767), FNSOrigin; FNSLength both forms (the prefs of the
 * last FNSPrint) and its unmasked font number (16, 17, -1, -2: the
 * variables and the font of the block); FNSHeight/Width/Underline,
 * InstallFNS (and a non-FNS address), RemoveFNS, FNSSlot, FNSInk,
 * FNSPrefs, FNSVersion (inline), FNSLoad (ok, BLoad fails, Start 0) and
 * FNSUnload.  Not covered: FNSPrint without a bitmap (a crash), depth 0
 * or above 8 and bitmaps with flags (the bitstack overrun), FNSSetTab 0
 * (division by zero), x below 0 with xmin below 0 (writes 64 KB away).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "rifnslib"
#define BPR 40                          /* 320 pixels */
#define ROWS 64
#define PLANE (BPR * ROWS)
#define GUARD 256
#define BMSZ (GUARD + 8 * PLANE + GUARD)

enum { PRINT, INSTALL, OUTPUT, INK, PREFS, HEIGHT, CLIP, CLIPOUT, REMOVE, UNDERL, LENGTH,
       VERSION, ORIGIN, WIDTH, LOAD, SLOT, UNLOAD, SETTAB, SHADOW, FINIT, INIT };

static UBYTE *bmbuf;              /* the planes */
static UWORD bm[4 + 16];          /* struct BitMap */
static UBYTE *font[5];             /* the test fonts */
static ULONG fontsz[5];
static const char *VECSTR;

/* -------------------------------------------------------- RIAmosFuncLib */

enum { BLOAD = 1, START, ERASE };
static struct { int fn; ULONG d0, d1; } flog[8];
static int nflog, bloadfail, startnull;
static UBYTE *bank;

static void flogit(int fn, ULONG d0, ULONG d1)
{
    if (nflog < 8) {
        flog[nflog].fn = fn;
        flog[nflog].d0 = d0;
        flog[nflog].d1 = d1;
        nflog++;
    }
}

static ULONG __attribute__((used)) st_bload(ULONG name __asm("d0"), ULONG b __asm("d1"))
{
    flogit(BLOAD, strcmp((const char *)name, "RAM:font"), b);
    if (bloadfail)
        return 0;
    memcpy(bank, font[1], fontsz[1]);
    return (ULONG)-1;
}

static ULONG __attribute__((used)) st_start(ULONG b __asm("d0"))
{
    flogit(START, b, 0);
    return startnull ? 0 : (ULONG)bank;
}

static void __attribute__((used)) st_erase(ULONG b __asm("d0"))
{
    flogit(ERASE, b, 0);
}

static const struct stub stubs[] = {
    { (75 << 7) + 15, (APTR)st_bload },
    { (75 << 7) + 17, (APTR)st_start },
    { (75 << 7) + 19, (APTR)st_erase },
    { 0, NULL }
};

/* ------------------------------------------------------------- helpers */

static ULONG fnsslot(void)
{
    struct regs r;
    ULONG popped;
    markregs(&r);
    callstk(entry_form(getlib(&loaded, LIB), SLOT, 0), &r, NULL, 0, &popped);
    return r.d[0];
}

/* a font: width fw, height h, random glyphs and widths 0..fw */
static ULONG mkfont(UBYTE *f, UWORD fw, UWORD h, UWORD under)
{
    UWORD multi = (UWORD)(fw / 8 * h);
    ULONG i, n = 256 + 128UL * multi;
    for (i = 0; i < n; i++)
        f[i] = (UBYTE)rnd();
    memcpy(f, "FNS.", 4);
    *(UWORD *)(f + 4) = h;
    *(UWORD *)(f + 6) = fw;
    *(UWORD *)(f + 8) = under;
    *(UWORD *)(f + 10) = multi;
    for (i = 32; i < 160; i++)
        f[i] = (UBYTE)(rnd() % 4 == 0 ? 0 : rnd() % (fw + 1));
    f[' '] = (UBYTE)(fw / 2);
    return n;
}

static void same_bitmap(void)
{
    OBSBYTES("bitmap", bmbuf, BMSZ);
}

static void output(int depth, int form)
{
    int i;
    bm[0] = BPR;
    bm[1] = ROWS;
    bm[2] = (UWORD)depth;               /* Flags 0, Depth */
    bm[3] = 0;
    for (i = 0; i < 8; i++)
    {
        ULONG pa = (ULONG)(bmbuf + GUARD + i * PLANE);
        bm[4 + 2 * i] = (UWORD)(pa >> 16);
        bm[5 + 2 * i] = (UWORD)pa;
    }
    ROUT = "FNSOutput";
    sprintf(VEC, "depth %d form %d", depth, form);
    DO1(ENT(LIB, OUTPUT, form), R.a[0] = (ULONG)bm; R.d[0] = 0x5a000003;
        R.d[1] = form ? 0x5a000001 : 0x5a000000);
}

static void clip(WORD x0, WORD y0, WORD x1, WORD y1)
{
    ROUT = "FNSClip";
    sprintf(VEC, "%d,%d,%d,%d", x0, y0, x1, y1);
    DO1(ENT(LIB, CLIP, 0), R.d[0] = 0x5a000000 | (UWORD)x0;
        R.d[1] = 0x5a000000 | (UWORD)y0; R.d[2] = 0x5a000000 | (UWORD)x1;
        R.d[3] = 0x5a000000 | (UWORD)y1);
}

static void stmt1(int e, int form, const char *name, ULONG d0)
{
    ROUT = name;
    sprintf(VEC, "%lX", d0);
    DO1(ENT(LIB, e, form), R.d[0] = d0);
}

/* the end position at a0 (relative to the block), or a0 itself */
static void endpos(ULONG a0in)
{
    OBS("a0", R.a[0] == a0in ? 1 : R.a[0] - fnsslot());
    if (R.a[0] != a0in) {
        OBS("end x", *(UWORD *)R.a[0]);
        OBS("end y", *(UWORD *)(R.a[0] + 2));
    }
}

/* FNSPrint (form 0/1) or FNSShadowPrint (form 0/1/2 as 10/11/12) */
static void print(int form, UWORD fnt, WORD x, WORD y, const char *str, UWORD prefs,
                  UWORD colour, UWORD shadow)
{
    int e = form >= 10 ? SHADOW : PRINT, f = form % 10;
    ROUT = e == SHADOW ? "FNSShadowPrint" : "FNSPrint";
    snprintf(VEC, sizeof VEC, "f%d font %u %d,%d \"%.20s\" prefs %X col %u/%u", f, fnt, x, y,
             str, prefs, colour, shadow);
    VECSTR = str;
    DO1(ENT(LIB, e, f), R.d[0] = 0x5a000000 | fnt; R.d[1] = 0x5a000000 | (UWORD)x;
        R.d[2] = 0x5a000000 | (UWORD)y; R.d[3] = (ULONG)str;
        if (f >= 1) { R.d[4] = 0x5a000000 | prefs; R.d[5] = 0x5a000000 | colour; }
        if (f >= 2) R.d[6] = 0x5a000000 | shadow);
    endpos(MARK(8));
    same_bitmap();
}

static void clear(void)
{
    memset(bmbuf, 0x5a, BMSZ);
}

static void install(int slot, int f)
{
    ROUT = "InstallFNS";
    sprintf(VEC, "slot %d font %d", slot, f);
    DO1(ENT(LIB, INSTALL, 0), R.d[0] = (ULONG)slot; R.d[1] = (ULONG)font[f]);
    OBS("d0", R.d[0]);
}

static void fontfuncs(void)
{
    static const ULONG n[] = { 0, 1, 2, 3, 4, 5, 9, 15, 0x5a000010, 0x5a000005 };
    static const char *rn[] = { "FNSHeight", "FNSWidth", "FNSUnderline" };
    static const int en[] = { HEIGHT, WIDTH, UNDERL };
    unsigned i, k;
    for (k = 0; k < 3; k++)
        for (i = 0; i < sizeof n / sizeof n[0]; i++) {
            ROUT = rn[k];
            sprintf(VEC, "%lX", n[i]);
            DO1(ENT(LIB, en[k], 0), R.d[0] = n[i]);
            OBS("d0", R.d[0]);
        }
}

static void length(int form, ULONG n, const char *str, UWORD prefs)
{
    ROUT = "FNSLength";
    snprintf(VEC, sizeof VEC, "f%d font %lX \"%.20s\" prefs %X", form, n, str, prefs);
    DO1(ENT(LIB, LENGTH, form), R.d[0] = n; R.d[1] = (ULONG)str;
        if (form) R.d[2] = 0x5a000000 | prefs);
    OBS("d0", R.d[0]);
}

static const char *texts[] = {
    "Hello, World!",
    "",
    "\n",
    "a\n\nb",
    "\nleading",
    "tab\there\t!",
    "col\001\003our\001\005s",
    "high \240 ends",
    "ctrl \002 ends",
    "line one\nline two\nthree",
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz",
    /* ALLCODES (the last): the codes behind 'z' too - the built-in font
       has glyphs up to 'z' only, the others are read behind it (the
       original's code: the DIFF), so not with font 0 */
    "{|}~\177\200\217\237 the codes up to 159",
};
#define NTEXTS (sizeof texts / sizeof texts[0])

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UWORD fws[5] = { 16, 32, 48, 64, 24 };
    static const UWORD fhs[5] = { 9, 12, 7, 10, 8 };
    unsigned i, t;
    int f, d;

    if (argc < 2) {
        printf("usage: rifnslib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    KEEPS("init", d0-d7|a1-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    KEEPS("FNSVersion", d1-d7|a0-a3|a6);
    KEEPS("FNSHeight", d1-d7|a1-a3|a6);
    KEEPS("FNSWidth", d1-d7|a1-a3|a6);
    KEEPS("FNSUnderline", d1-d7|a1-a3|a6);
    KEEPS("InstallFNS", d1-d7|a1-a3|a6);
    KEEPS("RemoveFNS", d1-d7|a1-a3|a6);
    KEEPS("FNSOutput", d2-d6|a0-a3|a6);
    KEEPS("FNSClipOutput", d1-d7|a1-a3|a6);
    KEEPS("FNSClip", d1|d3-d7|a0-a3|a6);
    KEEPS("FNSPrint", d0|d1|d6|a1-a3|a6);
    KEEPS("FNSShadowPrint", a1-a3|a6);
    KEEPS("FNSInk", d0-d7|a0-a3|a6);
    KEEPS("FNSPrefs", d0-d7|a0-a3|a6);
    KEEPS("FNSOrigin", d0-d7|a0-a3|a6);
    KEEPS("FNSSetTab", d0-d7|a0-a3|a6);
    KEEPS("FNSLength", d1|d3-d7|a0-a3|a6);
    KEEPS("FNSLoad", d2|d3|d6|d7|a2|a3|a6);
    KEEPS("FNSUnload", d2|d3|d5-d7|a1-a3|a6);
    bmbuf = AllocMem(BMSZ, MEMF_PUBLIC);
    bank = AllocMem(20000, MEMF_PUBLIC | MEMF_CLEAR);
    for (f = 0; f < 5; f++) {
        font[f] = AllocMem(20000, MEMF_PUBLIC);
        fontsz[f] = mkfont(font[f], fws[f], fhs[f], (UWORD)(fhs[f] - 2 + f % 2 * 4));
    }
    clear();

    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, INIT, 0), (void)0);
    BOTH("slot 0 = the built-in font", *(ULONG *)fnsslot() == fnsslot() + 64);
    OBSBYTES("built-in font", (const UBYTE *)(fnsslot() + 64), 1712);
    {   /* FNSVersion: inline, the code behind $A000,len */
        ROUT = "FNSVersion";
        BOTH("inline", W(ENT(LIB, VERSION, 0)) == 0xa000);
        OBS("inline length", W(ENT(LIB, VERSION, 0) + 2));
        DO1(ENT(LIB, VERSION, 0) + 4, (void)0);
        OBS("d0", R.d[0]);
    }

    fontfuncs();
    install(1, 0);
    install(2, 1);
    install(3, 2);
    install(4, 3);
    install(5, 4);
    install(0x5a000017, 0);             /* slot 7 */
    ROUT = "InstallFNS";
    strcpy(VEC, "no FNS");
    DO1(ENT(LIB, INSTALL, 0), R.d[0] = 6; R.d[1] = (ULONG)bmbuf);
    OBS("d0", R.d[0]);
    fontfuncs();
    stmt1(REMOVE, 0, "RemoveFNS", 0x5a000017);
    fontfuncs();

    /* depths, both FNSOutput forms, every font and text */
    for (d = 1; d <= 8; d++) {
        output(d, d & 1);
        if (!(d & 1)) {
            ROUT = "FNSClipOutput";
            strcpy(VEC, "");
            DO1(ENT(LIB, CLIPOUT, 0), (void)0);
        }
        for (f = 0; f <= 5; f++) {
            clear();
            for (t = 0; t < NTEXTS; t++) {
                if (f == 0 && t == NTEXTS - 1)  /* see ALLCODES */
                    continue;
                print(t & 1, (UWORD)f, (WORD)(t * 7 + d), (WORD)(t * 3 + f), texts[t],
                      (UWORD)((t + d) & 15), (UWORD)(d * 3 + t), 0);
            }
        }
    }

    /* prefs and colours on one font each, x on every bit position */
    output(4, 1);
    for (f = 0; f <= 5; f++)
        for (i = 0; i < 16; i++) {
            clear();
            print(1, (UWORD)f, (WORD)(100 + i * 3), (WORD)(10 + i), "Prefs AbC\tx", (UWORD)i,
                  (UWORD)(i ^ 5), 0);
            print(1, (UWORD)f, (WORD)(i * 5), 30, "", (UWORD)i, 3, 0);
            print(1, (UWORD)f, 300, 40, "near the edge", (UWORD)i, 15, 0);
        }

    /* clipping: rows at ymin/ymax, x below xmin (x rounded to 16) */
    for (f = 0; f <= 5; f++) {
        clear();
        clip(37, 20, 250, 30);
        print(1, (UWORD)f, 20, 15, "clipped top AND left", 4, 7, 0);
        print(1, (UWORD)f, 20, 26, "clipped bottom\nnext", 4, 6, 0);
        print(1, (UWORD)f, 240, 22, "right", 0, 5, 0);
        clip(0, 0, 320, 63);
        print(1, (UWORD)f, 3, 58, "below the bitmap", 0, 5, 0);
        ROUT = "FNSClipOutput";
        strcpy(VEC, "");
        DO1(ENT(LIB, CLIPOUT, 0), (void)0);
    }

    /* settings: ink, prefs, origin, tab size; the forms without them */
    clear();
    stmt1(INK, 0, "FNSInk", 0x5a000006);
    stmt1(PREFS, 0, "FNSPrefs", 0x5a000005);
    print(0, 0, 10, 10, "ink and prefs", 0, 0, 0);
    ROUT = "FNSPrefs";
    DO1(ENT(LIB, PREFS, 1), R.d[0] = 0x5a00000a; R.d[1] = 0x5a000003);
    print(0, 2, 200, 10, "prefs 2", 0, 0, 0);
    ROUT = "FNSOrigin";
    DO1(ENT(LIB, ORIGIN, 1), R.d[0] = 0x5a000011; R.d[1] = 0x5a000007);
    print(0, 1, 10, 20, "origin", 0, 0, 0);
    stmt1(SETTAB, 0, "FNSSetTab", 0x5a000050);
    print(1, 3, 10, 30, "a\tb\tc\td", 1, 2, 0);
    stmt1(SETTAB, 0, "FNSSetTab", 7);
    print(1, 0, 5, 40, "\ta\t\tb\t", 4, 3, 0);
    ROUT = "FNSOrigin";
    DO1(ENT(LIB, ORIGIN, 0), (void)0);
    print(1, 1, 10, 50, "origin 0", 2, 1, 0);

    /* FNSShadowPrint, three forms */
    for (f = 0; f <= 5; f++) {
        clear();
        print(10, (UWORD)f, 10, 5, "shadow 0", 0, 0, 0);
        print(11, (UWORD)f, 10, 20, "shadow 1\nnew line", 7, 3, 0);
        print(12, (UWORD)f, 100, 40, "shadow 2", 13, 2, 5);
        print(12, (UWORD)f, 100, 50, "", 4, 2, 5);
    }

    /* FNSLength: both forms (form 0 takes the last print's prefs), the
       unmasked number reading the block */
    for (t = 0; t < NTEXTS; t++)
        for (f = 0; f <= 6; f++) {
            length(1, 0x5a000000 | (ULONG)f, texts[t], (UWORD)(t * 3));
            length(0, 0x5a000000 | (ULONG)f, texts[t], 0);
        }
    print(1, 0, 10, 10, "bold", 2, 1, 0);
    length(0, 2, "bold?", 0);
    length(1, 0x10, "x", 0);
    length(1, 0x11, "x", 0);
    length(1, 0xffff, "x", 0);
    length(1, 0xfffe, "x", 0);

    /* FNSLoad / FNSUnload through the stubs */
    {
        static const char name[] = "RAM:font";
        static const ULONG slots[] = { 9, 0x5a000019, 3 };
        for (i = 0; i < 5; i++) {
            bloadfail = i == 3;
            startnull = i == 4;
            nflog = 0;
            ROUT = "FNSLoad";
            sprintf(VEC, "case %u", i);
            DO1(ENT(LIB, LOAD, 0), R.d[0] = (ULONG)name; R.d[1] = slots[i % 3]);
            OBS("d0", R.d[0]);
            OBS("calls", nflog);
            for (t = 0; t < (unsigned)nflog; t++) {
                OBS("call", flog[t].fn);
                OBS("call d0", flog[t].d0);
                OBS("call d1", flog[t].d1);
            }
            fontfuncs();
            print(1, 9, 30, 30, "loaded", 0, 1, 0);
        }
        nflog = 0;
        stmt1(UNLOAD, 0, "FNSUnload", 0x5a000019);
        OBS("calls", nflog);
        OBS("erase bank", flog[0].d0);
        fontfuncs();
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), (void)0);
    FreeMem(bmbuf, BMSZ);
    FreeMem(bank, 20000);
    for (f = 0; f < 5; f++)
        FreeMem(font[f], 20000);
    (void)VECSTR;
    return rwe_done(LIB);
}
