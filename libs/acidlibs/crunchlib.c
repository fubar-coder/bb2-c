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
 * overwritten, as the Crunch-Mania listing requires - BUG(orig) (#116), see
 * below).
 * PPDecrunch start,end,dest: end = end of the packed file (the last long
 * holds the original length and the number of empty bits).
 *
 * Kept from the original (BUG(orig)):
 *   - Implode: the match search reads up to 254 bytes past the end of
 *     the data (im30: up to 15, the length measurement im32/im33: up to
 *     254 - the two do not add up; results do not depend on them). (#112)
 *   - CrMDecrunch with one argument (destination = source) and with
 *     overlapping buffers: the crunched data are moved to MinSecDist
 *     bytes BEFORE the destination - up to MinSecDist bytes of foreign
 *     memory in front of the buffer are overwritten (the BB2 docs only
 *     say "works like the Deplode command"). (#116)
 *   - CrMDecrunch: overlap copy of (CrLen>>2) longs with SUBQ/BNE: a
 *     crunched length below 4 copies 4 GB; the address comparisons are
 *     signed (BLE). (#115)
 *   - CrM2: ReadTab with a count of 0 reads 65536 table entries (DBF);
 *     the tables are static data inside the library, as in the original. (#117)
 *   - PPDecrunch: ReadD1_SUB with an efficiency value 0 reads 65536 bits. (#118)
 *   - Implode crunches in place while it searches: a result of 0 (not
 *     crunchable) after at least 64 bytes leaves the buffer partly
 *     overwritten. (#114)
 *   - No checks of the data at all: corrupt data write anywhere. (#119)
 * Kept from the original, not a bug: after $4012 literal bytes in a row
 * Implode stores the rest of the data uncrunched - a limit of the
 * Imploder format (imtab4); the result decrunches correctly (#113).
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
    infofunc *info = (infofunc *)SL(6); /* d0 */
    ULONG res;                          /* d0: the inverted result */

    if (!info)
        return 0;
    res = info(SL(0x22) - SL(0x26));
    res = ~res;
    return res != 0;
}

/* im37 - put the low nbits (d0) bits of value (d1) into the bit stream
   (LSB first); returns value shifted (LSR.l) */
static ULONG im37(UBYTE *st, UBYTE nbits, ULONG value)
{
    UBYTE bitbuf = SB(0x2c), bitcnt = SB(0x2d), carry;
    UBYTE *dest = (UBYTE *)SL(0x26);

    do {
        carry = value & 1;              /* LSR.l #1,d1 */
        value >>= 1;
        bitbuf = (UBYTE)((bitbuf >> 1) | (carry << 7)); /* ROXR.b #1,d2 */
        if ((BYTE)--bitcnt < 0) {
            bitcnt = 7;
            *dest++ = bitbuf;
            bitbuf = 0;
        }
    } while (--nbits);
    SL(0x26) = (ULONG)dest;
    SB(0x2d) = bitcnt;
    SB(0x2c) = bitbuf;
    return value;
}

/* im28/im29..im36 - search the matches at the source position.  Returns 0
   if fewer than 3 bytes are left (then the rest is stored as literals),
   else -1 with the best match per length 2..8 and the longest in
   $34/$3C. */
static int im28(UBYTE *st)
{
    UBYTE *src = (UBYTE *)SL(0x22);
    ULONG end = SL(0x0e), searchend;
    UBYTE *cmp1, *cmp2, *scan, *ahead, *slots = st + 0x34;
    UWORD bestlen, len;
    UBYTE first;
    ULONG winend = (ULONG)src + 1 + SL(0x1a);

    if (winend > end) {
        winend = end;
        if (winend - (ULONG)src < 3)
            return 0;
    }
    /* im29 */
    searchend = winend;
    scan = src + 1;
    ahead = scan;
    bestlen = 1;
    first = *src;
    for (;;) {
        /* im30: 16 compares per loop test */
        int i;
        for (i = 0; i < 16; i++)
            if (*scan++ == first)
                goto im32;
        if (searchend > (ULONG)scan)
            continue;
        return -1;                      /* im31 */
im32:
        if (searchend <= (ULONG)scan)
            return -1;
        cmp1 = ahead;
        cmp2 = scan;
        if (*cmp1++ != *cmp2++)
            continue;
        if (*cmp1++ != *cmp2++)
            goto im35;
        if (*cmp1++ == *cmp2++) {
            UWORD n = 251;              /* DBNE: up to 252 more */
            for (;;) {
                if (*cmp1++ != *cmp2++)
                    break;
                if (n-- == 0)
                    break;
            }
        }
        /* im34 */
        if ((ULONG)cmp2 > end)
            cmp2 = (UBYTE *)end;
im35:
        len = (UWORD)((ULONG)cmp2 - (ULONG)scan);
        if (bestlen >= len)             /* CMP.w d6,d7: BCC */
            continue;
        bestlen = len;
        if (len > 8) {                  /* im36: long match */
            slots[7] = (UBYTE)len;
            RL(slots + 0x24) = (ULONG)scan - (ULONG)src - 2;
            if ((UBYTE)len == 0xff)
                return -1;
            continue;
        }
        if (slots[(WORD)len - 2])
            continue;
        slots[(WORD)len - 2] = (UBYTE)len;
        RL(slots + (WORD)(len << 2)) = (ULONG)scan - (ULONG)src - 2;
    }
}

/* im40 - code of a match: d0 = length, d1 = offset; 0 if it cannot be
   coded (offset or literal count too large) */
static int im40(UBYTE *st, ULONG len, ULONG offset)
{
    const UBYTE *t2 = (const UBYTE *)imtab2, *t3 = (const UBYTE *)imtab3;
    const UBYTE *tab;
    UWORD litcnt, litbase;
    UBYTE nbits, codebits, shift;
    ULONG prefix, offbase;
    UBYTE mlen = (UBYTE)len;            /* d0: the match length */
    UBYTE clen;                         /* d0: 5 for the long ones */
    ULONG idx;                          /* d0: the length - 2 table index */

    if (mlen > 13) {                    /* im42 */
        SB(0x70) = 0x1f;
        SB(0x71) = mlen;
        SB(0x69) = 13;
        clen = 5;
    } else if (mlen > 5) {              /* im41 */
        SB(0x71) = (UBYTE)((mlen - 6) | 0xf0);
        SB(0x69) = 8;
        clen = 5;
    } else {
        SB(0x71) = t2[(WORD)mlen - 2];
        SB(0x69) = t2[(WORD)mlen + 2];
        clen = mlen;
    }
    /* im44 */
    idx = (UBYTE)(clen - 2);
    litcnt = (UWORD)SL(0x30);
    tab = (const UBYTE *)imtab4 + idx + idx;
    if (litcnt < RW(tab)) {
        nbits = t3[(WORD)idx];
        codebits = nbits + 1;
        SB(0x73) = 0;
        litbase = 0;
    } else if (litcnt < RW(tab + 8)) {  /* im45 */
        nbits = t3[(WORD)idx + 4];
        codebits = nbits + 2;
        SB(0x73) = 2;
        litbase = RW(tab);
    } else if (litcnt >= RW(tab + 0x10)) { /* im46 */
        return 0;
    } else {                            /* im47 */
        nbits = t3[(WORD)idx + 8];
        codebits = nbits + 2;
        SB(0x73) = 3;
        litbase = RW(tab + 8);
    }
    /* im48 */
    SB(0x6a) = codebits;
    litcnt -= litbase;
    shift = 16 - nbits;
    litcnt = (shift & 63) >= 16 ? 0 : (UWORD)(litcnt << (shift & 63));
    do {                                /* im49 */
        UWORD carry = litcnt >> 15;     /* ADD.w d2,d2 */
        litcnt <<= 1;
        SW(0x72) = (UWORD)((SW(0x72) << 1) | carry);    /* ROXL $72(a6) */
    } while (--nbits);

    tab = st + 0x74 + 4 * (WORD)(UWORD)idx;
    if (offset < RL(tab)) {
        nbits = SB(0xa4 + (WORD)idx);
        codebits = nbits + 1;
        prefix = 0;
        offbase = 0;
    } else if (offset < RL(tab + 0x10)) { /* im50 */
        nbits = SB(0xa4 + 4 + (WORD)idx);
        codebits = nbits + 2;
        prefix = 2;
        offbase = RL(tab);
    } else if (offset >= RL(tab + 0x20)) { /* im51 */
        return 0;
    } else {                            /* im52 */
        nbits = SB(0xa4 + 8 + (WORD)idx);
        codebits = nbits + 2;
        prefix = 3;
        offbase = RL(tab + 0x10);
    }
    /* im53 */
    SB(0x68) = codebits;
    offset -= offbase;
    shift = 32 - nbits;
    offset = (shift & 63) >= 32 ? 0 : offset << (shift & 63);
    do {                                /* im54: ADD.l d1,d1:ADDX.l d7,d7 */
        ULONG carry = offset >> 31;
        offset <<= 1;
        prefix = prefix + prefix + carry;
    } while (--nbits);
    SL(0x6c) = prefix;
    return -1;
}

/* im55 - choose the match with the best gain (bits saved); the code goes
   to $5C.., its length to $2E.  Clears the length slots.  TRUE if one was
   found (TST.w $2E(a6)). */
static int im55(UBYTE *st)
{
    UBYTE *lens = st + 0x34, *offs = st + 0x3c;

    SW(0x2a) = 0;
    SB(0x2e) = 0;
    do {
        ULONG offset = RL(offs);
        UBYTE len = *lens++;
        offs += 4;
        if (len && im40(st, len, offset)) {
            UWORD gain = (UWORD)(lens[-1] << 3);
            UBYTE bits = 0;
            int i;

            bits += SB(0x69);
            bits += SB(0x68);
            bits += SB(0x6a);
            gain -= bits;
            if ((WORD)gain < 0 || gain < SW(0x2a))
                continue;
            SW(0x2a) = gain;
            SB(0x2e) = lens[-1];
            for (i = 0; i < 13; i++)
                SB(0x5c + i) = SB(0x68 + i);
        }
    } while (lens - st != 0x3c);
    RL(lens - 4) = 0;                   /* CLR.l -(a4) twice */
    RL(lens - 8) = 0;
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
                     UBYTE *tabout)
{
    ULONG work[WS / 4];
    UBYTE *st = (UBYTE *)work;
    const ULONG *tab;
    ULONG tabno;                        /* the table number */
    ULONG window;
    ULONG idx;                          /* the table index */
    ULONG bits;                         /* the bit values (upper word carried) */
    UBYTE nbits;                        /* the length code bits */
    ULONG clen;                         /* the crunched length */
    ULONG gain;
    int i;

    for (i = 0; i < WS / 4; i++)
        work[i] = 0;
    SL(2) = (ULONG)blitz_globals;
    if (len < 0x40)
        goto im23;
    SB(0) = (mode & 0x80) ? 0xff : 0;   /* LSR.l #8,d1:SCS (a6) */
    tabno = mode >> 16;
    if ((UBYTE)tabno >= 12)
        tabno = 0;
    SL(6) = (ULONG)info;
    SL(0x0a) = (ULONG)buf;
    SL(0x22) = (ULONG)buf;
    SL(0x26) = (ULONG)buf;
    SL(0x12) = len;
    SL(0x0e) = (ULONG)buf + len;
    window = imtab0[(UWORD)(tabno << 2) >> 2] + 1;
    if (window > len)
        window = len - 1;
    SL(0x1a) = window;
    window--;
    idx = 0;
    tab = imtab0;
    while (window > *tab++)             /* im04 */
        idx = (idx & ~0xffUL) | (UBYTE)(idx + 1);
    SB(1) = (UBYTE)idx;
    {
        const UBYTE *s = (const UBYTE *)imtab1 + (UWORD)idx * 12;
        for (i = 0; i < 12; i++)
            SB(0xa4 + i) = s[i];
    }
    for (i = 0; i < 12; i++)            /* im07 */
        SL(0x74 + 4 * i) = 1UL << (SB(0xa4 + i) & 31);
    for (i = 0; i < 8; i++)             /* im08 */
        SL(0x84 + 4 * i) += SL(0x74 + 4 * i);
    if (SB(0)) {                        /* never with mode 8 */
        for (i = 0; i < 8; i++) {
            RW(tabout) = (UWORD)SL(0x74 + 4 * i);
            tabout += 2;
        }
        for (i = 0; i < 12; i++)
            *tabout++ = SB(0xa4 + i);
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
        bits = im37(st, SB(0x5c), SL(0x60));
        bits = (bits & 0xffff0000UL) | SW(0x66);
        bits = im37(st, SB(0x5e), bits);
        nbits = SB(0x5d);
        bits = (bits & 0xffff0000UL) | SW(0x64);
        if (nbits == 13) {
            UBYTE *dest = (UBYTE *)SL(0x26);
            *dest++ = (UBYTE)bits;
            SL(0x26) = (ULONG)dest;
            nbits = 5;
            bits = 0x1f;
        }
        im37(st, nbits, bits);          /* im14 */
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
        UBYTE *head, *out;
        ULONG oddflag;
        UWORD last;

        clen = SL(0x26) - SL(0x0a);
        if (clen < 12)
            goto im23;
        gain = SL(0x12) - clen;
        if (gain <= 0x36)
            goto im23;
        head = (UBYTE *)SL(0x0a);
        out = (UBYTE *)SL(0x26);
        oddflag = 0xff00;
        if (clen & 1) {
            oddflag = 0;
            clen++;
            *out++ = 0;
        }
        /* im16: the first 12 bytes go to the end, header in front */
        RL(out + 8) = RL(head);
        RL(head) = ID_IMP;
        RL(out + 4) = RL(head + 4);
        RL(head + 4) = SL(0x12);
        RL(out) = RL(head + 8);
        RL(head + 8) = clen;
        clen += 0x2e;
        SL(0x16) = clen;
        RL(out + 12) = SL(0x30);
        last = SB(0x2c) & 0xfe;         /* MOVE.b:AND.w #$FE */
        last |= (UWORD)(1U << (SB(0x2d) & 31)); /* BSET d0,d1 */
        last |= (UWORD)oddflag;
        RW(out + 0x10) = last;
        out += 0x12;
        for (i = 0; i < 8; i++) {       /* im17 */
            RW(out) = (UWORD)SL(0x74 + 4 * i);
            out += 2;
        }
        for (i = 0; i < 12; i++)        /* im18 */
            *out++ = SB(0xa4 + i);
    } else {                            /* im19, never with mode 8 */
        UBYTE lastbyte, *out;

        clen = SL(0x26) - SL(0x0a);
        gain = SL(0x12) - clen;
        if (gain <= 6)
            goto im23;
        lastbyte = (UBYTE)((SB(0x2c) & 0xfe) | (1U << (SB(0x2d) & 7)));
        out = (UBYTE *)SL(0x26);
        if (clen & 1) {
            *out++ = lastbyte;
            RL(out) = SL(0x30);
        } else {
            RL(out) = SL(0x30);
            out += 4;
            *out = lastbyte;
        }
        clen += 5;
        SL(0x16) = clen;
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
static int imbit(UBYTE *bitbuf, const UBYTE **src)
{
    UBYTE v = *bitbuf;
    int c = v >> 7;

    v <<= 1;
    if (v == 0) {
        UBYTE n = *--*src;
        v = (UBYTE)((n << 1) | c);
        c = n >> 7;
    }
    *bitbuf = v;
    return c;
}

/* deplode - a0 = crunched data; -1 = ok, 0 = error */
static ULONG deplode(UBYTE *buf)
{
    UBYTE *dst;
    const UBYTE *packed, *hdr;
    UBYTE tab[28];
    ULONG litcnt;
    UBYTE bitbuf;
    int i;

    if (RL(buf) != ID_IMP)
        return 0;
    dst = buf + RL(buf + 4);
    packed = buf + RL(buf + 8);
    hdr = packed;
    RL(buf + 8) = RL(hdr);
    RL(buf + 4) = RL(hdr + 4);
    RL(buf) = RL(hdr + 8);
    litcnt = RL(hdr + 12);
    bitbuf = (UBYTE)RW(hdr + 16);       /* only the low byte is used */
    if (!(RW(hdr + 16) & 0x8000))
        packed--;
    hdr += 18;
    for (i = 0; i < 28; i++)            /* 7 longs */
        tab[i] = hdr[i];

    for (;;) {
        UWORD len, idx, lenidx, cnt;
        ULONG litextra;                 /* the literal count base */
        ULONG offbits;                  /* the offset bits value */
        ULONG src;
        UBYTE n;

        /* I1E70: literal run */
        for (; litcnt; litcnt--)
            *--dst = *--packed;
        /* I1E7A */
        if (buf >= dst)
            return packed == buf ? (ULONG)-1 : 0;
        /* I1E92: match length */
        if (!imbit(&bitbuf, &packed)) {
            len = 1; idx = 0;
        } else if (!imbit(&bitbuf, &packed)) {
            len = 2; idx = 1;
        } else if (!imbit(&bitbuf, &packed)) {
            len = 3; idx = 2;
        } else if (!imbit(&bitbuf, &packed)) {
            len = 4; idx = 3;
        } else {
            UBYTE b = 0;
            if (imbit(&bitbuf, &packed)) {
                b = *--packed;
                b--;
            } else {
                b = (UBYTE)(b + b + imbit(&bitbuf, &packed));
                b = (UBYTE)(b + b + imbit(&bitbuf, &packed));
                b = (UBYTE)(b + b + imbit(&bitbuf, &packed));
                b += 5;
            }
            len = b;
            idx = 3;
        }
        /* I1F08: next literal count */
        litextra = 0;
        lenidx = idx;
        if (imbit(&bitbuf, &packed)) {
            if (imbit(&bitbuf, &packed)) {
                litextra = I1F8C[idx];
                idx = (UBYTE)(idx + 8);
            } else {
                litextra = 2;
                idx = (UBYTE)(idx + 4);
            }
        }
        n = I1F90[idx];
        {
            UWORD w = (UWORD)litcnt;
            do
                w = (UWORD)(w + w + imbit(&bitbuf, &packed));
            while (--n);
            w += (UWORD)litextra;
            litcnt = (litcnt & 0xffff0000UL) | w;
        }
        /* offset */
        offbits = 0;
        src = 0;
        idx = lenidx;
        if (imbit(&bitbuf, &packed)) {
            lenidx += lenidx;
            if (imbit(&bitbuf, &packed)) {
                src = (ULONG)(LONG)(WORD)RW(tab + 8 + lenidx);
                idx = (UBYTE)(idx + 8);
            } else {
                src = (ULONG)(LONG)(WORD)RW(tab + lenidx);
                idx = (UBYTE)(idx + 4);
            }
        }
        n = tab[0x10 + idx];
        do
            offbits = offbits + offbits + imbit(&bitbuf, &packed);
        while (--n);
        src += 1 + offbits + (ULONG)dst;
        {
            const UBYTE *s = (const UBYTE *)src;
            cnt = len;
            do
                *--dst = *--s;
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
    ULONG bits;                 /* d6 */
    UWORD count;                /* d7 */
    const UBYTE *src;           /* a2 */
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
    int c = b->bits & 1;

    if (--b->count != 0) {
        b->bits >>= 1;
    } else {                            /* next word into the high word */
        b->count = 16;
        b->src -= 2;
        b->bits = ((ULONG)RW(b->src) << 16) | ((b->bits >> 1) & 0xffff);
    }
    return c;
}

/* GetBits/GetBits2 - d1 bits (1..16) */
static UWORD crmgetbits(struct crmbits *b, UWORD n)
{
    UWORD value = (UWORD)b->bits;

    b->bits = (n & 63) >= 32 ? 0 : b->bits >> (n & 63);
    b->count -= n;
    if ((WORD)b->count <= 0) {
        b->count += 16;                    /* ADD.w d3,d7 */
        b->bits = ror32(b->bits, b->count & 63);
        b->src -= 2;
        b->bits = (b->bits & 0xffff0000UL) | RW(b->src);
        b->bits = rol32(b->bits, b->count & 63);
    }
    return value & (UWORD)((1UL << n) - 1);     /* AndData */
}

/* start of the bit stream at the end of the data (a2 = end) */
static void crminit(struct crmbits *b, const UBYTE *end)
{
    UWORD lastbits;
    UWORD shift;

    b->src = end - 2;
    lastbits = RW(b->src);               /* bits in the last word */
    b->src -= 4;
    b->bits = RL(b->src);
    shift = (UWORD)(16 - lastbits);
    b->bits = (shift & 63) >= 32 ? 0 : b->bits >> (shift & 63);
    b->count = lastbits;
}

/* FastDecruncher ("CrM!") - a1 = dest, a2 = packed data, d1/d2 = their
   lengths */
static void fastdecruncher(UBYTE *dst, const UBYTE *src, ULONG dstlen, ULONG srclen)
{
    UBYTE *start = dst;
    struct crmbits b;
    UWORD len = 0;

    dst += dstlen;
    crminit(&b, src + srclen);
    for (;;) {
        UWORD bits, distbase;
        const UBYTE *from;

        if ((LONG)dst <= (LONG)start)
            return;                     /* DecrEnd */
        if (crmbit(&b)) {
            len = 0;
insertbytes:
            do
                *--dst = (UBYTE)crmgetbits(&b, 8);
            while (len-- != 0);
            continue;
        }
        /* InsertSeq: length */
        if (!crmbit(&b)) {
            bits = 1; len = 1;
        } else if (!crmbit(&b)) {
            bits = 2; len = 3;
        } else if (!crmbit(&b)) {
            bits = 4; len = 7;
        } else {
            bits = 8; len = 0x17;
        }
        len += crmgetbits(&b, bits);
        if (len == 22) {                /* SpecialInsert */
            len = 14;
            bits = crmbit(&b) ? 5 : 14;
            len += crmgetbits(&b, bits);
            goto insertbytes;
        }
        if ((WORD)len > 22)
            len--;
        /* distance */
        if (!crmbit(&b)) {
            bits = 9; distbase = 0x20;
        } else if (!crmbit(&b)) {
            bits = 5; distbase = 0;
        } else {
            bits = 14; distbase = 0x220;
        }
        from = dst + (WORD)(UWORD)(crmgetbits(&b, bits) + distbase);
        do
            *--dst = *--from;
        while (len-- != 0);
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
static void readtab(struct crmbits *b, UWORD maxbits, UBYTE *counts, UBYTE *codes)
{
    UWORD n, bitn = 0, nbits;
    UWORD total = 0;                    /* only the low word is used */

    n = (UWORD)(crmgetbits(b, 4) - 1);
    do {                                /* RTlop */
        UWORD count;
        bitn++;
        nbits = bitn;
        if ((WORD)nbits > (WORD)maxbits)
            nbits = maxbits;
        count = crmgetbits(b, nbits);
        RW(counts) = count;
        counts += 2;
        total += count;
    } while (n-- != 0);
    n = (UWORD)(total - 1);
    do {                                /* RTlp2 */
        RW(codes) = crmgetbits(b, maxbits);
        codes += 2;
    } while (n-- != 0);
}

/* CalcCmpTab - a3 = AnzPerBits part, a4 = CmpTab part (-2) */
static void calccmptab(const UBYTE *counts, UBYTE *cmp)
{
    UWORD code = 0, sum = 0, count;
    int i;

    RW(cmp) = 0;
    cmp += 2;
    for (i = 0; i < 15; i++) {          /* CClop */
        count = RW(counts);
        counts += 2;
        RW(cmp + 64) = sum;
        RW(cmp + 64) -= (UWORD)(RW(cmp - 2) + RW(cmp - 2));
        sum += count;
        code += count;                  /* MULU d1(=1),d6 */
        RW(cmp) = code;
        cmp += 2;
        code <<= 1;
    }
}

/* ReadIt - one Huffman code: a4 = CmpTab part, a0 = RealTab part */
static UWORD readit(struct crmbits *b, const UBYTE *cmp, const UBYTE *real)
{
    UWORD code = 0, limit;

    do {
        code = (UWORD)((code << 1) | crmbit(b)); /* ROXL.w #1,d1 */
        limit = RW(cmp);
        cmp += 2;
    } while (limit <= code);
    code += RW(cmp + 62);
    code += code;
    return RW(real + (WORD)code);
}

/* LZHDecruncher ("CrM2") */
static void lzhdecruncher(UBYTE *dst, const UBYTE *src, ULONG dstlen, ULONG srclen)
{
    UBYTE *tabs = (UBYTE *)tabbs + 2;   /* a6 = Tabbs+2 */
    struct crmbits b;

    dst += dstlen;
    crminit(&b, src + srclen);
    do {                                /* BufLoop */
        const UBYTE *real, *realdist;
        UWORD n;
        int i;

        for (i = 0; i < 32; i++)
            RW(tabs + OANZPERBITS + 2 * i) = 0;
        readtab(&b, 9, tabs + OANZPERBITS + 32, tabs + OREALTAB + 30);
        readtab(&b, 4, tabs + OANZPERBITS, tabs + OREALTAB);
        calccmptab(tabs + OANZPERBITS + 32, tabs + OCMPTAB - 2);
        calccmptab(tabs + OANZPERBITS, tabs + OCMPTAB + 30);
        n = crmgetbits(&b, 16);
        real = tabs + OREALTAB + 30;
        realdist = real - 30;
        do {                            /* decrloop */
            UWORD sym = readit(&b, tabs, real);
            UBYTE c = (UBYTE)sym;
            if (!(sym & 0x100)) {
                UWORD len = sym, bits, topbit;
                UWORD dbits;            /* the distance bit count */
                UWORD dist;
                const UBYTE *from;
                dbits = readit(&b, tabs + OCMPTAB + 32, realdist);
                bits = dbits;
                topbit = dbits;
                if (!dbits) {
                    bits = 1;
                    topbit = 16;
                }
                dist = crmgetbits(&b, bits);
                if ((topbit & 31) < 16) /* BSET d2,d0: bits 16-31 */
                    dist |= (UWORD)(1U << (topbit & 31)); /* are not used */
                from = dst + 1 + (WORD)dist;
                do
                    *--dst = *--from;
                while (len-- != 0);
                *--dst = *--from;
                c = *--from;
            }
            *--dst = c;                 /* skip */
        } while (n-- != 0);
    } while (crmgetbits(&b, 1));
}

/* OverlapDecrunch - a0 = source with header, a1 = destination */
static void overlapdecrunch(const UBYTE *src, UBYTE *dst)
{
    void (*dec)(UBYTE *, const UBYTE *, ULONG, ULONG) = fastdecruncher;
    ULONG id = RL(src), mindist, dstlen, srclen;
    const UBYTE *limit;

    src += 4;
    if (id != ID_CRM) {
        dec = lzhdecruncher;
        if (id != ID_CRM2)
            return;                     /* NotCrunched */
    }
    mindist = RW(src);                  /* MinSecDist */
    dstlen = RL(src + 2);               /* DestLen */
    srclen = RL(src + 6);               /* SrcLen */
    src += 10;
    limit = src + mindist;
    /* BUG(orig) (#116): a1 - MinSecDist lies in front of the destination buffer
       (always for _crm, destination = source) */
    if ((LONG)limit > (LONG)dst) {      /* copy below the destination */
        ULONG *from = (ULONG *)src, *to, n = srclen >> 2;
        src = dst - mindist;
        to = (ULONG *)src;
        do
            *to++ = *from++;
        while (--n != 0);
        *to = *from;                    /* in case of ... */
    }
    dec(dst, src, dstlen, srclen);
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
    ULONG bits;                 /* d5 */
    const UBYTE *src;           /* a0 */
};

/* ReadBit: LSR.l #1,d5, at 0 the next long with ROXR.l #1,d5 */
static int ppbit(struct ppbits *b)
{
    int bit = b->bits & 1;

    b->bits >>= 1;
    if (b->bits == 0) {
        ULONG n;
        b->src -= 4;
        n = RL(b->src);
        b->bits = ((ULONG)bit << 31) | (n >> 1);
        bit = n & 1;
    }
    return bit;
}

/* ReadD1 - d0+1 bits (DBF on the low word) */
static UWORD ppread(struct ppbits *b, UWORD n)
{
    ULONG value = 0;

    do
        value = (value << 1) | ppbit(b);
    while (n-- != 0);
    return (UWORD)value;
}

static void ppdecrunch(const UBYTE *file, const UBYTE *end, UBYTE *dest)
{
    const UBYTE *eff;
    UBYTE *dst;
    struct ppbits b;
    ULONG lenword;
    int crunched;

    if (RL(file) != ID_PP20)
        return;                         /* nopower */
    eff = file + 4;                     /* efficiency */
    b.src = end;
    dst = dest;
    b.bits = 1;
    b.src -= 4;
    lenword = RL(b.src);                 /* length and empty bits */
    if ((UBYTE)lenword) {
        ppbit(&b);                      /* always the next long */
        lenword = (lenword & ~0xffUL) | (UBYTE)((UBYTE)lenword - 1);
        b.bits = (lenword & 63) >= 32 ? 0 : b.bits >> (lenword & 63);
    }
    lenword >>= 8;
    dst += lenword;                     /* end of the file */
    crunched = ppbit(&b);               /* LoopCheckCrunch */
    for (;;) {
        UWORD count, n, offset, offbits;

        if (!crunched) {                /* NormalBytes */
            count = 0;
            do {
                n = ppread(&b, 1);
                count += n;
            } while (n == 3);
            do
                *--dst = (UBYTE)ppread(&b, 7);
            while (count-- != 0);
            if (!(dest < dst))
                return;
        }
        /* CrunchedBytes */
        n = ppread(&b, 1);              /* code = length-2 */
        offbits = eff[n];               /* offset bits */
        count = n;
        if (count == 3) {
            if (!ppbit(&b))
                offbits = 7;
            offset = ppread(&b, (UWORD)(offbits - 1)); /* ReadD1_SUB */
            do {
                n = ppread(&b, 2);
                count += n;
            } while (n == 7);
        } else {
            offset = ppread(&b, (UWORD)(offbits - 1));
        }
        count++;
        do {                            /* MOVE.b 0(a3,d3.w),-(a3) */
            UBYTE v = dst[(WORD)offset];
            *--dst = v;
        } while (count-- != 0);
        if (!(dest < dst))
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
