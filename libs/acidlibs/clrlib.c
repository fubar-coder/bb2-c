/*
 * clrlib - system library 64635 ($CC00): clear the local variables of a
 * procedure (token clrloc, blitz2.s equate 180).
 *
 * C translation of bblibsrc/clrlib.  Called right after the procedure has
 * reserved its locals on the stack: everything from the caller's stack
 * pointer up to a4 (the locals base) is cleared, longword by longword.
 */
#include "blitzlib.h"

/* entry points (assembler, defined below) */
void clrloc(void);

/* !syslibheader{#clrlib,$cc00,$cc01,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_clrlib, 0xcc00, 0xcc01, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(clrloc, 0, 0);                    /* $CC00 clrloc */
BLITZ_LIBFIN();

/**
 * @brief clrloc ($CC00): clear the local variables of a procedure.
 *
 * Called by compiled code right after a procedure has reserved its locals
 * on the stack (LINK a4,#-n): clears everything from the caller's stack
 * pointer up to a4 (the locals base), longword by longword.  The
 * compiler inserts clrloc only with a link size of 4..32768, a multiple
 * of 4 (blitz2.s:13640, 14041), so there is at least one long and the
 * original's DBF count (a4-p)/4-1 fits in a word (a4 == p, which would
 * clear 65536 longs there, does not occur in compiled code, #109).
 *
 * Keeps d0-d7/a2-a6: the compiler calls it in the procedure prologue
 * with the parameters in d0-d5 (perf-analysis thunks.md 1.4); a0 and a1
 * are free, no third-party library calls it.  An assembler entry that
 * uses a0/a1 only (C cannot keep d0/d1 without a thunk): MOVE.l a1,(a0)+
 * up to a4, 28 cycles per long and no save; the original saves d0-d1
 * for a DBF loop (22 per long, 60 more for the saves and the count),
 * which is faster only from 11 longs of locals on.
 */
BLITZ_ASM_ROUTINE(clrloc,
        "   lea     4(sp),a0\n"             /* the caller's stack pointer */
        "   suba.l  a1,a1\n"
        "1: move.l  a1,(a0)+\n"
        "   cmpa.l  a4,a0\n"
        "   bcs.s   1b\n"
        "   rts");
