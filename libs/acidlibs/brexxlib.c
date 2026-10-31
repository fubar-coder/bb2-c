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
 * clears bit 15, remembers the time stamp of the first real event
 * (firstval) and puts dumevent in front of the list.  The handler
 *   - records RAWKEY, RAWMOUSE and POINTERPOS events (copies, the list is
 *     passed on unchanged),
 *   - plays back: all tape events whose time (minus the tape's start
 *     time) is not later than the first incoming event's time (minus
 *     firstval) - or all, with QuickPlay - are linked IN FRONT of the
 *     incoming list (the tape's own nodes; the hijacked link is restored
 *     on the next call); at the end of the tape mode = 0 and, for
 *     PlayWait, Signal(mytask, mysig),
 *   - with mode 0 checks RAWKEY events for a MacroKey and starts its tape.
 * Simple events (Type, AbsMouse, RelMouse, MouseButton, ClickButton) are
 * built in dumevent and either executed (mode = $8000) or, after TapeTrap
 * Tape#, appended to that tape (with QuietTrap On only appended).
 *
 * File format (SaveTape/LoadTape): the 8 byte start time, then 18 bytes
 * per event (the event without its link).
 *
 * BUG(orig), kept:
 *  - TapeTrap without a tape number falls through into TapeTrap Tape#:
 *    tapeadd = the caller's a3 (Blitz string work pointer), so events are
 *    appended to whatever a3 points to (not tested). (#95)
 *  - Appending an event to a tape clears `mode` (a running Record or
 *    PlayBack stops working, without restoring the hijacked link). (#96)
 *  - ClickButton: the second (button up) call gets d0 as left by the
 *    first one's addtape: 0 normally, but the QuietTrap value when the
 *    event went to a tape - ClickButton 0 under TapeTrap with QuietTrap
 *    On records "left down, RIGHT up". (#97)
 *  - AbsMouse and RelMouse both make POINTERPOS events (RelMouse with
 *    qualifier RELATIVEMOUSE). (#98)
 *  - PlayWait restarts the given tape (with signal) when the low byte of
 *    mode is 2 and waits; otherwise it returns at once. (#99)
 *  - The playback time comparison takes the time of the FIRST event of
 *    the incoming list only. (#100)
 *  - LoadTape reads the start time unchecked and stops at the first
 *    short read (no error for a damaged file). (#101)
 *  - Type: a character rawkeylib cannot convert gives code $FFFF. (#102)
 *  - XStatus returns mode including bit 15 ($8001 ...). (#103)
 *  - A failed AllocSignal(-1) is not noticed: mybit = $FFFF, mysig = 0
 *    (LSL.l by 63), FreeSignal(255)/(65535). (#104)
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

/* entry points (thunks, defined below) */
void dorecord(void), dorecord2(void), doplayback(void), doplayback2(void);
void dosavetape(void), doloadtape(void), doabsmouse(void), dorelmouse(void);
void domousebutton(void), doclickbutton(void), domacrokey(void);
void dotapeadd(void), dotapeadd2(void), dotapemode(void), dotype(void);
void doqplay(void), doxstatus(void), doplaywait(void), dofreemacrokey(void);
void codeinit(void), codefinit(void), freetape(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#brexxlib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_brexxlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

#define TAPEOBJ LIB_brexxlib, 0x1380            /* a3 = tape object */

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
BLITZ_LIBS(TAPEOBJ, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dosavetape, 0, 0);
BLITZ_NAME("SaveTape", "Tape#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(TAPEOBJ, LIB_doslib, BL_LA(6));
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

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

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
static struct InputEvent *lastrec;      /* last node of the tape (record),
                                           node whose link is hijacked (play) */
static struct InputEvent *lastlink;     /* its real link */
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

/* addtape: append dumevent to tapeadd, or let the handler inject it.
   Returns the d0 the original leaves (see ClickButton). */
static ULONG addtape(void)
{
    struct InputEvent *n, **link;

    if (!tapeadd) {
        mode = 0x8000;
        return 0;
    }
    n = globalloc(IESIZE);
    copyw((UWORD *)n, (const UWORD *)&dumevent, IESIZE / 2);
    link = &tapeadd->first;             /* find where in the tape */
    while (*link)
        link = &(*link)->ie_NextEvent;
    *link = n;
    n->ie_NextEvent = 0;
    mode = 0;
    if (trap_mode)
        return trap_mode;
    mode = 0x8000;
    return 0;
}

/* ---------------------------------------------------- simple settings */

static ULONG BLITZ_ROUTINE doxstatus_c(void)
{
    return mode;                                /* MOVEQ #0:MOVE */
}
BLITZ_THUNK(doxstatus, doxstatus_c, "d1/a0-a1");

static void BLITZ_ROUTINE doqplay_c(ULONG v BL_REG(d0))
{
    qplay = (UWORD)v;
}
BLITZ_THUNK(doqplay, doqplay_c, "d0-d1/a0-a1");

/* TapeTrap (no tape): CLR.l tapeadd, then falls into TapeTrap Tape# with
   the caller's a3 (BUG(orig), #95) */
static void BLITZ_ROUTINE dotapeadd2_c(void)
{
    tapeadd = (struct tape *)blitz_string;
}
BLITZ_THUNK(dotapeadd, dotapeadd2_c, "d0-d1/a0-a1");
BLITZ_THUNK(dotapeadd2, dotapeadd2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dotapemode_c(ULONG v BL_REG(d0))
{
    trap_mode = (UWORD)v;
}
BLITZ_THUNK(dotapemode, dotapemode_c, "d0-d1/a0-a1");

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

static void BLITZ_ROUTINE dofreemacrokey_c(ULONG key BL_REG(d0), ULONG qual BL_REG(d1))
{
    freemacrokey((UBYTE)key, (UBYTE)qual);
}
BLITZ_THUNK(dofreemacrokey, dofreemacrokey_c, "d0-d1/a0-a1");

/* MacroKey: a3 = tape, d1 = rawkey, d2 = qualifier */
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
BLITZ_THUNK(domacrokey, domacrokey_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- simple events */

static ULONG movemouse(ULONG x, ULONG y, UWORD qual)
{
    clearie();
    dumevent.ie_Class = IECLASS_POINTERPOS;
    dumevent.ie_Qualifier = qual;
    dumevent.ie_X = (WORD)x;
    dumevent.ie_Y = (WORD)y;
    return addtape();
}

static void BLITZ_ROUTINE doabsmouse_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1))
{
    movemouse(x, y, 0);
}
BLITZ_THUNK(doabsmouse, doabsmouse_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dorelmouse_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1))
{
    movemouse(x, y, IEQUALIFIER_RELATIVEMOUSE);
}
BLITZ_THUNK(dorelmouse, dorelmouse_c, "d0-d1/a0-a1");

/* mouse button d0 (0 = left), d1 = down (<> 0) / up */
static ULONG mousebutton(ULONG b, ULONG on)
{
    UWORD code;

    clearie();
    dumevent.ie_Class = IECLASS_RAWMOUSE;
    dumevent.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    code = (UWORD)b ? IECODE_RBUTTON : IECODE_LBUTTON;
    if (!(UWORD)on)
        code |= IECODE_UP_PREFIX;
    dumevent.ie_Code = code;
    return addtape();
}

static void BLITZ_ROUTINE domousebutton_c(ULONG b BL_REG(d0), ULONG on BL_REG(d1))
{
    mousebutton(b, on);
}
BLITZ_THUNK(domousebutton, domousebutton_c, "d0-d1/a0-a1");

/* click: down, then up - with the d0 the first call left (BUG(orig), #97) */
static void BLITZ_ROUTINE doclickbutton_c(ULONG b BL_REG(d0))
{
    mousebutton(mousebutton(b, 1), 0);
}
BLITZ_THUNK(doclickbutton, doclickbutton_c, "d0-d1/a0-a1");

/* Type: d0 = string, -4(a2) = length; every character as RAWKEY event
   (rawkeylib unconvert: d0 = raw key, d1 = qualifier) */
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
BLITZ_THUNK(dotype, dotype_c, "d0-d1/a0-a1");

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

/* PlayBack: turn off playback; restores the hijacked link */
static void playbackoff(void)
{
    waitlast();
    Forbid();
    if (mode == 2) {
        lastrec->ie_NextEvent = lastlink;
        mode = 0;
    }
    Permit();
}

/* start playing tape t (also from the handler: macro keys) */
static void playback3(struct tape *tape)
{
    lastrec = (struct InputEvent *)tape;        /* the tape's link = node 0 */
    lastlink = tape->first;
    playval.tv_secs = tape->start.tv_secs;
    playval.tv_micro = tape->start.tv_micro;
    dumevent.ie_Class = IECLASS_NULL;
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

static void BLITZ_ROUTINE freetape_c(void)
{
    freetape_((struct tape *)blitz_string);
}
BLITZ_THUNK(freetape, freetape_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dorecord_c(void)
{
    recordoff();
}
BLITZ_THUNK(dorecord, dorecord_c, "d0-d1/a0-a1");

/* Record Tape#: a3 = tape */
static void BLITZ_ROUTINE dorecord2_c(void)
{
    struct tape *tape = (struct tape *)blitz_string;

    recordoff();                                /* any other recording */
    freetape_(tape);
    firsta3 = tape;
    lastrec = (struct InputEvent *)tape;
    dumevent.ie_Class = IECLASS_NULL;
    mode = 0x8001;
}
BLITZ_THUNK(dorecord2, dorecord2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE doplayback_c(void)
{
    playbackoff();
}
BLITZ_THUNK(doplayback, doplayback_c, "d0-d1/a0-a1");

/* dopb: play tape t if it has events; sig = signal at the end */
static void dopb(struct tape *tape, UWORD sig)
{
    if (!tape->first)
        return;
    playbackoff();
    sigit = sig;
    playback3(tape);
}

static void BLITZ_ROUTINE doplayback2_c(void)
{
    dopb((struct tape *)blitz_string, 0);
}
BLITZ_THUNK(doplayback2, doplayback2_c, "d0-d1/a0-a1");

/* PlayWait: only while playing (low byte of mode = 2): play tape a3 again
   with signal and wait for it */
static void BLITZ_ROUTINE doplaywait_c(void)
{
    if ((UBYTE)mode != 2)
        return;
    dopb((struct tape *)blitz_string, 0xffff);
    Wait(mysig);
}
BLITZ_THUNK(doplaywait, doplaywait_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- files */

static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

/* "Can't Open File" - in the normal code (DIFF, see the head comment) */
const char mess1[] = "Can't Open File";

/* SaveTape: a3 = tape, d1 = file name, a6 = dos */
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
BLITZ_THUNK(dosavetape, dosavetape_c, "d0-d1/a0-a1");

/* LoadTape: a3 = tape, d1 = file name, a6 = dos */
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
    Read(fh, &tape->start, 8);
    while (Read(fh, dumtape, 18) == 18) {
        struct InputEvent *n = globalloc(IESIZE);

        copyw((UWORD *)n + 2, dumtape, 9);
        n->ie_NextEvent = 0;
        *link = n;
        link = &n->ie_NextEvent;
    }
    Close(fh);
}
BLITZ_THUNK(doloadtape, doloadtape_c, "d0-d1/a0-a1");

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
    mysig = ((LONG)sig & 63) < 32 ? 1UL << ((LONG)sig & 63) : 0;  /* LSL.l d0,d1 */
    mytask = FindTask(0);
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE codefinit_c(void)
{
    playbackoff();                              /* turn off any tape... */
    handlercmd(IND_REMHANDLER);
    CloseDevice((struct IORequest *)&io_request.io);
    /* FreeSignal zero extended (MOVEQ #0,d0:MOVE.b / MOVE) */
    BL_FREESIGNAL(io_request.port.mp_SigBit);
    BL_FREESIGNAL(mybit);
}
BLITZ_THUNK(codefinit, codefinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- the handler */

/* (secs:micros) - (s:m) as 64 bit value, compared unsigned */
static int timebefore(ULONG s1, ULONG m1, ULONG s0, ULONG m0)
{
    /* is (s1:m1) < (s0:m0) ? */
    return s1 < s0 || (s1 == s0 && m1 < m0);
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
        /* execute dumevent - get timeval */
        mode = curmode & 0x7fff;
        firstval.tv_secs = events->ie_TimeStamp.tv_secs;
        firstval.tv_micro = events->ie_TimeStamp.tv_micro;
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
        /* playback */
        struct InputEvent *ev, *lastplayed, *first;

        lastrec->ie_NextEvent = lastlink;
        if (!lastlink) {
            mode = 0;
            if (sigit)
                Signal(mytask, mysig);
            return events;
        }
        ev = lastlink;
        lastplayed = events;
        do {
            /* is it time to play this one back yet? */
            if (!qplay) {
                ULONG s = ev->ie_TimeStamp.tv_secs,
                      m = ev->ie_TimeStamp.tv_micro;

                if (s | m) {
                    ULONG ps = s - playval.tv_secs - (m < playval.tv_micro);   /* SUB/SUBX */
                    ULONG pm = m - playval.tv_micro;
                    ULONG is = events->ie_TimeStamp.tv_secs,
                          im = events->ie_TimeStamp.tv_micro;
                    ULONG ds = is - firstval.tv_secs - (im < firstval.tv_micro);
                    ULONG dm = im - firstval.tv_micro;

                    if (timebefore(ds, dm, ps, pm))
                        break;                                  /* not yet */
                }
            }
            lastplayed = ev;                                    /* yes now */
            ev = ev->ie_NextEvent;
        } while (ev);
        if (lastplayed == events)
            return events;
        first = lastrec->ie_NextEvent;                          /* where we start from */
        lastrec = lastplayed;
        lastlink = lastplayed->ie_NextEvent;
        lastplayed->ie_NextEvent = events;
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
