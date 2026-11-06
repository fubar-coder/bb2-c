/*
 * vallib - command library 120: Val Int Sgn Frac QFrac QWrap Rnd QAbs Abs
 * QLimit NTSC DispHeight VPos True False QAngle.
 *
 * C translation of bblibsrc/vallib ("ALRIGHTY - A VAL( FUNCITON"; binary
 * of the CD's acidlibs checked: same as the source, the table arc.bin of
 * QAngle is taken from the binary).
 *
 * Floats are Motorola FFP: all float arithmetic is done by mathffp (a6 =
 * ffplib base, !libs #ffplib,#la6) with the same calls and the same
 * operands as the original.
 *
 * Int/Sgn/Abs take any numeric type (!args{8}, #unknown): the compiler
 * passes the value in d0 and pushes the type as a word on the a2 stack
 * (1 byte, 2 word, 3 long, 4 quick, 5 float); the routine pops it
 * (MOVE -(a2),d1).  Val pops the string length (MOVE.l -(a2),d2) and
 * sets a3 to the string (frees the work area).
 * DIFF: a2 is a register parameter of the C routines that read it
 * (const UBYTE *args BL_REG(a2), read with negative offsets) and is left
 * unchanged; the original returns with a2 decremented (by 2 resp. 4).
 * The compiler sets a2 before every library call (preplibst: LEA
 * N(a7),a2) and never reads it afterwards; library code relies on a2
 * being preserved (GCC: a2 is callee-saved).
 *
 * Registers: the original changes d0-d7 (Val, Rnd(n)), a0/a1 through
 * mathffp; the thunks keep d1/a0/a1 for all routines (more than the
 * original, never less).  The CCR is no result.
 *
 * Inline routines (`$a000,len` prefix, copied into the program): QFrac,
 * QWrap, QAbs, VPos, True, False - assembler text, as in the original.
 *
 * Rnd reads hardware (VHPOSR $DFF006, JOY0DAT $DFF00A, CIA-A timer A low
 * $BFE601) in the original order; the sequence depends on the timing and
 * can only be compared bit for bit in theory (the tests check ranges).
 *
 * Error checks (debugger only, after `errors`): intchk for Int and Sgn -
 * type >= 6 (a string) -> runtime error "Illegal Type".
 */
#include "blitzlib.h"
#include <graphics/gfxbase.h>
#include <stddef.h>

/* a2 = the compiler's stack of string lengths (longs) / types (words),
   read through a local cursor `sp` (a copy of a2; a2 is not changed):
   BL_POPL(sp) = MOVE.l -(a2),dN, BL_POPW(sp) = MOVE -(a2),dN */

/* entry points (thunks / inline code, defined below) */
void doval(void), doint(void), dosgn(void), doffrac(void), doqfrac(void);
void doqwrap(void), dornd1(void), dornd2(void), doqabs(void), dofabs(void);
void doqlimit(void), dontsc(void), dodispheight(void), dobeamy(void);
void dotrue(void), dofalse(void), doqangle(void), intchk(void);

/* !libheader{#vallib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_vallib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_FLOAT);              /* return a FLOAT */
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(doval, 0, 0);
BLITZ_NAME("Val", "(String$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(doint, intchk, 0);
BLITZ_NAME("Int", "(Expression)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(dosgn, intchk, 0);
BLITZ_NAME("Sgn", "(Expression)");

BLITZ_AFUNCTION(BL_FLOAT);
BLITZ_ARGS(BL_FLOAT);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(doffrac, 0, 0);
BLITZ_NAME("Frac", "(Expression)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(doqfrac, 0, 0);
BLITZ_NAME("QFrac", "(Quick)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_QUICK, BL_QUICK, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(doqwrap, 0, 0);
BLITZ_NAME("QWrap", "(Quick,Low,High)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dornd1, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dornd2, 0, 0);
BLITZ_NAME("Rnd", "[(Range)]");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(doqabs, 0, 0);
BLITZ_NAME("QAbs", "(Quick)");

BLITZ_AFUNCTION(BL_FLOAT);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(dofabs, 0, 0);
BLITZ_NAME("Abs", "(Expression)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_QUICK, BL_QUICK, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(doqlimit, 0, 0);
BLITZ_NAME("QLimit", "(Quick,Low,High)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_graphicslib, BL_LA(0));
BLITZ_SUBS(dontsc, 0, 0);
BLITZ_NAME("NTSC", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_graphicslib, BL_LA(0));
BLITZ_SUBS(dodispheight, 0, 0);
BLITZ_NAME("DispHeight", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dobeamy, 0, 0);
BLITZ_NAME("VPos", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dotrue, 0, 0);
BLITZ_NAME("True", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dofalse, 0, 0);
BLITZ_NAME("False", "");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doqangle, 0, 0);
BLITZ_NAME("QAngle", "(Src X,Src Y,Dest X,Dest Y)");

BLITZ_LIBFIN();

/* ----------------------------------------------------------- Rnd state */

static UWORD last;
static UWORD sweor;

/* -------------------------------------------------------------- QAngle */

/* octant table 'oct: Dc.l per octant (index = d2 = 0,4,...,28): high word
   = base angle, low word = $FFFF to mirror the arc value (EOR) */
static const ULONG oct[8] = {
    0x00000000, 0x4000ffff, 0x0000ffff, 0xc0000000,
    0x8000ffff, 0x40000000, 0x80000000, 0xc000ffff,
};

/* 'arc: IncBin arc.bin - arctan(i/512) for i = 0..511, $2000 = 45 degrees
   (from the binary) */
static const UWORD arc[512] = {
    0x0000, 0x0014, 0x0028, 0x003d, 0x0051, 0x0065, 0x007a, 0x008e,
    0x00a2, 0x00b7, 0x00cb, 0x00e0, 0x00f4, 0x0108, 0x011d, 0x0131,
    0x0145, 0x015a, 0x016e, 0x0182, 0x0197, 0x01ab, 0x01bf, 0x01d4,
    0x01e8, 0x01fc, 0x0211, 0x0225, 0x0239, 0x024e, 0x0262, 0x0276,
    0x028a, 0x029f, 0x02b3, 0x02c7, 0x02dc, 0x02f0, 0x0304, 0x0318,
    0x032d, 0x0341, 0x0355, 0x0369, 0x037e, 0x0392, 0x03a6, 0x03ba,
    0x03ce, 0x03e3, 0x03f7, 0x040b, 0x041f, 0x0433, 0x0447, 0x045c,
    0x0470, 0x0484, 0x0498, 0x04ac, 0x04c0, 0x04d4, 0x04e8, 0x04fc,
    0x0510, 0x0524, 0x0539, 0x054d, 0x0561, 0x0575, 0x0589, 0x059d,
    0x05b1, 0x05c5, 0x05d8, 0x05ec, 0x0600, 0x0614, 0x0628, 0x063c,
    0x0650, 0x0664, 0x0678, 0x068c, 0x069f, 0x06b3, 0x06c7, 0x06db,
    0x06ef, 0x0702, 0x0716, 0x072a, 0x073e, 0x0751, 0x0765, 0x0779,
    0x078d, 0x07a0, 0x07b4, 0x07c7, 0x07db, 0x07ef, 0x0802, 0x0816,
    0x0829, 0x083d, 0x0851, 0x0864, 0x0878, 0x088b, 0x089f, 0x08b2,
    0x08c5, 0x08d9, 0x08ec, 0x0900, 0x0913, 0x0926, 0x093a, 0x094d,
    0x0960, 0x0974, 0x0987, 0x099a, 0x09ae, 0x09c1, 0x09d4, 0x09e7,
    0x09fa, 0x0a0e, 0x0a21, 0x0a34, 0x0a47, 0x0a5a, 0x0a6d, 0x0a80,
    0x0a93, 0x0aa6, 0x0ab9, 0x0acc, 0x0adf, 0x0af2, 0x0b05, 0x0b18,
    0x0b2b, 0x0b3e, 0x0b51, 0x0b63, 0x0b76, 0x0b89, 0x0b9c, 0x0bae,
    0x0bc1, 0x0bd4, 0x0be7, 0x0bf9, 0x0c0c, 0x0c1f, 0x0c31, 0x0c44,
    0x0c56, 0x0c69, 0x0c7b, 0x0c8e, 0x0ca0, 0x0cb3, 0x0cc5, 0x0cd8,
    0x0cea, 0x0cfc, 0x0d0f, 0x0d21, 0x0d33, 0x0d46, 0x0d58, 0x0d6a,
    0x0d7d, 0x0d8f, 0x0da1, 0x0db3, 0x0dc5, 0x0dd7, 0x0de9, 0x0dfc,
    0x0e0e, 0x0e20, 0x0e32, 0x0e44, 0x0e56, 0x0e67, 0x0e79, 0x0e8b,
    0x0e9d, 0x0eaf, 0x0ec1, 0x0ed3, 0x0ee4, 0x0ef6, 0x0f08, 0x0f1a,
    0x0f2b, 0x0f3d, 0x0f4f, 0x0f60, 0x0f72, 0x0f83, 0x0f95, 0x0fa6,
    0x0fb8, 0x0fc9, 0x0fdb, 0x0fec, 0x0ffe, 0x100f, 0x1020, 0x1032,
    0x1043, 0x1054, 0x1066, 0x1077, 0x1088, 0x1099, 0x10aa, 0x10bb,
    0x10cd, 0x10de, 0x10ef, 0x1100, 0x1111, 0x1122, 0x1133, 0x1144,
    0x1155, 0x1165, 0x1176, 0x1187, 0x1198, 0x11a9, 0x11ba, 0x11ca,
    0x11db, 0x11ec, 0x11fc, 0x120d, 0x121e, 0x122e, 0x123f, 0x124f,
    0x1260, 0x1270, 0x1281, 0x1291, 0x12a2, 0x12b2, 0x12c2, 0x12d3,
    0x12e3, 0x12f3, 0x1303, 0x1314, 0x1324, 0x1334, 0x1344, 0x1354,
    0x1364, 0x1375, 0x1385, 0x1395, 0x13a5, 0x13b5, 0x13c5, 0x13d4,
    0x13e4, 0x13f4, 0x1404, 0x1414, 0x1424, 0x1433, 0x1443, 0x1453,
    0x1463, 0x1472, 0x1482, 0x1492, 0x14a1, 0x14b1, 0x14c0, 0x14d0,
    0x14df, 0x14ef, 0x14fe, 0x150e, 0x151d, 0x152c, 0x153c, 0x154b,
    0x155a, 0x1569, 0x1579, 0x1588, 0x1597, 0x15a6, 0x15b5, 0x15c4,
    0x15d4, 0x15e3, 0x15f2, 0x1601, 0x1610, 0x161f, 0x162d, 0x163c,
    0x164b, 0x165a, 0x1669, 0x1678, 0x1686, 0x1695, 0x16a4, 0x16b3,
    0x16c1, 0x16d0, 0x16de, 0x16ed, 0x16fc, 0x170a, 0x1719, 0x1727,
    0x1736, 0x1744, 0x1752, 0x1761, 0x176f, 0x177d, 0x178c, 0x179a,
    0x17a8, 0x17b6, 0x17c5, 0x17d3, 0x17e1, 0x17ef, 0x17fd, 0x180b,
    0x1819, 0x1827, 0x1835, 0x1843, 0x1851, 0x185f, 0x186d, 0x187b,
    0x1889, 0x1897, 0x18a4, 0x18b2, 0x18c0, 0x18ce, 0x18db, 0x18e9,
    0x18f7, 0x1904, 0x1912, 0x191f, 0x192d, 0x193b, 0x1948, 0x1955,
    0x1963, 0x1970, 0x197e, 0x198b, 0x1998, 0x19a6, 0x19b3, 0x19c0,
    0x19ce, 0x19db, 0x19e8, 0x19f5, 0x1a02, 0x1a0f, 0x1a1d, 0x1a2a,
    0x1a37, 0x1a44, 0x1a51, 0x1a5e, 0x1a6b, 0x1a78, 0x1a84, 0x1a91,
    0x1a9e, 0x1aab, 0x1ab8, 0x1ac5, 0x1ad1, 0x1ade, 0x1aeb, 0x1af7,
    0x1b04, 0x1b11, 0x1b1d, 0x1b2a, 0x1b37, 0x1b43, 0x1b50, 0x1b5c,
    0x1b69, 0x1b75, 0x1b81, 0x1b8e, 0x1b9a, 0x1ba7, 0x1bb3, 0x1bbf,
    0x1bcb, 0x1bd8, 0x1be4, 0x1bf0, 0x1bfc, 0x1c09, 0x1c15, 0x1c21,
    0x1c2d, 0x1c39, 0x1c45, 0x1c51, 0x1c5d, 0x1c69, 0x1c75, 0x1c81,
    0x1c8d, 0x1c99, 0x1ca5, 0x1cb0, 0x1cbc, 0x1cc8, 0x1cd4, 0x1ce0,
    0x1ceb, 0x1cf7, 0x1d03, 0x1d0e, 0x1d1a, 0x1d26, 0x1d31, 0x1d3d,
    0x1d48, 0x1d54, 0x1d5f, 0x1d6b, 0x1d76, 0x1d82, 0x1d8d, 0x1d98,
    0x1da4, 0x1daf, 0x1dbb, 0x1dc6, 0x1dd1, 0x1ddc, 0x1de8, 0x1df3,
    0x1dfe, 0x1e09, 0x1e14, 0x1e1f, 0x1e2b, 0x1e36, 0x1e41, 0x1e4c,
    0x1e57, 0x1e62, 0x1e6d, 0x1e78, 0x1e83, 0x1e8e, 0x1e98, 0x1ea3,
    0x1eae, 0x1eb9, 0x1ec4, 0x1ecf, 0x1ed9, 0x1ee4, 0x1eef, 0x1efa,
    0x1f04, 0x1f0f, 0x1f1a, 0x1f24, 0x1f2f, 0x1f39, 0x1f44, 0x1f4e,
    0x1f59, 0x1f63, 0x1f6e, 0x1f78, 0x1f83, 0x1f8d, 0x1f98, 0x1fa2,
    0x1fac, 0x1fb7, 0x1fc1, 0x1fcb, 0x1fd6, 0x1fe0, 0x1fea, 0x1ff4,
};

/* DIVU.w y,d0: quotient in the low word, remainder in the high word; on
   overflow (quotient > $FFFF, i.e. high word >= y) d0 is unchanged.
   Restoring division (no libgcc on this target). */
static ULONG divu(ULONG dividend, UWORD y)
{
    ULONG rem = dividend >> 16;
    UWORD q = 0;
    WORD i;

    if (rem >= y)
        return dividend;
    for (i = 15; i >= 0; i--) {
        rem = rem << 1 | ((dividend >> i) & 1);
        q <<= 1;
        if (rem >= y) {
            rem -= y;
            q |= 1;
        }
    }
    return rem << 16 | q;
}

/**
 * @brief QAngle(Src X,Src Y,Dest X,Dest Y) - the angle of the line dest -> src.
 *
 * Octant from the signs and the steepness, arctan from the table arc
 * (arc.bin of the original), mirrored and offset per octant (oct).  Word
 * arithmetic as in the original: NEG of -32768 stays negative, CMP/BMI is
 * the sign of the 16 bit difference (not a signed compare), DIVU with
 * overflow leaves the dividend unchanged.
 *
 * @param sx d0: source x
 * @param sy d1: source y
 * @param dx d2: destination x
 * @param dy d3: destination y
 * @return d0: Quick, the fraction of a full turn (0..$FFFF in the low word)
 */
static ULONG BLITZ_ROUTINE doqangle_c(UWORD sx BL_REG(d0), UWORD sy BL_REG(d1),
                                      UWORD dx BL_REG(d2), UWORD dy BL_REG(d3))
{
    UWORD x = sx - dx, y = sy - dy, octant = 0, a, old_x, r;
    ULONG quot;

    if ((WORD)y < 0) {                          /* TST d1:BPL 'hpos */
        octant = 16;
        y = -y;
    }
    if ((WORD)x < 0) {                          /* TST d0:BPL 'wpos */
        octant ^= 8;
        x = -x;
    }
    if ((WORD)(UWORD)(x - y) >= 0) {            /* CMP d1,d0:BMI 'notsteep */
        if (x == y) {                           /* BNE 'neq */
            a = 0x2000;
            goto flow;
        }
        octant ^= 4;                            /* 'neq */
        old_x = x;                              /* EXG d1,d0 */
        x = y;
        y = old_x;
    }
    if (!y) {                                   /* 'notsteep: TST d1:BNE 'noflow */
        a = 0;
        goto flow;
    }
    quot = (ULONG)(LONG)(WORD)x;                /* EXT.l d0 */
    quot = quot << 16 | quot >> 16;             /* SWAP d0 */
    quot = divu(quot, y);                       /* DIVU d1,d0 */
    a = arc[(((UWORD)quot >> 6) & 1022) >> 1];  /* LSR #6,d0:AND #1022,d0 */
flow:
    a ^= (UWORD)oct[octant >> 2];               /* MOVE.l 'oct(pc,d2),d0:EOR d0,d1 */
    r = (UWORD)(oct[octant >> 2] >> 16) + a;    /* CLR d0:SWAP d0:ADD d1,d0 */
    r = -r;                                     /* NEG d0 */
    r -= 0x4000;                                /* SUB #$4000,d0 */
    return r;
}
BLITZ_THUNK(doqangle, doqangle_c, "d1/a0-a1");

/* ------------------------------------------------ True, False, VPos */

/**
 * @brief True - the constant -1 (inline: MOVEQ #-1,d0).
 * @par Result
 * d0: -1 (a word function: Blitz's true)
 */
BLITZ_INLINE(dotrue, "   moveq   #-1,d0");

/**
 * @brief False - the constant 0 (inline: MOVEQ #0,d0).
 * @par Result
 * d0: 0
 */
BLITZ_INLINE(dofalse, "   moveq   #0,d0");

/**
 * @brief VPos - the vertical beam position (inline).
 *
 * MOVE.l $dff004,d0 (pick up whole long word...):LSR.l #8,d0:AND #$1ff,d0
 * (the AND is word sized: the upper word keeps VPOSR>>8).
 *
 * @par Result
 * d0: the beam line 0..511 in the low word
 */
BLITZ_INLINE(dobeamy,
    "   move.l  0xdff004,d0\n"
    "   lsr.l   #8,d0\n"
    "   and.w   #0x1ff,d0");

/* ------------------------------------------------- NTSC, DispHeight */

/* the original tests the low byte of DisplayFlags: BTST #0,207(a0) */
static_assert(offsetof(struct GfxBase, DisplayFlags) == 206);

/**
 * @brief DispHeight - the display height: 200 for NTSC or 256 for PAL.
 *
 * Bit 0 of the low byte of GfxBase->DisplayFlags (207(a0)).  MOVE.w: the
 * upper word of d0 is kept.
 *
 * @param hiword d0: the caller's d0 (its upper word is kept)
 * @param gfx a0: the graphics base
 * @return d0: 200 or 256 in the low word
 */
static ULONG BLITZ_ROUTINE dodispheight_c(ULONG hiword BL_REG(d0),
                                          const struct GfxBase *gfx BL_REG(a0))
{
    return (hiword & 0xffff0000) | ((gfx->DisplayFlags & NTSC) ? 200 : 256);
}
BLITZ_THUNK(dodispheight, dodispheight_c, "d1/a0-a1");

/**
 * @brief NTSC - whether the display is NTSC.
 *
 * BTST #0,207(a0):SNE d0:EXT d0 (upper word of d0 kept).
 *
 * @param hiword d0: the caller's d0 (its upper word is kept)
 * @param gfx a0: the graphics base
 * @return d0: -1 ($FFFF in the low word) for NTSC, 0 for PAL
 */
static ULONG BLITZ_ROUTINE dontsc_c(ULONG hiword BL_REG(d0),
                                    const struct GfxBase *gfx BL_REG(a0))
{
    return (hiword & 0xffff0000) | ((gfx->DisplayFlags & NTSC) ? 0xffff : 0);
}
BLITZ_THUNK(dontsc, dontsc_c, "d1/a0-a1");

/* ---------------------------------------------------------- Frac, Abs */

/**
 * @brief Frac(Expression) - the fractional part of a float.
 *
 * f - Floor(f) for positive, f - Ceil(f) for negative f (FFP sign = bit
 * 7): SPSub with d0 = f, d1 = Floor/Ceil.
 *
 * @param f d0: the float (FFP)
 * @param ffp a6: the mathffp base
 * @return d0: the fraction (FFP)
 */
static ULONG BLITZ_ROUTINE doffrac_c(ULONG f BL_REG(d0), const void *ffp BL_REG(a6))
{
    ULONG i;

    if (f & 0x80)                               /* BTST #7,d0 */
        i = BLITZ_BASEJSR1(ffp, LVO_SPCeil, f);
    else
        i = BLITZ_BASEJSR1(ffp, LVO_SPFloor, f);
    return BLITZ_BASEJSR2(ffp, LVO_SPSub, f, i);
}
BLITZ_THUNK(doffrac, doffrac_c, "d1/a0-a1");

/**
 * @brief Abs(Expression) - the absolute value, as a float.
 *
 * The value as Float with the sign bit (bit 7) cleared: float as is,
 * quick via floatquicklib ($D300, Quick 0 -> $80 -> 0), byte/word sign
 * extended, everything else (long, and types >= 6) with SPFlt.
 *
 * @param v d0: the value
 * @param ffp a6: the mathffp base
 * @param args a2: the argument stack, -2 = the type word (1 byte, 2 word,
 *        3 long, 4 quick, 5 float)
 * @return d0: |v| (FFP)
 */
static ULONG BLITZ_ROUTINE dofabs_c(ULONG v BL_REG(d0), const void *ffp BL_REG(a6),
                                    const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    UWORD type = BL_POPW(sp);

    if (type == 5)                              /* 'float */
        ;
    else if (type == 4)                         /* 'quick: ALibJsr $d300 */
        v = BLITZ_ALIBJSR_A6(TOK_qutofl, ffp, v);
    else {
        if (type < 2)                           /* 'byte: EXT d0 */
            v = (UWORD)(WORD)(BYTE)v;
        if (type <= 2)                          /* 'word: EXT.l d0 */
            v = (LONG)(WORD)v;
        v = BLITZ_BASEJSR1(ffp, LVO_SPFlt, v);  /* 'long */
    }
    return v & ~0x80UL;                         /* BCHG #7,d0:BNE 'skip:BCHG #7,d0 */
}
BLITZ_THUNK(dofabs, dofabs_c, "d1/a0-a1");

/* ------------------------------------------------------------ Int, Sgn */

/**
 * @brief Int(Expression) - the integer part as a long.
 *
 * Float with SPFix (truncates), quick truncated towards 0 (negative:
 * NEG.l, integer part, NEG), byte/word sign extended, long as is.
 *
 * Errors (debugger only, intchk): a string -> "Illegal Type".
 *
 * @param v d0: the value
 * @param ffp a6: the mathffp base
 * @param args a2: the argument stack, -2 = the type word
 * @return d0: the long
 */
static ULONG BLITZ_ROUTINE doint_c(ULONG v BL_REG(d0), const void *ffp BL_REG(a6),
                                   const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    UWORD type = BL_POPW(sp);

    if (type >= 4) {                            /* CMP #4,d1:BCC 'over3 */
        if (type != 4)                          /* 'float: JMP -30(a6) */
            return BLITZ_BASEJSR1(ffp, LVO_SPFix, v);
        if ((LONG)v >= 0)                       /* 'pos: CLR d0:SWAP d0:EXT.l d0 */
            return (LONG)(WORD)(v >> 16);
        v = -v;                                 /* NEG.l d0:CLR d0:SWAP d0 */
        return (LONG)(WORD)-(WORD)(v >> 16);    /* NEG d0:EXT.l d0 */
    }
    if (type == 3)                              /* 'done */
        return v;
    if (type != 2)                              /* EXT d0 */
        v = (UWORD)(WORD)(BYTE)v;
    return (LONG)(WORD)v;                       /* 'word: EXT.l d0 */
}
BLITZ_THUNK(doint, doint_c, "d1/a0-a1");

/**
 * @brief Sgn(Expression) - the sign: -1, 0 or 1.
 *
 * Float through SPTst, long and quick tested as long, byte (type 1) and
 * word (types 0, 2) sign extended.
 *
 * Errors (debugger only, intchk): a string -> "Illegal Type".
 *
 * BUG(orig) (#301): mathffp SPTst tests D1, not D0 - and d1 holds the type
 * (MOVE -(a2),d1: 5 in the low word, the caller's upper word), so Sgn of
 * a float is 1 whatever the value (Sgn(-1.0) = Sgn(0.0) = 1).  Kept: d1
 * is passed as the original has it.
 *
 * @param v d0: the value
 * @param caller_d1 d1: the caller's d1 (its upper word goes to SPTst)
 * @param ffp a6: the mathffp base
 * @param args a2: the argument stack, -2 = the type word
 * @return d0: Quick -1, 0 or 1 ($FFFF0000, 0, $00010000)
 */
static ULONG BLITZ_ROUTINE dosgn_c(ULONG v BL_REG(d0), ULONG caller_d1 BL_REG(d1),
                                   const void *ffp BL_REG(a6), const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    UWORD type = BL_POPW(sp);

    if (type > 4)                               /* 'float: JSR -48(a6) */
        v = BLITZ_BASEJSR2(ffp, LVO_SPTst, v, (caller_d1 & 0xffff0000) | type);
    else if (type != 4 && type != 3) {
        if (type == 1)                          /* EXT d0 */
            v = (UWORD)(WORD)(BYTE)v;
        v = (LONG)(WORD)v;                      /* 'word: EXT.l d0 */
    }
    if ((LONG)v < 0)                            /* 'minus: MOVEQ #-1,d0:SWAP d0:CLR d0 */
        return 0xffff0000;
    if (!v)                                     /* 'zilch */
        return 0;
    return 0x00010000;                          /* 'plus: MOVEQ #1,d0:SWAP d0 */
}
BLITZ_THUNK(dosgn, dosgn_c, "d1/a0-a1");

/**
 * @brief intchk - error check of Int and Sgn (debugger only).
 *
 * CMP #6,-2(a2):BCC err1:RTS - the type word on the a2 stack (not popped)
 * must be numeric; keeps all registers.
 *
 * Errors: type >= 6 (a string) -> runtime error "Illegal Type".
 *
 * @param args a2: the argument stack, -2 = the type word
 */
static void BLITZ_ERRCHK intchk_c(const UBYTE *args BL_REG(a2))
{
    if (((const UWORD *)args)[-1] >= 6)
        BLITZ_RUNERR("Illegal Type");           /* err1: MOVE.l #mess1,d0:TRAP #0 */
}
BLITZ_ERRCHK_THUNK(intchk, intchk_c, "d0-d1/a0-a1");

/* ------------------------------------------------ QFrac, QWrap, QAbs */

/**
 * @brief QFrac(Quick) - the fraction of |q| (inline).
 *
 * TST.l d0:BPL:NEG.l d0, SWAP:CLR:SWAP.
 *
 * @arg q d0: the Quick value
 * @par Result
 * d0: Quick, the fraction of |q| (the low word)
 */
BLITZ_INLINE(doqfrac,
    "   tst.l   d0\n"
    "   bpl.s   3f\n"
    "   neg.l   d0\n"
    "3: swap    d0\n"
    "   clr.w   d0\n"
    "   swap    d0");

/**
 * @brief QWrap(Quick,Low,High) - wrap q into low..high (inline).
 *
 * Below low: +high-low, at or above high: -high+low (once, signed long
 * compares).
 *
 * @arg q d0: the Quick value
 * @arg low d1: the lower limit
 * @arg high d2: the upper limit (exclusive)
 * @par Result
 * d0: the wrapped value
 */
BLITZ_INLINE(doqwrap,
    "   cmp.l   d1,d0\n"
    "   bge.s   3f\n"
    "   sub.l   d1,d0\n"
    "   add.l   d2,d0\n"
    "   bra.s   2f\n"
    "3: cmp.l   d2,d0\n"
    "   blt.s   2f\n"
    "   sub.l   d2,d0\n"
    "   add.l   d1,d0");

/**
 * @brief QAbs(Quick) - the absolute value (inline).
 *
 * NEG.l d0:BPL 'skip:NEG.l d0 ($80000000 stays).
 *
 * @arg q d0: the Quick value
 * @par Result
 * d0: |q|
 */
BLITZ_INLINE(doqabs,
    "   neg.l   d0\n"
    "   bpl.s   2f\n"
    "   neg.l   d0");

/**
 * @brief QLimit(Quick,Low,High) - limit q to low..high.
 *
 * Signed: below low -> low, at or above high -> high (CMP.l d2,d0:BLT -
 * q = high also gives high, low wins if low > high only below low).
 *
 * @param q d0: the Quick value
 * @param lo d1: the lower limit
 * @param hi d2: the upper limit
 * @return d0: the limited value
 */
static LONG BLITZ_ROUTINE doqlimit_c(LONG q BL_REG(d0), LONG lo BL_REG(d1), LONG hi BL_REG(d2))
{
    if (q < lo)                                 /* CMP.l d1,d0:BGE 'skip */
        return lo;
    if (q >= hi)                                /* CMP.l d2,d0:BLT 'skip2 */
        return hi;
    return q;
}
BLITZ_THUNK(doqlimit, doqlimit_c, "d1/a0-a1");

/* ----------------------------------------------------------------- Rnd */

/* genrnd ("not bad, this one...") - 16 random bits in the low word of d0
   (the upper word is the caller's): VHPOSR ^ ror8(JOY0DAT) ^
   rol(JOY0DAT-low<<8 | CIA-A TALO, last) ^ last ^ sweor, with sweor =
   ror1(sweor ^ $AAAA); last = the result.  MOVE.b $bfe601,d1 only
   replaces the low byte of d1, the high byte is still the rotated
   JOY0DAT.  ROL.w d2,d1 rotates by last mod 64 = mod 16 for a word. */
static UWORD genrnd(void)
{
    UWORD r, noise, n;

    r = *(volatile UWORD *)0xdff006;            /* MOVE $dff006,d0 */
    noise = *(volatile UWORD *)0xdff00a;        /* MOVE $dff00a,d1:ROR #8,d1 */
    noise = noise >> 8 | noise << 8;
    r ^= noise;
    noise = (noise & 0xff00) | *(volatile UBYTE *)0xbfe601; /* MOVE.b $bfe601,d1 */
    n = last & 15;                              /* MOVE last(pc),d2:ROL d2,d1 */
    if (n)
        noise = noise << n | noise >> (16 - n);
    r ^= noise;
    r ^= last;                                  /* EOR d2,d0 */
    noise = sweor ^ 0xaaaa;                     /* EOR #$aaaa,d1:ROR #1,d1 */
    noise = noise >> 1 | noise << 15;
    sweor = noise;
    r ^= noise;
    last = r;
    return r;
}

/**
 * @brief Rnd - a random fraction.
 *
 * MOVEQ #0,d0 + genrnd (hardware noise, see genrnd).
 *
 * @return d0: Quick 0..$FFFF/$10000 (16 random bits in the low word)
 */
static ULONG BLITZ_ROUTINE dornd1_c(void)
{
    return genrnd();
}
BLITZ_THUNK(dornd1, dornd1_c, "d1/a0-a1");

/**
 * @brief Rnd(Range) - a random value 0..range.
 *
 * MULU range.w,genrnd (range as unsigned word: Rnd(-1) = Rnd(65535)).
 *
 * @param range d0: the range (unsigned word)
 * @return d0: Quick 0..range (exclusive)
 */
static ULONG BLITZ_ROUTINE dornd2_c(UWORD range BL_REG(d0))
{
    return (ULONG)genrnd() * range;
}
BLITZ_THUNK(dornd2, dornd2_c, "d1/a0-a1");

/* ----------------------------------------------------------------- Val */

/* parse state of doval2: a3 (characters), d2.w (count), d3 */
struct valstate {
    const UBYTE *p;
    WORD n;
};

/* getdec - get a decimal number into d7, 10^ in d6 (both mod 2^32);
   d3 = the character that ended it (or the last digit's value / 0 if the
   count ran out) */
static UWORD getdec(struct valstate *s, ULONG *num, ULONG *scale)
{
    UWORD ch = 0;
    ULONG a = 0, m = 1;

    while (--s->n >= 0) {                       /* SUBQ #1,d2:BMI 'done */
        ch = *s->p++;                           /* MOVE.b (a3)+,d3 */
        if (ch < '0' || ch > '9')
            break;
        ch -= '0';
        m = (m << 1) + (m << 3);                /* multiply d6 and d7 by ten */
        a = (a << 1) + (a << 3) + ch;           /* add digit to d7 */
    }
    *num = a;
    *scale = m;
    return ch;
}

/* 'notneg - integer part, optionally "." and fraction:
   SPFlt(int) [+ SPDiv(SPFlt(frac), SPFlt(10^n))].  No fraction digits
   give n = 0 (divisor 1, + 0.0); BUG(orig) (#304): 32 or more fraction digits
   make 10^n mod 2^32 = 0 and the fraction is dropped. */
static ULONG notneg(const void *ffp, struct valstate *s)
{
    ULONG scale, ipart, x;
    ULONG inum;                                 /* d7: the integer part */
    ULONG frac;                                 /* d7: the fraction digits */
    ULONG fracflt;                              /* d7: the fraction as float */

    if (getdec(s, &inum, &scale) != '.')
        return BLITZ_BASEJSR1(ffp, LVO_SPFlt, inum); /* that's it! */
    ipart = BLITZ_BASEJSR1(ffp, LVO_SPFlt, inum);
    getdec(s, &frac, &scale);
    if (!scale)
        return ipart;
    x = BLITZ_BASEJSR1(ffp, LVO_SPFlt, scale);  /* divisor to float */
    fracflt = BLITZ_BASEJSR1(ffp, LVO_SPFlt, frac); /* fraction to float */
    x = BLITZ_BASEJSR2(ffp, LVO_SPDiv, fracflt, x);
    return BLITZ_BASEJSR2(ffp, LVO_SPAdd, x, ipart);
}

/* gethex - "$" and hex digits (0-9, A-F, a-f), byte arithmetic as in the
   original; the count still includes the "$" (DBRA starts with it), so
   one byte behind the string is read: BUG(orig) (#302).  In compiled programs
   that byte is the terminating 0 of the string argument (tested:
   Val(Left$("$123",2)) = 1), so it has no visible effect there. */
static ULONG gethex(const void *ffp, struct valstate *s)
{
    ULONG v = 0;
    UBYTE c;

    s->p++;
    while (--s->n != -1) {                      /* inhx: DBRA d2,nxhx */
        c = *s->p++ - 48;                       /* SUB.b #48,d1:BMI dunhx */
        if ((BYTE)c < 0)
            break;
        if (c >= 10) {                          /* CMP.b #10,d1:BCS gotdd */
            c -= 17;
            if ((BYTE)c < 0)
                break;
            if (c >= 6) {                       /* CMP.b #6,d1:BCS gothd */
                c -= 32;
                if ((BYTE)c < 0 || c >= 6)
                    break;
            }
            c += 10;                            /* gothd */
        }
        v = (v << 4) + c;                       /* gotdd: ASL.l #4,d0:ADD.b d1,d0 */
    }
    return BLITZ_BASEJSR1(ffp, LVO_SPFlt, v);   /* dunhx */
}

/* getbin - "%" and binary digits; count as gethex (BUG(orig) (#302): one byte
   behind the string is read, normally the terminating 0).  ADD.b #255,d1 sets X for "1" and ends the
   number (BGT) for characters above "1". */
static ULONG getbin(const void *ffp, struct valstate *s)
{
    ULONG v = 0;
    UBYTE c;

    s->p++;
    while (--s->n != -1) {                      /* inbin: DBRA d2,nxbin */
        c = *s->p++ - '0';                      /* SUB.b d3,d1:BMI dunbin */
        if ((BYTE)c < 0 || c >= 2)              /* ADD.b #255,d1:BGT dunbin */
            break;
        v = v << 1 | c;                         /* ROXL.l #1,d0 */
    }
    return BLITZ_BASEJSR1(ffp, LVO_SPFlt, v);
}

/* doval2 - calc val of string: leading spaces skipped, then "$" hex, "%"
   binary, "-" negative (SPNeg), else decimal.  The count is the low WORD
   of the length (SUBQ #1,d2:BMI): BUG(orig) (#303) lengths 32769..65536 give 0,
   longer ones count mod 65536.
   BUG(orig) (#302): after "-" the count is not decremented, so one byte behind
   the string is read (normally the terminating 0: Val(Left$("-12",2)) =
   -1 in the tests). */
static ULONG doval2(const void *ffp, struct valstate *s)
{
    for (;;) {                                  /* remove leading spaces... */
        if (--s->n < 0)
            return 0;
        if (*s->p++ != ' ')
            break;
    }
    s->n++;
    s->p--;
    if (*s->p == '$')
        return gethex(ffp, s);
    if (*s->p == '%')
        return getbin(ffp, s);
    if (*s->p == '-') {
        s->p++;
        return BLITZ_BASEJSR1(ffp, LVO_SPNeg, notneg(ffp, s));
    }
    return notneg(ffp, s);
}

/**
 * @brief Val(String$) - the numeric value of a string, as a float.
 *
 * Leading spaces skipped, then "$" hex, "%" binary, "-" negative, else
 * decimal with an optional fraction (doval2).  MOVE.l d0,a3: a3 = the
 * string afterwards (frees the work area).
 *
 * BUG(orig) (#302): hex, binary and negative numbers read one byte behind
 * the string (normally its terminating 0).  BUG(orig) (#303): the length
 * is used as a word (lengths 32769..65536 give 0).  BUG(orig) (#304): 32
 * or more fraction digits drop the fraction.
 *
 * @param str d0: the string
 * @param ffp a6: the mathffp base
 * @param args a2: the argument stack, -4 = the string length (long)
 * @return d0: the value (FFP), 0 for an empty string
 */
static ULONG BLITZ_ROUTINE doval_c(UBYTE *str BL_REG(d0), const void *ffp BL_REG(a6),
                                   const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    struct valstate s;

    s.p = str;
    s.n = (WORD)BL_POPL(sp);                    /* MOVE.l -(a2),d2 (used as word) */
    blitz_string = str;
    return doval2(ffp, &s);
}
BLITZ_THUNK(doval, doval_c, "d1/a0-a1");
