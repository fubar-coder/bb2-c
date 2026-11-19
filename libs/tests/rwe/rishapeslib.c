/*
 * rwe rishapeslib - the C build of rishapeslib (userlib 46) or the
 * reference build of its source (ref/rishapeslib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), one set as LIBSET says, observed as
 * cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_rishapeslib.txt).
 *
 *   rishapeslib DIR     (DIR with rishapeslib, memlib, shapeslib,
 *                        ripacklib and the libraries they call, .obj/.orig)
 *
 * The set has its own memlib, shapeslib and RIPackLib (the original set
 * the CD's binaries, the C set this build's); the test has the shape
 * objects (70), palette objects, a bitmap object and files in RAM:
 * (rwesh_N).  Observed: d0 of the functions and commands, the shape
 * objects (all fields but the data and cookie pointers; the data and the
 * cookie bytes; for linked shapes the offset of the data in the source
 * buffer), the palettes made by the loaders, the files LESaveShapes
 * writes (all bytes), the kept registers; memlib's memory all back at
 * the end.
 * The shape numbers in d0/d1 of SafeToBlit, LESaveShapes and LELoadShapes
 * are passed as compiled code passes them (shifted by 5, OBJ), and
 * LELoadShapes reaches shapeslib's LoadShapes through a stub as compiled
 * code does (lsh_stub).
 * Covered: init, LEInitShape (fresh, used: fixed #810, the old cookie freed),
 * LEScanShapes and LEMaxShape*, SafeToBlit (beyond the maximum, no cookie,
 * negative and too large coordinates),
 * LESaveShapes both forms (without and with a palette, packed with
 * packable and unpackable data - PackPBM's failure path -, the shape
 * count, with an upper word in d1: BUG(orig) #815; with dos Write
 * returning Z set: fixed #814, and globfree leaving an upper word in
 * d0: BUG(orig) #817, both as the system may leave them, D-019),
 * LELoadShapes all four forms (form 3, which compiled code never reaches,
 * called directly with a0 set),
 * a missing file, a file that is FORM but no LESH, a file in shapeslib's
 * format (LoadShapes into the first shape), LECludgeShapes from
 * fast memory (copied) and chip memory (linked, "cL", and copied with
 * copymemflag -1), packed (copied), with and without a palette, a CMAP
 * of 7 bytes (fixed #811: passed by its length), no FORM;
 * CludgeShapes/CludgeShapesToAddress with and without LEAutoCookie, an
 * empty shape in memory (fixed #809: the stale size skipped the padding), 0 shapes, into used
 * and linked shapes; LECompressShapes; finit; a second run (finit and init
 * again: LEMaxShape* and LEAutoCookie before anything else).
 * Not covered: LELoadShapes/LECludgeShapes on a linked shape (the
 * original returns past its caller; fixed, #807: bug-orig test
 * rishapeslib-cludged-shape-pop), failing allocations in the shape loop
 * (a jump into data), failing chunk allocations, a palette of 0 colours (65536 colours), the
 * runtime error of a failing pack buffer.
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "rishapeslib"
#define NSH 70

enum { CLUDGE, SAFE, LELOAD, LESAVE, COMPRESS, LECLUDGE, INITSHAPE, SCAN, MAXW, MAXH, MAXD,
       AUTOCOOKIE, TOADDR, FINIT, INIT };

static UBYTE shp[NSH][32];
static UBYTE pal[4][8];
static UBYTE bm[64];
static UBYTE *srcpal;                   /* a palette to save (12 colours) */
static UBYTE *mem;                      /* sources of LECludgeShapes / CludgeShapes */
static ULONG memlen;
static char fname[40];

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))   /* word, marker above */
/* a shape number in a #pd register as compiled code passes it: the
   compiler shifts it in place by the object's size shift (LSL.w #5,
   libcall.c toreg) before it adds it to the #ia register */
#define OBJ(n) ((UWORD)((n) << 5))

/* shapeslib's LoadShapes form 1 as compiled code reaches it from
   LELoadShapes: through the ALibJsr stub that loads the callee's !libs
   first (#ia3|#pd0, #ia2|#pd1, #doslib,#la6; docs/libs/cmdlibs-port.md,
   editlib #130), so a3 and a2 are the shapes of the numbers in d0 and
   d1, whatever a2/a3 the library leaves.  link_loadshapes_stub links
   rishapeslib's call to this stub. */
UBYTE *lsh_base;                        /* shape 0 */
APTR lsh_loadshapes;                    /* LoadShapes form 1 */
void lsh_stub(void);
__asm__("   .text\n"
        "   .even\n"
        "_lsh_stub:\n"
        "   move.l  _lsh_base,a3\n"
        "   lsl.w   #5,d0\n"
        "   adda.w  d0,a3\n"
        "   move.l  _lsh_base,a2\n"
        "   lsl.w   #5,d1\n"
        "   adda.w  d1,a2\n"
        "   move.l  _DOSBase,a6\n"
        "   move.l  _lsh_loadshapes,-(sp)\n"
        "   rts\n");

/* What the system leaves (D-019), for one LESaveShapes each:
   - dos Write returning with the Z flag set (wz_on): the original's
     _saveCMAP tests that flag instead of the palette pointer (#814);
   - memlib's globfree returning with 1 in the upper word of d0 (mf_on),
     as exec's FreeMem may leave it: the original's _writeshapes sizes
     its header buffer with that upper word (#817); the sizes of
     rishapeslib's globalloc calls are noted meanwhile (ma_size).
   wz_stub is Write while the test runs (SetFunction); rishapeslib's
   links to globalloc/globfree go through ma_stub/mf_stub. */
ULONG wz_on, mf_on;
APTR wz_old, ma_old, mf_old;
static ULONG ma_size[8], ma_n;
void wz_stub(void), ma_stub(void), mf_stub(void);
void __attribute__((used)) ma_note(ULONG size)
{
    if (ma_n < 8)
        ma_size[ma_n++] = size;
}
__asm__("   .text\n"
        "   .even\n"
        "_wz_stub:\n"
        "   tst.l   _wz_on\n"
        "   bne.s   1f\n"
        "   move.l  _wz_old,-(sp)\n"
        "   rts\n"
        "1: pea     2f(pc)\n"
        "   move.l  _wz_old,-(sp)\n"
        "   rts\n"
        "2: cmp.l   d0,d0\n"
        "   rts\n"
        "_ma_stub:\n"
        "   tst.l   _mf_on\n"
        "   beq.s   1f\n"
        "   movem.l d0-d1/a0-a1,-(sp)\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     _ma_note\n"
        "   addq.l  #4,sp\n"
        "   movem.l (sp)+,d0-d1/a0-a1\n"
        "1: move.l  _ma_old,-(sp)\n"
        "   rts\n"
        "_mf_stub:\n"
        "   tst.l   _mf_on\n"
        "   bne.s   1f\n"
        "   move.l  _mf_old,-(sp)\n"
        "   rts\n"
        "1: pea     2f(pc)\n"
        "   move.l  _mf_old,-(sp)\n"
        "   rts\n"
        "2: swap    d0\n"
        "   move.w  #1,d0\n"
        "   swap    d0\n"
        "   rts\n");

/* rishapeslib's links to the routine `to` go through `stub`; the count */
static int relink(ULONG to, APTR stub)
{
    struct lib *l = getlib(&loaded, LIB);
    ULONG r;
    int n = 0;

    for (r = 0; r < l->nrelocs; r++) {
        ULONG *at = (ULONG *)(l->code + l->relocs[r]);
        if (*at == to) {
            *at = (ULONG)stub;
            n++;
        }
    }
    CacheClearU();
    return n;
}

static const char *setname(int n)
{
    sprintf(fname, "RAM:rwesh_%d", n);
    return fname;
}

/* ------------------------------------------------------------ observe */

static void sameshape(int i)
{
    char w[60];

    sprintf(w, "shape %d fields", i);
    OBSBYTES(w, shp[i], 14);
    OBSBYTES(w, shp[i] + 22, 10);
    sprintf(w, "shape %d data set", i);
    OBS(w, SL(shp[i], 14) != 0);
    sprintf(w, "shape %d cookie set", i);
    OBS(w, SL(shp[i], 18) != 0);
    if (SL(shp[i], 14)) {
        sprintf(w, "shape %d data", i);
        OBSBYTES(w, (UBYTE *)SL(shp[i], 14), SW(shp[i], 26));
        if (SW(shp[i], 30) == 0x634c) {
            sprintf(w, "shape %d linked at", i);
            OBS(w, SL(shp[i], 14) - (ULONG)mem);
        }
    }
    if (SL(shp[i], 18)) {
        sprintf(w, "shape %d cookie", i);
        OBSBYTES(w, (UBYTE *)SL(shp[i], 18), SW(shp[i], 22));
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
    OBS(w, SW(pal[p], 4));
    sprintf(w, "palette %d data set", p);
    OBS(w, SL(pal[p], 0) != 0);
    if (SL(pal[p], 0)) {
        sprintf(w, "palette %d data", p);
        OBSBYTES(w, (UBYTE *)SL(pal[p], 0), 4 + 12 * (ULONG)SW(pal[p], 4));
    }
}

/* the file read back */
static UBYTE fbuf[30000];
static LONG flen;

static void readback(int n)
{
    BPTR f;
    flen = -1;
    memset(fbuf, 0, sizeof fbuf);
    if ((f = Open((STRPTR)setname(n), MODE_OLDFILE)) != 0) {
        flen = Read(f, fbuf, sizeof fbuf);
        Close(f);
    }
}

static void samefile(int n)
{
    readback(n);
    OBS("file length", flen);
    BOTH("file written", flen > 0);
    OBSBYTES("file", fbuf, sizeof fbuf);
}

/* ------------------------------------------------------------- calls */

static void initshape(int s, UWORD w, UWORD h, UWORD d)
{
    ROUT = "LEInitShape";
    sprintf(VEC, "%d %ux%ux%u", s, w, h, d);
    DO1(ENT(LIB, INITSHAPE, 0), R.d[0] = WA(s); R.d[1] = WA(w);
        R.d[2] = WA(h); R.d[3] = WA(d); R.a[2] = (ULONG)shp[s]);
    sameshape(s);
}

/* the data of shape s: a pattern */
static void fillshape(int s, UBYTE seed, int runs)
{
    UWORD i, n = SW(shp[s], 26);
    UBYTE *p = (UBYTE *)SL(shp[s], 14);
    for (i = 0; i < n; i++)
        p[i] = runs ? (UBYTE)(i / 9 * 0x11 + seed) : (UBYTE)(seed + i * 37 + (i >> 3));
}


static void scan(ULONG a, ULONG b)
{
    ROUT = "LEScanShapes";
    sprintf(VEC, "%lu..%lu", a, b);
    DO1(ENT(LIB, SCAN, 0), R.d[0] = WA(a); R.d[1] = WA(b);
        R.a[2] = (ULONG)shp[a]; R.a[3] = (ULONG)shp[b]);
    ROUT = "LEMaxShapeWidth";
    DO1(ENT(LIB, MAXW, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "LEMaxShapeHeight";
    DO1(ENT(LIB, MAXH, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "LEMaxShapeDepth";
    DO1(ENT(LIB, MAXD, 0), (void)0);
    OBS("d0", R.d[0]);
}

static void safe(ULONG s, ULONG x, ULONG y, ULONG max)
{
    ROUT = "SafeToBlit";
    sprintf(VEC, "shape %lu at %ld,%ld max %lu", s, (LONG)(WORD)x, (LONG)(WORD)y, max);
    DO1(ENT(LIB, SAFE, 0), R.d[0] = WA(OBJ(s)); R.d[1] = WA(x); R.d[2] = WA(y);
        R.d[6] = WA(max); R.a[3] = (ULONG)shp[s % NSH];
        R.a[2] = (ULONG)bm);
    OBS("d0", R.d[0]);
}

static void compress(ULONG on)
{
    ROUT = "LECompressShapes";
    sprintf(VEC, "%ld", (LONG)(WORD)on);
    DO1(ENT(LIB, COMPRESS, 0), R.d[0] = WA(on));
}

/* LESaveShapes s1,s2 (d1 with the upper word `up`), palette p (-1:
   form 0) into file n */
static void save(int n, int s1, int s2, ULONG up, int p)
{
    ROUT = "LESaveShapes";
    sprintf(VEC, "%d..%d up $%lx pal %d file %d", s1, s2, up, p, n);
    DO1(ENT(LIB, LESAVE, p < 0 ? 0 : 1), R.d[0] = WA(OBJ(s1));
        R.d[1] = up << 16 | OBJ(s2); R.d[2] = (ULONG)setname(n);
        R.d[3] = WA(p); R.a[0] = p < 0 ? 0 : (ULONG)pal[p];
        R.a[2] = (ULONG)shp[s1]; R.a[3] = (ULONG)shp[s2];
        R.a[6] = (ULONG)DOSBase);
    samefile(n);
}

/* LELoadShapes, form 0/1 (s1,s2[,palette p]) or 2/3 (s, max[,palette]) */
static void load(int form, int n, int s1, int s2, int p, const char *name)
{
    ROUT = "LELoadShapes";
    sprintf(VEC, "form %d %d,%d pal %d file %d %s", form, s1, s2, p, n, name ? name : "");
    DO1(ENT(LIB, LELOAD, form),
        const char *fn = name ? name : setname(n);
        R.d[0] = WA(OBJ(s1));
        R.a[2] = (ULONG)shp[s1];
        R.a[6] = (ULONG)DOSBase;
        R.a[0] = p < 0 ? 0 : (ULONG)pal[p];
        if (form < 2) {
            R.d[1] = WA(OBJ(s2)); R.d[2] = (ULONG)fn; R.a[3] = (ULONG)shp[s2];
        } else {
            /* the maximum in d4, a3 = shape 0 */
            R.d[1] = (ULONG)fn; R.d[4] = WA(s2); R.a[3] = (ULONG)shp[0];
        });
}

/* the file n into mem[] (chip or fast memory) */
static void tomem(int n, ULONG flags)
{
    readback(n);
    if (mem)
        FreeMem(mem, memlen);
    mem = AllocMem(sizeof fbuf, flags | MEMF_CLEAR);
    memcpy(mem, fbuf, sizeof fbuf);
    memlen = sizeof fbuf;
}

static void lecludge(int form, int s1, int s2, int p, ULONG flag, const char *what)
{
    ROUT = "LECludgeShapes";
    sprintf(VEC, "form %d %d,%d pal %d flag %ld %s", form, s1, s2, p, (LONG)(WORD)flag, what);
    DO1(ENT(LIB, LECLUDGE, form), R.d[0] = WA(s1); R.d[1] = WA(s2);
        R.d[2] = (ULONG)mem; R.d[3] = WA(p); R.d[4] = WA(flag);
        R.a[0] = p < 0 ? 0 : (ULONG)pal[p];
        R.a[2] = (ULONG)shp[s1]; R.a[3] = (ULONG)shp[s2];
        R.a[6] = (ULONG)DOSBase);
}

static void cludge(int toaddr, int s, ULONG num, ULONG off, const char *what)
{
    ROUT = toaddr ? "CludgeShapesToAddress" : "CludgeShapes";
    sprintf(VEC, "%d num %lu at %lu %s", s, num, off, what);
    DO1(ENT(LIB, toaddr ? TOADDR : CLUDGE, 0),
        R.d[0] = toaddr ? (ULONG)shp[s] : (ULONG)s; R.d[1] = num;
        R.d[2] = (ULONG)mem + off; R.a[0] = (ULONG)shp[s];
        R.a[6] = (ULONG)SysBase);
    OBS("d0", R.d[0]);
}

static void autocookie(ULONG on)
{
    ROUT = "LEAutoCookie";
    sprintf(VEC, "%ld", (LONG)on);
    DO1(ENT(LIB, AUTOCOOKIE, 0), R.d[0] = on);
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
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}

static APTR nullsub(const char *name, int off)
{
    struct lib *l = getlib(&loaded, name);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

/* rishapeslib's link to LoadShapes form 1 (token #shapeslib<<7+10, form
   1; shapeslib = 154) goes through lsh_stub, its links to memlib's
   globalloc/globfree (tokens $c002/$c003) through ma_stub/mf_stub; FALSE
   unless there is exactly one LoadShapes and some of each memlib routine */
static BOOL link_loadshapes_stub(void)
{
    ULONG to = (ULONG)find_token(&loaded, (UWORD)((154 << 7) + 10), 1) & ~1UL;

    lsh_base = shp[0];
    lsh_loadshapes = (APTR)to;
    if (relink(to, (APTR)lsh_stub) != 1)
        return FALSE;
    ma_old = (APTR)((ULONG)find_token(&loaded, 0xc002, 0) & ~1UL);
    mf_old = (APTR)((ULONG)find_token(&loaded, 0xc003, 0) & ~1UL);
    return relink((ULONG)ma_old, (APTR)ma_stub) > 0 && relink((ULONG)mf_old, (APTR)mf_stub) > 0;
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "memlib", "shapeslib", "ripacklib", "riamosfunclib",
                                   "bitmaplib", "environlib", "iffiolib", "ilbmifflib",
                                   "intlib", "palettelib", "scrolllib", NULL };
    ULONG avail, off;
    int i;

    if (argc < 2) {
        printf("usage: rishapeslib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    if (!link_loadshapes_stub()) {
        printf("rishapeslib: not one call of LoadShapes in %s\n", LIB);
        return 20;
    }
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("LEInitShape", d3-d7|a0|a2|a3|a6);
    KEEPS("LEScanShapes", d3-d7|a0|a1|a3|a6);
    KEEPS("LEMaxShapeWidth", d1-d7|a0-a3|a6);
    KEEPS("LEMaxShapeHeight", d1-d7|a0-a3|a6);
    KEEPS("LEMaxShapeDepth", d1-d7|a0-a3|a6);
    KEEPS("SafeToBlit", d3-d7|a0-a3|a6);
    KEEPS("LESaveShapes", a3|a6);
    KEEPS("LECompressShapes", d0-d7|a0-a3|a6);
    KEEPS("LELoadShapes", a6);
    KEEPS("LECludgeShapes", d5|d6|a6);
    KEEPS("CludgeShapes", d3-d6|a3|a6);
    KEEPS("LEAutoCookie", d0-d7|a0-a3|a6);
    KEEPS("CludgeShapesToAddress", d3-d6|a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib/shapeslib init";
    memlib(7);
    {
        struct regs r;
        markregs(&r);
        callregs(nullsub("shapeslib", 22), &r, 0, 0);
    }
    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, INIT, 0), (void)0);
    SW(bm, 56) = 320;
    SW(bm, 58) = 200;
    srcpal = AllocMem(4 + 12 * 12, MEMF_CLEAR);
    SW(srcpal, 0) = 12;
    for (i = 0; i < 36; i++)
        SL(srcpal, 4 + 4 * i) = 0x01010101UL * (UBYTE)(i * 7 + 3);
    SL(pal[0], 0) = (ULONG)srcpal;
    SW(pal[0], 4) = 12;

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
    safe(31, 0, 0, 1);                  /* beyond the maximum */
    safe(32, 0, 0, 1);                  /* beyond the maximum */
    safe(0, 0, 0, 0);
    safe(0, 0, 0, 0xffff);

    /* LESaveShapes, LELoadShapes */
    save(0, 0, 3, 0, -1);               /* count 3 */
    save(1, 0, 3, 0, 0);                /* with the palette */
    printf("rishapeslib: LESaveShapes with a palette: %s\n",
           fbuf[40] == 'C' ? "CMAP written" : "no CMAP (Write's Z flag)");
    save(2, 0, 64, 0, 0);               /* count 64 */
    save(3, 0, 3, 0x5A01, -1);          /* the upper word of d1 in the count (#815) */
    compress(0xffff);
    save(4, 0, 64, 0, 0);               /* packed: shapes 0, 2 pack, 1, 3 not */
    compress(0);
    save(5, 2, 2, 0, 0);
    /* dos Write returns with Z set: a save with a palette (#814) */
    wz_old = SetFunction((struct Library *)DOSBase, -48, (APTR)wz_stub);
    wz_on = 1;
    save(6, 0, 3, 0, 0);
    wz_on = 0;
    SetFunction((struct Library *)DOSBase, -48, wz_old);
    OBS("CMAP written", fbuf[40] == 'C');
    /* globfree leaves 1 in the upper word of d0: the allocations of a
       save with a palette (#817) */
    ma_n = 0;
    mf_on = 1;
    save(7, 0, 3, 0, 0);
    mf_on = 0;
    for (i = 0; i < (int)ma_n; i++)
        OBS("globalloc size", ma_size[i]);

    load(1, 0, 10, 13, -1, NULL);
    sameshapes(10, 13);
    BOTH("four shapes loaded", SW(shp[10], 0) == 20 && SW(shp[13], 0) == 1);
    load(0, 1, 14, 17, 1, NULL);
    sameshapes(14, 17);
    samepal(1);
    load(0, 2, 20, 23, 1, NULL);        /* 4 of 65 shapes; the old palette freed */
    sameshapes(20, 23);
    samepal(1);
    load(1, 4, 24, 27, -1, NULL);       /* packed */
    sameshapes(24, 27);
    BOTH("packed shapes loaded", SW(shp[24], 0) == 20 && SW(shp[26], 0) == 40);
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
    {
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
        BPTR f = Open((STRPTR)"RAM:rwesh_acid", MODE_NEWFILE);
        Write(f, acid, n);
        Close(f);
    }
    load(1, 0, 40, 43, -1, "RAM:rwesh_acid");
    sameshapes(40, 43);
    BOTH("LoadShapes into the first shape", SW(shp[40], 0) == 24 && !SW(shp[43], 0));

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
    BOTH("linked", SW(shp[55], 30) == 0x634c);
    initshape(55, 8, 8, 1);             /* a linked shape: not freed */
    /* a CMAP that claims 7 of its 36 bytes: SDAT is not found (the original
       stops behind the 2 colours, the port behind the 7 bytes) */
    SL(mem, 44) = 7;
    lecludge(0, 58, 59, 3, 0, "CMAP 7");
    sameshapes(58, 59);
    samepal(3);
    SL(mem, 0) = 0x464f524e;           /* FORN */
    lecludge(0, 58, 59, -1, 0, "no FORM");
    sameshapes(58, 59);

    /* CludgeShapes */
    if (mem)
        FreeMem(mem, memlen);
    mem = AllocMem(4096, MEMF_CLEAR);
    memlen = 4096;
    /* padding of the size of the shape before behind each empty shape:
       the original skips it (#809: 32 + the stale size), the port reads
       it as an empty shape (zeros) */
    off = ramshape(mem, 20, 4, 2, 1);                  /* 32 bytes */
    off += ramshape(mem + off, 0, 4, 2, 2) + 32;       /* empty */
    off += ramshape(mem + off, 17, 3, 3, 3);           /* 36 bytes */
    off += ramshape(mem + off, 8, 0, 1, 4) + 36;       /* empty */
    off += ramshape(mem + off, 64, 2, 1, 5);
    cludge(0, 60, 0, 0, "none");
    cludge(0, 60, 3, 0, "");
    sameshapes(60, 62);
    cludge(0, 60, 5, 0, "again, used");
    sameshapes(60, 64);
    BOTH("the stale size skips to the next shape",
         SW(shp[62], 0) == 17 && SW(shp[64], 0) == 64);
    autocookie(0);
    cludge(1, 65, 1, 0, "no cookie");
    sameshapes(65, 65);
    autocookie(1);
    cludge(1, 66, 2, 0, "");
    sameshapes(66, 67);
    cludge(0, 56, 1, 0, "into a linked shape");
    sameshapes(56, 56);

    /* a second run of a program from the editor: the library stays
       loaded, finit and init run between the runs.  The run before ends
       with LEAutoCookie off and the maxima of an LEScanShapes set */
    autocookie(0);
    scan(0, 3);
    ROUT = "finit";
    strcpy(VEC, "end of the run");
    DO1(ENT(LIB, FINIT, 0), R.a[6] = (ULONG)DOSBase);
    ROUT = "init";
    strcpy(VEC, "second run");
    DO1(ENT(LIB, INIT, 0), (void)0);
    ROUT = "LEMaxShapeWidth";
    DO1(ENT(LIB, MAXW, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "LEMaxShapeHeight";
    DO1(ENT(LIB, MAXH, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "LEMaxShapeDepth";
    DO1(ENT(LIB, MAXD, 0), (void)0);
    OBS("d0", R.d[0]);
    cludge(0, 68, 1, 0, "second run");  /* a cookie if LEAutoCookie is on */
    sameshapes(68, 68);

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), R.a[6] = (ULONG)DOSBase);

    ROUT = "memlib finit";
    memlib(8);
    FreeMem(srcpal, 4 + 12 * 12);
    FreeMem(mem, memlen);
    for (i = 0; i < 8; i++)
        DeleteFile((STRPTR)setname(i));
    DeleteFile((STRPTR)"RAM:rwesh_form");
    DeleteFile((STRPTR)"RAM:rwesh_acid");
    rwe_mem_check(LIB, avail, TRUE);
    return rwe_done(LIB);
}
