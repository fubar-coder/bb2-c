/*
 * rwe rifxlib - RWE userlib 66: the C build or the CD binary (BSS2
 * RIFXLIB.OBJ), one set as LIBSET says, observed as cases (sysload.h,
 * LIBS_ONE_SET; the expected data libs/tests/expected/rwe_rifxlib.txt).
 *
 *   rifxlib DIR      (DIR: rifxlib and memlib .orig or .obj)
 *
 * The bitmaps are built in a memory arena (the same pseudo-random
 * contents before every call, gaps between the planes), the other
 * registers are markers.  Observed are the whole arena afterwards (the
 * planes and everything around them), d0 where the routine has a result,
 * and the kept registers.
 * rifxlib calls memlib (globalloc/globfree), so memlib is loaded too.
 *
 * Covered: every routine and every form - the zooms before init (no
 * tables: nothing happens), GetZoomTables (the tables init builds),
 * ADDValue, ClearBitmap and FadeInBitmap (both forms, offsets, delay 1-3),
 * ZoomX4 (both forms, xy_select 0-3), ZoomX2, ZoomX8, ReduceX2 (also with
 * a width or height that ends them early), ZoomyZoom, InitZoomXY with
 * ZoomXY (screen copy and x/y factors), Derez (0, 1 = screen copy, 2 =
 * UnrezX2, 3-17 with the mask table, 18 and 40 bit by bit).  ZoomXY is
 * tested before Derez: a Derez >= 3 of the original overwrites the first
 * instruction of ZoomXY (BUG(orig) (#671), see rifxlib.c).
 *
 * Not covered: delay 0 (2^32 turns), bitmaps with bm_Flags != 0 (the
 * depth word: plane counts in the thousands), depths above 8.
 */
#define LIBS_ONE_SET
#include "sysload.h"

#define LIBNAME "rifxlib"

/* entries of the token table */
enum {
    E_CLEAR, E_FADE, E_ZOOMX4, E_DEREZ, E_ZOOMXY, E_INITXY, E_ZOOMX2, E_TABLES,
    E_NULL, E_ZOOMX8, E_ADDVALUE, E_REDUCE, E_ZOOMY, E_FINIT, E_INIT
};

/* ------------------------------------------------------------ framework */

static struct regs R, IN;

/* code of entry e, form f (bit 0 = Amiga only, cleared) */
static APTR fn(int e, int f)
{
    return (APTR)((ULONG)entry_form(getlib(&loaded, LIBNAME), e, f) & ~1UL);
}

/* the arena */
#define ARENA 32768
static UBYTE *arena;

static ULONG seed;
static void fill(ULONG sd)
{
    ULONG i, x = sd;

    for (i = 0; i < ARENA; i++) {
        x = x * 1103515245UL + 12345;
        arena[i] = (UBYTE)(x >> 16);
    }
}

/* one call: fresh arena contents, registers = markers, then `setup`;
   afterwards the kept registers and the arena are observed */
#define DO1(e, f, setup) do {                                               \
        fill(seed += 0x9e3779b9UL);                                         \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callregs(fn(e, f), &R, 0, 0);                                       \
        obs_kept(&IN, &R);                                                  \
        OBSBYTES("memory", arena, ARENA);                                   \
    } while (0)

/* bitmaps: struct BitMap {bpr.w, rows.w, flags.b, depth.b, pad.w,
   planes[8]}, the planes in the arena */
struct fbm {
    UWORD bpr, rows;
    UBYTE flags, depth;
    UWORD pad;
    ULONG planes[8];
};
static struct fbm bm[2];                /* [source, dest] */
#define SRC (ULONG)&bm[0]
#define DST (ULONG)&bm[1]

static ULONG off;                       /* next free arena offset */

/* bitmap i: `depth` planes of bpr*rows bytes, with 64 bytes between them
   (and before the first) */
static void mkbm(int i, UWORD bpr, UWORD rows, UBYTE depth, ULONG size)
{
    int p;

    if (!size)
        size = (ULONG)bpr * rows;
    bm[i].bpr = bpr;
    bm[i].rows = rows;
    bm[i].flags = 0;
    bm[i].depth = depth;
    bm[i].pad = 0;
    for (p = 0; p < 8; p++) {
        if (p < depth) {
            off += 64;
            bm[i].planes[p] = (ULONG)arena + off;
            off += (size + 3) & ~3UL;
        } else {
            bm[i].planes[p] = 0;
        }
    }
    if (off + 64 > ARENA)
        printf("rifxlib test: arena too small (%lu)\n", off);
}

static void bitmaps(UWORD sbpr, UWORD srows, UBYTE sdepth, UWORD dbpr, UWORD drows,
                    UBYTE ddepth)
{
    off = 0;
    mkbm(0, sbpr, srows, sdepth, 0);
    mkbm(1, dbpr, drows, ddepth, 0);
}

/* the argument frame for ZoomX4's seventh argument, -1(a2) */
static UWORD frame[4];

/* ------------------------------------------------------------ tests */

/* the zooms without tables (before init) */
static void test_notables(void)
{
    ROUT = "before init";
    bitmaps(8, 8, 2, 32, 32, 2);
    strcpy(VEC, "ZoomX2");
    DO1(E_ZOOMX2, 0, R.d[2] = 0; R.d[3] = 0; R.d[4] = 32; R.d[5] = 4;
        R.a[0] = SRC; R.a[1] = DST);
    strcpy(VEC, "ZoomX4");
    DO1(E_ZOOMX4, 0, R.d[2] = 0; R.d[3] = 0; R.d[4] = 32; R.d[5] = 4;
        R.a[0] = SRC; R.a[1] = DST);
    strcpy(VEC, "ZoomX8");
    DO1(E_ZOOMX8, 0, R.d[2] = 0; R.d[3] = 0; R.d[4] = 16; R.d[5] = 2;
        R.a[0] = SRC; R.a[1] = DST);
    strcpy(VEC, "ReduceX2");
    DO1(E_REDUCE, 0, R.d[2] = 0; R.d[3] = 0; R.d[4] = 64; R.d[5] = 8;
        R.a[0] = SRC; R.a[1] = DST);
    strcpy(VEC, "ZoomyZoom");
    DO1(E_ZOOMY, 0, R.a[0] = SRC);
    strcpy(VEC, "GetZoomTables");
    DO1(E_TABLES, 0, (void)0);
    OBS("tables 0", L((UBYTE *)R.d[0]) | L((UBYTE *)R.d[0] + 4) |
                     L((UBYTE *)R.d[0] + 8) | L((UBYTE *)R.d[0] + 12));
}

static void test_tables(void)
{
    ROUT = "GetZoomTables";
    VEC[0] = 0;
    DO1(E_TABLES, 0, (void)0);
    OBS("table 0 set", L((UBYTE *)R.d[0]) != 0);
    OBS("x2 - reduce", L((UBYTE *)R.d[0] + 4) - L((UBYTE *)R.d[0]));
    OBS("x4 - reduce", L((UBYTE *)R.d[0] + 8) - L((UBYTE *)R.d[0]));
    OBS("x8 - reduce", L((UBYTE *)R.d[0] + 12) - L((UBYTE *)R.d[0]));
    {
        UBYTE *t = (UBYTE *)L((UBYTE *)R.d[0]);
        if (t)
            OBSBYTES("table contents", t, 1856);
    }
}

static void test_addvalue(void)
{
    static const ULONG xs[] = { 0, 7, 8, 319, 0x7fff, 0x8000, 0xffff, 0x12340010 };
    static const ULONG ys[] = { 0, 1, 255, 0xffff, 0xabcd0003 };
    unsigned i, j;

    ROUT = "ADDValue";
    bitmaps(40, 4, 1, 4, 4, 1);
    for (i = 0; i < sizeof xs / sizeof xs[0]; i++)
        for (j = 0; j < sizeof ys / sizeof ys[0]; j++) {
            sprintf(VEC, "x $%lX y $%lX", xs[i], ys[j]);
            DO1(E_ADDVALUE, 0, R.d[0] = 0; R.d[1] = xs[i]; R.d[2] = ys[j];
                R.a[0] = SRC);
            OBS("d0", R.d[0]);
        }
}

static void test_clearfade(void)
{
    static const struct { UWORD bpr, rows; UBYTE depth; ULONG h, o; } c[] = {
        { 8, 12, 2, 0, 0 }, { 10, 30, 3, 0, 0 }, { 8, 12, 2, 5, 3 }, { 8, 12, 2, 12, 0 },
        { 6, 25, 1, 21, 2 }, { 8, 12, 2, 0x10005, 0xffff0002 },
    };
    unsigned i;

    ROUT = "ClearBitmap";
    for (i = 0; i < sizeof c / sizeof c[0]; i++) {
        bitmaps(c[i].bpr, c[i].rows, c[i].depth, 4, 4, 1);
        if (!c[i].h) {
            sprintf(VEC, "%ux%u x%u", c[i].bpr, c[i].rows, c[i].depth);
            DO1(E_CLEAR, 0, R.d[0] = 0; R.d[1] = 1 + i % 3; R.a[0] = SRC);
        } else {
            sprintf(VEC, "%ux%u x%u h $%lX off $%lX", c[i].bpr, c[i].rows, c[i].depth,
                    c[i].h, c[i].o);
            DO1(E_CLEAR, 1, R.d[0] = 0; R.d[1] = 1 + i % 3; R.d[2] = c[i].h;
                R.d[3] = c[i].o; R.a[0] = SRC);
        }
    }

    ROUT = "FadeInBitmap";
    for (i = 0; i < sizeof c / sizeof c[0]; i++) {
        bitmaps(c[i].bpr, c[i].rows, c[i].depth, c[i].bpr + 4, c[i].rows + 4, c[i].depth);
        if (!c[i].h) {
            sprintf(VEC, "%ux%u x%u", c[i].bpr, c[i].rows, c[i].depth);
            DO1(E_FADE, 0, R.d[0] = 0; R.d[1] = 1; R.d[2] = 1 + i % 3;
                R.a[0] = SRC; R.a[1] = DST);
        } else {
            sprintf(VEC, "%ux%u x%u h $%lX off $%lX,1", c[i].bpr, c[i].rows, c[i].depth,
                    c[i].h, c[i].o);
            DO1(E_FADE, 1, R.d[0] = 0; R.d[1] = 1; R.d[2] = 1 + i % 3;
                R.d[3] = c[i].h; R.d[4] = c[i].o; R.d[5] = 1;
                R.a[0] = SRC; R.a[1] = DST);
        }
    }
}

/* the table zooms: source sbpr x srows, dest big enough */
static const struct zc { UWORD sbpr, srows; UBYTE depth; ULONG sadd, dadd, w, h; } zcs[] = {
    { 4, 4, 2, 0, 0, 32, 4 },
    { 6, 5, 2, 1, 3, 24, 3 },           /* source/dest offsets, odd */
    { 8, 6, 3, 8, 2, 40, 5 },
    { 4, 4, 1, 0, 0, 7, 4 },            /* width < 8: nothing */
    { 4, 4, 1, 0, 0, 32, 0 },           /* height 0: nothing */
    { 4, 4, 1, 0, 0, 0xffff, 2 },       /* width -1: nothing */
};
#define NZ (sizeof zcs / sizeof zcs[0])

static void zoomvec(const struct zc *z)
{
    sprintf(VEC, "%ux%u x%u add %lu,%lu w $%lX h %lu", z->sbpr, z->srows, z->depth,
            z->sadd, z->dadd, z->w, z->h);
}

#define ZREGS(z) R.d[0] = 0; R.d[1] = 1; R.d[2] = (z)->sadd;          \
    R.d[3] = (z)->dadd; R.d[4] = (z)->w; R.d[5] = (z)->h;             \
    R.a[0] = SRC; R.a[1] = DST

static void test_zooms(void)
{
    unsigned i;
    int sel;

    ROUT = "ZoomX2";
    for (i = 0; i < NZ; i++) {
        const struct zc *z = &zcs[i];
        bitmaps(z->sbpr, z->srows, z->depth, z->sbpr * 2 + 4, z->srows * 2 + 2, z->depth);
        zoomvec(z);
        DO1(E_ZOOMX2, 0, ZREGS(z));
    }
    ROUT = "ZoomX4";
    for (i = 0; i < NZ; i++) {
        const struct zc *z = &zcs[i];
        bitmaps(z->sbpr, z->srows, z->depth, z->sbpr * 4 + 4, z->srows * 4 + 2, z->depth);
        zoomvec(z);
        DO1(E_ZOOMX4, 0, ZREGS(z));
        for (sel = 0; sel < 4; sel++) {
            sprintf(VEC + strlen(VEC), " sel %d", sel);
            DO1(E_ZOOMX4, 1, ZREGS(z); frame[0] = 0x5a00 | sel;
                R.a[2] = (ULONG)&frame[1]);
            VEC[strlen(VEC) - 6] = 0;
        }
    }
    ROUT = "ZoomX8";
    for (i = 0; i < NZ; i++) {
        const struct zc *z = &zcs[i];
        bitmaps(z->sbpr, z->srows, z->depth, z->sbpr * 8 + 4, z->srows * 8 + 2, z->depth);
        zoomvec(z);
        DO1(E_ZOOMX8, 0, ZREGS(z));
    }
    ROUT = "ReduceX2";
    for (i = 0; i < NZ; i++) {
        const struct zc *z = &zcs[i];
        bitmaps(z->sbpr * 2, z->srows * 2, z->depth, z->sbpr, z->srows, z->depth);
        zoomvec(z);
        DO1(E_REDUCE, 0, ZREGS(z));
    }
    ROUT = "ReduceX2";
    bitmaps(8, 8, 2, 4, 4, 2);
    strcpy(VEC, "height 1");
    DO1(E_REDUCE, 0, R.d[2] = 0; R.d[3] = 0; R.d[4] = 64; R.d[5] = 1;
        R.a[0] = SRC; R.a[1] = DST);

    ROUT = "ZoomyZoom";
    off = 0;
    mkbm(0, 80, 32, 2, 0);
    strcpy(VEC, "80x32 x2");
    DO1(E_ZOOMY, 0, R.d[0] = 0; R.a[0] = SRC);
    off = 0;
    mkbm(0, 40, 64, 1, 0);
    strcpy(VEC, "40x64 x1 (the size is ignored)");
    DO1(E_ZOOMY, 0, R.d[0] = 0; R.a[0] = SRC);
}

/* lores bitmaps (40 bytes per line) for ZoomXY and Derez */
static void lores(UWORD srows, UBYTE depth, UWORD drows)
{
    off = 0;
    mkbm(0, 40, srows, depth, 40UL * srows + 40);
    mkbm(1, 40, drows, depth, 40UL * drows + 40);
}

static void test_zoomxy(void)
{
    static const struct { ULONG x, y, h; } c[] = {
        { 1, 1, 6 }, { 2, 1, 6 }, { 1, 2, 7 }, { 3, 2, 8 }, { 2, 3, 9 }, { 5, 4, 10 },
        { 0, 2, 6 }, { 2, 0, 6 }, { 0x10002, 0x30001, 5 },
    };
    unsigned i;

    for (i = 0; i < sizeof c / sizeof c[0]; i++) {
        lores(12, 2, 24);
        ROUT = "InitZoomXY";
        strcpy(VEC, "add 40,80");
        DO1(E_INITXY, 0, R.d[0] = 0; R.d[1] = 1; R.d[2] = 40; R.d[3] = 80;
            R.a[0] = SRC; R.a[1] = DST);
        ROUT = "ZoomXY";
        sprintf(VEC, "%lX,%lX h %lu", c[i].x, c[i].y, c[i].h);
        DO1(E_ZOOMXY, 0, R.d[0] = c[i].x; R.d[1] = c[i].y; R.d[2] = c[i].h);
    }
}

static void test_derez(void)
{
    static const struct { ULONG v, h; UBYTE depth; } c[] = {
        { 0, 6, 2 }, { 1, 6, 2 }, { 2, 6, 2 }, { 2, 7, 1 }, { 3, 9, 2 }, { 4, 8, 1 },
        { 5, 11, 2 }, { 7, 8, 1 }, { 16, 20, 1 }, { 17, 20, 2 }, { 18, 20, 1 },
        { 25, 30, 1 }, { 40, 41, 1 }, { 0xffff0003, 6, 1 },
    };
    unsigned i;

    ROUT = "Derez";
    for (i = 0; i < sizeof c / sizeof c[0]; i++) {
        lores(c[i].h + 2, c[i].depth, c[i].h + 48);
        sprintf(VEC, "%lX h %lu x%u", c[i].v, c[i].h, c[i].depth);
        DO1(E_DEREZ, 0, R.d[0] = 0x00120034; R.d[1] = 1; R.d[2] = 0;
            R.d[3] = 40; R.d[4] = c[i].v; R.d[5] = c[i].h;
            R.a[0] = SRC; R.a[1] = DST);
        OBS("d0", R.d[0]);
    }
}

/* ------------------------------------------------------------ main */

static const char *names[] = { "memlib", LIBNAME, NULL };

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: rifxlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("before init", d1-d6|a1-a3|a6);
    KEEPS("init", d3-d7|a1-a3|a6);
    KEEPS("GetZoomTables", d1-d7|a1-a3|a6);
    KEEPS("ADDValue", d2-d7|a0-a3|a6);
    KEEPS("ClearBitmap", a3|a6);
    KEEPS("FadeInBitmap", a6);
    KEEPS("ZoomX2", d0-d7|a0-a3|a6);
    KEEPS("ZoomX4", d1-d6|a0-a3|a6);
    KEEPS("ZoomX8", d0-d7|a0-a3|a6);
    KEEPS("ReduceX2", d0-d7|a0-a3|a6);
    KEEPS("ZoomyZoom", d1-d3|a2|a3|a6);
    KEEPS("InitZoomXY", d0-d7|a0-a3|a6);
    KEEPS("ZoomXY", d1-d7|a0-a3|a6);
    KEEPS("Derez", d1-d7|a0-a3|a6);
    KEEPS("finit", d1-d7|a0|a2|a3|a6);
    arena = AllocMem(ARENA, MEMF_ANY);
    if (!arena) {
        check("rifxlib test: memory", FALSE);
        return 20;
    }
    seed = 0x13579bdf;

    ROUT = "memlib init";
    VEC[0] = 0;
    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 7, 0), &R, 0, 0);
    test_notables();
    ROUT = "init";
    DO1(E_INIT, 0, (void)0);
    test_tables();
    test_addvalue();
    test_clearfade();
    test_zooms();
    test_zoomxy();
    test_derez();                       /* after ZoomXY, see the head */
    ROUT = "finit";
    VEC[0] = 0;
    DO1(E_FINIT, 0, (void)0);
    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 8, 0), &R, 0, 0);
    FreeMem(arena, ARENA);
    printf("rifxlib: %d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
