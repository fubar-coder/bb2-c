/*
 * hooks.h - everything the core needs from outside: memory, files, the UI
 * routines the core calls (drawing, requesters, pointer) and the callbacks
 * into blitz2.  The seam between src/core (host testable) and src/ (Amiga).
 *
 * Implementations:
 *   Amiga  the UI routine of the named chunk (src/<chunk>.c) - the hook IS
 *          the original routine, with its original label as name, so a core
 *          call looks like the asm `bsr showall`;  ted_* in src/glue.c
 *   host   tests/host/stubs.c (records calls and their order, keeps the
 *          observable state changes, see the comment at each hook)
 *
 * Rules: a hook never changes core state except where noted ("state:");
 * the core calls hooks AFTER it changed its data, in the order of the asm
 * (PLAN.md 3.3).  Owner: coordinator - translators report needed hooks
 * (notes/PHASE2B-BRIEF.md), they do not add them themselves.
 */
#ifndef TED_HOOKS_H
#define TED_HOOKS_H

#include "tedcore.h"

struct defblock;                /* state.h */

/* ------------------------------------------------------------ memory */

/* exec AllocMem/FreeMem 1:1 (flags TED_MEMF_*); NULL on failure.
   Callers that must not fail use alloc2() (services.h) -> outmem().
   Host: adds slack for the larger struct item header, keeps the requested
   size and reports a FreeMem with a different size via ORIG_HAZARD (the
   join bug, survey-02 I.1). */
APTR ted_alloc(ULONG size, ULONG flags);
void ted_free(APTR p, ULONG size);

/* ted.s:3038 outmem (chunk 03, src/mouse.c): alert, close filehand,
   curstop = cursy = 0, thisitem = firstitem, showall, then longjmp to
   *memstack (asm: move.l memstack,a7 / bra memcont).  Never returns.
   Host: longjmp to the test's jmp_buf (tests/host/stubs.c). */
NORETURN void outmem(void);

/* -------------------------------------------------------------- files */

/* dos.library 1:1 (Amiga: src/glue.c; host: stdio).  Handles/locks are
   BPTRs (0 = failure).  mode: TED_MODE_OLDFILE/NEWFILE. */
tedfh ted_open(const char *name, LONG mode);
LONG  ted_read(tedfh fh, APTR buf, LONG len);      /* -1 = error */
LONG  ted_write(tedfh fh, const void *buf, LONG len);
void  ted_close(tedfh fh);
LONG  ted_rename(const char *oldname, const char *newname);   /* DOS BOOL */
LONG  ted_delete(const char *name);                /* DeleteFile, DOS BOOL */
tedfh ted_lock(const char *name, LONG mode);       /* TED_ACCESS_READ */
void  ted_unlock(tedfh lock);

/* -------------------------------------------- messages / requesters */

/* ted.s:3511 error (chunk 04, src/reqs.c; comdata+12): modal requester with
   `msg`, waits for OKAY.  Always returns. */
void error(const char *msg);
/* ted.s:3485 getbname (chunk 04, src/reqs.c): file requester for a block
   name (blokpath/blokname, title a2).  Returns the full name (asm: d7) or
   NULL on cancel (asm: addq #4,a7 - the CALLER returns at once).  No
   memory: outmem() (asm: dead branch, survey-07 I.3). */
UBYTE *getbname(const char *title);

/* ---------------------------------------------------- pointer / title */

void pointeron(void);           /* ted.s:209  chunk 01 normal pointer, menus on */
void sleepon(void);             /* ted.s:223  chunk 01 busy pointer, menus off */
void showfname(void);           /* ted.s:731  chunk 01 "File: "+filename as title */
void beepem(void);              /* ted.s:1054 chunk 02 pointeron + DisplayBeep */

/* ------------------------------------------------------------ drawing */

/* chunk 04 (src/reqs.c): draw lines from the list into chmap + bitmap.
   state: they fill chmap rows (showa2item writes chmap[chloc + col]) and
   may move cursy/scloc/chloc temporarily (storepos/restorepos around);
   showall destroys d7 (nothing to do in C).  Host stub: renders the lines
   into chmap like showa2item (detok + bit 7 for token characters). */
void showall(void);             /* ted.s:4050 */
void showtopitem(void);         /* ted.s:4073 */
void showbotitem(void);         /* ted.s:4088 */
void showthisitem(void);        /* ted.s:4102 */
/* block status text and the block menu items (chunk 04, src/reqs.c) */
void blokprnt2(void);           /* ted.s:3899 status line part bltext ("block"/spaces) */
/* NEW: the menu part of bloken/blokdis (ted.s:3890-3912): set/clear
   ITEMENABLED of menu4item1/2/4/a/b.  bloken/blokdis themselves are core
   (fileio.c): state first, then blokmenus(), bltext, blokprnt2(). */
void blokmenus(BOOL enable);

/* chunk 05 (src/screen.c): blitter scrolls of the text area.  state: the
   chmap part (rows moved, new row filled with spaces) is done by the core
   helpers chmap_scrollup/chmap_scrolldown/chmap_eraseline/chmap_cls
   (tokit.h), which the UI routine calls; the host stub calls only them. */
void scrollup(UWORD row);       /* ted.s:4346 rows row+1..edrows-1 up */
void scrupall(void);            /* ted.s:4345 scrollup(0) */
void scrolldown(UWORD row);     /* ted.s:4434 rows row..edrows-2 down */
void scrdownall(void);          /* ted.s:4433 scrolldown(0) */
void cls(void);                 /* ted.s:4986 chmap = spaces, text area = 0 */
void scrollrite(void);          /* ted.s:5019 bitmap 2 columns right (after cursoff-=2) */
void scrollleft(void);          /* ted.s:5069 bitmap 2 columns left (after cursoff+=2) */

/* chunk 06 (src/input.c) */
void refresh(void);             /* ted.s:5337 redraw the text area from chmap */
void showhprop(void);           /* ted.s:5305 column + horizontal prop */
void showvprop(void);           /* ted.s:5275 line + vertical prop */

/* --------------------------------------------- options / gadget state */

/* ted.s:3092 setdefs (chunk 04, src/reqs.c): option block -> globals and
   the gadgets of defaultreq.  loaddefs (core) ends with `bra setdefs`.
   state: copies the 25 words of *a5 to defstart (and clears a5->tokenise
   first if tokens == 0); host stub does exactly that. */
void setdefs(struct defblock *a5);
/* NEW: SELECTED of the "Create Icons" gadget (asm: btst #7,defaultreqga9+13,
   realsave ted.s:805 - the gadget, not `makeicons`, survey-01 I.15).
   chunk 07 (src/data.c). */
BOOL icongad(void);
/* NEW: SELECTED of the "Case sensitive" gadget (asm: btst #7,findreqga8+13,
   isithere2 ted.s:3543/3550).  chunk 07 (src/data.c). */
BOOL findcase(void);

/* --------------------------------------------------- blitz2 callbacks */

/* ted.s:2771 doclean (chunk 03, src/mouse.c): calls `cleanup` (if != 0)
   with all registers saved.  Host: counts. */
void doclean(void);
/* `loadxtra`/`savextra` (fileopen2 ted.s:710, realsave ted.s:795): jsr with
   d7 = file handle, a6 = DOSBase (src/glue.c via callregs; host: counts).
   Only called when the pointer is != 0. */
void call_loadxtra(tedfh fh);
void call_savextra(tedfh fh);

#endif
