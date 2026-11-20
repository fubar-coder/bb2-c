/*
 * runerrlib - system library 65500 ($C300-$C321): runtime error checks of
 * programs compiled with the debugger ("heavy trap level shit"), plus
 * GetVBR/GetCPU.
 *
 * C translation of bblibsrc/runerrlib ("this has getvbr and getcpu
 * commands added"); ref/runerrlib.obj matches the source.  The compiler
 * calls the checks (blitz2.s equates around finalscheck/retchk/nextchk/
 * stackchk/arrerr/maxerr/gosuper/gouser/sarraychk/.../ftolover) only when
 * runtime error checking is on.  A failed check is `MOVE.l #msg,d0:
 * TRAP #0`, handled by debuglib (runtime error requester); it does not
 * return to the check.
 *
 * init: d2 = stack size -> lowest allowed stack pointer (maxstack, with
 * 1K reserve) and the expected stack pointer at the end (ostack); installs
 * `trapper` as the task's trap handler (CPU exceptions become runtime
 * errors "GURU! - ..."); reads the CPU type and the VBR.  finit restores
 * the trap handler (also the end of those of debuglib, rundebuglib and
 * errtraplib, which come in front of it, #1069).
 *
 * C: the value checks (overflow, array, data, current item), the FFP
 * checks (the V flag of SPFix is read by an inline assembler call),
 * getvbr/getcpu, init (MOVEC VBR as the original's Dc) and finit.
 * Assembler, copied from the original, because they work on the caller's
 * stack or flags, which C cannot see:
 *   finalscheck, stackchk  compare a7 itself
 *   retchk                 pops the Gosub frame and jumps back
 *   nextchkw/l, casechkw/l look into the caller's stack frame
 *   bmodechk, amodechk     input is the N flag
 *   trapper                exception handler (RTE)
 *   gosuper, gouser        the trap vectors of a SetInt routine on the
 *                          stack (fixed, #1069)
 *   ctrlcend ($C321)       the end after Ctrl-C with return code 20 on
 *                          the program's stack (fixed, #43; not in the
 *                          original)
 *   the error chain       `MOVE.l #msg,d0:TRAP #0` (arrerr/maxerr are
 *                          tokens themselves, the asm checks branch here)
 * Their data (cpu_ ... oldtrap, the messages) is in the assembler block,
 * so the PC relative accesses are the original's.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <stddef.h>

/* entry points (thunks / asm, defined below) */
void finalscheck(void), retchk(void), nextchkw(void), nextchkl(void);
void stackchk(void), arrerr(void), maxerr(void), gosuper(void), gouser(void);
void sarraychk(void), aarraychk(void), datachk(void), bmodechk(void);
void amodechk(void), casechkw(void), casechkl(void), currentchk(void);
void ltobover(void), wtobover(void), ltowover(void), qtobover(void);
void ftobover(void), ftowover(void), ftolover(void), getvbr(void);
void getcpu(void), runerrlib_codeinit(void), ctrlcend(void);
static void codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#runerrlib,$c300,$c322,init,0,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_runerrlib, 0xc300, 0xc322, init, BL_NORET, finit, 0);
BLITZ_NULLSUB(finalscheck, 0, 0);               /* $C300 */
BLITZ_NULLSUB(0, 0, 0);                         /* $C301 */
BLITZ_NULLSUB(0, 0, 0);                         /* $C302 */
BLITZ_NULLSUB(0, 0, 0);                         /* $C303 */
BLITZ_NULLSUB(0, 0, 0);                         /* $C304 */
BLITZ_NULLSUB(retchk, 0, 0);                    /* $C305 */
BLITZ_NULLSUB(nextchkw, 0, 0);                  /* $C306 */
BLITZ_NULLSUB(nextchkl, 0, 0);                  /* $C307 */
BLITZ_NULLSUB(stackchk, 0, 0);                  /* $C308 */
BLITZ_NULLSUB(arrerr, 0, 0);                    /* $C309 */
BLITZ_NULLSUB(maxerr, 0, 0);                    /* $C30A */
BLITZ_NULLSUB(gosuper, 0, 0);                   /* $C30B */
BLITZ_NULLSUB(gouser, 0, 0);                    /* $C30C */
BLITZ_NULLSUB(0, 0, 0);                         /* $C30D */
BLITZ_NULLSUB(sarraychk, 0, 0);                 /* $C30E */
BLITZ_NULLSUB(aarraychk, 0, 0);                 /* $C30F */
BLITZ_NULLSUB(datachk, 0, 0);                   /* $C310 */
BLITZ_NULLSUB(bmodechk, 0, 0);                  /* $C311 */
BLITZ_NULLSUB(amodechk, 0, 0);                  /* $C312 */
BLITZ_NULLSUB(casechkw, 0, 0);                  /* $C313 */
BLITZ_NULLSUB(casechkl, 0, 0);                  /* $C314 */
BLITZ_NULLSUB(currentchk, 0, 0);                /* $C315 */
BLITZ_NULLSUB(ltobover, 0, 0);                  /* $C316 */
BLITZ_NULLSUB(wtobover, 0, 0);                  /* $C317 */
BLITZ_NULLSUB(ltowover, 0, 0);                  /* $C318 */
BLITZ_NULLSUB(qtobover, 0, 0);                  /* $C319 */
BLITZ_NULLSUB(ftobover, 0, 0, LIB_ffplib, BL_LA6);      /* $C31A */
BLITZ_NULLSUB(ftowover, 0, 0, LIB_ffplib, BL_LA6);      /* $C31B */
BLITZ_NULLSUB(ftolover, 0, 0, LIB_ffplib, BL_LA6);      /* $C31C */
BLITZ_NULLSUB(getvbr, 0, 0);                    /* $C31D */
BLITZ_NULLSUB(getcpu, 0, 0);                    /* $C31E */
/* fixed (#43): ctrlcend; $C31F and $C320 stay free (AmiBlitz3's
   runerrlib has its updatestack and checkpointer there) */
BLITZ_NULLSUB(0, 0, 0);                         /* $C31F */
BLITZ_NULLSUB(0, 0, 0);                         /* $C320 */
BLITZ_NULLSUB(ctrlcend, 0, 0);                  /* $C321 */
BLITZ_NULLSUB_AT(init, runerrlib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------- data, messages, error chain */

extern UWORD cpu_, supernest;
extern ULONG vbr_;
extern UBYTE *ostack;                   /* stack should meet up with this value at end of prog! */
extern UBYTE *maxstack;
extern APTR oldtrap;
extern struct Task *ourtask;
extern const char overmess[], sarraymess[], datamess[], currentmess[];
extern char trapper[];

BLITZ_ASM_BLOCK(data,
"   .globl  _cpu_,_vbr_,_ostack,_supernest,_maxstack,_oldtrap,_ourtask\n"
"_cpu_:      .word   0\n"
"_vbr_:      .long   0\n"
"_ostack:    .long   0\n"
"_supernest: .word   0\n"
"_maxstack:  .long   0\n"
"_oldtrap:   .long   0\n"
"_ourtask:   .long   0\n"               /* fixed (#1069): the program's task */
"curtc:      .long   0\n"               /* its tc_TrapCode, for the stubs */
"trapmesss:  .long   trapn2,trapn3,trapn4,trapn5,trapn6,trapn7\n"
"            .long   trapn8,trapn9,trapn10,trapn11,trapn12,trapn13\n"
/* GURUS */
"trapn2:  .asciz \"GURU! - Bus Error\"\n"
"trapn3:  .asciz \"GURU! - Address Error\"\n"
"trapn4:  .asciz \"GURU! - ILLEGAL Instruction\"\n"
"trapn5:  .asciz \"GURU! - Division by Zero\"\n"
"trapn6:  .asciz \"GURU! - CHK Instruction\"\n"
"trapn7:  .asciz \"GURU! - TRAPV Instruction\"\n"
"trapn8:  .asciz \"GURU! - Privilege Violation\"\n"
"trapn9:  .asciz \"GURU! - Trace\"\n"
"trapn10: .asciz \"GURU! - Line 1010 Emulator\"\n"
"trapn11: .asciz \"GURU! - Line 1111 Emulator\"\n"
"trapn12: .asciz \"TRAP! - Trap Instruction\"\n"
"trapn13: .asciz \"GURU! - No Idea\"\n"
"   .even\n"
/* MESSAGES */
"   .globl  _overmess,_sarraymess,_datamess,_currentmess\n"
"returnmess:   .asciz \"Return without Gosub\"\n"
"nextmess:     .asciz \"Next without For\"\n"
"stackmess:    .asciz \"Stack Error on Exit - Unterminated Gosub or For?\"\n"
"nostackmess:  .asciz \"Out of Stack Space\"\n"
"arrmess:      .asciz \"Array Subscript out of Range\"\n"
"maxmess:      .asciz \"Value out of Maximum Range\"\n"
"_sarraymess:  .asciz \"Array Dimension out of Range\"\n"
"_datamess:    .asciz \"Data type mismatch\"\n"
"bmodemess:    .asciz \"Command only available in BLITZ mode!\"\n"
"amodemess:    .asciz \"Command only available in AMIGA mode!\"\n"
"casemess:     .asciz \"End Select/Case/Default without Select\"\n"
"_currentmess: .asciz \"No Current List Item\"\n"
"_overmess:    .asciz \"OverFlow Error\"\n"
"   .even\n"
/* ERRORS - each falls through into the next if the trap returned */
"overerr:    move.l  #_overmess,d0\n   trap #0\n"
"returnerr:  move.l  #returnmess,d0\n  trap #0\n"
"nexterr:    move.l  #nextmess,d0\n    trap #0\n"
"stackerr:   move.l  #stackmess,d0\n   trap #0\n"
"nostackerr: move.l  #nostackmess,d0\n trap #0\n"
"   .globl  _arrerr,_maxerr\n"
"_arrerr:    move.l  #arrmess,d0\n     trap #0\n"
"_maxerr:    move.l  #maxmess,d0\n     trap #0\n"
"sarrayerr:  move.l  #_sarraymess,d0\n trap #0\n"
"dataerr:    move.l  #_datamess,d0\n   trap #0\n"
"bmodeerr:   move.l  #bmodemess,d0\n   trap #0\n"
"amodeerr:   move.l  #amodemess,d0\n   trap #0\n"
"caseerr:    move.l  #casemess,d0\n    trap #0\n"
"currenterr: move.l  #_currentmess,d0\n trap #0\n"
);

/* ------------------------------------------- checks on stack and flags */

/**
 * @brief finalscheck ($C300) - at program end a7 must be where it was at
 * init (ostack).
 *
 * Errors: "Stack Error on Exit - Unterminated Gosub or For?".
 */
BLITZ_ASM_ROUTINE(finalscheck,
"   cmp.l   _ostack(pc),a7\n"
"   bne     stackerr\n"
"   rts");

/**
 * @brief stackchk ($C308) - out of stack: a7 below maxstack (not while in
 * supervisor mode, supernest <> 0).
 *
 * Errors: "Out of Stack Space".
 */
BLITZ_ASM_ROUTINE(stackchk,
"   tst.w   _supernest\n"
"   bne.w   1f\n"
"   cmp.l   _maxstack(pc),a7\n"
"   bcs     nostackerr\n"
"1: rts");

/**
 * @brief retchk ($C305) - Return: the Gosub frame (return address, "gS")
 * must be on the stack; pop it and return there.
 *
 * Errors: "Return without Gosub".
 */
BLITZ_ASM_ROUTINE(retchk,
"   cmp.w   #0x6753,8(a7)\n"            /* "gS" */
"   bne     returnerr\n"
"   move.l  4(a7),a0\n"
"   lea     10(a7),a7\n"
"   jmp     (a0)");

/**
 * @brief nextchkw/nextchkl ($C306/$C307) - Next: the For frame ("fO") must
 * be on the stack (at 12(a7) for a word loop, 16(a7) for a long one).
 *
 * Errors: "Next without For".
 */
BLITZ_ASM_ROUTINE(nextchkw,
"   cmp.w   #0x664f,12(a7)\n"           /* "fO" */
"   bne     nexterr\n"
"   rts");
BLITZ_ASM_ROUTINE(nextchkl,
"   cmp.w   #0x664f,16(a7)\n"
"   bne     nexterr\n"
"   rts");

/**
 * @brief casechkw/casechkl ($C313/$C314) - End Select, Case, Default: the
 * Select frame ("sE") must be on the stack (6(a7) / 8(a7)).
 *
 * Errors: "End Select/Case/Default without Select".
 */
BLITZ_ASM_ROUTINE(casechkw,
"   cmp.w   #0x7345,6(a7)\n"            /* "sE" */
"   bne     caseerr\n"
"   rts");
BLITZ_ASM_ROUTINE(casechkl,
"   cmp.w   #0x7345,8(a7)\n"
"   bne     caseerr\n"
"   rts");

/**
 * @brief bmodechk/amodechk ($C311/$C312) - Blitz / Amiga mode only: the
 * caller has just tested the mode flag (N set = Blitz mode).
 *
 * Errors: "Command only available in BLITZ mode!" / "... AMIGA mode!".
 */
BLITZ_ASM_ROUTINE(bmodechk,
"   bpl     bmodeerr\n"
"   rts");
BLITZ_ASM_ROUTINE(amodechk,
"   bmi     amodeerr\n"
"   rts");

/* trapper - the task's trap handler (tc_TrapCode): the exception number
   is on the supervisor stack; return to user mode at 'more and raise the
   runtime error "GURU! - ..." (traps #0-#15 and above: "TRAP! - Trap
   Instruction", unknown numbers: "No Idea") */
BLITZ_ASM_BLOCK(trapper,
"   .globl  _trapper\n"
"_trapper:\n"
"   move.l  (a7)+,d0\n"
"   move.l  #1f,2(a7)\n"
"   rte\n"
"1: cmp.w   #32,d0\n"
"   bcs.w   2f\n"
"   moveq   #12,d0\n"
"2: cmp.w   #14,d0\n"
"   bcs.w   3f\n"
"   moveq   #13,d0\n"
"3: lea     trapmesss(pc),a0\n"
"   lsl.w   #2,d0\n"
"   move.l  -8(a0,d0.w),d0\n"
"   trap    #0\n"
);

/* ------------------------------------------------------- value checks */

/* All checks keep every register (the thunks); they only return when the
   value is fine.  DIFF: if the runtime error trap returned, the original
   would run on through the error chain, the C checks return. */

static void ltob(LONG v)
{
    if (v > 127 || v < -128)
        BLITZ_RUNERR(overmess);
}

static void ltow(LONG v)
{
    if (v > 32767 || v < -32768)
        BLITZ_RUNERR(overmess);
}

/**
 * @brief ltobover ($C316) - long to byte: -128..127.
 *
 * Errors: "OverFlow Error".
 *
 * @param v d0: the value
 */
static void BLITZ_ROUTINE ltobover_c(LONG v BL_REG(d0)) { ltob(v); }
BLITZ_THUNK(ltobover, ltobover_c, "d0-d1/a0-a1");

/**
 * @brief wtobover ($C317) - word to byte (CMP.w): -128..127.
 *
 * Errors: "OverFlow Error".
 *
 * @param v d0.w: the value
 */
static void BLITZ_ROUTINE wtobover_c(LONG v BL_REG(d0))
{
    WORD w = (WORD)v;

    if (w > 127 || w < -128)
        BLITZ_RUNERR(overmess);
}
BLITZ_THUNK(wtobover, wtobover_c, "d0-d1/a0-a1");

/**
 * @brief ltowover ($C318) - long to word: -32768..32767.
 *
 * Errors: "OverFlow Error".
 *
 * @param v d0: the value
 */
static void BLITZ_ROUTINE ltowover_c(LONG v BL_REG(d0)) { ltow(v); }
BLITZ_THUNK(ltowover, ltowover_c, "d0-d1/a0-a1");

/**
 * @brief qtobover ($C319) - quick (16.16 fixed point) to byte: the integer
 * part is the upper word (SWAP d0:CMP.w), -128..127.
 *
 * Errors: "OverFlow Error".
 *
 * @param v d0: the quick value
 */
static void BLITZ_ROUTINE qtobover_c(ULONG v BL_REG(d0))
{
    WORD w = (WORD)(v >> 16);

    if (w > 127 || w < -128)
        BLITZ_RUNERR(overmess);
}
BLITZ_THUNK(qtobover, qtobover_c, "d0-d1/a0-a1");

/* SPFix of mathffp (JSR -30(a6), base from ffplib in a6); *v = the V
   flag after the call (BVS overerr in the original) */
static LONG spfix(ULONG x, APTR base, UBYTE *v)
{
    register ULONG d0 __asm("d0") = x;
    register APTR a6 __asm("a6") = base;
    UBYTE vs;

    __asm volatile("jsr     -30(a6)\n"
                   "   svs     %1"
                   : "+d"(d0), "=d"(vs) : "a"(a6) : "d1", "a0", "a1", "cc");
    *v = vs;
    return d0;
}

/**
 * @brief ftobover ($C31A) - float to byte: SPFix must not overflow and the
 * integer must be -128..127.
 *
 * Errors: "OverFlow Error".
 *
 * @param f   d0: the FFP float
 * @param ffp a6: mathffp.library (from ffplib)
 */
static void BLITZ_ROUTINE ftobover_c(ULONG f BL_REG(d0), APTR ffp BL_REG(a6))
{
    UBYTE v;
    LONG i = spfix(f, ffp, &v);

    if (v)
        BLITZ_RUNERR(overmess);
    else
        ltob(i);
}
BLITZ_THUNK(ftobover, ftobover_c, "d0-d1/a0-a1");

/**
 * @brief ftowover ($C31B) - float to word: SPFix must not overflow and the
 * integer must be -32768..32767.
 *
 * Errors: "OverFlow Error".
 *
 * @param f   d0: the FFP float
 * @param ffp a6: mathffp.library (from ffplib)
 */
static void BLITZ_ROUTINE ftowover_c(ULONG f BL_REG(d0), APTR ffp BL_REG(a6))
{
    UBYTE v;
    LONG i = spfix(f, ffp, &v);

    if (v)
        BLITZ_RUNERR(overmess);
    else
        ltow(i);
}
BLITZ_THUNK(ftowover, ftowover_c, "d0-d1/a0-a1");

/**
 * @brief ftolover ($C31C) - float to long: SPFix must not overflow (V flag).
 *
 * Errors: "OverFlow Error".
 *
 * @param f   d0: the FFP float
 * @param ffp a6: mathffp.library (from ffplib)
 */
static void BLITZ_ROUTINE ftolover_c(ULONG f BL_REG(d0), APTR ffp BL_REG(a6))
{
    UBYTE v;

    spfix(f, ffp, &v);
    if (v)
        BLITZ_RUNERR(overmess);
}
BLITZ_THUNK(ftolover, ftolover_c, "d0-d1/a0-a1");

/**
 * @brief sarraychk ($C30E) - an array dimension (its highest index) must
 * be < 65535.
 *
 * An array or list has at most 65535 elements: arrmult counts the
 * elements of a dimension as a word (highest index + 1).  Fixed (#1037):
 * the highest index 65535 is refused too (the original refused only
 * 65536 and more; 65535 gave 0 elements, every access outside the
 * block).
 *
 * Errors: "Array Dimension out of Range".
 *
 * @param d d0: the dimension
 */
static void BLITZ_ROUTINE sarraychk_c(ULONG d BL_REG(d0))
{
    if (d >= 65535)                             /* fixed (#1037) */
        BLITZ_RUNERR(sarraymess);
}
BLITZ_THUNK(sarraychk, sarraychk_c, "d0-d1/a0-a1");

/**
 * @brief aarraychk ($C30F) - as sarraychk, the dimension in d1 (the check
 * of each dimension of Dim and Dim List).
 *
 * Fixed (#1037): refuses 65535 too, as sarraychk.
 *
 * Errors: "Array Dimension out of Range".
 *
 * @param d d1: the dimension
 */
static void BLITZ_ROUTINE aarraychk_c(ULONG d BL_REG(d1))
{
    if (d >= 65535)                             /* fixed (#1037) */
        BLITZ_RUNERR(sarraymess);
}
BLITZ_THUNK(aarraychk, aarraychk_c, "d0-d1/a0-a1");

/**
 * @brief datachk ($C310) - Read: the data item at a3 must be "D" + type.
 *
 * a3 is advanced over both bytes.  (The original leaves the "D" in d1.b,
 * the thunk keeps d1.)
 *
 * Errors: "Data type mismatch".
 *
 * @param type d0.b: the type the Read expects
 */
static void BLITZ_ROUTINE datachk_c(ULONG type BL_REG(d0))
{
    if (*blitz_string++ != 'D') {
        BLITZ_RUNERR(datamess);
        return;
    }
    if (*blitz_string++ != (UBYTE)type)
        BLITZ_RUNERR(datamess);
}
BLITZ_THUNK(datachk, datachk_c, "d0-d1/a0-a1");

/**
 * @brief currentchk ($C315) - a list command needs a current item.
 *
 * Errors: "No Current List Item".
 *
 * @param item a2: the current item (0 = none)
 */
static void BLITZ_ROUTINE currentchk_c(APTR item BL_REG(a2))
{
    if (!item)
        BLITZ_RUNERR(currentmess);
}
BLITZ_THUNK(currentchk, currentchk_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------------- misc */

/* tc_TrapCode in struct Task: the offset the assembler below uses */
static_assert(offsetof(struct Task, tc_TrapCode) == 50);

/**
 * @brief gosuper ($C30B) - the start of a SetInt routine (the compiler
 * calls it there, in front of the routine's TRAP #2, only with runtime
 * error checking): no stack check (supernest + 1), and the CPU's TRAP
 * #0-#5 vectors lead to the program's trap handler while the routine runs.
 *
 * Fixed (#1069): debuglib, rundebuglib and errtraplib take the traps of
 * their program through its task's trap handler (tc_TrapCode), not the
 * CPU's vector table, which belongs to the whole machine.  exec passes
 * only a trap of user mode to tc_TrapCode; one in supervisor mode (an
 * interrupt) is a dead end.  So gosuper saves the six vectors on the
 * stack (24 bytes, the vector of TRAP #0 first, left there for gouser)
 * and sets stubs that hand the trap to the program's handler as exec
 * does for user mode (the trap number as a long on the stack, then the
 * exception frame).  No other task runs while an interrupt routine runs,
 * and nested interrupts restore in reverse order, so nothing else sees
 * the stubs.  rundebuglib reads the saved vectors there (its more2).
 * The original only counted supernest; the CPU's vectors were the
 * libraries' all the time.  Keeps all registers.
 */
BLITZ_ASM_ROUTINE(gosuper,
"   lea     -24(sp),sp\n"               /* room for the six vectors */
"   move.l  24(sp),(sp)\n"              /* the return address below it */
"   movem.l a0-a1,-(sp)\n"
"   addq.w  #1,_supernest\n"
"   move.l  _ourtask(pc),a0\n"
"   move.l  50(a0),curtc\n"             /* tc_TrapCode */
"   move.l  _vbr_(pc),a0\n"
"   lea     128(a0),a0\n"               /* TRAP #0 */
"   lea     12(sp),a1\n"
"   move.l  (a0),(a1)+\n"
"   move.l  #tstub0,(a0)+\n"
"   move.l  (a0),(a1)+\n"
"   move.l  #tstub1,(a0)+\n"
"   move.l  (a0),(a1)+\n"
"   move.l  #tstub2,(a0)+\n"
"   move.l  (a0),(a1)+\n"
"   move.l  #tstub3,(a0)+\n"
"   move.l  (a0),(a1)+\n"
"   move.l  #tstub4,(a0)+\n"
"   move.l  (a0),(a1)\n"
"   move.l  #tstub5,(a0)\n"
"   movem.l (sp)+,a0-a1\n"
"   rts\n"
/* the stubs: as exec for a trap of user mode */
"tstub0: move.l  #32,-(sp)\n"
"   bra.s   tstub\n"
"tstub1: move.l  #33,-(sp)\n"
"   bra.s   tstub\n"
"tstub2: move.l  #34,-(sp)\n"
"   bra.s   tstub\n"
"tstub3: move.l  #35,-(sp)\n"
"   bra.s   tstub\n"
"tstub4: move.l  #36,-(sp)\n"
"   bra.s   tstub\n"
"tstub5: move.l  #37,-(sp)\n"
"tstub:  move.l  curtc(pc),-(sp)\n"
"   rts");

/**
 * @brief gouser ($C30C) - the end of a SetInt routine: supernest - 1, and
 * the six vectors saved by gosuper back into the CPU's table (fixed
 * (#1069), see gosuper).  The saved vectors are taken from the stack.
 * Keeps all registers.
 */
BLITZ_ASM_ROUTINE(gouser,
"   movem.l a0-a1,-(sp)\n"
"   subq.w  #1,_supernest\n"
"   move.l  _vbr_(pc),a0\n"
"   lea     128(a0),a0\n"
"   lea     12(sp),a1\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1)+,(a0)+\n"
"   move.l  (a1),(a0)\n"
"   movem.l (sp)+,a0-a1\n"
"   move.l  (sp),24(sp)\n"              /* the return address above them */
"   lea     24(sp),sp\n"
"   rts");

/**
 * @brief getvbr ($C31D) - the vector base register as read by the init.
 *
 * Keeps d0-d1/a1 (the thunk moves the result to a0).
 *
 * Fixed (intended deviation, #42): also on a 68010 (the original gives 0
 * there, though a 68010 has a VBR).
 *
 * @return a0: the VBR (0 on a 68000)
 */
static ULONG BLITZ_ROUTINE getvbr_c(void) { return vbr_; }
BLITZ_THUNK_POST(getvbr, getvbr_c, "d0-d1/a1", "move.l d0,a0");

/**
 * @brief getcpu ($C31E) - the CPU type.
 *
 * The number of the highest AttnFlags bit of 68010..68040.
 *
 * @return d0.w: 0 = 68000 ... 4 = 68040; the upper word is kept
 */
static UWORD BLITZ_ROUTINE getcpu_c(void) { return cpu_; }
BLITZ_THUNK_POST(getcpu, getcpu_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief ctrlcend ($C321) - end the program with return code 20 (fixed,
 * #43: Ctrl-C at an input or output on the console).
 *
 * Called by inputoutputlib's Ctrl-C checks, which only error check code
 * calls.  The program's return address (the long above the stack of its
 * main code: ostack + 4, the level finalscheck compares) is replaced by
 * `rc20`, then exitslib's endjmp ends the program as End does: the finits
 * run and the end code's `moveq #0,d0:rts` returns to rc20, which sets
 * the return code 20 (that of a C program after Ctrl-C) and goes on to
 * the caller.  Without a change of the end code every program has, which
 * would change every executable.  Assembler: it works on the program's
 * stack.  Does not return.
 */
BLITZ_ASM_BLOCK(ctrlcend,
"   .globl  _ctrlcend\n"
"_ctrlcend:\n"
"   move.l  _ostack(pc),a0\n"
"   addq.l  #4,a0\n"
"   lea     ctrlcret(pc),a1\n"
"   move.l  (a0),(a1)\n"
"   lea     rc20(pc),a1\n"
"   move.l  a1,(a0)\n"
"   jmp     _blitz_tok+0x8000c800\n"    /* endjmp */
"rc20:\n"
"   move.l  ctrlcret(pc),-(sp)\n"
"   moveq   #20,d0\n"
"   rts\n"
"ctrlcret: .long 0\n");

/**
 * @brief Library init: stack limits, trap handler, CPU and VBR.
 *
 * maxstack = the lowest allowed stack pointer (with 1K reserve), ostack =
 * the stack pointer expected at the end; `trapper` becomes the task's
 * tc_TrapCode (CPU exceptions become runtime errors "GURU! - ..."); the
 * CPU type from AttnFlags and, from a 68010 on, the VBR (MOVEC in
 * supervisor state).  The original returns d2 = VBR (and changes
 * d0-d1/a0/a6); the thunk keeps everything.
 *
 * Fixed (intended deviation, #42): the original reads the VBR only from a
 * 68020 on, though a 68010 has one too.
 *
 * @param sp    a0: the stack pointer at entry (pointing to the return
 *              address; the thunk computes it)
 * @param stack d2: the stack size
 */
static void BLITZ_ROUTINE codeinit_c(UBYTE *sp BL_REG(a0), ULONG stack BL_REG(d2))
{
    UWORD f, n = 0;
    ULONG vbr = 0;

    maxstack = sp - stack + 1024;
    ostack = sp + 4;
    ourtask = SysBase->ThisTask;
    oldtrap = ourtask->tc_TrapCode;
    ourtask->tc_TrapCode = trapper;
    /* CPU: number of the highest AttnFlags bit of 68010..68040 */
    for (f = SysBase->AttnFlags & 15; f; f >>= 1)
        n++;
    cpu_ = n;
    if (n >= 1) {                       /* 68010+ (fixed, #42: the original 68020+) */
        APTR ssp = SuperState();        /* supervisor state! */
        register ULONG r __asm("d0");

        __asm volatile(".word 0x4e7a,0x0801" : "=d"(r));  /* MOVEC VBR,d0 */
        vbr = r;
        UserState(ssp);
    }
    vbr_ = vbr;
}
BLITZ_THUNK_PRE(runerrlib_codeinit, codeinit_c, "d0-d1/a0-a1",
                "lea 16(sp),a0");

/**
 * @brief Library finit: restore the task's trap handler.
 *
 * The handler the program started with: since #1069 also the end of the
 * handlers of debuglib, rundebuglib and errtraplib, which init after
 * runerrlib and put theirs in front (debuglib and rundebuglib leave theirs
 * until here, for SetInt routines that run until intlib's finit).
 */
static void BLITZ_ROUTINE codefinit(void)
{
    SysBase->ThisTask->tc_TrapCode = oldtrap;
}
