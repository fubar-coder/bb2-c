/*
 * medlib - command library 82: MED 3 / OctaMED modules (MMD0, MMD1).
 * LoadMedModule StartMedModule PlayMed StopMed JumpMed SetMedVolume
 * GetMedVolume GetMedNote GetMedInstr SetMedMask DecodeMedModule.
 *
 * C translation of bblibsrc/medlib ("MED 3 Module Player Library, By
 * Paul"), which contains Teijo Kinnunen's MED V3 player source with the
 * options MIDI=0 AUDDEV=0 CHECK=0 SYNTH=1 RELVOL=1 IFFMOCT=1 HOLD=1
 * PLAYMMD0=1 VBLANK=0 CIAB=0 EASY=1; binary of the CD's acidlibs
 * checked: same as the source.  No interrupt: the program calls PlayMed
 * for every timing pulse (_IntHandler), so the player state can be
 * followed with GetMedNote/GetMedInstr/GetMedVolume (each clears the
 * value it returns, "simon was here").
 *
 * Object (32 bytes, maximum 8): {MMD id, module length, module (chip
 * memory) / 0 when DecodeMedModule uses the module in place}.
 * StartMedModule relocates the module (id "MMDx" -> "RMDx", once),
 * initialises it (track volumes = master * track volume / 16, hold /
 * decay / finetune from the InstrExt) and starts it (twice, the "EASY"
 * start routine repeats it); ChanMask = 15.
 *
 * The player is plain C with structs for the module (MMD0/MMD1 header,
 * song, instruments, expansion data, synth instruments) and the track
 * structures, and the integer semantics of the original: byte counters
 * and positions that wrap (ADDQ.b on a word), DIVU/MULU/MULS as
 * instructions, the word products the original takes.  The data base "DB"
 * (track structures, block and repeat state, hold/decay/finetune tables,
 * the 16 period tables taken from the binary) has the layout of the
 * original.  Where a result depends on a register left over from other
 * code, that value is kept explicitly: the qualifier word of a line
 * (Bxx/1Dxx take the upper byte of d4 from a track before, #203) and the
 * d1 that _PlayNote leaves (1Fxx, #205) - struct left.
 * Assembler: the line routine (plr_loop0 with _PlayNote inline,
 * medlib_newline) is an assembler block in the original's form, for speed
 * (libs-performance task 10.12); otherwise the hardware (Paula registers,
 * DMACON, filter LED, VHPOSR waits) is accessed through volatile pointers.
 *
 * BUG(orig), kept (selection):
 *   - Bxx (position jump) stores the word of d4: upper byte = rest of the
 *     previous track's d4 (0 after a plain note, the soffset or 64 after
 *     _PlayNote) - the jump target may be wrong. (#203)
 *   - 1Dxx (pattern break) adds 1 to the word of d4 as Bxx stores it: with
 *     an upper byte left from a track before, the next line is far behind
 *     the block and kept at the change of the block (nxtnoclrln), so the
 *     notes are read from the memory behind it (test medlib-1dxx-word;
 *     issue to be filed).
 *   - 7xy with x = 0 jumps into the vibrato code (BEQ nonvib). (#204)
 *   - Fxx 1Fxx: the retrigger nibble is taken from d1 after the delayed
 *     note was played (playfxnote/_PlayNote change d1; only the delay
 *     tick is affected). (#205)
 *   - the synth period clamp sets d1 instead of d5 (no clamp at 113). (#206)
 *   - pushnewvals: for a channel mask with gaps it still takes tracks
 *     0..k-1 with their own bits (no values of a wrong track), so the
 *     higher channels of the mask get no follow-up values. (#208)
 *   - the line routine tests a track against ChanMask with BTST on its
 *     low byte (the track number mod 8): tracks 8-11 pass like tracks 0-3,
 *     and 03 (portamento) and FFD (period), whose track check exists only
 *     with #CHECK = 1, write trk_porttrgper (+32), trk_prevportspd (+82)
 *     and trk_prevper (+20) of such a track into the 20-byte structures
 *     behind it, also with the default mask (test medlib-tracks-8-11-cmd3;
 *     issue to be filed).
 *   - GetMedVolume clears trk_prevvol: the channel is silent until the
 *     next note. (#212)
 * Fixed (#207): after a MIDI dump in the expansion data the relocation read
 * the next module pointer from (expansion address AND $FFFF0000) - relocmdd
 * overwrote the low word of d0 - and relocated whatever long was there as a
 * further module; it reads the pointer of the expansion data now.
 * Fixed (#209): with SetMedMask bits 4-15 set, _PlayNote handled a note of
 * tracks 4-15 (20 byte structures) like one of tracks 0-3: it wrote behind
 * their structures and to the "audio registers" at the address in the next
 * structure.  SetMedMask keeps bits 0-3 only (once per call), so
 * _PlayNote's own mask test returns for tracks 4-15; nothing is added to
 * the player.
 * Fixed (#211): a finetune outside -8..7 (InstrExt or 15xx; the checks
 * were assembled only with #CHECK = 1) read pointers outside _periodtable
 * as period tables.  The value is limited where it enters: 15xx ignores
 * one outside the range (as the original's #CHECK code), and InitModule
 * takes 0 for an InstrExt value outside it (once per start); the period
 * lookups per note are unchanged.
 * Fixed (intended deviation, #210): JumpMed without a playing module wrote
 * to addresses 44-47 (mmd_pline/mmd_pseqnum of module 0 = exception vector
 * 11).  The original has no error check area; here the library has one:
 * with runtime error checking JumpMed without a playing module is the
 * runtime error "No Module Playing" (jumpmedchk); release code unchanged
 * (an invalid argument).
 * Fixed (#1061): GetMedVolume, GetMedNote and GetMedInstr loaded a3 from the
 * player's structure (MOVEA.l (a2),a3) and did not restore it; a function
 * keeps a3, the build pointer of a string expression ("V"+Str$(
 * GetMedVolume(0)) was built at the wrong place).  Here a3 is unchanged
 * (library test cases "a3 kept", bug-orig test medlib-getmedvolume-a3).
 * DIFF: a3 (string pointer) is not changed by LoadMedModule and
 * SetMedVolume (the original leaves it at object+8 / a track structure;
 * statements, nothing reads it).
 */
#include "blitzlib.h"
#include <stddef.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (C routines / aliases / thunks, defined below) */
void _initmedlib(void), _finitmedlib(void), _freemedmodule(void);
void _PlayMed(void), _StopMed(void), _JumpMed(void), _SetMedVolume(void);
void _GetMedVolume(void), _GetMedNote(void), _GetMedInstr(void), _SetMedMask(void);
void _StartMedModule(void), _cludgemedmodule(void);
void jumpmedchk(void);
void BLITZ_ROUTINE _loadmedmodule(const UBYTE *name BL_REG(d1),
                                  struct DosLibrary *DOSBase BL_REG(a6));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader {#medlib,init,0,finit,0}; error field: the error check area
   of jumpmedchk (fixed, #210) */
BLITZ_LIBHEADER(LIB_medlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_medlib, BL_IA3|BL_PD0, LIB_doslib, BL_LA6);
BLITZ_SUBS(_loadmedmodule, 0, 0);
BLITZ_NAME("LoadMedModule", "MedModule#,Name");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_medlib, BL_IA3|BL_PD0);
BLITZ_SUBS(_StartMedModule, 0, 0);
BLITZ_NAME("StartMedModule", "MedModule#");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_PlayMed, 0, 0);
BLITZ_NAME("PlayMed", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_StopMed, 0, 0);
BLITZ_NAME("StopMed", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_JumpMed, jumpmedchk, 0);        /* check: fixed #210 */
BLITZ_NAME("JumpMed", "Pattern#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_SetMedVolume, 0, 0);
BLITZ_NAME("SetMedVolume", "Volume");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_GetMedVolume, 0, 0);
BLITZ_NAME("GetMedVolume", "Channel");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_GetMedNote, 0, 0);
BLITZ_NAME("GetMedNote", "Channel");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_GetMedInstr, 0, 0);
BLITZ_NAME("GetMedInstr", "Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_SetMedMask, 0, 0);
BLITZ_NAME("SetMedMask", "Channel Mask");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_medlib, BL_IA3|BL_PD0);
BLITZ_SUBS(_cludgemedmodule, 0, 0);
BLITZ_NAME("DecodeMedModule", "MedModule#,MemoryLocation");

BLITZ_DUMTOKE(_toke, "MedModule", "");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _freemedmodule, 0, 0);
BLITZ_NULLSUB_AT(init, _initmedlib, 0, 0);
BLITZ_NULLSUB_AT(finit, _finitmedlib, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 8, 5);


/* ------------------------------------------------------------ modules */

/* MMD0/MMD1 module header; the player keeps its position in it */
struct mmd {
    UBYTE id[4];                /* "MMD0"/"MMD1"; "RMDx" once relocated */
    ULONG modlen;
    struct song *song;          /* +8 */
    UWORD psecnum, pseq;
    UBYTE **blockarr;           /* +16 */
    UBYTE mmdflags, reserved[3];
    UBYTE **smplarr;            /* +24 */
    ULONG reserved2;
    struct expdata *expdata;    /* +32 */
    ULONG reserved3;
    WORD pstate;                /* +40 */
    UWORD pblock, pline, pseqnum;
    WORD actplayline;
    UBYTE counter, extra_songs; /* +50 */
} BL_PACKED;

/* an instrument of the song */
struct instr {
    UWORD repeat, replen;       /* in words */
    UBYTE midich, midipreset;
    UBYTE svol;
    BYTE strans;
} BL_PACKED;

/* MMD0song */
struct song {
    struct instr sample[63];
    UWORD numblocks, songlen;   /* +504 */
    UBYTE playseq[256];         /* +508 */
    UWORD deftempo;
    BYTE playtransp;            /* +766 */
    UBYTE flags, flags2, tempo2;
    UBYTE trkvol[16];           /* +770 */
    UBYTE mastervol, numsamples;
} BL_PACKED;

/* expansion data, the part the player relocates and reads */
struct expdata {
    struct mmd *nextmod;
    UBYTE *exp_smp;             /* InstrExt: hold, decay, -, finetune */
    UWORD s_ext_entries, s_ext_entrsz;
    UBYTE *annotxt;
    ULONG annolen;
    UBYTE *iinfo;               /* +20 */
    UWORD i_ext_entries, i_ext_entrsz;
    ULONG jumpmask;
    UWORD *rgbtable;            /* +32 */
    UBYTE channelsplit[4];
    UBYTE *n_info;              /* +40 */
    UBYTE *songname;
    ULONG songnamelen;
    UBYTE *dumps;               /* +52 MIDI dumps */
} BL_PACKED;

/* a sample (type 0, 1-7 = IFF octaves) or a synth/hybrid instrument
   (type -1/-2) */
struct sample {
    ULONG length;
    WORD type;
} BL_PACKED;

struct synth {
    ULONG length;
    WORD type;
    UBYTE defaultdecay, reserved[3];
    UWORD rep, replen, voltbllen, wftbllen;
    UBYTE volspeed, wfspeed;    /* +18 */
    UWORD wforms;               /* +20 */
    UBYTE voltbl[128];          /* +22 */
    UBYTE wftbl[128];           /* +150 */
    UBYTE *wf[64];              /* +278: waveforms (length in words, data) */
} BL_PACKED;

#define SAMPLE(p) ((struct sample *)(p))

/* the track structure of the player: tracks 0-3 have all of it, tracks
   4-15 only the first 20 bytes (the rest belongs to the next one; their
   notes are not played, ChanMask has bits 0-3 only, #209) */
struct track {
    UBYTE prevnote, previnstr, prevvol, pad3;
    UBYTE cmd, cmdqual, pad6;
    BYTE noteoffcnt;            /* +7 */
    UBYTE inithold, initdecay;
    BYTE stransp, finetune;
    UWORD soffset;              /* +12 */
    struct instr *previnstra;   /* +14 */
    UWORD trackvol;             /* +18 */
    UWORD prevper;              /* +20 */
    volatile UBYTE *audioaddr;  /* +22: Paula's registers of the channel */
    ULONG sampleptr;            /* +26: repeat part (pushed after the start) */
    UWORD samplelen;
    UWORD porttrgper;           /* +32 */
    UBYTE vibshift, vibrspd;
    UWORD vibrsz;
    struct synth *synthptr;     /* +38 */
    UWORD arpgoffs, arpsoffs;   /* +42 */
    BYTE volxcnt, wfxcnt;
    UWORD volcmd, wfcmd;        /* +48: table positions */
    UBYTE volwait, wfwait;
    UWORD synthvibspd, wfchgspd;
    WORD perchg;                /* +58 */
    UBYTE *envptr;              /* +60 */
    UWORD synvibdep;
    const BYTE *synvibwf;       /* +66 */
    UWORD synviboffs;
    UBYTE initvolxspd, initwfxspd;  /* +72 */
    UBYTE volchgspd, prevnote2, synvol, synthtype;
    UWORD *periodtbl;           /* +78 */
    UBYTE prevportspd, pad83;
    UBYTE decay, fadespd;       /* +84 */
    UBYTE *envrestart;          /* +86 */
    BYTE envcount, pad91;
    UBYTE vibroffs, tremoffs;   /* +92 */
    UWORD tremsz;
    UBYTE tremspd;
    BYTE tempvol;               /* +97 */
} BL_PACKED;
#define T03SZ  98
#define T415SZ 20
_Static_assert(sizeof(struct track) == T03SZ, "track structure");

/* the low byte of a word field (MOVE.b to field+1) */
#define LO(w) (((UBYTE *)&(w))[1])

/* Paula's registers through audioaddr */
#define AC_PTR(r) (*(volatile ULONG *)(r))
#define AC_LEN(r) (*(volatile UWORD *)((r) + 4))
#define AC_PER(r) (*(volatile UWORD *)((r) + 6))
#define AC_VOL(r) (*(volatile UBYTE *)((r) + 9))       /* ac_vol+1 */

#define DMACON  (*(volatile UWORD *)0xdff096)
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define VHPOSRB (*(volatile UBYTE *)0xdff006)

/* ------------------------------------------------------------ variables */

static UWORD Music;
static struct mmd *UseMod;
static UWORD ChanMask;
static BPTR hand;

/* the data base "DB" of the player, in the order of the original */
static struct BL_PACKED {
    ULONG miscresbase;
    struct mmd *module;
    UWORD dmaonmsk;
    struct track t03d[4];
    UBYTE t415d[4 * T415SZ];
    UBYTE t815d[8 * T415SZ];
    struct track *trackdataptrs[16];
    UBYTE nextblock, nxtnoclrln;
    UWORD numtracks, numlines, nextblockline;
    UWORD rptline, rptcounter, blkdelay;
    UBYTE bpmcounter[2];
    ULONG bpmdiv;
    UBYTE holdvals[63], decays[63], finetunes[63], pad;
    UWORD periods[16][96];      /* 24 synth periods + 72, finetune 0..7, -8..-1 */
    UWORD *periodtable[16];     /* finetune -8 .. 7 */
    UBYTE *chipzero;
    UWORD modnum;
} db = {
#define AUDADDR(n) { .audioaddr = (volatile UBYTE *)(0xdff0a0 + 16 * (n)), .tempvol = -1 }
    .t03d = { AUDADDR(0), AUDADDR(1), AUDADDR(2), AUDADDR(3) },
#undef AUDADDR
#define T(a, n) ((struct track *)((a) + (n) * T415SZ))
    .trackdataptrs = {
        &db.t03d[0], &db.t03d[1], &db.t03d[2], &db.t03d[3],
        T(db.t415d, 0), T(db.t415d, 1), T(db.t415d, 2), T(db.t415d, 3),
        T(db.t815d, 0), T(db.t815d, 1), T(db.t815d, 2), T(db.t815d, 3),
        T(db.t815d, 4), T(db.t815d, 5), T(db.t815d, 6), T(db.t815d, 7),
    },
#undef T
    .bpmdiv = 3546895 / 2,
    .periods = {
        /* from the binary ($1C34-$2833 of ref/medlib.obj) */
        {
            3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1812,
            1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
            856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
            428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
            214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
            214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
            214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
            214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
        },
        {
            3400, 3209, 3029, 2859, 2699, 2547, 2404, 2269, 2142, 2022, 1908, 1801,
            1700, 1605, 1515, 1430, 1349, 1274, 1202, 1135, 1071, 1011, 954, 901,
            850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
            425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
            213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
            213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
            213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
            213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
        },
        {
            3376, 3187, 3008, 2839, 2680, 2529, 2387, 2253, 2127, 2007, 1895, 1788,
            1688, 1593, 1504, 1419, 1340, 1265, 1194, 1127, 1063, 1004, 947, 894,
            844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
            422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
            211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
            211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
            211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
            211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
        },
        {
            3352, 3164, 2986, 2819, 2660, 2511, 2370, 2237, 2112, 1993, 1881, 1776,
            1676, 1582, 1493, 1409, 1330, 1256, 1185, 1119, 1056, 997, 941, 888,
            838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
            419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
            209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
            209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
            209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
            209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
        },
        {
            3328, 3141, 2965, 2799, 2641, 2493, 2353, 2221, 2097, 1979, 1868, 1763,
            1664, 1571, 1482, 1399, 1321, 1247, 1177, 1111, 1048, 989, 934, 881,
            832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
            416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
            208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
            208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
            208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
            208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
        },
        {
            3304, 3119, 2944, 2778, 2622, 2475, 2336, 2205, 2081, 1965, 1854, 1750,
            1652, 1559, 1472, 1389, 1311, 1238, 1168, 1103, 1041, 982, 927, 875,
            826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
            413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
            206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
            206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
            206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
            206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
        },
        {
            3280, 3096, 2922, 2758, 2603, 2457, 2319, 2189, 2066, 1950, 1841, 1738,
            1640, 1548, 1461, 1379, 1302, 1229, 1160, 1095, 1033, 975, 920, 869,
            820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
            410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
            205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
            205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
            205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
            205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
        },
        {
            3256, 3073, 2901, 2738, 2584, 2439, 2302, 2173, 2051, 1936, 1827, 1725,
            1628, 1537, 1450, 1369, 1292, 1220, 1151, 1087, 1026, 968, 914, 862,
            814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
            407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
            204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
            204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
            204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
            204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
        },
        {
            3628, 3424, 3232, 3051, 2880, 2718, 2565, 2421, 2285, 2157, 2036, 1922,
            1814, 1712, 1616, 1525, 1440, 1359, 1283, 1211, 1143, 1079, 1018, 961,
            907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
            453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
            226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
            226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
            226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
            226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
        },
        {
            3588, 3387, 3197, 3017, 2848, 2688, 2537, 2395, 2260, 2133, 2014, 1901,
            1794, 1693, 1598, 1509, 1424, 1344, 1269, 1197, 1130, 1067, 1007, 950,
            900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
            450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
            225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
            225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
            225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
            225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
        },
        {
            3576, 3375, 3186, 3007, 2838, 2679, 2529, 2387, 2253, 2126, 2007, 1894,
            1788, 1688, 1593, 1504, 1419, 1339, 1264, 1193, 1126, 1063, 1003, 947,
            894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
            447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
            223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
            223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
            223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
            223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
        },
        {
            3548, 3349, 3161, 2984, 2816, 2658, 2509, 2368, 2235, 2110, 1991, 1879,
            1774, 1674, 1580, 1492, 1408, 1329, 1254, 1184, 1118, 1055, 996, 940,
            887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
            444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
            222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
            222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
            222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
            222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
        },
        {
            3524, 3326, 3140, 2963, 2797, 2640, 2492, 2352, 2220, 2095, 1978, 1867,
            1762, 1663, 1570, 1482, 1399, 1320, 1246, 1176, 1110, 1048, 989, 933,
            881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
            441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
            220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
            220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
            220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
            220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
        },
        {
            3500, 3304, 3118, 2943, 2778, 2622, 2475, 2336, 2205, 2081, 1964, 1854,
            1750, 1652, 1559, 1472, 1389, 1311, 1237, 1168, 1102, 1041, 982, 927,
            875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
            437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
            219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
            219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
            219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
            219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
        },
        {
            3472, 3277, 3093, 2920, 2756, 2601, 2455, 2317, 2187, 2064, 1949, 1839,
            1736, 1639, 1547, 1460, 1378, 1301, 1228, 1159, 1094, 1032, 974, 920,
            868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
            434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
            217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
            217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
            217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
            217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
        },
        {
            3448, 3254, 3072, 2899, 2737, 2583, 2438, 2301, 2172, 2050, 1935, 1827,
            1724, 1627, 1536, 1450, 1368, 1292, 1219, 1151, 1086, 1025, 968, 913,
            862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
            431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
            216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
            216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
            216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
            216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
        },
    },
    .periodtable = {
        &db.periods[8][24], &db.periods[9][24], &db.periods[10][24], &db.periods[11][24],
        &db.periods[12][24], &db.periods[13][24], &db.periods[14][24], &db.periods[15][24],
        &db.periods[0][24], &db.periods[1][24], &db.periods[2][24], &db.periods[3][24],
        &db.periods[4][24], &db.periods[5][24], &db.periods[6][24], &db.periods[7][24],
    },
};

static const BYTE sinetable[33] = {
    0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49,
    25, 0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117,
    -106, -90, -71, -49, -25, 0,
};

static const UBYTE gid_addtable[6] = { 0, 6, 12, 18, 24, 30 };
static const UBYTE gid_divtable[6] = { 31, 7, 3, 15, 63, 127 };
static const UBYTE shiftcnt[36] = {
    4, 3, 2, 1, 1, 0, 2, 2, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0,
    3, 3, 2, 2, 1, 0, 5, 4, 3, 2, 1, 0, 6, 5, 4, 3, 2, 1,
};
static const UBYTE mullencnt[36] = {
    15, 7, 3, 1, 1, 0, 3, 3, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0,
    7, 7, 3, 3, 1, 0, 31, 15, 7, 3, 1, 0, 63, 31, 15, 7, 3, 1,
};
static const UBYTE octstart[36] = {
    12, 12, 12, 12, 24, 24, 0, 12, 12, 24, 24, 36, 0, 12, 12, 24, 36, 36,
    0, 12, 12, 24, 24, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};

static const char mess[] = "Can't Open Med Module";
static const char mess2[] = "Med Library Error";

/* ---------------------------------------------------------- helpers */

/* ADDQ.b/SUBQ.b on a word register: only the low byte changes */
static UWORD addb(UWORD w, int n)
{
    return (w & 0xff00) | (UBYTE)(w + n);
}

/* the period table of a finetune: _periodtable+32 + (BYTE)(finetune * 4)
   as a byte offset (the original's MOVE.l 0(a0,d0.w)); the finetune is
   -8..7 (limited where it enters, #211) */
BL_ALWAYS_INLINE UWORD *periods_of(UBYTE finetune)
{
    return *(UWORD **)((UBYTE *)&db.periodtable[8] + (BYTE)(finetune << 2));
}

/* a period by a byte offset (the note doubled as a byte or a word) */
#define PERIOD(tbl, off) (*(UWORD *)((UBYTE *)(tbl) + (off)))

BL_ALWAYS_INLINE int mmd1(const struct mmd *m)
{
    return m->id[3] == '1';
}

/* the block of the module's position (pblock * 4 as a word) */
BL_ALWAYS_INLINE UBYTE *curblock(const struct mmd *m)
{
    return *(UBYTE **)((UBYTE *)m->blockarr + (WORD)(m->pblock << 2));
}

/* --------------------------------------------------------- relocation */

/* relocentr: a pointer of the module made absolute (if not 0) */
static void reloc(void *field, ULONG base)
{
    ULONG *p = field;

    if (*p)
        *p += base;
}

/* reloci: the samples; the waveforms of a synth/hybrid relative to it
   (wforms - 1 with DBF: 0 gives 65536) */
static void reloci(struct mmd *m, ULONG base)
{
    UBYTE **s = m->smplarr;
    UWORD k;

    if (!s)
        return;
    k = (UBYTE)(m->song->numsamples - 1);       /* DBF: 0 samples = 256 */
    do {
        struct synth *sy;

        reloc(s, base);
        sy = (struct synth *)*s++;
        if (sy && sy->type < 0) {
            UBYTE **wf = sy->wf;
            UWORD n = sy->wforms - 1;

            do
                *(ULONG *)wf++ += (ULONG)sy;
            while (n-- != 0);
        }
    } while (k-- != 0);
}

/* relocp: song, blocks, samples, expansion data */
static void relocp(struct mmd *m, ULONG base)
{
    reloc(&m->song, base);
    reloc(&m->blockarr, base);
    reloc(&m->smplarr, base);
    reloc(&m->expdata, base);
}

/* relocb: the blocks (numblocks - 1 with SUBQ.b and DBF), for MMD1 also
   the BlockInfo of each (backwards from where the first loop ended) */
static void relocb(struct mmd *m, ULONG base)
{
    UBYTE **b = m->blockarr;
    UWORD n = m->song->numblocks, k;

    if (!b)
        return;
    k = (n & 0xff00) | (UBYTE)(n - 1);
    do
        reloc(b++, base);
    while (k-- != 0);
    if (!mmd1(m))
        return;
    for (k = n; (WORD)--k >= 0; ) {             /* relocbi */
        UBYTE *block = *--b, *info;

        reloc(block + 4, base);                 /* BlockInfo */
        info = *(UBYTE **)(block + 4);
        if (!info)
            continue;
        reloc(info, base);                      /* hldata */
        reloc(info + 4, base);                  /* block name */
    }
}

/* RelocModule: the module (and the modules behind it) made absolute;
   "MMDx" -> "RMDx" once */
static void RelocModule(struct mmd *m)
{
    ULONG base = (ULONG)m;

    if (m->id[0] != 'M')
        return;
    m->id[0] = 'R';
    relocp(m, base);
    reloci(m, base);
    for (;;) {                                  /* rel_lp */
        struct expdata *e;
        ULONG next;

        relocb(m, base);
        e = m->expdata;
        if (!e)
            break;
        reloc(&e->nextmod, base);
        reloc(&e->exp_smp, base);
        reloc(&e->annotxt, base);
        reloc(&e->iinfo, base);
        reloc(&e->rgbtable, base);
        reloc(&e->n_info, base);
        reloc(&e->songname, base);
        reloc(&e->dumps, base);
        if (e->dumps) {                         /* relocmdd */
            UBYTE *p = e->dumps + 8;
            UWORD n = *(UWORD *)e->dumps;

            while (n) {                         /* mddloop */
                UBYTE *dump;

                reloc(p, base);
                dump = *(UBYTE **)p;
                p += 4;
                if (dump)                       /* relocdmp */
                    reloc(dump + 4, base);
                n--;
            }
            /* Fixed (#207): the original counted the dumps in the low
               word of the register with the expansion address and read
               the next module pointer from (address AND $FFFF0000) */
        }
        next = (ULONG)e->nextmod;               /* next module (0 -> end) */
        if (!next)
            break;
        m = (struct mmd *)next;
        relocp(m, base);
    }
}

/* ------------------------------------------------------------ channels */

/* ChannelOff: the channel's synth off and its DMA (channels 0-3); the
   number counts as a byte (ASL.b #2) */
static __attribute__((noinline, used)) void ChannelOff(UWORD ch)
{
    UBYTE off = ch << 2;
    struct track *t = *(struct track **)((UBYTE *)db.trackdataptrs
                                         + (WORD)((ch & 0xff00) | off));

    off >>= 2;
    if ((BYTE)off < 4) {
        t->synthptr = 0;
        t->synthtype = 0;
        DMACON = 1 << (off & 63);
    }
}

static __attribute__((noinline, used)) void SoundOff(void)
{
    UWORD ch = 15;

    do
        ChannelOff(ch);
    while (ch-- != 0);
    db.module = 0;
}

/* --------------------------------------------------------------- synth */

/* the envelope or vibrato waveform n of a synth (its data) */
static UBYTE *getwf(struct synth *sy, UBYTE n)
{
    return sy->wf[(BYTE)n] + 2;
}

/* synth_start2: one tick of the volume and the waveform table, the
   arpeggio and the vibrato of a synth/hybrid note; per = its period,
   returns the new one.  A table position counts from 1 (the command at
   table[pos - 1], its argument behind it) as a word whose low byte is
   counted (ADDQ.b). */
static UWORD synth(struct track *t, UWORD per)
{
    struct synth *sy = t->synthptr;
    volatile UBYTE *regs = t->audioaddr;
    const UBYTE *vt = (const UBYTE *)sy + 21, *wt = (const UBYTE *)sy + 149;
    UWORD pos;
    UBYTE c;

    if (--t->volxcnt > 0)
        goto wftable;
    t->volxcnt = t->initvolxspd;
    if (t->volchgspd) {
        BYTE v = t->volchgspd + t->synvol;

        if (v < 0)
            v = 0;
        if (v > 0x40)
            v = 0x40;
        t->synvol = v;
    }
    if (t->envptr) {                            /* the envelope */
        UBYTE *e = t->envptr;

        t->synvol = (UBYTE)(*e++ + 128) >> 2;
        if (++t->envcount < 0) {
            t->envcount = 0;
            e = t->envrestart;
        }
        t->envptr = e;
    }
    pos = t->volcmd;
    if (t->volwait && (BYTE)--t->volwait > 0)
        goto wftable;
    for (;;) {                                  /* getvolcmd */
        pos = addb(pos, 1);
        c = vt[(WORD)pos];
        if ((BYTE)c >= 0) {                     /* a volume */
            t->synvol = c;
            break;
        }
        switch (c & 0x0f) {
        case 0x0:                               /* SPD */
            t->initvolxspd = vt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x1:                               /* WAI */
            t->volwait = vt[(WORD)pos + 1];
            pos = addb(pos, 1);
            break;
        case 0x2:                               /* CHD */
            t->volchgspd = -vt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x3:                               /* CHU */
            t->volchgspd = vt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x4:                               /* EN1 */
        case 0x5:                               /* EN2: looping */
            t->envptr = getwf(sy, vt[(WORD)pos + 1]);
            t->envrestart = (c & 0x0f) == 0x5 ? t->envptr : 0;
            t->envcount = 0;
            pos = addb(pos, 1);
            continue;
        case 0x6:                               /* EN- */
            t->envptr = 0;
            continue;
        case 0xa:                               /* JWS */
            LO(t->wfcmd) = vt[(WORD)pos + 1];
            t->wfwait = 0;
            pos = addb(pos, 1);
            continue;
        case 0xb:                               /* HLT */
        case 0xf:                               /* END */
            pos = addb(pos, -1);
            break;
        case 0xe:                               /* JMP */
            pos = (pos & 0xff00) | vt[(WORD)pos + 1];
            continue;
        default:                                /* 7 8 9 C D */
            break;
        }
        break;
    }
    t->volcmd = pos;                            /* endvol */

wftable:
    t->prevvol = (UWORD)bl_mulu_w(t->synvol, t->trackvol) >> 8;     /* RELVOL */
    if (--t->wfxcnt > 0)
        goto arpeggio;
    t->wfxcnt = t->initwfxspd;
    pos = t->wfcmd;
    if (t->wfchgspd)
        t->perchg += t->wfchgspd;
    if (t->wfwait && --t->wfwait != 0)
        goto arpeggio;
    for (;;) {                                  /* getwfcmd */
        pos = addb(pos, 1);
        c = wt[(WORD)pos];
        if ((BYTE)c >= 0) {                     /* a waveform */
            UBYTE *w = sy->wf[c];

            AC_LEN(regs) = *(UWORD *)w;
            AC_PTR(regs) = (ULONG)(w + 2);
            break;
        }
        switch (c & 0x0f) {
        case 0x0:                               /* SPD */
            t->initwfxspd = wt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x1:                               /* WAI */
            t->wfwait = wt[(WORD)pos + 1];
            pos = addb(pos, 1);
            break;
        case 0x2:                               /* CHD */
            t->wfchgspd = wt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x3:                               /* CHU */
            t->wfchgspd = (WORD)(BYTE)-wt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x4:                               /* VBD */
            LO(t->synvibdep) = wt[(WORD)pos + 1];
            pos = addb(pos, 1);
            continue;
        case 0x5:                               /* VBS */
            LO(t->synthvibspd) = wt[(WORD)pos + 1] + 1;
            pos = addb(pos, 1);
            continue;
        case 0x6:                               /* RES */
            t->perchg = 0;
            per = t->prevper;
            continue;
        case 0x7:                               /* VWF */
            t->synvibwf = (const BYTE *)getwf(sy, wt[(WORD)pos + 1]);
            pos = addb(pos, 1);
            continue;
        case 0xa:                               /* JVS */
            LO(t->volcmd) = wt[(WORD)pos + 1];
            t->volwait = 0;
            pos = addb(pos, 1);
            continue;
        case 0xb:                               /* HLT */
        case 0xf:                               /* END */
            pos = addb(pos, -1);
            break;
        case 0xc:                               /* ARP: up to its end */
            t->arpsoffs = pos;
            t->arpgoffs = pos;
            do
                pos = addb(pos, 1);
            while ((BYTE)wt[(WORD)pos] >= 0);
            continue;
        case 0xd:                               /* ARE */
            continue;
        case 0xe:                               /* JMP */
            pos = (pos & 0xff00) | wt[(WORD)pos + 1];
            continue;
        default:                                /* 8 9 */
            break;
        }
        break;
    }
    t->wfcmd = pos;                             /* wfend */

arpeggio:
    pos = t->arpgoffs;
    if (pos) {
        UBYTE n = wt[(WORD)pos + 1] + t->prevnote2;

        per = t->periodtbl[n];
        pos = addb(pos, 1);
        if ((BYTE)wt[(WORD)pos + 1] < 0)
            pos = t->arpsoffs;
        t->arpgoffs = pos;
    }
    if (t->synvibdep) {                         /* synth_vibrato */
        BYTE v = t->synvibwf[(t->synviboffs >> 4) & 0x1f];

        per += (WORD)bl_muls_w(v, t->synvibdep) >> 8;
        t->synviboffs += t->synthvibspd;
    }
    per += t->perchg;
    /* BUG(orig) (#206): the limit at 113 sets d1, not the period */
    return per;
}

/* ------------------------------------------------------------ PlayNote */

/* The player state in the fixed registers of the libraries (user decision
   2026-11-15, libs-performance task 10.10): during a pulse a3 holds the
   module, a4 its song and a5 the track at hand, as the original's
   _IntHandler keeps them in a2/a4/a5 (its _PlayMed saves a4-a6).  GCC has
   only a0-a2/a6 for addresses, so as locals they were spilled and
   reloaded; as the global register variables blitz_string, blitz_locals
   and blitz_globals GCC addresses them directly (d16(a5)).  _PlayMed's
   thunk saves and restores a3-a5.  Nothing a pulse runs relies on their
   Blitz meaning: the player makes no ALibJsr, no OS call and no string
   work, its functions (playnote, synth, playfxnote, cmd_F, ChannelOff,
   SoundOff) take nothing from a3-a5 but this state; an interrupt during
   PlayMed finds the program's registers as it would in the original
   (an interrupt server gets a5 from exec, a Blitz handler takes the
   program's a5 from intlib's newint and loads a3 at each string
   expression). */
#define PM ((struct mmd *)blitz_string)         /* a3: the module */
#define PS ((struct song *)blitz_locals)        /* a4: its song */
#define PT ((struct track *)blitz_globals)      /* a5: the track */

/* what _PlayNote leaves in d1 and d4: d4 (soffset, 64 after a synth note)
   is the upper byte of a Bxx/1Dxx qualifier word of a later track (#203),
   d1 the retrigger count of 1Fxx after the note delay (#205) */
struct left {
    ULONG d1, d4;
};

/* _PlayNote: note (0-based instrument instr, its song entry ins) on the
   track ch (its structure at a5, PT): sample, IFF octave sample, synth or
   hybrid.  For the replays of the ticks (playfxnote); the line routine
   has its own copy inline (medlib_newline) */
static struct left playnote(UWORD ch, ULONG note, UWORD instr, struct instr *ins)
{
    struct left r = { note, 0 };
    ULONG mask, sp, start, end, rep, repend, len;
    UBYTE n;
    UWORD per;

    if (!ins)
        return r;
    mask = 1UL << (ch & 31);                    /* the mask made once */
    if (!(ChanMask & mask))
        return r;
    sp = *(ULONG *)((UBYTE *)PM->smplarr + (WORD)(instr << 2));
    n = note + PS->playtransp + ins->strans;
    if ((BYTE)ch < 4) {
        if (!sp || (BYTE)PT->synthtype <= 0 || SAMPLE(sp)->type != -1)
            DMACON = (UWORD)mask;               /* stpdma */
        PT->synthptr = 0;
    }
    n--;
    PT->fadespd = 0;
    PT->decay = PT->initdecay;
    PT->vibroffs = 0;
    PT->tremoffs = 0;
    db.dmaonmsk |= (UWORD)mask;
    if (SAMPLE(sp)->type < 0)
        goto synthnote;
    PT->synthtype = 0;
sample:
    /* the empty asm hides n from GCC's loop analysis: else it computes the
       number of passes with a DIVU (about 250 cycles) instead of adding
       12 per pass (22 cycles, as the original) */
    while ((BYTE)n < 0) {
        n += 12;
        __asm("" : "+d"(n));
    }
    if ((BYTE)n > 62)
        n -= 12;
    if (SAMPLE(sp)->type) {                     /* IFF octaves */
        ULONG q = bl_divu_w(n, 12);
        WORD type = SAMPLE(sp)->type;
        UBYTE oct = (WORD)q >= 6 ? 5 : q, idx, sh;

        if (type > 6)
            type = 6;
        idx = oct + gid_addtable[type - 1];
        len = bl_divu_w(SAMPLE(sp)->length, gid_divtable[type - 1]) & 0xffff;
        sh = shiftcnt[idx];
        rep = (UWORD)(ins->repeat << sh);
        repend = (UWORD)(ins->replen << sh);
        start = sp + 6 + bl_mulu_w(len, mullencnt[idx]);
        end = (UWORD)(len << sh) + start;
        PT->periodtbl = periods_of(PT->finetune);
        per = PERIOD(PT->periodtbl, (UBYTE)((UBYTE)((q >> 16) + octstart[idx]) * 2));
    } else {
        UWORD nn = n;                           /* 0-62 */

        PT->periodtbl = periods_of(PT->finetune);
        per = PERIOD(PT->periodtbl, nn + nn);
        start = sp + 6;
        end = SAMPLE(sp)->length + start;
        rep = ins->repeat;
        repend = ins->replen;
    }
    rep += rep;                                 /* gid_setrept */
    rep += start;
    if ((UWORD)repend > 1) {
        repend += repend;
        repend += rep;
    } else
        repend = 0;
    r.d4 = PT->soffset;                          /* gid_cont */
    start += r.d4;
    if (!(end > start))
        start -= r.d4;
    AC_PTR(PT->audioaddr) = start;
    if (repend > start) {                       /* rRepeat */
        PT->sampleptr = rep;
        r.d1 = len = (repend - start) >> 1;
        AC_LEN(PT->audioaddr) = len;
        PT->samplelen = (repend - rep) >> 1;
    } else {
        PT->sampleptr = (ULONG)db.chipzero;
        PT->samplelen = 1;
        r.d1 = len = (end - start) >> 1;
        AC_LEN(PT->audioaddr) = len;
    }
    PT->prevper = per;
    if (PT->synthtype)
        goto hsn2;
    return r;

synthnote:
    PT->prevnote2 = n;
    PT->synthptr = (struct synth *)sp;
    if (SAMPLE(sp)->type == -2) {               /* HYBRID: the sample */
        PT->synthtype = 0xff;
        sp = (ULONG)PT->synthptr->wf[0];
        goto sample;
    }
    PT->synthtype = 1;
    PT->periodtbl = periods_of(PT->finetune) - 24;
    per = PERIOD(PT->periodtbl, (UWORD)(n + n));
    r.d1 = (r.d1 & 0xffff0000UL) | per;
    PT->prevper = per;
    PT->sampleptr = 0;
hsn2:
    {
        ULONG *z = (ULONG *)&PT->arpgoffs;       /* arpgoffs .. synvibdep */
        int i;

        for (i = 0; i < 6; i++)
            *z++ = 0;
    }
    PT->synvibwf = sinetable;
    PT->synviboffs = 0;
    PT->initvolxspd = PT->synthptr->volspeed;
    PT->initwfxspd = PT->synthptr->wfspeed;
    PT->volchgspd = 0;
    if (PT->cmd == 0xe)
        LO(PT->wfcmd) = PT->cmdqual;
    r.d4 = 64;
    return r;
}

/* playfxnote: the previous note again (the hold counter continued by the
   tick counter); returns d1 (#205) */
static ULONG playfxnote(struct track *t, UWORD ch, UBYTE counter)
{
    BYTE c;

    if (!t->prevnote)
        return 0;
    c = t->noteoffcnt;
    if (c >= 0)
        c += counter;
    else {
        c = t->inithold;
        if (!c)
            c = -1;
    }
    t->noteoffcnt = c;
    return playnote(ch, t->prevnote, t->previnstr, t->previnstra).d1;
}

/* Fxx in the ticks: F1/F2 replay on tick 3, F3 on ticks 2, 4 and 6, F8/F9
   the filter */
static void cmd_F(struct track *t, UWORD ch, UBYTE q, UBYTE counter)
{
    switch (q) {
    case 0xf1:
    case 0xf2:
        if (counter == 3)
            playfxnote(t, ch, counter);
        return;
    case 0xf3:
        if (counter & (2 + 4))
            playfxnote(t, ch, counter);
        return;
    case 0xf8:
        CIAAPRA |= 2;                           /* filter off */
        return;
    case 0xf9:
        CIAAPRA &= ~2;
        return;
    }
}

/* ------------------------------------------------------------ player */

/* Wait1Lin: n + 1 raster lines (DBRA) */
BL_ALWAYS_INLINE void wait1lin(UWORD n)
{
    UBYTE b;

    do {
        b = VHPOSRB;
        while (VHPOSRB == b)
            ;
    } while (n-- != 0);
}

/* the volume slide of 5, 6, A, D: up by the high nibble, else down */
BL_ALWAYS_INLINE void volslide(struct track *t, UBYTE q)
{
    BYTE v;

    if (q & 0xf0) {
        v = t->prevvol + (q >> 4);
        if (v > 64)
            v = 64;
    } else {
        v = t->prevvol - q;
        if (v < 0)
            v = 0;
    }
    t->prevvol = v;
}

/* the instrument of the song by number (number * 8 as a word) */
BL_ALWAYS_INLINE struct instr *instrument(struct song *song, UBYTE i)
{
    return (struct instr *)((UBYTE *)song + (WORD)(i << 3));
}

/* plr_loop0 with _PlayNote inline: the notes and the commands of a new
   line, as an assembler line routine in the original's form
   (libs-performance task 10.12, design decision 25).  In C, GCC kept the
   pointer into trackdataptrs, numtracks and the period table pointer on
   the stack, addressed the data base with abs.l and zero-extended bytes
   before indexed accesses; PlayMed stayed 1.05 times the original.

   Registers as the original's _IntHandler, with the module in a3 instead
   of a2 (PM): a3 module, a4 song, a5 track (set here), a6 the data base
   db; d7 the track number, d4 the qualifier word (#203), d6 the note
   pointer and a2 the pointer into trackdataptrs (the original keeps both
   on the stack), d5 the note.  _PlayNote's instrument (the original's a3)
   is a1 until its repeat and length are in d2/d3.  newline() calls it
   with every register but a3-a5 clobbered (they are saved by _PlayMed_c's
   prologue anyway); d0 = 0 after FFE (stop), else 1.  ChannelOff and
   SoundOff are the C functions (they keep d2-d7/a2-a6, ch in d0).  The
   results are those of the C version it replaces: the same Paula writes
   and the same player state (library test systest_med). */
#define MED_OFFSETS(X)                                                      \
    X(mmd_songinfo,   struct mmd, song,        8)                           \
    X(mmd_blockarr,   struct mmd, blockarr,    16)                          \
    X(mmd_smplarr,    struct mmd, smplarr,     24)                          \
    X(mmd_pstate,     struct mmd, pstate,      40)                          \
    X(mmd_pblock,     struct mmd, pblock,      42)                          \
    X(mmd_pline,      struct mmd, pline,       44)                          \
    X(mmd_pseqnum,    struct mmd, pseqnum,     46)                          \
    X(inst_repeat,    struct instr, repeat,    0)                           \
    X(inst_replen,    struct instr, replen,    2)                           \
    X(inst_midipreset, struct instr, midipreset, 5)                         \
    X(inst_svol,      struct instr, svol,      6)                           \
    X(inst_strans,    struct instr, strans,    7)                           \
    X(msng_playtransp, struct song, playtransp, 766)                        \
    X(msng_flags,     struct song, flags,      767)                         \
    X(msng_tempo2,    struct song, tempo2,     769)                         \
    X(synth_speeds,   struct synth, volspeed,  18)                          \
    X(synth_wf,       struct synth, wf,        278)                         \
    X(trk_prevnote,   struct track, prevnote,  0)                           \
    X(trk_previnstr,  struct track, previnstr, 1)                           \
    X(trk_prevvol,    struct track, prevvol,   2)                           \
    X(trk_cmd,        struct track, cmd,       4)                           \
    X(trk_cmdqual,    struct track, cmdqual,   5)                           \
    X(trk_noteoffcnt, struct track, noteoffcnt, 7)                          \
    X(trk_inithold,   struct track, inithold,  8)                           \
    X(trk_initdecay,  struct track, initdecay, 9)                           \
    X(trk_stransp,    struct track, stransp,   10)                          \
    X(trk_finetune,   struct track, finetune,  11)                          \
    X(trk_soffset,    struct track, soffset,   12)                          \
    X(trk_previnstra, struct track, previnstra, 14)                         \
    X(trk_trackvol,   struct track, trackvol,  18)                          \
    X(trk_prevper,    struct track, prevper,   20)                          \
    X(trk_audioaddr,  struct track, audioaddr, 22)                          \
    X(trk_sampleptr,  struct track, sampleptr, 26)                          \
    X(trk_samplelen,  struct track, samplelen, 30)                          \
    X(trk_porttrgper, struct track, porttrgper, 32)                         \
    X(trk_synthptr,   struct track, synthptr,  38)                          \
    X(trk_arpgoffs,   struct track, arpgoffs,  42)                          \
    X(trk_wfcmd,      struct track, wfcmd,     50)                          \
    X(trk_synvibwf,   struct track, synvibwf,  66)                          \
    X(trk_initvolxspd, struct track, initvolxspd, 72)                       \
    X(trk_prevnote2,  struct track, prevnote2, 75)                          \
    X(trk_synthtype,  struct track, synthtype, 77)                          \
    X(trk_periodtbl,  struct track, periodtbl, 78)                          \
    X(trk_prevportspd, struct track, prevportspd, 82)                       \
    X(trk_decay,      struct track, decay,     84)                          \
    X(trk_fadespd,    struct track, fadespd,   85)                          \
    X(trk_vibroffs,   struct track, vibroffs,  92)                          \
    X(trk_tremoffs,   struct track, tremoffs,  93)                          \
    X(dmaonmsk,       __typeof__(db), dmaonmsk, 8)                          \
    X(trackdataptrs,  __typeof__(db), trackdataptrs, 642)                   \
    X(nextblock,      __typeof__(db), nextblock, 706)                       \
    X(nxtnoclrln,     __typeof__(db), nxtnoclrln, 707)                      \
    X(numtracks,      __typeof__(db), numtracks, 708)                       \
    X(numlines,       __typeof__(db), numlines, 710)                        \
    X(nextblockline,  __typeof__(db), nextblockline, 712)                   \
    X(rptline,        __typeof__(db), rptline, 714)                         \
    X(rptcounter,     __typeof__(db), rptcounter, 716)                      \
    X(holdvals,       __typeof__(db), holdvals, 726)                        \
    X(decays,         __typeof__(db), decays,  789)                         \
    X(finetunes,      __typeof__(db), finetunes, 852)                       \
    X(periodtable8,   __typeof__(db), periodtable[8], 4020)                 \
    X(chipzero,       __typeof__(db), chipzero, 4052)
#define MED_ASSERT(name, type, field, off)                                  \
    _Static_assert(offsetof(type, field) == off, #name);
#define MED_SET(name, type, field, off) "   .set " #name "," #off "\n"
MED_OFFSETS(MED_ASSERT)
_Static_assert(offsetof(struct track, initvolxspd) + 2
               == offsetof(struct track, volchgspd), "hSn2");
_Static_assert(offsetof(struct track, tremoffs)
               == offsetof(struct track, vibroffs) + 1, "vibroffs");

BLITZ_ASM_BLOCK(newline,
"   .globl  _medlib_newline\n"
MED_OFFSETS(MED_SET)
"_medlib_newline:\n"
"   lea     _db,a6\n"
"   move.w  mmd_pblock(a3),d0\n"
"   movea.l mmd_blockarr(a3),a0\n"
"   add.w   d0,d0\n"
"   add.w   d0,d0\n"
"   movea.l 0(a0,d0.w),a1\n"            /* block.. */
"   move.w  mmd_pline(a3),d0\n"
"   cmp.b   #0x31,3(a3)\n"              /* "1": MMD1 */
"   beq.s   plr_mmd1_0\n"
"   move.w  d0,d1\n"
"   add.w   d0,d0\n"
"   add.w   d1,d0\n"                    /* d0 = d0 * 3 */
"   clr.l   numtracks(a6)\n"
"   move.b  (a1)+,numtracks+1(a6)\n"
"   move.b  (a1),numlines+1(a6)\n"
"   mulu    numtracks(a6),d0\n"
"   lea     1(a1,d0.w),a1\n"
"   move.l  a1,d6\n"
"   bra.s   plr_begloop\n"
"plr_mmd1_0:\n"
"   add.w   d0,d0\n"
"   add.w   d0,d0\n"                    /* d0 = d0 * 4 */
"   mulu    (a1),d0\n"                  /* numtracks * d0 */
"   lea     8(a1,d0.l),a0\n"            /* address of the current note */
"   move.l  a0,d6\n"
"   move.w  (a1)+,numtracks(a6)\n"
"   move.w  (a1),numlines(a6)\n"
"plr_begloop:\n"
"   moveq   #0,d7\n"                    /* number of track */
"   moveq   #0,d4\n"
"   lea     trackdataptrs(a6),a2\n"
"plr_loop0:\n"
"   moveq   #0,d5\n"
"   movea.l (a2)+,a5\n"                 /* this track's structure */
/* BUG(orig) (#334): a masked track is skipped before its note is read, so
   the note pointer stays: the next track reads this track's note */
"   btst    d7,_ChanMask+1\n"
"   beq     plr_endloop0\n"
"   moveq   #0,d3\n"
"   movea.l d6,a1\n"
"   cmp.b   #0x31,3(a3)\n"
"   beq.s   plr_mmd1_1\n"
"   move.b  (a1)+,d5\n"
"   move.b  (a1)+,d1\n"
"   move.b  (a1)+,trk_cmdqual(a5)\n"
"   move.b  d1,d3\n"
"   and.w   #0x0f,d1\n"                 /* the command */
"   lsr.b   #4,d3\n"                    /* the instrument */
"   bclr    #7,d5\n"
"   beq.s   plr_bseti4\n"
"   bset    #4,d3\n"
"plr_bseti4:\n"
"   bclr    #6,d5\n"
"   beq.s   plr_nngok\n"
"   bset    #5,d3\n"
"   bra.s   plr_nngok\n"
"plr_mmd1_1:\n"
"   move.b  (a1)+,d5\n"                 /* the number of this note */
"   bpl.s   plr_nothinote\n"
"   moveq   #0,d5\n"
"plr_nothinote:\n"
"   move.b  (a1)+,d3\n"                 /* instrument number */
"   move.b  (a1)+,d1\n"                 /* command number */
"   and.w   #0x1f,d1\n"
"   move.b  (a1)+,trk_cmdqual(a5)\n"    /* qualifier */
"plr_nngok:\n"
"   move.l  a1,d6\n"
"   and.w   #0x3f,d3\n"
"   beq.s   noinstnum\n"
"   subq.b  #1,d3\n"
"   move.b  d3,trk_previnstr(a5)\n"
"   lea     holdvals(a6),a0\n"
"   adda.w  d3,a0\n"
"   move.b  (a0),trk_inithold(a5)\n"
"   move.b  decays-holdvals(a0),trk_initdecay(a5)\n"
"   move.b  finetunes-holdvals(a0),trk_finetune(a5)\n"
"   lsl.w   #3,d3\n"
"   lea     0(a4,d3.w),a0\n"            /* the instrument */
"   move.l  a0,trk_previnstra(a5)\n"
"   moveq   #0,d0\n"
"   move.b  inst_svol(a0),d0\n"         /* relative volume */
"   mulu    trk_trackvol(a5),d0\n"
"   lsr.w   #8,d0\n"
"   move.b  d0,trk_prevvol(a5)\n"
"   move.b  inst_strans(a0),trk_stransp(a5)\n"
"   clr.w   trk_soffset(a5)\n"
"noinstnum:\n"
"   move.b  d1,trk_cmd(a5)\n"
"   beq     fx\n"
"   move.b  trk_cmdqual(a5),d4\n"
"   add.b   d1,d1\n"
"   move.w  f_table(pc,d1.w),d0\n"
"   jmp     fst(pc,d0.w)\n"
"f_table:\n"
"   .word   fx-fst,fx-fst,fx-fst,f_03-fst,fx-fst,fx-fst,fx-fst,fx-fst\n"
"   .word   f_08-fst,f_09-fst,fx-fst,f_0b-fst,f_0c-fst,fx-fst,f_0e-fst,f_0f-fst\n"
"   .word   fx-fst,fx-fst,fx-fst,fx-fst,fx-fst,f_15-fst,f_16-fst,fx-fst\n"
"   .word   fx-fst,f_19-fst,fx-fst,fx-fst,f_1c-fst,f_1d-fst,fx-fst,f_1f-fst\n"
"fst:\n"
"f_0f:\n"                               /* tempo */
"   tst.b   d4\n"
"   beq.s   fx0fchgblck\n"
"   cmp.b   #0xf0,d4\n"
"   bls     fx\n"                       /* an ordinary tempo */
"   cmp.b   #0xf2,d4\n"
"   bne.s   isfxfe\n"
"f_1f:\n"                               /* FF2, 1Fxx: note delay */
"   move.b  d5,(a5)\n"                  /* trk_prevnote */
"   move.b  trk_inithold(a5),trk_noteoffcnt(a5)\n"
"   bne     plr_endloop0\n"
"   st      trk_noteoffcnt(a5)\n"
"   bra     plr_endloop0\n"
"isfxfe:\n"
"   cmp.b   #0xfe,d4\n"
"   bne.s   notcmdfe\n"
"   clr.w   mmd_pstate(a3)\n"           /* FFE: stop */
"   bsr     _SoundOff\n"
"   moveq   #0,d0\n"
"   rts\n"
/* BUG(orig) (issue to be filed): FFD and 03 of tracks 8-11 (they pass the
   mask test of plr_loop0) write behind their 20-byte structures */
"notcmdfe:\n"
"   cmp.b   #0xfd,d4\n"
"   bne.s   isfxff\n"
"   movea.l trk_periodtbl(a5),a0\n"     /* FFD: the period only */
"   subq.b  #1,d5\n"
"   bmi     plr_endloop0\n"
"   add.b   d5,d5\n"
"   move.w  0(a0,d5.w),trk_prevper(a5)\n"
"   bra     plr_endloop0\n"
"isfxff:\n"
"   cmp.b   #0xff,d4\n"
"   bne     fx\n"
"   moveq   #0,d0\n"
"   move.w  d7,d0\n"
"   bsr     _ChannelOff\n"
"   bra     fx\n"
"fx0fchgblck:\n"                        /* F00: pattern break */
"   move.b  #1,nextblock(a6)\n"
"   bra     fx\n"
"f_0e:\n"
"   cmp.b   #3,d7\n"
"   bgt     fx\n"
"   move.b  d4,trk_wfcmd+1(a5)\n"
"   bra     fx\n"
"f_0c:\n"                               /* volume */
"   moveq   #0,d0\n"
"   move.b  d4,d0\n"
"   bpl.s   plr_nosetdefvol\n"
"   and.b   #0x7f,d0\n"
"   moveq   #0,d1\n"
"   move.b  trk_previnstr(a5),d1\n"
"   lsl.w   #3,d1\n"
"   move.b  d0,inst_svol(a4,d1.w)\n"
"   bra.s   plr_setvol\n"
"plr_nosetdefvol:\n"
"   btst    #4,msng_flags(a4)\n"
"   bne.s   plr_setvol\n"
"   lsr.b   #4,d0\n"                    /* decimal */
"   mulu    #10,d0\n"
"   move.b  d4,d1\n"
"   and.b   #0x0f,d1\n"
"   add.b   d1,d0\n"
"plr_setvol:\n"
"   mulu    trk_trackvol(a5),d0\n"
"   lsr.w   #8,d0\n"
"   move.b  d0,trk_prevvol(a5)\n"
"   bra     fx\n"
"f_09:\n"
"   move.b  d4,msng_tempo2(a4)\n"
"   bra     fx\n"
"f_15:\n"                               /* Fixed (#211): -8..7, else ignored */
"   cmp.b   #7,d4\n"
"   bgt     fx\n"
"   cmp.b   #-8,d4\n"
"   blt     fx\n"
"   move.b  d4,trk_finetune(a5)\n"
"   bra     fx\n"
"f_16:\n"                               /* repeat loop */
"   tst.b   d4\n"
"   bne.s   plr_dorpt\n"
"   move.w  mmd_pline(a3),rptline(a6)\n"
"   bra     fx\n"
"plr_dorpt:\n"
"   tst.w   rptcounter(a6)\n"
"   beq.s   plr_newrpt\n"
"   subq.w  #1,rptcounter(a6)\n"
"   beq     fx\n"
"   bra.s   plr_setrptline\n"
"plr_newrpt:\n"
"   move.b  d4,rptcounter+1(a6)\n"
"plr_setrptline:\n"
"   move.w  rptline(a6),d0\n"
"   addq.w  #1,d0\n"
"   move.w  d0,nextblockline(a6)\n"
"   bra     fx\n"
"f_1c:\n"                               /* MIDI preset */
"   moveq   #0,d1\n"
"   move.b  trk_previnstr(a5),d1\n"
"   lsl.w   #3,d1\n"
"   move.b  d4,inst_midipreset(a4,d1.w)\n"
"   bra     fx\n"
"f_08:\n"                               /* hold / decay */
"   move.b  d4,d0\n"
"   lsr.b   #4,d4\n"
"   and.b   #0x0f,d0\n"
"   move.b  d4,trk_initdecay(a5)\n"
"   move.b  d0,trk_inithold(a5)\n"
"   bra     fx\n"
"f_19:\n"                               /* sample offset */
"   lsl.w   #8,d4\n"
"   move.w  d4,trk_soffset(a5)\n"
"   bra     fx\n"
/* BUG(orig) (#203): Bxx stores the word of d4 */
"f_0b:\n"
"   move.w  d4,mmd_pseqnum(a3)\n"
"   st      nextblock(a6)\n"
"   bra     fx\n"
/* BUG(orig) (issue to be filed): 1Dxx takes the word of d4 as Bxx does */
"f_1d:\n"
"   move.w  #0x1ff,nextblock(a6)\n"     /* nextblock 1, nxtnoclrln -1 */
"   addq.w  #1,d4\n"
"   move.w  d4,nextblockline(a6)\n"
"   bra     fx\n"
"f_03:\n"                               /* portamento */
"   subq.b  #1,d5\n"
"   bmi.s   plr_setfx3spd\n"
"   movea.l trk_periodtbl(a5),a0\n"
"   add.b   msng_playtransp(a4),d5\n"
"   add.b   trk_stransp(a5),d5\n"
"   bmi     plr_endloop0\n"
"   add.w   d5,d5\n"
"   move.w  0(a0,d5.w),trk_porttrgper(a5)\n"
"plr_setfx3spd:\n"
"   tst.b   d4\n"
"   beq     plr_endloop0\n"
"   move.b  d4,trk_prevportspd(a5)\n"
"   bra     plr_endloop0\n"
"fx:\n"                                 /* play a note? */
"   tst.b   d5\n"
"   beq     plr_endloop0\n"
"   move.b  d5,(a5)\n"
"   move.b  trk_inithold(a5),trk_noteoffcnt(a5)\n"
"   bne.s   _PlayNote\n"
"   st      trk_noteoffcnt(a5)\n"
/* _PlayNote: d7 track, d5 note, the instrument trk_previnstr and its song
   entry trk_previnstra; leaves d4 (soffset, 64 after a synth note) */
"_PlayNote:\n"
"   move.l  trk_previnstra(a5),d4\n"
"   beq     plr_endloop0\n"
"   movea.l d4,a1\n"
"   moveq   #0,d4\n"
"   moveq   #0,d0\n"
"   move.w  _ChanMask,d0\n"
"   btst    d7,d0\n"
"   beq     plr_endloop0\n"
"   bset    d7,d4\n"                    /* d4 is the mask of this channel */
"   move.w  d5,d1\n"                    /* the note */
"   moveq   #0,d3\n"
"   move.b  trk_previnstr(a5),d3\n"
"   add.w   d3,d3\n"
"   add.w   d3,d3\n"
"   movea.l mmd_smplarr(a3),a0\n"
"   move.l  0(a0,d3.w),d5\n"            /* the instrument's sample */
"   add.b   msng_playtransp(a4),d1\n"
"   add.b   inst_strans(a1),d1\n"
"   moveq   #0,d2\n"
"   move.w  inst_repeat(a1),d2\n"
"   moveq   #0,d3\n"
"   move.w  inst_replen(a1),d3\n"
"   cmp.b   #4,d7\n"
"   bge.s   nodmaoff\n"                 /* track >= 4: not an Amiga channel */
"   tst.l   d5\n"
"   beq.s   stpdma\n"
"   tst.b   trk_synthtype(a5)\n"
"   ble.s   stpdma\n"                   /* the previous one a sample/hybrid */
"   movea.l d5,a0\n"
"   cmp.w   #-1,4(a0)\n"                /* SYNTHETIC? */
"   beq.s   nostpdma\n"
"stpdma:\n"
"   move.w  d4,0xdff096\n"              /* this channel off */
"nostpdma:\n"
"   clr.l   trk_synthptr(a5)\n"
"nodmaoff:\n"
"   subq.b  #1,d1\n"
"   clr.b   trk_fadespd(a5)\n"
"   move.b  trk_initdecay(a5),trk_decay(a5)\n"
"   clr.w   trk_vibroffs(a5)\n"         /* and trk_tremoffs */
"   or.w    d4,dmaonmsk(a6)\n"
"   movea.l d5,a0\n"
"   tst.w   4(a0)\n"
"   bmi     handleSynthnote\n"
"   clr.b   trk_synthtype(a5)\n"
"tlwtst0:\n"
"   tst.b   d1\n"
"   bpl.s   notenot2low\n"
"   add.b   #12,d1\n"                   /* too low, an octave up */
"   bra.s   tlwtst0\n"
"notenot2low:\n"
"   cmp.b   #62,d1\n"
"   ble.s   endpttest\n"
"   sub.b   #12,d1\n"                   /* too high, an octave down */
"endpttest:\n"
"   move.w  4(a0),d0\n"
"   bne     iff5R3oct\n"
"   move.b  trk_finetune(a5),d0\n"
"   add.b   d0,d0\n"
"   add.b   d0,d0\n"
"   ext.w   d0\n"
"   lea     periodtable8(a6),a1\n"
"   movea.l 0(a1,d0.w),a1\n"            /* period table */
"   move.l  a1,trk_periodtbl(a5)\n"
"   add.b   d1,d1\n"
"   move.w  0(a1,d1.w),d5\n"            /* the period */
"   move.l  a0,d0\n"
"   addq.l  #6,d0\n"                    /* the start */
"   move.l  (a0),d1\n"
"   add.l   d0,d1\n"                    /* the end */
"gid_setrept:\n"
"   add.l   d2,d2\n"
"   add.l   d0,d2\n"                    /* the repeat start */
"   cmp.w   #1,d3\n"
"   bhi.s   gid_noreplen2\n"
"   moveq   #0,d3\n"                    /* no repeat */
"   bra.s   gid_cont\n"
"gid_noreplen2:\n"
"   add.l   d3,d3\n"
"   add.l   d2,d3\n"                    /* the repeat end */
"gid_cont:\n"
"   moveq   #0,d4\n"
"   move.w  trk_soffset(a5),d4\n"
"   add.l   d4,d0\n"
"   cmp.l   d0,d1\n"
"   bhi.s   pn_nooffsovf\n"
"   sub.l   d4,d0\n"
"pn_nooffsovf:\n"
"   movea.l trk_audioaddr(a5),a1\n"
"   move.l  d0,(a1)+\n"                 /* ac_ptr */
"   cmp.l   d0,d3\n"
"   bhi.s   rRepeat\n"
"   move.l  chipzero(a6),trk_sampleptr(a5)\n"
"   move.w  #1,trk_samplelen(a5)\n"
"   sub.l   d0,d1\n"
"   lsr.l   #1,d1\n"
"   move.w  d1,(a1)\n"                  /* ac_len */
"   bra.s   retsn1\n"
"rRepeat:\n"
"   move.l  d2,trk_sampleptr(a5)\n"
"   move.l  d3,d1\n"
"   sub.l   d0,d1\n"
"   lsr.l   #1,d1\n"
"   move.w  d1,(a1)\n"                  /* ac_len */
"   sub.l   d2,d3\n"
"   lsr.l   #1,d3\n"
"   move.w  d3,trk_samplelen(a5)\n"
"retsn1:\n"
"   move.w  d5,trk_prevper(a5)\n"
"   tst.b   trk_synthtype(a5)\n"
"   bne.s   hSn2\n"
"plr_endloop0:\n"                       /* the next track, or done */
"   addq.b  #1,d7\n"
"   cmp.w   numtracks(a6),d7\n"
"   blt     plr_loop0\n"
"   moveq   #1,d0\n"
"   rts\n"
"handleSynthnote:\n"
"   move.b  d1,trk_prevnote2(a5)\n"
"   move.l  a0,trk_synthptr(a5)\n"
"   cmp.w   #-2,4(a0)\n"                /* HYBRID? */
"   bne.s   hSn_nossn\n"
"   st      trk_synthtype(a5)\n"
"   movea.l synth_wf(a0),a0\n"          /* its waveform: the sample */
"   bra     tlwtst0\n"
"hSn_nossn:\n"
"   move.b  #1,trk_synthtype(a5)\n"
"   move.b  trk_finetune(a5),d0\n"
"   add.b   d0,d0\n"
"   add.b   d0,d0\n"
"   ext.w   d0\n"
"   lea     periodtable8(a6),a1\n"
"   movea.l 0(a1,d0.w),a1\n"
"   suba.w  #48,a1\n"
"   move.l  a1,trk_periodtbl(a5)\n"     /* for the synth periods */
"   add.w   d1,d1\n"
"   move.w  0(a1,d1.w),trk_prevper(a5)\n"
"   clr.l   trk_sampleptr(a5)\n"
"hSn2:\n"
"   lea     trk_arpgoffs(a5),a1\n"      /* arpgoffs .. synvibdep */
"   clr.l   (a1)+\n"
"   clr.l   (a1)+\n"
"   clr.l   (a1)+\n"
"   clr.l   (a1)+\n"
"   clr.l   (a1)+\n"
"   clr.l   (a1)+\n"
"   lea     _sinetable(pc),a0\n"
"   move.l  a0,(a1)+\n"                 /* synvibwf */
"   clr.w   (a1)+\n"                    /* synviboffs */
"   movea.l trk_synthptr(a5),a0\n"
"   move.w  synth_speeds(a0),(a1)+\n"   /* initvolxspd, initwfxspd */
"   clr.b   (a1)\n"                     /* volchgspd */
"   cmp.b   #0x0e,trk_cmd(a5)\n"
"   bne.s   hSn_nocmdE\n"
"   move.b  trk_cmdqual(a5),trk_wfcmd+1(a5)\n"
"hSn_nocmdE:\n"
"   moveq   #64,d4\n"
"   bra     plr_endloop0\n"
"iff5R3oct:\n"                          /* IFF octaves, note in d1 */
"   movem.l d6-d7,-(sp)\n"
"   moveq   #0,d7\n"
"   move.w  d1,d7\n"
"   divu    #12,d7\n"                   /* the octave */
"   move.l  d7,d5\n"
"   cmp.w   #6,d7\n"
"   blt.s   nohioct\n"
"   moveq   #5,d7\n"
"nohioct:\n"
"   swap    d5\n"                       /* the note in this octave */
"   move.l  (a0),d1\n"
"   cmp.w   #6,d0\n"
"   ble.s   nounrecit\n"
"   moveq   #6,d0\n"
"nounrecit:\n"
"   lea     _gid_addtable-1(pc),a1\n"
"   add.b   0(a1,d0.w),d7\n"
"   lea     _gid_divtable-1(pc),a1\n"
"   move.b  0(a1,d0.w),d0\n"
"   divu    d0,d1\n"                    /* the length of the highest octave */
"   swap    d1\n"
"   clr.w   d1\n"
"   swap    d1\n"
"   move.l  d1,d0\n"
"   moveq   #0,d6\n"
"   lea     _shiftcnt(pc),a1\n"
"   move.b  0(a1,d7.w),d6\n"
"   lsl.w   d6,d2\n"
"   lsl.w   d6,d3\n"
"   lsl.w   d6,d1\n"
"   lea     _mullencnt(pc),a1\n"
"   move.b  0(a1,d7.w),d6\n"
"   mulu    d6,d0\n"                    /* the offset of this octave */
"   add.l   a0,d0\n"
"   addq.l  #6,d0\n"
"   add.l   d0,d1\n"
"   move.b  trk_finetune(a5),d6\n"
"   add.b   d6,d6\n"
"   add.b   d6,d6\n"
"   ext.w   d6\n"
"   lea     periodtable8(a6),a1\n"
"   movea.l 0(a1,d6.w),a1\n"
"   move.l  a1,trk_periodtbl(a5)\n"
"   lea     _octstart(pc),a0\n"
"   add.b   0(a0,d7.w),d5\n"
"   add.b   d5,d5\n"
"   move.w  0(a1,d5.w),d5\n"
"   movem.l (sp)+,d6-d7\n"
"   bra     gid_setrept\n");

/* the line routine above; a3 and a4 hold the module and the song, a5 is
   left at a track (nothing reads it before the next assignment).  a3 and
   a4 are operands so that GCC reads them again after the call: as plain
   global register variables GCC kept a copy of the module and the song
   in a stack slot across the call and addressed them through it */
BL_ALWAYS_INLINE int newline(void)
{
    register int r __asm("d0");

    __asm volatile("bsr     _medlib_newline"
                   : "=d"(r), "+a"(blitz_string), "+a"(blitz_locals)
                   :
                   : "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                     "a0", "a1", "a2", "a6", "cc", "memory");
    return r;
}

/* advance the song pointer: the next line, a pattern break, the next
   position (only the low bytes of pseqnum and pblock) */
BL_ALWAYS_INLINE void nextline(void)
{
    UWORD line;

    if (db.nextblockline) {
        line = db.nextblockline - 1;
        db.nextblockline = 0;
    } else
        line = PM->pline + 1;
    if (line > db.numlines || db.nextblock) {   /* plr_chgblock */
        if (!db.nxtnoclrln)
            line = 0;
        if (PM->pstate < 0) {
            UWORD pos = PM->pseqnum;

            if ((BYTE)db.nextblock >= 0)
                pos++;
            if (!((WORD)pos < (WORD)PS->songlen))
                pos = 0;
            LO(PM->pseqnum) = pos;
            LO(PM->pblock) = PS->playseq[(WORD)pos];
        }
        db.nextblock = 0;
        db.nxtnoclrln = 0;
    }
    PM->pline = line;
}

/* hold: the notes held on into the new line (no new note, or a note with
   command 3) get the ticks of the line added */
BL_ALWAYS_INLINE void hold(void)
{
    struct track **tdp = db.trackdataptrs;
    UBYTE *block = curblock(PM), *p, tempo = PS->tempo2;
    UWORD line = PM->pline, k;

    if (!mmd1(PM)) {
        UBYTE tracks = block[0];

        p = block + 2 + (WORD)bl_mulu_w((UWORD)(line * 3), tracks);
        k = (UBYTE)(tracks - 1);
        do {
            blitz_globals = (UBYTE *)*tdp++;
            if (PT->noteoffcnt >= 0
                && (p[0] ? !(p[0] & 0x3f) || (UBYTE)((p[1] & 0x0f) - 3) == 0 : (p[1] & 0xf0)))
                PT->noteoffcnt += tempo;
            p += 3;
        } while (k-- != 0);
    } else {
        UWORD tracks = *(UWORD *)block;

        p = block + 8 + bl_mulu_w((UWORD)(line * 4), tracks);
        k = (tracks & 0xff00) | (UBYTE)(tracks - 1);
        do {
            blitz_globals = (UBYTE *)*tdp++;
            if (PT->noteoffcnt >= 0
                && (p[0] ? !(p[0] & 0x7f) || (UBYTE)(p[2] - 3) == 0 : (p[1] & 0x3f)))
                PT->noteoffcnt += tempo;
            p += 4;
        } while (k-- != 0);
    }
}

/* plr_loop1: the commands in every tick, the synth tables, the period
   and the volume to Paula (tracks 0-3) */
BL_ALWAYS_INLINE void ticks(UBYTE counter)
{
    struct track **tdp = db.trackdataptrs;
    /* ChanMask read once: in the loop GCC read it again for every track
       (the calls of synth and playfxnote may change memory); nothing a
       tick runs changes it */
    UWORD ntracks = db.numtracks, chanmask = ChanMask;
    UBYTE ch = 0;

    do {
        UBYTE cmd, q;
        UWORD per = 0;
        ULONG d1;
        BYTE v;

        blitz_globals = (UBYTE *)*tdp++;
        if (!(chanmask & (1 << (ch & 7))))
            continue;
        cmd = PT->cmd;
        q = PT->cmdqual;
        if ((WORD)ch >= 4)
            continue;
        if (PT->noteoffcnt >= 0 && --PT->noteoffcnt < 0) {   /* hold ends */
            if (PT->synthtype) {
                LO(PT->volcmd) = PT->decay;
                PT->volwait = 0;
            } else {
                PT->fadespd = PT->decay;
                if (!PT->fadespd)
                    DMACON = 1 << (ch & 31);
            }
        }
        if (PT->fadespd) {                       /* plr_nowaitoff */
            PT->prevvol -= PT->fadespd;
            if ((BYTE)PT->prevvol < 0) {
                PT->prevvol = 0;
                PT->fadespd = 0;
            }
        }
        /* plr_nofade: trk_cmd is 0-31 (newline masks it with $0F or $1F);
           the AND tells GCC so, then its jump table needs no range check
           and no extension (the original: ADD.b d6,d6 and its table) */
        switch (cmd & 0x1f) {
        case 0x01:
            if (!counter && (PS->flags & 0x20))
                break;
            PT->prevper -= q;
            per = PT->prevper;
            if ((WORD)per < 113) {
                per = 113;
                PT->prevper = per;
            }
            break;
        case 0x11:
            if (counter)
                break;
            PT->prevper -= q;
            per = PT->prevper;
            break;
        case 0x02:
            if (!counter && (PS->flags & 0x20))
                break;
            PT->prevper += q;
            per = PT->prevper;
            break;
        case 0x12:
            if (counter)
                break;
            PT->prevper += q;
            per = PT->prevper;
            break;
        case 0x00: {                            /* arpeggio */
            UBYTE n;

            if (!q)
                break;
            switch (bl_divu_w(counter, 3) >> 16) {
            case 0:
                n = (q & 0x0f) + PT->prevnote;
                break;
            case 1:
                n = (q >> 4) + PT->prevnote;
                break;
            default:
                n = PT->prevnote;
                break;
            }
            n = n - 1 + PS->playtransp + PT->stransp;
            per = PERIOD(PT->periodtbl, (UBYTE)(n + n));
            goto newtmp;
        }
        case 0x14:
            PT->vibshift = 6;
            goto vibrato;
        case 0x04:
            PT->vibshift = 5;
vibrato:
            if (counter || !q)
                goto nonvib;
            if (q & 0x0f)
                PT->vibrsz = q & 0x0f;
            if (!(q & 0xf0))
                goto nonvib;
            PT->vibrspd = ((q & 0xf0) >> 3) & 0x3e;
nonvib: {
            WORD prod = bl_muls_w(sinetable[(PT->vibroffs >> 2) & 0x1f], PT->vibrsz);
            UWORD sh = PT->vibshift & 63;

            per = sh >= 16 ? (prod < 0 ? 0xffff : 0) : (UWORD)(prod >> sh);
            per += PT->prevper;
            PT->vibroffs += PT->vibrspd;
            goto newtmp;
        }
        case 0x06:
            if (!counter && (PS->flags & 0x20))
                break;
            volslide(PT, q);
            goto nonvib;
        case 0x07:                              /* tremolo */
            if (counter || !q)
                goto nontre;
            if (q & 0x0f)
                PT->tremsz = q & 0x0f;
            if (!(q & 0xf0))
                goto nonvib;                    /* BUG(orig) (#204) */
            PT->tremspd = ((q & 0xf0) >> 2) & 0x3e;
nontre: {
            WORD prod = bl_muls_w(sinetable[(PT->tremoffs >> 3) & 0x1f], PT->tremsz);

            PT->tremoffs += PT->tremspd;
            v = PT->prevvol + (UBYTE)(prod >> 7);
            if (v < 0)
                v = 0;
            if (v > 64)
                v = 64;
            PT->tempvol = v;
            break;
        }
        case 0x0a:
        case 0x0d:
            if (!counter && (PS->flags & 0x20))
                break;
            volslide(PT, q);
            break;
        case 0x1a:
            if (counter)
                break;
            v = PT->prevvol + q;
            if (v > 64)                         /* voltest */
                v = 64;
            PT->prevvol = v;
            break;
        case 0x1b:
            if (counter)
                break;
            v = PT->prevvol - q;
            if (v < 0)                          /* voltest0 */
                v = 0;
            PT->prevvol = v;
            break;
        case 0x05:
            if (!counter && (PS->flags & 0x20))
                break;
            volslide(PT, q);
            goto portamento;
        case 0x03:
            if (!counter && (PS->flags & 0x20))
                break;
portamento: {
            UWORD target = PT->porttrgper, p = PT->prevper, spd = PT->prevportspd;

            if (!target)
                break;
            if (p > target) {                   /* subper */
                p -= spd;
                if ((WORD)p > (WORD)target)
                    goto notreached;
            } else {
                p += spd;
                if (!((WORD)p >= (WORD)target))
                    goto notreached;
            }
            p = target;                         /* targreached */
            PT->porttrgper = 0;
notreached:
            PT->prevper = p;
            per = p;
            break;
        }
        case 0x13:
            per = PT->prevper;
            if ((BYTE)counter >= 3)
                break;
            per -= q;
            break;
        case 0x1e:                              /* block delay */
            if (db.blkdelay)
                break;
            db.blkdelay = q + 1;
            break;
        case 0x18:                              /* note cut */
            if (counter != q)
                break;
            PT->prevvol = 0;
            break;
        case 0x1f:                              /* note delay / retrigger */
            d1 = q;
            if (q >> 4) {
                if ((BYTE)counter < (BYTE)(q >> 4))
                    break;
                if (counter == q >> 4)
                    d1 = playfxnote(PT, ch, counter);    /* BUG(orig) (#205) */
            }
            if (!(d1 & 0x0f))                   /* nonotedelay */
                break;
            if (!(bl_divu_w(counter, d1 & 0x0f) >> 16))
                playfxnote(PT, ch, counter);
            break;
        case 0x0f:
            cmd_F(PT, ch, q, counter);
            break;
        default:                                /* 08 09 0b 0c 0e 10 15-17 19 1c 1d */
            break;
        }
        /* fx_xx */
        per = PT->synthptr ? synth(PT, PT->prevper) : PT->prevper;
        goto tmpper;
newtmp:
        if (PT->synthptr)
            per = synth(PT, per);
tmpper:
        AC_PER(PT->audioaddr) = per;
        if (!per)
            continue;
        if (PT->tempvol >= 0) {
            AC_VOL(PT->audioaddr) = PT->tempvol;
            PT->tempvol = -1;
        } else
            AC_VOL(PT->audioaddr) = PT->prevvol;
    } while (ch = (UBYTE)(ch + 1), (WORD)ch < (WORD)ntracks);
}

/* _IntHandler: one timing pulse.  It and the routines of a pulse
   (nextline, hold, ticks and their helpers) are inline in _PlayMed, one
   function as in the original: as functions each saved its registers and
   got its arguments (libs-performance task 10.10); the line routine is
   the assembler block medlib_newline (task 10.12), playnote (for the
   replays of the ticks) and synth stay functions. */
BL_ALWAYS_INLINE void IntHandler(void)
{
    UBYTE counter;
    UWORD dma;

    if ((BYTE)db.bpmcounter[0] >= 0) {          /* SEQ: every 4th pulse */
        if ((BYTE)--db.bpmcounter[0] > 0)
            return;
        db.bpmcounter[0] = 4;
    }
    blitz_string = (UBYTE *)db.module;
    if (!PM || !PM->pstate)
        return;
    db.dmaonmsk = 0;
    blitz_locals = (UBYTE *)PM->song;
    counter = PM->counter + 1;
    if ((BYTE)counter < (BYTE)PS->tempo2) {
        PM->counter = counter;
        if (counter)
            goto fxtime;
    }
    PM->counter = 0;                             /* plr_pnewnote */
    if (!db.blkdelay || !--db.blkdelay) {
        if (!newline())
            return;
        nextline();
        hold();
    }
    counter = PM->counter;                       /* nonewnote */
fxtime:
    ticks(counter);
    dma = db.dmaonmsk;                          /* plr_endfx */
    if (!dma)
        return;
    dma = (dma & ChanMask) | 0x8000;
    wait1lin(8);
    DMACON = dma;
    wait1lin(8);
    /* pushnewvals: the repeat part.  BUG(orig) (#208): for a channel mask
       with gaps the k-th channel of the mask takes track k and bit k.  The
       mask and the DMA bits are shifted as they are used, the tracks taken
       in turn (a DBF loop over bits 0-3) */
    {
        struct track **tp = db.trackdataptrs;
        UWORD mask = ChanMask;
        WORD n = 3;

        do {
            if (mask & 1) {
                struct track *t = *tp++;
                ULONG rep = t->sampleptr;

                if ((dma & 1) && rep) {
                    volatile UBYTE *a = t->audioaddr;

                    AC_PTR(a) = rep;
                    AC_LEN(a) = t->samplelen;
                }
                dma >>= 1;
            }
            mask >>= 1;
        } while (--n != -1);
    }
}

/* ------------------------------------------------- module start / stop */

/* InitModule: track volumes = master * track volume / 16, hold, decay
   and finetune of the instruments from the InstrExt (or cleared) */
static void InitModule(struct mmd *m)
{
    struct song *song;
    struct expdata *e;
    UBYTE *h = db.holdvals;
    UWORD master;
    int i;

    if (!m)
        return;
    song = m->song;
    master = (WORD)(BYTE)song->mastervol;
    for (i = 0; i < 16; i++)
        db.trackdataptrs[i]->trackvol
            = (UWORD)bl_mulu_w(master, (WORD)(BYTE)song->trkvol[i]) >> 4;
    e = m->expdata;
    if (e && e->exp_smp && e->s_ext_entries) {
        UBYTE *x = e->exp_smp;
        UWORD n = e->s_ext_entries - 1;

        do {
            if ((WORD)e->s_ext_entrsz > 3)      /* finetune, Fixed (#211): */
                h[126] = (UBYTE)(x[3] + 8) <= 15 ? x[3] : 0;   /* -8..7, else 0 */
            h[63] = x[1];                       /* decay */
            *h++ = x[0];                        /* hold */
            x += (WORD)e->s_ext_entrsz;
        } while (n-- != 0);
        return;
    }
    for (i = 0; i < 3 * 63; i++)                /* clrhlddec */
        *h++ = 0;
}

/* _PlayModule / contpoint - clear = from the start (ST d0) */
static void PlayModule(struct mmd *m, int clear)
{
    struct song *song;
    UWORD n;

    InitModule(m);
    if (!m)
        return;
    db.module = 0;
    n = db.modnum;
    if (n) {
        do {                                    /* PM_nextmod */
            if (!m->expdata || !m->expdata->nextmod)
                break;
            m = m->expdata->nextmod;
        } while ((WORD)--n > 0);
    }
    song = m->song;                             /* PM_modfound */
    m->counter = song->tempo2;
    if (song->flags & 1)
        CIAAPRA &= ~2;
    else
        CIAAPRA |= 2;
    if (clear) {
        m->pline = 0;
        m->pseqnum = 0;
        db.rptline = 0;
        db.rptcounter = 0;
        db.blkdelay = 0;
    }
    LO(m->pblock) = ((UBYTE *)song)[(WORD)(m->pseqnum + offsetof(struct song, playseq))];
    m->pstate = -1;
    db.module = m;
    db.bpmcounter[0] = (song->flags2 & 0x20) ? 0 : 0xff;      /* SEQ */
}

static void StopPlayer(void)
{
    if (db.module) {
        db.module->pstate = 0;
        db.module = 0;
    }
    SoundOff();
}

/* --------------------------------------------------- library routines */

static void freemedmodule(ULONG *obj)
{
    struct bl_regs r = { 0 };

    if (obj[1]) {
        r.a1 = obj[2];
        r.d0 = obj[1];
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    obj[0] = obj[1] = obj[2] = 0;
}

static void BLITZ_ROUTINE _freemedmodule_c(void)
{
    freemedmodule((ULONG *)blitz_string);
}
BLITZ_THUNK(_freemedmodule, _freemedmodule_c, "d1/a0-a1");

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

/* DecodeMedModule n,address - in place if in chip memory, else a copy */
static void BLITZ_ROUTINE _cludgemedmodule_c(ULONG addr BL_REG(d1))
{
    ULONG *obj = (ULONG *)blitz_string;
    UWORD *src = (UWORD *)addr, *dst;
    ULONG n;

    freemedmodule(obj);
    obj[0] = *(ULONG *)src;
    if (*(UWORD *)obj != 0x4d4d)                /* "MM" */
        return;
    if ((addr & 0x1fffff) == addr) {            /* inchip */
        obj[1] = 0;
        obj[2] = addr;
        return;
    }
    obj[1] = ((ULONG *)src)[1];                 /* infast */
    obj[2] = globalloc(obj[1], MEMF_CHIP);
    dst = (UWORD *)obj[2];
    n = obj[1] >> 1;
    do
        *dst++ = *src++;
    while ((LONG)--n > 0);                      /* SUBQ.l #1,d0:BGT */
    if (obj[1] & 1)                             /* LSR #1,d1:BCC */
        *(UBYTE *)dst = *(UBYTE *)src;
}
BLITZ_ALIAS(_cludgemedmodule, _cludgemedmodule_c);

/* LoadMedModule n,name$ - a3 = object, d1 = name, a6 = dos: the first 8
   bytes into the object, then the module into chip memory */
void BLITZ_ROUTINE _loadmedmodule(const UBYTE *name BL_REG(d1),
                                  struct DosLibrary *DOSBase BL_REG(a6))
{
    ULONG *obj = (ULONG *)blitz_string;
    ULONG len;
    UBYTE *mem;

    freemedmodule(obj);
    hand = Open((STRPTR)name, MODE_OLDFILE);
    if (!hand) {
        BLITZ_RUNERR(mess);                     /* fileerr */
        return;
    }
    Read(hand, obj, 8);
    if (*(UWORD *)obj == 0x4d4d) {
        obj[2] = globalloc(obj[1], MEMF_CHIP);
        if (obj[2]) {
            mem = (UBYTE *)obj[2];
            ((ULONG *)mem)[0] = obj[0];
            ((ULONG *)mem)[1] = obj[1];
            len = obj[1];
            BL_SETW(len, len - 8);              /* SUBQ #8,d3 (word) */
            Read(hand, mem + 8, len);
        }
    }
    Close(hand);                                /* notmed */
}

static void BLITZ_ROUTINE _initmedlib_c(void)
{
    db.chipzero = (UBYTE *)globalloc(64, MEMF_CHIP | MEMF_CLEAR);
    Music = 0;
    UseMod = 0;
}
BLITZ_THUNK(_initmedlib, _initmedlib_c, "d1/a0-a1");

static void BLITZ_ROUTINE _StopMed_c(void)
{
    Music = 0;
    StopPlayer();                               /* _endmusic: _RemPlayer */
}
BLITZ_ALIAS(_StopMed, _StopMed_c);
BLITZ_THUNK(_finitmedlib, _StopMed_c, "d1/a0-a1");

/* StartMedModule n - relocate, init and start (twice, see head) */
static void BLITZ_ROUTINE _StartMedModule_c(void)
{
    ULONG *obj = (ULONG *)blitz_string;

    if (*(UWORD *)obj != 0x4d4d) {
        BLITZ_RUNERR(mess2);                    /* mederr */
        return;
    }
    Music = 0;
    UseMod = (struct mmd *)obj[2];
    RelocModule(UseMod);
    PlayModule(UseMod, 1);
    ChanMask = 15;
    RelocModule(UseMod);                        /* _startmusic */
    PlayModule(UseMod, 1);
    Music = 1;
}
BLITZ_ALIAS(_StartMedModule, _StartMedModule_c);

static void BLITZ_ROUTINE _PlayMed_c(void)
{
    if (Music)
        IntHandler();
}
BLITZ_THUNK(_PlayMed, _PlayMed_c, "a3-a5");     /* the player state, see PM */

/* JumpMed n - position n-1 of the song, line 0 (without a playing module
   the module pointer is 0 and it writes to address 44-47, an invalid
   argument: checked by jumpmedchk with runtime error checking, #210) */
static void BLITZ_ROUTINE _JumpMed_c(UWORD n BL_REG(d0))
{
    struct mmd *m = db.module;

    m->pseqnum = n - 1;
    m->pline = 0;
    db.nextblock = 0xff;
}
BLITZ_ALIAS(_JumpMed, _JumpMed_c);

/* SetMedVolume v - track volumes = v * track volume / 16 */
static void BLITZ_ROUTINE _SetMedVolume_c(UWORD v BL_REG(d0))
{
    UBYTE *trkvol = UseMod->song->trkvol;
    int i;

    for (i = 0; i < 16; i++) {
        UWORD tv = (WORD)(BYTE)*trkvol++;

        db.trackdataptrs[i]->trackvol = (UWORD)bl_mulu_w(tv, v) >> 4;
    }
}
BLITZ_ALIAS(_SetMedVolume, _SetMedVolume_c);

/* GetMedVolume/Note/Instr(channel) - read and clear */
static UBYTE *trackfield(UWORD ch, UBYTE *field)
{
    struct track *t = *(struct track **)((UBYTE *)db.trackdataptrs + (WORD)(UWORD)(ch << 2));

    return (UBYTE *)t + (field - (UBYTE *)&db.t03d[0]);
}

static ULONG getclear(UBYTE *p)
{
    ULONG v = *p;

    *p = 0;
    return v;
}

static ULONG BLITZ_ROUTINE _GetMedVolume_c(UWORD ch BL_REG(d0))
{
    return getclear(trackfield(ch, &db.t03d[0].prevvol));      /* #212 */
}
BLITZ_ALIAS(_GetMedVolume, _GetMedVolume_c);

static ULONG BLITZ_ROUTINE _GetMedNote_c(UWORD ch BL_REG(d0))
{
    return getclear(trackfield(ch, &db.t03d[0].prevnote));
}
BLITZ_ALIAS(_GetMedNote, _GetMedNote_c);

static ULONG BLITZ_ROUTINE _GetMedInstr_c(UWORD ch BL_REG(d0))
{
    return getclear(trackfield(ch, &db.t03d[0].previnstr));
}
BLITZ_ALIAS(_GetMedInstr, _GetMedInstr_c);

/* SetMedMask m - the channels 0-3 to play.  Fixed (#209): only bits 0-3
   are kept; the original kept the word, and a note of tracks 4-15 then
   passed _PlayNote's mask test and was played like one of tracks 0-3 */
static void BLITZ_ROUTINE _SetMedMask_c(UWORD m BL_REG(d0))
{
    ChanMask = m & 15;
}
BLITZ_ALIAS(_SetMedMask, _SetMedMask_c);

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(nomodule, "No Module Playing");        /* fixed, #210 */

/**
 * @brief The error check of JumpMed (debugger only).
 *
 * Fixed (#210), not in the original (JumpMed did not check that a module
 * plays; medlib had no error check area): without a playing module - none
 * started, or stopped by StopMed or an FFE command - the module pointer is
 * 0 and JumpMed would write mmd_pline/mmd_pseqnum at address 44-47
 * (exception vector 11).
 *
 * Errors: "No Module Playing".
 */
static void BLITZ_ERRCHK jumpmedchk_c(void)
{
    if (!db.module)
        BLITZ_RUNERR(nomodule);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(jumpmedchk, jumpmedchk_c, "d0-d1/a0-a1");
