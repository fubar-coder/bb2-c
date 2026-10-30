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

/* iffopen - a0 = file name; d0 unchanged if opened, else -1 */
static ULONG BLITZ_ROUTINE iffopen_c(ULONG d0 BL_REG(d0), const char *name BL_REG(a0))
{
    filehand = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!filehand) {
        iff_fail = 0xffff;
        return (ULONG)-1;
    }
    readbuff();
    iff_fail = 0;
    return d0;
}
BLITZ_THUNK_POST(iffopen, iffopen_c, "d1/a0-a1", "tst.w _iff_fail");

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
static ULONG byte_(ULONG d0)
{
    LONG b = getbyte();

    if (b < 0) {
        iff_fail = 0xffff;
        return (ULONG)-1;
    }
    iff_fail = 0;
    return (d0 & 0xffffff00UL) | (UBYTE)b;
}

/* iffword - d0.w = word (rest of d0 kept), or d0 = -1 */
static ULONG word_(ULONG d0)
{
    ULONG d1;

    d0 = byte_(d0);
    if (iff_fail)
        return d0;
    d1 = (UWORD)(d0 << 8);              /* MOVE d0,d1:LSL #8,d1 */
    d0 = byte_(d0);
    if (iff_fail)
        return d0;
    BL_SETB(d1, d0);                    /* MOVE.b d0,d1 */
    BL_SETW(d0, d1);                    /* MOVE d1,d0 */
    return d0;
}

/* ifflong - d0 = long, or -1 */
static ULONG long_(ULONG d0)
{
    ULONG d1;

    d0 = word_(d0);
    if (iff_fail)
        return d0;
    d1 = (ULONG)(UWORD)d0 << 16;        /* MOVE d0,d1:SWAP d1 (low word of
                                           the caller's d1 is overwritten) */
    d0 = word_(d0);
    if (iff_fail)
        return d0;
    return d1 | (UWORD)d0;
}

static ULONG BLITZ_ROUTINE iffbyte_c(ULONG d0 BL_REG(d0))
{
    return byte_(d0);
}
BLITZ_THUNK_POST(iffbyte, iffbyte_c, "d1/a0-a1", "tst.w _iff_fail");

static ULONG BLITZ_ROUTINE iffword_c(ULONG d0 BL_REG(d0))
{
    return word_(d0);
}
BLITZ_THUNK_POST(iffword, iffword_c, "d1/a0-a1", "tst.w _iff_fail");

static ULONG BLITZ_ROUTINE ifflong_c(ULONG d0 BL_REG(d0))
{
    return long_(d0);
}
BLITZ_THUNK_POST(ifflong, ifflong_c, "d1/a0-a1", "tst.w _iff_fail");

/* ifffind - d0 = id; reads up to and including the id, then the chunk
   length (ifflong); -1 at the end of the data or at "BODY" */
static ULONG BLITZ_ROUTINE ifffind_c(ULONG id BL_REG(d0))
{
    ULONG d2 = 0;
    UWORD d1 = charat;

    for (;;) {
        if (d1 >= BUFSIZE) {
            readbuff();
            d1 = 0;
        }
        if (d1 >= charsleft) {
            iff_fail = 0xffff;
            return (ULONG)-1;
        }
        d1++;
        charat = d1;
        d2 = (d2 << 8) | iffbuff[(UWORD)(d1 - 1)];  /* MOVE.b -1(a0,d1),d2 */
        if (d2 == id)
            return long_(id);
        if (d2 == ID_BODY) {
            iff_fail = 0xffff;
            return (ULONG)-1;
        }
    }
}
BLITZ_THUNK_POST(ifffind, ifffind_c, "d1/a0-a1", "tst.w _iff_fail");

/* iffskip - skip d0 bytes; Z when done, not Z at the end of the data */
static ULONG BLITZ_ROUTINE iffskip_c(ULONG n BL_REG(d0))
{
    ULONG d0 = n;

    for (;;) {
        d0 = byte_(d0);
        if (iff_fail)
            return d0;                  /* BNE 'done: Z clear */
        if (--n == 0)
            return d0;                  /* SUBQ.l: Z set, iff_fail = 0 */
    }
}
BLITZ_THUNK_POST(iffskip, iffskip_c, "d1/a0-a1", "tst.w _iff_fail");

/* iffread - copy d0 bytes to a0 (see the head comment) */
static ULONG BLITZ_ROUTINE iffread_c(ULONG n BL_REG(d0), UBYTE *dst BL_REG(a0))
{
    UWORD d1 = charat;
    LONG res;

    for (;;) {
        if (d1 >= BUFSIZE)
            break;
        if (d1 >= charsleft) {
            charat = d1;
            iff_a0 = dst;
            return n;
        }
        *dst++ = iffbuff[d1++];
        if (--n == 0) {
            charat = d1;
            iff_a0 = dst;
            return 0;
        }
    }
    charat = d1;
    res = Read(filehand, dst, n);
    iff_a0 = dst;
    return (ULONG)res;
}
BLITZ_THUNK_POST(iffread, iffread_c, "d1/a1", "move.l _iff_a0,a0");
