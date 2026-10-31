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
 * translated register by register (dostc in optidec.h, which
 * ristcdecrunchlib.c shares): d6 is the 32 bit bit buffer, the counts are words, shifts are taken modulo 64 as by the
 * 68000, DBF loops count the low word down to -1 - the unpacked data are
 * the original's for every input it handles (see DIFF).  No assembler
 * besides the thunks.  _stcdecrunch2 of the source (unpack to another
 * address) is not in the token table and not called: left out.
 *
 * BUG(orig):
 *   - STCLoad: OpenFromLock takes over the lock, but after Close the lock
 *     is unlocked again (oopsfreefib: FreeMem + UnLock) - a second UnLock
 *     of a lock the file system has already freed (#835).
 *   - STCLoad does not check the header: any other file is "unpacked"
 *     with whatever its words say; it reads the whole file whatever the
 *     buffer's size; Examine's and Read's results are not checked (#836).
 *   - dos.library (opened by init) is never closed (finit is 0) (#834).
 *   - The decruncher does not check the data: corrupt data write below
 *     the buffer (a match or literal run past the start) and read the
 *     packed words below the file (#833).
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

/* the head of an S404 file, then the packed data */
struct s404hdr {
    ULONG id;                           /* S404 */
    ULONG offset;                       /* the security offset */
    ULONG unpacked;                     /* the unpacked length */
    ULONG packed;                       /* the packed length */
};
static_assert(sizeof(struct s404hdr) == 16);
#define HDR(p) ((const struct s404hdr *)(p))

#define FIBSIZE 264     /* AllocMem'd for Examine: a little more than the FIB */
static_assert(sizeof(struct FileInfoBlock) <= FIBSIZE);

static struct Library *dosbase;         /* _dosbase */
static const char dosname[] = "dos.library";

/* the decruncher: dostc */
#include "optidec.h"

/* _stcdecrunch: unpack the file at a in place; the unpacked length (read
   before) */
static ULONG stcdecrunch(UBYTE *a)
{
    ULONG len = HDR(a)->unpacked;       /* MOVE.l 8(a1),-(a7) */

    dostc(a, a + HDR(a)->offset);
    return len;
}

/* ------------------------------------------------------------ commands */

/**
 * @brief init (codeinit): opens dos.library, any version.
 *
 * BUG(orig) (#834): it is never closed (finit is 0).
 * @return nothing
 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    dosbase = OpenLibrary((CONST_STRPTR)dosname, 0);
}

/**
 * @brief STCLength(address): the unpacked size of an S404 file in memory.
 * @param a  d0: the file
 * @return d0: the unpacked length, 0 if a is no S404 file
 */
static ULONG BLITZ_ROUTINE stclength_c(const UBYTE *a BL_REG(d0))
{
    if (HDR(a)->id != S404)
        return 0;
    return HDR(a)->unpacked;
}

/**
 * @brief STCStart(address): where the data of an S404 file unpacked in
 * place begin (address + security offset).
 *
 * The header check is only in the binary (not in the source).
 * @param a  d0: the file
 * @return d0: address + offset, 0 if a is no S404 file
 */
static ULONG BLITZ_ROUTINE stcstart_c(const UBYTE *a BL_REG(d0))
{
    if (HDR(a)->id != S404)
        return 0;
    return (ULONG)a + HDR(a)->offset;
}

/**
 * @brief STCSafeLen(address): the buffer size an S404 file needs to be
 * unpacked in place (unpacked length + security offset).
 *
 * The header check is only in the binary (not in the source).
 * @param a  d0: the file
 * @return d0: the size, 0 if a is no S404 file
 */
static ULONG BLITZ_ROUTINE stcsafelen_c(const UBYTE *a BL_REG(d0))
{
    if (HDR(a)->id != S404)
        return 0;
    return HDR(a)->unpacked + HDR(a)->offset;
}

/**
 * @brief STCLoad filename$,address: reads an S404 file to address (its
 * whole length) and unpacks it in place.
 *
 * Examine's and Read's results are not checked.  BUG(orig): the lock that
 * OpenFromLock took over is unlocked again after Close (#835); the header
 * is not checked and the whole file is read whatever the buffer's size
 * (#836); the decruncher does not check the data (#833).
 * @param name  d0: filename$
 * @param ad    d1: the buffer (STCSafeLen bytes)
 * @return d0: the start of the unpacked data (address + offset), 0 if the
 *         file cannot be locked or opened or there is no memory for the
 *         FileInfoBlock
 */
static ULONG BLITZ_ROUTINE stcload_c(CONST_STRPTR name BL_REG(d0), UBYTE *ad BL_REG(d1))
{
    struct Library *dos = dosbase;
    struct FileInfoBlock *fib;
    BPTR lock, fh;
    LONG size;

    lock = Lock(name, ACCESS_READ);     /* MOVEQ #-2,d2 */
    if (!lock)
        return 0;
    fib = AllocMem(FIBSIZE, MEMF_CLEAR);
    if (!fib) {
        UnLock(lock);
        return 0;
    }
    Examine(lock, fib);                 /* not checked */
    size = fib->fib_Size;
    fh = OpenFromLock(lock);
    if (!fh) {
        FreeMem(fib, FIBSIZE);
        UnLock(lock);
        return 0;
    }
    Read(fh, ad, size);                 /* not checked */
    Close(fh);
    FreeMem(fib, FIBSIZE);              /* BSR oopsfreefib */
    UnLock(lock);                       /* BUG(orig): OpenFromLock took it (#835) */
    stcdecrunch(ad);                    /* BUG(orig): no header check (#836) */
    return (ULONG)ad + HDR(ad)->offset;
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
