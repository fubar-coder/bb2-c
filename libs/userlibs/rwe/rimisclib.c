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
 *                        var..) they are the format arguments
 *   EZRequestArgs args.. the body is built from the arguments: %s / %ld /
 *                        %d per type, separated by the EZSeparator
 *                        character ('|': a line feed), gadget "Ok"
 *   InitArgParse s$      copies s$ to a 256 byte buffer; NextArg$ gives
 *                        the next word (quotes group, "" inside quotes is
 *                        a quote), NextArgChar$ c everything up to c
 *   ProgPath$/Dir$/Name$ GetProgramDir/GetProgramName (the task name if
 *                        there is no program name)
 *   OpenFontPrefs        reads ENV:SYS/Font.Prefs into a 16K buffer;
 *                        DefaultFontHeight/Name(type) search its FONT
 *                        chunks (fp_Type), CloseFontPrefs frees it
 *   Flip$, LtoS, ASCRaw, RawASC, BInstr, PortSigBit, FromBlitz,
 *   WorkbenchHere, TimerReset (CIA-A TOD), ClickWait (left mouse button)
 *
 * Everything is C behind the thunks; dos and intuition through the NDK
 * inlines, floatquicklib's fltoqu through BLITZ_ALIBJSR_A6.
 *
 * BUG(orig), kept:
 *   - WorkbenchHere takes IntuitionBase from d0 (MOVE.l intuibase,a0 is
 *     overwritten by MOVE.l d0,a0), and the function has no arguments: d0
 *     is whatever the program left there (#759).  The search for
 *     "Workbench" in the screen titles never ends the list (no 0 check:
 *     it reads the "screen" at 0 and goes on), so it returns -1 or hangs
 *     (#761); a title that ends with the start of "Workbench" ("...Work")
 *     restarts the same screen for ever (#762); after a partial match it
 *     goes on with the next screen ("WWorkbench" is not found in its own
 *     title) (#760).
 *   - Flip$ of a string with 0 or 1 characters swaps 65536 byte pairs
 *     around the string (DBRA with -1) (#751).
 *   - InitArgParse copies without a limit into its 256 byte buffer (the
 *     original overwrites the code behind it, the C build other data)
 *     (#752).
 *   - NextArgChar$ does not check the parse pointer: before InitArgParse
 *     and after NextArg$ came to the end it reads from address 0 (#754).
 *   - NextArg$: after "" inside quotes, the next quote copies the
 *     character behind it (#755).
 *   - EZRequestArgs writes no 0 behind the format text it builds (160
 *     byte buffer): a shorter text than the one before keeps the end of
 *     the old one (#747).  Neither form limits the arguments (16 longs)
 *     (#748).
 *   - A Quick argument is passed as its integer part zero extended
 *     (SWAP:ANDI.l #$ffff): negative values become 65536 - n (#750); a
 *     Float is converted with floatquicklib's fltoqu with the caller's a6
 *     (not the ffp base) and the result written back onto the argument
 *     stack (#749).
 *   - EZRequestArgs without arguments reads one type word from a2 (the
 *     count 0 runs the DBRA loop 65536 times) (#746).
 *   - ProgPath$ leaves a3 behind the 0 of the result (d0 is right) (#743)
 *     and reads the byte before the result when the directory name is
 *     empty (#758); in ProgDir$/ProgName$ a3 is behind the 0 too (#743).
 *   - LtoS replaces the bytes 0-31 and 128-255 by 128 (signed compare)
 *     (#753).
 *   - ASCRaw/RawASC do not check the range: a raw key beyond the 114
 *     entries of the table (e.g. a key up code) reads the code behind it,
 *     RawASC of a character not in the table searches until it finds the
 *     byte somewhere (DIFF: what is behind the table differs, see the
 *     notes) (#744).
 *   - OpenFontPrefs does not check AllocMem (#756), and a second
 *     OpenFontPrefs loses the first buffer (#757).  DefaultFontHeight/Name
 *     step through the buffer by 164 bytes (the size of a FONT chunk)
 *     until its end, through the uncleared memory behind the file, and
 *     read the last chunk up to 26 bytes behind the buffer (#745).
 *
 * Original behaviour that is no defect: NextArg$ ends a quoted word at
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
 * docs/libs/cmdlibs-port.md "General".  EZRequest/EZRequestArgs leave
 * a3 = the argument array (0 without) like the original: the NDK inline
 * of EasyRequestArgs loads the fixed register a3 and does not restore it.
 * The format text of EZRequestArgs is written byte by byte (the
 * original's MOVE.w at odd addresses is an address error on a 68000);
 * FromBlitz compares bytes (the original CMPM.l, also at an odd task
 * name).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/execbase.h>
#include <dos/dos.h>
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
void rimisclib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rimisclib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rimisclib, init, BL_NORET, finit, 0);

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
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy), 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy2), 0, 0);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy3), 0, 0);
BLITZ_REPARGS(3, 1, BL_STRING, BL_STRING, BL_STRING, BL_UNKNOWN | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy4), 0, 0);
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
BLITZ_LIBS(LIB_windowslib, BL_LA(0) | BL_ITEM | BL_PD(0));
BLITZ_SUBS(_ezoutput, 0, 0);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_ezoutput2, 0, 0);
BLITZ_NAME("EZOutput", "[win#]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_REPARGS(0, 1, BL_UNKNOWN | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_doeasy6), 0, 0);
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

BLITZ_NULLSUB_AT(finit, rimisclib_libfinit, 0, 0, LIB_doslib, BL_LA(6));
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

/* copy s with its 0 to a3, a3 behind the 0; the length */
static ULONG copyout(const UBYTE *s)
{
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
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
 * Searches the screen titles from IntuitionBase->FirstScreen.
 * BUG(orig): IntuitionBase is taken from d0, which is no argument (#759);
 * the screen list is never ended (no 0 check), so it returns BB_TRUE or
 * hangs (#761); a title that ends with the start of "Workbench" restarts
 * the same screen for ever (#762); after a partial match it goes on with
 * the next screen (#760).
 * @param ib  d0: whatever the program left there, used as IntuitionBase
 * @return d0: BB_TRUE when found (it never returns BB_FALSE)
 */
static BB_BOOL BLITZ_ROUTINE iswbhere_c(const struct IntuitionBase *ib BL_REG(d0))
{
    const struct Screen *scr = ib->FirstScreen;
    const UBYTE *t, *s;
    UBYTE c;

    thisscreen = (const UBYTE *)scr;
    for (;;) {
    restart:                                            /* 'loop */
        t = (const UBYTE *)titlestring;
        s = scr->Title;
        for (;;) {                                      /* 'findloop */
            c = *s++;
            if (!c)
                break;
            if (c != *t)
                continue;
            t++;
            for (;;) {                                  /* 'innerloop */
                if (!*t)
                    return BB_TRUE;
                if (!*s)
                    goto restart;                       /* BUG(orig): same screen (#762) */
                c = *t++;
                if (c != *s++)
                    goto next;
            }
        }
    next:
        scr = scr->NextScreen;                          /* BUG(orig): no 0 check (#761) */
    }
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
 * Highest byte first, written to a3 without a 0.  BUG(orig) (#753): the
 * bytes 0-31 and 128-255 become 128 (a signed CMPI.b #32).
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
 * From the original's table of 114 entries; BUG(orig) (#744): the code
 * is not checked, a key beyond the table reads behind it.
 * @param key  d0.w: the raw key code (signed)
 * @return d0: 1, the length of the string at a3
 */
static ULONG BLITZ_ROUTINE ascraw_c(UWORD key BL_REG(d0))
{
    *blitz_string++ = rawtable[(WORD)key];
    *blitz_string = 0;
    return 1;
}

/**
 * @brief RawASC(char): the raw key code of a character.
 *
 * The first index of the string's first character in the table.
 * BUG(orig) (#744): a character not in the table is searched for behind
 * it until the byte is found somewhere.
 * @param s  d0: the string
 * @return d0.w: the raw key code
 */
static ULONG BLITZ_ROUTINE rawasc_c(const UBYTE *s BL_REG(d0))
{
    UBYTE c = *s;
    UWORD i = 0;

    while (rawtable[(WORD)i] != c)
        i++;                                            /* ADDQ.w #1,d0 */
    return i;
}

/**
 * @brief Flip$(String$): the string reversed.
 *
 * Reversed in place: the argument is the string below a3, a3 stays.
 * BUG(orig) (#751): a string of 0 or 1 characters swaps 65536 byte pairs
 * (DBRA with -1).
 * @param lens  a2: the argument stack, -4(a2) the length of the string
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doflip_c(const ULONG *lens BL_REG(a2))
{
    ULONG len = lens[-1];
    UBYTE *front = blitz_string - len - 1, *e = blitz_string - 2;
    UWORD n = (UWORD)((len >> 1) - 1);                  /* BUG(orig): len < 2 (#751) */

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
 * Copies it into the 256 byte buffer with four more 0 behind it.
 * BUG(orig) (#752): the copy has no limit.
 * @param s  d0: the string
 */
static void BLITZ_ROUTINE parseinit_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *dst = argbuffer;

    while ((*dst++ = *s++) != 0)
        ;
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0;
    parseadr = argbuffer;
    curadr = argbuffer;
}

/**
 * @brief NextArg$: the next word of the InitArgParse string.
 *
 * Leading spaces are skipped; quotes group (a quoted word ends at the
 * closing quote even if no space follows, as ReadItem does), "" inside
 * quotes is a quote.  The word goes to a3 without a 0 (none needed,
 * #257).  BUG(orig) (#755): after "" inside quotes, the next quote copies
 * the character behind it.
 * @return d0: the length; 0 at the end (also before InitArgParse)
 */
static ULONG BLITZ_ROUTINE nextparsearg_c(void)
{
    UBYTE *pos = curadr, *start = blitz_string, c;
    ULONG quotes = 0;                                   /* d7 */

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
        if (!c || c == ' ')
            goto endthis;
        if (c == '"')
            break;
        *blitz_string++ = c;
    }
    quotes++;                                           /* 'insidequotedloop */
    for (;;) {                                          /* 'quoteloop */
        c = *pos++;
        if (!c)
            break;
        if (c != '"') {
            *blitz_string++ = c;
            continue;
        }
        quotes++;                                       /* 'gotquote */
        if (!(quotes & 1) && *pos != '"')
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
 * position goes behind char.  BUG(orig) (#754): the parse pointer is not
 * checked: before InitArgParse and after NextArg$ came to the end it
 * reads from address 0.
 * @param ch  d0.b: the character
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE nextparseuntilchar_c(UBYTE ch BL_REG(d0))
{
    UBYTE *pos = curadr, c;
    ULONG n = 0;

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

/* doeasybegin: '|' to line feeds in the body (the compiler's copy of
   the argument), EasyRequestArgs on the EZOutput window */
static ULONG doeasybegin(void)
{
    UBYTE *p = (UBYTE *)ez.es.es_TextFormat, c;

    while ((c = *p++) != 0)
        if (c == '|')
            p[-1] = 10;
    return (ULONG)EasyRequestArgs(win, &ez.es, NULL, ez.args);
}

/* the argument array (and with fmt the format text) from the repeated
   #unknown|#push arguments below sp: per argument the type word, below
   it the value (a string: its length, below that the address); `cnt`
   arguments (DBRA: 0 = 65536).  a6 is passed to fltoqu. */
static void doargs(const UBYTE *sp, UWORD cnt, UBYTE *fmt, const void *caller_a6)
{
    UBYTE *out = (UBYTE *)ez.args2;
    UWORD n = (UWORD)(cnt - 1);

    flag = 0;
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
            (void)BL_POPL(sp);                          /* the length */
            *(ULONG *)out = BL_POPL(sp);
            out += 4;
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
            if (type != BL_QUICK) {
                /* MOVE.l -(a2),d0:ALibJsr $d301:MOVE.l d0,(a2)+ */
                v = BL_POPL(sp);
                *(ULONG *)(UBYTE *)sp = BLITZ_ALIBJSR_A6(TOK_fltoqu, caller_a6, v);
                sp += 4;
            }
            v = BL_POPL(sp);
            *(ULONG *)out = v >> 16;                    /* SWAP:ANDI.l #$ffff */
            out += 4;
            if (fmt) {
                *fmt++ = '%';
                *fmt++ = 'l';
                *fmt++ = 'd';
            }
        }
    } while (n-- != 0);
}

/**
 * @brief EZRequest title$,body$,gadget$: an EasyRequest on the EZOutput
 * window ('|' in the body: a line feed).
 *
 * DIFF: a2 stays (the original: 0); a3 = 0 afterwards, like the
 * original (the NDK inline of EasyRequestArgs).
 * @param title   d0: the title
 * @param body    d1: the body text
 * @param gadget  d2: the gadgets ('|' separated)
 * @return d0: what EasyRequestArgs returns (the gadget number)
 */
static ULONG BLITZ_ROUTINE doeasy_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                    STRPTR gadget BL_REG(d2))
{
    ez.es.es_Title = title;
    ez.es.es_TextFormat = body;
    ez.es.es_GadgetFormat = gadget;
    ez.args = NULL;
    return doeasybegin();
}

/**
 * @brief EZRequest body$,gadget$: an EasyRequest without a title.
 * @param body    d0: the body text
 * @param gadget  d1: the gadgets
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy2_c(STRPTR body BL_REG(d0), STRPTR gadget BL_REG(d1))
{
    ez.es.es_GadgetFormat = gadget;
    ez.es.es_TextFormat = body;
    ez.es.es_Title = NULL;
    ez.args = NULL;
    return doeasybegin();
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
 * See doargs; BUG(orig): no limit of the 16 argument longs (#748), Quick
 * and Float arguments as in #749/#750.  a3 = the argument array
 * afterwards, like the original.
 * @param title      d0: the title
 * @param body       d1: the body (format) text
 * @param gadget     d2: the gadgets
 * @param args       a2: the argument stack: the three lengths, below
 *                   them the repeated arguments
 * @param reps       d7.w: the number of arguments
 * @param caller_a6  a6: the caller's a6, passed to fltoqu (#749)
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy4_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                     STRPTR gadget BL_REG(d2), UBYTE *args BL_REG(a2),
                                     UWORD reps BL_REG(d7), const void *caller_a6 BL_REG(a6))
{
    ez.es.es_Title = title;
    ez.es.es_TextFormat = body;
    ez.es.es_GadgetFormat = gadget;
    ez.args = ez.args2;
    doargs(args - 12, reps, NULL, caller_a6);
    return doeasybegin();
}

/**
 * @brief EZRequestArgs args..: an EasyRequest whose body is built from the
 * arguments.
 *
 * %s / %ld / %d per type, separated by the EZSeparator character ('|': a
 * line feed), gadget "Ok", no title.  BUG(orig): no 0 behind the format
 * text (160 byte buffer, #747); no limit of the arguments (#748);
 * without arguments a type word is read from a2 and the count 0 runs the
 * loop 65536 times (#746); Quick/Float as in #749/#750.
 * @param args       a2: the argument stack, the repeated arguments
 * @param reps       d7.w: the number of arguments
 * @param caller_a6  a6: the caller's a6, passed to fltoqu (#749)
 * @return d0: what EasyRequestArgs returns
 */
static ULONG BLITZ_ROUTINE doeasy6_c(UBYTE *args BL_REG(a2), ULONG reps BL_REG(d7),
                                     const void *caller_a6 BL_REG(a6))
{
    numargs = reps & 0xffff;
    ez.es.es_TextFormat = ezargsbuf;
    ez.args = ez.args2;
    doargs(args, (UWORD)reps, ezargsbuf, caller_a6);    /* BUG(orig): no 0 (#747) */
    ez.es.es_GadgetFormat = (STRPTR)okgad;
    ez.es.es_Title = NULL;
    return doeasybegin();
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

/**
 * @brief ProgPath$: the program's directory and name.
 *
 * GetProgramDir (without one: the current directory) and
 * GetProgramName (empty: the task name); fills the buffers of ProgDir$
 * and ProgName$.  BUG(orig): a3 stays behind the 0 of the result (d0 is
 * right) (#743); with an empty directory name it reads the byte before
 * the result (#758).
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE doprogpath_c(void)
{
    BPTR lock = GetProgramDir();
    const UBYTE *s;
    UBYTE *dst, c;
    ULONG n;

    if (!lock)
        GetCurrentDirName(pd.progdir, 255);
    else
        NameFromLock(lock, pd.progdir, 255);
    pd.rawprogname[0] = 0;
    GetProgramName(pd.rawprogname, 255);
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
    blitz_string--;
    n--;
    if (blitz_string[-1] != ':') {
        n++;
        *blitz_string++ = '/';
    }
    s = pd.progname;
    do {
        n++;
        c = *s++;
        *blitz_string++ = c;
    } while (c);
    return n;
}

/**
 * @brief ProgDir$: the program's directory.
 *
 * Calls ProgPath$ only while its buffer is empty (the directory does not
 * change).  BUG(orig) (#743): a3 stays behind the 0.
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
 * Calls ProgPath$ only while its buffer is empty.  BUG(orig) (#743): a3
 * stays behind the 0.
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
 * BUG(orig): AllocMem is not checked (#756); a second OpenFontPrefs
 * loses the first buffer (#757).
 * @return d0: BB_TRUE if it is a FORM PREF with PRHD and a FONT chunk
 *         behind it; else BB_FALSE, the buffer freed (the tail call of
 *         CloseFontPrefs, which returns 0)
 */
static BB_BOOL BLITZ_ROUTINE openfontprefs_c(void)
{
    const ULONG *p;

    fontloc = AllocMem(FONTBUF, 0);                     /* BUG(orig): unchecked (#756) */
    fontfh = Open((CONST_STRPTR)fontname, MODE_OLDFILE);
    if (!fontfh)
        return closefontprefs_c(), BB_FALSE;
    Read(fontfh, fontloc, FONTBUF);
    Close(fontfh);
    p = (const ULONG *)fontloc;
    if (*p++ != BL_ID4('F', 'O', 'R', 'M'))
        return closefontprefs_c(), BB_FALSE;
    p++;
    if (*p++ != BL_ID4('P', 'R', 'E', 'F'))
        return closefontprefs_c(), BB_FALSE;
    if (*p++ != BL_ID4('P', 'R', 'H', 'D'))
        return closefontprefs_c(), BB_FALSE;
    p = (const ULONG *)((const UBYTE *)p + *p);         /* ADD.l (a0),a0 */
    p++;
    if (*p++ != BL_ID4('F', 'O', 'N', 'T'))
        return closefontprefs_c(), BB_FALSE;
    p++;
    fontpos = (UBYTE *)p;
    return BB_TRUE;
}

/* the FONT chunk with fp_Type = type, stepping by 164 bytes to the end of
   the buffer; NULL if none.  BUG(orig) (#745): through the uncleared
   memory behind the file, and the last chunk up to 26 bytes behind the
   buffer */
static const struct FontPrefs *findfont(UWORD type)
{
    const UBYTE *end, *p;

    if (!fontpos)
        return NULL;
    end = fontloc + FONTBUF;
    for (p = fontpos; (LONG)end >= (LONG)p; p += FONTCHUNK)
        if (((const struct FontPrefs *)p)->fp_Type == type)
            return (const struct FontPrefs *)p;
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
 * @return d0: the length of fp_Name (at a3, a3 behind the 0); 0 ("") if
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
