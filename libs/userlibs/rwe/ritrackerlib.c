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
 * table into the voices, see BUG(orig), #481 #474) and the variables of the glue
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
 *     ciatime (#571).
 *   - Fxx compares signed (CMP.B #32: BGE): F80-FFF set the speed to
 *     $80-$FF instead of the tempo (#569).
 *   - TrackerChannels: a masked voice does not advance the pattern
 *     offset, so the following voices play the notes of the voice
 *     before (#568); effects (mt_CheckEfx) and the loop pointers of
 *     mt_SetDMA still go to all four voices (#567).
 *   - Tracker Off clears mt_Enable (mt_end); Tracker On only ANDs it, so
 *     the song stays stopped until TrackInit or TrackerPause 0 (#572).
 *   - TrackInit while Tracker Off does not initialise the song (mt_init
 *     returns) but enables playing; Tracker On then plays with the old
 *     song pointer (#573).
 *   - F00 (stop: mt_SetSpeed branches to mt_end, which loads a0 with
 *     $DFF000) on voice 1-3: the voices behind read their notes from the
 *     custom chip registers instead of the pattern (#475).
 * Not a defect: effect 8xx (RI's track event) stores its value in
 * nor_TrkEvt on the ticks after the first, which no command reads (an
 * unused variable).
 * BUG(orig) of the play routine (the known PT2.3A bugs, cf. the changelog
 * of 8bitbubsy's pt23f replayer):
 *   - mt_init: the pattern count scan (signed compare; the scan ends
 *     only through DBRA, a new maximum at order position 126 lets it run
 *     on through the pattern data) (#477); the CLR.L of the first
 *     longword of every sample also clears 4 bytes behind the module for
 *     empty samples at its end (#570).
 *   - mt_LowMask starts at 0: the first 1xx/2xx before any E1x/E2x does
 *     not slide (#478).
 *   - The period lookups (mt_SetPeriod, mt_Arpeggio, mt_SetTonePorta,
 *     glissando) read behind the row of the finetune, and for finetune
 *     -1 behind the table into the voice data, when no entry fits (#481,
 *     #474); a finetune byte above 15 from the sample header reads far
 *     behind (#476).
 *   - Tremolo's ramp-down waveform tests the vibrato position (#482).
 *   - 9xx: compared signed with the length (samples of 64 KB and more
 *     get length 1) (#472); with a note it is applied twice
 *     (mt_CheckMoreEfx before and after mt_SetPeriod) (#473); loop
 *     offsets are shifted as words (repeat >= 32768 words wraps) (#479).
 *   - Sample numbers above 31 read behind mt_SampleStarts and the sample
 *     headers (#480).
 */
#include "blitzlib.h"
#include <stddef.h>
#include <hardware/custom.h>
#include <exec/execbase.h>

#define CUSTOM  (*(volatile struct Custom *)0xdff000)
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define CIABTALO (*(volatile UBYTE *)0xbfd400)
#define CIABTAHI (*(volatile UBYTE *)0xbfd500)
#define CIABICR (*(volatile UBYTE *)0xbfdd00)

typedef volatile struct AudChannel aud_t;

/* entries (thunks below) */
void _initmodule(void), ritrackerlib_mt_music(void), _stopmodule(void);
void _trackSetDMA(void), _tracker(void), _trackermask(void);
void _trackerpause(void), ritrackerlib_libinit(void);
void ritrackerlib_libfinit(void);

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
BLITZ_SUBS(ritrackerlib_mt_music, 0, 0);
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

BLITZ_NULLSUB_AT(finit, ritrackerlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ritrackerlib_libinit, 0, 0);

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
    0,                                  /* mt_LowMask: BUG(orig), see above (#478) */
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
   the row or the table (BUG(orig): the lookups do not stop there, #481
   #474) */
#define PERIOD(off) (*(UWORD *)((UBYTE *)&pt + offsetof(struct ptdata, periods) + (off)))
#define SAMPLESTART(off) (*(UBYTE **)((UBYTE *)&pt + offsetof(struct ptdata, samplestarts) + (off)))
#define W(p) (*(UWORD *)(p))

/* a0 of mt_GetNewNote, the pattern data; mt_end sets a0 to $DFF000, so
   after an F00 (mt_SetSpeed: BEQ mt_end) the voices behind read their
   notes from the custom chip registers (BUG(orig), #475) */
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
   deftime / tempo - deftime is 0 (BUG(orig), #571) */
static void settempo(UWORD tempo)
{
    ULONG timer;

    if ((WORD)tempo <= 32)              /* CMP.w #32,d0: BGT */
        tempo = 32;
    glue.realtempo = tempo;
    timer = glue.deftime;
    timer = bl_divu_w(timer, tempo);
    glue.ciatime = (UWORD)timer;
    CIABTALO = (UBYTE)timer;
    CIABTAHI = (UBYTE)((UWORD)timer >> 8);
}

/* l6_Int, the interrupt code of the dead _ciaplayerinit */
static void BLITZ_ROUTINE l6_int(void)
{
    (void)CIABICR;                      /* TST.b $bfdd00: restart timers */
    mt_music_c();
}

/* the body of _ciaplayerinit behind its first RTS: never reached
   (BUG(orig), #571); kept, with its SetInt call that links intlib */
static void BLITZ_ROUTINE ciaplayerinit_unreached(void)
{
    struct ExecBase *eb = *(struct ExecBase **)4;
    ULONG clock = 1773447;              /* PAL CIA time */

    if (eb->VBlankFrequency != 50)      /* 530(a6) */
        clock = 1789773;                /* NTSC */
    glue.deftime = clock;
    glue.ciatime = (UWORD)bl_divu_w(clock, 125);   /* the 50 Hz timer */
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
    UBYTE *mod, *order, *sample;
    UWORD count;
    ULONG pattern, highest;
    int i;

    if (!pt.status)
        return;
    mod = glue.mt_data;
    pt.songdataptr = mod;
    order = mod + 952;
    count = 127;
    pattern = 0;
newhighest:                             /* mtloop */
    highest = pattern;
    count--;
nextentry:                              /* mtloop2 */
    pattern = (pattern & ~0xffUL) | *order++;
    if ((BYTE)pattern > (BYTE)highest)  /* BUG(orig): signed, see above (#477) */
        goto newhighest;
    if (count-- != 0)
        goto nextentry;
    highest = (highest & ~0xffUL) | (UBYTE)(highest + 1);
    sample = (UBYTE *)((highest << 10) + 1084 + (ULONG)mod);
    for (i = 0; i < 31; i++) {
        *(ULONG *)sample = 0;           /* BUG(orig): also behind the module (#570) */
        pt.samplestarts[i] = sample;
        sample += (ULONG)W(mod + 42) << 1;
        mod += 30;
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
    UBYTE speed = c->glissfunk >> 4;
    UBYTE *wave;
    ULONG end;

    if (!speed)
        return;
    c->funkoffset += pt.funktable[speed];
    if (!(c->funkoffset & 0x80))
        return;
    c->funkoffset = 0;
    end = (ULONG)c->loopstart + 2 * (ULONG)c->replen;
    wave = c->wavestart + 1;
    if (!((ULONG)wave < end))
        wave = c->loopstart;
    c->wavestart = wave;
    *wave = (UBYTE)~*wave;
}

/* mt_Arpeggio (effect 0xy) */
static void mt_arpeggio(aud_t *hw, struct chan *c)
{
    UWORD tick, step, arpperiod;        /* d0: counter % 3; d0: the note step; d2 */
    ULONG row;
    UWORD period;
    int left;

    tick = (UWORD)(pt.counter % 3);     /* DIVS #3: SWAP */
    if (tick == 0) {
        hw->ac_per = c->period;
        return;
    }
    if (tick == 2)
        step = c->cmdlo & 15;
    else
        step = c->cmdlo >> 4;
    step <<= 1;
    row = (ULONG)c->finetune * 36 * 2;
    period = c->period;
    for (left = 36; left >= 0; left--) {  /* BUG(orig): 37 entries (#474) */
        arpperiod = PERIOD(row + step);
        if (period >= PERIOD(row)) {
            hw->ac_per = arpperiod;
            return;
        }
        row += 2;
    }
}

/* nor_TrackEvent (effect 8xx, RI's): nothing reads nor_TrkEvt (an unused
   variable, no defect) */
static void nor_trackevent(struct chan *c)
{
    pt.trkevt = c->cmdlo;
}

/* mt_PortaUp (1xx), mt_FinePortaUp (E1x) */
static void mt_portaup(aud_t *hw, struct chan *c)
{
    UWORD amount = c->cmdlo & pt.lowmask;

    pt.lowmask = 0xff;
    c->period -= amount;
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
    UWORD amount = c->cmdlo & pt.lowmask;

    pt.lowmask = 0xff;
    c->period += amount;
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
    UWORD target = c->note & 0x0fff;    /* d2 */
    UWORD off, period;                  /* d0: the table offset; d0: the current period */
    ULONG row = (ULONG)c->finetune * 36 * 2;

    for (off = 0; ; ) {
        if (target >= PERIOD(row + off))
            break;
        off += 2;
        if (off >= 36 * 2) {
            off = 35 * 2;
            break;
        }
    }
    if ((c->finetune & 8) && off != 0)
        off -= 2;
    target = PERIOD(row + off);
    c->wantedperiod = target;
    period = c->period;
    c->toneportdirec = 0;
    if (target == period) {
        c->wantedperiod = 0;
        return;
    }
    if ((WORD)target >= (WORD)period)
        return;
    c->toneportdirec = 1;
}

/* mt_TonePortNoChange: slide to the wanted period (glissando: rounded to
   the table) */
static void mt_toneportnochange(aud_t *hw, struct chan *c)
{
    UWORD speed, off, per;              /* d0: the speed; d0: the table offset; d2 */

    if (!c->wantedperiod)
        return;
    speed = c->toneportspeed;
    if (!c->toneportdirec) {
        c->period += speed;
        if ((WORD)c->wantedperiod <= (WORD)c->period) {
            c->period = c->wantedperiod;
            c->wantedperiod = 0;
        }
    } else {
        c->period -= speed;
        if ((WORD)c->wantedperiod >= (WORD)c->period) {
            c->period = c->wantedperiod;
            c->wantedperiod = 0;
        }
    }
    per = c->period;
    if (c->glissfunk & 0x0f) {
        ULONG row = (ULONG)c->finetune * 36 * 2;
        for (off = 0; ; ) {
            if (per >= PERIOD(row + off))
                break;
            off += 2;
            if (off >= 36 * 2) {
                off = 35 * 2;
                break;
            }
        }
        per = PERIOD(row + off);
    }
    hw->ac_per = per;
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
    UBYTE pos = (c->vibratopos >> 2) & 0x1f;
    UWORD delta, per;

    switch (c->wavecontrol & 3) {
    case 0:
        delta = pt.vibtable[pos];
        break;
    case 1:                             /* ramp down */
        pos = (UBYTE)(pos << 3);
        if ((BYTE)c->vibratopos < 0)
            delta = (UBYTE)(255 - pos);
        else
            delta = pos;
        break;
    default:                            /* square */
        delta = 255;
        break;
    }
    delta = (UWORD)(delta * (c->vibratocmd & 15)) >> 7;
    per = c->period;
    if ((BYTE)c->vibratopos < 0)
        per -= delta;
    else
        per += delta;
    hw->ac_per = per;
    c->vibratopos += (c->vibratocmd >> 2) & 0x3c;
}

/* mt_Vibrato (4xy) */
static void mt_vibrato(aud_t *hw, struct chan *c)
{
    UBYTE arg = c->cmdlo, cmd;

    if (arg) {
        cmd = c->vibratocmd;
        if (arg & 0x0f)
            cmd = (cmd & 0xf0) | (arg & 0x0f);
        if (c->cmdlo & 0xf0)
            cmd = (cmd & 0x0f) | (c->cmdlo & 0xf0);
        c->vibratocmd = cmd;
    }
    mt_vibrato2(hw, c);
}

/* mt_Tremolo (7xy) */
static void mt_tremolo(aud_t *hw, struct chan *c)
{
    UBYTE arg = c->cmdlo, b;            /* d0: the argument */
    UBYTE pos;                          /* d0: the table position */
    UWORD delta, vol;

    if (arg) {
        b = c->tremolocmd;
        if (arg & 0x0f)
            b = (b & 0xf0) | (arg & 0x0f);
        if (c->cmdlo & 0xf0)
            b = (b & 0x0f) | (c->cmdlo & 0xf0);
        c->tremolocmd = b;
    }
    pos = (c->tremolopos >> 2) & 0x1f;
    switch ((c->wavecontrol >> 4) & 3) {
    case 0:
        delta = pt.vibtable[pos];
        break;
    case 1:                             /* ramp down */
        pos = (UBYTE)(pos << 3);
        if ((BYTE)c->vibratopos < 0)    /* BUG(orig): the vibrato position (#482) */
            delta = (UBYTE)(255 - pos);
        else
            delta = pos;
        break;
    default:
        delta = 255;
        break;
    }
    delta = (UWORD)(delta * (c->tremolocmd & 15)) >> 6;
    vol = c->volume;
    if ((BYTE)c->tremolopos < 0)
        vol -= delta;
    else
        vol += delta;
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
    UWORD offset;

    if (c->cmdlo)
        c->sampleoffset = c->cmdlo;
    offset = (UWORD)(c->sampleoffset << 7);
    if ((WORD)offset >= (WORD)c->length) {  /* BUG(orig): signed (#472) */
        c->length = 1;
        return;
    }
    c->length -= offset;
    c->start += (UWORD)(offset << 1);
}

/* mt_VolSlideUp / mt_VolSlideDown2 (Axy, EAx, EBx) */
static void mt_volslideup(aud_t *hw, struct chan *c, UBYTE amount)
{
    c->volume += amount;
    if ((BYTE)(UBYTE)(c->volume - 0x40) >= 0)   /* CMP.B #$40: BMI */
        c->volume = 0x40;
    hw->ac_vol = c->volume;
}

static void mt_volslidedown2(aud_t *hw, struct chan *c, UBYTE amount)
{
    c->volume -= amount;
    if ((BYTE)c->volume < 0)
        c->volume = 0;
    hw->ac_vol = c->volume;
}

static void mt_volumeslide(aud_t *hw, struct chan *c)
{
    UBYTE up = c->cmdlo >> 4;

    if (up)
        mt_volslideup(hw, c, up);
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
    UBYTE row = (UBYTE)((c->cmdlo >> 4) * 10 + (c->cmdlo & 0x0f));

    if (row > 63) {
        mt_pj2();
        return;
    }
    pt.pbreakpos = row;
    pt.posjumpflag = 0xff;
}

/* mt_VolumeChange (Cxx) */
static void mt_volumechange(aud_t *hw, struct chan *c)
{
    UBYTE vol = c->cmdlo;

    if (vol > 0x40)
        vol = 0x40;
    c->volume = vol;
    hw->ac_vol = vol;
}

/* mt_SetSpeed (Fxx): 0 stops (mt_end), >= 32 signed the tempo
   (BUG(orig): F80-FFF are speeds, #569) */
static void mt_setspeed(struct chan *c)
{
    UBYTE speed = c->cmdlo;

    if (!speed) {
        mt_end();
        return;
    }
    if ((BYTE)speed >= 32) {
        settempo(speed);
        return;
    }
    pt.counter = 0;
    pt.speed = speed;
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
    UBYTE x = c->cmdlo & 0x0f;
    UBYTE filterbit, funkspeed;         /* d0 (E0x); d0 (EFx) */

    switch (c->cmdlo >> 4) {
    case 0x0:                           /* mt_FilterOnOff */
        filterbit = (UBYTE)((c->cmdlo & 1) << 1);
        CIAAPRA &= 0xfd;
        CIAAPRA |= filterbit;
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
        UBYTE tick;

        if (!x)
            break;
        tick = pt.counter;
        if (!tick && (c->note & 0x0fff))
            break;
        if (tick % x)
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
        funkspeed = (UBYTE)(x << 4);
        c->glissfunk = (c->glissfunk & 0x0f) | funkspeed;
        if (funkspeed)
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
    UWORD note = c->note & 0x0fff, off = 0;
    int left;

    for (left = 36; left >= 0; left--) {  /* not found: d0 = 74, BUG(orig) (#481) */
        if (note >= PERIOD(off))
            break;
        off += 2;
    }
    c->period = PERIOD((ULONG)c->finetune * 36 * 2 + off);
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
static void mt_playvoice(aud_t *hw, struct chan *c, ULONG *patpos, UBYTE *sampleinfo)
{
    ULONG v;
    UWORD sample;                       /* d2: the sample number */
    ULONG startoff;                     /* d2: its offset in the start table */
    ULONG loop;                         /* d2: the loop start */
    UWORD repeat;
    ULONG infooff;

    if (!c->note && !c->cmd && !c->cmdlo)
        mt_pernop(hw, c);
    v = *(volatile ULONG *)(patdata + *patpos);    /* MOVE.L: one access */
    c->note = (UWORD)(v >> 16);
    c->cmd = (UBYTE)(v >> 8);
    c->cmdlo = (UBYTE)v;
    *patpos += 4;
    sample = (UBYTE)((c->cmd & 0xf0) >> 4 | (UBYTE)(c->note >> 8 & 0xf0));
    if (sample) {
        startoff = ((ULONG)sample - 1) << 2;
        infooff = (ULONG)sample * 30;
        c->start = SAMPLESTART(startoff);   /* BUG(orig): sample > 31 (#480) */
        c->length = W(sampleinfo + infooff);
        c->reallength = W(sampleinfo + infooff);
        c->finetune = sampleinfo[infooff + 2];
        c->volume = sampleinfo[infooff + 3];
        repeat = W(sampleinfo + infooff + 4);    /* repeat */
        if (repeat) {
            loop = (ULONG)c->start + (UWORD)(repeat << 1);  /* BUG(orig): word (#479) */
            c->loopstart = (UBYTE *)loop;
            c->wavestart = (UBYTE *)loop;
            c->length = (UWORD)(W(sampleinfo + infooff + 4) + W(sampleinfo + infooff + 6));
            c->replen = W(sampleinfo + infooff + 6);
        } else {
            c->loopstart = c->start;
            c->wavestart = c->start;
            c->replen = W(sampleinfo + infooff + 6);
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
        checkmoreefx(hw, c);            /* BUG(orig): 9xx twice (#473) */
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
        UBYTE *mod = pt.songdataptr, *sampleinfo = mod + 12, *order = mod + 952;
        ULONG patpos;

        patdata = mod + 1084;
        patpos = (ULONG)order[pt.songpos] << 10;
        patpos = (patpos & 0xffff0000UL) | (UWORD)((UWORD)patpos + pt.patternpos);
        pt.dmacontemp = 0;
        /* BUG(orig): a masked voice does not advance d1 (#568) */
        for (i = 0; i < 4; i++)
            if (pt.mask & (8 >> i))
                mt_playvoice(&CUSTOM.aud[i], &pt.chan[i], &patpos, sampleinfo);
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

/**
 * @brief init: nothing plays yet (playing = 0).
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    pt.playing = 0;
}

/**
 * @brief finit: silence the voices (mt_end) if TrackInit was called.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!pt.playing)
        return;
    pt.playing = 0;
    ciaplayerfinit();
}

/**
 * @brief Tracker on+off: switch the player on or off.
 *
 * Nothing happens if the status does not change.  0: mt_end (voices
 * silent, audio DMA off) and status off; else status on - mt_Enable is
 * only ANDed with the value, so the song stays stopped until TrackInit
 * or TrackerPause 0 (BUG(orig) (#572)).  While off, TrackInit does not
 * initialise the song (#573).
 * @param on  d0.b: 0 off, else on (any nonzero byte)
 */
static void BLITZ_ROUTINE tracker_c(UBYTE on BL_REG(d0))
{
    if ((BYTE)on == pt.status)
        return;
    pt.enable &= on;                    /* BUG(orig): On does not resume (#572) */
    pt.status = (BYTE)on;
    if (on)
        ciaplayerinit();
    else
        ciaplayerfinit();
}

/**
 * @brief TrackInit address: initialise a module and enable playing.
 *
 * mt_init: the sample starts behind the highest pattern, song position
 * 0 (its pattern scan and sample clearing: BUG(orig) (#477, #570)).
 * Playing is enabled even while Tracker Off, where mt_init does nothing:
 * Tracker On then plays with the old song pointer (BUG(orig) (#573)).
 * The program calls TrackPlay itself (the CIA interrupt setup is dead,
 * #571).
 * @param module  d0: the module (a Protracker file in memory, 31
 *                samples, "M.K."; not checked)
 */
static void BLITZ_ROUTINE initmodule_c(UBYTE *module BL_REG(d0))
{
    glue.mt_data = module;
    ciaplayerinit();
    pt.playing = 1;
    mt_init();                          /* BUG(orig): nothing if Tracker Off (#573) */
    pt.enable = 0xff;
}

/**
 * @brief TrackDMA dmawait.w: the DBRA count of the waits for Paula's DMA.
 *
 * The waits run DMA_Wait + 1 passes; the default is 4000.
 * @param wait  d0.w: the count
 */
static void BLITZ_ROUTINE tracksetdma_c(UWORD wait BL_REG(d0))
{
    pt.dmawait = wait;
}

/**
 * @brief TrackStop: stop playing (mt_end: volumes 0, audio DMA off).
 *
 * The module address goes to mt_olddata, which nothing reads.
 */
static void BLITZ_ROUTINE stopmodule_c(void)
{
    pt.olddata = glue.mt_data;
    mt_end();
}

/**
 * @brief TrackerPause On|Off: pause or continue the module.
 *
 * Nonzero stops (mt_end), 0 continues (_contmodule): enables playing and
 * plays one tick at once.
 * @param pause  d0.b: nonzero pause, 0 continue
 */
static void BLITZ_ROUTINE trackerpause_c(UBYTE pause BL_REG(d0))
{
    if (pause) {
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

/**
 * @brief TrackerChannels mask: the voices to play.
 *
 * Bit 3 = voice 1 ... bit 0 = voice 4 (default 15).  A cleared voice's
 * pointer, length, period and volume are zeroed (its DMA stays).  A
 * masked voice does not advance the pattern offset, so the following
 * voices play the notes of the voice before (BUG(orig) (#568)); effects
 * and the loop pointers still go to all four voices (#567).
 * @param mask  d0.b: the voice bits
 */
static void BLITZ_ROUTINE trackermask_c(UBYTE mask BL_REG(d0))
{
    int i;

    pt.mask = mask;
    for (i = 0; i < 4; i++)
        if (!(mask & (8 >> i)))
            cut(&CUSTOM.aud[i]);
}

/**
 * @brief TrackPlay: one tick of the play routine (mt_music).
 *
 * Called by the program once per tick (e.g. from a SetInt 5 vertical
 * blank handler); does nothing unless playing is enabled and Tracker is
 * on.  The known PT2.3A bugs of the routine are listed in the head of
 * the file.
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE music_c(void)
{
    mt_music_c();
}

/* Registers: the original keeps all in mt_music, changes d0-d2/a0-a2 in
   TrackInit, a0 in TrackStop and TrackerChannels; every thunk keeps
   d0/d1/a0/a1 (no results), GCC keeps the others. */
BLITZ_THUNK(_initmodule, initmodule_c, "d0-d1/a0-a1");
BLITZ_THUNK(ritrackerlib_mt_music, music_c, "d0-d1/a0-a1");
BLITZ_THUNK(_stopmodule, stopmodule_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackSetDMA, tracksetdma_c, "d0-d1/a0-a1");
BLITZ_THUNK(_tracker, tracker_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackermask, trackermask_c, "d0-d1/a0-a1");
BLITZ_THUNK(_trackerpause, trackerpause_c, "d0-d1/a0-a1");
BLITZ_THUNK(ritrackerlib_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(ritrackerlib_libfinit, libfinit_c, "d0-d1/a0-a1");
