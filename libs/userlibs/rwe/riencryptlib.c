/*
 * riencryptlib - RWE userlib 52 (Reflective Images, Stephen McNamara,
 * 1994/96): Encrypt, Decrypt, GetWheel - a simple three-wheel byte cipher.
 *
 * C translation of rwelibsrc/Encryptlib/Encrypt-Library.bb; reference
 * BSS2 RIENCRYPTLIB.OBJ (the reference build of the source equals it byte
 * for byte, docs/rwelibs/README.md 3.1).  Notes:
 * docs/rwelibs/port/riencryptlib.md.
 *
 * Every byte b of the block becomes b + w1 + w2 + w3 (Encrypt), the
 * wheels stepped before each byte like an odometer (w1 + 1, on its carry
 * w2 + 1, on that carry w3 + 1); Decrypt goes backwards from the end and
 * steps the wheels back after each byte.  The last wheel state is kept
 * for GetWheel.  Encrypt without wheels takes random start values (Rnd of
 * vallib).  Decrypt with mode = 1 first adds the bytes 0..2 of the length
 * to the wheels, to get from the start state to the state Encrypt left.
 *
 * No assembler besides the thunks.
 *
 * BUG(orig):
 *   - The loops count with SUBQ.l #1 before a BPL loop whose first pass
 *     is unconditional: a length of 0 (or one above 2^31) processes one
 *     byte, Decrypt the one at memadr-1 (#630).
 *   - Decrypt mode 1 adds the length's bytes to the wheels without the
 *     carries between them: if w1 + (length & $FF) overflows (or w2 + the
 *     next byte), the state is one step off in w2 (w3) and the data is not
 *     decrypted correctly (#629).
 * Encrypt's random wheels: `ALibJsr Rnd` is the first form of Rnd, the
 * one without a range (Quick 0..1, the fraction in the low word): the
 * ranges 256, 65535 and -1 loaded into d0 are ignored; each wheel is the
 * low byte of the second of two Rnd calls.  No defect: the wheels are
 * random bytes either way.
 */
#include "blitzlib.h"

/* vallib Rnd, its first form (no range): -> d0 = 16 random bits */
#define TOK_rnd ((LIB_vallib << 7) + 7)

/* entry points (thunks, below) */
void _encrypt2(void), _encrypt(void), _decrypt(void), _decrypt2(void);
void _getwheel(void);

/* !libheader {#riencryptlib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_riencryptlib, 0, BL_NORET, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_encrypt2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_encrypt, 0, 0);
BLITZ_NAME("Encrypt", "memadr,length[,wheel1,wheel2,wheel3]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_decrypt, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_decrypt2, 0, 0);
BLITZ_NAME("Decrypt", "memadr,length,wheel1,wheel2,wheel3[,mode]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_getwheel, 0, 0);
BLITZ_NAME("GetWheel", "wheel#");

BLITZ_LIBFIN();

/* wheels: Dcb.b 3,0 - the state after the last Encrypt/Decrypt */
static UBYTE wheels[3];

/* the loop counter: SUBQ.l #1,d1 first, then the body until SUBQ.l #1,d1
   makes it negative (BPL) - at least one pass (BUG(orig) (#630), see above) */
#define NEG(n) ((n) & 0x80000000UL)

/**
 * @brief Encrypt memadr,length,wheel1,wheel2,wheel3: encrypts a block in
 * place with the three-wheel cipher.
 *
 * Every byte b becomes b + w1 + w2 + w3, the wheels stepped before each
 * byte like an odometer (w1 + 1, on its carry w2 + 1, on that w3 + 1);
 * the last state is kept for GetWheel.  BUG(orig) (#630): length 0 (or
 * one above 2^31) processes one byte.
 * @param p       d0: memadr
 * @param len     d1: length
 * @param wheel1  d2: the start of wheel 1 (its low byte, ANDI.l #$ff)
 * @param wheel2  d3: the start of wheel 2
 * @param wheel3  d4: the start of wheel 3
 * @return d0: memadr with the last byte written in its low byte
 */
static ULONG BLITZ_ROUTINE encrypt_c(UBYTE *p BL_REG(d0), ULONG len BL_REG(d1),
                                    UWORD wheel1 BL_REG(d2),
                                    UWORD wheel2 BL_REG(d3),
                                    UWORD wheel3 BL_REG(d4))
{
    UBYTE a = (UBYTE)wheel1, b = (UBYTE)wheel2, c = (UBYTE)wheel3, x;
    ULONG n = len - 1, result = (ULONG)p;           /* d0 */

    do {
        if (++a == 0)                                   /* ADDQ.b #1:BCC */
            if (++b == 0)
                ++c;
        *p = x = (UBYTE)(*p + a + b + c);
        p++;
    } while (!NEG(--n));
    wheels[0] = a;
    wheels[1] = b;
    wheels[2] = c;
    return BL_SETB(result, x);                          /* d0.b = the last byte */
}

/* two Rnd calls (the second gets the first's result in d0), the low byte
   of the second; `range` is what the original loads into d0 (ignored by
   the first form of Rnd, no defect: see above) */
static UBYTE rndwheel(ULONG range)
{
    struct bl_regs r = { .d0 = range };

    BLITZ_ALIBJSR(TOK_rnd, r);
    BLITZ_ALIBJSR(TOK_rnd, r);
    return (UBYTE)r.d0;
}

/**
 * @brief Encrypt memadr,length: as above with random start wheels (vallib
 * Rnd); GetWheel tells the state after it.
 * @param p    d0: memadr
 * @param len  d1: length
 * @return d0: memadr with the last byte written in its low byte
 */
static ULONG BLITZ_ROUTINE encrypt2_c(UBYTE *p BL_REG(d0), ULONG len BL_REG(d1))
{
    UBYTE a = rndwheel(256);
    UBYTE b = rndwheel(65535);
    UBYTE c = rndwheel(0xffffffffUL);

    return encrypt_c(p, len, a, b, c);
}

/**
 * @brief Decrypt memadr,length,wheel1,wheel2,wheel3: decrypts a block in
 * place, from the end backwards.
 *
 * The wheels are those Encrypt left (GetWheel); each byte loses w1 + w2 +
 * w3, then the wheels step back.  The last state is kept for GetWheel.
 * BUG(orig) (#630): length 0 processes one byte, the one at memadr-1.
 * @param base    d0: memadr
 * @param len     d1: length
 * @param wheel1  d2: wheel 1 (its low byte)
 * @param wheel2  d3: wheel 2
 * @param wheel3  d4: wheel 3
 * @return d0: memadr with the last byte written in its low byte
 */
static ULONG BLITZ_ROUTINE decrypt_c(UBYTE *base BL_REG(d0), ULONG len BL_REG(d1),
                                    UWORD wheel1 BL_REG(d2),
                                    UWORD wheel2 BL_REG(d3),
                                    UWORD wheel3 BL_REG(d4))
{
    UBYTE a = (UBYTE)wheel1, b = (UBYTE)wheel2, c = (UBYTE)wheel3, x;
    ULONG n = len - 1, result = (ULONG)base;        /* d0 */
    UBYTE *p = base + n;                                /* ADD.l d1,a0 */

    do {
        *p = x = (UBYTE)(*p - a - b - c);
        p--;
        if (a-- == 0)                                   /* SUBQ.b #1:BCC */
            if (b-- == 0)
                c--;
    } while (!NEG(--n));
    wheels[0] = a;
    wheels[1] = b;
    wheels[2] = c;
    return BL_SETB(result, x);                          /* d0.b = the last byte */
}

/**
 * @brief Decrypt memadr,length,wheel1,wheel2,wheel3,mode: as above; mode 1
 * starts from the wheels Encrypt started with.
 *
 * Mode 1 (the low byte, CMPI.b) adds the length's bytes 0..2 to the
 * wheels first.  BUG(orig) (#629): each without the carry into the next
 * wheel, so the state can be one step off.
 * @param p       d0: memadr
 * @param len     d1: length
 * @param wheel1  d2: wheel 1
 * @param wheel2  d3: wheel 2
 * @param wheel3  d4: wheel 3
 * @param mode    d5: 1 = the start wheels, else as Decrypt above
 * @return d0: memadr with the last byte written in its low byte
 */
static ULONG BLITZ_ROUTINE decrypt2_c(UBYTE *p BL_REG(d0), ULONG len BL_REG(d1),
                                     UWORD wheel1 BL_REG(d2),
                                     UWORD wheel2 BL_REG(d3),
                                     UWORD wheel3 BL_REG(d4), UWORD mode BL_REG(d5))
{
    if ((UBYTE)mode == 1) {
        wheel1 += len & 0xff;
        wheel2 += (len >> 8) & 0xff;
        wheel3 += (len >> 16) & 0xff;
    }
    return decrypt_c(p, len, wheel1, wheel2, wheel3);
}

/**
 * @brief GetWheel(wheel#): a wheel after the last Encrypt/Decrypt.
 * @param n  d0.b: wheel# 1..3
 * @return d0: the wheel zero extended, -1 for another wheel#
 */
static ULONG BLITZ_ROUTINE getwheel_c(UWORD n BL_REG(d0))
{
    switch ((UBYTE)n) {
    case 1: return wheels[0];
    case 2: return wheels[1];
    case 3: return wheels[2];
    }
    return (ULONG)-1;
}

/* Registers: the original changes d0-d4 and a0 (Encrypt/Decrypt, d0.b =
   the last byte written; mode 1 also d5/d6), d0/d1 (GetWheel), and
   Encrypt without wheels whatever vallib Rnd changes.  The thunks keep a1
   (and d1 in GetWheel) as well. */
BLITZ_THUNK(_encrypt, encrypt_c, "a1");
BLITZ_THUNK(_encrypt2, encrypt2_c, "a1");
BLITZ_THUNK(_decrypt, decrypt_c, "a1");
BLITZ_THUNK(_decrypt2, decrypt2_c, "a1");
BLITZ_THUNK(_getwheel, getwheel_c, "d1/a0-a1");
