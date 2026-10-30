/*
 * data.h - chunk 07 (ted.s 6286-8072): Intuition data - NewScreen/
 * NewWindow, window gadgets and their borders, the menu strip, the
 * requesters (findreq, repall, errorreq, erasereq, defaultreq, prtreq),
 * undobuffer, alertstuff.  The code of the chunk (flink, the embedded
 * rawed/filereq hunks) is in rawkey.c / filereq.c.
 * Declared here: every label other chunks use (notes/xrefs.md), the alias
 * labels as #defines, and pointer tables for the fixed strides the asm
 * walks across structures (survey-07 G.1; DIFF: tables instead of
 * address arithmetic, same behaviour).
 * Requester initialisers: tools/req2c.py (fork of blitz2-c's, phase 2b);
 * menus, screen and window by hand.
 * Hooks implemented here: icongad, findcase (hooks.h).
 */
#ifndef TED_DATA_H
#define TED_DATA_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include "tedcore.h"

/* MenuItem directly followed by its IntuiText: getstuff copies 54 bytes
   from menu2item1 (= m2i1text - menu2item1 without the text) */
struct mitem {
    struct MenuItem item;       /* 34 */
    struct IntuiText it;        /* 20 */
};

/* ------------------------------------------------------ screen / window */

extern struct ExtNewScreen newscreen;           /* ted.s:6286 */
#define viewmode (newscreen.ViewModes)          /* ted.s:6288 */
extern char newsctit[32 + 80];                  /* ted.s:6312 screen title (patched) */
extern struct NewWindow newwindow;              /* ted.s:6316 */
#define idcmp     (newwindow.IDCMPFlags)        /* ted.s:6318 (unused) */
#define winscreen (newwindow.Screen)            /* ted.s:6321 */
extern struct Border newgadbord, nextngad;      /* ted.s:6324/6329 */
extern struct Border newgadbord2, nextngad2;    /* ted.s:6335/6340 */
extern WORD ngc1[6], ngc2[6], ngc3[6], ngc4[6]; /* ted.s:6328/6333/6339/6344 */
extern struct Gadget newgads;                   /* ted.s:6346 vertical prop, ID 0 */
extern struct PropInfo prop1sinfo;              /* ted.s:6359 */
extern struct Gadget newgads2;                  /* ted.s:6362 horizontal prop, ID 1 */
extern struct PropInfo prop2sinfo;              /* ted.s:6375 */

/* ------------------------------------------------------------- menus */

/* chain menu1 (PROJECT) -> menu4 (EDIT) -> menu3 (SOURCE) -> menu2
   (SEARCH) -> COMPILER (getstuff, at runtime) */
extern struct Menu menu1, menu2, menu3, menu4;  /* ted.s:6383/6554/6646/6704 */
extern char menu2name[];                        /* ted.s:6561 'SEARCH' */
extern struct mitem menu2item1;                 /* ted.s:6565 template for getstuff */
extern char m2i1text[];                         /* ted.s:6578 'FIND' */
/* ITEMENABLED toggled by blokmenus (bloken/blokdis) */
extern struct mitem menu4item1, menu4item2, menu4item4,  /* ted.s:6714/6730/6746 */
    menu4itema, menu4itemb;                     /* ted.s:6858/6874 */

/* -------------------------------------------------------- requesters */

extern struct Requester findreq;                /* ted.s:6906 */
extern struct Gadget findreqga1, findreqga2, findreqga8;   /* 7052/7063/7099 */
extern struct StringInfo findreqsg1, findreqsg2;           /* 7057/7068 */
/* findreqla16/findreqla17 (7061/7072): state.h (shared with the core) */
extern UBYTE undobuffer[192];                   /* ted.s:7105 shared UndoBuffer */
extern struct Requester repall;                 /* ted.s:7111 */
extern struct Requester errorreq;               /* ted.s:7195 */
extern struct IntuiText errorin1;               /* ted.s:7222 */
#define errorpnt (errorin1.IText)               /* ted.s:7225 */
extern struct Requester erasereq;               /* ted.s:7239 */
extern struct Requester defaultreq;             /* ted.s:7310 */
extern struct Border defaultreqbo14, defaultreqbo15;       /* ted.s:7426/7434 */
extern struct Border defaultreqla63;            /* ted.s:7430 second border of bo14 */
extern struct Gadget defaultreqga1;             /* ted.s:7781 first slider */
extern struct PropInfo defaultreqhp1, defaultreqhp2, defaultreqhp3; /* 7787/7795/7803 */
extern struct Gadget defaultreqga4, defaultreqga5;         /* ted.s:7805/7817 */
extern struct StringInfo defaultreqsg1, defaultreqsg2, defaultreqsg3; /* 7810/7822/7834 */
#define int1 (defaultreqsg1.LongInt)            /* ted.s:7813 (init 12) */
#define int2 (defaultreqsg2.LongInt)            /* ted.s:7825 (init 6) */
#define int3 (defaultreqsg3.LongInt)            /* ted.s:7837 (init 4, BUG(orig) #363, survey-07 I.1) */
extern UBYTE defaultreqla47[3], defaultreqla48[3], defaultreqla49[3]; /* 7814/7826/7838 */
extern struct Gadget defaultreqga7, defaultreqga8, defaultreqga9;     /* 7842/7847/7852 */
extern struct Gadget defaultreqga10, defaultreqga11;      /* ted.s:7857/7862 */
extern const char alertstuff[];                 /* ted.s:7948 */
extern struct Requester prtreq;                 /* ted.s:7971 */
extern struct Gadget prtreqga1;                 /* ted.s:8047 */
extern char prtname[64];                        /* ted.s:8056 'PRT:' */

/* fixed strides of the asm as pointer tables (survey-07 G.1) */
extern struct Gadget *const intgads[3];         /* ga4..ga6, stride 84 */
extern struct PropInfo *const hps[3];           /* hp1..hp3, stride 86 */
extern struct Gadget *const colgads[16];        /* ga10..ga25, stride 44 */
extern struct Border *const colbords[32];       /* bo14.. two per button, stride 28 */

/* icongad / findcase: hooks.h */

#endif
