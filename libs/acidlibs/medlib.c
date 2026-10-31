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
 * Assembler: only the hardware (Paula registers, DMACON, filter LED,
 * VHPOSR waits) is accessed through volatile pointers - no inline asm.
 *
 * BUG(orig), kept (selection):
 *   - Bxx (position jump) stores the word of d4: upper byte = rest of the
 *     previous track's d4 (0 after a plain note, the soffset or 64 after
 *     _PlayNote) - the jump target may be wrong. (#203)
 *   - 7xy with x = 0 jumps into the vibrato code (BEQ nonvib). (#204)
 *   - Fxx 1Fxx: the retrigger nibble is taken from d1 after the delayed
 *     note was played (playfxnote/_PlayNote change d1; only the delay
 *     tick is affected). (#205)
 *   - the synth period clamp sets d1 instead of d5 (no clamp at 113). (#206)
 *   - after a MIDI dump in the expansion data the relocation reads the
 *     next module pointer from (expansion address AND $FFFF0000): relocmdd
 *     overwrites the low word of d0. (#207)
 *   - pushnewvals: for a channel mask with gaps it still takes tracks
 *     0..k-1 with their own bits (no values of a wrong track), so the
 *     higher channels of the mask get no follow-up values. (#208)
 *   - tracks 4-15 (20 byte structures) are handled by _PlayNote like
 *     tracks 0-3 (writes behind their structures, hardware pointer read
 *     from the next structure); only modules with more than 4 tracks. (#209)
 *   - JumpMed without a playing module writes to addresses 44-47
 *     (mmd_pline/mmd_pseqnum of module 0 = exception vector 11). (#210)
 *   - finetune values outside -8..7 read pointers outside _periodtable. (#211)
 *   - GetMedVolume clears trk_prevvol: the channel is silent until the
 *     next note. (#212)
 * DIFF: a3 (string pointer) is not changed by LoadMedModule and
 * SetMedVolume (the original leaves it at object+8 / a track structure;
 * statements, nothing reads it).
 */
#include "blitzlib.h"
#include <stddef.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (C routines / thunks, defined below) */
void _initmedlib(void), _finitmedlib(void), _freemedmodule(void);
void _PlayMed(void), _StopMed(void), _JumpMed(void), _SetMedVolume(void);
void _GetMedVolume(void), _GetMedNote(void), _GetMedInstr(void), _SetMedMask(void);
void _StartMedModule(void), _cludgemedmodule(void);
void BLITZ_ROUTINE _loadmedmodule(const UBYTE *name BL_REG(d1),
                                  struct DosLibrary *DOSBase BL_REG(a6));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader {#medlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_medlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_medlib, 0x1380, LIB_doslib, BL_LA(6));
BLITZ_SUBS(_loadmedmodule, 0, 0);
BLITZ_NAME("LoadMedModule", "MedModule#,Name");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_medlib, 0x1380);
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
BLITZ_SUBS(_JumpMed, 0, 0);
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
BLITZ_LIBS(LIB_medlib, 0x1380);
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
   4-15 only the first 20 bytes (the rest belongs to the next one, #209) */
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

/* the period table of a finetune: _periodtable+32 + (BYTE)(finetune * 4);
   finetunes outside -8..7 read pointers outside the table (#211) */
static UWORD *periods_of(UBYTE finetune)
{
    return db.periodtable[8 + (BYTE)(finetune << 2) / 4];
}

/* a period by a byte offset (the note doubled as a byte or a word) */
#define PERIOD(tbl, off) (*(UWORD *)((UBYTE *)(tbl) + (off)))

static int mmd1(const struct mmd *m)
{
    return m->id[3] == '1';
}

/* the block of the module's position (pblock * 4 as a word) */
static UBYTE *curblock(const struct mmd *m)
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
        next = (ULONG)e;
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
            /* BUG(orig) (#207): the dump count was loaded into the low
               word of the register with the expansion address */
            next &= 0xffff0000UL;
        }
        next = *(volatile ULONG *)next;         /* next module (0 -> end) */
        if (!next)
            break;
        m = (struct mmd *)next;
        relocp(m, base);
    }
}

/* ------------------------------------------------------------ channels */

/* ChannelOff: the channel's synth off and its DMA (channels 0-3); the
   number counts as a byte (ASL.b #2) */
static void ChannelOff(UWORD ch)
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

static void SoundOff(void)
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

/* what _PlayNote leaves in d1 and d4: d4 (soffset, 64 after a synth note)
   is the upper byte of a Bxx/1Dxx qualifier word of a later track (#203),
   d1 the retrigger count of 1Fxx after the note delay (#205) */
struct left {
    ULONG d1, d4;
};

/* _PlayNote: note (0-based instrument instr, its song entry ins) on the
   track ch: sample, IFF octave sample, synth or hybrid */
static struct left playnote(struct track *t, UWORD ch, ULONG note, UWORD instr,
                            struct instr *ins)
{
    struct mmd *m = db.module;
    struct song *song = m->song;
    struct left r = { note, 0 };
    ULONG mask, sp, start, end, rep, repend, len;
    UBYTE n;
    UWORD per;

    if (!ins)
        return r;
    if (!((ULONG)ChanMask & (1UL << (ch & 31))))
        return r;
    mask = 1UL << (ch & 31);
    sp = *(ULONG *)((UBYTE *)m->smplarr + (WORD)(instr << 2));
    n = note + song->playtransp + ins->strans;
    if ((BYTE)ch < 4) {
        if (!sp || (BYTE)t->synthtype <= 0 || SAMPLE(sp)->type != -1)
            DMACON = (UWORD)mask;               /* stpdma */
        t->synthptr = 0;
    }
    n--;
    t->fadespd = 0;
    t->decay = t->initdecay;
    t->vibroffs = 0;
    t->tremoffs = 0;
    db.dmaonmsk |= (UWORD)mask;
    if (SAMPLE(sp)->type < 0)
        goto synthnote;
    t->synthtype = 0;
sample:
    while ((BYTE)n < 0)
        n += 12;
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
        t->periodtbl = periods_of(t->finetune);
        per = PERIOD(t->periodtbl, (UBYTE)((UBYTE)((q >> 16) + octstart[idx]) * 2));
    } else {
        t->periodtbl = periods_of(t->finetune);
        per = PERIOD(t->periodtbl, (UBYTE)(n + n));
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
    r.d4 = t->soffset;                          /* gid_cont */
    start += r.d4;
    if (!(end > start))
        start -= r.d4;
    AC_PTR(t->audioaddr) = start;
    if (repend > start) {                       /* rRepeat */
        t->sampleptr = rep;
        r.d1 = len = (repend - start) >> 1;
        AC_LEN(t->audioaddr) = len;
        t->samplelen = (repend - rep) >> 1;
    } else {
        t->sampleptr = (ULONG)db.chipzero;
        t->samplelen = 1;
        r.d1 = len = (end - start) >> 1;
        AC_LEN(t->audioaddr) = len;
    }
    t->prevper = per;
    if (t->synthtype)
        goto hsn2;
    return r;

synthnote:
    t->prevnote2 = n;
    t->synthptr = (struct synth *)sp;
    if (SAMPLE(sp)->type == -2) {               /* HYBRID: the sample */
        t->synthtype = 0xff;
        sp = (ULONG)t->synthptr->wf[0];
        goto sample;
    }
    t->synthtype = 1;
    t->periodtbl = periods_of(t->finetune) - 24;
    per = PERIOD(t->periodtbl, (UWORD)(n + n));
    r.d1 = (r.d1 & 0xffff0000UL) | per;
    t->prevper = per;
    t->sampleptr = 0;
hsn2:
    {
        ULONG *z = (ULONG *)&t->arpgoffs;       /* arpgoffs .. synvibdep */
        int i;

        for (i = 0; i < 6; i++)
            *z++ = 0;
    }
    t->synvibwf = sinetable;
    t->synviboffs = 0;
    t->initvolxspd = t->synthptr->volspeed;
    t->initwfxspd = t->synthptr->wfspeed;
    t->volchgspd = 0;
    if (t->cmd == 0xe)
        LO(t->wfcmd) = t->cmdqual;
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
    return playnote(t, ch, t->prevnote, t->previnstr, t->previnstra).d1;
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
static void wait1lin(UWORD n)
{
    UBYTE b;

    do {
        b = VHPOSRB;
        while (VHPOSRB == b)
            ;
    } while (n-- != 0);
}

/* the volume slide of 5, 6, A, D: up by the high nibble, else down */
static void volslide(struct track *t, UBYTE q)
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
static struct instr *instrument(struct song *song, UBYTE i)
{
    return (struct instr *)((UBYTE *)song + (WORD)(i << 3));
}

/* plr_loop0: the notes and the commands of a new line */
static int newline(struct mmd *m, struct song *song)
{
    struct track **tdp = db.trackdataptrs;
    UBYTE *block = curblock(m), *noteptr;
    UWORD line = m->pline;
    /* d4 of the original: the qualifier in the low byte, the upper byte
       left over from a track before (PlayNote, 19xx) - Bxx and 1Dxx take
       the whole word (#203) */
    UWORD qual = 0;
    UBYTE ch = 0;

    if (!mmd1(m)) {
        db.numtracks = block[0];
        db.numlines = block[1];
        noteptr = block + 2 + (WORD)bl_mulu_w((UWORD)(line * 3), db.numtracks);
    } else {
        noteptr = block + 8 + bl_mulu_w((UWORD)(line * 4), *(UWORD *)block);
        db.numtracks = *(UWORD *)block;
        db.numlines = *(UWORD *)(block + 2);
    }
    do {
        struct track *t = *tdp++;
        UBYTE note, instr, cmd, q, *p;

        /* BUG(orig) (#334): a masked track is skipped before its note is
           read, so noteptr stays: the next track reads this track's note */
        if (!(ChanMask & (1 << (ch & 7))))
            continue;
        p = noteptr;
        if (!mmd1(m)) {
            note = *p++;
            instr = *p >> 4;
            cmd = *p++ & 0x0f;
            t->cmdqual = *p++;
            if (note & 0x80) {
                note &= ~0x80;
                instr |= 0x10;
            }
            if (note & 0x40) {
                note &= ~0x40;
                instr |= 0x20;
            }
        } else {
            note = *p++;
            if ((BYTE)note < 0)
                note = 0;
            instr = *p++;
            cmd = *p++ & 0x1f;
            t->cmdqual = *p++;
        }
        noteptr = p;
        instr &= 0x3f;
        if (instr) {
            UBYTE *h = db.holdvals + --instr;
            struct instr *ins = instrument(song, instr);

            t->previnstr = instr;
            t->inithold = h[0];
            t->initdecay = h[63];
            t->finetune = h[126];
            t->previnstra = ins;
            t->prevvol = (UWORD)bl_mulu_w(ins->svol, t->trackvol) >> 8;
            t->stransp = ins->strans;
            t->soffset = 0;
        }
        t->cmd = cmd;
        if (!cmd)
            goto fx;
        q = t->cmdqual;
        qual = (qual & 0xff00) | q;
        switch (cmd) {
        case 0x0f:                              /* tempo */
            if (!q) {
                db.nextblock = 1;               /* F00: pattern break */
                break;
            }
            if (q <= 0xf0)
                break;
            if (q == 0xf2)
                goto f_1f;
            if (q == 0xfe) {                    /* FFE: stop */
                m->pstate = 0;
                SoundOff();
                return 0;
            }
            if (q == 0xfd) {                    /* FFD: period only */
                if ((BYTE)--note < 0)
                    continue;
                t->prevper = PERIOD(t->periodtbl, (UBYTE)(note + note));
                note = 0;
                break;
            }
            if (q == 0xff)
                ChannelOff(ch);
            break;
        case 0x1f:                              /* note delay */
f_1f:
            t->prevnote = note;
            t->noteoffcnt = t->inithold;
            if (!t->noteoffcnt)
                t->noteoffcnt = -1;
            continue;
        case 0x0e:
            if ((BYTE)ch <= 3)
                LO(t->wfcmd) = q;
            break;
        case 0x0c: {                            /* volume */
            UWORD v = q;

            if ((BYTE)q < 0) {
                v = q & 0x7f;
                instrument(song, t->previnstr)->svol = v;
            } else if (!(song->flags & 0x10))
                v = (UBYTE)((q >> 4) * 10 + (q & 0x0f));      /* decimal */
            t->prevvol = (UWORD)bl_mulu_w(v, t->trackvol) >> 8;
            break;
        }
        case 0x09:
            song->tempo2 = q;
            break;
        case 0x15:
            t->finetune = q;
            break;
        case 0x16:                              /* repeat loop */
            if (!q) {
                db.rptline = m->pline;
                break;
            }
            if (db.rptcounter) {
                if (!--db.rptcounter)
                    break;
            } else
                LO(db.rptcounter) = q;
            db.nextblockline = db.rptline + 1;
            break;
        case 0x1c:
            instrument(song, t->previnstr)->midipreset = q;
            break;
        case 0x08:                              /* hold / decay */
            t->initdecay = q >> 4;
            t->inithold = q & 0x0f;
            qual = (qual & 0xff00) | q >> 4;
            break;
        case 0x19:                              /* sample offset */
            qual <<= 8;
            t->soffset = qual;
            break;
        case 0x0b:                              /* BUG(orig) (#203): the word */
            m->pseqnum = qual;
            db.nextblock = 0xff;
            break;
        case 0x1d:
            db.nextblock = 1;
            db.nxtnoclrln = 0xff;
            db.nextblockline = ++qual;
            break;
        case 0x03:                              /* portamento */
            if ((BYTE)--note >= 0) {
                UBYTE n = note + song->playtransp + t->stransp;

                if ((BYTE)n < 0)
                    continue;
                t->porttrgper = PERIOD(t->periodtbl, (UWORD)(n + n));
            }
            if (q)
                t->prevportspd = q;
            continue;
        }
fx:
        if (!note)
            continue;
        t->prevnote = note;
        t->noteoffcnt = t->inithold;
        if (!t->noteoffcnt)
            t->noteoffcnt = -1;
        qual = playnote(t, ch, note, t->previnstr, t->previnstra).d4;
    } while (ch = (UBYTE)(ch + 1), (WORD)ch < (WORD)db.numtracks);
    return 1;
}

/* advance the song pointer: the next line, a pattern break, the next
   position (only the low bytes of pseqnum and pblock) */
static void nextline(struct mmd *m, struct song *song)
{
    UWORD line;

    if (db.nextblockline) {
        line = db.nextblockline - 1;
        db.nextblockline = 0;
    } else
        line = m->pline + 1;
    if (line > db.numlines || db.nextblock) {   /* plr_chgblock */
        if (!db.nxtnoclrln)
            line = 0;
        if (m->pstate < 0) {
            UWORD pos = m->pseqnum;

            if ((BYTE)db.nextblock >= 0)
                pos++;
            if (!((WORD)pos < (WORD)song->songlen))
                pos = 0;
            LO(m->pseqnum) = pos;
            LO(m->pblock) = song->playseq[(WORD)pos];
        }
        db.nextblock = 0;
        db.nxtnoclrln = 0;
    }
    m->pline = line;
}

/* hold: the notes held on into the new line (no new note, or a note with
   command 3) get the ticks of the line added */
static void hold(struct mmd *m, struct song *song)
{
    struct track **tdp = db.trackdataptrs;
    UBYTE *block = curblock(m), *p, tempo = song->tempo2;
    UWORD line = m->pline, k;

    if (!mmd1(m)) {
        UBYTE tracks = block[0];

        p = block + 2 + (WORD)bl_mulu_w((UWORD)(line * 3), tracks);
        k = (UBYTE)(tracks - 1);
        do {
            struct track *t = *tdp++;

            if (t->noteoffcnt >= 0
                && (p[0] ? !(p[0] & 0x3f) || (UBYTE)((p[1] & 0x0f) - 3) == 0 : (p[1] & 0xf0)))
                t->noteoffcnt += tempo;
            p += 3;
        } while (k-- != 0);
    } else {
        UWORD tracks = *(UWORD *)block;

        p = block + 8 + bl_mulu_w((UWORD)(line * 4), tracks);
        k = (tracks & 0xff00) | (UBYTE)(tracks - 1);
        do {
            struct track *t = *tdp++;

            if (t->noteoffcnt >= 0
                && (p[0] ? !(p[0] & 0x7f) || (UBYTE)(p[2] - 3) == 0 : (p[1] & 0x3f)))
                t->noteoffcnt += tempo;
            p += 4;
        } while (k-- != 0);
    }
}

/* plr_loop1: the commands in every tick, the synth tables, the period
   and the volume to Paula (tracks 0-3) */
static void ticks(struct song *song, UBYTE counter)
{
    struct track **tdp = db.trackdataptrs;
    UBYTE ch = 0;

    do {
        struct track *t = *tdp++;
        UBYTE cmd, q;
        UWORD per = 0;
        ULONG d1;
        BYTE v;

        if (!(ChanMask & (1 << (ch & 7))))
            continue;
        cmd = t->cmd;
        q = t->cmdqual;
        if ((WORD)ch >= 4)
            continue;
        if (t->noteoffcnt >= 0 && --t->noteoffcnt < 0) {   /* hold ends */
            if (t->synthtype) {
                LO(t->volcmd) = t->decay;
                t->volwait = 0;
            } else {
                t->fadespd = t->decay;
                if (!t->fadespd)
                    DMACON = 1 << (ch & 31);
            }
        }
        if (t->fadespd) {                       /* plr_nowaitoff */
            t->prevvol -= t->fadespd;
            if ((BYTE)t->prevvol < 0) {
                t->prevvol = 0;
                t->fadespd = 0;
            }
        }
        switch (cmd) {                          /* plr_nofade */
        case 0x01:
            if (!counter && (song->flags & 0x20))
                break;
            t->prevper -= q;
            per = t->prevper;
            if ((WORD)per < 113) {
                per = 113;
                t->prevper = per;
            }
            break;
        case 0x11:
            if (counter)
                break;
            t->prevper -= q;
            per = t->prevper;
            break;
        case 0x02:
            if (!counter && (song->flags & 0x20))
                break;
            t->prevper += q;
            per = t->prevper;
            break;
        case 0x12:
            if (counter)
                break;
            t->prevper += q;
            per = t->prevper;
            break;
        case 0x00: {                            /* arpeggio */
            UBYTE n;

            if (!q)
                break;
            switch (bl_divu_w(counter, 3) >> 16) {
            case 0:
                n = (q & 0x0f) + t->prevnote;
                break;
            case 1:
                n = (q >> 4) + t->prevnote;
                break;
            default:
                n = t->prevnote;
                break;
            }
            n = n - 1 + song->playtransp + t->stransp;
            per = PERIOD(t->periodtbl, (UBYTE)(n + n));
            goto newtmp;
        }
        case 0x14:
            t->vibshift = 6;
            goto vibrato;
        case 0x04:
            t->vibshift = 5;
vibrato:
            if (counter || !q)
                goto nonvib;
            if (q & 0x0f)
                t->vibrsz = q & 0x0f;
            if (!(q & 0xf0))
                goto nonvib;
            t->vibrspd = ((q & 0xf0) >> 3) & 0x3e;
nonvib: {
            WORD prod = bl_muls_w(sinetable[(t->vibroffs >> 2) & 0x1f], t->vibrsz);
            UWORD sh = t->vibshift & 63;

            per = sh >= 16 ? (prod < 0 ? 0xffff : 0) : (UWORD)(prod >> sh);
            per += t->prevper;
            t->vibroffs += t->vibrspd;
            goto newtmp;
        }
        case 0x06:
            if (!counter && (song->flags & 0x20))
                break;
            volslide(t, q);
            goto nonvib;
        case 0x07:                              /* tremolo */
            if (counter || !q)
                goto nontre;
            if (q & 0x0f)
                t->tremsz = q & 0x0f;
            if (!(q & 0xf0))
                goto nonvib;                    /* BUG(orig) (#204) */
            t->tremspd = ((q & 0xf0) >> 2) & 0x3e;
nontre: {
            WORD prod = bl_muls_w(sinetable[(t->tremoffs >> 3) & 0x1f], t->tremsz);

            t->tremoffs += t->tremspd;
            v = t->prevvol + (UBYTE)(prod >> 7);
            if (v < 0)
                v = 0;
            if (v > 64)
                v = 64;
            t->tempvol = v;
            break;
        }
        case 0x0a:
        case 0x0d:
            if (!counter && (song->flags & 0x20))
                break;
            volslide(t, q);
            break;
        case 0x1a:
            if (counter)
                break;
            v = t->prevvol + q;
            if (v > 64)                         /* voltest */
                v = 64;
            t->prevvol = v;
            break;
        case 0x1b:
            if (counter)
                break;
            v = t->prevvol - q;
            if (v < 0)                          /* voltest0 */
                v = 0;
            t->prevvol = v;
            break;
        case 0x05:
            if (!counter && (song->flags & 0x20))
                break;
            volslide(t, q);
            goto portamento;
        case 0x03:
            if (!counter && (song->flags & 0x20))
                break;
portamento: {
            UWORD target = t->porttrgper, p = t->prevper, spd = t->prevportspd;

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
            t->porttrgper = 0;
notreached:
            t->prevper = p;
            per = p;
            break;
        }
        case 0x13:
            per = t->prevper;
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
            t->prevvol = 0;
            break;
        case 0x1f:                              /* note delay / retrigger */
            d1 = q;
            if (q >> 4) {
                if ((BYTE)counter < (BYTE)(q >> 4))
                    break;
                if (counter == q >> 4)
                    d1 = playfxnote(t, ch, counter);    /* BUG(orig) (#205) */
            }
            if (!(d1 & 0x0f))                   /* nonotedelay */
                break;
            if (!(bl_divu_w(counter, d1 & 0x0f) >> 16))
                playfxnote(t, ch, counter);
            break;
        case 0x0f:
            cmd_F(t, ch, q, counter);
            break;
        default:                                /* 08 09 0b 0c 0e 10 15-17 19 1c 1d */
            break;
        }
        /* fx_xx */
        per = t->synthptr ? synth(t, t->prevper) : t->prevper;
        goto tmpper;
newtmp:
        if (t->synthptr)
            per = synth(t, per);
tmpper:
        AC_PER(t->audioaddr) = per;
        if (!per)
            continue;
        if (t->tempvol >= 0) {
            AC_VOL(t->audioaddr) = t->tempvol;
            t->tempvol = -1;
        } else
            AC_VOL(t->audioaddr) = t->prevvol;
    } while (ch = (UBYTE)(ch + 1), (WORD)ch < (WORD)db.numtracks);
}

/* _IntHandler: one timing pulse */
static void IntHandler(void)
{
    struct mmd *m;
    struct song *song;
    UBYTE counter;
    UWORD dma;
    int k, bit;

    if ((BYTE)db.bpmcounter[0] >= 0) {          /* SEQ: every 4th pulse */
        if ((BYTE)--db.bpmcounter[0] > 0)
            return;
        db.bpmcounter[0] = 4;
    }
    m = db.module;
    if (!m || !m->pstate)
        return;
    db.dmaonmsk = 0;
    song = m->song;
    counter = m->counter + 1;
    if ((BYTE)counter < (BYTE)song->tempo2) {
        m->counter = counter;
        if (counter)
            goto fxtime;
    }
    m->counter = 0;                             /* plr_pnewnote */
    if (!db.blkdelay || !--db.blkdelay) {
        if (!newline(m, song))
            return;
        nextline(m, song);
        hold(m, song);
    }
    counter = m->counter;                       /* nonewnote */
fxtime:
    ticks(song, counter);
    dma = db.dmaonmsk;                          /* plr_endfx */
    if (!dma)
        return;
    dma = (dma & ChanMask) | 0x8000;
    wait1lin(8);
    DMACON = dma;
    wait1lin(8);
    /* pushnewvals: the repeat part.  BUG(orig) (#208): for a channel mask
       with gaps the k-th channel of the mask takes track k and bit k */
    for (bit = 0, k = 0; bit < 4; bit++) {
        struct track *t;

        if (!(ChanMask & (1 << bit)))
            continue;
        t = db.trackdataptrs[k];
        if ((dma >> k++) & 1 && t->sampleptr) {
            AC_PTR(t->audioaddr) = t->sampleptr;
            AC_LEN(t->audioaddr) = t->samplelen;
        }
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
            if ((WORD)e->s_ext_entrsz > 3)
                h[126] = x[3];                  /* finetune */
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
BLITZ_THUNK(_cludgemedmodule, _cludgemedmodule_c, "d1/a0-a1");

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
BLITZ_THUNK(_StopMed, _StopMed_c, "d1/a0-a1");
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
BLITZ_THUNK(_StartMedModule, _StartMedModule_c, "d1/a0-a1");

static void BLITZ_ROUTINE _PlayMed_c(void)
{
    if (Music)
        IntHandler();
}
BLITZ_THUNK(_PlayMed, _PlayMed_c, "d1/a0-a1");

/* JumpMed n - position n-1 of the song, line 0 (BUG(orig) #210: without a
   playing module at address 44-47) */
static void BLITZ_ROUTINE _JumpMed_c(UWORD n BL_REG(d0))
{
    struct mmd *m = db.module;

    m->pseqnum = n - 1;
    m->pline = 0;
    db.nextblock = 0xff;
}
BLITZ_THUNK(_JumpMed, _JumpMed_c, "d1/a0-a1");

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
BLITZ_THUNK(_SetMedVolume, _SetMedVolume_c, "d1/a0-a1");

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
BLITZ_THUNK(_GetMedVolume, _GetMedVolume_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE _GetMedNote_c(UWORD ch BL_REG(d0))
{
    return getclear(trackfield(ch, &db.t03d[0].prevnote));
}
BLITZ_THUNK(_GetMedNote, _GetMedNote_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE _GetMedInstr_c(UWORD ch BL_REG(d0))
{
    return getclear(trackfield(ch, &db.t03d[0].previnstr));
}
BLITZ_THUNK(_GetMedInstr, _GetMedInstr_c, "d1/a0-a1");

static void BLITZ_ROUTINE _SetMedMask_c(UWORD m BL_REG(d0))
{
    ChanMask = m;
}
BLITZ_THUNK(_SetMedMask, _SetMedMask_c, "d0-d1/a0-a1");
