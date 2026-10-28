/*
 * start.c - first code of a load file on the edstub frame.  MUST be the
 * first object on the link line: blitz2 calls BADDR(seglist)+8, i.e. byte 4
 * of the first hunk (byte 0..3 of BADDR(seglist) is the next-segment BPTR).
 *
 *   hunk+0  _start    moveq #20,d0 / rts  (started from the shell: fail)
 *   hunk+4  _outside  init entry (ted.s `outside`), a1 = edstruct
 *                     out: Z=1 ok, a0 = main entry, a1 = comdata
 *
 * Register frames: every entry pushes d0-d7/a0-a6 (struct regs, d0 at the
 * lowest address) and hands a pointer to it to a C handler; what the
 * handler writes into the frame becomes the returned registers.  So every
 * register the C side does not change is preserved.
 */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _start\n"
"_start:\n"
"   moveq   #20,d0\n"
"   rts\n"
"   .globl  _outside\n"
"_outside:\n"                           /* must be at hunk offset 4 */
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   move.l  sp,-(sp)\n"
"   jsr     _edstub_init_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   tst.l   d0\n"                       /* d0 = 0: Z=1 = ok */
"   rts\n"

/* main entry (a0 = file name).  Keeps the stack pointer for edstub_abort(). */
"   .even\n"
"   .globl  _edstub_tomain\n"
"_edstub_tomain:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   move.l  sp,_edstub_abort_sp\n"
"   move.l  sp,-(sp)\n"
"   jsr     _edstub_main_c\n"
"   addq.l  #4,sp\n"
"main_out:\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rts\n"

/* void edstub_abort(void): drop everything below the main entry frame and
   return to the compiler as if the editor had quit. */
"   .even\n"
"   .globl  _edstub_abort\n"
"_edstub_abort:\n"
"   move.l  _edstub_abort_sp,sp\n"
"   bra     main_out\n"

/* ULONG edstub_callregs(APTR fn, struct regs *r): load d0-d7/a0-a6 from
   *r, call fn, store all registers back.  For code that may clobber every
   register (compiler callbacks, TED routines). */
"   .even\n"
"   .globl  _edstub_callregs\n"
"_edstub_callregs:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"        /* 44 bytes; fn 48(sp), r 52(sp) */
"   move.l  52(sp),a1\n"
"   move.l  a1,-(sp)\n"                 /* [r] */
"   pea     1f\n"                       /* [ret][r] */
"   move.l  56(sp),-(sp)\n"             /* [fn][ret][r] */
"   movem.l (a1),d0-d7/a0-a6\n"
"   rts\n"                              /* jump to fn, returns to 1: */
"1: move.l  a6,-(sp)\n"
"   move.l  4(sp),a6\n"                 /* r */
"   movem.l d0-d7/a0-a5,(a6)\n"
"   move.l  (sp)+,56(a6)\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);

/* comdata entry stubs: name_stub -> name_c(struct regs *).  The final
   tst.l d0 gives ezfreq its Z flag (Z=1 = cancelled). */
#define REGSTUB(name)                       \
__asm__(                                    \
"   .text\n"                                \
"   .even\n"                                \
"   .globl  _" #name "_stub\n"              \
"_" #name "_stub:\n"                        \
"   movem.l d0-d7/a0-a6,-(sp)\n"            \
"   move.l  sp,-(sp)\n"                     \
"   jsr     _" #name "_c\n"                 \
"   addq.l  #4,sp\n"                        \
"   movem.l (sp)+,d0-d7/a0-a6\n"            \
"   tst.l   d0\n"                           \
"   rts\n"                                  \
)

REGSTUB(screenclose);
REGSTUB(screenopen2);
REGSTUB(openreq);
REGSTUB(closereq);
REGSTUB(getinput);
REGSTUB(error);
REGSTUB(detokallz);
REGSTUB(tokall);
REGSTUB(ezfreq);
REGSTUB(refgads);
REGSTUB(getinput2);
REGSTUB(tokone);
REGSTUB(openio);
REGSTUB(closeio);
REGSTUB(actgad);
REGSTUB(pointeron);
REGSTUB(sleepon);
REGSTUB(extalloc);
