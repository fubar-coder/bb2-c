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
 * 68020 or better) tests rectangles with CMP2 (the original's code: only
 * called on a 68020 or better).  The 68000 mode compares words.
 *
 * Zone and ZoneTest are assembler entries (speed: the per-frame test of
 * a game, tests/perf bzone), the rest C.  ReadJoyPad clocks the CD32 pad's
 * shift register through the fire line; its delays are CIA reads (each
 * synchronised to the E clock), kept as the same volatile accesses in
 * the same order, so its timing does not depend on the code around them.
 *
 * BUG(orig):
 *   - No routine checks a zone number for < 0 (ZoneInit2, SetZone write
 *     before the table) (#873).
 *   - NewZoneTable initialises the CURRENT table (ZoneInit1 on
 *     _zonetable), not the new one, which keeps AllocMem's garbage
 *     (#875); if the old table of that number was freed and the AllocMem
 *     fails, the entry keeps the freed block (#874).
 *   - FreeZoneTable leaves the freed block in its entry (finit and a
 *     NewZoneTable of that number free it again) (#868) and always makes
 *     table 0 the current one (#869).  finit leaves all entries set and
 *     init clears only entry 0: a second init/finit frees the other
 *     tables again (#871).
 *   - SetZone without a table returns -1 (= done); ZoneTable without a
 *     table returns -1, not 0 (#876).
 *   - ZoneTest(start,end,x,y) finds the start zone through the low BYTE
 *     of start (the counter is a word): from zone 256 on it tests the
 *     wrong zones under the right numbers (#881).
 *   - The 68000 test of x2/y2 uses BMI (the sign of x2-x), not BLT: wrong
 *     for differences beyond 32767 (#879).  The circle test squares word
 *     differences; the radius squared is stored as a word (radius >= 256
 *     wraps) (#867), and the 68020 mode compares the distance as a word
 *     too (#880).
 *   - GetZoneX1/Y1/X2/Y2 compare the zone count with the field offset
 *     (0/2/4/6) instead of the zone number: no range check (a table of up
 *     to 6 zones returns -1 for some fields), the index is a word, and
 *     without a table they read from address 0 (#870).
 *   - ReadJoyPad reads the joystick port for every port number but 2
 *     (#877).
 * Fixed (intended deviations):
 *   - NewZoneTable, UseZoneTable and FreeZoneTable compare the table
 *     number unsigned: a negative number is refused (0).  The original
 *     compared it signed (BPL on number - 16): UseZoneTable -1 selected
 *     entry -1 of the list, _zonetable itself, FreeZoneTable -1 and
 *     NewZoneTable -1 freed the current table and left it there (#872).
 * Original behaviour that is no defect: JHoriz(1) "right" returns d0
 * unchanged (the port number 1, which is also the result for "right").
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <stddef.h>

/* entry points (thunks, below) */
void _ZoneInit1(void), _ZoneInit2(void), _ZoneInit(void), _SetZone2(void);
void _SetZone(void), _ZoneTest(void), _TestZoneRange2(void);
void _TestZoneRange(void), _ZoneTable(void), _jfire(void), _jvert(void);
void _jhoriz(void), _allfire(void), _allfire2(void), _ZoneTableSize(void);
void _NewZoneTable(void), _UseZoneTable(void), _FreeZoneTable(void);
void _zonetablenumber(void), _zonemode(void), _setzonemode(void);
void _fouradaptor(void), _zoneinfox1(void), _zoneinfoy1(void);
void _zoneinfox2(void), _zoneinfoy2(void), _readjoypad(void);
void rizonejoylib_libfinit(void), _Initmem(void);

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

BLITZ_NULLSUB_AT(finit, rizonejoylib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _Initmem, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

/* a zone: a rectangle x1..x2, y1..y2, or a circle (y2 = -1, x2 = the
   radius squared, (x1, y1) the centre); x1 < 0 = not set */
struct zone {
    WORD x1, x2, y1, y2;
};
static_assert(sizeof(struct zone) == 8);

/* a zone table (AllocMem, count * 8 + 4 bytes) */
struct zonetable {
    ULONG count;
    struct zone zones[];
};
static_assert(sizeof(struct zonetable) == 4);

/* endzone, _zonetable, zonetablelist, zones020, is020 in the original's
   order (the entry -1 of the list is _zonetable; the table numbers are
   compared unsigned, #872).  The zone tests (assembler) read it as _zd. */
struct zdata {
    UWORD endzone;                                      /* Dc.w 1, unused */
    struct zonetable *zonetable;                        /* _zonetable */
    struct zonetable *list[16];                         /* zonetablelist */
    UBYTE zones020, is020;
} BL_PACKED;
static struct zdata zd __asm__("_zd") __attribute__((used)) = { 1, 0, { 0 }, 0, 0 };
static_assert(sizeof zd == 2 + 4 + 16 * 4 + 2);
/* the offsets the zone tests use */
#define ZD_ZONETABLE 2
#define ZD_ZONES020 70
static_assert(offsetof(struct zdata, zonetable) == ZD_ZONETABLE);
static_assert(offsetof(struct zdata, zones020) == ZD_ZONES020);

static UBYTE fourplayer = 0xff;                         /* Dc.b -1 */

/* CMP.l b,a : BMI - the sign of a - b (no overflow check, like BMI) */
#define MI(a, b) ((((ULONG)(a) - (ULONG)(b)) & 0x80000000UL) != 0)
/* the sign of a word difference (CMP.w b,a : BMI) */
#define MIW(a, b) ((((UWORD)(a) - (UWORD)(b)) & 0x8000) != 0)
/* ASL.w #n,d / ADDQ.w: the low word changed, the upper word kept */
#define ASLW(d, n) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) << (n)))
#define INCW(d) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) + 1))

/* the list entry of a table number: offset n*4 by ASL.w, added as a long */
static struct zonetable **entry(ULONG n)
{
    return (struct zonetable **)((UBYTE *)zd.list + (LONG)ASLW(n, 2));
}

/* the zone at the byte offset `off` (zone# << 3, computed by the caller as
   the original does) */
static struct zone *zone(struct zonetable *zt, ULONG off)
{
    return (struct zone *)((UBYTE *)(zt + 1) + (LONG)off);    /* zt->zones */
}

/* _free_mem: a table of count*8+4 bytes */
static void free_mem(struct zonetable *table)
{
    FreeMem(table, (table->count << 3) + 4);
}

/* ------------------------------------------------------- init, finit */

static void BLITZ_ROUTINE initmem_c(void);

/**
 * @brief finit: frees every zone table of the list.
 *
 * BUG(orig) (#871): the entries stay set (and #868: FreeZoneTable leaves
 * its freed block there, freed here again).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    int i;

    for (i = 0; i < 16; i++)
        if (zd.list[i])
            free_mem(zd.list[i]);
}

/* ------------------------------------------------------------ ZoneInit */

/**
 * @brief ZoneInit start_num,end_num: clears the zones start..end of the current table.
 *
 * x1..y2 = -1 (not set).  BUG(orig) (#873): start is not checked for < 0.
 * @param zonenum  d0: the first zone
 * @param lastnum  d1: the last zone
 * @return d0: end + 1 (the low word), -1 without a table or for a bad range
 */
static ULONG BLITZ_ROUTINE zoneinit_c(ULONG zonenum BL_REG(d0), ULONG lastnum BL_REG(d1))
{
    struct zonetable *zt = zd.zonetable;
    ULONG last;
    LONG *z;                            /* a zone as two longs: x1/x2, y1/y2 */
    UWORD end;

    if (!zt)
        return (ULONG)-1;                               /* _nozones */
    last = zt->count - 1;
    if (MI(last, zonenum) || MI(last, lastnum) || MI(lastnum, zonenum))
        return (ULONG)-1;
    end = (UWORD)(lastnum + 1);                         /* ADDQ.w #1,d1 */
    z = (LONG *)zone(zt, ASLW(zonenum, 3));
    do {
        *z++ = -1;
        *z++ = -1;
        zonenum = INCW(zonenum);
    } while ((UWORD)zonenum != end);
    return zonenum;
}

/**
 * @brief ZoneInit: clears all zones of the current table.
 *
 * Without a table the original reads the count from address 0 - not
 * observable, left out.
 * @return d0: as ZoneInit start,end; -1 without a table
 */
static ULONG BLITZ_ROUTINE zoneinit1_c(void)
{
    if (!zd.zonetable)
        return (ULONG)-1;
    return zoneinit_c(0, zd.zonetable->count - 1);
}

/**
 * @brief ZoneInit zone_num: clears one zone of the current table.
 *
 * Only the upper bound is checked (BUG(orig) #873: a negative number
 * writes before the table).
 * @param zonenum  d0: the zone
 * @return d0: the zone's byte offset, -1 without a table or out of range
 */
static ULONG BLITZ_ROUTINE zoneinit2_c(ULONG zonenum BL_REG(d0))
{
    struct zonetable *zt = zd.zonetable;
    ULONG off;                          /* d0: the zone's byte offset */
    LONG *z;                            /* the zone as two longs */

    if (!zt || MI(zt->count - 1, zonenum))
        return (ULONG)-1;
    off = ASLW(zonenum, 3);
    z = (LONG *)zone(zt, off);
    z[0] = -1;
    z[1] = -1;
    return off;
}

/**
 * @brief init: the 68020 mode and zone table 0 with 256 zones.
 *
 * is020 and the 68020 mode from AttnFlags (AFB_68020); table 0 (2052
 * bytes, MEMF_PUBLIC) is the current one, all zones cleared; without
 * memory there is no table.  BUG(orig) (#871): only entry 0 of the list
 * is cleared.
 */
static void BLITZ_ROUTINE initmem_c(void)
{
    UBYTE f = (SysBase->AttnFlags & AFF_68020) ? 0xff : 0;     /* SF/ST */
    struct zonetable *mem;

    zd.is020 = f;
    zd.zones020 = f;
    zd.zonetable = NULL;
    zd.list[0] = NULL;
    mem = AllocMem(2052, MEMF_PUBLIC);
    if (!mem)
        return;
    zd.zonetable = mem;
    zd.list[0] = mem;
    mem->count = 256;
    zoneinit1_c();
}

/* ------------------------------------------------------------- SetZone */

/* doset: zone d0 = x1, x2, y1, y2 */
static BB_BOOL doset(ULONG n, UWORD x1, UWORD y1, UWORD x2, UWORD y2)
{
    struct zonetable *zt = zd.zonetable;
    struct zone *z;

    if (!zt)
        return BB_TRUE;                             /* _nozones: BUG(orig) #876 */
    if (MI(zt->count - 1, n))
        return BB_FALSE;
    z = zone(zt, ASLW(n, 3));
    z->x1 = x1;
    z->x2 = x2;
    z->y1 = y1;
    z->y2 = y2;
    return BB_TRUE;
}

/**
 * @brief SetZone(num,x1,y1,x2,y2): sets a rectangular zone of the current table.
 *
 * The corners are ordered (EXG) so that x1 < x2 and y1 < y2.  BUG(orig):
 * no check for num < 0 (#873); without a table the result is -1 (#876).
 * @param n   d0: the zone number
 * @param x1  d1: a corner x
 * @param y1  d2: a corner y
 * @param x2  d3: the other corner x
 * @param y2  d4: the other corner y
 * @return d0: BB_TRUE done (also without a table), BB_FALSE out of range
 */
static BB_BOOL BLITZ_ROUTINE setzone_c(ULONG n BL_REG(d0), UWORD x1 BL_REG(d1),
                                       UWORD y1 BL_REG(d2), UWORD x2 BL_REG(d3),
                                       UWORD y2 BL_REG(d4))
{
    UWORD swap;

    if (!((WORD)x2 > (WORD)x1)) {                       /* CMP.w d1,d3:BGT */
        swap = x1;
        x1 = x2;
        x2 = swap;
    }
    if (!((WORD)y2 > (WORD)y1)) {
        swap = y1;
        y1 = y2;
        y2 = swap;
    }
    return doset(n, x1, y1, x2, y2);
}

/**
 * @brief SetZone(num,x,y,radius): sets a circular zone of the current table.
 *
 * x1, y1 = the centre, x2 = radius^2 as a word (BUG(orig) #867: radius
 * 256 and more wraps), y2 = -1.  Results and bugs as the rectangle form.
 * @param n  d0: the zone number
 * @param x  d1: the centre x
 * @param y  d2: the centre y
 * @param r  d3: the radius
 * @return d0: BB_TRUE done (also without a table), BB_FALSE out of range
 */
static BB_BOOL BLITZ_ROUTINE setzone2_c(ULONG n BL_REG(d0), UWORD x BL_REG(d1),
                                        UWORD y BL_REG(d2), UWORD r BL_REG(d3))
{
    return doset(n, x, y, (UWORD)bl_muls_w(r, r), 0xffff);
}

/* ---------------------------------------------------------- zone tests */

/*
 * Zone(x,y), ZoneTest(start_num,x,y), ZoneTest(start_num,end_num,x,y):
 * assembler entries for speed (a game's per-frame test of a point against
 * the zone table, bzone in tests/perf), in the original's form and
 * without a thunk.  They change d1-d7 and a0 (ZoneTest with a range keeps
 * d2 and d3), as the original does, which the register rule allows for a
 * user-level function.
 *
 * Zone(x,y) (d0.w x, d1.w y): the first zone of the current table that
 * contains x,y - ZoneTest(0,count-1,x,y); -1 if none or without a table
 * (the original reads the count from address 0 before it notices, not
 * observable: here the table is tested first).
 *
 * ZoneTest(start_num,x,y) (d0 start, d1.w x, d2.w y): whether the zone
 * start contains x,y - ZoneTest(start,start,x,y).
 *
 * ZoneTest(start_num,end_num,x,y) (d0 start, d1 end, d2.w x, d3.w y): the
 * first zone of start..end that contains x,y, its number in d0 (the
 * counter: the low word counted, the upper word of start kept); -1 if
 * none, without a table or for a bad range (end beyond the table or
 * before start, CMP.l : BMI); start beyond the table is the last zone.
 * Rectangles x1 <= x <= x2, y1 <= y <= y2; zones with x1 < 0 are not
 * set.  BUG(orig) (#881): the start zone is found through the low byte of
 * start (MOVE.b), from zone 256 on the wrong zones are tested under the
 * right numbers.
 *
 * The 68020 mode (zones020 set): every zone with a negative y2 is a
 * circle (BUG(orig) #866), a rectangle is tested with CMP2.w (x1..x2,
 * y1..y2 as bounds), a circle by its squared distance against the word
 * x2 as words (CMP.w, BUG(orig) #880).  The 68000 mode: a circle has
 * y2 = -1, its squared distance is compared as a long; x1 > x, y1 > y
 * (BGT) and the sign of the word differences x2 - x, y2 - y (BMI, BUG(orig)
 * #879) leave a rectangle.  Both square the word differences x - x1,
 * y - y1 (MULS.w; BUG(orig) #867, the radius squared a word).
 */
#define S_(x) BL_STR(x)
BLITZ_ASM_BLOCK(zonetests,
"   .globl  __ZoneTest\n"
"__ZoneTest:\n"
"   move.l  d1,d3\n"                        /* y */
"   move.l  d0,d2\n"                        /* x */
"   move.l  _zd+" S_(ZD_ZONETABLE) ",d1\n"
"   beq.s   9f\n"
"   move.l  d1,a0\n"
"   moveq   #0,d0\n"                        /* from zone 0 */
"   move.l  (a0)+,d1\n"
"   subq.l  #1,d1\n"                        /* to the last: the DBF count */
"   bpl.s   2f\n"
"   bra.s   __TestZoneRange\n"              /* count 0 or above $80000000 */
"   .globl  __TestZoneRange2\n"
"__TestZoneRange2:\n"
"   move.l  d2,d3\n"                        /* y */
"   move.l  d1,d2\n"                        /* x */
"   move.l  d0,d1\n"                        /* end = start */
"   .globl  __TestZoneRange\n"
"__TestZoneRange:\n"
"   move.l  _zd+" S_(ZD_ZONETABLE) ",d4\n"
"   beq.s   9f\n"
"   move.l  d4,a0\n"
"   move.l  (a0)+,d7\n"
"   subq.l  #1,d7\n"                        /* the last zone */
"   cmp.l   d0,d7\n"
"   bpl.s   1f\n"
"   move.l  d7,d0\n"                        /* start beyond the table: the last */
"1: cmp.l   d1,d7\n"
"   bmi.s   9f\n"
"   cmp.l   d0,d1\n"
"   bmi.s   9f\n"
"   sub.w   d0,d1\n"                        /* the DBF count: end - start */
"   moveq   #0,d4\n"
"   move.b  d0,d4\n"                        /* MOVE.b: BUG(orig) (#881) */
"   lsl.w   #3,d4\n"
"   add.l   d4,a0\n"
"2: tst.b   _zd+" S_(ZD_ZONES020) "\n"
"   beq.s   5f\n"
/* the 68020 mode (only on a 68020 or better: CMP2) */
"3: tst.w   (a0)\n"                         /* x1 < 0: not set */
"   bmi.s   4f\n"
"   tst.w   6(a0)\n"
"   bmi.s   8f\n"                           /* y2 < 0: a circle, BUG(orig) (#866) */
"   .long   0x02d02000\n"                   /* CMP2.w (a0),d2: x1 <= x <= x2 */
"   bcs.s   4f\n"
"   .long   0x02e83000\n"                   /* CMP2.w 4(a0),d3: y1 <= y <= y2 */
"   .word   4\n"
"   bcs.s   4f\n"
"   rts\n"
"4: addq.l  #8,a0\n"
"   addq.w  #1,d0\n"
"   dbf     d1,3b\n"
"9: moveq   #-1,d0\n"
"   rts\n"
"8: movem.w (a0),d4-d7\n"                   /* x1, x2, y1, y2 */
"   sub.w   d2,d4\n"
"   muls.w  d4,d4\n"
"   sub.w   d3,d6\n"
"   muls.w  d6,d6\n"
"   add.l   d6,d4\n"                        /* the squared distance */
"   cmp.w   d4,d5\n"                        /* as words: BUG(orig) (#880) */
"   blt.s   4b\n"
"   rts\n"
/* the 68000 mode */
"5: move.w  (a0)+,d4\n"                     /* x1; < 0: not set */
"   bmi.s   6f\n"
"   movem.w (a0)+,d5-d7\n"                  /* x2, y1, y2 */
"   cmp.w   #-1,d7\n"
"   beq.s   7f\n"                           /* a circle */
"   cmp.w   d4,d2\n"                        /* x1 > x: BGT */
"   blt.s   0f\n"
"   cmp.w   d6,d3\n"                        /* y1 > y */
"   blt.s   0f\n"
"   cmp.w   d2,d5\n"                        /* x2 - x: BMI, BUG(orig) (#879) */
"   bmi.s   0f\n"
"   cmp.w   d3,d7\n"                        /* y2 - y */
"   bmi.s   0f\n"
"   rts\n"
"6: addq.l  #6,a0\n"
"0: addq.w  #1,d0\n"
"   dbf     d1,5b\n"
"   moveq   #-1,d0\n"
"   rts\n"
"7: sub.w   d2,d4\n"
"   muls.w  d4,d4\n"
"   sub.w   d3,d6\n"
"   muls.w  d6,d6\n"
"   add.l   d6,d4\n"                        /* the squared distance */
"   moveq   #0,d7\n"
"   move.w  d5,d7\n"                        /* the radius squared (ANDI.l #$ffff) */
"   cmp.l   d4,d7\n"
"   blt.s   0b\n"
"   rts");
#undef S_

/* --------------------------------------------------------- zone tables */

/**
 * @brief ZoneTable: the address of the current zone table.
 * @return d0: the table, -1 if none (BUG(orig) #876: not 0)
 */
static ULONG BLITZ_ROUTINE zonetable_c(void)
{
    return zd.zonetable ? (ULONG)zd.zonetable : (ULONG)-1;
}

/**
 * @brief ZoneTableSize: the number of zones of the current table.
 * @return d0: the count, 0 without a table
 */
static ULONG BLITZ_ROUTINE zonetablesize_c(void)
{
    return zd.zonetable ? zd.zonetable->count : 0;
}

/**
 * @brief NewZoneTable table#,num_zones: allocates zone table n.
 *
 * n 1..15, num_zones 2..65535; an old table of that number is freed
 * first.  BUG(orig): ZoneInit runs on the CURRENT table, the new one keeps
 * AllocMem's garbage (#875); if the AllocMem fails after the free, the
 * entry keeps the freed block (#874).  Fixed (#872): n is compared
 * unsigned, a negative n is refused (the original passed it: entry -1 is
 * _zonetable).
 * @param n    d0: the table number
 * @param num  d1: the number of zones
 * @return d0: BB_TRUE done, BB_FALSE bad arguments or no memory
 */
static BB_BOOL BLITZ_ROUTINE newzonetable_c(ULONG n BL_REG(d0), ULONG num BL_REG(d1))
{
    struct zonetable **e, *mem;

    /* n unsigned: fixed (#872) */
    if (!n || n >= 16 || MI(num, 2) || !MI(num, 65536))
        return BB_FALSE;                                /* _return0 */
    e = entry(n);
    if (*e)
        free_mem(*e);
    mem = AllocMem((num << 3) + 4, MEMF_PUBLIC);
    if (!mem)
        return BB_FALSE;
    *e = mem;
    mem->count = num;
    zoneinit1_c();                                      /* BUG(orig): _zonetable (#875) */
    return BB_TRUE;
}

/**
 * @brief UseZoneTable table#: makes table n the current one.
 *
 * Fixed (#872): n is compared unsigned, a negative n is refused (the
 * original passed it and selected entry -1, _zonetable itself).
 * @param n  d0: the table number (< 16)
 * @return d0: BB_TRUE done, BB_FALSE n too large or no such table
 */
static BB_BOOL BLITZ_ROUTINE usezonetable_c(ULONG n BL_REG(d0))
{
    struct zonetable **e;

    if (n >= 16)                                        /* unsigned: fixed (#872) */
        return BB_FALSE;
    e = entry(n);
    if (!*e)
        return BB_FALSE;
    zd.zonetable = *e;
    return BB_TRUE;
}

/**
 * @brief FreeZoneTable table#: frees zone table n (1..15).
 *
 * BUG(orig): the freed block stays in its entry (#868), table 0 always
 * becomes the current one (#869).  Fixed (#872): n is compared unsigned,
 * a negative n is refused (the original freed the current table).
 * @param n  d0: the table number
 * @return d0: BB_TRUE done, BB_FALSE bad number or no such table
 */
static BB_BOOL BLITZ_ROUTINE freezonetable_c(ULONG n BL_REG(d0))
{
    struct zonetable **e;

    if (!n || n >= 16)                                  /* unsigned: fixed (#872) */
        return BB_FALSE;
    e = entry(n);
    if (!*e)
        return BB_FALSE;
    free_mem(*e);                                       /* BUG(orig): stays (#868) */
    zd.zonetable = zd.list[0];
    return BB_TRUE;
}

/**
 * @brief ZoneTableNumber: the number of the current zone table.
 * @return d0: its index in the list, -1 without a table (or not listed)
 */
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

/**
 * @brief ZoneMode: -1 for the 68020 mode, 0 for the 68000 mode.
 *
 * MOVE.b zones020,d0: the thunk replaces the low byte of d0 only.
 * @return d0.b: the mode byte ($FF or 0, or what SetZoneMode stored)
 */
static ULONG BLITZ_ROUTINE zonemode_c(void)
{
    return zd.zones020;
}

/**
 * @brief SetZoneMode mode: sets the zone test mode, only on a 68020 or better.
 * @param mode  d0.b: the mode byte (non-zero: 68020 mode)
 */
static void BLITZ_ROUTINE setzonemode_c(ULONG mode BL_REG(d0))
{
    if (zd.is020)
        zd.zones020 = (UBYTE)mode;
}

/* GetZoneX1/X2/Y1/Y2(zone) (d0.w): field off of the zone (its byte offset
   in struct zone); the low word of d0 replaced (MOVE.w 0(a0,d0.w),d0) */
static ULONG zoneinfo(ULONG zonenum, UWORD off)
{
    const struct zonetable *zt = zd.zonetable;          /* 0: reads address 0 */

    if (!((LONG)zt->count > (LONG)off))                 /* BUG(orig): count vs offset (#870) */
        return (ULONG)-1;
    return (zonenum & 0xffff0000UL)
        | *(const UWORD *)((const UBYTE *)(zt + 1) + (WORD)((UWORD)(zonenum << 3) + off));
}

/* GetZoneX1(zone), GetZoneX2(zone), GetZoneY1(zone), GetZoneY2(zone):
   d0.w = the zone, d0 = the field (the low word replaced), -1 if the zone
   count is not above the field's offset (BUG(orig) #870: no range check
   of the zone, without a table address 0 is read) */
#define ZONEINFO(f, member)                                                  \
    static ULONG BLITZ_ROUTINE f(ULONG zonenum BL_REG(d0))                   \
    {                                                                        \
        return zoneinfo(zonenum, offsetof(struct zone, member));             \
    }
/** @brief GetZoneX1(zone): the start x of a zone (see ZONEINFO). */
ZONEINFO(zoneinfox1_c, x1)
/** @brief GetZoneX2(zone): the end x (circle: radius^2) of a zone. */
ZONEINFO(zoneinfox2_c, x2)
/** @brief GetZoneY1(zone): the start y of a zone. */
ZONEINFO(zoneinfoy1_c, y1)
/** @brief GetZoneY2(zone): the end y of a zone (circle: -1). */
ZONEINFO(zoneinfoy2_c, y2)

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

/**
 * @brief JFire(port): the fire buttons of a joystick port.
 *
 * Ports 1, 2: CIAA PRA /FIR1 resp. /FIR0, the second button through
 * POTGO/POTINP; ports 3, 4: the four player adaptor (CIAB PRA SEL/BUSY),
 * 0 if it is switched off (JAdaptorStatus).
 * @param port  d0.b: the port 1..4
 * @return d0: 1 = fire, + 2 = the second button (ports 1, 2); 0 for other ports
 */
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

/**
 * @brief JVert(port): the vertical direction of a joystick port.
 *
 * Ports 1, 2: JOY1DAT resp. JOY0DAT; ports 3, 4: the four player adaptor
 * in CIAA PRB (0 if switched off).
 * @param port  d0.b: the port 1..4
 * @return d0: -1 up, 1 down, 0 none (or another port)
 */
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

/**
 * @brief JHoriz(port): the horizontal direction of a joystick port.
 *
 * As JVert; port 1 "right" returns d0 unchanged (the port number 1, which
 * is also the result for "right": no defect).
 * @param port  d0.b: the port 1..4
 * @return d0: -1 left, 1 right, 0 none (or another port)
 */
static ULONG BLITZ_ROUTINE jhoriz_c(ULONG port BL_REG(d0))
{
    UWORD d;

    switch ((UBYTE)port) {
    case 1:
        d = JOY1DAT;
        if (d & 0x0002)
            return port;
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

/**
 * @brief AllFire(joybits): the fire buttons of the ports asked for.
 *
 * Bit n-1 of joybits asks for port n (1..4; 3 and 4 only with the four
 * player adaptor switched on).
 * @param bits  d0.b: the ports
 * @return d0: a bit per pressed fire button (bit n-1 = port n)
 */
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

/**
 * @brief AllFire: the fire buttons of all four ports (joybits %1111).
 * @return d0: a bit per pressed fire button
 */
static ULONG BLITZ_ROUTINE allfire_c(void)
{
    return allfire2_c(0x0f);
}

/**
 * @brief JAdaptorStatus On/Off: switches the reading of the four player adaptor.
 *
 * On by default ($FF); with 0, ports 3 and 4 read as nothing.
 * @param status  d0.b: the new status
 * @return d0: the old status in the low byte, d0's upper bytes kept
 */
static ULONG BLITZ_ROUTINE fouradaptor_c(ULONG status BL_REG(d0))
{
    UBYTE old = fourplayer;

    fourplayer = (UBYTE)status;
    return (status & 0xffffff00UL) | old;
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

/**
 * @brief ReadJoyPad(port#): the buttons of a CD32 pad.
 *
 * Clocks the pad's shift register through the fire line (see readpad).
 * BUG(orig) (#877): every port number but 2 reads the joystick port.
 * @param port  d0.w: the port (2 = the mouse port)
 * @return d0: bits 7..0 = B, R, Y, G, FF, RW, PL, 0 (as the help text)
 */
static ULONG BLITZ_ROUTINE readjoypad_c(UWORD port BL_REG(d0))
{
    if (port == 2)
        return readpad(0x40, 0xf600, 0x0400);           /* readmouseport */
    return readpad(0x80, 0x6f00, 0x4000);
}

/* Registers: the original changes d0 and, depending on the routine,
   d1-d7, a0, a1, a6; the thunks keep d1/a0/a1, the C code d2-d7/a2-a6
   (Zone and ZoneTest are assembler entries, above).
   ZoneMode replaces only the low byte of d0 (MOVE.b). */
BLITZ_THUNK(rizonejoylib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_Initmem, initmem_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit1, zoneinit1_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit2, zoneinit2_c, "d1/a0-a1");
BLITZ_THUNK(_ZoneInit, zoneinit_c, "d1/a0-a1");
BLITZ_THUNK(_SetZone2, setzone2_c, "d1/a0-a1");
BLITZ_THUNK(_SetZone, setzone_c, "d1/a0-a1");
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
