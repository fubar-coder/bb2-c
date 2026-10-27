/*
 * filereq.c - the file requester of TED (ted.s:7967 `filerequest` + the
 * runtime relocation `flink`, ted.s:7953).
 *
 * The original embeds the hunk file `filereq` (no source; the same code as
 * the requester of freqlib) and relocates it itself at startup.  Here the
 * requester is the C core shared with the command library freqlib:
 * ../../shared/filereq/filereq_core.c ("Variante C": one source, compiled by
 * both programs with their own options; TED's environment for it is
 * src/filereq_env.h).
 *
 * DIFF: the data block is the freqlib version - TED's embedded copy differs
 * in 7 bytes, all padding behind gadget texts ("OK",0,$06 instead of $00)
 * and the rest at the end of the hunk; never read (reference.md 3.2).
 * Listed in notes/abweichungen.md.
 */
#include <exec/types.h>
#include "filereq.h"
#include "filereq_core.h"

/* ted.s:7953 flink - DIFF: not needed.  The original adds the load address
   of the embedded hunk to its 312 RELOC32 longs at every outside (no cache
   flush; a second run would relocate twice, BUG(orig) survey-07 I.4).  The
   C requester is ordinary code and data: LoadSeg relocates it.  Row in
   notes/abweichungen.md (category T). */

/* ted.s:7967 filerequest (+36 = code+4: movem.l d1-a6,-(sp) / a5 = a0) */
UBYTE *filerequest(const ULONG *a0)
{
    fq_args[0] = a0[0];         /* path buffer */
    fq_args[1] = a0[1];         /* name buffer */
    fq_args[2] = a0[2];         /* title */
    fq_args[3] = a0[3];         /* screen */
    return (UBYTE *)fq_request();
}
