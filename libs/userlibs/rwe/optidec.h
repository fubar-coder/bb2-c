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
    ULONG d6;                           /* bit buffer: bits at the top of the low word */
    UWORD d7;                           /* bits left in it */
    UWORD d1, d3;
    const UBYTE *a1;                    /* packed words, read backwards */
    UBYTE *a0;                          /* output, written backwards */
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
    s->a1 -= 2;
    s->d6 = (s->d6 & 0xffff0000UL) | WORDAT(s->a1);
}

/* DBF d7,ok:MOVE.w -(a1),d6:MOVEQ #15,d7:ok: */
static void next(struct stc *s)
{
    if (s->d7-- == 0) {
        loadword(s);
        s->d7 = 15;
    }
}

/* ADD.w d6,d6 - the carry */
static int bit(struct stc *s)
{
    UWORD w = (UWORD)s->d6;
    s->d6 = (s->d6 & 0xffff0000UL) | (UWORD)(w << 1);
    return w >> 15;
}

/* ROL.w #8,d6:MOVE.b d6,-(a0) */
static void rolbyte(struct stc *s)
{
    UWORD w = (UWORD)s->d6;
    w = (UWORD)(w << 8 | w >> 8);
    s->d6 = (s->d6 & 0xffff0000UL) | w;
    *--s->a0 = (UBYTE)w;
}

/* LSL.l d1,d6 ... SWAP d6:MOVE.b d6,-(a0):SWAP d6 - a byte from the upper word */
static void upperbyte(struct stc *s)
{
    *--s->a0 = (UBYTE)(s->d6 >> 16);
}

/* the getbits routine ($404): d1 bits -> d1 */
static void getbits(struct stc *s)
{
    UWORD n = s->d1;

    s->d6 &= 0xffff;                    /* AND.l d4,d6 */
    s->d7 = (UWORD)(s->d7 - n);
    if ((WORD)s->d7 < 0) {              /* BPL not taken */
        n = (UWORD)(n + s->d7);         /* ADD.w d7,d1 */
        s->d6 = lsl(s->d6, n);
        n = s->d7;
        loadword(s);
        n = (UWORD)-n;
        s->d7 = (UWORD)(s->d7 + 16);    /* ADD.w d5,d7 */
    }
    s->d6 = lsl(s->d6, n);
    s->d1 = (UWORD)(s->d6 >> 16);       /* MOVE.l d6,d1:SWAP d1 */
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
    const UBYTE *a1 = file + 8;         /* ADDQ.w #8,a1 */
    UBYTE *a5 = dest, *a2;
    WORD d2;
    UWORD n;

    s->a0 = dest + LONGAT(a1);          /* ADDA.l (a1)+,a0 */
    a1 += 4;
    a1 += LONGAT(a1);                   /* ADDA.l (a1),a1 */
    s->a1 = a1;
    d2 = (WORD)WORDAT(a1);              /* MOVEM.w (a1),d2/d6/d7 */
    s->d6 = (ULONG)(LONG)(WORD)WORDAT(a1 + 2);
    s->d7 = WORDAT(a1 + 4);
    s->d1 = 0;
    s->d3 = 0xffff;                     /* MOVEQ #-1,d3 */
    goto check;

literal:                                /* $32C: 8 bits */
    s->d7 = (UWORD)(s->d7 - 8);
    if ((WORD)s->d7 >= 0) {
        rolbyte(s);
        goto check;
    }
    n = s->d7;                          /* across the next word */
    s->d7 = (UWORD)(s->d7 + 8);
    s->d6 = lsl(s->d6, s->d7);
    loadword(s);
    n = (UWORD)-n;
    s->d6 = lsl(s->d6, n);
    s->d7 = (UWORD)(s->d7 + 8);
    upperbyte(s);
check:                                  /* $350: CMPA.l a0,a5:DBCC d7,$35C */
    if ((ULONG)a5 >= (ULONG)s->a0)
        return s->a0;
    next(s);
    /* $35C */
    if (!bit(s))
        goto literal;
    next(s);
    if (bit(s)) {                       /* $3CC: 11x - 2 or 3 bytes */
        next(s);
        s->d3 = (UWORD)(s->d3 + 1 + bit(s));   /* ADDX.w d0,d3 (d0 = 1) */
        goto offset;
    }
    next(s);
    s->d1 = 2;
    s->d3 = 2;
    if (bit(s))                         /* 101xx: 4..7 bytes */
        goto length;
    next(s);
    if (!bit(s)) {                      /* 1000: 23 + n bytes, n in bytes */
        s->d3 = 21;
        goto length8;
    }
    s->d1 = 4;                          /* 1001xxxx: 8..22 bytes */
    s->d3 = 6;
    getbits(s);
    s->d3 = (UWORD)(s->d3 + s->d1);
    if (s->d1 < 15)                     /* CMP.w #15,d1:BCS */
        goto offset;
    s->d1 = 5;                          /* 10011111xxxxx: 14..45 literals */
    s->d3 = 14;
    getbits(s);
    s->d3 = (UWORD)(s->d3 + s->d1);
    s->d1 = (UWORD)((s->d1 & 0xff00) | runtab[s->d7 & 15]);   /* DIFF: & 15 */
    if (s->d1 & 0x80) {                 /* a whole byte left in the word */
        s->d7 = (UWORD)(s->d7 - 8);
        /* DBF d3 with d3 = 14..45 always branches (else: into the table) */
        s->d3--;
        goto run2;
    }
    goto run1;

run2:                                   /* $424 */
    rolbyte(s);
run1:                                   /* $428 */
    s->d6 = lsl(s->d6, s->d7);
    loadword(s);
    s->d6 = lsl(s->d6, s->d1);
    upperbyte(s);
    if ((LONG)(WORD)s->d3 - 2 > 0) {    /* SUBQ.w #2,d3:BGT */
        s->d3 = (UWORD)(s->d3 - 2);
        goto run2;
    }
    s->d3 = (UWORD)(s->d3 - 2);
    if (s->d3 == 0) {                   /* BEQ: one byte more */
        s->d3 = 0xffff;
        rolbyte(s);
        goto check;
    }
    s->d7 = (UWORD)(s->d7 + 8);
    goto check;                         /* $3C2: the same as $350 */

length8:                                /* $3A6 */
    s->d1 = 8;
length:                                 /* $3A8, back at $3AC */
    getbits(s);
    s->d3 = (UWORD)(s->d3 + s->d1);
    if ((UBYTE)~s->d1 == 0)             /* NOT.b d1: 255 - one more byte */
        goto length8;
offset:                                 /* $3D8 / $3E0 */
    next(s);
    if (bit(s)) {                       /* 1: far, d2 bits + 544 */
        a2 = s->a0 + 544;
        s->d1 = (UWORD)d2;
    } else {
        next(s);
        s->d1 = 9;                      /* 00: 9 bits + 32 */
        a2 = s->a0 + 32;
        if (bit(s)) {                   /* 01: 5 bits */
            s->d1 = 5;
            a2 = s->a0;
        }
    }
    getbits(s);
    a2 += (WORD)s->d1;                  /* $3B8: ADDA.w d1,a2 */
    *--s->a0 = *a2;
    do
        *--s->a0 = *--a2;
    while (s->d3-- != 0);               /* DBF d3 */
    goto check;                         /* $3C2 */
}

#endif
