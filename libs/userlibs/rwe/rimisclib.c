/*
 * rimisclib - RWE userlib 47 (Reflective Images, font prefs and fixes by
 * Steve Matty): miscellaneous commands - TimerReset, PortSigBit,
 * WorkbenchHere, EZRequest, Flip$, InitArgParse, NextArg$, EZOutput,
 * EZRequestArgs, EZSeparator, ClickWait, ProgPath$, ProgDir$, ProgName$,
 * FromBlitz, LtoS, ASCRaw, RawASC, OpenFontPrefs, CloseFontPrefs,
 * DefaultFontHeight, DefaultFontName, NextArgChar$, BInstr.
 *
 * C translation of rwelibsrc/misclib/misclib2.bb (1997-03-13, newer than
 * the CD's BSS2 RIMISCLIB.OBJ, docs/rwelibs/README.md 3.2); reference the
 * reference build of that source by blitz2 2.10 (refbuild).  Notes:
 * docs/rwelibs/port/rimisclib.md.
 *
 * init opens dos.library and intuition.library (any version); finit
 * closes both and frees the font prefs buffer.
 *   EZRequest            EasyRequestArgs on the EZOutput window (0: the
 *                        default screen); '|' in the body becomes a line
 *                        feed; with arguments (title$,body$,gadget$,
 *                        var..) they are the format arguments; below
 *                        intuition V36 AutoRequest (fixed #1150)
 *   EZRequestArgs args.. the body is built from the arguments: %s / %ld /
 *                        %d per type, separated by the EZSeparator
 *                        character ('|': a line feed), gadget "Ok"
 *   InitArgParse s$      copies s$ to a 256 byte buffer (at most 255
 *                        characters); NextArg$ gives
 *                        the next word (quotes group, "" inside quotes is
 *                        a quote), NextArgChar$ c everything up to c
 *   ProgPath$/Dir$/Name$ GetProgramDir/GetProgramName (the task name if
 *                        there is no program name); below dos V36 the
 *                        current directory and the shell's command name
 *                        (fixed #1150)
 *   OpenFontPrefs        reads ENV:SYS/Font.Prefs into a 16K buffer;
 *                        DefaultFontHeight/Name(type) search its FONT
 *                        chunks (fp_Type), CloseFontPrefs frees it
 *   Flip$, LtoS, ASCRaw, RawASC, BInstr, PortSigBit, FromBlitz,
 *   WorkbenchHere, TimerReset (CIA-A TOD), ClickWait (left mouse button)
 *
 * Everything is C behind the thunks; dos and intuition through the NDK
 * inlines.
 *
 * Kept on purpose (not fixed, decision):
 *   - BUG(orig) (#746): EZRequestArgs without arguments reads one type word from a2 (the
 *     count 0 runs the DBRA loop 65536 times).
 *
 * Fixed (intended deviation, #748): EZRequest title$,body$,gadget$,var..
 * and EZRequestArgs store their arguments in 16 longs without a limit;
 * an invalid argument (the number is in the source): the commands are
 * unchanged, the library has an error check area (the original has
 * none) whose checks ezargschk and ezreqargschk stop more than 64 bytes
 * of arguments with "Too Many EZRequest Arguments" (debugger only).
 * Fixed (intended deviation, #754): NextArgChar$ before InitArgParse and
 * after NextArg$ came to the end gives "", as NextArg$ does; the original
 * did not check the parse pointer and read from address 0.
 * Fixed (intended deviation, #744): ASCRaw of a raw key outside 0..113
 * (a key up code, a negative value) gives the character 0, RawASC of a
 * character not in the table and of "" gives -1.  The original did not
 * check the range: ASCRaw read the code behind (or before) its table,
 * RawASC searched on behind it until it found the byte somewhere (or for
 * ever) and found "" (the 0) at 14.
 * Fixed (intended deviation, #745): DefaultFontHeight/Name go through the
 * chunks by their sizes as far as the bytes read, and take only FONT
 * chunks; OpenFontPrefs checks the PRHD size against them and fails on
 * a read error.  The original stepped by 164 bytes (the size of a FONT
 * chunk) to the end of the 16 KB buffer, through the uncleared memory
 * behind the file (a type not in the file was "found" there), reading
 * the last step up to 26 bytes behind the buffer.
 * Fixed (intended deviation, #757): OpenFontPrefs frees the buffer of an
 * OpenFontPrefs before; the original lost it (16 KB of AllocMem until
 * the next reboot).
 * Fixed (intended deviation, #747): EZRequestArgs ends the format text
 * it builds with a 0; the original wrote none, so a shorter text than
 * the one before kept the end of the old one (more formats than
 * arguments).
 * Fixed (intended deviation, #756): OpenFontPrefs returns 0 when the
 * AllocMem of its 16 KB buffer fails; the original did not check it and
 * read ENV:SYS/Font.Prefs to address 0 (over the exception vectors and
 * the ExecBase pointer).
 * Fixed (intended deviation, #758): ProgPath$ without a directory name
 * is the program name alone; the original decided on the "/" by the byte
 * before its result (CMPI.b #":",-1(a3)), so it gave "/name" or, behind
 * a ":", "name".
 * Fixed (intended deviation, #750, #749): a Quick or Float argument of
 * EZRequest/EZRequestArgs is passed as its integer part truncated
 * towards 0, a long (-3.5 gives -3), the Float converted in C.  The
 * original passed the integer part zero extended (SWAP:ANDI.l #$ffff:
 * -3.5 gave 65532) (#750), and converted a Float with floatquicklib's
 * fltoqu, whose mathffp calls went through the caller's a6 (the
 * commands load no library base), writing the Quick back onto the
 * argument stack (#749).
 * Fixed (intended deviation, #751): Flip$ of a string with 0 or 1
 * characters returns it unchanged; the original swapped 65536 byte
 * pairs around the string (the count len/2 - 1 = -1 in a DBRA).
 * Fixed (intended deviation, #759): WorkbenchHere searches the screens
 * of the library's own IntuitionBase; the original took it from d0
 * (MOVE.l intuibase,a0 overwritten by MOVE.l d0,a0), and the function
 * has no arguments: d0 was whatever the program left there.
 * Fixed (intended deviation, #761): WorkbenchHere ends at the end of the
 * screen list and returns 0 there, and skips a screen without a title;
 * the original never ended the list (no 0 check: it read the "screen"
 * at address 0 and went on), so it returned -1 or hung.
 * Fixed (intended deviation, #762): WorkbenchHere goes on with the next
 * screen when a title ends during a partial match; the original
 * restarted the same title ('loop with the same screen), so a title that
 * ends with the start of "Workbench" ("...Work") hung it.
 * Fixed (intended deviation, #760): WorkbenchHere searches the whole
 * title, from every character on; the original went on with the next
 * screen at the first mismatch after a "W", so "A Window on Workbench"
 * or "WWorkbench" was not found.
 * Fixed (intended deviation, #755): NextArg$ takes "" inside quotes as
 * one quote and ends the word at the quote that closes it (the original
 * miscounted the quotes after "" and copied the character behind the
 * closing quote); it stays on the 0 at the end of the string.
 * Fixed (intended deviation, #752): InitArgParse copies at most 255
 * characters and the 0 into its 256 byte buffer (the original copies
 * without a limit and overwrites the code behind the buffer); the rest
 * of a longer string is cut off.
 * Fixed (intended deviation, #743): ProgPath$, ProgDir$, ProgName$ and
 * DefaultFontName copied their result with its 0 and left a3 behind the
 * 0, so a string expression went on one byte too far; a3 now ends on
 * the 0.
 * Fixed (intended deviation, #1027): EZRequest and EZRequestArgs left a3
 * on the argument array they pass to EasyRequestArgs (0 for the forms
 * without one); a3 now ends on the first string argument (title$, body$
 * without a title, for EZRequestArgs the first string of the arguments,
 * unchanged without one), giving back the space of the string arguments,
 * which the compiler does not take back after a function with a numeric
 * result (#846).  The call keeps a3 (the libraries' safe form of the NDK
 * macro, cmake/ndk-calls.cmake).
 * Fixed (intended deviation, #1150): EZRequest/EZRequestArgs called
 * EasyRequestArgs (intuition V36) and ProgPath$/ProgDir$/ProgName$
 * GetProgramDir, GetCurrentDirName, NameFromLock and GetProgramName (dos
 * V36) with both libraries opened in any version: on Kickstart 1.x a
 * crash.  Below intuition V36 the library formats body and gadgets
 * itself (RawDoFmt, safe form) and calls AutoRequest with one or two
 * gadgets, the title not shown; more than two gadgets: no requester, 0,
 * and with runtime error checking "More than two EZRequest gadgets need
 * Kickstart 2.0" (ezgad3chk, ezgad2chk, ezargschk).  Below dos V36 the
 * directory is the current directory, its path built with Examine and
 * ParentDir, the name the shell's command name (the task name without a
 * shell): a program started with a path from another directory gets the
 * current directory, not its own (the user's decision of 2026-11-20).
 *
 * Original behaviour that is no defect: LtoS replaces the bytes 0-31 and
 * 128-255 by 128 (a signed compare), as its documentation says
 * (RIMiscLib.guide, LtoS: "Non-ASCII characters are replaced with
 * character 128"; #753).  NextArg$ ends a quoted word at
 * the closing quote even if no space follows ("ab"cd gives ab, then cd),
 * as ReadItem does.  NextArg$/NextArgChar$ write no 0 behind the result
 * (string results need none: d0 has the length, the compiler adds the 0
 * where it passes the result on, see #257).  EZRequest/EZRequestArgs
 * replace '|' in the body string they get, which is the compiler's copy
 * of the argument.  ProgDir$/ProgName$ call ProgPath$ only while their
 * buffer is empty (the program's directory and name do not change).
 * DefaultFontHeight keeps d0's high word (the result is a word).  finit
 * closes intuition.library and dos.library without clearing the bases
 * (nothing uses them after finit).
 *
 * DIFF: a2 stays unchanged where the original uses it as a work register
 * (EZRequest/EZRequestArgs: 0; BInstr: on the string), and so does a3 in
 * BInstr (the original leaves it on the search string),
 * docs/libs/cmdlibs-port.md "General".  The format text of EZRequestArgs is written byte by byte (the
 * original's MOVE.w at odd addresses is an address error on a 68000);
 * FromBlitz compares bytes (the original CMPM.l, also at an odd task
 * name).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <prefs/font.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase_
#define INTUITION_BASE_NAME intuibase
#include <proto/dos.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _resettimer(void), _getsigbit(void), _iswbhere(void), _doeasy(void);
void _doeasy2(void), _doeasy3(void), _doeasy4(void), _doflip(void);
void _parseinit(void), rimisclib_nextparsearg(void), _ezoutput(void);
void _ezoutput2(void), _doeasy6(void), _doeasysep(void), _doclickwait(void);
void _doprogpath(void), _doprogdir(void), _doprogname(void), _fromblitz(void);
void _ltos(void), _ascraw(void), _rawasc(void), _openfontprefs(void);
void _closefontprefs(void), _defaultfontheight(void), _defaultfontname(void);
void _nextparseuntilchar(void), _binstr(void), rimisclib_libinit(void);
void rimisclib_libfinit(void), ezargschk(void), ezreqargschk(void);
void ezgad3chk(void), ezgad2chk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rimisclib,init,0,finit,0}; error field: the error check area
   of ezargschk and ezreqargschk (fixed, #748), ezgad3chk and ezgad2chk
   (fixed, #1150) */
BLITZ_LIBHEADER(LIB_rimisclib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_resettimer, 0, 0);
BLITZ_NAME("TimerReset", "");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_getsigbit, 0, 0);
BLITZ_NAME("PortSigBit", "msgport");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_iswbhere), 0, 0);
BLITZ_NAME("WorkbenchHere", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy), ezgad3chk, 0);       /* check: fixed #1150 */
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy2), ezgad2chk, 0);      /* check: fixed #1150 */
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy3), 0, 0);
BLITZ_REPARGS(3, 1, BL_STRING, BL_STRING, BL_STRING, BL_UNKNOWN | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy4), ezargschk, 0);      /* check: fixed #748 */
BLITZ_NAME("EZRequest", "[title$,]body$[,gadget$] | title$,body$,gadget$[,var..]");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_doflip, 0, 0);
BLITZ_NAME("Flip$", "(String$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_parseinit, 0, 0);
BLITZ_NAME("InitArgParse", "parameter$");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(rimisclib_nextparsearg, 0, 0);
BLITZ_NAME("NextArg$", "- get next parameter");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
BLITZ_SUBS(_ezoutput, 0, 0);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_ezoutput2, 0, 0);
BLITZ_NAME("EZOutput", "[win#]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_REPARGS(0, 1, BL_UNKNOWN | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy6), ezreqargschk, 0);   /* check: fixed #748 */
BLITZ_NAME("EZRequestArgs", "args..");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasysep), 0, 0);
BLITZ_NAME("EZSeparator", "char");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_doclickwait, 0, 0);
BLITZ_NAME("ClickWait", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doprogpath), 0, 0);
BLITZ_NAME("ProgPath$", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doprogdir), 0, 0);
BLITZ_NAME("ProgDir$", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doprogname), 0, 0);
BLITZ_NAME("ProgName$", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_fromblitz, 0, 0);
BLITZ_NAME("FromBlitz", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ltos, 0, 0);
BLITZ_NAME("LtoS", "(longword) -- convert LONG to STRING");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_ascraw, 0, 0);
BLITZ_NAME("ASCRaw", "(rawkey)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rawasc, 0, 0);
BLITZ_NAME("RawASC", "(char)");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_openfontprefs), 0, 0);
BLITZ_NAME("OpenFontPrefs", "[filename$]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_closefontprefs), 0, 0);
BLITZ_NAME("CloseFontPrefs", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_defaultfontheight, 0, 0);
BLITZ_NAME("DefaultFontHeight", "fonttype  (0=Icon,1=System,2=Screen)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_defaultfontname, 0, 0);
BLITZ_NAME("DefaultFontName", "fonttype  (0=Icon,1=System,2=Screen)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_nextparseuntilchar, 0, 0);
BLITZ_NAME("NextArgChar$", "char - get next parameter up until char");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_binstr, 0, 0);
BLITZ_NAME("BInstr", "(String$,Findstring$)");

BLITZ_NULLSUB_AT(finit, rimisclib_libfinit, 0, 0, LIB_doslib, BL_LA6);
BLITZ_NULLSUB_AT(init, rimisclib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ the data */

#define BL_ID4(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#define FONTBUF 16384
#define FONTCHUNK 164                   /* "FONT", size, struct FontPrefs */
static_assert(FONTCHUNK == 8 + sizeof(struct FontPrefs));

static struct DosLibrary *dosbase_;                     /* _dosbase */
static struct IntuitionBase *intuibase;
static const char dosname[] = "dos.library";
static const char intuiname[] = "intuition.library";

/* font prefs: the buffer (fontloc) and the first FONT chunk's data */
static UBYTE *fontpos, *fontloc;
static ULONG fontlen;                   /* the bytes read */
static BPTR fontfh;
static const char fontname[] = "ENV:SYS/Font.Prefs";

/* ProgPath$: the 0 in front of rawprogname ends the backward search */
static struct {
    UBYTE guard0[2], rawprogname[256];
    UBYTE guard1[2], progname[256];
    UBYTE guard2[2], progdir[256];
} pd;

/* the argument parser */
static UBYTE *parseadr, *curadr;
static UBYTE argbuffer[256];            /* InitArgParse's copy of the string */

/* EZRequest: the EasyStruct, the argument array, the window */
static struct {
    struct EasyStruct es;                               /* ezstruct */
    APTR args;                                          /* ezargs */
    ULONG args2[16];                                    /* ezargs2 */
} ez = { { 20, 0, NULL, NULL, NULL }, NULL, { 0 } };
static UBYTE ezargsbuf[160];
static ULONG numargs;
static UBYTE flag, sep = '|';
static UBYTE *ezfirst;                  /* doargs: the first string argument */
static struct Window *win;
static const char okgad[] = "Ok";

static const UBYTE *thisscreen;
static const char titlestring[] = "Workbench";
static const char procname[] = "Blitz ][ Program Proc";

/* ASCRaw / RawASC: the raw key codes, the 114 bytes of the original (its
   comments count the rows one off from the third on) */
static const UBYTE rawtable[114] = {
    /* 0-15 */
    '\'', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\\', 0, '0',
    /* 16-31 */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, '1', '2', '3',
    /* 32-48 */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '#', 0, 0, '4', '5', '6', 0,
    /* 49-68 */
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '.', '7', '8', '9', ' ', 0, 9, 13,
    0,
    /* 69-79 */
    27, 8, 0, 0, 0, '-', 0, 28, 29, 30, 31,
    /* 80-113 */
    129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    '(', ')', '/', '*', '+', 139, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* CIA-A */
#define CIAA_PRA     (*(volatile UBYTE *)0xbfe001)
#define CIAA_TODLOW  (*(volatile UBYTE *)0xbfe801)
#define CIAA_TODMID  (*(volatile UBYTE *)0xbfe901)
#define CIAA_TODHI   (*(volatile UBYTE *)0xbfea01)

/* copy s with its 0 to a3, a3 on the 0 (fixed, #743: the original left
   it behind the 0); the length */
static ULONG copyout(const UBYTE *s)
{
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;
    return n;
}

/* ------------------------------------------------------------ init/finit */

/**
 * @brief init: open dos.library and intuition.library (any version).
 *
 * No font prefs buffer yet.
 * @return d0: the intuition.library base (0 if it is missing)
 */
static ULONG BLITZ_ROUTINE libinit_c(void)
{
    dosbase_ = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)dosname, 0);
    intuibase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)intuiname, 0);
    fontloc = NULL;
    fontpos = NULL;
    return (ULONG)intuibase;
}

static ULONG BLITZ_ROUTINE closefontprefs_c(void);

/**
 * @brief finit: close intuition.library (if open) and dos.library, free
 * the font prefs buffer.
 *
 * The bases are not cleared (no defect: nothing uses them after finit).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (intuibase)
        CloseLibrary(&intuibase->LibNode);
    CloseLibrary(&dosbase_->dl_lib);                    /* bases kept (no defect) */
    closefontprefs_c();
}

/* ------------------------------------------------------------ hardware */

/**
 * @brief TimerReset: set the CIA-A TOD counter to 0.
 *
 * Writing the high byte stops it, the low byte starts it again.
 */
static void BLITZ_ROUTINE resettimer_c(void)
{
    CIAA_TODHI = 0;
    CIAA_TODMID = 0;
    CIAA_TODLOW = 0;
}

/**
 * @brief ClickWait: wait for a click of the left mouse button.
 *
 * Busy waits on CIA-A until the button is pressed, a short delay (DBRA
 * of 12767), until it is released.
 */
static void BLITZ_ROUTINE doclickwait_c(void)
{
    UWORD n = 12767;

    while (CIAA_PRA & 0x40)
        ;
    do
        __asm__ volatile("");                           /* DBRA d0,'loopy */
    while (n-- != 0);
    while (!(CIAA_PRA & 0x40))
        ;
}

/* ---------------------------------------------------------- small ones */

/**
 * @brief PortSigBit(msgport): the signal bit of a message port.
 * @param port  d0: the MsgPort
 * @return d0: its mp_SigBit (a byte result)
 */
static ULONG BLITZ_ROUTINE getsigbit_c(const struct MsgPort *port BL_REG(d0))
{
    return port->mp_SigBit;
}

/**
 * @brief WorkbenchHere: whether a screen's title contains "Workbench".
 *
 * Searches the screen titles from the library's IntuitionBase
 * (FirstScreen) up to the end of the list, each title from every
 * character on; a screen without a title is skipped.
 * Fixed (#759): the original took IntuitionBase from d0, which is no
 * argument (MOVE.l intuibase,a0 overwritten by MOVE.l d0,a0).
 * Fixed (#761): the original never ends the screen list (no 0 check: it
 * reads the "screen" at address 0 and goes on), so it returned BB_TRUE
 * or hung, never BB_FALSE.  Fixed (#762): the original restarted the
 * same title when it ended during a partial match, for ever for a title
 * that ends with the start of "Workbench".  Fixed (#760): the original
 * went on with the next screen at the first mismatch after a "W"
 * ("A Window on Workbench" was not found).
 * @return d0: BB_TRUE when found, BB_FALSE at the end of the list
 */
static BB_BOOL BLITZ_ROUTINE iswbhere_c(void)
{
    const struct Screen *scr = intuibase->FirstScreen;  /* fixed (#759): not d0 */
    const UBYTE *t, *s, *p;

    thisscreen = (const UBYTE *)scr;
    for (; scr; scr = scr->NextScreen) {                /* fixed (#761): the end */
        s = scr->Title;
        if (!s)
            continue;                                   /* fixed (#761): no title */
        for (; *s; s++) {                               /* fixed (#760): every start */
            for (p = s, t = (const UBYTE *)titlestring; *t && *p == *t; p++, t++)
                ;                                       /* fixed (#762): ends at a 0 */
            if (!*t)
                return BB_TRUE;
        }
    }
    return BB_FALSE;                                    /* 'oops */
}

/**
 * @brief FromBlitz: whether the program runs from Blitz (the task name
 * starts with "Blitz ][ Program Proc").
 *
 * Compares bytes (DIFF: the original's CMPM.l is an address error at an
 * odd task name on a 68000).
 * @return d0: BB_TRUE if it does, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE fromblitz_c(void)
{
    const UBYTE *n = (const UBYTE *)SysBase->ThisTask->tc_Node.ln_Name;
    int i;

    for (i = 0; i < 20; i++)                            /* 5 x CMPM.l */
        if (n[i] != (UBYTE)procname[i])
            return BB_FALSE;
    return n[20] == 'c' ? BB_TRUE : BB_FALSE;
}

/**
 * @brief LtoS(longword): a long as a string of its 4 bytes.
 *
 * Highest byte first, written to a3 without a 0.  The bytes 0-31 and
 * 128-255 become 128 (a signed CMPI.b #32), the documented "non-ASCII
 * characters are replaced with character 128" (no defect, #753).
 * @param v  d0: the long
 * @return d0: 4, the length
 */
static ULONG BLITZ_ROUTINE ltos_c(ULONG v BL_REG(d0))
{
    int i;

    for (i = 0; i < 4; i++) {
        BYTE b;

        v = v << 8 | v >> 24;                           /* ROL.l #8 */
        b = (BYTE)v;
        if (b < 32)
            b = (BYTE)128;
        *blitz_string++ = (UBYTE)b;
    }
    return 4;
}

/**
 * @brief ASCRaw(rawkey): the character of a raw key code.
 *
 * From the original's table of 114 entries; a key outside 0..113 (a key
 * up code, a negative value) gives the character 0, as the table's keys
 * without a character do.  Fixed (#744): the original did not check the
 * code and read behind (or before) the table.
 * @param key  d0.w: the raw key code
 * @return d0: 1, the length of the string at a3
 */
static ULONG BLITZ_ROUTINE ascraw_c(UWORD key BL_REG(d0))
{
    *blitz_string++ = key < sizeof rawtable ? rawtable[key] : 0;  /* fixed (#744) */
    *blitz_string = 0;
    return 1;
}

/**
 * @brief RawASC(char): the raw key code of a character.
 *
 * The first index of the string's first character in the table; -1 for
 * a character not in the table and for "" (as Asc gives -1 for "").
 * Fixed (#744): the original searched on behind the table until it
 * found the byte somewhere (or for ever), and found "" (its 0) at 14.
 * @param s  d0: the string
 * @return d0.w: the raw key code, -1 if there is none
 */
static ULONG BLITZ_ROUTINE rawasc_c(const UBYTE *s BL_REG(d0))
{
    UBYTE c = *s;
    UWORD i;

    if (c)
        for (i = 0; i < sizeof rawtable; i++)           /* fixed (#744) */
            if (rawtable[i] == c)
                return i;
    return (ULONG)-1;
}

/**
 * @brief Flip$(String$): the string reversed.
 *
 * Reversed in place: the argument is the string below a3, a3 stays.
 * A string of 0 or 1 characters stays as it is.  Fixed (#751): the
 * original swapped 65536 byte pairs around it (DBRA with -1).
 * @param lens  a2: the argument stack, -4(a2) the length of the string
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doflip_c(const ULONG *lens BL_REG(a2))
{
    ULONG len = lens[-1];
    UBYTE *front = blitz_string - len - 1, *e = blitz_string - 2;
    UWORD n = (UWORD)((len >> 1) - 1);

    if (len < 2)
        return len;                                     /* fixed (#751) */
    do {
        UBYTE c = *front;

        *front++ = *e;
        *e-- = c;
    } while (n-- != 0);
    return len;
}

/**
 * @brief BInstr(String$,Findstring$): the position of the last
 * Findstring$ in String$.
 *
 * Searches backwards.  DIFF: a2 and a3 stay unchanged (the original
 * leaves them on the strings).
 * @param src   d0: String$
 * @param find  d1: Findstring$
 * @param lens  a2: the argument stack, -4(a2) the length of String$
 * @return d0: the position, 1 based; 0 if none (and for Findstring$ = "")
 */
static ULONG BLITZ_ROUTINE binstr_c(const UBYTE *src BL_REG(d0), const UBYTE *find BL_REG(d1),
                                    const ULONG *lens BL_REG(a2))
{
    const UBYTE *pos = src + lens[-1], *p, *q;

    for (;;) {
        pos--;                                          /* 'check */
        if ((LONG)src > (LONG)pos)
            return 0;
        p = pos;
        q = find;
        for (;;) {                                      /* 'checkpos */
            if (!*p)
                break;
            if (*p++ != *q++)
                break;
            if (!*q)
                return (ULONG)(pos - src) + 1;
        }
    }
}

/* -------------------------------------------------------- arg parsing */

/**
 * @brief InitArgParse parameter$: start parsing a string with NextArg$ /
 * NextArgChar$.
 *
 * Copies at most 255 characters of it and a 0 into the 256 byte
 * buffer; the rest of a longer string is cut off.  Fixed (#752): the
 * original copies without a limit and clears four more bytes behind the
 * copy, an end guard for NextArg$, which stepped behind the 0 (it stays
 * on it since #755).
 * @param s  d0: the string
 */
static void BLITZ_ROUTINE parseinit_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *dst = argbuffer, c;

    while (dst < argbuffer + sizeof argbuffer - 1 && (c = *s++) != 0)
        *dst++ = c;
    *dst = 0;
    parseadr = argbuffer;
    curadr = argbuffer;
}

/**
 * @brief NextArg$: the next word of the InitArgParse string.
 *
 * Leading spaces are skipped; quotes group (a quoted word ends at the
 * closing quote even if no space follows, as ReadItem does), "" inside
 * quotes is a quote.  The word goes to a3 without a 0 (none needed,
 * #257).  The parse position stays on the 0 at the end of the string.
 * Fixed (#755): the original counts the quotes (d7) and ends the word
 * at an even count unless a quote follows; "" moved the count on by one
 * only, so the quote behind it copied the next character (possibly the
 * 0, stepping behind it).  A quote now ends the word unless a quote
 * follows, which is copied once.
 * @return d0: the length; 0 at the end (also before InitArgParse)
 */
static ULONG BLITZ_ROUTINE nextparsearg_c(void)
{
    UBYTE *pos = curadr, *start = blitz_string, c;

    if (!curadr)
        return 0;
    for (;;) {                                          /* 'strip */
        c = *pos;
        if (!c) {
            curadr = NULL;                              /* 'lastarg */
            return 0;
        }
        if (c != ' ')
            break;
        pos++;
    }
    for (;;) {                                          /* 'cmploop */
        c = *pos++;
        if (!c) {
            pos--;                                      /* stay on the 0 */
            goto endthis;
        }
        if (c == ' ')
            goto endthis;
        if (c == '"')
            break;
        *blitz_string++ = c;
    }
    for (;;) {                                          /* 'quoteloop */
        c = *pos++;
        if (!c) {
            pos--;                                      /* stay on the 0 */
            break;
        }
        if (c != '"') {
            *blitz_string++ = c;
            continue;
        }
        if (*pos != '"')                                /* 'gotquote */
            break;
        *blitz_string++ = *pos++;                       /* 'cantquityet */
    }
endthis:
    curadr = pos;
    return (ULONG)(blitz_string - start);
}

/**
 * @brief NextArgChar$ char: the InitArgParse string up to a character.
 *
 * Everything up to char (or the end) to a3, without a 0; the parse
 * position goes behind char.  Before InitArgParse and after NextArg$
 * came to the end the result is "", as NextArg$'s.  Fixed (#754): the
 * original did not check the parse pointer and read from address 0.
 * @param ch  d0.b: the character
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE nextparseuntilchar_c(UBYTE ch BL_REG(d0))
{
    UBYTE *pos = curadr, c;
    ULONG n = 0;

    if (!pos)
        return 0;                                       /* fixed (#754) */
    for (;;) {
        c = *pos++;
        if (!c) {
            pos--;                                      /* 'eol */
            break;
        }
        if (c == ch)
            break;
        *blitz_string++ = c;
        n++;
    }
    curadr = pos;
    return n;
}

/* ------------------------------------------------------------ EZRequest */

/* below intuition V36 (Kickstart 1.x, fixed #1150): RawDoFmt's PutChProcs
   (the character in d0, the data in a3) - store it, or count it in the
   long at a3; machine code as words (the RKM's form of the first) */
static const UWORD ez_putch[] = { 0x16c0, 0x4e75 };      /* move.b d0,(a3)+ ; rts */
static const UWORD ez_countch[] = { 0x5293, 0x4e75 };    /* addq.l #1,(a3) ; rts */

static struct TextAttr topaz8 = { (STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT };

/* the bytes of the argument stream a RawDoFmt format of exec V33 reads:
   %[-][0][width][.limit][l]type; s and b a long, d u x c a word (a long
   with l), other characters none */
static ULONG fmtbytes(const UBYTE *f)
{
    ULONG n = 0;
    UBYTE c, l;

    while ((c = *f++) != 0) {
        if (c != '%')
            continue;
        if (*f == '-')
            f++;
        while ((*f >= '0' && *f <= '9') || *f == '.')
            f++;
        l = *f == 'l';
        if (l)
            f++;
        if (!(c = *f))
            break;
        f++;
        if (c == 's' || c == 'b')
            n += 4;
        else if (c == 'd' || c == 'u' || c == 'x' || c == 'c')
            n += l ? 4 : 2;
    }
    return n;
}

/* a format with its arguments as RawDoFmt makes it, in a block of
   AllocMem of *size bytes (the 0 included); NULL without memory */
static UBYTE *ezformat(const UBYTE *f, APTR args, ULONG *size)
{
    ULONG n = 0;
    UBYTE *buf;

    RawDoFmt((STRPTR)f, args, (APTR)ez_countch, &n);
    buf = AllocMem(n, MEMF_ANY);
    if (buf)
        RawDoFmt((STRPTR)f, args, (APTR)ez_putch, buf);
    *size = n;
    return buf;
}

/* an IntuiText of the 1.x requester: AutoRequest's pens and offsets,
   topaz 8 (its height known for the line spacing) */
static UWORD ezitext(struct IntuiText *it, UBYTE *s, WORD top, struct IntuiText *next)
{
    it->FrontPen = AUTOFRONTPEN;
    it->BackPen = AUTOBACKPEN;
    it->DrawMode = AUTODRAWMODE;
    it->LeftEdge = AUTOLEFTEDGE;
    it->TopEdge = top;
    it->ITextFont = &topaz8;
    it->IText = s;
    it->NextText = next;
    return (UWORD)IntuiTextLength(it);
}

/* EZRequest below intuition V36 (Kickstart 1.x has no EasyRequestArgs;
   fixed #1150, the original called it there, behind intuition's jump
   table): body and gadgets formatted with the arguments (the gadgets with
   those behind the body's, as EasyRequestArgs), the body's lines (line
   feeds) as IntuiTexts, AutoRequest with one gadget (negative) or two
   (positive | negative); the title is not shown.  The result as
   EasyRequestArgs': 1 for the left gadget of two, 0 for the right one.
   More than two gadgets (refused by the error check) or no memory: no
   requester, 0. */
static ULONG doeasy13(void)
{
    const UBYTE *body = (const UBYTE *)ez.es.es_TextFormat;
    UBYTE *text, *gads, *p, *pos = NULL, *neg;
    struct IntuiText *it;
    ULONG tsize, gsize, lines, i, r = 0, w, h, gw, maxw = 640, maxh = 200;

    if (!(text = ezformat(body, ez.args, &tsize)))
        return 0;
    gads = ezformat((const UBYTE *)ez.es.es_GadgetFormat,
                    ez.args ? (UBYTE *)ez.args + fmtbytes(body) : NULL, &gsize);
    if (!gads)
        goto freetext;
    neg = gads;
    for (p = gads; *p; p++)
        if (*p == '|') {
            if (pos)
                goto freegads;                          /* three or more */
            *p = 0;
            pos = gads;
            neg = p + 1;
        }
    lines = 1;
    for (p = text; *p; p++)
        if (*p == 10)
            lines++;
    it = AllocMem((lines + 2) * sizeof *it, MEMF_ANY);
    if (!it)
        goto freegads;
    w = 0;
    p = text;
    for (i = 0; i < lines; i++) {
        UBYTE *s = p;

        while (*p && *p != 10)
            p++;
        if (*p)
            *p++ = 0;
        gw = ezitext(&it[i], s, AUTOTOPEDGE + i * 9, i + 1 < lines ? &it[i + 1] : NULL);
        if (gw > w)
            w = gw;
    }
    gw = ezitext(&it[lines + 1], neg, AUTOTOPEDGE, NULL) + 40;
    if (pos)
        gw += ezitext(&it[lines], pos, AUTOTOPEDGE, NULL) + 40;
    w = (w + 40 > gw ? w + 40 : gw) + 16;
    h = lines * 9 + 46;
    if (win) {
        maxw = win->WScreen->Width;
        maxh = win->WScreen->Height;
    }
    r = AutoRequest(win, it, pos ? &it[lines] : NULL, &it[lines + 1], 0, 0,
                    w < maxw ? w : maxw, h < maxh ? h : maxh) ? 1 : 0;
    FreeMem(it, (lines + 2) * sizeof *it);
freegads:
    FreeMem(gads, gsize);
freetext:
    FreeMem(text, tsize);
    return r;
}

/* doeasybegin: '|' to line feeds in the body (the compiler's copy of
   the argument), EasyRequestArgs on the EZOutput window; below intuition
   V36 the library's own requester (fixed #1150) */
static ULONG doeasybegin(void)
{
    UBYTE *p = (UBYTE *)ez.es.es_TextFormat, c;

    while ((c = *p++) != 0)
        if (c == '|')
            p[-1] = 10;
    if (intuibase->LibNode.lib_Version < 36)
        return doeasy13();                              /* fixed (#1150) */
    return (ULONG)EasyRequestArgs(win, &ez.es, NULL, ez.args);
}

/* the integer part of a Quick (16.16), truncated towards 0 as Int does
   (fixed #750: the original passed the high word zero extended, -3.5
   gave 65532) */
static LONG quicktrunc(ULONG q)
{
    LONG i = (LONG)q >> 16;

    if ((LONG)q < 0 && (q & 0xffff))
        i++;
    return i;
}

/* the integer part of a Float (FFP: mantissa in bits 8-31 with its top
   bit set, sign bit 7, exponent bits 0-6 excess 64), truncated towards 0
   (fixed #750, #749: the original converted it with floatquicklib's
   fltoqu, whose mathffp calls went through the caller's a6, and passed
   the Quick's high word zero extended); from 2^31 on the low 32 bits */
static LONG ffptrunc(ULONG f)
{
    WORD e = (WORD)(f & 0x7f) - 64;
    ULONG m = f >> 8, i;

    if (e <= 0)
        return 0;
    if (e <= 24)
        i = m >> (24 - e);
    else
        i = e - 24 < 32 ? m << (e - 24) : 0;
    return f & 0x80 ? -(LONG)i : (LONG)i;
}

/* the argument array (and with fmt the format text) from the repeated
   #unknown|#push arguments below sp: per argument the type word, below
   it the value (a string: its length, below that the address); `cnt`
   arguments (DBRA: 0 = 65536); the format text ends with a 0 (fixed
   #747: the original wrote none).  A Quick or Float is passed as its
   integer part, a long (%ld).  `ezfirst`: the lowest address of a string
   argument, the first one the compiler put at a3 (NULL without one,
   #1027). */
static void doargs(const UBYTE *sp, UWORD cnt, UBYTE *fmt)
{
    UBYTE *out = (UBYTE *)ez.args2;
    UWORD n = (UWORD)(cnt - 1);

    flag = 0;
    ezfirst = NULL;
    do {
        UWORD type;
        ULONG v;

        if (fmt) {
            if (flag)
                *fmt++ = sep;
            flag = 0xff;
        }
        type = BL_POPW(sp);
        if (type == BL_STRING) {
            UBYTE *str;

            (void)BL_POPL(sp);                          /* the length */
            str = (UBYTE *)BL_POPL(sp);
            *(ULONG *)out = (ULONG)str;
            out += 4;
            if (!ezfirst || str < ezfirst)
                ezfirst = str;
            if (fmt) {
                *fmt++ = '%';
                *fmt++ = 's';
            }
        } else if (type == BL_LONG) {
            *(ULONG *)out = BL_POPL(sp);
            out += 4;
            if (fmt) {
                *fmt++ = '%';
                *fmt++ = 'l';
                *fmt++ = 'd';
            }
        } else if (type == BL_WORD || type == BL_BYTE) {
            *(UWORD *)out = BL_POPW(sp);
            out += 2;
            if (fmt) {
                *fmt++ = '%';
                *fmt++ = 'd';
            }
        } else {
            v = BL_POPL(sp);
            /* fixed (#750, #749): the integer part truncated towards 0 */
            *(LONG *)out = type == BL_QUICK ? quicktrunc(v) : ffptrunc(v);
            out += 4;
            if (fmt) {
                *fmt++ = '%';
                *fmt++ = 'l';
                *fmt++ = 'd';
            }
        }
    } while (n-- != 0);
    if (fmt)
        *fmt = 0;                                       /* fixed (#747) */
}

/**
 * @brief EZRequest title$,body$,gadget$: an EasyRequest on the EZOutput
 * window ('|' in the body: a line feed).
 *
 * DIFF: a2 stays (the original: 0).  Fixed (#1027): a3 = title$
 * afterwards (the original: 0).
 * @param title   d0: the title
 * @param body    d1: the body text
 * @param gadget  d2: the gadgets ('|' separated)
 * @return d0: what EasyRequestArgs returns (the gadget number)
 */
static ULONG BLITZ_ROUTINE doeasy_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                    STRPTR gadget BL_REG(d2))
{
    ULONG r;

    ez.es.es_Title = title;
    ez.es.es_TextFormat = body;
    ez.es.es_GadgetFormat = gadget;
    ez.args = NULL;
    r = doeasybegin();
    blitz_string = (UBYTE *)title;      /* fixed (#1027) */
    return r;
}

/**
 * @brief EZRequest body$,gadget$: an EasyRequest without a title.
 *
 * Fixed (#1027): a3 = body$ afterwards (the original: 0).
 * @param body    d0: the body text
 * @param gadget  d1: the gadgets
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy2_c(STRPTR body BL_REG(d0), STRPTR gadget BL_REG(d1))
{
    ULONG r;

    ez.es.es_GadgetFormat = gadget;
    ez.es.es_TextFormat = body;
    ez.es.es_Title = NULL;
    ez.args = NULL;
    r = doeasybegin();
    blitz_string = (UBYTE *)body;       /* fixed (#1027) */
    return r;
}

/**
 * @brief EZRequest body$: an EasyRequest without a title, gadget "Ok".
 * @param body  d0: the body text
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy3_c(STRPTR body BL_REG(d0))
{
    return doeasy2_c(body, (STRPTR)okgad);
}

/**
 * @brief EZRequest title$,body$,gadget$,var..: an EasyRequest with the
 * arguments as the format arguments.
 *
 * See doargs; no limit of the 16 argument longs (an invalid argument:
 * ezargschk, fixed #748).  Quick
 * and Float arguments: their integer part (fixed #750, #749).  Fixed
 * (#1027): a3 = title$
 * afterwards (the original: the argument array).
 * @param title      d0: the title
 * @param body       d1: the body (format) text
 * @param gadget     d2: the gadgets
 * @param args       a2: the argument stack: the three lengths, below
 *                   them the repeated arguments
 * @param reps       d7.w: the number of arguments
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy4_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                     STRPTR gadget BL_REG(d2), UBYTE *args BL_REG(a2),
                                     UWORD reps BL_REG(d7))
{
    ULONG r;

    ez.es.es_Title = title;
    ez.es.es_TextFormat = body;
    ez.es.es_GadgetFormat = gadget;
    ez.args = ez.args2;
    doargs(args - 12, reps, NULL);
    r = doeasybegin();
    blitz_string = (UBYTE *)title;      /* fixed (#1027) */
    return r;
}

/**
 * @brief EZRequestArgs args..: an EasyRequest whose body is built from the
 * arguments.
 *
 * %s / %ld / %d per type, separated by the EZSeparator character ('|': a
 * line feed), gadget "Ok", no title.  Fixed (#747): the format text
 * ends with a 0 (the original wrote none, a shorter text kept the end of
 * the one before).  No limit of the arguments (an invalid argument:
 * ezreqargschk, fixed #748); BUG(orig):
 * without arguments a type word is read from a2 and the count 0 runs the
 * loop 65536 times (#746).  Quick/Float: their integer part (fixed #750,
 * #749).  Fixed (#1027):
 * a3 = the first string argument afterwards, unchanged without one (the
 * original: the argument array).
 * @param args       a2: the argument stack, the repeated arguments
 * @param reps       d7.w: the number of arguments
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy6_c(UBYTE *args BL_REG(a2), ULONG reps BL_REG(d7))
{
    ULONG r;

    numargs = reps & 0xffff;
    ez.es.es_TextFormat = ezargsbuf;
    ez.args = ez.args2;
    doargs(args, (UWORD)reps, ezargsbuf);
    ez.es.es_GadgetFormat = (STRPTR)okgad;
    ez.es.es_Title = NULL;
    r = doeasybegin();
    if (ezfirst)
        blitz_string = ezfirst;         /* fixed (#1027) */
    return r;
}

/**
 * @brief EZOutput win#: the window of the EZRequests.
 * @param w  a0: the window object (windowslib, #ia0|#pd0), its first long
 *           the Window
 */
static void BLITZ_ROUTINE ezoutput_c(struct Window **w BL_REG(a0))
{
    win = *w;
}

/**
 * @brief EZOutput: the EZRequests on the default screen again (no window).
 */
static void BLITZ_ROUTINE ezoutput2_c(void)
{
    win = NULL;
}

/**
 * @brief EZSeparator char: the separator EZRequestArgs puts between the
 * arguments (default '|', a line feed).
 * @param s  d0: the string, its first character
 */
static void BLITZ_ROUTINE doeasysep_c(const UBYTE *s BL_REG(d0))
{
    sep = *s;
}

/* ------------------------------------------------------------ the paths */

/* below dos V36 (fixed #1150): the path of the current directory in buf
   (at most 254 characters and the 0, as NameFromLock's 255 bytes), built
   from the names of its directories up to the root (Examine, ParentDir:
   "Volume:dir/dir"); "" if it is longer, without memory for the
   FileInfoBlock or when a call fails */
static void dirpath13(UBYTE *buf)
{
    struct FileInfoBlock *fib = AllocMem(sizeof *fib, MEMF_ANY);
    UBYTE *end = buf + 254, *p = end;
    BPTR lock, parent;
    const UBYTE *s;
    ULONG n, first = 1, ok = 1;

    buf[0] = 0;
    if (!fib)
        return;
    lock = Lock((CONST_STRPTR)"", SHARED_LOCK);
    *p = 0;
    while (lock) {
        if (!Examine(lock, fib)) {
            ok = 0;
            break;
        }
        parent = ParentDir(lock);
        UnLock(lock);
        lock = parent;
        for (s = (const UBYTE *)fib->fib_FileName, n = 0; s[n]; n++)
            ;
        if ((ULONG)(p - buf) < n + (parent ? !first : 1)) {
            ok = 0;                                     /* too long */
            break;
        }
        if (!parent)
            *--p = ':';
        else if (!first)
            *--p = '/';
        while (n)
            *--p = s[--n];
        first = 0;
    }
    if (lock)
        UnLock(lock);
    if (ok)
        for (s = p; (*buf++ = *s++) != 0;)
            ;
    FreeMem(fib, sizeof *fib);
}

/* below dos V36 (fixed #1150): the shell's command name (a BSTR) in buf,
   at most 254 characters; "" without a shell or name */
static void progname13(UBYTE *buf)
{
    struct Process *pr = (struct Process *)SysBase->ThisTask;
    struct CommandLineInterface *cli;
    const UBYTE *b;
    ULONG n, i;

    buf[0] = 0;
    if (pr->pr_Task.tc_Node.ln_Type != NT_PROCESS
        || !(cli = BADDR(pr->pr_CLI)) || !cli->cli_CommandName)
        return;
    b = BADDR(cli->cli_CommandName);
    n = b[0] < 254 ? b[0] : 254;
    for (i = 0; i < n; i++)
        buf[i] = b[i + 1];
    buf[n] = 0;
}

/**
 * @brief ProgPath$: the program's directory and name.
 *
 * GetProgramDir (without one: the current directory) and
 * GetProgramName (empty: the task name); fills the buffers of ProgDir$
 * and ProgName$.  a3 ends on the 0 of the result (fixed, #743: the
 * original left it behind the 0).  Without a directory name the result
 * is the name alone; fixed (#758): the original decided on the "/" by
 * the byte before the result.  "/" is added unless the directory ends
 * in ":".
 * Below dos V36 (Kickstart 1.x has none of the four functions; fixed
 * #1150, the original called them there, behind dos's jump table): the
 * current directory, its path built by the library (dirpath13), and the
 * shell's command name (cli_CommandName; without a shell the task name).
 * A program started with a path from another directory gets the current
 * directory there, not its own.
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doprogpath_c(void)
{
    const UBYTE *s;
    UBYTE *dst, c;
    ULONG n;

    pd.rawprogname[0] = 0;
    if (dosbase_->dl_lib.lib_Version < 36) {           /* fixed (#1150) */
        dirpath13(pd.progdir);
        progname13(pd.rawprogname);
    } else {
        BPTR lock = GetProgramDir();

        if (!lock)
            GetCurrentDirName(pd.progdir, 255);
        else
            NameFromLock(lock, pd.progdir, 255);
        GetProgramName(pd.rawprogname, 255);
    }
    if (!pd.rawprogname[0]) {
        s = (const UBYTE *)SysBase->ThisTask->tc_Node.ln_Name;
        dst = pd.progname;
        while ((*dst++ = *s++) != 0)
            ;
    } else {
        s = pd.rawprogname;
        while (*s++)
            ;
        s--;
        do                                              /* back to : / or the 0 */
            c = *--s;
        while (c && c != ':' && c != '/');
        s++;
        dst = pd.progname;
        while ((*dst++ = *s++) != 0)
            ;
    }
    n = copyout(pd.progdir);
    n--;
    if (pd.progdir[0] && blitz_string[-1] != ':') {     /* fixed (#758) */
        n++;
        *blitz_string++ = '/';
    }
    s = pd.progname;
    do {
        n++;
        c = *s++;
        *blitz_string++ = c;
    } while (c);
    blitz_string--;                                     /* fixed (#743) */
    return n;
}

/**
 * @brief ProgDir$: the program's directory.
 *
 * Calls ProgPath$ only while its buffer is empty (the directory does not
 * change).  a3 ends on the 0 (fixed, #743).
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doprogdir_c(void)
{
    if (!pd.progdir[0]) {
        UBYTE *a3 = blitz_string;

        doprogpath_c();
        blitz_string = a3;
    }
    return copyout(pd.progdir);
}

/**
 * @brief ProgName$: the program's name.
 *
 * Calls ProgPath$ only while its buffer is empty.  a3 ends on the 0
 * (fixed, #743).
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doprogname_c(void)
{
    if (!pd.progname[0]) {
        UBYTE *a3 = blitz_string;

        doprogpath_c();
        blitz_string = a3;
    }
    return copyout(pd.progname);
}

/* ---------------------------------------------------------- font prefs */

/**
 * @brief CloseFontPrefs: free the font prefs buffer of OpenFontPrefs.
 * @return d0: 0 (OpenFontPrefs' failures return it)
 */
static ULONG BLITZ_ROUTINE closefontprefs_c(void)
{
    if (!fontloc)
        return 0;
    FreeMem(fontloc, FONTBUF);
    fontloc = NULL;
    fontpos = NULL;
    return 0;
}

/**
 * @brief OpenFontPrefs: read ENV:SYS/Font.Prefs into a 16K buffer.
 *
 * Without memory for the buffer it returns BB_FALSE; fixed (#756): the
 * original did not check AllocMem and read the file to address 0.
 * The buffer of an OpenFontPrefs before is freed first; fixed (#757):
 * the original lost it.  Fixed (#745): the chunks are read within the
 * bytes read (the original ignored Read's result and took the PRHD size
 * unchecked; a read error is a failed open now).
 * @return d0: BB_TRUE if it is a FORM PREF with PRHD and a FONT chunk
 *         behind it; else BB_FALSE, the buffer freed (the tail call of
 *         CloseFontPrefs, which returns 0)
 */
static BB_BOOL BLITZ_ROUTINE openfontprefs_c(void)
{
    const ULONG *p;
    LONG n;
    ULONG at;

    closefontprefs_c();                                 /* fixed (#757) */
    fontloc = AllocMem(FONTBUF, 0);
    if (!fontloc)
        return BB_FALSE;                                /* fixed (#756) */
    fontfh = Open((CONST_STRPTR)fontname, MODE_OLDFILE);
    if (!fontfh)
        return closefontprefs_c(), BB_FALSE;
    n = Read(fontfh, fontloc, FONTBUF);
    Close(fontfh);
    if (n < 28)                                         /* fixed (#745) */
        return closefontprefs_c(), BB_FALSE;
    fontlen = (ULONG)n;
    p = (const ULONG *)fontloc;
    if (*p++ != BL_ID4('F', 'O', 'R', 'M'))
        return closefontprefs_c(), BB_FALSE;
    p++;
    if (*p++ != BL_ID4('P', 'R', 'E', 'F'))
        return closefontprefs_c(), BB_FALSE;
    if (*p++ != BL_ID4('P', 'R', 'H', 'D'))
        return closefontprefs_c(), BB_FALSE;
    at = *p;                                            /* ADD.l (a0),a0 */
    if (at > fontlen - 28)                              /* fixed (#745) */
        return closefontprefs_c(), BB_FALSE;
    at += 20 + (at & 1);                                /* the next chunk */
    if (at > fontlen - 8)
        return closefontprefs_c(), BB_FALSE;
    p = (const ULONG *)(fontloc + at);
    if (*p++ != BL_ID4('F', 'O', 'N', 'T'))
        return closefontprefs_c(), BB_FALSE;
    p++;
    fontpos = (UBYTE *)p;
    return BB_TRUE;
}

/* the FONT chunk with fp_Type = type: the chunks from the first FONT
   chunk on, each by its size (padded to even), as far as they lie in the
   bytes read; only FONT chunks of a FontPrefs' size count; NULL if none.
   Fixed (#745): the original stepped by 164 bytes to the end of the
   buffer, through the uncleared memory behind the file and up to 26
   bytes behind the buffer, and took every step for a FONT chunk */
static const struct FontPrefs *findfont(UWORD type)
{
    ULONG at, size;
    const ULONG *c;

    if (!fontpos)
        return NULL;
    for (at = (ULONG)(fontpos - fontloc) - 8; at <= fontlen - 8; at += 8 + size + (size & 1)) {
        c = (const ULONG *)(fontloc + at);
        size = c[1];
        if (size > fontlen - 8 - at)
            break;
        if (c[0] == BL_ID4('F', 'O', 'N', 'T') && size >= sizeof(struct FontPrefs)
            && ((const struct FontPrefs *)(c + 2))->fp_Type == type)
            return (const struct FontPrefs *)(c + 2);
    }
    return NULL;
}

/**
 * @brief DefaultFontHeight(fonttype): the height of a font of the font
 * prefs (OpenFontPrefs).
 * @param type  d0.w: fp_Type (0 icon, 1 system, 2 screen)
 * @return d0.w: its ta_YSize (d0's high word kept); 0 if there is no
 *         buffer or no such chunk (#745)
 */
static ULONG BLITZ_ROUTINE defaultfontheight_c(ULONG type BL_REG(d0))
{
    const struct FontPrefs *p = findfont((UWORD)type);

    if (!p)
        return 0;
    return (type & 0xffff0000UL) | p->fp_TextAttr.ta_YSize;
}

/**
 * @brief DefaultFontName(fonttype): the name of a font of the font prefs.
 * @param type  d0.w: fp_Type (0 icon, 1 system, 2 screen)
 * @return d0: the length of fp_Name (at a3, a3 on the 0, fixed #743); 0 ("") if
 *         there is no buffer or no such chunk (#745)
 */
static ULONG BLITZ_ROUTINE defaultfontname_c(UWORD type BL_REG(d0))
{
    const struct FontPrefs *p = findfont(type);

    if (!p)
        return 0;
    return copyout((const UBYTE *)p->fp_Name);
}

/* Registers: the original changes d0, a0/a1 (most), d1-d3/a6 (the system
   calls), d2 (LtoS, Flip$, NextArg$...), d6/d7 (EZRequest, BInstr), a2/a3
   (EZRequest, BInstr); the thunks keep d1/a0/a1, the statements that keep
   d0 also d0, the C code d2-d7/a2-a6. */
BLITZ_THUNK(rimisclib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(rimisclib_libfinit, libfinit_c, "d1/a0-a1");
BLITZ_THUNK(_resettimer, resettimer_c, "d0-d1/a0-a1");
BLITZ_THUNK(_doclickwait, doclickwait_c, "d1/a0-a1");
BLITZ_THUNK(_getsigbit, getsigbit_c, "d1/a0-a1");
BLITZ_THUNK(_iswbhere, iswbhere_c, "d1/a0-a1");
BLITZ_THUNK(_fromblitz, fromblitz_c, "d1/a0-a1");
BLITZ_THUNK(_ltos, ltos_c, "d1/a0-a1");
BLITZ_THUNK(_ascraw, ascraw_c, "d1/a0-a1");
BLITZ_THUNK(_rawasc, rawasc_c, "d1/a0-a1");
BLITZ_THUNK(_doflip, doflip_c, "d1/a0-a1");
BLITZ_THUNK(_binstr, binstr_c, "d1/a0-a1");
BLITZ_THUNK(_parseinit, parseinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(rimisclib_nextparsearg, nextparsearg_c, "d1/a0-a1");
BLITZ_THUNK(_nextparseuntilchar, nextparseuntilchar_c, "d1/a0-a1");
BLITZ_THUNK(_doeasy, doeasy_c, "d1/a0-a1");
BLITZ_THUNK(_doeasy2, doeasy2_c, "d1/a0-a1");
BLITZ_THUNK(_doeasy3, doeasy3_c, "d1/a0-a1");
BLITZ_THUNK(_doeasy4, doeasy4_c, "d1/a0-a1");
BLITZ_THUNK(_doeasy6, doeasy6_c, "d1/a0-a1");
BLITZ_THUNK(_ezoutput, ezoutput_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ezoutput2, ezoutput2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_doeasysep, doeasysep_c, "d0-d1/a0-a1");
BLITZ_THUNK(_doprogpath, doprogpath_c, "d1/a0-a1");
BLITZ_THUNK(_doprogdir, doprogdir_c, "d1/a0-a1");
BLITZ_THUNK(_doprogname, doprogname_c, "d1/a0-a1");
BLITZ_THUNK(_openfontprefs, openfontprefs_c, "d1/a0-a1");
BLITZ_THUNK(_closefontprefs, closefontprefs_c, "d1/a0-a1");
BLITZ_THUNK(_defaultfontheight, defaultfontheight_c, "d1/a0-a1");
BLITZ_THUNK(_defaultfontname, defaultfontname_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(toomanyargs, "Too Many EZRequest Arguments");  /* fixed, #748 */
BLITZ_ERRCHK_MESSAGE(toomanygads, "More than two EZRequest gadgets need Kickstart 2.0");  /* fixed, #1150 */

/* below intuition V36 (fixed #1150): the library's requester has one
   gadget or two ('|' separated); with more it shows none and returns 0 */
static void BLITZ_ERRCHK gadchk(const UBYTE *g)
{
    ULONG n = 0;

    if (!intuibase || intuibase->LibNode.lib_Version >= 36)
        return;
    while (*g)
        if (*g++ == '|')
            n++;
    if (n > 1)
        BLITZ_RUNERR(toomanygads);
}

/* the bytes the repeated arguments below sp take in the argument array
   (a string, long, Quick or Float 4, a word or byte 2), as doargs stores
   them; `cnt` arguments, counted only until there are more than 64 */
static ULONG argbytes(const UBYTE *sp, UWORD cnt)
{
    ULONG n = 0;

    while (cnt-- && n <= sizeof ez.args2) {
        UWORD type = BL_POPW(sp);

        if (type == BL_STRING) {
            sp -= 8;
            n += 4;
        } else if (type == BL_WORD || type == BL_BYTE) {
            sp -= 2;
            n += 2;
        } else {
            sp -= 4;
            n += 4;
        }
    }
    return n;
}

/**
 * @brief The error check of EZRequest title$,body$,gadget$,var..
 * (debugger only).
 *
 * Fixed (#748), not in the original (neither form limits its arguments;
 * the library had no error check area): arguments that take more than
 * the 64 bytes of the argument array (16 longs), which doargs would write
 * behind it.  The release code is unchanged (the number of arguments is
 * in the program's source).
 *
 * Fixed (#1150): below intuition V36 also more than two gadgets, as
 * ezgad3chk.
 *
 * Errors: "Too Many EZRequest Arguments"; "More than two EZRequest
 * gadgets need Kickstart 2.0".
 * @param args    a2: the argument stack: the three lengths, below them the
 *                repeated arguments
 * @param reps    d7.w: the number of arguments
 * @param gadget  d2: the gadgets
 */
static void BLITZ_ERRCHK ezargschk_c(const UBYTE *args BL_REG(a2), UWORD reps BL_REG(d7),
                                     const UBYTE *gadget BL_REG(d2))
{
    if (argbytes(args - 12, reps) > sizeof ez.args2)
        BLITZ_RUNERR(toomanyargs);
    gadchk(gadget);                                     /* fixed (#1150) */
}

/**
 * @brief The error check of EZRequest title$,body$,gadget$ (debugger
 * only).
 *
 * Fixed (#1150), not in the original (it called EasyRequestArgs also on
 * Kickstart 1.x, a crash): below intuition V36 more than two gadgets,
 * which the library's AutoRequest cannot show (the release code shows no
 * requester and returns 0).
 *
 * Errors: "More than two EZRequest gadgets need Kickstart 2.0".
 * @param gadget  d2: the gadgets
 */
static void BLITZ_ERRCHK ezgad3chk_c(const UBYTE *gadget BL_REG(d2))
{
    gadchk(gadget);
}

/**
 * @brief The error check of EZRequest body$,gadget$ (debugger only).
 *
 * As ezgad3chk (fixed #1150).
 *
 * Errors: "More than two EZRequest gadgets need Kickstart 2.0".
 * @param gadget  d1: the gadgets
 */
static void BLITZ_ERRCHK ezgad2chk_c(const UBYTE *gadget BL_REG(d1))
{
    gadchk(gadget);
}

/**
 * @brief The error check of EZRequestArgs args.. (debugger only).
 *
 * As ezargschk (fixed #748): more than 64 bytes of arguments.  Within 64
 * bytes the format text cannot overflow its 160 bytes (at most 32 words,
 * 3 bytes each with the separator: 96).
 *
 * Errors: "Too Many EZRequest Arguments".
 * @param args  a2: the argument stack, the repeated arguments
 * @param reps  d7.w: the number of arguments
 */
static void BLITZ_ERRCHK ezreqargschk_c(const UBYTE *args BL_REG(a2), UWORD reps BL_REG(d7))
{
    if (argbytes(args, reps) > sizeof ez.args2)
        BLITZ_RUNERR(toomanyargs);
}

BLITZ_ERRCHK_THUNK(ezargschk, ezargschk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ezreqargschk, ezreqargschk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ezgad3chk, ezgad3chk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ezgad2chk, ezgad2chk_c, "d0-d1/a0-a1");
