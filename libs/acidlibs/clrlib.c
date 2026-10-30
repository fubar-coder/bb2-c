/*
 * clrlib - system library 64635 ($CC00): clear the local variables of a
 * procedure (token clrloc, blitz2.s equate 180).
 *
 * C translation of bblibsrc/clrlib.  Called right after the procedure has
 * reserved its locals on the stack: everything from the caller's stack
 * pointer up to a4 (the locals base) is cleared, longword by longword.
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void clrloc(void);

/* !syslibheader{#clrlib,$cc00,$cc01,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_clrlib, 0xcc00, 0xcc01, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(clrloc, 0, 0);                    /* $CC00 clrloc */
BLITZ_LIBFIN();

/* clrloc - `p` = the caller's stack pointer (the thunk computes it:
   LEA 8+4(a7),a0 after saving d0-d1).  Count = (a4-p)/4 longs, run as
   DBF loop: a4 == p would clear 65536 longs, but the compiler inserts
   clrloc only with a link size of 4..32768 (blitz2.s:13640, 14041), so
   that case does not occur in compiled code (#109). */
static void BLITZ_ROUTINE clrloc_c(ULONG *p BL_REG(a0))
{
    UWORD n = (UWORD)((((ULONG)blitz_locals - (ULONG)p) >> 2) - 1);

    do
        *p++ = 0;
    while (n-- != 0);
}
BLITZ_THUNK_PRE(clrloc, clrloc_c, "d0-d1", "lea 8+4(sp),a0");
