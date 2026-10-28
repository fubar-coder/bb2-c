/*
 * callregs - the register call thunk of the tests (see sysload.h).
 */
#include <exec/types.h>

/* callregs(fn, r, arg): push `arg` (if pusharg), load d0-d7/a0-a6 from r,
   call fn, store all registers back, pop arg.  Returns the CCR (Scc). */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _callregs\n"
"_callregs:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"    /* 44: fn 48, r 52, arg 56, push 60 */
"   move.l  52(sp),a1\n"
"   move.l  a1,-(sp)\n"             /* [r] */
"   tst.l   64(sp)\n"
"   beq.s   0f\n"
"   move.l  60(sp),-(sp)\n"         /* [arg][r] */
"   pea     1f\n"
"   move.l  60(sp),-(sp)\n"         /* fn: 48+4+4+4 */
"   bra.s   2f\n"
"0: pea     3f\n"
"   move.l  56(sp),-(sp)\n"
"2: movem.l (a1),d0-d7/a0-a6\n"
"   rts\n"
"1: addq.l  #4,sp\n"                /* drop arg; addq does not change CCR */
"3: scs     -(sp)\n"
"   seq     -(sp)\n"
"   smi     -(sp)\n"
"   move.l  a6,-(sp)\n"
"   move.l  10(sp),a6\n"            /* r */
"   movem.l d0-d7/a0-a5,(a6)\n"
"   move.l  (sp)+,56(a6)\n"
"   moveq   #0,d0\n"
"   tst.b   (sp)+\n"
"   beq.s   4f\n"
"   or.b    #8,d0\n"
"4: tst.b   (sp)+\n"
"   beq.s   5f\n"
"   or.b    #4,d0\n"
"5: tst.b   (sp)+\n"
"   beq.s   6f\n"
"   or.b    #1,d0\n"
"6: addq.l  #4,sp\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);
