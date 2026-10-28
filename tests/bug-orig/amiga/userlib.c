/*
 * userlib - a Blitz library for the bug-orig tests that load libraries
 * from blitz2:userlibs (the compiler's loadblitzlibs/loadalib): library
 * number 10 (no library of the CD or of this build uses it), one function
 * BugOrigUserLib that returns 42 (token $8501 = 10 * 128 + 1, listed in
 * userlib.tokens for tokenising the tests).  No relocations: the file
 * ends with HUNK_END behind the code.
 */
#include "blitzlib.h"

/* entry points (asm, defined below) */
void douserlib(void);

/* !libheader{10,0,0,0,0} */
BLITZ_LIBHEADER(10, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(douserlib, 0, 0);
BLITZ_NAME("BugOrigUserLib", "");

BLITZ_LIBFIN();

BLITZ_ASM_ROUTINE(douserlib,
"   moveq   #42,d0\n"
"   rts");
