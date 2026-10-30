/*
 * input.h - chunk 06, Amiga part (ted.s 5253-6285): cursor, status line +
 * props, refresh, the line editor loop getline, IDCMP input (getinput,
 * getinput2, handled0, refme), the comdata table and the dispatch tables.
 * Core part of chunk 06: src/core/lineedit.h (+ state.c: the data block).
 * Hooks implemented here: refresh, showhprop, showvprop (hooks.h).
 */
#ifndef TED_INPUT_H
#define TED_INPUT_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include "tedcore.h"
#include "structs.h"

/* ted.s:5253 cursor (= cursor2, ted.s:5254, same address): XOR the cursor
   cell in both planes (twice = off) */
void cursor(void);
void cursor2(void);
/* ted.s:5263 showrow: "Line:" number into the status line */
void showrow(void);
/* showvprop (5275), showhprop (5305), refresh (5337): hooks.h */

/* ted.s:5416 getline: the line editor.  Loops until an event it does not
   handle itself; returns the event (d7: key 1..$FF, -1 gadget, -2 menu,
   -3/-4 mouse; bit 30 = germs replacement is handled inside) and *id (d6).  Local
   labels (goto targets / sections inside getline, keep the markers):
   getline2 5420, getline3 5422, charlow 5484, charnot31 5521, curspos 5537,
   cis30skip 5540, charnot30 5543, charnot8 5555, charnot9 5658, charhi
   5662, char127 5665, charnot127 5718, charnotgad 5726, charnotmenu 5738,
   glinedone 5740.  Data parts: gl_* in lineedit.h. */
LONG getline(UWORD *id);
/* ted.s:5645 reline: redraw the visible chmap row (print3), then
   curspos(col) = horizpos (asm: the column is a word on the stack) */
void reline(WORD col);
/* ted.s:5810 flash: debug, COLOR00 65536 times (unused) */
void flash(UWORD unused);

/* ------------------------------------------------------------- IDCMP */

/* ted.s:5827 pickwindow: req_window if open, else mywindow */
struct Window *pickwindow(void);
/* ted.s:5833 getinput2 (comdata+32): at most one message, no wait.
   Returns the event (d7; 0 = nothing / ignored), *id (d6). */
LONG getinput2(UWORD *id);
/* ted.s:5845 getinput (comdata+8): wait until handled0 understands a
   message (`ginp1` 5852 = loop head).  Returns the event (d7), *id (d6). */
LONG getinput(UWORD *id);
/* ted.s:5866 handled0: decode + ReplyMsg of im (d0).  Returns the event
   (d7; 0 = ignored, asm d0 = 0/Z=1), *id (d6).  ENDGADGET up -> closereq BEFORE returning; requester
   keys Return/Esc -> refme + first/last bool ENDGADGET; RAWKEY ->
   rawed_convert (rawkey.h) + germs_map (keymap.h). */
LONG handled0(struct IntuiMessage *im, UWORD *id);
/* ted.s:6003 refme: flash gadget gad (a0; SELECTED, refresh, Delay(4), off) */
void refme(struct Gadget *gad);

/* ------------------------------------------------------------- data */

/* struct mydata mydata (ted.s:6027, comdata 6030): structs.h, defined in
   input.c with the stub addresses of glue.h */
extern struct Window *mywindow;         /* ted.s:6154 */
extern struct RastPort *myrast;         /* ted.s:6155 */
/* ted.s:6171 jumptable (end: code 0), ted.s:6199 menutable (end: 255) */
extern const struct jtentry jumptable[];
extern const struct jtentry menutable[];

#endif
