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
void codeinit(void);
void BLITZ_ROUTINE codefinit(ULONG size BL_REG(d0));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#varslib,0,0,init,1,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_varslib, 0, 0, init, BL_LONGRET, finit, 0);
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/* codeinit - allocate the globals, set a5 (blitz_globals) to block+$8000
   and return it; 0 if there is no memory */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG size BL_REG(d0))
{
    UBYTE *mem = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR);

    if (!mem)
        return 0;
    blitz_globals = mem + 0x8000;       /* SUB #$8000,a5 (word: -(-32768)) */
    return (ULONG)blitz_globals;
}

/* entry: a0 is preserved.  Without memory the original drops 12 bytes
   (saved a0, its own return address and the start code's return address)
   and so returns from the program start - the program ends. */
BLITZ_ASM_ROUTINE(codeinit,
"   move.l  a0,-(sp)\n"
"   jsr     _codeinit_c\n"
"   tst.l   d0\n"
"   beq.s   1f\n"
"   move.l  (sp)+,a0\n"
"   rts\n"
"1: lea     12(sp),sp\n"                /* 'fuct: sever crap out */
"   rts");

/* codefinit - free the globals (d0 = size, as given to codeinit).
   a5-$8000: ADD #$8000,a1 adds the sign extended word -32768. */
void BLITZ_ROUTINE codefinit(ULONG size BL_REG(d0))
{
    FreeMem(blitz_globals - 0x8000, size);
}
