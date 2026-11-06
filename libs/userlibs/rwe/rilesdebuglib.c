/*
 * rilesdebuglib - RWE userlib 49 (LES debug, the helper library of RWE's
 * "REDDebugger"): memory and register views as text, an 8x8 font printer
 * for 640 pixel wide bitmaps with a detokeniser for tokenised Blitz
 * lines, copper list and custom chip names, an argument parser and a
 * search through memory.
 *
 * C translation of rwelibsrc/LESDebuglib/LESDEBUGlib.bb (1996-12-01, newer
 * than every binary, docs/rwelibs/README.md 3.2); reference the reference
 * build of that source by blitz2 2.10 (refbuild).  Notes:
 * docs/rwelibs/port/rilesdebuglib.md.
 *
 * It is a library of ordinary commands: it does not talk to blitz2's
 * runtime debugger (no error checks, no debuglib calls).  The debugger
 * program that used it hands it its token list (SetupLESDebug) and the
 * tokenised lines of the source (FPrintDetoke, FindLineToken).
 *   init/finit           open/close graphics.library (any version)
 *   D_FindVolume         FindDosEntry (devices, volumes, assigns)
 *   D_GetSR, D_HexWord, D_CheckNMove[A|Word], GetHex$, GetHexAdr$,
 *   PeekMax$, GetDataItem   memory and registers as text; addresses
 *                        below $420 show "*" (not D_CheckNMove, GetHex$,
 *                        GetHexAdr$, PeekMax$)
 *   FLocate, FPrint, FNPrint, FInverse, FYOffset   the font printer: 8
 *                        bytes per character into plane 0 of the used
 *                        bitmap, rows of 80 bytes, 80 columns
 *   FPrintDetoke         a tokenised line into the bitmap, the tokens
 *                        (bit 15) as their names from the token list,
 *                        also into plane 1 if SetupLESDebug asked so; a
 *                        marker ('>' or an inverse space) at the
 *                        current instruction; columns skipped from the
 *                        left (lineoffset)
 *   GetCopIns$, GetCustomReg, GetCustomRegAdr   copper instructions and
 *                        the names of the custom chip registers
 *   OutText, OutTextPen  Move/Text/SetAPen on a RastPort
 *   D_InitParse, D_NextArg$   comma separated arguments
 *   ProcessString, AddProcessPtr, ClearProcessPtrs   %x replaced by a
 *                        string (%p "REDDebugger", %c the token list
 *                        address in hex, up to 20 more)
 *   SearchMem, SearchMemFound, FindStringPtr, FindLineToken, ExtractLabel
 *
 * All C behind the thunks; graphics and dos through the NDK inlines,
 * RawStatus (blitzkeyslib) through BLITZ_ALIBJSR.  The font is the 2048
 * bytes the source IncBins from "Blitz2:Debugger/Font.bin":
 * rilesdebuglib-font.bin, cut from AmiBlitz3's binary (archive/README.md).
 *
 * BUG(orig), kept:
 *   - D_HexWord, D_CheckNMove, D_CheckNMoveA, D_CheckNMoveWord with count
 *     0 (D_CheckNMoveWord also $8000) run 65536 times (DBRA); GetDataItem
 *     with size 0 reads 65536 bytes, with a negative size fewer (-1: 65535)
 *     (DBRA) (#701).
 *   - D_HexWord at an odd source writes "XX YY" per word, without the
 *     space behind it that the even case writes (#715).
 *   - The "address below $420" checks compare signed: addresses from
 *     $80000000 show "*" too (D_HexWord, D_CheckNMoveA/Word) or give 0
 *     (GetDataItem) (#717).
 *   - PeekMax$ copies max+1 characters but returns the length max (a3 is
 *     one behind the result) (#718).
 *   - GetCustomReg of a name of 8 characters copies one more character
 *     (the first of the next name) and returns 8 (#712); a negative
 *     register word (d0.l not negative) reads in front of the name table
 *     (#714).
 *   - GetCustomRegAdr matches a prefix (the name may go on behind the
 *     table's), never finds a name of 8 characters (#713), and never
 *     looks at the last register ($1FE NOOP) (#702).
 *   - GetCopIns$ clears bit 0 of both nibbles of the horizontal positions
 *     (WAIT/SKIP position and mask), so odd digits never show (#711).
 *   - FPrint and FPrintDetoke: the column check is a byte compare with
 *     80 tested on N (columns 80-207 stop, 208-255 print); nothing checks
 *     the bitmap's size (#707).  FPrint ignores a string of 32769
 *     characters or more (SUBQ.w/BMI) (#710).
 *   - FInverse sets a word that FPrint tests as a word but FPrintDetoke
 *     as its high byte (FInverse 1 inverts FPrint only); FPrintDetoke's
 *     cursor marker sets and clears that high byte (#706).  With the high
 *     byte set and tokens drawn into two planes, the second plane gets
 *     the character of the last glyph byte (the inverse drawing
 *     overwrites d0's low byte) (#709).
 *   - FPrintDetoke: after the '>' marker, an inverse space follows if the
 *     column count (d6 with the caller's high word) became 0 (#708).
 *   - D_FindStringPtr never compares the long at areg5 itself (#705);
 *     ExtractLabel searches the '.' without a limit (#703), and a space
 *     does not end the label: blitz2 assembles the source's
 *     CMPI.b #" ",d2 as CMPI.b #0 (the reference build, the CD's and
 *     AmiBlitz3's binaries alike) - compiler bug #343, triggered by the
 *     later line MOVE.w #">"*8,d0 (line 1016 of the source; the
 *     operands are evaluated last line first, so what matters is the
 *     line's position, not the destination register) (#704).
 *   - SearchMem's string search does not go back after a partial match
 *     ("ab" is not found in "aab") (#723); an empty string compares the
 *     bytes behind its 0 (#721).  The word/long searches compare bytes up
 *     to 1/3 behind the end (#722).
 *   - D_InitParse copies without a limit into its 256 byte buffer (#716);
 *     ProcessString works on the parser's current position (its "string"
 *     is no argument), reads from 0 when there is none (#719), and goes
 *     on behind a '%' at the end of the string (#720).
 *   - AddProcessPtr does not stop after 20 entries (the original then
 *     overwrites the code of D_InitParse behind the table, the C build
 *     other data) (#700).
 * Like the original, no defects: ClearProcessPtrs only clears the first
 * entry (it ends the lookup, and the next AddProcessPtr reuses it); the
 * %p and %c entries are set by init and point to 0 before it (init
 * always runs first); a %x without an entry drops x (as the source
 * intends).
 *
 * DIFF: a2 and a3 stay unchanged where the original uses them as work
 * registers (FPrint: a2 = the font, a3 = the glyph; SearchMem: a2 = the
 * end, a3 in the string search; GetCustomRegAdr: a2), as in all C ports
 * (docs/libs/cmdlibs-port.md "General").  The inverse drawing of
 * FPrintDetoke does not invert the font in place and back (NOT.l on the
 * glyph in the original), same output.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfxbase
#define DOS_BASE_NAME dosbase
#include <proto/graphics.h>
#include <proto/dos.h>
#undef __NOLIBBASE__

/* blitzkeyslib RawStatus: d0 = raw key -> d0 = -1 (down) / 0 */
#define TOK_rawstatus 0x3102
#define RAW_SPACE 64

/* entry points (thunks, below) */
void rilesdebuglib_finddev(void), _getsr(void), _hexword(void);
void _checknmove(void), _checknmove2(void), _findstringptr(void);
void _getcopins(void), _flocate(void), _fprint(void), _fnprint(void);
void _finverse(void), _getprogmode(void), _fyoffset(void), _searchmem(void);
void _searchmemfound(void), _peekmax(void), _checknmoveword(void);
void _checknmoveword2(void), _gethex(void), _gethexadr(void);
void _getcustomreg(void), _getcustomregadr(void), _fprintdetoke(void);
void _setuplesdebug(void), _extractlabel(void), _outtext(void);
void _checknmovea(void), _checknmovea2(void), _outtextpen(void);
void _initparse(void), rilesdebuglib_nextparsearg(void), _processstring(void);
void _addprocessptr(void), _clearprocessptrs(void), _findlinetoken(void);
void _getdataitem(void), rilesdebuglib_libinit(void);
void rilesdebuglib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#riLESDEBUGlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riLESDEBUGlib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(rilesdebuglib_finddev), 0, 0);
BLITZ_NAME("D_FindVolume", "(volumename)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_getsr, 0, 0);
BLITZ_NAME("D_GetSR", "buffer,SR");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_hexword, 0, 0);
BLITZ_NAME("D_HexWord", "source,address,num");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmove, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmove2, 0, 0);
BLITZ_NAME("D_CheckNMove", "source_address,dest_address[,count]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_findstringptr, 0, 0);
BLITZ_NAME("D_FindStringPtr", "areg5,lookfor");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_getcopins, 0, 0);
BLITZ_NAME("GetCopIns$", "copdata");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_flocate, 0, 0);
BLITZ_NAME("FLocate", "x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_bitmaplib, BL_LA(0) | BL_USED);
BLITZ_SUBS(_fprint, 0, 0);
BLITZ_NAME("FPrint", "a$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_bitmaplib, BL_LA(0) | BL_USED);
BLITZ_SUBS(_fnprint, 0, 0);
BLITZ_NAME("FNPrint", "a$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_finverse, 0, 0);
BLITZ_NAME("FInverse", "inversemode");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_getprogmode, 0, 0);
BLITZ_NAME("GetProgMode", "mode,buffer");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fyoffset, 0, 0);
BLITZ_NAME("FYOffset", "pixel_offset - offset from vertical y cursor to print at");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_BYTE, BL_UNKNOWN, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_searchmem, 0, 0);
BLITZ_NAME("SearchMem", "type,value,start_ad,end_ad");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_searchmemfound, 0, 0);
BLITZ_NAME("SearchMemFound", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_peekmax, 0, 0);
BLITZ_NAME("PeekMax$", "(address,max_chars)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmoveword, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmoveword2, 0, 0);
BLITZ_NAME("D_CheckNMoveWord", "source_address,dest_address[,count]");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gethex, 0, 0);
BLITZ_NAME("GetHex$", "value,mode  (1=byte, 2=word, 3=longword)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gethexadr, 0, 0);
BLITZ_NAME("GetHexAdr$", "*value,mode  (1=byte, 2=word, 3=longword)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_getcustomreg, 0, 0);
BLITZ_NAME("GetCustomReg", "address");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_getcustomregadr, 0, 0);
BLITZ_NAME("GetCustomRegAdr", "name$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_LA(0) | BL_USED);
BLITZ_SUBS(_fprintdetoke, 0, 0);
BLITZ_NAME("FPrintDetoke", "tokenised_string,cio,typ,lineoffset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_setuplesdebug, 0, 0);
BLITZ_NAME("SetupLESDebug", "token_addr,tokenise");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_extractlabel, 0, 0);
BLITZ_NAME("ExtractLabel", "label_string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_outtext, 0, 0);
BLITZ_NAME("OutText", "x,y,rastport,address/string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmovea, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_checknmovea2, 0, 0);
BLITZ_NAME("D_CheckNMoveA", "source_address,dest_address[,count]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_outtextpen, 0, 0);
BLITZ_NAME("OutTextPen", "rastport,pen");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_initparse, 0, 0);
BLITZ_NAME("D_InitParse", "string");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(rilesdebuglib_nextparsearg, 0, 0);
BLITZ_NAME("D_NextArg$", "- get next parameter");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_processstring, 0, 0);
BLITZ_NAME("ProcessString", "string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addprocessptr, 0, 0);
BLITZ_NAME("AddProcessPtr", "character,string");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_clearprocessptrs, 0, 0);
BLITZ_NAME("ClearProcessPtrs", "-- reset process string pointers");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_findlinetoken, 0, 0);
BLITZ_NAME("FindLineToken", "Start_Address");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_getdataitem, 0, 0);
BLITZ_NAME("GetDataItem", "address,size  -- size=1 byte, size=2 word, otherwise long");

BLITZ_NULLSUB_AT(finit, rilesdebuglib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rilesdebuglib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ----------------------------------------------------------------- data */

/* the font: IncBin "Blitz2:Debugger/Font.bin", 256 characters of 8 bytes
   (one per row) - rilesdebuglib-font.bin, the bytes of AmiBlitz3's
   RILESDEBUGLib.obj at 4566, the same as in the CD's RILESDEBUGLIB.OBJ
   (archive/README.md, ctest rwe_lesfont_cd) */
#define FONT_FILE "rilesdebuglib-font.bin"
#define FONTSIZE 2048
_Static_assert(sizeof((const UBYTE[]){
#embed FONT_FILE
    }) == FONTSIZE, "rilesdebuglib-font.bin: 2048 bytes");
static const UBYTE font_dat[FONTSIZE] __attribute__((aligned(2))) = {
#embed FONT_FILE
};

static const UBYTE hexdigits[16] = "0123456789ABCDEF";

static struct GfxBase *gfxbase;

/* the font printer: cursor (column, row of 8 pixels), inverse, y offset */
static UWORD xcurs, ycurs, inverse, yoffset;
/* the high byte of inverse (TST.b/ST/SF inverse in FPrintDetoke) */
#define INVERSE_HI (*(UBYTE *)&inverse)

/* FPrintDetoke: the token list (SetupLESDebug), its options */
static ULONG tokens;
static const UBYTE notoke[] = "?????";
static UBYTE chartyp, plz_tokenise;

/* SearchMem's result */
static ULONG findaddress;

/* the argument parser (D_InitParse, D_NextArg$, ProcessString) */
static UBYTE parseadr[256];
static UBYTE *curadr;

/* ProcessString's % entries: character, 0, string pointer; "p" and "c"
   (set by init), 20 of AddProcessPtr, the end (a 0 character) */
struct ptok {
    UBYTE c, z;
    const UBYTE *p;
} __attribute__((packed, aligned(2)));
static struct {
    struct ptok fixed[2], user[20];
    UBYTE end[2];
} __attribute__((packed, aligned(2))) stringtokens = { { { 'p', 0, NULL }, { 'c', 0, NULL } } };
static const UBYTE progname[] = "REDDebugger";
static UBYTE tokenaddr[9];

/* GetProgMode's texts (6 characters, no 0) */
static const UBYTE qamigatext[6] = "QAMIGA", amigatext[6] = "AMIGA ", blitztext[6] = "BLITZ ";

/* the custom chip registers $000-$1FE, 8 characters each */
static const UBYTE customchipnames[256][8] = {
    "BLTDDAT ", "DMACONR ", "VPOSR   ", "VHPOSR  ", "DSKDATR ", "JOY0DAT ", "JOY1DAT ", "CLXDAT  ",
    "ADKCONR ", "POT0DAT ", "POT1DAT ", "POTINP  ", "SERDATR ", "DSKBYTR ", "INTENAR ", "INTREQR ",
    "DSKPTH  ", "DSKPTL  ", "DSKLEN  ", "DSKDAT  ", "REFPTR  ", "VPOSW   ", "VHPOSW  ", "COPCON  ",
    "SERDAT  ", "SERPER  ", "POTGO   ", "JOYTEST ", "STR     ", "STRVBL  ", "STRHOR  ", "STRLONG ",
    "BLTCON0 ", "BLTCON1 ", "BLTAFWM ", "BLTALWM ", "BLTCPTH ", "BLTCPTL ", "BLTBPTH ", "BLTBPTL ",
    "BLTAPTH ", "BLTAPTL ", "BLTDPTH ", "BLTDPTL ", "BLTSIZE ", "BLTCON0L", "BLTSIZV ", "BLTSIZH ",
    "BLTCMOD ", "BLTBMOD ", "BLTAMOD ", "BLTDMOD ", "<UNUSED>", "<UNUSED>", "<UNUSED>", "<UNUSED>",
    "BLTCDAT ", "BLTBDAT ", "BLTADAT ", "<UNUSED>", "SPRHDAT ", "<UNUSED>", "DENISEID", "DSKSYNC ",
    "COP1LCH ", "COP1LCL ", "COP2LCH ", "COP2LCL ", "COPJMP1 ", "COPJMP2 ", "COPINS  ", "DIWSTRT ",
    "DIWSTOP ", "DDFSTRT ", "DDFSTOP ", "DMACON  ", "CLXCON  ", "INTENA  ", "INTREQ  ", "ADKCON  ",
    "AUD0LCH ", "AUD0LCL ", "AUD0LEN ", "AUD0PER ", "AUD0VOL ", "AUD0DAT ", "<UNUSED>", "<UNUSED>",
    "AUD1LCH ", "AUD1LCL ", "AUD1LEN ", "AUD1PER ", "AUD1VOL ", "AUD1DAT ", "<UNUSED>", "<UNUSED>",
    "AUD2LCH ", "AUD2LCL ", "AUD2LEN ", "AUD2PER ", "AUD2VOL ", "AUD2DAT ", "<UNUSED>", "<UNUSED>",
    "AUD3LCH ", "AUD3LCL ", "AUD3LEN ", "AUD3PER ", "AUD3VOL ", "AUD3DAT ", "<UNUSED>", "<UNUSED>",
    "BPL0PTH ", "BPL0PTL ", "BPL1PTH ", "BPL1PTL ", "BPL2PTH ", "BPL2PTL ", "BPL3PTH ", "BPL3PTL ",
    "BPL4PTH ", "BPL4PTL ", "BPL5PTH ", "BPL5PTL ", "BPL6PTH ", "BPL6PTL ", "BPL7PTH ", "BPL7PTL ",
    "BPLCON0 ", "BPLCON1 ", "BPLCON2 ", "BPLCON3 ", "BPL1MOD ", "BPL2MOD ", "BPLCON4 ", "CLXCON2 ",
    "BPLDAT0 ", "BPLDAT1 ", "BPLDAT2 ", "BPLDAT3 ", "BPLDAT4 ", "BPLDAT5 ", "BPLDAT6 ", "BPLDAT7 ",
    "SPR0PTH ", "SPR0PTL ", "SPR1PTH ", "SPR1PTL ", "SPR2PTH ", "SPR2PTL ", "SPR3PTH ", "SPR3PTL ",
    "SPR4PTH ", "SPR4PTL ", "SPR5PTH ", "SPR5PTL ", "SPR6PTH ", "SPR6PTL ", "SPR7PTH ", "SPR7PTL ",
    "SPR0POS ", "SPR0CTL ", "SPR0DATA", "SPR0DATB", "SPR1POS ", "SPR1CTL ", "SPR1DATA", "SPR1DATB",
    "SPR2POS ", "SPR2CTL ", "SPR2DATA", "SPR2DATB", "SPR3POS ", "SPR3CTL ", "SPR3DATA", "SPR3DATB",
    "SPR4POS ", "SPR4CTL ", "SPR4DATA", "SPR4DATB", "SPR5POS ", "SPR5CTL ", "SPR5DATA", "SPR5DATB",
    "SPR6POS ", "SPR6CTL ", "SPR6DATA", "SPR6DATB", "SPR7POS ", "SPR7CTL ", "SPR7DATA", "SPR7DATB",
    "COLOR00 ", "COLOR01 ", "COLOR02 ", "COLOR03 ", "COLOR04 ", "COLOR05 ", "COLOR06 ", "COLOR07 ",
    "COLOR08 ", "COLOR09 ", "COLOR10 ", "COLOR11 ", "COLOR12 ", "COLOR13 ", "COLOR14 ", "COLOR15 ",
    "COLOR16 ", "COLOR17 ", "COLOR18 ", "COLOR19 ", "COLOR20 ", "COLOR21 ", "COLOR22 ", "COLOR23 ",
    "COLOR24 ", "COLOR25 ", "COLOR26 ", "COLOR27 ", "COLOR28 ", "COLOR29 ", "COLOR30 ", "COLOR31 ",
    "HTOTAL  ", "HSSTOP  ", "HBSTRT  ", "HBSTOP  ", "VTOTAL  ", "VSSTOP  ", "VBSTRT  ", "VBSTOP  ",
    "SPRHSTRT", "SPRHSTOP", "BPLHSTRT", "BPLHSTOP", "HHPOSW  ", "HHPOSR  ", "BEAMCON0", "HSSTRT  ",
    "VSSTRT  ", "HCENTER ", "DIWHIGH ", "BPLHMOD ", "SPRHPTH ", "SPRHPTL ", "BPLHPTH ", "BPLHPTL ",
    "<UNUSED>", "<UNUSED>", "<UNUSED>", "<UNUSED>", "<UNUSED>", "<UNUSED>", "FMODE   ", "NOOP    ",
};

/* "address below $420" (CMP.l with $420, signed) */
#define LOWMEM(p) ((LONG)(p) < 0x420)

/* --------------------------------------------------------- init, finit */

/**
 * @brief init: open graphics.library (any version) and set ProcessString's
 * %p ("REDDebugger") and %c (the token list address in hex) entries.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    gfxbase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    stringtokens.fixed[0].p = progname;
    stringtokens.fixed[1].p = tokenaddr;
}

/**
 * @brief finit: close graphics.library.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary((struct Library *)gfxbase);
    gfxbase = NULL;
}

/* ------------------------------------------------------- D_FindVolume */

/**
 * @brief D_FindVolume(volumename): look up a device, volume or assign
 * (Amiga mode only).
 *
 * LockDosList, FindDosEntry, UnLockDosList with LDF_READ, devices,
 * volumes and assigns.
 * @param name    d0: the name (without the colon)
 * @param dosbase a6: dos.library (!libs #doslib)
 * @return d0: the DosList entry, 0 if not found (or LockDosList failed);
 *         the list is no longer locked
 */
#define FINDFLAGS (LDF_READ | LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_ENTRY)
static ULONG BLITZ_ROUTINE finddev_c(CONST_STRPTR name BL_REG(d0),
                                     struct DosLibrary *dosbase BL_REG(a6))
{
    struct DosList *dl = LockDosList(FINDFLAGS);

    if (!dl)
        return 0;
    dl = FindDosEntry(dl, name, FINDFLAGS);
    UnLockDosList(FINDFLAGS);
    return (ULONG)dl;
}

/* -------------------------------------------- registers and memory as text */

/**
 * @brief D_GetSR buffer,SR: the condition codes as text.
 *
 * "XNZVC" with '-' for the bits not set, and a 0 (6 bytes).
 * @param buf  d0: the buffer
 * @param sr   d1.b: the status register (its low byte)
 */
static void BLITZ_ROUTINE getsr_c(UBYTE *buf BL_REG(d0), UBYTE sr BL_REG(d1))
{
    static const UBYTE flags[5] = "CVZNX";
    int i;

    *(ULONG *)buf = 0x2d2d2d2d;
    *(UWORD *)(buf + 4) = 0x2d00;
    for (i = 0; i < 5; i++, sr >>= 1)
        if (sr & 1)
            buf[4 - i] = flags[i];
}

/* two hex digits of a byte / "**" for an address below $420 */
static UBYTE *hexbyte(UBYTE *d, const UBYTE **s)
{
    if (LOWMEM(*s)) {
        (*s)++;
        *d++ = '*';
        *d++ = '*';
    } else {
        UBYTE b = *(*s)++;

        *d++ = hexdigits[b >> 4];
        *d++ = hexdigits[b & 15];
    }
    return d;
}

/**
 * @brief D_HexWord source,address,num: num words of memory as hex text.
 *
 * "XXXX " each; at an odd source "XX YY" without the space behind
 * (BUG(orig) (#715)).  Bytes at addresses below $420 show "*" (a signed
 * compare: from $80000000 too, BUG(orig) (#717)).  No 0 is written.
 * num 0 runs 65536 times (DBRA, BUG(orig) (#701)).
 * @param src  d0: the source address
 * @param dst  d1: the buffer
 * @param num  d2.w: the number of words
 */
static void BLITZ_ROUTINE hexword_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                    UWORD num BL_REG(d2))
{
    UWORD n = num - 1;                                  /* d6, DBRA */

    if ((ULONG)src & 1) {
        do {                                            /* 'hexwordodd */
            dst = hexbyte(dst, &src);
            *dst++ = ' ';
            dst = hexbyte(dst, &src);
        } while (n-- != 0);
        return;
    }
    do {
        if (LOWMEM(src)) {
            src += 2;
            *dst++ = '*';
            *dst++ = '*';
            *dst++ = '*';
            *dst++ = '*';
        } else {
            UWORD w = *(const UWORD *)src;

            src += 2;
            *dst++ = hexdigits[w >> 12];
            *dst++ = hexdigits[(w >> 8) & 15];
            *dst++ = hexdigits[(w >> 4) & 15];
            *dst++ = hexdigits[w & 15];
        }
        *dst++ = ' ';                                   /* 'jumper */
    } while (n-- != 0);
}

/**
 * @brief D_CheckNMove source_address,dest_address,count: count longs as 4
 * characters each, one byte left between them.
 *
 * The bytes are copied as they are (also 0 and below $420).  count 0
 * runs 65536 times (DBRA, BUG(orig) (#701)).
 * @param s      d0: the source
 * @param d      d1: the destination (5 bytes per long)
 * @param count  d2.w: the number of longs
 */
static void BLITZ_ROUTINE checknmove2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                        UWORD count BL_REG(d2))
{
    UWORD n = count - 1;                                /* d7, DBRA */

    do {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        s += 4;
        d += 5;
    } while (n-- != 0);
}

/**
 * @brief D_CheckNMove source_address,dest_address: one long (count 1).
 * @param s  d0: the source
 * @param d  d1: the destination
 */
static void BLITZ_ROUTINE checknmove_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1))
{
    checknmove2_c(s, d, 1);
}

/* one byte as a character: '*' below $420, a 0 as 1 */
static UBYTE *checkbyte(UBYTE *d, const UBYTE **s)
{
    if (LOWMEM(*s)) {
        (*s)++;
        *d++ = '*';
    } else {
        UBYTE c = *(*s)++;

        *d++ = c ? c : 1;
    }
    return d;
}

/**
 * @brief D_CheckNMoveA source_address,dest_address,count: as
 * D_CheckNMove, with no 0 in the characters and "*" below $420.
 *
 * A 0 byte becomes 1; an address below $420 (signed compare, BUG(orig)
 * (#717)) gives '*'.  count 0 runs 65536 times (#701).
 * @param s      d0: the source
 * @param d      d1: the destination (5 bytes per long)
 * @param count  d2.w: the number of longs
 */
static void BLITZ_ROUTINE checknmovea2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                         UWORD count BL_REG(d2))
{
    UWORD n = count - 1;                                /* d7, DBRA */
    int i;

    do {
        for (i = 0; i < 4; i++)
            d = checkbyte(d, &s);
        d++;
    } while (n-- != 0);
}

/**
 * @brief D_CheckNMoveA source_address,dest_address: one long (count 1).
 * @param s  d0: the source
 * @param d  d1: the destination
 */
static void BLITZ_ROUTINE checknmovea_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1))
{
    checknmovea2_c(s, d, 1);
}

/**
 * @brief D_CheckNMoveWord source_address,dest_address,count: count words
 * as 2 characters each, no gap; no 0, "*" below $420.
 *
 * count 0 (and $8000: the byte count count*2 is a word) runs 65536 times
 * (DBRA, BUG(orig) (#701)); the address check is signed (#717).
 * @param s      d0: the source
 * @param d      d1: the destination
 * @param count  d2.w: the number of words
 */
static void BLITZ_ROUTINE checknmoveword2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                            UWORD count BL_REG(d2))
{
    UWORD n = (UWORD)(count << 1) - 1;                  /* d7, DBRA */

    do
        d = checkbyte(d, &s);
    while (n-- != 0);
}

/**
 * @brief D_CheckNMoveWord source_address,dest_address: one word (count 1).
 * @param s  d0: the source
 * @param d  d1: the destination
 */
static void BLITZ_ROUTINE checknmoveword_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1))
{
    checknmoveword2_c(s, d, 1);
}

/**
 * @brief D_FindStringPtr(areg5,lookfor): search a long in the 32 KB below
 * an address.
 *
 * From areg5-32768 in steps of 2 up to areg5; the long at areg5 itself
 * is never compared (BUG(orig) (#705)).
 * @param areg5  d0: the end of the search
 * @param look   d1: the long to find
 * @return d0: the first address with the long, -1 if not found (not a
 *         truth value)
 */
static ULONG BLITZ_ROUTINE findstringptr_c(const UBYTE *areg5 BL_REG(d0), ULONG look BL_REG(d1))
{
    const UBYTE *p = areg5 - 32768;

    do {
        if (*(const ULONG *)p == look)
            return (ULONG)p;
        p += 2;
    } while (p != areg5);
    return (ULONG)-1;
}

/**
 * @brief PeekMax$(address,max_chars): the string at an address, at most
 * max_chars characters.
 *
 * The characters up to the 0 go to a3; at most max+1 are written, the
 * length then counts one less (a3 one behind the result, BUG(orig)
 * (#718)).  No check of low addresses.
 * @param s    d0: the address
 * @param max  d1.w: the maximum number of characters
 * @return d0: the length (counted as a word)
 */
static ULONG BLITZ_ROUTINE peekmax_c(const UBYTE *s BL_REG(d0), UWORD max BL_REG(d1))
{
    ULONG n = 0xffff;
    UBYTE c;

    for (;;) {
        BL_SETW(n, n + 1);
        c = *s++;
        if (!c)
            break;
        *blitz_string++ = c;
        if ((WORD)--max < 0)
            break;
    }
    return n;
}

/* the hex digits of GetHex$/GetHexAdr$ ('__dobyte, '__doword, '__dohex) */
static ULONG hexout(ULONG v, int mode)
{
    UWORD digits;
    ULONG n;

    if (mode == 1) {
        *blitz_string++ = hexdigits[((UBYTE)v >> 4) & 15];
        *blitz_string++ = hexdigits[v & 15];
        return 2;
    }
    if (mode == 2) {
        digits = 3;
        v = BL_SWAP(v);
    } else
        digits = 7;
    n = (ULONG)digits + 1;
    do {
        v = v << 4 | v >> 28;                           /* ROL.l #4 */
        *blitz_string++ = hexdigits[v & 15];
    } while (digits-- != 0);
    return n;
}

/**
 * @brief GetHex$(value,mode): a value as 2, 4 or 8 hex digits.
 *
 * Written at a3, no 0.
 * @param v     d0: the value
 * @param mode  d1.b: 1 byte, 2 word, otherwise long
 * @return d0: the length (2, 4, 8)
 */
static ULONG BLITZ_ROUTINE gethex_c(ULONG v BL_REG(d0), UBYTE mode BL_REG(d1))
{
    return hexout(v, mode == 1 ? 1 : mode == 2 ? 2 : 3);
}

/**
 * @brief GetHexAdr$(*value,mode): the byte, word or long at an address in
 * hex.
 *
 * As GetHex$; no check of low addresses (a word or long at an odd
 * address: an address error on a 68000).
 * @param addr  d0: the address
 * @param mode  d1.b: 1 byte, 2 word, otherwise long
 * @return d0: the length (2, 4, 8)
 */
static ULONG BLITZ_ROUTINE gethexadr_c(const UBYTE *addr BL_REG(d0), UBYTE mode BL_REG(d1))
{
    if (mode == 1)
        return hexout(*addr, 1);
    if (mode == 2)
        return hexout(*(const UWORD *)addr, 2);
    return hexout(*(const ULONG *)addr, 3);
}

/**
 * @brief GetDataItem(address,size): the byte, word or long at an address.
 *
 * Read byte by byte (also at odd addresses); size 3 or more: a long.
 * Size 0 reads 65536 bytes, a negative size fewer (-1: 65535; DBRA from
 * the word size-1, BUG(orig) (#701)).
 * @param addr  d0: the address
 * @param size  d1.w: 1 byte, 2 word, otherwise long
 * @return d0: the value, 0 for an address below $420 (a signed compare:
 *         from $80000000 too, BUG(orig) (#717))
 */
static ULONG BLITZ_ROUTINE getdataitem_c(const UBYTE *addr BL_REG(d0), UWORD size BL_REG(d1))
{
    ULONG v = 0;

    if (LOWMEM(addr))
        return 0;
    if ((WORD)size >= 3)
        size = 4;
    size--;
    do
        v = v << 8 | *addr++;
    while (size-- != 0);
    return v;
}

/**
 * @brief FindLineToken(Start_Address): the first token of a tokenised
 * line.
 *
 * A token is a byte with bit 7 and the next byte.
 * @param s  d0: the line
 * @return d0.w: the token as a word without bit 15, 0 at the end of the
 *         line; d0's high word stays (the address's)
 */
static ULONG BLITZ_ROUTINE findlinetoken_c(const UBYTE *s BL_REG(d0))
{
    ULONG result = (ULONG)s;
    UBYTE c;

    for (;;) {
        c = *s++;
        if (!c)
            return 0;
        if (c & 0x80)
            break;
    }
    BL_SETW(result, ((UWORD)c << 8 | *s) & 0x7fff);
    return result;
}

/* --------------------------------------------------------- the printer */

/* the column test: CMPI.b #80,d6 / BPL exit - go on while the byte
   difference is negative */
static int column_ok(ULONG x)
{
    return ((UBYTE)((UBYTE)x - 80) & 0x80) != 0;
}

/* the offset of the cursor in a bitplane of 80 bytes per row */
static ULONG cursor_offset(void)
{
    return (ULONG)xcurs + (ULONG)(UWORD)((UWORD)(ycurs << 3) + yoffset) * 80;
}

/**
 * @brief FLocate x,y: the cursor of the font printer.
 * @param x  d0.w: the column (8 pixels each)
 * @param y  d1.w: the row (8 pixels each)
 */
static void BLITZ_ROUTINE flocate_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1))
{
    xcurs = x;
    ycurs = y;
}

/**
 * @brief FInverse inversemode: inverse printing on or off.
 *
 * FPrint tests the word, FPrintDetoke only its high byte: FInverse 1
 * inverts FPrint only (BUG(orig) (#706)).
 * @param on  d0.w: nonzero inverse
 */
static void BLITZ_ROUTINE finverse_c(UWORD on BL_REG(d0))
{
    inverse = on;
}

/**
 * @brief FYOffset pixel_offset: the pixel offset added to the cursor's
 * row.
 * @param off  d0.w: the offset in pixel lines
 */
static void BLITZ_ROUTINE fyoffset_c(UWORD off BL_REG(d0))
{
    yoffset = off;
}

/**
 * @brief FPrint a$: print a string with the 8x8 font into plane 0 of the
 * used bitmap.
 *
 * At the cursor, 8 bytes per character in rows of 80 bytes, inverted if
 * the FInverse word is set; the cursor moves on.  The column check is a
 * byte compare with 80 tested on N: columns 80-207 stop, 208-255 print;
 * nothing checks the bitmap's size (BUG(orig) (#707)).  A string of 32769
 * characters or more prints nothing (SUBQ.w/BMI, #710).
 * @param s     d0: the string
 * @param bm    a0: the used bitmap (!libs #bitmaplib)
 * @param args  a2: the argument stack, the string's length at -4(a2)
 */
static void BLITZ_ROUTINE fprint_c(const UBYTE *s BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                   const ULONG *args BL_REG(a2))
{
    UBYTE *p = bm->Planes[0] + cursor_offset();
    UWORD x = xcurs;                                    /* d6 */
    UWORD n = (UWORD)args[-1] - 1;                      /* d5 */
    int i;

    if ((WORD)n < 0 || !column_ok(x))
        goto out;
    do {
        const UBYTE *g = font_dat + ((ULONG)*s++ << 3);

        if (inverse)
            for (i = 0; i < 8; i++)
                p[i * 80] = (UBYTE)~g[i];
        else
            for (i = 0; i < 8; i++)
                p[i * 80] = g[i];
        p++;
        x++;
        if (!column_ok(x))
            break;
    } while (n-- != 0);
out:
    xcurs = x;
}

/**
 * @brief FNPrint a$: FPrint, then the cursor to the start of the next
 * row.
 * @param s     d0: the string
 * @param bm    a0: the used bitmap (!libs #bitmaplib)
 * @param args  a2: the argument stack, the string's length at -4(a2)
 */
static void BLITZ_ROUTINE fnprint_c(const UBYTE *s BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                    const ULONG *args BL_REG(a2))
{
    fprint_c(s, bm, args);
    xcurs = 0;
    ycurs++;
}

/* draw_char of FPrintDetoke: a0 = plane 0 (drawn into), a4 = plane 1
   (moved along), d6 = the column */
struct dt {
    UBYTE *plane0, *plane1;             /* a0, a4 */
    ULONG col;                          /* d6 */
};

/* the glyph at d0 (character * 8), inverted if the high byte of inverse is
   set; returns d0 - its low byte the last glyph byte drawn inverted */
static UWORD draw_char(struct dt *t, UWORD glyph)
{
    const UBYTE *g = font_dat + (WORD)glyph;
    int i;

    if (INVERSE_HI)
        for (i = 0; i < 8; i++) {
            UBYTE b = (UBYTE)~g[i];

            t->plane0[i * 80] = b;
            glyph = (glyph & 0xff00) | b;
        }
    else
        for (i = 0; i < 8; i++)
            t->plane0[i * 80] = g[i];
    t->plane0++;
    t->plane1++;
    t->col++;
    return glyph;
}

/**
 * @brief FPrintDetoke tokenised_string,cio,typ,lineoffset: print a
 * tokenised line with the 8x8 font, its tokens as their names.
 *
 * At the cursor in plane 0 of the used bitmap; tokens (a byte with bit
 * 7 and the next) as their names from the token list of SetupLESDebug
 * ("?????" for an unknown one), also into plane 1 with SetupLESDebug's
 * tokenise; lineoffset characters skipped from the left; before the
 * character at cio a marker, '>' (typ 0) or an inverse space.  Inverse
 * printing tests the high byte of the FInverse word, the marker sets and
 * clears it (BUG(orig) (#706)); with it set and tokens drawn into two
 * planes, the second plane gets the character of the last glyph byte
 * (#709).  After the '>' marker an inverse space follows if the column
 * count (d6 with the caller's high word) became 0 (#708).  The column
 * check is FPrint's (#707).
 * @param line       d0: the tokenised line (0-terminated)
 * @param cio        d1.w: the character before which the marker goes
 * @param typ        d2.b: the marker, 0 '>', else an inverse space
 * @param lineoff    d3.w: the characters to skip
 * @param callercol  d6: the caller's d6, its high word kept in the column
 *                   count
 * @param bm         a0: the used bitmap (!libs #bitmaplib)
 */
static void BLITZ_ROUTINE fprintdetoke_c(const UBYTE *line BL_REG(d0), UWORD cio BL_REG(d1),
                                         UWORD typ BL_REG(d2), UWORD lineoff BL_REG(d3),
                                         ULONG callercol BL_REG(d6), struct BitMap *bm BL_REG(a0))
{
    struct dt t;
    UWORD tocursor = cio, skip = lineoff;
    UWORD code;                                         /* d0: the token number */
    UWORD glyph;                                        /* d0: draw_char's glyph */
    ULONG off = cursor_offset();
    const UBYTE *p = line, *name;
    UBYTE c;

    chartyp = (UBYTE)typ;
    t.plane1 = bm->Planes[1];
    t.plane0 = bm->Planes[0];
    if (!t.plane1)
        t.plane1 = t.plane0;
    t.plane0 += off;
    t.plane1 += off;
    t.col = (callercol & 0xffff0000UL) | xcurs;
    for (;;) {                                          /* 'mainloop */
        if (!column_ok(t.col))
            goto out;
        if (!tocursor && (WORD)--skip < 0) {
            if (chartyp)
                goto curins;
            draw_char(&t, '>' * 8);
            if (t.col != 0)                             /* BSR draw_char:SBNE 'con */
                goto con;
        curins:
            INVERSE_HI = 0xff;
            draw_char(&t, ' ' * 8);
            INVERSE_HI = 0;
        }
    con:
        c = *p++;
        if (!c)
            goto out;
        if (!(c & 0x80)) {
            tocursor--;
            if ((WORD)--skip >= 0)
                continue;
            draw_char(&t, (UWORD)c << 3);
            continue;
        }
        tocursor -= 2;                                  /* 'detokechars */
        code = ((UWORD)c << 8 | *p++) & 0x7fff;
        name = notoke;                                  /* 'err */
        for (ULONG n = tokens; n; n = *(const ULONG *)n)
            if (*(const UWORD *)(n + 4) == code) {
                name = (const UBYTE *)n + 6;
                break;
            }
        for (;;) {                                      /* 'loop3 */
            UBYTE *other;

            if (!column_ok(t.col))
                goto out;
            c = *name++;
            if (!c)
                break;
            if ((WORD)--skip >= 0)
                continue;
            glyph = draw_char(&t, (UWORD)c << 3);
            if (!plz_tokenise)
                continue;
            t.plane1--;                                 /* plane 1, same place */
            other = t.plane0, t.plane0 = t.plane1, t.plane1 = other;
            draw_char(&t, glyph);
            t.plane1--;
            other = t.plane0, t.plane0 = t.plane1, t.plane1 = other;
            BL_SETW(t.col, t.col - 1);
        }
    }
out:
    xcurs = (UWORD)t.col;
}

/**
 * @brief OutText x,y,rastport,address/string: print a string on a
 * RastPort.
 *
 * Move to x, y + the baseline of the rastport's font (a word add), Text
 * with the string's length (counted up to the 0).
 * @param x   d0.w: the x position
 * @param y   d1.w: the y position (of the top of the text)
 * @param rp  d2: the RastPort
 * @param s   d3: the string (an address or a string)
 */
static void BLITZ_ROUTINE outtext_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                    struct RastPort *rp BL_REG(d2), CONST_STRPTR s BL_REG(d3))
{
    const UBYTE *p = (const UBYTE *)s;
    UWORD n = 0;

    BL_SETW(y, y + rp->Font->tf_Baseline);
    Move(rp, x, y);
    while (*p++)
        n++;
    Text(rp, s, n);
}

/**
 * @brief OutTextPen rastport,pen: SetAPen on a RastPort.
 * @param rp   d0: the RastPort
 * @param pen  d1.w: the pen (SetAPen gets d0's high word, the rastport's,
 *             with the pen as the low word)
 */
static void BLITZ_ROUTINE outtextpen_c(struct RastPort *rp BL_REG(d0), UWORD pen BL_REG(d1))
{
    ULONG penarg = (ULONG)rp;

    BL_SETW(penarg, pen);
    SetAPen(rp, penarg);
}

/* ------------------------------------------------- copper and registers */

/* "(XX,YY)" of a WAIT/SKIP word: horizontal digits without bit 0 */
static void coords(UWORD w)
{
    UBYTE h = (UBYTE)w, v = (UBYTE)(w >> 8);

    *blitz_string++ = '(';
    *blitz_string++ = hexdigits[(h >> 4) & 0xe];
    *blitz_string++ = hexdigits[h & 0xe];
    *blitz_string++ = ',';
    *blitz_string++ = hexdigits[v >> 4];
    *blitz_string++ = hexdigits[v & 15];
    *blitz_string++ = ')';
}

static void outs(const char *s)
{
    while (*s)
        *blitz_string++ = (UBYTE)*s++;
}

/**
 * @brief GetCopIns$(copdata): a copper instruction as text.
 *
 * "MOVE vvvv to NAME    " (the register's 8 character name) or
 * "WAIT|SKIP (xx,yy) MASK (xx,yy)[ [BFD]]"; the horizontal digits lose
 * bit 0 of both nibbles, so odd digits never show (BUG(orig) (#711)).
 * Written at a3, no 0.
 * @param ins  d0: the instruction (two words)
 * @return d0: the length (21, 25 or 31)
 */
static ULONG BLITZ_ROUTINE getcopins_c(const UWORD *ins BL_REG(d0))
{
    UWORD ir1 = ins[0], ir2 = ins[1];
    const UBYTE *nm;
    int i;

    if (!(ir1 & 1)) {                                   /* 'moveins */
        outs("MOVE ");
        *blitz_string++ = hexdigits[ir2 >> 12];
        *blitz_string++ = hexdigits[(ir2 >> 8) & 15];
        *blitz_string++ = hexdigits[(ir2 >> 4) & 15];
        *blitz_string++ = hexdigits[ir2 & 15];
        outs(" to ");
        nm = customchipnames[(ir1 & 0x1ff) >> 1];
        for (i = 0; i < 8; i++)
            *blitz_string++ = nm[i];
        return 21;
    }
    outs(ir2 & 1 ? "SKIP " : "WAIT ");
    coords(ir1);
    outs(" MASK ");
    coords(ir2);
    if (!(ir2 & 0x8000))
        return 25;
    outs(" [BFD]");
    return 31;
}

/**
 * @brief GetCustomReg(address): the name of a custom chip register.
 *
 * The name up to its first space, at most 8 characters, at a3; a name
 * of 8 characters copies one more (the first of the next name) and
 * returns 8 (BUG(orig) (#712)).  The range test checks the word above
 * $1FC but the long for negative: a negative word with d0.l not negative
 * reads in front of the name table (#714).
 * @param reg  d0.w: the register's offset ($000-$1FC)
 * @return d0: the length, 0 for an offset out of range
 */
static ULONG BLITZ_ROUTINE getcustomreg_c(ULONG reg BL_REG(d0))
{
    const UBYTE *p;
    ULONG n;

    if ((WORD)reg > 0x1fc || (LONG)reg < 0)
        return 0;
    p = &customchipnames[0][0] + (WORD)(reg << 2);
    for (n = 0;; n++) {
        if (*p == ' ')
            break;
        *blitz_string++ = *p++;
        if ((UBYTE)n == 8)
            break;
    }
    return n;
}

/**
 * @brief GetCustomRegAdr(name$): the offset of a custom chip register.
 *
 * The first register among $000-$1FC whose name (up to a space) the
 * string starts with: a prefix match, the string may go on behind the
 * name; a name of 8 characters is never found (BUG(orig) (#713)), and
 * the last register ($1FE NOOP) is never looked at (#702).
 * @param name  d0: the name
 * @return d0: the offset, -1 if not found (not a truth value)
 */
static ULONG BLITZ_ROUTINE getcustomregadr_c(const UBYTE *name BL_REG(d0))
{
    const UBYTE *t = &customchipnames[0][0];
    ULONG reg = 0;

    do {
        const UBYTE *a = name, *b = t;

        for (;;) {
            if (*b == ' ')
                return reg;
            if (*a++ != *b++)
                break;
        }
        t += 8;
        reg += 2;
    } while ((WORD)reg <= 0x1fc);
    return (ULONG)-1;
}

/**
 * @brief GetProgMode mode,buffer: the name of a program mode.
 *
 * Six characters, no 0: "BLITZ " (< 0), "AMIGA " (0), "QAMIGA" (> 0).
 * @param mode  d0.w: the mode
 * @param buf   d1: the buffer
 */
static void BLITZ_ROUTINE getprogmode_c(WORD mode BL_REG(d0), UBYTE *buf BL_REG(d1))
{
    const UBYTE *t = mode < 0 ? blitztext : mode == 0 ? amigatext : qamigatext;
    int i;

    for (i = 0; i < 6; i++)
        buf[i] = t[i];
}

/* ------------------------------------------------------------ SearchMem */

/* RawStatus 64: the space bar down? */
static int space_down(void)
{
    struct bl_regs r = { .d0 = RAW_SPACE };

    BLITZ_ALIBJSR(TOK_rawstatus, r);
    return (UBYTE)r.d0 != 0;
}

/**
 * @brief SearchMem(type,value,start_ad,end_ad): search memory for a value.
 *
 * The first place in start..end-1 with the byte (type 1), word (2), long
 * (3, 5, 6) or string (4, value = its address); another type finds
 * nothing.  The space bar is tested (RawStatus) every 127 steps and
 * ends the search.  The string search does not go back after a partial
 * match ("ab" is not found in "aab", BUG(orig) (#723)); an empty string
 * compares the bytes behind its 0 (#721).  The word and long searches
 * compare bytes up to 1/3 behind the end (#722).
 * @param type   d0.b: the type
 * @param value  d1: the value, or the string's address
 * @param p      d2: the start
 * @param end    d3: the end (exclusive)
 * @return d0: -1 found (SearchMemFound = the address), 0 not found
 *         (SearchMemFound = 0), 1 when the space bar was down
 *         (SearchMemFound kept) - three values, so not BB_BOOL
 */
static ULONG BLITZ_ROUTINE searchmem_c(UBYTE type BL_REG(d0), ULONG value BL_REG(d1),
                                       const UBYTE *p BL_REG(d2), const UBYTE *end BL_REG(d3))
{
    UWORD steps = 127;
    ULONG l;
    UWORD w;
    const UBYTE *s, *match;

/* the macro CheckEnd */
#define CHECKEND() do {                                                     \
        if (!--steps) {                                                     \
            if (space_down())                                               \
                return 1;                                                   \
            steps = 127;                                                    \
        }                                                                   \
        if (p == end)                                                       \
            goto notfound;                                                  \
    } while (0)

    switch (type) {
    case 1:
        for (;;) {
            CHECKEND();
            if (*p == (UBYTE)value)
                goto found;
            p++;
        }
    case 2:
        w = p[0];
        for (;;) {
            CHECKEND();
            w = (UWORD)(w << 8) | p[1];
            if (w == (UWORD)value)
                goto found;
            p++;
        }
    case 3:
    case 5:
    case 6:
        l = (ULONG)p[0] << 16 | (ULONG)p[1] << 8 | p[2];
        for (;;) {
            CHECKEND();
            l = l << 8 | p[3];
            if (l == value)
                goto found;
            p++;
        }
    case 4:
        s = (const UBYTE *)value;
        match = s;
        p--;
        for (;;) {
            p++;
            CHECKEND();
            if (*match != *p) {
                match = s;
                continue;
            }
            if (*++match)
                continue;
            p -= match - (s + 1);
            goto found;
        }
    default:
        goto notfound;
    }
#undef CHECKEND
found:
    findaddress = (ULONG)p;
    return BB_TRUE;                     /* not BB_BOOL: 1 = space bar down */
notfound:
    findaddress = 0;
    return 0;
}

/**
 * @brief SearchMemFound: the address SearchMem found.
 * @return d0: the address, 0 after a search that found nothing
 */
static ULONG BLITZ_ROUTINE searchmemfound_c(void)
{
    return findaddress;
}

/* ---------------------------------------------- tokens, labels, parsing */

/**
 * @brief SetupLESDebug token_addr,tokenise: the token list for
 * FPrintDetoke.
 *
 * Also its address as 8 hex digits, ProcessString's %c.  The list: a
 * chain of entries, the next at +0, the token number at +4, the name at
 * +6.
 * @param tok       d0: the token list
 * @param tokenise  d1.b: nonzero: FPrintDetoke draws token names into
 *                  plane 1 too
 */
static void BLITZ_ROUTINE setuplesdebug_c(ULONG tok BL_REG(d0), UBYTE tokenise BL_REG(d1))
{
    int i;

    tokens = tok;
    plz_tokenise = tokenise;
    for (i = 0; i < 8; i++) {
        tok = tok << 4 | tok >> 28;
        tokenaddr[i] = hexdigits[tok & 15];
    }
}

/**
 * @brief ExtractLabel(label_string): the label name in a line.
 *
 * From 9 characters on, the name behind the first '.' up to a 0 or ':',
 * at a3.  The '.' is searched without a limit (BUG(orig) (#703)); the
 * source's test for a space is assembled as CMPI.b #0 - BUG(orig) (#704),
 * compiler bug #343, in every build - so a space does not end the label.
 * @param s  d0: the line
 * @return d0: the length of the name
 */
static ULONG BLITZ_ROUTINE extractlabel_c(const UBYTE *s BL_REG(d0))
{
    const UBYTE *name = s + 9;
    WORD n = 0;
    UBYTE c;

    while (*name++ != '.')
        ;
    for (;;) {
        c = name[n];
        if (!c || c == ':')
            break;
        n++;
    }
    for (WORD i = 0; i < n; i++)
        *blitz_string++ = name[i];
    return (UWORD)n;
}

/**
 * @brief D_InitParse string: start parsing a string of comma separated
 * arguments.
 *
 * The string is copied into the parser's 256 byte buffer without a limit
 * (BUG(orig) (#716)); the position goes to its start.
 * @param s  d0: the string
 */
static void BLITZ_ROUTINE initparse_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *d = parseadr;

    curadr = parseadr;
    while ((*d++ = *s++) != 0)
        ;
}

/**
 * @brief D_NextArg$: the next argument of D_InitParse's string.
 *
 * The characters up to the next ',' (at a3); after the last argument
 * the position is 0 and the result empty.
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE nextparsearg_c(void)
{
    const UBYTE *a = curadr;
    ULONG n = 0;
    UBYTE c;

    if (!a)
        return 0;
    for (;;) {
        c = *a++;
        if (!c) {
            curadr = NULL;
            return n;
        }
        if (c == ',') {
            curadr = (UBYTE *)a;
            return n;
        }
        *blitz_string++ = c;
        n++;
    }
}

/**
 * @brief ProcessString string: the parser's text with %x replaced.
 *
 * The text from the parser's current position (its "string" is no
 * argument), at a3, with %x replaced by the string of entry x (%p
 * "REDDebugger", %c the token list address, then AddProcessPtr's
 * entries); %% gives a '%', a %x without an entry drops x.  Reads from
 * 0 when there is no position (BUG(orig) (#719)) and goes on behind a
 * '%' at the end of the string (#720).
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE processstring_c(void)
{
    const UBYTE *a = curadr, *p;
    const struct ptok *e;
    ULONG n = 0;
    UBYTE c;

    for (;;) {
        c = *a++;
        if (!c)
            return n;
        if (c == '%') {
            c = *a++;
            if (c != '%') {                             /* 'tokloop */
                for (e = stringtokens.fixed; e->c; e++)
                    if (e->c == c) {
                        for (p = e->p; (c = *p++) != 0; n++)
                            *blitz_string++ = c;
                        break;
                    }
                continue;
            }
        }
        *blitz_string++ = c;                            /* 'putchar */
        n++;
    }
}

/**
 * @brief AddProcessPtr character,string: a new %x entry for ProcessString.
 *
 * Behind the last entry, without a limit: after 20 entries the original
 * overwrites the code of D_InitParse behind the table, the C build other
 * data (BUG(orig) (#700)).
 * @param ch  d0: the string whose first character is x
 * @param s   d1: the string %x stands for
 */
static void BLITZ_ROUTINE addprocessptr_c(const UBYTE *ch BL_REG(d0), const UBYTE *s BL_REG(d1))
{
    struct ptok *e = stringtokens.user;

    while (e->c)
        e++;
    e->c = *ch;
    e->p = s;
    e[1].c = 0;
}

/**
 * @brief ClearProcessPtrs: remove AddProcessPtr's entries.
 *
 * Only the first entry is cleared: it ends the lookup, and the next
 * AddProcessPtr reuses it (no defect).  %p and %c stay.
 */
static void BLITZ_ROUTINE clearprocessptrs_c(void)
{
    stringtokens.user[0].c = 0;
}

/* Registers: the original changes d0, d1 and a0/a1 in most routines, more
   in some (d2, d3, d5-d7, a2, a3, a6); the thunks keep d1/a0/a1, the
   statements also d0, the C code d2-d7/a2-a6. */
BLITZ_THUNK(rilesdebuglib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(rilesdebuglib_libfinit, libfinit_c, "d1/a0-a1");
BLITZ_THUNK(rilesdebuglib_finddev, finddev_c, "d1/a0-a1");
BLITZ_THUNK(_getsr, getsr_c, "d0-d1/a0-a1");
BLITZ_THUNK(_hexword, hexword_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checknmove, checknmove_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checknmove2, checknmove2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_findstringptr, findstringptr_c, "d1/a0-a1");
BLITZ_THUNK(_getcopins, getcopins_c, "d1/a0-a1");
BLITZ_THUNK(_flocate, flocate_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fprint, fprint_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnprint, fnprint_c, "d0-d1/a0-a1");
BLITZ_THUNK(_finverse, finverse_c, "d0-d1/a0-a1");
BLITZ_THUNK(_getprogmode, getprogmode_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fyoffset, fyoffset_c, "d0-d1/a0-a1");
BLITZ_THUNK(_searchmem, searchmem_c, "d1/a0-a1");
BLITZ_THUNK(_searchmemfound, searchmemfound_c, "d1/a0-a1");
BLITZ_THUNK(_peekmax, peekmax_c, "d1/a0-a1");
BLITZ_THUNK(_checknmoveword, checknmoveword_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checknmoveword2, checknmoveword2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_gethex, gethex_c, "d1/a0-a1");
BLITZ_THUNK(_gethexadr, gethexadr_c, "d1/a0-a1");
BLITZ_THUNK(_getcustomreg, getcustomreg_c, "d1/a0-a1");
BLITZ_THUNK(_getcustomregadr, getcustomregadr_c, "d1/a0-a1");
BLITZ_THUNK(_fprintdetoke, fprintdetoke_c, "d0-d1/a0-a1");
BLITZ_THUNK(_setuplesdebug, setuplesdebug_c, "d0-d1/a0-a1");
BLITZ_THUNK(_extractlabel, extractlabel_c, "d1/a0-a1");
BLITZ_THUNK(_outtext, outtext_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checknmovea, checknmovea_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checknmovea2, checknmovea2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_outtextpen, outtextpen_c, "d0-d1/a0-a1");
BLITZ_THUNK(_initparse, initparse_c, "d0-d1/a0-a1");
BLITZ_THUNK(rilesdebuglib_nextparsearg, nextparsearg_c, "d1/a0-a1");
BLITZ_THUNK(_processstring, processstring_c, "d1/a0-a1");
BLITZ_THUNK(_addprocessptr, addprocessptr_c, "d0-d1/a0-a1");
BLITZ_THUNK(_clearprocessptrs, clearprocessptrs_c, "d0-d1/a0-a1");
BLITZ_THUNK(_findlinetoken, findlinetoken_c, "d1/a0-a1");
BLITZ_THUNK(_getdataitem, getdataitem_c, "d1/a0-a1");
