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
 *   - CustomChunky with a split position copies 16 bytes from cchunky
 *     to cchunky2: the last word (pad0, 0) overwrites spri, so a later
 *     DisplaySprite on hires/superhires sprites positions by a wrong x
 *     handle. (#949)
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
#include <stddef.h>

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

/* the CopList object, NEWTYPE .mycop (128 bytes, 2^7) */
struct mycop {
    ULONG size;                 /* +0 0 = not initialised */
    ULONG coppos;               /* +4 the list (chip memory) */
    ULONG colors;               /* +8 pointers into the list */
    ULONG sprites;              /* +12 */
    ULONG bpcons;               /* +16 */
    ULONG bplanes;              /* +20 */
    ULONG dot;                  /* +24 */
    ULONG customs;              /* +28 */
    ULONG dob;                  /* +32 */
    UWORD bot;                  /* +36 */
    UWORD numbp;                /* +38 */
    UWORD colpokes;             /* +40 */
    UWORD fetchwid;             /* +42 */
    UWORD xand;                 /* +44 */
    UWORD xshift;               /* +46 */
    UWORD ypos;                 /* +48 */
    UWORD height;               /* +50 */
    UWORD res;                  /* +52 */
    UWORD numsprites;           /* +54 */
    UWORD numcols;              /* +56 */
    UWORD numcustoms;           /* +58 */
    UWORD cpline;               /* +60 */
    ULONG x0;                   /* +62 .q */
    ULONG x1;                   /* +66 .q */
    UWORD bplcon3;              /* +70 */
    UWORD aga;                  /* +72 $8000 24 bit, fetch $00-$c0 in the low byte */
    UWORD resshift;             /* +74 */
    UWORD setup;                /* +76 */
    UWORD cblow;                /* +78 */
    UWORD sfetch;               /* +80 */
    UWORD spres;                /* +82 */
    UWORD spif;                 /* +84 */
    UWORD spwid;                /* +86 */
    UWORD sspwid;               /* +88 */
    UWORD ham8;                 /* +90 */
    ULONG csprites;             /* +92 */
    ULONG cchunky;              /* +96 location of the copper chunky */
    UWORD chy, chw, chh, chr, chc, pad0;        /* +100 ypos width height res mod */
    ULONG cchunky2;             /* +112 the same for the split part */
    UWORD chy2, chw2, chh2, chr2, chc2;         /* +116 */
    UWORD spri;                 /* +126 the sprite resolution shift (#949) */
} BL_PACKED;
static_assert(sizeof(struct mycop) == 128);
static_assert(offsetof(struct mycop, x0) == 62 && offsetof(struct mycop, cchunky) == 96);
static_assert(offsetof(struct mycop, chy2) == 116 && offsetof(struct mycop, spri) == 126);

/* member m of the coplist at the UBYTE pointer p (the original's
   !m{m}(a3): the code keeps the address arithmetic of the pointer) */
#define CMOFF(m) ((int)offsetof(struct mycop, m))
#define CM(p, m) (*(__typeof__(((struct mycop *)0)->m) *)((UBYTE *)(p) + CMOFF(m)))

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
#define OVERRUN(cop, ins) do {                                              \
        if ((LONG)(ULONG)(ins) > (LONG)CM(cop, dob))                       \
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
#define T(maxbp, fetchwid, xmask, xrshift, adjwid, adjstop, fmode,          \
          diwstrt, diwstop, ddfstrt, ddfstop, bplcon0)                      \
    { maxbp, fetchwid, xmask, xrshift, adjwid, adjstop, 0x1fc, fmode,      \
      0x8e, diwstrt, 0x90, diwstop, 0x92, ddfstrt, 0x94, ddfstop,           \
      0x100, bplcon0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
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
static ULONG *putwait(ULONG *ins, UWORD line, UWORD prev, UBYTE h)
{
    UBYTE *b;

    *ins++ = ((line & 0x100) && !(prev & 0x100)) ? WAITBH : NOWCOP;
    b = (UBYTE *)ins;
    b[0] = (UBYTE)line;
    b[1] = h;
    ((UWORD *)ins)[1] = 0xfffe;
    return ins + 1;
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
static void BLITZ_ROUTINE _doadjust_c(UWORD fetchwid BL_REG(d1), UWORD ddfstrt BL_REG(d2),
                                      UWORD ddfstop BL_REG(d3), UWORD diwstrt BL_REG(d4),
                                      UWORD diwstop BL_REG(d5))
{
    UBYTE *cop = blitz_string;
    UBYTE *ctrl = (UBYTE *)CM(cop, coppos) + 8;

    CM(cop, fetchwid) += fetchwid;
    W(ctrl, 14) += ddfstrt;
    W(ctrl, 18) += ddfstop;
    W(ctrl, 6) += diwstrt;
    W(ctrl, 10) += diwstop;
}
BLITZ_THUNK(_doadjust, _doadjust_c, "d0-d1/a0");

/* DisplayControls */
static void BLITZ_ROUTINE _dobplcons_c(UWORD bplcon2 BL_REG(d1), UWORD bplcon3 BL_REG(d2),
                                       UWORD bplcon4 BL_REG(d3))
{
    UBYTE *cop = blitz_string;
    ULONG *moves = (ULONG *)CM(cop, bpcons);
    ULONG move2 = moves[0], move3 = moves[1], move4 = moves[2];

    BL_SETW(move2, (UWORD)move2 ^ bplcon2);
    BL_SETW(move3, (UWORD)move3 ^ bplcon3);
    BL_SETW(move4, (UWORD)move4 ^ bplcon4);
    CM(cop, bplcon3) ^= bplcon3;
    moves[0] = move2;
    moves[1] = move3;
    moves[2] = move4;
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
    UBYTE *cop = blitz_string;
    struct bl_regs r;

    if (!CM(cop, size))
        return;
    r.d0 = CM(cop, size);
    r.d1 = 0;
    r.a0 = 0;
    r.a1 = CM(cop, coppos);
    BLITZ_ALIBJSR(TOK_globfree, r);
    CM(cop, size) = 0;
    CM(cop, cpline) = 0;
}

/* _initcoplist: d1 = ypos, d2 = height, d3 = type, d4 = sprites, d5 =
   colours, -2(a2) = numcustoms, a0 = spriteslib's value, d6 = used
   offset, a3 = coplist */
static void initcoplist(ULONG ypos, ULONG height, ULONG type, ULONG sprites, ULONG colours,
                        const UWORD *args, const UWORD *sprmode, WORD used)
{
    UBYTE *cop = blitz_string;
    ULONG customs, *ins;
    UWORD spmode;                               /* d0: the sprite mode */
    UWORD spoff;                                /* d0: its table offset */
    ULONG aga;                                  /* d0: the AGA flags */
    UWORD banks;                                /* d7: the AGA bank moves */
    UWORD nbp;                                  /* d3: the bitplanes */
    UWORD bpmoves;                              /* d3: their moves */
    ULONG size;                                 /* d0: the list size */
    UWORD res;                                  /* d0: the resolution */
    ULONG bpmove;                               /* d0: MOVE BPLxPT */
    ULONG dotoff;                               /* d0: the dot offset */
    ULONG nop;                                  /* d7: COPNOP */
    const UWORD *mode;
    UWORD i;

    *(UBYTE **)(blitz_globals + used) = cop;    /* make used */
    spmode = *sprmode;
    CM(cop, spif) = spmode;
    CM(cop, spif) += 2;
    spoff = (UWORD)(spmode + spmode);
    CM(cop, sfetch) = *(const UWORD *)((const UBYTE *)d.slup + (WORD)spoff);
    CM(cop, spres) = *(const UWORD *)((const UBYTE *)d.shup + (WORD)spoff);
    CM(cop, spwid) = *(const UWORD *)((const UBYTE *)d.swup + (WORD)spoff);
    CM(cop, ham8) = 0;
    if ((UBYTE)(type & 0x8f) == 0x88)
        CM(cop, ham8) = 0xffff;
    CM(cop, ypos) = (UWORD)ypos;               /* MOVEM d1-d5 */
    CM(cop, height) = (UWORD)height;
    CM(cop, res) = (UWORD)type;
    CM(cop, numsprites) = (UWORD)sprites;
    CM(cop, numcols) = (UWORD)colours;
    CM(cop, bot) = (UWORD)ypos;
    _freecoplist();
    /* calccustoms */
    customs = *--args;
    CM(cop, numcustoms) = (UWORD)customs;
    if ((WORD)customs < 0) {
        customs = (UWORD)(-(WORD)customs + 1);
        customs = (UWORD)(customs << 2);
        CM(cop, cpline) = (UWORD)customs;
        customs >>= 2;
        BL_SETW(height, (UWORD)height - 1);
        customs = mulu16((UWORD)height, (UWORD)customs);
        CM(cop, numcustoms) = (UWORD)customs;
    }
    /* checkaga */
    aga = type;
    aga = lsr_w(aga, 5);
    aga >>= 1;
    BL_SETW(aga, (UWORD)aga & 0x80c0);
    CM(cop, aga) = (UWORD)aga;
    if ((WORD)aga < 0 && (UWORD)colours) {
        banks = (UWORD)((UWORD)((UWORD)colours - 1) >> 5) + 1;
        banks = (UWORD)(banks + banks);
        BL_SETW(colours, (UWORD)colours + (UWORD)colours + banks);
    }
    /* calcsize */
    CM(cop, colpokes) = (UWORD)colours;
    nbp = (UWORD)type & 15;
    CM(cop, numbp) = nbp;
    bpmoves = nbp + nbp;
    BL_SETW(sprites, (UWORD)sprites << 2);
    size = (UWORD)(bpmoves + (UWORD)sprites + (UWORD)colours + (UWORD)customs + 44);
    size <<= 2;
    CM(cop, size) = size;
    ins = (ULONG *)globalloc(size, 65538);
    CM(cop, coppos) = (ULONG)ins;
    res = CM(cop, res);
    /* firstwait */
    nop = COPNOP;
    *ins++ = nop;
    *ins++ = nop;
    /* ctrl1 */
    {
        UWORD modeoff = (res & 0x300) | B(cop, CMOFF(aga) + 1); /* d1w */
        UWORD resindex = modeoff >> 8;          /* d1w */
        ULONG ctrl[6];
        int k;

        mode = (const UWORD *)((const UBYTE *)table + (WORD)modeoff);
        CM(cop, resshift) = (UWORD)(2 - resindex);
        for (k = 0; k < 6; k++)
            ctrl[k] = ((const ULONG *)(mode + 6))[k];
        BL_SETW(ctrl[0], (UWORD)ctrl[0] | CM(cop, sfetch));
        BL_SETW(ctrl[5], (UWORD)ctrl[5] | d.genlock);
        for (k = 0; k < 6; k++)
            ins[k] = ctrl[k];
        CM(cop, fetchwid) = mode[1];
        CM(cop, xand) = mode[2];
        CM(cop, xshift) = mode[3];
        if (res & (1 << 4)) {
            CM(cop, fetchwid) += mode[4];
            W((UBYTE *)ins, 14) -= mode[5];
        }
        if ((WORD)CM(cop, numbp) > (WORD)mode[0])
            GOTO(maxbperr);
        i = bpb[CM(cop, numbp)];
        if (res & (1 << 7))
            i |= 1 << 11;
        if (res & (1 << 5))
            i |= 1 << 10;
        W((UBYTE *)ins, 22) |= i;
    }
    ins += 6;
    nop = COPNOP;
    /* cols */
    CM(cop, colors) = (ULONG)ins;
    i = CM(cop, colpokes);
    if (i) {
        i--;
        do
            *ins++ = nop;
        while ((WORD)--i != -1);
    }
    /* sprites */
    CM(cop, sprites) = (ULONG)ins;
    i = CM(cop, numsprites);
    if (i) {
        ULONG movpos = spp[0], movctl = spp[1], movpth = spp[2], movptl = spp[3];

        i--;
        do {
            ins[0] = movpos;
            ins[1] = movctl;
            ins[2] = movpth;
            ins[3] = movptl;
            ins += 4;
            movpos += 0x80000;
            movctl += 0x80000;
            movpth += 0x40000;
            movptl += 0x40000;
        } while ((WORD)--i != -1);
    }
    /* ctrl2 */
    CM(cop, bpcons) = (ULONG)ins;
    {
        ULONG mvcon2 = bcp[0], mvcon3 = bcp[1];
        UWORD sprbits;

        if (res & (1 << 6))
            mvcon2 &= ~(1UL << 9);
        sprbits = (res & 0xc00) >> 4;
        BL_SETW(mvcon3, (UWORD)mvcon3 | sprbits);
        sprbits >>= 6;
        if (sprbits)
            sprbits -= 1;
        CM(cop, spri) = sprbits;
        ins[0] = mvcon2;
        ins[1] = mvcon3;
        ins[2] = bcp[2];
        ins[3] = bcp[3];
        ins[4] = bcp[4];
        ins[5] = bcp[5];
        ins += 6;
        CM(cop, bplcon3) = (UWORD)mvcon3;
        CM(cop, sspwid) = (UWORD)lsr_w(CM(cop, spwid), sprbits);
    }
    /* bps */
    CM(cop, bplanes) = (ULONG)ins;
    bpmove = 0x00e00000;
    i = CM(cop, numbp);
    if (i) {
        i = i + i - 1;
        do {
            *ins++ = bpmove;
            bpmove += 0x20000;
        } while ((WORD)--i != -1);
    }
    /* dot */
    dotoff = (ULONG)ins - CM(cop, coppos);
    CM(cop, setup) = (UWORD)((UWORD)bl_divu_w(dotoff, 222) + 3);
    CM(cop, dot) = (ULONG)ins;
    *ins++ = nop;
    *ins++ = nop;
    *ins++ = DMAON;
    /* customs */
    CM(cop, customs) = (ULONG)ins;
    i = CM(cop, cpline);
    if (!i) {
        UWORD n = CM(cop, numcustoms);

        if (n) {
            n--;
            do
                *ins++ = nop;
            while ((WORD)--n != -1);
        }
    } else {
        /* lino */
        UWORD per = (i >> 2) - 2, y = CM(cop, ypos) - 1, n = CM(cop, height) - 2;

        do {
            UBYTE *b = (UBYTE *)ins;
            UWORD k = per;

            b[0] = (UBYTE)y;
            b[1] = 0xe1;
            ((UWORD *)ins)[1] = 0xfffe;
            ins++;
            do
                *ins++ = nop;
            while ((WORD)--k != -1);
            y++;
        } while ((WORD)--n != -1);
        CM(cop, bot) = y - 1;
    }
    /* dob */
    CM(cop, dob) = (ULONG)ins;
    *ins++ = nop;
    *ins++ = nop;
    *ins++ = DMAOFF;
}

/* InitCopList CopList#,ypos,height,type,sprites,colors,numcustoms */
static void BLITZ_ROUTINE _initcoplist_c(ULONG ypos BL_REG(d1), ULONG height BL_REG(d2),
                                         ULONG type BL_REG(d3), ULONG sprites BL_REG(d4),
                                         ULONG colours BL_REG(d5), ULONG used BL_REG(d6),
                                         const UWORD *args BL_REG(a2),
                                         const UWORD *sprmode BL_REG(a0))
{
    initcoplist(ypos, height, type, sprites, colours, args, sprmode, (WORD)used);
}

/* InitCopList CopList#,type (quick version): d1 arrives as the type and
   becomes the ypos */
static void BLITZ_ROUTINE _initcoplist2_c(ULONG type BL_REG(d1), ULONG height BL_REG(d2),
                                          ULONG used BL_REG(d6), const UWORD *sprmode BL_REG(a0))
{
    ULONG ypos = type, colours;

    BL_SETW(ypos, 44);
    BL_SETW(height, 256);
    colours = colup[(UWORD)type & 15];
    if (type & (1 << 5))
        colours >>= 1;
    if (type & (1 << 6))
        colours = 32;
    initcoplist(ypos, height, type, 8, colours, colup + 1, sprmode, (WORD)used);
}
BLITZ_THUNK(_initcoplist, _initcoplist_c, "a2");
BLITZ_THUNK(_initcoplist2, _initcoplist2_c, "a2");

/* ---------------------------------------- CreateDisplay (Blitz mode only) */

static void BLITZ_ROUTINE _createdisplay_c(UWORD count BL_REG(d7), const UWORD *args BL_REG(a2),
                                           UBYTE *coplists BL_REG(a0))
{
    UBYTE *cop;
    ULONG *ins;
    UWORD line, height;

    if ((WORD)--count < 0)
        return;
    cop = coplists + (WORD)(UWORD)(*--args << 7);
    blitz_string = cop;
    if (!CM(cop, size))
        GOTO(uniniterr);
    cd.topcop = CM(cop, coppos);
    cd.lastwait = 0;
    for (;;) {
        /* nxlink */
        line = CM(cop, ypos);
        height = CM(cop, height);
        line -= CM(cop, setup);
        ins = putwait((ULONG *)CM(cop, coppos), line, cd.lastwait, 0xe1);
        cd.lastwait = line;
        line += CM(cop, setup);
        ins = putwait((ULONG *)CM(cop, dot), line, cd.lastwait, 0x01);
        line += height;
        ins = putwait((ULONG *)CM(cop, dob), line, CM(cop, bot), 0x01);
        cd.lastwait = line;
        /* dolink */
        ins++;
        if ((WORD)--count == -1)
            break;
        /* another */
        cop = coplists + (WORD)(UWORD)(*--args << 7);
        blitz_string = cop;
        if (!CM(cop, size))
            GOTO(uniniterr);
        {
            ULONG addr = CM(cop, coppos);
            UWORD *move = (UWORD *)ins;

            move[0] = 0x86;
            move[1] = (UWORD)addr;
            move[2] = 0x84;
            move[3] = (UWORD)(addr >> 16);
            ins += 2;
            *ins = STROBE1;
        }
    }
    *ins = WAITEND;
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
static void displaybmap2(UBYTE *cop, const UBYTE *bm, ULONG xoff, ULONG yoff)
{
    UWORD *bplptr;
    ULONG planes, modulo, fine, xmask, shift;
    ULONG fetchwid;                             /* d5 */
    ULONG ham8;                                 /* d5: the HAM8 flag */

    CM(cop, x0) = xoff;
    CM(cop, x1) = xoff;
    shift = CM(cop, resshift);
    xoff = BL_SWAP(asl_l(xoff, shift));
    bplptr = (UWORD *)CM(cop, bplanes);
    planes = CM(cop, numbp);
    if (!planes)
        return;
    planes--;
    fetchwid = BL_EXTL(CM(cop, fetchwid));
    xmask = BL_EXTL(CM(cop, xand));
    shift = BL_EXTL(CM(cop, xshift));
    modulo = (UWORD)(W(bm, 0) - (UWORD)fetchwid);
    bplptr[-3] = (UWORD)modulo;
    bplptr[-1] = (UWORD)modulo;
    fine = (UWORD)((UWORD)xoff & (UWORD)xmask);
    ham8 = CM(cop, ham8);
    fine = (UWORD)(fine + fine);
    blitz_string = (UBYTE *)bp1;                /* LEA bp1(pc),a3 */
    bplptr[-5] = BP1(fine);
    BL_SETW(xmask, ~(UWORD)xmask);
    BL_SETW(xoff, (UWORD)xoff & (UWORD)xmask);
    xoff = BL_EXTL(xoff);
    xoff = asr_w(xoff, shift);
    yoff = mulu16(W(bm, 0), (UWORD)yoff);
    xoff += yoff;
    bm += 8;
    bplptr += 1;
    if ((UWORD)ham8) {
        bplptr = setptr(bplptr, L(bm, 24) + xoff);
        bplptr = setptr(bplptr, L(bm, 28) + xoff);
        BL_SETW(planes, (UWORD)planes - 2);
    }
    do {
        bplptr = setptr(bplptr, *(const ULONG *)bm + xoff);
        bm += 4;
    } while ((WORD)--planes != -1);
}

static void BLITZ_ROUTINE _displaybmap2_c(const UBYTE *bm BL_REG(a0), ULONG x BL_REG(d2),
                                          ULONG y BL_REG(d3))
{
    displaybmap2(blitz_string, bm, x, y);
}

/* DisplayBitMap CopList#,bmap - d2/d3 are the caller's registers */
static void BLITZ_ROUTINE _displaybmap_c(const UBYTE *bm BL_REG(a0), ULONG staled2 BL_REG(d2),
                                         ULONG staled3 BL_REG(d3))
{
    UBYTE *cop = blitz_string;
    UWORD *bplptr;
    UWORD planes, modulo;
    const UWORD *plane;

    if (B(cop, CMOFF(res) + 1) & (1 << 4)) {
        displaybmap2(cop, bm, 0, 0);
        return;
    }
    CM(cop, x0) = staled2;                     /* BUG(orig) (#411): stale d2/d3 */
    CM(cop, x1) = staled3;
    bplptr = (UWORD *)CM(cop, bplanes);
    planes = CM(cop, numbp);
    if (!planes)
        return;
    planes--;
    modulo = W(bm, 0) - CM(cop, fetchwid);
    bplptr[-3] = modulo;
    bplptr[-1] = modulo;
    plane = (const UWORD *)(bm + 8);
    bplptr += 1;
    if (CM(cop, ham8)) {
        bplptr[0] = plane[12];
        bplptr[2] = plane[13];
        bplptr[4] = plane[14];
        bplptr[6] = plane[15];
        bplptr += 8;
        planes -= 2;
    }
    do {
        bplptr[0] = *plane++;
        bplptr[2] = *plane++;
        bplptr += 4;
    } while ((WORD)--planes != -1);
}
BLITZ_THUNK(_displaybmap, _displaybmap_c, "a1");
BLITZ_THUNK(_displaybmap2, _displaybmap2_c, "a1");

/* scludge: two bitmaps, d2/d5 = x (quick), d3/d6 = y */
static void displaydbmap2(UBYTE *cop, const UBYTE *bm1, const UBYTE *bm2,
                          ULONG xoff1, ULONG yoff1, ULONG xoff2, ULONG yoff2)
{
    UWORD *bplptr;
    ULONG fetchwid, bpr, xmask, shift;
    UWORD n;

    CM(cop, x0) = xoff1;
    CM(cop, x1) = xoff2;
    shift = CM(cop, resshift);
    xoff1 = BL_SWAP(asl_l(xoff1, shift));
    xoff2 = BL_SWAP(asl_l(xoff2, shift));
    bplptr = (UWORD *)CM(cop, bplanes);
    n = CM(cop, numbp);
    if (!n)
        return;
    fetchwid = BL_EXTL(CM(cop, fetchwid));
    xmask = BL_EXTL(CM(cop, xand));
    shift = BL_EXTL(CM(cop, xshift));
    blitz_string = (UBYTE *)bp1;                /* LEA bp1(pc),a3 */
    bpr = W(bm1, 0);
    yoff1 = mulu16((UWORD)bpr, (UWORD)yoff1);
    bplptr[-3] = (UWORD)(bpr - fetchwid);
    bpr = W(bm2, 0);
    yoff2 = mulu16((UWORD)bpr, (UWORD)yoff2);
    bplptr[-1] = (UWORD)(bpr - fetchwid);
    {
        UWORD scroll1 = BP1((UWORD)((UWORD)xoff1 & (UWORD)xmask) * 2);
        UWORD scroll2 = BP1((UWORD)((UWORD)xoff2 & (UWORD)xmask) * 2);

        bplptr[-5] = (scroll2 & 0xf0f0) | (scroll1 & 0x0f0f);
    }
    BL_SETW(xmask, ~(UWORD)xmask);
    BL_SETW(xoff1, (UWORD)xoff1 & (UWORD)xmask);
    xoff1 = asr_w(BL_EXTL(xoff1), shift) + yoff1;
    BL_SETW(xoff2, (UWORD)xoff2 & (UWORD)xmask);
    xoff2 = asr_w(BL_EXTL(xoff2), shift) + yoff2;
    n--;
    bm1 += 8;
    bm2 += 8;
    bplptr += 1;
    for (;;) {
        bplptr = setptr(bplptr, *(const ULONG *)bm1 + xoff1);
        bm1 += 4;
        if ((WORD)--n == -1)
            return;
        bplptr = setptr(bplptr, *(const ULONG *)bm2 + xoff2);
        bm2 += 4;
        if ((WORD)--n == -1)
            return;
    }
}

static void BLITZ_ROUTINE _displaydbmap2_c(const UBYTE *bm1 BL_REG(a0), const UBYTE *bm2 BL_REG(a1),
                                           ULONG x1 BL_REG(d2), ULONG y1 BL_REG(d3),
                                           ULONG x2 BL_REG(d5), const UWORD *args BL_REG(a2))
{
    displaydbmap2(blitz_string, bm1, bm2, x1, y1, x2, args[-1]);
}

static void BLITZ_ROUTINE _displaydbmap_c(const UBYTE *bm1 BL_REG(a0), const UBYTE *bm2 BL_REG(a1))
{
    UBYTE *cop = blitz_string;
    UWORD *bplptr;
    UWORD planes, modulo;

    if (B(cop, CMOFF(res) + 1) & (1 << 4)) {
        displaydbmap2(cop, bm1, bm2, 0, 0, 0, 0);
        return;
    }
    CM(cop, x0) = 0;
    CM(cop, x1) = 0;
    bplptr = (UWORD *)CM(cop, bplanes);
    planes = CM(cop, numbp);
    if (!planes)
        return;
    planes--;
    modulo = W(bm1, 0) - CM(cop, fetchwid);
    bplptr[-3] = modulo;
    modulo = W(bm2, 0) - CM(cop, fetchwid);
    bplptr[-1] = modulo;
    bm1 += 8;
    bm2 += 8;
    bplptr += 1;
    for (;;) {
        bplptr = setptr(bplptr, *(const ULONG *)bm1);
        bm1 += 4;
        if ((WORD)--planes == -1)
            return;
        bplptr = setptr(bplptr, *(const ULONG *)bm2);
        bm2 += 4;
        if ((WORD)--planes == -1)
            return;
    }
}
BLITZ_THUNK(_displaydbmap, _displaydbmap_c, "a2");
BLITZ_THUNK(_displaydbmap2, _displaydbmap2_c, "a2");

/* ------------------------------------------------------ sprite routines */

static void spriteoff(UBYTE *cop, const UBYTE *spr, UWORD chan)
{
    UWORD count = W(spr, 6) - 1;
    UBYTE *sprmove = (UBYTE *)CM(cop, sprites) + (WORD)(UWORD)(chan << 4);

    do {
        W(sprmove, 2) = 0;
        W(sprmove, 6) = 0;
        W(sprmove, 10) = (UWORD)(d.mysprite >> 16);
        W(sprmove, 14) = (UWORD)d.mysprite;
        sprmove += 16;
        if ((WORD)W(spr, 8) < 0) {
            W(sprmove, 2) = 0;
            W(sprmove, 6) = 0;
            W(sprmove, 10) = (UWORD)(d.mysprite >> 16);
            W(sprmove, 14) = (UWORD)d.mysprite;
            sprmove += 16;
            count--;
        }
    } while ((WORD)--count != -1);
}

/* DisplaySprite CopList#,Sprite#,X,Y,Channel: a1 = sprite, d2 = x
   (quick), d3 = y, d4 = channel.  Every register is reused; the word
   loads into d2/d3 keep the upper word of the caller's register (BL_SETW
   below) */
static void BLITZ_ROUTINE _displaysprite_c(const UBYTE *spr BL_REG(a1), ULONG x BL_REG(d2),
                                           ULONG y BL_REG(d3), ULONG channel BL_REG(d4))
{
    UBYTE *cop = blitz_string, *sprmove;
    ULONG vpos;                                 /* d2: y, then the start line */
    ULONG vstop;                                /* d2: the stop line */
    ULONG count;                                /* d2: the sprite count */
    ULONG chan;                                 /* d3: the channel */
    ULONG xhandle;                              /* d4 */
    ULONG skip;                                 /* d4: the data skip */
    ULONG chanend;                              /* d5: channel + sprite width */
    ULONG yhandle;                              /* d5 */
    ULONG height;                               /* d5 */
    ULONG lines;                                /* d0: the visible lines */
    ULONG data;                                 /* d0: the data address */
    ULONG hpos;                                 /* d1: the x position */
    ULONG step;                                 /* d1: the data step */
    ULONG nspr;                                 /* d6: the sprite channels */
    ULONG posword, ctlword;                     /* d6 d7: SPRxPOS, SPRxCTL */

    if (!L(spr, 0))
        GOTO(nosprierr);
    chanend = (UWORD)channel;
    nspr = CM(cop, numsprites);
    if (CM(cop, csprites))
        nspr = (UWORD)(nspr + nspr);
    chanend = (UWORD)(chanend + W(spr, 6));
    if ((WORD)((UWORD)nspr - (UWORD)chanend) < 0) /* CMP d5,d6:BMI */
        GOTO(spriterr);
    hpos = x;
    vpos = x;
    BL_SETW(vpos, (UWORD)y);
    chan = y;
    BL_SETW(chan, (UWORD)channel);
    xhandle = BL_EXTL(W(spr, 12));
    yhandle = BL_EXTL(W(spr, 14));
    xhandle = asr_w(asl_w(xhandle, 2), CM(cop, spri));
    hpos = BL_SWAP(hpos);
    hpos = rol_l(hpos, CM(cop, resshift));
    BL_SETW(hpos, (UWORD)hpos + 512);
    BL_SETW(hpos, (UWORD)hpos - (UWORD)xhandle);
    BL_SETW(vpos, (UWORD)vpos - (UWORD)yhandle);
    {
        WORD bottom = (WORD)(CM(cop, height) + 1);

        if (bottom <= (WORD)vpos) {             /* SUB d2,d0:BLE */
            spriteoff(cop, spr, (UWORD)chan);
            return;
        }
        lines = (UWORD)(bottom - (WORD)vpos);
    }
    skip = 0;
    height = W(spr, 4);
    if ((WORD)vpos < 0) {                       /* off the top */
        if ((LONG)(WORD)height + (WORD)vpos <= 0) { /* ADD d2,d5:BLE */
            spriteoff(cop, spr, (UWORD)chan);
            return;
        }
        height = (UWORD)((UWORD)height + (UWORD)vpos);
        skip = vpos;                            /* EXG d2,d4 */
        vpos = 0;
        BL_SETW(skip, -(WORD)skip);
        skip = asl_l(BL_EXTL(skip), CM(cop, spif));
    }
    if ((WORD)((UWORD)lines - (UWORD)height) < 0) /* CMP d5,d0:BPL */
        height = (UWORD)lines;
    BL_SETW(vpos, (UWORD)vpos + CM(cop, ypos));
    ctlword = 0;
    posword = (UWORD)vpos;
    if (posword & 0x100)                        /* ASL #8,d6: carry */
        ctlword |= 1 << 2;
    posword = (UWORD)(posword << 8);
    if (!d.agamac)
        BL_SETW(hpos, (UWORD)hpos & 0xfffc);
    if (hpos & 1)
        ctlword |= 1 << 3;
    BL_SETW(hpos, (UWORD)hpos >> 1);
    if (hpos & 1)
        ctlword |= 1 << 4;
    BL_SETW(hpos, (UWORD)hpos >> 1);
    if (hpos & 1)
        ctlword |= 1 << 0;
    BL_SETW(hpos, (UWORD)hpos >> 1);
    if ((WORD)hpos < 0)
        hpos = 0;
    if ((WORD)((UWORD)hpos - 256) >= 0)         /* CMP #256,d1:BMI */
        BL_SETW(hpos, 255);
    posword = (posword & 0xff00) | (UBYTE)hpos;
    vstop = vpos;
    BL_SETW(vstop, (UWORD)vpos + (UWORD)height - 1);
    if (vstop & 0x100)                          /* LSL #8,d2: carry */
        ctlword |= 1 << 1;
    BL_SETW(vstop, (UWORD)vstop << 8);
    ctlword |= (UWORD)vstop;
    if ((WORD)W(spr, 8) < 0)
        ctlword |= 1 << 7;
    sprmove = (UBYTE *)CM(cop, sprites);
    if ((UWORD)chan >= CM(cop, numsprites)) {
        BL_SETW(chan, (UWORD)chan - CM(cop, numsprites));
        sprmove = (UBYTE *)CM(cop, csprites);
    }
    sprmove += (WORD)(UWORD)((UWORD)chan << 4);
    data = L(spr, 0) + skip;
    step = BL_EXTL(W(spr, 10));
    count = (UWORD)(W(spr, 6) - 1);
    do {
        W(sprmove, 2) = (UWORD)posword;
        W(sprmove, 6) = (UWORD)ctlword;
        W(sprmove, 14) = (UWORD)data;
        W(sprmove, 10) = (UWORD)(data >> 16);
        data += step;
        sprmove += 16;
        if ((WORD)W(spr, 8) < 0) {              /* attached */
            W(sprmove, 2) = (UWORD)posword;
            W(sprmove, 6) = (UWORD)ctlword;
            W(sprmove, 14) = (UWORD)data;
            W(sprmove, 10) = (UWORD)(data >> 16);
            data += step;
            sprmove += 16;
            count = (UWORD)(count - 1);
        }
        posword = (UWORD)(posword + CM(cop, sspwid));
    } while ((WORD)(count = (UWORD)(count - 1)) != -1);
}
BLITZ_THUNK(_displaysprite, _displaysprite_c, "a1");

/* ------------------------------------------------ palette routines */

/* simple: ECS colour moves from the palette at a0 */
static void simple(UBYTE *cop, const UBYTE *pal, ULONG *ins, UWORD count)
{
    ULONG move = 0x01800000;                    /* MOVE #$180,d0:SWAP d0 (low
                                                   word replaced below) */

    if ((WORD)count > 32)
        GOTO(agapalerr);
    count--;
    do {
        BL_SETW(move, W(pal, 0) >> 4);
        move = (move & 0xffffff00) | B(pal, 4);
        BL_SETW(move, (UWORD)move << 4);
        move = (move & 0xffffff00) | B(pal, 8);
        BL_SETW(move, (UWORD)move >> 4);
        *ins++ = move;
        move += 0x20000;
        pal += 12;
    } while ((WORD)--count != -1);
    OVERRUN(cop, ins);
}

/* complex2: AGA, banks of 32 colours, high and low nibbles */
static void complex2(UBYTE *cop, const UBYTE *pal, ULONG *ins, ULONG bankmove, UWORD count)
{
    ULONG move;

    count--;
    do {
        UWORD last = 31, left;
        const UBYTE *bankpal = pal;

        if ((WORD)(count - last) < 0)           /* CMP d6,d7:BPL */
            last = count;
        *ins++ = bankmove;
        left = last;
        move = 0x01800000;                      /* MOVE #$180,d1:SWAP d1 */
        do {
            BL_SETW(move, W(pal, 0) >> 4);
            move = (move & 0xffffff00) | B(pal, 4);
            BL_SETW(move, (UWORD)move << 4);
            move = (move & 0xffffff00) | B(pal, 8);
            BL_SETW(move, (UWORD)move >> 4);
            *ins++ = move;
            move += 0x20000;
            pal += 12;
        } while ((WORD)--left != -1);
        bankmove ^= 1 << 9;
        *ins++ = bankmove;
        left = last;
        move = 0x01800000;
        pal = bankpal;
        do {
            UBYTE b = (UBYTE)(B(pal, 4) << 4);

            BL_SETW(move, W(pal, 0));
            move = (move & 0xffffff00) | (UBYTE)((B(pal, 8) & 15) | b);
            *ins++ = move;
            move += 0x20000;
            pal += 12;
        } while ((WORD)--left != -1);
        bankmove ^= 1 << 9;
        BL_SETW(bankmove, (UWORD)bankmove + 0x2000);
        count -= 32;
    } while ((WORD)count >= 0);
    OVERRUN(cop, ins);
}

/* the palette data: (a0) -> +4, entries of 12 bytes */
static const UBYTE *paldata(const UBYTE *palobj)
{
    ULONG p = L(palobj, 0);

    if (!p)
        GOTO(nopalerr);
    return (const UBYTE *)p + 4;
}

static void BLITZ_ROUTINE _displaypalette_c(const UBYTE *pal BL_REG(a0), ULONG palnum BL_REG(d1))
{
    UBYTE *cop = blitz_string;
    ULONG *ins;
    UWORD count;

    pal = paldata(pal);
    ins = (ULONG *)CM(cop, colors);
    count = CM(cop, numcols);
    if (!count) {
        OVERRUN(cop, ins);
        return;
    }
    if ((WORD)CM(cop, aga) >= 0)
        simple(cop, pal, ins, count);
    else
        complex2(cop, pal, ins, 0x01060000 | CM(cop, bplcon3), count);
}

static void BLITZ_ROUTINE _displaypalette2_c(const UBYTE *pal BL_REG(a0), ULONG palnum BL_REG(d1),
                                             UWORD coloff BL_REG(d2))
{
    UBYTE *cop = blitz_string;
    ULONG *ins;
    UWORD count;

    pal = paldata(pal);
    pal += (WORD)(UWORD)mulu16(coloff, 12);
    ins = (ULONG *)CM(cop, colors);
    count = CM(cop, numcols);
    if (!count) {
        OVERRUN(cop, ins);
        return;
    }
    if ((WORD)CM(cop, aga) >= 0)
        simple(cop, pal, ins, count);
    else
        complex2(cop, pal, ins, 0x01060000 | CM(cop, bplcon3), count);
}

static void BLITZ_ROUTINE _displaypalette3_c(const UBYTE *pal BL_REG(a0), ULONG palnum BL_REG(d1),
                                             UWORD coloff BL_REG(d2), UWORD bankoff BL_REG(d3))
{
    UBYTE *cop = blitz_string;
    ULONG *ins;
    UWORD count;

    pal = paldata(pal);
    pal += (WORD)(UWORD)mulu16(coloff, 12);
    ins = (ULONG *)CM(cop, colors);
    count = CM(cop, numcols);
    if (!count) {
        OVERRUN(cop, ins);
        return;
    }
    if ((WORD)CM(cop, aga) >= 0)
        simple(cop, pal, ins, count);
    else
        complex2(cop, pal, ins,
                 0x01060000 | (UWORD)(CM(cop, bplcon3) + (UWORD)(bankoff << 13)), count);
}
BLITZ_THUNK(_displaypalette, _displaypalette_c, "a2");
BLITZ_THUNK(_displaypalette2, _displaypalette2_c, "a2");
BLITZ_THUNK(_displaypalette3, _displaypalette3_c, "a2");

/* CustomColors CopList#,CCOffset,YPos,Palette,startcol,numcols */
static void customcols(UWORD ccoffset, UWORD ypos, const UBYTE *pal, UWORD startcol, UWORD numcols)
{
    UBYTE *cop = blitz_string;
    ULONG *ins = (ULONG *)((UBYTE *)CM(cop, customs) + (WORD)(UWORD)(ccoffset << 2));
    UWORD y = ypos + CM(cop, ypos);
    ULONG bankmove;

    ins = putwait(ins, y, CM(cop, bot), 0xe1);
    CM(cop, bot) = y + 1;
    pal = paldata(pal);
    pal += (WORD)(UWORD)mulu16(startcol, 12);
    if (!numcols) {
        OVERRUN(cop, ins);
        return;
    }
    if ((WORD)CM(cop, aga) >= 0) {
        simple(cop, pal, ins, numcols);
        return;
    }
    bankmove = 0x01060000 | (UWORD)(CM(cop, bplcon3) | (UWORD)((startcol & 0xe0) << 8));
    complex2(cop, pal, ins, bankmove, numcols);
}

static void BLITZ_ROUTINE _customcols_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                        const UBYTE *pal BL_REG(a0), UWORD startcol BL_REG(d4),
                                        UWORD numcols BL_REG(d5))
{
    customcols(ccoffset, ypos, pal, startcol, numcols);
}

static void BLITZ_ROUTINE _customcols2_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                         const UBYTE *pal BL_REG(a0), UWORD startcol BL_REG(d4),
                                         UWORD numcols BL_REG(d5))
{
    customcols(ccoffset, ypos, pal, startcol, numcols);
    CM(blitz_string, bot) = 257;
}
BLITZ_THUNK(_customcols, _customcols_c, "a2");
BLITZ_THUNK(_customcols2, _customcols2_c, "a2");

/* ------------------------------------------------- custom copper routines */

/* CustomString CopList#,CCOffset,YPos,Copper$ */
static void BLITZ_ROUTINE _customstring_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                          const ULONG *s BL_REG(d3), const ULONG *lens BL_REG(a2))
{
    UBYTE *cop = blitz_string;
    ULONG *ins = (ULONG *)((UBYTE *)CM(cop, customs) + ((LONG)(WORD)ccoffset << 2));
    UWORD y = ypos + CM(cop, ypos);
    ULONG size = lens[-1];

    ins = putwait(ins, y, CM(cop, bot), 0xe1);
    CM(cop, bot) = y + 1;
    if (size & 3)
        GOTO(freakerr);
    size >>= 2;
    if (!size)
        GOTO(freakerr);
    {
        UWORD n = (UWORD)(size - 1);            /* SUBQ #1,d0:DBRA (words) */

        do
            *ins++ = *s++;
        while ((WORD)--n != -1);
    }
    OVERRUN(cop, ins);
}
BLITZ_THUNK(_customstring, _customstring_c, "a2");

/* DisplayDblScan CopList#,Mode[,copoffset] */
static void dblscan(UWORD mode, UWORD off)
{
    UBYTE *cop = blitz_string;
    UBYTE *ins;
    UWORD end = (UWORD)((off + 1) << 2), cpline = CM(cop, cpline), lines;
    WORD modeidx;
    ULONG dblmod1, dblmod2, mod1, mod2;

    ins = (UBYTE *)CM(cop, customs) + (WORD)end;
    end += 8;
    if ((WORD)end > (WORD)cpline)
        GOTO(linerr);
    lines = CM(cop, height) - 2;
    modeidx = (WORD)(mode - 1);
    if (modeidx < 0) {                                  /* clrdbl */
        do {
            L(ins, 0) = COPNOP;
            L(ins, 4) = COPNOP;
            ins += (WORD)cpline; OPAQUE(ins);
        } while ((WORD)--lines != -1);
        return;
    }
    {
        const ULONG *bplptr = (const ULONG *)CM(cop, bplanes);

        dblmod1 = bplptr[-2];
        dblmod2 = bplptr[-1];
    }
    mod1 = dblmod1;
    mod2 = dblmod2;
    BL_SETW(dblmod1, -(WORD)CM(cop, fetchwid));
    BL_SETW(dblmod2, (UWORD)dblmod1);
#define STORE() do {                                                    \
        L(ins, 0) = dblmod1; L(ins, 4) = dblmod2;                       \
        ins += (WORD)cpline; OPAQUE(ins);                               \
    } while (0)
#define SWAPMODS() do {                                                 \
        ULONG t_ = dblmod1; dblmod1 = mod1; mod1 = t_;                  \
        t_ = dblmod2; dblmod2 = mod2; mod2 = t_;                        \
    } while (0)
    if (!modeidx) {                                     /* nxline */
        do {
            STORE();
            SWAPMODS();
        } while ((WORD)--lines != -1);
        return;
    }
    /* dbldbl */
    dl.loopd = *(const UWORD *)((const UBYTE *)dl.dloop + (WORD)(UWORD)(modeidx + modeidx));
    for (;;) {
        UWORD k = dl.loopd;                             /* nux1 */

        for (;;) {
            STORE();                                    /* lbub */
            if ((WORD)--lines == -1)
                return;
            if ((WORD)--k == -1)                        /* nux2 */
                break;
        }
        STORE();
        SWAPMODS();
        if ((WORD)--lines == -1)
            return;
        STORE();                                        /* nux4 */
        SWAPMODS();
        if ((WORD)--lines == -1)
            return;
    }
#undef STORE
#undef SWAPMODS
}

static void BLITZ_ROUTINE _displaydblscan0_c(UWORD mode BL_REG(d1)) { dblscan(mode, 0); }
static void BLITZ_ROUTINE _displaydblscan_c(UWORD mode BL_REG(d1), UWORD off BL_REG(d2))
{
    dblscan(mode, off);
}
BLITZ_THUNK(_displaydblscan0, _displaydblscan0_c, "a0");
BLITZ_THUNK(_displaydblscan, _displaydblscan_c, "a0");

/* CustomSprites CopList#,CCOffset,YPos,NumSprites.  BUG(orig) (#324): d1 is
   CCOffset * 16 (see the declaration), the a1 the compiler loaded is not
   used: the offset is CCOffset * 64 bytes */
static void BLITZ_ROUTINE _customsprites_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                           UWORD count BL_REG(d3))
{
    UBYTE *cop = blitz_string;
    ULONG *ins = (ULONG *)((UBYTE *)CM(cop, customs) + (WORD)(UWORD)(ccoffset << 2));
    UWORD y = ypos + CM(cop, ypos), n;
    ULONG movpos = spp[0], movctl = spp[1], movpth = spp[2], movptl = spp[3];

    ins = putwait(ins, y, CM(cop, bot), 0xe1);
    CM(cop, bot) = y + 1;
    CM(cop, csprites) = (ULONG)ins;
    n = count - 1;
    do {
        ins[0] = movpos;
        ins[1] = movctl;
        ins[2] = movpth;
        ins[3] = movptl;
        ins += 4;
        movpos += 0x80000;
        movctl += 0x80000;
        movpth += 0x40000;
        movptl += 0x40000;
    } while ((WORD)--n != -1);
    OVERRUN(cop, ins);
}
BLITZ_THUNK(_customsprites, _customsprites_c, "a0");

/* ---------------------------------------------------- colour splits */

/* DisplayRainbow CopList#,Register,Palette[,copoffset]: a2 = palette */
static void rainbow(UWORD reg, const UBYTE *pal, UWORD off)
{
    UBYTE *cop = blitz_string;
    const ULONG *entries = (const ULONG *)(L(pal, 0) + 4);
    UWORD end = (UWORD)((off + 1) << 2), cpline = CM(cop, cpline), lines;
    UBYTE *ins = (UBYTE *)CM(cop, customs) + (WORD)end;

    if ((WORD)CM(cop, aga) < 0) {
        ULONG bankmove, move, red, green, blue;

        end += 16;
        if ((WORD)end > (WORD)cpline)
            GOTO(linerr);
        bankmove = 0x01060000 | (UWORD)(CM(cop, bplcon3) | (UWORD)((reg & 0xe0) << 8));
        move = (ULONG)(UWORD)(((reg & 0x1f) << 1) + 0x180) << 16;
        lines = CM(cop, height) - 2;
        do {
            red = entries[0];
            green = entries[1];
            blue = entries[2];
            entries += 3;
            red = rol_l(red, 4);
            BL_SETW(green, (UWORD)red);
            green = rol_l(green, 4);
            BL_SETW(blue, (UWORD)green);
            blue = rol_l(blue, 4);
            BL_SETW(move, (UWORD)blue);
            L(ins, 0) = bankmove;
            L(ins, 4) = move;
            bankmove ^= 1 << 9;
            red = rol_l(red, 4);
            BL_SETW(green, (UWORD)red);
            green = rol_l(green, 4);
            BL_SETW(blue, (UWORD)green);
            blue = rol_l(blue, 4);
            BL_SETW(move, (UWORD)blue);
            L(ins, 8) = bankmove;
            L(ins, 12) = move;
            bankmove ^= 1 << 9;
            ins += (WORD)cpline; OPAQUE(ins);
        } while ((WORD)--lines != -1);
    } else {
        ULONG red, green, blue;
        UWORD colreg;

        end += 4;
        if ((WORD)end > (WORD)cpline)
            GOTO(linerr);
        colreg = (UWORD)(reg + reg + 0x180);
        lines = CM(cop, height) - 2;
        do {
            red = entries[0];
            green = entries[1];
            blue = entries[2];
            entries += 3;
            red = rol_l(red, 4);
            BL_SETW(green, (UWORD)red);
            green = rol_l(green, 4);
            BL_SETW(blue, (UWORD)green);
            blue = rol_l(blue, 4);
            W(ins, 0) = colreg;
            W(ins, 2) = (UWORD)blue;
            ins += (WORD)cpline; OPAQUE(ins);
        } while ((WORD)--lines != -1);
    }
    ins -= (WORD)cpline;
    OVERRUN(cop, ins);
}

static void BLITZ_ROUTINE _displayrainbow0_c(UWORD reg BL_REG(d1), const UBYTE *pal BL_REG(a2))
{
    rainbow(reg, pal, 0);
}
static void BLITZ_ROUTINE _displayrainbow_c(UWORD reg BL_REG(d1), const UBYTE *pal BL_REG(a2),
                                            UWORD off BL_REG(d3))
{
    rainbow(reg, pal, off);
}
BLITZ_THUNK(_displayrainbow0, _displayrainbow0_c, "a0");
BLITZ_THUNK(_displayrainbow, _displayrainbow_c, "a0");

/* DisplayRGB CopList#,Register,line,r,g,b[,copoffset]: -2(a2) = offset */
static void rgb(UWORD reg, UWORD line, UWORD red, UWORD green, UWORD blue, UWORD off)
{
    UBYTE *cop = blitz_string;
    UWORD end = (UWORD)((off + 1) << 2), cpline = CM(cop, cpline);
    UBYTE *ins = (UBYTE *)CM(cop, customs) + (WORD)end;

    ins += mulu16(cpline, line);
    if ((WORD)CM(cop, aga) < 0) {
        ULONG bankmove, move;
        UWORD w;

        end += 16;
        if ((WORD)end > (WORD)cpline)
            GOTO(linerr);
        bankmove = 0x01060000 | (UWORD)(CM(cop, bplcon3) | (UWORD)((reg & 0xe0) << 8));
        move = (ULONG)(UWORD)(((reg & 0x1f) << 1) + 0x180) << 16;
        w = red;
        w = (UWORD)(w << 4 | w >> 12);                  /* ROL.w #4 */
        w = (w & 0xff00) | (UBYTE)green;
        w = (UWORD)(w << 4 | w >> 12);
        w = (w & 0xff00) | (UBYTE)blue;
        w >>= 4;
        L(ins, 0) = bankmove;
        L(ins, 4) = move | w;
        bankmove ^= 1 << 9;
        w = blue;
        w = (UWORD)(w >> 4 | w << 12);                  /* ROR.w #4 */
        w = (w & 0xff00) | (UBYTE)green;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)red;
        w = (UWORD)(w << 8 | w >> 8);                   /* ROL.w #8 */
        L(ins, 8) = bankmove;
        L(ins, 12) = move | w;
    } else {
        UWORD w;

        end += 4;
        if ((WORD)end > (WORD)cpline)
            GOTO(linerr);
        reg = (UWORD)(reg + reg + 0x180);
        w = blue;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)green;
        w = (UWORD)(w >> 4 | w << 12);
        w = (w & 0xff00) | (UBYTE)red;
        w = (UWORD)(w << 8 | w >> 8);
        W(ins, 0) = reg;
        W(ins, 2) = w;
    }
    OVERRUN(cop, ins);
}

static void BLITZ_ROUTINE _displayrgb0_c(UWORD reg BL_REG(d1), UWORD line BL_REG(d2),
                                         UWORD red BL_REG(d3), UWORD green BL_REG(d4),
                                         UWORD blue BL_REG(d5))
{
    rgb(reg, line, red, green, blue, blah[0]);
}
static void BLITZ_ROUTINE _displayrgb_c(UWORD reg BL_REG(d1), UWORD line BL_REG(d2),
                                        UWORD red BL_REG(d3), UWORD green BL_REG(d4),
                                        UWORD blue BL_REG(d5), const UWORD *args BL_REG(a2))
{
    rgb(reg, line, red, green, blue, args[-1]);
}
BLITZ_THUNK(_displayrgb0, _displayrgb0_c, "d0/a0");
BLITZ_THUNK(_displayrgb, _displayrgb_c, "d0/a0");

/* DisplayUser CopList#,Line,String[,Offset] */
static void user(UWORD line, const ULONG *s, UWORD off, ULONG len)
{
    UBYTE *cop = blitz_string;
    UWORD end = (UWORD)((off + 1) << 2), cpline = CM(cop, cpline), count;
    ULONG *ins = (ULONG *)((UBYTE *)CM(cop, customs) + (WORD)end);

    ins = (ULONG *)((UBYTE *)ins + mulu16(cpline, line));
    end += (UWORD)len;
    if ((WORD)end > (WORD)cpline)
        GOTO(linerr);
    count = (UWORD)len;
    if (count & 3)
        GOTO(freakerr);
    count >>= 2;
    if (count) {
        count--;
        do
            *ins++ = *s++;
        while ((WORD)--count != -1);
    }
    OVERRUN(cop, ins);
}

static void BLITZ_ROUTINE _displayuser0_c(UWORD line BL_REG(d1), const ULONG *text BL_REG(d2),
                                          const ULONG *lens BL_REG(a2))
{
    user(line, text, 0, lens[-1]);
}
static void BLITZ_ROUTINE _displayuser_c(UWORD line BL_REG(d1), const ULONG *text BL_REG(d2),
                                         UWORD off BL_REG(d3), const ULONG *lens BL_REG(a2))
{
    user(line, text, off, lens[-1]);
}
BLITZ_THUNK(_displayuser0, _displayuser0_c, "a2");
BLITZ_THUNK(_displayuser, _displayuser_c, "a2");

/* DisplayScroll CopList#,&xpos.q(n),&xpos.q(n)[,Offset]: per line a
   BPLCON1 move and the two modulos */
static void scroll(const LONG *xpos1, const LONG *xpos2, UWORD off, ULONG callerd0)
{
    UBYTE *cop = blitz_string, *ins;
    ULONG pos1, pos2, con1, mod1, mod2, shifts, fine2;
    ULONG resbits;                      /* d0: the resolution bits */
    ULONG mask;                         /* d4 */
    UWORD resw;                         /* d6: the resolution */
    ULONG flup;                         /* d6 */
    ULONG rot;                          /* d6: the rotate count */
    UWORD idx;

    resbits = (callerd0 & 0xffffff00) | B(cop, CMOFF(res));
    resw = (UWORD)resbits;
    BL_SETW(resbits, (UWORD)resbits & 3);
    mask = 0xfffff;
    idx = (resw & 0x30) >> 2;
    shifts = *(const ULONG *)((const UBYTE *)shifs + idx);
    idx >>= 2;
    flup = flups[idx];                  /* MOVE.b flups(pc,d6),d6 (d6.w = idx) */
    mask = asl_l(mask, flup);
    rot = flup;
    BL_SETW(rot, (UWORD)flup + 6 - (UWORD)resbits);
    {
        UWORD end = (UWORD)((off + 1) << 2);

        ins = (UBYTE *)CM(cop, customs) + (WORD)end;
        end += 12;
        if ((WORD)end > (WORD)CM(cop, cpline))
            GOTO(linerr);
    }
    sc.ycont = CM(cop, height) - 2;
    pos1 = CM(cop, x0) & mask;
    pos2 = CM(cop, x1) & mask;
    {
        const ULONG *a2 = (const ULONG *)CM(cop, bplanes);

        con1 = a2[-3];
        mod1 = a2[-2];
        mod2 = a2[-1];
    }
    sc.mods[0] = (UWORD)mod1;
    sc.mods[1] = (UWORD)mod2;
    fine2 = 0;
    do {
        L(ins, 0) = con1;
        pos1 += (ULONG)*xpos1++;
        pos2 += (ULONG)*xpos2++;
        pos1 = asr_l(pos1, shifts);
        pos2 = asr_l(pos2, shifts);
        shifts = BL_SWAP(shifts);
        pos1 = BL_SWAP(pos1);
        pos2 = BL_SWAP(pos2);
        BL_SETW(mod1, (UWORD)pos1);
        BL_SETW(mod2, (UWORD)pos2);
        mod1 = asl_w(mod1, shifts);
        mod2 = asl_w(mod2, shifts);
        shifts = BL_SWAP(shifts);
        BL_SETW(mod1, (UWORD)mod1 + sc.mods[0]);
        BL_SETW(mod2, (UWORD)mod2 + sc.mods[1]);
        BL_SETW(pos1, 0);
        pos1 = rol_l(pos1, rot);
        BL_SETW(con1, (UWORD)pos1);
        pos1 = ror_l(pos1, rot);
        pos1 = asl_l(BL_SWAP(pos1), shifts);
        BL_SETW(pos2, 0);
        pos2 = rol_l(pos2, rot);
        BL_SETW(fine2, (UWORD)pos2);
        pos2 = ror_l(pos2, rot);
        pos2 = asl_l(BL_SWAP(pos2), shifts);
        BL_SETW(con1, BP1((UWORD)((UWORD)con1 + (UWORD)con1)) & 0x0f0f);
        BL_SETW(fine2, BP1((UWORD)((UWORD)fine2 + (UWORD)fine2)) & 0xf0f0);
        BL_SETW(con1, (UWORD)con1 | (UWORD)fine2);
        L(ins, 4) = mod1;
        L(ins, 8) = mod2;
        ins += (WORD)CM(cop, cpline); OPAQUE(ins);
    } while ((WORD)--sc.ycont >= 0);
}

static void BLITZ_ROUTINE _displayscroll0_c(ULONG callerd0 BL_REG(d0), const LONG *xpos1 BL_REG(d1),
                                            const LONG *xpos2 BL_REG(d2))
{
    scroll(xpos1, xpos2, 0, callerd0);
}
static void BLITZ_ROUTINE _displayscroll_c(ULONG callerd0 BL_REG(d0), const LONG *xpos1 BL_REG(d1),
                                           const LONG *xpos2 BL_REG(d2), UWORD off BL_REG(d3))
{
    scroll(xpos1, xpos2, off, callerd0);
}
BLITZ_THUNK(_displayscroll0, _displayscroll0_c, "a2");
BLITZ_THUNK(_displayscroll, _displayscroll_c, "a2");

/* ------------------------------------------- chunky copper routines (AGA) */

/* CustomChunkySize(width,height) */
static ULONG BLITZ_ROUTINE _customchunkysize_c(ULONG width BL_REG(d0), UWORD height BL_REG(d1))
{
    UWORD banks = (UWORD)((UWORD)((UWORD)width + 31) >> 5);
    ULONG size;                         /* d0 */

    size = mulu16((UWORD)((UWORD)width + banks + 3), height);
    BL_SETW(size, (UWORD)size + 2);
    return size;
}
BLITZ_THUNK(_customchunkysize, _customchunkysize_c, "d1/a0-a1");

/* CustomChunky CopList#,CCOffset,YPos,width,height,res */
static void customchunky(UWORD ccoffset, UWORD ypos, UWORD width, UWORD height, UWORD res)
{
    UBYTE *cop = blitz_string;
    ULONG *ins, waitmove, waitstep, con4move, bankmove;
    UWORD y, lines;
    UWORD linebytes;                    /* d3: the bytes per line */

    CM(cop, chy) = ypos;
    CM(cop, chw) = width;
    CM(cop, chh) = height;
    CM(cop, chr) = res;
    ins = (ULONG *)((UBYTE *)CM(cop, customs) + (WORD)(UWORD)(ccoffset << 2));
    y = ypos - res;
    if ((WORD)y < 0)
        GOTO(toolowerr);
    y += CM(cop, ypos);
    ypos += CM(cop, ypos);
    ins = putwait(ins, y, CM(cop, bot), 0xe1);
    CM(cop, cchunky) = (ULONG)ins + 6;
    CM(cop, bot) = (UWORD)(mulu16(res, height) + ypos);
    linebytes = (UWORD)(width + (width >> 5) + 3);
    linebytes = (UWORD)(linebytes << 2);
    CM(cop, chc) = linebytes;
    waitmove = BL_SWAP(BL_EXTL((UWORD)(y + res)));
    waitmove <<= 8;
    waitmove |= 0xe1fffe;
    waitstep = BL_SWAP((ULONG)(UWORD)(res << 8) | (BL_EXTL(res) & 0xffff0000));
    lines = height - 1;
    con4move = 0x010c0011;
    bankmove = 0x01066c00;
    do {
        UWORD cols = CM(cop, chw) - 1, r = 0x17e;
        ULONG bank = bankmove;

        bankmove ^= 1UL << 15;
        do {
            if (r == 0x17e) {
                r = 0x1be;
                *ins++ = bank;
                BL_SETW(bank, (UWORD)bank - 0x2000);
            }
            ((UWORD *)ins)[0] = r;
            ((UWORD *)ins)[1] = 0;
            ins++;
            r -= 2;
        } while ((WORD)--cols != -1);
        *ins++ = waitmove;
        *ins++ = con4move;
        con4move ^= 1UL << 15;
        waitmove += waitstep;
    } while ((WORD)--lines != -1);
    OVERRUN(cop, ins);
}

static void BLITZ_ROUTINE _customchunky_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                          UWORD width BL_REG(d3), UWORD height BL_REG(d4),
                                          UWORD res BL_REG(d5))
{
    customchunky(ccoffset, ypos, width, height, res);
}

/* the second form: -2(a2) = split position */
static void BLITZ_ROUTINE _customchunky2_c(UWORD ccoffset BL_REG(d1), UWORD ypos BL_REG(d2),
                                           UWORD width BL_REG(d3), UWORD height BL_REG(d4),
                                           UWORD res BL_REG(d5), const UWORD *args BL_REG(a2))
{
    UBYTE *cop = blitz_string;
    ULONG saved[4];
    UWORD split;
    int k;

    customchunky(ccoffset, ypos, width, height, res);
    /* MOVEM.l !m{cchunky},d0-d3 ... MOVEM.l d0-d3,!m{cchunky2}: 16 bytes,
       cchunky..pad0 to cchunky2..spri.  BUG(orig) (#949): the last word,
       pad0 (always 0), overwrites spri, the sprite resolution shift that
       DisplaySprite uses for the x handle */
    for (k = 0; k < 4; k++)
        saved[k] = L(cop, CMOFF(cchunky) + 4 * k);
    split = args[-1];
    CM(cop, chh) = split;
    for (k = 0; k < 4; k++)
        L(cop, CMOFF(cchunky2) + 4 * k) = saved[k];
    CM(cop, chh2) -= split;
    CM(cop, cchunky2) += mulu16(CM(cop, chc), split);
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
