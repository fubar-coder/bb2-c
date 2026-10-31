/*
 * audiolib - command library 116: Sound objects and the audio hardware.
 * DiskPlay DiskBuffer LoadSound Sound Volume Filter LoopSound InitSound
 * SoundData PeekSound DecodeSound SetPeriod.
 *
 * C translation of bblibsrc/audiolib ("bum7 added cludgesound"); binary
 * of the CD's acidlibs checked: same as the source (the vertical blank
 * code between the two CELSE lines is not in the binary).
 *
 * Sound object (16 bytes, maximum 10):
 *   0.l data (chip memory)   4.w period   6.w length in words
 *   8.l repeat position      12.w repeat length in words
 *   14.w "cL" when the data belongs to DecodeSound (not freed)
 *
 * Everything goes directly to Paula ($DFF0A0 + 16 * channel: LC, LEN,
 * PER, VOL; DMACON $DFF096, INTENA $DFF09A, INTREQ(R) $DFF09C/$DFF01E):
 * Sound/LoopSound switch the channels' DMA off, wait for 12 changes of
 * the VHPOSR line byte, start the sound and after another 12 lines set
 * the repeat part (LoopSound) or a zero word in chip memory (Sound).
 * IFF 8SVX files are read with iffiolib (162): +1 open (a0 = name),
 * +6 find chunk (d0 = id), +5 read long, +4 read word, +7 skip d0 bytes,
 * +8 read d0 bytes to a0, +2 close; errors come back in the CCR (NE),
 * which C cannot read: iffcall() below reads it with SNE.
 * period = 3579440 / samplesPerSec (DIVU: overflow below 55 Hz leaves
 * 3579440).  DiskPlay streams a file through two halves of a chip buffer
 * (DiskBuffer, default 1024 bytes each) and waits for the audio
 * interrupt bits in INTREQR (busy wait, as in the original).
 *
 * BUG(orig), kept:
 *   - the error check of SetPeriod (sndchk) goes on into dpchk and tests
 *     the PERIOD (d1) as channel mask: SetPeriod with a period > 15 is a
 *     runtime error "Illegal Chanel Mask" with the debugger. (#66)
 *   - LoadSound: Sound too long (>= 128K) after the file was opened: the
 *     file stays open (#67); DecodeSound of chip data marks the sound "cL",
 *     a later DecodeSound into fast memory does not clear that mark (the
 *     copy is then never freed) (#68).
 *   - DiskPlay: a file whose length is a multiple of the buffer length
 *     never ends (loadbuff returns "more" at length 0). (#69)
 *   - SetPeriod without a sound: `BEQ err5` from the normal code into
 *     the error area (dangling without the debugger) - the C library
 *     calls err5 there in the same way; err1-err4 are in the normal code
 *     with their texts in the error area, as in the original. (#70)
 *   - PeekSound (like every $1380 object function): the object pointer
 *     comes in a3, which is also the string buffer pointer; inside a
 *     string expression the following string parts are written into the
 *     Sound object. (#46)
 * DIFF: a2 stays unchanged (the original pops volumes / lengths).
 */
#include "blitzlib.h"

#define TOK_iffchunk  TOK_ifffind      /* "iffchunk" in audiolib */

#define AUD(ch, off)  (0xdff0a0 + 16 * (ch) + (off))
#define CIAAPRA       (*(volatile UBYTE *)0xbfe001)
#define VHPOSR_B      (*(volatile UBYTE *)0xdff006)

/* entry points (thunks / C routines / inline code, defined below) */
void dodplay2(void), dodplay(void), dodiskbuff(void), dofilter(void);
void dovolume(void), dosdata(void), dopsound(void), codeinit(void), codefinit(void);
void dofreesound(void);
void sndchk(void), dpchk(void), vchk(void), ischk1(void), ischk2(void), sdatachk(void);
BLITZ_ENTRY_C(doloadsound); BLITZ_ENTRY_C(dosound2); BLITZ_ENTRY_C(dosound); BLITZ_ENTRY_C(dosoundl2); BLITZ_ENTRY_C(dosoundl);
BLITZ_ENTRY_C(doisound2); BLITZ_ENTRY_C(doisound3); BLITZ_ENTRY_C(doisound); BLITZ_ENTRY_C(_cludgesound); BLITZ_ENTRY_C(_setperiod);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#audiolib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_audiolib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dodplay2, dpchk, 0);
BLITZ_REPARGS(2, 1, BL_STRING, BL_WORD, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(dodplay, dpchk, 0);
BLITZ_NAME("DiskPlay", "Filename$,Channelmask[,Vol1[,Vol2...]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dodiskbuff, 0, 0);
BLITZ_NAME("DiskBuffer", "Bufferlen");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_doloadsound, 0, 0);
BLITZ_NAME("LoadSound", "Sound#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_dosound2, sndchk, 0);
BLITZ_REPARGS(2, 1, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_dosound, sndchk, 0);
BLITZ_NAME_AT(_toke, "Sound", "Sound#,Channelmask[,Vol1[,Vol2...]]");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(1, 1, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dovolume, vchk, 0);
BLITZ_NAME("Volume", "Channelmask,Vol1[,Vol2...]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dofilter, 0, 0);
BLITZ_NAME("Filter", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_dosoundl2, sndchk, 0);
BLITZ_REPARGS(2, 1, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_dosoundl, sndchk, 0);
BLITZ_NAME("LoopSound", "Sound#,Channelmask[,Vol1[,Vol2...]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_doisound2, ischk1, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_doisound3, ischk1, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e_doisound, ischk2, 0);
BLITZ_NAME("InitSound", "Sound#,Length[,Period[,Repeat]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_BYTE);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(dosdata, sdatachk, 0);
BLITZ_NAME("SoundData", "Sound#,Offset,Data");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(dopsound, sdatachk, 0);
BLITZ_NAME("PeekSound", "(Sound#,Offset)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e__cludgesound, 0, 0);
BLITZ_NAME("DecodeSound", "Sound#,MemoryLocation");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_audiolib, 0x1380);
BLITZ_SUBS(e__setperiod, sndchk, 0);
BLITZ_NAME("SetPeriod", "Sound#,Period");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, dofreesound, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 4);

/* ------------------------------------------------------------ variables */

static ULONG dbufflen = 1024;           /* length of disk buffer */
static LONG dlen;                       /* length of sample! */
static UBYTE *dbuffat;                  /* buffer */
static UBYTE *dbuffend;                 /* end of buffer */
static UWORD *chzero;                   /* a zero word in chipmem */

struct sound {
    UBYTE *data;                        /* +0 */
    UWORD period, len;                  /* +4, +6 (words) */
    UBYTE *rep;                         /* +8 */
    UWORD replen, mark;                 /* +12 (words), +14 */
};
static_assert(sizeof(struct sound) == 16);
#define MARK_CL 0x634c                  /* "cL" */

/* the start of an 8SVX VHDR chunk (IFF Voice8Header) */
struct vhdr {
    ULONG oneshot;                      /* +0 oneShotHiSamples */
    ULONG repeat;                       /* +4 repeatHiSamples */
    ULONG cycle;                        /* +8 samplesPerHiCycle */
    UWORD rate;                         /* +12 samplesPerSec */
};

/* messages / error routines: err1-err4 in the normal code (texts in the
   error area), err5 in the error area */
extern const char mess1[], mess2[], mess3[], mess4[], mess5[], mess6[];
extern const char mess7[], mess8[], mess9[], messa[];
static void err5(void);

static UBYTE *alloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void dealloc(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* iffiolib call: in/out d0/d1/a0/a1, result nonzero if it returned NE (error) */
#define IFFCALL(tok, r) BLITZ_ALIBJSR_CCR(tok, r, "ne")

/* ---------------------------------------------------- Filter, Volume */

/**
 * @brief Filter On|Off: the audio low pass filter (the power LED bit,
 *        BSET/BCLR #1,$BFE001).
 *
 * @param on  d0.w: not 0 = on
 */
static void BLITZ_ROUTINE dofilter_c(UWORD on BL_REG(d0))
{
    if (on)
        CIAAPRA |= 2;                           /* BSET #1,$bfe001 */
    else
        CIAAPRA &= ~2;
}
BLITZ_THUNK(dofilter, dofilter_c, "d0-d1/a0-a1");

/**
 * @brief DiskBuffer Bufferlen: the length of each half of DiskPlay's chip
 *        buffer (default 1024).
 *
 * @param len  d0: the length in bytes
 */
static void BLITZ_ROUTINE dodiskbuff_c(ULONG len BL_REG(d0))
{
    dbufflen = len;
}
BLITZ_THUNK(dodiskbuff, dodiskbuff_c, "d0-d1/a0-a1");

/**
 * @brief Volume Channelmask,Vol1[,Vol2...]: set the volume of the
 *        channels of the mask.
 *
 * The volumes from the a2 stack, one per set bit (channel 0 first).
 * Errors (debugger, vchk): "Illegal Chanel Mask" (mask > 15).
 *
 * @param mask  d0.w: the channel mask
 * @param args  a2: the compiler's stack with the volumes (words)
 */
static void BLITZ_ROUTINE dovolume_c(UWORD mask BL_REG(d0), const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    int ch;

    for (ch = 0; ch < 4; ch++, mask >>= 1)
        if (mask & 1)
            *(volatile UWORD *)AUD(ch, 8) = BL_POPW(sp);
}
BLITZ_THUNK(dovolume, dovolume_c, "d0-d1/a1");

/* ------------------------------------------------------------- sounds */

/* dofreesound - a3 = sound */
static void freesound(struct sound *s)
{
    UBYTE *d = s->data;

    if (!d)
        return;
    s->data = 0;
    if (s->mark == MARK_CL)
        return;
    dealloc(d, (ULONG)s->len << 1);
}

/**
 * @brief Free Sound (the object's _free nullsub): free the sample of the
 *        sound in a3 (not for "cL" data of DecodeSound).
 */
static void BLITZ_ROUTINE dofreesound_c(void)
{
    freesound((struct sound *)blitz_string);
}
BLITZ_THUNK(dofreesound, dofreesound_c, "d1/a0-a1");

struct opened {
    ULONG len, period, rep, replen;             /* d0, d1, d6, d5 */
};

/* doopensound - a0 = name: VHDR, then positioned at BODY.  0 on error
   (after the runtime error trap) */
static int opensound(const UBYTE *name, struct opened *o)
{
    struct bl_regs r = { .a0 = (ULONG)name };

    if (IFFCALL(TOK_iffopen, r)) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return 0;
    }
    r.d0 = 0x56484452;                          /* VHDR */
    if (IFFCALL(TOK_iffchunk, r)) {
        BLITZ_RUNERR(mess2);
        return 0;
    }
    if (IFFCALL(TOK_ifflong, r))
        goto err3;
    o->rep = r.d0;                              /* repeat start */
    if (IFFCALL(TOK_ifflong, r))
        goto err3;
    o->replen = r.d0;                           /* repeat length */
    r.d0 = 4;
    if (IFFCALL(TOK_iffskip, r))
        goto err3;
    if (IFFCALL(TOK_iffword, r))
        goto err3;
    o->period = bl_divu_w(3579440, r.d0);       /* DIVU d0,d1 */
    r.d0 = 0x424f4459;                          /* BODY */
    if (IFFCALL(TOK_iffchunk, r))
        goto err3;
    o->len = r.d0;
    return 1;
err3:
    BLITZ_RUNERR(mess3);
    return 0;
}

/**
 * @brief LoadSound Sound#,Filename$: load an IFF 8SVX file into the sound.
 *
 * Through iffiolib: VHDR (repeat start and length, period = 3579440 /
 * samplesPerSec, DIVU: overflow below 55 Hz leaves 3579440), then BODY
 * into chip memory.  Errors: "Illegal Sound file" (no file), "Can't Find
 * VHDR", "Error Reading File", "Sound too long" (>= 128K) - BUG(orig)
 * (#67): then the file stays open.
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param name  d1: the file name
 */
void BLITZ_ROUTINE doloadsound(const UBYTE *name BL_REG(d1))
{
    struct sound *s = (struct sound *)blitz_string;
    struct opened o;
    struct bl_regs r = { 0 };

    freesound(s);
    if (!opensound(name, &o))
        return;
    s->period = o.period;
    if (o.len >= 65536 * 2) {
        BLITZ_RUNERR(mess4);                    /* err4 */
        return;
    }
    s->data = alloc(o.len, MEMF_CHIP);
    r.a0 = (ULONG)s->data;
    r.d0 = o.len;
    IFFCALL(TOK_iffread, r);
    s->len = o.len >> 1;
    s->rep = s->data + o.rep;
    s->replen = o.replen >> 1;
    IFFCALL(TOK_iffclose, r);
}

/**
 * @brief DecodeSound Sound#,MemoryLocation: a sound from an 8SVX FORM in
 *        memory.
 *
 * Looks for VHDR and BODY; chip data (below 2 MB) is used in place
 * ("cL", never freed), other data copied to chip memory.  BUG(orig)
 * (#68): a later DecodeSound into fast memory does not clear the "cL"
 * mark (the copy is never freed).  Errors: "Illegal Sound file" (no
 * FORM/8SVX, no VHDR/BODY before the end of the FORM), "Sound too long"
 * (>= 128K).
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param p  d1: the address of the FORM
 */
void BLITZ_ROUTINE _cludgesound(ULONG *p BL_REG(d1))
{
    struct sound *s = (struct sound *)blitz_string;
    ULONG *end, id, size, oneshot, rep, period, len;
    UBYTE *mem;

    freesound(s);
    if (*p++ != 0x464f524d)                     /* FORM */
        goto err1;
    size = *p++;
    end = (ULONG *)((UBYTE *)p + size);
    if (*p++ != 0x38535658)                     /* 8SVX */
        goto err1;
    for (;;) {                                  /* wv */
        id = *p++;
        size = *p++;
        if (id == 0x56484452)                   /* VHDR */
            break;
        p = (ULONG *)((UBYTE *)p + size);
        if ((LONG)end <= (LONG)p)               /* CMP.l a0,a1:BLE */
            goto err1;
    }
    oneshot = ((const struct vhdr *)p)->oneshot; /* MOVEM.l (a0),d5-d6 */
    rep = ((const struct vhdr *)p)->repeat;
    period = bl_divu_w(3579440, ((const struct vhdr *)p)->rate);
    p = (ULONG *)((UBYTE *)p + size);
    for (;;) {                                  /* wb */
        id = *p++;
        size = *p++;
        if (id == 0x424f4459)                   /* BODY */
            break;
        p = (ULONG *)((UBYTE *)p + size);
        if ((LONG)end <= (LONG)p)
            goto err1;
    }
    if (size >= 65536 * 2) {
        BLITZ_RUNERR(mess4);                    /* err4 */
        return;
    }
    len = size >> 1;
    s->len = len;
    s->replen = rep >> 1;
    s->period = period;
    if (((ULONG)p & 0x1fffff) == (ULONG)p) {    /* inchip */
        s->data = (UBYTE *)p;
        s->rep = (UBYTE *)p + oneshot;
        s->mark = MARK_CL;
        return;
    }
    mem = alloc(size, MEMF_CHIP);               /* infast */
    s->data = mem;
    s->rep = mem + oneshot;
    {
        UWORD *d = (UWORD *)mem, *q = (UWORD *)p;
        UWORD n = s->len - 1;

        do
            *d++ = *q++;
        while (n-- != 0);                       /* DBRA d1 */
    }
    return;
err1:
    BLITZ_RUNERR(mess1);
}

/**
 * @brief SetPeriod Sound#,Period: set the sound's period.
 *
 * BUG(orig) (#70): without a sound `BEQ err5` from the normal code into
 * the error check area ("Uninitialized Sound"; dangling without the
 * debugger).  BUG(orig) (#66): the error check (sndchk) goes on into
 * dpchk and tests the period as channel mask: a period > 15 is "Illegal
 * Chanel Mask" with the debugger.
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param period  d1.w: the period
 */
void BLITZ_ROUTINE _setperiod(UWORD period BL_REG(d1))
{
    struct sound *s = (struct sound *)blitz_string;

    if (!s->data) {
        err5();                                 /* BEQ err5 (error area) */
        return;
    }
    s->period = period;
}

/* InitSound n,len[,period[,repeat]] - empty chip memory */
static void initsound(ULONG len, UWORD period, ULONG rep)
{
    struct sound *s = (struct sound *)blitz_string;

    freesound(s);
    s->period = period;
    s->len = len >> 1;
    s->rep = (UBYTE *)rep;
    s->replen = (len - rep) >> 1;
    s->data = alloc(len, MEMF_CHIP | MEMF_CLEAR);
    s->rep += (ULONG)s->data;
}

/**
 * @brief InitSound Sound#,Length: an empty sound of Length bytes in chip
 *        memory (cleared), period 428, repeat from the start.
 *
 * Errors (debugger, ischk1): "Sound too long" (>= 128K), "Sound Length
 * MUST be even".
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param len  d1: the length in bytes
 */
void BLITZ_ROUTINE doisound2(ULONG len BL_REG(d1))
{
    initsound(len, 428, 0);
}

/**
 * @brief InitSound Sound#,Length,Period: InitSound with a period.
 *
 * Errors (debugger, ischk1): as InitSound Sound#,Length.
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param len     d1: the length in bytes
 * @param period  d2.w: the period
 */
void BLITZ_ROUTINE doisound3(ULONG len BL_REG(d1), UWORD period BL_REG(d2))
{
    initsound(len, period, 0);
}

/**
 * @brief InitSound Sound#,Length,Period,Repeat: InitSound with a period
 *        and a repeat offset.
 *
 * Errors (debugger, ischk2): as InitSound Sound#,Length, and "Repeat
 * offset MUST be even", "Repeat MUST be less than Length".
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param len     d1: the length in bytes
 * @param period  d2.w: the period
 * @param rep     d3: the repeat offset in bytes
 */
void BLITZ_ROUTINE doisound(ULONG len BL_REG(d1), UWORD period BL_REG(d2),
                            ULONG rep BL_REG(d3))
{
    initsound(len, period, rep);
}

/**
 * @brief SoundData Sound#,Offset,Data: write a byte of the sample (inline
 *        MOVE.l (a3),a0:MOVE.b d2,0(a0,d1.l), as in the original).
 *
 * a3 = the Sound object (!libs {#audiolib,$1380}), d1 = the offset, d2 =
 * the byte.  Errors (debugger, sdatachk): "Uninitialized Sound", "Data
 * offset too large".
 */
BLITZ_INLINE(dosdata,
    "   move.l  (a3),a0\n"
    "   move.b  d2,0(a0,d1.l)");
/**
 * @brief PeekSound(Sound#,Offset): a byte of the sample, sign extended to
 *        a word (inline, as in the original).
 *
 * a3 = the Sound object (!libs {#audiolib,$1380}), d1 = the offset; d0.w
 * = the byte.  BUG(orig) (#46): a3 is left as it is - a PeekSound inside
 * a string expression makes the rest of the expression write into the
 * object.  Errors (debugger, sdatachk): as SoundData.
 */
BLITZ_INLINE(dopsound,
    "   move.l  (a3),a0\n"
    "   move.b  0(a0,d1.l),d0\n"
    "   ext.w   d0");

/* ----------------------------------------------------- Sound, LoopSound */

/* wait for 12 changes of the low byte of VHPOSR (the beam line) */
static void wait12(void)
{
    UBYTE last = VHPOSR_B, now;
    int n;

    for (n = 0; n < 12; n++) {
        do
            now = VHPOSR_B;
        while (now == last);
        last = now;
    }
}

/* dochansoff - DMA of the channels off, wait; int bits = mask << 7 */
static UWORD chansoff(UWORD mask)
{
    BL_CUSTOMW(BL_CUSTOM, 0x96) = mask;
    wait12();
    return mask << 7;
}

static void playsound(UWORD mask, WORD nvol, int loop, const UBYTE *args)
{
    struct sound *s = (struct sound *)blitz_string;
    const UBYTE *sp = args;
    UWORD m = mask, len;
    ULONG ptr;
    int ch;

    chansoff(mask);
    for (ch = 0; ch < 4; ch++, m >>= 1) {
        if (!(m & 1))
            continue;
        *(volatile UWORD *)AUD(ch, 8) = 0;
        *(volatile ULONG *)AUD(ch, 0) = (ULONG)s->data;
        *(volatile UWORD *)AUD(ch, 6) = s->period;
        *(volatile UWORD *)AUD(ch, 4) = s->len;
        *(volatile UWORD *)AUD(ch, 8) = 64;
        if (--nvol >= 0)                        /* SUBQ #1,d7:BMI */
            *(volatile UWORD *)AUD(ch, 8) = BL_POPW(sp);
    }
    BL_CUSTOMW(BL_CUSTOM, 0x96) = mask | 0x8000;
    wait12();
    ptr = (ULONG)chzero;
    len = 1;
    if (loop) {
        ptr = (ULONG)s->rep;
        len = s->replen;
    }
    for (ch = 0, m = mask; ch < 4; ch++, m >>= 1)
        if (m & 1) {
            *(volatile UWORD *)AUD(ch, 4) = len;
            *(volatile ULONG *)AUD(ch, 0) = ptr;
        }
}

/**
 * @brief Sound Sound#,Channelmask: play the sound once on the channels.
 *
 * Switches the channels' DMA off, waits for 12 changes of the VHPOSR
 * line byte, starts the sound on every channel of the mask (volume 64 or
 * the next given one) and after another 12 lines sets a zero word in chip memory as the repeat part.
 * Errors (debugger, sndchk): "Uninitialized Sound", "Illegal Chanel
 * Mask" (mask > 15).
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param mask  d1.w: the channel mask
 */
void BLITZ_ROUTINE dosound2(UWORD mask BL_REG(d1))
{
    playsound(mask, 0, 0, 0);
}

/**
 * @brief Sound Sound#,Channelmask,Vol1[,Vol2...]: Sound with volumes.
 *
 * Switches the channels' DMA off, waits for 12 changes of the VHPOSR
 * line byte, starts the sound on every channel of the mask (volume 64 or
 * the next given one) and after another 12 lines sets a zero word in chip memory as the repeat part.
 * Errors (debugger, sndchk): "Uninitialized Sound", "Illegal Chanel
 * Mask" (mask > 15).
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param mask  d1.w: the channel mask
 * @param nvol  d7.w: the number of volumes
 * @param args  a2: the compiler's stack with the volumes (words)
 */
void BLITZ_ROUTINE dosound(UWORD mask BL_REG(d1), WORD nvol BL_REG(d7),
                           const UBYTE *args BL_REG(a2))
{
    playsound(mask, nvol, 0, args);
}

/**
 * @brief LoopSound Sound#,Channelmask: play the sound and repeat its
 *        repeat part.
 *
 * Switches the channels' DMA off, waits for 12 changes of the VHPOSR
 * line byte, starts the sound on every channel of the mask (volume 64 or
 * the next given one) and after another 12 lines sets the repeat part.
 * Errors (debugger, sndchk): "Uninitialized Sound", "Illegal Chanel
 * Mask" (mask > 15).
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param mask  d1.w: the channel mask
 */
void BLITZ_ROUTINE dosoundl2(UWORD mask BL_REG(d1))
{
    playsound(mask, 0, 1, 0);
}

/**
 * @brief LoopSound Sound#,Channelmask,Vol1[,Vol2...]: LoopSound with
 *        volumes.
 *
 * Switches the channels' DMA off, waits for 12 changes of the VHPOSR
 * line byte, starts the sound on every channel of the mask (volume 64 or
 * the next given one) and after another 12 lines sets the repeat part.
 * Errors (debugger, sndchk): "Uninitialized Sound", "Illegal Chanel
 * Mask" (mask > 15).
 * The Sound object comes in a3 (!libs {#audiolib,$1380}, from Sound# in
 * d0).
 *
 * @param mask  d1.w: the channel mask
 * @param nvol  d7.w: the number of volumes
 * @param args  a2: the compiler's stack with the volumes (words)
 */
void BLITZ_ROUTINE dosoundl(UWORD mask BL_REG(d1), WORD nvol BL_REG(d7),
                            const UBYTE *args BL_REG(a2))
{
    playsound(mask, nvol, 1, args);
}

/* ------------------------------------------------------------- DiskPlay */

/* intwait - wait for the audio interrupt bits, then clear them */
static void intwait(UWORD bits)
{
    while ((BL_CUSTOMW(BL_CUSTOM, 0x1e) & bits) != bits)
        ;
    BL_CUSTOMW(BL_CUSTOM, 0x9c) = bits;
}

/* loadbuff - next part of the file into the buffer, 0 = more, -1 = end
   (a length of exactly 0 also returns 0, BUG(orig) (#69)) */
static LONG loadbuff(UBYTE **pp, UWORD dma)
{
    struct bl_regs r = { 0 };
    ULONG n;
    UWORD m = dma;
    int ch;

    if (!dlen)
        return 0;                               /* 'fail2: d0 = 0 */
    if (dlen < 0)
        return -1;
    n = dlen;
    if (n > dbufflen)
        n = dbufflen;
    for (ch = 0; ch < 4; ch++, m >>= 1)
        if (m & 1) {
            *(volatile ULONG *)AUD(ch, 0) = (ULONG)*pp;
            *(volatile UWORD *)AUD(ch, 4) = (UWORD)n >> 1;
        }
    r.a0 = (ULONG)*pp;
    r.d0 = n;
    IFFCALL(TOK_iffread, r);
    dlen -= dbufflen;
    *pp += dbufflen;
    if (*pp >= dbuffend)
        *pp = dbuffat;
    return 0;
}

static void diskplay(const UBYTE *name, UWORD mask, WORD nvol, const UBYTE *args)
{
    const UBYTE *sp = args;
    struct opened o;
    struct bl_regs r = { 0 };
    UBYTE *p;
    UWORD dma, ints, m;
    int ch;

    dbuffat = alloc(dbufflen << 1, MEMF_CHIP);
    p = dbuffat;
    dbuffend = dbuffat + dbufflen + dbufflen;
    if (!opensound(name, &o))
        return;
    dlen = o.len;
    ints = chansoff(mask);
    dma = mask;
    sp -= 4;                                    /* drop string length */
    for (ch = 0, m = mask; ch < 4; ch++, m >>= 1) {
        if (!(m & 1))
            continue;
        *(volatile UWORD *)AUD(ch, 8) = 0;
        *(volatile UWORD *)AUD(ch, 10) = 0;
        *(volatile UWORD *)AUD(ch, 6) = o.period;
        *(volatile UWORD *)AUD(ch, 8) = 64;
        if (nvol) {
            nvol--;
            *(volatile UWORD *)AUD(ch, 8) = BL_POPW(sp);
        }
    }
    loadbuff(&p, dma);
    dma |= 0x8000;
    BL_CUSTOMW(BL_CUSTOM, 0x9c) = ints;
    BL_CUSTOMW(BL_CUSTOM, 0x96) = dma;
    intwait(ints);
    do {
        LONG more = loadbuff(&p, dma);

        intwait(ints);
        if (more)
            break;
    } while (1);
    dma &= 0x7fff;
    BL_CUSTOMW(BL_CUSTOM, 0x96) = dma;
    dealloc(dbuffat, dbufflen << 1);
    IFFCALL(TOK_iffclose, r);
}

/**
 * @brief DiskPlay Filename$,Channelmask: stream an 8SVX file from disk.
 *
 * Through two halves of a chip buffer (DiskBuffer, default 1024 bytes
 * each), waiting for the audio interrupt bits in INTREQR (busy wait, as
 * in the original).  BUG(orig) (#69): a file whose length is a multiple
 * of the buffer length never ends.  Errors: as LoadSound (no file, no
 * VHDR, read error); (debugger, dpchk) "Illegal Chanel Mask".
 *
 * @param name  d0: the file name
 * @param mask  d1.w: the channel mask
 * @param args  a2: the compiler's stack (its string length is dropped)
 */
static void BLITZ_ROUTINE dodplay2_c(const UBYTE *name BL_REG(d0), UWORD mask BL_REG(d1),
                                     const UBYTE *args BL_REG(a2))
{
    diskplay(name, mask, 0, args);
}
BLITZ_THUNK(dodplay2, dodplay2_c, "d1/a0-a1");

/**
 * @brief DiskPlay Filename$,Channelmask,Vol1[,Vol2...]: DiskPlay with
 *        volumes.
 *
 * As DiskPlay Filename$,Channelmask (BUG(orig) #69 too).
 *
 * @param name  d0: the file name
 * @param mask  d1.w: the channel mask
 * @param nvol  d7.w: the number of volumes
 * @param args  a2: the compiler's stack with the string length and the
 *              volumes (words)
 */
static void BLITZ_ROUTINE dodplay_c(const UBYTE *name BL_REG(d0), UWORD mask BL_REG(d1),
                                    WORD nvol BL_REG(d7), const UBYTE *args BL_REG(a2))
{
    diskplay(name, mask, nvol, args);
}
BLITZ_THUNK(dodplay, dodplay_c, "d1/a0-a1");

/* --------------------------------------------------------- init / finit */

/**
 * @brief init: audio DMA and interrupts off, DiskBuffer 1024, a zero word
 *        in chip memory (memlib globalloc).
 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    BL_CUSTOMW(BL_CUSTOM, 0x96) = 0xf;                         /* audio DMA OFF */
    BL_CUSTOMW(BL_CUSTOM, 0x9a) = 0x780;                       /* and interupts */
    dbufflen = 1024;
    chzero = (UWORD *)alloc(2, MEMF_CHIP);
    *chzero = 0;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/**
 * @brief finit: audio DMA off, the volume of all channels 0.
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    int ch;

    BL_CUSTOMW(BL_CUSTOM, 0x96) = 0xf;
    for (ch = 0; ch < 4; ch++)
        *(volatile UWORD *)AUD(ch, 8) = 0;
}
BLITZ_THUNK(codefinit, codefinit_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

static void BLITZ_ERRCHK err5(void)
{
    BLITZ_RUNERR(mess5);
}

/* dpchk - CMP #15,d1:BHI err6 */
static void BLITZ_ERRCHK dpchk_c(UWORD mask BL_REG(d1))
{
    if (mask > 15)
        BLITZ_RUNERR(mess6);
}

/* sndchk - TST.l (a3):BEQ err5, then dpchk (also for SetPeriod's period) */
static void BLITZ_ERRCHK sndchk_c(UWORD value BL_REG(d1))
{
    if (!((struct sound *)blitz_string)->data)
        BLITZ_RUNERR(mess5);
    dpchk_c(value);
}

static void BLITZ_ERRCHK vchk_c(UWORD mask BL_REG(d0))
{
    if (mask > 15)
        BLITZ_RUNERR(mess6);
}

/* ischk1 - length < 128K and even */
static void BLITZ_ERRCHK ischk1_c(ULONG len BL_REG(d1))
{
    if (len >= 65536 * 2)
        BLITZ_RUNERR(mess4);
    if (len & 1)
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK ischk2_c(ULONG len BL_REG(d1), ULONG rep BL_REG(d3))
{
    ischk1_c(len);
    if (rep & 1)
        BLITZ_RUNERR(messa);
    if (rep >= len)
        BLITZ_RUNERR(mess8);
}

/* sdatachk - sound there, offset < length in bytes */
static void BLITZ_ERRCHK sdatachk_c(ULONG off BL_REG(d1))
{
    struct sound *s = (struct sound *)blitz_string;

    if (!s->data)
        BLITZ_RUNERR(mess5);
    if (off >= (ULONG)s->len * 2)
        BLITZ_RUNERR(mess9);
}

/* the thunks and messages last: they leave the assembler in .text while
   GCC still assumes the error check section */
BLITZ_ERRCHK_THUNK(sndchk, sndchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(dpchk, dpchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(vchk, vchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ischk1, ischk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ischk2, ischk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sdatachk, sdatachk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Sound file");
BLITZ_ERRCHK_MESSAGE(mess2, "Can't Find VHDR");
BLITZ_ERRCHK_MESSAGE(mess3, "Error Reading File");
BLITZ_ERRCHK_MESSAGE(mess4, "Sound too long");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Sound");
BLITZ_ERRCHK_MESSAGE(mess6, "Illegal Chanel Mask");
BLITZ_ERRCHK_MESSAGE(mess7, "Sound Length MUST be even");
BLITZ_ERRCHK_MESSAGE(mess8, "Repeat MUST be less than Length");
BLITZ_ERRCHK_MESSAGE(mess9, "Data offset too large");
BLITZ_ERRCHK_MESSAGE(messa, "Repeat offset MUST be even");
