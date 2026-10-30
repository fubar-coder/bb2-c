/*
 * runerrlib - system library 65500 ($C300-$C31E): runtime error checks of
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
 * the trap handler.
 *
 * C: the value checks (overflow, array, data, current item), the FFP
 * checks (the V flag of SPFix is read by an inline assembler call),
 * gosuper/gouser, getvbr/getcpu, init (MOVEC VBR as the original's Dc)
 * and finit.
 * Assembler, copied from the original, because they work on the caller's
 * stack or flags, which C cannot see:
 *   finalscheck, stackchk  compare a7 itself
 *   retchk                 pops the Gosub frame and jumps back
 *   nextchkw/l, casechkw/l look into the caller's stack frame
 *   bmodechk, amodechk     input is the N flag
 *   trapper                exception handler (RTE)
 *   the error chain        `MOVE.l #msg,d0:TRAP #0` (arrerr/maxerr are
 *                          tokens themselves, the asm checks branch here)
 * Their data (cpu_ ... oldtrap, the messages) is in the assembler block,
 * so the PC relative accesses are the original's.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks / asm, defined below) */
void finalscheck(void), retchk(void), nextchkw(void), nextchkl(void);
void stackchk(void), arrerr(void), maxerr(void), gosuper(void), gouser(void);
void sarraychk(void), aarraychk(void), datachk(void), bmodechk(void);
void amodechk(void), casechkw(void), casechkl(void), currentchk(void);
void ltobover(void), wtobover(void), ltowover(void), qtobover(void);
void ftobover(void), ftowover(void), ftolover(void), getvbr(void), getcpu(void);
void codeinit(void), codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#runerrlib,$c300,$c31f,init,0,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_runerrlib, 0xc300, 0xc31f, init, BL_NORET, finit, 0);
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
BLITZ_NULLSUB(ftobover, 0, 0, LIB_ffplib, BL_LA(6));    /* $C31A */
BLITZ_NULLSUB(ftowover, 0, 0, LIB_ffplib, BL_LA(6));    /* $C31B */
BLITZ_NULLSUB(ftolover, 0, 0, LIB_ffplib, BL_LA(6));    /* $C31C */
BLITZ_NULLSUB(getvbr, 0, 0);                    /* $C31D */
BLITZ_NULLSUB(getcpu, 0, 0);                    /* $C31E */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------- data, messages, error chain */

extern UWORD cpu_, supernest;
extern ULONG vbr_;
extern UBYTE *ostack;                   /* stack should meet up with this value at end of prog! */
extern UBYTE *maxstack;
extern APTR oldtrap;
extern const char overmess[], sarraymess[], datamess[], currentmess[];
extern char trapper[];

__asm__(
"   .text\n"
"   .even\n"
"   .globl  _cpu_,_vbr_,_ostack,_supernest,_maxstack,_oldtrap\n"
"_cpu_:      .word   0\n"
"_vbr_:      .long   0\n"
"_ostack:    .long   0\n"
"_supernest: .word   0\n"
"_maxstack:  .long   0\n"
"_oldtrap:   .long   0\n"
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

/* finalscheck - at program end a7 must be where it was at init */
BLITZ_ASM_ROUTINE(finalscheck,
"   cmp.l   _ostack(pc),a7\n"
"   bne     stackerr\n"
"   rts");

/* stackchk - out of stack (not while in supervisor mode) */
BLITZ_ASM_ROUTINE(stackchk,
"   tst.w   _supernest\n"
"   bne.w   1f\n"
"   cmp.l   _maxstack(pc),a7\n"
"   bcs     nostackerr\n"
"1: rts");

/* retchk - Return: the Gosub frame (return address, "gS") must be on the
   stack; pop it and return there */
BLITZ_ASM_ROUTINE(retchk,
"   cmp.w   #0x6753,8(a7)\n"            /* "gS" */
"   bne     returnerr\n"
"   move.l  4(a7),a0\n"
"   lea     10(a7),a7\n"
"   jmp     (a0)");

/* nextchkw/l - Next: the For frame ("fO") must be on the stack */
BLITZ_ASM_ROUTINE(nextchkw,
"   cmp.w   #0x664f,12(a7)\n"           /* "fO" */
"   bne     nexterr\n"
"   rts");
BLITZ_ASM_ROUTINE(nextchkl,
"   cmp.w   #0x664f,16(a7)\n"
"   bne     nexterr\n"
"   rts");

/* casechkw/l - End Select etc.: the Select frame ("sE") */
BLITZ_ASM_ROUTINE(casechkw,
"   cmp.w   #0x7345,6(a7)\n"            /* "sE" */
"   bne     caseerr\n"
"   rts");
BLITZ_ASM_ROUTINE(casechkl,
"   cmp.w   #0x7345,8(a7)\n"
"   bne     caseerr\n"
"   rts");

/* bmodechk/amodechk - the caller has just tested the mode flag: N set =
   Blitz mode */
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
__asm__(
"   .text\n"
"   .even\n"
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

/* ltobover - long to byte */
static void BLITZ_ROUTINE ltobover_c(LONG v BL_REG(d0)) { ltob(v); }
BLITZ_THUNK(ltobover, ltobover_c, "d0-d1/a0-a1");

/* wtobover - word to byte (CMP.w) */
static void BLITZ_ROUTINE wtobover_c(LONG v BL_REG(d0))
{
    WORD w = (WORD)v;

    if (w > 127 || w < -128)
        BLITZ_RUNERR(overmess);
}
BLITZ_THUNK(wtobover, wtobover_c, "d0-d1/a0-a1");

/* ltowover - long to word */
static void BLITZ_ROUTINE ltowover_c(LONG v BL_REG(d0)) { ltow(v); }
BLITZ_THUNK(ltowover, ltowover_c, "d0-d1/a0-a1");

/* qtobover - quick (16.16 fixed point) to byte: the integer part is the
   upper word (SWAP d0:CMP.w) */
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

/* ftobover/ftowover/ftolover - float to byte/word/long */
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

static void BLITZ_ROUTINE ftolover_c(ULONG f BL_REG(d0), APTR ffp BL_REG(a6))
{
    UBYTE v;

    spfix(f, ffp, &v);
    if (v)
        BLITZ_RUNERR(overmess);
}
BLITZ_THUNK(ftolover, ftolover_c, "d0-d1/a0-a1");

/* sarraychk/aarraychk - array dimension (d0 / d1) must be < 65536 */
static void BLITZ_ROUTINE sarraychk_c(ULONG d BL_REG(d0))
{
    if (d >= 65536)
        BLITZ_RUNERR(sarraymess);
}
BLITZ_THUNK(sarraychk, sarraychk_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE aarraychk_c(ULONG d BL_REG(d1))
{
    if (d >= 65536)
        BLITZ_RUNERR(sarraymess);
}
BLITZ_THUNK(aarraychk, aarraychk_c, "d0-d1/a0-a1");

/* datachk - Read: the data item at a3 must be "D" + type (d0.b); a3 is
   advanced over both bytes.  (The original leaves the "D" in d1.b, the
   thunk keeps d1.) */
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

/* currentchk - a list command needs a current item (a2) */
static void BLITZ_ROUTINE currentchk_c(APTR item BL_REG(a2))
{
    if (!item)
        BLITZ_RUNERR(currentmess);
}
BLITZ_THUNK(currentchk, currentchk_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------------- misc */

/* gosuper/gouser - no stack check while in supervisor mode */
static void BLITZ_ROUTINE gosuper_c(void) { supernest++; }
BLITZ_THUNK(gosuper, gosuper_c, "d0-d1/a0-a1");
static void BLITZ_ROUTINE gouser_c(void) { supernest--; }
BLITZ_THUNK(gouser, gouser_c, "d0-d1/a0-a1");

/* getvbr - a0 = VBR (0 on a 68000); keeps d0-d1/a1 */
static ULONG BLITZ_ROUTINE getvbr_c(void) { return vbr_; }
BLITZ_THUNK_POST(getvbr, getvbr_c, "d0-d1/a1", "move.l d0,a0");

/* getcpu - d0.w = CPU (0 = 68000 ... 4 = 68040); upper word kept */
static UWORD BLITZ_ROUTINE getcpu_c(void) { return cpu_; }
BLITZ_THUNK_POST(getcpu, getcpu_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/* codeinit - sp = the stack pointer at entry (pointing to the return
   address; the thunk computes it), d2 = stack size.
   The original returns d2 = VBR (and changes d0-d1/a0/a6); the thunk
   keeps everything. */
static void BLITZ_ROUTINE codeinit_c(UBYTE *sp BL_REG(a0), ULONG stack BL_REG(d2))
{
    UWORD f, n = 0;
    ULONG vbr = 0;

    maxstack = sp - stack + 1024;
    ostack = sp + 4;
    oldtrap = SysBase->ThisTask->tc_TrapCode;
    SysBase->ThisTask->tc_TrapCode = trapper;
    /* CPU: number of the highest AttnFlags bit of 68010..68040 */
    for (f = SysBase->AttnFlags & 15; f; f >>= 1)
        n++;
    cpu_ = n;
    if (n >= 2) {                       /* 68020+ (BUG(orig) (#42): 68010 has a VBR too) */
        APTR ssp = SuperState();        /* supervisor state! */
        register ULONG r __asm("d0");

        __asm volatile(".word 0x4e7a,0x0801" : "=d"(r));  /* MOVEC VBR,d0 */
        vbr = r;
        UserState(ssp);
    }
    vbr_ = vbr;
}
BLITZ_THUNK_PRE(codeinit, codeinit_c, "d0-d1/a0-a1", "lea 16(sp),a0");

/* codefinit - restore the task's trap handler */
void BLITZ_ROUTINE codefinit(void)
{
    SysBase->ThisTask->tc_TrapCode = oldtrap;
}
