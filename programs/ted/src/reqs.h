/*
 * reqs.h - chunk 04, Amiga part (ted.s 3063-4197): options requester
 * helpers, erase/error, requesters as windows (openreq/closereq), the file
 * requester bridge, mouse marking helpers, the line output (showall ...).
 * Core part of chunk 04: src/core/fileio.h.
 * Hooks implemented here: setdefs, error, getbname, showall, showtopitem,
 * showbotitem, showthisitem, blokprnt2, blokmenus (hooks.h).
 */
#ifndef TED_REQS_H
#define TED_REQS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include "tedcore.h"

/* ---------------------------------------------- options requester */

/* setdefs (ted.s:3092): hooks.h */
/* ted.s:3167 setprops: HorizPot of defaultreqhp1..3 from palit[pen*3+k]
   (pen = d0; v*65536/15, 15 -> $FFFF) */
void setprops(UWORD pen);
/* ted.s:3189 dispprops: RefreshGList(defaultreqga1, req_window, defaultreq, 3) */
void dispprops(void);
/* ted.s:3196 propadj: rgb (a5) = palit entry (r,g,b), pen (d4) = colour
   register: read the sliders, changed -> write back + SetRGB4 */
void propadj(UWORD *rgb, UWORD pen);
/* ted.s:3228 dispgadd0: RefreshGList of colour button d0 (ga10 + d0*44) */
void dispgadd0(UWORD button);
/* ted.s:3238 swapd0bord: swap pens of the two borders of button d0
   (bo14 + d0*56) */
void swapd0bord(UWORD button);
/* ted.s:3247 swapbords: swap pens of all 32 borders from defaultreqbo14 */
void swapbords(void);

/* ------------------------------------------------ erase / error */

/* ted.s:3257 erase: erase2; FALSE = cancelled (asm: addq #4,a7 - the CALLER
   returns at once: every caller checks) */
BOOL erase(void);
/* ted.s:3262 erase2: allalt == 0 or OK -> TRUE (asm Z=1), mousealt = -1 */
BOOL erase2(void);
/* error (ted.s:3511, comdata+12): hooks.h */

/* ----------------------------------------- file requester bridge */

/* getbname (ted.s:3485): hooks.h */
/* ted.s:3489 getfname: title filetext, falls into getfname2 */
UBYTE *getfname(void);
/* ted.s:3490 getfname2: filepath/filename, title a2 */
UBYTE *getfname2(const char *title);
/* ted.s:3493 getfname3: ezfreq; name -> returned (asm: d7); cancel ->
   NULL (asm: addq #4,a7, the CALLER returns); d0 == 1 -> outmem (dead,
   survey-07 I.3) */
UBYTE *getfname3(char *path, char *name, const char *title);
/* ted.s:3502 ezfreq (comdata+24): reqstruct = {path a0, name a1, title a2,
   screen}, filerequest() (filereq.h).  Returns d0 (0 = cancel; asm Z=1) */
UBYTE *ezfreq(char *path, char *name, const char *title);
/* ted.s:3509 reqstruct: {path, name, title, screen} - [3] set by
   screenopen (ted.s:4675) */
extern ULONG reqstruct[4];

/* ----------------------------------------- requesters as windows */

/* ted.s:3580 openreq (comdata+0): window for requester template a0
   (centered, gadgets moved once by +6/+12, marked in UserData of the first
   gadget), window stack req_old2 <- req_old <- req_window, PrintIText/
   DrawBorder, pointeron.  OpenWindow failure -> outmem (never returns
   0, survey-04 I.14). */
void openreq(struct Requester *rq);
/* ted.s:3643 closereq (comdata+4): close req_window (if any), pop the
   stack, flushmw; always makemenus */
void closereq(void);
/* ted.s:3672 flushmw: reply all messages of mywindow's port */
void flushmw(void);
extern struct NewWindow reqwindow;      /* ted.s:3565 */
#define req_gads   (reqwindow.FirstGadget)      /* ted.s:3569 */
#define req_screen (reqwindow.Screen)           /* ted.s:3572 */
extern struct Window *req_old, *req_old2;       /* ted.s:3577/3578 */

/* ------------------------------------------------ mouse marking */

/* ted.s:3929 showprops2: cursx/cursy temporarily from col/row (d2/d3,
   clamped), showhprop + showvprop, restored */
void showprops2(WORD col, UWORD row);
/* ted.s:3974 invertarea: corners (x1,y1) (d0,d1) and (x2,y2) (d2,d3),
   orderem, falls into invertarea2 */
void invertarea(WORD x1, WORD y1, WORD x2, WORD y2);
/* ted.s:3977 invertarea2: not.b over the cells (col,row)-(endcol,endrow)
   (d0,d1)-(d2,d3) in both planes (direct chip RAM access) */
void invertarea2(WORD col, WORD row, WORD endcol, WORD endrow);
/* getmousexy's result (asm d0-d3) */
struct mousexy {
    WORD x, y;                  /* d0, d1: screen MouseX/MouseY */
    WORD col, row;              /* d2, d3: character column and row */
};
/* ted.s:4015 getmousexy: screen MouseX/Y -> r->x, r->y, r->col = x>>3,
   r->row = (y-topskip)/fonthite (word; the asm leaves the remainder in the
   high word).  Outside the text area: r->row = -1, FALSE (asm N=1). */
BOOL getmousexy(struct mousexy *r);

/* -------------------------------------------------- line output */

/* showall (4050), showtopitem (4073), showbotitem (4088), showthisitem
   (4102): hooks.h */
/* ted.s:4104 showa2item: draw item a2 into the cursor row (chmap +
   bitmap).  endofline (4124) is its end label (the cursx saved on the asm
   stack is a local).  normprint (4142), normprint2 (4143) and tokprint
   (4152) share the output registers a2/a3/d2/d3 with it: static helpers
   in reqs.c with a small state struct. */
void showa2item(struct item *item);

/* bloken (3890) / blokdis (3903): core (fileio.h); blokprnt2 (3899) and
   the menu part blokmenus: hooks.h */

#endif
