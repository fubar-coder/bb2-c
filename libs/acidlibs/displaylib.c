/*
 * displaylib - command library 143: the AGA aware Blitz mode display
 * ("coplists", bum5..bum7) - InitCopList, CreateDisplay, DisplayBitMap,
 * DisplaySprite, DisplayPalette, DisplayControls, DisplayAdjust,
 * CustomColors, CustomString, DisplayDblScan, DisplayRainbow, DisplayRGB,
 * DisplayUser, DisplayScroll, CustomSprites, CustomChunky,
 * CustomChunkySize, DisplayGenlock.
 *
 * C translation of bblibsrc/displaylib (binary checked instruction by
 * instruction: same as the source).  Object library "CopList" (128
 * bytes, maximum 10, Free = _freecoplist), NEWTYPE .mycop:
 *   +0 size.l (0 = not initialised)  +4 coppos.l (chip memory)
 *   +8 colors  +12 sprites  +16 bpcons  +20 bplanes  +24 dot
 *   +28 customs  +32 dob (pointers into the list)
 *   +36 bot.w  +38 numbp  +40 colpokes  +42 fetchwid  +44 xand
 *   +46 xshift  +48 ypos  +50 height  +52 res  +54 numsprites
 *   +56 numcols  +58 numcustoms  +60 cpline  +62 x0.q  +66 x1.q
 *   +70 bplcon3  +72 aga ($8000 24 bit, fetch $00-$c0 in the low byte)
 *   +74 resshift  +76 setup  +78 cblow  +80 sfetch  +82 spres
 *   +84 spif  +86 spwid  +88 sspwid  +90 ham8  +92 csprites.l
 *   +96 cchunky.l  +100 chy chw chh chr chc pad0  +112 cchunky2.l
 *   +116 chy2 chw2 chh2 chr2 chc2  +126 spri
 * InitCopList builds the list: 2 NOPs (later the start wait), FMODE/DIW/
 * DDF/BPLCON0 from a table (resolution x fetch mode, 64 bytes each),
 * colour NOPs, sprite moves, BPLCON2/3/4/1 and BPL1/2MOD, bitplane
 * pointers, a wait + DMA on, the custom area (NOPs, or per line a wait
 * $e1 and cpline/4-1 NOPs for negative numcustoms), a wait + DMA off.
 * CreateDisplay (Blitz mode only) fills the waits, links several lists
 * with COP2LC/COPJMP2 and puts the first on COP1LC (switchlib $C205).
 * Everything else writes into the list (Amiga mode too).  The init
 * allocates a blank sprite (`mysprite`, chip) and reads LISAID ($DFF07C)
 * for agamac.
 *
 * Kept from the original (BUG(orig)):
 *   - every error is a branch from the normal code into the error check
 *     area (behind the library without the debugger): reproduced as
 *     JSR to the same labels. (#412)
 *   - the init writes mysprite's address into the data words of the
 *     SPR0POS/SPR0CTL moves of `spp` (+2/+6), not into SPR0PTH/PTL. (#413)
 *   - DisplayBitMap without x,y stores the caller's d2/d3 in x0/x1. (#411)
 *   - Not a bug (only with out-of-range arguments that do not occur):
 *     word arithmetic, DBF counters (a 0/negative count runs 65536 times,
 *     e.g. cpline 4 per line), ADDA.w of colour offsets (palette entries
 *     >= 2731 go backwards), signed CMPA.l in the overrun checks.
 *   - Not a bug (statements: the compiler reloads a3 for every string
 *     expression): DisplayBitMap / the double form leave a3 on the bp1
 *     table, CreateDisplay on the last coplist - reproduced.
 *   - CustomSprites is declared with the !libs of DisplaySprite
 *     (#spriteslib,$1181): the compiler treats CCOffset as a sprite
 *     number (maximum check, d1 shifted by 4) and the routine's ASL #2
 *     makes the offset CCOffset * 64 bytes instead of * 4. (#324)
 * DIFF: a2 stays unchanged where it is the argument stack (port
 * convention, notes/cmdlibs-port.md); the error branches are JSRs (a
 * return address more on the stack); indexes outside the data tables
 * (sprite mode > 5, mode table beyond resolution 2 / fetch 3, double
 * scan mode > 6) read other data than the original (which reads its own
 * code there).
 */
#include "blitzlib.h"

/* entry points (thunks / C routines, defined below) */
void _initcoplist2(void), _initcoplist(void), _createdisplay(void);
void _displaybmap(void), _displaybmap2(void), _displaydbmap(void), _displaydbmap2(void);
void _displaysprite(void), _displaypalette(void), _displaypalette2(void);
void _displaypalette3(void), _dobplcons(void), _doadjust(void);
void _customcols2(void), _customcols(void), _customstring(void);
void _displaydblscan0(void), _displaydblscan(void);
void _displayrainbow0(void), _displayrainbow(void);
void _displayrgb0(void), _displayrgb(void), _displayuser0(void), _displayuser(void);
void _displayscroll0(void), _displayscroll(void), _customsprites(void);
void _customchunky(void), _customchunky2(void), _customchunkysize(void);
void _setgenlock(void), _codeinit(void), _checkcoplist(void);
void BLITZ_ROUTINE _freecoplist(void);
void BLITZ_ROUTINE _codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#displaylib,init,0,finit,errs} */
BLITZ_LIBHEADER(LIB_displaylib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_DUMTOKE(_toke, "CopList", "");

#define COP 0x1380                      /* #displaylib,$1380: a3 = coplist */

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_displaylib, COP, LIB_spriteslib, BL_LA(0), LIB_displaylib, 0x0604);
BLITZ_SUBS(_initcoplist2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_spriteslib, BL_LA(0), LIB_displaylib, 0x0604);
BLITZ_SUBS(_initcoplist, 0, 0);
BLITZ_NAME("InitCopList", "CopList#[,type]|[,ypos,height,type,sprites,colors,numcustoms]");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(0, 1, BL_PUSH | BL_WORD);
BLITZ_LIBS(LIB_displaylib, 0x1001);
BLITZ_SUBS(_createdisplay, (void *)1, 0);
BLITZ_NAME("CreateDisplay", "CopList#[,CopList#..]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_bitmaplib, 0x1081);
BLITZ_SUBS(_displaybmap, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_bitmaplib, 0x1081);
BLITZ_SUBS(_displaybmap2, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_bitmaplib, 0x1081, LIB_bitmaplib, 0x1182);
BLITZ_SUBS(_displaydbmap, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK, BL_WORD, BL_WORD, BL_QUICK, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_bitmaplib, 0x1081, LIB_bitmaplib, 0x1184);
BLITZ_SUBS(_displaydbmap2, _checkcoplist, 0);
BLITZ_NAME("DisplayBitMap", "CopList#,bmap[,x,y] [,bmap[,x,y]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_spriteslib, 0x1181, LIB_displaylib, COP);
BLITZ_SUBS(_displaysprite, _checkcoplist, 0);
BLITZ_NAME("DisplaySprite", "CopList#,Sprite#,X,Y,Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1081);
BLITZ_SUBS(_displaypalette, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1081);
BLITZ_SUBS(_displaypalette2, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1081);
BLITZ_SUBS(_displaypalette3, _checkcoplist, 0);
BLITZ_NAME("DisplayPalette", "CopList#,Palette# [,coloroffset] [,coloroffset,bankoffset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_dobplcons, _checkcoplist, 0);
BLITZ_NAME("DisplayControls", "CopList#,BPLCON2,BPLCON3,BPLCON4");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_doadjust, _checkcoplist, 0);
BLITZ_NAME("DisplayAdjust", "CopList#,fetchwid,ddfstrt,ddfstop,diwstrt,diwstop");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1083);
BLITZ_SUBS(_customcols2, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1083);
BLITZ_SUBS(_customcols, _checkcoplist, 0);
BLITZ_NAME("CustomColors", "CopList#,CCOffset,YPos,Palette,startcol,numcols[,thru256flag]  ;ecs=2+n aga=2+n+n+n/16");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_customstring, _checkcoplist, 0);
BLITZ_NAME("CustomString", "CopList#,CCOffset,YPos,Copper$  ;2+n {n=len(a$)/4}");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displaydblscan0, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displaydblscan, _checkcoplist, 0);
BLITZ_NAME("DisplayDblScan", "CopList#,Mode[,copoffset]   ;2");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1282);
BLITZ_SUBS(_displayrainbow0, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP, LIB_palettelib, 0x1282);
BLITZ_SUBS(_displayrainbow, _checkcoplist, 0);
BLITZ_NAME("DisplayRainbow", "CopList#,Register,Palette[,copoffset] ;ecs=1 aga=4");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayrgb0, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayrgb, _checkcoplist, 0);
BLITZ_NAME("DisplayRGB", "CopList#,Register,line,r,g,b[,copoffset] ;ecs=1 aga=4");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayuser0, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayuser, _checkcoplist, 0);
BLITZ_NAME("DisplayUser", "CopList#,Line,String[,Offset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayscroll0, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_displayscroll, _checkcoplist, 0);
BLITZ_NAME("DisplayScroll", "CopList#,&xpos.q(n),&xpos.q(n)[,Offset]   ;3");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
/* BUG(orig) (#324): copied from DisplaySprite - $1181 loads a sprite object for
   CCOffset (d1 arrives shifted by 4, checked against the sprite maximum) */
BLITZ_LIBS(LIB_spriteslib, 0x1181, LIB_displaylib, COP);
BLITZ_SUBS(_customsprites, _checkcoplist, 0);
BLITZ_NAME("CustomSprites", "CopList#,CCOffset,YPos,NumSprites ;4n+2");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_customchunky, _checkcoplist, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, COP);
BLITZ_SUBS(_customchunky2, _checkcoplist, 0);
BLITZ_NAME("CustomChunky", "CopList#,CCOffset,YPos,width,height,res[,splitpos] ;4n+2");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_customchunkysize, 0, 0);
BLITZ_NAME("CustomChunkySize", "width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_setgenlock, 0, 0);
BLITZ_NAME("DisplayGenlock", "BPLCON0     ;2=use external sync 4=interlace");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _freecoplist, 0, 0);
BLITZ_NULLSUB_AT(init, _codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, _codefinit, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 7);

/* --------------------------------------------- constants and helpers */

#define W(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o) (*(UBYTE *)((UBYTE *)(p) + (o)))

/* .mycop offsets */
#define M_SIZE 0
#define M_COPPOS 4
#define M_COLORS 8
#define M_SPRITES 12
#define M_BPCONS 16
#define M_BPLANES 20
#define M_DOT 24
#define M_CUSTOMS 28
#define M_DOB 32
#define M_BOT 36
#define M_NUMBP 38
#define M_COLPOKES 40
#define M_FETCHWID 42
#define M_YPOS 48
#define M_HEIGHT 50
#define M_RES 52
#define M_NUMSPRITES 54
#define M_NUMCOLS 56
#define M_NUMCUSTOMS 58
#define M_CPLINE 60
#define M_X0 62
#define M_X1 66
#define M_BPLCON3 70
#define M_AGA 72
#define M_RESSHIFT 74
#define M_SETUP 76
#define M_SFETCH 80
#define M_SPRES 82
#define M_SPIF 84
#define M_SPWID 86
#define M_SSPWID 88
#define M_HAM8 90
#define M_CSPRITES 92
#define M_CCHUNKY 96
#define M_CHY 100
#define M_CHW 102
#define M_CHH 104
#define M_CHC 108
#define M_CCHUNKY2 112
#define M_CHH2 120
#define M_SPRI 126

#define DMAOFF  0x00960120
#define DMAON   0x00968120
#define WAITBH  0xffe1fffe
#define NOWCOP  0x01fe0000
#define COPNOP  0x01fe0000
#define WAITEND 0xfffffffe
#define STROBE1 0x008a0001

#define TOK_setcoplist 0xc205

/* register shifts/rotates with the 68000's count (mod 64) and sizes */
#define SHIFTOP(name, insn)                                                 \
    static inline ULONG name(ULONG x, ULONG n)                              \
    {                                                                       \
        __asm__(insn " %1,%0" : "+d"(x) : "d"(n) : "cc");                   \
        return x;                                                           \
    }
SHIFTOP(asl_l, "asl.l")
SHIFTOP(asr_l, "asr.l")
SHIFTOP(rol_l, "rol.l")
SHIFTOP(ror_l, "ror.l")
SHIFTOP(asl_w, "asl.w")
SHIFTOP(asr_w, "asr.w")
SHIFTOP(lsr_w, "lsr.w")

/* MULU.w as instruction (GCC would call __mulsi3) */
static inline ULONG mulu16(UWORD a, UWORD b)
{
    ULONG r = a;

    __asm__("mulu.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
}

/* keeps GCC from replacing a strided pointer walk by a multiplication
   (__mulsi3, no libgcc in libraries) */
#define OPAQUE(x) __asm__("" : "+r"(x))

/* a branch into the error check area (never returns): a call of the
   noreturn label - JSR instead of the original's Bcc, only the stack
   differs (an inline BRA followed by __builtin_unreachable lets GCC
   drop the path) */
#define GOTO(label) label()
#define NORET __attribute__((noreturn))
void maxbperr(void) NORET, uniniterr(void) NORET, nopalerr(void) NORET;
void nosprierr(void) NORET, freakerr(void) NORET, linerr(void) NORET;
void agapalerr(void) NORET, toolowerr(void) NORET, overrunerr(void) NORET;
void spriterr(void) NORET;

/* CMP.l !m{dob},a1:BGT overrunerr (signed) */
#define OVERRUN(a3, a1) do {                                                \
        if ((LONG)(ULONG)(a1) > (LONG)L(a3, M_DOB))                         \
            GOTO(overrunerr);                                               \
    } while (0)

/* data in the original's order (out of range indexes read neighbours) */
static const UWORD colup[16] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 256, 256, 256,
                                 256, 256, 256, 256 };
static struct BL_PACKED {
    UWORD genlock;
    UWORD slup[3], shup[3], swup[3];
    UWORD agaflag;
    ULONG mysprite;
    UWORD agamac;
} d = { 0, { 0, 4, 12 }, { 2, 1, 0 }, { 8, 16, 32 }, 0, 0, 0 };

static const UWORD bpb[16] = { 0, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000,
                               0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
static const ULONG bcp[6] = { 0x01040224, 0x01060c00, 0x010c0011, 0x01020000,
                              0x01080000, 0x010a0000 };
static ULONG spp[4] = { 0x01400000, 0x01420000, 0x01200000, 0x01220000 };

/* table: maxbp, fetchwid, xmask, xrshift, adjwid, adjstop, ctrl (6 longs),
   14 bytes pad = 64 bytes per entry */
#define T(a, b, c, e, f, g, fm, ds, de, dfs, dfe, b0) \
    { a, b, c, e, f, g, 0x1fc, fm, 0x8e, ds, 0x90, de, 0x92, dfs, 0x94, dfe, 0x100, b0, \
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
static const UWORD table[12][32] = {
    T(8, 40, 0x3f, 5, 2, 8, 0, 0x81, 0x70c1, 0x38, 0xd0, 0x201),
    T(8, 40, 0x7f, 5, 4, 16, 1, 0x81, 0x70c1, 0x38, 0xc0, 0x201),
    T(8, 40, 0x7f, 5, 4, 16, 2, 0x81, 0x70c1, 0x38, 0xc0, 0x201),
    T(8, 40, 0xff, 5, 8, 32, 3, 0x81, 0x70c1, 0x38, 0xb8, 0x201),
    T(8, 80, 0x1f, 4, 4, 4, 0, 0x81, 0x70c1, 0x3c, 0xd4, 0x8201),
    T(8, 80, 0x3f, 4, 4, 8, 1, 0x81, 0x70c1, 0x38, 0xd0, 0x8201),
    T(8, 80, 0x3f, 4, 4, 8, 2, 0x81, 0x70c1, 0x38, 0xd0, 0x8201),
    T(8, 80, 0x7f, 4, 8, 16, 3, 0x81, 0x70c1, 0x38, 0xc0, 0x8201),
    T(8, 160, 0x0f, 3, 0, 0, 0, 0x81, 0x70c1, 0x3e, 0xd6, 0x241),
    T(8, 160, 0x1f, 3, 4, 4, 1, 0x81, 0x70c1, 0x38, 0xd0, 0x241),
    T(8, 160, 0x1f, 3, 4, 4, 2, 0x81, 0x70c1, 0x38, 0xd0, 0x241),
    T(8, 160, 0x3f, 3, 8, 8, 3, 0x81, 0x70c1, 0x38, 0xd0, 0x241),
};
#undef T

/* bp1: BPLCON1 scroll values (index = fine x position) */
static const UWORD bp1[256] = {
    0xFFFF, 0xEEFF, 0xDDFF, 0xCCFF, 0xFFEE, 0xEEEE, 0xDDEE, 0xCCEE,
    0xFFDD, 0xEEDD, 0xDDDD, 0xCCDD, 0xFFCC, 0xEECC, 0xDDCC, 0xCCCC,
    0xFFBB, 0xEEBB, 0xDDBB, 0xCCBB, 0xFFAA, 0xEEAA, 0xDDAA, 0xCCAA,
    0xFF99, 0xEE99, 0xDD99, 0xCC99, 0xFF88, 0xEE88, 0xDD88, 0xCC88,
    0xFF77, 0xEE77, 0xDD77, 0xCC77, 0xFF66, 0xEE66, 0xDD66, 0xCC66,
    0xFF55, 0xEE55, 0xDD55, 0xCC55, 0xFF44, 0xEE44, 0xDD44, 0xCC44,
    0xFF33, 0xEE33, 0xDD33, 0xCC33, 0xFF22, 0xEE22, 0xDD22, 0xCC22,
    0xFF11, 0xEE11, 0xDD11, 0xCC11, 0xFF00, 0xEE00, 0xDD00, 0xCC00,
    0xBBFF, 0xAAFF, 0x99FF, 0x88FF, 0xBBEE, 0xAAEE, 0x99EE, 0x88EE,
    0xBBDD, 0xAADD, 0x99DD, 0x88DD, 0xBBCC, 0xAACC, 0x99CC, 0x88CC,
    0xBBBB, 0xAABB, 0x99BB, 0x88BB, 0xBBAA, 0xAAAA, 0x99AA, 0x88AA,
    0xBB99, 0xAA99, 0x9999, 0x8899, 0xBB88, 0xAA88, 0x9988, 0x8888,
    0xBB77, 0xAA77, 0x9977, 0x8877, 0xBB66, 0xAA66, 0x9966, 0x8866,
    0xBB55, 0xAA55, 0x9955, 0x8855, 0xBB44, 0xAA44, 0x9944, 0x8844,
    0xBB33, 0xAA33, 0x9933, 0x8833, 0xBB22, 0xAA22, 0x9922, 0x8822,
    0xBB11, 0xAA11, 0x9911, 0x8811, 0xBB00, 0xAA00, 0x9900, 0x8800,
    0x77FF, 0x66FF, 0x55FF, 0x44FF, 0x77EE, 0x66EE, 0x55EE, 0x44EE,
    0x77DD, 0x66DD, 0x55DD, 0x44DD, 0x77CC, 0x66CC, 0x55CC, 0x44CC,
    0x77BB, 0x66BB, 0x55BB, 0x44BB, 0x77AA, 0x66AA, 0x55AA, 0x44AA,
    0x7799, 0x6699, 0x5599, 0x4499, 0x7788, 0x6688, 0x5588, 0x4488,
    0x7777, 0x6677, 0x5577, 0x4477, 0x7766, 0x6666, 0x5566, 0x4466,
    0x7755, 0x6655, 0x5555, 0x4455, 0x7744, 0x6644, 0x5544, 0x4444,
    0x7733, 0x6633, 0x5533, 0x4433, 0x7722, 0x6622, 0x5522, 0x4422,
    0x7711, 0x6611, 0x5511, 0x4411, 0x7700, 0x6600, 0x5500, 0x4400,
    0x33FF, 0x22FF, 0x11FF, 0x00FF, 0x33EE, 0x22EE, 0x11EE, 0x00EE,
    0x33DD, 0x22DD, 0x11DD, 0x00DD, 0x33CC, 0x22CC, 0x11CC, 0x00CC,
    0x33BB, 0x22BB, 0x11BB, 0x00BB, 0x33AA, 0x22AA, 0x11AA, 0x00AA,
    0x3399, 0x2299, 0x1199, 0x0099, 0x3388, 0x2288, 0x1188, 0x0088,
    0x3377, 0x2277, 0x1177, 0x0077, 0x3366, 0x2266, 0x1166, 0x0066,
    0x3355, 0x2255, 0x1155, 0x0055, 0x3344, 0x2244, 0x1144, 0x0044,
    0x3333, 0x2233, 0x1133, 0x0033, 0x3322, 0x2222, 0x1122, 0x0022,
    0x3311, 0x2211, 0x1111, 0x0011, 0x3300, 0x2200, 0x1100, 0x0000,
};
#define BP1(i) (*(const UWORD *)((const UBYTE *)bp1 + (WORD)(i)))   /* 0(a2,dN.w) */

static struct BL_PACKED { UWORD lastwait; ULONG topcop; } cd;
static struct BL_PACKED { UWORD dloop[5], loopd; } dl = { { 0, 1, 5, 13, 29 }, 0 };
static struct BL_PACKED { UWORD mods[2], ycont; } sc;
static const UWORD shifs[8] = { 1, 4, 2, 5, 2, 5, 3, 6 };
static const UBYTE flups[4] = { 0, 1, 1, 2 };
static const UWORD blah[1] = { 0 };     /* DisplayRGB's offset 0 */

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

/* a wait at line d1 (bit 8: the line 256 wait unless `prev` has bit 8) */
static ULONG *putwait(ULONG *a1, UWORD d1, UWORD prev, UBYTE h)
{
    UBYTE *b;

    *a1++ = ((d1 & 0x100) && !(prev & 0x100)) ? WAITBH : NOWCOP;
    b = (UBYTE *)a1;
    b[0] = (UBYTE)d1;
    b[1] = h;
    ((UWORD *)a1)[1] = 0xfffe;
    return a1 + 1;
}

/* ------------------------------------------------- main initialisation */

static void BLITZ_ROUTINE _codeinit_c(void)
{
    ULONG m = globalloc(32, 65538);

    d.mysprite = m;
    ((UWORD *)spp)[3] = (UWORD)m;               /* BUG(orig) (#413): spp+6, spp+2 */
    ((UWORD *)spp)[1] = (UWORD)(m >> 16);
    if ((*(volatile UWORD *)0xdff07c & 255) == 248)
        d.agamac = 0xffff;
    else
        d.agamac = 0;
}
BLITZ_THUNK(_codeinit, _codeinit_c, "a0-a1");

void BLITZ_ROUTINE _codefinit(void)
{
}

/* DisplayAdjust */
static void BLITZ_ROUTINE _doadjust_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                      UWORD d3 BL_REG(d3), UWORD d4 BL_REG(d4),
                                      UWORD d5 BL_REG(d5))
{
    UBYTE *a3 = blitz_string;
    UBYTE *a1 = (UBYTE *)L(a3, M_COPPOS) + 8;

    W(a3, M_FETCHWID) += d1;
    W(a1, 14) += d2;
    W(a1, 18) += d3;
    W(a1, 6) += d4;
    W(a1, 10) += d5;
}
BLITZ_THUNK(_doadjust, _doadjust_c, "d0-d1/a0");

/* DisplayControls */
static void BLITZ_ROUTINE _dobplcons_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                       UWORD d3 BL_REG(d3))
{
    UBYTE *a3 = blitz_string;
    ULONG *a0 = (ULONG *)L(a3, M_BPCONS);
    ULONG d4 = a0[0], d5 = a0[1], d6 = a0[2];

    BL_SETW(d4, (UWORD)d4 ^ d1);
    BL_SETW(d5, (UWORD)d5 ^ d2);
    BL_SETW(d6, (UWORD)d6 ^ d3);
    W(a3, M_BPLCON3) ^= d2;
    a0[0] = d4;
    a0[1] = d5;
    a0[2] = d6;
}
BLITZ_THUNK(_dobplcons, _dobplcons_c, "d0-d1/a1");

/* DisplayGenlock */
static void BLITZ_ROUTINE _setgenlock_c(UWORD v BL_REG(d0))
{
    d.genlock = v;
}
BLITZ_THUNK(_setgenlock, _setgenlock_c, "d0-d1/a0-a1");

/* _freecoplist (also Free): a3 = coplist */
void BLITZ_ROUTINE _freecoplist(void)
{
    UBYTE *a3 = blitz_string;
    struct bl_regs r;

    if (!L(a3, M_SIZE))
        return;
    r.d0 = L(a3, M_SIZE);
    r.d1 = 0;
    r.a0 = 0;
    r.a1 = L(a3, M_COPPOS);
    BLITZ_ALIBJSR(TOK_globfree, r);
    L(a3, M_SIZE) = 0;
    W(a3, M_CPLINE) = 0;
}

/* _initcoplist: d1 = ypos, d2 = height, d3 = type, d4 = sprites, d5 =
   colours, -2(a2) = numcustoms, a0 = spriteslib's value, d6 = used
   offset, a3 = coplist */
static void initcoplist(ULONG d1, ULONG d2, ULONG d3, ULONG d4, ULONG d5,
                        const UWORD *a2, const UWORD *a0, WORD used)
{
    UBYTE *a3 = blitz_string;
    ULONG d0, d6, d7, *a1;
    const UWORD *t;
    UWORD i;

    *(UBYTE **)(blitz_globals + used) = a3;     /* make used */
    d0 = *a0;
    W(a3, M_SPIF) = (UWORD)d0;
    W(a3, M_SPIF) += 2;
    d0 = (UWORD)(d0 + d0);
    W(a3, M_SFETCH) = *(const UWORD *)((const UBYTE *)d.slup + (WORD)d0);
    W(a3, M_SPRES) = *(const UWORD *)((const UBYTE *)d.shup + (WORD)d0);
    W(a3, M_SPWID) = *(const UWORD *)((const UBYTE *)d.swup + (WORD)d0);
    W(a3, M_HAM8) = 0;
    if ((UBYTE)(d3 & 0x8f) == 0x88)
        W(a3, M_HAM8) = 0xffff;
    W(a3, M_YPOS) = (UWORD)d1;                  /* MOVEM d1-d5 */
    W(a3, M_HEIGHT) = (UWORD)d2;
    W(a3, M_RES) = (UWORD)d3;
    W(a3, M_NUMSPRITES) = (UWORD)d4;
    W(a3, M_NUMCOLS) = (UWORD)d5;
    W(a3, M_BOT) = (UWORD)d1;
    _freecoplist();
    /* calccustoms */
    d6 = *--a2;
    W(a3, M_NUMCUSTOMS) = (UWORD)d6;
    if ((WORD)d6 < 0) {
        d6 = (UWORD)(-(WORD)d6 + 1);
        d6 = (UWORD)(d6 << 2);
        W(a3, M_CPLINE) = (UWORD)d6;
        d6 >>= 2;
        BL_SETW(d2, (UWORD)d2 - 1);
        d6 = mulu16((UWORD)d2, (UWORD)d6);
        W(a3, M_NUMCUSTOMS) = (UWORD)d6;
    }
    /* checkaga */
    d0 = d3;
    d0 = lsr_w(d0, 5);
    d0 >>= 1;
    BL_SETW(d0, (UWORD)d0 & 0x80c0);
    W(a3, M_AGA) = (UWORD)d0;
    if ((WORD)d0 < 0 && (UWORD)d5) {
        d7 = (UWORD)((UWORD)((UWORD)d5 - 1) >> 5) + 1;
        d7 = (UWORD)(d7 + d7);
        BL_SETW(d5, (UWORD)d5 + (UWORD)d5 + (UWORD)d7);
    }
    /* calcsize */
    W(a3, M_COLPOKES) = (UWORD)d5;
    BL_SETW(d3, (UWORD)d3 & 15);
    W(a3, M_NUMBP) = (UWORD)d3;
    BL_SETW(d3, (UWORD)d3 + (UWORD)d3);
    d0 = (UWORD)d3;
    BL_SETW(d4, (UWORD)d4 << 2);
    d0 = (UWORD)(d0 + (UWORD)d4 + (UWORD)d5 + (UWORD)d6 + 44);
    d0 <<= 2;
    L(a3, M_SIZE) = d0;
    d0 = globalloc(d0, 65538);
    L(a3, M_COPPOS) = d0;
    a1 = (ULONG *)d0;
    d0 = W(a3, M_RES);
    /* firstwait */
    d7 = COPNOP;
    *a1++ = d7;
    *a1++ = d7;
    /* ctrl1 */
    {
        UWORD d1w = ((UWORD)d0 & 0x300) | B(a3, M_AGA + 1);
        ULONG c[6];
        int k;

        t = (const UWORD *)((const UBYTE *)table + (WORD)d1w);
        d1w >>= 8;
        W(a3, M_RESSHIFT) = (UWORD)(2 - d1w);
        for (k = 0; k < 6; k++)
            c[k] = ((const ULONG *)(t + 6))[k];
        BL_SETW(c[0], (UWORD)c[0] | W(a3, M_SFETCH));
        BL_SETW(c[5], (UWORD)c[5] | d.genlock);
        for (k = 0; k < 6; k++)
            a1[k] = c[k];
        W(a3, M_FETCHWID) = t[1];
        W(a3, M_FETCHWID + 2) = t[2];
        W(a3, M_FETCHWID + 4) = t[3];
        if (d0 & (1 << 4)) {
            W(a3, M_FETCHWID) += t[4];
            W((UBYTE *)a1, 14) -= t[5];
        }
        if ((WORD)W(a3, M_NUMBP) > (WORD)t[0])
            GOTO(maxbperr);
        i = bpb[W(a3, M_NUMBP)];
        if (d0 & (1 << 7))
            i |= 1 << 11;
        if (d0 & (1 << 5))
            i |= 1 << 10;
        W((UBYTE *)a1, 22) |= i;
    }
    a1 += 6;
    d7 = COPNOP;
    /* cols */
    L(a3, M_COLORS) = (ULONG)a1;
    i = W(a3, M_COLPOKES);
    if (i) {
        i--;
        do
            *a1++ = d7;
        while ((WORD)--i != -1);
    }
    /* sprites */
    L(a3, M_SPRITES) = (ULONG)a1;
    i = W(a3, M_NUMSPRITES);
    if (i) {
        ULONG s3 = spp[0], s4 = spp[1], s5 = spp[2], s6 = spp[3];

        i--;
        do {
            a1[0] = s3;
            a1[1] = s4;
            a1[2] = s5;
            a1[3] = s6;
            a1 += 4;
            s3 += 0x80000;
            s4 += 0x80000;
            s5 += 0x40000;
            s6 += 0x40000;
        } while ((WORD)--i != -1);
    }
    /* ctrl2 */
    L(a3, M_BPCONS) = (ULONG)a1;
    {
        ULONG c1 = bcp[0], c2 = bcp[1];
        UWORD x;

        if (d0 & (1 << 6))
            c1 &= ~(1UL << 9);
        x = ((UWORD)d0 & 0xc00) >> 4;
        BL_SETW(c2, (UWORD)c2 | x);
        x >>= 6;
        if (x)
            x -= 1;
        W(a3, M_SPRI) = x;
        a1[0] = c1;
        a1[1] = c2;
        a1[2] = bcp[2];
        a1[3] = bcp[3];
        a1[4] = bcp[4];
        a1[5] = bcp[5];
        a1 += 6;
        W(a3, M_BPLCON3) = (UWORD)c2;
        W(a3, M_SSPWID) = (UWORD)lsr_w(W(a3, M_SPWID), x);
    }
    /* bps */
    L(a3, M_BPLANES) = (ULONG)a1;
    d0 = 0x00e00000;
    i = W(a3, M_NUMBP);
    if (i) {
        i = i + i - 1;
        do {
            *a1++ = d0;
            d0 += 0x20000;
        } while ((WORD)--i != -1);
    }
    /* dot */
    d0 = (ULONG)a1 - L(a3, M_COPPOS);
    d0 = bl_divu_w(d0, 222);
    W(a3, M_SETUP) = (UWORD)((UWORD)d0 + 3);
    L(a3, M_DOT) = (ULONG)a1;
    *a1++ = d7;
    *a1++ = d7;
    *a1++ = DMAON;
    /* customs */
    L(a3, M_CUSTOMS) = (ULONG)a1;
    i = W(a3, M_CPLINE);
    if (!i) {
        UWORD n = W(a3, M_NUMCUSTOMS);

        if (n) {
            n--;
            do
                *a1++ = d7;
            while ((WORD)--n != -1);
        }
    } else {
        /* lino */
        UWORD per = (i >> 2) - 2, y = W(a3, M_YPOS) - 1, n = W(a3, M_HEIGHT) - 2;

        do {
            UBYTE *b = (UBYTE *)a1;
            UWORD k = per;

            b[0] = (UBYTE)y;
            b[1] = 0xe1;
            ((UWORD *)a1)[1] = 0xfffe;
            a1++;
            do
                *a1++ = d7;
            while ((WORD)--k != -1);
            y++;
        } while ((WORD)--n != -1);
        W(a3, M_BOT) = y - 1;
    }
    /* dob */
    L(a3, M_DOB) = (ULONG)a1;
    *a1++ = d7;
    *a1++ = d7;
    *a1++ = DMAOFF;
}

/* InitCopList CopList#,ypos,height,type,sprites,colors,numcustoms */
static void BLITZ_ROUTINE _initcoplist_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                         ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4),
                                         ULONG d5 BL_REG(d5), ULONG d6 BL_REG(d6),
                                         const UWORD *a2 BL_REG(a2), const UWORD *a0 BL_REG(a0))
{
    initcoplist(d1, d2, d3, d4, d5, a2, a0, (WORD)d6);
}

/* InitCopList CopList#,type (quick version) */
static void BLITZ_ROUTINE _initcoplist2_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                          ULONG d6 BL_REG(d6), const UWORD *a0 BL_REG(a0))
{
    ULONG d3 = d1, d5;

    BL_SETW(d1, 44);
    BL_SETW(d2, 256);
    d5 = colup[(UWORD)d3 & 15];
    if (d3 & (1 << 5))
        d5 >>= 1;
    if (d3 & (1 << 6))
        d5 = 32;
    initcoplist(d1, d2, d3, 8, d5, colup + 1, a0, (WORD)d6);
}
BLITZ_THUNK(_initcoplist, _initcoplist_c, "a2");
BLITZ_THUNK(_initcoplist2, _initcoplist2_c, "a2");

/* ---------------------------------------- CreateDisplay (Blitz mode only) */

static void BLITZ_ROUTINE _createdisplay_c(UWORD d7 BL_REG(d7), const UWORD *a2 BL_REG(a2),
                                           UBYTE *a0 BL_REG(a0))
{
    UBYTE *a3;
    ULONG *a1;
    UWORD d1, d2;

    if ((WORD)--d7 < 0)
        return;
    a3 = a0 + (WORD)(UWORD)(*--a2 << 7);
    blitz_string = a3;
    if (!L(a3, M_SIZE))
        GOTO(uniniterr);
    cd.topcop = L(a3, M_COPPOS);
    cd.lastwait = 0;
    for (;;) {
        /* nxlink */
        d1 = W(a3, M_YPOS);
        d2 = W(a3, M_HEIGHT);
        d1 -= W(a3, M_SETUP);
        a1 = putwait((ULONG *)L(a3, M_COPPOS), d1, cd.lastwait, 0xe1);
        cd.lastwait = d1;
        d1 += W(a3, M_SETUP);
        a1 = putwait((ULONG *)L(a3, M_DOT), d1, cd.lastwait, 0x01);
        d1 += d2;
        a1 = putwait((ULONG *)L(a3, M_DOB), d1, W(a3, M_BOT), 0x01);
        cd.lastwait = d1;
        /* dolink */
        a1++;
        if ((WORD)--d7 == -1)
            break;
        /* another */
        a3 = a0 + (WORD)(UWORD)(*--a2 << 7);
        blitz_string = a3;
        if (!L(a3, M_SIZE))
            GOTO(uniniterr);
        {
            ULONG c = L(a3, M_COPPOS);
            UWORD *w = (UWORD *)a1;

            w[0] = 0x86;
            w[1] = (UWORD)c;
            w[2] = 0x84;
            w[3] = (UWORD)(c >> 16);
            a1 += 2;
            *a1 = STROBE1;
        }
    }
    *a1 = WAITEND;
    {
        volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;
        struct bl_regs r = { .a0 = cd.topcop };

        (void)(*dmaconr & (1 << 6));            /* BlitWait */
        while (*dmaconr & (1 << 6))
            ;
        __asm volatile("move.l %1,%0" : "=m"(*(volatile ULONG *)0xdff080) : "d"(cd.topcop));
        BLITZ_ALIBJSR(TOK_setcoplist, r);
    }
}
BLITZ_THUNK(_createdisplay, _createdisplay_c, "a2");

/* ------------------------------------------------- display bitmap routines */

/* the bitplane pointer moves: one plane */
static UWORD *setptr(UWORD *a2, ULONG v)
{
    a2[2] = (UWORD)v;
    a2[0] = (UWORD)(v >> 16);
    return a2 + 4;
}

/* _displaybmap2: d2 = x (quick), d3 = y, a0 = bitmap, a3 = coplist */
static void displaybmap2(UBYTE *a3, const UBYTE *a0, ULONG d2, ULONG d3)
{
    UWORD *a2;
    ULONG d0, d1, d4, d5, d6, d7;

    L(a3, M_X0) = d2;
    L(a3, M_X1) = d2;
    d7 = W(a3, M_RESSHIFT);
    d2 = BL_SWAP(asl_l(d2, d7));
    a2 = (UWORD *)L(a3, M_BPLANES);
    d0 = W(a3, M_NUMBP);
    if (!d0)
        return;
    d0--;
    d5 = BL_EXTL(W(a3, M_FETCHWID));
    d6 = BL_EXTL(W(a3, M_FETCHWID + 2));
    d7 = BL_EXTL(W(a3, M_FETCHWID + 4));
    d1 = (UWORD)(W(a0, 0) - (UWORD)d5);
    a2[-3] = (UWORD)d1;
    a2[-1] = (UWORD)d1;
    d4 = (UWORD)((UWORD)d2 & (UWORD)d6);
    d5 = W(a3, M_HAM8);
    d4 = (UWORD)(d4 + d4);
    blitz_string = (UBYTE *)bp1;                /* LEA bp1(pc),a3 */
    a2[-5] = BP1(d4);
    BL_SETW(d6, ~(UWORD)d6);
    BL_SETW(d2, (UWORD)d2 & (UWORD)d6);
    d2 = BL_EXTL(d2);
    d2 = asr_w(d2, d7);
    d3 = mulu16(W(a0, 0), (UWORD)d3);
    d2 += d3;
    a0 += 8;
    a2 += 1;
    if ((UWORD)d5) {
        a2 = setptr(a2, L(a0, 24) + d2);
        a2 = setptr(a2, L(a0, 28) + d2);
        BL_SETW(d0, (UWORD)d0 - 2);
    }
    do {
        a2 = setptr(a2, *(const ULONG *)a0 + d2);
        a0 += 4;
    } while ((WORD)--d0 != -1);
}

static void BLITZ_ROUTINE _displaybmap2_c(const UBYTE *a0 BL_REG(a0), ULONG d2 BL_REG(d2),
                                          ULONG d3 BL_REG(d3))
{
    displaybmap2(blitz_string, a0, d2, d3);
}

/* DisplayBitMap CopList#,bmap - d2/d3 are the caller's registers */
static void BLITZ_ROUTINE _displaybmap_c(const UBYTE *a0 BL_REG(a0), ULONG d2 BL_REG(d2),
                                         ULONG d3 BL_REG(d3))
{
    UBYTE *a3 = blitz_string;
    UWORD *a2;
    UWORD d0, d1;
    const UWORD *p;

    if (B(a3, M_RES + 1) & (1 << 4)) {
        displaybmap2(a3, a0, 0, 0);
        return;
    }
    L(a3, M_X0) = d2;                           /* BUG(orig) (#411): stale d2/d3 */
    L(a3, M_X1) = d3;
    a2 = (UWORD *)L(a3, M_BPLANES);
    d0 = W(a3, M_NUMBP);
    if (!d0)
        return;
    d0--;
    d1 = W(a0, 0) - W(a3, M_FETCHWID);
    a2[-3] = d1;
    a2[-1] = d1;
    p = (const UWORD *)(a0 + 8);
    a2 += 1;
    if (W(a3, M_HAM8)) {
        a2[0] = p[12];
        a2[2] = p[13];
        a2[4] = p[14];
        a2[6] = p[15];
        a2 += 8;
        d0 -= 2;
    }
    do {
        a2[0] = *p++;
        a2[2] = *p++;
        a2 += 4;
    } while ((WORD)--d0 != -1);
}
BLITZ_THUNK(_displaybmap, _displaybmap_c, "a1");
BLITZ_THUNK(_displaybmap2, _displaybmap2_c, "a1");

/* scludge: two bitmaps, d2/d5 = x (quick), d3/d6 = y */
static void displaydbmap2(UBYTE *a3, const UBYTE *a0, const UBYTE *a1,
                          ULONG d2, ULONG d3, ULONG d5, ULONG d6)
{
    UWORD *a2;
    ULONG d0, d1, d4, d7;
    UWORD n;

    L(a3, M_X0) = d2;
    L(a3, M_X1) = d5;
    d7 = W(a3, M_RESSHIFT);
    d2 = BL_SWAP(asl_l(d2, d7));
    d5 = BL_SWAP(asl_l(d5, d7));
    a2 = (UWORD *)L(a3, M_BPLANES);
    n = W(a3, M_NUMBP);
    if (!n)
        return;
    d0 = BL_EXTL(W(a3, M_FETCHWID));
    d4 = BL_EXTL(W(a3, M_FETCHWID + 2));
    d7 = BL_EXTL(W(a3, M_FETCHWID + 4));
    blitz_string = (UBYTE *)bp1;                /* LEA bp1(pc),a3 */
    d1 = W(a0, 0);
    d3 = mulu16((UWORD)d1, (UWORD)d3);
    a2[-3] = (UWORD)(d1 - d0);
    d1 = W(a1, 0);
    d6 = mulu16((UWORD)d1, (UWORD)d6);
    a2[-1] = (UWORD)(d1 - d0);
    {
        UWORD x0 = BP1((UWORD)((UWORD)d2 & (UWORD)d4) * 2);
        UWORD x1 = BP1((UWORD)((UWORD)d5 & (UWORD)d4) * 2);

        a2[-5] = (x1 & 0xf0f0) | (x0 & 0x0f0f);
    }
    BL_SETW(d4, ~(UWORD)d4);
    BL_SETW(d2, (UWORD)d2 & (UWORD)d4);
    d2 = asr_w(BL_EXTL(d2), d7) + d3;
    BL_SETW(d5, (UWORD)d5 & (UWORD)d4);
    d5 = asr_w(BL_EXTL(d5), d7) + d6;
    n--;
    a0 += 8;
    a1 += 8;
    a2 += 1;
    for (;;) {
        a2 = setptr(a2, *(const ULONG *)a0 + d2);
        a0 += 4;
        if ((WORD)--n == -1)
            return;
        a2 = setptr(a2, *(const ULONG *)a1 + d5);
        a1 += 4;
        if ((WORD)--n == -1)
            return;
    }
}

static void BLITZ_ROUTINE _displaydbmap2_c(const UBYTE *a0 BL_REG(a0), const UBYTE *a1 BL_REG(a1),
                                           ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3),
                                           ULONG d5 BL_REG(d5), const UWORD *a2 BL_REG(a2))
{
    displaydbmap2(blitz_string, a0, a1, d2, d3, d5, a2[-1]);
}

static void BLITZ_ROUTINE _displaydbmap_c(const UBYTE *a0 BL_REG(a0), const UBYTE *a1 BL_REG(a1))
{
    UBYTE *a3 = blitz_string;
    UWORD *a2;
    UWORD d0, d1;

    if (B(a3, M_RES + 1) & (1 << 4)) {
        displaydbmap2(a3, a0, a1, 0, 0, 0, 0);
        return;
    }
    L(a3, M_X0) = 0;
    L(a3, M_X1) = 0;
    a2 = (UWORD *)L(a3, M_BPLANES);
    d0 = W(a3, M_NUMBP);
    if (!d0)
        return;
    d0--;
    d1 = W(a0, 0) - W(a3, M_FETCHWID);
    a2[-3] = d1;
    d1 = W(a1, 0) - W(a3, M_FETCHWID);
    a2[-1] = d1;
    a0 += 8;
    a1 += 8;
    a2 += 1;
    for (;;) {
        a2 = setptr(a2, *(const ULONG *)a0);
        a0 += 4;
        if ((WORD)--d0 == -1)
            return;
        a2 = setptr(a2, *(const ULONG *)a1);
        a1 += 4;
        if ((WORD)--d0 == -1)
            return;
    }
}
BLITZ_THUNK(_displaydbmap, _displaydbmap_c, "a2");
BLITZ_THUNK(_displaydbmap2, _displaydbmap2_c, "a2");

/* ------------------------------------------------------ sprite routines */

static void spriteoff(UBYTE *a3, const UBYTE *a1, UWORD d3)
{
    UWORD d2 = W(a1, 6) - 1;
    UBYTE *a0 = (UBYTE *)L(a3, M_SPRITES) + (WORD)(UWORD)(d3 << 4);

    do {
        W(a0, 2) = 0;
        W(a0, 6) = 0;
        W(a0, 10) = (UWORD)(d.mysprite >> 16);
        W(a0, 14) = (UWORD)d.mysprite;
        a0 += 16;
        if ((WORD)W(a1, 8) < 0) {
            W(a0, 2) = 0;
            W(a0, 6) = 0;
            W(a0, 10) = (UWORD)(d.mysprite >> 16);
            W(a0, 14) = (UWORD)d.mysprite;
            a0 += 16;
            d2--;
        }
    } while ((WORD)--d2 != -1);
}

/* DisplaySprite CopList#,Sprite#,X,Y,Channel: a1 = sprite, d2 = x
   (quick), d3 = y, d4 = channel */
static void BLITZ_ROUTINE _displaysprite_c(const UBYTE *a1 BL_REG(a1), ULONG d2 BL_REG(d2),
                                           ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4))
{
    UBYTE *a3 = blitz_string, *a0;
    ULONG d0, d1, d5, d6, d7;

    if (!L(a1, 0))
        GOTO(nosprierr);
    d5 = (UWORD)d4;
    d6 = W(a3, M_NUMSPRITES);
    if (L(a3, M_CSPRITES))
        d6 = (UWORD)(d6 + d6);
    d5 = (UWORD)(d5 + W(a1, 6));
    if ((WORD)((UWORD)d6 - (UWORD)d5) < 0)      /* CMP d5,d6:BMI */
        GOTO(spriterr);
    d1 = d2;
    BL_SETW(d2, (UWORD)d3);
    BL_SETW(d3, (UWORD)d4);
    d4 = BL_EXTL(W(a1, 12));
    d5 = BL_EXTL(W(a1, 14));
    d6 = W(a3, M_SPRI);
    d4 = asr_w(asl_w(d4, 2), d6);
    d1 = BL_SWAP(d1);
    d6 = W(a3, M_RESSHIFT);
    d1 = rol_l(d1, d6);
    BL_SETW(d1, (UWORD)d1 + 512);
    BL_SETW(d1, (UWORD)d1 - (UWORD)d4);
    BL_SETW(d2, (UWORD)d2 - (UWORD)d5);
    {
        WORD h = (WORD)(W(a3, M_HEIGHT) + 1);

        if (h <= (WORD)d2) {                    /* SUB d2,d0:BLE */
            spriteoff(a3, a1, (UWORD)d3);
            return;
        }
        d0 = (UWORD)(h - (WORD)d2);
    }
    d4 = 0;
    d5 = W(a1, 4);
    if ((WORD)d2 < 0) {                         /* off the top */
        if ((LONG)(WORD)d5 + (WORD)d2 <= 0) {   /* ADD d2,d5:BLE */
            spriteoff(a3, a1, (UWORD)d3);
            return;
        }
        d5 = (UWORD)((UWORD)d5 + (UWORD)d2);
        {
            ULONG t = d2;                       /* EXG d2,d4 */

            d2 = d4;
            d4 = t;
        }
        BL_SETW(d4, -(WORD)d4);
        d7 = W(a3, M_SPIF);
        d4 = asl_l(BL_EXTL(d4), d7);
    }
    if ((WORD)((UWORD)d0 - (UWORD)d5) < 0)      /* CMP d5,d0:BPL */
        d5 = (UWORD)d0;
    BL_SETW(d2, (UWORD)d2 + W(a3, M_YPOS));
    d7 = 0;
    d6 = (UWORD)d2;
    if (d6 & 0x100)                             /* ASL #8,d6: carry */
        d7 |= 1 << 2;
    d6 = (UWORD)(d6 << 8);
    if (!d.agamac)
        BL_SETW(d1, (UWORD)d1 & 0xfffc);
    if (d1 & 1)
        d7 |= 1 << 3;
    BL_SETW(d1, (UWORD)d1 >> 1);
    if (d1 & 1)
        d7 |= 1 << 4;
    BL_SETW(d1, (UWORD)d1 >> 1);
    if (d1 & 1)
        d7 |= 1 << 0;
    BL_SETW(d1, (UWORD)d1 >> 1);
    if ((WORD)d1 < 0)
        d1 = 0;
    if ((WORD)((UWORD)d1 - 256) >= 0)           /* CMP #256,d1:BMI */
        BL_SETW(d1, 255);
    d6 = (d6 & 0xff00) | (UBYTE)d1;
    BL_SETW(d2, (UWORD)d2 + (UWORD)d5 - 1);
    if (d2 & 0x100)                             /* LSL #8,d2: carry */
        d7 |= 1 << 1;
    BL_SETW(d2, (UWORD)d2 << 8);
    d7 |= (UWORD)d2;
    if ((WORD)W(a1, 8) < 0)
        d7 |= 1 << 7;
    a0 = (UBYTE *)L(a3, M_SPRITES);
    if ((UWORD)d3 >= W(a3, M_NUMSPRITES)) {
        BL_SETW(d3, (UWORD)d3 - W(a3, M_NUMSPRITES));
        a0 = (UBYTE *)L(a3, M_CSPRITES);
    }
    a0 += (WORD)(UWORD)((UWORD)d3 << 4);
    d0 = L(a1, 0) + d4;
    d1 = BL_EXTL(W(a1, 10));
    d2 = (UWORD)(W(a1, 6) - 1);
    do {
        W(a0, 2) = (UWORD)d6;
        W(a0, 6) = (UWORD)d7;
        W(a0, 14) = (UWORD)d0;
        W(a0, 10) = (UWORD)(d0 >> 16);
        d0 += d1;
        a0 += 16;
        if ((WORD)W(a1, 8) < 0) {               /* attached */
            W(a0, 2) = (UWORD)d6;
            W(a0, 6) = (UWORD)d7;
            W(a0, 14) = (UWORD)d0;
            W(a0, 10) = (UWORD)(d0 >> 16);
            d0 += d1;
            a0 += 16;
            d2 = (UWORD)(d2 - 1);
        }
        d6 = (UWORD)(d6 + W(a3, M_SSPWID));
    } while ((WORD)(d2 = (UWORD)(d2 - 1)) != -1);
}
BLITZ_THUNK(_displaysprite, _displaysprite_c, "a1");

/* ------------------------------------------------ palette routines */

/* simple: ECS colour moves from the palette at a0 */
static void simple(UBYTE *a3, const UBYTE *a0, ULONG *a1, UWORD d7)
{
    ULONG d0 = 0x01800000;                      /* MOVE #$180,d0:SWAP d0 (low
                                                   word replaced below) */

    if ((WORD)d7 > 32)
        GOTO(agapalerr);
    d7--;
    do {
        BL_SETW(d0, W(a0, 0) >> 4);
        d0 = (d0 & 0xffffff00) | B(a0, 4);
        BL_SETW(d0, (UWORD)d0 << 4);
        d0 = (d0 & 0xffffff00) | B(a0, 8);
        BL_SETW(d0, (UWORD)d0 >> 4);
        *a1++ = d0;
        d0 += 0x20000;
        a0 += 12;
    } while ((WORD)--d7 != -1);
    OVERRUN(a3, a1);
}

/* complex2: AGA, banks of 32 colours, high and low nibbles */
static void complex2(UBYTE *a3, const UBYTE *a0, ULONG *a1, ULONG d0, UWORD d7)
{
    ULONG d1;

    d7--;
    do {
        UWORD d6 = 31, d5;
        const UBYTE *a2 = a0;

        if ((WORD)(d7 - d6) < 0)                /* CMP d6,d7:BPL */
            d6 = d7;
        *a1++ = d0;
        d5 = d6;
        d1 = 0x01800000;                        /* MOVE #$180,d1:SWAP d1 */
        do {
            BL_SETW(d1, W(a0, 0) >> 4);
            d1 = (d1 & 0xffffff00) | B(a0, 4);
            BL_SETW(d1, (UWORD)d1 << 4);
            d1 = (d1 & 0xffffff00) | B(a0, 8);
            BL_SETW(d1, (UWORD)d1 >> 4);
            *a1++ = d1;
            d1 += 0x20000;
            a0 += 12;
        } while ((WORD)--d5 != -1);
        d0 ^= 1 << 9;
        *a1++ = d0;
        d5 = d6;
        d1 = 0x01800000;
        a0 = a2;
        do {
            UBYTE b = (UBYTE)(B(a0, 4) << 4);

            BL_SETW(d1, W(a0, 0));
            d1 = (d1 & 0xffffff00) | (UBYTE)((B(a0, 8) & 15) | b);
            *a1++ = d1;
            d1 += 0x20000;
            a0 += 12;
        } while ((WORD)--d5 != -1);
        d0 ^= 1 << 9;
        BL_SETW(d0, (UWORD)d0 + 0x2000);
        d7 -= 32;
    } while ((WORD)d7 >= 0);
    OVERRUN(a3, a1);
}

/* the palette data: (a0) -> +4, entries of 12 bytes */
static const UBYTE *paldata(const UBYTE *a0)
{
    ULONG p = L(a0, 0);

    if (!p)
        GOTO(nopalerr);
    return (const UBYTE *)p + 4;
}

static void BLITZ_ROUTINE _displaypalette_c(const UBYTE *a0 BL_REG(a0), ULONG d1 BL_REG(d1))
{
    UBYTE *a3 = blitz_string;
    ULONG *a1;
    UWORD d7;

    a0 = paldata(a0);
    a1 = (ULONG *)L(a3, M_COLORS);
    d7 = W(a3, M_NUMCOLS);
    if (!d7) {
        OVERRUN(a3, a1);
        return;
    }
    if ((WORD)W(a3, M_AGA) >= 0)
        simple(a3, a0, a1, d7);
    else
        complex2(a3, a0, a1, 0x01060000 | W(a3, M_BPLCON3), d7);
}

static void BLITZ_ROUTINE _displaypalette2_c(const UBYTE *a0 BL_REG(a0), ULONG d1 BL_REG(d1),
                                             UWORD d2 BL_REG(d2))
{
    UBYTE *a3 = blitz_string;
    ULONG *a1;
    UWORD d7;

    a0 = paldata(a0);
    a0 += (WORD)(UWORD)mulu16(d2, 12);
    a1 = (ULONG *)L(a3, M_COLORS);
    d7 = W(a3, M_NUMCOLS);
    if (!d7) {
        OVERRUN(a3, a1);
        return;
    }
    if ((WORD)W(a3, M_AGA) >= 0)
        simple(a3, a0, a1, d7);
    else
        complex2(a3, a0, a1, 0x01060000 | W(a3, M_BPLCON3), d7);
}

static void BLITZ_ROUTINE _displaypalette3_c(const UBYTE *a0 BL_REG(a0), ULONG d1 BL_REG(d1),
                                             UWORD d2 BL_REG(d2), UWORD d3 BL_REG(d3))
{
    UBYTE *a3 = blitz_string;
    ULONG *a1;
    UWORD d7;

    a0 = paldata(a0);
    a0 += (WORD)(UWORD)mulu16(d2, 12);
    a1 = (ULONG *)L(a3, M_COLORS);
    d7 = W(a3, M_NUMCOLS);
    if (!d7) {
        OVERRUN(a3, a1);
        return;
    }
    if ((WORD)W(a3, M_AGA) >= 0)
        simple(a3, a0, a1, d7);
    else
        complex2(a3, a0, a1,
                 0x01060000 | (UWORD)(W(a3, M_BPLCON3) + (UWORD)(d3 << 13)), d7);
}
BLITZ_THUNK(_displaypalette, _displaypalette_c, "a2");
BLITZ_THUNK(_displaypalette2, _displaypalette2_c, "a2");
BLITZ_THUNK(_displaypalette3, _displaypalette3_c, "a2");

/* CustomColors CopList#,CCOffset,YPos,Palette,startcol,numcols */
static void customcols(UWORD d1, UWORD d2, const UBYTE *a0, UWORD d4, UWORD d5)
{
    UBYTE *a3 = blitz_string;
    ULONG *a1 = (ULONG *)((UBYTE *)L(a3, M_CUSTOMS) + (WORD)(UWORD)(d1 << 2));
    UWORD y = d2 + W(a3, M_YPOS);
    ULONG d0;

    a1 = putwait(a1, y, W(a3, M_BOT), 0xe1);
    W(a3, M_BOT) = y + 1;
    a0 = paldata(a0);
    a0 += (WORD)(UWORD)mulu16(d4, 12);
    if (!d5) {
        OVERRUN(a3, a1);
        return;
    }
    if ((WORD)W(a3, M_AGA) >= 0) {
        simple(a3, a0, a1, d5);
        return;
    }
    d0 = 0x01060000 | (UWORD)(W(a3, M_BPLCON3) | (UWORD)((d4 & 0xe0) << 8));
    complex2(a3, a0, a1, d0, d5);
}

static void BLITZ_ROUTINE _customcols_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                        const UBYTE *a0 BL_REG(a0), UWORD d4 BL_REG(d4),
                                        UWORD d5 BL_REG(d5))
{
    customcols(d1, d2, a0, d4, d5);
}

static void BLITZ_ROUTINE _customcols2_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                         const UBYTE *a0 BL_REG(a0), UWORD d4 BL_REG(d4),
                                         UWORD d5 BL_REG(d5))
{
    customcols(d1, d2, a0, d4, d5);
    W(blitz_string, M_BOT) = 257;
}
BLITZ_THUNK(_customcols, _customcols_c, "a2");
BLITZ_THUNK(_customcols2, _customcols2_c, "a2");

/* ------------------------------------------------- custom copper routines */

/* CustomString CopList#,CCOffset,YPos,Copper$ */
static void BLITZ_ROUTINE _customstring_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                          const ULONG *s BL_REG(d3), const ULONG *lens BL_REG(a2))
{
    UBYTE *a3 = blitz_string;
    ULONG *a1 = (ULONG *)((UBYTE *)L(a3, M_CUSTOMS) + ((LONG)(WORD)d1 << 2));
    UWORD y = d2 + W(a3, M_YPOS);
    ULONG d0 = lens[-1];

    a1 = putwait(a1, y, W(a3, M_BOT), 0xe1);
    W(a3, M_BOT) = y + 1;
    if (d0 & 3)
        GOTO(freakerr);
    d0 >>= 2;
    if (!d0)
        GOTO(freakerr);
    {
        UWORD n = (UWORD)(d0 - 1);              /* SUBQ #1,d0:DBRA (words) */

        do
            *a1++ = *s++;
        while ((WORD)--n != -1);
    }
    OVERRUN(a3, a1);
}
BLITZ_THUNK(_customstring, _customstring_c, "a2");

/* DisplayDblScan CopList#,Mode[,copoffset] */
static void dblscan(UWORD mode, UWORD off)
{
    UBYTE *a3 = blitz_string;
    UBYTE *a1;
    UWORD d2 = (UWORD)((off + 1) << 2), d5 = W(a3, M_CPLINE), d7;
    WORD d6;
    ULONG d1, dd2, d3, d4;

    a1 = (UBYTE *)L(a3, M_CUSTOMS) + (WORD)d2;
    d2 += 8;
    if ((WORD)d2 > (WORD)d5)
        GOTO(linerr);
    d7 = W(a3, M_HEIGHT) - 2;
    d6 = (WORD)(mode - 1);
    if (d6 < 0) {                                       /* clrdbl */
        do {
            L(a1, 0) = COPNOP;
            L(a1, 4) = COPNOP;
            a1 += (WORD)d5; OPAQUE(a1);
        } while ((WORD)--d7 != -1);
        return;
    }
    {
        const ULONG *a2 = (const ULONG *)L(a3, M_BPLANES);

        d1 = a2[-2];
        dd2 = a2[-1];
    }
    d3 = d1;
    d4 = dd2;
    BL_SETW(d1, -(WORD)W(a3, M_FETCHWID));
    BL_SETW(dd2, (UWORD)d1);
#define STORE() do { L(a1, 0) = d1; L(a1, 4) = dd2; a1 += (WORD)d5; OPAQUE(a1); } while (0)
#define SWAPMODS() do { ULONG t_ = d1; d1 = d3; d3 = t_; t_ = dd2; dd2 = d4; d4 = t_; } while (0)
    if (!d6) {                                          /* nxline */
        do {
            STORE();
            SWAPMODS();
        } while ((WORD)--d7 != -1);
        return;
    }
    /* dbldbl */
    dl.loopd = *(const UWORD *)((const UBYTE *)dl.dloop + (WORD)(UWORD)(d6 + d6));
    for (;;) {
        UWORD k = dl.loopd;                             /* nux1 */

        for (;;) {
            STORE();                                    /* lbub */
            if ((WORD)--d7 == -1)
                return;
            if ((WORD)--k == -1)                        /* nux2 */
                break;
        }
        STORE();
        SWAPMODS();
        if ((WORD)--d7 == -1)
            return;
        STORE();                                        /* nux4 */
        SWAPMODS();
        if ((WORD)--d7 == -1)
            return;
    }
#undef STORE
#undef SWAPMODS
}

static void BLITZ_ROUTINE _displaydblscan0_c(UWORD d1 BL_REG(d1)) { dblscan(d1, 0); }
static void BLITZ_ROUTINE _displaydblscan_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2))
{
    dblscan(d1, d2);
}
BLITZ_THUNK(_displaydblscan0, _displaydblscan0_c, "a0");
BLITZ_THUNK(_displaydblscan, _displaydblscan_c, "a0");

/* CustomSprites CopList#,CCOffset,YPos,NumSprites.  BUG(orig) (#324): d1 is
   CCOffset * 16 (see the declaration), the a1 the compiler loaded is not
   used: the offset is CCOffset * 64 bytes */
static void BLITZ_ROUTINE _customsprites_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                           UWORD d3 BL_REG(d3))
{
    UBYTE *a3 = blitz_string;
    ULONG *a1 = (ULONG *)((UBYTE *)L(a3, M_CUSTOMS) + (WORD)(UWORD)(d1 << 2));
    UWORD y = d2 + W(a3, M_YPOS), n;
    ULONG s3 = spp[0], s4 = spp[1], s5 = spp[2], s6 = spp[3];

    a1 = putwait(a1, y, W(a3, M_BOT), 0xe1);
    W(a3, M_BOT) = y + 1;
    L(a3, M_CSPRITES) = (ULONG)a1;
    n = d3 - 1;
    do {
        a1[0] = s3;
        a1[1] = s4;
        a1[2] = s5;
        a1[3] = s6;
        a1 += 4;
        s3 += 0x80000;
        s4 += 0x80000;
        s5 += 0x40000;
        s6 += 0x40000;
    } while ((WORD)--n != -1);
    OVERRUN(a3, a1);
}
BLITZ_THUNK(_customsprites, _customsprites_c, "a0");

/* ---------------------------------------------------- colour splits */

/* DisplayRainbow CopList#,Register,Palette[,copoffset]: a2 = palette */
static void rainbow(UWORD reg, const UBYTE *pal, UWORD off)
{
    UBYTE *a3 = blitz_string;
    const ULONG *a2 = (const ULONG *)(L(pal, 0) + 4);
    UWORD d3 = (UWORD)((off + 1) << 2), d5 = W(a3, M_CPLINE), d7;
    UBYTE *a1 = (UBYTE *)L(a3, M_CUSTOMS) + (WORD)d3;

    if ((WORD)W(a3, M_AGA) < 0) {
        ULONG r3, d6, d0, d1, d2;

        d3 += 16;
        if ((WORD)d3 > (WORD)d5)
            GOTO(linerr);
        r3 = 0x01060000 | (UWORD)(W(a3, M_BPLCON3) | (UWORD)((reg & 0xe0) << 8));
        d6 = (ULONG)(UWORD)(((reg & 0x1f) << 1) + 0x180) << 16;
        d7 = W(a3, M_HEIGHT) - 2;
        do {
            d0 = a2[0];
            d1 = a2[1];
            d2 = a2[2];
            a2 += 3;
            d0 = rol_l(d0, 4);
            BL_SETW(d1, (UWORD)d0);
            d1 = rol_l(d1, 4);
            BL_SETW(d2, (UWORD)d1);
            d2 = rol_l(d2, 4);
            BL_SETW(d6, (UWORD)d2);
            L(a1, 0) = r3;
            L(a1, 4) = d6;
            r3 ^= 1 << 9;
            d0 = rol_l(d0, 4);
            BL_SETW(d1, (UWORD)d0);
            d1 = rol_l(d1, 4);
            BL_SETW(d2, (UWORD)d1);
            d2 = rol_l(d2, 4);
            BL_SETW(d6, (UWORD)d2);
            L(a1, 8) = r3;
            L(a1, 12) = d6;
            r3 ^= 1 << 9;
            a1 += (WORD)d5; OPAQUE(a1);
        } while ((WORD)--d7 != -1);
    } else {
        ULONG d2, r3, d4;
        UWORD d1;

        d3 += 4;
        if ((WORD)d3 > (WORD)d5)
            GOTO(linerr);
        d1 = (UWORD)(reg + reg + 0x180);
        d7 = W(a3, M_HEIGHT) - 2;
        do {
            d2 = a2[0];
            r3 = a2[1];
            d4 = a2[2];
            a2 += 3;
            d2 = rol_l(d2, 4);
            BL_SETW(r3, (UWORD)d2);
            r3 = rol_l(r3, 4);
            BL_SETW(d4, (UWORD)r3);
            d4 = rol_l(d4, 4);
            W(a1, 0) = d1;
            W(a1, 2) = (UWORD)d4;
            a1 += (WORD)d5; OPAQUE(a1);
        } while ((WORD)--d7 != -1);
    }
    a1 -= (WORD)d5;
    OVERRUN(a3, a1);
}

static void BLITZ_ROUTINE _displayrainbow0_c(UWORD d1 BL_REG(d1), const UBYTE *a2 BL_REG(a2))
{
    rainbow(d1, a2, 0);
}
static void BLITZ_ROUTINE _displayrainbow_c(UWORD d1 BL_REG(d1), const UBYTE *a2 BL_REG(a2),
                                            UWORD d3 BL_REG(d3))
{
    rainbow(d1, a2, d3);
}
BLITZ_THUNK(_displayrainbow0, _displayrainbow0_c, "a0");
BLITZ_THUNK(_displayrainbow, _displayrainbow_c, "a0");

/* DisplayRGB CopList#,Register,line,r,g,b[,copoffset]: -2(a2) = offset */
static void rgb(UWORD d1, UWORD d2, UWORD d3, UWORD d4, UWORD d5, UWORD off)
{
    UBYTE *a3 = blitz_string;
    UWORD d6 = (UWORD)((off + 1) << 2), d7 = W(a3, M_CPLINE);
    UBYTE *a1 = (UBYTE *)L(a3, M_CUSTOMS) + (WORD)d6;

    a1 += mulu16(d7, d2);
    if ((WORD)W(a3, M_AGA) < 0) {
        ULONG r2, v;
        UWORD w;

        d6 += 16;
        if ((WORD)d6 > (WORD)d7)
            GOTO(linerr);
        r2 = 0x01060000 | (UWORD)(W(a3, M_BPLCON3) | (UWORD)((d1 & 0xe0) << 8));
        v = (ULONG)(UWORD)(((d1 & 0x1f) << 1) + 0x180) << 16;
        w = d3;
        w = (UWORD)(w << 4 | w >> 12);                  /* ROL.w #4 */
        w = (w & 0xff00) | (UBYTE)d4;
        w = (UWORD)(w << 4 | w >> 12);
        w = (w & 0xff00) | (UBYTE)d5;
        w >>= 4;
        L(a1, 0) = r2;
        L(a1, 4) = v | w;
        r2 ^= 1 << 9;
        w = d5;
        w = (UWORD)(w >> 4 | w << 12);                  /* ROR.w #4 */
        w = (w & 0xff00) | (UBYTE)d4;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)d3;
        w = (UWORD)(w << 8 | w >> 8);                   /* ROL.w #8 */
        L(a1, 8) = r2;
        L(a1, 12) = v | w;
    } else {
        UWORD w;

        d6 += 4;
        if ((WORD)d6 > (WORD)d7)
            GOTO(linerr);
        d1 = (UWORD)(d1 + d1 + 0x180);
        w = d5;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)d4;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)d3;
        w = (UWORD)(w << 8 | w >> 8);
        W(a1, 0) = d1;
        W(a1, 2) = w;
    }
    OVERRUN(a3, a1);
}

static void BLITZ_ROUTINE _displayrgb0_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                         UWORD d3 BL_REG(d3), UWORD d4 BL_REG(d4),
                                         UWORD d5 BL_REG(d5))
{
    rgb(d1, d2, d3, d4, d5, blah[0]);
}
static void BLITZ_ROUTINE _displayrgb_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                        UWORD d3 BL_REG(d3), UWORD d4 BL_REG(d4),
                                        UWORD d5 BL_REG(d5), const UWORD *a2 BL_REG(a2))
{
    rgb(d1, d2, d3, d4, d5, a2[-1]);
}
BLITZ_THUNK(_displayrgb0, _displayrgb0_c, "d0/a0");
BLITZ_THUNK(_displayrgb, _displayrgb_c, "d0/a0");

/* DisplayUser CopList#,Line,String[,Offset] */
static void user(UWORD line, const ULONG *s, UWORD off, ULONG len)
{
    UBYTE *a3 = blitz_string;
    UWORD d3 = (UWORD)((off + 1) << 2), d7 = W(a3, M_CPLINE), d0;
    ULONG *a1 = (ULONG *)((UBYTE *)L(a3, M_CUSTOMS) + (WORD)d3);

    a1 = (ULONG *)((UBYTE *)a1 + mulu16(d7, line));
    d3 += (UWORD)len;
    if ((WORD)d3 > (WORD)d7)
        GOTO(linerr);
    d0 = (UWORD)len;
    if (d0 & 3)
        GOTO(freakerr);
    d0 >>= 2;
    if (d0) {
        d0--;
        do
            *a1++ = *s++;
        while ((WORD)--d0 != -1);
    }
    OVERRUN(a3, a1);
}

static void BLITZ_ROUTINE _displayuser0_c(UWORD d1 BL_REG(d1), const ULONG *d2 BL_REG(d2),
                                          const ULONG *lens BL_REG(a2))
{
    user(d1, d2, 0, lens[-1]);
}
static void BLITZ_ROUTINE _displayuser_c(UWORD d1 BL_REG(d1), const ULONG *d2 BL_REG(d2),
                                         UWORD d3 BL_REG(d3), const ULONG *lens BL_REG(a2))
{
    user(d1, d2, d3, lens[-1]);
}
BLITZ_THUNK(_displayuser0, _displayuser0_c, "a2");
BLITZ_THUNK(_displayuser, _displayuser_c, "a2");

/* DisplayScroll CopList#,&xpos.q(n),&xpos.q(n)[,Offset]: per line a
   BPLCON1 move and the two modulos */
static void scroll(const LONG *a0, const LONG *a1, UWORD off, ULONG d0in)
{
    UBYTE *a3 = blitz_string, *a4;
    ULONG d0, d1, d2, d3, d4, d5, d6, d7;
    UWORD idx;

    d0 = (d0in & 0xffffff00) | B(a3, M_RES);
    d6 = (UWORD)d0;
    BL_SETW(d0, (UWORD)d0 & 3);
    d4 = 0xfffff;
    idx = ((UWORD)d6 & 0x30) >> 2;
    d5 = *(const ULONG *)((const UBYTE *)shifs + idx);
    idx >>= 2;
    d6 = flups[idx];                    /* MOVE.b flups(pc,d6),d6 (d6.w = idx) */
    d4 = asl_l(d4, d6);
    BL_SETW(d6, (UWORD)d6 + 6 - (UWORD)d0);
    {
        UWORD d3w = (UWORD)((off + 1) << 2);

        a4 = (UBYTE *)L(a3, M_CUSTOMS) + (WORD)d3w;
        d3w += 12;
        if ((WORD)d3w > (WORD)W(a3, M_CPLINE))
            GOTO(linerr);
    }
    sc.ycont = W(a3, M_HEIGHT) - 2;
    d0 = L(a3, M_X0) & d4;
    d1 = L(a3, M_X1) & d4;
    {
        const ULONG *a2 = (const ULONG *)L(a3, M_BPLANES);

        d2 = a2[-3];
        d3 = a2[-2];
        d4 = a2[-1];
    }
    sc.mods[0] = (UWORD)d3;
    sc.mods[1] = (UWORD)d4;
    d7 = 0;
    do {
        L(a4, 0) = d2;
        d0 += (ULONG)*a0++;
        d1 += (ULONG)*a1++;
        d0 = asr_l(d0, d5);
        d1 = asr_l(d1, d5);
        d5 = BL_SWAP(d5);
        d0 = BL_SWAP(d0);
        d1 = BL_SWAP(d1);
        BL_SETW(d3, (UWORD)d0);
        BL_SETW(d4, (UWORD)d1);
        d3 = asl_w(d3, d5);
        d4 = asl_w(d4, d5);
        d5 = BL_SWAP(d5);
        BL_SETW(d3, (UWORD)d3 + sc.mods[0]);
        BL_SETW(d4, (UWORD)d4 + sc.mods[1]);
        BL_SETW(d0, 0);
        d0 = rol_l(d0, d6);
        BL_SETW(d2, (UWORD)d0);
        d0 = ror_l(d0, d6);
        d0 = asl_l(BL_SWAP(d0), d5);
        BL_SETW(d1, 0);
        d1 = rol_l(d1, d6);
        BL_SETW(d7, (UWORD)d1);
        d1 = ror_l(d1, d6);
        d1 = asl_l(BL_SWAP(d1), d5);
        BL_SETW(d2, BP1((UWORD)((UWORD)d2 + (UWORD)d2)) & 0x0f0f);
        BL_SETW(d7, BP1((UWORD)((UWORD)d7 + (UWORD)d7)) & 0xf0f0);
        BL_SETW(d2, (UWORD)d2 | (UWORD)d7);
        L(a4, 4) = d3;
        L(a4, 8) = d4;
        a4 += (WORD)W(a3, M_CPLINE); OPAQUE(a4);
    } while ((WORD)--sc.ycont >= 0);
}

static void BLITZ_ROUTINE _displayscroll0_c(ULONG d0 BL_REG(d0), const LONG *d1 BL_REG(d1),
                                            const LONG *d2 BL_REG(d2))
{
    scroll(d1, d2, 0, d0);
}
static void BLITZ_ROUTINE _displayscroll_c(ULONG d0 BL_REG(d0), const LONG *d1 BL_REG(d1),
                                           const LONG *d2 BL_REG(d2), UWORD d3 BL_REG(d3))
{
    scroll(d1, d2, d3, d0);
}
BLITZ_THUNK(_displayscroll0, _displayscroll0_c, "a2");
BLITZ_THUNK(_displayscroll, _displayscroll_c, "a2");

/* ------------------------------------------- chunky copper routines (AGA) */

/* CustomChunkySize(width,height) */
static ULONG BLITZ_ROUTINE _customchunkysize_c(ULONG d0 BL_REG(d0), UWORD d1 BL_REG(d1))
{
    UWORD d2 = (UWORD)((UWORD)((UWORD)d0 + 31) >> 5);

    BL_SETW(d0, (UWORD)d0 + d2 + 3);
    d0 = mulu16((UWORD)d0, d1);
    BL_SETW(d0, (UWORD)d0 + 2);
    return d0;
}
BLITZ_THUNK(_customchunkysize, _customchunkysize_c, "d1/a0-a1");

/* CustomChunky CopList#,CCOffset,YPos,width,height,res */
static void customchunky(UWORD d1, UWORD d2, UWORD d3, UWORD d4, UWORD d5)
{
    UBYTE *a3 = blitz_string;
    ULONG *a1, d0, d5l, d6, d4l;
    UWORD y, d7;

    W(a3, M_CHY) = d2;
    W(a3, M_CHW) = d3;
    W(a3, M_CHH) = d4;
    W(a3, M_CHH + 2) = d5;
    a1 = (ULONG *)((UBYTE *)L(a3, M_CUSTOMS) + (WORD)(UWORD)(d1 << 2));
    y = d2 - d5;
    if ((WORD)y < 0)
        GOTO(toolowerr);
    y += W(a3, M_YPOS);
    d2 += W(a3, M_YPOS);
    a1 = putwait(a1, y, W(a3, M_BOT), 0xe1);
    L(a3, M_CCHUNKY) = (ULONG)a1 + 6;
    W(a3, M_BOT) = (UWORD)(mulu16(d5, d4) + d2);
    d3 = (UWORD)(d3 + (d3 >> 5) + 3);
    d3 = (UWORD)(d3 << 2);
    W(a3, M_CHC) = d3;
    d0 = BL_SWAP(BL_EXTL((UWORD)(y + d5)));
    d0 <<= 8;
    d0 |= 0xe1fffe;
    d5l = BL_SWAP((ULONG)(UWORD)(d5 << 8) | (BL_EXTL(d5) & 0xffff0000));
    d7 = d4 - 1;
    d6 = 0x010c0011;
    d4l = 0x01066c00;
    do {
        UWORD d2c = W(a3, M_CHW) - 1, r = 0x17e;
        ULONG d3l = d4l;

        d4l ^= 1UL << 15;
        do {
            if (r == 0x17e) {
                r = 0x1be;
                *a1++ = d3l;
                BL_SETW(d3l, (UWORD)d3l - 0x2000);
            }
            ((UWORD *)a1)[0] = r;
            ((UWORD *)a1)[1] = 0;
            a1++;
            r -= 2;
        } while ((WORD)--d2c != -1);
        *a1++ = d0;
        *a1++ = d6;
        d6 ^= 1UL << 15;
        d0 += d5l;
    } while ((WORD)--d7 != -1);
    OVERRUN(a3, a1);
}

static void BLITZ_ROUTINE _customchunky_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                          UWORD d3 BL_REG(d3), UWORD d4 BL_REG(d4),
                                          UWORD d5 BL_REG(d5))
{
    customchunky(d1, d2, d3, d4, d5);
}

/* the second form: -2(a2) = split position */
static void BLITZ_ROUTINE _customchunky2_c(UWORD d1 BL_REG(d1), UWORD d2 BL_REG(d2),
                                           UWORD d3 BL_REG(d3), UWORD d4 BL_REG(d4),
                                           UWORD d5 BL_REG(d5), const UWORD *a2 BL_REG(a2))
{
    UBYTE *a3 = blitz_string;
    ULONG c[4];
    UWORD split;
    int k;

    customchunky(d1, d2, d3, d4, d5);
    for (k = 0; k < 4; k++)
        c[k] = L(a3, M_CCHUNKY + 4 * k);
    split = a2[-1];
    W(a3, M_CHH) = split;
    for (k = 0; k < 4; k++)
        L(a3, M_CCHUNKY2 + 4 * k) = c[k];
    W(a3, M_CHH2) -= split;
    L(a3, M_CCHUNKY2) += mulu16(W(a3, M_CHC), split);
}
BLITZ_THUNK(_customchunky, _customchunky_c, "a2");
BLITZ_THUNK(_customchunky2, _customchunky2_c, "a2");

/* ---------------------------------------------------------- error handling */

__asm__("   .section .blitzlib.errchk,\"ax\"\n   .even\n"
        "   .globl  _maxbperr, _uniniterr, _nopalerr, _nosprierr, _freakerr\n"
        "   .globl  _linerr, _agapalerr, _toolowerr, _overrunerr, _spriterr\n"
        "_maxbperr:   move.l  #_mess1,d0\n   trap    #0\n"
        "_uniniterr:  move.l  #_mess2,d0\n   trap    #0\n"
        "_nopalerr:   move.l  #_mess3,d0\n   trap    #0\n"
        "_nosprierr:  move.l  #_mess4,d0\n   trap    #0\n"
        "_freakerr:   move.l  #_mess5,d0\n   trap    #0\n"
        "_linerr:     move.l  #_mess6,d0\n   trap    #0\n"
        "_agapalerr:  move.l  #_mess7,d0\n   trap    #0\n"
        "_toolowerr:  move.l  #_mess8,d0\n   trap    #0\n"
        "_overrunerr: move.l  #_mess9,d0\n   trap    #0\n"
        "_spriterr:   move.l  #_mess10,d0\n   trap    #0\n"
        "   .text\n");

BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Number of BitPlanes for Resolution");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialised CopList");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialised Palette");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialised Sprite");
BLITZ_ERRCHK_MESSAGE(mess5, "Custom Copper String Must Be Multiple Of 4 Bytes Silly!");
BLITZ_ERRCHK_MESSAGE(mess6, "Not Enough Custom Copper Space");
BLITZ_ERRCHK_MESSAGE(mess7, "Need AGA palette mode ($10000) for this many colors");
BLITZ_ERRCHK_MESSAGE(mess8, "CustomChunky must start at ypos greater than res");
BLITZ_ERRCHK_MESSAGE(mess9, "Not Enough Custom Cops");
BLITZ_ERRCHK_MESSAGE(mess10, "Not Enough Sprite Channels");

/* _checkcoplist: TST.l !m{size}:BEQ uniniterr */
__asm__("   .section .blitzlib.errchk,\"ax\"\n   .even\n"
        "   .globl  __checkcoplist\n"
        "__checkcoplist:\n"
        "   tst.l   (a3)\n"
        "   beq.w   _uniniterr\n"
        "   rts\n"
        "   .text\n");
