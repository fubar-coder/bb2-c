/*
 * glue_direct.c - direct mode trap (blitz2.s 3805-3877, chunk 03).
 *
 * The debugger / runtime of a compiled program executes
 *     trap #15
 *     dc.l code, firstlocal, firstglobal, returned
 *     dc.w blitzmode
 * to compile one line of BASIC in "direct mode" and run it.
 *
 * directtrap (supervisor, vector VBR+$bc installed at blitz2.s 1705):
 *   skips the 18 parameter bytes, keeps PC/SR in ret15add/ret15cc and
 *   returns with rte to directdo (in the caller's user mode).
 * directdo: builds an rtr frame [ccr.w, ret15add.l], saves d0-d7/a0-a6 and
 *   calls the C part directdo_c(frame) (stmts.c), which compiles the line
 *   into the direct mode buffer.  A non-zero result replaces the frame's
 *   return address: rtr then enters the generated code, which ends with
 *   `jmp ret15add`.  On error directdo_c returns 0 (error text stored in
 *   the parameter block) and rtr returns straight to ret15add.
 * direxit: restores the ccr into the frame, the registers, rtr.
 *   (flushc is called by directdo_c before it returns.)
 */
#include "blitz2.h"

__asm__(
"   .text\n"
"   .even\n"
"   .globl  _directtrap\n"
"_directtrap:\n"
"   add.l   #18,2(sp)\n"
"   move.l  2(sp),_ret15add\n"
"   move.w  (sp),_ret15cc\n"
"   move.l  #_directdo,2(sp)\n"
"   rte\n"
"\n"
"   .globl  _directdo\n"
"_directdo:\n"
"   move.l  _ret15add,-(sp)\n"
"   move.w  _ret15cc,-(sp)\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"    /* 15*4 */
GLUE_LOAD_A4                        /* a4 belongs to the compiled program */
"   move.l  sp,-(sp)\n"
"   jsr     _directdo_c\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   beq.s   1f\n"
"   move.l  d0,4*15+2(sp)\n"        /* install code address */
"1:\n"                              /* direxit */
"   move.w  _ret15cc,4*15(sp)\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rtr\n"
);
