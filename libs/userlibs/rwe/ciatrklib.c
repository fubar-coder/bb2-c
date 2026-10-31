/*
 * ciatrklib - RWE userlib 56 (#ciatrklib): the CIA Protracker library
 * V1.02b (Crossbones/Binary Assault 1995, from Neil O'Rourke's tracker
 * library) - LoadTrackerModule, StartTracker, StopTracker, DecodeModule,
 * GetTrackerSize, GetTrackerLocation, FreeTrackerModule, GetTrackerEvent,
 * CheckTrackerEvent, WaitTrackerEvent, CheckTrackerID, ModuleToChip,
 * GetModuleName$, ModulePositionJump, GetModulePosition, PauseModule,
 * ChangeTrackerVBlank, PlayTracker, TrackerVolume, ChangeTrackerMask,
 * ModulePatterns.
 *
 * C translation of rwelibsrc/ptlib/CIATrackerLib1.bb; reference the
 * reference build of the source by blitz2 2.10 (refbuild).  The play
 * routine is the Protracker 2.3A CIA play routine (Peter "Crayon" Hanning,
 * Mushroom Studios, released as Public Domain; archive/README.md) with the
 * library's changes: the busy-wait fix of Delirium Softdesign
 * (mt_PlaySamples), a master volume (TrackerVolume), a channel mask
 * (ChangeTrackerMask: a masked channel writes its registers into
 * fake_channel) and effect 8 as the "tracker event".  Notes:
 * docs/rwelibs/port/ciatrklib.md.
 *
 * Modules: a table of 10 slots {address, length, flag} (flag 0 = given by
 * DecodeModule, 1 = loaded, 2 = copied to chip memory), slots 0-8 allowed
 * by the error checks.  The library's init installs the CIA interrupt
 * (mt_music every 1/50 s, tempo commands reprogram the timer); after
 * ChangeTrackerVBlank the program calls PlayTracker itself.
 *
 * Data: the three blocks of the original - the library's variables
 * (pause_status ... modtable), the CIA part (RealTempo ... musintname)
 * and the play routine's (mt_FunkTable ... nor_LstEvt) - are structs with
 * the original's layout.  The play routine depends on it: it reads behind
 * mt_PeriodTable into mt_chan1temp and behind mt_SampleStarts (BUG(orig)
 * below, #474 #480 #481), and the test finds the blocks in both builds.
 *
 * Everything is C; the entries are thunks, the hardware registers
 * volatile accesses of the original's width.
 *
 * BUG(orig), kept:
 *   - SetCIAInt loads the ICR bit for AddICRVector from the ADDRESSES 1
 *     (timer A of ciaa) and 2 (timer B of ciab): MOVE.l %00000001,D0
 *     without `#` - the long at address 1 / 2 (an address error on a
 *     68000) (#450).  ResetCIAInt removes the vector with bit 0 (MOVEQ #1
 *     is overwritten; in Amiberry the system's timer A vector of ciaa)
 *     (#448) and toggles the filter (BCHG #1,$bfe001) (#449).
 *   - LoadTrackerModule (no !libs): the dos calls go through whatever a6
 *     holds (#443); the length is the file size + 3 as a byte add
 *     (ADD.b), so the carry is lost (#442); the chip memory is not
 *     checked (#440; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!"); a Read error unlocks the lock OpenFromLock took over
 *     (#444); the runtime errors after Lock (no FileInfoBlock, Examine
 *     failing) leave the lock allocated (#441).
 *   - DecodeModule into chip memory (an address < $200000, signed) keeps
 *     the address (#432); from other memory it copies the module to chip
 *     memory with the size + 3 as a word add (ADD.w) (#434); when the
 *     memory is missing the slot keeps the old address with flag 2 and
 *     the new length (#433; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").  The size is computed with mt_init's scan
 *     (below).
 *   - StartTracker of an empty slot returns the slot number (the MOVEM
 *     restores d0 over the 0) (#451).  ModuleToChip does nothing and
 *     returns its argument (#446).  ModulePatterns returns the song length
 *     (byte 950) (#445).
 *   - CheckTrackerID returns the long at ADDRESS 0 (MOVE.l 0,d0) for
 *     "not a module" and an empty slot (#430).
 *   - GetModuleName$ of an empty slot returns slot*10 as the length and
 *     writes nothing (not even the 0) (#437).
 *   - WaitTrackerEvent calls WaitTOF through whatever a6 holds (no
 *     !libs) (#453); CheckTrackerEvent changes only the low byte of d0
 *     (#431).
 *   - ModulePositionJump n: the song position becomes n + 1 at the next
 *     tick (mt_NextPosition increments it) (#447).
 *   - TrackerVolume 0 or 64 does not change the volumes of the notes that
 *     play (only 0 < volume < 64 calls Change_Volume_Levels) (#452).
 *   - mt_init compares the module LENGTH with the sample addresses (CMP.l
 *     a2,a4): the first long of a sample is cleared only when the length
 *     is greater than its address (#438).  Its scan of the song positions
 *     reads positions 0-126 (SUBQ for a new maximum, DBRA otherwise; a
 *     new maximum at 126 reads ~64 KB more), patterns >= $80 never count
 *     (BGT, signed) (#477).
 *   - Play routine (Protracker 2.3A): mt_LowMask starts as 0, so the
 *     first 1xx/2xx before an E1x/E2x does not slide (#478); the sample
 *     offset (9xx with a note) is applied twice to n_start/n_length
 *     (mt_SetRegs calls mt_CheckMoreEfx before mt_SetPeriod, which runs it
 *     again) (#473); tremolo's ramp-down tests the VIBRATO position
 *     (#482); mt_SetPeriod without a matching period (#481), the arpeggio
 *     (#474) and a sample number > 31 (#480) read behind the tables
 *     (mt_chan1temp, mt_SongDataPtr ...); E8x (Karplus-Strong) with a
 *     loop length of 0 rewrites 64 KB (#439); the repeat offset is
 *     repeat * 2 as a word (ASL.w) (#479); the sample offset compares
 *     signed with n_length (CMP.w, BGE): 9xx on a length of $8000 words
 *     or more sets the length to 1 (#472); F00 stops the module (as in
 *     every Protracker), but mt_end loads a0 with $DFF000: after an F00
 *     on voice 1-3 the voices behind read their notes from the custom
 *     chip registers ($DFF000 + the pattern offset) instead of the
 *     pattern (#475); effect 8 (the event) is seen only on ticks other
 *     than 0 (#435).
 *   - FreeTrackerModule does not stop a module that plays from the slot
 *     (#436).
 * Not defects: GetTrackerEvent changes only the low byte of d0 (it is
 * declared #byte).
 *
 * DIFF: none in the results.  Not reproducible: a finetune byte > 15 in a
 * sample header (n_finetune * 72) reads behind the play routine's data,
 * where the original has its own code and the C build other memory
 * (#476).
 * The C build keeps a3 in FreeTrackerModule (the original's LEA
 * modtable,a3 changes the string pointer).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <graphics/gfxbase.h>
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dos
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _loadtrkmodule(void), _StartTrkModule(void), _StopTracker(void), _decodemod(void);
void _gettrklen(void), _gettrkloc(void), _freetrackermodule(void), _gettrackerevent(void);
void _checktrackerevent(void), _waittrackerevent(void), _checkid(void), _modtochip2(void);
void _getmodulename(void), _modulepositionjump(void), _getmoduleposition(void);
void _pausemodule(void), _change_playtype(void), _Play_Tracker(void), _Volume_Control(void);
void _ChangeTrackerMask(void), _maxpatterns(void), _inittrackerlib(void);
void _finittrackerlib(void), mt_music(void);
void _loadcheck(void), _checkused(void), _decodecheck(void), _slotcheck(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ciatrklib,init,0,finit,_runerrors} */
BLITZ_LIBHEADER(LIB_ciatrklib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_loadtrkmodule, _loadcheck, 0);
BLITZ_NAME("LoadTrackerModule", "(TrackerModule#,Name)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_StartTrkModule, _checkused, 0);
BLITZ_NAME("StartTracker", "(TrackerModule#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_StopTracker, 0, 0);
BLITZ_NAME("StopTracker", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_decodemod, _decodecheck, 0);
BLITZ_NAME("DecodeModule", "TrackerModule#,ModuleAddress");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gettrklen, _checkused, 0);
BLITZ_NAME("GetTrackerSize", "(TrackerModule#)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_gettrkloc, _checkused, 0);
BLITZ_NAME("GetTrackerLocation", "(TrackerModule#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_freetrackermodule, _slotcheck, 0);
BLITZ_NAME("FreeTrackerModule", "TrackerModule#");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_gettrackerevent, 0, 0);
BLITZ_NAME("GetTrackerEvent", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_checktrackerevent, 0, 0);
BLITZ_NAME("CheckTrackerEvent", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_waittrackerevent, 0, 0);
BLITZ_NAME("WaitTrackerEvent", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_checkid, _checkused, 0);
BLITZ_NAME("CheckTrackerID", "(TrackerModule#)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_modtochip2, 0, 0);
BLITZ_NAME("ModuleToChip", "(TrackerModule#)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_getmodulename, _checkused, 0);
BLITZ_NAME("GetModuleName$", "(TrackerModule#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_modulepositionjump, 0, 0);
BLITZ_NAME("ModulePositionJump", "(Position#)");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_getmoduleposition, 0, 0);
BLITZ_NAME("GetModulePosition", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_pausemodule, 0, 0);
BLITZ_NAME("PauseModule", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_change_playtype, 0, 0);
BLITZ_NAME("ChangeTrackerVBlank", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_Play_Tracker, 0, 0);
BLITZ_NAME("PlayTracker", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_Volume_Control, 0, 0);
BLITZ_NAME("TrackerVolume", "(Volume Range: 0-64)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_ChangeTrackerMask, 0, 0);
BLITZ_NAME("ChangeTrackerMask", "Channel_Mask");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_maxpatterns, _checkused, 0);
BLITZ_NAME("ModulePatterns", "(Module #)");

/* #InHouse=0: TrackerLine, TrackerPatternToMem and TrackerSongPos are
   not part of the library (CNIF) */

BLITZ_NULLSUB_AT(init, _inittrackerlib, 0, 0);
BLITZ_NULLSUB_AT(finit, _finittrackerlib, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define CUSTOM ((volatile struct Custom *)0xdff000)
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define VPOSR_LO (*(volatile UBYTE *)0xdff006)  /* the vertical position, bits 0-7 */
#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))
#define WORDAT(p) (*(UWORD *)(p))
#define LONGAT(p) (*(ULONG *)(p))

typedef volatile struct AudChannel aud_t;

/* the CIAs: struct CIA at the base of the chip ($BFD000 ciab / $BFE001
   ciaa), its registers $100 apart */
static_assert(__builtin_offsetof(struct CIA, ciatalo) == 0x400);
static_assert(__builtin_offsetof(struct CIA, ciacrb) == 0xf00);

/* a module slot: Dc.l address, length, Dc.w flag (10 bytes) */
struct modent {
    UBYTE *addr;
    ULONG len;
    UWORD flag;
};

/* the library's variables (pause_status ... modtable) */
static struct {
    UBYTE pause_status, play_type;
    UWORD old_volume, master_volume, volume_flag;
    UBYTE master_mask, master_mask_lo;  /* Dc.w; BSET/BTST/BCLR address its first byte */
    struct modent modtable[10];         /* Dc.l x25: 10 slots */
} libvars;

/* the CIA part (RealTempo ... musintname) */
static struct {
    UWORD RealTempo;
    volatile struct CIA *CIAAaddr;
    char CIAAname[14], CIABname[14];
    struct Library *CIAAbase;
    ULONG TimerFlag, TimerValue;
    UBYTE *mt_data;
    char GfxName[18];
    struct Interrupt MusicIntServer;
    char musintname[20];
} ciapart = {
    125, NULL, "ciaa.resource", "ciab.resource", NULL, 0, 0, NULL, "graphics.library",
    { { NULL, NULL, NT_INTERRUPT, 5, ciapart.musintname }, NULL, (VOID (*)())mt_music },
    "Protracker MusicInt",
};

/* a voice of the play routine (mt_chanNtemp, 44 bytes) */
struct chan {
    union {
        ULONG l;                        /* the note as read from the pattern */
        struct { UWORD note; UBYTE cmd, cmdlo; } f;
    } n;                                /* 0 n_note, 2 n_cmd, 3 n_cmdlo */
    UBYTE *start;                       /* 4 */
    UWORD length;                       /* 8 */
    UBYTE *loopstart;                   /* 10 */
    UWORD replen;                       /* 14 */
    UWORD period;                       /* 16 */
    UBYTE finetune, volume;             /* 18, 19 */
    UWORD dmabit;                       /* 20 */
    UBYTE toneportdirec, toneportspeed; /* 22, 23 */
    UWORD wantedperiod;                 /* 24 */
    UBYTE vibratocmd, vibratopos;       /* 26, 27 */
    UBYTE tremolocmd, tremolopos;       /* 28, 29 */
    UBYTE wavecontrol, glissfunk;       /* 30, 31 */
    UBYTE sampleoffset, pattpos;        /* 32, 33 */
    UBYTE loopcount, funkoffset;        /* 34, 35 */
    UBYTE *wavestart;                   /* 36 */
    UWORD reallength;                   /* 40 */
    UWORD pad;
};
_Static_assert(__builtin_offsetof(struct chan, wavestart) == 36, "n_wavestart");
_Static_assert(sizeof(struct chan) == 44, "mt_chanNtemp");

#define CMDW(ch) ((UWORD)((ch)->n.f.cmd << 8 | (ch)->n.f.cmdlo))  /* MOVE.W n_cmd(A6) */

/* the play routine's data (mt_FunkTable ... nor_LstEvt) */
static struct {
    UBYTE FunkTable[16];
    UBYTE VibratoTable[32];
    UWORD PeriodTable[16 * 36];
    struct chan chan[4];
    struct AudChannel fake_channel;     /* the registers of a masked voice */
    UBYTE fake_rest[60 - sizeof(struct AudChannel)];    /* (Dc.l x15) */
    UBYTE *SampleStarts[32];
    UBYTE *SongDataPtr;
    UBYTE speed, counter, SongPos, PBreakPos, PosJumpFlag, PBreakFlag, LowMask;
    UBYTE PattDelTime, PattDelTime2, Enable;
    UWORD PatternPos, DMACONtemp;
    UBYTE TrkEvt, LstEvt;
} playdata = {
    { 0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128 },
    {   0,  24,  49,  74,  97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250, 253,
      255, 253, 250, 244, 235, 224, 212, 197, 180, 161, 141, 120,  97,  74,  49,  24 },
    {
        /* Tuning 0, Normal */
        856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
        428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
        214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
        /* Tuning 1 */
        850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
        425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
        213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
        /* Tuning 2 */
        844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
        422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
        211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
        /* Tuning 3 */
        838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
        419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
        209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
        /* Tuning 4 */
        832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
        416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
        208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
        /* Tuning 5 */
        826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
        413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
        206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
        /* Tuning 6 */
        820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
        410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
        205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
        /* Tuning 7 */
        814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
        407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
        204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
        /* Tuning -8 */
        907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
        453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
        226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
        /* Tuning -7 */
        900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
        450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
        225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
        /* Tuning -6 */
        894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
        447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
        223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
        /* Tuning -5 */
        887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
        444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
        222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
        /* Tuning -4 */
        881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
        441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
        220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
        /* Tuning -3 */
        875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
        437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
        219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
        /* Tuning -2 */
        868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
        434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
        217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
        /* Tuning -1 */
        862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
        431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
        216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
    },
    { { .dmabit = 1 }, { .dmabit = 2 }, { .dmabit = 4 }, { .dmabit = 8 } },
    { 0 }, { 0 }, { 0 }, NULL,
    6, 0, 0, 0, 0, 0, 0,                /* mt_speed 6 ... mt_LowMask 0 (BUG(orig), #478) */
    0, 0, 0, 0, 0, 0, 0,
};
_Static_assert(__builtin_offsetof(__typeof__(playdata), chan) == 48 + 1152, "mt_chan1temp");
_Static_assert(sizeof(playdata) == 1584, "mt_FunkTable ... nor_LstEvt");
_Static_assert(sizeof(libvars) == 110, "pause_status ... modtable");
_Static_assert(sizeof(ciapart) == 110, "RealTempo ... musintname");

/* LoadTrackerModule's variables */
static BPTR hand, lock;
static ULONG fname, fib, readerr, thismod;

static const char mess[] = "Tracker: File error! (Not found?)";
static const char mess2[] = "Tracker Library Error";

/* a pointer GCC knows nothing about: the play routine reads behind its
   tables (into the following data, like the original), and some reads
   are from fixed low addresses (0, 1, 2) */
#define LAUNDER(ptr) __asm__("" : "+a"(ptr))

static ULONG peekl(ULONG addr)
{
    const ULONG *q = (const ULONG *)addr;
    LAUNDER(q);
    return *(volatile const ULONG *)q;
}

/* LEA modtable,a3: MULU #10,d0: ADDA d0,a3 (word, sign extended) */
static struct modent *slot(ULONG num)
{
    UBYTE *table = (UBYTE *)libvars.modtable;
    LAUNDER(table);
    return (struct modent *)(table + (WORD)bl_mulu_w((UWORD)num, 10));
}

/* the period table of finetune `ft` (MULU #36*2: also > 15) */
static const UWORD *periods(UBYTE ft)
{
    const UWORD *table = playdata.PeriodTable;
    LAUNDER(table);
    return (const UWORD *)((const UBYTE *)table + bl_mulu_w(ft, 36 * 2));
}

/* the registers of voice `ch`: Paula's, or fake_channel if masked */
static aud_t *voice(int ch)
{
    if (libvars.master_mask & (1 << ch))
        return &CUSTOM->aud[ch];
    return &playdata.fake_channel;
}

/* ------------------------------------------------------- memlib calls */

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };
    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };
    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* ------------------------------------------------------------ CIA part */

static void mt_end(void);
static void resetciaint(void);

/* a0 of mt_GetNewNote, the pattern data; mt_end sets a0 to $DFF000, so
   after an F00 (mt_SetSpeed: BEQ mt_end) the voices behind read their
   notes from the custom chip registers (BUG(orig), #475) */
static const UBYTE *patdata;

/* SetCIAInt: the interrupt on timer A of ciaa, else timer B of ciab */
static void setciaint(void)
{
    const char *name = ciapart.CIAAname;
    struct Library *res, *gfx;
    UWORD tries = 2;
    ULONG clock;                        /* d7: the PAL/NTSC clock */
    ULONG timerval;                     /* d7: clock/125 */

    for (;;) {
        res = OpenResource((STRPTR)name);
        ciapart.CIAAbase = res;
        if (!res)
            return;
        gfx = OpenLibrary((STRPTR)ciapart.GfxName, 0);
        if (!gfx) {
            resetciaint();              /* BEQ ResetCIAInt */
            return;
        }
        if (((struct GfxBase *)gfx)->DisplayFlags & PAL)
            clock = 1773447;            /* PAL */
        else
            clock = 1789773;            /* NTSC */
        ciapart.TimerValue = clock;
        timerval = bl_divu_w(clock, 125);   /* default: 50 Hz */
        CloseLibrary(gfx);
        if (tries != 2) {               /* 'TryTimerB */
            volatile struct CIA *cia = (volatile struct CIA *)0xbfd000;
            /* BUG(orig): the bit is the long at address 2 (#450) */
            struct bl_regs r = { .d0 = peekl(2), .a1 = (ULONG)&ciapart.MusicIntServer };
            ULONG err = BLITZ_BASEJSR(res, -6, r);      /* AddICRVector */
            ciapart.TimerFlag = 1;
            if (!err) {
                ciapart.CIAAaddr = cia;
                cia->ciatblo = (UBYTE)timerval;
                cia->ciatbhi = (UBYTE)((UWORD)timerval >> 8);
                cia->ciacrb |= 1;
                return;
            }
        } else {                        /* 'TryTimerA */
            volatile struct CIA *cia = (volatile struct CIA *)0xbfe001;
            /* BUG(orig): the bit is the long at address 1 (#450) */
            struct bl_regs r = { .d0 = peekl(1), .a1 = (ULONG)&ciapart.MusicIntServer };
            ULONG err = BLITZ_BASEJSR(res, -6, r);      /* AddICRVector */
            ciapart.TimerFlag = 0;
            if (!err) {
                ciapart.CIAAaddr = cia;
                cia->ciatalo = (UBYTE)timerval;
                cia->ciatahi = (UBYTE)((UWORD)timerval >> 8);
                cia->ciacra |= 1;
                return;
            }
        }
        name = ciapart.CIABname;              /* 'CIAError */
        if (--tries == 0)
            break;
    }
    ciapart.CIAAbase = NULL;
}

/* ResetCIAInt */
static void resetciaint(void)
{
    struct Library *res = ciapart.CIAAbase;
    volatile struct CIA *cia;

    if (!res)
        return;
    ciapart.CIAAbase = NULL;
    cia = ciapart.CIAAaddr;
    if (ciapart.TimerFlag)
        cia->ciacrb &= ~1;
    else
        cia->ciacra &= ~1;
    {
        /* BUG(orig): always bit 0 (MOVEQ #0,d0 after MOVEQ #1,D0) (#448) */
        struct bl_regs r = { .d0 = 0, .a1 = (ULONG)&ciapart.MusicIntServer };
        BLITZ_BASEJSR(res, -12, r);     /* RemICRVector */
    }
    CIAAPRA ^= 2;                       /* BCHG #1,$bfe001 (BUG(orig), #449) */
}

/* SetTempo: d0 = beats per minute (>= 32), the timer TimerValue / d0 */
static void settempo(UWORD bpm)
{
    volatile struct CIA *cia;
    ULONG count;

    if (!ciapart.CIAAbase)
        return;
    if (bpm < 32)
        bpm = 32;
    ciapart.RealTempo = bpm;
    count = bl_divu_w(ciapart.TimerValue, bpm);
    cia = ciapart.CIAAaddr;
    if (ciapart.TimerFlag) {
        cia->ciatblo = (UBYTE)count;
        cia->ciatbhi = (UBYTE)((UWORD)count >> 8);
    } else {
        cia->ciatalo = (UBYTE)count;
        cia->ciatahi = (UBYTE)((UWORD)count >> 8);
    }
}

/* -------------------------------------------------------- play routine */

/* Set_Volume: volume * master_volume / 64 (MULU, DIVU #$40) */
static UWORD set_volume(UWORD v)
{
    return (UWORD)bl_divu_w(bl_mulu_w(v, libvars.master_volume), 0x40);
}

/* Set_Real_Mask: the DMA bits of masked channels off */
static UWORD set_real_mask(UWORD bits)
{
    return bits & (0xfff0 | libvars.master_mask);
}

/* the highest pattern number of the song positions (mt_init, _modtochip):
   positions 0-126 (BUG(orig), see the head, #477); signed compare */
static UBYTE maxpattern(const UBYTE *mod)
{
    const BYTE *pos = (const BYTE *)mod + 952;
    UWORD left = 126;                   /* MOVEQ #127: SUBQ.W #1 */
    BYTE max = 0, pat;

    for (;;) {
        pat = *pos++;
        if (pat > max) {                /* BGT mtloop */
            max = pat;
            left--;
            continue;
        }
        if (left-- == 0)                /* DBRA */
            break;
    }
    return (UBYTE)max;
}

/* the size of the pattern data and samples: 1084 + (max+1)*1024 */
static ULONG patternsize(const UBYTE *mod)
{
    return ((ULONG)(UBYTE)(maxpattern(mod) + 1) << 10) + 1084;
}

/* mt_end */
static void mt_end(void)
{
    volatile struct Custom *cu = CUSTOM;

    playdata.Enable = 0;
    patdata = (const UBYTE *)cu;        /* LEA $DFF000,A0 (BUG(orig), #475) */
    cu->aud[0].ac_vol = 0;
    cu->aud[1].ac_vol = 0;
    cu->aud[2].ac_vol = 0;
    cu->aud[3].ac_vol = 0;
    cu->dmacon = 0xf;
}

/* mt_init: a4 = the module length (4(a3) of the slot) */
static void mt_init(ULONG modlen)
{
    UBYTE *hdr = ciapart.mt_data, *sampdata;
    int i;

    playdata.SongDataPtr = hdr;
    sampdata = hdr + patternsize(hdr);
    for (i = 0; i < 31; i++) {
        if ((LONG)modlen > (LONG)sampdata) /* BUG(orig): the length (#438) */
            LONGAT(sampdata) = 0;
        playdata.SampleStarts[i] = sampdata;
        sampdata += (ULONG)WORDAT(hdr + 42) << 1;
        hdr += 30;
    }
    CIAAPRA |= 2;                       /* filter off */
    playdata.speed = 6;
    playdata.counter = 0;
    playdata.SongPos = 0;
    playdata.PatternPos = 0;
    mt_end();
}

/* wait until the end of the raster line */
static void waitline(void)
{
    UBYTE v = VPOSR_LO;

    while (VPOSR_LO == v)
        ;
}

/* mt_PlaySamples (the busy-wait fix of Delirium Softdesign): stop the
   channels of d2 that play, start them, then set their periods */
static void playsamples(UWORD chans)
{
    volatile struct Custom *cu = CUSTOM;
    UWORD active, activeint;            /* the playing channels, their INTREQ bits */
    UWORD start, startint;              /* the channels to start, their INTREQ bits */
    int i;

    waitline();
    for (i = 0; i < 4; i++)
        if (chans & (1 << i))
            cu->aud[i].ac_per = 1;      /* max. speed */
    active = cu->dmaconr & chans;       /* active channels */
    activeint = (UWORD)(active << 7);
    cu->intreq = activeint;             /* stop channels */
    cu->dmacon = active;
    while ((cu->intreqr & activeint) != activeint) /* mt_Wait2 */
        ;
    start = ~cu->dmaconr & chans;
    startint = (UWORD)(start << 7);
    cu->intreq = startint;
    cu->dmacon = start | 0x8000;        /* start channels */
    while ((cu->intreqr & startint) != startint) /* mt_Wait3 */
        ;
    waitline();
    for (i = 0; i < 4; i++)
        if (chans & (1 << i))
            cu->aud[i].ac_per = playdata.chan[i].period;
}

static void pernop(struct chan *ch, aud_t *aud)
{
    aud->ac_per = ch->period;
}

/* mt_UpdateFunk: invert loop (EFx) */
static void updatefunk(struct chan *ch)
{
    UBYTE speed = ch->glissfunk >> 4, *pos, *end;

    if (!speed)
        return;
    ch->funkoffset += playdata.FunkTable[speed];
    if (!(ch->funkoffset & 0x80))
        return;
    ch->funkoffset = 0;
    end = ch->loopstart + ch->replen + ch->replen;
    pos = ch->wavestart + 1;
    if (!(pos < end))
        pos = ch->loopstart;
    ch->wavestart = pos;
    *pos = (UBYTE)(0xff - *pos);        /* MOVEQ #-1,D0: SUB.B (A0),D0 */
}

/* mt_Arpeggio */
static void arpeggio(struct chan *ch, aud_t *aud)
{
    UWORD step = (UWORD)(bl_divs_w(playdata.counter, 3) >> 16), halftones; /* DIVS: SWAP */
    const UWORD *table;
    int i;

    if (step == 0) {
        aud->ac_per = ch->period;
        return;
    }
    halftones = step == 2 ? ch->n.f.cmdlo & 15 : ch->n.f.cmdlo >> 4;
    table = periods(ch->finetune);
    for (i = 0; i <= 36; i++) {         /* reads up to 15 words behind */
        UWORD per = table[i + halftones];
        if (ch->period >= table[i]) {
            aud->ac_per = per;
            return;
        }
    }
}

/* mt_PortaUp (mt_LowMask: $0F for E1x, else $FF after the first use) */
static void portaup(struct chan *ch, aud_t *aud)
{
    UBYTE delta = ch->n.f.cmdlo & playdata.LowMask;

    playdata.LowMask = 0xff;
    ch->period -= delta;
    if ((WORD)((ch->period & 0x0fff) - 113) < 0)
        ch->period = (ch->period & 0xf000) | 113;
    aud->ac_per = ch->period & 0x0fff;
}

static void portadown(struct chan *ch, aud_t *aud)
{
    UBYTE delta = ch->n.f.cmdlo & playdata.LowMask;

    playdata.LowMask = 0xff;
    ch->period += delta;
    if ((WORD)((ch->period & 0x0fff) - 856) >= 0)
        ch->period = (ch->period & 0xf000) | 856;
    aud->ac_per = ch->period & 0x0fff;
}

static void fineportaup(struct chan *ch, aud_t *aud)
{
    if (playdata.counter)
        return;
    playdata.LowMask = 0x0f;
    portaup(ch, aud);
}

static void fineportadown(struct chan *ch, aud_t *aud)
{
    if (playdata.counter)
        return;
    playdata.LowMask = 0x0f;
    portadown(ch, aud);
}

/* the index of the first period <= d2 in table t (35 if none) */
static UWORD findperiod(const UWORD *table, UWORD per)
{
    UWORD i;

    for (i = 0; i < 36; i++)
        if (per >= table[i])
            return i;
    return 35;
}

/* mt_SetTonePorta */
static void settoneporta(struct chan *ch)
{
    const UWORD *table = periods(ch->finetune);
    UWORD i = findperiod(table, ch->n.f.note & 0x0fff), per;

    if ((ch->finetune & 8) && i)
        i--;
    per = table[i];
    ch->wantedperiod = per;
    ch->toneportdirec = 0;
    if (per == ch->period)
        ch->wantedperiod = 0;
    else if ((WORD)per < (WORD)ch->period)
        ch->toneportdirec = 1;
}

/* mt_TonePortNoChange */
static void toneportnochange(struct chan *ch, aud_t *aud)
{
    UWORD per;

    if (!ch->wantedperiod)
        return;
    if (!ch->toneportdirec) {
        ch->period += ch->toneportspeed;
        if (!((WORD)ch->wantedperiod > (WORD)ch->period)) {
            ch->period = ch->wantedperiod;
            ch->wantedperiod = 0;
        }
    } else {
        ch->period -= ch->toneportspeed;
        if (!((WORD)ch->wantedperiod < (WORD)ch->period)) {
            ch->period = ch->wantedperiod;
            ch->wantedperiod = 0;
        }
    }
    per = ch->period;
    if (ch->glissfunk & 0x0f) {         /* glissando: the table's period */
        const UWORD *table = periods(ch->finetune);
        per = table[findperiod(table, per)];
    }
    aud->ac_per = per;
}

static void toneportamento(struct chan *ch, aud_t *aud)
{
    if (ch->n.f.cmdlo) {
        ch->toneportspeed = ch->n.f.cmdlo;
        ch->n.f.cmdlo = 0;
    }
    toneportnochange(ch, aud);
}

/* the wave value of position `pos` (vibrato and tremolo): sine, ramp
   down (tested on `rpos`), square */
static UBYTE wave(UBYTE pos, UBYTE form, UBYTE rpos)
{
    UBYTE val = (pos >> 2) & 0x1f;

    if (form == 0)
        return playdata.VibratoTable[val];
    val <<= 3;
    if (form != 1)
        return 255;
    if ((BYTE)rpos < 0)
        return (UBYTE)(255 - val);
    return val;
}

/* mt_Vibrato2 */
static void vibrato2(struct chan *ch, aud_t *aud)
{
    UWORD delta = wave(ch->vibratopos, ch->wavecontrol & 3, ch->vibratopos);
    UWORD per = ch->period;

    delta = (UWORD)(bl_mulu_w(delta, ch->vibratocmd & 15) >> 7);
    if ((BYTE)ch->vibratopos < 0)
        per -= delta;
    else
        per += delta;
    aud->ac_per = per;
    ch->vibratopos += (ch->vibratocmd >> 2) & 0x3c;
}

/* the new depth/speed nibbles of 4xy / 7xy */
static UBYTE newcmd(UBYTE old, UBYTE cmdlo)
{
    if (cmdlo & 0x0f)
        old = (old & 0xf0) | (cmdlo & 0x0f);
    if (cmdlo & 0xf0)
        old = (old & 0x0f) | (cmdlo & 0xf0);
    return old;
}

static void vibrato(struct chan *ch, aud_t *aud)
{
    if (ch->n.f.cmdlo)
        ch->vibratocmd = newcmd(ch->vibratocmd, ch->n.f.cmdlo);
    vibrato2(ch, aud);
}

static void volumeslide(struct chan *ch, aud_t *aud);

static void toneplusvolslide(struct chan *ch, aud_t *aud)
{
    toneportnochange(ch, aud);
    volumeslide(ch, aud);
}

static void vibratoplusvolslide(struct chan *ch, aud_t *aud)
{
    vibrato2(ch, aud);
    volumeslide(ch, aud);
}

/* mt_Tremolo: BUG(orig) the ramp down tests n_vibratopos (#482) */
static void tremolo(struct chan *ch, aud_t *aud)
{
    UWORD delta;
    WORD vol;

    if (ch->n.f.cmdlo)
        ch->tremolocmd = newcmd(ch->tremolocmd, ch->n.f.cmdlo);
    delta = wave(ch->tremolopos, (ch->wavecontrol >> 4) & 3, ch->vibratopos);
    delta = (UWORD)(bl_mulu_w(delta, ch->tremolocmd & 15) >> 6);
    vol = ch->volume;
    if ((BYTE)ch->tremolopos < 0)
        vol -= delta;
    else
        vol += delta;
    if (vol < 0)
        vol = 0;
    if ((UWORD)vol > 0x40)
        vol = 0x40;
    aud->ac_vol = set_volume(vol);
    ch->tremolopos += (ch->tremolocmd >> 2) & 0x3c;
}

/* mt_SampleOffset */
static void sampleoffset(struct chan *ch)
{
    UWORD offset;

    if (ch->n.f.cmdlo)
        ch->sampleoffset = ch->n.f.cmdlo;
    offset = (UWORD)(ch->sampleoffset << 7);
    if ((WORD)offset >= (WORD)ch->length) { /* BUG(orig): signed (CMP.w, BGE) (#472) */
        ch->length = 1;
        return;
    }
    ch->length -= offset;
    ch->start += (UWORD)(offset << 1);
}

static void volslideup(struct chan *ch, aud_t *aud, UBYTE delta)
{
    ch->volume += delta;
    if ((BYTE)(ch->volume - 0x40) >= 0) /* CMP.B #$40: BMI */
        ch->volume = 0x40;
    aud->ac_vol = set_volume(ch->volume);
}

static void volslidedown2(struct chan *ch, aud_t *aud, UBYTE delta)
{
    ch->volume -= delta;
    if ((BYTE)ch->volume < 0)           /* BPL */
        ch->volume = 0;
    aud->ac_vol = set_volume(ch->volume);
}

static void volumeslide(struct chan *ch, aud_t *aud)
{
    UBYTE up = ch->n.f.cmdlo >> 4;

    if (up)
        volslideup(ch, aud, up);
    else
        volslidedown2(ch, aud, ch->n.f.cmdlo & 0x0f);
}

static void positionjump(struct chan *ch)
{
    playdata.SongPos = ch->n.f.cmdlo - 1;
    playdata.PBreakPos = 0;
    playdata.PosJumpFlag = 0xff;
}

static void volumechange(struct chan *ch, aud_t *aud)
{
    UBYTE vol = ch->n.f.cmdlo;

    if (vol > 0x40)
        vol = 0x40;
    ch->volume = vol;
    aud->ac_vol = set_volume(vol);
}

/* mt_PatternBreak: the row in BCD */
static void patternbreak(struct chan *ch)
{
    UBYTE row = (UBYTE)(bl_mulu_w(ch->n.f.cmdlo >> 4, 10) + (ch->n.f.cmdlo & 0x0f));

    playdata.PBreakPos = row > 63 ? 0 : row;
    playdata.PosJumpFlag = 0xff;
}

/* mt_SetSpeed: 0 stops (mt_end), >= 32 is the tempo */
static void setspeed(struct chan *ch)
{
    UBYTE speed = ch->n.f.cmdlo;

    if (!speed) {
        mt_end();
        return;
    }
    if (speed >= 32) {
        settempo(speed);
        return;
    }
    playdata.counter = 0;
    playdata.speed = speed;
}

static void filteronoff(struct chan *ch)
{
    CIAAPRA &= 0xfd;
    CIAAPRA |= (UBYTE)((ch->n.f.cmdlo & 1) << 1);
}

static void jumploop(struct chan *ch)
{
    UBYTE count;

    if (playdata.counter)
        return;
    count = ch->n.f.cmdlo & 0x0f;
    if (!count) {                       /* mt_SetLoop */
        ch->pattpos = (UBYTE)(playdata.PatternPos >> 4);
        return;
    }
    if (ch->loopcount) {
        if (--ch->loopcount == 0)
            return;
    } else {
        ch->loopcount = count;
    }
    playdata.PBreakPos = ch->pattpos;
    playdata.PBreakFlag = 0xff;
}

/* mt_KarplusStrong: BUG(orig) a loop length 0 is 65536 bytes (DBRA)
   (#439) */
static void karplusstrong(struct chan *ch)
{
    BYTE *pos = (BYTE *)ch->loopstart, *first = pos;
    UWORD n = (UWORD)(ch->replen + ch->replen - 2);

    do {
        *pos = (BYTE)((pos[0] + pos[1]) >> 1);
        pos++;
    } while (n-- != 0);
    *pos = (BYTE)((*pos + *first) >> 1);
}

/* mt_DoRetrig: the sample from its start, then the loop; the length and
   the period as one long (MOVE.L n_replen(A6),4(A5)) */
static void doretrig(struct chan *ch, aud_t *aud)
{
    aud->ac_ptr = (UWORD *)ch->start;
    aud->ac_len = ch->length;
    playsamples(set_real_mask(ch->dmabit));
    aud->ac_ptr = (UWORD *)ch->loopstart;
    *(volatile ULONG *)&aud->ac_len = (ULONG)ch->replen << 16 | ch->period;
}

static void retrignote(struct chan *ch, aud_t *aud)
{
    UWORD rate = ch->n.f.cmdlo & 0x0f, tick;

    if (!rate)
        return;
    tick = playdata.counter;
    if (!tick && (ch->n.f.note & 0x0fff))
        return;
    if (bl_divu_w(tick, rate) >> 16)    /* DIVU: the remainder */
        return;
    doretrig(ch, aud);
}

static void volumefineup(struct chan *ch, aud_t *aud)
{
    if (playdata.counter)
        return;
    volslideup(ch, aud, ch->n.f.cmdlo & 0x0f);
}

static void volumefinedown(struct chan *ch, aud_t *aud)
{
    if (playdata.counter)
        return;
    volslidedown2(ch, aud, ch->n.f.cmdlo & 0x0f);
}

static void notecut(struct chan *ch, aud_t *aud)
{
    if ((ch->n.f.cmdlo & 0x0f) != playdata.counter)
        return;
    ch->volume = 0;
    aud->ac_vol = 0;
}

/* mt_NoteDelay: the whole note word (sample number too) must be set */
static void notedelay(struct chan *ch, aud_t *aud)
{
    if ((ch->n.f.cmdlo & 0x0f) != playdata.counter)
        return;
    if (!ch->n.f.note)
        return;
    doretrig(ch, aud);
}

static void patterndelay(struct chan *ch)
{
    if (playdata.counter || playdata.PattDelTime2)
        return;
    playdata.PattDelTime = (ch->n.f.cmdlo & 0x0f) + 1;
}

static void funkit(struct chan *ch)
{
    UBYTE speed;

    if (playdata.counter)
        return;
    speed = (UBYTE)((ch->n.f.cmdlo & 0x0f) << 4);
    ch->glissfunk = (ch->glissfunk & 0x0f) | speed;
    if (!speed)
        return;
    updatefunk(ch);
}

/* mt_E_Commands */
static void e_commands(struct chan *ch, aud_t *aud)
{
    UBYTE lo = ch->n.f.cmdlo & 0x0f;

    switch (ch->n.f.cmdlo >> 4) {
    case 0x0: filteronoff(ch); break;
    case 0x1: fineportaup(ch, aud); break;
    case 0x2: fineportadown(ch, aud); break;
    case 0x3: ch->glissfunk = (ch->glissfunk & 0xf0) | lo; break;
    case 0x4: ch->wavecontrol = (ch->wavecontrol & 0xf0) | lo; break;
    case 0x5: ch->finetune = lo; break;
    case 0x6: jumploop(ch); break;
    case 0x7: ch->wavecontrol = (ch->wavecontrol & 0x0f) | (UBYTE)(lo << 4); break;
    case 0x8: karplusstrong(ch); break;
    case 0x9: retrignote(ch, aud); break;
    case 0xa: volumefineup(ch, aud); break;
    case 0xb: volumefinedown(ch, aud); break;
    case 0xc: notecut(ch, aud); break;
    case 0xd: notedelay(ch, aud); break;
    case 0xe: patterndelay(ch); break;
    case 0xf: funkit(ch); break;
    }
}

/* mt_CheckEfx: the effects of the ticks between the rows */
static void checkefx(struct chan *ch, aud_t *aud)
{
    UBYTE cmd;

    updatefunk(ch);
    if (!(CMDW(ch) & 0x0fff)) {
        pernop(ch, aud);
        return;
    }
    switch (cmd = ch->n.f.cmd & 0x0f) {
    case 0x0: arpeggio(ch, aud); return;
    case 0x1: portaup(ch, aud); return;
    case 0x2: portadown(ch, aud); return;
    case 0x3: toneportamento(ch, aud); return;
    case 0x4: vibrato(ch, aud); return;
    case 0x5: toneplusvolslide(ch, aud); return;
    case 0x6: vibratoplusvolslide(ch, aud); return;
    case 0xe: e_commands(ch, aud); return;
    }
    aud->ac_per = ch->period;           /* SetBack */
    if (cmd == 7)
        tremolo(ch, aud);
    else if (cmd == 0xa)
        volumeslide(ch, aud);
    else if (cmd == 8)
        playdata.TrkEvt = ch->n.f.cmdlo;       /* nor_TrackEvent */
}

/* mt_CheckMoreEfx: the effects of the row's tick */
static void checkmoreefx(struct chan *ch, aud_t *aud)
{
    updatefunk(ch);
    switch (ch->n.f.cmd & 0x0f) {
    case 0x9: sampleoffset(ch); return;
    case 0xb: positionjump(ch); return;
    case 0xd: patternbreak(ch); return;
    case 0xe: e_commands(ch, aud); return;
    case 0xf: setspeed(ch); return;
    case 0xc: volumechange(ch, aud); return;
    }
    pernop(ch, aud);
}

/* mt_SetPeriod: BUG(orig) a note below 113 takes entry 37 (#481) */
static void setperiod(struct chan *ch, aud_t *aud)
{
    const UWORD *table = playdata.PeriodTable;
    UWORD note = ch->n.f.note & 0x0fff, idx;

    LAUNDER(table);
    for (idx = 0; idx < 37; idx++)
        if (note >= table[idx])
            break;
    ch->period = periods(ch->finetune)[idx];
    if ((CMDW(ch) & 0x0ff0) == 0x0ed0) {        /* note delay */
        checkmoreefx(ch, aud);
        return;
    }
    if (!(ch->wavecontrol & 4))
        ch->vibratopos = 0;
    if (!(ch->wavecontrol & 0x40))
        ch->tremolopos = 0;
    aud->ac_ptr = (UWORD *)ch->start;           /* mt_trenoc */
    aud->ac_len = ch->length;
    playdata.DMACONtemp |= ch->dmabit;
    checkmoreefx(ch, aud);
}

/* mt_PlayVoice: the voice's note at pattern data + d1 */
static void playvoice(struct chan *ch, aud_t *aud, const UBYTE *samples, const UBYTE *note)
{
    UWORD sample;

    if (!ch->n.l)
        pernop(ch, aud);
    ch->n.l = LONGAT(note);
    sample = ((ch->n.f.cmd & 0xf0) >> 4) | ((ch->n.f.note >> 8) & 0xf0);
    if (sample) {
        UBYTE *const *starts = playdata.SampleStarts;
        const UBYTE *h;
        UWORD repeat;

        LAUNDER(starts);
        h = samples + bl_mulu_w(sample, 30);
        ch->start = starts[sample - 1]; /* > 31: behind the table */
        ch->length = WORDAT(h);
        ch->reallength = WORDAT(h);
        ch->finetune = h[2];
        ch->volume = h[3];
        repeat = WORDAT(h + 4);         /* repeat */
        if (repeat) {
            ch->loopstart = ch->start + (UWORD)(repeat << 1);   /* ASL.W */
            ch->wavestart = ch->loopstart;
            ch->length = WORDAT(h + 4) + WORDAT(h + 6);
            ch->replen = WORDAT(h + 6);
        } else {
            ch->loopstart = ch->start;
            ch->wavestart = ch->start;
            ch->replen = WORDAT(h + 6);
        }
        aud->ac_vol = set_volume(ch->volume);
    }
    if (!(ch->n.f.note & 0x0fff)) {     /* mt_SetRegs: no note */
        checkmoreefx(ch, aud);
        return;
    }
    if ((CMDW(ch) & 0x0ff0) == 0x0e50) {
        ch->finetune = ch->n.f.cmdlo & 0x0f;
        setperiod(ch, aud);
        return;
    }
    switch (ch->n.f.cmd & 0x0f) {
    case 3:
    case 5:
        settoneporta(ch);
        checkmoreefx(ch, aud);
        return;
    case 9:                             /* BUG(orig): the offset twice (#473) */
        checkmoreefx(ch, aud);
        break;
    }
    setperiod(ch, aud);
}

/* mt_NextPosition */
static void nextposition(void)
{
    playdata.PatternPos = (UWORD)(playdata.PBreakPos << 4);
    playdata.PBreakPos = 0;
    playdata.PosJumpFlag = 0;
    playdata.SongPos = (playdata.SongPos + 1) & 0x7f;
    if (playdata.SongPos >= playdata.SongDataPtr[950])
        playdata.SongPos = 0;
}

/* mt_dskip ... mt_NoNewPosYet */
static void dskip(void)
{
    playdata.PatternPos += 16;
    if (playdata.PattDelTime) {
        playdata.PattDelTime2 = playdata.PattDelTime;
        playdata.PattDelTime = 0;
    }
    if (playdata.PattDelTime2 && --playdata.PattDelTime2)
        playdata.PatternPos -= 16;
    if (playdata.PBreakFlag) {
        playdata.PBreakFlag = 0;
        playdata.PatternPos = (UWORD)(playdata.PBreakPos << 4);
        playdata.PBreakPos = 0;
    }
    if (playdata.PatternPos >= 1024)
        nextposition();
    if (playdata.PosJumpFlag)
        nextposition();
}

static void nonewallchannels(void)
{
    int i;

    for (i = 0; i < 4; i++)
        checkefx(&playdata.chan[i], voice(i));
}

/* mt_GetNewNote, mt_SetDMA */
static void getnewnote(void)
{
    const UBYTE *mod = playdata.SongDataPtr;
    const UBYTE *samples = mod + 12;
    ULONG off = (ULONG)mod[952 + playdata.SongPos] << 10;
    volatile struct Custom *cu = CUSTOM;
    int i;

    off = (off & 0xffff0000UL) | (UWORD)(off + playdata.PatternPos);  /* ADD.W */
    patdata = mod + 1084;
    playdata.DMACONtemp = 0;
    for (i = 0; i < 4; i++, off += 4)
        playvoice(&playdata.chan[i], voice(i), samples, patdata + off); /* 0(A0,D1.L) */
    playsamples(set_real_mask(playdata.DMACONtemp));   /* mt_SetDMA */
    for (i = 0; i < 4; i++)
        if (libvars.master_mask & (1 << i)) {
            cu->aud[i].ac_ptr = (UWORD *)playdata.chan[i].loopstart;
            cu->aud[i].ac_len = playdata.chan[i].replen;
        }
    dskip();
}

/* mt_music: one tick (the CIA interrupt, PlayTracker) */
static void BLITZ_ROUTINE mt_music_c(void)
{
    if (!playdata.Enable)
        return;
    playdata.counter++;
    if (playdata.counter < playdata.speed) {          /* mt_NoNewNote */
        nonewallchannels();
        if (playdata.PosJumpFlag)
            nextposition();
        return;
    }
    playdata.counter = 0;
    if (playdata.PattDelTime2) {
        nonewallchannels();
        dskip();
        return;
    }
    getnewnote();
}

/* Change_Volume_Levels */
static void change_volume_levels(void)
{
    int i;

    for (i = 0; i < 4; i++)
        if (libvars.master_mask & (1 << i))
            CUSTOM->aud[i].ac_vol = set_volume(playdata.chan[i].volume);
}

/* ---------------------------------------------------------- the module */

static void freetrackermodule_c(ULONG num BL_REG(d0))
{
    struct modent *e = slot(num);

    if (!e->addr)
        return;
    if (e->flag)
        globfree((ULONG)e->addr, e->len);
    e->addr = NULL;
    e->len = 0;
    e->flag = 0;
}

/* LoadTrackerModule (a6 = whatever the program has: BUG(orig), #443) */
static ULONG BLITZ_ROUTINE loadtrkmodule_c(ULONG num BL_REG(d0), ULONG name BL_REG(d1),
                                           struct DosLibrary *dos BL_REG(a6))
{
    struct modent *e;
    LONG readlen;
    ULONG size;                         /* d0: the file size */
    ULONG result;                       /* d0: the Read result */

    thismod = num;
    fname = name;
    freetrackermodule_c(num);
    e = slot(thismod);
    lock = Lock((STRPTR)fname, -2);
    if (!lock)
        BLITZ_RUNERR(mess);
    fib = globalloc(512, 0);
    if (!fib)
        BLITZ_RUNERR(mess2);
    if (!Examine(lock, (struct FileInfoBlock *)fib))
        BLITZ_RUNERR(mess);
    size = (ULONG)((struct FileInfoBlock *)fib)->fib_Size;
    e->len = BL_SETB(size, size + 3);   /* ADD.b (BUG(orig), #442) */
    globfree(fib, 512);
    e->addr = (UBYTE *)globalloc(e->len, MEMF_CHIP);
    hand = OpenFromLock(lock);
    readlen = Read(hand, e->addr, e->len);
    e->flag = 1;
    readerr = (ULONG)readlen;
    Close(hand);
    result = readerr;
    readerr = (ULONG)-1;
    if (result == (ULONG)-1) {
        UnLock(lock);                   /* BUG(orig): OpenFromLock's lock (#444) */
        readerr = 0;
        freetrackermodule_c(thismod);
    }
    if (readerr != (ULONG)-1)
        readerr = (ULONG)IoErr();
    return readerr;
}

/* _modtochip: a module outside chip memory (address >= $200000) to chip
   memory; d0 -1 or 0 */
static ULONG modtochip(ULONG num)
{
    struct modent *e = slot(num);
    UBYTE *src = e->addr, *hdr, *dst;
    ULONG size, n;
    int i;

    if ((LONG)src < 0x200000)
        return 0;
    size = patternsize(src);
    for (i = 0, hdr = src; i < 31; i++, hdr += 30)
        size += (ULONG)WORDAT(hdr + 42) << 1;
    e->flag = 2;
    e->len = BL_SETW(size, size + 3);   /* ADD.w (BUG(orig), #434) */
    dst = (UBYTE *)globalloc(e->len, MEMF_CHIP);
    if (!dst)
        return 0;
    e->addr = dst;
    n = e->len;
    do
        *dst++ = *src++;
    while (--n);
    return (ULONG)-1;
}

static void BLITZ_ROUTINE decodemod_c(ULONG num BL_REG(d0), ULONG addr BL_REG(d1))
{
    struct modent *e = slot(num);

    e->addr = (UBYTE *)addr;
    e->len = 1;                         /* (so CheckTrackerID works) */
    e->flag = 0;
    modtochip(num);
}

static ULONG BLITZ_ROUTINE gettrklen_c(ULONG num BL_REG(d0))
{
    return slot(num)->len;
}

static ULONG BLITZ_ROUTINE gettrkloc_c(ULONG num BL_REG(d0))
{
    return (ULONG)slot(num)->addr;
}

static void BLITZ_ROUTINE freetrk_c(ULONG num BL_REG(d0))
{
    freetrackermodule_c(num);
}

/* CheckTrackerID: -1 for M.K. / M!K!, else the long at address 0 */
static ULONG BLITZ_ROUTINE checkid_c(ULONG num BL_REG(d0))
{
    struct modent *e = slot(num);
    ULONG id;

    if (!e->len)
        return peekl(0);                /* BUG(orig): MOVE.l 0,d0 (#430) */
    id = LONGAT(e->addr + 0x438);
    if (id == ID('M', '.', 'K', '.') || id == ID('M', '!', 'K', '!'))
        return (ULONG)-1;
    return peekl(0);
}

/* ModuleToChip: RTS (the argument comes back) */
static ULONG BLITZ_ROUTINE modtochip2_c(ULONG arg BL_REG(d0))
{
    return arg;
}

/* GetModuleName$: up to 20 characters of the module's name at a3 */
static ULONG BLITZ_ROUTINE getmodulename_c(ULONG num BL_REG(d0))
{
    ULONG off = bl_mulu_w((UWORD)num, 10);
    const UBYTE *name = slot(num)->addr;
    UWORD left = 19;
    ULONG len;                          /* d0: the name length */

    if (!name)
        return off;                     /* BUG(orig): no string, d0 = slot*10 (#437) */
    len = 0;
    do {
        if (!*name)
            break;
        BL_SETW(len, len + 1);          /* ADDQ #1,d0 */
        *blitz_string++ = *name++;
    } while (left-- != 0);
    *blitz_string = 0;
    return len;
}

/* StartTracker */
static ULONG BLITZ_ROUTINE starttrkmodule_c(ULONG num BL_REG(d0))
{
    struct modent *e = slot(num);

    if (!e->addr)
        return num;                     /* BUG(orig): the slot number (#451) */
    ciapart.mt_data = e->addr;
    resetciaint();                      /* _ResetCIATempo */
    if (libvars.play_type != 1)
        setciaint();
    mt_init(e->len);
    playdata.Enable = 0xff;
    return (ULONG)-1;
}

static void BLITZ_ROUTINE stoptracker_c(void)
{
    mt_end();
    playdata.Enable = 0;
}

static ULONG BLITZ_ROUTINE maxpatterns_c(ULONG num BL_REG(d0))
{
    const UBYTE *mod = slot(num)->addr;

    if (!mod)
        return 0;
    return mod[950];
}

/* ------------------------------------------------------------ the rest */

static ULONG BLITZ_ROUTINE gettrackerevent_c(ULONG old BL_REG(d0))
{
    return BL_SETB(old, playdata.TrkEvt);
}

/* CheckTrackerEvent: d0.w -1 for a new event, else d0.b the last one */
static ULONG BLITZ_ROUTINE checktrackerevent_c(ULONG old BL_REG(d0))
{
    if (playdata.LstEvt == playdata.TrkEvt)
        return BL_SETB(old, playdata.LstEvt);
    playdata.LstEvt = playdata.TrkEvt;
    return BL_SETW(old, 0xffff);
}

/* WaitTrackerEvent: WaitTOF (-270) through a6 (BUG(orig): no !libs,
   #453) */
static void BLITZ_ROUTINE waittrackerevent_c(const void *caller_a6 BL_REG(a6))
{
    checktrackerevent_c(0);
    for (;;) {
        struct bl_regs r = { 0 };
        BLITZ_BASEJSR(caller_a6, -270, r);
        if (playdata.LstEvt != playdata.TrkEvt) {
            playdata.LstEvt = playdata.TrkEvt;
            return;
        }
    }
}

static void BLITZ_ROUTINE modulepositionjump_c(ULONG pos BL_REG(d0))
{
    playdata.SongPos = (UBYTE)pos;
    playdata.PBreakPos = 0;
    playdata.PosJumpFlag = 0xff;
}

static ULONG BLITZ_ROUTINE getmoduleposition_c(ULONG old BL_REG(d0))
{
    return BL_SETB(old, playdata.SongPos);
}

static void BLITZ_ROUTINE pausemodule_c(void)
{
    if (libvars.pause_status == 0) {
        libvars.pause_status = 1;
        mt_end();
    } else if (libvars.pause_status == 1) {
        libvars.pause_status = 0;
        playdata.Enable = 0xff;
    }
}

static void BLITZ_ROUTINE change_playtype_c(void)
{
    libvars.play_type = 1;
    mt_end();
    resetciaint();
}

/* TrackerVolume: only 0 < new < 64 sets the playing notes (BUG(orig),
   #452) */
static void BLITZ_ROUTINE volume_control_c(ULONG vol BL_REG(d0))
{
    UWORD level, diff;

    if ((UWORD)vol > 0x40)
        return;
    libvars.old_volume = libvars.master_volume;
    level = libvars.old_volume;
    libvars.master_volume = (UWORD)vol;
    diff = (UWORD)(libvars.master_volume - libvars.old_volume);
    level += diff;
    if ((WORD)(level - 0x40) >= 0)      /* CMP.W #$40,d1: BMI */
        return;
    if ((WORD)level <= 0)               /* CMP.w #0,d1: BGT */
        return;
    libvars.volume_flag = diff;
    change_volume_levels();
}

static void BLITZ_ROUTINE changetrackermask_c(ULONG mask BL_REG(d0))
{
    int i;

    for (i = 0; i < 4; i++) {
        if (mask & (1 << i)) {
            libvars.master_mask |= 1 << i;
        } else {
            libvars.master_mask &= ~(1 << i);
            CUSTOM->aud[i].ac_vol = 0;
        }
    }
}

static void BLITZ_ROUTINE inittrackerlib_c(void)
{
    int i;

    if (libvars.play_type != 1)
        setciaint();                    /* the CIA interrupt */
    for (i = 0; i < 8; i++) {           /* DBNE d0 from 7: slots 0-7 */
        libvars.modtable[i].addr = NULL;
        libvars.modtable[i].len = 0;
        libvars.modtable[i].flag = 0;
    }
    libvars.master_volume = 0x40;
    libvars.master_mask |= 0x0f;
    libvars.volume_flag = 0;
}

static void BLITZ_ROUTINE finittrackerlib_c(void)
{
    stoptracker_c();
    if (libvars.play_type != 1)
        resetciaint();
}

/* Registers: the thunks keep d0-d1/a0-a1 but the result; the C code
   keeps d2-d7/a2-a6 (the original keeps all of them but its results and
   scratch registers: LoadTrackerModule d0-d3, DecodeModule d0-d5 ...). */
BLITZ_THUNK(_loadtrkmodule, loadtrkmodule_c, "d1/a0-a1");
BLITZ_THUNK(_StartTrkModule, starttrkmodule_c, "d1/a0-a1");
BLITZ_THUNK(_StopTracker, stoptracker_c, "d0-d1/a0-a1");
BLITZ_THUNK(_decodemod, decodemod_c, "d0-d1/a0-a1");
BLITZ_THUNK(_gettrklen, gettrklen_c, "d1/a0-a1");
BLITZ_THUNK(_gettrkloc, gettrkloc_c, "d1/a0-a1");
BLITZ_THUNK(_freetrackermodule, freetrk_c, "d0-d1/a0-a1");
BLITZ_THUNK(_gettrackerevent, gettrackerevent_c, "d1/a0-a1");
BLITZ_THUNK(_checktrackerevent, checktrackerevent_c, "d1/a0-a1");
BLITZ_THUNK(_waittrackerevent, waittrackerevent_c, "d0-d1/a0-a1");
BLITZ_THUNK(_checkid, checkid_c, "d1/a0-a1");
BLITZ_THUNK(_modtochip2, modtochip2_c, "d1/a0-a1");
BLITZ_THUNK(_getmodulename, getmodulename_c, "d1/a0-a1");
BLITZ_THUNK(_modulepositionjump, modulepositionjump_c, "d0-d1/a0-a1");
BLITZ_THUNK(_getmoduleposition, getmoduleposition_c, "d1/a0-a1");
BLITZ_THUNK(_pausemodule, pausemodule_c, "d0-d1/a0-a1");
BLITZ_THUNK(_change_playtype, change_playtype_c, "d0-d1/a0-a1");
BLITZ_THUNK(_Play_Tracker, mt_music_c, "d0-d1/a0-a1");
BLITZ_THUNK(mt_music, mt_music_c, "d0-d1/a0-a1");       /* is_Code */
BLITZ_THUNK(_Volume_Control, volume_control_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ChangeTrackerMask, changetrackermask_c, "d0-d1/a0-a1");
BLITZ_THUNK(_maxpatterns, maxpatterns_c, "d1/a0-a1");
BLITZ_THUNK(_inittrackerlib, inittrackerlib_c, "d0-d1/a0-a1");
BLITZ_THUNK(_finittrackerlib, finittrackerlib_c, "d0-d1/a0-a1");

/* ------------------------------------------------- error checks (_runerrors) */

BLITZ_ERRCHK_MESSAGE(msg00, "Tracker: You may only use slots 0-8.");
BLITZ_ERRCHK_MESSAGE(msg01, "Tracker: Cannot load module on top of another.");
BLITZ_ERRCHK_MESSAGE(msg02, "Tracker: Load/Decode module first.");
BLITZ_ERRCHK_MESSAGE(msg03, "Tracker: Decode to an empty slot.");

/* _slotcheck: CMP #9,d0 (word) */
static void BLITZ_ERRCHK slotcheck_c(ULONG num BL_REG(d0))
{
    if ((UWORD)num >= 9)
        BLITZ_RUNERR(msg00);
}

static void BLITZ_ERRCHK loadcheck_c(ULONG num BL_REG(d0))
{
    slotcheck_c(num);
    if (slot(num)->addr)
        BLITZ_RUNERR(msg01);
}

static void BLITZ_ERRCHK checkused_c(ULONG num BL_REG(d0))
{
    slotcheck_c(num);
    if (!slot(num)->addr)
        BLITZ_RUNERR(msg02);
}

static void BLITZ_ERRCHK decodecheck_c(ULONG num BL_REG(d0))
{
    struct modent *e;

    slotcheck_c(num);
    e = slot(num);
    if (e->flag && e->addr)
        BLITZ_RUNERR(msg03);
}

BLITZ_ERRCHK_THUNK(_loadcheck, loadcheck_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_checkused, checkused_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_decodecheck, decodecheck_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_slotcheck, slotcheck_c, "d0-d1/a0-a1");
