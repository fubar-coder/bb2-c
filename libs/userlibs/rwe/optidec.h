/*
 * optidec.h - the decruncher of StoneCracker 4.04 ("S404"), optidec.bin
 * (third party code that the RWE sources stclib.bb and stcdecrunchlib.bb
 * include with IncBin; its source is optidec.S v1.1 of StoneCracker 4.10,
 * Aminet util/pack/Stc4103.lha), in C for ristonecrackerlib.c and
 * ristcdecrunchlib.c (included by both; everything static).
 *
 * The entry point is dostc() (see there).
 *
 * Translated register by register: d6 is the 32 bit bit buffer, the
 * counts are words, shifts are taken modulo 64 as by the 68000, DBF loops
 * count the low word down to -1 - the unpacked data are the original's
 * for every input it handles (see DIFF).
 *
 * Fixed (#833): corrupt data no longer write below the destination; the
 *   original wrote a match or literal run that starts just above it past
 *   its start, into the memory below.  The checks cost nothing per token
 *   on the main path: the loop's one test (CMPA.l a0,a5) compares with
 *   dest + STC_SHORTMAX - 1 instead of dest, so every token but a long
 *   match ends at or above dest; the last bytes are unpacked by a second
 *   copy of the loop that checks each literal run and match against the
 *   room left; a long match (23 bytes or more) is checked in both.  Once
 *   per call: far offset bits (the efficiency) above 16 and an odd packed
 *   length are refused.  Corrupt data now give 0 and write only inside
 *   dest .. dest + unpacked length.
 *   Not checked (a known limit): reads.  A stream that runs out reads
 *   words below the packed data (the header, then the memory below the
 *   file), and a match may read up to 544 + 65535 bytes above the output
 *   written so far, also above dest + unpacked length; such data unpack
 *   wrong, but nothing is written outside the destination.
 * DIFF: the literal run reads its shift count from a table indexed by the
 *   bit count d7 (MOVE.b table(pc,d7.w)); d7 leaves 0..15 only with a
 *   first bit count (the trailer's third word) above 16, and then the
 *   original reads whatever lies around the table (its own code) - the C
 *   build takes the index modulo 16.  With d7 = 0..15 the result is the
 *   same.
 */
#ifndef OPTIDEC_H
#define OPTIDEC_H

#define LONGAT(p) (*(const ULONG *)(p))
#define WORDAT(p) (*(const UWORD *)(p))

/**
 * @brief The 68000 registers the decruncher works with.
 *
 * d1 is not here: it is dostc()'s nbits, value and shift.
 */
struct stc {
    ULONG bitbuf;                       /* d6: bit buffer: bits at the top of the low word */
    UWORD bitsleft;                     /* d7: bits left in it */
    UWORD count;                       /* d3: bytes to copy */
    const UBYTE *in;                    /* a1: packed words, read backwards */
    UBYTE *out;                         /* a0: output, written backwards */
};

/* LSL.l by a register: count modulo 64, 32..63 give 0 */
static ULONG lsl(ULONG v, UWORD n)
{
    n &= 63;
    return n >= 32 ? 0 : v << n;
}

/* MOVE.w -(a1),d6 */
static void loadword(struct stc *s)
{
    s->in -= 2;
    s->bitbuf = (s->bitbuf & 0xffff0000UL) | WORDAT(s->in);
}

/* DBF d7,ok:MOVE.w -(a1),d6:MOVEQ #15,d7:ok: */
static void next(struct stc *s)
{
    if (s->bitsleft-- == 0) {
        loadword(s);
        s->bitsleft = 15;
    }
}

/* ADD.w d6,d6 - the carry */
static int bit(struct stc *s)
{
    UWORD w = (UWORD)s->bitbuf;
    s->bitbuf = (s->bitbuf & 0xffff0000UL) | (UWORD)(w << 1);
    return w >> 15;
}

/* ROL.w #8,d6:MOVE.b d6,-(a0) */
static void rolbyte(struct stc *s)
{
    UWORD w = (UWORD)s->bitbuf;
    w = (UWORD)(w << 8 | w >> 8);
    s->bitbuf = (s->bitbuf & 0xffff0000UL) | w;
    *--s->out = (UBYTE)w;
}

/* LSL.l d1,d6 ... SWAP d6:MOVE.b d6,-(a0):SWAP d6 - a byte from the upper word */
static void upperbyte(struct stc *s)
{
    *--s->out = (UBYTE)(s->bitbuf >> 16);
}

/* the getbits routine ($404): d1 bits -> d1 */
static UWORD getbits(struct stc *s, UWORD n)
{
    s->bitbuf &= 0xffff;                /* AND.l d4,d6 */
    s->bitsleft = (UWORD)(s->bitsleft - n);
    if ((WORD)s->bitsleft < 0) {        /* BPL not taken */
        n = (UWORD)(n + s->bitsleft);   /* ADD.w d7,d1 */
        s->bitbuf = lsl(s->bitbuf, n);
        n = s->bitsleft;
        loadword(s);
        n = (UWORD)-n;
        s->bitsleft = (UWORD)(s->bitsleft + 16);  /* ADD.w d5,d7 */
    }
    s->bitbuf = lsl(s->bitbuf, n);
    return (UWORD)(s->bitbuf >> 16);    /* MOVE.l d6,d1:SWAP d1 */
}

/* the table behind the decruncher ($44C): the shift count of a literal
   run's straddling byte, by the bits left (8 - n, $80 + 16 - n) */
static const UBYTE runtab[16] = {
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
    0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81,
};


/* the longest output of one token other than a long match: a literal run
   (14..45 bytes; a literal byte 1, the shorter matches 2..3, 4..7, 8..22) */
#define STC_SHORTMAX 45

/**
 * @brief The decruncher's loop, from its test ($350) on.
 *
 * Inlined twice by dostc() with `tail` a constant: the main loop (tail 0,
 * limit = start + STC_SHORTMAX - 1) leaves as soon as the output has come
 * within STC_SHORTMAX bytes of start, so no token of it but a long match
 * can reach below start; the tail (tail 1, limit = start) unpacks the
 * rest and checks each literal run and match against the room left.  A
 * long match is checked in both.
 * @param s        the registers
 * @param start    a5: the destination
 * @param limit    the loop ends as soon as a0 <= limit
 * @param farbits  d2: the far offset bits (0..16)
 * @param tail     1: check every literal run and match
 * @return 1 when the output has reached limit, 0 for a token that would
 *         write below start (fixed #833)
 */
static inline __attribute__((always_inline)) int stcloop(struct stc *s,
        const UBYTE *start, const UBYTE *limit, UWORD farbits, int tail)
{
    UBYTE *match;
    UWORD n;
    UWORD nbits;                        /* d1: getbits' count */
    UWORD value;                        /* d1: getbits' result */
    UWORD shift;                        /* d1: a literal run's shift count */

    goto check;

literal:                                /* $32C: 8 bits */
    s->bitsleft = (UWORD)(s->bitsleft - 8);
    if ((WORD)s->bitsleft >= 0) {
        rolbyte(s);
        goto check;
    }
    n = s->bitsleft;                    /* across the next word */
    s->bitsleft = (UWORD)(s->bitsleft + 8);
    s->bitbuf = lsl(s->bitbuf, s->bitsleft);
    loadword(s);
    n = (UWORD)-n;
    s->bitbuf = lsl(s->bitbuf, n);
    s->bitsleft = (UWORD)(s->bitsleft + 8);
    upperbyte(s);
check:                                  /* $350: CMPA.l a0,a5:DBCC d7,$35C */
    if ((ULONG)limit >= (ULONG)s->out)  /* fixed (#833): limit, not a5 */
        return 1;
    next(s);
    /* $35C */
    if (!bit(s))
        goto literal;
    next(s);
    if (bit(s)) {                       /* $3CC: 11x - 2 or 3 bytes */
        next(s);
        s->count = (UWORD)(s->count + 1 + bit(s));  /* ADDX.w d0,d3 (d0 = 1) */
        goto offset;
    }
    next(s);
    s->count = 2;
    if (bit(s)) {                       /* 101xx: 4..7 bytes */
        /* $3A8 with 2 bits: NOT.b d1 is never 0, no continuation */
        value = getbits(s, 2);
        s->count = (UWORD)(s->count + value);
        goto offset;
    }
    next(s);
    if (!bit(s)) {                      /* 1000: 23 + n bytes, n in bytes */
        s->count = 21;
        goto length8;
    }
    s->count = 6;                       /* 1001xxxx: 8..22 bytes */
    value = getbits(s, 4);
    s->count = (UWORD)(s->count + value);
    if (value < 15)                     /* CMP.w #15,d1:BCS */
        goto offset;
    s->count = 14;                      /* 10011111xxxxx: 14..45 literals */
    value = getbits(s, 5);
    s->count = (UWORD)(s->count + value);
    if (tail && s->count > (ULONG)(s->out - start))
        return 0;                       /* fixed (#833): the run's bytes */
    /* MOVE.b: the upper byte of d1 stays, LSL.l uses only bits 0-5 */
    shift = (UWORD)((value & 0xff00) | runtab[s->bitsleft & 15]);  /* DIFF: & 15 */
    if (shift & 0x80) {                 /* a whole byte left in the word */
        s->bitsleft = (UWORD)(s->bitsleft - 8);
        /* DBF d3 with d3 = 14..45 always branches (else: into the table) */
        s->count--;
        goto run2;
    }
    goto run1;

run2:                                   /* $424 */
    rolbyte(s);
run1:                                   /* $428 */
    s->bitbuf = lsl(s->bitbuf, s->bitsleft);
    loadword(s);
    s->bitbuf = lsl(s->bitbuf, shift);
    upperbyte(s);
    if ((LONG)(WORD)s->count - 2 > 0) { /* SUBQ.w #2,d3:BGT */
        s->count = (UWORD)(s->count - 2);
        goto run2;
    }
    s->count = (UWORD)(s->count - 2);
    if (s->count == 0) {                /* BEQ: one byte more */
        s->count = 0xffff;
        rolbyte(s);
        goto check;
    }
    s->bitsleft = (UWORD)(s->bitsleft + 8);
    goto check;                         /* $3C2: the same as $350 */

length8:                                /* $3A6, $3A8 with 8 bits */
    do {
        value = getbits(s, 8);
        s->count = (UWORD)(s->count + value);
    } while ((UBYTE)~value == 0);       /* NOT.b d1: 255 - one more byte */
    /* fixed (#833): a long match writes count + 2 bytes */
    if ((ULONG)s->count + 2 > (ULONG)(s->out - start))
        return 0;
offset:                                 /* $3D8 / $3E0 */
    if (tail && (ULONG)s->count + 2 > (ULONG)(s->out - start))
        return 0;                       /* fixed (#833): the match's bytes */
    next(s);
    if (bit(s)) {                       /* 1: far, d2 bits + 544 */
        match = s->out + 544;
        nbits = farbits;
    } else {
        next(s);
        nbits = 9;                      /* 00: 9 bits + 32 */
        match = s->out + 32;
        if (bit(s)) {                   /* 01: 5 bits */
            nbits = 5;
            match = s->out;
        }
    }
    value = getbits(s, nbits);
    match += (WORD)value;               /* $3B8: ADDA.w d1,a2 */
    *--s->out = *match;
    do
        *--s->out = *--match;
    while (s->count-- != 0);            /* DBF d3 */
    goto check;                         /* $3C2 */
}

/**
 * @brief Unpack a StoneCracker 4.04 ("S404") file.
 *
 * The data unpack backwards from dest + the unpacked length (the long at
 * file + 8) down to dest.  Fixed (#833): corrupt data are refused and
 * never write below dest; far offset bits above 16 and an odd packed
 * length are refused at once.  Reads are not checked (see the head of
 * the file).  The original leaves d0 = 1 (MOVEQ #1,d0, the 1 of its
 * ADDX); its other registers are working registers.
 *
 * @param file  a1: the S404 file
 * @param dest  a0: the destination buffer
 * @param end   gets a0 at the end (dest for good data: the loop ends as
 *              soon as a0 <= dest); not set for refused data
 * @return 1 for data unpacked, 0 for refused (corrupt) data
 */
static int dostc(const UBYTE *file, UBYTE *dest, UBYTE **end)
{
    struct stc st, *s = &st;
    const UBYTE *packed = file + 8;     /* ADDQ.w #8,a1 */
    UWORD farbits;

    s->out = dest + LONGAT(packed);     /* ADDA.l (a1)+,a0 */
    packed += 4;
    if (LONGAT(packed) & 1)             /* fixed (#833): the trailer is words */
        return 0;
    packed += LONGAT(packed);           /* ADDA.l (a1),a1 */
    s->in = packed;
    farbits = WORDAT(packed);           /* MOVEM.w (a1),d2/d6/d7 */
    if (farbits > 16)                   /* fixed (#833): efficiency 0..16 */
        return 0;
    s->bitbuf = (ULONG)(LONG)(WORD)WORDAT(packed + 2);
    s->bitsleft = WORDAT(packed + 4);
    s->count = 0xffff;                  /* MOVEQ #-1,d3 */
    if (!stcloop(s, dest, dest + STC_SHORTMAX - 1, farbits, 0)
        || !stcloop(s, dest, dest, farbits, 1))
        return 0;
    *end = s->out;
    return 1;
}

#endif
