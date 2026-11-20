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
 *                  cannot be locked, examined, opened or read whole, the
 *                  FileInfoBlock not allocated, or the file is no S404
 *                  file whose packed data and trailer it holds)
 *
 * The decruncher (optidec.bin, third party code of StoneCracker 4.04) is
 * its source optidec.S in assembler (sd_dec in optidec.h, which
 * ristcdecrunchlib.c shares; for speed, perf bstc) with the checks of
 * #833 - the unpacked data are the original's for every input it handles
 * (see DIFF); the rest is C.  _stcdecrunch2 of the source (unpack to another
 * address) is not in the token table and not called: left out.
 *
 * STCLoad reads the whole file whatever the buffer's size: the command
 *   has no size argument (the caller gives a buffer of STCSafeLen bytes and
 *   at least the file's size).
 *
 * Fixed:
 *   - Corrupt data no longer write below the buffer: the original's
 *     decruncher wrote a match or literal run past its start (#833).  The
 *     checks cost nothing per token on the main path (optidec.h); STCLoad
 *     then returns 0.  Reads are not checked (a known limit, optidec.h).
 *   - finit closes dos.library; the original never closed it (finit 0)
 *     (#834).
 *   - STCLoad unlocks no lock a file handle owns.  The original opened
 *     the file with OpenFromLock, which takes the lock over, and unlocked
 *     it again after Close (oopsfreefib: FreeMem + UnLock), an UnLock of a
 *     lock the file system had already freed (#835).
 *   - STCLoad opens the file by its name (Open, MODE_OLDFILE) while it
 *     holds the lock, and unlocks the lock after that: one path on every
 *     Kickstart.  The original called OpenFromLock, a dos.library V36
 *     function, on the dos.library it opened in any version: on Kickstart
 *     1.x a crash (#1147).
 *   - STCLoad checks Examine, that Read read the whole file and the
 *     header ("S404", the packed data and the trailer within the file)
 *     before it unpacks, else 0.  The original checked none of it and
 *     unpacked whatever it had read with the offsets and lengths its words
 *     gave (#836).
 * DIFF: the literal run reads its shift count from a table indexed by the
 *   bit count d7 (MOVE.b table(pc,d7.w)); d7 leaves 0..15 only with a
 *   first bit count (the trailer's third word) above 16 (or more than 16
 *   offset bits, which the C build refuses, #833), and then the original
 *   reads whatever lies around the table (its own code) - the C build
 *   takes the index modulo 16.  With d7 = 0..15 the result is the same.
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
void ristonecrackerlib_codeinit(void), ristonecrackerlib_codefinit(void);

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

BLITZ_NULLSUB_AT(init, ristonecrackerlib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, ristonecrackerlib_codefinit, 0, 0);

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

/* the decruncher: sd_dec (assembler for speed, perf bstc; optidec.h) */
#include "optidec.h"
BLITZ_ASM_BLOCK(optidec, OPTIDEC_ASM);

/* _stcdecrunch: unpack the file at a in place; 1, or 0 for corrupt data
   (fixed #833; the original returned the unpacked length, read before,
   which STCLoad did not use) */
static int stcdecrunch(UBYTE *a)
{
    register ULONG d0 __asm("d0");
    register UBYTE *a0 __asm("a0") = a + HDR(a)->offset;
    register UBYTE *a1 __asm("a1") = a;

    __asm volatile("jsr     sd_dec(pc)"
                   : "=d" (d0), "+a" (a0), "+a" (a1)
                   :
                   : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
    return (int)d0;
}

/* ------------------------------------------------------------ commands */

/**
 * @brief init (codeinit): opens dos.library, any version (finit closes
 * it).
 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    dosbase = OpenLibrary((CONST_STRPTR)dosname, 0);
}

/**
 * @brief finit: closes dos.library if init opened it.
 *
 * Fixed (#834): the original's finit is 0, dos.library was never closed.
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    if (dosbase) {
        CloseLibrary(dosbase);
        dosbase = NULL;
    }
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
 * The file is opened by its name while the lock is held, and the lock is
 * unlocked after that (fixed #1147: the original opened it with
 * OpenFromLock, a dos V36 function, and crashed on Kickstart 1.x; it also
 * unlocked the lock OpenFromLock had taken over again after Close, fixed
 * #835).  Unpacked only if Examine
 * succeeded, Read read the whole file and it is an S404 file whose packed
 * data and trailer lie within it (fixed #836: the original checked none
 * of it).  The whole file is read whatever the buffer's size (the command
 * has no size argument).  Corrupt data give 0 and are not written below
 * the buffer (fixed #833).
 * @param name  d0: filename$
 * @param ad    d1: the buffer (STCSafeLen bytes, at least the file's size)
 * @return d0: the start of the unpacked data (address + offset), 0 if the
 *         file cannot be locked, examined, opened or read whole, there is
 *         no memory for the FileInfoBlock, or it is no S404 file whose
 *         packed data and trailer it holds, or its data are corrupt
 */
static ULONG BLITZ_ROUTINE stcload_c(CONST_STRPTR name BL_REG(d0), UBYTE *ad BL_REG(d1))
{
    struct Library *dos = dosbase;
    struct FileInfoBlock *fib;
    BPTR lock, fh;
    LONG size, got;

    lock = Lock(name, ACCESS_READ);     /* MOVEQ #-2,d2 */
    if (!lock)
        return 0;
    fib = AllocMem(FIBSIZE, MEMF_CLEAR);
    if (!fib) {
        UnLock(lock);
        return 0;
    }
    fh = 0;
    if (Examine(lock, fib))             /* fixed (#836): checked */
        fh = Open(name, MODE_OLDFILE);  /* fixed (#1147): not OpenFromLock */
    size = fib->fib_Size;
    FreeMem(fib, FIBSIZE);
    UnLock(lock);
    if (!fh)
        return 0;
    got = Read(fh, ad, size);
    Close(fh);
    /* fixed (#836): the whole file read, "S404", the packed data and the
       trailer (12 + packed length + 6 bytes) within it */
    if (got != size || size < (LONG)sizeof(struct s404hdr) + 2
        || HDR(ad)->id != S404 || HDR(ad)->packed > (ULONG)size - 18)
        return 0;
    if (!stcdecrunch(ad))               /* fixed (#833): corrupt data */
        return 0;
    return (ULONG)ad + HDR(ad)->offset;
}

/* Registers: the original changes d0/a0 (STCLength, STCStart,
   STCSafeLen), d0-d1/a0-a1/a6 (init), and d0-d7/a0-a1/a6 (STCLoad: the
   system calls and the decruncher); finit (only in the C build) keeps
   d2-d7/a2-a6 as init; the thunks keep d1/a0/a1, the C code d2-d7/a2-a6. */
BLITZ_THUNK(ristonecrackerlib_codeinit, codeinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(ristonecrackerlib_codefinit, codefinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_stclength, stclength_c, "d1/a0-a1");
BLITZ_THUNK(_stcstart, stcstart_c, "d1/a0-a1");
BLITZ_THUNK(_stcsafelen, stcsafelen_c, "d1/a0-a1");
BLITZ_THUNK(_stcload, stcload_c, "d1/a0-a1");
