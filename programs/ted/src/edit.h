/*
 * edit.h - chunk 02, Amiga part (ted.s 1016-1980): escape, beepem, the
 * vertical prop gadget loop, the external COMPILER menu, the menu dispatch,
 * new.  Core part of chunk 02: src/core/lines.h.
 * Hooks implemented here: beepem (hooks.h).
 */
#ifndef TED_EDIT_H
#define TED_EDIT_H

#include <exec/types.h>
#include "tedcore.h"

/* ted.s:1016 escape - menu $90: erase (cancel: return), free the COMPILER
   menu (still attached, survey-02 I.7), doclean, freemice, allalt = 0,
   freeall, edquit = TRUE (asm: lea 12(a7),a7 - returns from mainedit,
   DIFF stack trick) */
void escape(void);
/* beepem (ted.s:1054): hooks.h */
/* ted.s:1460 gadget - event 255: drag the vertical prop (busy poll with
   getinput2, the ending message is swallowed, survey-02 I.11); uses
   bloktopx as scratch (harmless, keep the side effect) */
void gadget(void);
/* ted.s:1542 extmenu: menurout == 0 -> return; call_menurout(item) (glue.h);
   showme; -2 -> showall, -1 -> return, else gotolined0 */
void extmenu(UWORD d6);
/* ted.s:1565 menu - event 254: ev_d6 = Intuition menu code; menu 4 ->
   extmenu, else search menutable (input.h) for item<<4|menu, call, flag ->
   showvprop */
void menu(void);
/* ted.s:1593 new - menu $00: erase, doclean, freeall, title = deftitname,
   showme, donew */
void new(void);

extern const char deftitname[]; /* ted.s:1604 'Blitz Basic 2 Editor V1.0' */

#endif
