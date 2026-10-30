/*
 * ristcdecrunchlib - RWE userlib 18 (Reflective Images): STCDecrunch,
 * unpacking StoneCracker 4.04 ("S404") data in memory.
 *
 * C translation of rwelibsrc/stcdecrunchlib.bb with the decruncher
 * optidec.bin it includes (optidec.h, shared with ristonecrackerlib.c);
 * reference: the reference build of the source (it is newer than the
 * BSS2 binary, docs/rwelibs/README.md 3.2: its second form checks the
 * header too and the first returns the address of the data, not the
 * length).  Notes: docs/rwelibs/port/ristcdecrunchlib.md.
 *
 *   STCDecrunch(source)       unpacks the S404 data at source in place,
 *                             to source + the security offset; returns
 *                             that address (a0 after the decruncher), 0
 *                             if source is no S404 file
 *   STCDecrunch(source,dest)  unpacks to dest (no offset); returns 1 (d0
 *                             of the decruncher), 0 if no S404 file
 *
 * init and finit are empty (!nullsub{0,0,0}).  No assembler besides the
 * thunks.
 *
 * BUG(orig): the decruncher does not check the data (optidec.h): corrupt
 *   data write below the destination and read below the packed words
 *   (#833).
 * Form 2 does not check that dest is large enough: no defect, the caller
 *   gives a dest for the unpacked length (the header has it).
 * DIFF: the decruncher's run table index (optidec.h).
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _stcdecrunch(void), _stcdecrunch2(void);

/* !libheader{#ristcdecrunchlib,init,0,finit,0} */
BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_LIBHEADER(LIB_ristcdecrunchlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stcdecrunch, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_stcdecrunch2, 0, 0);
BLITZ_NAME("STCDecrunch", "source[,dest]");

BLITZ_NULLSUB_AT(init, 0, 0, 0);
BLITZ_NULLSUB_AT(finit, 0, 0, 0);

BLITZ_LIBFIN();

#define S404 0x53343034UL

/* the decruncher: dostc */
#include "optidec.h"

/* STCDecrunch source: in place, to source + offset; a0 at the end */
static ULONG BLITZ_ROUTINE stcdecrunch_c(UBYTE *a BL_REG(d0))
{
    if (LONGAT(a) != S404)
        return 0;
    return (ULONG)dostc(a, a + LONGAT(a + 4));
}

/* STCDecrunch source,dest: d0 of the decruncher (MOVEQ #1,d0) */
static ULONG BLITZ_ROUTINE stcdecrunch2_c(const UBYTE *a BL_REG(d0), UBYTE *dest BL_REG(d1))
{
    if (LONGAT(a) != S404)
        return 0;
    dostc(a, dest);
    return 1;
}

/* Registers: the original changes d0-d7/a0-a1 (the decruncher; a0/a1 =
   source without an S404 header in form 1, a1 in form 2) and keeps
   a2-a6; the thunks keep d1/a0/a1, the C code d2-d7/a2-a6. */
BLITZ_THUNK(_stcdecrunch, stcdecrunch_c, "d1/a0-a1");
BLITZ_THUNK(_stcdecrunch2, stcdecrunch2_c, "d1/a0-a1");
