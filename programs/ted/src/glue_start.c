/*
 * glue_start.c - the start of the code hunk (must be the FIRST object of
 * the link; -nostartfiles: no libnix startup in front of it).
 *
 *   hunk+0  bsr.w edstart_stub    (ted.s:22 `bsr edstart`: DOS starts a
 *                                  standalone TED here)
 *   hunk+4  outside_stub           (ted.s:24 `outside`: blitz2 calls
 *                                  `jsr 8(BADDR(seg))` with a1 = edstruct)
 *
 * Standalone, exactly like the original: DOS jumps to +0, edstart (wbheader)
 * reads the arguments and returns with a1 = 0 (ted.s:137 `sub.l a1,a1 /
 * rts`) into +4, so outside runs with a1 = 0 and its final rts returns to
 * DOS.  External: blitz2 calls +4, outside returns a0 = tomain_stub, a1 =
 * comdata, d0 = 0 (Z=1); later blitz2 calls tomain_stub with a0 = file name
 * and gets control back when the editor ends.
 *
 * All three stubs use the REGSTUB frame (glue.h struct m68kregs): the C
 * part may change the saved registers; they are restored with the changes
 * and `tst.l d0` sets the flags.  tools/checkhunk.py checks the layout.
 */
#include "ted.h"

__asm__(
"   .text\n"
"   .even\n"
"   .globl  _begin\n"
"_begin:\n"
"   bsr.w   _edstart_stub\n"        /* hunk+0, 4 bytes */
"   .globl  _outside_stub\n"
"_outside_stub:\n"                  /* hunk+4 */
"   movem.l d0-d7/a0-a6,-(sp)\n"
GLUE_LOAD_A4
"   move.l  sp,-(sp)\n"
"   jsr     _outside_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   tst.l   d0\n"
"   rts\n"
"\n"
"   .globl  _tomain_stub\n"
"_tomain_stub:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
GLUE_LOAD_A4
"   move.l  sp,-(sp)\n"
"   jsr     _tomain_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   tst.l   d0\n"
"   rts\n"
"\n"
/* edstart: called by the bsr at +0 with the DOS registers (d0/a0 = CLI
   arguments).  Returns into +4 with a1 = 0 (other registers as edstart_c
   left them in the frame). */
"_edstart_stub:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
GLUE_LOAD_A4
"   move.l  sp,-(sp)\n"
"   jsr     _edstart_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   sub.l   a1,a1\n"
"   rts\n"
);
