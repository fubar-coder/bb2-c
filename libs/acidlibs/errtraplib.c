/*
 * errtraplib - system library 64040 ($D200-$D205): runtime error trapping
 * of executables, `SetErr ... End SetErr`, `ClrErr`, `Ern`, `ErrFail`
 * (tokens ern, seterr, clrerr, errfail; blitz2.s equates 244-249,
 * getsp/putsp are commented out there).
 *
 * Translation of bblibsrc/errtraplib (the binary matches the source).
 * Runtime errors are raised with `MOVE.l #message,d0:TRAP #0` (d1 is not
 * set, see newtrap).  codeinit makes newtrap the task's trap handler
 * (tc_TrapCode; fixed (#1069): the original replaced the CPU's TRAP #0
 * vector, VBR+128, for the whole machine): if SetErr installed a handler
 * (dosettrap: d0 = handler code, the stack pointer of the caller is
 * remembered), a user mode trap continues at the handler with that stack;
 * otherwise the old handler takes the trap.
 *
 * C: codeinit, codefinit, dofail, doreadern, doclrerr.
 * Assembler: the data block (pc-relative from the trap code), newtrap /
 * ercont (exception frame, RTE, stack switch), dogetsp / doputsp /
 * dosettrap (they read or set the caller's stack pointer).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks / C / asm, defined below) */
void doreadern(void), dosettrap(void), doclrerr(void);
void dogetsp(void), doputsp(void), dofail(void);
static void BLITZ_ROUTINE codeinit(void);
void errtraplib_codefinit(void), newtrap(void);

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
BLITZ_NULLSUB_AT(finit, errtraplib_codefinit, 0, 0);
BLITZ_LIBFIN();

/* data: in the code hunk, read pc-relative by the trap code */
BLITZ_ASM_BLOCK(data,
"_oldtrap: .long 0\n"                   /* the task's old trap handler */
"_ernum:   .word 0\n"                   /* Ern: d1 at the last trap */
"_ertext:  .long 0\n"                   /* last error message (d0) */
"_ercode:  .long 0\n"                   /* SetErr handler, 0 = none */
"_erstack: .long 0\n"                   /* stack pointer for the handler */
"   .globl  _oldtrap, _ernum, _ertext, _ercode, _erstack\n"
);
extern APTR oldtrap;
extern ULONG ertext, ercode, erstack;
extern UWORD ernum;

/**
 * @brief ErrFail ($D205 errfail): report the trapped error after all.
 *
 * If an error happened (a message was stored by newtrap), the old trap
 * handler is put back (fixed (#1069): the task's, the original put back
 * the CPU's TRAP #0 vector) and the error is given to it with TRAP #0 (the
 * debugger / runtime error handler), which normally ends the program.
 * Keeps d1/a0-a1 (a0 only if there was no error).
 *
 * @param num  d1: passed on in d1 to the trap
 * @return d0: the stored error message, 0 if there was no error
 */
static ULONG BLITZ_ROUTINE dofail_c(ULONG num BL_REG(d1))
{
    ULONG text = ertext;

    if (text) {
        SysBase->ThisTask->tc_TrapCode = oldtrap;
        {
            register ULONG d0 __asm("d0") = text;
            register ULONG d1 __asm("d1") = num;
            __asm volatile("trap #0" : "+d"(d0), "+d"(d1) : : "memory");
        }
    }
    return text;
}
BLITZ_THUNK(dofail, dofail_c, "d1/a0-a1");

/**
 * @brief $D203 (getsp, commented out in blitz2.s): the caller's stack
 * pointer.
 *
 * Assembler (it reads sp): the stack pointer after the return.
 *
 * @par Result
 * d0: the stack pointer (a0 too)
 */
BLITZ_ASM_ROUTINE(dogetsp,
"   lea     4(sp),a0\n"
"   move.l  a0,d0\n"
"   rts");

/**
 * @brief $D204 (putsp, commented out in blitz2.s): set the stack pointer.
 *
 * Assembler: sets sp to d0 and returns through the return address taken
 * from the old stack.
 *
 * @arg d0  the new stack pointer
 */
BLITZ_ASM_ROUTINE(doputsp,
"   move.l  (sp),a0\n"                  /* return address */
"   move.l  d0,sp\n"
"   jmp     (a0)");

/* codeinit - install newtrap as the task's trap handler.  Fixed (#1069):
   the original read the VBR (68020+; #42) and put newtrap into the CPU's
   TRAP #0 vector */
static void BLITZ_ROUTINE codeinit(void)
{
    struct Task *me = SysBase->ThisTask;

    oldtrap = me->tc_TrapCode;
    me->tc_TrapCode = newtrap;
    ernum = 0;
    ercode = 0;
}

/* codefinit - restore the old trap handler; changes only a0 in the
   original */
static void BLITZ_ROUTINE codefinit_c(void)
{
    SysBase->ThisTask->tc_TrapCode = oldtrap;
}
BLITZ_THUNK(errtraplib_codefinit, codefinit_c, "d0-d1/a0-a1");

/**
 * @brief SetErr ($D201 seterr): install an error handler.
 *
 * Assembler (it reads sp): remembers the handler code and the stack
 * pointer of the caller (after the return).  A later runtime error
 * (TRAP #0 in user mode) continues at the handler with that stack.
 *
 * @arg d0  the code to jump to on an error
 */
BLITZ_ASM_ROUTINE(dosettrap,
"   move.l  d0,_ercode\n"
"   lea     4(sp),a0\n"
"   move.l  a0,_erstack\n"
"   rts");

/**
 * @brief Ern ($D200 ern): the number of the last trapped error.
 *
 * BUG(orig) (#133): newtrap stores d1 of the trap, but no runtime error
 * sets d1, so the value is random.
 *
 * @return d0: the stored word (d1 at the last trap)
 */
static ULONG BLITZ_ROUTINE doreadern_c(void)
{
    return ernum;
}
BLITZ_THUNK(doreadern, doreadern_c, "d1/a0-a1");

/**
 * @brief ClrErr ($D202 clrerr): remove the SetErr handler.
 *
 * Runtime errors go to the old TRAP #0 vector again.  Keeps all
 * registers.
 */
static void BLITZ_ROUTINE doclrerr_c(void)
{
    ercode = 0;
}
BLITZ_THUNK(doclrerr, doclrerr_c, "d0-d1/a0-a1");

/* newtrap - the task's trap handler (fixed (#1069), see codeinit): the
   trap number as a long on the stack in front of the exception frame
   (exec for a trap of user mode, runerrlib's gosuper in a SetInt
   routine).  Every number but 32 (TRAP #0) goes to the old handler as it
   came.  A user mode TRAP #0 with a SetErr handler returns (RTE) to
   ercont, which continues at the handler with the SetErr stack; any other
   goes to the old handler with its number.
   BUG(orig) (#133): d1 is stored in ernum (Ern) before ercode is checked, but no
   runtime error sets d1 (compiler moveq #n,d0:trap #0, libraries
   MOVE.l #mess,d0:TRAP #0), so Ern returns a random value.  The old
   handler gets the trap with d1 = 0; the old handlers (debuglib/
   rundebuglib mytrap0) only read d0, so that has no effect.  (The
   original jumped to the old TRAP #0 vector with a0 changed.) */
BLITZ_ASM_BLOCK(newtrap,
"   .globl  _newtrap\n"
"_newtrap:\n"
"   cmp.l   #32,(sp)\n"
"   bne.s   chain\n"
"   addq.l  #4,sp\n"
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
"   move.l  #32,-(sp)\n"                /* the old TRAP #0 routine..... */
"chain:\n"
"   move.l  _oldtrap(pc),-(sp)\n"
"   rts\n"
);
