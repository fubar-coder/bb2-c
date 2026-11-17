/*
 * glue.c - register calling conventions and CPU access (see glue.h).
 *
 * This file and other glue*.c files are the only places with assembler.
 * Everything is 68000 code (MIT syntax, gcc symbols with leading '_').
 */
#include <proto/exec.h>
#include "blitz2.h"

/* UWORD callregs(APTR fn, struct m68kregs *r)
 *
 * Loads d0-d7/a0-a6 from *r, calls fn, stores all registers back and
 * returns the CCR the callee left.  `move sr,<ea>` is privileged on
 * 68010+ and `move ccr,<ea>` does not exist on the 68000, so the flags are
 * captured with Scc instructions before anything else can change them. */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _callregs\n"
"_callregs:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"    /* 44 bytes; fn at 48(sp), r at 52(sp) */
"   move.l  52(sp),a1\n"
"   move.l  a1,-(sp)\n"             /* [r] */
"   pea     1f\n"                   /* [ret][r] */
"   move.l  56(sp),-(sp)\n"         /* [fn][ret][r] */
"   movem.l (a1),d0-d7/a0-a6\n"
"   rts\n"                          /* 'call' fn, it returns to 1: */
"1: scs     -(sp)\n"                /* each byte push moves sp by 2 */
"   svs     -(sp)\n"
"   seq     -(sp)\n"
"   smi     -(sp)\n"                /* [N][Z][V][C][r] */
"   move.l  a6,-(sp)\n"
"   move.l  12(sp),a6\n"            /* a6 = r */
"   movem.l d0-d7/a0-a5,(a6)\n"
"   move.l  (sp)+,56(a6)\n"         /* r->a[6] */
"   moveq   #0,d0\n"
"   tst.b   (sp)+\n"
"   beq.s   2f\n"
"   or.b    #8,d0\n"                /* N */
"2: tst.b   (sp)+\n"
"   beq.s   3f\n"
"   or.b    #4,d0\n"                /* Z */
"3: tst.b   (sp)+\n"
"   beq.s   4f\n"
"   or.b    #2,d0\n"                /* V */
"4: tst.b   (sp)+\n"
"   beq.s   5f\n"
"   or.b    #1,d0\n"                /* C */
"5: addq.l  #4,sp\n"                /* drop r */
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);

/* Entry stubs for code that is called with register arguments by the
   editor or by exec.  The stack frame after the movem is a struct
   m68kregs; the C handler may change it, the changes become the return
   registers. */
#define REGSTUB(name)                                   \
__asm__(                                                \
"   .text\n"                                            \
"   .even\n"                                            \
"   .globl  _" #name "_stub\n"                          \
"_" #name "_stub:\n"                                    \
"   movem.l d0-d7/a0-a6,-(sp)\n"                        \
GLUE_LOAD_A4                                            \
"   move.l  sp,-(sp)\n"                                 \
"   jsr     _" #name "_c\n"                             \
"   addq.l  #4,sp\n"                                    \
"   movem.l (sp)+,d0-d7/a0-a6\n"                        \
"   tst.l   d0\n"                                       \
"   rts\n"                                              \
)

REGSTUB(menu);
REGSTUB(loadxtra);
REGSTUB(savextra);
REGSTUB(stopcode);

ULONG ffpcall(WORD lvo, ULONG arg1, ULONG arg2, UWORD *ccr)
{
    struct m68kregs r = {{0}};
    UWORD cc;

    r.d[0] = arg1;
    r.d[1] = arg2;
    r.a[6] = (ULONG)MathBase;
    cc = callregs(LVO(MathBase, lvo), &r);
    if (ccr)
        *ccr = cc;
    return r.d[0];
}

/* supervisor part of getvbr: movec vbr,d0 / rte */
extern ULONG vbr_super(void);
__asm__(
"   .text\n"
"   .even\n"
"_vbr_super:\n"
"   .word   0x4e7a,0x0801\n"        /* movec vbr,d0 (68010+) */
"   rte\n"
);

ULONG getvbr(void)
{
    return Supervisor((ULONG (*)())vbr_super);
}

/* ULONG stackcall(ULONG (*fn)(APTR), APTR arg, struct StackSwapStruct *s)
 *
 * SWAPSTACK exchanges tc_SPLower/tc_SPUpper of the task (a0) with
 * stk_Lower/stk_Upper of *s (a3) and sp with stk_Pointer, as exec's
 * StackSwap does; called under Disable.  Done twice, the second time
 * restores the task's stack and leaves *s as it was given. */
#define SWAPSTACK                                       \
"   move.l  276(a6),a0\n"           /* ThisTask */      \
"   move.l  58(a0),d0\n"            /* tc_SPLower */    \
"   move.l  (a3),58(a0)\n"                              \
"   move.l  d0,(a3)\n"                                  \
"   move.l  62(a0),d0\n"            /* tc_SPUpper */    \
"   move.l  4(a3),62(a0)\n"                             \
"   move.l  d0,4(a3)\n"                                 \
"   move.l  8(a3),d0\n"             /* stk_Pointer */   \
"   move.l  sp,8(a3)\n"                                 \
"   move.l  d0,sp\n"
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _stackcall\n"
"_stackcall:\n"
"   movem.l d2/a2-a3/a6,-(sp)\n"    /* 16 bytes; fn at 20(sp), arg 24, s 28 */
"   move.l  20(sp),a2\n"
"   move.l  24(sp),d2\n"
"   move.l  28(sp),a3\n"
"   move.l  4.w,a6\n"
"   jsr     -120(a6)\n"             /* Disable */
SWAPSTACK
"   jsr     -126(a6)\n"             /* Enable */
"   move.l  d2,-(sp)\n"
"   jsr     (a2)\n"                 /* fn(arg); d2/a2-a6 survive the call */
"   addq.l  #4,sp\n"
"   move.l  d0,d2\n"
"   jsr     -120(a6)\n"             /* Disable */
SWAPSTACK
"   jsr     -126(a6)\n"             /* Enable */
"   move.l  d2,d0\n"
"   movem.l (sp)+,d2/a2-a3/a6\n"
"   rts\n"
);
