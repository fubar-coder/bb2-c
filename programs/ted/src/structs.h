/*
 * structs.h - fixed layouts of the Amiga side: what blitz2 sees of TED
 * (comdata/mydata table, the line list, edstruct) and the dispatch table
 * entries.  Every offset that the original or blitz2 uses as a number is
 * asserted with OFFS()/SIZE() (m68k-amigaos-gcc aligns WORD/LONG/pointers
 * to 2 bytes, like Devpac's `even`).
 *
 * Only for the Amiga build (includes NDK headers).  The core layouts
 * (struct item, defblock ...) are defined in src/core/tedcore.h/state.h and
 * only asserted here.  Owner: coordinator (notes/changes.md).
 */
#ifndef TED_STRUCTS_H
#define TED_STRUCTS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <stddef.h>
#include "tedcore.h"
#include "state.h"

#define OFFS(type, field, off) \
    static_assert(offsetof(struct type, field) == (off), #type "." #field " must be at " #off)
#define SIZE(type, size) \
    static_assert(sizeof(struct type) == (size), "sizeof " #type " must be " #size)

/* ------------------------------------------------- core layouts (asserts) */

/* ted.s:15 rsreset - blitz2 walks this list (menurout a5 = firstitem) */
OFFS(item, nextitem, 0); OFFS(item, lastitem, 4);
OFFS(item, numchars, 8); OFFS(item, chars, 9);
static_assert(lenitem == offsetof(struct item, chars), "lenitem");

/* blitz2's token list node */
OFFS(tokennode, num, 4); OFFS(tokennode, name, 6);

/* mice node (remember2: AllocMem(8)) */
OFFS(micenode, line, 4); SIZE(micenode, 8);

/* option block = l:blitzeditor.opts (50 bytes) */
OFFS(defblock, d_fonthite, 0); OFFS(defblock, d_tokenise, 6);
OFFS(defblock, d_makeicons, 8); OFFS(defblock, d_normcol, 10);
OFFS(defblock, d_hscmarg, 18); OFFS(defblock, d_tabstop, 22);
OFFS(defblock, d_palit, 24); OFFS(defblock, d_origscrows, 48);
SIZE(defblock, 50);

/* -------------------------------------------------------- edstruct */

/* blitz2.s 274 (blitz2-c/src/structs.h struct edstruct): handed to
   outside in a1; TED copies these 28 bytes (14 words) into
   tokens..remark (state.h). */
struct edstruct {
    struct tokennode *toklist;  /* 0  -> tokens */
    UBYTE *menus;               /* 4  -> menitems */
    APTR loadxtra;              /* 8 */
    APTR savextra;              /* 12 */
    APTR menurout;              /* 16 */
    APTR cleanup;               /* 20 */
    UBYTE colon, quote, label, remark;  /* 24..27 */
};
SIZE(edstruct, 28);

/* ---------------------------------------------- mydata / comdata table */

/* ted.s:6027 mydata: service and data table for blitz2.  outside returns
   a1 = comdata = &mydata + 8; blitz2 also uses -8/-4.  Code entries are the
   register stubs of src/glue.c (REGSTUB: all registers saved, a4 of TED
   loaded, result flags from `tst.l d0`).  Data fields double as editor
   globals (#defines below). */
struct mydata {
    APTR m_screenclose;         /* -8  screenclose */
    APTR m_screenopen2;         /* -4  screenopen2 */
    APTR m_openreq;             /* 0   openreq      a0 = Requester template */
    APTR m_closereq;            /* 4   closereq */
    APTR m_getinput;            /* 8   getinput     -> d7, d6 */
    APTR m_error;               /* 12  error        a0 = text */
    APTR m_detokallz;           /* 16  detokallz */
    APTR m_tokall;              /* 20  tokall */
    APTR m_ezfreq;              /* 24  ezfreq       a0 path a1 name a2 title */
    APTR m_refgads;             /* 28  refgads      a0 gadget d0 count */
    APTR m_getinput2;           /* 32  getinput2    -> d7 (0 = none), d6 */
    UBYTE *m_combitmap;         /* 36  plane 0 of the screen (data) */
    struct Screen *m_myscreen;  /* 40  editor screen (data) */
    struct IntuitionBase *m_extint; /* 44 IntuitionBase (data) */
    APTR m_colors;              /* 48  colors */
    APTR m_halfcolors;          /* 52  halfcolors */
    APTR m_tokone;              /* 56  tokone       a0 = line */
    APTR m_openio;              /* 60  openio */
    APTR m_closeio;             /* 64  closeio */
    APTR m_actgad;              /* 68  actgad       a0 = gadget -> d0 */
    APTR m_pointeron;           /* 72  pointeron    (keeps registers) */
    APTR m_sleepon;             /* 76  sleepon      (keeps registers) */
    struct Window *m_mywindow2; /* 80  editor window (data) */
    UBYTE *m_iconflag;          /* 84  0; outside: &low byte of
                                       defaultreqga9.Flags (bit 7 = icons) */
    APTR m_extalloc;            /* 88  extalloc     d0 size d1 flags -> d0 */
    UWORD m_bmapmul;            /* 92  bytes per bitplane row (data) */
    struct Window *m_req_window;/* 94  current requester window (data) */
};
OFFS(mydata, m_openreq, 8);     /* comdata+0 */
OFFS(mydata, m_combitmap, 8 + 36);
OFFS(mydata, m_myscreen, 8 + 40);
OFFS(mydata, m_extint, 8 + 44);
OFFS(mydata, m_tokone, 8 + 56);
OFFS(mydata, m_mywindow2, 8 + 80);
OFFS(mydata, m_iconflag, 8 + 84);
OFFS(mydata, m_extalloc, 8 + 88);
OFFS(mydata, m_bmapmul, 8 + 92);
OFFS(mydata, m_req_window, 8 + 94);
SIZE(mydata, 8 + 98);

extern struct mydata mydata;            /* ted.s:6027 (input.c) */
#define comdata     ((APTR)((UBYTE *)&mydata + 8))      /* ted.s:6030 */
#define combitmap   (mydata.m_combitmap)                /* ted.s:6040 */
#define myscreen    (mydata.m_myscreen)                 /* ted.s:6041 */
#define extint      (mydata.m_extint)                   /* ted.s:6042 */
#define mywindow2   (mydata.m_mywindow2)                /* ted.s:6051 */
#define bmapmul     (mydata.m_bmapmul)                  /* ted.s:6054 */
#define req_window  (mydata.m_req_window)               /* ted.s:6055 */

/* ------------------------------------------------- dispatch tables */

/* ted.s:6171 jumptable / 6199 menutable: 6 byte entries {code.b, flag.b,
   routine.l} in the asm, searched with cmp.b; jumptable ends with code 0,
   menutable with 255.  C: an array of this struct in the same order
   (DIFF: C function pointers instead of the packed bytes; nobody else
   reads the tables).  flag != 0: showvprop after the routine. */
struct jtentry {
    UBYTE code;
    UBYTE flag;
    void (*routine)(void);
};

#endif
