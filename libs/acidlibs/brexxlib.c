/*
 * brexxlib - command library 132: BREXX macros (tapes of input events):
 * Record PlayBack SaveTape LoadTape AbsMouse RelMouse MouseButton
 * ClickButton MacroKey TapeTrap QuietTrap Type QuickPlay XStatus PlayWait
 * FreeMacroKey; object library "Tape" (16 bytes: first event, start time
 * secs, micros; maximum 5; _free = freetape).
 *
 * C translation of bblibsrc/brexxlib; binary of the CD's acidlibs checked:
 * same as the source.
 *
 * init opens input.device (own IOStdReq + MsgPort, the result of
 * OpenDevice is not checked) and adds an input handler (priority 51, "one
 * more than intuition"); allocates a signal (mybit/mysig) and remembers
 * the task.  finit stops a playback, removes the handler, closes the
 * device and frees both signals.
 *
 * A tape is a list of InputEvents (22 bytes, memlib globalloc; the time
 * stamps are relative to the start of the recording).  `mode`: 0 nothing,
 * 1 record, 2 playback; bit 15 = "inject dumevent first": the handler then
 * clears bit 15 and puts dumevent in front of the list; at the start of a
 * Record or PlayBack (`newstart`) it also remembers the time stamp of the
 * first real event (firstval).  The handler
 *   - records RAWKEY, RAWMOUSE and POINTERPOS events (copies, the list is
 *     passed on unchanged),
 *   - plays back: the tape events whose time (minus the tape's start
 *     time) is not later than an incoming event's time (minus firstval) -
 *     or all, with QuickPlay - are linked in front of that incoming event
 *     (the tape's own nodes; the hijacked links, up to 9 per call, are
 *     restored on the next call); at the end of the tape mode = 0 and,
 *     for PlayWait, Signal(mytask, mysig),
 *   - with mode 0 checks RAWKEY events for a MacroKey and starts its tape.
 * Simple events (Type, AbsMouse, RelMouse, MouseButton, ClickButton) are
 * built in dumevent and either executed (bit 15 of mode) or, after TapeTrap
 * Tape#, appended to that tape (with QuietTrap On only appended).
 *
 * File format (SaveTape/LoadTape): the 8 byte start time, then 18 bytes
 * per event (the event without its link).
 *
 * Fixed (#95): TapeTrap without a tape number fell through (no RTS) into
 * TapeTrap Tape#: tapeadd = the caller's a3 (Blitz string work pointer),
 * so the redirection never ended and the events were chained into the
 * memory at a3.  Here it clears tapeadd and returns (bug-orig test
 * brexxlib-tapetrap-a3).
 * Fixed (#104): a failed AllocSignal(-1) was not noticed: mybit = $FFFF,
 * mysig = 0 (LSL.l by 63), so PlayWait waited with Wait(0) for ever, and
 * the finit called FreeSignal(255) and FreeSignal(65535).  Here a failed
 * signal stays unused: PlayWait returns at once without one, and the
 * finit frees only the signals it got (bug-orig test brexxlib-allocsignal).
 * Fixed (#96): appending an event to a tape cleared `mode`, and an
 * executed event set it to $8000: every simple event ended a running
 * Record or PlayBack (without the start time, without restoring the
 * hijacked link).  Here addtape only sets bit 15 for an executed event,
 * and the handler takes firstval only at a start (newstart), so that the
 * time base of the running Record or PlayBack stays (bug-orig test
 * brexxlib-addtape-mode).
 * Fixed (#103): XStatus returned the mode word with bit 15 ($8001, $8002,
 * $8000 until the handler had run); here without it (bug-orig test
 * brexxlib-xstatus-bit15).
 * Fixed (#97): ClickButton's second (button up) call got d0 as left by
 * the first one's addtape: 0 (left) for an executed event, the QuietTrap
 * value when the event went to a tape - ClickButton 0 under TapeTrap with
 * QuietTrap On recorded "left down, RIGHT up".  Here both events are of
 * the given button (bug-orig test brexxlib-clickbutton-d0, corpus t101).
 * Fixed (#98): AbsMouse and RelMouse both made POINTERPOS events (RelMouse
 * with the qualifier RELATIVEMOUSE); RelMouse makes a RAWMOUSE event (bug-
 * orig test brexxlib-relmouse-class, corpus t101).
 * Fixed (#100): the playback compared the time of every tape event with
 * the FIRST event of the incoming list only and linked the due ones in
 * front of the list: a tape event due between two incoming events of one
 * call came behind them.  Here each incoming event gets the tape events
 * due by its time in front of it (bug-orig test brexxlib-playback-time).
 * Fixed (#101): LoadTape read the start time unchecked, so a file shorter
 * than 8 bytes gave a tape with a start time from its bytes; here such a
 * file leaves the tape empty with the start time 0 (bug-orig test
 * brexxlib-loadtape-unchecked).  The format has no identifier, so any
 * file of 8 bytes or more still loads, and a short event record ends the
 * load without an error, as before.
 * BUG(orig), kept:
 *  - PlayWait restarts the given tape (with signal) when the low byte of
 *    mode is 2 and waits; otherwise it returns at once. (#99)
 *  - Type: a character rawkeylib cannot convert gives code $FFFF. (#102)
 * DIFF: "Can't Open File" (SaveTape/LoadTape) is `BEQ err1` into the
 * error check area in the original (without the debugger a jump behind
 * the library); here trap and message are in the normal code.
 *
 * The input handler runs on input.device's task; it calls memlib
 * globalloc from there (as the original).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <exec/interrupts.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (aliases / thunks, defined below) */
void dorecord(void), dorecord2(void), doplayback(void), doplayback2(void);
void dosavetape(void), doloadtape(void), doabsmouse(void), dorelmouse(void);
void domousebutton(void), doclickbutton(void), domacrokey(void);
void dotapeadd(void), dotapeadd2(void), dotapemode(void), dotype(void);
void doqplay(void), doxstatus(void), doplaywait(void), dofreemacrokey(void);
void brexxlib_codeinit(void), brexxlib_codefinit(void), freetape(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#brexxlib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_brexxlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

#define TAPEOBJ LIB_brexxlib, BL_IA3|BL_PD0     /* a3 = tape object */

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dorecord, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(TAPEOBJ);
BLITZ_SUBS(dorecord2, 0, 0);
BLITZ_NAME("Record", "[Tape#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doplayback, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(TAPEOBJ);
BLITZ_SUBS(doplayback2, 0, 0);
BLITZ_NAME("PlayBack", "[Tape#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(TAPEOBJ, LIB_doslib, BL_LA6);
BLITZ_SUBS(dosavetape, 0, 0);
BLITZ_NAME("SaveTape", "Tape#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(TAPEOBJ, LIB_doslib, BL_LA6);
BLITZ_SUBS(doloadtape, 0, 0);
BLITZ_NAME("LoadTape", "Tape#,filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doabsmouse, 0, 0);
BLITZ_NAME("AbsMouse", "X,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dorelmouse, 0, 0);
BLITZ_NAME("RelMouse", "X Offset,Y Offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domousebutton, 0, 0);
BLITZ_NAME("MouseButton", "Button,On|Off : 0=Left");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doclickbutton, 0, 0);
BLITZ_NAME("ClickButton", "Button : 0=Left");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(TAPEOBJ);
BLITZ_SUBS(domacrokey, 0, 0);
BLITZ_NAME("MacroKey", "Tape#,Rawkey,Qualifier");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dotapeadd, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(TAPEOBJ);
BLITZ_SUBS(dotapeadd2, 0, 0);
BLITZ_NAME("TapeTrap", "[Tape#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dotapemode, 0, 0);
BLITZ_NAME("QuietTrap", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(dotype, 0, 0);
BLITZ_NAME("Type", "String$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doqplay, 0, 0);
BLITZ_NAME("QuickPlay", "On|Off");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doxstatus, 0, 0);
BLITZ_NAME("XStatus", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(TAPEOBJ);
BLITZ_SUBS(doplaywait, 0, 0);
BLITZ_NAME("PlayWait", "");

BLITZ_NULLSUB_AT(init, brexxlib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, brexxlib_codefinit, 0, 0);

BLITZ_DUMTOKE(_toke, "Tape", "A BREXX Macro");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dofreemacrokey, 0, 0);
BLITZ_NAME("FreeMacroKey", "Rawkey,Qualifier");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freetape, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 4);

/* ------------------------------------------------------------ variables */

/* a tape object */
struct tape {
    struct InputEvent *first;
    struct timeval start;               /* start time (firstval of Record) */
};
static_assert(sizeof(struct tape) == 12);  /* of the 16 byte object */

/* a macro key (10 bytes) */
struct mkey {
    struct mkey *next;
    struct tape *tape;
    UBYTE rawkey, qual;
};
static_assert(sizeof(struct mkey) == 10);

#define IESIZE 22                       /* ief-ie */

static volatile UWORD mode;             /* 0, 1 record, 2 playback; bit 15 */
static volatile UWORD newstart;         /* bit 15 starts a Record/PlayBack */
static struct InputEvent *lastrec;      /* last node of the tape (record),
                                           node whose link is hijacked (play) */
static struct InputEvent *lastlink;     /* its real link */
#define NHIJACK 8                       /* more links of one handler call */
static struct InputEvent *hjnode[NHIJACK], *hjlink[NHIJACK];
static UWORD nhijack;
static struct timeval firstval;          /* time of the first event */
static struct tape *firsta3;            /* tape being recorded */
static struct timeval playval;           /* start time of the tape played */
static struct tape *tapeadd;            /* tape for simple events */
static UWORD trap_mode = 1;
static UWORD qplay;
static ULONG mysig;                     /* "playback finished" */
static struct Task *mytask;
static UWORD mybit;
static UWORD sigit;                     /* signal at the end of a playback? */
static struct InputEvent dumevent __attribute__((aligned(2))) = { .ie_Class = IECLASS_POINTERPOS };
static struct mkey *mkeys;

/* the handler's Interrupt structure: priority 51 */
static struct InputEvent *my_handler(struct InputEvent *ev BL_REG(a0));
static struct Interrupt my_interrupt = {
    .is_Node = { .ln_Pri = 51 },
    .is_Data = 0,
    .is_Code = (void (*)(void))my_handler,
};

static const char devname[] = "input.device";

/* IOStdReq and its reply port, cleared together by openinput */
static struct {
    struct IOStdReq io;
    struct MsgPort port;
} io_request;

/* dumtape: one event as read from a file */
static UWORD dumtape[9];

/* ------------------------------------------------------------ helpers */

static void *globalloc(ULONG size)
{
    struct bl_regs r = { .d0 = size, .d1 = MEMF_PUBLIC };

    return (void *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* copy n words */
static void copyw(UWORD *d, const UWORD *s, UWORD n)
{
    do
        *d++ = *s++;
    while (--n);
}

/* waitlast: wait until the handler has taken a pending dumevent */
static void waitlast(void)
{
    while ((WORD)mode < 0)
        ;
}

/* clearie: waitlast, then clear code, qualifier, x/y and time stamp */
static void clearie(void)
{
    waitlast();
    dumevent.ie_Code = 0;
    dumevent.ie_Qualifier = 0;
    *(ULONG *)&dumevent.ie_X = 0;
    dumevent.ie_TimeStamp.tv_secs = 0;
    dumevent.ie_TimeStamp.tv_micro = 0;
}

/* inject: let the handler put dumevent in front of the next events; bit
   15 is set on the mode, under Forbid (the handler may end a playback and
   clear the mode meanwhile).  Fixed (#96): the original set the mode to
   $8000, which ended a running Record or PlayBack. */
static void inject(void)
{
    Forbid();
    mode |= 0x8000;
    Permit();
}

/* addtape: append dumevent to tapeadd, or let the handler inject it.
   Fixed (#96): the mode of a running Record or PlayBack stays (the
   original cleared it). */
static void addtape(void)
{
    struct InputEvent *n, **link;

    if (tapeadd) {
        n = globalloc(IESIZE);
        copyw((UWORD *)n, (const UWORD *)&dumevent, IESIZE / 2);
        link = &tapeadd->first;         /* find where in the tape */
        while (*link)
            link = &(*link)->ie_NextEvent;
        *link = n;
        n->ie_NextEvent = 0;
        if (trap_mode)
            return;
    }
    inject();
}

/* ---------------------------------------------------- simple settings */

/**
 * @brief XStatus: what BREXX is doing.
 *
 * Fixed (#103): without bit 15, the handler's flag (the original returned
 * $8001, $8002 or $8000 until the handler had run).
 *
 * @return d0: mode - 0 nothing, 1 record, 2 playback (MOVEQ #0:MOVE)
 */
static ULONG BLITZ_ROUTINE doxstatus_c(void)
{
    return mode & 0x7fff;                       /* MOVEQ #0:MOVE */
}
BLITZ_ALIAS(doxstatus, doxstatus_c);

/**
 * @brief QuickPlay On|Off: play tapes without their timing.
 *
 * On: the handler plays all events of the tape at once.
 *
 * @param v  d0.w: 0 = Off, otherwise On
 */
static void BLITZ_ROUTINE doqplay_c(ULONG v BL_REG(d0))
{
    qplay = (UWORD)v;
}
BLITZ_ALIAS(doqplay, doqplay_c);

/**
 * @brief TapeTrap: end the redirection of the simple events to a tape.
 *
 * Type, AbsMouse, RelMouse, MouseButton and ClickButton are executed
 * again.  Fixed (#95): the original falls through into TapeTrap Tape#
 * (no RTS behind CLR.l tapeadd), so tapeadd became the caller's a3.
 */
static void BLITZ_ROUTINE dotapeadd_c(void)
{
    tapeadd = 0;                                /* CLR.l tapeadd:RTS */
}
BLITZ_ALIAS(dotapeadd, dotapeadd_c);

/**
 * @brief TapeTrap Tape#: append the simple events to a tape.
 *
 * Type, AbsMouse, RelMouse, MouseButton and ClickButton then append their
 * events to this tape (with QuietTrap On only appended, otherwise also
 * executed).
 * Fixed (#96): appending an event no longer clears `mode`.
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 */
static void BLITZ_ROUTINE dotapeadd2_c(void)
{
    tapeadd = (struct tape *)blitz_string;
}
BLITZ_ALIAS(dotapeadd2, dotapeadd2_c);

/**
 * @brief QuietTrap On|Off: whether trapped events are only appended.
 *
 * On (the default, 1 after init): an event under TapeTrap goes to the
 * tape only; Off: it is also executed.
 *
 * @param v  d0.w: 0 = Off, otherwise On
 */
static void BLITZ_ROUTINE dotapemode_c(ULONG v BL_REG(d0))
{
    trap_mode = (UWORD)v;
}
BLITZ_ALIAS(dotapemode, dotapemode_c);

/* ----------------------------------------------------------- macro keys */

/* free the macro key d0 = rawkey, d1 = qualifier (the first match) */
static void freemacrokey(UBYTE key, UBYTE qual)
{
    struct mkey *prev, *cur = (struct mkey *)&mkeys;

    for (;;) {
        if (!cur->next)
            return;
        prev = cur;
        cur = cur->next;
        if (cur->rawkey == key && cur->qual == qual)
            break;
    }
    prev->next = cur->next;                     /* fix link */
    globfree(cur, 10);                          /* free it! */
}

/**
 * @brief FreeMacroKey Rawkey,Qualifier: remove a macro key.
 *
 * Frees the first macro key with this raw key and qualifier (bytes);
 * nothing if there is none.
 *
 * @param key  d0: the raw key code (low byte)
 * @param qual  d1: the qualifier (low byte)
 */
static void BLITZ_ROUTINE dofreemacrokey_c(ULONG key BL_REG(d0), ULONG qual BL_REG(d1))
{
    freemacrokey((UBYTE)key, (UBYTE)qual);
}
BLITZ_ALIAS(dofreemacrokey, dofreemacrokey_c);

/**
 * @brief MacroKey Tape#,Rawkey,Qualifier: a key that plays a tape.
 *
 * Replaces a macro key with the same key and qualifier and puts the new
 * one first in the list (10 bytes, memlib globalloc).  The input handler
 * checks RAWKEY events against the list while nothing is recorded or
 * played and starts the tape.
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 * @param key  d1: the raw key code (low byte)
 * @param qual  d2: the qualifier (low byte)
 */
static void BLITZ_ROUTINE domacrokey_c(ULONG key BL_REG(d1), ULONG qual BL_REG(d2))
{
    struct mkey *m;

    freemacrokey((UBYTE)key, (UBYTE)qual);
    m = globalloc(10);
    m->next = mkeys;
    mkeys = m;
    m->tape = (struct tape *)blitz_string;
    m->rawkey = (UBYTE)key;
    m->qual = (UBYTE)qual;
}
BLITZ_ALIAS(domacrokey, domacrokey_c);

/* ------------------------------------------------------- simple events */

/* the mouse event of AbsMouse (POINTERPOS) or RelMouse (RAWMOUSE) */
static void movemouse(ULONG x, ULONG y, UBYTE class, UWORD qual)
{
    clearie();
    dumevent.ie_Class = class;                  /* Fixed (#98): MOVE.b d2 */
    dumevent.ie_Qualifier = qual;
    dumevent.ie_X = (WORD)x;
    dumevent.ie_Y = (WORD)y;
    addtape();
}

/**
 * @brief AbsMouse X,Y: a POINTERPOS event to X,Y.
 *
 * Built in dumevent and executed, or appended to the TapeTrap tape.
 *
 * @param x  d0.w: x
 * @param y  d1.w: y
 */
static void BLITZ_ROUTINE doabsmouse_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1))
{
    movemouse(x, y, IECLASS_POINTERPOS, 0);
}
BLITZ_ALIAS(doabsmouse, doabsmouse_c);

/**
 * @brief RelMouse X Offset,Y Offset: a relative mouse move.
 *
 * A RAWMOUSE event with the qualifier RELATIVEMOUSE and the offsets,
 * executed or appended to the TapeTrap tape.
 * Fixed (#98): the original made a POINTERPOS event like AbsMouse (the
 * class 2 it passes in d2 was not used).
 *
 * @param x  d0.w: the x offset
 * @param y  d1.w: the y offset
 */
static void BLITZ_ROUTINE dorelmouse_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1))
{
    movemouse(x, y, IECLASS_RAWMOUSE, IEQUALIFIER_RELATIVEMOUSE);
}
BLITZ_ALIAS(dorelmouse, dorelmouse_c);

/* mouse button d0 (0 = left), d1 = down (<> 0) / up */
static void mousebutton(ULONG b, ULONG on)
{
    UWORD code;

    clearie();
    dumevent.ie_Class = IECLASS_RAWMOUSE;
    dumevent.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    code = (UWORD)b ? IECODE_RBUTTON : IECODE_LBUTTON;
    if (!(UWORD)on)
        code |= IECODE_UP_PREFIX;
    dumevent.ie_Code = code;
    addtape();
}

/**
 * @brief MouseButton Button,On|Off: a mouse button event.
 *
 * A RAWMOUSE event (LBUTTON, or RBUTTON for Button <> 0; UP_PREFIX for
 * Off), executed or appended to the TapeTrap tape.
 *
 * @param b  d0.w: the button, 0 = left, otherwise right
 * @param on  d1.w: 0 = released, otherwise pressed
 */
static void BLITZ_ROUTINE domousebutton_c(ULONG b BL_REG(d0), ULONG on BL_REG(d1))
{
    mousebutton(b, on);
}
BLITZ_ALIAS(domousebutton, domousebutton_c);

/**
 * @brief ClickButton Button: a mouse button press and release.
 *
 * Two MouseButton events of the button: down, then up.
 * Fixed (#97): in the original the up event got the d0 the first call
 * left: 0 (the left button) for an executed event, the QuietTrap value
 * when the event went to a tape - ClickButton 0 under TapeTrap with
 * QuietTrap On recorded "left down, RIGHT up".
 *
 * @param b  d0.w: the button, 0 = left, otherwise right
 */
static void BLITZ_ROUTINE doclickbutton_c(ULONG b BL_REG(d0))
{
    mousebutton(b, 1);
    mousebutton(b, 0);                          /* Fixed (#97) */
}
BLITZ_ALIAS(doclickbutton, doclickbutton_c);

/**
 * @brief Type String$: the characters of a string as key events.
 *
 * Every character as RAWKEY event (rawkeylib unconvert: d0 = raw key,
 * d1 = qualifier), executed or appended to the TapeTrap tape.
 * BUG(orig) (#102): a character rawkeylib cannot convert gives code
 * $FFFF.
 *
 * @param s  d0: the string
 * @param lens  a2: the argument stack, -4(a2) = the string's length
 */
static void BLITZ_ROUTINE dotype_c(const UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG n = BL_POPL(sp);

    while (n) {
        struct bl_regs r = { .d0 = *s++ };

        clearie();
        dumevent.ie_Class = IECLASS_RAWKEY;
        BLITZ_ALIBJSR(TOK_unconvert, r);                 /* unconvert! */
        dumevent.ie_Code = (UWORD)r.d0;
        dumevent.ie_Qualifier = (UWORD)r.d1;
        addtape();                                      /* doit! */
        n--;
    }
}
BLITZ_ALIAS(dotype, dotype_c);

/* ------------------------------------------------ record and playback */

/* Record: turn off recording; the start time goes into the tape */
static void recordoff(void)
{
    waitlast();
    Forbid();
    if (mode == 1) {
        firsta3->start.tv_secs = firstval.tv_secs;
        firsta3->start.tv_micro = firstval.tv_micro;
        mode = 0;
    }
    Permit();
}

/* the links of the tape's nodes the handler pointed at incoming events:
   the last one played (lastrec, its real link lastlink) and the ones
   before it (hjnode, hjlink) */
static void restorelinks(void)
{
    UWORD i;

    for (i = 0; i < nhijack; i++)
        hjnode[i]->ie_NextEvent = hjlink[i];
    nhijack = 0;
    lastrec->ie_NextEvent = lastlink;
}

/* PlayBack: turn off playback; restores the hijacked links */
static void playbackoff(void)
{
    waitlast();
    Forbid();
    if (mode == 2) {
        restorelinks();
        mode = 0;
    }
    Permit();
}

/* start playing tape t (also from the handler: macro keys) */
static void playback3(struct tape *tape)
{
    lastrec = (struct InputEvent *)tape;        /* the tape's link = node 0 */
    lastlink = tape->first;
    nhijack = 0;
    playval.tv_secs = tape->start.tv_secs;
    playval.tv_micro = tape->start.tv_micro;
    dumevent.ie_Class = IECLASS_NULL;
    newstart = 1;
    mode = 0x8002;
}

/* freetape: the events of tape a3 */
static void freetape_(struct tape *tape)
{
    struct InputEvent *ev, *gone;

    playbackoff();                              /* just in case..... */
    recordoff();
    ev = tape->first;
    tape->first = 0;
    while (ev) {
        gone = ev;
        ev = ev->ie_NextEvent;
        globfree(gone, IESIZE);
    }
}

/**
 * @brief The object's free routine: free the events of a tape.
 *
 * Stops a playback and a recording first (just in case), then frees the
 * tape's events.
 *
 * Reads a3: the tape object.
 */
static void BLITZ_ROUTINE freetape_c(void)
{
    freetape_((struct tape *)blitz_string);
}
BLITZ_THUNK(freetape, freetape_c, "d0-d1/a0-a1");

/**
 * @brief Record: stop recording.
 *
 * The start time goes into the tape being recorded (if mode is 1).
 */
static void BLITZ_ROUTINE dorecord_c(void)
{
    recordoff();
}
BLITZ_ALIAS(dorecord, dorecord_c);

/**
 * @brief Record Tape#: record the input events into a tape.
 *
 * Stops any other recording, frees the tape's events and lets the input
 * handler record RAWKEY, RAWMOUSE and POINTERPOS events (mode $8001: the
 * handler first takes the time of the first event, firstval).
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 */
static void BLITZ_ROUTINE dorecord2_c(void)
{
    struct tape *tape = (struct tape *)blitz_string;

    recordoff();                                /* any other recording */
    freetape_(tape);
    firsta3 = tape;
    lastrec = (struct InputEvent *)tape;
    dumevent.ie_Class = IECLASS_NULL;
    newstart = 1;
    mode = 0x8001;
}
BLITZ_ALIAS(dorecord2, dorecord2_c);

/**
 * @brief PlayBack: stop a playback.
 *
 * Restores the link the handler hijacked (if mode is 2).
 */
static void BLITZ_ROUTINE doplayback_c(void)
{
    playbackoff();
}
BLITZ_ALIAS(doplayback, doplayback_c);

/* dopb: play tape t if it has events; sig = signal at the end */
static void dopb(struct tape *tape, UWORD sig)
{
    if (!tape->first)
        return;
    playbackoff();
    sigit = sig;
    playback3(tape);
}

/**
 * @brief PlayBack Tape#: play a tape.
 *
 * Nothing for an empty tape; otherwise stops a playback and starts this
 * one (no signal at the end).  The handler links the tape's events in
 * front of the first incoming event later than them (Fixed (#100)).
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 */
static void BLITZ_ROUTINE doplayback2_c(void)
{
    dopb((struct tape *)blitz_string, 0);
}
BLITZ_ALIAS(doplayback2, doplayback2_c);

/**
 * @brief PlayWait: wait for the end of a playback.
 *
 * BUG(orig) (#99): only while playing (low byte of mode = 2), and then
 * it plays the given tape again (with the end signal) and waits for it
 * (Wait(mysig)); otherwise it returns at once.
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from the word
 * argument in d0).
 */
static void BLITZ_ROUTINE doplaywait_c(void)
{
    if ((UBYTE)mode != 2 || !mysig)             /* no signal: Fixed (#104) */
        return;
    dopb((struct tape *)blitz_string, 0xffff);
    Wait(mysig);
}
BLITZ_ALIAS(doplaywait, doplaywait_c);

/* ------------------------------------------------------- files */

static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

/* "Can't Open File" - in the normal code (DIFF, see the head comment) */
const char mess1[] = "Can't Open File";

/**
 * @brief SaveTape Tape#,Filename$: save a tape to a file.
 *
 * Nothing for an empty tape; otherwise the 8 byte start time, then 18
 * bytes per event (the event without its link).  Write errors are not
 * checked.  Errors: "Can't Open File" (in the normal code; DIFF: `BEQ
 * err1` into the error check area in the original).
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 * @param name  d1: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE dosavetape_c(const char *name BL_REG(d1), struct DosLibrary *dos BL_REG(a6))
{
    struct tape *tape = (struct tape *)blitz_string;
    struct InputEvent *ev;
    BPTR fh;

    if (!tape->first)
        return;
    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess1);
    Write(fh, &tape->start, 8);
    for (ev = tape->first; ev; ev = ev->ie_NextEvent)
        Write(fh, (UBYTE *)ev + 4, IESIZE - 4);
    Close(fh);
}
BLITZ_ALIAS(dosavetape, dosavetape_c);

/**
 * @brief LoadTape Tape#,filename$: load a tape from a file.
 *
 * Frees the tape's events, then reads the format of SaveTape.  Errors:
 * "Can't Open File" (in the normal code; DIFF as SaveTape).  A file whose
 * start time cannot be read whole (shorter than 8 bytes) leaves the tape
 * empty, with the start time 0 (Fixed (#101): the original took the
 * read unchecked); the load ends at the first short event record.
 *
 * Reads a3: the tape object (!libs{#brexxlib,$1380} from Tape# in d0).
 * @param name  d1: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadtape_c(const char *name BL_REG(d1), struct DosLibrary *dos BL_REG(a6))
{
    struct tape *tape = (struct tape *)blitz_string;
    struct InputEvent **link = &tape->first;
    BPTR fh;

    freetape_(tape);
    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!fh)
        BLITZ_RUNERR(mess1);
    if (Read(fh, dumtape, 8) != 8) {            /* Fixed (#101) */
        tape->start.tv_secs = tape->start.tv_micro = 0;
        Close(fh);
        return;
    }
    copyw((UWORD *)&tape->start, dumtape, 4);
    while (Read(fh, dumtape, 18) == 18) {
        struct InputEvent *n = globalloc(IESIZE);

        copyw((UWORD *)n + 2, dumtape, 9);
        n->ie_NextEvent = 0;
        *link = n;
        link = &n->ie_NextEvent;
    }
    Close(fh);
}
BLITZ_ALIAS(doloadtape, doloadtape_c);

/* ------------------------------------------------- input.device */

static LONG openinput(void)
{
    UWORD *word = (UWORD *)&io_request;
    struct MsgPort *port = &io_request.port;
    struct List *l = &port->mp_MsgList;
    UWORD n = sizeof(io_request) / 2;
    struct Task *task;

    do
        *word++ = 0;
    while (--n);
    io_request.io.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io_request.io.io_Message.mn_Length = sizeof(struct IOStdReq);
    l->lh_TailPred = (struct Node *)l;
    l->lh_Head = (struct Node *)&l->lh_Tail;
    task = FindTask(0);
    port->mp_Node.ln_Type = NT_MSGPORT;
    port->mp_SigTask = task;
    port->mp_SigBit = (UBYTE)AllocSignal(-1);
    io_request.io.io_Message.mn_ReplyPort = port;
    return OpenDevice((CONST_STRPTR)devname, 0, (struct IORequest *)&io_request.io, 0);
}

static void handlercmd(UWORD cmd)
{
    io_request.io.io_Data = &my_interrupt;
    io_request.io.io_Command = cmd;
    DoIO((struct IORequest *)&io_request.io);
}

/**
 * @brief The init routine: open input.device and add the input handler.
 *
 * Own IOStdReq + MsgPort (the result of OpenDevice is not checked), the
 * handler with priority 51 ("one more than intuition"); clears the macro
 * keys, QuietTrap On, QuickPlay Off; allocates the "playback finished"
 * signal and remembers the task.
 * Fixed (#104): without a free signal mysig stays 0 (PlayWait then returns
 * at once) and mybit -1 (not freed); the original took the -1 as the bit.
 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    BYTE sig;

    openinput();
    handlercmd(IND_ADDHANDLER);
    mkeys = 0;
    trap_mode = 1;
    qplay = 0;
    sig = AllocSignal(-1);
    mybit = (UWORD)(WORD)sig;                   /* MOVE d0,mybit */
    mysig = sig >= 0 ? 1UL << sig : 0;
    mytask = FindTask(0);
}
BLITZ_THUNK(brexxlib_codeinit, codeinit_c, "d0-d1/a0-a1");

/**
 * @brief The finit routine: stop a playback, remove the handler.
 *
 * Removes the input handler, closes input.device and frees both signals.
 * Fixed (#104): only the signals AllocSignal gave (the original freed 255
 * and 65535 after a failed one).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    playbackoff();                              /* turn off any tape... */
    handlercmd(IND_REMHANDLER);
    CloseDevice((struct IORequest *)&io_request.io);
    /* FreeSignal zero extended (MOVEQ #0,d0:MOVE.b / MOVE) */
    if (io_request.port.mp_SigBit != 0xff)
        BL_FREESIGNAL(io_request.port.mp_SigBit);
    if (mysig)
        BL_FREESIGNAL(mybit);
}
BLITZ_THUNK(brexxlib_codefinit, codefinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- the handler */

/* (secs:micros) - (s:m) as 64 bit value, compared unsigned */
static int timebefore(ULONG s1, ULONG m1, ULONG s0, ULONG m0)
{
    /* is (s1:m1) < (s0:m0) ? */
    return s1 < s0 || (s1 == s0 && m1 < m0);
}

/* is it time to play the tape event ev back before the incoming event in?
   (always with QuickPlay and for a tape event of time 0) */
static int due(const struct InputEvent *ev, const struct InputEvent *in)
{
    ULONG s = ev->ie_TimeStamp.tv_secs, m = ev->ie_TimeStamp.tv_micro;
    ULONG ps, pm, is, im, ds, dm;

    if (qplay || !(s | m))
        return 1;
    ps = s - playval.tv_secs - (m < playval.tv_micro);         /* SUB/SUBX */
    pm = m - playval.tv_micro;
    is = in->ie_TimeStamp.tv_secs;
    im = in->ie_TimeStamp.tv_micro;
    ds = is - firstval.tv_secs - (im < firstval.tv_micro);
    dm = im - firstval.tv_micro;
    return !timebefore(ds, dm, ps, pm);
}

static struct InputEvent *my_handler(struct InputEvent *events BL_REG(a0))
{
    UWORD curmode;

    if (!events)
        return events;
    curmode = mode;
    if (curmode == 0)
        goto isitmacro;
    if ((WORD)curmode < 0) {
        /* execute dumevent - get timeval (at a start only: Fixed (#96)) */
        mode = curmode & 0x7fff;
        if (newstart) {
            newstart = 0;
            firstval.tv_secs = events->ie_TimeStamp.tv_secs;
            firstval.tv_micro = events->ie_TimeStamp.tv_micro;
        }
        dumevent.ie_NextEvent = events;
        return &dumevent;
    }
    if (curmode == 1) {
        /* record */
        struct InputEvent *e = events;

        do {
            UBYTE c = e->ie_Class;

            if (c == IECLASS_RAWMOUSE || c == IECLASS_POINTERPOS || c == IECLASS_RAWKEY) {
                struct InputEvent *n = globalloc(IESIZE);

                copyw((UWORD *)n, (const UWORD *)e, IESIZE / 2);
                n->ie_NextEvent = 0;
                lastrec->ie_NextEvent = n;
                lastrec = n;
            }
            e = e->ie_NextEvent;
        } while (e);
        return events;
    }
    if (curmode == 2) {
        /* playback: the due tape events of each incoming event go in front
           of it (Fixed (#100): the original compared all with the first
           incoming event and put them in front of the list) */
        struct InputEvent *ev, *seg, *segend, *in, *prev, *first;
        UWORD played = 0;

        restorelinks();
        if (!lastlink) {
            mode = 0;
            if (sigit)
                Signal(mytask, mysig);
            return events;
        }
        ev = lastlink;                                          /* next to play */
        first = events;
        prev = 0;
        in = events;
        do {
            seg = segend = 0;
            while (ev && due(ev, in)) {                         /* yes now */
                if (!seg)
                    seg = ev;
                segend = ev;
                ev = ev->ie_NextEvent;
            }
            if (seg) {
                if (played) {                                   /* keep the last hijack */
                    hjnode[nhijack] = lastrec;
                    hjlink[nhijack++] = lastlink;
                }
                played = 1;
                if (prev)
                    prev->ie_NextEvent = seg;
                else
                    first = seg;
                lastrec = segend;
                lastlink = ev;
                segend->ie_NextEvent = in;
                if (nhijack == NHIJACK)                         /* the rest next time */
                    break;
            }
            prev = in;
            in = in->ie_NextEvent;
        } while (in && ev);
        return first;
    }
    return events;                                              /* notplay */

isitmacro:
    if (events->ie_Class == IECLASS_RAWKEY) {
        /* key hit, check if it's a macro key! */
        UBYTE key = (UBYTE)events->ie_Code, qual = (UBYTE)events->ie_Qualifier;
        struct mkey *ev = (struct mkey *)&mkeys;

        while ((ev = ev->next) != 0)
            if (ev->rawkey == key && ev->qual == qual) {
                playback3(ev->tape);                            /* was 2 */
                break;
            }
    }
    return events;
}
