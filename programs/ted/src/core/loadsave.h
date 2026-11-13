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
/* ted.s:361 blockuntab - menu $a1: remove up to tabstop leading spaces of
   every block line.  Fixed (#348): the original removes the first tabstop
   characters, WHATEVER they are (survey-01 I.11). */
void blockuntab(void);

/* ted.s:570 (prtfile, core part - NEW name): format line (a4) for the printer
   into buf (charbuff): with tokenise on a token -> ESC[1m name ESC[22m
   (without it bytes >= $80 are characters, Fixed (#371)), the umlaut
   codes 26-31 -> the Latin-1 umlauts (Fixed (#1017)), end of line -> LF.
   The line stops silently where the next character or token would leave
   no room for the LF before charbuff4 (Fixed (#372)).  Returns the number
   of bytes (asm: Write length).  The requester/Open/Write loop is prtfile
   (main.h). */
UWORD prtfile_line(struct item *line, UBYTE *buf);

/* ----------------------------------------------------- .xtra / .info */

extern const char xtratxt[];    /* ted.s:638 '.xtra' */
extern const char infotxt[];    /* ted.s:639 '.info' */
extern const char bakname[];    /* ted.s:742 '.bak' (filesave, main.c) */

/* ted.s:641 openinfo: name(xtraat)+".info" exists (Lock) -> 0; else
   Open(..., NEWFILE).  asm: d7 = handle, Z=1 if 0. */
tedfh openinfo(void);
/* ted.s:658 openxtra: Open(name+".xtra", mode (d2)).  asm: d7, Z=1 if
   failed. */
tedfh openxtra(LONG mode);
/* ted.s:664 openmore: append extension ext (a1) at xtraat, remember the
   old end in xtranull.  Returns the name (asm: d1 = xtraat). */
UBYTE *openmore(const char *ext);
/* ted.s:674 closextra: *xtranull = 0 (remove the extension again) */
void closextra(void);

/* ------------------------------------------------------- load / save */

/* ted.s:680 fileopen2 - load file name (d7, full path) into the line
   list: xtraat = name, doopen (failure: error() was shown, return),
   allalt = 0, doclean, fillbuff (empty file: doclose, freeall (fixed
   #358), donew; outmem if it fails, fixed #356), sleepon, blokdis,
   freeall, readline/allocline loop (until the first line is linked
   firstitem = thisitem = enditem = 0: outmem then leaves an empty text,
   fixed #1009), doclose, loadxtra callback (.xtra),
   cursor/scroll 0, showhprop, showall, pointeron, showfname.
   Also reached from fileopen (main.c) and outside.tomain. */
void fileopen2(UBYTE *name);
/* ted.s:769 realsave - write the list to file name (d7) (filesave falls
   through after the .bak rename): doopen(NEWFILE) (failure: return -
   the file is then only there as .bak, survey-01 I.15), sleepon, write
   loop (ASCII mode: LF instead of the 0), doclose, savextra callback,
   .info if icongad(), pointeron, showfname; a write error: error(error3),
   doclose, no .xtra and no .info (fixed #380), pointeron, showfname. */
void realsave(UBYTE *name);

/* --------------------------------------------------- search / replace */

/* ted.s:926 next - menu $13: search from the line after thisitem forwards */
void next(void);
/* ted.s:959 previous - menu $23: backwards from the line before thisitem */
void previous(void);
/* ted.s:931 dosearch: dir (d6) = direction +1/-1, line (a4) = first line
   to search (already the neighbour).  Empty search text / not found:
   beepem.  Found: pointeron, gotolined0, horizpos(column), showhprop.
   Never searches the rest of the current line (survey-01 I.18). */
void dosearch(struct item *line, WORD dir);
/* ted.s:965 replace - menu $33: replace2 on the current line; nothing
   replaced -> beepem, else showall */
void replace(void);
/* ted.s:971 replace2: replace ALL occurrences in line (a4, thisitem).
   asm: Z=1 (d0=0) if at least one replacement -> TRUE; a4/thisitem = the
   new line (makeline).  New line via *lineout (may be NULL). */
BOOL replace2(struct item *line, struct item **lineout);

#endif
