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
 *                             if source is no S404 file or its data are
 *                             corrupt
 *   STCDecrunch(source,dest)  unpacks to dest (no offset); returns 1 (d0
 *                             of the decruncher), 0 if no S404 file or
 *                             corrupt data
 *
 * init and finit are empty (!nullsub{0,0,0}).  The entries and the
 * decruncher are assembler (speed: perf bstc; optidec.h).
 *
 * Fixed (#833): corrupt data no longer write below the destination; the
 *   original's decruncher wrote a match or literal run past its start.
 *   The checks cost nothing per token on the main path (optidec.h); such
 *   data give 0.  Reads are not checked (a known limit, optidec.h).
 * Form 2 does not check that dest is large enough: no defect, the caller
 *   gives a dest for the unpacked length (the header has it).
 * DIFF: the decruncher's run table index (optidec.h).
 */
#include "blitzlib.h"

/* entry points (assembler, below) */
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

/** init and finit: no code (!nullsub{0,0,0}). */
BLITZ_NULLSUB_AT(init, 0, 0, 0);
BLITZ_NULLSUB_AT(finit, 0, 0, 0);

BLITZ_LIBFIN();

/* the decruncher: sd_dec (OPTIDEC_ASM) */
#include "optidec.h"

/*
 * _stcdecrunch, STCDecrunch(source): unpack S404 data in place, to source
 * + the security offset (the long at source + 4); returns d0 = that
 * address (a0 at the end of the decruncher), 0 if source is no S404 file
 * or its data are corrupt.
 * _stcdecrunch2, STCDecrunch(source,dest): unpack to dest (no offset;
 * dest is not checked: the caller gives one for the unpacked length of
 * the header, no defect); returns d0 = 1 (d0 of the decruncher, MOVEQ
 * #1,d0), 0 for no S404 file or corrupt data.  It runs on into sd_dec.
 * In: d0 the S404 file in memory, d1 (form 2) the destination.
 * Corrupt data are refused and never written below the destination
 * (fixed #833: the original wrote a match or literal run past its start).
 * Registers: the original's entries change d0-d7/a0-a1 and keep a2-a6
 * (MOVEM.l a2-a6 around the decruncher); so do these (sd_dec saves
 * a2-a6).  Assembler for speed (perf bstc, optidec.h).
 */
BLITZ_ASM_BLOCK(stcdecrunch,
"   .globl  __stcdecrunch, __stcdecrunch2\n"
"__stcdecrunch:\n"
"   move.l  d0,a1\n"
"   cmp.l   #0x53343034,(a1)\n"         /* "S404" */
"   bne.s   sd_oops\n"
"   move.l  a1,a0\n"
"   add.l   4(a1),a0\n"                 /* + the security offset */
"   bsr     sd_dec\n"
"   tst.l   d0\n"
"   beq.s   sd_oops\n"
"   move.l  a0,d0\n"
"   rts\n"
"sd_oops:\n"
"   moveq   #0,d0\n"
"   rts\n"
"__stcdecrunch2:\n"
"   move.l  d0,a1\n"
"   cmp.l   #0x53343034,(a1)\n"
"   bne.s   sd_oops\n"
"   move.l  d1,a0\n"
OPTIDEC_ASM);
