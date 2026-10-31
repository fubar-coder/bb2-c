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
 * The player is translated at register level: d0-d7 and a0-a5 are static
 * variables of this file, every instruction keeps its operand size (byte
 * and word operations change only the low part, DIVU/MULU/MULS/ASR as on
 * the 68000), the data base "DB" (track structures, block and repeat
 * state, hold/decay/finetune tables, the 16 period tables taken from the
 * binary) has the layout of the original.  This is what the original's
 * results depend on - e.g. a Bxx command stores the whole word of d4,
 * whose upper byte is left over from the previous track (#203).
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

/* ---------------------------------------------------------- constants */

/* MMD0 */
#define mmd_songinfo   8
#define mmd_blockarr   16
#define mmd_smplarr    24
#define mmd_expdata    32
#define mmd_pstate     40
#define mmd_pblock     42
#define mmd_pline      44
#define mmd_pseqnum    46
#define mmd_counter    50
/* MMD0song */
#define msng_numblocks 504
#define msng_songlen   506
#define msng_playseq   508
#define msng_playtransp 766
#define msng_flags     767
#define msng_flags2    768
#define msng_tempo2    769
#define msng_trkvol    770
#define msng_mastervol 786
#define msng_numsamples 787
/* instrument */
#define inst_repeat    0
#define inst_replen    2
#define inst_midipreset 5
#define inst_svol      6
#define inst_strans    7
/* track data */
#define trk_prevnote   0
#define trk_previnstr  1
#define trk_prevvol    2
#define trk_cmd        4
#define trk_cmdqual    5
#define trk_noteoffcnt 7
#define trk_inithold   8
#define trk_initdecay  9
#define trk_stransp    10
#define trk_finetune   11
#define trk_soffset    12
#define trk_previnstra 14
#define trk_trackvol   18
#define trk_prevper    20
#define trk_audioaddr  22
#define trk_sampleptr  26
#define trk_samplelen  30
#define trk_porttrgper 32
#define trk_vibshift   34
#define trk_vibrspd    35
#define trk_vibrsz     36
#define trk_synthptr   38
#define trk_arpgoffs   42
#define trk_arpsoffs   44
#define trk_volxcnt    46
#define trk_wfxcnt     47
#define trk_volcmd     48
#define trk_wfcmd      50
#define trk_volwait    52
#define trk_wfwait     53
#define trk_synthvibspd 54
#define trk_wfchgspd   56
#define trk_perchg     58
#define trk_envptr     60
#define trk_synvibdep  64
#define trk_synvibwf   66
#define trk_synviboffs 70
#define trk_initvolxspd 72
#define trk_initwfxspd 73
#define trk_volchgspd  74
#define trk_prevnote2  75
#define trk_synvol     76
#define trk_synthtype  77
#define trk_periodtbl  78
#define trk_prevportspd 82
#define trk_decay      84
#define trk_fadespd    85
#define trk_envrestart 86
#define trk_envcount   90
#define trk_vibroffs   92
#define trk_tremoffs   93
#define trk_tremsz     94
#define trk_tremspd    96
#define trk_tempvol    97
#define T03SZ  98
#define T415SZ 20

/* memory access with an offset (byte addresses as in the original) */
#define MB(p, o)  (*(UBYTE *)((UBYTE *)(p) + (o)))
#define MSB(p, o) (*(BYTE *)((UBYTE *)(p) + (o)))
#define MW(p, o)  (*(UWORD *)((UBYTE *)(p) + (o)))
#define ML(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define MP(p, o)  (*(UBYTE **)((UBYTE *)(p) + (o)))

/* register operations: byte / word ops change only the low part */
#define B(r)       ((UBYTE)(r))
#define SB(r)      ((BYTE)(r))
#define W(r)       ((UWORD)(r))
#define SW(r)      ((WORD)(r))
#define SETB(r, v) ((r) = ((r) & 0xffffff00UL) | (UBYTE)(v))
#define SETW(r, v) ((r) = ((r) & 0xffff0000UL) | (UWORD)(v))
#define MULU(a, b) ((ULONG)(UWORD)(a) * (UWORD)(b))
#define MULS(a, b) ((ULONG)((LONG)(WORD)(a) * (WORD)(b)))
#define SWAP(r)    ((ULONG)(r) << 16 | (ULONG)(r) >> 16)

#define DMACON  (*(volatile UWORD *)0xdff096)
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define VHPOSRB (*(volatile UBYTE *)0xdff006)

/* ------------------------------------------------------------ variables */

static UWORD Music;
static UBYTE *UseMod;
static UWORD ChanMask;
static BPTR hand;

/* the data base "DB" of the player, in the order of the original */
static struct BL_PACKED {
    ULONG miscresbase;
    UBYTE *module;
    UWORD dmaonmsk;
    UBYTE t03d[4][T03SZ];
    UBYTE t415d[4 * T415SZ];
    UBYTE t815d[8 * T415SZ];
    UBYTE *trackdataptrs[16];
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
#define AUDADDR(n) [22] = 0, [23] = 0xdf, [24] = 0xf0, [25] = 0xa0 + 16 * (n), [97] = 0xff
    .t03d = { { AUDADDR(0) }, { AUDADDR(1) }, { AUDADDR(2) }, { AUDADDR(3) } },
#undef AUDADDR
    .trackdataptrs = {
        db.t03d[0], db.t03d[1], db.t03d[2], db.t03d[3],
        db.t415d, db.t415d + T415SZ, db.t415d + 2 * T415SZ, db.t415d + 3 * T415SZ,
        db.t815d, db.t815d + T415SZ, db.t815d + 2 * T415SZ, db.t815d + 3 * T415SZ,
        db.t815d + 4 * T415SZ, db.t815d + 5 * T415SZ, db.t815d + 6 * T415SZ,
        db.t815d + 7 * T415SZ,
    },
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

#define PERIODTABLE0 ((UBYTE *)db.periodtable + 32)    /* _periodtable+32 */

static const BYTE sinetable[33] = {
    0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49,
    25, 0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117,
    -106, -90, -71, -49, -25, 0,
};

/* the jump tables of the two command dispatchers (offsets from fst / fxs,
   from the binary): MOVE.w table(pc,d6.w),d0 leaves the offset in d0 */
static const UWORD f_table[32] = {
    0x018e, 0x018e, 0x018e, 0x0162, 0x018e, 0x018e, 0x018e, 0x018e, 0x0128, 0x00d2,
    0x018e, 0x0146, 0x0092, 0x018e, 0x0082, 0x0000, 0x018e, 0x018e, 0x018e, 0x018e,
    0x018e, 0x00da, 0x00e2, 0x018e, 0x018e, 0x013c, 0x018e, 0x018e, 0x0118, 0x0152,
    0x018e, 0x001a,
};
static const UWORD fx_table[32] = {
    0x006c, 0x0000, 0x003e, 0x0228, 0x00bc, 0x0210, 0x011c, 0x0134, 0x02e2, 0x02e2,
    0x01a0, 0x02e2, 0x02e2, 0x01a0, 0x02e2, 0x02de, 0x0286, 0x002c, 0x005a, 0x0274,
    0x00b2, 0x02e2, 0x02e2, 0x02e2, 0x029c, 0x02e2, 0x01e8, 0x01fc, 0x02e2, 0x02e2,
    0x028a, 0x02aa,
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

/* the player's registers */
static ULONG d0, d1, d2, d3, d4, d5, d6, d7;
static UBYTE *a0, *a1, *a2, *a3, *a4, *a5;

static const char mess[] = "Can't Open Med Module";
static const char mess2[] = "Med Library Error";

/* --------------------------------------------------------- relocation */

/* relocentr - if the long at a0 is not 0 add d1; a0 += 4 */
static void relocentr(void)
{
    if (ML(a0, 0))
        ML(a0, 0) += d1;
    a0 += 4;
}

static void reloci(void)
{
    d0 = ML(a2, 24);
    if (!d0)
        return;
    a0 = (UBYTE *)d0;
    d0 = 0;
    SETB(d0, MB(a1, msng_numsamples));
    SETB(d0, d0 - 1);
    do {                                        /* relocs */
        relocentr();
        d3 = ML(a0, -4);
        if (d3) {
            a3 = (UBYTE *)d3;
            if (SW(MW(a3, 4)) < 0) {            /* synth: waveform pointers */
                SETW(d2, MW(a3, 20));
                a3 += 278;
                SETW(d2, d2 - 1);
                do {
                    ML(a3, 0) += d3;
                    a3 += 4;
                } while (SETW(d2, d2 - 1), W(d2) != 0xffff);
            }
        }
    } while (SETW(d0, d0 - 1), W(d0) != 0xffff);   /* DBF d0 */
}

static void relocp(void)
{
    a0 = a2 + 8;
    relocentr();
    a0 += 4;
    relocentr();
    a0 += 4;
    relocentr();
    a0 += 4;
    relocentr();
}

static void relocdmp(void)
{
    d3 = ML(a0, -4);
    if (!d3)
        return;
    {
        UBYTE *other = (UBYTE *)d3;             /* EXG.l a0,d3 */

        d3 = (ULONG)a0;
        a0 = other;
    }
    a0 += 4;
    relocentr();
    a0 = (UBYTE *)d3;
}

static void relocb(void)
{
    d0 = ML(a2, 16);
    if (!d0)
        return;
    a0 = (UBYTE *)d0;
    SETW(d0, MW(a1, msng_numblocks));
    SETB(d0, d0 - 1);                           /* SUBQ.b */
    do
        relocentr();
    while (SETW(d0, d0 - 1), W(d0) != 0xffff);
    if (MB(a2, 3) != '1')
        return;
    SETW(d0, MW(a1, msng_numblocks));           /* relocbi */
    a3 = a0;
    for (;;) {
        SETW(d0, d0 - 1);
        if (SW(d0) < 0)
            return;
        a3 -= 4;
        a0 = MP(a3, 0);
        a0 += 4;
        relocentr();                            /* BlockInfo pointer */
        a0 -= 4;
        if (!ML(a0, 0))
            continue;
        a0 = MP(a0, 0);
        relocentr();                            /* hldata */
        relocentr();                            /* block name */
    }
}

static void relocmdd(void)
{
    a0 -= 4;
    if (!ML(a0, 0))
        return;
    a0 = MP(a0, 0);
    SETW(d0, MW(a0, 0));                        /* # of msg dumps */
    a0 += 8;
    while (W(d0)) {                             /* mddloop: BEQ */
        relocentr();
        relocdmp();
        SETW(d0, d0 - 1);
    }
}

static void RelocModule(void)
{
    if (MB(a2, 0) != 'M')
        return;
    {
        UBYTE *saved_a2 = a2, *saved_a3 = a3;   /* MOVEM.l a2-a3/d2-d3 */
        ULONG saved_d2 = d2, saved_d3 = d3;

        MB(a2, 0) = 'R';
        d1 = (ULONG)a2;
        relocp();
        a1 = MP(a2, 8);
        reloci();
        for (;;) {                              /* rel_lp */
            relocb();
            d0 = ML(a2, 32);
            if (!d0)
                break;
            a0 = (UBYTE *)d0;
            relocentr();                        /* next module */
            relocentr();                        /* InstrExt */
            a0 += 4;
            relocentr();                        /* annotxt */
            a0 += 4;
            relocentr();                        /* InstrInfo */
            a0 += 8;
            relocentr();                        /* rgbtable */
            a0 += 4;
            relocentr();                        /* NotationInfo */
            relocentr();                        /* songname */
            a0 += 4;
            relocentr();                        /* MIDI dump */
            relocmdd();
            a0 = (UBYTE *)d0;
            d0 = *(volatile ULONG *)a0;         /* next module (0 -> end) */
            if (!d0)
                break;
            a2 = (UBYTE *)d0;
            relocp();
            a1 = MP(a2, 8);
        }
        a2 = saved_a2;
        a3 = saved_a3;
        d2 = saved_d2;
        d3 = saved_d3;
    }
}

/* ------------------------------------------------------------ channels */

static void ChannelOff(void)
{
    a0 = (UBYTE *)&db;
    a1 = (UBYTE *)db.trackdataptrs;
    SETB(d0, d0 << 2);
    a1 += SW(d0);                               /* ADDA.w d0,a1 */
    SETB(d0, B(d0) >> 2);
    a1 = MP(a1, 0);
    if (SB(d0) < 4) {                           /* CMP.b #4,d0:BGE */
        ML(a1, trk_synthptr) = 0;
        MB(a1, trk_synthtype) = 0;
        d1 = 1;
        SETW(d1, d1 << (d0 & 63));
        DMACON = W(d1);
    }
}

static void SoundOff(void)
{
    ULONG saved_d2 = d2;

    for (d2 = 15; ; ) {
        d0 = d2;
        ChannelOff();
        SETW(d2, d2 - 1);
        if (W(d2) == 0xffff)
            break;
    }
    db.module = 0;
    d2 = saved_d2;
}

/* --------------------------------------------------------------- synth */

static void synth_getwf(void)
{
    SETW(d1, (WORD)SB(d1));
    SETW(d1, d1 + d1);
    SETW(d1, d1 + d1);
    a1 = a0 + 278;
    a1 += SW(d1);
    a1 = MP(a1, 0);
    a1 += 2;
}

/* synth_start2 - d0 = synth instrument, d5 = period; result d5 */
static void synth_start2(void)
{
    UBYTE *saved_a3 = a3;

    a0 = (UBYTE *)d0;
    a3 = MP(a5, trk_audioaddr);
    if (--MSB(a5, trk_volxcnt) > 0)             /* SUBQ.b:BGT synth_wftbl */
        goto synth_wftbl;
    MB(a5, trk_volxcnt) = MB(a5, trk_initvolxspd);
    SETB(d0, MB(a5, trk_volchgspd));
    if (B(d0)) {
        SETB(d0, d0 + MB(a5, trk_synvol));
        if (SB(d0) < 0)
            d0 = 0;
        if (SB(d0) > 0x40)
            d0 = 0x40;
        MB(a5, trk_synvol) = d0;
    }
    d1 = ML(a5, trk_envptr);                    /* synth_nochgvol */
    if (d1) {
        a1 = (UBYTE *)d1;
        SETB(d0, *a1++);
        SETB(d0, d0 + 128);
        SETB(d0, B(d0) >> 2);
        MB(a5, trk_synvol) = d0;
        if (++MSB(a5, trk_envcount) < 0) {      /* ADDQ.b:BPL */
            MB(a5, trk_envcount) = 0;
            a1 = MP(a5, trk_envrestart);
        }
        MP(a5, trk_envptr) = a1;
    }
    SETW(d0, MW(a5, trk_volcmd));               /* synth_novolenv */
    if (MB(a5, trk_volwait)) {
        if (--MSB(a5, trk_volwait) <= 0)
            goto getvolcmd;
        goto synth_wftbl;
    }
    goto getvolcmd;
inccnt:
    SETB(d0, d0 + 1);
getvolcmd:
    SETB(d0, d0 + 1);
    SETB(d1, MB(a0 + SW(d0), 21));
    if (SB(d1) >= 0) {
        MB(a5, trk_synvol) = d1;
        goto endvol;
    }
    SETW(d1, d1 & 0x000f);                      /* synth_cmd */
    switch (W(d1)) {
    case 0x0:
        MB(a5, trk_initvolxspd) = MB(a0 + SW(d0), 22);
        goto inccnt;
    case 0x1:
        MB(a5, trk_volwait) = MB(a0 + SW(d0), 22);
        SETB(d0, d0 + 1);
        goto endvol;
    case 0x2:
        SETB(d1, MB(a0 + SW(d0), 22));
        SETB(d1, -B(d1));
        MB(a5, trk_volchgspd) = d1;
        goto inccnt;
    case 0x3:
        MB(a5, trk_volchgspd) = MB(a0 + SW(d0), 22);
        goto inccnt;
    case 0x4:
        SETB(d1, MB(a0 + SW(d0), 22));
        synth_getwf();
        ML(a5, trk_envrestart) = 0;
f4nd:
        MP(a5, trk_envptr) = a1;
        MB(a5, trk_envcount) = 0;
        goto inccnt;
    case 0x5:
        SETB(d1, MB(a0 + SW(d0), 22));
        synth_getwf();
        MP(a5, trk_envrestart) = a1;
        goto f4nd;
    case 0x6:
        ML(a5, trk_envptr) = 0;
        goto getvolcmd;
    case 0xa:
        MB(a5, trk_wfcmd + 1) = MB(a0 + SW(d0), 22);
        MB(a5, trk_wfwait) = 0;
        goto inccnt;
    case 0xb:
    case 0xf:
        SETB(d0, d0 - 1);                       /* syv_ff */
        goto endvol;
    case 0xe:
        SETB(d0, MB(a0 + SW(d0), 22));          /* JMP */
        goto getvolcmd;
    default:                                    /* 7 8 9 c d */
        goto endvol;
    }
endvol:
    MW(a5, trk_volcmd) = d0;
synth_wftbl:
    d0 = 0;                                     /* RELVOL */
    SETB(d0, MB(a5, trk_synvol));
    d0 = MULU(d0, MW(a5, trk_trackvol));
    SETW(d0, W(d0) >> 8);
    MB(a5, trk_prevvol) = d0;

    a0 += 158;
    if (--MSB(a5, trk_wfxcnt) > 0)
        goto synth_arpeggio;
    MB(a5, trk_wfxcnt) = MB(a5, trk_initwfxspd);
    SETW(d0, MW(a5, trk_wfcmd));
    SETW(d1, MW(a5, trk_wfchgspd));
    if (W(d1)) {
        SETW(d1, d1 + MW(a5, trk_perchg));
        MW(a5, trk_perchg) = d1;
    }
    if (MB(a5, trk_wfwait)) {                   /* synth_tstwfwai */
        if (--MB(a5, trk_wfwait) == 0)
            goto getwfcmd;
        goto synth_arpeggio;
    }
    goto getwfcmd;
incwfc:
    SETB(d0, d0 + 1);
getwfcmd:
    SETB(d0, d0 + 1);
    SETB(d1, MB(a0 + SW(d0), -9));
    if (SB(d1) >= 0) {
        SETW(d1, (WORD)SB(d1));
        SETW(d1, d1 + d1);
        SETW(d1, d1 + d1);
        a1 = MP(a0 + SW(d1), 120);
        *(volatile UWORD *)(a3 + 4) = *(UWORD *)a1;     /* ac_len */
        a1 += 2;
        *(volatile ULONG *)a3 = (ULONG)a1;              /* ac_ptr */
        goto wfend;
    }
    SETW(d1, d1 & 0x000f);                      /* synth_wfcmd */
    switch (W(d1)) {
    case 0x0:
        MB(a5, trk_initwfxspd) = MB(a0 + SW(d0), -8);
        goto incwfc;
    case 0x1:
        MB(a5, trk_wfwait) = MB(a0 + SW(d0), -8);
        SETB(d0, d0 + 1);
        goto wfend;
    case 0x2:
        d1 = 0;
        SETB(d1, MB(a0 + SW(d0), -8));
setsld:
        MW(a5, trk_wfchgspd) = d1;
        goto incwfc;
    case 0x3:
        SETB(d1, MB(a0 + SW(d0), -8));
        SETB(d1, -B(d1));
        SETW(d1, (WORD)SB(d1));
        goto setsld;
    case 0x4:
        MB(a5, trk_synvibdep + 1) = MB(a0 + SW(d0), -8);
        goto incwfc;
    case 0x5:
        MB(a5, trk_synthvibspd + 1) = MB(a0 + SW(d0), -8);
        MB(a5, trk_synthvibspd + 1)++;
        goto incwfc;
    case 0x6:
        MW(a5, trk_perchg) = 0;
        SETW(d5, MW(a5, trk_prevper));
        goto getwfcmd;
    case 0x7:
        SETB(d1, MB(a0 + SW(d0), -8));
        SETW(d1, (WORD)SB(d1));
        SETW(d1, d1 + d1);
        SETW(d1, d1 + d1);
        a1 = MP(a0 + SW(d1), 120);
        a1 += 2;
        MP(a5, trk_synvibwf) = a1;
        goto incwfc;
    case 0xa:
        MB(a5, trk_volcmd + 1) = MB(a0 + SW(d0), -8);
        MB(a5, trk_volwait) = 0;
        goto incwfc;
    case 0xb:
    case 0xf:
        SETB(d0, d0 - 1);                       /* syw_ff */
        goto wfend;
    case 0xc:
        MW(a5, trk_arpsoffs) = d0;
        MW(a5, trk_arpgoffs) = d0;
        do
            SETB(d0, d0 + 1);
        while (SB(MB(a0 + SW(d0), -9)) >= 0);
        goto getwfcmd;
    case 0xd:
        goto getwfcmd;
    case 0xe:
        SETB(d0, MB(a0 + SW(d0), -8));
        goto getwfcmd;
    default:                                    /* 8 9 */
        goto wfend;
    }
wfend:
    MW(a5, trk_wfcmd) = d0;
synth_arpeggio:
    SETW(d0, MW(a5, trk_arpgoffs));
    if (W(d0)) {
        d1 = 0;
        SETB(d1, MB(a0 + SW(d0), -8));
        SETB(d1, d1 + MB(a5, trk_prevnote2));
        a1 = MP(a5, trk_periodtbl);
        SETW(d1, d1 + d1);
        SETW(d5, MW(a1, SW(d1)));
        SETB(d0, d0 + 1);
        if (SB(MB(a0 + SW(d0), -8)) < 0)
            SETW(d0, MW(a5, trk_arpsoffs));
        MW(a5, trk_arpgoffs) = d0;
    }
    SETW(d1, MW(a5, trk_synvibdep));            /* synth_vibrato */
    if (W(d1)) {
        SETW(d0, MW(a5, trk_synviboffs));
        SETW(d0, W(d0) >> 4);
        SETW(d0, d0 & 0x1f);
        a0 = MP(a5, trk_synvibwf);
        SETB(d0, MB(a0, SW(d0)));
        SETW(d0, (WORD)SB(d0));
        d0 = MULS(d0, d1);
        SETW(d0, SW(d0) >> 8);
        SETW(d5, d5 + d0);
        SETW(d0, MW(a5, trk_synthvibspd));
        MW(a5, trk_synviboffs) += W(d0);
    }
    SETW(d5, d5 + MW(a5, trk_perchg));          /* synth_rts */
    if (SW(d5) < 113)
        d1 = 113;                               /* BUG(orig) (#206): d1, not d5 */
    a3 = saved_a3;
}

static void synth_start(void)
{
    SETW(d5, MW(a5, trk_prevper));
    synth_start2();
}

/* ------------------------------------------------------------ PlayNote */

/* _PlayNote - d7 = track, d1 = note, d3 = instrument, a3 = its data */
static void PlayNote(void)
{
    d4 = (ULONG)a3;
    if (!d4)
        return;
    d4 = 0;
    SETW(d5, ChanMask);
    if (!(d5 & (1UL << (d7 & 31))))
        return;
    d4 |= 1UL << (d7 & 31);                     /* BSET d7,d4 */
    a0 = MP(a2, mmd_smplarr);
    SETW(d3, d3 + d3);
    SETW(d3, d3 + d3);
    d5 = ML(a0, SW(d3));
    SETB(d1, d1 + MB(a4, msng_playtransp));     /* inmem */
    SETB(d1, d1 + MB(a3, inst_strans));
    if (SB(d7) < 4) {
        a1 = (UBYTE *)d5;
        if (!d5 || SB(MB(a5, trk_synthtype)) <= 0 || MW(a1, 4) != 0xffff)
            DMACON = W(d4);                     /* stpdma */
        ML(a5, trk_synthptr) = 0;               /* nostpdma */
    }
    SETB(d1, d1 - 1);                           /* nodmaoff */
    MB(a5, trk_fadespd) = 0;
    MB(a5, trk_decay) = MB(a5, trk_initdecay);
    MW(a5, trk_vibroffs) = 0;
    db.dmaonmsk |= W(d4);
    a0 = (UBYTE *)d5;
    if (SW(MW(a0, 4)) < 0)
        goto handleSynthnote;
    MB(a5, trk_synthtype) = 0;
tlwtst0:
    while (SB(d1) < 0)
        SETB(d1, d1 + 12);
    if (SB(d1) > 62)
        SETB(d1, d1 - 12);
    d2 = 0;                                     /* endpttest */
    d3 = 0;
    SETW(d0, MW(a0, 4));
    if (W(d0)) {                                /* iff5R3oct */
        ULONG saved_d7 = d7;

        d7 = 0;
        SETW(d7, d1);
        d7 = bl_divu_w(d7, 12);                 /* octave */
        d5 = d7;
        if (SW(d7) >= 6)
            d7 = 5;
        d5 = SWAP(d5);                          /* note in the octave */
        d1 = ML(a0, 0);
        if (SW(d0) > 6)
            d0 = 6;
        SETB(d7, d7 + gid_addtable[SW(d0) - 1]);
        SETB(d0, gid_divtable[SW(d0) - 1]);
        d1 = bl_divu_w(d1, W(d0));              /* length of the highest octave */
        d1 = SWAP(d1);
        SETW(d1, 0);
        d1 = SWAP(d1);
        d0 = d1;
        SETW(d2, MW(a3, inst_repeat));
        SETW(d3, MW(a3, inst_replen));
        d6 = 0;
        SETB(d6, shiftcnt[SW(d7)]);
        SETW(d2, W(d2) << (d6 & 63));
        SETW(d3, W(d3) << (d6 & 63));
        SETW(d1, W(d1) << (d6 & 63));
        SETB(d6, mullencnt[SW(d7)]);
        d0 = MULU(d0, d6);
        d0 += (ULONG)a0;
        d0 += 6;
        d1 += d0;
        a1 = PERIODTABLE0;
        SETB(d6, MB(a5, trk_finetune));
        SETB(d6, d6 + d6);
        SETB(d6, d6 + d6);
        SETW(d6, (WORD)SB(d6));
        a1 = MP(a1, SW(d6));
        MP(a5, trk_periodtbl) = a1;
        SETB(d5, d5 + octstart[SW(d7)]);
        SETB(d5, d5 + d5);
        SETW(d5, MW(a1, SW(d5)));
        d7 = saved_d7;
    } else {
        a1 = PERIODTABLE0;
        SETB(d2, MB(a5, trk_finetune));
        SETB(d2, d2 + d2);
        SETB(d2, d2 + d2);
        SETW(d2, (WORD)SB(d2));
        a1 = MP(a1, SW(d2));
        MP(a5, trk_periodtbl) = a1;
        SETB(d1, d1 + d1);
        SETW(d5, MW(a1, SW(d1)));
        d0 = (ULONG)a0;
        d0 += 6;                                /* skip structure */
        d1 = ML(a0, 0);
        d1 += d0;                               /* sample end */
        SETW(d2, MW(a3, inst_repeat));
        SETW(d3, MW(a3, inst_replen));
    }
    d2 += d2;                                   /* gid_setrept */
    d2 += d0;                                   /* repeat start */
    if (W(d3) > 1) {
        d3 += d3;
        d3 += d2;                               /* repeat end */
    } else
        d3 = 0;
    d4 = 0;                                     /* gid_cont */
    SETW(d4, MW(a5, trk_soffset));
    d0 += d4;
    if (!(d1 > d0))
        d0 -= d4;
    a1 = MP(a5, trk_audioaddr);
    *(volatile ULONG *)a1 = d0;                 /* ac_ptr */
    a1 += 4;
    if (d3 > d0) {                              /* rRepeat */
        ML(a5, trk_sampleptr) = d2;
        d1 = d3;
        d1 -= d0;
        d1 >>= 1;
        *(volatile UWORD *)a1 = W(d1);          /* ac_len */
        a1 += 2;
        d3 -= d2;
        d3 >>= 1;
        MW(a5, trk_samplelen) = d3;
    } else {
        MP(a5, trk_sampleptr) = db.chipzero;
        MW(a5, trk_samplelen) = 1;
        d1 -= d0;
        d1 >>= 1;
        *(volatile UWORD *)a1 = W(d1);
        a1 += 2;
    }
    MW(a5, trk_prevper) = d5;                   /* retsn1 */
    if (MB(a5, trk_synthtype))
        goto hSn2;
    return;

handleSynthnote:
    MB(a5, trk_prevnote2) = d1;
    MP(a5, trk_synthptr) = a0;
    if (MW(a0, 4) == 0xfffe) {                  /* HYBRID */
        MB(a5, trk_synthtype) = 0xff;
        a0 = MP(a0, 278);
        goto tlwtst0;
    }
    MB(a5, trk_synthtype) = 1;
    a1 = PERIODTABLE0;
    SETB(d0, MB(a5, trk_finetune));
    SETB(d0, d0 + d0);
    SETB(d0, d0 + d0);
    SETW(d0, (WORD)SB(d0));
    a1 = MP(a1, SW(d0));
    a1 -= 48;
    MP(a5, trk_periodtbl) = a1;
    SETW(d1, d1 + d1);
    SETW(d1, MW(a1, SW(d1)));
    MW(a5, trk_prevper) = d1;
    ML(a5, trk_sampleptr) = 0;
hSn2:
    a1 = a5 + trk_arpgoffs;
    {
        int i;

        for (i = 0; i < 6; i++) {
            ML(a1, 0) = 0;
            a1 += 4;
        }
    }
    ML(a1, 0) = (ULONG)sinetable;
    a1 += 4;
    MW(a1, 0) = 0;
    a1 += 2;
    a0 = MP(a5, trk_synthptr);
    MW(a1, 0) = MW(a0, 18);
    a1 += 2;
    MB(a1, 0) = 0;
    if (MB(a5, trk_cmd) == 0xe)
        MB(a5, trk_wfcmd + 1) = MB(a5, trk_cmdqual);
    d4 = 64;
}

/* playfxnote - replay the previous note (hold counter continued) */
static void playfxnote(void)
{
    ULONG saved_d3;
    UBYTE *saved_a3;

    d1 = 0;
    SETB(d1, MB(a5, trk_prevnote));
    if (!B(d1))
        return;
    SETB(d0, MB(a5, trk_noteoffcnt));
    if (SB(d0) >= 0)
        SETB(d0, d0 + d3);
    else {
        SETB(d0, MB(a5, trk_inithold));
        if (!B(d0))
            SETB(d0, 0xff);
    }
    MB(a5, trk_noteoffcnt) = d0;
    saved_d3 = d3;
    saved_a3 = a3;
    d3 = 0;
    SETB(d3, MB(a5, trk_previnstr));
    a3 = MP(a5, trk_previnstra);
    PlayNote();
    d3 = saved_d3;
    a3 = saved_a3;
}

static void cmd_F(void)
{
    switch (B(d4)) {
    case 0xf1:
    case 0xf2:
        if (B(d3) == 3)
            playfxnote();
        return;
    case 0xf3:
        SETB(d0, d3);
        SETB(d0, d0 & (2 + 4));
        if (B(d0))
            playfxnote();
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

static void Wait1Lin(void)
{
    UBYTE b;

    do {
        b = VHPOSRB;
        while (VHPOSRB == b)
            ;
        SETW(d1, d1 - 1);
    } while (W(d1) != 0xffff);
}

static void pushnewvals(void)
{
    a5 = MP(a1, 0);
    a1 += 4;
    {
        int c = d0 & 1;

        SETB(d0, B(d0) >> 1);
        if (!c)
            return;
    }
    d1 = ML(a5, trk_sampleptr);
    if (!d1)
        return;
    a0 = MP(a5, trk_audioaddr);
    *(volatile ULONG *)a0 = d1;
    *(volatile UWORD *)(a0 + 4) = MW(a5, trk_samplelen);
}

/* the volume slide of 5, 6, A, D */
static void plr_volslide(void)
{
    SETB(d0, d4);
    d1 = 0;
    SETB(d1, MB(a5, trk_prevvol));
    SETB(d0, d0 & 0xf0);
    if (B(d0)) {                                /* crescendo */
        SETB(d0, B(d0) >> 4);
        SETB(d1, d1 + d0);
        if (SB(d1) > 64)
            d1 = 64;
    } else {
        SETB(d1, d1 - d4);
        if (SB(d1) < 0)
            d1 = 0;
    }
    MB(a5, trk_prevvol) = d1;
}

static ULONG IntHandler(void)
{
    UBYTE *noteptr, **tdp;

    if (SB(db.bpmcounter[0]) >= 0) {
        if (SB(--db.bpmcounter[0]) > 0)
            goto plr_exit;
        db.bpmcounter[0] = 4;
    }
    a2 = db.module;
    if (!a2)
        goto plr_exit;
    if (!MW(a2, mmd_pstate))
        goto plr_exit;
    db.dmaonmsk = 0;
    a4 = MP(a2, mmd_songinfo);
    d3 = 0;
    SETB(d3, MB(a2, mmd_counter));
    SETB(d3, d3 + 1);
    if (SB(d3) < SB(MB(a4, msng_tempo2))) {
        MB(a2, mmd_counter) = d3;
        if (B(d3))
            goto plr_fxtime;
    }
    MB(a2, mmd_counter) = 0;                    /* plr_pnewnote */
    if (db.blkdelay) {
        if (--db.blkdelay)
            goto nonewnote;
    }
    SETW(d0, MW(a2, mmd_pblock));               /* plr_noblkdelay */
    a0 = MP(a2, mmd_blockarr);
    SETW(d0, d0 + d0);
    SETW(d0, d0 + d0);
    a1 = MP(a0, SW(d0));
    SETW(d0, MW(a2, mmd_pline));
    if (MB(a2, 3) != '1') {
        SETW(d1, d0);
        SETW(d0, d0 + d0);
        SETW(d0, d0 + d1);
        db.numtracks = 0;
        db.numlines = 0;
        db.numtracks = *a1++;
        db.numlines = *a1;
        d0 = MULU(d0, db.numtracks);
        noteptr = a1 + 1 + SW(d0);
    } else {
        SETW(d0, d0 + d0);
        SETW(d0, d0 + d0);
        d0 = MULU(d0, MW(a1, 0));
        noteptr = a1 + 8 + (LONG)d0;
        db.numtracks = MW(a1, 0);
        a1 += 2;
        db.numlines = MW(a1, 0);
    }
    d7 = 0;                                     /* plr_begloop */
    d4 = 0;
    tdp = db.trackdataptrs;
plr_loop0:
    d5 = 0;
    a5 = *tdp++;
    /* BUG(orig) (#334): a masked track is skipped before its note is read, so
       noteptr stays: the next track reads this track's note */
    if (!(B(ChanMask) & (1 << (d7 & 7))))
        goto plr_endloop0;
    d3 = 0;
    a1 = noteptr;
    if (MB(a2, 3) != '1') {
        SETB(d5, *a1++);
        SETB(d6, *a1++);
        MB(a5, trk_cmdqual) = *a1++;
        SETB(d3, d6);
        SETW(d6, d6 & 0x0f);
        SETB(d3, B(d3) >> 4);
        if (d5 & 0x80) {
            SETB(d5, d5 & ~0x80);
            d3 |= 0x10;
        }
        if (d5 & 0x40) {
            SETB(d5, d5 & ~0x40);
            d3 |= 0x20;
        }
    } else {
        SETB(d5, *a1++);
        if (SB(d5) < 0)
            d5 = 0;
        SETB(d3, *a1++);
        SETB(d6, *a1++);
        SETW(d6, d6 & 0x1f);
        MB(a5, trk_cmdqual) = *a1++;
    }
    noteptr = a1;                               /* plr_nngok */
    SETW(d3, d3 & 0x3f);
    if (W(d3)) {
        SETB(d3, d3 - 1);
        MB(a5, trk_previnstr) = d3;
        a0 = db.holdvals;
        a0 += SW(d3);
        MB(a5, trk_inithold) = MB(a0, 0);
        MB(a5, trk_initdecay) = MB(a0, 63);
        MB(a5, trk_finetune) = MB(a0, 126);
        SETW(d3, d3 << 3);
        a3 = a4 + SW(d3);
        MP(a5, trk_previnstra) = a3;
        d0 = 0;
        SETB(d0, MB(a3, inst_svol));
        d0 = MULU(d0, MW(a5, trk_trackvol));
        SETW(d0, W(d0) >> 8);
        MB(a5, trk_prevvol) = d0;
        MB(a5, trk_stransp) = MB(a3, inst_strans);
        MW(a5, trk_soffset) = 0;
    }
    MB(a5, trk_cmd) = d6;                       /* noinstnum */
    if (!B(d6))
        goto fx;
    SETB(d4, MB(a5, trk_cmdqual));
    SETB(d6, d6 + d6);
    SETW(d0, f_table[W(d6) >> 1]);
    switch (W(d6) >> 1) {
    case 0x0f:                                  /* tempo */
        if (!B(d4)) {
            db.nextblock = 1;                   /* F00: pattern break */
            goto fx;
        }
        if (!(B(d4) > 0xf0))
            goto fx;
        if (B(d4) == 0xf2)
            goto f_1f;
        if (B(d4) == 0xfe) {                    /* FFE: stop */
            MW(a2, mmd_pstate) = 0;
            SoundOff();
            goto plr_exit;
        }
        if (B(d4) == 0xfd) {                    /* FFD: period only */
            a0 = MP(a5, trk_periodtbl);
            SETB(d5, d5 - 1);
            if (SB(d5) < 0)
                goto plr_endloop0;
            SETB(d5, d5 + d5);
            MW(a5, trk_prevper) = MW(a0, SW(d5));
            d5 = 0;
            goto fx;
        }
        if (B(d4) == 0xff) {
            SETW(d0, d7);
            ChannelOff();
        }
        goto fx;
    case 0x1f:
f_1f:
        MB(a5, trk_prevnote) = d5;
        d5 = 0;
        MB(a5, trk_noteoffcnt) = MB(a5, trk_inithold);
        if (!MB(a5, trk_noteoffcnt))
            MB(a5, trk_noteoffcnt) = 0xff;
        goto plr_endloop0;
    case 0x0e:
        if (SB(d7) > 3)
            goto fx;
        MB(a5, trk_wfcmd + 1) = d4;
        goto fx;
    case 0x0c:                                  /* volume */
        SETB(d0, d4);
        if (SB(d0) < 0) {
            SETB(d0, d0 & 0x7f);
            d1 = 0;
            SETB(d1, MB(a5, trk_previnstr));
            SETW(d1, d1 << 3);
            MB(a4 + SW(d1), inst_svol) = d0;
        } else if (!(MB(a4, msng_flags) & 0x10)) {
            SETB(d0, B(d0) >> 4);
            d0 = MULU(d0, 10);
            SETB(d1, d4);
            SETB(d1, d1 & 0x0f);
            SETB(d0, d0 + d1);
        }
        d0 = MULU(d0, MW(a5, trk_trackvol));    /* plr_setvol */
        SETW(d0, W(d0) >> 8);
        MB(a5, trk_prevvol) = d0;
        goto fx;
    case 0x09:
        MB(a4, msng_tempo2) = d4;
        goto fx;
    case 0x15:
        MB(a5, trk_finetune) = d4;
        goto fx;
    case 0x16:                                  /* repeat loop */
        if (!B(d4)) {
            db.rptline = MW(a2, mmd_pline);
            goto fx;
        }
        if (db.rptcounter) {
            if (!--db.rptcounter)
                goto fx;
        } else
            *((UBYTE *)&db.rptcounter + 1) = d4;
        SETW(d0, db.rptline);
        SETW(d0, d0 + 1);
        db.nextblockline = d0;
        goto fx;
    case 0x1c:
        d1 = 0;
        SETB(d1, MB(a5, trk_previnstr));
        SETW(d1, d1 << 3);
        MB(a4 + SW(d1), inst_midipreset) = d4;
        goto fx;
    case 0x08:                                  /* hold / decay */
        SETB(d0, d4);
        SETB(d4, B(d4) >> 4);
        SETB(d0, d0 & 0x0f);
        MB(a5, trk_initdecay) = d4;
        MB(a5, trk_inithold) = d0;
        goto fx;
    case 0x19:
        SETW(d4, d4 << 8);
        MW(a5, trk_soffset) = d4;
        goto fx;
    case 0x0b:
        MW(a2, mmd_pseqnum) = d4;
        db.nextblock = 0xff;
        goto fx;
    case 0x1d:
        db.nextblock = 1;
        db.nxtnoclrln = 0xff;
        SETW(d4, d4 + 1);
        db.nextblockline = d4;
        goto fx;
    case 0x03:                                  /* portamento */
        SETB(d5, d5 - 1);
        if (SB(d5) >= 0) {
            a0 = MP(a5, trk_periodtbl);
            SETB(d5, d5 + MB(a4, msng_playtransp));
            SETB(d5, d5 + MB(a5, trk_stransp));
            if (SB(d5) < 0)
                goto plr_endloop0;
            SETW(d5, d5 + d5);
            MW(a5, trk_porttrgper) = MW(a0, SW(d5));
        }
        if (!B(d4))
            goto plr_endloop0;
        MB(a5, trk_prevportspd) = d4;
        goto plr_endloop0;
    default:
        goto fx;
    }
fx:
    if (!B(d5))
        goto plr_endloop0;
    MB(a5, trk_prevnote) = d5;
    SETW(d1, d5);
    d3 = 0;
    SETB(d3, MB(a5, trk_previnstr));
    a3 = MP(a5, trk_previnstra);
    MB(a5, trk_noteoffcnt) = MB(a5, trk_inithold);
    if (!MB(a5, trk_noteoffcnt))
        MB(a5, trk_noteoffcnt) = 0xff;
    PlayNote();
plr_endloop0:
    SETB(d7, d7 + 1);
    if (SW(d7) < SW(db.numtracks))
        goto plr_loop0;

    a3 = &db.nextblock;                         /* advance song pointer */
    SETW(d1, db.nextblockline);
    if (W(d1)) {
        db.nextblockline = 0;
        SETW(d1, d1 - 1);
    } else {
        SETW(d1, MW(a2, mmd_pline));
        SETW(d1, d1 + 1);
    }
    if (W(d1) > db.numlines || db.nextblock) {  /* plr_chgblock */
        if (!db.nxtnoclrln)
            d1 = 0;
        if (SW(MW(a2, mmd_pstate)) < 0) {
            SETW(d0, MW(a2, mmd_pseqnum));
            if (SB(db.nextblock) >= 0)
                SETW(d0, d0 + 1);
            if (!(SW(d0) < SW(MW(a4, msng_songlen))))
                d0 = 0;
            MB(a2, mmd_pseqnum + 1) = d0;
            a0 = a4 + msng_playseq;
            SETB(d0, MB(a0, SW(d0)));
            MB(a2, mmd_pblock + 1) = d0;
        }
        MW(a3, 0) = 0;                          /* plr_nonewseq */
    }
    MW(a2, mmd_pline) = d1;                     /* plr_nochgblock */

    /* hold: continue holding the notes of the new line */
    a5 = (UBYTE *)db.trackdataptrs;
    a0 = MP(a2, mmd_blockarr);
    SETW(d0, MW(a2, mmd_pblock));
    SETW(d0, d0 + d0);
    SETW(d0, d0 + d0);
    a1 = MP(a0, SW(d0));
    SETW(d0, MW(a2, mmd_pline));
    SETB(d3, MB(a4, msng_tempo2));
    if (MB(a2, 3) != '1') {
        SETB(d7, MB(a1, 0));
        SETW(d1, d0);
        SETW(d0, d0 + d0);
        SETW(d0, d0 + d1);
        d0 = MULU(d0, d7);
        a3 = a1 + 2 + SW(d0);
        SETB(d7, d7 - 1);
        do {
            a1 = MP(a5, 0);
            a5 += 4;
            if (SB(MB(a1, trk_noteoffcnt)) >= 0) {
                SETB(d1, MB(a3, 0));
                if (B(d1)) {
                    SETB(d1, d1 & 0x3f);
                    if (B(d1)) {
                        SETB(d1, MB(a3, 1));
                        SETB(d1, d1 & 0x0f);
                        SETB(d1, d1 - 3);
                        if (B(d1))
                            goto holdendb;
                    }
                } else {
                    SETB(d1, MB(a3, 1));
                    SETB(d1, d1 & 0xf0);
                    if (!B(d1))
                        goto holdendb;
                }
                MB(a1, trk_noteoffcnt) += B(d3);
            }
holdendb:
            a3 += 3;
            SETW(d7, d7 - 1);
        } while (W(d7) != 0xffff);
    } else {
        SETW(d7, MW(a1, 0));
        SETW(d0, d0 + d0);
        SETW(d0, d0 + d0);
        d0 = MULU(d0, d7);
        a3 = a1 + 8 + (LONG)d0;
        SETB(d7, d7 - 1);
        do {
            a1 = MP(a5, 0);
            a5 += 4;
            if (SB(MB(a1, trk_noteoffcnt)) >= 0) {
                SETB(d1, MB(a3, 0));
                if (B(d1)) {
                    SETB(d1, d1 & 0x7f);
                    if (B(d1)) {
                        SETB(d1, MB(a3, 2));
                        SETB(d1, d1 - 3);
                        if (B(d1))
                            goto holdend;
                    }
                } else {
                    SETB(d0, MB(a3, 1));
                    SETB(d0, d0 & 0x3f);
                    if (!B(d0))
                        goto holdend;
                }
                MB(a1, trk_noteoffcnt) += B(d3);
            }
holdend:
            a3 += 4;
            SETW(d7, d7 - 1);
        } while (W(d7) != 0xffff);
    }
nonewnote:
    d3 = 0;
    SETB(d3, MB(a2, mmd_counter));
plr_fxtime:
    a3 = (UBYTE *)db.trackdataptrs;
    d7 = 0;
plr_loop1:
    a5 = MP(a3, 0);
    a3 += 4;
    d4 = 0;
    d5 = 0;
    d6 = 0;
    if (!(B(ChanMask) & (1 << (d7 & 7))))
        goto endl;
    SETB(d6, MB(a5, trk_cmd));
    SETB(d4, MB(a5, trk_cmdqual));
    if (SW(d7) >= 4)
        goto endl;
    if (SB(MB(a5, trk_noteoffcnt)) >= 0) {      /* hold */
        if (SB(--MB(a5, trk_noteoffcnt)) < 0) {
            if (MB(a5, trk_synthtype)) {
                MB(a5, trk_volcmd + 1) = MB(a5, trk_decay);
                MB(a5, trk_volwait) = 0;
            } else {
                MB(a5, trk_fadespd) = MB(a5, trk_decay);
                if (!MB(a5, trk_fadespd)) {
                    d5 |= 1UL << (d7 & 31);
                    DMACON = W(d5);
                    d5 = 0;
                }
            }
        }
    }
    SETB(d0, MB(a5, trk_fadespd));              /* plr_nowaitoff */
    if (B(d0)) {
        MB(a5, trk_prevvol) -= B(d0);
        if (SB(MB(a5, trk_prevvol)) < 0) {
            MB(a5, trk_prevvol) = 0;
            MB(a5, trk_fadespd) = 0;
        }
    }
    SETB(d6, d6 + d6);                          /* plr_nofade */
    SETW(d0, fx_table[W(d6) >> 1]);
    switch (W(d6) >> 1) {
    case 0x01:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
        MW(a5, trk_prevper) -= W(d4);
        SETW(d5, MW(a5, trk_prevper));
        if (SW(d5) < 113) {
            SETW(d5, 113);
            MW(a5, trk_prevper) = d5;
        }
        goto fx_xx;
    case 0x11:
        if (B(d3))
            goto fx_xx;
        MW(a5, trk_prevper) -= W(d4);
        SETW(d5, MW(a5, trk_prevper));
        goto fx_xx;
    case 0x02:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
        MW(a5, trk_prevper) += W(d4);
        SETW(d5, MW(a5, trk_prevper));
        goto fx_xx;
    case 0x12:
        if (B(d3))
            goto fx_xx;
        MW(a5, trk_prevper) += W(d4);
        SETW(d5, MW(a5, trk_prevper));
        goto fx_xx;
    case 0x00:                                  /* arpeggio */
        if (!B(d4))
            goto fx_xx;
        d0 = d3;
        d0 = bl_divu_w(d0, 3);
        d0 = SWAP(d0);
        if (!W(d0)) {
            SETB(d4, d4 & 0x0f);
            SETB(d4, d4 + MB(a5, trk_prevnote));
        } else {
            SETB(d0, d0 - 1);
            if (!B(d0)) {
                SETB(d4, B(d4) >> 4);
                SETB(d4, d4 + MB(a5, trk_prevnote));
            } else
                SETB(d4, MB(a5, trk_prevnote));
        }
        SETB(d4, d4 - 1);                       /* fx_doarp */
        SETB(d4, d4 + MB(a4, msng_playtransp));
        SETB(d4, d4 + MB(a5, trk_stransp));
        SETB(d4, d4 + d4);
        a1 = MP(a5, trk_periodtbl);
        SETW(d5, MW(a1, SW(d4)));
        goto plr_newtmp;
    case 0x14:
        MB(a5, trk_vibshift) = 6;
        goto vib_cont;
    case 0x04:
        MB(a5, trk_vibshift) = 5;
vib_cont:
        if (B(d3))
            goto nonvib;
        SETB(d1, d4);
        if (!B(d1))
            goto nonvib;
        SETW(d1, d1 & 0x0f);
        if (W(d1))
            MW(a5, trk_vibrsz) = d1;
        SETB(d4, d4 & 0xf0);                    /* plr_chgvibspd */
        if (!B(d4))
            goto nonvib;
        SETB(d4, B(d4) >> 3);
        SETB(d4, d4 & 0x3e);
        MB(a5, trk_vibrspd) = d4;
nonvib:
        SETB(d0, MB(a5, trk_vibroffs));
        SETB(d0, B(d0) >> 2);
        SETW(d0, d0 & 0x1f);
        d1 = 0;
        SETB(d5, sinetable[W(d0)]);
        SETW(d5, (WORD)SB(d5));
        d5 = MULS(d5, MW(a5, trk_vibrsz));
        SETB(d1, MB(a5, trk_vibshift));
        {
            UWORD sh = d1 & 63;

            SETW(d5, sh >= 16 ? (SW(d5) < 0 ? 0xffff : 0) : (UWORD)(SW(d5) >> sh));
        }
        SETW(d5, d5 + MW(a5, trk_prevper));
        SETB(d0, MB(a5, trk_vibrspd));
        MB(a5, trk_vibroffs) += B(d0);
        goto plr_newtmp;
    case 0x06:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
        plr_volslide();
        goto nonvib;
    case 0x07:                                  /* tremolo */
        if (B(d3))
            goto nontre;
        SETB(d1, d4);
        if (!B(d1))
            goto nontre;
        SETW(d1, d1 & 0x0f);
        if (W(d1))
            MW(a5, trk_tremsz) = d1;
        SETB(d4, d4 & 0xf0);                    /* plr_chgtrespd */
        if (!B(d4))
            goto nonvib;                        /* BUG(orig) (#204) */
        SETB(d4, B(d4) >> 2);
        SETB(d4, d4 & 0x3e);
        MB(a5, trk_tremspd) = d4;
nontre:
        SETB(d0, MB(a5, trk_tremoffs));
        SETB(d0, B(d0) >> 3);
        SETW(d0, d0 & 0x1f);
        d1 = 0;
        SETB(d5, sinetable[W(d0)]);
        SETW(d5, (WORD)SB(d5));
        d5 = MULS(d5, MW(a5, trk_tremsz));
        SETW(d5, SW(d5) >> 7);
        SETB(d0, MB(a5, trk_tremspd));
        MB(a5, trk_tremoffs) += B(d0);
        SETB(d1, MB(a5, trk_prevvol));
        SETB(d1, d1 + d5);
        if (SB(d1) < 0)
            d1 = 0;
        if (SB(d1) > 64)
            d1 = 64;
        MB(a5, trk_tempvol) = d1;
        goto fx_xx;
    case 0x0a:
    case 0x0d:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
        plr_volslide();
        goto fx_xx;
    case 0x1a:
        if (B(d3))
            goto fx_xx;
        SETB(d1, MB(a5, trk_prevvol));
        SETB(d1, d1 + d4);
        if (SB(d1) > 64)                        /* voltest */
            d1 = 64;
        MB(a5, trk_prevvol) = d1;
        goto fx_xx;
    case 0x1b:
        if (B(d3))
            goto fx_xx;
        SETB(d1, MB(a5, trk_prevvol));
        SETB(d1, d1 - d4);
        if (SB(d1) < 0)                         /* voltest0 */
            d1 = 0;
        MB(a5, trk_prevvol) = d1;
        goto fx_xx;
    case 0x05:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
        plr_volslide();
        goto fx_03nocnt0;
    case 0x03:
        if (!B(d3) && (MB(a4, msng_flags) & 0x20))
            goto fx_xx;
fx_03nocnt0:
        SETW(d0, MW(a5, trk_porttrgper));
        if (!W(d0))
            goto fx_xx;
        SETW(d1, MW(a5, trk_prevper));
        SETB(d4, MB(a5, trk_prevportspd));
        if (W(d1) > W(d0)) {                    /* subper */
            SETW(d1, d1 - d4);
            if (SW(d1) > SW(d0))
                goto targnreach;
        } else {
            SETW(d1, d1 + d4);
            if (!(SW(d1) >= SW(d0)))
                goto targnreach;
        }
        SETW(d1, MW(a5, trk_porttrgper));       /* targreached */
        MW(a5, trk_porttrgper) = 0;
targnreach:
        MW(a5, trk_prevper) = d1;
        SETW(d5, d1);
        goto fx_xx;
    case 0x13:
        SETW(d5, MW(a5, trk_prevper));
        if (SB(d3) >= 3)
            goto fx_xx;
        SETW(d5, d5 - d4);
        goto fx_xx;
    case 0x1e:
        if (db.blkdelay)
            goto fx_xx;
        SETW(d4, d4 + 1);
        db.blkdelay = d4;
        goto fx_xx;
    case 0x18:
        if (B(d3) != B(d4))
            goto fx_xx;
        MB(a5, trk_prevvol) = 0;
        goto fx_xx;
    case 0x1f:                                  /* note delay / retrigger */
        SETB(d1, d4);
        SETB(d4, B(d4) >> 4);
        if (B(d4)) {
            if (SB(d3) < SB(d4))
                goto fx_xx;
            if (B(d3) == B(d4))
                playfxnote();
        }
        SETW(d1, d1 & 0x0f);                    /* nonotedelay */
        if (!W(d1))
            goto fx_xx;
        d0 = 0;
        SETB(d0, d3);
        d0 = bl_divu_w(d0, W(d1));
        d0 = SWAP(d0);
        if (W(d0))
            goto fx_xx;
        playfxnote();
        goto fx_xx;
    case 0x0f:
        cmd_F();
        goto fx_xx;
    default:                                    /* 08 09 0b 0c 0e 10 15-17 19 1c 1d */
        goto fx_xx;
    }
fx_xx:
    d0 = ML(a5, trk_synthptr);
    if (d0) {
        synth_start();
        goto plr_tmpper;
    }
    SETW(d5, MW(a5, trk_prevper));              /* plr_nosynth */
    goto plr_tmpper;
plr_newtmp:
    d0 = ML(a5, trk_synthptr);
    if (d0)
        synth_start2();
plr_tmpper:
    a1 = MP(a5, trk_audioaddr);
    *(volatile UWORD *)(a1 + 6) = W(d5);        /* ac_per */
    if (!W(d5))
        goto endl;
    SETB(d0, MB(a5, trk_tempvol));
    if (SB(d0) >= 0) {
        *(volatile UBYTE *)(a1 + 9) = d0;       /* ac_vol+1 */
        MB(a5, trk_tempvol) = 0xff;
    } else
        *(volatile UBYTE *)(a1 + 9) = MB(a5, trk_prevvol);
endl:
    SETB(d7, d7 + 1);
    if (SW(d7) < SW(db.numtracks))
        goto plr_loop1;

    SETW(d0, db.dmaonmsk);                      /* plr_endfx */
    if (!W(d0))
        goto plr_exit;
    SETW(d0, d0 & ChanMask);
    d0 |= 0x8000;
    d1 = 8;
    Wait1Lin();
    DMACON = W(d0);
    d1 = 8;
    Wait1Lin();
    a1 = (UBYTE *)db.trackdataptrs;
    if (B(ChanMask) & 1)
        pushnewvals();
    if (B(ChanMask) & 2)
        pushnewvals();
    if (B(ChanMask) & 4)
        pushnewvals();
    if (B(ChanMask) & 8)
        pushnewvals();
plr_exit:
    return 1;
}

/* ------------------------------------------------- module start / stop */

static void InitModule(void)
{
    UBYTE *saved_a2 = a2, *saved_a3 = a3;
    ULONG s2d = d2;
    int i;

    if (!a0)
        goto exit;
    a1 = MP(a0, mmd_songinfo);
    SETB(d0, MB(a1, msng_mastervol));
    SETW(d0, (WORD)SB(d0));
    a1 += msng_trkvol;
    a2 = (UBYTE *)db.trackdataptrs;
    for (i = 0; i < 16; i++) {
        SETB(d2, *a1++);
        SETW(d2, (WORD)SB(d2));
        a3 = MP(a2, 0);
        a2 += 4;
        d2 = MULU(d0, d2);
        SETW(d2, W(d2) >> 4);
        MW(a3, trk_trackvol) = d2;
    }
    SETW(d1, 0xffff);                           /* DBF d1 */
    a2 = db.holdvals;
    a3 = a0;
    d0 = ML(a0, mmd_expdata);
    if (!d0)
        goto clrhlddec;
    a1 = (UBYTE *)d0;
    d0 = ML(a1, 4);                             /* exp_smp */
    if (!d0)
        goto clrhlddec;
    a0 = (UBYTE *)d0;
    SETW(d2, MW(a1, 8));                        /* # of entries */
    if (!W(d2))
        goto clrhlddec;
    SETW(d2, d2 - 1);
    SETW(d0, MW(a1, 10));                       /* entry size */
    do {
        if (SW(d0) > 3)
            MB(a2, 126) = MB(a0, 3);            /* finetune */
        MB(a2, 63) = MB(a0, 1);                 /* decay */
        *a2++ = MB(a0, 0);                      /* hold */
        a0 += SW(d0);
        SETW(d2, d2 - 1);
    } while (W(d2) != 0xffff);
    goto exit;
clrhlddec:
    SETW(d0, 3 * 63 - 1);
    do {
        *a2++ = 0;
        SETW(d0, d0 - 1);
    } while (W(d0) != 0xffff);
exit:
    a2 = saved_a2;
    a3 = saved_a3;
    d2 = s2d;
}

/* _PlayModule / contpoint - d0.b = clear the position (ST d0) */
static void PlayModule(int clear)
{
    UBYTE *m = a0;

    InitModule();
    a0 = m;
    d1 = (ULONG)a0;
    if (!a0)
        return;
    db.module = 0;
    SETW(d1, db.modnum);
    if (W(d1)) {
        do {                                    /* PM_nextmod */
            if (!ML(a0, mmd_expdata))
                break;
            a1 = MP(a0, mmd_expdata);
            if (!ML(a1, 0))
                break;
            a0 = MP(a1, 0);
            SETW(d1, d1 - 1);
        } while (SW(d1) > 0);
    }
    a1 = MP(a0, mmd_songinfo);                  /* PM_modfound */
    MB(a0, mmd_counter) = MB(a1, msng_tempo2);
    if (MB(a1, msng_flags) & 1)
        CIAAPRA &= ~2;
    else
        CIAAPRA |= 2;
    if (clear) {
        ML(a0, mmd_pline) = 0;
        db.rptline = 0;
        db.rptcounter = 0;
        db.blkdelay = 0;
    }
    SETW(d1, MW(a0, mmd_pseqnum));
    SETW(d1, d1 + msng_playseq);
    SETB(d1, MB(a1, SW(d1)));
    MB(a0, mmd_pblock + 1) = d1;
    MW(a0, mmd_pstate) = 0xffff;
    db.module = a0;
    db.bpmcounter[0] = (MB(a1, msng_flags2) & 0x20) ? 0 : 0xff;     /* SEQ */
}

static void StopPlayer(void)
{
    if (db.module) {
        MW(db.module, mmd_pstate) = 0;
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
    UseMod = (UBYTE *)obj[2];
    a2 = UseMod;
    RelocModule();
    a0 = UseMod;
    PlayModule(1);
    ChanMask = 15;
    a2 = UseMod;                                /* _startmusic */
    RelocModule();
    a0 = UseMod;
    PlayModule(1);
    Music = 1;
}
BLITZ_THUNK(_StartMedModule, _StartMedModule_c, "d1/a0-a1");

static void BLITZ_ROUTINE _PlayMed_c(void)
{
    if (Music)
        IntHandler();
}
BLITZ_THUNK(_PlayMed, _PlayMed_c, "d1/a0-a1");

/* JumpMed n - position n-1 of the song, line 0 */
static void BLITZ_ROUTINE _JumpMed_c(UWORD n BL_REG(d0))
{
    UBYTE *m = db.module;

    MW(m, mmd_pseqnum) = n - 1;
    MW(m, mmd_pline) = 0;
    db.nextblock = 0xff;
}
BLITZ_THUNK(_JumpMed, _JumpMed_c, "d1/a0-a1");

/* SetMedVolume v - track volumes = v * track volume / 16 */
static void BLITZ_ROUTINE _SetMedVolume_c(UWORD v BL_REG(d0))
{
    UBYTE *song = MP(UseMod, mmd_songinfo) + msng_trkvol;
    int i;

    for (i = 0; i < 16; i++) {
        UWORD tv = (WORD)(BYTE)*song++;

        MW(db.trackdataptrs[i], trk_trackvol) = (UWORD)MULU(tv, v) >> 4;
    }
}
BLITZ_THUNK(_SetMedVolume, _SetMedVolume_c, "d1/a0-a1");

/* GetMedVolume/Note/Instr(channel) - read and clear */
static ULONG getclear(UWORD ch, int off)
{
    UBYTE *track = *(UBYTE **)((UBYTE *)db.trackdataptrs + (WORD)(UWORD)(ch << 2));
    ULONG v = MB(track, off);

    MB(track, off) = 0;
    return v;
}

static ULONG BLITZ_ROUTINE _GetMedVolume_c(UWORD ch BL_REG(d0))
{
    return getclear(ch, trk_prevvol);
}
BLITZ_THUNK(_GetMedVolume, _GetMedVolume_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE _GetMedNote_c(UWORD ch BL_REG(d0))
{
    return getclear(ch, trk_prevnote);
}
BLITZ_THUNK(_GetMedNote, _GetMedNote_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE _GetMedInstr_c(UWORD ch BL_REG(d0))
{
    return getclear(ch, trk_previnstr);
}
BLITZ_THUNK(_GetMedInstr, _GetMedInstr_c, "d1/a0-a1");

static void BLITZ_ROUTINE _SetMedMask_c(UWORD m BL_REG(d0))
{
    ChanMask = m;
}
BLITZ_THUNK(_SetMedMask, _SetMedMask_c, "d0-d1/a0-a1");
