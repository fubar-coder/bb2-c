/*
 * ribbmaplib - RWE userlib 34 (Reflective Images): accessors over the
 * map ("FORM" .. "BMAP") and scenario ("FORM" .. "SCEN") files of Blitz
 * Bombers in memory.
 *
 * C translation of rwelibsrc/blitzbomberslib/bbmaplib2.bb (30/9/95);
 * reference: the reference build of the source (no binary exists,
 * docs/rwelibs/README.md 3.3).  Notes: docs/rwelibs/port/ribbmaplib.md.
 *
 *   SetBBMapLoc address / SetBBScenarioLoc address
 *       the file in memory: -1, 0 if it is no FORM of type BMAP / SCEN
 *   BBScenario... / BBNum... / BBTunnel... / BBShadowValue / BBDestructLoc
 *       a chunk of the scenario: its address (+0, +2, +8), a word, a
 *       byte or a long in it; 0 if the chunk is missing
 *   BBScenarioName / BBScenarioAuthor / BBMapName$   the strings of the
 *       ANNO / NAME chunk
 *   BBGetPalette palette#   the COLR chunk into a palette object
 *   BBMap...   the chunks of the map
 *
 * A chunk: id.l, length.l, data; the search starts 12 bytes into the
 * file and steps by 8 + length (no padding).  The address of the last
 * chunk found is kept per file (one id each), and returned without a
 * search when the same id is asked for again.
 *
 * init and finit are RTS.  No assembler besides the thunks.  Results in
 * d0 as the original leaves them: the word functions have $FFFF in the
 * upper word (MOVEQ #-1 of the chunk search; BBMapWidth/Height: the
 * caller's), 0 when the chunk is missing.
 *
 * BUG(orig), kept:
 *   - SetBBMapLoc stores the address before it checks it: after a failed
 *     call BBMapScenarioID/Width/Height/Info use the MFRM chunk of the
 *     map before (and the chunk searches the new address), and the
 *     cache (the id alone) returns a chunk of the old map for its id
 *     (#554).  A map without MFRM sets the info chunk to the address
 *     behind the last chunk id (#551).
 *   - The search ends only after an id that does not match (the id read
 *     can be behind the end) and compares the addresses signed (#553);
 *     an odd length makes the next id odd (an address error on a 68000)
 *     (#552).
 *   - BBGetPalette: an object with high colour >= its colour count gets
 *     a new block of high+1 colours, but its count is set to high (the
 *     last colour outside) (#548); the colours are written from low to
 *     high even if low > high (one colour at low, possibly behind the
 *     block) (#549).  BBScenarioLowCol/HighCol extend the byte with its
 *     sign (#550).
 * Original behaviour, not defects: the scenario routines before a
 * SetBBScenarioLoc and the map routines (but BBMapScenarioID,
 * BBMapWidth, BBMapHeight, BBMapInfo) before a SetBBMapLoc search from
 * address 12 (a program sets the location first); a failed
 * SetBBScenarioLoc keeps the old file and its cache; BBGetPalette frees
 * the old colour block with the size count*12 (memlib's globfree ignores
 * the size).
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _setbbmaploc(void), _setbbscenarioloc(void), _bbscenarioname(void);
void _bbscenarioauthor(void), _bbscenariodepth(void), _bbscenarioshapes(void);
void _bbscenariosounds(void), _bbscenariobaddies(void), _bbscenariopalette(void);
void _bbscenariolowcol(void), _bbscenariohighcol(void), _bbscenariolevel(void);
void _bbnumbackground(void), _bbnumdestruct(void), _bbnumindestruct(void);
void _bbscenarioprefix(void), _bbnumtunnels(void), _bbshadowvalue(void);
void _bbtunnelholes(void), _bbtunnelsloc(void), _bbgetpalette(void);
void _bbdestructloc(void), _bbmapscenarioid(void), _bbmapwidth(void);
void _bbmapheight(void), _bbmappowerups(void), _bbmapdata(void);
void _bbmapstartpos(void), _bbmapname(void), _bbmapinfo(void);
void _bbmappumask(void), _bbmapdboverlay(void), _bbmaponeplayerinfo(void);
void _bbmapobjects(void), _bbnumbaddies(void);
void codeinit(void), codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader{#ribbmaplib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ribbmaplib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_setbbmaploc, 0, 0);
BLITZ_NAME("SetBBMapLoc", "address");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_setbbscenarioloc, 0, 0);
BLITZ_NAME("SetBBScenarioLoc", "address");

BLITZ_NULLSUB(0, 0, 0);

/* a function without arguments: !afunction{t}:!args:!libs:!subs:!name */
#define FN0(t, code, name, help) \
    BLITZ_AFUNCTION(t); BLITZ_ARGS0(); BLITZ_LIBS0(); BLITZ_SUBS(code, 0, 0); \
    BLITZ_NAME(name, help)

FN0(BL_STRING, _bbscenarioname, "BBScenarioName", "");
FN0(BL_STRING, _bbscenarioauthor, "BBScenarioAuthor", "");

BLITZ_NULLSUB(0, 0, 0);

FN0(BL_WORD, _bbscenariodepth, "BBScenarioDepth", "");
FN0(BL_LONG, _bbscenarioshapes, "BBScenarioShapeLoc", "");
FN0(BL_LONG, _bbscenariosounds, "BBScenarioSounds", "");
FN0(BL_LONG, _bbscenariobaddies, "BBScenarioBaddiesLoc", "");
FN0(BL_LONG, _bbscenariopalette, "BBScenarioPalette", "");
FN0(BL_WORD, _bbscenariolowcol, "BBScenarioLowCol", "");
FN0(BL_WORD, _bbscenariohighcol, "BBScenarioHighCol", "");
FN0(BL_WORD, _bbscenariolevel, "BBScenarioLevel", "");
FN0(BL_WORD, _bbnumbackground, "BBNumBackGround", "");
FN0(BL_WORD, _bbnumdestruct, "BBNumDestruct", "");
FN0(BL_WORD, _bbnumindestruct, "BBNumIndestruct", "");
FN0(BL_LONG, _bbscenarioprefix, "BBScenarioPrefix", "");
FN0(BL_WORD, _bbnumtunnels, "BBNumTunnels", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_bbshadowvalue, 0, 0);
BLITZ_NAME("BBShadowValue", "shadowset");

FN0(BL_LONG, _bbtunnelholes, "BBTunnelHoles", "");
FN0(BL_LONG, _bbtunnelsloc, "BBTunnelsLoc", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0, 0));
BLITZ_SUBS(_bbgetpalette, 0, 0);
BLITZ_NAME("BBGetPalette", "palette#");

FN0(BL_LONG, _bbdestructloc, "BBDestructLoc", "");
FN0(BL_LONG, _bbmapscenarioid, "BBMapScenarioID", "- returns scenarios of map");
FN0(BL_WORD, _bbmapwidth, "BBMapWidth", "- returns map width");
FN0(BL_WORD, _bbmapheight, "BBMapHeight", "- returns map height");
FN0(BL_LONG, _bbmappowerups, "BBMapPowerUps", "- returns location of custom powerups");
FN0(BL_LONG, _bbmapdata, "BBMapData", "- returns location of actual map data");
FN0(BL_LONG, _bbmapstartpos, "BBMapStartPos", "- returns location of start position data");
FN0(BL_STRING, _bbmapname, "BBMapName$", "- returns the name of the map");
FN0(BL_LONG, _bbmapinfo, "BBMapInfo", "- returns map information chunk");
FN0(BL_LONG, _bbmappumask, "BBMapPUMask", "- returns map powerup mask (or zero for none)");
FN0(BL_LONG, _bbmapdboverlay, "BBMapDBOverLay",
    "- returns map destructible block overlay (or zero for none)");
FN0(BL_LONG, _bbmaponeplayerinfo, "BBMapOnePlayerInfo",
    "- returns one player data chunk (or zero for none)");
FN0(BL_LONG, _bbmapobjects, "BBMapObjects", "- returns one player object chunk (or zero for none)");
FN0(BL_WORD, _bbnumbaddies, "BBNumBaddies", "- returns number of baddies");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_LIBFIN();

/* init, finit: RTS */
void BLITZ_ROUTINE codeinit(void)
{
}

void BLITZ_ROUTINE codefinit(void)
{
}

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))
#define HI 0xffff0000UL                 /* MOVEQ #-1,d0 of the chunk search */

static UBYTE *maploc, *mapend, *mfrmchunk;
static UBYTE *scenarioloc, *scenarioend;
static ULONG lastchunk, lastmapchunk;
static UBYTE *lastchunkaddr, *lastmapchunkaddr;

/* _findchunk / _findmapchunk: the data of chunk `id`, or NULL; *at = a0
   at the end (also when not found: behind the last id read) */
static UBYTE *search(ULONG id, UBYTE *loc, UBYTE *end, ULONG *last, UBYTE **lastaddr,
                     UBYTE **at)
{
    UBYTE *a0;

    if (id == *last)
        return *at = *lastaddr;
    a0 = loc + 12;
    for (;;) {
        ULONG c = LONGAT(a0);
        a0 += 4;
        if (c == id)
            break;
        if ((LONG)a0 >= (LONG)end) {    /* CMP.l a1,a0: BGE */
            *at = a0;
            return NULL;
        }
        a0 += LONGAT(a0) + 4;
    }
    a0 += 4;
    *last = id;
    *lastaddr = a0;
    return *at = a0;
}

static UBYTE *findchunk(ULONG id)
{
    UBYTE *at;
    return search(id, scenarioloc, scenarioend, &lastchunk, &lastchunkaddr, &at);
}

static UBYTE *findmapchunk(ULONG id)
{
    UBYTE *at;
    return search(id, maploc, mapend, &lastmapchunk, &lastmapchunkaddr, &at);
}

/* ------------------------------------------------------------- the files */

static ULONG BLITZ_ROUTINE setbbscenarioloc_c(UBYTE *a BL_REG(d0))
{
    if (LONGAT(a) != ID('F', 'O', 'R', 'M') || LONGAT(a + 8) != ID('S', 'C', 'E', 'N'))
        return 0;
    scenarioloc = a;
    scenarioend = a + LONGAT(a + 4) + 8;
    lastchunk = 0;
    return (ULONG)-1;
}

static ULONG BLITZ_ROUTINE setbbmaploc_c(UBYTE *a BL_REG(d0))
{
    maploc = a;                         /* BUG(orig): before the check (#554) */
    if (LONGAT(a) != ID('F', 'O', 'R', 'M') || LONGAT(a + 8) != ID('B', 'M', 'A', 'P'))
        return 0;
    mapend = a + LONGAT(a + 4) + 8;
    lastmapchunk = 0;
    search(ID('M', 'F', 'R', 'M'), maploc, mapend, &lastmapchunk, &lastmapchunkaddr, &mfrmchunk);
    return (ULONG)-1;
}

/* --------------------------------------------------------- the scenario */

/* a word of a chunk: $FFFF in the upper word */
static ULONG chunkword(ULONG id, int off)
{
    UBYTE *a = findchunk(id);
    return a ? HI | WORDAT(a + off) : 0;
}

static ULONG chunkaddr(ULONG id, int off)
{
    UBYTE *a = findchunk(id);
    return a ? (ULONG)a + off : 0;
}

static ULONG BLITZ_ROUTINE bbscenariolevel_c(void)
{
    return chunkword(ID('L', 'E', 'V', 'L'), 0);
}

static ULONG BLITZ_ROUTINE bbscenariodepth_c(void)
{
    return chunkword(ID('D', 'E', 'P', 'F'), 0);
}

static ULONG BLITZ_ROUTINE bbscenarioprefix_c(void)
{
    UBYTE *a = findchunk(ID('P', 'R', 'F', 'X'));
    return a ? LONGAT(a) : 0;
}

static ULONG BLITZ_ROUTINE bbscenariopalette_c(void)
{
    return chunkaddr(ID('C', 'O', 'L', 'R'), 2);
}

/* the byte with its sign (EXT.w) */
static ULONG BLITZ_ROUTINE bbscenariolowcol_c(void)
{
    UBYTE *a = findchunk(ID('C', 'O', 'L', 'R'));
    return a ? HI | (UWORD)(WORD)(BYTE)a[0] : 0;
}

static ULONG BLITZ_ROUTINE bbscenariohighcol_c(void)
{
    UBYTE *a = findchunk(ID('C', 'O', 'L', 'R'));
    return a ? HI | (UWORD)(WORD)(BYTE)a[1] : 0;
}

static ULONG BLITZ_ROUTINE bbnumbackground_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 0);
}

static ULONG BLITZ_ROUTINE bbnumdestruct_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 2);
}

static ULONG BLITZ_ROUTINE bbnumindestruct_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 4);
}

static ULONG BLITZ_ROUTINE bbnumtunnels_c(void)
{
    return chunkword(ID('T', 'U', 'N', 'L'), 0);
}

static ULONG BLITZ_ROUTINE bbnumbaddies_c(void)
{
    UBYTE *a = findchunk(ID('B', 'A', 'D', '!'));
    return a ? a[0] : 0;
}

/* the long number `set` of SHDW (ASL.w #2: a word index) */
static ULONG BLITZ_ROUTINE bbshadowvalue_c(ULONG set BL_REG(d0))
{
    UBYTE *a = findchunk(ID('S', 'H', 'D', 'W'));
    return a ? LONGAT(a + (WORD)(set << 2)) : 0;
}

static ULONG BLITZ_ROUTINE bbtunnelsloc_c(void)
{
    return chunkaddr(ID('T', 'U', 'N', 'L'), 8);
}

static ULONG BLITZ_ROUTINE bbtunnelholes_c(void)
{
    UBYTE *a = findchunk(ID('T', 'U', 'N', 'L'));
    return a ? LONGAT(a + 2) : 0;
}

static ULONG BLITZ_ROUTINE bbscenarioshapes_c(void)
{
    return chunkaddr(ID('G', 'F', 'X', '!'), 0);
}

static ULONG BLITZ_ROUTINE bbscenariosounds_c(void)
{
    return chunkaddr(ID('S', 'F', 'X', '!'), 0);
}

static ULONG BLITZ_ROUTINE bbscenariobaddies_c(void)
{
    return chunkaddr(ID('B', 'A', 'D', '!'), 2);
}

static ULONG BLITZ_ROUTINE bbdestructloc_c(void)
{
    return chunkaddr(ID('D', 'E', 'S', 'T'), 0);
}

/* returnname: the string at a to a3 (without its 0, a3 behind the last
   character), d0 = its length; 0 if no chunk */
static ULONG copystring(const UBYTE *a)
{
    ULONG n = 0;
    if (!a)
        return 0;
    while (*a) {
        *blitz_string++ = *a++;
        n++;
    }
    return n;
}

static ULONG BLITZ_ROUTINE bbscenarioname_c(void)
{
    return copystring(findchunk(ID('A', 'N', 'N', 'O')));
}

/* returnauthor: the string behind the name's 0, at an even address */
static ULONG BLITZ_ROUTINE bbscenarioauthor_c(void)
{
    const UBYTE *a = findchunk(ID('A', 'N', 'N', 'O'));
    if (!a)
        return 0;
    while (*a++)
        ;
    if ((ULONG)a & 1)
        a++;
    return copystring(a);
}

/* BBGetPalette: the colours low..high of COLR (bytes r,g,b, each made a
   long $xxxxxxxx) into the palette object at its colour low; a new
   colour block if high >= the colour count (see BUG(orig) above, #548) */
static void BLITZ_ROUTINE bbgetpalette_c(UBYTE *pal BL_REG(a0))
{
    const UBYTE *a = findchunk(ID('C', 'O', 'L', 'R'));
    UWORD low, high, num;
    ULONG *d;

    if (!a)
        return;
    low = a[0];
    high = a[1];
    a += 2;
    num = WORDAT(pal + 4);
    if (!((WORD)high < (WORD)num)) {
        struct bl_regs r = { .d0 = (ULONG)(UWORD)(high + 1) * 12 + 4, .d1 = 0x10001 };
        UBYTE *new = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
        UBYTE *old = (UBYTE *)LONGAT(pal);
        if (num) {
            ULONG *s = (ULONG *)(old + 4), *t = (ULONG *)(new + 4);
            UWORD i = num - 1;
            do {
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
            } while (i--);
        }
        LONGAT(pal) = (ULONG)new;
        WORDAT(new) = high;
        WORDAT(pal + 4) = high;
        if (num) {
            struct bl_regs f = { .d0 = (ULONG)num * 12, .a1 = (ULONG)old };
            BLITZ_ALIBJSR(TOK_globfree, f);
        }
    }
    d = (ULONG *)(LONGAT(pal) + 4 + (WORD)(low * 12));
    do {
        int k;
        for (k = 0; k < 3; k++) {
            ULONG w = (ULONG)*a++ * 0x101;
            *d++ = w << 16 | w;
        }
        low++;
    } while ((WORD)high >= (WORD)low);
}

/* -------------------------------------------------------------- the map */

static ULONG mapchunkaddr(ULONG id)
{
    return (ULONG)findmapchunk(id);
}

static ULONG BLITZ_ROUTINE bbmapscenarioid_c(void)
{
    return maploc ? LONGAT(mfrmchunk) : 0;
}

/* the word functions on the info chunk: the upper word of d0 stays */
static ULONG BLITZ_ROUTINE bbmapwidth_c(ULONG d0 BL_REG(d0))
{
    return maploc ? (d0 & HI) | WORDAT(mfrmchunk + 4) : 0;
}

static ULONG BLITZ_ROUTINE bbmapheight_c(ULONG d0 BL_REG(d0))
{
    return maploc ? (d0 & HI) | WORDAT(mfrmchunk + 6) : 0;
}

static ULONG BLITZ_ROUTINE bbmapinfo_c(void)
{
    return maploc ? (ULONG)mfrmchunk : 0;
}

static ULONG BLITZ_ROUTINE bbmappowerups_c(void)
{
    return mapchunkaddr(ID('P', 'W', 'U', 'P'));
}

static ULONG BLITZ_ROUTINE bbmapstartpos_c(void)
{
    return mapchunkaddr(ID('S', 'T', 'R', 'T'));
}

static ULONG BLITZ_ROUTINE bbmapdata_c(void)
{
    return mapchunkaddr(ID('M', 'D', 'A', 'T'));
}

static ULONG BLITZ_ROUTINE bbmapname_c(void)
{
    return copystring(findmapchunk(ID('N', 'A', 'M', 'E')));
}

static ULONG BLITZ_ROUTINE bbmappumask_c(void)
{
    return mapchunkaddr(ID('P', 'W', 'M', 'K'));
}

static ULONG BLITZ_ROUTINE bbmapdboverlay_c(void)
{
    return mapchunkaddr(ID('D', 'B', 'O', 'L'));
}

static ULONG BLITZ_ROUTINE bbmaponeplayerinfo_c(void)
{
    return mapchunkaddr(ID('1', 'P', 'L', 'Y'));
}

static ULONG BLITZ_ROUTINE bbmapobjects_c(void)
{
    return mapchunkaddr(ID('O', 'B', 'J', 'S'));
}

/* Registers: the original changes d0, mostly d3/a0/a1 (the chunk
   search), the string routines d1 (and d2), BBShadowValue d7,
   BBGetPalette d0-d7/a0-a3; the thunks keep d1/a0/a1, the C code
   d2-d7/a2-a6. */
BLITZ_THUNK(_setbbmaploc, setbbmaploc_c, "d1/a0-a1");
BLITZ_THUNK(_setbbscenarioloc, setbbscenarioloc_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenarioname, bbscenarioname_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenarioauthor, bbscenarioauthor_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariodepth, bbscenariodepth_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenarioshapes, bbscenarioshapes_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariosounds, bbscenariosounds_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariobaddies, bbscenariobaddies_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariopalette, bbscenariopalette_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariolowcol, bbscenariolowcol_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariohighcol, bbscenariohighcol_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenariolevel, bbscenariolevel_c, "d1/a0-a1");
BLITZ_THUNK(_bbnumbackground, bbnumbackground_c, "d1/a0-a1");
BLITZ_THUNK(_bbnumdestruct, bbnumdestruct_c, "d1/a0-a1");
BLITZ_THUNK(_bbnumindestruct, bbnumindestruct_c, "d1/a0-a1");
BLITZ_THUNK(_bbscenarioprefix, bbscenarioprefix_c, "d1/a0-a1");
BLITZ_THUNK(_bbnumtunnels, bbnumtunnels_c, "d1/a0-a1");
BLITZ_THUNK(_bbshadowvalue, bbshadowvalue_c, "d1/a0-a1");
BLITZ_THUNK(_bbtunnelholes, bbtunnelholes_c, "d1/a0-a1");
BLITZ_THUNK(_bbtunnelsloc, bbtunnelsloc_c, "d1/a0-a1");
BLITZ_THUNK(_bbgetpalette, bbgetpalette_c, "d0-d1/a0-a1");
BLITZ_THUNK(_bbdestructloc, bbdestructloc_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapscenarioid, bbmapscenarioid_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapwidth, bbmapwidth_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapheight, bbmapheight_c, "d1/a0-a1");
BLITZ_THUNK(_bbmappowerups, bbmappowerups_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapdata, bbmapdata_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapstartpos, bbmapstartpos_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapname, bbmapname_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapinfo, bbmapinfo_c, "d1/a0-a1");
BLITZ_THUNK(_bbmappumask, bbmappumask_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapdboverlay, bbmapdboverlay_c, "d1/a0-a1");
BLITZ_THUNK(_bbmaponeplayerinfo, bbmaponeplayerinfo_c, "d1/a0-a1");
BLITZ_THUNK(_bbmapobjects, bbmapobjects_c, "d1/a0-a1");
BLITZ_THUNK(_bbnumbaddies, bbnumbaddies_c, "d1/a0-a1");
