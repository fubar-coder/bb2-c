/*
 * main.h - chunk 01, Amiga part (ted.s 1-1015 + wbheader, aboutreq,
 * specialreq): init/teardown (outside, .tomain, fail chain), standalone
 * start, mainedit/memcont, mouse pointer + busy pointer (VBlank server),
 * menus on/off, the options requester `defaults`, printing, load/save
 * requesters, about, find/replace requesters.
 * Core part of chunk 01: src/core/loadsave.h.
 * Hooks implemented here: pointeron, sleepon, showfname (hooks.h).
 * Glue entries implemented here: outside_c, tomain_c, edstart_c (glue.h).
 */
#ifndef TED_MAIN_H
#define TED_MAIN_H

#include <exec/types.h>
#include <exec/interrupts.h>
#include <intuition/intuition.h>
#include "tedcore.h"

/* ---------------------------------------------------- library bases */

/* asm: `dos` (wbheader), `int`/`gr` (macros openint/opengr, ted.s:33/35,
   data inside the code).  There is no libnix startup (-nostartfiles):
   SysBase is set from address 4 by edstart_c/outside_c. */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;          /* asm dos */
extern struct IntuitionBase *IntuitionBase; /* asm int (also comdata+44) */
extern struct GfxBase *GfxBase;             /* asm gr */

/* ---------------------------------------- start / init / end (glue.h) */

/* outside_c, tomain_c, edstart_c: glue.h.  The fail chain allocfailed
   (ted.s:116), openfailed (118), fail5 (128), fail3 (130), fail2 (131),
   fail1 (132) are goto labels at the end of outside_c, shared by the
   teardown after .domain (keep the `ted.s:NNN label` markers).
   wbheader's fromcli (wb:73), wb_exit (wb:88), dosopen (wb:97) and
   wb_allgo (wb:112) are parts of edstart_c / the teardown. */

/* ted.s:142 mainedit: main loop - getline, charstobuff/makeline if anyalt,
   dispatch through jumptable (setjmp = memstack/memcont, ted.s:167), then
   printmem/showmice/showvprop as flagged.  Returns when escape set edquit.
   `funcs` (ted.s:182) is an empty label behind it. */
void mainedit(void);
/* Fixed (#1007): TED cannot go on (its screen could not be reopened):
   longjmp to the frame of outside_c/tomain_c, which frees what escape
   frees (endfree), closes the screen if it is open and ends TED with
   return code -1, as a failed start (#357).  The caller shows the out of
   memory alert first (doalert).  Set from .tomain to the end of the
   editor (also over blitz2's frames: comdata-4 after a CloseEd run). */
NORETURN void tedfail(void);

/* ------------------------------------------------ pointer / busy pointer */

/* ted.s:184 mouseinit: chip copy of pointer+dumsleep (152 bytes).  FALSE
   when the AllocMem fails (fixed #367: the original did not check it,
   survey-01 I.5) */
BOOL mouseinit(void);
/* ted.s:200 mousefinit: ClearPointer, FreeMem (VBlank server not removed,
   survey-01 I.6) */
void mousefinit(void);
/* pointeron (ted.s:209), sleepon (ted.s:223): hooks.h - comdata+72/+76
   via the glue (registers kept) */
/* ted.s:242 setpoint: SetPointer(a1 chip data, height d0) on req_window
   (if any) and mywindow, falls into makemenus */
void setpoint(UWORD *data, WORD height);
/* ted.s:258 makemenus: req_window or sleepcurr -> menusoff, else menuson
   (also from closereq) */
void makemenus(void);
void menuson(void);             /* ted.s:263 clear WFLG_RMBTRAP (byte +25 bit 0) */
void menusoff(void);            /* ted.s:271 set WFLG_RMBTRAP */
/* ted.s:281 makedum: copy sleep frame a0 (20 longs from +4) to dumat+4 */
void makedum(const UBYTE *frame);
/* ted.s:291 myroutine: VBlank code (called by myint_stub, glue.c):
   animate the busy pointer every sleepwait+1 frames */
void myroutine(void);

extern struct Interrupt myint;  /* ted.s:306 NT_INTERRUPT, is_Code = myint_stub */
extern UWORD *pointerat;        /* ted.s:311 chip copy of pointer (152 bytes) */
extern UWORD *dumat;            /* ted.s:312 = pointerat + 64 bytes */
extern const UBYTE *volatile sleepcurr; /* ted.s:313 current frame in sleep_ (0 = off) */
extern volatile WORD sleepdelay;        /* ted.s:314 */
extern UBYTE dumsleep[88];      /* ted.s:322 template of the chip sleep frame */
/* sleep (316) / pointer (319) / srcinfo (827): keymap.h (generated res.c);
   sleepf (317), pointerf (320), dumsleepf (323), finish (325), srcinfof
   (828) are end labels: sizeof(). */

/* -------------------------------------------------- menu handlers */

/* ted.s:401 defaults - menu $30: the options requester */
void defaults(void);
/* ted.s:570 prtfile - menu $50: print requester, Open(prtname), lines via
   prtfile_line (loadsave.h), Write, Close */
void prtfile(void);
/* ted.s:678 fileopen - menu $10: erase (cancel: return), getfname
   (cancel: return), falls into fileopen2 (loadsave.h) */
void fileopen(void);
/* showfname (ted.s:731): hooks.h */
/* ted.s:737 showme: ShowTitle(myscreen, TRUE) (title changed in place) */
void showme(void);
/* ted.s:745 filesave - menu $20: getfname2, .bak (DeleteFile + Rename),
   falls into realsave (loadsave.h) */
void filesave(void);
/* ted.s:834 doabout - menu $40: aboutreq / specialreq */
void doabout(void);
/* ted.s:853 find - menu $03: find requester; actions are tail calls of
   next/previous/replace/replaceall */
void find(void);
/* ted.s:902 replaceall - menu $43: repall requester, replace2 per line */
void replaceall(void);

/* ------------------------------------------------- wbheader data */

extern struct Task *us;         /* wb: own task */
extern APTR wbmess;             /* wb: WBStartup message (0 = CLI) */
extern char progfile[192];      /* wb: file name argument (also .info buffer) */

/* aboutreq / specialreq (include files, chunk 01): requester templates for
   openreq, generated by tools/req2c.py in phase 2b */
extern struct Requester aboutreq, specialreq;

#endif
