/*
 * screen.h - chunk 05, Amiga part (ted.s 4198-5252): blitter scrolling,
 * screen open/close, user copper list, colours, blitter bracket, cls,
 * horizontal scrolling, the own text output (print*, qprint) and calcphys.
 * Core part of chunk 05: src/core/tokit.h (tokeniser, godown/goup, line
 * alloc/free, donew, chmap halves of the blits); fontcalc: keymap.h.
 * Hooks implemented here: scrollup, scrupall, scrolldown, scrdownall, cls,
 * scrollrite, scrollleft (hooks.h).
 *
 * Blitter routines: custom chips via <hardware/custom.h> (volatile struct
 * Custom *), between OwnBlitter/DisownBlitter, WaitBlit after each blit as
 * in the asm - no inline asm needed (PLAN.md 4).
 */
#ifndef TED_SCREEN_H
#define TED_SCREEN_H

#include <exec/types.h>
#include "tedcore.h"

/* scrupall (4345), scrollup (4346), scrdownall (4433), scrolldown (4434):
   hooks.h.  Internal (static in screen.c): scup2 (4407, one plane up +
   fill), scdown2 (4523, one plane down). */
/* ted.s:4479 eraseline: row d7 = spaces in chmap, 0/$FFFF (block) in
   both planes (blitter already owned: entered from scrolldown) */
void eraseline(UWORD d7);

/* ------------------------------------------------------------ screen */

/* ted.s:4619 calcscreen: geometry from the Workbench (GetScreenData):
   widths, edcols/edrows/pixrows, NewScreen/NewWindow sizes */
void calcscreen(void);
/* ted.s:4661 screenopen2 (comdata-4): screenopen (result ignored,
   survey-05 I.2), showvprop, showhprop, showall */
void screenopen2(void);
/* ted.s:4668 screenopen: open screen+window, chmap (CHIP), gadget/border
   patches, menus, colours, status line, pointer, makecop.  asm: d0 = 0/Z=1
   OK -> TRUE.  Failure paths call CloseScreen with a6 = ExecBase
   (BUG(orig) #382, survey-05 I.1 - reproduce as the same LVO call or DIFF). */
BOOL screenopen(void);
/* ted.s:4823 makecop: colour gradient of pen 2 in the title bar (UCopList)
   unless copok */
void makecop(void);
/* ted.s:4880 pstats: string a3 into the status line at a1 (one plane,
   qprint per character) */
void pstats(const char *a3, UBYTE *a1);
/* ted.s:4888 freechmap: FreeMem(chmap, chcols*edrows) */
void freechmap(void);
/* ted.s:4895 screenclose (comdata-8): window, menus, chmap, screen */
void screenclose(void);
/* ted.s:4913 mwait: busy wait for CIA-A $BFE001 bit 7 (dead code) */
void mwait(void);
/* ted.s:4917 halfcolors (comdata+52): SetRGB4 0-3 with palit/2 */
void halfcolors(void);
/* ted.s:4940 colors (comdata+48): SetRGB4 0-3 from palit, 17-19 from mpalit */
void colors(void);

/* ------------------------------------------------ blitter / cls */

/* ted.s:4964 getblitter: OwnBlitter, WaitBlit, AFWM/ALWM = $FFFF, CON1 = 0 */
void getblitter(void);
/* ted.s:4973 putblitter: WaitBlit, DisownBlitter */
void putblitter(void);
/* cls (4986), scrollrite (5019), scrollleft (5069): hooks.h.  Internal:
   cls2 (5006), scrite2 (5064), scleft2 (5108), hscmods (5112). */

/* ------------------------------------------------------ text output */

/* ted.s:5125 print3: d0 bit 7 -> tokcol else normcol, bit 7 cleared, draw
   only (print4) - no chmap write */
void print3(UBYTE d0);
/* ted.s:5132 print: print d0 at cursx/cursy in `color` (print2: chmap +
   draw), through printa0 */
void print(UBYTE d0);
/* ted.s:5135 printa0: `inverse` from the block bounds, then a0(d0)
   (computed call).  `prinv` (5163) is a label inside. */
void printa0(UBYTE d0, void (*a0)(UBYTE));
/* ted.s:5167 print2: chmap[chloc+cursoff+cursx] = d0 (| $80 if color !=
   normcol), falls into print4 */
void print2(UBYTE d0);
/* ted.s:5178 print4: draw glyph d0 into both planes (colour bits, inverse) */
void print4(UBYTE d0);
/* ted.s:5205 qprint: glyph d0 at a0, one plane, no colour/inverse */
void qprint(UBYTE d0, UBYTE *a0);
/* ted.s:5244 calcphys: *a0, *a1 = bitmaps[0/1] + scloc + cursx */
void calcphys(UBYTE **a0, UBYTE **a1);

/* ------------------------------------------------------------- data */

extern struct Screen scdata;    /* ted.s:4617 ds.b 84: GetScreenData copy of
                                   the Workbench (only the first 84 bytes) */
extern UWORD blitmod;           /* ted.s:4666 bytes per row - edcols */
extern APTR oldmywindow;        /* ted.s:4811 saved pr_WindowPtr */
extern UWORD screensopen;       /* ted.s:4812 -1 = open */
extern struct UCopList *coplist;        /* ted.s:4814 (only written) */
extern UWORD cols[12];          /* ted.s:4816 gradient; cols[11] = col2 (4819) */
#define col2 (cols[11])
extern UWORD copok;             /* ted.s:4821 != 0: no makecop */
extern ULONG pixrows22;         /* ted.s:5017 bpr * pixrows */

#endif
