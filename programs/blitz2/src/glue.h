/*
 * glue.h - the parts that cannot be plain C: register based calling
 * conventions (editor <-> compiler, generated code, mathffp by LVO),
 * supervisor/CPU access.  All inline assembler lives in glue*.c.
 */
#ifndef BLITZ2_GLUE_H
#define BLITZ2_GLUE_H

#include <exec/types.h>

/* Built with -fbaserel, C code addresses its globals relative to a4.  Every
   asm entry point that callers outside the C code reach (editor, exec, trap)
   loads a4 before calling C; the original a4 is restored with the frame. */
#ifdef __baserel__
#define GLUE_LOAD_A4 "   lea     ___a4_init,a4\n"
#else
#define GLUE_LOAD_A4 ""
#endif

/* ------------------------------------------------------- register frames */

/* A complete 68k register set.  The layout equals what
   `movem.l d0-d7/a0-a6,-(sp)` leaves on the stack, so asm entry stubs can
   hand their stack frame to C as a struct m68kregs *. */
struct m68kregs {
    ULONG d[8];
    ULONG a[7];
};

/* condition code bits */
#define CCR_C 1
#define CCR_V 2
#define CCR_Z 4
#define CCR_N 8
#define CCR_X 16    /* never set by callregs(): Scc cannot test X (no caller needs it) */

/* Call `fn` with d0-d7/a0-a6 loaded from *r (a5 and a6 included).  After
   the call all registers are written back to *r; the function result is the
   CCR the callee returned with (e.g. test `& CCR_Z`).  Any register may be
   clobbered by the callee.  Used for editor callbacks, the editor entry,
   running generated code and calling library LVOs. */
UWORD callregs(APTR fn, struct m68kregs *r);

/* Library vector by negative offset, e.g. LVO(MathBase, -66) = SPAdd. */
#define LVO(base, off) ((APTR)((UBYTE *)(base) + (off)))

/* mathffp call with raw FFP bit patterns: returns d0; *ccr (may be NULL)
   receives the CCR (SPFix uses V for overflow, SPCmp returns via CCR).
   `lvo` is the negative offset: -30 SPFix, -36 SPFlt, -42 SPCmp, -48 SPTst,
   -54 SPAbs, -60 SPNeg, -66 SPAdd, -72 SPSub, -78 SPMul, -84 SPDiv. */
ULONG ffpcall(WORD lvo, ULONG arg1, ULONG arg2, UWORD *ccr);

/* ------------------------------------------- callbacks called BY the editor */

/* Asm entry stubs: save all registers, call the C handler with the saved
   frame (the handler reads/modifies r->d[]/r->a[]), restore all registers
   (with the handler's modifications) and return with `tst.l d0` flags.
   The stubs' addresses go into edstruct.
     menu_stub     -> menu_c     (in: d6 item, a5 first line, d7 numlines;
                                  out: d0.w menu return code)
     loadxtra_stub -> loadxtra_c (in: d7 file handle, a6 DOSBase)
     savextra_stub -> savextra_c (in: d7 file handle, a6 DOSBase)
   stopcode_stub is an exec tc_ExceptCode: in d0 = signals, out d0. */
extern void menu_stub(void), loadxtra_stub(void), savextra_stub(void);
extern void stopcode_stub(void);
void menu_c(struct m68kregs *r);
void loadxtra_c(struct m68kregs *r);
void savextra_c(struct m68kregs *r);
void stopcode_c(struct m68kregs *r);

/* ------------------------------------------------------------- CPU */

/* VBR via movec in supervisor mode. 68010+ only - the caller (calcvbr)
   must check AttnFlags first, as the original did. */
ULONG getvbr(void);

#endif
