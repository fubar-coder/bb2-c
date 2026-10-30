/*
 * tokit.h - chunk 05 core (ted.s 4198-5252): the tokeniser, alpha, cursor
 * row arithmetic, line alloc/free, donew, and the chmap half of the
 * blitter routines (scrollup/scrolldown/eraseline/cls).  UI part of chunk
 * 05: src/screen.h (blitter, screen open/close, colours, print*).
 */
#ifndef TED_TOKIT_H
#define TED_TOKIT_H

#include "tedcore.h"

/* ---------------------------------------------------------- tokeniser */

/* ted.s:4198 tokit (= tokit4, ted.s:4202, same address): tokenise a2 into
   a1; d1 = index of the LAST character (charstobuff: chcols-1).  The asm
   keeps the start of a1 on the stack for enddo; here the start is a1.
   Returns d5 = output length incl. 0; *a1end (may be NULL) = a1 after the
   asm: behind the 0 - but ON the 0 for an all-space line (survey-05 I.16).
   Quote/remark characters and the token list come from the globals quote,
   remark, tokens; invflag is the quote flag (global side effect, kept).
   Reads beyond d1 while matching names (chmap: into the next row;
   BUG(orig) #386, survey-05 A, ORIG_HAZARD). */
UWORD tokit(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end);
/* ted.s:4204 tokit3: d1 = LENGTH (subq #1 first), then tokit.  Entry of
   tokit2 (fileio.h). */
UWORD tokit3(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end);
/* The internal labels tokskip (4210), putchar (4319), pchover (4324) and
   pchover2 (4325) are static in tokit.c (putchar -> putchar_: libc name).
   pchover leaves two levels at once (addq #4,a7): goto/return in C. */

/* ted.s:4328 alpha: d2 is 'A'-'Z' or 'a'-'z' (asm: Z=1) */
BOOL alpha(UBYTE d2);
/* ted.s:4340 alphad0: the same for d0 */
BOOL alphad0(UBYTE d0);

/* ------------------------------------------------------ cursor rows */

void godown(void);              /* ted.s:4530 godown: cursy++, scloc += fontmul, chloc += chcols */
void goup(void);                /* ted.s:4538 goup: the opposite (no bounds check) */
void storepos(void);            /* ted.s:4976 storepos: cursx/cursy/scloc/chloc -> *2 */
void restorepos(void);          /* ted.s:4981 restorepos: *2 -> cursx/cursy/scloc/chloc */
void calcpos(void);             /* ted.s:5234 calcpos: scloc = cursy*fontmul, chloc = cursy*chcols */

/* ---------------------------------------------------------- line list */

/* ted.s:4547 allocnull: AllocMem(lenitem+1, PUBLIC) (NOT alloc2: no outmem),
   next = last = 0, numchars = 1, chars[0] = 0, memalt = -1.  NULL on
   failure (asm: Z=1). */
struct item *allocnull(void);
/* ted.s:4560 donew: colour, blokdis, cursor/scroll/allalt = 0, filepath =
   filename = "", numlines = 1, allocfirst, showhprop, showvprop.  Does not
   free the old lines (caller: freeall).  asm: always d0 = -1 / Z=0. */
void donew(void);
/* ted.s:4582 freethis: free THISITEM (not unlinked); label -> mforget2;
   size lenitem + numchars with add.b (byte overflow from 247, unreachable,
   survey-05 I.5); memalt = size */
void freethis(void);
/* ted.s:4602 freeall: free numlines items from firstitem (dbf: numlines 0
   = 65536 rounds); list globals stay (dangling) */
void freeall(void);

/* --------------------------------------- chmap half of the blitter ops */

/* NEW names: the chmap part of the blits in scrollup (4346), scrolldown
   (4434), eraseline (4479) and cls (4986) as plain memory operations
   (DIFF: CPU instead of blitter - same bytes; the asm blits chmap with the
   same parameters as the bitplanes, including the one-row-too-many quirk
   of scrolldown, survey-05 I.4, which is invisible afterwards).
   Called by the UI routines in screen.c and by the host stubs. */
void chmap_scrollup(UWORD row);     /* scrollup: rows row+1.. up, last row = spaces */
void chmap_scrolldown(UWORD row);   /* scrolldown: rows row..edrows-2 down */
void chmap_eraseline(UWORD row);    /* eraseline: row = spaces */
void chmap_cls(void);               /* cls: all rows = spaces */

#endif
