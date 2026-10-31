/*
 * exitslib - system library 65035 ($C800): `End` - leave the program from
 * anywhere (token endjmp, blitz2.s equate 162).
 *
 * Translation of bblibsrc/exitslib.  Both routines exist only to save and
 * restore the stack pointer (a non-local exit like longjmp), which C
 * cannot express; they stay assembler:
 *   codeinit: called from the program start code with d0 = address of the
 *             cleanup code; remembers d0 and a7 and returns `quit` as init
 *             value (the global used by `End`).
 *   quit:     restores a7 (+8: the start code's return address and the
 *             init call's own), then jumps to the cleanup code.
 */
#include "blitzlib.h"

/* entry points (asm, defined below) */
void quit(void);
void codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !syslibheader{#exitslib,$c800,$c801,init,1,0,0} */
BLITZ_SYSLIBHEADER7(LIB_exitslib, 0xc800, 0xc801, init, BL_LONGRET, 0, 0);
BLITZ_NULLSUB(quit, 0, 0);                      /* $C800 endjmp */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);         /* init */
BLITZ_LIBFIN();

__asm__(
"   .text\n"
"   .even\n"
"cleanup: .long 0\n"
"stack:   .long 0\n"
);

/**
 * @brief init: remember the cleanup code and the stack pointer.
 *
 * Called from the program start code.  Assembler (it saves sp).
 *
 * @param d0  the address of the cleanup code
 * @return d0: the address of quit, the init value used by `End`
 */
BLITZ_ASM_ROUTINE(codeinit,
"   move.l  d0,cleanup\n"
"   move.l  sp,stack\n"
"   move.l  #_quit,d0\n"
"   rts");

/**
 * @brief End ($C800 endjmp): leave the program from anywhere.
 *
 * Assembler (a non-local exit like longjmp): restores the stack pointer
 * saved by init (+8: the start code's return address and the init call's
 * own) and jumps to the cleanup code.  Does not return.
 */
BLITZ_ASM_ROUTINE(quit,
"   move.l  stack(pc),sp\n"
"   addq.w  #8,sp\n"
"   move.l  cleanup(pc),a0\n"
"   jmp     (a0)");
