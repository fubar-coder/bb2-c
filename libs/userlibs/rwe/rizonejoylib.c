/*
 * rizonejoylib - RWE userlib 64 (Reflective Images, 12/8/95): screen
 * zones (rectangles and circles, in up to 16 zone tables) and joystick
 * reading (two ports, the four player adaptor, the CD32 pad) - ZoneInit,
 * SetZone, Zone, ZoneTest, ZoneTable, JFire, JVert, JHoriz, AllFire,
 * ZoneTableSize, NewZoneTable, UseZoneTable, FreeZoneTable,
 * ZoneTableNumber, ZoneMode, SetZoneMode, JAdaptorStatus,
 * GetZoneX1/Y1/X2/Y2, ReadJoyPad.
 *
 * C translation of rwelibsrc/zonejoylib/ZoneJoy-Library4.bb (versions 3
 * and 4 have the same declarations; the reference build of 4 is the one
 * that equals the binary, docs/rwelibs/README.md 3.1); reference BSS2
 * RIZONEJOYLIB.OBJ.  Notes: docs/rwelibs/port/rizonejoylib.md.
 *
 * A zone table: count.l, then per zone x1.w, x2.w, y1.w, y2.w; x1 < 0 =
 * not set (ZoneInit: all -1); a circle has y2 = -1 (68020 mode: any
 * negative y2), x2 = the radius squared (a word), (x1, y1) the centre.
 * init allocates table 0 with 256 zones (MEMF_PUBLIC), finit frees every
 * table.  The 68020 mode (ZoneMode, SetZoneMode; on by default on a
 * 68020 or better) tests rectangles with CMP2, here the same signed
 * bounds test in C.
 *
 * No assembler besides the thunks.  ReadJoyPad clocks the CD32 pad's
 * shift register through the fire line; its delays are CIA reads (each
 * synchronised to the E clock), kept as the same volatile accesses in
 * the same order, so its timing does not depend on the code around them.
 *
 * BUG(orig):
 *   - No routine checks a zone number for < 0 (ZoneInit2, SetZone write
 *     before the table), nor NewZoneTable/UseZoneTable/FreeZoneTable a
 *     negative table number (entry -1 is _zonetable itself).
 *   - NewZoneTable initialises the CURRENT table (ZoneInit1 on
 *     _zonetable), not the new one, which keeps AllocMem's garbage; if
 *     the old table of that number was freed and the AllocMem fails, the
 *     entry keeps the freed block.
 *   - FreeZoneTable leaves the freed block in its entry (finit and a
 *     NewZoneTable of that number free it again) and always makes table
 *     0 the current one.  finit leaves all entries set and init clears
 *     only entry 0: a second init/finit frees the other tables again.
 *   - SetZone without a table returns -1 (= done); ZoneTable without a
 *     table returns -1, not 0.
 *   - ZoneTest(start,end,x,y) finds the start zone through the low BYTE
 *     of start (the counter is a word): from zone 256 on it tests the
 *     wrong zones under the right numbers.
 *   - The 68000 test of x2/y2 uses BMI (the sign of x2-x), not BLT: wrong
 *     for differences beyond 32767.  The circle test squares word
 *     differences; the radius squared is stored as a word (radius >= 256
 *     wraps), and the 68020 mode compares the distance as a word too.
 *   - GetZoneX1/Y1/X2/Y2 compare the zone count with the field offset
 *     (0/2/4/6) instead of the zone number: no range check (a table of up
 *     to 6 zones returns -1 for some fields), the index is a word, and
 *     without a table they read from address 0.
 *   - JHoriz(1) "right" returns d0 unchanged (the port number 1, upper
 *     bits included); ReadJoyPad reads the joystick port for every port
 *     number but 2.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, below) */
void _ZoneInit1(void), _ZoneInit2(void), _ZoneInit(void), _SetZone2(void), _SetZone(void);
void _ZoneTest(void), _TestZoneRange2(void), _TestZoneRange(void), _ZoneTable(void);
void _jfire(void), _jvert(void), _jhoriz(void), _allfire(void), _allfire2(void);
void _ZoneTableSize(void), _NewZoneTable(void), _UseZoneTable(void), _FreeZoneTable(void);
void _zonetablenumber(void), _zonemode(void), _setzonemode(void), _fouradaptor(void);
void _zoneinfox1(void), _zoneinfoy1(void), _zoneinfox2(void), _zoneinfoy2(void);
void _readjoypad(void), _libfinit(void), _Initmem(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rizonejoylib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rizonejoylib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneInit1, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneInit2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneInit, 0, 0);
BLITZ_NAME("ZoneInit", "[zone_num#]|[#start_num,#end_num] - intialize zones");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_SetZone2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_SetZone, 0, 0);
BLITZ_NAME("SetZone", "num,x1,y1[,x2,y2]|[,radius]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneTest, 0, 0);
BLITZ_NAME("Zone", "(x,y)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_TestZoneRange2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_TestZoneRange, 0, 0);
BLITZ_NAME("ZoneTest", "(start_num[,end_num],x,y)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneTable, 0, 0);
BLITZ_NAME("ZoneTable", "- returns address of the current zone table");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_jfire, 0, 0);
BLITZ_NAME("JFire", "(port)");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_jvert, 0, 0);
BLITZ_NAME("JVert", "(port)");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_jhoriz, 0, 0);
BLITZ_NAME("JHoriz", "(port)");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_allfire, 0, 0);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_allfire2, 0, 0);
BLITZ_NAME("AllFire", "([joybits])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_ZoneTableSize, 0, 0);
BLITZ_NAME("ZoneTableSize", "- returns size of the current zone table");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_NewZoneTable, 0, 0);
BLITZ_NAME("NewZoneTable", "table#,num_zones");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_UseZoneTable, 0, 0);
BLITZ_NAME("UseZoneTable", " - select a zone table for use");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_FreeZoneTable, 0, 0);
BLITZ_NAME("FreeZoneTable", "table#");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_zonetablenumber, 0, 0);
BLITZ_NAME("ZoneTableNumber", "- returns the number of the current zone table");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_zonemode, 0, 0);
BLITZ_NAME("ZoneMode", " - returns -1 for 68020 mode or 0 for 68000 mode");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_setzonemode, 0, 0);
BLITZ_NAME("SetZoneMode", "mode");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fouradaptor, 0, 0);
BLITZ_NAME("JAdaptorStatus", "On/Off - status of four player adaptor reading");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_zoneinfox1, 0, 0);
BLITZ_NAME("GetZoneX1", "(zone) - returns the start x pos of a zone");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_zoneinfoy1, 0, 0);
BLITZ_NAME("GetZoneY1", "(zone) - returns the start y pos of a zone");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_zoneinfox2, 0, 0);
BLITZ_NAME("GetZoneX2", "(zone) - returns the end x pos of a zone");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_zoneinfoy2, 0, 0);
BLITZ_NAME("GetZoneY2", "(zone) - returns the end pos of a zone");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_readjoypad, 0, 0);
BLITZ_NAME("ReadJoyPad", "port#      --B,R,Y,G,FF,RW,PL,0");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _Initmem, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

/* endzone, _zonetable, zonetablelist, zones020, is020 in the original's
   order: the entry -1 of the list is _zonetable (negative table numbers,
   BUG(orig)) */
static struct BL_PACKED {
    UWORD endzone;                                      /* Dc.w 1, unused */
    ULONG *zonetable;                                   /* _zonetable */
    ULONG *list[16];                                    /* zonetablelist */
    UBYTE zones020, is020;
} zd = { 1, 0, { 0 }, 0, 0 };

static UBYTE fourplayer = 0xff;                         /* Dc.b -1 */

/* CMP.l b,a : BMI - the sign of a - b (no overflow check, like BMI) */
#define MI(a, b) ((((ULONG)(a) - (ULONG)(b)) & 0x80000000UL) != 0)
/* the sign of a word difference (CMP.w b,a : BMI) */
#define MIW(a, b) ((((UWORD)(a) - (UWORD)(b)) & 0x8000) != 0)
/* ASL.w #n,d / ADDQ.w: the low word changed, the upper word kept */
#define ASLW(d, n) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) << (n)))
#define INCW(d) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) + 1))

/* the list entry of a table number: offset n*4 by ASL.w, added as a long */
static ULONG **entry(ULONG n)
{
    return (ULONG **)((UBYTE *)zd.list + (LONG)ASLW(n, 2));
}

/* a zone: x1, x2, y1, y2 */
static WORD *zone(ULONG *zt, ULONG off)
{
    return (WORD *)((UBYTE *)(zt + 1) + (LONG)off);
}

/* _free_mem: a table of count*8+4 bytes */
static void free_mem(ULONG *t)
{
    FreeMem(t, (t[0] << 3) + 4);
}

/* ------------------------------------------------------- init, finit */

static void BLITZ_ROUTINE initmem_c(void);

static void BLITZ_ROUTINE libfinit_c(void)
{
    int i;

    for (i = 0; i < 16; i++)
        if (zd.list[i])
            free_mem(zd.list[i]);
}

/* ------------------------------------------------------------ ZoneInit */

/* ZoneInit start,end (d0, d1): -1 without table or for a bad range */
static ULONG BLITZ_ROUTINE zoneinit_c(ULONG d0 BL_REG(d0), ULONG d1 BL_REG(d1))
{
    ULONG *zt = zd.zonetable, last;
    LONG *z;
    UWORD end;

    if (!zt)
        return (ULONG)-1;                               /* _nozones */
    last = zt[0] - 1;
    if (MI(last, d0) || MI(last, d1) || MI(d1, d0))
        return (ULONG)-1;
    end = (UWORD)(d1 + 1);                              /* ADDQ.w #1,d1 */
    z = (LONG *)zone(zt, ASLW(d0, 3));
    do {
        *z++ = -1;
        *z++ = -1;
        d0 = INCW(d0);
    } while ((UWORD)d0 != end);
    return d0;
}

/* ZoneInit: all zones of the current table (without one, the original
   reads the count from address 0 first - not observable, left out) */
static ULONG BLITZ_ROUTINE zoneinit1_c(void)
{
    if (!zd.zonetable)
        return (ULONG)-1;
    return zoneinit_c(0, zd.zonetable[0] - 1);
}

/* ZoneInit zone (d0): only the upper bound is checked */
static ULONG BLITZ_ROUTINE zoneinit2_c(ULONG d0 BL_REG(d0))
{
    ULONG *zt = zd.zonetable;
    LONG *z;

    if (!zt || MI(zt[0] - 1, d0))
        return (ULONG)-1;
    d0 = ASLW(d0, 3);
    z = (LONG *)zone(zt, d0);
    z[0] = -1;
    z[1] = -1;
    return d0;
}

/* init: is020 (AttnFlags bit AFB_68020), table 0 with 256 zones */
static void BLITZ_ROUTINE initmem_c(void)
{
    UBYTE f = (SysBase->AttnFlags & AFF_68020) ? 0xff : 0;     /* SF/ST */
    ULONG *mem;

    zd.is020 = f;
    zd.zones020 = f;
    zd.zonetable = NULL;
    zd.list[0] = NULL;
    mem = AllocMem(2052, MEMF_PUBLIC);
    if (!mem)
        return;
    zd.zonetable = mem;
    zd.list[0] = mem;
    mem[0] = 256;
    zoneinit1_c();
}

/* ------------------------------------------------------------- SetZone */

/* doset: zone d0 = x1, x2, y1, y2 */
static ULONG doset(ULONG n, UWORD x1, UWORD y1, UWORD x2, UWORD y2)
{
    ULONG *zt = zd.zonetable;
    WORD *z;

    if (!zt)
        return (ULONG)-1;                           /* _nozones: BUG(orig) */
    if (MI(zt[0] - 1, n))
        return 0;
    z = zone(zt, ASLW(n, 3));
    z[0] = x1;
    z[1] = x2;
    z[2] = y1;
    z[3] = y2;
    return (ULONG)-1;
}

/* SetZone num,x1,y1,x2,y2 (d0-d4): x1 < x2 and y1 < y2 by EXG */
static ULONG BLITZ_ROUTINE setzone_c(ULONG n BL_REG(d0), UWORD x1 BL_REG(d1),
                                     UWORD y1 BL_REG(d2), UWORD x2 BL_REG(d3),
                                     UWORD y2 BL_REG(d4))
{
    UWORD t;

    if (!((WORD)x2 > (WORD)x1)) {                       /* CMP.w d1,d3:BGT */
        t = x1;
        x1 = x2;
        x2 = t;
    }
    if (!((WORD)y2 > (WORD)y1)) {
        t = y1;
        y1 = y2;
        y2 = t;
    }
    return doset(n, x1, y1, x2, y2);
}

/* SetZone num,x,y,radius (d0-d3): a circle, x2 = radius^2 (a word) */
static ULONG BLITZ_ROUTINE setzone2_c(ULONG n BL_REG(d0), UWORD x BL_REG(d1),
                                      UWORD y BL_REG(d2), UWORD r BL_REG(d3))
{
    return doset(n, x, y, (UWORD)bl_muls_w(r, r), 0xffff);
}

/* ---------------------------------------------------------- zone tests */

/* _TestZoneRange: start d0, end d1, x, y - the number of the first zone
   that contains (x,y), -1 if none */
static ULONG testrange(ULONG d0, ULONG d1, UWORD x, UWORD y)
{
    ULONG *zt = zd.zonetable, last;
    const WORD *z;
    UWORD end;

    if (!zt)
        return (ULONG)-1;
    last = zt[0] - 1;
    if (MI(last, d0))
        d0 = last;
    if (MI(last, d1) || MI(d1, d0))
        return (ULONG)-1;
    end = (UWORD)(d1 + 1);
    z = zone(zt, (UWORD)((UBYTE)d0 << 3));              /* MOVE.b d0,d4: BUG(orig) */
    do {
        WORD x1 = z[0], x2 = z[1], y1 = z[2], y2 = z[3];

        z += 4;
        if (x1 >= 0) {                                  /* TST.w: BMI = not set */
            if (zd.zones020 ? y2 < 0 : y2 == -1) {      /* circle */
                WORD dx = (WORD)(x - x1), dy = (WORD)(y - y1);
                ULONG d = (ULONG)bl_muls_w(dx, dx) + (ULONG)bl_muls_w(dy, dy);
                ULONG r = (UWORD)x2;                    /* ANDI.l #$ffff,d6 */

                if (zd.zones020 ? (WORD)r >= (WORD)d    /* CMP.w: BUG(orig) */
                                : (LONG)r >= (LONG)d)   /* CMP.l:BGE */
                    return d0;
            } else if (zd.zones020) {
                /* CMP2.w (a0),d2 / CMP2.w 4(a0),d3: signed bounds */
                if ((WORD)x >= x1 && (WORD)x <= x2 && (WORD)y >= y1 && (WORD)y <= y2)
                    return d0;
            } else {
                /* x1 > x, y1 > y: BGT; x2 - x, y2 - y < 0: BMI (BUG(orig)) */
                if (!(x1 > (WORD)x) && !(y1 > (WORD)y) && !MIW(x2, x) && !MIW(y2, y))
                    return d0;
            }
        }
        d0 = INCW(d0);
    } while ((UWORD)d0 != end);
    return (ULONG)-1;
}

/* Zone(x,y) (d0, d1): all zones of the table */
static ULONG BLITZ_ROUTINE zonetest_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1))
{
    if (!zd.zonetable)
        return (ULONG)-1;
    return testrange(0, zd.zonetable[0] - 1, x, y);
}

/* ZoneTest(start,x,y) (d0-d2): one zone */
static ULONG BLITZ_ROUTINE testzonerange2_c(ULONG s BL_REG(d0), UWORD x BL_REG(d1),
                                            UWORD y BL_REG(d2))
{
    return testrange(s, s, x, y);
}

/* ZoneTest(start,end,x,y) (d0-d3) */
static ULONG BLITZ_ROUTINE testzonerange_c(ULONG s BL_REG(d0), ULONG e BL_REG(d1),
                                           UWORD x BL_REG(d2), UWORD y BL_REG(d3))
{
    return testrange(s, e, x, y);
}

/* --------------------------------------------------------- zone tables */

/* ZoneTable: the current table, -1 if none (BUG(orig)) */
static ULONG BLITZ_ROUTINE zonetable_c(void)
{
    return zd.zonetable ? (ULONG)zd.zonetable : (ULONG)-1;
}

static ULONG BLITZ_ROUTINE zonetablesize_c(void)
{
    return zd.zonetable ? zd.zonetable[0] : 0;
}

/* NewZoneTable(n,zones) (d0, d1): n 1..15, zones 2..65535 */
static ULONG BLITZ_ROUTINE newzonetable_c(ULONG n BL_REG(d0), ULONG num BL_REG(d1))
{
    ULONG **e, *mem;

    if (!n || !MI(n, 16) || MI(num, 2) || !MI(num, 65536))
        return 0;                                       /* _return0 */
    e = entry(n);
    if (*e)
        free_mem(*e);
    mem = AllocMem((num << 3) + 4, MEMF_PUBLIC);
    if (!mem)
        return 0;
    *e = mem;
    mem[0] = num;
    zoneinit1_c();                                      /* BUG(orig): _zonetable */
    return (ULONG)-1;
}

/* UseZoneTable n (d0): n < 16 with a table */
static ULONG BLITZ_ROUTINE usezonetable_c(ULONG n BL_REG(d0))
{
    ULONG **e;

    if (!MI(n, 16))
        return 0;
    e = entry(n);
    if (!*e)
        return 0;
    zd.zonetable = *e;
    return (ULONG)-1;
}

/* FreeZoneTable(n) (d0): n 1..15; the entry stays, table 0 current */
static ULONG BLITZ_ROUTINE freezonetable_c(ULONG n BL_REG(d0))
{
    ULONG **e;

    if (!n || !MI(n, 16))
        return 0;
    e = entry(n);
    if (!*e)
        return 0;
    free_mem(*e);                                       /* BUG(orig): stays */
    zd.zonetable = zd.list[0];
    return (ULONG)-1;
}

/* ZoneTableNumber: the list index of the current table, -1 */
static ULONG BLITZ_ROUTINE zonetablenumber_c(void)
{
    int i;

    if (!zd.zonetable)
        return (ULONG)-1;
    for (i = 0; i < 16; i++)
        if (zd.list[i] == zd.zonetable)
            return i;
    return (ULONG)-1;
}

/* ZoneMode: MOVE.b zones020,d0 (thunk: the low byte only) */
static ULONG BLITZ_ROUTINE zonemode_c(void)
{
    return zd.zones020;
}

/* SetZoneMode mode (d0.b): only on a 68020 */
static void BLITZ_ROUTINE setzonemode_c(ULONG mode BL_REG(d0))
{
    if (zd.is020)
        zd.zones020 = (UBYTE)mode;
}

/* GetZoneX1/X2/Y1/Y2(zone) (d0.w): field off of the zone; the low word
   of d0 replaced (MOVE.w 0(a0,d0.w),d0) */
static ULONG zoneinfo(ULONG d0, UWORD off)
{
    const ULONG *zt = zd.zonetable;                     /* 0: reads address 0 */

    if (!((LONG)zt[0] > (LONG)off))                     /* BUG(orig): count vs offset */
        return (ULONG)-1;
    return (d0 & 0xffff0000UL)
        | *(const UWORD *)((const UBYTE *)(zt + 1) + (WORD)((UWORD)(d0 << 3) + off));
}

static ULONG BLITZ_ROUTINE zoneinfox1_c(ULONG d0 BL_REG(d0)) { return zoneinfo(d0, 0); }
static ULONG BLITZ_ROUTINE zoneinfox2_c(ULONG d0 BL_REG(d0)) { return zoneinfo(d0, 2); }
static ULONG BLITZ_ROUTINE zoneinfoy1_c(ULONG d0 BL_REG(d0)) { return zoneinfo(d0, 4); }
static ULONG BLITZ_ROUTINE zoneinfoy2_c(ULONG d0 BL_REG(d0)) { return zoneinfo(d0, 6); }

/* ----------------------------------------------------------- joysticks */

#define CIAA_PRA   (*(volatile UBYTE *)0xbfe001)    /* /FIR1 bit 7, /FIR0 bit 6 */
#define CIAA_PRB   (*(volatile UBYTE *)0xbfe101)    /* parallel port data */
#define CIAA_DDRA  (*(volatile UBYTE *)0xbfe201)
#define CIAB_PRA   (*(volatile UBYTE *)0xbfd000)    /* BUSY bit 0, SEL bit 2 */
#define POTGO      (*(volatile UWORD *)0xdff034)
#define POTINP     (*(volatile UWORD *)0xdff016)
#define POTINP_HI  (*(volatile UBYTE *)0xdff016)    /* BTST #n,$dff016 */
#define JOY0DAT    (*(volatile UWORD *)0xdff00a)
#define JOY1DAT    (*(volatile UWORD *)0xdff00c)

/* JFire(port) (d0.b): 1 = fire, +2 = second button (ports 1, 2) */
static ULONG BLITZ_ROUTINE jfire_c(UWORD port BL_REG(d0))
{
    ULONG r = 0;

    switch ((UBYTE)port) {
    case 1:
        if (!(CIAA_PRA & 0x80))
            r = 1;
        POTGO = 0x4f00;
        if (!(POTINP_HI & 0x40))
            r += 2;
        return r;
    case 2:
        if (!(CIAA_PRA & 0x40))
            r = 1;
        POTGO = 0xf400;
        if (!(POTINP_HI & 0x04))
            r += 2;
        return r;
    case 3:
        if (!fourplayer)                                /* !adaptortest */
            return 0;
        return !(CIAB_PRA & 0x04);
    case 4:
        if (!fourplayer)
            return 0;
        return !(CIAB_PRA & 0x01);
    }
    return 0;
}

/* the direction of a JOYxDAT: -1 / 1 / 0 (JVert: up/down, JHoriz:
   left/right) */
static ULONG vert(UWORD d)
{
    d ^= d >> 1;
    if (d & 0x100)
        return (ULONG)-1;
    return d & 1;
}

/* the adaptor's bits of port 3/4 in CIAA PRB: `first` clear = -1, else
   `second` clear = 1, else 0 */
static ULONG adaptor(UBYTE first, UBYTE second)
{
    UBYTE b;

    if (!fourplayer)
        return 0;
    b = CIAA_PRB;
    if (!(b & first))
        return (ULONG)-1;
    return !(b & second);
}

/* JVert(port) (d0.b) */
static ULONG BLITZ_ROUTINE jvert_c(UWORD port BL_REG(d0))
{
    switch ((UBYTE)port) {
    case 1: return vert(JOY1DAT);
    case 2: return vert(JOY0DAT);
    case 3: return adaptor(0x01, 0x02);
    case 4: return adaptor(0x10, 0x20);
    }
    return 0;
}

/* JHoriz(port) (d0.b): port 1 "right" returns d0 unchanged (BUG(orig)) */
static ULONG BLITZ_ROUTINE jhoriz_c(ULONG d0 BL_REG(d0))
{
    UWORD d;

    switch ((UBYTE)d0) {
    case 1:
        d = JOY1DAT;
        if (d & 0x0002)
            return d0;
        return d & 0x0200 ? (ULONG)-1 : 0;
    case 2:
        d = JOY0DAT;
        if (d & 0x0002)
            return 1;
        return d & 0x0200 ? (ULONG)-1 : 0;
    case 3: return adaptor(0x04, 0x08);
    case 4: return adaptor(0x40, 0x80);
    }
    return 0;
}

/* AllFire([bits]) (d0.b, default %1111): a bit per pressed fire button
   of the ports 1..4 asked for (3 and 4 only with the adaptor) */
static ULONG BLITZ_ROUTINE allfire2_c(UWORD bits BL_REG(d0))
{
    UBYTE b = (UBYTE)bits;
    ULONG r = 0;

    if (!fourplayer)
        b &= 3;
    if ((b & 1) && !(CIAA_PRA & 0x80))
        r += 1;
    if ((b & 2) && !(CIAA_PRA & 0x40))
        r += 2;
    if ((b & 4) && !(CIAB_PRA & 0x04))
        r += 4;
    if ((b & 8) && !(CIAB_PRA & 0x01))
        r += 8;
    return r;
}

static ULONG BLITZ_ROUTINE allfire_c(void)
{
    return allfire2_c(0x0f);
}

/* JAdaptorStatus on (d0.b): the old status in d0.b */
static ULONG BLITZ_ROUTINE fouradaptor_c(ULONG d0 BL_REG(d0))
{
    UBYTE old = fourplayer;

    fourplayer = (UBYTE)d0;
    return (d0 & 0xffffff00UL) | old;
}

/* ReadJoyPad(port) (d0.w): the 8 bits of a CD32 pad, clocked through the
   fire line (`fire` in CIAA PRA/DDRA), the data from POTINP bit `data`;
   5 (later 8) CIA reads as the delay */
static ULONG readpad(UBYTE fire, UWORD go, UWORD data)
{
    ULONG r = 0;
    WORD i = 7;
    UWORD pot;

    CIAA_DDRA |= fire;                                  /* BSET #7,$bfe201 */
    CIAA_PRA &= (UBYTE)~fire;
    POTGO = go;
    goto first;
    do {
        (void)CIAA_PRA;
        (void)CIAA_PRA;
        (void)CIAA_PRA;
first:
        (void)CIAA_PRA;
        (void)CIAA_PRA;
        (void)CIAA_PRA;
        (void)CIAA_PRA;
        (void)CIAA_PRA;
        pot = POTINP;
        CIAA_PRA |= fire;
        CIAA_PRA &= (UBYTE)~fire;
        if (!(pot & data))
            r |= 1UL << i;                              /* BSET d1,d0 */
    } while (--i >= 0);                                 /* DBF */
    CIAA_DDRA &= (UBYTE)~fire;
    POTGO = 0xffff;
    return r;
}

static ULONG BLITZ_ROUTINE readjoypad_c(UWORD port BL_REG(d0))
{
    if (port == 2)
        return readpad(0x40, 0xf600, 0x0400);           /* readmouseport */
    return readpad(0x80, 0x6f00, 0x4000);
}

/* Registers: the original changes d0 and, depending on the routine,
   d1-d7, a0, a1, a6; the thunks keep d1/a0/a1, the C code d2-d7/a2-a6.
   ZoneMode replaces only the low byte of d0 (MOVE.b). */
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_Initmem, initmem_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit1, zoneinit1_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit2, zoneinit2_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit, zoneinit_c, "d1/a0-a1");
BLITZ_THUNK(_SetZone2, setzone2_c, "d1/a0-a1");
BLITZ_THUNK(_SetZone, setzone_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneTest, zonetest_c, "d1/a0-a1");
BLITZ_THUNK(_TestZoneRange2, testzonerange2_c, "d1/a0-a1");
BLITZ_THUNK(_TestZoneRange, testzonerange_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneTable, zonetable_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneTableSize, zonetablesize_c, "d1/a0-a1");
BLITZ_THUNK(_NewZoneTable, newzonetable_c, "d1/a0-a1");
BLITZ_THUNK(_UseZoneTable, usezonetable_c, "d1/a0-a1");
BLITZ_THUNK(_FreeZoneTable, freezonetable_c, "d1/a0-a1");
BLITZ_THUNK(_zonetablenumber, zonetablenumber_c, "d1/a0-a1");
BLITZ_THUNK_POST(_zonemode, zonemode_c, "d0-d1/a0-a1", "move.b d0,3(sp)");
BLITZ_THUNK(_setzonemode, setzonemode_c, "d0-d1/a0-a1");
BLITZ_THUNK(_zoneinfox1, zoneinfox1_c, "d1/a0-a1");
BLITZ_THUNK(_zoneinfox2, zoneinfox2_c, "d1/a0-a1");
BLITZ_THUNK(_zoneinfoy1, zoneinfoy1_c, "d1/a0-a1");
BLITZ_THUNK(_zoneinfoy2, zoneinfoy2_c, "d1/a0-a1");
BLITZ_THUNK(_jfire, jfire_c, "d1/a0-a1");
BLITZ_THUNK(_jvert, jvert_c, "d1/a0-a1");
BLITZ_THUNK(_jhoriz, jhoriz_c, "d1/a0-a1");
BLITZ_THUNK(_allfire, allfire_c, "d1/a0-a1");
BLITZ_THUNK(_allfire2, allfire2_c, "d1/a0-a1");
BLITZ_THUNK(_fouradaptor, fouradaptor_c, "d1/a0-a1");
BLITZ_THUNK(_readjoypad, readjoypad_c, "d1/a0-a1");
