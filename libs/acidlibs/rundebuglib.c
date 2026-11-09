/*
 * rundebuglib - system library 64050 (no tokens): minimal runtime error
 * handling of executables that use SetErr (blitz2.s doseterr: used
 * instead of debuglib/dhandlerlib when an executable is created).
 *
 * Translation of bblibsrc/rundebuglib ("shit knows"; the binary matches
 * the source).  init (codeinit) opens intuition, remembers a5 and replaces
 * the vectors of TRAP #0..#5 (VBR from runerrlib's getvbr, $C31D) by its
 * own handlers; finit (codefinit) puts the old vectors back and closes
 * intuition.  The handlers:
 *   TRAP #0  runtime error, d0 = message (or the codes 1-3 of blitz2's
 *            code): in user mode continue (RTE) at printerr, which shows
 *            the message in an AutoRequest and ends the program (exitslib
 *            endjmp $C800).  In supervisor mode
 *            restore the vectors and give the trap to the old vector.
 *   TRAP #1  skip the 8 bytes after the TRAP (unless ctrlc is set - it
 *            never is: BUG(orig) (#43), ctrlc is only cleared)
 *   TRAP #2  skip 6 bytes
 *   TRAP #3  nothing
 *   TRAP #4  skip 8 bytes
 *   TRAP #5  nothing
 * (the debugger's checks that the compiler emits with debug code on).
 *
 * C: codeinit, codefinit, fixtraps, printerr (normal user mode code that
 * the TRAP #0 handler continues at, never returns).
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
void mytrap0(void), mytrap1(void), mytrap2(void);
void mytrap3(void), mytrap4(void), mytrap5(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#rundebuglib,0,0,init,0,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_rundebuglib, 0, 0, init, BL_NORET, finit, 0);
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

#define TRAP0 (128 / 4)                 /* vectors 32..37 = TRAP #0..#5 */

static const char intname[] = "intuition.library";

ULONG oldtrap[6];                       /* oldtrap0..oldtrap5 */
UBYTE *olda5;
struct IntuitionBase *myint;
UWORD ctrlc;                            /* read by mytrap1 */

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

/* the trap handlers: exception frames (SR, PC at 2(sp)) and RTE */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _mytrap0, _mytrap1, _mytrap2, _mytrap3, _mytrap4, _mytrap5\n"
"_mytrap4:\n"
"   addq.l  #8,2(sp)\n"
"_mytrap5:\n"
"_mytrap3:\n"
"   rte\n"
"_mytrap2:\n"
"   addq.l  #6,2(sp)\n"
"dorte:\n"
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
"more2:\n"
/* BUG(orig) (#246): codefinit changes d0 (the message) before the trap */
"   jsr     _codefinit\n"
"   trap    #0\n"
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
 * @brief Library init: open intuition, remember a5 and install the TRAP
 * #0..#5 handlers.
 *
 * The old vectors (VBR from runerrlib's getvbr, $C31D) are saved in
 * oldtrap and replaced by mytrap0..mytrap5; ctrlc is cleared.
 * OldOpenLibrary is not checked.
 */
static void BLITZ_ROUTINE codeinit(void)
{
    struct bl_regs r = { 0, 0, 0, 0 };
    ULONG *v;
    WORD i;

    ctrlc = 0;
    myint = (struct IntuitionBase *)OldOpenLibrary((STRPTR)intname);
    olda5 = blitz_globals;

    BLITZ_ALIBJSR(TOK_getvbr, r);
    v = (ULONG *)r.a0 + TRAP0;
    for (i = 0; i < 6; i++)             /* MOVEQ #5,d0 ... DBF d0 */
        oldtrap[i] = v[i];

    BLITZ_ALIBJSR(TOK_getvbr, r);
    v = (ULONG *)r.a0 + TRAP0;
    v[0] = (ULONG)mytrap0;
    v[1] = (ULONG)mytrap1;
    v[2] = (ULONG)mytrap2;
    v[3] = (ULONG)mytrap3;
    v[4] = (ULONG)mytrap4;
    v[5] = (ULONG)mytrap5;
}

static void fixtraps(void)
{
    struct bl_regs r = { 0, 0, 0, 0 };
    ULONG *v;
    WORD i;

    BLITZ_ALIBJSR(TOK_getvbr, r);
    v = (ULONG *)r.a0 + TRAP0;
    for (i = 0; i < 6; i++)
        v[i] = oldtrap[i];
}

/**
 * @brief Library finit: put the old TRAP #0..#5 vectors back (fixtraps)
 * and close intuition.
 */
static void BLITZ_ROUTINE codefinit(void)
{
    fixtraps();
    CloseLibrary((struct Library *)myint);
}
