/*
 * rwe rizonejoylib - the zone and joystick commands: the C build against
 * the CD original (BSS2 RIZONEJOYLIB.OBJ), method of rwecmp.h.
 *
 *   rizonejoylib DIR
 *
 * Covered, every form:
 *   - init/finit; init without memory (AllocMem patched to fail): every
 *     zone command without a table (GetZoneX1... read from address 0 -
 *     the same in both builds);
 *   - SetZone (rectangles, swapped corners, circles with radius^2 above
 *     32767 and above 65535, a negative y2, zone out of range), Zone(x,y)
 *     on a grid of 19 x 19 points in the 68000 and the 68020 mode
 *     (SetZoneMode, ZoneMode), ZoneTest with one zone and with ranges
 *     (start clamped, bad ranges), GetZoneX1/Y1/X2/Y2, ZoneInit's three
 *     forms (bad ranges too);
 *   - NewZoneTable (bad numbers and sizes; it initialises the current
 *     table, BUG(orig), #875), UseZoneTable, ZoneTableNumber, ZoneTableSize,
 *     ZoneTable, ZoneTest from zone 256 on (low byte, BUG(orig), #881),
 *     FreeZoneTable (the entry stays set, BUG(orig), #868);
 *   - JFire, JVert, JHoriz for the ports 0..5 with the adaptor on and
 *     off, JAdaptorStatus, AllFire with and without bits, ReadJoyPad for
 *     ports 0, 1, 2 - with nobody at the joysticks the hardware gives the
 *     same values to both builds.
 * Freed tables stay in the library's list (BUG(orig), #868 #871); the test finds the
 * list in each build's data and clears those entries, so that a later
 * finit does not free them twice.  Not covered: negative zone or table
 * numbers and zones behind a table (they write or read outside it).
 */
#include "rwecmp.h"

#define LIB "rizonejoylib"
enum { E_ZINIT, E_SETZONE, E_ZONE, E_ZTEST, E_ZTABLE, E_JFIRE, E_JVERT, E_JHORIZ,
       E_ALLFIRE, E_ZSIZE, E_NEWZT, E_USEZT, E_FREEZT, E_ZTNUM, E_ZMODE, E_SETZMODE,
       E_ADAPTOR, E_X1, E_Y1, E_X2, E_Y2, E_PAD, E_FINIT, E_INIT };

static ULONG *zlist[2];                 /* each build's zonetablelist */

/* find zonetablelist: _zonetable and zonetablelist[0] (both = table 0)
   are consecutive longs in the library's data */
static void findlist(void)
{
    for (cur = 0; cur < 2; cur++) {
        struct lib *l = getlib(&sets[cur], LIB);
        ULONG t0 = R[cur].d[0], i;
        zlist[cur] = NULL;
        for (i = 0; i + 8 <= l->size; i += 2)
            if (L(l->code + i) == t0 && L(l->code + i + 4) == t0) {
                zlist[cur] = (ULONG *)(l->code + i + 4);
                break;
            }
    }
    BOTH("zonetablelist found", zlist[cur] != NULL);
}

#define CALLV(e, f, what, setup) do {                                       \
        ROUT = what;                                                        \
        DO2(ENT(LIB, e, f), setup);                                         \
        SAME("d0", R[cur].d[0]);                                            \
    } while (0)

static void notable(void)
{
    static const ULONG z[] = { 0, 1, 255 };
    unsigned i;
    strcpy(VEC, "no table");
    CALLV(E_ZTABLE, 0, "ZoneTable", (void)0);
    CALLV(E_ZSIZE, 0, "ZoneTableSize", (void)0);
    CALLV(E_ZTNUM, 0, "ZoneTableNumber", (void)0);
    CALLV(E_ZINIT, 0, "ZoneInit", (void)0);
    CALLV(E_ZINIT, 1, "ZoneInit zone", R[cur].d[0] = 3);
    CALLV(E_ZINIT, 2, "ZoneInit range", R[cur].d[0] = 1; R[cur].d[1] = 2);
    CALLV(E_SETZONE, 0, "SetZone circle", R[cur].d[0] = 1; R[cur].d[1] = 5; R[cur].d[2] = 5;
          R[cur].d[3] = 3);
    CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = 1; R[cur].d[1] = 5; R[cur].d[2] = 5;
          R[cur].d[3] = 9; R[cur].d[4] = 9);
    CALLV(E_ZONE, 0, "Zone", R[cur].d[0] = 5; R[cur].d[1] = 5);
    CALLV(E_ZTEST, 0, "ZoneTest 1", R[cur].d[0] = 0; R[cur].d[1] = 5; R[cur].d[2] = 5);
    CALLV(E_ZTEST, 1, "ZoneTest range", R[cur].d[0] = 0; R[cur].d[1] = 3; R[cur].d[2] = 5;
          R[cur].d[3] = 5);
    for (i = 0; i < sizeof z / sizeof z[0]; i++) {
        sprintf(VEC, "no table zone %lu", z[i]);
        CALLV(E_X1, 0, "GetZoneX1", R[cur].d[0] = z[i]);
        CALLV(E_Y1, 0, "GetZoneY1", R[cur].d[0] = z[i]);
        CALLV(E_X2, 0, "GetZoneX2", R[cur].d[0] = z[i]);
        CALLV(E_Y2, 0, "GetZoneY2", R[cur].d[0] = z[i]);
    }
}

static void info(ULONG zmax)
{
    static const ULONG z[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0x12340003UL };
    unsigned i;
    for (i = 0; i < sizeof z / sizeof z[0]; i++) {
        if ((z[i] & 0xffff) >= zmax)
            continue;
        sprintf(VEC, "zone $%lX", z[i]);
        CALLV(E_X1, 0, "GetZoneX1", R[cur].d[0] = z[i]);
        CALLV(E_Y1, 0, "GetZoneY1", R[cur].d[0] = z[i]);
        CALLV(E_X2, 0, "GetZoneX2", R[cur].d[0] = z[i]);
        CALLV(E_Y2, 0, "GetZoneY2", R[cur].d[0] = z[i]);
    }
}

static const WORD grid[] = { -32768, -150, -100, -1, 0, 1, 9, 10, 11, 50, 99, 100, 101,
                             150, 199, 200, 305, 400, 32767 };
#define NG (sizeof grid / sizeof grid[0])

static void points(const char *mode)
{
    unsigned i, j;
    ULONG hits = 0;
    ROUT = "Zone";
    for (i = 0; i < NG; i++)
        for (j = 0; j < NG; j++) {
            sprintf(VEC, "%s (%d,%d)", mode, grid[i], grid[j]);
            DO2(ENT(LIB, E_ZONE, 0), R[cur].d[0] = 0x5a5a0000UL | (UWORD)grid[i];
                R[cur].d[1] = 0xa5a50000UL | (UWORD)grid[j]);
            SAME("d0", R[cur].d[0]);
            if (R[0].d[0] != (ULONG)-1)
                hits++;
        }
    printf("rizonejoylib: %s: %lu of %u points in a zone\n", mode, hits, (unsigned)(NG * NG));
    check("rizonejoylib: points in zones", hits > 0);
    for (i = 0; i < 12; i++) {
        sprintf(VEC, "%s zone %u (50,55)", mode, i);
        CALLV(E_ZTEST, 0, "ZoneTest 1", R[cur].d[0] = i; R[cur].d[1] = 50; R[cur].d[2] = 55);
        sprintf(VEC, "%s zone %u (150,150)", mode, i);
        CALLV(E_ZTEST, 0, "ZoneTest 1", R[cur].d[0] = i; R[cur].d[1] = 150; R[cur].d[2] = 150);
    }
    {
        static const ULONG r[][2] = { { 0, 255 }, { 1, 255 }, { 2, 3 }, { 3, 2 }, { 300, 255 },
                                      { 0, 256 }, { 5, 5 }, { 0x10002, 0x10004 } };
        for (i = 0; i < sizeof r / sizeof r[0]; i++) {
            sprintf(VEC, "%s %lu..%lu (150,150)", mode, r[i][0], r[i][1]);
            CALLV(E_ZTEST, 1, "ZoneTest range", R[cur].d[0] = r[i][0]; R[cur].d[1] = r[i][1];
                  R[cur].d[2] = 150; R[cur].d[3] = 150);
            sprintf(VEC, "%s %lu..%lu (305,0)", mode, r[i][0], r[i][1]);
            CALLV(E_ZTEST, 1, "ZoneTest range", R[cur].d[0] = r[i][0]; R[cur].d[1] = r[i][1];
                  R[cur].d[2] = 305; R[cur].d[3] = 0);
        }
    }
}

static void setzones(void)
{
    static const WORD rects[][5] = {
        { 0, 100, 100, 10, 10 },        /* swapped corners */
        { 1, 50, 50, 200, 60 },
        { 5, -100, -50, 50, -1 },       /* y2 = -1: a "circle" */
        { 6, -32768, -32768, 32767, 32767 },     /* x1 < 0: not set */
        { 7, 0, -200, 10, -100 },       /* y2 < 0: a circle in 68020 mode */
        { 9, 300, -32768, 310, 32767 }, /* y2 - y overflows */
        { 10, 10, 10, 10, 10 },         /* a point */
    };
    static const WORD circles[][4] = {
        { 2, 150, 150, 40 }, { 3, 0, 0, 200 }, { 4, -100, -100, 300 }, { 8, 400, 400, 1 },
    };
    unsigned i;
    for (i = 0; i < sizeof rects / sizeof rects[0]; i++) {
        sprintf(VEC, "%d: %d,%d,%d,%d", rects[i][0], rects[i][1], rects[i][2], rects[i][3],
                rects[i][4]);
        CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = rects[i][0];
              R[cur].d[1] = 0x11110000UL | (UWORD)rects[i][1];
              R[cur].d[2] = 0x22220000UL | (UWORD)rects[i][2];
              R[cur].d[3] = 0x33330000UL | (UWORD)rects[i][3];
              R[cur].d[4] = 0x44440000UL | (UWORD)rects[i][4]);
    }
    for (i = 0; i < sizeof circles / sizeof circles[0]; i++) {
        sprintf(VEC, "%d: %d,%d r %d", circles[i][0], circles[i][1], circles[i][2],
                circles[i][3]);
        CALLV(E_SETZONE, 0, "SetZone circle", R[cur].d[0] = circles[i][0];
              R[cur].d[1] = (UWORD)circles[i][1]; R[cur].d[2] = (UWORD)circles[i][2];
              R[cur].d[3] = 0x77770000UL | (UWORD)circles[i][3]);
    }
    strcpy(VEC, "zone 256 of 256");
    CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = 256; R[cur].d[1] = 1; R[cur].d[2] = 1;
          R[cur].d[3] = 2; R[cur].d[4] = 2);
    strcpy(VEC, "zone 255");
    CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = 255; R[cur].d[1] = 0; R[cur].d[2] = 0;
          R[cur].d[3] = 1000; R[cur].d[4] = 1000);
}

static void joysticks(void)
{
    int a, p;
    for (a = 0; a < 2; a++) {
        sprintf(VEC, "adaptor %s", a ? "on" : "off");
        CALLV(E_ADAPTOR, 0, "JAdaptorStatus", R[cur].d[0] = a ? 0x123400ffUL : 0x12340000UL);
        for (p = 0; p < 6; p++) {
            sprintf(VEC, "adaptor %s port %d", a ? "on" : "off", p);
            CALLV(E_JFIRE, 0, "JFire", R[cur].d[0] = 0x5a5a5a00UL | p);
            CALLV(E_JVERT, 0, "JVert", R[cur].d[0] = 0x5a5a5a00UL | p);
            CALLV(E_JHORIZ, 0, "JHoriz", R[cur].d[0] = 0x5a5a5a00UL | p);
            CALLV(E_JFIRE, 0, "JFire", R[cur].d[0] = 0x100 | p);
        }
        sprintf(VEC, "adaptor %s", a ? "on" : "off");
        CALLV(E_ALLFIRE, 0, "AllFire", (void)0);
        for (p = 0; p < 16; p++) {
            sprintf(VEC, "adaptor %s bits %d", a ? "on" : "off", p);
            CALLV(E_ALLFIRE, 1, "AllFire bits", R[cur].d[0] = 0xabcdef00UL | p);
        }
    }
    for (p = 0; p < 3; p++) {
        sprintf(VEC, "port %d", p);
        CALLV(E_PAD, 0, "ReadJoyPad", R[cur].d[0] = p);
    }
    strcpy(VEC, "port $10002");
    CALLV(E_PAD, 0, "ReadJoyPad", R[cur].d[0] = 0x10002);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    ULONG t0[2], avail;
    APTR old;
    unsigned i;

    if (argc < 2) {
        printf("usage: rizonejoylib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;

    /* ------------------------------------------------ with table 0 */
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    strcpy(VEC, "");
    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    CALLV(E_ZMODE, 0, "ZoneMode", (void)0);
    ROUT = "ZoneTable";
    DO2(ENT(LIB, E_ZTABLE, 0), (void)0);
    SAME("table", R[cur].d[0] != 0 && R[cur].d[0] != (ULONG)-1);
    for (cur = 0; cur < 2; cur++)
        t0[cur] = R[cur].d[0];
    findlist();
    if (!zlist[0] || !zlist[1])
        return rwe_done(LIB);
    CALLV(E_ZSIZE, 0, "ZoneTableSize", (void)0);
    CALLV(E_ZTNUM, 0, "ZoneTableNumber", (void)0);
    info(256);
    setzones();
    info(256);

    strcpy(VEC, "-1");
    CALLV(E_SETZMODE, 0, "SetZoneMode", R[cur].d[0] = 0xffffffffUL);
    CALLV(E_ZMODE, 0, "ZoneMode", (void)0);
    BOTH("68020 mode on (a 68020 in the emulator)", (UBYTE)R[cur].d[0] == 0xff);
    points("020");
    strcpy(VEC, "0");
    CALLV(E_SETZMODE, 0, "SetZoneMode", R[cur].d[0] = 0x12345600UL);
    CALLV(E_ZMODE, 0, "ZoneMode", (void)0);
    points("000");

    /* ZoneInit: one zone, ranges, all */
    {
        static const ULONG one[] = { 2, 255, 256, 0x10001 };
        static const ULONG rng[][2] = { { 3, 4 }, { 4, 3 }, { 0, 256 }, { 256, 256 },
                                        { 255, 255 } };
        for (i = 0; i < sizeof one / sizeof one[0]; i++) {
            sprintf(VEC, "zone $%lX", one[i]);
            CALLV(E_ZINIT, 1, "ZoneInit zone", R[cur].d[0] = one[i]);
        }
        for (i = 0; i < sizeof rng / sizeof rng[0]; i++) {
            sprintf(VEC, "%lu..%lu", rng[i][0], rng[i][1]);
            CALLV(E_ZINIT, 2, "ZoneInit range", R[cur].d[0] = rng[i][0];
                  R[cur].d[1] = rng[i][1]);
        }
        info(256);
        points("000 after ZoneInit");
        strcpy(VEC, "all");
        CALLV(E_ZINIT, 0, "ZoneInit", (void)0);
        info(256);
    }

    /* zone tables */
    {
        static const ULONG bad[][2] = { { 0, 10 }, { 16, 10 }, { 3, 1 }, { 3, 65536 },
                                        { 0x80000000UL, 10 } };
        for (i = 0; i < sizeof bad / sizeof bad[0]; i++) {
            sprintf(VEC, "%lX, %lu", bad[i][0], bad[i][1]);
            CALLV(E_NEWZT, 0, "NewZoneTable", R[cur].d[0] = bad[i][0]; R[cur].d[1] = bad[i][1]);
        }
    }
    setzones();
    strcpy(VEC, "3, 300 (initialises table 0)");
    CALLV(E_NEWZT, 0, "NewZoneTable", R[cur].d[0] = 3; R[cur].d[1] = 300);
    info(256);
    CALLV(E_ZTNUM, 0, "ZoneTableNumber", (void)0);
    strcpy(VEC, "16");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 16);
    strcpy(VEC, "5 (empty)");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 5);
    strcpy(VEC, "3");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 3);
    CALLV(E_ZTNUM, 0, "ZoneTableNumber", (void)0);
    CALLV(E_ZSIZE, 0, "ZoneTableSize", (void)0);
    CALLV(E_ZINIT, 0, "ZoneInit", (void)0);
    setzones();
    strcpy(VEC, "table 3: zone 260");
    CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = 260; R[cur].d[1] = 0; R[cur].d[2] = 0;
          R[cur].d[3] = 20; R[cur].d[4] = 20);
    info(300);
    {
        static const ULONG r[][2] = { { 256, 299 }, { 260, 260 }, { 250, 270 }, { 4, 299 } };
        for (i = 0; i < sizeof r / sizeof r[0]; i++) {
            sprintf(VEC, "table 3: %lu..%lu (5,5)", r[i][0], r[i][1]);
            CALLV(E_ZTEST, 1, "ZoneTest range", R[cur].d[0] = r[i][0]; R[cur].d[1] = r[i][1];
                  R[cur].d[2] = 5; R[cur].d[3] = 5);
        }
    }
    strcpy(VEC, "table 3");
    CALLV(E_ZONE, 0, "Zone", R[cur].d[0] = 150; R[cur].d[1] = 150);
    strcpy(VEC, "0");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 0);
    strcpy(VEC, "7, 2");
    CALLV(E_NEWZT, 0, "NewZoneTable", R[cur].d[0] = 7; R[cur].d[1] = 2);
    strcpy(VEC, "3, 40 (replaces 3)");
    CALLV(E_NEWZT, 0, "NewZoneTable", R[cur].d[0] = 3; R[cur].d[1] = 40);
    strcpy(VEC, "7");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 7);
    CALLV(E_ZINIT, 0, "ZoneInit", (void)0);
    info(2);
    CALLV(E_SETZONE, 1, "SetZone", R[cur].d[0] = 1; R[cur].d[1] = 3; R[cur].d[2] = 4;
          R[cur].d[3] = 5; R[cur].d[4] = 6);
    info(2);
    {
        static const ULONG f[] = { 0, 16, 5 };
        for (i = 0; i < sizeof f / sizeof f[0]; i++) {
            sprintf(VEC, "%lu", f[i]);
            CALLV(E_FREEZT, 0, "FreeZoneTable", R[cur].d[0] = f[i]);
        }
    }
    strcpy(VEC, "7 (current)");
    CALLV(E_FREEZT, 0, "FreeZoneTable", R[cur].d[0] = 7);
    SAME("entry 7 still set", zlist[cur][7] != 0);
    for (cur = 0; cur < 2; cur++)
        zlist[cur][7] = 0;                      /* not freed twice by finit */
    CALLV(E_ZTNUM, 0, "ZoneTableNumber", (void)0);
    ROUT = "ZoneTable";
    DO2(ENT(LIB, E_ZTABLE, 0), (void)0);
    SAME("table 0 again", R[cur].d[0] == t0[cur]);

    joysticks();

    strcpy(VEC, "");
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    for (cur = 0; cur < 2; cur++)
        for (i = 0; i < 16; i++)
            zlist[cur][i] = 0;                  /* finit leaves them set */
    rwe_mem_check(LIB, avail, TRUE);

    /* ------------------------------------------- no memory for table 0 */
    Forbid();
    old = SetFunction((struct Library *)SysBase, -198, (APTR)rwe_fail0);
    CacheClearU();
    strcpy(VEC, "no memory");
    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, -198, old);
    CacheClearU();
    Permit();
    notable();
    strcpy(VEC, "no table");
    CALLV(E_USEZT, 0, "UseZoneTable", R[cur].d[0] = 0);
    CALLV(E_FREEZT, 0, "FreeZoneTable", R[cur].d[0] = 1);
    joysticks();
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    return rwe_done(LIB);
}
