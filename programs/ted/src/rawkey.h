/*
 * rawkey.h - rawed.obj (ted.s:7109, incbin, no source) in C: console.device
 * RawKeyConvert.  Offsets relative to rawed+32 (reference.md 3.1).  The
 * special key mapping of convert is core: rawspecial (keymap.h).
 */
#ifndef TED_RAWKEY_H
#define TED_RAWKEY_H

#include <exec/types.h>

/* rawed+32+0 (called at ted.s:39): open console.device (result unchecked,
   BUG(orig)), build the two 105 byte tables (never read, BUG(orig)) */
void rawed_init(void);
/* rawed+32+4 (called at ted.s:5928): d0 = raw key (key down), d1 =
   qualifier -> character (0 = none) */
UWORD rawed_convert(UWORD d0, UWORD d1);
/* rawed+32+8 (called from the fail chain, ted.s line 128): CloseDevice (signal not freed) */
void rawed_finit(void);

#endif
