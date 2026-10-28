/*
 * crunchlib - command library 74 (#romuluscrunchlib): Implode, Deplode
 * (Imploder), CrMDecrunch (Crunch-Mania V1.7 "CrM!"/"CrM2"), PPDecrunch
 * (PowerPacker "PP20").
 *
 * C translation of bblibsrc/crunchlib (binary of the CD's acidlibs
 * checked: same as the source).  The three algorithms are third party
 * assembler code (the Imploder cruncher/decruncher, Thomas Schwarz's
 * Crunch-Mania decruncher, the PowerPacker decruncher); they are
 * translated register by register: every value has the width of the
 * original operation (byte/word/long, word results keep the rest of the
 * register where it matters), shift and rotate counts are taken modulo
 * 64 as by the 68000, DBF loops count the low word down to -1.  The
 * crunched output and the decrunched data are therefore byte-identical to
 * the original for every input, including invalid data.
 *
 * Implode(buffer,length): crunches in place, "mode" 8 (MOVEQ #8,d1 of
 * _startcrunch: LSR.l #8 twice leaves mode 0 = the smallest table, window
 * 128 bytes, and the "explode table" flag 0).  Result: -1 = break
 * (left mouse button), 0 = error/not crunchable (< 64 bytes, crunched
 * < 12 bytes or not at least 55 bytes shorter), else the crunched length
 * (data + $2E bytes of tables; the first 12 bytes of the buffer become
 * "IMP!", length, crunched length).  The info routine `Own` is called
 * before every step: it writes the low word of (source-dest) to COLOR00
 * ($DFF180) - the screen background flickers while crunching - and tests
 * the left mouse button ($BFE001 bit 6).
 * Deplode(buffer): -1 = ok, 0 = not "IMP!" / error.
 * CrMDecrunch buffer[,dest]: one argument = in place; then the data are
 * first copied to buffer-MinSecDist (memory BEFORE the buffer is
 * overwritten, as the Crunch-Mania listing requires - BUG(orig), see
 * below).
 * PPDecrunch start,end,dest: end = end of the packed file (the last long
 * holds the original length and the number of empty bits).
 *
 * Kept from the original (BUG(orig)):
 *   - Implode: the match search reads up to 254 bytes past the end of
 *     the data (im30: up to 15, the length measurement im32/im33: up to
 *     254 - the two do not add up; results do not depend on them).
 *   - CrMDecrunch with one argument (destination = source) and with
 *     overlapping buffers: the crunched data are moved to MinSecDist
 *     bytes BEFORE the destination - up to MinSecDist bytes of foreign
 *     memory in front of the buffer are overwritten (the BB2 docs only
 *     say "works like the Deplode command").
 *   - CrMDecrunch: overlap copy of (CrLen>>2) longs with SUBQ/BNE: a
 *     crunched length below 4 copies 4 GB; the address comparisons are
 *     signed (BLE).
 *   - CrM2: ReadTab with a count of 0 reads 65536 table entries (DBF);
 *     the tables are static data inside the library, as in the original.
 *   - PPDecrunch: ReadD1_SUB with an efficiency value 0 reads 65536 bits.
 *   - Implode crunches in place while it searches: a result of 0 (not
 *     crunchable) after at least 64 bytes leaves the buffer partly
 *     overwritten.
 *   - No checks of the data at all: corrupt data write anywhere.
 * Kept from the original, not a bug: after $4012 literal bytes in a row
 * Implode stores the rest of the data uncrunched - a limit of the
 * Imploder format (imtab4); the result decrunches correctly.
 * DIFF: corrupt CrM2 data can make ReadIt index RealTab with any word
 * (and GetBits2 use masks behind AndData2), i.e. read the library's own
 * code and data; the result then depends on the library's memory layout.
 * Valid data and all other formats (also corrupt) give the same result.
 * The original clears its 176 byte work area on the stack before
 * returning (CLR.w (a7)+); the C version leaves its locals.  a5 is not
 * changed during Implode (the original uses it as work register and
 * restores it).
 * Registers: Implode/Deplode keep d2-d7/a2-a6 like the original;
 * CrMDecrunch and PPDecrunch keep all registers (thunks).
 */
#include "blitzlib.h"

/* entry points (defined below) */
ULONG BLITZ_ROUTINE _startcrunch(UBYTE *buf BL_REG(d0), ULONG len BL_REG(d1));
ULONG BLITZ_ROUTINE _decrunch(UBYTE *buf BL_REG(d0));
void _crm(void), _crm2(void), _ppdecrunch(void);

/* !libheader {#romuluscrunchlib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_romuluscrunchlib, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_startcrunch, 0, 0);
BLITZ_NAME("Implode", "Error/CrunchedLen=Bufferadr,DataLen");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_decrunch, 0, 0);
BLITZ_NAME("Deplode", "Success=Startadr");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_crm, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_crm2, 0, 0);
BLITZ_NAME("CrMDecrunch", "Bufferadr");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ppdecrunch, 0, 0);
BLITZ_NAME("PPDecrunch", "Bufferadr,Bufferend,Destinationadr");

BLITZ_LIBFIN();

/* big endian memory access with the width of the original instruction */
#define RB(p)   (*(UBYTE *)(p))
#define RW(p)   (*(UWORD *)(p))
#define RL(p)   (*(ULONG *)(p))

#define ID_IMP  0x494d5021UL    /* "IMP!" */
#define ID_CRM  0x43724d21UL    /* "CrM!" */
#define ID_CRM2 0x43724d32UL    /* "CrM2" */
#define ID_PP20 0x50503230UL    /* "PP20" */

/* ================================================================ Implode */

static const ULONG imtab0[12] = {
    128, 256, 512, 1024, 1792, 3328, 5376, 9472, 20736, 37376, 67840, 67840
};
static const ULONG imtab1[36] = {
    0x5050505, 0x5050505, 0x6060606, 0x5060707, 0x6060606, 0x7070606,
    0x5060707, 0x7070707, 0x8080808, 0x5060708, 0x7070808, 0x8080909,
    0x6070708, 0x7080909, 0x8090A0A, 0x6070708, 0x709090A, 0x80A0B0B,
    0x6070808, 0x709090A, 0x80A0B0C, 0x6070808, 0x709090A, 0x90A0C0D,
    0x6070708, 0x709090C, 0x90A0C0E, 0x6070809, 0x7090A0C, 0x90B0D0F,
    0x6070808, 0x70A0B0B, 0x90C0D10, 0x6080809, 0x70B0C0C, 0x90D0E11
};
static const ULONG imtab2[2] = { 0x2060E, 0x1020304 };
static const ULONG imtab3[3] = { 0x1010101, 0x2030304, 0x405070E };
static const ULONG imtab4[6] = { 0x20002, 0x20002, 0x6000A, 0xA0012,
                                 0x16002A, 0x8A4012 };

/* The work area (a6 of the original, 176 bytes on the stack):
     $00.b explode table flag      $01.b table number
     $02.l a5 of the caller        $06.l info routine
     $0A.l buffer start            $0E.l buffer end
     $12.l data length             $16.l result
     $1A.l window (max. offset)    $1E.l literal counter (not read)
     $22.l source                  $26.l destination
     $2A.w best gain               $2C.b bit buffer   $2D.b bit counter
     $2E.b best length             $30.l literals since the last match
     $34.b[8] match lengths (2..8, long)   $3C.l[8] their offsets
     $5C..$68 code of the best match (copy of $68..$74)
     $68.b offset bits  $69.b length bits  $6A.b literal count bits
     $6C.l offset code  $70.w length code  $72.w literal count code
     $74.l[12] 1<<table bits ($84.. + $74..)   $A4.b[12] table bits */
#define WS      0xB0
#define SB(o)   RB(st + (o))
#define SW(o)   RW(st + (o))
#define SL(o)   RL(st + (o))

typedef LONG infofunc(ULONG gain);

/* Own - the info routine of _startcrunch: d1 (source - dest) to COLOR00,
   0 (break) if the left mouse button is pressed, else -1 */
static LONG own(ULONG gain)
{
    *(volatile UWORD *)0xdff180 = (UWORD)gain;
    if (*(volatile UBYTE *)0xbfe001 & 0x40)
        return -1;
    return 0;
}

/* im26 - call the info routine; TRUE = break */
static int im26(UBYTE *st)
{
    ULONG d0 = SL(6);

    if (d0) {
        d0 = ((infofunc *)d0)(SL(0x22) - SL(0x26));
        d0 = ~d0;
    }
    return d0 != 0;
}

/* im37 - put the low d0 bits of d1 into the bit stream (LSB first);
   returns d1 shifted (LSR.l) */
static ULONG im37(UBYTE *st, UBYTE d0, ULONG d1)
{
    UBYTE d2 = SB(0x2c), d3 = SB(0x2d), x;
    UBYTE *a0 = (UBYTE *)SL(0x26);

    do {
        x = d1 & 1;                     /* LSR.l #1,d1 */
        d1 >>= 1;
        d2 = (UBYTE)((d2 >> 1) | (x << 7));     /* ROXR.b #1,d2 */
        if ((BYTE)--d3 < 0) {
            d3 = 7;
            *a0++ = d2;
            d2 = 0;
        }
    } while (--d0);
    SL(0x26) = (ULONG)a0;
    SB(0x2d) = d3;
    SB(0x2c) = d2;
    return d1;
}

/* im28/im29..im36 - search the matches at the source position.  Returns 0
   if fewer than 3 bytes are left (then the rest is stored as literals),
   else -1 with the best match per length 2..8 and the longest in
   $34/$3C. */
static int im28(UBYTE *st)
{
    UBYTE *a5 = (UBYTE *)SL(0x22);
    ULONG d4 = SL(0x0e), d5;
    UBYTE *a0, *a1, *a2, *a4, *a3 = st + 0x34;
    UWORD d7, d6;
    UBYTE d3;
    ULONG d0 = (ULONG)a5 + 1 + SL(0x1a);

    if (d0 > d4) {
        d0 = d4;
        if (d0 - (ULONG)a5 < 3)
            return 0;
    }
    /* im29 */
    d5 = d0;
    a2 = a5 + 1;
    a4 = a2;
    d7 = 1;
    d3 = *a5;
    for (;;) {
        /* im30: 16 compares per loop test */
        int i;
        for (i = 0; i < 16; i++)
            if (*a2++ == d3)
                goto im32;
        if (d5 > (ULONG)a2)
            continue;
        return -1;                      /* im31 */
im32:
        if (d5 <= (ULONG)a2)
            return -1;
        a0 = a4;
        a1 = a2;
        if (*a0++ != *a1++)
            continue;
        if (*a0++ != *a1++)
            goto im35;
        if (*a0++ == *a1++) {
            UWORD n = 251;              /* DBNE: up to 252 more */
            for (;;) {
                if (*a0++ != *a1++)
                    break;
                if (n-- == 0)
                    break;
            }
        }
        /* im34 */
        if ((ULONG)a1 > d4)
            a1 = (UBYTE *)d4;
im35:
        d6 = (UWORD)((ULONG)a1 - (ULONG)a2);
        if (d7 >= d6)                   /* CMP.w d6,d7: BCC */
            continue;
        d7 = d6;
        if (d6 > 8) {                   /* im36: long match */
            a3[7] = (UBYTE)d6;
            RL(a3 + 0x24) = (ULONG)a2 - (ULONG)a5 - 2;
            if ((UBYTE)d6 == 0xff)
                return -1;
            continue;
        }
        if (a3[(WORD)d6 - 2])
            continue;
        a3[(WORD)d6 - 2] = (UBYTE)d6;
        RL(a3 + (WORD)(d6 << 2)) = (ULONG)a2 - (ULONG)a5 - 2;
    }
}

/* im40 - code of a match: d0 = length, d1 = offset; 0 if it cannot be
   coded (offset or literal count too large) */
static int im40(UBYTE *st, ULONG d0, ULONG d1)
{
    const UBYTE *t2 = (const UBYTE *)imtab2, *t3 = (const UBYTE *)imtab3;
    const UBYTE *a0;
    UWORD d2, d4w;
    UBYTE d6, d3, d5;
    ULONG d7, d4;

    d0 &= 0xff;
    if ((UBYTE)d0 > 13) {               /* im42 */
        SB(0x70) = 0x1f;
        SB(0x71) = (UBYTE)d0;
        SB(0x69) = 13;
        d0 = 5;
    } else if ((UBYTE)d0 > 5) {         /* im41 */
        d0 = (d0 & ~0xffUL) | (UBYTE)(((UBYTE)d0 - 6) | 0xf0);
        SB(0x71) = (UBYTE)d0;
        SB(0x69) = 8;
        d0 = 5;
    } else {
        SB(0x71) = t2[(WORD)d0 - 2];
        SB(0x69) = t2[(WORD)d0 + 2];
    }
    /* im44 */
    d0 = (d0 & ~0xffUL) | (UBYTE)((UBYTE)d0 - 2);
    d2 = (UWORD)SL(0x30);
    a0 = (const UBYTE *)imtab4 + d0 + d0;
    if (d2 < RW(a0)) {
        d6 = t3[(WORD)d0];
        d3 = d6 + 1;
        SB(0x73) = 0;
        d4w = 0;
    } else if (d2 < RW(a0 + 8)) {       /* im45 */
        d6 = t3[(WORD)d0 + 4];
        d3 = d6 + 2;
        SB(0x73) = 2;
        d4w = RW(a0);
    } else if (d2 >= RW(a0 + 0x10)) {   /* im46 */
        return 0;
    } else {                            /* im47 */
        d6 = t3[(WORD)d0 + 8];
        d3 = d6 + 2;
        SB(0x73) = 3;
        d4w = RW(a0 + 8);
    }
    /* im48 */
    SB(0x6a) = d3;
    d2 -= d4w;
    d5 = 16 - d6;
    d2 = (d5 & 63) >= 16 ? 0 : (UWORD)(d2 << (d5 & 63));
    do {                                /* im49 */
        UWORD x = d2 >> 15;             /* ADD.w d2,d2 */
        d2 <<= 1;
        SW(0x72) = (UWORD)((SW(0x72) << 1) | x);        /* ROXL $72(a6) */
    } while (--d6);

    a0 = st + 0x74 + 4 * (WORD)(UWORD)d0;
    if (d1 < RL(a0)) {
        d6 = SB(0xa4 + (WORD)d0);
        d3 = d6 + 1;
        d7 = 0;
        d4 = 0;
    } else if (d1 < RL(a0 + 0x10)) {    /* im50 */
        d6 = SB(0xa4 + 4 + (WORD)d0);
        d3 = d6 + 2;
        d7 = 2;
        d4 = RL(a0);
    } else if (d1 >= RL(a0 + 0x20)) {   /* im51 */
        return 0;
    } else {                            /* im52 */
        d6 = SB(0xa4 + 8 + (WORD)d0);
        d3 = d6 + 2;
        d7 = 3;
        d4 = RL(a0 + 0x10);
    }
    /* im53 */
    SB(0x68) = d3;
    d1 -= d4;
    d5 = 32 - d6;
    d1 = (d5 & 63) >= 32 ? 0 : d1 << (d5 & 63);
    do {                                /* im54: ADD.l d1,d1:ADDX.l d7,d7 */
        ULONG x = d1 >> 31;
        d1 <<= 1;
        d7 = d7 + d7 + x;
    } while (--d6);
    SL(0x6c) = d7;
    return -1;
}

/* im55 - choose the match with the best gain (bits saved); the code goes
   to $5C.., its length to $2E.  Clears the length slots.  TRUE if one was
   found (TST.w $2E(a6)). */
static int im55(UBYTE *st)
{
    UBYTE *a4 = st + 0x34, *a5 = st + 0x3c;

    SW(0x2a) = 0;
    SB(0x2e) = 0;
    do {
        ULONG d1 = RL(a5);
        UBYTE len = *a4++;
        a5 += 4;
        if (len && im40(st, len, d1)) {
            UWORD d0 = (UWORD)(a4[-1] << 3);
            UBYTE b = 0;
            int i;

            b += SB(0x69);
            b += SB(0x68);
            b += SB(0x6a);
            d0 -= b;
            if ((WORD)d0 < 0 || d0 < SW(0x2a))
                continue;
            SW(0x2a) = d0;
            SB(0x2e) = a4[-1];
            for (i = 0; i < 13; i++)
                SB(0x5c + i) = SB(0x68 + i);
        }
    } while (a4 - st != 0x3c);
    RL(a4 - 4) = 0;                     /* CLR.l -(a4) twice */
    RL(a4 - 8) = 0;
    return SW(0x2e) != 0;
}

/* copy one literal byte (im12 / im15) */
static void literal(UBYTE *st)
{
    *(UBYTE *)SL(0x26) = *(UBYTE *)SL(0x22);
    SL(0x22)++;
    SL(0x26)++;
    SL(0x30)++;
    SL(0x1e)++;
}

/* Implode - a0 = buffer, d0 = length, a1 = info routine, d1 = mode,
   a2 = output of the explode table (only with the mode flag) */
static ULONG implode(UBYTE *buf, ULONG len, infofunc *info, ULONG mode,
                     UBYTE *a2)
{
    ULONG work[WS / 4];
    UBYTE *st = (UBYTE *)work;
    const ULONG *a0;
    ULONG d0, d1;
    int i;

    for (i = 0; i < WS / 4; i++)
        work[i] = 0;
    SL(2) = (ULONG)blitz_globals;
    if (len < 0x40)
        goto im23;
    SB(0) = (mode & 0x80) ? 0xff : 0;   /* LSR.l #8,d1:SCS (a6) */
    d1 = mode >> 16;
    if ((UBYTE)d1 >= 12)
        d1 = 0;
    SL(6) = (ULONG)info;
    SL(0x0a) = (ULONG)buf;
    SL(0x22) = (ULONG)buf;
    SL(0x26) = (ULONG)buf;
    SL(0x12) = len;
    SL(0x0e) = (ULONG)buf + len;
    d1 = imtab0[(UWORD)(d1 << 2) >> 2] + 1;
    if (d1 > len)
        d1 = len - 1;
    SL(0x1a) = d1;
    d1--;
    d0 = 0;
    a0 = imtab0;
    while (d1 > *a0++)                  /* im04 */
        d0 = (d0 & ~0xffUL) | (UBYTE)(d0 + 1);
    SB(1) = (UBYTE)d0;
    {
        const UBYTE *s = (const UBYTE *)imtab1 + (UWORD)d0 * 12;
        for (i = 0; i < 12; i++)
            SB(0xa4 + i) = s[i];
    }
    for (i = 0; i < 12; i++)            /* im07 */
        SL(0x74 + 4 * i) = 1UL << (SB(0xa4 + i) & 31);
    for (i = 0; i < 8; i++)             /* im08 */
        SL(0x84 + 4 * i) += SL(0x74 + 4 * i);
    if (SB(0)) {                        /* never with mode 8 */
        for (i = 0; i < 8; i++) {
            RW(a2) = (UWORD)SL(0x74 + 4 * i);
            a2 += 2;
        }
        for (i = 0; i < 12; i++)
            *a2++ = SB(0xa4 + i);
    }
    SB(0x2d) = 7;

    for (;;) {                          /* im12 */
        if (im26(st))
            goto im22;
        if (!im28(st))
            break;
        if (!im55(st)) {
            literal(st);
            if (SL(0x30) < 0x4012)
                continue;
            break;
        }
        /* im13 */
        d1 = im37(st, SB(0x5c), SL(0x60));
        d1 = (d1 & 0xffff0000UL) | SW(0x66);
        d1 = im37(st, SB(0x5e), d1);
        d0 = SB(0x5d);
        d1 = (d1 & 0xffff0000UL) | SW(0x64);
        if ((UBYTE)d0 == 13) {
            UBYTE *d = (UBYTE *)SL(0x26);
            *d++ = (UBYTE)d1;
            SL(0x26) = (ULONG)d;
            d0 = 5;
            d1 = 0x1f;
        }
        im37(st, (UBYTE)d0, d1);        /* im14 */
        SL(0x22) += SB(0x2e);
        SL(0x30) = 0;
    }
    do {                                /* im15 */
        if (im26(st))
            goto im22;
        literal(st);
    } while (SL(0x22) != SL(0x0e));
    if (im26(st))
        goto im22;

    if (!SB(0)) {
        UBYTE *a1, *o;
        ULONG d7;
        UWORD w;

        d0 = SL(0x26) - SL(0x0a);
        if (d0 < 12)
            goto im23;
        d1 = SL(0x12) - d0;
        if (d1 <= 0x36)
            goto im23;
        a1 = (UBYTE *)SL(0x0a);
        o = (UBYTE *)SL(0x26);
        d7 = 0xff00;
        if (d0 & 1) {
            d7 = 0;
            d0++;
            *o++ = 0;
        }
        /* im16: the first 12 bytes go to the end, header in front */
        RL(o + 8) = RL(a1);
        RL(a1) = ID_IMP;
        RL(o + 4) = RL(a1 + 4);
        RL(a1 + 4) = SL(0x12);
        RL(o) = RL(a1 + 8);
        RL(a1 + 8) = d0;
        d0 += 0x2e;
        SL(0x16) = d0;
        RL(o + 12) = SL(0x30);
        w = SB(0x2c) & 0xfe;            /* MOVE.b:AND.w #$FE */
        w |= (UWORD)(1U << (SB(0x2d) & 31));    /* BSET d0,d1 */
        w |= (UWORD)d7;
        RW(o + 0x10) = w;
        o += 0x12;
        for (i = 0; i < 8; i++) {       /* im17 */
            RW(o) = (UWORD)SL(0x74 + 4 * i);
            o += 2;
        }
        for (i = 0; i < 12; i++)        /* im18 */
            *o++ = SB(0xa4 + i);
    } else {                            /* im19, never with mode 8 */
        UBYTE b, *o;

        d0 = SL(0x26) - SL(0x0a);
        d1 = SL(0x12) - d0;
        if (d1 <= 6)
            goto im23;
        b = (UBYTE)((SB(0x2c) & 0xfe) | (1U << (SB(0x2d) & 7)));
        o = (UBYTE *)SL(0x26);
        if (d0 & 1) {
            *o++ = b;
            RL(o) = SL(0x30);
        } else {
            RL(o) = SL(0x30);
            o += 4;
            *o = b;
        }
        d0 += 5;
        SL(0x16) = d0;
    }
im23:
    return SL(0x16);
im22:
    return (ULONG)-1;
}

/* _startcrunch - d0 = buffer, d1 = length */
ULONG BLITZ_ROUTINE _startcrunch(UBYTE *buf BL_REG(d0), ULONG len BL_REG(d1))
{
    return implode(buf, len, own, 8, NULL);
}

/* ================================================================ Deplode */

static const UBYTE I1F8C[4] = { 0x06, 0x0a, 0x0a, 0x12 };
static const UBYTE I1F90[12] = { 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x03,
                                 0x04, 0x04, 0x05, 0x07, 0x0e };

/* the bit stream of the Imploder: ADD.b d3,d3, at 0 the next byte
   (backwards) with ADDX.b d3,d3 (the marker bit comes in at bit 0) */
static int imbit(UBYTE *d3, const UBYTE **a3)
{
    UBYTE v = *d3;
    int c = v >> 7;

    v <<= 1;
    if (v == 0) {
        UBYTE n = *--*a3;
        v = (UBYTE)((n << 1) | c);
        c = n >> 7;
    }
    *d3 = v;
    return c;
}

/* deplode - a0 = crunched data; -1 = ok, 0 = error */
static ULONG deplode(UBYTE *a0)
{
    UBYTE *a4;
    const UBYTE *a3, *a2;
    UBYTE tab[28];
    ULONG d2;
    UBYTE d3;
    int i;

    if (RL(a0) != ID_IMP)
        return 0;
    a4 = a0 + RL(a0 + 4);
    a3 = a0 + RL(a0 + 8);
    a2 = a3;
    RL(a0 + 8) = RL(a2);
    RL(a0 + 4) = RL(a2 + 4);
    RL(a0) = RL(a2 + 8);
    d2 = RL(a2 + 12);
    d3 = (UBYTE)RW(a2 + 16);            /* only the low byte is used */
    if (!(RW(a2 + 16) & 0x8000))
        a3--;
    a2 += 18;
    for (i = 0; i < 28; i++)            /* 7 longs */
        tab[i] = a2[i];

    for (;;) {
        UWORD d4, d0, d1, cnt;
        ULONG d5, src;
        UBYTE n;

        /* I1E70: literal run */
        for (; d2; d2--)
            *--a4 = *--a3;
        /* I1E7A */
        if (a0 >= a4)
            return a3 == a0 ? (ULONG)-1 : 0;
        /* I1E92: match length */
        if (!imbit(&d3, &a3)) {
            d4 = 1; d0 = 0;
        } else if (!imbit(&d3, &a3)) {
            d4 = 2; d0 = 1;
        } else if (!imbit(&d3, &a3)) {
            d4 = 3; d0 = 2;
        } else if (!imbit(&d3, &a3)) {
            d4 = 4; d0 = 3;
        } else {
            UBYTE b = 0;
            if (imbit(&d3, &a3)) {
                b = *--a3;
                b--;
            } else {
                b = (UBYTE)(b + b + imbit(&d3, &a3));
                b = (UBYTE)(b + b + imbit(&d3, &a3));
                b = (UBYTE)(b + b + imbit(&d3, &a3));
                b += 5;
            }
            d4 = b;
            d0 = 3;
        }
        /* I1F08: next literal count */
        d5 = 0;
        d1 = d0;
        if (imbit(&d3, &a3)) {
            if (imbit(&d3, &a3)) {
                d5 = I1F8C[d0];
                d0 = (UBYTE)(d0 + 8);
            } else {
                d5 = 2;
                d0 = (UBYTE)(d0 + 4);
            }
        }
        n = I1F90[d0];
        {
            UWORD w = (UWORD)d2;
            do
                w = (UWORD)(w + w + imbit(&d3, &a3));
            while (--n);
            w += (UWORD)d5;
            d2 = (d2 & 0xffff0000UL) | w;
        }
        /* offset */
        d5 = 0;
        src = 0;
        d0 = d1;
        if (imbit(&d3, &a3)) {
            d1 += d1;
            if (imbit(&d3, &a3)) {
                src = (ULONG)(LONG)(WORD)RW(tab + 8 + d1);
                d0 = (UBYTE)(d0 + 8);
            } else {
                src = (ULONG)(LONG)(WORD)RW(tab + d1);
                d0 = (UBYTE)(d0 + 4);
            }
        }
        n = tab[0x10 + d0];
        do
            d5 = d5 + d5 + imbit(&d3, &a3);
        while (--n);
        src += 1 + d5 + (ULONG)a4;
        {
            const UBYTE *s = (const UBYTE *)src;
            cnt = d4;
            do
                *--a4 = *--s;
            while (cnt-- != 0);
        }
    }
}

/* _decrunch - d0 = crunched data */
ULONG BLITZ_ROUTINE _decrunch(UBYTE *buf BL_REG(d0))
{
    return deplode(buf);
}

/* ============================================================ Crunch-Mania */

/* bit reader state of the CrM decrunchers (d6 bits, d7 count, a2 source) */
struct crmbits {
    ULONG d6;
    UWORD d7;
    const UBYTE *a2;
};

static ULONG ror32(ULONG v, unsigned n)
{
    n &= 31;
    return n ? (v >> n) | (v << (32 - n)) : v;
}

static ULONG rol32(ULONG v, unsigned n)
{
    n &= 31;
    return n ? (v << n) | (v >> (32 - n)) : v;
}

/* BitTest / one bit of ReadIt: the bit shifted out of d6 */
static int crmbit(struct crmbits *b)
{
    int c = b->d6 & 1;

    if (--b->d7 != 0) {
        b->d6 >>= 1;
    } else {                            /* next word into the high word */
        b->d7 = 16;
        b->a2 -= 2;
        b->d6 = ((ULONG)RW(b->a2) << 16) | ((b->d6 >> 1) & 0xffff);
    }
    return c;
}

/* GetBits/GetBits2 - d1 bits (1..16) */
static UWORD crmgetbits(struct crmbits *b, UWORD d1)
{
    UWORD d0 = (UWORD)b->d6;

    b->d6 = (d1 & 63) >= 32 ? 0 : b->d6 >> (d1 & 63);
    b->d7 -= d1;
    if ((WORD)b->d7 <= 0) {
        b->d7 += 16;                    /* ADD.w d3,d7 */
        b->d6 = ror32(b->d6, b->d7 & 63);
        b->a2 -= 2;
        b->d6 = (b->d6 & 0xffff0000UL) | RW(b->a2);
        b->d6 = rol32(b->d6, b->d7 & 63);
    }
    return d0 & (UWORD)((1UL << d1) - 1);       /* AndData */
}

/* start of the bit stream at the end of the data (a2 = end) */
static void crminit(struct crmbits *b, const UBYTE *end)
{
    UWORD d0;
    UWORD d7;

    b->a2 = end - 2;
    d0 = RW(b->a2);                     /* bits in the last word */
    b->a2 -= 4;
    b->d6 = RL(b->a2);
    d7 = (UWORD)(16 - d0);
    b->d6 = (d7 & 63) >= 32 ? 0 : b->d6 >> (d7 & 63);
    b->d7 = d0;
}

/* FastDecruncher ("CrM!") - a1 = dest, a2 = packed data, d1/d2 = their
   lengths */
static void fastdecruncher(UBYTE *a1, const UBYTE *a2, ULONG d1, ULONG d2)
{
    UBYTE *a5 = a1;
    struct crmbits b;
    UWORD d4 = 0;

    a1 += d1;
    crminit(&b, a2 + d2);
    for (;;) {
        UWORD bits, d2w;
        const UBYTE *a3;

        if ((LONG)a1 <= (LONG)a5)
            return;                     /* DecrEnd */
        if (crmbit(&b)) {
            d4 = 0;
insertbytes:
            do
                *--a1 = (UBYTE)crmgetbits(&b, 8);
            while (d4-- != 0);
            continue;
        }
        /* InsertSeq: length */
        if (!crmbit(&b)) {
            bits = 1; d4 = 1;
        } else if (!crmbit(&b)) {
            bits = 2; d4 = 3;
        } else if (!crmbit(&b)) {
            bits = 4; d4 = 7;
        } else {
            bits = 8; d4 = 0x17;
        }
        d4 += crmgetbits(&b, bits);
        if (d4 == 22) {                 /* SpecialInsert */
            d4 = 14;
            bits = crmbit(&b) ? 5 : 14;
            d4 += crmgetbits(&b, bits);
            goto insertbytes;
        }
        if ((WORD)d4 > 22)
            d4--;
        /* distance */
        if (!crmbit(&b)) {
            bits = 9; d2w = 0x20;
        } else if (!crmbit(&b)) {
            bits = 5; d2w = 0;
        } else {
            bits = 14; d2w = 0x220;
        }
        a3 = a1 + (WORD)(UWORD)(crmgetbits(&b, bits) + d2w);
        do
            *--a1 = *--a3;
        while (d4-- != 0);
    }
}

/* LZHDecruncher tables (Tabbs: Dc.w 0, then CmpTab 32 words, AddTab 32,
   RealTab 527, AnzPerBits 32) - static, as in the original */
static UWORD tabbs[1 + 32 + 32 + 527 + 32];
#define OCMPTAB     0
#define OADDTAB     64
#define OREALTAB    128
#define OANZPERBITS 1182

/* ReadTab - d2 = max. bits; a0 = AnzPerBits, a4 = RealTab part */
static void readtab(struct crmbits *b, UWORD d2, UBYTE *a0, UBYTE *a4)
{
    UWORD d5, d4 = 0, d1;
    UWORD a3 = 0;                       /* only the low word is used */

    d5 = (UWORD)(crmgetbits(b, 4) - 1);
    do {                                /* RTlop */
        UWORD d0;
        d4++;
        d1 = d4;
        if ((WORD)d1 > (WORD)d2)
            d1 = d2;
        d0 = crmgetbits(b, d1);
        RW(a0) = d0;
        a0 += 2;
        a3 += d0;
    } while (d5-- != 0);
    d5 = (UWORD)(a3 - 1);
    do {                                /* RTlp2 */
        RW(a4) = crmgetbits(b, d2);
        a4 += 2;
    } while (d5-- != 0);
}

/* CalcCmpTab - a3 = AnzPerBits part, a4 = CmpTab part (-2) */
static void calccmptab(const UBYTE *a3, UBYTE *a4)
{
    UWORD d2 = 0, d3 = 0, d6;
    int i;

    RW(a4) = 0;
    a4 += 2;
    for (i = 0; i < 15; i++) {          /* CClop */
        d6 = RW(a3);
        a3 += 2;
        RW(a4 + 64) = d3;
        RW(a4 + 64) -= (UWORD)(RW(a4 - 2) + RW(a4 - 2));
        d3 += d6;
        d2 += d6;                       /* MULU d1(=1),d6 */
        RW(a4) = d2;
        a4 += 2;
        d2 <<= 1;
    }
}

/* ReadIt - one Huffman code: a4 = CmpTab part, a0 = RealTab part */
static UWORD readit(struct crmbits *b, const UBYTE *a4, const UBYTE *a0)
{
    UWORD d1 = 0, d0;

    do {
        d1 = (UWORD)((d1 << 1) | crmbit(b));    /* ROXL.w #1,d1 */
        d0 = RW(a4);
        a4 += 2;
    } while (d0 <= d1);
    d1 += RW(a4 + 62);
    d1 += d1;
    return RW(a0 + (WORD)d1);
}

/* LZHDecruncher ("CrM2") */
static void lzhdecruncher(UBYTE *a1, const UBYTE *a2, ULONG d1, ULONG d2)
{
    UBYTE *t = (UBYTE *)tabbs + 2;      /* a6 = Tabbs+2 */
    struct crmbits b;

    a1 += d1;
    crminit(&b, a2 + d2);
    do {                                /* BufLoop */
        const UBYTE *a0, *a5;
        UWORD d5;
        int i;

        for (i = 0; i < 32; i++)
            RW(t + OANZPERBITS + 2 * i) = 0;
        readtab(&b, 9, t + OANZPERBITS + 32, t + OREALTAB + 30);
        readtab(&b, 4, t + OANZPERBITS, t + OREALTAB);
        calccmptab(t + OANZPERBITS + 32, t + OCMPTAB - 2);
        calccmptab(t + OANZPERBITS, t + OCMPTAB + 30);
        d5 = crmgetbits(&b, 16);
        a0 = t + OREALTAB + 30;
        a5 = a0 - 30;
        do {                            /* decrloop */
            UWORD d0 = readit(&b, t, a0);
            UBYTE c = (UBYTE)d0;
            if (!(d0 & 0x100)) {
                UWORD d4 = d0, bits, d2w;
                const UBYTE *a3;
                d0 = readit(&b, t + OCMPTAB + 32, a5);
                bits = d0;
                d2w = d0;
                if (!d0) {
                    bits = 1;
                    d2w = 16;
                }
                d0 = crmgetbits(&b, bits);
                if ((d2w & 31) < 16)    /* BSET d2,d0: bits 16-31 */
                    d0 |= (UWORD)(1U << (d2w & 31));    /* are not used */
                a3 = a1 + 1 + (WORD)d0;
                do
                    *--a1 = *--a3;
                while (d4-- != 0);
                *--a1 = *--a3;
                c = *--a3;
            }
            *--a1 = c;                  /* skip */
        } while (d5-- != 0);
    } while (crmgetbits(&b, 1));
}

/* OverlapDecrunch - a0 = source with header, a1 = destination */
static void overlapdecrunch(const UBYTE *a0, UBYTE *a1)
{
    void (*dec)(UBYTE *, const UBYTE *, ULONG, ULONG) = fastdecruncher;
    ULONG id = RL(a0), d0, d1, d2;
    const UBYTE *a2;

    a0 += 4;
    if (id != ID_CRM) {
        dec = lzhdecruncher;
        if (id != ID_CRM2)
            return;                     /* NotCrunched */
    }
    d0 = RW(a0);                        /* MinSecDist */
    d1 = RL(a0 + 2);                    /* DestLen */
    d2 = RL(a0 + 6);                    /* SrcLen */
    a0 += 10;
    a2 = a0 + d0;
    /* BUG(orig): a1 - MinSecDist lies in front of the destination buffer
       (always for _crm, destination = source) */
    if ((LONG)a2 > (LONG)a1) {          /* copy below the destination */
        ULONG *s = (ULONG *)a0, *d, n = d2 >> 2;
        a0 = a1 - d0;
        d = (ULONG *)a0;
        do
            *d++ = *s++;
        while (--n != 0);
        *d = *s;                        /* in case of ... */
    }
    dec(a1, a0, d1, d2);
}

/* _crm - d0 = buffer (in place); _crm2 - d0 = source, d1 = destination;
   all registers kept */
static void BLITZ_ROUTINE crm_c(const UBYTE *src BL_REG(d0))
{
    overlapdecrunch(src, (UBYTE *)src);
}
BLITZ_THUNK(_crm, crm_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE crm2_c(const UBYTE *src BL_REG(d0),
                                 UBYTE *dst BL_REG(d1))
{
    overlapdecrunch(src, dst);
}
BLITZ_THUNK(_crm2, crm2_c, "d0-d1/a0-a1");

/* =========================================================== PowerPacker */

/* bit reader of the PowerPacker decruncher (d5 bits, a0 source) */
struct ppbits {
    ULONG d5;
    const UBYTE *a0;
};

/* ReadBit: LSR.l #1,d5, at 0 the next long with ROXR.l #1,d5 */
static int ppbit(struct ppbits *b)
{
    int x = b->d5 & 1;

    b->d5 >>= 1;
    if (b->d5 == 0) {
        ULONG n;
        b->a0 -= 4;
        n = RL(b->a0);
        b->d5 = ((ULONG)x << 31) | (n >> 1);
        x = n & 1;
    }
    return x;
}

/* ReadD1 - d0+1 bits (DBF on the low word) */
static UWORD ppread(struct ppbits *b, UWORD d0)
{
    ULONG d1 = 0;

    do
        d1 = (d1 << 1) | ppbit(b);
    while (d0-- != 0);
    return (UWORD)d1;
}

static void ppdecrunch(const UBYTE *a0, const UBYTE *a1, UBYTE *a2)
{
    const UBYTE *a5;
    UBYTE *a3;
    struct ppbits b;
    ULONG d1;
    int crunched;

    if (RL(a0) != ID_PP20)
        return;                         /* nopower */
    a5 = a0 + 4;                        /* efficiency */
    b.a0 = a1;
    a3 = a2;
    b.d5 = 1;
    b.a0 -= 4;
    d1 = RL(b.a0);                      /* length and empty bits */
    if ((UBYTE)d1) {
        ppbit(&b);                      /* always the next long */
        d1 = (d1 & ~0xffUL) | (UBYTE)((UBYTE)d1 - 1);
        b.d5 = (d1 & 63) >= 32 ? 0 : b.d5 >> (d1 & 63);
    }
    d1 >>= 8;
    a3 += d1;                           /* end of the file */
    crunched = ppbit(&b);               /* LoopCheckCrunch */
    for (;;) {
        UWORD d2, n, d3, d0;

        if (!crunched) {                /* NormalBytes */
            d2 = 0;
            do {
                n = ppread(&b, 1);
                d2 += n;
            } while (n == 3);
            do
                *--a3 = (UBYTE)ppread(&b, 7);
            while (d2-- != 0);
            if (!(a2 < a3))
                return;
        }
        /* CrunchedBytes */
        n = ppread(&b, 1);              /* code = length-2 */
        d0 = a5[n];                     /* offset bits */
        d2 = n;
        if (d2 == 3) {
            if (!ppbit(&b))
                d0 = 7;
            d3 = ppread(&b, (UWORD)(d0 - 1));   /* ReadD1_SUB */
            do {
                n = ppread(&b, 2);
                d2 += n;
            } while (n == 7);
        } else {
            d3 = ppread(&b, (UWORD)(d0 - 1));
        }
        d2++;
        do {                            /* MOVE.b 0(a3,d3.w),-(a3) */
            UBYTE v = a3[(WORD)d3];
            *--a3 = v;
        } while (d2-- != 0);
        if (!(a2 < a3))
            return;
        crunched = ppbit(&b);           /* LoopCheckCrunch */
    }
}

/* _ppdecrunch - d0 = packed file, d1 = its end, d2 = destination; all
   registers kept */
static void BLITZ_ROUTINE ppdecrunch_c(const UBYTE *start BL_REG(d0),
                                       const UBYTE *end BL_REG(d1),
                                       UBYTE *dest BL_REG(d2))
{
    ppdecrunch(start, end, dest);
}
BLITZ_THUNK(_ppdecrunch, ppdecrunch_c, "d0-d1/a0-a1");
