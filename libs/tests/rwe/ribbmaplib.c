/*
 * rwe ribbmaplib - the C build of ribbmaplib (userlib 34) against the
 * reference build of rwelibsrc/blitzbomberslib/bbmaplib2.bb (blitz2
 * 2.10), method of rwecmp.h.
 *
 *   ribbmaplib DIR     (DIR with ribbmaplib and memlib, .obj/.orig)
 *
 * Scenario and map files are built in one buffer per set (the same bytes
 * at the same offsets): chunks in shuffled order with random lengths and
 * contents.  Compared: d0 of every routine (addresses relative to the
 * buffer; the full register, also the upper word of the word
 * functions), the string results and a3, the palette objects (fields,
 * the colour blocks, which memlib allocates per set), the kept
 * registers.  Covered: both Set routines with no FORM, the wrong type
 * and good files; every accessor with the chunk present (twice: the
 * cache) and missing; a chunk id read behind the end of the file
 * (BUG(orig) #553: found); BBShadowValue with word index overflow and a
 * negative index; author strings at an odd and an even address;
 * BBGetPalette without a new block, with one (from 0 and from 4
 * colours), low > high, no COLR; a map without MFRM; a failed
 * SetBBMapLoc after a good one (BUG(orig) #554: the info chunk, the cache and
 * the end of the old map stay).
 * Not covered: the routines before the first Set (they search from
 * address 12), odd chunk lengths (address errors), BBGetPalette writing
 * behind its new block.
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ribbmaplib"
#define BUFSZ 8192

enum { SETMAP, SETSCEN, NUL1, SCNAME, SCAUTHOR, NUL2, DEPTH, SHAPES, SOUNDS, BADDIES, PALETTE,
       LOWCOL, HIGHCOL, LEVEL, NUMBACK, NUMDEST, NUMINDEST, PREFIX, NUMTUNNELS, SHADOW,
       TUNNELHOLES, TUNNELSLOC, GETPAL, DESTRUCT, MAPSCENID, MAPWIDTH, MAPHEIGHT, MAPPOWERUPS,
       MAPDATA, MAPSTART, MAPNAME, MAPINFO, MAPPUMASK, MAPDBOVERLAY, MAPONEPLAYER, MAPOBJECTS,
       NUMBADDIES, INIT, FINIT };

static UBYTE *buf[2];                   /* the files, BUFSZ */
static UBYTE out[2][300];               /* string results */
static UBYTE pal[2][8];                 /* palette objects */
static UBYTE tmpl[BUFSZ];               /* the files being built */

#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define ID(s) ((ULONG)(s)[0] << 24 | (ULONG)(s)[1] << 16 | (ULONG)(s)[2] << 8 | (ULONG)(s)[3])

/* ------------------------------------------------------- building files */

struct chunk { const char *id; UBYTE data[200]; ULONG len; };

static ULONG pick(ULONG a, ULONG b)
{
    return a + rnd() % (b - a + 1);
}

static void fillrnd(UBYTE *p, ULONG n)
{
    while (n--)
        *p++ = (UBYTE)rnd();
}

/* a FORM of `type` at `at` in tmpl with the chunks c[0..n-1] (shuffled);
   returns its end */
static ULONG form(ULONG at, const char *type, struct chunk *c, int n)
{
    ULONG p = at + 12;
    int i, order[20];
    for (i = 0; i < n; i++)
        order[i] = i;
    for (i = n - 1; i > 0; i--) {
        int j = (int)(rnd() % (ULONG)(i + 1)), t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    memcpy(tmpl + at, "FORM", 4);
    memcpy(tmpl + at + 8, type, 4);
    for (i = 0; i < n; i++) {
        struct chunk *k = &c[order[i]];
        memcpy(tmpl + p, k->id, 4);
        SL(tmpl, p + 4) = k->len;
        memcpy(tmpl + p + 8, k->data, k->len);
        p += 8 + k->len;
    }
    SL(tmpl, at + 4) = p - at - 8;
    return p;
}

static void chunk(struct chunk *c, const char *id, ULONG len)
{
    c->id = id;
    c->len = len;
    fillrnd(c->data, len);
}

/* ANNO: name, 0, (pad to even), author, 0 */
static void anno(struct chunk *c, int namelen, int authlen)
{
    int i, p;
    c->id = "ANNO";
    for (i = 0; i < namelen; i++)
        c->data[i] = (UBYTE)pick('A', 'Z');
    c->data[i++] = 0;
    p = i;
    if (p & 1)
        c->data[p++] = 'x';             /* skipped (address odd) */
    for (i = 0; i < authlen; i++)
        c->data[p + i] = (UBYTE)pick('a', 'z');
    c->data[p + i] = 0;
    c->len = (ULONG)(p + i + 2) & ~1UL;
}

static void install(void)
{
    int s;
    for (s = 0; s < 2; s++)
        memcpy(buf[s], tmpl, BUFSZ);
}

/* ------------------------------------------------------------- calls */

#define REL(v) ((v) >= (ULONG)buf[cur] && (v) < (ULONG)buf[cur] + BUFSZ ? (v) - (ULONG)buf[cur] : (v))

static void setloc(int idx, ULONG off)
{
    ROUT = idx == SETMAP ? "SetBBMapLoc" : "SetBBScenarioLoc";
    sprintf(VEC, "at %lu", off);
    DO2(ENT(LIB, idx, 0), R[cur].d[0] = (ULONG)buf[cur] + off);
    SAME("d0", R[cur].d[0]);
}

static const char *const names[] = {
    "SetBBMapLoc", "SetBBScenarioLoc", "", "BBScenarioName", "BBScenarioAuthor", "",
    "BBScenarioDepth", "BBScenarioShapeLoc", "BBScenarioSounds", "BBScenarioBaddiesLoc",
    "BBScenarioPalette", "BBScenarioLowCol", "BBScenarioHighCol", "BBScenarioLevel",
    "BBNumBackGround", "BBNumDestruct", "BBNumIndestruct", "BBScenarioPrefix", "BBNumTunnels",
    "BBShadowValue", "BBTunnelHoles", "BBTunnelsLoc", "BBGetPalette", "BBDestructLoc",
    "BBMapScenarioID", "BBMapWidth", "BBMapHeight", "BBMapPowerUps", "BBMapData",
    "BBMapStartPos", "BBMapName$", "BBMapInfo", "BBMapPUMask", "BBMapDBOverLay",
    "BBMapOnePlayerInfo", "BBMapObjects", "BBNumBaddies" };

/* a function without arguments (d0 = a marker: the word functions of
   the map keep its upper word) */
static void fn(int idx, const char *what)
{
    ROUT = names[idx];
    strcpy(VEC, what);
    DO2(ENT(LIB, idx, 0), (void)0);
    SAME("d0", REL(R[cur].d[0]));
}

static void strfn(int idx, const char *what)
{
    ROUT = names[idx];
    strcpy(VEC, what);
    for (cur = 0; cur < 2; cur++)
        memset(out[cur], 0xEE, sizeof out[cur]);
    DO2(ENT(LIB, idx, 0), R[cur].a[3] = (ULONG)out[cur] + 2);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
    SAMEBYTES("string", out, sizeof out[0]);
}

static void shadow(ULONG set, const char *what)
{
    ROUT = "BBShadowValue";
    sprintf(VEC, "%s set $%lX", what, set);
    DO2(ENT(LIB, SHADOW, 0), R[cur].d[0] = set);
    SAME("d0", R[cur].d[0]);
}

static void allscenario(const char *what)
{
    static const int fns[] = { DEPTH, SHAPES, SOUNDS, BADDIES, PALETTE, LOWCOL, HIGHCOL, LEVEL,
                               NUMBACK, NUMDEST, NUMINDEST, PREFIX, NUMTUNNELS, TUNNELHOLES,
                               TUNNELSLOC, DESTRUCT, NUMBADDIES };
    unsigned i;
    int twice;
    for (twice = 0; twice < 2; twice++) {
        for (i = 0; i < sizeof fns / sizeof fns[0]; i++) {
            fn(fns[i], what);
            fn(fns[i], what);           /* the cached chunk */
        }
        strfn(SCNAME, what);
        strfn(SCAUTHOR, what);
        shadow(0, what);
        shadow(3, what);
        shadow(0x4001, what);           /* word index: 4 */
        shadow(0xffff, what);           /* index -4: the length */
    }
}

static void allmap(const char *what)
{
    static const int fns[] = { MAPSCENID, MAPWIDTH, MAPHEIGHT, MAPPOWERUPS, MAPDATA, MAPSTART,
                               MAPINFO, MAPPUMASK, MAPDBOVERLAY, MAPONEPLAYER, MAPOBJECTS };
    unsigned i;
    for (i = 0; i < sizeof fns / sizeof fns[0]; i++) {
        fn(fns[i], what);
        fn(fns[i], what);
    }
    strfn(MAPNAME, what);
}

/* memlib (per set): globalloc (idx 2), init 7, finit 8 */
static ULONG memcall(int idx, ULONG d0, ULONG d1)
{
    struct regs r;
    ULONG pop;
    markregs(&r);
    r.d[0] = d0;
    r.d[1] = d1;
    callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &r, NULL, 0, &pop);
    return r.d[0];
}

/* a palette object of n colours (memlib's memory, a pattern) */
static void newpal(UWORD n)
{
    for (cur = 0; cur < 2; cur++) {
        UBYTE *d = (UBYTE *)memcall(2, 4 + 12 * (ULONG)n, 0x10001);
        ULONG i;
        SL(pal[cur], 0) = (ULONG)d;
        SW(pal[cur], 4) = n;
        SW(d, 0) = n;
        for (i = 0; i < 12 * (ULONG)n; i++)
            d[4 + i] = (UBYTE)(i * 7 + n);
    }
}

static void getpal(const char *what)
{
    ULONG was[2] = { SL(pal[0], 0), SL(pal[1], 0) };
    ROUT = "BBGetPalette";
    sprintf(VEC, "%s, %u colours", what, SW(pal[0], 4));
    DO2(ENT(LIB, GETPAL, 0), R[cur].a[0] = (ULONG)pal[cur]; R[cur].d[0] = 0x5a000003);
    SAME("count", SW(pal[cur], 4));
    SAME("new block", SL(pal[cur], 0) != was[cur]);
    {
        const UBYTE *d[2] = { (UBYTE *)SL(pal[0], 0), (UBYTE *)SL(pal[1], 0) };
        /* a new block has count+1 colours (BUG(orig), #548) */
        SAMEBYTES("colours", d, 4 + 12 * ((ULONG)SW(pal[0], 4) + (SL(pal[0], 0) != was[0])));
    }
}

/* a scenario with COLR low..high */
static ULONG scenario(ULONG at, int low, int high, int full, int namelen)
{
    struct chunk c[16];
    int n = 0, i;
    if (full) {
        anno(&c[n++], namelen, (int)pick(0, 40));
        chunk(&c[n++], "DEPF", 2);
        chunk(&c[n++], "LEVL", 2);
        chunk(&c[n++], "SHAP", 6);
        chunk(&c[n++], "TUNL", 8 + 2 * pick(0, 20));
        chunk(&c[n++], "BAD!", 2 + 2 * pick(0, 20));
        chunk(&c[n++], "SHDW", 16);
        chunk(&c[n++], "PRFX", 4);
    }
    chunk(&c[n++], "GFX!", 2 * pick(0, 40));
    chunk(&c[n++], "SFX!", 2 * pick(0, 40));
    if (full)
        chunk(&c[n++], "DEST", 2 * pick(0, 40));
    if (low >= 0) {
        c[n].id = "COLR";
        c[n].data[0] = (UBYTE)low;
        c[n].data[1] = (UBYTE)high;
        c[n].len = 2 + ((3 * (ULONG)((UBYTE)high - (UBYTE)low + 1) + 1) & ~1UL);
        if (c[n].len > sizeof c[n].data || (UBYTE)high < (UBYTE)low)
            c[n].len = 2 + 3 + 1;
        for (i = 2; i < (int)c[n].len; i++)
            c[n].data[i] = (UBYTE)rnd();
        n++;
    }
    return form(at, "SCEN", c, n);
}

static ULONG map(ULONG at, int full)
{
    struct chunk c[12];
    int n = 0, i;
    if (full)
        chunk(&c[n++], "MFRM", 12);
    chunk(&c[n++], "MDAT", 2 * pick(1, 60));
    chunk(&c[n++], "STRT", 2 * pick(1, 10));
    c[n].id = "NAME";
    c[n].len = 2 * pick(1, 20);
    for (i = 0; i < (int)c[n].len - 1; i++)
        c[n].data[i] = (UBYTE)pick('0', 'z');
    c[n].data[i] = 0;
    n++;
    if (full) {
        chunk(&c[n++], "PWUP", 2 * pick(0, 10));
        chunk(&c[n++], "PWMK", 4);
        chunk(&c[n++], "DBOL", 2 * pick(0, 30));
        chunk(&c[n++], "1PLY", 2 * pick(0, 10));
        chunk(&c[n++], "OBJS", 2 * pick(0, 10));
    }
    return form(at, "BMAP", c, n);
}

int main(int argc, char **argv)
{
    static const char *libs[] = { LIB, "memlib", NULL };
    char what[60];
    ULONG end;
    int s, round;

    if (argc < 2) {
        printf("usage: ribbmaplib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], libs))
        return 20;
    for (s = 0; s < 2; s++)
        buf[s] = AllocMem(BUFSZ, MEMF_PUBLIC);
    for (cur = 0; cur < 2; cur++)
        memcall(7, 0, 0);
    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);

    for (round = 0; round < 6; round++) {
        memset(tmpl, 0, BUFSZ);
        fillrnd(tmpl, BUFSZ);
        /* scenario at 0 (full), map at 2048, a scenario without most
           chunks at 4096, a map without MFRM at 6144 */
        scenario(0, (int)pick(0, 15), (int)pick(16, 40), 1, (int)pick(0, 30));
        map(2048, 1);
        /* the ids behind the ends of the others: found on odd rounds */
        end = scenario(4096, round == 3 ? -1 : 0x81, 0x83, 0, (int)pick(0, 30));
        memcpy(tmpl + end, round & 1 ? "DEST" : "NONE", 4);
        end = map(6144, 0);
        memcpy(tmpl + end, round & 1 ? "OBJS" : "NONE", 4);
        memcpy(tmpl + 1024, "FORM\0\0\0\x10MAPS", 12);     /* a wrong type */
        install();

        sprintf(what, "round %d", round);
        setloc(SETSCEN, 1024);                  /* wrong type: 0 */
        setloc(SETSCEN, 1500);                  /* no FORM */
        setloc(SETMAP, 1024);
        setloc(SETSCEN, 0);
        allscenario(what);
        setloc(SETMAP, 2048);
        allmap(what);
        setloc(SETSCEN, 1024);                  /* failed: the old one stays */
        fn(LEVEL, "after a failed SetBBScenarioLoc");

        /* palettes */
        newpal(32);
        getpal(what);                           /* no new block */
        newpal((UWORD)pick(0, 4));
        getpal(what);                           /* a new block */
        getpal(what);                           /* count = high: again */

        /* the second scenario: chunks missing, COLR $81..$83 */
        setloc(SETSCEN, 4096);
        sprintf(what, "round %d, scenario 2", round);
        allscenario(what);
        newpal(4);
        getpal(what);                           /* low $81: a new block */
        newpal(200);
        getpal(what);

        /* low > high: one colour at low */
        if (round == 5) {
            memset(tmpl, 0, BUFSZ);
            scenario(0, 20, 10, 0, 3);
            install();
            setloc(SETSCEN, 0);
            newpal(32);
            getpal("low > high");
        }

        /* the map without MFRM; then a failed SetBBMapLoc */
        setloc(SETMAP, 6144);
        sprintf(what, "round %d, map without MFRM", round);
        allmap(what);
        setloc(SETMAP, 2048);
        fn(MAPDATA, "map 1 again");
        setloc(SETMAP, 3000);                   /* no FORM: maploc stays set */
        sprintf(what, "round %d, after a failed SetBBMapLoc", round);
        fn(MAPDATA, what);                      /* the cache */
        fn(MAPWIDTH, what);
        fn(MAPHEIGHT, what);
        fn(MAPSCENID, what);
        fn(MAPINFO, what);
        fn(MAPSTART, what);                     /* a search from 3000 to the old end */
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    for (cur = 0; cur < 2; cur++)
        memcall(8, 0, 0);
    for (s = 0; s < 2; s++)
        FreeMem(buf[s], BUFSZ);
    return rwe_done(LIB);
}
