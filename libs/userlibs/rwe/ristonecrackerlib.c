/*
 * ristonecrackerlib - RWE userlib 17 (Reflective Images): StoneCracker
 * 4.04 ("S404") files - STCLength, STCStart, STCSafeLen, STCLoad.
 *
 * C translation of rwelibsrc/stclib.bb with the decruncher optidec.bin
 * it includes; reference BSS2 RISTONECRACKERLIB.OBJ, which is newer than
 * the source (docs/rwelibs/README.md 3.2): STCStart and STCSafeLen check
 * the "S404" header too (CMP.l #"S404",(a0):BNE oops, as STCLength in the
 * source), so they return 0 for other data, as the BSS2 manual says.
 * Notes: docs/rwelibs/port/ristonecrackerlib.md.
 *
 * An S404 file: "S404", the security offset, the unpacked length, the
 * packed length, the packed data; behind them three words (the number of
 * offset bits of far matches, the first bit buffer, its number of bits).
 * The data unpack backwards, from address + offset + unpacked length down
 * to address + offset, so a buffer of offset + unpacked length bytes
 * (STCSafeLen) unpacks in place; STCStart = address + offset.
 *
 *   STCLength(a)   the unpacked length, 0 if a is no S404 file
 *   STCStart(a)    a + offset, 0 if no S404 file
 *   STCSafeLen(a)  unpacked length + offset, 0 if no S404 file
 *   STCLoad f$,a   reads file f$ to a (its whole length), unpacks it in
 *                  place, returns the start of the data (0 if the file
 *                  cannot be locked, opened or the FileInfoBlock not
 *                  allocated)
 *
 * The decruncher (optidec.bin, third party code of StoneCracker 4.04) is
 * translated register by register (dostc below): d6 is the 32 bit bit
 * buffer, the counts are words, shifts are taken modulo 64 as by the
 * 68000, DBF loops count the low word down to -1 - the unpacked data are
 * the original's for every input it handles (see DIFF).  No assembler
 * besides the thunks.  _stcdecrunch2 of the source (unpack to another
 * address) is not in the token table and not called: left out.
 *
 * BUG(orig):
 *   - STCLoad: OpenFromLock takes over the lock, but after Close the lock
 *     is unlocked again (oopsfreefib: FreeMem + UnLock) - a second UnLock
 *     of a lock the file system has already freed.
 *   - STCLoad does not check the header: any other file is "unpacked"
 *     with whatever its words say; it reads the whole file whatever the
 *     buffer's size; Examine's and Read's results are not checked.
 *   - dos.library (opened by init) is never closed (finit is 0).
 *   - The decruncher does not check the data: corrupt data write below
 *     the buffer (a match or literal run past the start) and read the
 *     packed words below the file.
 * DIFF: the literal run reads its shift count from a table indexed by the
 *   bit count d7 (MOVE.b table(pc,d7.w)); d7 leaves 0..15 only with a
 *   header of more than 16 offset bits, and then the original reads
 *   whatever lies around the table (its own code) - the C build takes the
 *   index modulo 16.  With d7 = 0..15 (every file with 0..16 offset bits)
 *   the result is the same.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dos
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _stclength(void), _stcstart(void), _stcsafelen(void), _stcload(void);
void codeinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#ristonecrackerlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ristonecrackerlib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stclength, 0, 0);
BLITZ_NAME("STCLength", "address   -- returns unpacked size");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stcstart, 0, 0);
BLITZ_NAME("STCStart", "address   -- returns address of unpacked data");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stcsafelen, 0, 0);
BLITZ_NAME("STCSafeLen", "address   -- safe unpack buffer size");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stcload, 0, 0);
BLITZ_NAME("STCLoad", "filename$,address");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, 0, 0, 0);

BLITZ_LIBFIN();

#define S404 0x53343034UL
#define LONGAT(p) (*(const ULONG *)(p))
#define WORDAT(p) (*(const UWORD *)(p))

static struct Library *dosbase;         /* _dosbase */
static const char dosname[] = "dos.library";

/* ------------------------------------------------------- the decruncher */

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

/* dostc: a1 = the file, a0 = the destination (address + offset) */
static void dostc(const UBYTE *file, UBYTE *dest)
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
        return;
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

/* _stcdecrunch: unpack the file at a in place; the unpacked length (read
   before) */
static ULONG stcdecrunch(UBYTE *a)
{
    ULONG len = LONGAT(a + 8);          /* MOVE.l 8(a1),-(a7) */

    dostc(a, a + LONGAT(a + 4));
    return len;
}

/* ------------------------------------------------------------ commands */

/* init (codeinit): dos.library, any version */
static void BLITZ_ROUTINE codeinit_c(void)
{
    dosbase = OpenLibrary((CONST_STRPTR)dosname, 0);
}

/* STCLength(address) */
static ULONG BLITZ_ROUTINE stclength_c(const UBYTE *a BL_REG(d0))
{
    if (LONGAT(a) != S404)
        return 0;
    return LONGAT(a + 8);
}

/* STCStart(address): the check only in the binary */
static ULONG BLITZ_ROUTINE stcstart_c(const UBYTE *a BL_REG(d0))
{
    if (LONGAT(a) != S404)
        return 0;
    return (ULONG)a + LONGAT(a + 4);
}

/* STCSafeLen(address): the check only in the binary */
static ULONG BLITZ_ROUTINE stcsafelen_c(const UBYTE *a BL_REG(d0))
{
    if (LONGAT(a) != S404)
        return 0;
    return LONGAT(a + 8) + LONGAT(a + 4);
}

/* STCLoad filename$,address (d0, d1) */
static ULONG BLITZ_ROUTINE stcload_c(CONST_STRPTR name BL_REG(d0), UBYTE *ad BL_REG(d1))
{
    struct Library *dos = dosbase;
    struct FileInfoBlock *fib;
    BPTR lock, fh;
    LONG size;

    lock = Lock(name, ACCESS_READ);     /* MOVEQ #-2,d2 */
    if (!lock)
        return 0;
    fib = AllocMem(264, MEMF_CLEAR);    /* sizeof(struct FileInfoBlock) */
    if (!fib) {
        UnLock(lock);
        return 0;
    }
    Examine(lock, fib);                 /* not checked */
    size = fib->fib_Size;
    fh = OpenFromLock(lock);
    if (!fh) {
        FreeMem(fib, 264);
        UnLock(lock);
        return 0;
    }
    Read(fh, ad, size);                 /* not checked */
    Close(fh);
    FreeMem(fib, 264);                  /* BSR oopsfreefib */
    UnLock(lock);                       /* BUG(orig): OpenFromLock took it */
    stcdecrunch(ad);                    /* BUG(orig): no header check */
    return (ULONG)ad + LONGAT(ad + 4);
}

/* Registers: the original changes d0/a0 (STCLength, STCStart,
   STCSafeLen), d0-d1/a0-a1/a6 (init), and d0-d7/a0-a1/a6 (STCLoad: the
   system calls and the decruncher); the thunks keep d1/a0/a1, the C code
   d2-d7/a2-a6. */
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_stclength, stclength_c, "d1/a0-a1");
BLITZ_THUNK(_stcstart, stcstart_c, "d1/a0-a1");
BLITZ_THUNK(_stcsafelen, stcsafelen_c, "d1/a0-a1");
BLITZ_THUNK(_stcload, stcload_c, "d1/a0-a1");
