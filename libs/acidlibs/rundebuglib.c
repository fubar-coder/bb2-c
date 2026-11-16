/*
 * rundebuglib - system library 64050 (no tokens): minimal runtime error
 * handling of executables that use SetErr (blitz2.s doseterr: used
 * instead of debuglib/dhandlerlib when an executable is created).
 *
 * Translation of bblibsrc/rundebuglib ("shit knows"; the binary matches
 * the source).  init (codeinit) opens intuition, remembers a5 and installs
 * the trap handler; finit (codefinit) closes intuition (runerrlib's finit
 * removes the handler).
 * Fixed (#1069): the handler is the task's (tc_TrapCode, `tasktrap`), and
 * in a SetInt routine runerrlib's gosuper/gouser lead the CPU's vectors
 * to it while the routine runs; the original replaced the CPU's vectors of
 * TRAP #0..#5 (VBR from runerrlib's getvbr, $C31D) for the whole machine
 * and put the old ones back at its end.  The handlers:
 *   TRAP #0  runtime error, d0 = message (or the codes 1-3 of blitz2's
 *            code): in user mode continue (RTE) at printerr, which shows
 *            the message in an AutoRequest and ends the program (exitslib
 *            endjmp $C800).  In supervisor mode (a SetInt routine)
 *            restore the vectors and give the trap with the message to
 *            the old vector (fixed (#246): the original called codefinit
 *            there, which changed d0 and closed intuition in the
 *            supervisor context; intuition is now closed by finit only).
 *   TRAP #1  skip the 8 bytes after the TRAP (unless ctrlc is set - it
 *            never is: BUG(orig) (#43), ctrlc is only cleared)
 *   TRAP #2  skip 6 bytes (and remember the routine, #1069)
 *   TRAP #3  nothing (forget it)
 *   TRAP #4  skip 8 bytes
 *   TRAP #5  nothing
 * (the debugger's checks that the compiler emits with debug code on).
 *
 * C: codeinit, codefinit, printerr (normal user mode code that the
 * TRAP #0 handler continues at, never returns).
 * Assembler: the trap handlers (exception frames, RTE) and more2.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <intuition/intuition.h>
#define __NOLIBBASE__
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (C, defined below) */
static void BLITZ_ROUTINE codeinit(void);
static void BLITZ_ROUTINE codefinit(void);
void BLITZ_ROUTINE printerr(ULONG msg BL_REG(d0));
void tasktrap(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#rundebuglib,0,0,init,0,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_rundebuglib, 0, 0, init, BL_NORET, finit, 0);
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

static const char intname[] = "intuition.library";

APTR oldtc;                             /* the task's old tc_TrapCode */
UBYTE *olda5;
struct IntuitionBase *myint;
UWORD ctrlc;                            /* read by mytrap1 */
/* fixed (#1069): per SetInt routine running (nested ones of different
   levels) the stack pointer at its TRAP #2 and the TRAP #2's parameter */
ULONG intstack[2 * 8];
ULONG *intsp;

/* fixed (#6): the texts of the numeric codes 1-3, as in debuglib */
static UBYTE maxerrtext[] = "No currently used object!";
static UBYTE braerrtext[] = "Offset out of range";
static UBYTE eoptext[] = "End of Program";
static UBYTE text2[] = "OKAY!";
static UBYTE text3[] = "OKAY!";
/* the original's t1: the message (tadd = t1's IText); t2/t3: the two
   "OKAY!" gadgets, AutoRequest's positive and negative text */
static struct IntuiText bodytext = { 1, 0, 0, 0, 8, NULL, NULL, NULL };
static struct IntuiText postext = { 1, 0, 0, 0, 0, NULL, text2, NULL };
static struct IntuiText negtext = { 1, 0, 0, 0, 0, NULL, text3, NULL };

/* Fixed (#1069): the traps come through the task's trap handler
   (tc_TrapCode, set by codeinit): exec hands it a trap of user mode with
   the trap number as a long on the stack, in front of the exception frame.
   tasktrap takes TRAP #0-#5 (the number off the stack, then the handler
   as if the trap had come through its vector) and gives every other
   number to the handler it found (runerrlib's trapper).  In a SetInt
   routine (supervisor mode, where exec does not call tc_TrapCode) the
   CPU's vectors lead here through runerrlib's gosuper, with the trap
   number pushed the same way.  The original put mytrap0..mytrap5 into the
   CPU's vector table, which belongs to the whole machine: a second program
   with runtime error checking took the vectors over, and the one that
   ended first put back what it had found while the other still ran. */
BLITZ_ASM_BLOCK(traps,
"   .globl  _tasktrap\n"
"_tasktrap:\n"
"   cmp.l   #33,(sp)\n"                 /* TRAP #1 first: every statement */
"   beq.s   t1\n"
"   cmp.l   #32,(sp)\n"
"   beq.s   t0\n"
"   cmp.l   #34,(sp)\n"
"   beq.s   t2\n"
"   cmp.l   #35,(sp)\n"
"   beq.s   t3\n"
"   cmp.l   #36,(sp)\n"
"   beq.s   t4\n"
"   cmp.l   #37,(sp)\n"
"   beq.s   t5\n"
"   move.l  _oldtc,-(sp)\n"             /* not ours: the old handler */
"   rts\n"
"t0: addq.l #4,sp\n"
"   bra.s   _mytrap0\n"
"t1: addq.l #4,sp\n"
"   bra.s   _mytrap1\n"
"t2: addq.l #4,sp\n"
"   bra.s   _mytrap2\n"
"t3: addq.l #4,sp\n"
"   bra.s   _mytrap3\n"
"t4: addq.l #4,sp\n"
"   bra.s   _mytrap4\n"
"t5: addq.l #4,sp\n"
"   bra.s   _mytrap5\n"
/* the trap handlers: exception frames (SR, PC at 2(sp)) and RTE */
"   .globl  _mytrap0, _mytrap1, _mytrap2, _mytrap3, _mytrap4, _mytrap5\n"
"_mytrap4:\n"
"   addq.l  #8,2(sp)\n"
"_mytrap5:\n"
"   rte\n"
/* TRAP #2, the start of a SetInt routine: skip its 6 bytes (the address
   of its end, a word) - and, fixed (#1069), remember the stack pointer
   and that address on intstack for more2; TRAP #3 drops them.  Changes
   a0/a1, as debuglib's TRAP #2 (the original: nothing).  The slot is
   reserved before it is written: a nested routine of the same program
   uses the next one */
"_mytrap2:\n"
"   move.l  2(sp),a0\n"
"   move.l  #1f,2(sp)\n"
"   rte\n"
"1: move.l  _intsp,a1\n"
"   addq.l  #8,_intsp\n"
"   move.l  a7,(a1)+\n"
"   move.l  a0,(a1)\n"
"   addq.w  #6,a0\n"
"   jmp     (a0)\n"
"_mytrap3:\n"
"   subq.l  #8,_intsp\n"
"   rte\n"
"_mytrap1:\n"
"   tst.w   _ctrlc\n"
"   bne.s   doctrlc\n"
"   addq.l  #8,2(sp)\n"
"   rte\n"
"doctrlc:\n"
"   rte\n"
"_mytrap0:\n"
"   btst    #5,(sp)\n"
"   bne.s   super_trap\n"
"   move.l  #_printerr,2(sp)\n"         /* user mode: continue at printerr */
"   rte\n"
"super_trap:\n"                         /* can't do nothing about this! */
"   move.l  #more2,2(sp)\n"
"   rte\n"
/* A runtime error in a SetInt routine (supervisor mode): the CPU's
   vectors back as they were before the routine, and the trap to the old
   TRAP #0 handler with d0, the message (fixed (#246): the original called
   codefinit here, which changed d0 and closed intuition in the supervisor
   context).  Fixed (#1069): the vectors are the ones runerrlib's gosuper
   saved at the routine's start (6 longs at the stack pointer of its
   TRAP #2, intstack), not the ones of the program's start; if the old
   handler returns, the routine is left through its end (gouser puts the
   same vectors back).  The original fell through behind the trap.
   BUG(orig): the old vector is exec's in practice, and exec stops the
   machine at a trap of supervisor mode: a runtime error in a SetInt
   routine of an executable is never reported, the machine hangs (test
   rundebuglib-setint-runtime-error).  The debugger leaves the routine
   and reports the error at the next statement (debuglib mytrap0) */
"more2:\n"
"   move.l  _intsp,a1\n"
"   move.l  -(a1),a0\n"                 /* the parameter of TRAP #2 */
"   move.l  -(a1),a7\n"                 /* the stack at TRAP #2: the vectors */
"   subq.l  #8,_intsp\n"
"   move.l  a0,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _blitz_tok+0x80000000+" BL_STR(TOK_getvbr) "\n"   /* a0 = VBR */
"   lea     128(a0),a0\n"
"   lea     8(sp),a1\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1),(a0)\n"
"   move.l  (sp)+,d0\n"
"   trap    #0\n"
"   move.l  (sp)+,a0\n"
"   move.l  (a0),a0\n"                  /* the end of the routine */
"   jmp     (a0)\n"
);

/**
 * @brief printerr - show a runtime error and end the program ("user
 * mode...OK to generate error").
 *
 * The TRAP #0 handler continues here in user mode.  a5 is set back to
 * the globals of the init, the message is shown in an AutoRequest
 * (64,64,640,64, both gadgets "OKAY!") and the program ends through
 * exitslib's endjmp ($C800).  Never returns.
 *
 * Fixed (intended deviation, #6): the numeric codes 1-3 that blitz2's
 * code raises get the texts of debuglib (1 "No currently used object!":
 * the used object test; 2 "Offset out of range": the bounds check of an
 * array field of a NewType; 3 "End of Program": blitz2 appends moveq
 * #3,d0:trap #0 (eopcode) to a program compiled with runtime errors on,
 * End jumps behind it).  The original took d0 as the message pointer as
 * it is: the requester of these codes had an empty text.
 *
 * @param msg d0: the message (C string) of the runtime error, or one of
 *            the codes 1-3
 */
void BLITZ_ROUTINE printerr(ULONG msg BL_REG(d0))
{
    struct bl_regs r = { 0, 0, 0, 0 };

    blitz_globals = olda5;
    if (msg == 1)                       /* fixed (#6): debuglib's texts */
        msg = (ULONG)maxerrtext;
    else if (msg == 2)
        msg = (ULONG)braerrtext;
    else if (msg == 3)
        msg = (ULONG)eoptext;
    bodytext.IText = (UBYTE *)msg;      /* MOVE.l d0,tadd */
#define IntuitionBase myint
    AutoRequest(NULL, &bodytext, &postext, &negtext, 64, 64, 640, 64);
#undef IntuitionBase
    BLITZ_ALIBJSR(TOK_endjmp, r);       /* end! */
    __builtin_unreachable();
}

/**
 * @brief Library init: open intuition, remember a5 and install the trap
 * handler.
 *
 * Fixed (#1069): `tasktrap` becomes the task's trap handler
 * (tc_TrapCode; the old one is kept in oldtc), the CPU's vector table is
 * not touched.  The original saved the vectors of TRAP #0..#5 (VBR from
 * runerrlib's getvbr, $C31D) and put mytrap0..mytrap5 there.  ctrlc is
 * cleared.  OldOpenLibrary is not checked.
 */
static void BLITZ_ROUTINE codeinit(void)
{
    struct Task *me = SysBase->ThisTask;

    ctrlc = 0;
    myint = (struct IntuitionBase *)OldOpenLibrary((STRPTR)intname);
    olda5 = blitz_globals;
    intsp = intstack;
    oldtc = me->tc_TrapCode;
    me->tc_TrapCode = tasktrap;
}

/**
 * @brief Library finit: close intuition.
 *
 * The trap handler stays the task's: a SetInt routine can still run until
 * intlib's finit, which comes later (finits go from the low library
 * numbers up), and runerrlib's gosuper hands its traps to the task's
 * handler.  runerrlib's finit, the last of them, puts back the handler the
 * program started with.  (The original put its saved CPU vectors back
 * here, #1069.)
 */
static void BLITZ_ROUTINE codefinit(void)
{
    CloseLibrary((struct Library *)myint);
}
