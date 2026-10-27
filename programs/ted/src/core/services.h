/*
 * services.h - chunk 03 core (ted.s 1981-3062): F1/F2 block marks,
 * tokenise/detokenise all lines, token help lookup, label column list
 * ("mice"), a0toname, alloc2.  UI part of chunk 03: src/mouse.h (mouse,
 * goto requester, Workbench/CLI windows, help display, getstuff, showmice,
 * printmem, outmem, extalloc, doalert, doclean).
 */
#ifndef TED_SERVICES_H
#define TED_SERVICES_H

#include "tedcore.h"

/* ted.s:2458 blockstart - key 129 (F1): bstx/bsty = cursor (absolute) */
void blockstart(void);
/* ted.s:2468 blockend - key 130 (F2): block = bstx/bsty .. cursor-1;
   no start: beepem.  First writes btopx from an UNDEFINED d0 (the event
   code byte of mainedit, survey-03 I.2): C uses the event code. */
void blockend(void);

/* ted.s:2636 tokone (comdata+56): tokenise the 0-terminated line a0 IN
   PLACE (tokit2 with a1 = a2 = a0, d1 = strlen).  Returns d5 = length incl.
   0. */
UWORD tokone(UBYTE *a0);
/* ted.s:2648 tokall (comdata+20): tokenise != 0 -> beepem; else
   tokenise = $FFFF (not) and fall into tokall2 */
void tokall(void);
/* ted.s:2653 tokall2: tokit2 + makeline for every line, then detokallc */
void tokall2(void);
/* ted.s:2670 detokallz (comdata+16): tokenise == 0 -> beepem; else
   tokenise = 0, detokallz2 (no redraw: thisitem = last line, survey-03 I.14) */
void detokallz(void);
/* ted.s:2673 detokallz2: detok + makeline for every line */
void detokallz2(void);
/* ted.s:2689 detokall: no caller (dead entry); falls into detokall2 */
void detokall(void);
/* ted.s:2692 detokall2 (from defaults): detokallz2, falls into detokallc */
void detokall2(void);
/* ted.s:2694 detokallc: calcitem + showall */
void detokallc(void);

/* ted.s:2697 (help, core part - NEW name): the LAST token that starts at or
   before column `col` of line a4 (tokenised).  Returns its name (asm: a3
   after findtoke; the help text follows the name's 0 byte) or NULL if none; *num = token number
   (asm: helptoke).  Unknown token: findtoke's tokeerr (survey-03 I.13).
   The title/Execute part is help() in mouse.h. */
const char *help_core(struct item *a4, UWORD col, UWORD *num);

/* ------------------------------------------------- label column (mice) */

/* ted.s:2850 makemice: remember2 every line whose first non-space char is
   `label`; mousealt = the label char */
void makemice(void);
/* ted.s:2869 freemice: free nummice nodes, mousealt = old count,
   nummice = 0 (mice stays dangling) */
void freemice(void);
/* ted.s:2883 remember: only mousealt = -1 (a0 = line, ignored) */
void remember(struct item *a0);
/* ted.s:2887 remember2: append a node for line a0 (AllocMem(8) not
   cleared, failure -> outmem; `next` of the new node stays
   uninitialised, survey-03 I.4) */
void remember2(struct item *a0);
/* ted.s:2912 mforget2: only mousealt = -1 */
void mforget2(void);

/* ted.s:2990 a0toname: split a0 (maybe in "...") into filepath/filename.
   BUG(orig): compares d0 (the caller's register at .tomain - blitz2: the
   LoadSeg result) instead of the copied character (survey-03 I.1), so d0
   is a parameter. */
void a0toname(const UBYTE *a0, ULONG d0);

/* ted.s:3033 alloc2: memalt = -1, AllocMem(d0, d1); failure -> outmem
   (never returns NULL) */
APTR alloc2(ULONG d0, ULONG d1);

#endif
