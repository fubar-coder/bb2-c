/*
 * optidec.h - the decruncher of StoneCracker 4.04 ("S404"), optidec.bin
 * (third party code that the RWE sources stclib.bb and stcdecrunchlib.bb
 * include with IncBin), in C for ristonecrackerlib.c and
 * ristcdecrunchlib.c (included by both; everything static).
 *
 * dostc(file, dest): file = the S404 file (a1), dest = the destination
 * (a0); the data unpack backwards from dest + unpacked length down to
 * dest.  Returns a0 at the end: dest for good data (the loop ends as soon
 * as a0 <= dest).  The original leaves d0 = 1 (MOVEQ #1,d0, the 1 of its
 * ADDX); the other registers are working registers.
 *
 * Translated register by register: d6 is the 32 bit bit buffer, the
 * counts are words, shifts are taken modulo 64 as by the 68000, DBF loops
 * count the low word down to -1 - the unpacked data are the original's
 * for every input it handles (see DIFF).
 *
 * BUG(orig): the decruncher does not check the data: corrupt data write
 *   below the buffer (a match or literal run past the start) and read the
 *   packed words below the file (#833).
 * DIFF: the literal run reads its shift count from a table indexed by the
 *   bit count d7 (MOVE.b table(pc,d7.w)); d7 leaves 0..15 only with a
 *   header of more than 16 offset bits, and then the original reads
 *   whatever lies around the table (its own code) - the C build takes the
 *   index modulo 16.  With d7 = 0..15 (every file with 0..16 offset bits)
 *   the result is the same.
 */
#ifndef OPTIDEC_H
#define OPTIDEC_H

#define LONGAT(p) (*(const ULONG *)(p))
#define WORDAT(p) (*(const UWORD *)(p))

/* the 68000 registers optidec works with */
struct stc {
    ULONG bitbuf;                       /* d6: bit buffer: bits at the top of the low word */
    UWORD bitsleft;                     /* d7: bits left in it */
    UWORD bits;                         /* d1: getbits' count and result, a run's shift */
    UWORD count;                        /* d3: bytes to copy */
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
static void getbits(struct stc *s)
{
    UWORD n = s->bits;

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
    s->bits = (UWORD)(s->bitbuf >> 16); /* MOVE.l d6,d1:SWAP d1 */
}

/* the table behind the decruncher ($44C): the shift count of a literal
   run's straddling byte, by the bits left (8 - n, $80 + 16 - n) */
static const UBYTE runtab[16] = {
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
    0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81,
};

/* dostc: a1 = the file, a0 = the destination; a0 at the end */
static UBYTE *dostc(const UBYTE *file, UBYTE *dest)
{
    struct stc st, *s = &st;
    const UBYTE *packed = file + 8;     /* ADDQ.w #8,a1 */
    UBYTE *start = dest, *match;
    WORD farbits;
    UWORD n;

    s->out = dest + LONGAT(packed);     /* ADDA.l (a1)+,a0 */
    packed += 4;
    packed += LONGAT(packed);           /* ADDA.l (a1),a1 */
    s->in = packed;
    farbits = (WORD)WORDAT(packed);     /* MOVEM.w (a1),d2/d6/d7 */
    s->bitbuf = (ULONG)(LONG)(WORD)WORDAT(packed + 2);
    s->bitsleft = WORDAT(packed + 4);
    s->bits = 0;
    s->count = 0xffff;                  /* MOVEQ #-1,d3 */
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
    if ((ULONG)start >= (ULONG)s->out)
        return s->out;
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
    s->bits = 2;
    s->count = 2;
    if (bit(s))                         /* 101xx: 4..7 bytes */
        goto length;
    next(s);
    if (!bit(s)) {                      /* 1000: 23 + n bytes, n in bytes */
        s->count = 21;
        goto length8;
    }
    s->bits = 4;                        /* 1001xxxx: 8..22 bytes */
    s->count = 6;
    getbits(s);
    s->count = (UWORD)(s->count + s->bits);
    if (s->bits < 15)                   /* CMP.w #15,d1:BCS */
        goto offset;
    s->bits = 5;                        /* 10011111xxxxx: 14..45 literals */
    s->count = 14;
    getbits(s);
    s->count = (UWORD)(s->count + s->bits);
    s->bits = (UWORD)((s->bits & 0xff00) | runtab[s->bitsleft & 15]);  /* DIFF: & 15 */
    if (s->bits & 0x80) {               /* a whole byte left in the word */
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
    s->bitbuf = lsl(s->bitbuf, s->bits);
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

length8:                                /* $3A6 */
    s->bits = 8;
length:                                 /* $3A8, back at $3AC */
    getbits(s);
    s->count = (UWORD)(s->count + s->bits);
    if ((UBYTE)~s->bits == 0)           /* NOT.b d1: 255 - one more byte */
        goto length8;
offset:                                 /* $3D8 / $3E0 */
    next(s);
    if (bit(s)) {                       /* 1: far, d2 bits + 544 */
        match = s->out + 544;
        s->bits = (UWORD)farbits;
    } else {
        next(s);
        s->bits = 9;                    /* 00: 9 bits + 32 */
        match = s->out + 32;
        if (bit(s)) {                   /* 01: 5 bits */
            s->bits = 5;
            match = s->out;
        }
    }
    getbits(s);
    match += (WORD)s->bits;             /* $3B8: ADDA.w d1,a2 */
    *--s->out = *match;
    do
        *--s->out = *--match;
    while (s->count-- != 0);            /* DBF d3 */
    goto check;                         /* $3C2 */
}

#endif
