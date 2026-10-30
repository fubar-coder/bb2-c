/*
 * lines.h - chunk 02 core (ted.s 1016-1980): cursor up/down, insert/
 * delete/join/split/return, top/bottom, gotolined0, block kill/save/load/
 * copy/forget.  UI part of chunk 02: src/edit.h (escape, beepem, gadget,
 * extmenu, menu, new).
 *
 * Event handlers: `void name(void)`, asm a4 = thisitem at entry.  Most of
 * them only set thisitem (mainedit reloads a4 every round).
 * Block coordinate rules (blokadj, deleteline, join, split, blokcopy):
 * survey-02 H - unsigned row compares, signed column compares.
 */
#ifndef TED_LINES_H
#define TED_LINES_H

#include "tedcore.h"

/* ted.s:1041 cursdown - key 29: Shift: page down (gotolined02); no next
   line: beepem; else falls into cursdownok */
void cursdown(void);
/* ted.s:1058 cursdownok: thisitem = d0 (a4 stays old), falls into movecdown */
void cursdownok(struct item *d0);
/* ted.s:1061 movecdown: godown, or curstop++/scrupall/showbotitem.
   Second entry from return. */
void movecdown(void);
/* ted.s:1074 cursup - key 28 (Shift: page up) */
void cursup(void);
/* ted.s:1098 markindent - menu $b1: block = all following/preceding lines
   with indentation >= that of a4 (empty lines included); bloken, showall */
void markindent(void);
/* ted.s:1153 insertline - menu $51: new empty line BEFORE a4 (allocnull,
   failure -> outmem), allalt, blokadj, numlines++, scrolldown(cursy) */
void insertline(void);
/* ted.s:1173 deleteline - menu $61: block adjust, unlink + freethis, three
   cases: middle (scrollup + showbotitem), last (freelast), only (freeonly) */
void deleteline(void);
/* ted.s:1208 freelast - (bra from deleteline) delete the last line */
void freelast(void);
/* ted.s:1226 freeonly - delete the only line: freethis, allocfirst */
void freeonly(void);
/* ted.s:1229 join - menu $81: current chmap line + next line; frees the
   next line with numchars instead of numchars+lenitem (BUG(orig) #364,
   survey-02 I.1 - keep it, ORIG_HAZARD on the host) */
void join(void);
/* ted.s:1313 split - Shift+Return: cut the chmap line at the cursor
   (writes spaces into chmap), new line after a4, showall */
void split(void);
/* ted.s:1402 return - key 13 (Shift -> split): new empty line after a4,
   cursx = 0, scroll (with edrows/pixrows changed TEMPORARILY around
   scrupall) or scrolldown + movecdown.  `return` is a C keyword. */
void return_(void);
/* ted.s:1453 top - menu $02: gotolined0(0) */
void top(void);
/* ted.s:1456 bottom - menu $12: gotolined0(numlines-1) */
void bottom(void);

/* ted.s:1490 gotolined0: go to line d0 (0 based, MUST be < numlines - no
   check, survey-02 I.13), a4 = current item (thisitem).  cursx = 0
   (cursoff stays), curstop/cursy from vscmarg rules, calcpos, showall if
   curstop changed, showhprop.  Returns the new a4 (= thisitem). */
struct item *gotolined0(UWORD d0);
/* ted.s:1495 gotolined02: same without showhprop (asm pushes flag 0) */
struct item *gotolined02(UWORD d0);
/* ted.s:1496 gd03: common part; flag != 0 -> showhprop at the end
   (asm: the word pushed by gotolined0/gotolined02) */
struct item *gd03(UWORD d0, WORD flag);

/* ------------------------------------------------------------- blocks */

/* ted.s:1607 blokkill - menu $11 (and Shift-F3): delete the block */
void blokkill(void);
/* ted.s:1671 blocksave - key 131 (F3): Shift -> blokkill + showvprop,
   else falls into bloksave */
void blocksave(void);
/* ted.s:1677 bloksave - menu $21: write the block to a file (getbname,
   doopen NEWFILE, byte-wise Write; bloktopx is the 1 byte buffer in the
   asm).  Cancel/open error: return. */
void bloksave(void);
/* ted.s:1738 blokload - menu $31: insert a file at the cursor.  The asm
   labels done3 (1778), done2 (1783), done4 (1785), done (1791), blalldone
   (1801) are global but only used by blokload: goto labels inside it
   (keep the `ted.s:NNN label` comments). */
void blokload(void);
/* ted.s:1808 blokcopy2 - key 132 (F4): blokcopy + showvprop */
void blokcopy2(void);
/* ted.s:1811 blokcopy - menu $01: copy the block to the cursor; cursor
   inside the block (inbloks/inblokf) -> beepem.  Falls into bcopyok. */
void blokcopy(void);
/* ted.s:1832 bcopyok: the copy itself (only reached from blokcopy) */
void bcopyok(void);
/* ted.s:1976 blokforget - menu $41: blokdis + showall (no block: beepem) */
void blokforget(void);

#endif
