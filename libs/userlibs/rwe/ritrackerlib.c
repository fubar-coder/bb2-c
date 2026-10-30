/*
 * ritrackerlib - RWE userlib 58 (#ritrackerlib, Reflective Images): a
 * Protracker module player, the Protracker 2.3A play routine of Peter
 * "Crayon" Hanning / Mushroom Studios (released as Public Domain,
 * archive/README.md) with RI's commands around it:
 *   TrackInit address    TrackPlay    TrackStop    TrackDMA dmawait.w
 *   Tracker on+off       TrackerChannels mask      TrackerPause On|Off
 *
 * C translation of rwelibsrc/ptlib/ptlib6.bb, the latest of the eight
 * ptlib versions (choice: docs/rwelibs/port/ritrackerlib.md); reference:
 * the reference build of that source with blitz2 2.10 (REFERENCE
 * refbuild, ref/ritrackerlib.obj) - no binary of the library exists.
 * The CIA tracker library (ciatrklib, ptlib/CIATrackerLib1.bb) contains
 * the same play routine; the two ports share no code.
 *
 * The library plays nothing by itself: _ciaplayerinit returns at once
 * (its first instruction is an RTS), so the program calls TrackPlay
 * itself, once per tick (e.g. from a SetInt 5 vertical blank handler).
 *   TrackInit address   the module (a Protracker file in memory, 31
 *                       samples, "M.K."), initialises the song (unless
 *                       Tracker Off) and enables playing
 *   TrackPlay           one tick of the play routine (mt_music)
 *   TrackStop           mt_end (voices silent, audio DMA off)
 *   TrackDMA n          the DBRA count of the waits for Paula's DMA
 *   Tracker on+off      0: mt_end and status off, else status on
 *   TrackerChannels m   the voices to play (bit 3 = voice 1 ... bit 0 =
 *                       voice 4); a cleared voice's registers are zeroed
 *   TrackerPause On|Off nonzero: mt_end; 0: enable and play one tick
 *   init: playing = 0; finit: mt_end if TrackInit was called
 *
 * State: the original's data block from mt_FunkTable to mt_olddata
 * (struct ptdata, the same layout: the routine reads behind the period
 * table into the voices, see BUG(orig)) and the variables of the glue
 * code (struct ptglue, the same layout).  Paula's registers and the CIAs
 * are written as the original writes them (volatile, same width).
 * The routines are C; register contents the original carries from one
 * routine to the next (the pattern offset d1, ...) are parameters and
 * locals.  No assembler besides the thunks (and DIVU through bl_divu_w).
 *
 * BUG(orig) of RI's code:
 *   - _ciaplayerinit is dead (RTS first): no interrupt plays the module,
 *     SetInt (intlib $C100) and the CIA-B timer setup behind it never
 *     run; the code is kept (ciaplayerinit_unreached) and links intlib
 *     like the original.  deftime stays 0, so a tempo command (Fxx >=
 *     $20) writes 0 to CIA-B timer A's latch ($BFD400/$BFD500) and to
 *     ciatime.
 *   - Fxx compares signed (CMP.B #32: BGE): F80-FFF set the speed to
 *     $80-$FF instead of the tempo.
 *   - TrackerChannels: a masked voice does not advance the pattern
 *     offset, so the following voices play the notes of the voice
 *     before; effects (mt_CheckEfx) and the loop pointers of mt_SetDMA
 *     still go to all four voices.
 *   - Tracker Off clears mt_Enable (mt_end); Tracker On only ANDs it, so
 *     the song stays stopped until TrackInit or TrackerPause 0.
 *   - TrackInit while Tracker Off does not initialise the song (mt_init
 *     returns) but enables playing; Tracker On then plays with the old
 *     song pointer.
 *   - F00 (stop: mt_SetSpeed branches to mt_end, which loads a0 with
 *     $DFF000) on voice 1-3: the voices behind read their notes from the
 *     custom chip registers instead of the pattern.
 *   - Effect 8xx (RI's track event) stores its value in nor_TrkEvt on
 *     the ticks after the first, but no command reads it.
 * BUG(orig) of the play routine (the known PT2.3A bugs, cf. the changelog
 * of 8bitbubsy's pt23f replayer):
 *   - mt_init: the pattern count scan (signed compare; the scan ends
 *     only through DBRA, a new maximum at order position 126 lets it run
 *     on through the pattern data); the CLR.L of the first longword of
 *     every sample also clears 4 bytes behind the module for empty
 *     samples at its end.
 *   - mt_LowMask starts at 0: the first 1xx/2xx before any E1x/E2x does
 *     not slide.
 *   - The period lookups (mt_SetPeriod, mt_Arpeggio, mt_SetTonePorta,
 *     glissando) read behind the row of the finetune, and for finetune
 *     -1 behind the table into the voice data, when no entry fits; a
 *     finetune byte above 15 from the sample header reads far behind.
 *   - Tremolo's ramp-down waveform tests the vibrato position.
 *   - 9xx: compared signed with the length (samples of 64 KB and more
 *     get length 1); with a note it is applied twice (mt_CheckMoreEfx
 *     before and after mt_SetPeriod); loop offsets are shifted as words
 *     (repeat >= 32768 words wraps).
 *   - Sample numbers above 31 read behind mt_SampleStarts and the sample
 *     headers.
 */
#include "blitzlib.h"
#include <stddef.h>
#include <hardware/custom.h>

#define CUSTOM  (*(volatile struct Custom *)0xdff000)
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define CIABTALO (*(volatile UBYTE *)0xbfd400)
#define CIABTAHI (*(volatile UBYTE *)0xbfd500)
#define CIABICR (*(volatile UBYTE *)0xbfdd00)

typedef volatile struct AudChannel aud_t;

/* entries (thunks below) */
void _initmodule(void), mt_music(void), _stopmodule(void), _trackSetDMA(void);
void _tracker(void), _trackermask(void), _trackerpause(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ritrackerlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ritrackerlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_initmodule, 0, 0);
BLITZ_NAME("TrackInit", "address");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(mt_music, 0, 0);
BLITZ_NAME("TrackPlay", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_stopmodule, 0, 0);
BLITZ_NAME("TrackStop", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_trackSetDMA, 0, 0);
BLITZ_NAME("TrackDMA", "dmawait.w");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_tracker, 0, 0);
BLITZ_NAME("Tracker", "on+off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_trackermask, 0, 0);
BLITZ_NAME("TrackerChannels", "mask");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_trackerpause, 0, 0);
BLITZ_NAME("TrackerPause", "On|Off");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- state */

/* mt_chanNtemp: 44 bytes per voice (n_note ... n_reallength) */
struct chan {
    UWORD note;                         /*  0 n_note (sample hi nibble, period) */
    UBYTE cmd;                          /*  2 n_cmd (sample lo nibble, effect) */
    UBYTE cmdlo;                        /*  3 n_cmdlo */
    UBYTE *start;                       /*  4 n_start */
    UWORD length;                       /*  8 n_length */
    UBYTE *loopstart;                   /* 10 n_loopstart */
    UWORD replen;                       /* 14 n_replen */
    UWORD period;                       /* 16 n_period */
    UBYTE finetune;                     /* 18 n_finetune */
    UBYTE volume;                       /* 19 n_volume */
    UWORD dmabit;                       /* 20 n_dmabit */
    UBYTE toneportdirec;                /* 22 */
    UBYTE toneportspeed;                /* 23 */
    UWORD wantedperiod;                 /* 24 */
    UBYTE vibratocmd;                   /* 26 */
    UBYTE vibratopos;                   /* 27 */
    UBYTE tremolocmd;                   /* 28 */
    UBYTE tremolopos;                   /* 29 */
    UBYTE wavecontrol;                  /* 30 */
    UBYTE glissfunk;                    /* 31 */
    UBYTE sampleoffset;                 /* 32 */
    UBYTE pattpos;                      /* 33 */
    UBYTE loopcount;                    /* 34 */
    UBYTE funkoffset;                   /* 35 */
    UBYTE *wavestart;                   /* 36 n_wavestart */
    UWORD reallength;                   /* 40 n_reallength */
    UWORD pad;                          /* 42 (Dc.l x 11) */
};

/* the original's data from mt_FunkTable to mt_olddata */
static struct ptdata {
    UBYTE funktable[16];                /*    0 mt_FunkTable */
    UBYTE vibtable[32];                 /*   16 mt_VibratoTable */
    UWORD periods[16 * 36];             /*   48 mt_PeriodTable */
    struct chan chan[4];                /* 1200 mt_chan1temp ... */
    UBYTE *samplestarts[32];            /* 1376 mt_SampleStarts */
    UBYTE *songdataptr;                 /* 1504 mt_SongDataPtr */
    UBYTE speed;                        /* 1508 */
    UBYTE counter;
    UBYTE songpos;
    UBYTE pbreakpos;
    UBYTE posjumpflag;
    UBYTE pbreakflag;
    UBYTE lowmask;
    UBYTE pattdeltime;
    UBYTE pattdeltime2;
    UBYTE enable;                       /* 1517 mt_Enable */
    UBYTE pad[2];                       /* Dc.b 0, Even */
    UWORD patternpos;                   /* 1520 */
    UWORD dmacontemp;
    UBYTE trkevt;                       /* 1524 nor_TrkEvt */
    UBYTE lstevt;                       /* nor_LstEvt (unused) */
    UWORD dmawait;                      /* 1526 DMA_Wait */
    ULONG playing;                      /* 1528 */
    BYTE status;                        /* 1532 Tracker on/off */
    UBYTE mask;                         /* 1533 TrackerChannels */
    UBYTE *olddata;                     /* 1534 mt_olddata */
} pt = {
    { 0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128 },
    {   0,  24,  49,  74,  97, 120, 141, 161,
      180, 197, 212, 224, 235, 244, 250, 253,
      255, 253, 250, 244, 235, 224, 212, 197,
      180, 161, 141, 120,  97,  74,  49,  24 },
    {
        /* Tuning 0, Normal */
        856,808,762,720,678,640,604,570,538,508,480,453,
        428,404,381,360,339,320,302,285,269,254,240,226,
        214,202,190,180,170,160,151,143,135,127,120,113,
        /* Tuning 1 */
        850,802,757,715,674,637,601,567,535,505,477,450,
        425,401,379,357,337,318,300,284,268,253,239,225,
        213,201,189,179,169,159,150,142,134,126,119,113,
        /* Tuning 2 */
        844,796,752,709,670,632,597,563,532,502,474,447,
        422,398,376,355,335,316,298,282,266,251,237,224,
        211,199,188,177,167,158,149,141,133,125,118,112,
        /* Tuning 3 */
        838,791,746,704,665,628,592,559,528,498,470,444,
        419,395,373,352,332,314,296,280,264,249,235,222,
        209,198,187,176,166,157,148,140,132,125,118,111,
        /* Tuning 4 */
        832,785,741,699,660,623,588,555,524,495,467,441,
        416,392,370,350,330,312,294,278,262,247,233,220,
        208,196,185,175,165,156,147,139,131,124,117,110,
        /* Tuning 5 */
        826,779,736,694,655,619,584,551,520,491,463,437,
        413,390,368,347,328,309,292,276,260,245,232,219,
        206,195,184,174,164,155,146,138,130,123,116,109,
        /* Tuning 6 */
        820,774,730,689,651,614,580,547,516,487,460,434,
        410,387,365,345,325,307,290,274,258,244,230,217,
        205,193,183,172,163,154,145,137,129,122,115,109,
        /* Tuning 7 */
        814,768,725,684,646,610,575,543,513,484,457,431,
        407,384,363,342,323,305,288,272,256,242,228,216,
        204,192,181,171,161,152,144,136,128,121,114,108,
        /* Tuning -8 */
        907,856,808,762,720,678,640,604,570,538,508,480,
        453,428,404,381,360,339,320,302,285,269,254,240,
        226,214,202,190,180,170,160,151,143,135,127,120,
        /* Tuning -7 */
        900,850,802,757,715,675,636,601,567,535,505,477,
        450,425,401,379,357,337,318,300,284,268,253,238,
        225,212,200,189,179,169,159,150,142,134,126,119,
        /* Tuning -6 */
        894,844,796,752,709,670,632,597,563,532,502,474,
        447,422,398,376,355,335,316,298,282,266,251,237,
        223,211,199,188,177,167,158,149,141,133,125,118,
        /* Tuning -5 */
        887,838,791,746,704,665,628,592,559,528,498,470,
        444,419,395,373,352,332,314,296,280,264,249,235,
        222,209,198,187,176,166,157,148,140,132,125,118,
        /* Tuning -4 */
        881,832,785,741,699,660,623,588,555,524,494,467,
        441,416,392,370,350,330,312,294,278,262,247,233,
        220,208,196,185,175,165,156,147,139,131,123,117,
        /* Tuning -3 */
        875,826,779,736,694,655,619,584,551,520,491,463,
        437,413,390,368,347,328,309,292,276,260,245,232,
        219,206,195,184,174,164,155,146,138,130,123,116,
        /* Tuning -2 */
        868,820,774,730,689,651,614,580,547,516,487,460,
        434,410,387,365,345,325,307,290,274,258,244,230,
        217,205,193,183,172,163,154,145,137,129,122,115,
        /* Tuning -1 */
        862,814,768,725,684,646,610,575,543,513,484,457,
        431,407,384,363,342,323,305,288,272,256,242,228,
        216,203,192,181,171,161,152,144,136,128,121,114,
    },
    { { .dmabit = 1 }, { .dmabit = 2 }, { .dmabit = 4 }, { .dmabit = 8 } },
    { 0 },
    0,
    6, 0, 0, 0, 0, 0,
    0,                                  /* mt_LowMask: BUG(orig), see above */
    0, 0, 0,
    { 0, 0 },
    0, 0, 0, 0,
    4000,
    0,
    -1, 15,
    0,
};

_Static_assert(sizeof(struct chan) == 44, "mt_chanNtemp");
_Static_assert(offsetof(struct ptdata, chan) == 1200, "mt_chan1temp");
_Static_assert(offsetof(struct ptdata, songdataptr) == 1504, "mt_SongDataPtr");
_Static_assert(offsetof(struct ptdata, enable) == 1517, "mt_Enable");
_Static_assert(offsetof(struct ptdata, patternpos) == 1520, "mt_PatternPos");
_Static_assert(offsetof(struct ptdata, dmawait) == 1526, "DMA_Wait");
_Static_assert(offsetof(struct ptdata, olddata) == 1534, "mt_olddata");

/* the variables of the glue code (ciatime ... mt_data) */
static struct ptglue {
    UWORD ciatime;                      /*  0 Dc.l, written as a word */
    UWORD ciatime_lo;
    ULONG deftime;                      /*  4 CIA clock, only set by the dead
                                           _ciaplayerinit: 0 */
    ULONG old;                          /*  8 unused (ptlib5's vector) */
    UWORD counter;                      /* 12 unused */
    UWORD pt_signal;                    /* 14 unused */
    ULONG pt_signo;                     /* 16 unused */
    UWORD pt_pos;                       /* 20 unused */
    UWORD realtempo;                    /* 22 RealTempo */
    UBYTE *mt_data;                     /* 24 the module of TrackInit */
} glue = { .realtempo = 125 };

_Static_assert(offsetof(struct ptglue, mt_data) == 24, "mt_data");

/* a word / longword of the period table at byte offset `off`, also behind
   the row or the table (BUG(orig): the lookups do not stop there) */
#define PERIOD(off) (*(UWORD *)((UBYTE *)&pt + offsetof(struct ptdata, periods) + (off)))
#define SAMPLESTART(off) (*(UBYTE **)((UBYTE *)&pt + offsetof(struct ptdata, samplestarts) + (off)))
#define W(p) (*(UWORD *)(p))

/* a0 of mt_GetNewNote, the pattern data; mt_end sets a0 to $DFF000, so
   after an F00 (mt_SetSpeed: BEQ mt_end) the voices behind read their
   notes from the custom chip registers (BUG(orig)) */
static UBYTE *patdata;

static void mt_end(void);
static void mt_music_c(void);
static void checkmoreefx(aud_t *hw, struct chan *c);

/* the DBRA loops on DMA_Wait: DMA_Wait + 1 passes */
static void dmawait(void)
{
    UWORD n = pt.dmawait;

    do
        __asm__ volatile ("");
    while (n-- != 0);
}

static UWORD cmdword(const struct chan *c)
{
    return (UWORD)(c->cmd << 8 | c->cmdlo);
}

/* ---------------------------------------------------------------- glue */

/* SetTempo: d0 = the tempo (>= 32 from mt_SetSpeed); the CIA value is
   deftime / tempo - deftime is 0 (BUG(orig)) */
static void settempo(UWORD d0)
{
    ULONG d2;

    if ((WORD)d0 <= 32)                 /* CMP.w #32,d0: BGT */
        d0 = 32;
    glue.realtempo = d0;
    d2 = glue.deftime;
    d2 = bl_divu_w(d2, d0);
    glue.ciatime = (UWORD)d2;
    CIABTALO = (UBYTE)d2;
    CIABTAHI = (UBYTE)((UWORD)d2 >> 8);
}

/* l6_Int, the interrupt code of the dead _ciaplayerinit */
static void BLITZ_ROUTINE l6_int(void)
{
    (void)CIABICR;                      /* TST.b $bfdd00: restart timers */
    mt_music_c();
}

/* the body of _ciaplayerinit behind its first RTS: never reached
   (BUG(orig)); kept, with its SetInt call that links intlib */
static void BLITZ_ROUTINE ciaplayerinit_unreached(void)
{
    UBYTE *eb = *(UBYTE **)4;          /* ExecBase */
    ULONG d7 = 1773447;                 /* PAL CIA time */

    if (eb[530] != 50)       /* VBlankFrequency */
        d7 = 1789773;                   /* NTSC */
    glue.deftime = d7;
    glue.ciatime = (UWORD)bl_divu_w(d7, 125);   /* the 50 Hz timer */
    {
        struct bl_regs r = { .d0 = 0x8043, .d1 = (ULONG)l6_int };
        BLITZ_ALIBJSR(TOK_setint, r);
    }
    CIABTALO = (UBYTE)glue.ciatime;
    CIABTAHI = (UBYTE)(glue.ciatime >> 8);
}

/* _ciaplayerinit: RTS */
static void ciaplayerinit(void)
{
}

/* _ciaplayerfinit (the ClrInt is commented out in the source) */
static void ciaplayerfinit(void)
{
    mt_end();
}

/* ------------------------------------------------------- the play routine */

/* mt_init: the sample starts behind the highest pattern, song position 0 */
static void mt_init(void)
{
    UBYTE *a0, *a1, *a2;
    UWORD d0;
    ULONG d1, d2;
    int i;

    if (!pt.status)
        return;
    a0 = glue.mt_data;
    pt.songdataptr = a0;
    a1 = a0 + 952;
    d0 = 127;
    d1 = 0;
mtloop:
    d2 = d1;
    d0--;
mtloop2:
    d1 = (d1 & ~0xffUL) | *a1++;
    if ((BYTE)d1 > (BYTE)d2)            /* BUG(orig): signed, see above */
        goto mtloop;
    if (d0-- != 0)
        goto mtloop2;
    d2 = (d2 & ~0xffUL) | (UBYTE)(d2 + 1);
    a2 = (UBYTE *)((d2 << 10) + 1084 + (ULONG)a0);
    for (i = 0; i < 31; i++) {
        *(ULONG *)a2 = 0;               /* BUG(orig): also behind the module */
        pt.samplestarts[i] = a2;
        a2 += (ULONG)W(a0 + 42) << 1;
        a0 += 30;
    }
    CIAAPRA |= 2;                       /* filter off */
    pt.speed = 6;
    pt.counter = 0;
    pt.songpos = 0;
    pt.patternpos = 0;
    mt_end();
}

/* mt_end: voices silent, audio DMA off */
static void mt_end(void)
{
    pt.enable = 0;
    patdata = (UBYTE *)0xdff000;      /* MOVE.l #$DFF000,A0 */
    CUSTOM.aud[0].ac_vol = 0;
    CUSTOM.aud[1].ac_vol = 0;
    CUSTOM.aud[2].ac_vol = 0;
    CUSTOM.aud[3].ac_vol = 0;
    CUSTOM.dmacon = 0xf;
}

static void mt_pernop(aud_t *hw, struct chan *c)
{
    hw->ac_per = c->period;
}

/* mt_UpdateFunk: invert loop (EFx) */
static void mt_updatefunk(struct chan *c)
{
    UBYTE d0 = c->glissfunk >> 4;
    UBYTE *a0;
    ULONG end;

    if (!d0)
        return;
    c->funkoffset += pt.funktable[d0];
    if (!(c->funkoffset & 0x80))
        return;
    c->funkoffset = 0;
    end = (ULONG)c->loopstart + 2 * (ULONG)c->replen;
    a0 = c->wavestart + 1;
    if (!((ULONG)a0 < end))
        a0 = c->loopstart;
    c->wavestart = a0;
    *a0 = (UBYTE)~*a0;
}

/* mt_Arpeggio (effect 0xy) */
static void mt_arpeggio(aud_t *hw, struct chan *c)
{
    UWORD d0, d2;
    ULONG a0;
    UWORD d1;
    int d3;

    d0 = (UWORD)(pt.counter % 3);       /* DIVS #3: SWAP */
    if (d0 == 0) {
        hw->ac_per = c->period;
        return;
    }
    if (d0 == 2)
        d0 = c->cmdlo & 15;
    else
        d0 = c->cmdlo >> 4;
    d0 <<= 1;
    a0 = (ULONG)c->finetune * 36 * 2;
    d1 = c->period;
    for (d3 = 36; d3 >= 0; d3--) {      /* BUG(orig): 37 entries */
        d2 = PERIOD(a0 + d0);
        if (d1 >= PERIOD(a0)) {
            hw->ac_per = d2;
            return;
        }
        a0 += 2;
    }
}

/* nor_TrackEvent (effect 8xx, RI's): BUG(orig), nothing reads it */
static void nor_trackevent(struct chan *c)
{
    pt.trkevt = c->cmdlo;
}

/* mt_PortaUp (1xx), mt_FinePortaUp (E1x) */
static void mt_portaup(aud_t *hw, struct chan *c)
{
    UWORD d0 = c->cmdlo & pt.lowmask;

    pt.lowmask = 0xff;
    c->period -= d0;
    if ((WORD)(c->period & 0x0fff) < 113)
        c->period = (c->period & 0xf000) | 113;
    hw->ac_per = c->period & 0x0fff;
}

static void mt_fineportaup(aud_t *hw, struct chan *c)
{
    if (pt.counter)
        return;
    pt.lowmask = 0x0f;
    mt_portaup(hw, c);
}

/* mt_PortaDown (2xx), mt_FinePortaDown (E2x) */
static void mt_portadown(aud_t *hw, struct chan *c)
{
    UWORD d0 = c->cmdlo & pt.lowmask;

    pt.lowmask = 0xff;
    c->period += d0;
    if ((WORD)(c->period & 0x0fff) >= 856)
        c->period = (c->period & 0xf000) | 856;
    hw->ac_per = c->period & 0x0fff;
}

static void mt_fineportadown(aud_t *hw, struct chan *c)
{
    if (pt.counter)
        return;
    pt.lowmask = 0x0f;
    mt_portadown(hw, c);
}

/* mt_SetTonePorta: the wanted period of the note (3xx/5xx with a note) */
static void mt_settoneporta(struct chan *c)
{
    UWORD d2 = c->note & 0x0fff, d0;
    ULONG a0 = (ULONG)c->finetune * 36 * 2;

    for (d0 = 0; ; ) {
        if (d2 >= PERIOD(a0 + d0))
            break;
        d0 += 2;
        if (d0 >= 36 * 2) {
            d0 = 35 * 2;
            break;
        }
    }
    if ((c->finetune & 8) && d0 != 0)
        d0 -= 2;
    d2 = PERIOD(a0 + d0);
    c->wantedperiod = d2;
    d0 = c->period;
    c->toneportdirec = 0;
    if (d2 == d0) {
        c->wantedperiod = 0;
        return;
    }
    if ((WORD)d2 >= (WORD)d0)
        return;
    c->toneportdirec = 1;
}

/* mt_TonePortNoChange: slide to the wanted period (glissando: rounded to
   the table) */
static void mt_toneportnochange(aud_t *hw, struct chan *c)
{
    UWORD d0, d2;

    if (!c->wantedperiod)
        return;
    d0 = c->toneportspeed;
    if (!c->toneportdirec) {
        c->period += d0;
        if ((WORD)c->wantedperiod <= (WORD)c->period) {
            c->period = c->wantedperiod;
            c->wantedperiod = 0;
        }
    } else {
        c->period -= d0;
        if ((WORD)c->wantedperiod >= (WORD)c->period) {
            c->period = c->wantedperiod;
            c->wantedperiod = 0;
        }
    }
    d2 = c->period;
    if (c->glissfunk & 0x0f) {
        ULONG a0 = (ULONG)c->finetune * 36 * 2;
        for (d0 = 0; ; ) {
            if (d2 >= PERIOD(a0 + d0))
                break;
            d0 += 2;
            if (d0 >= 36 * 2) {
                d0 = 35 * 2;
                break;
            }
        }
        d2 = PERIOD(a0 + d0);
    }
    hw->ac_per = d2;
}

/* mt_TonePortamento (3xx) */
static void mt_toneportamento(aud_t *hw, struct chan *c)
{
    if (c->cmdlo) {
        c->toneportspeed = c->cmdlo;
        c->cmdlo = 0;
    }
    mt_toneportnochange(hw, c);
}

/* mt_Vibrato2: the vibrato step (4xy, 6xy) */
static void mt_vibrato2(aud_t *hw, struct chan *c)
{
    UBYTE d0 = (c->vibratopos >> 2) & 0x1f;
    UWORD d2, per;

    switch (c->wavecontrol & 3) {
    case 0:
        d2 = pt.vibtable[d0];
        break;
    case 1:                             /* ramp down */
        d0 = (UBYTE)(d0 << 3);
        if ((BYTE)c->vibratopos < 0)
            d2 = (UBYTE)(255 - d0);
        else
            d2 = d0;
        break;
    default:                            /* square */
        d2 = 255;
        break;
    }
    d2 = (UWORD)(d2 * (c->vibratocmd & 15)) >> 7;
    per = c->period;
    if ((BYTE)c->vibratopos < 0)
        per -= d2;
    else
        per += d2;
    hw->ac_per = per;
    c->vibratopos += (c->vibratocmd >> 2) & 0x3c;
}

/* mt_Vibrato (4xy) */
static void mt_vibrato(aud_t *hw, struct chan *c)
{
    UBYTE d0 = c->cmdlo, d2;

    if (d0) {
        d2 = c->vibratocmd;
        if (d0 & 0x0f)
            d2 = (d2 & 0xf0) | (d0 & 0x0f);
        if (c->cmdlo & 0xf0)
            d2 = (d2 & 0x0f) | (c->cmdlo & 0xf0);
        c->vibratocmd = d2;
    }
    mt_vibrato2(hw, c);
}

/* mt_Tremolo (7xy) */
static void mt_tremolo(aud_t *hw, struct chan *c)
{
    UBYTE d0 = c->cmdlo, b;
    UWORD d2, vol;

    if (d0) {
        b = c->tremolocmd;
        if (d0 & 0x0f)
            b = (b & 0xf0) | (d0 & 0x0f);
        if (c->cmdlo & 0xf0)
            b = (b & 0x0f) | (c->cmdlo & 0xf0);
        c->tremolocmd = b;
    }
    d0 = (c->tremolopos >> 2) & 0x1f;
    switch ((c->wavecontrol >> 4) & 3) {
    case 0:
        d2 = pt.vibtable[d0];
        break;
    case 1:                             /* ramp down */
        d0 = (UBYTE)(d0 << 3);
        if ((BYTE)c->vibratopos < 0)    /* BUG(orig): the vibrato position */
            d2 = (UBYTE)(255 - d0);
        else
            d2 = d0;
        break;
    default:
        d2 = 255;
        break;
    }
    d2 = (UWORD)(d2 * (c->tremolocmd & 15)) >> 6;
    vol = c->volume;
    if ((BYTE)c->tremolopos < 0)
        vol -= d2;
    else
        vol += d2;
    if ((WORD)vol < 0)
        vol = 0;
    if (vol > 0x40)
        vol = 0x40;
    hw->ac_vol = vol;
    c->tremolopos += (c->tremolocmd >> 2) & 0x3c;
}

/* mt_SampleOffset (9xx) */
static void mt_sampleoffset(struct chan *c)
{
    UWORD d0;

    if (c->cmdlo)
        c->sampleoffset = c->cmdlo;
    d0 = (UWORD)(c->sampleoffset << 7);
    if ((WORD)d0 >= (WORD)c->length) {  /* BUG(orig): signed */
        c->length = 1;
        return;
    }
    c->length -= d0;
    c->start += (UWORD)(d0 << 1);
}

/* mt_VolSlideUp / mt_VolSlideDown2 (Axy, EAx, EBx) */
static void mt_volslideup(aud_t *hw, struct chan *c, UBYTE d0)
{
    c->volume += d0;
    if ((BYTE)(UBYTE)(c->volume - 0x40) >= 0)   /* CMP.B #$40: BMI */
        c->volume = 0x40;
    hw->ac_vol = c->volume;
}

static void mt_volslidedown2(aud_t *hw, struct chan *c, UBYTE d0)
{
    c->volume -= d0;
    if ((BYTE)c->volume < 0)
        c->volume = 0;
    hw->ac_vol = c->volume;
}

static void mt_volumeslide(aud_t *hw, struct chan *c)
{
    UBYTE d0 = c->cmdlo >> 4;

    if (d0)
        mt_volslideup(hw, c, d0);
    else
        mt_volslidedown2(hw, c, c->cmdlo & 0x0f);
}

/* mt_PositionJump (Bxx), mt_PatternBreak (Dxx) */
static void mt_pj2(void)
{
    pt.pbreakpos = 0;
    pt.posjumpflag = 0xff;
}

static void mt_positionjump(struct chan *c)
{
    pt.songpos = (UBYTE)(c->cmdlo - 1);
    mt_pj2();
}

static void mt_patternbreak(struct chan *c)
{
    UBYTE d0 = (UBYTE)((c->cmdlo >> 4) * 10 + (c->cmdlo & 0x0f));

    if (d0 > 63) {
        mt_pj2();
        return;
    }
    pt.pbreakpos = d0;
    pt.posjumpflag = 0xff;
}

/* mt_VolumeChange (Cxx) */
static void mt_volumechange(aud_t *hw, struct chan *c)
{
    UBYTE d0 = c->cmdlo;

    if (d0 > 0x40)
        d0 = 0x40;
    c->volume = d0;
    hw->ac_vol = d0;
}

/* mt_SetSpeed (Fxx): 0 stops (mt_end), >= 32 signed the tempo
   (BUG(orig): F80-FFF are speeds) */
static void mt_setspeed(struct chan *c)
{
    UBYTE d0 = c->cmdlo;

    if (!d0) {
        mt_end();
        return;
    }
    if ((BYTE)d0 >= 32) {
        settempo(d0);
        return;
    }
    pt.counter = 0;
    pt.speed = d0;
}

/* mt_DoRetrig: restart the sample (E9x, EDx) */
static void mt_doretrig(aud_t *hw, struct chan *c)
{
    CUSTOM.dmacon = c->dmabit;          /* voice DMA off */
    hw->ac_ptr = (UWORD *)c->start;
    hw->ac_len = c->length;
    dmawait();
    CUSTOM.dmacon = c->dmabit | 0x8000;
    dmawait();
    hw->ac_ptr = (UWORD *)c->loopstart;
    hw->ac_len = c->replen;             /* MOVE.L n_replen: length, period */
    hw->ac_per = c->period;
}

/* the E commands (Exy) */
static void mt_e_commands(aud_t *hw, struct chan *c)
{
    UBYTE x = c->cmdlo & 0x0f, d0;

    switch (c->cmdlo >> 4) {
    case 0x0:                           /* mt_FilterOnOff */
        d0 = (UBYTE)((c->cmdlo & 1) << 1);
        CIAAPRA &= 0xfd;
        CIAAPRA |= d0;
        break;
    case 0x1:
        mt_fineportaup(hw, c);
        break;
    case 0x2:
        mt_fineportadown(hw, c);
        break;
    case 0x3:                           /* mt_SetGlissControl */
        c->glissfunk = (c->glissfunk & 0xf0) | x;
        break;
    case 0x4:                           /* mt_SetVibratoControl */
        c->wavecontrol = (c->wavecontrol & 0xf0) | x;
        break;
    case 0x5:                           /* mt_SetFineTune */
        c->finetune = x;
        break;
    case 0x6:                           /* mt_JumpLoop */
        if (pt.counter)
            break;
        if (!x) {                       /* mt_SetLoop */
            c->pattpos = (UBYTE)(pt.patternpos >> 4);
            break;
        }
        if (c->loopcount) {
            if (--c->loopcount == 0)
                break;
        } else
            c->loopcount = x;
        pt.pbreakpos = c->pattpos;
        pt.pbreakflag = 0xff;
        break;
    case 0x7:                           /* mt_SetTremoloControl */
        c->wavecontrol = (c->wavecontrol & 0x0f) | (UBYTE)(x << 4);
        break;
    case 0x9: {                         /* mt_RetrigNote */
        UBYTE d1;

        if (!x)
            break;
        d1 = pt.counter;
        if (!d1 && (c->note & 0x0fff))
            break;
        if (d1 % x)
            break;
        mt_doretrig(hw, c);
        break;
    }
    case 0xa:                           /* mt_VolumeFineUp */
        if (!pt.counter)
            mt_volslideup(hw, c, x);
        break;
    case 0xb:                           /* mt_VolumeFineDown */
        if (!pt.counter)
            mt_volslidedown2(hw, c, x);
        break;
    case 0xc:                           /* mt_NoteCut */
        if (x != pt.counter)
            break;
        c->volume = 0;
        hw->ac_vol = 0;
        break;
    case 0xd:                           /* mt_NoteDelay */
        if (x != pt.counter || !c->note)
            break;
        mt_doretrig(hw, c);
        break;
    case 0xe:                           /* mt_PatternDelay */
        if (pt.counter || pt.pattdeltime2)
            break;
        pt.pattdeltime = (UBYTE)(x + 1);
        break;
    case 0xf:                           /* mt_FunkIt */
        if (pt.counter)
            break;
        d0 = (UBYTE)(x << 4);
        c->glissfunk = (c->glissfunk & 0x0f) | d0;
        if (d0)
            mt_updatefunk(c);
        break;
    default:                            /* E8x: nothing */
        break;
    }
}

/* mt_CheckEfx: the effects on the ticks between the rows */
static void mt_checkefx(aud_t *hw, struct chan *c)
{
    mt_updatefunk(c);
    if (!(cmdword(c) & 0x0fff)) {
        mt_pernop(hw, c);
        return;
    }
    switch (c->cmd & 0x0f) {
    case 0x0:
        mt_arpeggio(hw, c);
        return;
    case 0x1:
        mt_portaup(hw, c);
        return;
    case 0x2:
        mt_portadown(hw, c);
        return;
    case 0x3:
        mt_toneportamento(hw, c);
        return;
    case 0x4:
        mt_vibrato(hw, c);
        return;
    case 0x5:
        mt_toneportnochange(hw, c);
        mt_volumeslide(hw, c);
        return;
    case 0x6:
        mt_vibrato2(hw, c);
        mt_volumeslide(hw, c);
        return;
    case 0xe:
        mt_e_commands(hw, c);
        return;
    }
    hw->ac_per = c->period;             /* SetBack */
    switch (c->cmd & 0x0f) {
    case 0x7:
        mt_tremolo(hw, c);
        break;
    case 0xa:
        mt_volumeslide(hw, c);
        break;
    case 0x8:
        nor_trackevent(c);
        break;
    }
}

/* mt_CheckMoreEfx: the effects of a new row */
static void checkmoreefx(aud_t *hw, struct chan *c)
{
    mt_updatefunk(c);
    switch (c->cmd & 0x0f) {
    case 0x9:
        mt_sampleoffset(c);
        break;
    case 0xb:
        mt_positionjump(c);
        break;
    case 0xd:
        mt_patternbreak(c);
        break;
    case 0xe:
        mt_e_commands(hw, c);
        break;
    case 0xf:
        mt_setspeed(c);
        break;
    case 0xc:
        mt_volumechange(hw, c);
        break;
    default:
        mt_pernop(hw, c);
        break;
    }
}

/* mt_SetPeriod: the period of the note for the finetune, start the
   sample (not with EDx) */
static void mt_setperiod(aud_t *hw, struct chan *c)
{
    UWORD d1 = c->note & 0x0fff, d0 = 0;
    int d2;

    for (d2 = 36; d2 >= 0; d2--) {      /* not found: d0 = 74, BUG(orig) */
        if (d1 >= PERIOD(d0))
            break;
        d0 += 2;
    }
    c->period = PERIOD((ULONG)c->finetune * 36 * 2 + d0);
    if ((cmdword(c) & 0x0ff0) == 0x0ed0) {
        checkmoreefx(hw, c);
        return;
    }
    CUSTOM.dmacon = c->dmabit;
    if (!(c->wavecontrol & 4))
        c->vibratopos = 0;
    if (!(c->wavecontrol & 0x40))
        c->tremolopos = 0;
    hw->ac_ptr = (UWORD *)c->start;
    hw->ac_len = c->length;
    hw->ac_per = c->period;
    pt.dmacontemp |= c->dmabit;
    checkmoreefx(hw, c);
}

/* mt_PlayVoice: the next note of the pattern at patdata + *d1 */
static void mt_playvoice(aud_t *hw, struct chan *c, ULONG *d1, UBYTE *a3)
{
    ULONG v;
    ULONG d2;
    UWORD d3, d4w;
    ULONG d4;

    if (!c->note && !c->cmd && !c->cmdlo)
        mt_pernop(hw, c);
    v = *(volatile ULONG *)(patdata + *d1);    /* MOVE.L: one access */
    c->note = (UWORD)(v >> 16);
    c->cmd = (UBYTE)(v >> 8);
    c->cmdlo = (UBYTE)v;
    *d1 += 4;
    d2 = (UBYTE)((c->cmd & 0xf0) >> 4 | (UBYTE)(c->note >> 8 & 0xf0));
    if (d2) {
        d4w = (UWORD)d2;
        d2 = (d2 - 1) << 2;
        d4 = (ULONG)d4w * 30;
        c->start = SAMPLESTART(d2);     /* BUG(orig): sample > 31 */
        c->length = W(a3 + d4);
        c->reallength = W(a3 + d4);
        c->finetune = a3[d4 + 2];
        c->volume = a3[d4 + 3];
        d3 = W(a3 + d4 + 4);            /* repeat */
        if (d3) {
            d2 = (ULONG)c->start + (UWORD)(d3 << 1);    /* BUG(orig): word */
            c->loopstart = (UBYTE *)d2;
            c->wavestart = (UBYTE *)d2;
            c->length = (UWORD)(W(a3 + d4 + 4) + W(a3 + d4 + 6));
            c->replen = W(a3 + d4 + 6);
        } else {
            c->loopstart = c->start;
            c->wavestart = c->start;
            c->replen = W(a3 + d4 + 6);
        }
        hw->ac_vol = c->volume;
    }
    /* mt_SetRegs */
    if (!(c->note & 0x0fff)) {
        checkmoreefx(hw, c);
        return;
    }
    if ((cmdword(c) & 0x0ff0) == 0x0e50) {
        c->finetune = c->cmdlo & 0x0f;  /* mt_SetFineTune */
        mt_setperiod(hw, c);
        return;
    }
    switch (c->cmd & 0x0f) {
    case 3:
    case 5:
        mt_settoneporta(c);
        checkmoreefx(hw, c);
        return;
    case 9:
        checkmoreefx(hw, c);            /* BUG(orig): 9xx twice */
        break;
    }
    mt_setperiod(hw, c);
}

/* mt_music: one tick */
static void mt_music_c(void)
{
    int i;

    if (!pt.enable || !pt.status)
        return;
    pt.counter++;
    if (pt.counter < pt.speed) {
        for (i = 0; i < 4; i++)
            mt_checkefx(&CUSTOM.aud[i], &pt.chan[i]);
        goto nonewposyet;
    }
    pt.counter = 0;
    if (pt.pattdeltime2) {
        for (i = 0; i < 4; i++)
            mt_checkefx(&CUSTOM.aud[i], &pt.chan[i]);
    } else {
        /* mt_GetNewNote */
        UBYTE *a0 = pt.songdataptr, *a3 = a0 + 12, *a2 = a0 + 952;
        ULONG d1;

        patdata = a0 + 1084;
        d1 = (ULONG)a2[pt.songpos] << 10;
        d1 = (d1 & 0xffff0000UL) | (UWORD)((UWORD)d1 + pt.patternpos);
        pt.dmacontemp = 0;
        /* BUG(orig): a masked voice does not advance d1 */
        for (i = 0; i < 4; i++)
            if (pt.mask & (8 >> i))
                mt_playvoice(&CUSTOM.aud[i], &pt.chan[i], &d1, a3);
        /* mt_SetDMA */
        dmawait();
        CUSTOM.dmacon = pt.dmacontemp | 0x8000;
        dmawait();
        for (i = 3; i >= 0; i--) {
            CUSTOM.aud[i].ac_ptr = (UWORD *)pt.chan[i].loopstart;
            CUSTOM.aud[i].ac_len = pt.chan[i].replen;
        }
    }
    /* mt_dskip */
    pt.patternpos += 16;
    if (pt.pattdeltime) {
        pt.pattdeltime2 = pt.pattdeltime;
        pt.pattdeltime = 0;
    }
    if (pt.pattdeltime2 && --pt.pattdeltime2 != 0)
        pt.patternpos -= 16;
    if (pt.pbreakflag) {
        pt.pbreakflag = 0;
        pt.patternpos = (UWORD)(pt.pbreakpos << 4);
        pt.pbreakpos = 0;
    }
    if (pt.patternpos < 1024)
        goto nonewposyet;
nextposition:
    pt.patternpos = (UWORD)(pt.pbreakpos << 4);
    pt.pbreakpos = 0;
    pt.posjumpflag = 0;
    pt.songpos = (pt.songpos + 1) & 0x7f;
    if (!(pt.songpos < pt.songdataptr[950]))
        pt.songpos = 0;
nonewposyet:
    if (pt.posjumpflag)
        goto nextposition;
}

/* --------------------------------------------------------------- commands */

static void BLITZ_ROUTINE libinit_c(void)
{
    pt.playing = 0;
}

static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!pt.playing)
        return;
    pt.playing = 0;
    ciaplayerfinit();
}

/* Tracker on+off */
static void BLITZ_ROUTINE tracker_c(UBYTE d0 BL_REG(d0))
{
    if ((BYTE)d0 == pt.status)
        return;
    pt.enable &= d0;                    /* BUG(orig): On does not resume */
    pt.status = (BYTE)d0;
    if (d0)
        ciaplayerinit();
    else
        ciaplayerfinit();
}

/* TrackInit address */
static void BLITZ_ROUTINE initmodule_c(UBYTE *d0 BL_REG(d0))
{
    glue.mt_data = d0;
    ciaplayerinit();
    pt.playing = 1;
    mt_init();                          /* BUG(orig): nothing if Tracker Off */
    pt.enable = 0xff;
}

static void BLITZ_ROUTINE tracksetdma_c(UWORD d0 BL_REG(d0))
{
    pt.dmawait = d0;
}

static void BLITZ_ROUTINE stopmodule_c(void)
{
    pt.olddata = glue.mt_data;
    mt_end();
}

/* TrackerPause On|Off: nonzero stops, 0 continues (_contmodule) */
static void BLITZ_ROUTINE trackerpause_c(UBYTE d0 BL_REG(d0))
{
    if (d0) {
        ciaplayerfinit();
        return;
    }
    ciaplayerinit();
    pt.enable = 0xff;
    mt_music_c();
}

/* _cut: a voice's pointer, length, period and volume 0 (DMA stays) */
static void cut(aud_t *hw)
{
    hw->ac_ptr = 0;
    hw->ac_len = 0;
    hw->ac_per = 0;
    hw->ac_vol = 0;
}

/* TrackerChannels mask */
static void BLITZ_ROUTINE trackermask_c(UBYTE d0 BL_REG(d0))
{
    int i;

    pt.mask = d0;
    for (i = 0; i < 4; i++)
        if (!(d0 & (8 >> i)))
            cut(&CUSTOM.aud[i]);
}

static void BLITZ_ROUTINE music_c(void)
{
    mt_music_c();
}

/* Registers: the original keeps all in mt_music, changes d0-d2/a0-a2 in
   TrackInit, a0 in TrackStop and TrackerChannels; every thunk keeps
   d0/d1/a0/a1 (no results), GCC keeps the others. */
BLITZ_THUNK(_initmodule, initmodule_c, "d0-d1/a0-a1");
BLITZ_THUNK(mt_music, music_c, "d0-d1/a0-a1");
BLITZ_THUNK(_stopmodule, stopmodule_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackSetDMA, tracksetdma_c, "d0-d1/a0-a1");
BLITZ_THUNK(_tracker, tracker_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackermask, trackermask_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackerpause, trackerpause_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
