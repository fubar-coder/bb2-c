/*
 * rwe rishapeslib - the C build of rishapeslib (userlib 46) against the
 * reference build of its source (ref/rishapeslib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), method of rwecmp.h.
 *
 *   rishapeslib DIR     (DIR with rishapeslib, memlib, shapeslib,
 *                        ripacklib and the libraries they call, .obj/.orig)
 *
 * Each set has its own memlib, shapeslib and RIPackLib (the original set
 * the CD's binaries, the C set this build's), its own shape objects
 * (70), palette objects, bitmap object and files in RAM: (rwesh_o_N /
 * rwesh_c_N).  Compared: d0 of the functions and commands, the shape
 * objects (all fields but the data and cookie pointers; the data and the
 * cookie bytes; for linked shapes the offset of the data in the source
 * buffer), the palettes made by the loaders, the files LESaveShapes
 * writes (all bytes), the kept registers; memlib's memory all back at
 * the end.
 * Covered: init, LEInitShape (fresh, used: BUG(orig) the cookie stays),
 * LEScanShapes and LEMaxShape*, SafeToBlit (BUG(orig) shape/32 against
 * the maximum; no cookie, negative and too large coordinates),
 * LESaveShapes both forms (without and with a palette, packed with
 * packable and unpackable data - PackPBM's failure path -, the shape
 * count from the >>5 of the shape numbers, with the upper word of d1),
 * LELoadShapes all four forms (BUG(orig) form 3: the palette of d3),
 * a missing file, a file that is FORM but no LESH, a file in shapeslib's
 * format (BUG(orig): LoadShapes into the last shape), LECludgeShapes from
 * fast memory (copied) and chip memory (linked, "cL", and copied with
 * copymemflag -1), packed (copied), with and without a palette, a CMAP
 * of 7 bytes (BUG(orig): SDAT then not found), no FORM;
 * CludgeShapes/CludgeShapesToAddress with and without LEAutoCookie, an
 * empty shape in memory (BUG(orig): the stale size), 0 shapes, into used
 * and linked shapes; LECompressShapes; finit.
 * Not covered: the stack imbalances (LELoadShapes/LECludgeShapes on a
 * linked shape, failing allocations in the shape loop: jumps into data),
 * failing chunk allocations, a palette of 0 colours (65536 colours), the
 * runtime error of a failing pack buffer.
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "rishapeslib"
#define NSH 70

enum { CLUDGE, SAFE, LELOAD, LESAVE, COMPRESS, LECLUDGE, INITSHAPE, SCAN, MAXW, MAXH, MAXD,
       AUTOCOOKIE, TOADDR, FINIT, INIT };

static UBYTE shp[2][NSH][32];
static UBYTE pal[2][4][8];
static UBYTE bm[2][64];
static UBYTE *srcpal[2];                /* a palette to save (12 colours) */
static UBYTE *mem[2];                   /* sources of LECludgeShapes / CludgeShapes */
static ULONG memlen;
static char fname[2][40];

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))   /* word, marker above */

static const char *setname(int n)
{
    sprintf(fname[cur], "RAM:rwesh_%c_%d", cur ? 'c' : 'o', n);
    return fname[cur];
}

/* ------------------------------------------------------------ compare */

static void sameshape(int i)
{
    const UBYTE *a[2] = { shp[0][i], shp[1][i] };
    const UBYTE *b[2] = { shp[0][i] + 22, shp[1][i] + 22 };
    char w[60];

    sprintf(w, "shape %d fields", i);
    SAMEBYTES(w, a, 14);
    SAMEBYTES(w, b, 10);
    sprintf(w, "shape %d data set", i);
    SAME(w, SL(shp[cur][i], 14) != 0);
    sprintf(w, "shape %d cookie set", i);
    SAME(w, SL(shp[cur][i], 18) != 0);
    if (SL(shp[0][i], 14) && SL(shp[1][i], 14) && SW(shp[0][i], 26) == SW(shp[1][i], 26)) {
        const UBYTE *d[2] = { (UBYTE *)SL(shp[0][i], 14), (UBYTE *)SL(shp[1][i], 14) };
        sprintf(w, "shape %d data", i);
        SAMEBYTES(w, d, SW(shp[0][i], 26));
        if (SW(shp[0][i], 30) == 0x634c) {
            sprintf(w, "shape %d linked at", i);
            SAME(w, SL(shp[cur][i], 14) - (ULONG)mem[cur]);
        }
    }
    if (SL(shp[0][i], 18) && SL(shp[1][i], 18) && SW(shp[0][i], 22) == SW(shp[1][i], 22)) {
        const UBYTE *c[2] = { (UBYTE *)SL(shp[0][i], 18), (UBYTE *)SL(shp[1][i], 18) };
        sprintf(w, "shape %d cookie", i);
        SAMEBYTES(w, c, SW(shp[0][i], 22));
    }
}

static void sameshapes(int from, int to)
{
    int i;
    for (i = from; i <= to; i++)
        sameshape(i);
}

static void samepal(int p)
{
    char w[40];
    sprintf(w, "palette %d count", p);
    SAME(w, SW(pal[cur][p], 4));
    sprintf(w, "palette %d data set", p);
    SAME(w, SL(pal[cur][p], 0) != 0);
    if (SL(pal[0][p], 0) && SL(pal[1][p], 0)) {
        const UBYTE *d[2] = { (UBYTE *)SL(pal[0][p], 0), (UBYTE *)SL(pal[1][p], 0) };
        sprintf(w, "palette %d data", p);
        SAMEBYTES(w, d, 4 + 12 * (ULONG)SW(pal[0][p], 4));
    }
}

/* the files of both sets the same */
static UBYTE fbuf[2][30000];
static LONG flen[2];

static void readback(int n)
{
    BPTR f;
    for (cur = 0; cur < 2; cur++) {
        flen[cur] = -1;
        memset(fbuf[cur], 0, sizeof fbuf[cur]);
        if ((f = Open((STRPTR)setname(n), MODE_OLDFILE)) != 0) {
            flen[cur] = Read(f, fbuf[cur], sizeof fbuf[cur]);
            Close(f);
        }
    }
}

static void samefile(int n)
{
    const UBYTE *f[2] = { fbuf[0], fbuf[1] };
    readback(n);
    SAME("file length", flen[cur]);
    BOTH("file written", flen[cur] > 0);
    SAMEBYTES("file", f, sizeof fbuf[0]);
}

/* ------------------------------------------------------------- calls */

static void initshape(int s, UWORD w, UWORD h, UWORD d)
{
    ROUT = "LEInitShape";
    sprintf(VEC, "%d %ux%ux%u", s, w, h, d);
    DO2(ENT(LIB, INITSHAPE, 0), R[cur].d[0] = WA(s); R[cur].d[1] = WA(w);
        R[cur].d[2] = WA(h); R[cur].d[3] = WA(d); R[cur].a[2] = (ULONG)shp[cur][s]);
    sameshape(s);
}

/* the data of shape s: a pattern (the same in both sets) */
static void fillshape(int s, UBYTE seed, int runs)
{
    UWORD i, n = SW(shp[0][s], 26);
    for (cur = 0; cur < 2; cur++) {
        UBYTE *p = (UBYTE *)SL(shp[cur][s], 14);
        for (i = 0; i < n; i++)
            p[i] = runs ? (UBYTE)(i / 9 * 0x11 + seed) : (UBYTE)(seed + i * 37 + (i >> 3));
    }
}

static void scan(ULONG a, ULONG b)
{
    ROUT = "LEScanShapes";
    sprintf(VEC, "%lu..%lu", a, b);
    DO2(ENT(LIB, SCAN, 0), R[cur].d[0] = WA(a); R[cur].d[1] = WA(b);
        R[cur].a[2] = (ULONG)shp[cur][a]; R[cur].a[3] = (ULONG)shp[cur][b]);
    ROUT = "LEMaxShapeWidth";
    DO2(ENT(LIB, MAXW, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "LEMaxShapeHeight";
    DO2(ENT(LIB, MAXH, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "LEMaxShapeDepth";
    DO2(ENT(LIB, MAXD, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void safe(ULONG s, ULONG x, ULONG y, ULONG max)
{
    ROUT = "SafeToBlit";
    sprintf(VEC, "shape %lu at %ld,%ld max %lu", s, (LONG)(WORD)x, (LONG)(WORD)y, max);
    DO2(ENT(LIB, SAFE, 0), R[cur].d[0] = WA(s); R[cur].d[1] = WA(x); R[cur].d[2] = WA(y);
        R[cur].d[6] = WA(max); R[cur].a[3] = (ULONG)shp[cur][s % NSH];
        R[cur].a[2] = (ULONG)bm[cur]);
    SAME("d0", R[cur].d[0]);
}

static void compress(ULONG on)
{
    ROUT = "LECompressShapes";
    sprintf(VEC, "%ld", (LONG)(WORD)on);
    DO2(ENT(LIB, COMPRESS, 0), R[cur].d[0] = WA(on));
}

/* LESaveShapes s1,s2 (d1 with the upper word `up`), palette p (-1:
   form 0) into file n */
static void save(int n, int s1, int s2, ULONG up, int p)
{
    ROUT = "LESaveShapes";
    sprintf(VEC, "%d..%d up $%lx pal %d file %d", s1, s2, up, p, n);
    DO2(ENT(LIB, LESAVE, p < 0 ? 0 : 1), R[cur].d[0] = WA(s1);
        R[cur].d[1] = up << 16 | (UWORD)s2; R[cur].d[2] = (ULONG)setname(n);
        R[cur].d[3] = WA(p); R[cur].a[0] = p < 0 ? 0 : (ULONG)pal[cur][p];
        R[cur].a[2] = (ULONG)shp[cur][s1]; R[cur].a[3] = (ULONG)shp[cur][s2];
        R[cur].a[6] = (ULONG)DOSBase);
    samefile(n);
}

/* LELoadShapes, form 0/1 (s1,s2[,palette p]) or 2/3 (s, max[,palette]) */
static void load(int form, int n, int s1, int s2, int p, const char *name)
{
    ROUT = "LELoadShapes";
    sprintf(VEC, "form %d %d,%d pal %d file %d %s", form, s1, s2, p, n, name ? name : "");
    DO2(ENT(LIB, LELOAD, form),
        const char *fn = name ? name : setname(n);
        R[cur].d[0] = WA(s1);
        R[cur].a[2] = (ULONG)shp[cur][s1];
        R[cur].a[6] = (ULONG)DOSBase;
        R[cur].a[0] = p < 0 ? 0 : (ULONG)pal[cur][p];
        if (form < 2) {
            R[cur].d[1] = WA(s2); R[cur].d[2] = (ULONG)fn; R[cur].a[3] = (ULONG)shp[cur][s2];
        } else {
            /* the maximum in d4, a3 = shape 0 */
            R[cur].d[1] = (ULONG)fn; R[cur].d[4] = WA(s2); R[cur].a[3] = (ULONG)shp[cur][0];
        });
}

/* the file n into mem[] (chip or fast memory) */
static void tomem(int n, ULONG flags)
{
    readback(n);
    for (cur = 0; cur < 2; cur++) {
        if (mem[cur])
            FreeMem(mem[cur], memlen);
        mem[cur] = AllocMem(sizeof fbuf[0], flags | MEMF_CLEAR);
        memcpy(mem[cur], fbuf[cur], sizeof fbuf[0]);
    }
    memlen = sizeof fbuf[0];
}

static void lecludge(int form, int s1, int s2, int p, ULONG flag, const char *what)
{
    ROUT = "LECludgeShapes";
    sprintf(VEC, "form %d %d,%d pal %d flag %ld %s", form, s1, s2, p, (LONG)(WORD)flag, what);
    DO2(ENT(LIB, LECLUDGE, form), R[cur].d[0] = WA(s1); R[cur].d[1] = WA(s2);
        R[cur].d[2] = (ULONG)mem[cur]; R[cur].d[3] = WA(p); R[cur].d[4] = WA(flag);
        R[cur].a[0] = p < 0 ? 0 : (ULONG)pal[cur][p];
        R[cur].a[2] = (ULONG)shp[cur][s1]; R[cur].a[3] = (ULONG)shp[cur][s2];
        R[cur].a[6] = (ULONG)DOSBase);
}

static void cludge(int toaddr, int s, ULONG num, ULONG off, const char *what)
{
    ROUT = toaddr ? "CludgeShapesToAddress" : "CludgeShapes";
    sprintf(VEC, "%d num %lu at %lu %s", s, num, off, what);
    DO2(ENT(LIB, toaddr ? TOADDR : CLUDGE, 0),
        R[cur].d[0] = toaddr ? (ULONG)shp[cur][s] : (ULONG)s; R[cur].d[1] = num;
        R[cur].d[2] = (ULONG)mem[cur] + off; R[cur].a[0] = (ULONG)shp[cur][s];
        R[cur].a[6] = (ULONG)SysBase);
    SAME("d0", R[cur].d[0]);
}

static void autocookie(ULONG on)
{
    ROUT = "LEAutoCookie";
    sprintf(VEC, "%ld", (LONG)on);
    DO2(ENT(LIB, AUTOCOOKIE, 0), R[cur].d[0] = on);
}

/* shapes in memory in shapeslib's format (32 byte block + data) */
static ULONG ramshape(UBYTE *p, UWORD w, UWORD h, UWORD d, UBYTE seed)
{
    UWORD ebw = (UWORD)(((w + 15) >> 4) << 1), i;
    memset(p, 0, 32);
    SW(p, 0) = w;
    SW(p, 2) = h;
    SW(p, 4) = d;
    SW(p, 6) = ebw;
    SW(p, 8) = (UWORD)((ebw >> 1) + h * 64);
    SW(p, 10) = 3;
    SW(p, 12) = 2;
    SW(p, 22) = (UWORD)(ebw * h);
    SW(p, 24) = (UWORD)(ebw * h + 2 * h);
    SW(p, 26) = (UWORD)(ebw * h * d);
    SW(p, 28) = (UWORD)(2 * h * d + ebw * h * d);
    for (i = 0; i < SW(p, 26); i++)
        p[32 + i] = (UBYTE)(seed + i * 13);
    return 32 + (w && h ? SW(p, 26) : 0);
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

static APTR nullsub(const char *name, int off)
{
    struct lib *l = getlib(&sets[cur], name);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "memlib", "shapeslib", "ripacklib", "riamosfunclib",
                                   "bitmaplib", "environlib", "iffiolib", "ilbmifflib",
                                   "intlib", "palettelib", "scrolllib", NULL };
    ULONG avail, off;
    int i, s;

    if (argc < 2) {
        printf("usage: rishapeslib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib/shapeslib init";
    memlib(7);
    for (cur = 0; cur < 2; cur++) {
        struct regs r;
        markregs(&r);
        callregs(nullsub("shapeslib", 22), &r, 0, 0);
    }
    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);
    for (s = 0; s < 2; s++) {
        SW(bm[s], 56) = 320;
        SW(bm[s], 58) = 200;
        srcpal[s] = AllocMem(4 + 12 * 12, MEMF_CLEAR);
        SW(srcpal[s], 0) = 12;
        for (i = 0; i < 36; i++)
            SL(srcpal[s], 4 + 4 * i) = 0x01010101UL * (UBYTE)(i * 7 + 3);
        SL(pal[s][0], 0) = (ULONG)srcpal[s];
        SW(pal[s][0], 4) = 12;
    }

    /* LEInitShape, the data filled */
    initshape(0, 20, 10, 2);
    initshape(1, 16, 8, 1);             /* 16 bytes: PackPBM's failure stays inside */
    initshape(2, 33, 5, 3);
    initshape(3, 1, 30, 1);
    initshape(2, 40, 6, 2);             /* used: the old data freed */
    fillshape(0, 0x11, 1);
    fillshape(1, 0x22, 0);
    fillshape(2, 0x33, 1);
    fillshape(3, 0x44, 0);
    scan(0, 3);
    scan(1, 1);
    scan(4, 5);                         /* empty shapes */

    /* SafeToBlit */
    safe(0, 0, 0, 10);
    safe(0, 300, 190, 10);
    safe(0, 301, 190, 10);
    safe(0, 300, 191, 10);
    safe(0, 0xffff, 0, 10);
    safe(0, 0, 0x8000, 10);
    safe(1, 304, 199, 10);
    safe(4, 0, 0, 10);                  /* no cookie */
    safe(31, 0, 0, 1);                  /* 31/32 = 0 < 1 */
    safe(32, 0, 0, 1);                  /* 32/32 = 1: not below the maximum */
    safe(0, 0, 0, 0);
    safe(0, 0, 0, 0xffff);

    /* LESaveShapes, LELoadShapes */
    save(0, 0, 3, 0, -1);               /* count 0 */
    save(1, 0, 3, 0, 0);                /* with the palette */
    printf("rishapeslib: LESaveShapes with a palette: %s\n",
           fbuf[0][40] == 'C' ? "CMAP written" : "no CMAP (Write's Z flag)");
    save(2, 0, 64, 0, 0);               /* count 2 */
    save(3, 0, 3, 0x5A01, -1);          /* the upper word in the count */
    compress(0xffff);
    save(4, 0, 64, 0, 0);               /* packed: shapes 0, 2 pack, 1, 3 not */
    compress(0);
    save(5, 2, 2, 0, 0);

    load(1, 0, 10, 13, -1, NULL);
    sameshapes(10, 13);
    BOTH("one shape loaded", SW(shp[cur][10], 0) == 20 && !SW(shp[cur][11], 0));
    load(0, 1, 14, 17, 1, NULL);
    sameshapes(14, 17);
    samepal(1);
    load(0, 2, 20, 23, 1, NULL);        /* 3 shapes; the old palette freed */
    sameshapes(20, 23);
    samepal(1);
    load(1, 4, 24, 27, -1, NULL);       /* packed */
    sameshapes(24, 27);
    BOTH("packed shapes loaded", SW(shp[cur][24], 0) == 20 && SW(shp[cur][26], 0) == 40);
    load(1, 2, 10, 13, -1, NULL);       /* into used shapes */
    sameshapes(10, 13);
    load(2, 5, 30, 34, -1, NULL);       /* to the maximum - 1: shape 33 */
    sameshapes(30, 34);
    load(3, 1, 35, 37, 2, NULL);        /* the palette of d3 */
    sameshapes(35, 37);
    samepal(2);
    load(1, 0, 40, 41, -1, "RAM:rwesh_none");
    sameshapes(40, 41);

    /* files that are no LE file */
    for (cur = 0; cur < 2; cur++) {
        BPTR f = Open((STRPTR)"RAM:rwesh_form", MODE_NEWFILE);
        static const char form[] = "FORMxxxxILBMBMHD\0\0\0\0";
        Write(f, (APTR)form, sizeof form);
        Close(f);
    }
    load(1, 0, 40, 41, -1, "RAM:rwesh_form");
    sameshapes(40, 41);
    {
        static UBYTE acid[600];
        ULONG n = ramshape(acid, 24, 7, 2, 0x5c);
        for (cur = 0; cur < 2; cur++) {
            BPTR f = Open((STRPTR)"RAM:rwesh_acid", MODE_NEWFILE);
            Write(f, acid, n);
            Close(f);
        }
    }
    load(1, 0, 40, 43, -1, "RAM:rwesh_acid");
    sameshapes(40, 43);
    BOTH("LoadShapes into the last shape", SW(shp[cur][43], 0) == 24 && !SW(shp[cur][40], 0));

    /* LECludgeShapes */
    tomem(2, MEMF_FAST);
    lecludge(0, 44, 47, -1, 0, "fast");
    sameshapes(44, 47);
    tomem(1, MEMF_CHIP);
    lecludge(1, 48, 50, 3, 0xffff, "chip, copied");
    sameshapes(48, 50);
    samepal(3);
    tomem(4, MEMF_CHIP);
    lecludge(1, 51, 54, -1, 0, "chip, packed");
    sameshapes(51, 54);
    tomem(2, MEMF_CHIP);
    lecludge(0, 55, 57, 3, 0, "chip, linked");
    sameshapes(55, 57);
    samepal(3);
    BOTH("linked", SW(shp[cur][55], 30) == 0x634c);
    initshape(55, 8, 8, 1);             /* a linked shape: not freed */
    /* CMAP of 7 bytes: the colours end at +6, SDAT is not found */
    for (cur = 0; cur < 2; cur++)
        SL(mem[cur], 44) = 7;
    lecludge(0, 58, 59, 3, 0, "CMAP 7");
    sameshapes(58, 59);
    samepal(3);
    for (cur = 0; cur < 2; cur++)
        SL(mem[cur], 0) = 0x464f524e;   /* FORN */
    lecludge(0, 58, 59, -1, 0, "no FORM");
    sameshapes(58, 59);

    /* CludgeShapes */
    for (cur = 0; cur < 2; cur++) {
        if (mem[cur])
            FreeMem(mem[cur], memlen);
        mem[cur] = AllocMem(4096, MEMF_CLEAR);
    }
    memlen = 4096;
    for (cur = 0; cur < 2; cur++) {
        /* an empty shape advances by 32 + the size of the one before
           (BUG(orig)): padding of that size behind each */
        off = ramshape(mem[cur], 20, 4, 2, 1);                  /* 32 bytes */
        off += ramshape(mem[cur] + off, 0, 4, 2, 2) + 32;       /* empty */
        off += ramshape(mem[cur] + off, 17, 3, 3, 3);           /* 36 bytes */
        off += ramshape(mem[cur] + off, 8, 0, 1, 4) + 36;       /* empty */
        off += ramshape(mem[cur] + off, 64, 2, 1, 5);
    }
    cludge(0, 60, 0, 0, "none");
    cludge(0, 60, 3, 0, "");
    sameshapes(60, 62);
    cludge(0, 60, 5, 0, "again, used");
    sameshapes(60, 64);
    BOTH("the stale size skips to the next shape",
         SW(shp[cur][62], 0) == 17 && SW(shp[cur][64], 0) == 64);
    autocookie(0);
    cludge(1, 65, 1, 0, "no cookie");
    sameshapes(65, 65);
    autocookie(1);
    cludge(1, 66, 2, 0, "");
    sameshapes(66, 67);
    cludge(0, 56, 1, 0, "into a linked shape");
    sameshapes(56, 56);

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), R[cur].a[6] = (ULONG)DOSBase);

    ROUT = "memlib finit";
    memlib(8);
    for (s = 0; s < 2; s++) {
        FreeMem(srcpal[s], 4 + 12 * 12);
        FreeMem(mem[s], memlen);
    }
    for (cur = 0; cur < 2; cur++)
        for (i = 0; i < 6; i++)
            DeleteFile((STRPTR)setname(i));
    DeleteFile((STRPTR)"RAM:rwesh_form");
    DeleteFile((STRPTR)"RAM:rwesh_acid");
    if (AvailMem(MEMF_ANY) != avail)
        printf("rishapeslib: %ld bytes missing\n", (LONG)(avail - AvailMem(MEMF_ANY)));
    check("rishapeslib: all memory back", AvailMem(MEMF_ANY) == avail);
    return rwe_done(LIB);
}
