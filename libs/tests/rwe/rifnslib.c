/*
 * rwe rifnslib - the FNS library: the C build against the CD original
 * (BSS2 RIFNSLIB.OBJ), method of rwecmp.h.
 *
 *   rifnslib DIR     (DIR with rifnslib .obj/.orig)
 *
 * Text is printed into a bitmap in memory (per set one buffer with the
 * planes and guards, filled the same): compared after every call are
 * the whole buffer, FNSPrint's end position (the words at a0), d0 and the
 * registers the original keeps.  Fonts: the built-in one (slot 0) and
 * fonts made here, 16, 32, 48 and 64 pixels wide (random glyphs, widths
 * 0..width).  RIAmosFuncLib is not loaded: stubs (sysload_stubs) take
 * the place of BLoad, Start and Erase for both sets and record the calls.
 *
 * Covered: FNSOutput both forms, depths 1..8; FNSClip/FNSClipOutput;
 * FNSPrint/FNSShadowPrint all forms with all 16 prefs (centred, bold,
 * underlined, right aligned), colours, chr$(1) colour changes, tabs
 * (FNSSetTab), new lines (also leading and double ones: the binary's
 * missing guard), zero-width characters, codes from 160 (end), x on
 * every bit position, near xmax, below xmin, rows clipped at ymin/ymax
 * (the 48/64 pixel BUG(orig)), an empty text (underline 1 pixel, the
 * centring by -32767), FNSOrigin; FNSLength both forms (the prefs of the
 * last FNSPrint) and its unmasked font number (16, 17, -1, -2: the
 * variables and the font of the block); FNSHeight/Width/Underline,
 * InstallFNS (and a non-FNS address), RemoveFNS, FNSSlot, FNSInk,
 * FNSPrefs, FNSVersion (inline), FNSLoad (ok, BLoad fails, Start 0) and
 * FNSUnload.  Not covered: FNSPrint without a bitmap (a crash), depth 0
 * or above 8 and bitmaps with flags (the bitstack overrun), FNSSetTab 0
 * (division by zero), x below 0 with xmin below 0 (writes 64 KB away).
 */
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

static UBYTE *bmbuf[2];                 /* the planes */
static UWORD bm[2][4 + 16];             /* struct BitMap */
static UBYTE *font[2][5];               /* the test fonts */
static ULONG fontsz[5];
static const char *VECSTR;

/* -------------------------------------------------------- RIAmosFuncLib */

enum { BLOAD = 1, START, ERASE };
static struct { int fn; ULONG d0, d1; } flog[2][8];
static int nflog[2], bloadfail, startnull;
static UBYTE *bank[2];

static void flogit(int fn, ULONG d0, ULONG d1)
{
    if (nflog[cur] < 8) {
        flog[cur][nflog[cur]].fn = fn;
        flog[cur][nflog[cur]].d0 = d0;
        flog[cur][nflog[cur]].d1 = d1;
        nflog[cur]++;
    }
}

static ULONG __attribute__((used)) st_bload(ULONG name __asm("d0"), ULONG b __asm("d1"))
{
    flogit(BLOAD, strcmp((const char *)name, "RAM:font"), b);
    if (bloadfail)
        return 0;
    memcpy(bank[cur], font[cur][1], fontsz[1]);
    return (ULONG)-1;
}

static ULONG __attribute__((used)) st_start(ULONG b __asm("d0"))
{
    flogit(START, b, 0);
    return startnull ? 0 : (ULONG)bank[cur];
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

static ULONG fnsslot(int s)
{
    struct regs r;
    ULONG popped;
    markregs(&r);
    callstk(entry_form(getlib(&sets[s], LIB), SLOT, 0), &r, NULL, 0, &popped);
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
    SAMEBYTES("bitmap", bmbuf, BMSZ);
}

static void output(int depth, int form)
{
    int s, i;
    for (s = 0; s < 2; s++) {
        bm[s][0] = BPR;
        bm[s][1] = ROWS;
        bm[s][2] = (UWORD)depth;        /* Flags 0, Depth */
        bm[s][3] = 0;
        for (i = 0; i < 8; i++)
        {
            ULONG pa = (ULONG)(bmbuf[s] + GUARD + i * PLANE);
            bm[s][4 + 2 * i] = (UWORD)(pa >> 16);
            bm[s][5 + 2 * i] = (UWORD)pa;
        }
    }
    ROUT = "FNSOutput";
    sprintf(VEC, "depth %d form %d", depth, form);
    DO2(ENT(LIB, OUTPUT, form), R[cur].a[0] = (ULONG)bm[cur]; R[cur].d[0] = 0x5a000003;
        R[cur].d[1] = form ? 0x5a000001 : 0x5a000000);
}

static void clip(WORD x0, WORD y0, WORD x1, WORD y1)
{
    ROUT = "FNSClip";
    sprintf(VEC, "%d,%d,%d,%d", x0, y0, x1, y1);
    DO2(ENT(LIB, CLIP, 0), R[cur].d[0] = 0x5a000000 | (UWORD)x0;
        R[cur].d[1] = 0x5a000000 | (UWORD)y0; R[cur].d[2] = 0x5a000000 | (UWORD)x1;
        R[cur].d[3] = 0x5a000000 | (UWORD)y1);
}

static void stmt1(int e, int form, const char *name, ULONG d0)
{
    ROUT = name;
    sprintf(VEC, "%lX", d0);
    DO2(ENT(LIB, e, form), R[cur].d[0] = d0);
}

/* the end position at a0 (relative to the block), or a0 itself */
static void endpos(ULONG a0in)
{
    SAME("a0", R[cur].a[0] == a0in ? 1 : R[cur].a[0] - fnsslot(cur));
    if (R[0].a[0] != a0in && R[1].a[0] != a0in) {
        SAME("end x", *(UWORD *)R[cur].a[0]);
        SAME("end y", *(UWORD *)(R[cur].a[0] + 2));
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
    DO2(ENT(LIB, e, f), R[cur].d[0] = 0x5a000000 | fnt; R[cur].d[1] = 0x5a000000 | (UWORD)x;
        R[cur].d[2] = 0x5a000000 | (UWORD)y; R[cur].d[3] = (ULONG)str;
        if (f >= 1) { R[cur].d[4] = 0x5a000000 | prefs; R[cur].d[5] = 0x5a000000 | colour; }
        if (f >= 2) R[cur].d[6] = 0x5a000000 | shadow);
    endpos(MARK(8));
    same_bitmap();
}

static void clear(void)
{
    int s;
    for (s = 0; s < 2; s++)
        memset(bmbuf[s], 0x5a, BMSZ);
}

static void install(int slot, int f)
{
    ROUT = "InstallFNS";
    sprintf(VEC, "slot %d font %d", slot, f);
    DO2(ENT(LIB, INSTALL, 0), R[cur].d[0] = (ULONG)slot; R[cur].d[1] = (ULONG)font[cur][f]);
    SAME("d0", R[cur].d[0]);
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
            DO2(ENT(LIB, en[k], 0), R[cur].d[0] = n[i]);
            SAME("d0", R[cur].d[0]);
        }
}

static void length(int form, ULONG n, const char *str, UWORD prefs)
{
    ROUT = "FNSLength";
    snprintf(VEC, sizeof VEC, "f%d font %lX \"%.20s\" prefs %X", form, n, str, prefs);
    DO2(ENT(LIB, LENGTH, form), R[cur].d[0] = n; R[cur].d[1] = (ULONG)str;
        if (form) R[cur].d[2] = 0x5a000000 | prefs);
    SAME("d0", R[cur].d[0]);
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
    int s, f, d;

    if (argc < 2) {
        printf("usage: rifnslib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_sets(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    for (s = 0; s < 2; s++) {
        bmbuf[s] = AllocMem(BMSZ, MEMF_PUBLIC);
        bank[s] = AllocMem(20000, MEMF_PUBLIC | MEMF_CLEAR);
    }
    for (f = 0; f < 5; f++) {
        ULONG seed = rwe_seed;
        for (s = 0; s < 2; s++) {
            rwe_seed = seed;
            font[s][f] = AllocMem(20000, MEMF_PUBLIC);
            fontsz[f] = mkfont(font[s][f], fws[f], fhs[f], (UWORD)(fhs[f] - 2 + f % 2 * 4));
        }
    }
    clear();

    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);
    BOTH("slot 0 = the built-in font", *(ULONG *)fnsslot(cur) == fnsslot(cur) + 64);
    {
        const UBYTE *df[2];
        for (s = 0; s < 2; s++)
            df[s] = (const UBYTE *)(fnsslot(s) + 64);
        SAMEBYTES("built-in font", df, 1712);
    }
    {   /* FNSVersion: inline, the code behind $A000,len */
        ROUT = "FNSVersion";
        BOTH("inline", W(ENT(LIB, VERSION, 0)) == 0xa000);
        SAME("inline length", W(ENT(LIB, VERSION, 0) + 2));
        DO2(ENT(LIB, VERSION, 0) + 4, (void)0);
        SAME("d0", R[cur].d[0]);
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
    DO2(ENT(LIB, INSTALL, 0), R[cur].d[0] = 6; R[cur].d[1] = (ULONG)bmbuf[cur]);
    SAME("d0", R[cur].d[0]);
    fontfuncs();
    stmt1(REMOVE, 0, "RemoveFNS", 0x5a000017);
    fontfuncs();

    /* depths, both FNSOutput forms, every font and text */
    for (d = 1; d <= 8; d++) {
        output(d, d & 1);
        if (!(d & 1)) {
            ROUT = "FNSClipOutput";
            strcpy(VEC, "");
            DO2(ENT(LIB, CLIPOUT, 0), (void)0);
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
        DO2(ENT(LIB, CLIPOUT, 0), (void)0);
    }

    /* settings: ink, prefs, origin, tab size; the forms without them */
    clear();
    stmt1(INK, 0, "FNSInk", 0x5a000006);
    stmt1(PREFS, 0, "FNSPrefs", 0x5a000005);
    print(0, 0, 10, 10, "ink and prefs", 0, 0, 0);
    ROUT = "FNSPrefs";
    DO2(ENT(LIB, PREFS, 1), R[cur].d[0] = 0x5a00000a; R[cur].d[1] = 0x5a000003);
    print(0, 2, 200, 10, "prefs 2", 0, 0, 0);
    ROUT = "FNSOrigin";
    DO2(ENT(LIB, ORIGIN, 1), R[cur].d[0] = 0x5a000011; R[cur].d[1] = 0x5a000007);
    print(0, 1, 10, 20, "origin", 0, 0, 0);
    stmt1(SETTAB, 0, "FNSSetTab", 0x5a000050);
    print(1, 3, 10, 30, "a\tb\tc\td", 1, 2, 0);
    stmt1(SETTAB, 0, "FNSSetTab", 7);
    print(1, 0, 5, 40, "\ta\t\tb\t", 4, 3, 0);
    ROUT = "FNSOrigin";
    DO2(ENT(LIB, ORIGIN, 0), (void)0);
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
            nflog[0] = nflog[1] = 0;
            ROUT = "FNSLoad";
            sprintf(VEC, "case %u", i);
            DO2(ENT(LIB, LOAD, 0), R[cur].d[0] = (ULONG)name; R[cur].d[1] = slots[i % 3]);
            SAME("d0", R[cur].d[0]);
            SAME("calls", nflog[cur]);
            for (t = 0; t < (unsigned)nflog[0] && t < (unsigned)nflog[1]; t++) {
                SAME("call", flog[cur][t].fn);
                SAME("call d0", flog[cur][t].d0);
                SAME("call d1", flog[cur][t].d1);
            }
            fontfuncs();
            print(1, 9, 30, 30, "loaded", 0, 1, 0);
        }
        nflog[0] = nflog[1] = 0;
        stmt1(UNLOAD, 0, "FNSUnload", 0x5a000019);
        SAME("calls", nflog[cur]);
        SAME("erase bank", flog[cur][0].d0);
        fontfuncs();
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    for (s = 0; s < 2; s++) {
        FreeMem(bmbuf[s], BMSZ);
        FreeMem(bank[s], 20000);
        for (f = 0; f < 5; f++)
            FreeMem(font[s][f], 20000);
    }
    (void)VECSTR;
    return rwe_done(LIB);
}
