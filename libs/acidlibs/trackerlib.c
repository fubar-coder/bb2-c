/*
 * trackerlib - command library 96: Sound/NoiseTracker modules (M.K., 31
 * samples).  LoadModule PlayModule StopModule.
 *
 * C translation of bblibsrc/trackerlib; binary of the CD's acidlibs
 * checked: same as the source.  The replay routine (mt_init, mt_music,
 * mt_end) is the classic NoiseTracker player with Blitz changes (song
 * length/restart byte taken from the module at $3B6/$3B7).
 *
 * Module object (8 bytes, maximum 5): {data (chip memory), length}.
 * LoadModule: length from Examine, chip memory from memlib, one Read.
 * PlayModule: mt_init, then the vertical blank interrupt (intlib SetInt
 * $C100, level $8025) calls mt_music (myint) every frame; StopModule /
 * finit: ClrInt $C101 and mt_end (volumes 0, audio DMA off).  Both are
 * Blitz/Amiga variants ($A001: BLibJsr / ALibJsr to intlib).  Freeing a
 * module that is playing stops it first.
 *
 * mt_init: highest pattern number from the 128 song positions, the 31
 * sample starts behind the patterns; it CLEARS the first long of each
 * sample in the module (the player's "no sound" loop), filter off, speed
 * 6.  mt_music: every `speed` frames a new row (4 voices: sample, period,
 * DMA, commands B C D E F), otherwise the effects 0-4 and A.  Paula
 * registers $DFF0A0-$DFF0DF, DMACON $DFF096, filter $BFE001.
 *
 * Assembler: only the two DBF busy loops of mt_setdma (timing, 301
 * iterations each) and the Blitz/Amiga prefixes.
 *
 * BUG(orig), kept (as in the NoiseTracker player):
 *   - mt_init: every read position decrements the counter once (DBF, or
 *     SUBQ for a new highest pattern number, skipping the DBF), so
 *     positions 0-126 are checked, 127 never; a new maximum at position
 *     126 turns the counter 0 into $FFFF: the search reads ~64 KB more
 *     and mt_lop3 clears longs behind the module. (#289)
 *   - the sample's volume word includes the finetune byte (MOVE.w). (#290)
 *   - repeat start * 2 as a word (ASL.w): repeats beyond 64K wrap. (#291)
 *   - after a pattern break / position jump (D, B) the end of the song is
 *     compared with the first byte of the pattern data instead of the
 *     song length, unless the row was the last of the pattern. (#292)
 *   - set volume (C) writes the volume as a BYTE to AUDxVOL (both halves
 *     of the register) (#293); C and F only clamp positive values (signed
 *     compares) (#294).
 *   - a sample number above 31 reads the start from behind the table
 *     (the voice data; the C library has them in the same order, beyond
 *     that the original reads its own code). (#295)
 *   - LoadModule of a missing file / a directory: runtime error without
 *     FreeMem of the FileInfoBlock (a directory also stays locked). (#296)
 * Fixed (intended deviation, #957): "Error Accessing Module File" (err1)
 * traps in the normal code, but the original has its message in the error
 * check area (without the debugger d0 points behind the library); here
 * the message is in the normal code.
 * DIFF: none in the results; the interrupt routine is C (GCC keeps
 * d2-d7/a2-a6, the thunk d0-d1/a0-a1: everything like the original).
 */
#include "blitzlib.h"
#include <stddef.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

#define INTLEVEL   0x8025

#define CIAAPRA       (*(volatile UBYTE *)0xbfe001)

/* entry points (C routines / thunks / prefixes, defined below) */
void doplaymod(void), b_doplaymod(void), dostopmod(void), b_dostopmod(void);
void trackerlib_codefinit(void), freemod(void), freemod_b(void), myint(void);
void playchk(void);
BLITZ_ENTRY_C(doloadmod);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#trackerlib,0,0,finit,errors} */
BLITZ_LIBHEADER(LIB_trackerlib, 0, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_DUMTOKE(_toke, "Module", "A Sound or Noise tracker module");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_trackerlib, 0x1380, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doloadmod), 0, 0);
BLITZ_NAME("LoadModule", "Module#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_trackerlib, 0x1380);
BLITZ_SUBS(doplaymod, playchk, 0);
BLITZ_NAME("PlayModule", "Module#");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dostopmod, 0, 0);
BLITZ_NAME("StopModule", "");

BLITZ_NULLSUB_AT(finit, trackerlib_codefinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freemod, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 3);

/* ------------------------------------------------------------ variables */

struct module {
    UBYTE *data;
    ULONG len;
};

static UBYTE *mt_data;
static BYTE mt_speed = 6;
static UBYTE mt_songpos;
static UWORD mt_pattpos;
static BYTE mt_counter;
static UBYTE mt_break;
static UWORD mt_dmacon;

/* a voice of the player (28 bytes, mt_voice1..4) */
struct voice {
    union {
        ULONG note;             /* 0.l: the note of the pattern */
        struct {
            UWORD noteword;     /* 0.w: sample high nibble, period */
            UWORD cmdword;      /* 2.w: sample low nibble, command, parameter */
        };
        struct {
            UBYTE notehi;       /* 0.b: sample high nibble */
            UBYTE notelo;
            UBYTE cmd;          /* 2.b: sample low nibble, command */
            union {             /* 3.b: command parameter, */
                UBYTE param;    /*      read unsigned (VUB) */
                BYTE sparam;    /*      or signed (VB) */
            };
        };
    };
    ULONG start;                /* 4.l: sample start */
    UWORD length;               /* 8.w */
    ULONG loopstart;            /* $a.l */
    UWORD looplen;              /* $e.w */
    UWORD period;               /* $10.w */
    UWORD volume;               /* $12.w */
    UWORD dmabit;               /* $14.w */
    BYTE portdir;               /* $16.b: portamento direction */
    UBYTE portspeed;            /* $17.b: portamento speed */
    UWORD porttarget;           /* $18.w: portamento target */
    UBYTE vibcmd;               /* $1a.b: vibrato command */
    UBYTE vibpos;               /* $1b.b: vibrato position */
};
static_assert(sizeof(struct voice) == 28);

/* mt_samplestarts and the voices in one block as in the original: a
   sample number > 31 (bad module) reads the voices behind the table */
static struct {
    UBYTE *samplestarts[31];
    struct voice voices[4];
} mt = { .voices = { { .dmabit = 1 }, { .dmabit = 2 }, { .dmabit = 4 }, { .dmabit = 8 } } };
static_assert(sizeof mt == 31 * 4 + 4 * 28);
#define mt_samplestarts mt.samplestarts
#define mt_voices mt.voices

/* a sample of the module (30 bytes): the name, then the info the player
   reads (mt_playvoice addresses it as sinfo + sample * 30) */
struct modsampinfo {
    UWORD length;               /* in words */
    UWORD finevol;              /* finetune byte, volume byte: read as one
                                   word, BUG(orig) (#290) */
    UWORD repeat;               /* repeat start in words */
    UWORD replen;               /* repeat length in words */
};
struct modsample {
    UBYTE name[22];
    struct modsampinfo info;
};
static_assert(sizeof(struct modsample) == 30);

/* the module (M.K., 31 samples): the header, then the patterns (1024
   bytes each), then the samples */
struct modhead {
    UBYTE title[20];
    struct modsample samples[31];
    UBYTE songlen;              /* $3B6 */
    UBYTE restart;              /* $3B7 */
    UBYTE order[128];           /* $3B8: the song positions */
    UBYTE id[4];                /* $438: "M.K." */
    UBYTE patterns[];           /* $43C */
};
static_assert(sizeof(struct modhead) == 0x43c);

#define RW(r, o) (*(volatile UWORD *)((r) + (o)))      /* AUDx register */
#define RL(r, o) (*(volatile ULONG *)((r) + (o)))

static const UBYTE mt_sin[32] = {
    0x00, 0x18, 0x31, 0x4a, 0x61, 0x78, 0x8d, 0xa1, 0xb4, 0xc5, 0xd4, 0xe0,
    0xeb, 0xf4, 0xfa, 0xfd, 0xff, 0xfd, 0xfa, 0xf4, 0xeb, 0xe0, 0xd4, 0xc5,
    0xb4, 0xa1, 0x8d, 0x78, 0x61, 0x4a, 0x31, 0x18,
};

static const UWORD mt_periods[38] = {
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c, 0x023a, 0x021a,
    0x01fc, 0x01e0, 0x01c5, 0x01ac, 0x0194, 0x017d, 0x0168, 0x0153, 0x0140,
    0x012e, 0x011d, 0x010d, 0x00fe, 0x00f0, 0x00e2, 0x00d6, 0x00ca, 0x00be,
    0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f, 0x0087, 0x007f, 0x0078, 0x0071,
    0x0000, 0x0000,
};

/* err1's message in the normal code (fixed, #957: in the original it lies
   in the error check area); mess2 is in the error check area (playchk) */
static const char mess1[] = "Error Accessing Module File";
extern const char mess2[];

/* MOVE.w #$12c,d0:DBF d0,* - a timing loop, assembler so that it stays */
static inline void mt_delay(void)
{
    UWORD n = 0x12c;

    __asm volatile("1: dbf %0,1b" : "+d"(n));
}

/* ------------------------------------------------------------- mt_init */

static void mt_init(void)
{
    /* info: the module, moved by one sample per mt_lop3 step, so that
       info->samples[0] is the current sample (as 42(a0) in the original) */
    struct modhead *info = (struct modhead *)mt_data;
    UBYTE *order = info->order, *sdata;
    WORD n = 0x7f;
    BYTE pat = 0, maxpat;
    int i;

    /* highest pattern number (see BUG(orig) (#289)) */
    do {
        maxpat = pat;                           /* mt_loop */
        n--;
        for (;;) {                              /* mt_lop2 */
            pat = *order++;
            if (pat > maxpat)                   /* CMP.b d2,d1:BGT mt_loop */
                break;
            if (--n == -1)                      /* DBF d0,mt_lop2 */
                goto found;
        }
    } while (1);
found:
    maxpat++;
    sdata = (UBYTE *)info + offsetof(struct modhead, patterns)
            + ((ULONG)(UBYTE)maxpat << 10);
    for (i = 0; i < 31; i++) {                  /* mt_lop3 */
        *(ULONG *)sdata = 0;
        mt_samplestarts[i] = sdata;
        sdata += (ULONG)info->samples[0].info.length << 1;
        info = (struct modhead *)((UBYTE *)info + sizeof(struct modsample));
    }
    CIAAPRA |= 2;
    mt_speed = 6;
    BL_CUSTOMW(BL_CUSTOM, 0xa8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xb8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xc8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xd8) = 0;
    mt_songpos = 0;
    mt_counter = 0;
    mt_pattpos = 0;
}

static void mt_end(void)
{
    BL_CUSTOMW(BL_CUSTOM, 0xa8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xb8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xc8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0xd8) = 0;
    BL_CUSTOMW(BL_CUSTOM, 0x96) = 0xf;
}

/* ------------------------------------------------------------ effects */

static void mt_arpeggio(struct voice *v, ULONG aud)
{
    WORD rem = (WORD)(UBYTE)mt_counter % 3;     /* DIVS #3:SWAP */
    UWORD step, period, newper;
    const UWORD *tab;
    int n;

    if (rem == 0) {
        newper = v->period;                   /* mt_arp2 */
        RW(aud, 6) = newper;
        return;
    }
    if (rem == 2)
        step = v->param & 0xf;                 /* mt_arp1 */
    else
        step = v->param >> 4;
    period = v->period;
    tab = mt_periods;
    for (n = 0; n < 0x25; n++) {                /* MOVEQ #$24,d7:DBF */
        newper = tab[step];                     /* MOVE.w 0(a0,d0.w),d2 */
        if ((WORD)period >= (WORD)*tab) {       /* CMP.w (a0),d1:BGE */
            RW(aud, 6) = newper;
            return;
        }
        tab++;
    }
}

static void mt_setmyport(struct voice *v)
{
    UWORD target = v->noteword & 0xfff, period = v->period;

    v->porttarget = target;
    v->portdir = 0;
    if (target == period)
        v->porttarget = 0;                        /* mt_clrport */
    else if ((WORD)target < (WORD)period)       /* BGE mt_rt */
        v->portdir = 1;
}

static void mt_myport(struct voice *v, ULONG aud)
{
    UWORD speed;

    if (v->sparam) {
        v->portspeed = v->sparam;
        v->sparam = 0;
    }
    if (!v->porttarget)                           /* mt_myslide */
        return;
    speed = v->portspeed;
    if (v->portdir) {                          /* mt_mysub */
        v->period -= speed;
        if ((WORD)v->porttarget < (WORD)v->period) {
            RW(aud, 6) = v->period;
            return;
        }
        v->period = v->porttarget;
        v->porttarget = 0;
        RW(aud, 6) = v->period;
        return;
    }
    v->period += speed;
    if (!((WORD)v->porttarget > (WORD)v->period)) {
        v->period = v->porttarget;
        v->porttarget = 0;
    }
    RW(aud, 6) = v->period;                   /* mt_myok */
}

static void mt_vib(struct voice *v, ULONG aud)
{
    UWORD sinpos;                               /* d0: the sine index */
    UWORD depth;                                /* d0 */
    UWORD period;                               /* d0 */
    UWORD delta;

    if (v->sparam)
        v->vibcmd = v->sparam;
    sinpos = (v->vibpos >> 2) & 0x1f;        /* mt_vi */
    delta = mt_sin[sinpos];
    depth = v->vibcmd & 0xf;
    delta = (UWORD)(delta * depth) >> 6;        /* MULU:LSR.w #6 */
    period = v->period;
    if ((BYTE)v->vibpos < 0)
        period -= delta;
    else
        period += delta;
    RW(aud, 6) = period;
    v->vibpos += (v->vibcmd >> 2) & 0x3c;
}

static void mt_volslide(struct voice *v, ULONG aud)
{
    UWORD step = v->param >> 4;

    if (step) {
        v->volume += step;
        if ((WORD)(v->volume - 0x40) >= 0)    /* CMP.w #$40:BMI */
            v->volume = 0x40;
        RW(aud, 8) = v->volume;
        return;
    }
    step = v->param & 0xf;                     /* mt_voldown */
    v->volume -= step;
    if ((WORD)v->volume < 0)
        v->volume = 0;
    RW(aud, 8) = v->volume;
}

static void mt_portup(struct voice *v, ULONG aud)
{
    v->period -= v->param;
    if ((WORD)((v->period & 0xfff) - 0x71) < 0)
        v->period = (v->period & 0xf000) | 0x71;
    RW(aud, 6) = v->period & 0xfff;
}

static void mt_portdown(struct voice *v, ULONG aud)
{
    v->period += v->param;
    if ((WORD)((v->period & 0xfff) - 0x358) >= 0)
        v->period = (v->period & 0xf000) | 0x358;
    RW(aud, 6) = v->period & 0xfff;
}

static void mt_checkcom(struct voice *v, ULONG aud)
{
    UBYTE cmd;

    if (!(v->cmdword & 0xfff)) {
        RW(aud, 6) = v->period;               /* mt_nop */
        return;
    }
    cmd = v->cmd & 0xf;
    switch (cmd) {
    case 0:
        mt_arpeggio(v, aud);
        return;
    case 1:
        mt_portup(v, aud);
        return;
    case 2:
        mt_portdown(v, aud);
        return;
    case 3:
        mt_myport(v, aud);
        return;
    case 4:
        mt_vib(v, aud);
        return;
    }
    RW(aud, 6) = v->period;
    if (cmd == 0xa)
        mt_volslide(v, aud);
}

static void mt_checkcom2(struct voice *v, ULONG aud)
{
    switch (v->cmd & 0xf) {
    case 0xe:                                   /* mt_setfilt */
        CIAAPRA = (CIAAPRA & 0xfd) | ((v->param & 1) << 1);
        return;
    case 0xd:                                   /* mt_pattbreak */
        mt_break = ~mt_break;
        return;
    case 0xb:                                   /* mt_posjmp */
        mt_songpos = v->param - 1;
        mt_break = ~mt_break;
        return;
    case 0xc:                                   /* mt_setvol */
        if (v->sparam > 0x40)
            v->sparam = 0x40;
        *(volatile UBYTE *)(aud + 8) = v->param;   /* MOVE.b */
        return;
    case 0xf:                                   /* mt_setspeed */
        if (v->sparam > 0x1f)
            v->sparam = 0x1f;
        if (v->sparam) {
            mt_speed = v->sparam;
            mt_counter = 0;
        }
        return;
    }
}
/* ------------------------------------------------------------- new row */

/* the info of the sample at sinfo + sample * 30 */
/* a field of the info of the sample at sinfo + sample * 30 (read as a
   UWORD at its offset: a struct access compiles differently) */
#define SINFO(off, f) (*(const UWORD *)(sinfo + (off) + offsetof(struct modsampinfo, f)))
/* ------------------------------------------------------------- new row */

static void mt_playvoice(struct voice *v, ULONG aud, const UBYTE *pat, ULONG *pos,
                         const UBYTE *sinfo)
{
    ULONG sample;
    UWORD repeat, len;

    v->note = *(const ULONG *)(pat + *pos);
    *pos += 4;
    sample = (UBYTE)((v->cmd & 0xf0) >> 4) | (v->notehi & 0xf0);
    if (sample) {
        ULONG off = (UWORD)((UWORD)sample * 0x1e); /* MULU #$1e,d4 */

        v->start = *(ULONG *)((UBYTE *)mt_samplestarts + ((sample - 1) << 2));
        v->length = SINFO(off, length);
        v->volume = SINFO(off, finevol);     /* BUG(orig) (#290) */
        repeat = SINFO(off, repeat);
        if (repeat) {
            v->loopstart = v->start + (UWORD)(repeat << 1);
            len = repeat + SINFO(off, replen);
            v->length = len;
            v->looplen = SINFO(off, replen);
            RW(aud, 8) = v->volume;
        } else {                                /* mt_noloop */
            v->loopstart = v->start;
            v->looplen = SINFO(off, replen);
            RW(aud, 8) = v->volume;
        }
    }
    if (v->noteword & 0xfff) {                     /* mt_setregs */
        if ((v->cmd & 0xf) == 3)
            mt_setmyport(v);
        else {                                  /* mt_setperiod */
            v->period = v->noteword & 0xfff;
            BL_CUSTOMW(BL_CUSTOM, 0x96) = v->dmabit;
            v->vibpos = 0;
            RL(aud, 0) = v->start;
            RW(aud, 4) = v->length;
            RW(aud, 6) = v->period & 0xfff;
            mt_dmacon |= v->dmabit;
        }
    }
    mt_checkcom2(v, aud);
}

/* ------------------------------------------------------------ mt_music */

static void mt_music(void)
{
    const UBYTE *mod = mt_data;                 /* a0: the module */
    /* a0 at mt_endr: the song length and restart bytes; after a break
       still the patterns or the module (BUG(orig) (#292), see above) */
    const UBYTE *songend = mod;
    int i;

    mt_counter++;
    if (mt_counter < mt_speed) {                /* CMP.b mt_speed,d0:BLT */
        for (i = 0; i < 4; i++)                 /* mt_nonew */
            mt_checkcom(&mt_voices[i], 0xdff0a0 + 16 * i);
    } else {
        const UBYTE *sinfo, *order;
        const UBYTE *patterns;                  /* a0 */
        ULONG pos;

        mt_counter = 0;
        /* mt_getnew: sinfo + sample * 30 = the info of sample (1-31) */
        sinfo = mod + 0xc;              /* = &samples[-1].info: 20 + 22 - 30 */
        order = mod + offsetof(struct modhead, order);
        patterns = mod + offsetof(struct modhead, patterns);
        songend = patterns;
        pos = (ULONG)order[mt_songpos] << 10;
        BL_SETW(pos, pos + mt_pattpos);         /* ADD.w mt_pattpos,d1 */
        mt_dmacon = 0;
        for (i = 0; i < 4; i++)
            mt_playvoice(&mt_voices[i], 0xdff0a0 + 16 * i, patterns, &pos, sinfo);

        mt_delay();                             /* mt_setdma */
        BL_CUSTOMW(BL_CUSTOM, 0x96) = mt_dmacon | 0x8000;
        mt_delay();
        for (i = 3; i >= 0; i--) {
            const struct voice *v = &mt_voices[i];

            RL(0xdff0a0 + 16 * i, 0) = v->loopstart;
            RW(0xdff0a0 + 16 * i, 4) = v->looplen;
        }
        mt_pattpos += 0x10;
        if (mt_pattpos == 0x400) {
            songend = mt_data + offsetof(struct modhead, songlen);
            goto nex;
        }
    }
    while (mt_break) {                          /* mt_endr */
nex:
        mt_pattpos = 0;
        mt_break = 0;
        mt_songpos = (mt_songpos + 1) & 0x7f;
        if (mt_songpos == songend[0])           /* CMP.b (a0),d1 */
            mt_songpos = songend[1];
    }
}

/* myint - the interrupt routine (keeps all registers) */
static void BLITZ_ROUTINE myint_c(void)
{
    mt_music();
}
BLITZ_THUNK(myint, myint_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- module */

void BLITZ_ROUTINE doloadmod(const UBYTE *name BL_REG(d1), struct DosLibrary *DOSBase BL_REG(a6));

static void freemod2(struct module *m, int blitz);

/**
 * @brief LoadModule Module#,Filename$ - load a Sound/NoiseTracker module
 * into chip memory (Amiga mode only).
 *
 * Frees the old module (stopping it if it is playing), takes the length
 * from Examine (FileInfoBlock from memlib), allocates chip memory and
 * reads the file in one Read.
 *
 * Errors: "Error Accessing Module File" (runtime error also without the
 * debugger, its message in the normal code: fixed, #957) for a missing
 * file or a directory, or when the file cannot be opened.
 *
 * BUG(orig) (#296): on that error the FileInfoBlock is not freed (a
 * directory also stays locked).
 *
 * @param name    d1: the file name; a3: the module object
 * @param DOSBase a6: dos.library
 */
void BLITZ_ROUTINE doloadmod(const UBYTE *name BL_REG(d1), struct DosLibrary *DOSBase BL_REG(a6))
{
    struct module *m = (struct module *)blitz_string;
    struct bl_regs r = { 0 };
    struct FileInfoBlock *fib;
    BPTR lock, fh;

    freemod2(m, 0);
    r.d0 = 260;
    r.d1 = MEMF_PUBLIC;
    fib = (struct FileInfoBlock *)BLITZ_ALIBJSR(TOK_globalloc, r);
    lock = Lock((STRPTR)name, -2);
    if (!Examine(lock, fib) || fib->fib_DirEntryType > 0) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return;
    }
    m->len = fib->fib_Size;
    UnLock(lock);
    r.a1 = (ULONG)fib;
    r.d0 = 260;
    BLITZ_ALIBJSR(TOK_globfree, r);
    r.d0 = m->len;
    r.d1 = MEMF_CHIP;
    m->data = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    fh = Open((STRPTR)name, MODE_OLDFILE);
    if (!fh) {
        BLITZ_RUNERR(mess1);
        return;
    }
    Read(fh, m->data, m->len);
    Close(fh);
}

/* stopmod2 - ClrInt (ALibJsr / BLibJsr), mt_end */
static void stopmod2(int blitz)
{
    struct bl_regs r = { .d0 = INTLEVEL };

    if (blitz)
        BLITZ_BLIBJSR(TOK_clrint, r);
    else
        BLITZ_ALIBJSR(TOK_clrint, r);
    mt_end();
}

static void playmod2(int blitz)
{
    struct module *m = (struct module *)blitz_string;
    struct bl_regs r = { .d0 = INTLEVEL, .d1 = (ULONG)myint };

    mt_data = m->data;
    mt_init();
    if (blitz)
        BLITZ_BLIBJSR(TOK_setint, r);
    else
        BLITZ_ALIBJSR(TOK_setint, r);
}

/* freemod - stop it if it is playing, free the memory */
static void freemod2(struct module *m, int blitz)
{
    struct bl_regs r = { 0 };

    if (!m->data)
        return;
    if (m->data == mt_data)
        stopmod2(blitz);
    r.a1 = (ULONG)m->data;
    m->data = 0;
    r.d0 = m->len;
    BLITZ_ALIBJSR(TOK_globfree, r);
}

/**
 * @brief PlayModule Module# - start playing a module (Amiga / Blitz mode
 * variant).
 *
 * mt_init on the module (highest pattern, sample starts, clears the first
 * long of each sample, filter off, speed 6), then the vertical blank
 * interrupt (intlib SetInt $C100, level $8025; ALibJsr / BLibJsr) calls
 * mt_music every frame.  a3: the module object.
 *
 * Errors (debugger, playchk): "Uninitialized Module".
 *
 * BUG(orig): the NoiseTracker player's bugs, see the file header (#289 -
 * #295).
 */
static void BLITZ_ROUTINE doplaymod_a(void) { playmod2(0); }
/** @brief PlayModule Module# - the Blitz mode variant (BLibJsr). */
static void BLITZ_ROUTINE doplaymod_b(void) { playmod2(1); }
/**
 * @brief StopModule - stop the module (Amiga mode variant; also the
 * library's finit).
 *
 * ClrInt $C101 (level $8025), then mt_end: volumes 0, audio DMA off.
 */
static void BLITZ_ROUTINE dostopmod_a(void) { stopmod2(0); }
/** @brief StopModule - the Blitz mode variant (BLibJsr). */
static void BLITZ_ROUTINE dostopmod_b(void) { stopmod2(1); }
/**
 * @brief Free Module (the object library's _free; Amiga / Blitz mode
 * variant) - stop the module if it is playing, free its memory.
 *
 * a3: the module object.
 */
static void BLITZ_ROUTINE freemod_a(void) { freemod2((struct module *)blitz_string, 0); }
static void BLITZ_ROUTINE freemod_b_c(void) { freemod2((struct module *)blitz_string, 1); }

BLITZ_THUNK(b_doplaymod, doplaymod_b, "d1/a0-a1");
BLITZ_AB_ROUTINE(doplaymod, b_doplaymod,
"   movem.l d1/a0-a1,-(sp)\n"
"   jsr     _doplaymod_a\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   rts");

BLITZ_THUNK(b_dostopmod, dostopmod_b, "d1/a0-a1");
BLITZ_AB_ROUTINE(dostopmod, b_dostopmod,
"   movem.l d1/a0-a1,-(sp)\n"
"   jsr     _dostopmod_a\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   rts");
BLITZ_THUNK(trackerlib_codefinit, dostopmod_a, "d1/a0-a1");

BLITZ_THUNK(freemod_b, freemod_b_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(freemod, freemod_b,
"   movem.l d1/a0-a1,-(sp)\n"
"   jsr     _freemod_a\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   rts");

/* ------------------------------------------- error check (debugger only) */

static void BLITZ_ERRCHK playchk_c(void)
{
    if (!((struct module *)blitz_string)->data)
        BLITZ_RUNERR(mess2);
}

BLITZ_ERRCHK_THUNK(playchk, playchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized Module");
