/*
 * iffiolib - command library 162: buffered reading of IFF files for other
 * libraries (no tokens of its own).  Used through ALibJsr by ilbmifflib,
 * animlib, audiolib, palettelib and lib 154:
 *      +1 iffopen   a0 = file name
 *      +2 iffclose
 *      +3 iffbyte   -> d0.b = next byte
 *      +4 iffword   -> d0.w = next word (big endian)
 *      +5 ifflong   -> d0 = next long
 *      +6 ifffind   d0 = chunk id -> d0 = chunk length
 *      +7 iffskip   d0 = number of BYTES to skip (the source says "long
 *                   words")
 *      +8 iffread   d0 = number of bytes, a0 = destination
 * init: a0 = dos base (!libs #doslib,#la0); allocates the 2048 byte buffer
 * (memlib globalloc) and returns its address (return long).
 *
 * C translation of bblibsrc/iffiolib; binary of the CD's acidlibs checked:
 * same as the source.  One file at a time (filehand, iffbuff, charat,
 * charsleft are global).
 *
 * Results in the CCR (the callers branch on it):
 *      iffopen   Z = opened (d0 unchanged), else d0 = -1 (N)
 *      iffbyte, iffword, ifflong, ifffind
 *                Z = ok (CMP d0,d0), else d0 = -1 (MOVEQ: N, not Z);
 *                iffbyte only replaces d0.b, iffword d0.w (the rest of d0
 *                stays as the caller had it)
 *      iffskip   Z when all bytes were skipped (SUBQ.l reached 0), not Z
 *                at the end of the file; d0 = the last iffbyte result
 * The thunks set the CCR from the C result (tst.w iff_fail, -1 = failed).
 *
 * Registers: everything except d0 (and a0 for iffread, which returns the
 * advanced destination - or, after a direct dos Read, whatever dos left in
 * a0) is kept, like the original (iffopen loses a0 to dos.library in the
 * original; the C version keeps it).
 *
 * BUG(orig), kept:
 *  - charsleft is the low word of the Read result: a read error (-1) gives
 *    65535, so the buffer contents are taken as data (up to 2048 bytes per
 *    refill). (#177)
 *  - ifffind gives up at a "BODY" id (unless it is looking for BODY) and
 *    otherwise reads ahead byte by byte (the ids need not be aligned). (#178)
 *  - iffskip with d0 = 0 and iffread with d0 = 0 count down from 2^32
 *    (SUBQ.l/BNE). (#179)
 *  - Not a bug (all callers ignore d0/a0 after iffread): iffread: when the
 *    buffer position reaches 2048 the rest is read with one dos Read
 *    directly to a0 (d0 = its result, a0 not advanced); the buffer is
 *    refilled by the next iffbyte.  When the data ends inside
 *    the buffer, d0 = the bytes still wanted and a0 = the end.
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void iffopen(void), iffclose(void), iffbyte(void), iffword(void);
void ifflong(void), ifffind(void), iffskip(void), iffread(void);
ULONG codeinit(struct DosLibrary *dos BL_REG(a0));

BLITZ_ENTRY_DECL(init);

/* !libheader{#iffiolib,init,1,0,0} */
BLITZ_LIBHEADER(LIB_iffiolib, init, BL_LONGRET, 0, 0);

BLITZ_NULLSUB(iffopen, 0, 0);
BLITZ_NULLSUB(iffclose, 0, 0);
BLITZ_NULLSUB(iffbyte, 0, 0);
BLITZ_NULLSUB(iffword, 0, 0);
BLITZ_NULLSUB(ifflong, 0, 0);
BLITZ_NULLSUB(ifffind, 0, 0);
BLITZ_NULLSUB(iffskip, 0, 0);
BLITZ_NULLSUB(iffread, 0, 0);

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_doslib, BL_LA(0));

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

#define BUFSIZE 2048

static UBYTE *iffbuff;
static UWORD charat;            /* position in the buffer */
static UWORD charsleft;         /* bytes in the buffer (low word of Read) */
static BPTR filehand;
static struct DosLibrary *dosbase;
#define DOSBase dosbase         /* for proto/dos.h */

/* CCR result for the thunks: 0 = ok (Z), -1 = failed (N) */
UWORD iff_fail;
/* a0 result of iffread */
UBYTE *iff_a0;

#define ID_BODY 0x424f4459      /* "BODY" */

/* ------------------------------------------------------------------ init */

/**
 * @brief The library's init: remember dos.library, allocate the buffer.
 *
 * The 2048 byte buffer comes from memlib's globalloc (MEMF_PUBLIC).
 *
 * @param dos  a0: dos.library (!libs #doslib,#la0)
 * @return d0: the buffer's address (the library's init value)
 */
ULONG BLITZ_ROUTINE codeinit(struct DosLibrary *dos BL_REG(a0))
{
    struct bl_regs r = { .d0 = BUFSIZE, .d1 = MEMF_PUBLIC };

    dosbase = dos;
    iffbuff = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);     /* global alloc! */
    return (ULONG)iffbuff;
}

/* ---------------------------------------------------------------- reading */

static void readbuff(void)
{
    charsleft = (UWORD)Read(filehand, iffbuff, BUFSIZE);
    charat = 0;
}

/**
 * @brief iffopen (+1): open a file for reading and fill the buffer.
 *
 * One file at a time.  CCR for the caller (thunk): Z = opened.
 *
 * @param value  d0: returned unchanged if the file was opened
 * @param name   a0: the file name
 * @return d0: value if opened, -1 (N, not Z) if not
 */
static ULONG BLITZ_ROUTINE iffopen_c(ULONG value BL_REG(d0), const char *name BL_REG(a0))
{
    filehand = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!filehand) {
        iff_fail = 0xffff;
        return (ULONG)-1;
    }
    readbuff();
    iff_fail = 0;
    return value;
}
BLITZ_THUNK_POST(iffopen, iffopen_c, "d1/a0-a1", "tst.w _iff_fail");

/** @brief iffclose (+2): close the file (dos Close, also of 0). */
static void BLITZ_ROUTINE iffclose_c(void)
{
    Close(filehand);
}
BLITZ_THUNK(iffclose, iffclose_c, "d0-d1/a0-a1");

/* one byte: 0..255, or -1 at the end of the data */
static LONG getbyte(void)
{
    UWORD at = charat;

    if (at >= BUFSIZE) {
        readbuff();
        at = 0;
    }
    if (at >= charsleft)
        return -1;
    charat++;
    return iffbuff[at];
}

/* iffbyte - d0.b = byte (rest of d0 kept), or d0 = -1 */
static ULONG byte_(ULONG value)
{
    LONG b = getbyte();

    if (b < 0) {
        iff_fail = 0xffff;
        return (ULONG)-1;
    }
    iff_fail = 0;
    return (value & 0xffffff00UL) | (UBYTE)b;
}

/* iffword - d0.w = word (rest of d0 kept), or d0 = -1 */
static ULONG word_(ULONG value)
{
    ULONG part;                         /* d1 */

    value = byte_(value);
    if (iff_fail)
        return value;
    part = (UWORD)(value << 8);         /* MOVE d0,d1:LSL #8,d1 */
    value = byte_(value);
    if (iff_fail)
        return value;
    BL_SETB(part, value);               /* MOVE.b d0,d1 */
    BL_SETW(value, part);               /* MOVE d1,d0 */
    return value;
}

/* ifflong - d0 = long, or -1 */
static ULONG long_(ULONG value)
{
    ULONG part;                         /* d1 */

    value = word_(value);
    if (iff_fail)
        return value;
    part = (ULONG)(UWORD)value << 16;   /* MOVE d0,d1:SWAP d1 (low word of
                                           the caller's d1 is overwritten) */
    value = word_(value);
    if (iff_fail)
        return value;
    return part | (UWORD)value;
}

/**
 * @brief iffbyte (+3): the next byte of the file.
 *
 * Refills the buffer when it is used up.  CCR: Z = ok.
 * BUG(orig) (#177): after a read error (-1) the buffer counts as 65535
 * bytes long.
 *
 * @param value  d0: the caller's d0, only its low byte is replaced
 * @return d0: value with the byte in the low byte, -1 (N, not Z) at the
 *         end of the data
 */
static ULONG BLITZ_ROUTINE iffbyte_c(ULONG value BL_REG(d0))
{
    return byte_(value);
}
BLITZ_THUNK_POST(iffbyte, iffbyte_c, "d1/a0-a1", "tst.w _iff_fail");

/**
 * @brief iffword (+4): the next word (big endian), two iffbytes.
 *
 * CCR: Z = ok.
 *
 * @param value  d0: the caller's d0, only its low word is replaced
 * @return d0: value with the word in the low word, -1 at the end of the
 *         data
 */
static ULONG BLITZ_ROUTINE iffword_c(ULONG value BL_REG(d0))
{
    return word_(value);
}
BLITZ_THUNK_POST(iffword, iffword_c, "d1/a0-a1", "tst.w _iff_fail");

/**
 * @brief ifflong (+5): the next long (big endian), two iffwords.
 *
 * CCR: Z = ok.
 *
 * @param value  d0: the caller's d0
 * @return d0: the long, -1 at the end of the data
 */
static ULONG BLITZ_ROUTINE ifflong_c(ULONG value BL_REG(d0))
{
    return long_(value);
}
BLITZ_THUNK_POST(ifflong, ifflong_c, "d1/a0-a1", "tst.w _iff_fail");

/**
 * @brief ifffind (+6): find a chunk.
 *
 * Reads up to and including the id, then the chunk length (ifflong).
 * CCR: Z = found.
 * BUG(orig) (#178): it gives up at a "BODY" id (unless it is looking for
 * BODY) and reads ahead byte by byte (the ids need not be aligned).
 *
 * @param id  d0: the chunk id
 * @return d0: the chunk length, -1 at the end of the data or at "BODY"
 */
static ULONG BLITZ_ROUTINE ifffind_c(ULONG id BL_REG(d0))
{
    ULONG last4 = 0;                    /* d2: the last 4 bytes */
    UWORD at = charat;                  /* d1 */

    for (;;) {
        if (at >= BUFSIZE) {
            readbuff();
            at = 0;
        }
        if (at >= charsleft) {
            iff_fail = 0xffff;
            return (ULONG)-1;
        }
        at++;
        charat = at;
        last4 = (last4 << 8) | iffbuff[(UWORD)(at - 1)]; /* MOVE.b -1(a0,d1),d2 */
        if (last4 == id)
            return long_(id);
        if (last4 == ID_BODY) {
            iff_fail = 0xffff;
            return (ULONG)-1;
        }
    }
}
BLITZ_THUNK_POST(ifffind, ifffind_c, "d1/a0-a1", "tst.w _iff_fail");

/**
 * @brief iffskip (+7): skip bytes (the source says "long words").
 *
 * CCR: Z when all bytes were skipped, not Z at the end of the data.
 * BUG(orig) (#179): d0 = 0 counts down from 2^32.
 *
 * @param n  d0: the number of bytes
 * @return d0: the last iffbyte result
 */
static ULONG BLITZ_ROUTINE iffskip_c(ULONG n BL_REG(d0))
{
    ULONG value = n;                    /* d0 */

    for (;;) {
        value = byte_(value);
        if (iff_fail)
            return value;               /* BNE 'done: Z clear */
        if (--n == 0)
            return value;               /* SUBQ.l: Z set, iff_fail = 0 */
    }
}
BLITZ_THUNK_POST(iffskip, iffskip_c, "d1/a0-a1", "tst.w _iff_fail");

/**
 * @brief iffread (+8): copy bytes of the file to memory.
 *
 * From the buffer; when the buffer position reaches 2048 the rest is read
 * with one dos Read directly to a0 (a0 not advanced; the buffer is
 * refilled by the next iffbyte).  When the data ends inside the buffer,
 * d0 = the bytes still wanted and a0 = the end.  Not a bug: all callers
 * ignore d0/a0 afterwards.
 * BUG(orig) (#179): d0 = 0 counts down from 2^32.
 *
 * @param n    d0: the number of bytes
 * @param dst  a0: the destination; a0 afterwards: see above
 * @return d0: 0, the bytes still wanted, or the result of the dos Read
 */
static ULONG BLITZ_ROUTINE iffread_c(ULONG n BL_REG(d0), UBYTE *dst BL_REG(a0))
{
    UWORD at = charat;                  /* d1 */
    LONG res;

    for (;;) {
        if (at >= BUFSIZE)
            break;
        if (at >= charsleft) {
            charat = at;
            iff_a0 = dst;
            return n;
        }
        *dst++ = iffbuff[at++];
        if (--n == 0) {
            charat = at;
            iff_a0 = dst;
            return 0;
        }
    }
    charat = at;
    res = Read(filehand, dst, n);
    iff_a0 = dst;
    return (ULONG)res;
}
BLITZ_THUNK_POST(iffread, iffread_c, "d1/a1", "move.l _iff_a0,a0");
