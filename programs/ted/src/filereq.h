/*
 * filereq.h - the file requester (ted.s:7967 `filerequest`, incbin of the
 * filereq hunk, entry code+4) - the C requester core shared with libs-c
 * (../../shared/filereq/filereq_core.c, compiled as a source of ted).
 */
#ifndef TED_FILEREQ_H
#define TED_FILEREQ_H

#include <exec/types.h>

/* filerequest+36 (= code+4, ted.s:3507 `bra filerequest+36` from ezfreq):
   a0 = parameter block {path, name, title, screen} (reqstruct).  Returns
   the full name (inside the requester's data block) or 0 (cancel; asm
   d0 with Z=1). */
UBYTE *filerequest(const ULONG *a0);

#endif
