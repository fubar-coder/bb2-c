/*
 * errtraplib - system library 64040 ($D200-$D205): runtime error trapping
 * of executables, `SetErr ... End SetErr`, `ClrErr`, `Ern`, `ErrFail`
 * (tokens ern, seterr, clrerr, errfail; blitz2.s equates 244-249,
 * getsp/putsp are commented out there).
 *
 * Translation of bblibsrc/errtraplib (the binary matches the source).
 * Runtime errors are raised with `MOVE.l #message,d0:TRAP #0` (d1 is not
 * set, see newtrap).  codeinit replaces the TRAP #0 vector (VBR+128) by newtrap:
 * if SetErr installed a handler (dosettrap: d0 = handler code, the stack
 * pointer of the caller is remembered), a user mode trap continues at the
 * handler with that stack; otherwise the old vector takes the trap.
 *
 * C: codeinit (VBR via SuperState/MOVEC/UserState), codefinit, dofail,
 * doreadern, doclrerr.
 * Assembler: the data block (pc-relative from the trap code), newtrap /
 * ercont (exception frame, RTE, stack switch), dogetsp / doputsp /
 * dosettrap (they read or set the caller's stack pointer).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks / C / asm, defined below) */
void doreadern(void), dosettrap(void), doclrerr(void);
void dogetsp(void), doputsp(void), dofail(void);
void BLITZ_ROUTINE codeinit(void);
void codefinit(void);
void newtrap(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#errtraplib,$d200,$d206,init,0,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_errtraplib, 0xd200, 0xd206, init, BL_NORET, finit, 0);
BLITZ_NULLSUB(doreadern, 0, 0);                 /* $D200 ern */
BLITZ_NULLSUB(dosettrap, 0, 0);                 /* $D201 seterr */
BLITZ_NULLSUB(doclrerr, 0, 0);                  /* $D202 clrerr */
BLITZ_NULLSUB(dogetsp, 0, 0);                   /* $D203 (getsp) */
BLITZ_NULLSUB(doputsp, 0, 0);                   /* $D204 (putsp) */
BLITZ_NULLSUB(dofail, 0, 0);                    /* $D205 errfail */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/* data: in the code hunk, read pc-relative by the trap code */
__asm__(
"   .text\n"
"   .even\n"
"_oldtrap: .long 0\n"                   /* old TRAP #0 vector */
"_ernum:   .word 0\n"                   /* Ern: d1 at the last trap */
"_ertext:  .long 0\n"                   /* last error message (d0) */
"_ercode:  .long 0\n"                   /* SetErr handler, 0 = none */
"_erstack: .long 0\n"                   /* stack pointer for the handler */
"_vbr_:    .long 0\n"
"   .globl  _oldtrap, _ernum, _ertext, _ercode, _erstack, _vbr_\n"
);
extern ULONG oldtrap, ertext, ercode, erstack;
extern UWORD ernum;
extern ULONG *vbr_;

#define TRAP0 (128 / 4)                 /* vector 32 */

/* dofail - ErrFail: if an error happened, give it to the old TRAP #0
   (the debugger / runtime error handler) after all.  Keeps d1/a0-a1
   (a0 only if there was no error; the trap normally ends the program). */
static ULONG BLITZ_ROUTINE dofail_c(ULONG num BL_REG(d1))
{
    ULONG text = ertext;

    if (text) {
        vbr_[TRAP0] = oldtrap;
        {
            register ULONG d0 __asm("d0") = text;
            register ULONG d1 __asm("d1") = num;
            __asm volatile("trap #0" : "+d"(d0), "+d"(d1) : : "memory");
        }
    }
    return text;
}
BLITZ_THUNK(dofail, dofail_c, "d1/a0-a1");

/* dogetsp - return stack pointer as a long in d0 (a0 too) */
BLITZ_ASM_ROUTINE(dogetsp,
"   lea     4(sp),a0\n"
"   move.l  a0,d0\n"
"   rts");

/* doputsp - set stack pointer to d0 */
BLITZ_ASM_ROUTINE(doputsp,
"   move.l  (sp),a0\n"                  /* return address */
"   move.l  d0,sp\n"
"   jmp     (a0)");

/* codeinit - find the VBR (68020+), install newtrap */
void BLITZ_ROUTINE codeinit(void)
{
    UWORD f = SysBase->AttnFlags & 15;
    UWORD n = 0;
    ULONG vbr = 0;

    /* BUG(orig): this counts the shifts until the CPU flags are 0, so
       only 68020+ (2 flags or more) read the VBR; a 68010 uses 0. */
    while (f) {
        n++;
        f >>= 1;
    }
    if (n >= 2) {
        APTR ssp = SuperState();                /* supervisor state! */
        register ULONG v __asm("d2");
        __asm volatile(".short 0x4e7a,0x2801" : "=d"(v));  /* MOVEC vbr,d2 */
        vbr = v;
        UserState(ssp);
    }
    vbr_ = (ULONG *)vbr;
    oldtrap = vbr_[TRAP0];
    vbr_[TRAP0] = (ULONG)newtrap;
    ernum = 0;
    ercode = 0;
}

/* codefinit - restore the old TRAP #0; changes only a0 in the original */
static void BLITZ_ROUTINE codefinit_c(void)
{
    vbr_[TRAP0] = oldtrap;
}
BLITZ_THUNK(codefinit, codefinit_c, "d0-d1/a0-a1");

/* dosettrap - d0 = code to jump to on error; the stack pointer of the
   caller (after the return) is remembered: asm, it reads sp */
BLITZ_ASM_ROUTINE(dosettrap,
"   move.l  d0,_ercode\n"
"   lea     4(sp),a0\n"
"   move.l  a0,_erstack\n"
"   rts");

/* doreadern - read last generated ern into d0 */
static ULONG BLITZ_ROUTINE doreadern_c(void)
{
    return ernum;
}
BLITZ_THUNK(doreadern, doreadern_c, "d1/a0-a1");

/* doclrerr - no SetErr handler any more; keeps all registers */
static void BLITZ_ROUTINE doclrerr_c(void)
{
    ercode = 0;
}
BLITZ_THUNK(doclrerr, doclrerr_c, "d0-d1/a0-a1");

/* newtrap - new TRAP #0 routine: exception handler (frame, RTE) and a
   stack switch, asm.  A user mode trap with a SetErr handler returns (RTE)
   to ercont, which continues at the handler with the SetErr stack.
   BUG(orig): d1 is stored in ernum (Ern) before ercode is checked, but no
   runtime error sets d1 (compiler moveq #n,d0:trap #0, libraries
   MOVE.l #mess,d0:TRAP #0), so Ern returns a random value.  The old
   vector gets the trap with d1 = 0 and a0 changed; the old handlers
   (debuglib/rundebuglib mytrap0) only read d0, so that has no effect. */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _newtrap\n"
"_newtrap:\n"
"   btst    #5,(sp)\n"                  /* can't trap errors on interupts */
"   bne.s   insuper\n"
"   move.w  d1,_ernum\n"
"   move.l  _ercode(pc),d1\n"
"   beq.s   insuper\n"
"   move.l  d0,_ertext\n"
"   move.l  #ercont,2(sp)\n"
"   rte\n"
"ercont:\n"
"   move.l  _ercode(pc),a0\n"
"   move.l  _erstack(pc),sp\n"
"   jmp     (a0)\n"
"insuper:\n"                            /* we can't do anything, let's jump to */
"   move.l  _oldtrap(pc),a0\n"          /* the old TRAP #0 routine..... */
"   jmp     (a0)\n"
);
