/*
 * speaklib - command library 88: speech with narrator.device and
 * translator.library.  Speak SetVoice Translate$ PhoneticSpeak VoiceLoc.
 *
 * C translation of bblibsrc/speaklib; binary of the CD's acidlibs
 * checked: same as the source.
 *
 * init opens translator.library (V33) and narrator.device unit 0 with
 * the library's own narrator_rb (IOReq, mn_Length 88) and reply port,
 * allocates the port's signal and sets the audio channel masks
 * {3,5,10,12}.  Anything missing: runtime error "Failed To Open Speech
 * Library" (TRAP #0 in the normal code, also without the debugger).
 * Speak translates the text into a 1024 byte part of a 4K buffer and
 * sends it (CMD_WRITE, SendIO, length incl. the 0); every Speak /
 * PhoneticSpeak first waits for the previous one (WaitIO), finit waits
 * for the last one.  SetVoice clips its values and stores them in the
 * request: rate 40-399/400, pitch 65-319/320, mode and sex & 1, volume
 * at most 64, sample frequency 5000-27999/28000 (word compares, BPL/BMI:
 * values beyond +-32767 wrap).  VoiceLoc = address of the V37 part of
 * the request (19 bytes after numchan, at an odd address).
 *
 * BUG(orig), kept:
 *   - Translate$ writes the phonemes over its argument (a3 = the
 *     argument's address), returns the length INCLUDING the terminating 0
 *     and leaves a3 behind the 0. (#277)
 *   - Not a bug (a statement, a3 has no meaning after it): PhoneticSpeak
 *     leaves a3 = its argument.
 *   - finit closes the device and frees the signal even when init failed. (#278)
 *   - SetVoice clips with word CMP and BPL/BMI (N flag only, no V): large
 *     negative values whose difference overflows end on the maximum. (#279)
 * DIFF: a2 stays unchanged (the original pops the string length).
 */
#include "blitzlib.h"
#include "blitzexec.h"

#define LVO_Translate (-30)     /* translator: a0 = in, d0 = len, a1 = out, d1 = size */

/* entry points (thunks / C routines, defined below) */
void _setvoice(void), _voiceloc(void), speaklib_libinit(void);
void speaklib_libfinit(void);
ULONG BLITZ_ROUTINE _translate(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2));
void BLITZ_ROUTINE _speak(const UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2));
void BLITZ_ROUTINE _phospeak(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#speaklib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_speaklib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_speak, 0, 0);
BLITZ_NAME("Speak", "string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_setvoice, 0, 0);
BLITZ_NAME("SetVoice", "rate(150),pitch(110),expression(1),sex,volume(64),freq(22200)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_translate, 0, 0);
BLITZ_NAME("Translate$", "returns phoneme translation of a string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_phospeak, 0, 0);
BLITZ_NAME("PhoneticSpeak", "phonetic string");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_voiceloc, 0, 0);
BLITZ_NAME("VoiceLoc", "returns location of newvoice struct");

BLITZ_NULLSUB_AT(init, speaklib_libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, speaklib_libfinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

static struct Library *TransBase;

/* the narrator_rb of the original (IOEnd - IOReq = 88 bytes), byte for
   byte: IOStdReq, then rate ... numchan, then the 19 V37 bytes */
static struct BL_PACKED {
    struct IOStdReq io;
    UWORD rate, pitch, mode, sex;
    UBYTE *ch_masks;
    UWORD nm_masks, vol, sampfreq;
    UBYTE mouths, chanmask, numchan;
    UBYTE v37[19];
} ioreq;

static UBYTE audchan[4] = { 3, 5, 10, 12 };
static struct MsgPort port;
static WORD initial;
static UBYTE spkbuff[4096];

static const char report[] = "Failed To Open Speech Library";

/* ------------------------------------------------------------ init */

static void BLITZ_ROUTINE libinit_c(void)
{
    BYTE sig;

    /* static parts of the request and the port (Dc in the original) */
    ioreq.io.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq.io.io_Message.mn_ReplyPort = &port;
    ioreq.io.io_Message.mn_Length = sizeof(ioreq);
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_MsgList.lh_Head = (struct Node *)&port.mp_MsgList.lh_Tail;
    port.mp_MsgList.lh_TailPred = (struct Node *)&port.mp_MsgList.lh_Head;

    TransBase = OpenLibrary("translator.library", 33);
    if (!TransBase)
        goto failed;
    if (OpenDevice("narrator.device", 0, (struct IORequest *)&ioreq, 0))
        goto failed;
    sig = AllocSignal(-1);
    port.mp_SigBit = sig;
    if (sig < 0)
        goto failed;
    port.mp_SigTask = FindTask(NULL);
    ioreq.ch_masks = audchan;
    ioreq.nm_masks = 4;
    ioreq.mouths = 0;
    return;
failed:
    BLITZ_RUNERR(report);
}
BLITZ_THUNK(speaklib_libinit, libinit_c, "d1/a0-a1");

static void BLITZ_ROUTINE libfinit_c(void)
{
    if (initial)
        WaitIO((struct IORequest *)&ioreq);
    CloseDevice((struct IORequest *)&ioreq);
    FreeSignal((UBYTE)port.mp_SigBit);
    CloseLibrary(TransBase);
}
BLITZ_THUNK(speaklib_libfinit, libfinit_c, "d1/a0-a1");

/* ---------------------------------------------------------- SetVoice */

/* CMP #c,d / BPL / BMI: sign of the 16 bit difference */
#define GE(v, c) ((WORD)(UWORD)((v) - (c)) >= 0)

/**
 * @brief SetVoice rate,pitch,expression,sex,volume,freq - set the
 * narrator's voice for the following Speak/PhoneticSpeak.
 *
 * Clips the values and stores them in the request (MOVEM d0-d3,rate /
 * MOVEM d4-d5,vol): rate 40-400, pitch 65-320, mode (expression) and sex
 * & 1, volume at most 64, sample frequency 5000-28000.
 *
 * BUG(orig) (#279): the clipping uses word CMP and BPL/BMI (N flag only,
 * no V): values beyond +-32767 wrap, large negative values whose
 * difference overflows end on the maximum.
 *
 * @param rate  d0: speaking rate (words per minute, default 150)
 * @param pitch d1: pitch in Hz (default 110)
 * @param mode  d2: expression, the narrator's mode (& 1, default 1)
 * @param sex   d3: the narrator's sex (& 1: 0 male, 1 female)
 * @param vol   d4: volume (0-64, default 64)
 * @param freq  d5: sample frequency (default 22200)
 */
static void BLITZ_ROUTINE _setvoice_c(UWORD rate BL_REG(d0), UWORD pitch BL_REG(d1),
                                      UWORD mode BL_REG(d2), UWORD sex BL_REG(d3),
                                      UWORD vol BL_REG(d4), UWORD freq BL_REG(d5))
{
    if (!GE(rate, 40))
        rate = 40;
    if (GE(rate, 400))
        rate = 400;
    if (!GE(pitch, 65))
        pitch = 65;
    if (GE(pitch, 320))
        pitch = 320;
    mode &= 1;
    sex &= 1;
    if (GE(vol, 64))
        vol = 64;
    if (!GE(freq, 5000))
        freq = 5000;
    if (GE(freq, 28000))
        freq = 28000;
    ioreq.rate = rate;                          /* MOVEM d0-d3,rate */
    ioreq.pitch = pitch;
    ioreq.mode = mode;
    ioreq.sex = sex;
    ioreq.vol = vol;                            /* MOVEM d4-d5,vol */
    ioreq.sampfreq = freq;
}
BLITZ_THUNK(_setvoice, _setvoice_c, "d0-d1/a0-a1");

/**
 * @brief VoiceLoc - the address of the request's V37 part (the newvoice
 * fields).
 *
 * The 19 bytes after numchan in the narrator request, at an odd address.
 *
 * @return d0: the address of the V37 bytes of the request
 */
static ULONG BLITZ_ROUTINE _voiceloc_c(void)
{
    return (ULONG)ioreq.v37;
}
BLITZ_THUNK(_voiceloc, _voiceloc_c, "d1/a0-a1");

/* ------------------------------------------------- Translate$, Speak */

static void translate(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = len, .d1 = 1024, .a0 = (ULONG)s, .a1 = (ULONG)spkbuff };

    BLITZ_BASEJSR(TransBase, LVO_Translate, r);
}

/* send a buffer to the narrator (CMD_WRITE), after the previous one */
static void say(const UBYTE *data, ULONG len)
{
    if (initial)
        WaitIO((struct IORequest *)&ioreq);     /* don't hassle me */
    ioreq.io.io_Command = CMD_WRITE;
    ioreq.io.io_Offset = 0;
    ioreq.io.io_Data = (APTR)data;
    ioreq.io.io_Length = len;
    SendIO((struct IORequest *)&ioreq);
    initial = -1;
}

/**
 * @brief Speak string - translate a text into phonemes and speak it.
 *
 * Translates the text (translator.library Translate, into a 1024 byte
 * part of the 4K buffer) and sends the phonemes to narrator.device
 * (CMD_WRITE, SendIO, length incl. the 0) without waiting; first waits
 * for the previous Speak/PhoneticSpeak (WaitIO).
 * DIFF: a2 stays unchanged (the original pops the string length).
 *
 * @param s    d0: the text
 * @param lens a2: the string length on the stack (long)
 */
void BLITZ_ROUTINE _speak(const UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    const UBYTE *p = spkbuff;
    UWORD n = 0;

    translate(s, BL_POPL(sp));
    do                                          /* length incl. the 0 */
        n++;
    while (*p++);
    say(spkbuff, n);
}

/**
 * @brief Translate$(string) - the phoneme translation of a text.
 *
 * Translates into the library's buffer and copies the phonemes over the
 * argument (a3 = the argument's address).
 * DIFF: a2 stays unchanged (the original pops the string length).
 *
 * BUG(orig) (#277): writes the phonemes over its argument, returns the
 * length INCLUDING the terminating 0 and leaves a3 behind the 0.
 *
 * @param s    d0: the text (also the result buffer)
 * @param lens a2: the string length on the stack (long)
 * @return d0: the length of the phonemes including the 0; a3 behind it
 */
ULONG BLITZ_ROUTINE _translate(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    const UBYTE *p = spkbuff;
    UWORD n = 0;

    blitz_string = s;                           /* MOVE.l d0,a3 */
    translate(s, BL_POPL(sp));
    do
        n++;
    while ((*blitz_string++ = *p++));
    return n;                                   /* MOVEQ #0,d0:ADDQ #1,d0 */
}

/**
 * @brief PhoneticSpeak phonetic string - speak a string of phonemes.
 *
 * Sends the string as it is to narrator.device (CMD_WRITE, SendIO, the
 * popped length), after waiting for the previous one.  Leaves a3 = its
 * argument (not a bug: a statement, a3 has no meaning after it).
 * DIFF: a2 stays unchanged (the original pops the string length).
 *
 * @param s    d0: the phonemes
 * @param lens a2: the string length on the stack (long)
 */
void BLITZ_ROUTINE _phospeak(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;

    blitz_string = s;                           /* MOVE.l d0,a3 */
    say(s, BL_POPL(sp));
}
