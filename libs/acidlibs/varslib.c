/*
 * varslib - system library 65534: memory for the global variables.
 *
 * C translation of bblibsrc/varslib.  The program start code calls init
 * with d0 = size of the globals; a5 is then set to the middle of the block
 * (+$8000), so the globals are addressed with signed 16 bit offsets
 * -32768..32767 from a5.  finit frees the block again (d0 = size).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks / asm, defined below) */
void varslib_codeinit(void);
static void BLITZ_ROUTINE codefinit(ULONG size BL_REG(d0));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#varslib,0,0,init,1,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_varslib, 0, 0, init, BL_LONGRET, finit, 0);
BLITZ_NULLSUB_AT(init, varslib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/**
 * @brief init - allocate the global variables.
 *
 * Allocates the block (MEMF_PUBLIC | MEMF_CLEAR) and sets a5
 * (blitz_globals) to block+$8000, so the globals are addressed with
 * signed 16 bit offsets.
 *
 * @param size d0: the size of the globals
 * @return d0: the new a5, 0 if there is no memory (the entry codeinit then
 *         ends the program)
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG size BL_REG(d0))
{
    UBYTE *mem = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR);

    if (!mem)
        return 0;
    blitz_globals = mem + 0x8000;       /* SUB #$8000,a5 (word: -(-32768)) */
    return (ULONG)blitz_globals;
}

/**
 * @brief init entry (thunk of codeinit_c).
 *
 * a0 is preserved.  Without memory the original drops 12 bytes (saved a0,
 * its own return address and the start code's return address) and so
 * returns from the program start - the program ends.
 */
BLITZ_ASM_ROUTINE(varslib_codeinit,
"   move.l  a0,-(sp)\n"
"   jsr     _codeinit_c\n"
"   tst.l   d0\n"
"   beq.s   1f\n"
"   move.l  (sp)+,a0\n"
"   rts\n"
"1: lea     12(sp),sp\n"                /* 'fuct: sever crap out */
"   rts");

/**
 * @brief finit - free the global variables.
 *
 * Frees a5-$8000: ADD #$8000,a1 adds the sign extended word -32768.
 *
 * @param size d0: the size of the globals, as given to codeinit
 */
static void BLITZ_ROUTINE codefinit(ULONG size BL_REG(d0))
{
    FreeMem(blitz_globals - 0x8000, size);
}
