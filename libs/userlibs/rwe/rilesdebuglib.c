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
 * runtime debugger (no debuglib calls; the original has no error checks,
 * the font printer's check is the port's, #707).  The debugger program
 * that used it hands it its token list (SetupLESDebug) and the tokenised
 * lines of the source (FPrintDetoke, FindLineToken).
 *   init/finit           open/close graphics.library (any version); init
 *                        also resets the state (#700, #1169)
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
 * Fixed:
 *   - GetCustomReg tests the register word unsigned; the
 *     original tested the word signed and the long for a negative value,
 *     so a negative word with d0.l not negative read in front of the name
 *     table (#714).
 *   - GetCustomReg and GetCustomRegAdr take the last register of the
 *     table, $1FE NOOP, too; the original ended both at $1FC (#702).
 *   - GetCustomReg copies at most 8 characters; the original copied a 9th
 *     behind a name of 8 (the first of the next name) and returned 8, a3
 *     one behind the result (#712).
 *   - GetCustomRegAdr finds a register only by its whole name; the
 *     original matched a prefix (the string could go on behind the
 *     table's name) and never found a name of 8 characters (#713).
 *   - The "address below $420" checks compare unsigned; the original's
 *     compared signed, so every address from $80000000 showed "*" too
 *     (D_HexWord, D_CheckNMoveA/Word) or gave 0 (GetDataItem) (#717).
 *   - A count of 0 makes D_HexWord, D_CheckNMove, D_CheckNMoveA and
 *     D_CheckNMoveWord write nothing, D_CheckNMoveWord counts its bytes
 *     as a long, and GetDataItem reads a long for every size but 1 and 2
 *     (its help text); the original's DBRA ran 65536 times for a count
 *     of 0 (D_CheckNMoveWord also for $8000, and counts above lost their
 *     top bit), and GetDataItem with size 0 shifted 65536 bytes through
 *     the result, with a negative size fewer (#701).
 *   - PeekMax$ copies at most max characters (none for a max below 1);
 *     the original tested the count after the copy: max+1 characters
 *     and the length max, a3 one behind the result (#718).
 *   - ExtractLabel gives the empty string for a line without a '.' behind
 *     its first 9 characters, or one shorter than 9; the original
 *     searched the '.' without a limit, also behind a 0 among the 9
 *     characters it skips (#703).
 *   - A space ends ExtractLabel's label too, as the source says
 *     (CMPI.b #" ",d2).  The CD's blitz2 2.10 assembles that line as
 *     CMPI.b #0 (the reference build, the CD's and AmiBlitz3's binaries
 *     alike) - compiler bug #343, triggered by the later line
 *     MOVE.w #">"*8,d0 (line 1016 of the source) - so in the original a
 *     space did not end the label (#704).
 *   - D_InitParse copies at most 255 characters and a 0 into its 256
 *     byte buffer; the original copied without a limit, over the
 *     parser's pointer and the code behind the buffer (#716).
 *   - ProcessString gives the empty string without a parser position
 *     (before D_InitParse, after the last D_NextArg$), as D_NextArg$; the
 *     original read the text at address 0.  Its help text names no
 *     argument (the original's "string" was none: the command works on
 *     what D_InitParse stored) (#719).
 *   - ProcessString ends at a '%' at the end of the text; the original
 *     took the 0 behind it as an entry's name and read on behind the
 *     text (#720).
 *   - SearchMem's word and long searches find a value only if it lies in
 *     front of the end; the original tested only its start against the
 *     end and compared bytes up to 1 or 3 behind it (#722).
 *   - SearchMem's string search goes on one byte behind the start of a
 *     partial match after a mismatch; the original went on behind the
 *     mismatch ("ab" was not found in "aab") (#723).
 *   - SearchMem with an empty string finds nothing (0); the original
 *     compared its 0 and the bytes behind it (#721).
 *   - GetCopIns$ shows the high digit of a horizontal position (WAIT,
 *     SKIP and their masks) whole; the original cleared its bit 0 too, as
 *     that of the low digit (the instruction or SKIP bit), so its odd
 *     values never showed (#711).
 *   - FPrint counts the string's length as a long; the original's
 *     SUBQ.w/BMI printed nothing for 32769 characters or more (#710).
 *   - FPrintDetoke's '>' marker is never followed by the inverse space;
 *     the original drew it too when the column count (d6 with the
 *     caller's high word) became 0 (#708).
 *   - FPrintDetoke tests the FInverse word, as FPrint, and its inverse
 *     space marker leaves it as it was; the original tested the word's
 *     high byte (FInverse 1 inverted FPrint only), and the marker set and
 *     cleared that byte, which turned FInverse $100 off (#706).
 *   - FPrintDetoke draws a token's character into plane 1 with its own
 *     glyph; with inverse printing the original took the glyph offset
 *     from d0, whose low byte the inverse drawing had overwritten with
 *     the last glyph byte: another character (#709).
 *   - FPrint, FNPrint and FPrintDetoke compare the column as an unsigned
 *     word: from column 80 on nothing is printed, also not the character
 *     behind FPrintDetoke's marker; the original compared the low byte on
 *     the sign of the difference (CMPI.b #80 / BPL: columns 208-255
 *     printed on into the next rows) and drew the character behind the
 *     marker without the test.  With runtime error checking the bitmap's
 *     size is checked (fprintchk): "Font printer needs a bitmap of 640
 *     pixels", "Font printer row outside the bitmap"; the original never
 *     looked at it (#707).
 *   - AddProcessPtr's table holds 20 entries: a 21st is ignored, and
 *     with runtime error checking refused (addprocessptrchk): "AddProcessPtr
 *     table holds at most 20 entries"; init clears the 20 entries.  The
 *     original had no limit: the 21st went over the end mark and
 *     overwrote the code of D_InitParse behind the table (the C build:
 *     other data, its graphics.library base) (#700).
 *   - D_FindVolume returns -1 if the name is there, else 0, as
 *     riamosfunclib's FindVolume; the original returned the entry it had
 *     found after unlocking the list (#1167).
 *   - GetHexAdr$ reads a word or long byte by byte; the original's
 *     MOVE.w/MOVE.l was an address error on a 68000 at an odd address
 *     (#1168).
 *   - init sets the state to its first load's values (the cursor,
 *     FInverse, FYOffset, SearchMemFound, the token list and tokenise,
 *     the parser position, the %c text); the original kept the run
 *     before's in a second run from the editor (#1169).
 * Like the original, no defects: ClearProcessPtrs only clears the first
 * entry (it ends the lookup, and the next AddProcessPtr reuses it); the
 * %p and %c entries are set by init and point to 0 before it (init
 * always runs first); a %x without an entry drops x (as the source
 * intends).  D_HexWord at an odd source writes "XX YY" per loop: the lone
 * first byte, then the words at the even addresses ("12 3456 78" for the
 * bytes 12 34 56 78), 5 characters per word as in the even case - a path
 * of its own that keeps the memory's word boundaries (#715).
 * D_FindStringPtr searches the 32 KB below areg5, up to areg5-2: the
 * long at areg5 is the first of the globals above a5 (varslib's a5 is
 * the globals' block + $8000), which its caller does not search (#705).
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
#include "blitzdos.h"
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
void _getdataitem(void), rilesdebuglib_libinit(void), _fprintchk(void);
void _addprocessptrchk(void);
void rilesdebuglib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#riLESDEBUGlib,init,0,finit,0}; error field: the error
   check area (fixed, #707; the original has none) */
BLITZ_LIBHEADER(LIB_riLESDEBUGlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_doslib, BL_LA6);
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
BLITZ_LIBS(LIB_bitmaplib, BL_UA0);
BLITZ_SUBS(_fprint, _fprintchk, 0);                    /* check: fixed #707 */
BLITZ_NAME("FPrint", "a$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_bitmaplib, BL_UA0);
BLITZ_SUBS(_fnprint, _fprintchk, 0);                   /* check: fixed #707 */
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
BLITZ_LIBS(LIB_bitmaplib, BL_UA0);
BLITZ_SUBS(_fprintdetoke, _fprintchk, 0);              /* check: fixed #707 */
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
/* fixed (#719): the help text named an argument "string" */
BLITZ_NAME("ProcessString", "- the text of D_InitParse with %x replaced");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addprocessptr, _addprocessptrchk, 0);      /* check: fixed #700 */
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
#define USERTOKENS 20
static struct {
    struct ptok fixed[2], user[USERTOKENS];
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

/* "address below $420", unsigned (fixed #717: the original's CMP.l/BLT
   took every address from $80000000 for low memory too) */
#define LOWMEM(p) ((ULONG)(p) < 0x420)

/* --------------------------------------------------------- init, finit */

/**
 * @brief init: open graphics.library (any version) and set ProcessString's
 * %p ("REDDebugger") and %c (the token list address in hex) entries.
 *
 * AddProcessPtr's 20 entries are cleared (fixed #700: a run from the
 * editor started with the run before's entries, which counted toward the
 * limit and pointed into its freed strings).  The rest of the state gets
 * its first load's values too (fixed #1169: a second run started with
 * the run before's cursor, FInverse, FYOffset, SearchMemFound, token
 * list and tokenise, parser position and %c text): the library stays
 * loaded between two runs of a program from the editor.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    int i;

    gfxbase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    stringtokens.fixed[0].p = progname;
    stringtokens.fixed[1].p = tokenaddr;
    for (i = 0; i < USERTOKENS; i++) {                  /* fixed (#700) */
        stringtokens.user[i].c = 0;
        stringtokens.user[i].p = NULL;
    }
    xcurs = ycurs = inverse = yoffset = 0;              /* fixed (#1169) */
    tokens = 0;
    chartyp = plz_tokenise = 0;
    findaddress = 0;
    curadr = NULL;
    for (i = 0; i < (int)sizeof tokenaddr; i++)
        tokenaddr[i] = 0;
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
 * volumes and assigns.  Fixed (#1148): below dos.library V36 (no
 * LockDosList) the device list of DosInfo is walked under Forbid; the
 * original called the V36 functions there, a crash on Kickstart 1.x.
 * @param name    d0: the name (without the colon)
 * @param dosbase a6: dos.library (!libs #doslib)
 * @return d0: -1 if the name is there, 0 if not (or LockDosList failed)
 *         (fixed #1167: the original returned the entry it found, after
 *         the unlock - below V36 after Permit - an entry of a list the
 *         program no longer held), as riamosfunclib's FindVolume
 */
#define FINDFLAGS (LDF_READ | LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_ENTRY)
static ULONG BLITZ_ROUTINE finddev_c(CONST_STRPTR name BL_REG(d0),
                                     struct DosLibrary *dosbase BL_REG(a6))
{
    struct DosList *dl;

    if (dosbase->dl_lib.lib_Version < 36) {     /* fixed (#1148) */
        Forbid();
        dl = bl_finddosentry_v34(dosbase, (const UBYTE *)name,
                                 LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS);
        Permit();
        return dl ? BB_TRUE : 0;                /* fixed (#1167) */
    }
    dl = LockDosList(FINDFLAGS);
    if (!dl)
        return 0;
    dl = FindDosEntry(dl, name, FINDFLAGS);
    UnLockDosList(FINDFLAGS);
    return dl ? BB_TRUE : 0;                    /* fixed (#1167) */
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
 * "XXXX " each; at an odd source "XX YY" per loop, the words at the even
 * addresses (like the original, no defect, #715).  Bytes at addresses
 * below $420 show "*" (an unsigned compare, fixed #717).  No 0 is
 * written.  num 0 writes nothing (fixed #701: the original's DBRA ran
 * 65536 times).
 * @param src  d0: the source address
 * @param dst  d1: the buffer
 * @param num  d2.w: the number of words
 */
static void BLITZ_ROUTINE hexword_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                    UWORD num BL_REG(d2))
{
    UWORD n = num - 1;                                  /* d6, DBRA */

    if (!num)                                           /* fixed (#701) */
        return;
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
 * copies nothing (fixed #701: the original's DBRA ran 65536 times).
 * @param s      d0: the source
 * @param d      d1: the destination (5 bytes per long)
 * @param count  d2.w: the number of longs
 */
static void BLITZ_ROUTINE checknmove2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                        UWORD count BL_REG(d2))
{
    UWORD n = count - 1;                                /* d7, DBRA */

    if (!count)                                         /* fixed (#701) */
        return;
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
 * A 0 byte becomes 1; an address below $420 (unsigned compare, fixed
 * #717) gives '*'.  count 0 writes nothing (fixed #701: the original's
 * DBRA ran 65536 times).
 * @param s      d0: the source
 * @param d      d1: the destination (5 bytes per long)
 * @param count  d2.w: the number of longs
 */
static void BLITZ_ROUTINE checknmovea2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                         UWORD count BL_REG(d2))
{
    UWORD n = count - 1;                                /* d7, DBRA */
    int i;

    if (!count)                                         /* fixed (#701) */
        return;
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
 * count 0 writes nothing, and the byte count count*2 is a long (fixed
 * #701: the original's DBRA of the word count*2-1 ran 65536 times for 0
 * and $8000, and counts above $8000 lost their top bit); the address
 * check is unsigned (fixed #717).
 * @param s      d0: the source
 * @param d      d1: the destination
 * @param count  d2.w: the number of words
 */
static void BLITZ_ROUTINE checknmoveword2_c(const UBYTE *s BL_REG(d0), UBYTE *d BL_REG(d1),
                                            UWORD count BL_REG(d2))
{
    ULONG n = (ULONG)count << 1;                        /* fixed (#701) */

    while (n--)
        d = checkbyte(d, &s);
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
 * is not compared (like the original, no defect: the 32 KB below a5 are
 * what the caller searches, #705).
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
 * The characters up to the 0, at most max, go to a3; a max below 1 gives
 * the empty string (fixed #718: the original tested the count after the
 * copy, wrote max+1 characters and returned the length max, a3 one
 * behind the result).  No check of low addresses.
 * @param s    d0: the address
 * @param max  d1.w: the maximum number of characters
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE peekmax_c(const UBYTE *s BL_REG(d0), UWORD max BL_REG(d1))
{
    ULONG n = 0;
    UBYTE c;

    while ((LONG)n < (WORD)max && (c = *s++) != 0) {   /* fixed (#718) */
        *blitz_string++ = c;
        n++;
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
 * As GetHex$; no check of low addresses.  The word or long is read byte
 * by byte, as GetDataItem does, so an odd address works on a 68000 too
 * (fixed #1168: the original read it with one MOVE.w or MOVE.l, an
 * address error there).
 * @param addr  d0: the address
 * @param mode  d1.b: 1 byte, 2 word, otherwise long
 * @return d0: the length (2, 4, 8)
 */
static ULONG BLITZ_ROUTINE gethexadr_c(const UBYTE *addr BL_REG(d0), UBYTE mode BL_REG(d1))
{
    if (mode == 1)
        return hexout(*addr, 1);
    if (mode == 2)                                      /* fixed (#1168) */
        return hexout((ULONG)addr[0] << 8 | addr[1], 2);
    return hexout((ULONG)addr[0] << 24 | (ULONG)addr[1] << 16 |
                  (ULONG)addr[2] << 8 | addr[3], 3);
}

/**
 * @brief GetDataItem(address,size): the byte, word or long at an address.
 *
 * Read byte by byte (also at odd addresses); every size but 1 and 2 is a
 * long, as the help text says (fixed #701: the original read a long only
 * from size 3 on, size 0 shifted 65536 bytes through the result and a
 * negative size fewer, -1 65535, by a DBRA from the word size-1).
 * @param addr  d0: the address
 * @param size  d1.w: 1 byte, 2 word, otherwise long
 * @return d0: the value, 0 for an address below $420 (an unsigned
 *         compare, fixed #717)
 */
static ULONG BLITZ_ROUTINE getdataitem_c(const UBYTE *addr BL_REG(d0), UWORD size BL_REG(d1))
{
    ULONG v = 0;

    if (LOWMEM(addr))
        return 0;
    if (size != 1 && size != 2)                         /* fixed (#701) */
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

/* the column test: the column as an unsigned word below 80 (fixed #707:
   the original's CMPI.b #80,d6 / BPL compared the low byte on the sign of
   the difference, so columns 208-255, and every word whose low byte is
   one of them or below 80, printed on) */
static int column_ok(ULONG x)
{
    return (UWORD)x < 80;
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
 * FPrint and FPrintDetoke test the word (fixed #706: FPrintDetoke tested
 * only its high byte, FInverse 1 inverted FPrint only).
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
 * the FInverse word is set; the cursor moves on.  Columns of 80 and more
 * print nothing (fixed #707: the original compared the column's low byte
 * on the sign of the difference, columns 208-255 printed on); the
 * bitmap's size is checked only with runtime error checking (fprintchk,
 * fixed #707).  The length is counted as a long (fixed #710: the
 * original's SUBQ.w/BMI printed nothing for a string of 32769 characters
 * or more).
 * @param s     d0: the string
 * @param bm    a0: the used bitmap (!libs #bitmaplib)
 * @param args  a2: the argument stack, the string's length at -4(a2)
 */
static void BLITZ_ROUTINE fprint_c(const UBYTE *s BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                   const ULONG *args BL_REG(a2))
{
    UBYTE *p = bm->Planes[0] + cursor_offset();
    UWORD x = xcurs;                                    /* d6 */
    ULONG n = args[-1];                                 /* fixed (#710) */
    int i;

    if (!n || !column_ok(x))
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
    } while (--n != 0);
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

/* the glyph at d0 (character * 8), inverted if the FInverse word is set
   (fixed #706: the original tested its high byte); d0 stays (fixed
   #709: the original's inverse drawing left the last glyph byte in its
   low byte) */
static void draw_char(struct dt *t, UWORD glyph)
{
    const UBYTE *g = font_dat + (WORD)glyph;
    int i;

    if (inverse)                                        /* fixed (#706) */
        for (i = 0; i < 8; i++)
            t->plane0[i * 80] = (UBYTE)~g[i];
    else
        for (i = 0; i < 8; i++)
            t->plane0[i * 80] = g[i];
    t->plane0++;
    t->plane1++;
    t->col++;
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
 * printing tests the FInverse word, and the marker leaves it as it was
 * (fixed #706: the original tested its high byte, so FInverse 1 did not
 * invert here, and the marker set and cleared that byte); with it set and
 * tokens drawn into two planes, plane 1 gets the same inverted glyphs
 * (fixed #709: the original gave it the glyph of the character of the
 * last glyph byte).  The '>' marker is never followed by the inverse
 * space (fixed #708: the original's BSR draw_char : SBNE 'con drew it too
 * when the column count, d6 with the caller's high word, became 0).  The
 * column check is FPrint's, also for the character behind a marker
 * (fixed #707: the original drew that one without the test).
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
            goto marked;                                /* fixed (#708) */
        curins: {
            UWORD inv = inverse;                        /* fixed (#706) */

            inverse = 0xff00;
            draw_char(&t, ' ' * 8);
            inverse = inv;
        }
        marked:
            /* fixed (#707): the character behind the marker */
            if (!column_ok(t.col))
                goto out;
        }
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
            draw_char(&t, (UWORD)c << 3);
            if (!plz_tokenise)
                continue;
            t.plane1--;                                 /* plane 1, same place */
            other = t.plane0, t.plane0 = t.plane1, t.plane1 = other;
            draw_char(&t, (UWORD)c << 3);              /* fixed (#709) */
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

/* "(XX,YY)" of a WAIT/SKIP word: the low horizontal digit without bit 0
   (the instruction bit, and SKIP in the mask word) */
static void coords(UWORD w)
{
    UBYTE h = (UBYTE)w, v = (UBYTE)(w >> 8);

    *blitz_string++ = '(';
    *blitz_string++ = hexdigits[h >> 4];               /* fixed (#711) */
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
 * "WAIT|SKIP (xx,yy) MASK (xx,yy)[ [BFD]]"; the low horizontal digit
 * without bit 0, the instruction bit (fixed #711: the original cleared
 * bit 0 of the high digit too, its odd values never showed).
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
 * The name up to its first space, at most 8 characters, at a3 (fixed
 * #712: the original stopped only after a 9th character, so a name of 8
 * characters got the first one of the next name behind it, a3 one behind
 * the result of the length 8).  The range is $000-$1FE (fixed #702: the
 * original stopped at $1FC, the last register NOOP had no name); the
 * range test compares the word unsigned (fixed #714: the original tested
 * the word signed and the long for negative, so a negative word with d0.l
 * not negative read in front of the name table); the high word is not
 * looked at.
 * @param reg  d0.w: the register's offset ($000-$1FE)
 * @return d0: the length, 0 for an offset out of range
 */
static ULONG BLITZ_ROUTINE getcustomreg_c(ULONG reg BL_REG(d0))
{
    const UBYTE *p;
    ULONG n;

    if ((UWORD)reg > 0x1fe)                     /* fixed (#714, #702) */
        return 0;
    p = &customchipnames[0][0] + ((UWORD)reg << 2);
    for (n = 0; n < 8 && *p != ' '; n++)               /* fixed (#712) */
        *blitz_string++ = *p++;
    return n;
}

/**
 * @brief GetCustomRegAdr(name$): the offset of a custom chip register.
 *
 * The first register among $000-$1FE whose name (up to a space, at most
 * 8 characters) the string is (fixed #713: the original matched a prefix,
 * the string could go on behind the name, and never found a name of 8
 * characters, its compare ran into the next name).  The last register,
 * $1FE NOOP, is looked at too (fixed #702: the original stopped at $1FC).
 * @param name  d0: the name
 * @return d0: the offset, -1 if not found (not a truth value)
 */
static ULONG BLITZ_ROUTINE getcustomregadr_c(const UBYTE *name BL_REG(d0))
{
    const UBYTE *t = &customchipnames[0][0];
    ULONG reg = 0;

    do {
        int i;

        for (i = 0; i < 8 && t[i] != ' '; i++)          /* fixed (#713) */
            if (name[i] != t[i])
                break;
        if ((i == 8 || t[i] == ' ') && !name[i])
            return reg;
        t += 8;
        reg += 2;
    } while ((WORD)reg <= 0x1fe);                    /* fixed (#702) */
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
 * ends the search.  After a mismatch behind a partial match the string
 * search goes on one byte behind the partial match's start (fixed #723:
 * the original went on behind the mismatch, "ab" was not found in "aab");
 * an empty string finds nothing (fixed #721: the original compared its
 * 0 and the bytes behind it).  A word or long counts only if it lies in
 * front of the end (fixed #722: the original compared bytes up to 1 or 3
 * behind the end).
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
        /* fixed (#722): the word must lie in front of the end */
        if ((ULONG)(end - p) < 2)
            goto notfound;
        end -= 1;
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
        /* fixed (#722): the long must lie in front of the end */
        if ((ULONG)(end - p) < 4)
            goto notfound;
        end -= 3;
        l =(ULONG)p[0] << 16 | (ULONG)p[1] << 8 | p[2];
        for (;;) {
            CHECKEND();
            l = l << 8 | p[3];
            if (l == value)
                goto found;
            p++;
        }
    case 4:
        s = (const UBYTE *)value;
        if (!*s)                        /* fixed (#721): finds nothing */
            goto notfound;
        match = s;
        p--;
        for (;;) {
            p++;
            CHECKEND();
            if (*match != *p) {
                /* fixed (#723): on one byte behind the partial match's
                   start */
                p -= match - s;
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
 * From 9 characters on, the name behind the first '.' up to a 0, ':' or
 * space, at a3; a line without a '.' there (or shorter than 9 characters) gives
 * the empty string (fixed #703: the original searched the '.' without a
 * limit, also behind a 0 among the 9 characters it skips).  The space as
 * the source says (fixed #704: the CD's blitz2 2.10 assembled the
 * source's test for it as CMPI.b #0, compiler bug #343, fixed in
 * blitz2-c; in every binary a space did not end the label).
 * @param s  d0: the line
 * @return d0: the length of the name
 */
static ULONG BLITZ_ROUTINE extractlabel_c(const UBYTE *s BL_REG(d0))
{
    const UBYTE *name = s;
    WORD n = 0;
    UBYTE c;

    for (n = 0; n < 9; n++)                             /* fixed (#703) */
        if (!*name++)
            return 0;
    do {
        c = *name++;
        if (!c)                                         /* fixed (#703) */
            return 0;
    } while (c != '.');
    n = 0;
    for (;;) {
        c = name[n];
        if (!c || c == ':' || c == ' ')                 /* fixed (#704) */
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
 * At most 255 characters of the string are copied into the parser's 256
 * byte buffer, with a 0 behind them (fixed #716: the original copied
 * without a limit, over the parser's pointer and code behind the
 * buffer); the position goes to its start.
 * @param s  d0: the string
 */
static void BLITZ_ROUTINE initparse_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *d = parseadr;

    curadr = parseadr;
    /* fixed (#716): at most 255 characters */
    while (d < parseadr + sizeof parseadr - 1 && (*d = *s++) != 0)
        d++;
    *d = 0;
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
 * @brief ProcessString: the parser's text with %x replaced.
 *
 * The text from the parser's current position (no argument: the command
 * works on what D_InitParse stored, as its caller uses it), at a3, with
 * %x replaced by the string of entry x (%p "REDDebugger", %c the token
 * list address, then AddProcessPtr's entries); %% gives a '%', a %x
 * without an entry drops x.  Without a parser position (before
 * D_InitParse, after the last D_NextArg$) the result is empty, as
 * D_NextArg$'s (fixed #719: the original read the text at address 0, and
 * its help text named an argument "string").  A '%' at the end of the
 * text ends it (fixed #720: the original took the 0 behind it as the
 * entry's name and went on behind the text).
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE processstring_c(void)
{
    const UBYTE *a = curadr, *p;
    const struct ptok *e;
    ULONG n = 0;
    UBYTE c;

    if (!a)                                             /* fixed (#719) */
        return 0;
    for (;;) {
        c = *a++;
        if (!c)
            return n;
        if (c == '%') {
            c = *a++;
            if (!c)                                     /* fixed (#720) */
                return n;
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
 * Behind the last entry; the table holds 20, a 21st is ignored (with
 * runtime error checking refused by addprocessptrchk) (fixed #700: the
 * original had no limit, the 21st entry went over the end mark and
 * overwrote the code of D_InitParse behind the table).
 * @param ch  d0: the string whose first character is x
 * @param s   d1: the string %x stands for
 */
static void BLITZ_ROUTINE addprocessptr_c(const UBYTE *ch BL_REG(d0), const UBYTE *s BL_REG(d1))
{
    struct ptok *e = stringtokens.user;
    struct ptok *const full = stringtokens.user + USERTOKENS;

    while (e < full && e->c)                            /* fixed (#700) */
        e++;
    if (e == full)
        return;
    e->c = *ch;
    e->p = s;
    if (e + 1 < full)                   /* the 20th: the end mark is 0 */
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

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(narrowbitmap, "Font printer needs a bitmap of 640 pixels");  /* fixed, #707 */
BLITZ_ERRCHK_MESSAGE(rowoutside, "Font printer row outside the bitmap");          /* fixed, #707 */

/**
 * @brief The error check of FPrint, FNPrint and FPrintDetoke (debugger
 * only).
 *
 * Fixed (#707), not in the original (rilesdebuglib had no error check
 * area): the font printer writes rows of 80 bytes, 8 rows of pixels per
 * character, at the cursor's row plus FYOffset, and never looked at the
 * used bitmap's size: in a narrower bitmap the characters landed in the
 * wrong rows, and rows below the bitmap were written.  Arguments the
 * programmer controls (D-005): the release code prints as before.
 *
 * Errors: "Font printer needs a bitmap of 640 pixels" for fewer than 80
 * bytes per row; "Font printer row outside the bitmap" when the 8 pixel
 * rows at the cursor do not all lie in the bitmap.
 * @param bm  a0: the used bitmap (!libs #bitmaplib,#ua0)
 */
static void BLITZ_ERRCHK fprintchk_c(const struct BitMap *bm BL_REG(a0))
{
    if (bm->BytesPerRow < 80)
        BLITZ_RUNERR(narrowbitmap);
    if ((ULONG)(UWORD)((UWORD)(ycurs << 3) + yoffset) + 8 > bm->Rows)
        BLITZ_RUNERR(rowoutside);
}

BLITZ_ERRCHK_MESSAGE(tablefull, "AddProcessPtr table holds at most 20 entries");  /* fixed, #700 */

/**
 * @brief The error check of AddProcessPtr (debugger only).
 *
 * Fixed (#700), not in the original: the table has room for 20 entries;
 * the release code ignores further ones, the check tells the programmer.
 *
 * Errors: "AddProcessPtr table holds at most 20 entries" when all 20 are
 * in use.
 */
static void BLITZ_ERRCHK addprocessptrchk_c(void)
{
    int i;

    for (i = 0; i < USERTOKENS; i++)
        if (!stringtokens.user[i].c)
            return;
    BLITZ_RUNERR(tablefull);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(_fprintchk, fprintchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_addprocessptrchk, addprocessptrchk_c, "d0-d1/a0-a1");
