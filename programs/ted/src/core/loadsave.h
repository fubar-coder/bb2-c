/*
 * loadsave.h - chunk 01 core (ted.s 1-1015): blocktab/blockuntab, .xtra/
 * .info, the load loop fileopen2, the write loop realsave, search/replace,
 * print formatting.  UI part of chunk 01: src/main.h.
 *
 * Event handlers (called from jumptable/menutable by mainedit/menu, asm:
 * a4 = thisitem, d7 = event code) are `void name(void)`: read thisitem at
 * entry (`struct item *a4 = thisitem;`).
 */
#ifndef TED_LOADSAVE_H
#define TED_LOADSAVE_H

#include "tedcore.h"

/* ted.s:327 blocktab - menu $91: indent every line of the block by tabstop
   spaces (detok/makefirst per line), calcitem, showall.  No block: beepem. */
void blocktab(void);
/* ted.s:361 blockuntab - menu $a1: remove the first tabstop characters of
   every block line, WHATEVER they are (BUG(orig), survey-01 I.11). */
void blockuntab(void);

/* ted.s:570 (prtfile, core part - NEW name): format line a4 for the printer
   into buf: token -> ESC[1m name ESC[22m, end of line -> LF.  Returns the
   number of bytes (asm: Write length).  Bytes >= $80 are always tokens
   (survey-01 I.14).  The requester/Open/Write loop is prtfile (main.h). */
UWORD prtfile_line(struct item *a4, UBYTE *buf);

/* ----------------------------------------------------- .xtra / .info */

extern const char xtratxt[];    /* ted.s:638 '.xtra' */
extern const char infotxt[];    /* ted.s:639 '.info' */
extern const char bakname[];    /* ted.s:742 '.bak' (filesave, main.c) */

/* ted.s:641 openinfo: name(xtraat)+".info" exists (Lock) -> 0; else
   Open(..., NEWFILE).  asm: d7 = handle, Z=1 if 0. */
tedfh openinfo(void);
/* ted.s:658 openxtra: Open(name+".xtra", d2).  asm: d7, Z=1 if failed. */
tedfh openxtra(LONG d2);
/* ted.s:664 openmore: append extension a1 at xtraat, remember the old end
   in xtranull.  Returns the name (asm: d1 = xtraat). */
UBYTE *openmore(const char *a1);
/* ted.s:674 closextra: *xtranull = 0 (remove the extension again) */
void closextra(void);

/* ------------------------------------------------------- load / save */

/* ted.s:680 fileopen2 - load file d7 (full path) into the line list:
   xtraat = d7, doopen (failure: error() was shown, return), allalt = 0,
   doclean, fillbuff (empty file: doclose, donew), sleepon, blokdis,
   freeall, readline/allocline loop, doclose, loadxtra callback (.xtra),
   cursor/scroll 0, showhprop, showall, pointeron, showfname.
   Also reached from fileopen (main.c) and outside.tomain. */
void fileopen2(UBYTE *d7);
/* ted.s:769 realsave - write the list to file d7 (filesave falls through
   after the .bak rename): doopen(NEWFILE) (failure: return - the file is
   then only there as .bak, survey-01 I.15), sleepon, write loop (ASCII
   mode: LF instead of the 0), error(error3) on a write error, doclose,
   savextra callback, .info if icongad(), pointeron, showfname. */
void realsave(UBYTE *d7);

/* --------------------------------------------------- search / replace */

/* ted.s:926 next - menu $13: search from the line after a4 forwards */
void next(void);
/* ted.s:959 previous - menu $23: backwards from the line before a4 */
void previous(void);
/* ted.s:931 dosearch: d6 = direction +1/-1, a4 = first line to search
   (already the neighbour).  Empty search text / not found: beepem.  Found:
   pointeron, gotolined0, horizpos(column), showhprop.  Never searches the
   rest of the current line (survey-01 I.18). */
void dosearch(struct item *a4, WORD d6);
/* ted.s:965 replace - menu $33: replace2 on the current line; nothing
   replaced -> beepem, else showall */
void replace(void);
/* ted.s:971 replace2: replace ALL occurrences in line a4 (thisitem).
   asm: Z=1 (d0=0) if at least one replacement -> TRUE; a4/thisitem = the
   new line (makeline).  New a4 via *a4out (may be NULL). */
BOOL replace2(struct item *a4, struct item **a4out);

#endif
