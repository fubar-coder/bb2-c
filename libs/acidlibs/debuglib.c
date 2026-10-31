/*
 * debuglib - system library 64060 ($D500-$D509): the run time part of the
 * Blitz debugger.  Programs compiled with the debugger contain TRAP
 * instructions that come here (the vectors are set by init in the VBR
 * table, through runerrlib getvbr):
 *   TRAP #0  runtime error (d0 = message, or 1/2/3 = the texts below)
 *   TRAP #1  new line (single step, breakpoints, Stop, Eval, Exec, Skip)
 *   TRAP #2  start of an interrupt routine, TRAP #3 its end
 *   TRAP #4  start of a procedure, TRAP #5 its end
 * The editor (the debugger front end) talks to it through the tokens:
 *   $D500 donewmode   (nothing)       $D505 doexecute  a0 = text to run
 *   $D501 dostop      "Interupted"    $D506 doquit
 *   $D502 dosethandler                $D507 dofindoff  ascii offset
 *   $D503 dodetoke    a0 -> a1        $D508 doskip
 *   $D504 doevaluate  a0 = expression $D509 docall     calls handler 6
 * dosethandler (d0 = pointer to a word: call handler 7 at procedure
 * start/end when non zero, a0 = table of 8 handlers: state, super error,
 * user error, wait, eval, (unused), docall, procedure) returns the old
 * table in d0, a0 = the trap registers (d0-a7, pc.l, sr.w), a1 = the state
 * block from `oldmode` on (oldmode.w, flags.l, tokens.l, supertext,
 * usertext, evaltext, exectext, handler, ...) and a2 = the token list.
 * Its layout is therefore kept exactly (assembler data block below).
 *
 * ref/debuglib.obj is NEWER than bblibsrc/debuglib, the BINARY is the
 * reference:
 *   - header `last` is $D510 (source $D509), 10 tokens: $D509 docall is new;
 *   - two more handlers (table entries 6 and 7, `h6`/`h7`) and the word
 *     pointer `procflag` (dosethandler d0): TRAP #4/#5 call h7 with
 *     d1 = 0 (start) / -1 (end) when *procflag is non zero;
 *   - dosethandler also returns a0/a1/a2 (source: nothing);
 *   - the wait handler's d0 is kept (waitres): non zero = return to the
 *     program with goblitzS/goqamigaS (fixmode) instead of BLITZ/QAMIGA,
 *     i.e. without redrawing its display;
 *   - init: d0.b = 0 -> stop at once ("Interrupted - program start");
 *   - unreferenced `jsr` to printlib ($5D81) twice at the end: these
 *     relocations make the compiler include printlib (evaluate prints).
 *
 * C: the token routines (flag setters, dosethandler's table copy,
 * dodetoke, dofindoff), init/finit, the mode switches amigamode/fixmode,
 * the output handlers that collect evaluate's output, and the state
 * machine `handleflags` that runs in user mode between the trap and the
 * return into the program.
 * Assembler, copied from the binary, because it is exception handling:
 * the TRAP handlers (exception frames, RTE/RTR, register images), the
 * return into the program (executestate: registers back, RTR),
 * evaluate/execute (TRAP #15 with an
 * inline parameter block, return through touser2/RTE to dirdone), the
 * default handlers (CMP d0,d0 = Z result) and docall (passes all
 * registers through to the editor's handler).  The C routines that the
 * assembler calls are reached through thunks that keep what the original
 * keeps.
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void donewmode(void), dostop(void), dosethandler(void), dodetoke(void);
void doevaluate(void), doexecute(void), doquit(void), dofindoff(void);
void doskip(void), docall(void), codeinit(void), codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#debuglib,$d500,$d509,init,0,finit,0} - binary: $d510 */
BLITZ_SYSLIBHEADER7(LIB_debuglib, 0xd500, 0xd510, init, BL_NORET, finit, 0);
BLITZ_NULLSUB(donewmode, 0, 0);                 /* $D500 */
BLITZ_NULLSUB(dostop, 0, 0);                    /* $D501 */
BLITZ_NULLSUB(dosethandler, 0, 0);              /* $D502 */
BLITZ_NULLSUB(dodetoke, 0, 0);                  /* $D503 */
BLITZ_NULLSUB(doevaluate, 0, 0);                /* $D504 */
BLITZ_NULLSUB(doexecute, 0, 0);                 /* $D505 */
BLITZ_NULLSUB(doquit, 0, 0);                    /* $D506 */
BLITZ_NULLSUB(dofindoff, 0, 0);                 /* $D507 */
BLITZ_NULLSUB(doskip, 0, 0);                    /* $D508 */
BLITZ_NULLSUB(docall, 0, 0);                    /* $D509 */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

/* token list of the editor: 0.l next, 4.w token, 6 name */
struct toke {
    struct toke *next;
    UWORD num;
    char name[1];
};

/* ------------------------------------------------------- the state block
   (order and sizes as in the binary; the editor reads it through a1) */
extern UBYTE intsoff[2];                /* turn interupts off (ST/SF: high byte) */
extern UWORD oldmode;                   /* mode before amigamode (-1/0/1) */
extern volatile UBYTE flags[4];         /* BSET #n,flags: bits of the first byte */
extern struct toke *tokens;
extern const char *supertext, *usertext, *evaltext, *exectext;
extern APTR handler;                    /* current handler table */
extern APTR statehandler, superhandler, userhandler, waithandler, evalhandler;
extern APTR h6, h7;                     /* docall / procedure handler */
extern UBYTE *procsp, procstack[];
extern UBYTE *intsp, intstack[];
extern UWORD *procflag;                 /* dosethandler d0 */
extern char statehandler_[], superhandler_[], userhandler_[];
extern char waithandler_[], evalhandler_[];
extern char mytrap0[], mytrap1[], mytrap2[], mytrap3[], mytrap4[], mytrap5[];
extern UWORD dirmode;                   /* ne = in direct mode! */
extern APTR oldouth;                    /* ne = old output handler! */
extern UBYTE astring[];                 /* 256+16: evaluate in/output */
extern UWORD doutx;                     /* output position in astring */

__asm__(
"   .text\n"
"   .even\n"
"   .globl  _intsoff,_oldmode,_flags,_tokens,_supertext,_usertext,_evaltext\n"
"   .globl  _exectext,_handler,_statehandler,_superhandler,_userhandler\n"
"   .globl  _waithandler,_evalhandler,_h6,_h7,_procsp,_procstack,_intsp\n"
"   .globl  _intstack,_procflag\n"
"   .globl  _statehandler_,_superhandler_,_userhandler_,_waithandler_,_evalhandler_\n"
"_intsoff:      .word   0\n"
"_oldmode:      .word   0\n"
"_flags:        .long   0\n"
"_tokens:       .long   0\n"
"_supertext:    .long   0\n"
"_usertext:     .long   0\n"
"_evaltext:     .long   0\n"
"_exectext:     .long   0\n"
"_handler:      .long   _statehandler\n"
"_statehandler: .long   _statehandler_\n"
"_superhandler: .long   _superhandler_\n"
"_userhandler:  .long   _userhandler_\n"
"_waithandler:  .long   _waithandler_\n"
"_evalhandler:  .long   _evalhandler_\n"
"_h6:           .long   _evalhandler_\n"
"_h7:           .long   _evalhandler_\n"
"procnull:      .long   0,0\n"
"_procsp:       .long   _procstack-6\n"
"_procstack:    .long   procnull\n"
"               .word   0\n"
"               .space  6*32*4\n"       /* Ds.l 6*32 ;procmax  ;maximum proc nest */
"_intsp:        .long   _intstack\n"
"_intstack:     .space  2*16*4\n"       /* max interupt nest */
"_procflag:     .long   0\n"
/* default handlers */
"_statehandler_: cmp.w  d0,d0\n"
"   rts\n"
"_superhandler_: bra.w  _doquit\n"
"_userhandler_:  bra.w  _doquit\n"
"_waithandler_:  cmp.w  d0,d0\n"
"   rts\n"
"_evalhandler_:  rts\n"
);

/* ------------------------------------------------------- trap handlers */

/* TRAP #4 - start of procedure processing: the return address (behind
   the trap and its 8 byte parameter) is the procedure's; a new entry on
   procstack, or count up the recursion of the current one */
__asm__(
"   .globl  _mytrap4,_mytrap5\n"
"_mytrap4:\n"
"   movem.l a0-a1,-(a7)\n"
"   move.l  _procflag(pc),a0\n"
"   tst.w   (a0)\n"
"   beq.w   1f\n"
"   movem.l d0-d7/a0-a6,-(a7)\n"
"   moveq   #0,d1\n"
"   move.l  _h7(pc),a0\n"
"   jsr     (a0)\n"
"   movem.l (a7)+,d0-d7/a0-a6\n"
"1: move.l  10(a7),a0\n"
"   addq.l  #8,10(a7)\n"                /* fix return address */
"   move.l  _procsp(pc),a1\n"
"   cmp.l   (a1),a0\n"
"   beq.w   2f\n"
"   addq.w  #6,a1\n"
"   move.l  a1,_procsp\n"
"   move.l  a0,(a1)+\n"
"   clr.w   (a1)+\n"
"   movem.l (a7)+,a0-a1\n"
"   rte\n"
"2: addq.w  #1,4(a1)\n"
"   movem.l (a7)+,a0-a1\n"
"   rte\n"
/* TRAP #5 - end of proc processing */
"_mytrap5:\n"
"   move.l  a0,-(a7)\n"
"   move.l  _procflag(pc),a0\n"
"   tst.w   (a0)\n"
"   beq.w   1f\n"
"   movem.l d0-d7/a0-a6,-(a7)\n"
"   moveq   #-1,d1\n"
"   move.l  _h7(pc),a0\n"
"   jsr     (a0)\n"
"   movem.l (a7)+,d0-d7/a0-a6\n"
"1: move.l  _procsp(pc),a0\n"
"   subq.w  #1,4(a0)\n"
"   bpl.w   2f\n"
"   subq.l  #6,a0\n"
"   move.l  a0,_procsp\n"
"2: move.l  (a7)+,a0\n"
"   rte\n"
"maxerrtext: .asciz \"No currently used object!\"\n"
"   .even\n"
"braerrtext: .asciz \"Offset out of range\"\n"
"   .even\n"
"eoptext:    .asciz \"End of Program\"\n"
"   .even\n"
/* TRAP #2 - start of an interrupt routine: skipped (jump to the address
   in the parameter) while the debugger has interrupts off or the
   routine's flag word is negative; otherwise remember a7 and the
   parameter on intstack and run it in user mode */
"   .globl  _mytrap2,_mytrap3\n"
"_mytrap2:\n"
"   move.l  2(a7),a0\n"
"   tst.w   4(a0)\n"
"   bmi.w   1f\n"
"   move.w  _intsoff(pc),d0\n"
"   beq.w   2f\n"
"1: move.l  (a0),2(a7)\n"
"   rte\n"
"2: move.l  #3f,2(a7)\n"
"   rte\n"
"3: move.l  _intsp(pc),a1\n"
"   move.l  a7,(a1)+\n"
"   move.l  a0,(a1)+\n"
"   move.l  a1,_intsp\n"
"   addq.w  #6,a0\n"
"   jmp     (a0)\n"
/* TRAP #3 - exiting interupt routine */
"_mytrap3:\n"
"   subq.l  #8,_intsp\n"
"   rte\n"
);

/* TRAP #1 - new line: (user mode only) save the registers, ask the state
   handler (Z = go on); if it says stop or a flag is set, return to user
   mode at handleflags, else skip the 8 byte parameter and go on */
__asm__(
"   .globl  _mytrap1\n"
"_mytrap1:\n"
"   btst    #5,(a7)\n"
"   bne.w   1f\n"                       /* skip supervisor stuff! */
"   move.l  2(a7),trappc\n"
"   move.w  (a7),trapcc\n"
"   movem.l d0-d7/a0-a6,trapregs\n"
"   move.l  usp,a0\n"
"   move.l  a0,a7_\n"
"   lea     trapregs(pc),a0\n"
"   move.l  _statehandler(pc),a1\n"
"   jsr     (a1)\n"
"   bne.w   touser\n"
"   tst.b   _flags\n"
"   bne.w   touser\n"
"   movem.l a0_(pc),a0-a1\n"
"1: addq.l  #8,2(a7)\n"
"   rte\n"
"touser:\n"
"   move.l  #2f,2(a7)\n"
"   rte\n"
"2: jmp     _handleflags\n"              /* C below; never returns */
/* executestate */
"   .globl  _executestate\n"
"_executestate:\n"
"   movem.l trapregs(pc),d0-d7/a0-a7\n"
"   move.l  trappc(pc),-(a7)\n"
"   move.w  trapcc(pc),-(a7)\n"
"   addq.l  #8,2(a7)\n"
"   rtr\n"
"   .globl  _waitres\n"
"_waitres:  .word   0\n"
"trapregs:\n"
"d0_: .long 0\nd1_: .long 0\nd2_: .long 0\nd3_: .long 0\n"
"d4_: .long 0\nd5_: .long 0\nd6_: .long 0\nd7_: .long 0\n"
"a0_: .long 0\na1_: .long 0\na2_: .long 0\na3_: .long 0\n"
"a4_: .long 0\na5_: .long 0\na6_: .long 0\na7_: .long 0\n"
"trappc:    .long   0\n"
"trapcc:    .word   0\n"
);

/* TRAP #0 - runtime error.  User mode: the user error flag, then the
   flag loop (or, in direct mode, back to dirdone).  Supervisor mode
   (inside an interrupt routine): the super error flag, leave the
   interrupt routine (its flag word is set to -1 = never again) */
__asm__(
"   .globl  _mytrap0\n"
"_mytrap0:\n"
"   cmp.l   #1,d0\n"
"   bne.w   1f\n"
"   move.l  #maxerrtext,d0\n"
"1: cmp.l   #2,d0\n"
"   bne.w   2f\n"
"   move.l  #braerrtext,d0\n"
"2: cmp.l   #3,d0\n"
"   bne.w   3f\n"
"   move.l  #eoptext,d0\n"
"3: btst    #5,(a7)\n"
"   bne.w   4f\n"
"   move.l  d0,_usertext\n"
"   bset    #5,_flags\n"
"   tst.w   _dirmode\n"
"   beq     touser\n"
"   bra     touser2\n"
"4: move.l  d0,_supertext\n"
"   bset    #6,_flags\n"
"   move.l  #5f,2(a7)\n"
"   rte\n"
"5: move.l  _intsp(pc),a1\n"
"   move.l  -(a1),a0\n"
"   st      4(a0)\n"
"   move.l  -(a1),a7\n"
"   move.l  a1,_intsp\n"
"   move.l  (a0),a0\n"
"   jmp     (a0)\n"
"   .globl  _astring,_dirmode,_oldouth,_doutx\n"
"_astring:  .space  256+16\n"
"pstring:   .asciz  \"Print \"\n"
"   .even\n"
"_dirmode:  .word   0\n"
"_oldouth:  .long   0\n"
/* evaluate - a0 = expression: execute "Print "+expression with the
   output going to astring */
"evaluate:\n"
"   lea     pstring(pc),a1\n"
"   lea     _astring(pc),a2\n"
"1: move.b  (a1)+,(a2)+\n"
"   bne.w   1b\n"
"   subq.w  #1,a2\n"
"   move.l  #_douthandler,d0\n"
"   bra.w   exec2\n"
/* execute - a0 = text to execute: the compiler (TRAP #15, parameter
   block behind it) compiles and runs it in direct mode; the end comes
   back through TRAP #0 (touser2) to dirdone.  -> a0 = astring (output or
   error message) */
"execute:\n"
"   lea     _astring(pc),a2\n"
"   moveq   #0,d0\n"
"exec2:\n"
"   move.b  (a0)+,(a2)+\n"
"   bne.w   exec2\n"
"   tst.l   d0\n"
"   beq.w   1f\n"
"   move.l  d0,a0\n"
"   jsr     _blitz_tok+" BL_ASMTOK(LIB_inputoutputlib, 4) "\n"    /* ALibJsr */
"   move.l  d0,_oldouth\n"
"1: clr.w   _doutx\n"
"   move.l  _procsp(pc),a0\n"
"   move.l  (a0),a0\n"
"   move.l  (a0)+,t_flo\n"
"   move.l  (a0),t_fgl\n"
"   clr.l   t_err\n"
"   move.w  _oldmode(pc),d0\n"
"   smi     d0\n"
"   ext.w   d0\n"
"   move.w  d0,t_bmd\n"
"   bsr     _fixmode\n"
"   st      _dirmode\n"
"   trap    #15\n"                      /* execute the bastard! */
"t_str:     .long   _astring\n"         /* input string */
"t_flo:     .long   0\n"                /* firstlocal */
"t_fgl:     .long   0\n"                /* first global */
"t_err:     .long   0\n"                /* output error */
"t_bmd:     .word   0\n"                /* blitz mode! are we in blitz mode? */
"dirdone:\n"
"   sf      _dirmode\n"
"   bsr     _amigamode\n"
"   move.l  t_err(pc),d0\n"
"   beq.w   1f\n"
"   move.l  d0,a0\n"
"   lea     _astring(pc),a1\n"
"   move.w  #-1,_doutx\n"
"2: addq.w  #1,_doutx\n"
"   move.b  (a0)+,(a1)+\n"
"   bne.w   2b\n"
"1: move.l  _oldouth(pc),d0\n"
"   beq.w   3f\n"
"   clr.l   _oldouth\n"
"   move.l  d0,a0\n"
"   jsr     _blitz_tok+" BL_ASMTOK(LIB_inputoutputlib, 4) "\n"
"3: lea     _astring(pc),a0\n"
"   move.w  _doutx(pc),d0\n"
"   clr.b   0(a0,d0.w)\n"
"   rts\n"
"touser2:\n"
"   move.l  #dirdone,2(a7)\n"
"   rte\n"
"_doutx:    .word   0\n"
/* output handler for inputoutputlib: entry = output a byte, -4 = a string
   of length d0, -8 = a 0 terminated string */
"   bra.w   _dout3\n"
"   bra.w   _dout2\n"
"   .globl  _douthandler\n"
"_douthandler:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _douthandler_c\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
);

/* ------------------------------------------------- output into astring */

/* douthandler - output a byte (d0.b) */
static void BLITZ_ROUTINE douthandler_c(UBYTE c BL_REG(d0))
{
    if (doutx >= 256)
        return;
    astring[doutx++] = c;
}

/* dout2 - output a string of length d0.  BUG(orig) (#126): the length is only
   tested for 0, the copy runs until astring is full. */
static void BLITZ_ROUTINE dout2_c(UWORD len BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    UWORD pos;

    if (!len)
        return;
    for (pos = doutx; pos < 256; pos++)
        astring[pos] = *s++;
    doutx = pos;
}
BLITZ_THUNK(dout2, dout2_c, "d0-d1/a0-a1");

/* dout3 - output string until 0 */
static void BLITZ_ROUTINE dout3_c(const UBYTE *s BL_REG(a0))
{
    UWORD pos = doutx;

    while (pos < 256) {
        pos++;
        if (!(astring[pos - 1] = *s++)) {
            pos--;
            break;
        }
    }
    doutx = pos;
}
BLITZ_THUNK(dout3, dout3_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------- mode switch */

#define sw_goblitz   0xc200                     /* switchlib */
#define sw_goamiga   0xc201
#define sw_goqamiga  0xc202
#define sw_goblitzS  0xc206
#define sw_goqamigaS 0xc207

extern UWORD waitres;

/* amigamode - interrupts off, AMIGA; remember the old mode.  The
   original changes d0 only. */
static void BLITZ_ROUTINE amigamode_c(void)
{
    struct bl_regs r = { 0 };

    intsoff[0] = 0xff;                  /* ST intsoff */
    BLITZ_ALIBJSR(sw_goamiga, r);
    oldmode = r.d0;
}
BLITZ_THUNK(amigamode, amigamode_c, "d1/a0-a1");

/* fixmode - back to the old mode (BLITZ/QAMIGA; with waitres set
   BlitzS/QAmigaS: the display is not touched), interrupts on */
static void BLITZ_ROUTINE fixmode_c(void)
{
    struct bl_regs r = { 0 };

    if (oldmode) {
        if (waitres) {
            if ((WORD)oldmode < 0)
                BLITZ_ALIBJSR(sw_goblitzS, r);
            else
                BLITZ_ALIBJSR(sw_goqamigaS, r);
        } else {
            if ((WORD)oldmode < 0)
                BLITZ_ALIBJSR(sw_goblitz, r);
            else
                BLITZ_ALIBJSR(sw_goqamiga, r);
        }
    }
    intsoff[0] = 0;                     /* SF intsoff */
}
BLITZ_THUNK(fixmode, fixmode_c, "d1/a0-a1");

/* -------------------------------------------------------- handleflags */

/* labels of the assembler parts above */
/* the register image at the trap (MOVEM.l d0-d7/a0-a6,trapregs, then
   USP to a7_) */
struct trapregs {
    ULONG d[8];                                 /* d0_..d7_ */
    ULONG a[8];                                 /* a0_..a7_ */
};
static_assert(sizeof(struct trapregs) == 16 * 4);
extern struct trapregs trapregs __asm__("trapregs");
extern UBYTE *trappc __asm__("trappc");
extern char evaluate_[] __asm__("evaluate");    /* a0 = text -> a0 = output */
extern char execute_[] __asm__("execute");
void executestate(void) __attribute__((noreturn));

/* BCLR #bit,flags with its Z result.  Atomic (one instruction): the flags
   are also set from outside the program (e.g. the editor's stop key). */
static inline BOOL flagclr(int bit)
{
    UBYTE was0;
    __asm volatile("bclr %1,_flags\n\tseq %0"
                   : "=d"(was0) : "i"(bit) : "cc", "memory");
    return !was0;
}

/* JSR (a1) into a handler of the debugger front end (or evaluate/execute)
   with a0 = *arg; such code may change every register except a4/a5/a7.
   Returns d0, *arg receives a0. */
static inline ULONG callhandler(APTR fn, ULONG *arg)
{
    register ULONG ra0 __asm("a0") = *arg;
    register APTR ra1 __asm("a1") = fn;
    register ULONG rd0 __asm("d0");
    __asm volatile("jsr (%2)"
                   : "+a"(ra0), "=d"(rd0), "+a"(ra1) :
                   : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6",
                     "cc", "memory");
    *arg = ra0;
    return rd0;
}

/* handleflags - the debugger's state machine.  Reached in USER mode: a
   trap handler (TRAP #1 new line, TRAP #0 runtime error) changed the return
   PC of its exception frame to here (touser), the program's registers are
   saved in trapregs/trappc/trapcc.  Every set flag bit is handled once,
   then the wait handler (front end) decides: more flags -> again, else
   back into the program through executestate (registers restored, RTR).
     bit 7  End the program (exitslib $C800, with the program's a5)
     bit 6  supervisor error: superhandler(supertext)
     bit 5  user error:       userhandler(usertext)
     bit 4  evaluate:  evalhandler(evaluate(evaltext)) with program a4/a5
     bit 3  execute:   execute(exectext), output (if any) to evalhandler
     bit 2  skip: jump over the current statement (word at trappc+6 =
            distance, 0 = not possible) and leave at once
   The original is assembler (blitz2 debuglib binary); this is the same
   sequence in C, entered by `jmp _handleflags` from the trap code. */
void BLITZ_ROUTINE __attribute__((noreturn)) handleflags(void)
{
    ULONG arg;                                  /* a0 */

    amigamode_c();
    do {                                        /* handleflags: */
        if (flagclr(7)) {
            struct bl_regs r = { 0, 0, 0, 0 };
            blitz_globals = (UBYTE *)trapregs.a[5];
            BLITZ_ALIBJSR(TOK_endjmp, r);       /* does not return */
        }
        if (flagclr(6)) {
            arg = (ULONG)supertext;
            callhandler(superhandler, &arg);
        }
        if (flagclr(5)) {
            arg = (ULONG)usertext;
            callhandler(userhandler, &arg);
        }
        if (flagclr(4)) {
            blitz_locals = (UBYTE *)trapregs.a[4];  /* MOVEM.l a4_,a4-a5 */
            blitz_globals = (UBYTE *)trapregs.a[5];
            arg = (ULONG)evaltext;
            callhandler(evaluate_, &arg);
            callhandler(evalhandler, &arg);
        }
        if (flagclr(3)) {
            blitz_locals = (UBYTE *)trapregs.a[4];
            blitz_globals = (UBYTE *)trapregs.a[5];
            arg = (ULONG)exectext;
            callhandler(execute_, &arg);
            if (*(UBYTE *)arg)                  /* any output? */
                callhandler(evalhandler, &arg);
        }
        if (flagclr(2)) {
            WORD dist = *(WORD *)(trappc + 6);
            if (dist) {
                trappc = trappc + dist - 10;    /* ADD.w d0,a0: LEA -10(a0) */
                break;                          /* -> tofix */
            }
        }
        arg = (ULONG)waithandler;               /* MOVE.l _waithandler,a0 */
        waitres = (UWORD)callhandler(waithandler, &arg);
    } while (flags[0]);                         /* TST.b _flags */
    fixmode_c();                                /* tofix */
    executestate();
}

/* ------------------------------------------------------- init / finit */

static const char starttext[] = "Interrupted - program start";

/**
 * @brief Library init: resets the debugger state and installs the trap
 *        handlers.
 *
 * Clears the flags, direct mode and the output redirection, resets the
 * procedure and interrupt stacks and the default handlers, and sets the
 * trap vectors #0-#5 in the VBR table (runerrlib getvbr).  The old TRAP #1
 * vector is the editor's token list (tokens).  With d0.b = 0 the program
 * stops at once ("Interrupted - program start"; the binary, not the
 * source).
 *
 * @param autorun  d0: low byte non zero for auto run, 0 = stop at the
 *                 first line
 */
static void BLITZ_ROUTINE codeinit_c(ULONG autorun BL_REG(d0))
{
    struct bl_regs r = { 0 };
    APTR *vbr;

    dirmode = 0;
    oldouth = NULL;
    intsoff[0] = intsoff[1] = 0;
    oldmode = 0;
    *(volatile ULONG *)flags = 0;
    if (!(UBYTE)autorun) {
        usertext = starttext;
        flags[0] |= 0x20;
    }
    intsp = intstack;
    procsp = procstack;
    statehandler = statehandler_;
    superhandler = superhandler_;
    userhandler = userhandler_;
    waithandler = waithandler_;
    evalhandler = evalhandler_;
    BLITZ_ALIBJSR(TOK_getvbr, r);
    vbr = (APTR *)r.a0;
    tokens = vbr[33];                   /* 132(a0) */
    vbr[32] = mytrap0;                  /* TRAP #0 = 128(vbr) */
    vbr[33] = mytrap1;
    vbr[34] = mytrap2;
    vbr[35] = mytrap3;
    vbr[36] = mytrap4;
    vbr[37] = mytrap5;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/**
 * @brief Library finit: nothing (the trap vectors stay set).
 */
void BLITZ_ROUTINE codefinit(void)
{
}

/* -------------------------------------------------------------- tokens */

/* The flag setters keep all registers.  The flags are tested by the trap
   handlers: each is one read-modify-write instruction (bset/or.b). */

/**
 * @brief $D501 dostop: stops the program at the next new line.
 *
 * Sets the user error flag with the text "***** Interupted *****"; all
 * registers kept.
 */
static void BLITZ_ROUTINE dostop_c(void)
{
    usertext = "***** Interupted *****";
    flags[0] |= 0x20;
}
BLITZ_THUNK(dostop, dostop_c, "d0-d1/a0-a1");

/**
 * @brief $D504 doevaluate: evaluates an expression at the next new line.
 *
 * Sets the evaluate flag: handleflags runs "Print "+expression with the
 * output going to astring and passes it to the eval handler.  All
 * registers kept.
 *
 * @param text  a0: the expression
 */
static void BLITZ_ROUTINE doevaluate_c(const char *text BL_REG(a0))
{
    evaltext = text;
    flags[0] |= 0x10;
}
BLITZ_THUNK(doevaluate, doevaluate_c, "d0-d1/a0-a1");

/**
 * @brief $D505 doexecute: executes a line of Blitz code at the next new
 *        line.
 *
 * Sets the execute flag: handleflags lets the compiler run the text in
 * direct mode and passes any output or error message to the eval
 * handler.  All registers kept.
 *
 * @param text  a0: the text to execute
 */
static void BLITZ_ROUTINE doexecute_c(const char *text BL_REG(a0))
{
    exectext = text;
    flags[0] |= 0x08;
}
BLITZ_THUNK(doexecute, doexecute_c, "d0-d1/a0-a1");

/**
 * @brief $D506 doquit: ends the program at the next new line.
 *
 * Sets the end flag (handleflags: exitslib's end with the program's a5).
 * Also the default super/user error handlers.  All registers kept.
 */
static void BLITZ_ROUTINE doquit_c(void)
{
    flags[0] |= 0x80;
}
BLITZ_THUNK(doquit, doquit_c, "d0-d1/a0-a1");

/**
 * @brief $D508 doskip: skips the current statement at the next new line.
 *
 * Sets the skip flag: handleflags jumps over the statement if the word
 * at trappc+6 (its distance) is not 0.  All registers kept.
 */
static void BLITZ_ROUTINE doskip_c(void)
{
    flags[0] |= 0x04;
}
BLITZ_THUNK(doskip, doskip_c, "d0-d1/a0-a1");

/**
 * @brief $D500 donewmode: nothing.
 */
void BLITZ_ROUTINE donewmode(void)
{
}

/**
 * @brief $D509 docall: calls handler 6 of the front end's table.
 *
 * New in the binary (not in the source).  All registers are passed
 * through to the handler and back (a1 = the handler's address).
 */
BLITZ_ASM_ROUTINE(docall,
"   move.l  _h6(pc),a1\n"
"   jsr     (a1)\n"
"   rts");

/* the debugger front end's handler table (dosethandler a0) */
struct handlers {
    APTR state, super, user, wait, eval;
    APTR unused;                        /* entry 5: not read */
    APTR call;                          /* h6: docall */
    APTR proc;                          /* h7: procedure start/end */
};
static_assert(sizeof(struct handlers) == 8 * 4);

/**
 * @brief $D502 dosethandler: installs the debugger front end's handlers.
 *
 * C part: copies the table (state, super, user, wait, eval, -, docall,
 * procedure) into the handler variables and remembers the procedure flag
 * word (TRAP #4/#5 call the procedure handler while it is non zero).  The
 * assembler wrapper below also returns a0/a1/a2 (the binary, not the
 * source).
 *
 * @param pflag  d0: pointer to the procedure flag word
 * @param tab    a0: the handler table
 * @return d0: the old handler table
 */
static APTR BLITZ_ROUTINE dosethandler_c(UWORD *pflag BL_REG(d0),
                                         const struct handlers *tab BL_REG(a0))
{
    APTR old = handler;

    procflag = pflag;
    handler = (APTR)tab;
    statehandler = tab->state;
    superhandler = tab->super;
    userhandler = tab->user;
    waithandler = tab->wait;
    evalhandler = tab->eval;
    h6 = tab->call;
    h7 = tab->proc;
    return old;
}
/* d0 = old table, a0 = trap registers, a1 = &oldmode, a2 = tokens; the
   original keeps d1-d5 (and all others but a0-a2) */
BLITZ_ASM_ROUTINE(dosethandler,
"   move.l  d1,-(sp)\n"
"   jsr     _dosethandler_c\n"
"   move.l  (sp)+,d1\n"
"   lea     trapregs(pc),a0\n"
"   lea     _oldmode(pc),a1\n"
"   move.l  _tokens,a2\n"
"   rts");

/* name of token `token` in the editor's list, "?????" if unknown */
static const char *tokename(UWORD token)
{
    struct toke *k;

    for (k = tokens; k; k = k->next)
        if (k->num == token)
            return k->name;
    return "?????";
}

/**
 * @brief $D503 dodetoke: detokenises a line (a0 -> a1).
 *
 * A byte with bit 7 set starts a token (2 bytes, bit 15 cleared = the
 * number), which is replaced by its name from the editor's token list
 * ("?????" if unknown).  Out: a0/a1 behind the terminating 0 (the wrapper
 * passes them in memory: p[0] = a0, p[1] = a1).
 *
 * @param p  a0: p[0] = the tokenised line (a0), p[1] = the destination
 *           (a1); both are returned there
 */
static void BLITZ_ROUTINE dodetoke_c(UBYTE **p BL_REG(a0))
{
    UBYTE *s = p[0], *dst = p[1], c;
    const char *name;

    for (;;) {
        c = *s++;
        *dst++ = c;
        if (!c)
            break;
        if (!(c & 0x80))
            continue;
        dst--;                          /* MOVE.b -(a1),d0 */
        name = tokename((UWORD)(((c << 8) | *s++) & 0x7fff));
        while ((*dst++ = *name++) != 0)
            ;
        dst--;
    }
    p[0] = s;
    p[1] = dst;
}
BLITZ_ASM_ROUTINE(dodetoke,
"   move.l  a1,-(sp)\n"
"   move.l  a0,-(sp)\n"
"   move.l  sp,a0\n"
"   jsr     _dodetoke_c\n"
"   move.l  (sp)+,a0\n"
"   move.l  (sp)+,a1\n"
"   rts");

/**
 * @brief $D507 dofindoff: converts a position in a tokenised line into
 *        the position in the detokenised text.
 *
 * BUG(orig) (#127): the loop end test (BEQ at 'loop) sees the flags of
 * the last instruction: after a normal character `SUBQ #1,d0`, after a
 * token `SUBQ #1,d1` (the result, not the position); the position is
 * lowered by 2 for a token without a test, so a position inside a token
 * runs on to the end of the string.
 *
 * @param s    a0: the tokenised line
 * @param pos  d0: the position in it (word)
 * @return d0: the position in the detokenised text (word; the thunk
 *         writes only the low word of the saved d0)
 */
static UWORD BLITZ_ROUTINE dofindoff_c(const UBYTE *s BL_REG(a0), UWORD pos BL_REG(d0))
{
    UWORD off = 0;
    int zflag = (pos == 0);             /* TST d0 */
    UBYTE c;
    const char *name;

    while (!zflag) {
        c = *s++;
        if (!c)
            break;
        if (!(c & 0x80)) {
            off++;
            zflag = (--pos == 0);
            continue;
        }
        pos -= 2;
        name = tokename((UWORD)(((c << 8) | *s++) & 0x7fff));
        while (*name++)
            off++;
        zflag = (off == 0);                 /* ADDQ ... SUBQ #1,d1 */
    }
    return off;
}
BLITZ_THUNK_POST(dofindoff, dofindoff_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/* unreferenced, as in the binary: makes the compiler include printlib */
__asm__(
"   .text\n"
"   jsr     _blitz_tok+" BL_ASMTOK(LIB_printlib, 1) "\n"
"   jsr     _blitz_tok+" BL_ASMTOK(LIB_printlib, 1) "\n"
);
