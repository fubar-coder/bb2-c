/*
 * mouse.h - chunk 03, Amiga part (ted.s 1981-3062): mouse selection, goto
 * requester, Workbench open/close, the console window for running
 * programs, comdata gadget services, help display, the COMPILER menu,
 * showmice, printmem, memory error handling.
 * Core part of chunk 03: src/core/services.h.
 * Hooks implemented here: doclean, outmem (hooks.h).
 */
#ifndef TED_MOUSE_H
#define TED_MOUSE_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include "tedcore.h"

/* ted.s:1981 gomouse: click into the label column (only reached by a bcc
   from mousedown): col = screen column (asm d2), row = screen row (d3);
   line found -> gotolined0 */
void gomouse(WORD col, UWORD row);
/* ted.s:2001 mousedown - event 253: select with the mouse (busy poll with
   getinput2).  `oneloop` (ted.s:2290) is a label inside it (its local
   labels .skip/.skip2/.nben belong to oneloop, survey-03).  The saved
   btop/bbot, curstop, cursoff, a4 of the asm stack frame are locals. */
void mousedown(void);
/* ted.s:2409 actgad (comdata+68): Delay(8), ActivateGadget(gad (asm a0),
   req_window, NULL); returns d0 */
ULONG actgad(struct Gadget *gad);
/* ted.s:2420 gotoline - menu $22: goto requester; 0 or > numlines (word
   compare, survey-03 I.10) -> beepem, else gotolined0(n-1) */
void gotoline(const struct tedevent *ev);
void closewb(void);             /* ted.s:2501 menu $70: CloseWorkBench */
void openwb(void);              /* ted.s:2507 menu $80: OpenWorkBench, front, activate */
/* ted.s:2513 findio: device name (asm a0, length len = d0) in the DOS
   device list (no locking).  asm: Z=1 found -> TRUE */
BOOL findio(const char *name, UWORD len);
/* ted.s:2541 calciorows: 200 (NTSC) or 256 */
UWORD calciorows(void);
/* ted.s:2548 openio (comdata+60): NEWCON:/CON: window, pr_CIS/pr_COS,
   WBenchToFront.  Returns the handle, 0 when the window could not be
   opened (fixed #368: the original returned WBenchToFront's d0 and flags,
   survey-03 I.8) */
BPTR openio(void);
/* ted.s:2597 closeio (comdata+64): restore pr_CIS/pr_COS, Close(myhandle)
   (also 0), screen to front */
void closeio(void);
/* ted.s:2616 mycli - menu $60: openio, Execute("", myhandle, 0), closeio */
void mycli(void);
/* ted.s:2629 refgads (comdata+28): RefreshGList(gad (asm a0), req_window,
   NULL, count (d0)) */
void refgads(struct Gadget *gad, LONG count);    /* count: the whole d0 */
/* ted.s:2697 help - key 139 (Help): help_core (services.h), title = name
   + help text, ShowTitle; right Amiga: Execute("blitz2:help XXXX") */
void help(void);
/* doclean (ted.s:2771): hooks.h */
/* ted.s:2781 getstuff (once from outside, external only): tokenise = 1 +
   gadget flag if tokens; COMPILER menu from menitems (AllocMem unchecked),
   SetMenuStrip */
void getstuff(void);
/* ted.s:2916 showmice: rebuild the label list (freemice/makemice), open/
   close the label column (screen reopened), print the labels */
void showmice(void);
/* ted.s:3019 printmem: AvailMem(LARGEST|PUBLIC)/1024 into the status line */
void printmem(void);
/* outmem (ted.s:3038): hooks.h */
/* ted.s:3049 extalloc (comdata+88): AllocMem(size (asm d0), flags (d1));
   failure: closereq + outmem (longjmp out of the compiler, survey-03 I.7) */
APTR extalloc(ULONG size, ULONG flags);
/* ted.s:3057 doalert: DisplayAlert(RECOVERY_ALERT, alertstuff, 40) */
void doalert(void);

/* ------------------------------------------------------------- data */

extern UWORD newctop;           /* ted.s:2318 */
extern UWORD newcoff;           /* ted.s:2319 */
/* ted.s:2324-2407 goto requester (template for openreq; tools/req2c.py) */
extern struct Requester gotoreq;
extern struct Gadget gotoreqga1, gotoreqga2, gotoreqga3;
extern struct StringInfo gotoreqsg1;
extern UBYTE gotoreqla8[6];     /* ted.s:2395 string buffer */
#define gotoint (gotoreqsg1.LongInt)    /* ted.s:2394 alias inside sg1 */
/* ted.s:2446-2448: three labels inside ONE string (no 0 between them):
   ioname 'NEW', ioname2 'CON:0/0/640/', hiteasc '   /Blitz 2 CLI Window'
   (the 3 spaces are patched with the height) */
extern char ioname[];
#define ioname2 (ioname + 3)    /* ted.s:2447 */
#define hiteasc (ioname + 15)   /* ted.s:2448 */
extern BPTR oldih, oldoh;       /* ted.s:2451/2452 saved pr_CIS/pr_COS */
extern BPTR myhandle;           /* ted.s:2453 console handle */
extern UWORD helptoke;          /* ted.s:2767 */
extern ULONG nullcom;           /* ted.s:2614 empty string for Execute */
/* ted.s:2768/2769: helpex 'blitz2:help ' runs into helphex '1234',0 */
extern char helpex[];
#define helphex (helpex + 12)   /* ted.s:2769 patched with 4 hex digits */

#endif
