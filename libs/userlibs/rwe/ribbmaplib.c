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

/**
 * @brief init: does nothing (RTS).
 */
void BLITZ_ROUTINE codeinit(void)
{
}

/**
 * @brief finit: does nothing (RTS).
 */
void BLITZ_ROUTINE codefinit(void)
{
}

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))
#define HI 0xffff0000UL                 /* MOVEQ #-1,d0 of the chunk search */

/* the header of an IFF file */
struct form {
    ULONG id;                           /* "FORM" */
    ULONG length;                       /* of what follows */
    ULONG type;                         /* "BMAP" / "SCEN" */
};

/* the data of a map's MFRM chunk (BBMapInfo) */
struct mfrm {
    ULONG scenarioid;                   /* +0 */
    UWORD width, height;                /* +4, +6 */
};

/* a palette object of palettelib, as far as BBGetPalette uses it */
struct bpal {
    ULONG pdata;                        /* +0 the colour block */
    UWORD numcols;                      /* +4 */
} BL_PACKED;

static UBYTE *maploc, *mapend;
static struct mfrm *mfrmchunk;
static UBYTE *scenarioloc, *scenarioend;
static ULONG lastchunk, lastmapchunk;
static UBYTE *lastchunkaddr, *lastmapchunkaddr;

/* _findchunk / _findmapchunk: the data of chunk `id`, or NULL; *at = a0
   at the end (also when not found: behind the last id read) */
static UBYTE *search(ULONG id, UBYTE *loc, UBYTE *end, ULONG *last, UBYTE **lastaddr,
                     UBYTE **at)
{
    UBYTE *cursor;

    if (id == *last)
        return *at = *lastaddr;
    cursor = loc + 12;
    for (;;) {
        ULONG c = LONGAT(cursor);
        cursor += 4;
        if (c == id)
            break;
        if ((LONG)cursor >= (LONG)end) { /* CMP.l a1,a0: BGE */
            *at = cursor;
            return NULL;
        }
        cursor += LONGAT(cursor) + 4;
    }
    cursor += 4;
    *last = id;
    *lastaddr = cursor;
    return *at = cursor;
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

/**
 * @brief SetBBScenarioLoc address: the scenario file ("FORM" .. "SCEN") in
 * memory for the BBScenario.../BBNum.../BBTunnel... functions.
 *
 * A failed call keeps the old file and its cache (original behaviour).
 * @param form  d0: the address of the file
 * @return d0: BB_TRUE, BB_FALSE if it is no FORM of type SCEN
 */
static BB_BOOL BLITZ_ROUTINE setbbscenarioloc_c(struct form *form BL_REG(d0))
{
    if (form->id != ID('F', 'O', 'R', 'M') || form->type != ID('S', 'C', 'E', 'N'))
        return BB_FALSE;
    scenarioloc = (UBYTE *)form;
    scenarioend = (UBYTE *)form + form->length + 8;
    lastchunk = 0;
    return BB_TRUE;
}

/**
 * @brief SetBBMapLoc address: the map file ("FORM" .. "BMAP") in memory for
 * the BBMap... functions; finds its MFRM chunk.
 *
 * BUG(orig) (#554): the address is stored before it is checked (after a
 * failed call the chunk searches use the new address, the MFRM chunk
 * and the cache the old map's).  BUG(orig) (#551): without MFRM the info
 * chunk is the address behind the last chunk id.
 * @param form  d0: the address of the file
 * @return d0: BB_TRUE, BB_FALSE if it is no FORM of type BMAP
 */
static BB_BOOL BLITZ_ROUTINE setbbmaploc_c(struct form *form BL_REG(d0))
{
    maploc = (UBYTE *)form;             /* BUG(orig): before the check (#554) */
    if (form->id != ID('F', 'O', 'R', 'M') || form->type != ID('B', 'M', 'A', 'P'))
        return BB_FALSE;
    mapend = (UBYTE *)form + form->length + 8;
    lastmapchunk = 0;
    search(ID('M', 'F', 'R', 'M'), maploc, mapend, &lastmapchunk, &lastmapchunkaddr,
           (UBYTE **)&mfrmchunk);
    return BB_TRUE;
}

/* --------------------------------------------------------- the scenario */

/* a word of a chunk: $FFFF in the upper word */
static ULONG chunkword(ULONG id, int off)
{
    UBYTE *chunk = findchunk(id);
    return chunk ? HI | WORDAT(chunk + off) : 0;
}

static ULONG chunkaddr(ULONG id, int off)
{
    UBYTE *chunk = findchunk(id);
    return chunk ? (ULONG)chunk + off : 0;
}

/**
 * @brief BBScenarioLevel: the word of the LEVL chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariolevel_c(void)
{
    return chunkword(ID('L', 'E', 'V', 'L'), 0);
}

/**
 * @brief BBScenarioDepth: the word of the DEPF chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariodepth_c(void)
{
    return chunkword(ID('D', 'E', 'P', 'F'), 0);
}

/**
 * @brief BBScenarioPrefix: the long of the PRFX chunk.
 * @return d0: the long, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenarioprefix_c(void)
{
    UBYTE *chunk = findchunk(ID('P', 'R', 'F', 'X'));
    return chunk ? LONGAT(chunk) : 0;
}

/**
 * @brief BBScenarioPalette: the colours of the COLR chunk (behind its low
 * and high byte).
 * @return d0: their address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariopalette_c(void)
{
    return chunkaddr(ID('C', 'O', 'L', 'R'), 2);
}

/**
 * @brief BBScenarioLowCol: the first colour of the COLR chunk (its byte
 * 0).
 *
 * BUG(orig) (#550): the byte is extended with its sign (EXT.w).
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariolowcol_c(void)
{
    UBYTE *chunk = findchunk(ID('C', 'O', 'L', 'R'));
    return chunk ? HI | (UWORD)(WORD)(BYTE)chunk[0] : 0;
}

/**
 * @brief BBScenarioHighCol: the last colour of the COLR chunk (its byte
 * 1).
 *
 * BUG(orig) (#550): the byte is extended with its sign (EXT.w).
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariohighcol_c(void)
{
    UBYTE *chunk = findchunk(ID('C', 'O', 'L', 'R'));
    return chunk ? HI | (UWORD)(WORD)(BYTE)chunk[1] : 0;
}

/**
 * @brief BBNumBackGround: the word at +0 of the SHAP chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbnumbackground_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 0);
}

/**
 * @brief BBNumDestruct: the word at +2 of the SHAP chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbnumdestruct_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 2);
}

/**
 * @brief BBNumIndestruct: the word at +4 of the SHAP chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbnumindestruct_c(void)
{
    return chunkword(ID('S', 'H', 'A', 'P'), 4);
}

/**
 * @brief BBNumTunnels: the word at +0 of the TUNL chunk.
 * @return d0: $FFFF:word, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbnumtunnels_c(void)
{
    return chunkword(ID('T', 'U', 'N', 'L'), 0);
}

/**
 * @brief BBNumBaddies: the byte at +0 of the BAD! chunk.
 * @return d0: the byte, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbnumbaddies_c(void)
{
    UBYTE *chunk = findchunk(ID('B', 'A', 'D', '!'));
    return chunk ? chunk[0] : 0;
}

/**
 * @brief BBShadowValue shadowset: the long number `set` of the SHDW chunk
 * (ASL.w #2: a word index).
 * @param set  d0: the shadow set
 * @return d0: the long, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbshadowvalue_c(ULONG set BL_REG(d0))
{
    UBYTE *chunk = findchunk(ID('S', 'H', 'D', 'W'));
    return chunk ? LONGAT(chunk + (WORD)(set << 2)) : 0;
}

/**
 * @brief BBTunnelsLoc: the data of the TUNL chunk from +8 on.
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbtunnelsloc_c(void)
{
    return chunkaddr(ID('T', 'U', 'N', 'L'), 8);
}

/**
 * @brief BBTunnelHoles: the long at +2 of the TUNL chunk.
 * @return d0: the long, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbtunnelholes_c(void)
{
    UBYTE *chunk = findchunk(ID('T', 'U', 'N', 'L'));
    return chunk ? LONGAT(chunk + 2) : 0;
}

/**
 * @brief BBScenarioShapeLoc: the data of the GFX! chunk.
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenarioshapes_c(void)
{
    return chunkaddr(ID('G', 'F', 'X', '!'), 0);
}

/**
 * @brief BBScenarioSounds: the data of the SFX! chunk.
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariosounds_c(void)
{
    return chunkaddr(ID('S', 'F', 'X', '!'), 0);
}

/**
 * @brief BBScenarioBaddiesLoc: the data of the BAD! chunk from +2 on.
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenariobaddies_c(void)
{
    return chunkaddr(ID('B', 'A', 'D', '!'), 2);
}

/**
 * @brief BBDestructLoc: the data of the DEST chunk.
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbdestructloc_c(void)
{
    return chunkaddr(ID('D', 'E', 'S', 'T'), 0);
}

/* returnname: the string at text to a3 (without its 0, a3 behind the last
   character), d0 = its length; 0 if no chunk */
static ULONG copystring(const UBYTE *text)
{
    ULONG n = 0;
    if (!text)
        return 0;
    while (*text) {
        *blitz_string++ = *text++;
        n++;
    }
    return n;
}

/**
 * @brief BBScenarioName: the name in the ANNO chunk, at a3 (without its
 * 0, a3 behind the last character).
 * @return d0: its length, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenarioname_c(void)
{
    return copystring(findchunk(ID('A', 'N', 'N', 'O')));
}

/**
 * @brief BBScenarioAuthor: the author in the ANNO chunk (the string behind
 * the name's 0, at an even address), at a3 like BBScenarioName.
 * @return d0: its length, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbscenarioauthor_c(void)
{
    const UBYTE *text = findchunk(ID('A', 'N', 'N', 'O'));
    if (!text)
        return 0;
    while (*text++)
        ;
    if ((ULONG)text & 1)
        text++;
    return copystring(text);
}

/**
 * @brief BBGetPalette palette#: the colours of the COLR chunk into a
 * palette object.
 *
 * The colours low..high (bytes r,g,b, each made a long $xxxxxxxx) at the
 * object's colour low; a new colour block (the old colours copied, the
 * old block freed with the size count*12) if high >= the colour count.
 * BUG(orig) (#548): the new block has high+1 colours, the count is set
 * to high; BUG(orig) (#549): the colours are written from low on even if
 * low > high (one colour).  Nothing without the chunk.
 * @param pal  a0: the palette object (palettelib)
 */
static void BLITZ_ROUTINE bbgetpalette_c(struct bpal *pal BL_REG(a0))
{
    const UBYTE *chunk = findchunk(ID('C', 'O', 'L', 'R'));
    UWORD low, high, num;
    ULONG *colour;

    if (!chunk)
        return;
    low = chunk[0];
    high = chunk[1];
    chunk += 2;
    num = pal->numcols;
    if (!((WORD)high < (WORD)num)) {
        struct bl_regs r = { .d0 = (ULONG)(UWORD)(high + 1) * 12 + 4, .d1 = 0x10001 };
        UBYTE *new = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
        UBYTE *old = (UBYTE *)pal->pdata;
        if (num) {
            ULONG *src = (ULONG *)(old + 4), *dst = (ULONG *)(new + 4);
            UWORD i = num - 1;
            do {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            } while (i--);
        }
        pal->pdata = (ULONG)new;
        WORDAT(new) = high;
        pal->numcols = high;
        if (num) {
            struct bl_regs f = { .d0 = (ULONG)num * 12, .a1 = (ULONG)old };
            BLITZ_ALIBJSR(TOK_globfree, f);
        }
    }
    colour = (ULONG *)(pal->pdata + 4 + (WORD)(low * 12));
    do {
        int k;
        for (k = 0; k < 3; k++) {
            ULONG wide = (ULONG)*chunk++ * 0x101;
            *colour++ = wide << 16 | wide;
        }
        low++;
    } while ((WORD)high >= (WORD)low);
}

/* -------------------------------------------------------------- the map */

static ULONG mapchunkaddr(ULONG id)
{
    return (ULONG)findmapchunk(id);
}

/**
 * @brief BBMapScenarioID: the scenario id of the map (+0 of its MFRM
 * chunk).
 * @return d0: the id, 0 before a SetBBMapLoc
 */
static ULONG BLITZ_ROUTINE bbmapscenarioid_c(void)
{
    return maploc ? mfrmchunk->scenarioid : 0;
}

/**
 * @brief BBMapWidth: the width of the map (+4 of its MFRM chunk).
 * @param prev  d0: d0 at the call (its upper word stays)
 * @return d0: the width in the low word, 0 before a SetBBMapLoc
 */
static ULONG BLITZ_ROUTINE bbmapwidth_c(ULONG prev BL_REG(d0))
{
    return maploc ? (prev & HI) | mfrmchunk->width : 0;
}

/**
 * @brief BBMapHeight: the height of the map (+6 of its MFRM chunk).
 * @param prev  d0: d0 at the call (its upper word stays)
 * @return d0: the height in the low word, 0 before a SetBBMapLoc
 */
static ULONG BLITZ_ROUTINE bbmapheight_c(ULONG prev BL_REG(d0))
{
    return maploc ? (prev & HI) | mfrmchunk->height : 0;
}

/**
 * @brief BBMapInfo: the map's information chunk (the data of MFRM).
 * @return d0: its address, 0 before a SetBBMapLoc
 */
static ULONG BLITZ_ROUTINE bbmapinfo_c(void)
{
    return maploc ? (ULONG)mfrmchunk : 0;
}

/**
 * @brief BBMapPowerUps: the map's custom power-ups (PWUP chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmappowerups_c(void)
{
    return mapchunkaddr(ID('P', 'W', 'U', 'P'));
}

/**
 * @brief BBMapStartPos: the map's start positions (STRT chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmapstartpos_c(void)
{
    return mapchunkaddr(ID('S', 'T', 'R', 'T'));
}

/**
 * @brief BBMapData: the map data (MDAT chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmapdata_c(void)
{
    return mapchunkaddr(ID('M', 'D', 'A', 'T'));
}

/**
 * @brief BBMapName$: the name of the map (NAME chunk), at a3 like
 * BBScenarioName.
 * @return d0: its length, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmapname_c(void)
{
    return copystring(findmapchunk(ID('N', 'A', 'M', 'E')));
}

/**
 * @brief BBMapPUMask: the map's power-up mask (PWMK chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmappumask_c(void)
{
    return mapchunkaddr(ID('P', 'W', 'M', 'K'));
}

/**
 * @brief BBMapDBOverLay: the map's destructible block overlay (DBOL
 * chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmapdboverlay_c(void)
{
    return mapchunkaddr(ID('D', 'B', 'O', 'L'));
}

/**
 * @brief BBMapOnePlayerInfo: the map's one player data (1PLY chunk).
 * @return d0: the address, 0 without the chunk
 */
static ULONG BLITZ_ROUTINE bbmaponeplayerinfo_c(void)
{
    return mapchunkaddr(ID('1', 'P', 'L', 'Y'));
}

/**
 * @brief BBMapObjects: the map's one player objects (OBJS chunk).
 * @return d0: the address, 0 without the chunk
 */
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
