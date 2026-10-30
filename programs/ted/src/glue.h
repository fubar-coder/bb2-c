/*
 * glue.h - the parts of TED that cannot be plain C: the hunk entry
 * (outside at hunk+4), the comdata register entries, callbacks into blitz2,
 * the VBlank interrupt server.  All inline assembler lives in glue*.c
 * (glue_start.c: hunk start and entries; glue.c: callregs, REGSTUBs,
 * callbacks, VBlank stub).  Pattern and callregs taken over from
 * ../blitz2-c/src/glue.h.  Owner: coordinator.
 */
#ifndef TED_GLUE_H
#define TED_GLUE_H

#include <exec/types.h>
#include "tedtypes.h"

/* Built with -fbaserel, C code addresses its globals relative to a4.  Every
   asm entry point reached from outside the C code (blitz2, DOS, exec
   interrupt) loads TED's a4 before calling C; the caller's a4 is restored
   with the frame. */
#ifdef __baserel__
#define GLUE_LOAD_A4 "   lea     ___a4_init,a4\n"
#define GLUE_VAR(x)  "_" x ":W(a4)"     /* a global of TED in asm */
#else
#define GLUE_LOAD_A4 ""
#define GLUE_VAR(x)  "_" x
#endif

/* ------------------------------------------------------- register frames */

/* A complete 68k register set, laid out as `movem.l d0-d7/a0-a6,-(sp)`
   leaves it on the stack (asm entry stubs hand their frame to C). */
struct m68kregs {
    ULONG d[8];
    ULONG a[7];
};

#define CCR_C 1
#define CCR_V 2
#define CCR_Z 4
#define CCR_N 8

/* Call `fn` with d0-d7/a0-a6 from *r; all registers are written back to *r,
   the result is the CCR the callee returned with.  (blitz2-c/src/glue.c) */
UWORD callregs(APTR fn, struct m68kregs *r);

/* `divu.w src,d0` (glue.c): (remainder << 16) | quotient, d0 unchanged on
   overflow, zero divide trap on src = 0 - the real instruction.  Also
   declared in core/fileio.h (the host build defines it there in C). */
ULONG divu_w(ULONG d0, UWORD src);

/* ted.s:3046 outmem with memstack == 0 (glue.c): `move.l memstack,a7 /
   bra memcont` literally - a7 = 0, then memcont (printmem if memalt,
   showmice if mousealt, `move.b (a7)+,d0` = the byte at address 0,
   showvprop if != 0) and `bra mainedit`.  Crashes like the original (the
   first return pops its address from the top of the address space).
   Called by outmem (mouse.c) only when memstack is NULL (open point O1). */
NORETURN void memcont_nostack(void);

/* ------------------------------------------------ hunk start (glue_start.c) */

/* Hunk offset 0: `bsr edstart` (standalone: DOS jumps here; edstart returns
   with a1 = 0 into offset 4).  Hunk offset 4: outside_stub - blitz2 calls
   `jsr 8(BADDR(seg))` = hunk+4 with a1 = edstruct.  tools/checkhunk.py
   checks the layout after every build (glue_start.c must be the first
   object, no startup code: -nostartfiles). */
extern void outside_stub(void);
/* ted.s:24 outside, C part (src/main.c): r->a[1] = edstruct or 0.
   External: sets r->a[0] = tomain_stub, r->a[1] = comdata, r->d[0] = 0
   (Z=1 - also after a failure, BUG(orig) #357, survey-01 I.1).  Standalone:
   runs the whole editor (.edmain -> .tomain) and returns. */
void outside_c(struct m68kregs *r);
/* ted.s:87 outside.tomain: blitz2's second call, a0 = file name.  C part:
   tomain_c in main.c (a0toname gets r->d[0], survey-03 I.1). */
extern void tomain_stub(void);
void tomain_c(struct m68kregs *r);
/* ted.s:136 edstart (wbheader): standalone start, C part in main.c:
   d0/a0 = CLI arguments.  Opens dos.library (DOSBase), fills progfile. */
void edstart_c(struct m68kregs *r);

/* ---------------------------------------------- comdata entries (glue.c) */

/* REGSTUB(name): asm entry `name_stub` saves d0-d7/a0-a6, loads TED's a4,
   calls name_c(frame) (glue.c: unpacks the registers, calls the C routine,
   writes results into the frame), restores all registers incl. the
   results, `tst.l d0`, rts.  The originals destroy more registers than
   this; keeping them is allowed (survey-06 B). */
extern void screenclose_stub(void), screenopen2_stub(void), openreq_stub(void),
    closereq_stub(void), getinput_stub(void), error_stub(void),
    detokallz_stub(void), tokall_stub(void), ezfreq_stub(void),
    refgads_stub(void), getinput2_stub(void), colors_stub(void),
    halfcolors_stub(void), tokone_stub(void), openio_stub(void),
    closeio_stub(void), actgad_stub(void), pointeron_stub(void),
    sleepon_stub(void), extalloc_stub(void);

/* ------------------------------------------------ callbacks into blitz2 */

/* ted.s:1542 extmenu: jsr (menurout) with d6.w = item, a5 = firstitem,
   d7.w = numlines (a4 = thisitem as in the asm); returns d0.w (-1 nothing,
   -2 showall, else a line).  May not return at all: blitz2's menu can call
   extalloc -> outmem (longjmp over blitz2's frames, BUG(orig) #369). */
WORD call_menurout(UWORD d6);
/* ted.s:2771 doclean: jsr (cleanup), all registers saved */
void call_cleanup(void);
/* call_loadxtra / call_savextra: hooks.h */

/* -------------------------------------------- VBlank server (glue.c) */

/* ted.s:306 myint.is_Code: stub saves a4, loads TED's a4, calls
   myroutine() (main.c), returns d0 = 0 / Z=1 (server chain continues). */
extern void myint_stub(void);

#endif
