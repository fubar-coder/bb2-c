/*
 * data.c - chunk 07 (ted.s 6286-8072): Intuition data, see data.h.
 * Screen, window, window gadgets and menus by hand (the menu structures
 * were converted once from the asm with a throw-away script and checked
 * byte by byte against the listing, tools/checkdata.py); the requesters
 * by tools/req2c.py.  Every label stays a global symbol (checkdata needs
 * the symbols in ted.sym), named as in the asm.
 */
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include "ted.h"

/* ------------------------------------------------------ screen / window */

struct TextAttr screenfont;
ULONG extnewscreen[7];
UWORD pens[10];

struct ExtNewScreen newscreen = {       /* ted.s:6286 */
    0, 0, 640, 200, 2,
    1, 2,
    0x8000 + 4, 0x100f,                 /* viewmode (6288): HIRES|LACE,
                                           overwritten by calcscreen */
    &screenfont, (UBYTE *)newsctit, NULL, NULL,
    (struct TagItem *)extnewscreen
};
struct TextAttr screenfont = {          /* ted.s:6293 */
    (STRPTR)"topaz.font", 8, 0, 0       /* screenfontname (6297) */
};
/* ted.s:6301 TagItem list; the TAG_END has no ti_Data (4 bytes) */
ULONG extnewscreen[7] = {
    (1UL << 31) + 32 + 0x1a, (ULONG)pens,       /* SA_Pens */
    0x80000047, 1,                      /* SA_LikeWorkbench - like wb simon was here */
    0x80000025, 2,                      /* SA_Depth: depth=2 */
    0                                   /* TAG_END */
};
UWORD pens[10] = { 1, 2, 2, 1, 2, 3, 1, 0, 3, (UWORD)-1 };     /* ted.s:6310 */
char newsctit[32 + 80] = "Blitz Basic 2 - Ted Version 1.1";   /* ted.s:6312 */

struct NewWindow newwindow = {          /* ted.s:6316 */
    0, 0, 640, 200,
    0, 1,
    0x400 + 0x40 + 0x100 + 0x8 + 0x20,  /* idcmp (6318) */
    0x100 + 0x800 + 0x1000,
    &newgads, NULL, NULL,
    NULL, NULL, 0, 0, 0, 0,             /* winscreen (6321) */
    15
};

struct Border newgadbord = {            /* ted.s:6324 */
    0, 0, 1, 0, 1, 3, ngc1, &nextngad
};
WORD ngc1[6] = { 15, 0, 0, 0, 0, 0 };   /* ted.s:6328 */
struct Border nextngad = {              /* ted.s:6329 */
    0, 0, 2, 0, 1, 3, ngc2, NULL
};
WORD ngc2[6] = { 15, 1, 15, 0, 1, 0 };  /* ted.s:6333 */

struct Border newgadbord2 = {           /* ted.s:6335 */
    0, 0, 1, 0, 1, 3, ngc3, &nextngad2
};
WORD ngc3[6] = { 0, 0, 0, 0, 0, 7 };    /* ted.s:6339 */
struct Border nextngad2 = {             /* ted.s:6340 */
    0, 0, 2, 0, 1, 3, ngc4, NULL
};
WORD ngc4[6] = { 0, 1, 0, 7, 0, 7 };    /* ted.s:6344 */

struct Image proprend, proprend2;       /* ted.s:6354/6370 AUTOKNOB buffers */

struct Gadget newgads = {               /* ted.s:6346 vertical prop */
    &newgads2, 0, 0, 10, 0,
    4, 3, 3,
    &proprend, NULL, NULL, 0, &prop1sinfo,
    0, NULL
};
struct PropInfo prop1sinfo = {          /* ted.s:6359 */
    13, (UWORD)-1, 0, (UWORD)-1, (UWORD)-1, 0, 0, 0, 0, 0, 0
};
struct Gadget newgads2 = {              /* ted.s:6362 horizontal prop */
    NULL, 0, 0, 0, 4,
    4, 1, 3,
    &proprend2, NULL, NULL, 0, &prop2sinfo,
    1, NULL
};
struct PropInfo prop2sinfo = {          /* ted.s:6375 */
    11, 0, 0, 0x8000, (UWORD)-1, 0, 0, 0, 0, 0, 0
};

/* ------------------------------------------------------------- menus */

/* mp1 = FrontPen, mp2 = BackPen, mj = JAM2 (ted.s 6379-6381) */
#define mp1     2
#define mp2     1
#define mj      1
/* DIFF: the asm item has `dc.b 'X '` = Command + the pad byte behind it
   (a space); C cannot initialise the padding of struct MenuItem, it is 0
   there (never read; docs/ted/deviations.md). */
struct mitem menu1item1, menu1item2, menu1item3, menu1item4, menu1item6,
    menu1item7, menu1item8, menu1item9, menu1item10, menu1item11,
    menu2item1, menu2itema, menu2item2, menu2item3, menu2item4,
    menu3item1, menu3item2, menu3item3, menu4item1, menu4item2,
    menu4item4, menu4item5, menu4item3, menu4item6, menu4item7,
    menu4item8, menu4item9, menu4itema, menu4itemb, menu4itemc;
char menu1name[] = "PROJECT";                       /* ted.s:6390 */
struct Menu menu1 = {    /* ted.s:6383 */
    &menu4, 0, 0, 64, 10, 1, menu1name, (struct MenuItem *)&menu1item1
};
char m1i1text[] = "NEW";                            /* ted.s:6407 */
struct mitem menu1item1 = {    /* ted.s:6393 */
    { (struct MenuItem *)&menu1item2, 0, 0, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu1item1.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i1text, NULL }
};
char m1i2text[] = "LOAD";                           /* ted.s:6423 */
struct mitem menu1item2 = {    /* ted.s:6410 */
    { (struct MenuItem *)&menu1item3, 0, 12, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu1item2.it, NULL, 'L', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i2text, NULL }
};
char m1i3text[] = "SAVE";                           /* ted.s:6439 */
struct mitem menu1item3 = {    /* ted.s:6426 */
    { (struct MenuItem *)&menu1item4, 0, 24, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu1item3.it, NULL, 'S', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i3text, NULL }
};
char m1i4text[] = "DEFAULTS";                       /* ted.s:6455 */
struct mitem menu1item4 = {    /* ted.s:6442 */
    { (struct MenuItem *)&menu1item6, 0, 36, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu1item4.it, NULL, '?', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i4text, NULL }
};
char m1i6text[] = "ABOUT";                          /* ted.s:6471 */
struct mitem menu1item6 = {    /* ted.s:6458 */
    { (struct MenuItem *)&menu1item7, 0, 48, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu1item6.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i6text, NULL }
};
char m1i7text[] = "PRINT";                          /* ted.s:6487 */
struct mitem menu1item7 = {    /* ted.s:6474 */
    { (struct MenuItem *)&menu1item8, 0, 60, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu1item7.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i7text, NULL }
};
char m1i8text[] = "CLI";                            /* ted.s:6503 */
struct mitem menu1item8 = {    /* ted.s:6490 */
    { (struct MenuItem *)&menu1item9, 0, 72, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu1item8.it, NULL, 'Z', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i8text, NULL }
};
char m1i9text[] = "CLOSE WORKBENCH";                /* ted.s:6519 */
struct mitem menu1item9 = {    /* ted.s:6506 */
    { (struct MenuItem *)&menu1item10, 0, 84, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu1item9.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i9text, NULL }
};
char m1i10text[] = "OPEN WORKBENCH";                /* ted.s:6535 */
struct mitem menu1item10 = {    /* ted.s:6522 */
    { (struct MenuItem *)&menu1item11, 0, 96, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu1item10.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i10text, NULL }
};
char m1i11text[] = "QUIT";                          /* ted.s:6551 */
struct mitem menu1item11 = {    /* ted.s:6538 */
    { NULL, 0, 108, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu1item11.it, NULL, 'Q', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m1i11text, NULL }
};
char menu2name[15] = "SEARCH";                      /* ted.s:6561 */
struct Menu menu2 = {    /* ted.s:6554 */
    NULL, 184, 0, 56, 10, 1, menu2name, (struct MenuItem *)&menu2item1
};
char m2i1text[17] = "FIND";                         /* ted.s:6578 */
struct mitem menu2item1 = {    /* ted.s:6565 */
    { (struct MenuItem *)&menu2itema, 0, 0, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu2item1.it, NULL, 'F', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m2i1text, NULL }
};
char m2iatext[] = "NEXT";                           /* ted.s:6595 */
struct mitem menu2itema = {    /* ted.s:6582 */
    { (struct MenuItem *)&menu2item2, 0, 12, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu2itema.it, NULL, 'N', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m2iatext, NULL }
};
char m2i2text[] = "PREVIOUS";                       /* ted.s:6611 */
struct mitem menu2item2 = {    /* ted.s:6598 */
    { (struct MenuItem *)&menu2item3, 0, 24, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu2item2.it, NULL, 'P', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m2i2text, NULL }
};
char m2i3text[] = "REPLACE";                        /* ted.s:6627 */
struct mitem menu2item3 = {    /* ted.s:6614 */
    { (struct MenuItem *)&menu2item4, 0, 36, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu2item3.it, NULL, 'R', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m2i3text, NULL }
};
char m2i4text[] = "REPLACE ALL";                    /* ted.s:6643 */
struct mitem menu2item4 = {    /* ted.s:6630 */
    { NULL, 0, 48, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu2item4.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m2i4text, NULL }
};
char menu3name[] = "SOURCE";                        /* ted.s:6653 */
struct Menu menu3 = {    /* ted.s:6646 */
    &menu2, 120, 0, 56, 10, 1, menu3name, (struct MenuItem *)&menu3item1
};
char m3i1text[] = "TOP";                            /* ted.s:6669 */
struct mitem menu3item1 = {    /* ted.s:6656 */
    { (struct MenuItem *)&menu3item2, 0, 0, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu3item1.it, NULL, 'T', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m3i1text, NULL }
};
char m3i2text[] = "BOTTOM";                         /* ted.s:6685 */
struct mitem menu3item2 = {    /* ted.s:6672 */
    { (struct MenuItem *)&menu3item3, 0, 12, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu3item2.it, NULL, 'B', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m3i2text, NULL }
};
char m3i3text[] = "GOTO LINE";                      /* ted.s:6701 */
struct mitem menu3item3 = {    /* ted.s:6688 */
    { NULL, 0, 24, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu3item3.it, NULL, 'G', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m3i3text, NULL }
};
char menu4name[] = "EDIT";                          /* ted.s:6711 */
struct Menu menu4 = {    /* ted.s:6704 */
    &menu3, 72, 0, 40, 10, 1, menu4name, (struct MenuItem *)&menu4item1
};
char m4i1text[] = "COPY";                           /* ted.s:6727 */
struct mitem menu4item1 = {    /* ted.s:6714 */
    { (struct MenuItem *)&menu4item2, 0, 0, 160, 12, 2 + 64 + 4, 0,
      (APTR)&menu4item1.it, NULL, 'C', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i1text, NULL }
};
char m4i2text[] = "KILL";                           /* ted.s:6743 */
struct mitem menu4item2 = {    /* ted.s:6730 */
    { (struct MenuItem *)&menu4item4, 0, 12, 160, 12, 2 + 64 + 4, 0,
      (APTR)&menu4item2.it, NULL, 'K', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i2text, NULL }
};
char m4i4text[] = "BLOCK TO DISK";                  /* ted.s:6759 */
struct mitem menu4item4 = {    /* ted.s:6746 */
    { (struct MenuItem *)&menu4item5, 0, 24, 160, 12, 2 + 64, 0,
      (APTR)&menu4item4.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i4text, NULL }
};
char m4i5text[] = "INSERT FROM DISK";               /* ted.s:6775 */
struct mitem menu4item5 = {    /* ted.s:6762 */
    { (struct MenuItem *)&menu4item3, 0, 36, 160, 12, 2 + 16 + 64, 0,
      (APTR)&menu4item5.it, NULL, ' ', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i5text, NULL }
};
char m4i3text[] = "FORGET";                         /* ted.s:6791 */
struct mitem menu4item3 = {    /* ted.s:6778 */
    { (struct MenuItem *)&menu4item6, 0, 48, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4item3.it, NULL, 'W', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i3text, NULL }
};
char m4i6text[] = "INSERT LINE";                    /* ted.s:6807 */
struct mitem menu4item6 = {    /* ted.s:6794 */
    { (struct MenuItem *)&menu4item7, 0, 60, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4item6.it, NULL, 'I', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i6text, NULL }
};
char m4i7text[] = "DELETE LINE";                    /* ted.s:6823 */
struct mitem menu4item7 = {    /* ted.s:6810 */
    { (struct MenuItem *)&menu4item8, 0, 72, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4item7.it, NULL, 'D', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i7text, NULL }
};
char m4i8text[] = "DELETE RIGHT";                   /* ted.s:6839 */
struct mitem menu4item8 = {    /* ted.s:6826 */
    { (struct MenuItem *)&menu4item9, 0, 84, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4item8.it, NULL, 'Y', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i8text, NULL }
};
char m4i9text[] = "JOIN";                           /* ted.s:6855 */
struct mitem menu4item9 = {    /* ted.s:6842 */
    { (struct MenuItem *)&menu4itema, 0, 96, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4item9.it, NULL, 'J', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4i9text, NULL }
};
char m4iatext[] = "BLOCK TAB";                      /* ted.s:6871 */
struct mitem menu4itema = {    /* ted.s:6858 */
    { (struct MenuItem *)&menu4itemb, 0, 108, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4itema.it, NULL, ']', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4iatext, NULL }
};
char m4ibtext[] = "BLOCK UNTAB";                    /* ted.s:6887 */
struct mitem menu4itemb = {    /* ted.s:6874 */
    { (struct MenuItem *)&menu4itemc, 0, 120, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4itemb.it, NULL, '[', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4ibtext, NULL }
};
char m4ictext[] = "MARK INDENT";                    /* ted.s:6903 */
struct mitem menu4itemc = {    /* ted.s:6890 */
    { NULL, 0, 132, 160, 12, 2 + 16 + 64 + 4, 0,
      (APTR)&menu4itemc.it, NULL, 'A', NULL, 0 },
    { mp1, mp2, mj, 2, 2, NULL, (UBYTE *)m4ictext, NULL }
};

/* -------------------------------------------------------- requesters */

UBYTE undobuffer[192];                  /* ted.s:7105 shared UndoBuffer */

/* findreq (6906), repall (7111), errorreq (7195), erasereq (7239),
   defaultreq (7310), prtreq (7971): templates for openreq.  Buffers
   findreqla16/17 are in state.c (shared with the core). */
/* regenerate: tools/req2c.py TEDDIR data --update src/data.c */
/* req2c:begin data */
/* generated by tools/req2c.py data - do not edit, run it again */
/* forward declarations */
struct Requester findreq;
struct Border findreqbo1;
WORD findreqla1[6];
struct Border findreqla2;
WORD findreqla3[6];
struct Border findreqbo2;
WORD findreqla4[6];
struct Border findreqla5;
WORD findreqla6[6];
struct Border findreqbo3;
WORD findreqla7[6];
struct Border findreqla8;
WORD findreqla9[6];
struct Border findreqbo4;
WORD findreqla10[6];
struct Border findreqla11;
WORD findreqla12[6];
struct Border findreqbo5;
WORD findreqla18[6];
struct Border findreqla19;
WORD findreqla20[6];
struct Border findreqbo6;
WORD findreqla22[6];
struct Border findreqla23;
WORD findreqla24[6];
struct Border findreqbo7;
WORD findreqla26[6];
struct Border findreqla27;
WORD findreqla28[6];
struct Border findreqbo8;
WORD findreqla30[6];
struct Border findreqla31;
WORD findreqla32[6];
struct Border findreqbo9;
WORD findreqla34[6];
struct Border findreqla35;
WORD findreqla36[6];
struct Border findreqbo10;
WORD findreqla38[6];
struct Border findreqla39;
WORD findreqla40[6];
struct IntuiText findreqin1;
UBYTE findreqla13[9];
struct IntuiText findreqin2;
UBYTE findreqla14[6];
struct IntuiText findreqin3;
UBYTE findreqla15[13];
struct IntuiText findreqin4;
UBYTE findreqla21[5];
struct IntuiText findreqin5;
UBYTE findreqla25[9];
struct IntuiText findreqin6;
UBYTE findreqla29[8];
struct IntuiText findreqin7;
UBYTE findreqla33[12];
struct IntuiText findreqin8;
UBYTE findreqla37[7];
struct IntuiText findreqin9;
UBYTE findreqla41[15];
struct Gadget findreqga1;
struct StringInfo findreqsg1;
struct Gadget findreqga2;
struct StringInfo findreqsg2;
struct Gadget findreqga3;
struct Gadget findreqga4;
struct Gadget findreqga5;
struct Gadget findreqga6;
struct Gadget findreqga7;
struct Gadget findreqga8;
struct Requester repall;
struct Border repallbo1;
WORD repallla1[6];
struct Border repallla2;
WORD repallla3[6];
struct Border repallbo2;
WORD repallla5[6];
struct Border repallla6;
WORD repallla7[6];
struct Border repallbo3;
WORD repallla9[6];
struct Border repallla10;
WORD repallla11[6];
struct Border repallbo4;
WORD repallla13[6];
struct Border repallla14;
WORD repallla15[6];
struct IntuiText repallin1;
UBYTE repallla4[17];
struct IntuiText repallin2;
UBYTE repallla8[10];
struct IntuiText repallin3;
UBYTE repallla12[9];
struct IntuiText repallin4;
UBYTE repallla16[7];
struct Gadget repallga1;
struct Gadget repallga2;
struct Gadget repallga3;
struct Requester errorreq;
struct Border errorbo1;
WORD errorla1[6];
struct Border errorla2;
WORD errorla3[6];
struct Border errorbo2;
WORD errorla5[6];
struct Border errorla6;
WORD errorla7[6];
struct IntuiText errorin1;
struct IntuiText errorin2;
UBYTE errorla8[7];
struct Gadget errorga1;
struct Requester erasereq;
struct Border erasereqbo1;
WORD erasereqla1[6];
struct Border erasereqla2;
WORD erasereqla3[6];
struct Border erasereqbo2;
WORD erasereqla6[6];
struct Border erasereqla7;
WORD erasereqla8[6];
struct Border erasereqbo3;
WORD erasereqla10[6];
struct Border erasereqla11;
WORD erasereqla12[6];
struct IntuiText erasereqin1;
UBYTE erasereqla4[30];
struct IntuiText erasereqin2;
UBYTE erasereqla5[10];
struct IntuiText erasereqin3;
UBYTE erasereqla9[5];
struct IntuiText erasereqin4;
UBYTE erasereqla13[9];
struct Gadget erasereqga1;
struct Gadget erasereqga2;
struct Requester defaultreq;
struct Border defaultreqbo1;
WORD defaultreqla1[6];
struct Border defaultreqla2;
WORD defaultreqla3[6];
struct Border defaultreqbo2;
WORD defaultreqla4[6];
struct Border defaultreqla5;
WORD defaultreqla6[6];
struct Border defaultreqbo3;
WORD defaultreqla7[6];
struct Border defaultreqla8;
WORD defaultreqla9[6];
struct Border defaultreqbo4;
WORD defaultreqla10[6];
struct Border defaultreqla11;
WORD defaultreqla12[6];
struct Border defaultreqbo5;
WORD defaultreqla13[6];
struct Border defaultreqla14;
WORD defaultreqla15[6];
struct Border defaultreqbo6;
WORD defaultreqla16[6];
struct Border defaultreqla17;
WORD defaultreqla18[6];
struct Border defaultreqbo7;
WORD defaultreqla19[6];
struct Border defaultreqla20;
WORD defaultreqla21[6];
struct Border defaultreqbo8;
WORD defaultreqla36[6];
struct Border defaultreqla37;
WORD defaultreqla38[6];
struct Border defaultreqbo9;
WORD defaultreqla40[6];
struct Border defaultreqla41;
WORD defaultreqla42[6];
struct Border defaultreqbo10;
WORD defaultreqla44[6];
struct Border defaultreqla45;
WORD defaultreqla46[6];
struct Border defaultreqbo11;
WORD defaultreqla50[6];
struct Border defaultreqla51;
WORD defaultreqla52[6];
struct Border defaultreqbo12;
WORD defaultreqla54[6];
struct Border defaultreqla55;
WORD defaultreqla56[6];
struct Border defaultreqbo13;
WORD defaultreqla58[6];
struct Border defaultreqla59;
WORD defaultreqla60[6];
struct Border defaultreqbo14;
WORD defaultreqla62[6];
struct Border defaultreqla63;
WORD defaultreqla64[6];
struct Border defaultreqbo15;
WORD defaultreqla66[6];
struct Border defaultreqla67;
WORD defaultreqla68[6];
struct Border defaultreqbo16;
WORD defaultreqla70[6];
struct Border defaultreqla71;
WORD defaultreqla72[6];
struct Border defaultreqbo17;
WORD defaultreqla74[6];
struct Border defaultreqla75;
WORD defaultreqla76[6];
struct Border defaultreqbo18;
WORD defaultreqla78[6];
struct Border defaultreqla79;
WORD defaultreqla80[6];
struct Border defaultreqbo19;
WORD defaultreqla82[6];
struct Border defaultreqla83;
WORD defaultreqla84[6];
struct Border defaultreqbo20;
WORD defaultreqla86[6];
struct Border defaultreqla87;
WORD defaultreqla88[6];
struct Border defaultreqbo21;
WORD defaultreqla90[6];
struct Border defaultreqla91;
WORD defaultreqla92[6];
struct Border defaultreqbo22;
WORD defaultreqla94[6];
struct Border defaultreqla95;
WORD defaultreqla96[6];
struct Border defaultreqbo23;
WORD defaultreqla98[6];
struct Border defaultreqla99;
WORD defaultreqla100[6];
struct Border defaultreqbo24;
WORD defaultreqla102[6];
struct Border defaultreqla103;
WORD defaultreqla104[6];
struct Border defaultreqbo25;
WORD defaultreqla106[6];
struct Border defaultreqla107;
WORD defaultreqla108[6];
struct Border defaultreqbo26;
WORD defaultreqla110[6];
struct Border defaultreqla111;
WORD defaultreqla112[6];
struct Border defaultreqbo27;
WORD defaultreqla114[6];
struct Border defaultreqla115;
WORD defaultreqla116[6];
struct Border defaultreqbo28;
WORD defaultreqla118[6];
struct Border defaultreqla119;
WORD defaultreqla120[6];
struct Border defaultreqbo29;
WORD defaultreqla122[6];
struct Border defaultreqla123;
WORD defaultreqla124[6];
struct Border defaultreqbo30;
WORD defaultreqla126[6];
struct Border defaultreqla127;
WORD defaultreqla128[6];
struct Border defaultreqbo31;
WORD defaultreqla130[6];
struct Border defaultreqla131;
WORD defaultreqla132[6];
struct IntuiText defaultreqin1;
UBYTE defaultreqla22[16];
struct IntuiText defaultreqin2;
UBYTE defaultreqla23[15];
struct IntuiText defaultreqin3;
UBYTE defaultreqla24[9];
struct IntuiText defaultreqin4;
UBYTE defaultreqla25[13];
struct IntuiText defaultreqin5;
UBYTE defaultreqla26[25];
struct IntuiText defaultreqin6;
UBYTE defaultreqla27[23];
struct IntuiText defaultreqin7;
UBYTE defaultreqla28[9];
struct IntuiText defaultreqin8;
UBYTE defaultreqla29[13];
struct IntuiText defaultreqin9;
UBYTE defaultreqla30[1];
struct IntuiText defaultreqin10;
UBYTE defaultreqla31[14];
struct IntuiText defaultreqin11;
UBYTE defaultreqla32[5];
struct IntuiText defaultreqin12;
UBYTE defaultreqla33[7];
struct IntuiText defaultreqin13;
UBYTE defaultreqla34[6];
struct IntuiText defaultreqin14;
UBYTE defaultreqla53[3];
struct IntuiText defaultreqin15;
UBYTE defaultreqla57[3];
struct IntuiText defaultreqin16;
UBYTE defaultreqla61[3];
struct IntuiText defaultreqin17;
UBYTE defaultreqla65[2];
struct IntuiText defaultreqin18;
UBYTE defaultreqla69[2];
struct IntuiText defaultreqin19;
UBYTE defaultreqla73[2];
struct IntuiText defaultreqin20;
UBYTE defaultreqla77[2];
struct IntuiText defaultreqin21;
UBYTE defaultreqla81[2];
struct IntuiText defaultreqin22;
UBYTE defaultreqla85[2];
struct IntuiText defaultreqin23;
UBYTE defaultreqla89[2];
struct IntuiText defaultreqin24;
UBYTE defaultreqla93[2];
struct IntuiText defaultreqin25;
UBYTE defaultreqla97[2];
struct IntuiText defaultreqin26;
UBYTE defaultreqla101[2];
struct IntuiText defaultreqin27;
UBYTE defaultreqla105[2];
struct IntuiText defaultreqin28;
UBYTE defaultreqla109[2];
struct IntuiText defaultreqin29;
UBYTE defaultreqla113[9];
struct IntuiText defaultreqin30;
UBYTE defaultreqla117[9];
struct IntuiText defaultreqin31;
UBYTE defaultreqla121[9];
struct IntuiText defaultreqin32;
UBYTE defaultreqla125[9];
struct IntuiText defaultreqin33;
UBYTE defaultreqla129[5];
struct IntuiText defaultreqin34;
UBYTE defaultreqla133[9];
struct Gadget defaultreqga1;
struct Image defaultreqla35;
struct PropInfo defaultreqhp1;
struct Gadget defaultreqga2;
struct Image defaultreqla39;
struct PropInfo defaultreqhp2;
struct Gadget defaultreqga3;
struct Image defaultreqla43;
struct PropInfo defaultreqhp3;
struct Gadget defaultreqga4;
struct StringInfo defaultreqsg1;
UBYTE defaultreqla47[3];
struct Gadget defaultreqga5;
struct StringInfo defaultreqsg2;
UBYTE defaultreqla48[3];
struct Gadget defaultreqga6;
struct StringInfo defaultreqsg3;
UBYTE defaultreqla49[3];
struct Gadget defaultreqga7;
struct Gadget defaultreqga8;
struct Gadget defaultreqga9;
struct Gadget defaultreqga10;
struct Gadget defaultreqga11;
struct Gadget defaultreqga12;
struct Gadget defaultreqga13;
struct Gadget defaultreqga14;
struct Gadget defaultreqga15;
struct Gadget defaultreqga16;
struct Gadget defaultreqga17;
struct Gadget defaultreqga18;
struct Gadget defaultreqga19;
struct Gadget defaultreqga20;
struct Gadget defaultreqga21;
struct Gadget defaultreqga22;
struct Gadget defaultreqga23;
struct Gadget defaultreqga24;
struct Gadget defaultreqga25;
struct Gadget defaultreqga26;
struct Gadget defaultreqga27;
struct Requester prtreq;
struct Border prtreqbo1;
WORD prtreqla1[6];
struct Border prtreqla2;
WORD prtreqla3[6];
struct Border prtreqbo2;
WORD prtreqla4[6];
struct Border prtreqla5;
WORD prtreqla6[6];
struct Border prtreqbo3;
WORD prtreqla7[6];
struct Border prtreqla8;
WORD prtreqla9[6];
struct Border prtreqbo4;
WORD prtreqla13[6];
struct Border prtreqla14;
WORD prtreqla15[6];
struct Border prtreqbo5;
WORD prtreqla17[6];
struct Border prtreqla18;
WORD prtreqla19[6];
struct IntuiText prtreqin1;
UBYTE prtreqla10[16];
struct IntuiText prtreqin2;
UBYTE prtreqla11[11];
struct IntuiText prtreqin3;
UBYTE prtreqla16[8];
struct IntuiText prtreqin4;
UBYTE prtreqla20[9];
struct Gadget prtreqga1;
struct StringInfo prtreqsg1;
char prtname[64];
struct Gadget prtreqga2;
struct Gadget prtreqga3;

struct Requester findreq = {NULL, 80, 6, 376, 92, 0, 0, (struct Gadget *)&findreqga1, (struct Border *)&findreqbo1, (struct IntuiText *)&findreqin1};  /* ted.s:6906 */
struct Border findreqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla1, (struct Border *)&findreqla2};  /* ted.s:6916 */
WORD findreqla1[6] = {359, 40, 80, 40, 80, 51};  /* ted.s:6919 */
struct Border findreqla2 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla3, (struct Border *)&findreqbo2};  /* ted.s:6920 */
WORD findreqla3[6] = {359, 41, 359, 51, 81, 51};  /* ted.s:6923 */
struct Border findreqbo2 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla4, (struct Border *)&findreqla5};  /* ted.s:6924 */
WORD findreqla4[6] = {359, 24, 80, 24, 80, 35};  /* ted.s:6927 */
struct Border findreqla5 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla6, (struct Border *)&findreqbo3};  /* ted.s:6928 */
WORD findreqla6[6] = {359, 25, 359, 35, 81, 35};  /* ted.s:6931 */
struct Border findreqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla7, (struct Border *)&findreqla8};  /* ted.s:6932 */
WORD findreqla7[6] = {367, 20, 8, 20, 8, 55};  /* ted.s:6935 */
struct Border findreqla8 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla9, (struct Border *)&findreqbo4};  /* ted.s:6936 */
WORD findreqla9[6] = {367, 21, 367, 55, 9, 55};  /* ted.s:6939 */
struct Border findreqbo4 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla10, (struct Border *)&findreqla11};  /* ted.s:6940 */
WORD findreqla10[6] = {375, 0, 0, 0, 0, 91};  /* ted.s:6943 */
struct Border findreqla11 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla12, NULL};  /* ted.s:6944 */
WORD findreqla12[6] = {375, 1, 375, 91, 1, 91};  /* ted.s:6947 */
struct Border findreqbo5 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla18, (struct Border *)&findreqla19};  /* ted.s:6948 */
WORD findreqla18[6] = {39, 0, 0, 0, 0, 11};  /* ted.s:6951 */
struct Border findreqla19 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla20, NULL};  /* ted.s:6952 */
WORD findreqla20[6] = {39, 1, 39, 11, 1, 11};  /* ted.s:6955 */
struct Border findreqbo6 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla22, (struct Border *)&findreqla23};  /* ted.s:6956 */
WORD findreqla22[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:6959 */
struct Border findreqla23 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla24, NULL};  /* ted.s:6960 */
WORD findreqla24[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:6963 */
struct Border findreqbo7 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla26, (struct Border *)&findreqla27};  /* ted.s:6964 */
WORD findreqla26[6] = {63, 0, 0, 0, 0, 11};  /* ted.s:6967 */
struct Border findreqla27 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla28, NULL};  /* ted.s:6968 */
WORD findreqla28[6] = {63, 1, 63, 11, 1, 11};  /* ted.s:6971 */
struct Border findreqbo8 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla30, (struct Border *)&findreqla31};  /* ted.s:6972 */
WORD findreqla30[6] = {95, 0, 0, 0, 0, 11};  /* ted.s:6975 */
struct Border findreqla31 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla32, NULL};  /* ted.s:6976 */
WORD findreqla32[6] = {95, 1, 95, 11, 1, 11};  /* ted.s:6979 */
struct Border findreqbo9 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla34, (struct Border *)&findreqla35};  /* ted.s:6980 */
WORD findreqla34[6] = {55, 0, 0, 0, 0, 11};  /* ted.s:6983 */
struct Border findreqla35 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla36, NULL};  /* ted.s:6984 */
WORD findreqla36[6] = {55, 1, 55, 11, 1, 11};  /* ted.s:6987 */
struct Border findreqbo10 = {0, 0, 1, 2, 1, 3, (WORD *)findreqla38, (struct Border *)&findreqla39};  /* ted.s:6988 */
WORD findreqla38[6] = {119, 0, 0, 0, 0, 11};  /* ted.s:6991 */
struct Border findreqla39 = {0, 0, 2, 1, 1, 3, (WORD *)findreqla40, NULL};  /* ted.s:6992 */
WORD findreqla40[6] = {119, 1, 119, 11, 1, 11};  /* ted.s:6995 */
struct IntuiText findreqin1 = {1, 0, 1, 12, 42, NULL, (UBYTE *)findreqla13, (struct IntuiText *)&findreqin2};  /* ted.s:6997 */
UBYTE findreqla13[9] = "REPLACE:";  /* ted.s:7001 */
struct IntuiText findreqin2 = {1, 0, 1, 12, 26, NULL, (UBYTE *)findreqla14, (struct IntuiText *)&findreqin3};  /* ted.s:7003 */
UBYTE findreqla14[6] = "FIND:";  /* ted.s:7007 */
struct IntuiText findreqin3 = {1, 0, 1, 12, 6, NULL, (UBYTE *)findreqla15, NULL};  /* ted.s:7009 */
UBYTE findreqla15[13] = "Find/Replace";  /* ted.s:7013 */
struct IntuiText findreqin4 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla21, NULL};  /* ted.s:7015 */
UBYTE findreqla21[5] = "NEXT";  /* ted.s:7019 */
struct IntuiText findreqin5 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla25, NULL};  /* ted.s:7021 */
UBYTE findreqla25[9] = "PREVIOUS";  /* ted.s:7025 */
struct IntuiText findreqin6 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla29, NULL};  /* ted.s:7027 */
UBYTE findreqla29[8] = "REPLACE";  /* ted.s:7031 */
struct IntuiText findreqin7 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla33, NULL};  /* ted.s:7033 */
UBYTE findreqla33[12] = "REPLACE ALL";  /* ted.s:7037 */
struct IntuiText findreqin8 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla37, NULL};  /* ted.s:7039 */
UBYTE findreqla37[7] = "CANCEL";  /* ted.s:7043 */
struct IntuiText findreqin9 = {1, 0, 1, 4, 2, NULL, (UBYTE *)findreqla41, NULL};  /* ted.s:7045 */
UBYTE findreqla41[15] = "CASE SENSITIVE";  /* ted.s:7049 */
struct Gadget findreqga1 = {(struct Gadget *)&findreqga2, 84, 26, 272, 8, 0x200, 1, 4, NULL, NULL, NULL, 0, (APTR)&findreqsg1, 1, NULL};  /* ted.s:7052 */
struct StringInfo findreqsg1 = {(UBYTE *)findreqla16, (UBYTE *)undobuffer, 0, 64, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};  /* ted.s:7057 */
struct Gadget findreqga2 = {(struct Gadget *)&findreqga3, 84, 42, 272, 8, 0x200, 5, 4, NULL, NULL, NULL, 0, (APTR)&findreqsg2, 2, NULL};  /* ted.s:7063 */
struct StringInfo findreqsg2 = {(UBYTE *)findreqla17, (UBYTE *)undobuffer, 0, 64, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};  /* ted.s:7068 */
struct Gadget findreqga3 = {(struct Gadget *)&findreqga4, 8, 76, 40, 12, 0, 5, 1, (APTR)&findreqbo5, NULL, (struct IntuiText *)&findreqin4, 0, NULL, 3, NULL};  /* ted.s:7074 */
struct Gadget findreqga4 = {(struct Gadget *)&findreqga5, 56, 76, 72, 12, 0, 5, 1, (APTR)&findreqbo6, NULL, (struct IntuiText *)&findreqin5, 0, NULL, 4, NULL};  /* ted.s:7079 */
struct Gadget findreqga5 = {(struct Gadget *)&findreqga6, 136, 76, 64, 12, 0, 5, 1, (APTR)&findreqbo7, NULL, (struct IntuiText *)&findreqin6, 0, NULL, 5, NULL};  /* ted.s:7084 */
struct Gadget findreqga6 = {(struct Gadget *)&findreqga7, 208, 76, 96, 12, 0, 5, 1, (APTR)&findreqbo8, NULL, (struct IntuiText *)&findreqin7, 0, NULL, 6, NULL};  /* ted.s:7089 */
struct Gadget findreqga7 = {(struct Gadget *)&findreqga8, 312, 76, 56, 12, 0, 5, 1, (APTR)&findreqbo9, NULL, (struct IntuiText *)&findreqin8, 0, NULL, 7, NULL};  /* ted.s:7094 */
struct Gadget findreqga8 = {NULL, 136, 60, 120, 12, 0, 0x101, 1, (APTR)&findreqbo10, NULL, (struct IntuiText *)&findreqin9, 0, NULL, 8, NULL};  /* ted.s:7099 */
struct Requester repall = {NULL, 132, 46, 240, 40, 0, 0, (struct Gadget *)&repallga1, (struct Border *)&repallbo1, (struct IntuiText *)&repallin1};  /* ted.s:7111 */
struct Border repallbo1 = {0, 0, 1, 2, 1, 3, (WORD *)repallla1, (struct Border *)&repallla2};  /* ted.s:7121 */
WORD repallla1[6] = {239, 0, 0, 0, 0, 39};  /* ted.s:7124 */
struct Border repallla2 = {0, 0, 2, 1, 1, 3, (WORD *)repallla3, NULL};  /* ted.s:7125 */
WORD repallla3[6] = {239, 1, 239, 39, 1, 39};  /* ted.s:7128 */
struct Border repallbo2 = {0, 0, 1, 2, 1, 3, (WORD *)repallla5, (struct Border *)&repallla6};  /* ted.s:7129 */
WORD repallla5[6] = {79, 0, 0, 0, 0, 11};  /* ted.s:7132 */
struct Border repallla6 = {0, 0, 2, 1, 1, 3, (WORD *)repallla7, NULL};  /* ted.s:7133 */
WORD repallla7[6] = {79, 1, 79, 11, 1, 11};  /* ted.s:7136 */
struct Border repallbo3 = {0, 0, 1, 2, 1, 3, (WORD *)repallla9, (struct Border *)&repallla10};  /* ted.s:7137 */
WORD repallla9[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7140 */
struct Border repallla10 = {0, 0, 2, 1, 1, 3, (WORD *)repallla11, NULL};  /* ted.s:7141 */
WORD repallla11[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7144 */
struct Border repallbo4 = {0, 0, 1, 2, 1, 3, (WORD *)repallla13, (struct Border *)&repallla14};  /* ted.s:7145 */
WORD repallla13[6] = {55, 0, 0, 0, 0, 11};  /* ted.s:7148 */
struct Border repallla14 = {0, 0, 2, 1, 1, 3, (WORD *)repallla15, NULL};  /* ted.s:7149 */
WORD repallla15[6] = {55, 1, 55, 11, 1, 11};  /* ted.s:7152 */
struct IntuiText repallin1 = {1, 0, 1, 12, 6, NULL, (UBYTE *)repallla4, NULL};  /* ted.s:7154 */
UBYTE repallla4[17] = "Replace all.....";  /* ted.s:7158 */
struct IntuiText repallin2 = {1, 0, 1, 4, 2, NULL, (UBYTE *)repallla8, NULL};  /* ted.s:7160 */
UBYTE repallla8[10] = "FROM HERE";  /* ted.s:7164 */
struct IntuiText repallin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)repallla12, NULL};  /* ted.s:7166 */
UBYTE repallla12[9] = "FROM TOP";  /* ted.s:7170 */
struct IntuiText repallin4 = {1, 0, 1, 4, 2, NULL, (UBYTE *)repallla16, NULL};  /* ted.s:7172 */
UBYTE repallla16[7] = "CANCEL";  /* ted.s:7176 */
struct Gadget repallga1 = {(struct Gadget *)&repallga2, 8, 24, 80, 12, 0, 5, 1, (APTR)&repallbo2, NULL, (struct IntuiText *)&repallin2, 0, NULL, 1, NULL};  /* ted.s:7179 */
struct Gadget repallga2 = {(struct Gadget *)&repallga3, 96, 24, 72, 12, 0, 5, 1, (APTR)&repallbo3, NULL, (struct IntuiText *)&repallin3, 0, NULL, 2, NULL};  /* ted.s:7184 */
struct Gadget repallga3 = {NULL, 176, 24, 56, 12, 0, 5, 1, (APTR)&repallbo4, NULL, (struct IntuiText *)&repallin4, 0, NULL, 3, NULL};  /* ted.s:7189 */
struct Requester errorreq = {NULL, 128, 58, 312, 40, 0, 0, (struct Gadget *)&errorga1, (struct Border *)&errorbo1, (struct IntuiText *)&errorin1};  /* ted.s:7195 */
struct Border errorbo1 = {0, 0, 1, 2, 1, 3, (WORD *)errorla1, (struct Border *)&errorla2};  /* ted.s:7205 */
WORD errorla1[6] = {311, 0, 0, 0, 0, 39};  /* ted.s:7208 */
struct Border errorla2 = {0, 0, 2, 1, 1, 3, (WORD *)errorla3, NULL};  /* ted.s:7209 */
WORD errorla3[6] = {311, 1, 311, 39, 1, 39};  /* ted.s:7212 */
struct Border errorbo2 = {0, 0, 1, 2, 1, 3, (WORD *)errorla5, (struct Border *)&errorla6};  /* ted.s:7213 */
WORD errorla5[6] = {55, 0, 0, 0, 0, 11};  /* ted.s:7216 */
struct Border errorla6 = {0, 0, 2, 1, 1, 3, (WORD *)errorla7, NULL};  /* ted.s:7217 */
WORD errorla7[6] = {55, 1, 55, 11, 1, 11};  /* ted.s:7220 */
struct IntuiText errorin1 = {1, 0, 1, 12, 6, NULL, NULL, NULL};  /* ted.s:7222 */
struct IntuiText errorin2 = {1, 0, 1, 4, 2, NULL, (UBYTE *)errorla8, NULL};  /* ted.s:7226 */
UBYTE errorla8[7] = " OKAY ";  /* ted.s:7230 */
struct Gadget errorga1 = {NULL, 128, 24, 56, 12, 0, 5, 1, (APTR)&errorbo2, NULL, (struct IntuiText *)&errorin2, 0, NULL, 1, NULL};  /* ted.s:7233 */
struct Requester erasereq = {NULL, 128, 50, 256, 48, 0, 0, (struct Gadget *)&erasereqga1, (struct Border *)&erasereqbo1, (struct IntuiText *)&erasereqin1};  /* ted.s:7239 */
struct Border erasereqbo1 = {0, 0, 1, 2, 1, 3, (WORD *)erasereqla1, (struct Border *)&erasereqla2};  /* ted.s:7249 */
WORD erasereqla1[6] = {255, 0, 0, 0, 0, 47};  /* ted.s:7252 */
struct Border erasereqla2 = {0, 0, 2, 1, 1, 3, (WORD *)erasereqla3, NULL};  /* ted.s:7253 */
WORD erasereqla3[6] = {255, 1, 255, 47, 1, 47};  /* ted.s:7256 */
struct Border erasereqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)erasereqla6, (struct Border *)&erasereqla7};  /* ted.s:7257 */
WORD erasereqla6[6] = {39, 0, 0, 0, 0, 11};  /* ted.s:7260 */
struct Border erasereqla7 = {0, 0, 2, 1, 1, 3, (WORD *)erasereqla8, NULL};  /* ted.s:7261 */
WORD erasereqla8[6] = {39, 1, 39, 11, 1, 11};  /* ted.s:7264 */
struct Border erasereqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)erasereqla10, (struct Border *)&erasereqla11};  /* ted.s:7265 */
WORD erasereqla10[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7268 */
struct Border erasereqla11 = {0, 0, 2, 1, 1, 3, (WORD *)erasereqla12, NULL};  /* ted.s:7269 */
WORD erasereqla12[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7272 */
struct IntuiText erasereqin1 = {1, 0, 1, 12, 6, NULL, (UBYTE *)erasereqla4, (struct IntuiText *)&erasereqin2};  /* ted.s:7274 */
UBYTE erasereqla4[30] = "This will erase altered file!";  /* ted.s:7278 */
struct IntuiText erasereqin2 = {1, 0, 1, 12, 18, NULL, (UBYTE *)erasereqla5, NULL};  /* ted.s:7280 */
UBYTE erasereqla5[10] = "Continue?";  /* ted.s:7284 */
struct IntuiText erasereqin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)erasereqla9, NULL};  /* ted.s:7286 */
UBYTE erasereqla9[5] = " OK ";  /* ted.s:7290 */
struct IntuiText erasereqin4 = {1, 0, 1, 4, 2, NULL, (UBYTE *)erasereqla13, NULL};  /* ted.s:7292 */
UBYTE erasereqla13[9] = " CANCEL ";  /* ted.s:7296 */
struct Gadget erasereqga1 = {(struct Gadget *)&erasereqga2, 8, 32, 40, 12, 0, 5, 1, (APTR)&erasereqbo2, NULL, (struct IntuiText *)&erasereqin3, 0, NULL, 1, NULL};  /* ted.s:7299 */
struct Gadget erasereqga2 = {NULL, 176, 32, 72, 12, 0, 5, 1, (APTR)&erasereqbo3, NULL, (struct IntuiText *)&erasereqin4, 0, NULL, 2, NULL};  /* ted.s:7304 */
struct Requester defaultreq = {NULL, 60, 1, 400, 172, 0, 0, (struct Gadget *)&defaultreqga1, (struct Border *)&defaultreqbo1, (struct IntuiText *)&defaultreqin1};  /* ted.s:7310 */
struct Border defaultreqbo1 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla1, (struct Border *)&defaultreqla2};  /* ted.s:7320 */
WORD defaultreqla1[6] = {47, 52, 16, 52, 16, 63};  /* ted.s:7323 */
struct Border defaultreqla2 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla3, (struct Border *)&defaultreqbo2};  /* ted.s:7324 */
WORD defaultreqla3[6] = {47, 53, 47, 63, 17, 63};  /* ted.s:7327 */
struct Border defaultreqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla4, (struct Border *)&defaultreqla5};  /* ted.s:7328 */
WORD defaultreqla4[6] = {47, 64, 16, 64, 16, 75};  /* ted.s:7331 */
struct Border defaultreqla5 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla6, (struct Border *)&defaultreqbo3};  /* ted.s:7332 */
WORD defaultreqla6[6] = {47, 65, 47, 75, 17, 75};  /* ted.s:7335 */
struct Border defaultreqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla7, (struct Border *)&defaultreqla8};  /* ted.s:7336 */
WORD defaultreqla7[6] = {47, 76, 16, 76, 16, 87};  /* ted.s:7339 */
struct Border defaultreqla8 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla9, (struct Border *)&defaultreqbo4};  /* ted.s:7340 */
WORD defaultreqla9[6] = {47, 77, 47, 87, 17, 87};  /* ted.s:7343 */
struct Border defaultreqbo4 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla10, (struct Border *)&defaultreqla11};  /* ted.s:7344 */
WORD defaultreqla10[6] = {255, 12, 8, 12, 8, 91};  /* ted.s:7347 */
struct Border defaultreqla11 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla12, (struct Border *)&defaultreqbo5};  /* ted.s:7348 */
WORD defaultreqla12[6] = {255, 13, 255, 91, 9, 91};  /* ted.s:7351 */
struct Border defaultreqbo5 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla13, (struct Border *)&defaultreqla14};  /* ted.s:7352 */
WORD defaultreqla13[6] = {391, 12, 264, 12, 264, 91};  /* ted.s:7355 */
struct Border defaultreqla14 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla15, (struct Border *)&defaultreqbo6};  /* ted.s:7356 */
WORD defaultreqla15[6] = {391, 13, 391, 91, 265, 91};  /* ted.s:7359 */
struct Border defaultreqbo6 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla16, (struct Border *)&defaultreqla17};  /* ted.s:7360 */
WORD defaultreqla16[6] = {391, 96, 8, 96, 8, 151};  /* ted.s:7363 */
struct Border defaultreqla17 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla18, (struct Border *)&defaultreqbo7};  /* ted.s:7364 */
WORD defaultreqla18[6] = {391, 97, 391, 151, 9, 151};  /* ted.s:7367 */
struct Border defaultreqbo7 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla19, (struct Border *)&defaultreqla20};  /* ted.s:7368 */
WORD defaultreqla19[6] = {399, 0, 0, 0, 0, 171};  /* ted.s:7371 */
struct Border defaultreqla20 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla21, (struct Border *)&defaultreqbo8};  /* ted.s:7372 */
WORD defaultreqla21[6] = {399, 1, 399, 171, 1, 171};  /* ted.s:7375 */
struct Border defaultreqbo8 = {4, 0, 1, 2, 1, 3, (WORD *)defaultreqla36, (struct Border *)&defaultreqla37};  /* ted.s:7376 */
WORD defaultreqla36[6] = {379, 118, 68, 118, 68, 125};  /* ted.s:7379 */
struct Border defaultreqla37 = {4, 0, 2, 1, 1, 3, (WORD *)defaultreqla38, (struct Border *)&defaultreqbo9};  /* ted.s:7380 */
WORD defaultreqla38[6] = {379, 119, 379, 125, 69, 125};  /* ted.s:7383 */
struct Border defaultreqbo9 = {4, 0, 1, 2, 1, 3, (WORD *)defaultreqla40, (struct Border *)&defaultreqla41};  /* ted.s:7384 */
WORD defaultreqla40[6] = {379, 130, 68, 130, 68, 137};  /* ted.s:7387 */
struct Border defaultreqla41 = {4, 0, 2, 1, 1, 3, (WORD *)defaultreqla42, (struct Border *)&defaultreqbo10};  /* ted.s:7388 */
WORD defaultreqla42[6] = {379, 131, 379, 137, 69, 137};  /* ted.s:7391 */
struct Border defaultreqbo10 = {4, 0, 1, 2, 1, 3, (WORD *)defaultreqla44, (struct Border *)&defaultreqla45};  /* ted.s:7392 */
WORD defaultreqla44[6] = {379, 142, 68, 142, 68, 149};  /* ted.s:7395 */
struct Border defaultreqla45 = {4, 0, 2, 1, 1, 3, (WORD *)defaultreqla46, NULL};  /* ted.s:7396 */
WORD defaultreqla46[6] = {379, 143, 379, 149, 69, 149};  /* ted.s:7399 */
struct Border defaultreqbo11 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla50, (struct Border *)&defaultreqla51};  /* ted.s:7400 */
WORD defaultreqla50[6] = {31, 0, 0, 0, 0, 11};  /* ted.s:7403 */
struct Border defaultreqla51 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla52, NULL};  /* ted.s:7404 */
WORD defaultreqla52[6] = {31, 1, 31, 11, 1, 11};  /* ted.s:7407 */
struct Border defaultreqbo12 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla54, (struct Border *)&defaultreqla55};  /* ted.s:7408 */
WORD defaultreqla54[6] = {31, 0, 0, 0, 0, 11};  /* ted.s:7411 */
struct Border defaultreqla55 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla56, NULL};  /* ted.s:7412 */
WORD defaultreqla56[6] = {31, 1, 31, 11, 1, 11};  /* ted.s:7415 */
struct Border defaultreqbo13 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla58, (struct Border *)&defaultreqla59};  /* ted.s:7416 */
WORD defaultreqla58[6] = {31, 0, 0, 0, 0, 11};  /* ted.s:7419 */
struct Border defaultreqla59 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla60, NULL};  /* ted.s:7420 */
WORD defaultreqla60[6] = {31, 1, 31, 11, 1, 11};  /* ted.s:7423 */
struct Border defaultreqbo14 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla62, (struct Border *)&defaultreqla63};  /* ted.s:7426 */
WORD defaultreqla62[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7429 */
struct Border defaultreqla63 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla64, NULL};  /* ted.s:7430 */
WORD defaultreqla64[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7433 */
struct Border defaultreqbo15 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla66, (struct Border *)&defaultreqla67};  /* ted.s:7434 */
WORD defaultreqla66[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7437 */
struct Border defaultreqla67 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla68, NULL};  /* ted.s:7438 */
WORD defaultreqla68[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7441 */
struct Border defaultreqbo16 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla70, (struct Border *)&defaultreqla71};  /* ted.s:7442 */
WORD defaultreqla70[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7445 */
struct Border defaultreqla71 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla72, NULL};  /* ted.s:7446 */
WORD defaultreqla72[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7449 */
struct Border defaultreqbo17 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla74, (struct Border *)&defaultreqla75};  /* ted.s:7450 */
WORD defaultreqla74[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7453 */
struct Border defaultreqla75 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla76, NULL};  /* ted.s:7454 */
WORD defaultreqla76[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7457 */
struct Border defaultreqbo18 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla78, (struct Border *)&defaultreqla79};  /* ted.s:7458 */
WORD defaultreqla78[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7461 */
struct Border defaultreqla79 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla80, NULL};  /* ted.s:7462 */
WORD defaultreqla80[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7465 */
struct Border defaultreqbo19 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla82, (struct Border *)&defaultreqla83};  /* ted.s:7466 */
WORD defaultreqla82[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7469 */
struct Border defaultreqla83 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla84, NULL};  /* ted.s:7470 */
WORD defaultreqla84[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7473 */
struct Border defaultreqbo20 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla86, (struct Border *)&defaultreqla87};  /* ted.s:7474 */
WORD defaultreqla86[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7477 */
struct Border defaultreqla87 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla88, NULL};  /* ted.s:7478 */
WORD defaultreqla88[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7481 */
struct Border defaultreqbo21 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla90, (struct Border *)&defaultreqla91};  /* ted.s:7482 */
WORD defaultreqla90[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7485 */
struct Border defaultreqla91 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla92, NULL};  /* ted.s:7486 */
WORD defaultreqla92[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7489 */
struct Border defaultreqbo22 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla94, (struct Border *)&defaultreqla95};  /* ted.s:7490 */
WORD defaultreqla94[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7493 */
struct Border defaultreqla95 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla96, NULL};  /* ted.s:7494 */
WORD defaultreqla96[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7497 */
struct Border defaultreqbo23 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla98, (struct Border *)&defaultreqla99};  /* ted.s:7498 */
WORD defaultreqla98[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7501 */
struct Border defaultreqla99 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla100, NULL};  /* ted.s:7502 */
WORD defaultreqla100[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7505 */
struct Border defaultreqbo24 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla102, (struct Border *)&defaultreqla103};  /* ted.s:7506 */
WORD defaultreqla102[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7509 */
struct Border defaultreqla103 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla104, NULL};  /* ted.s:7510 */
WORD defaultreqla104[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7513 */
struct Border defaultreqbo25 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla106, (struct Border *)&defaultreqla107};  /* ted.s:7514 */
WORD defaultreqla106[6] = {15, 0, 0, 0, 0, 11};  /* ted.s:7517 */
struct Border defaultreqla107 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla108, NULL};  /* ted.s:7518 */
WORD defaultreqla108[6] = {15, 1, 15, 11, 1, 11};  /* ted.s:7521 */
struct Border defaultreqbo26 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla110, (struct Border *)&defaultreqla111};  /* ted.s:7522 */
WORD defaultreqla110[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7525 */
struct Border defaultreqla111 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla112, NULL};  /* ted.s:7526 */
WORD defaultreqla112[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7529 */
struct Border defaultreqbo27 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla114, (struct Border *)&defaultreqla115};  /* ted.s:7530 */
WORD defaultreqla114[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7533 */
struct Border defaultreqla115 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla116, NULL};  /* ted.s:7534 */
WORD defaultreqla116[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7537 */
struct Border defaultreqbo28 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla118, (struct Border *)&defaultreqla119};  /* ted.s:7538 */
WORD defaultreqla118[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7541 */
struct Border defaultreqla119 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla120, NULL};  /* ted.s:7542 */
WORD defaultreqla120[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7545 */
struct Border defaultreqbo29 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla122, (struct Border *)&defaultreqla123};  /* ted.s:7546 */
WORD defaultreqla122[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7549 */
struct Border defaultreqla123 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla124, NULL};  /* ted.s:7550 */
WORD defaultreqla124[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7553 */
struct Border defaultreqbo30 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla126, (struct Border *)&defaultreqla127};  /* ted.s:7558 */
WORD defaultreqla126[6] = {39, 0, 0, 0, 0, 11};  /* ted.s:7561 */
struct Border defaultreqla127 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla128, NULL};  /* ted.s:7562 */
WORD defaultreqla128[6] = {39, 1, 39, 11, 1, 11};  /* ted.s:7565 */
struct Border defaultreqbo31 = {0, 0, 1, 2, 1, 3, (WORD *)defaultreqla130, (struct Border *)&defaultreqla131};  /* ted.s:7566 */
WORD defaultreqla130[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:7569 */
struct Border defaultreqla131 = {0, 0, 2, 1, 1, 3, (WORD *)defaultreqla132, NULL};  /* ted.s:7570 */
WORD defaultreqla132[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:7573 */
struct IntuiText defaultreqin1 = {1, 0, 1, 20, 2, NULL, (UBYTE *)defaultreqla22, (struct IntuiText *)&defaultreqin2};  /* ted.s:7575 */
UBYTE defaultreqla22[16] = "Default Options";  /* ted.s:7579 */
struct IntuiText defaultreqin2 = {1, 0, 1, 56, 18, NULL, (UBYTE *)defaultreqla23, (struct IntuiText *)&defaultreqin3};  /* ted.s:7581 */
UBYTE defaultreqla23[15] = "Use Small Font";  /* ted.s:7585 */
struct IntuiText defaultreqin3 = {1, 0, 1, 56, 30, NULL, (UBYTE *)defaultreqla24, (struct IntuiText *)&defaultreqin4};  /* ted.s:7587 */
UBYTE defaultreqla24[9] = "Tokenise";  /* ted.s:7591 */
struct IntuiText defaultreqin4 = {1, 0, 1, 56, 42, NULL, (UBYTE *)defaultreqla25, (struct IntuiText *)&defaultreqin5};  /* ted.s:7593 */
UBYTE defaultreqla25[13] = "Create Icons";  /* ted.s:7597 */
struct IntuiText defaultreqin5 = {1, 0, 1, 56, 54, NULL, (UBYTE *)defaultreqla26, (struct IntuiText *)&defaultreqin6};  /* ted.s:7599 */
UBYTE defaultreqla26[25] = "Horizontal Scroll Margin";  /* ted.s:7603 */
struct IntuiText defaultreqin6 = {1, 0, 1, 56, 66, NULL, (UBYTE *)defaultreqla27, (struct IntuiText *)&defaultreqin7};  /* ted.s:7605 */
UBYTE defaultreqla27[23] = "Vertical Scroll Margin";  /* ted.s:7609 */
struct IntuiText defaultreqin7 = {1, 0, 1, 56, 78, NULL, (UBYTE *)defaultreqla28, (struct IntuiText *)&defaultreqin8};  /* ted.s:7611 */
UBYTE defaultreqla28[9] = "Tab Stop";  /* ted.s:7615 */
struct IntuiText defaultreqin8 = {1, 0, 1, 276, 18+8, NULL, (UBYTE *)defaultreqla29, (struct IntuiText *)&defaultreqin9};  /* ted.s:7617 */
UBYTE defaultreqla29[13] = "Text Colour:";  /* ted.s:7621 */
struct IntuiText defaultreqin9 = {1, 0, 1, 276, 42, NULL, (UBYTE *)defaultreqla30, (struct IntuiText *)&defaultreqin10};  /* ted.s:7623 */
UBYTE defaultreqla30[1] = "";  /* ted.s:7627 */
struct IntuiText defaultreqin10 = {1, 0, 1, 276, 66-8, NULL, (UBYTE *)defaultreqla31, (struct IntuiText *)&defaultreqin11};  /* ted.s:7629 */
UBYTE defaultreqla31[14] = "Token Colour:";  /* ted.s:7633 */
struct IntuiText defaultreqin11 = {1, 0, 1, 20, 118, NULL, (UBYTE *)defaultreqla32, (struct IntuiText *)&defaultreqin12};  /* ted.s:7635 */
UBYTE defaultreqla32[5] = "Red:";  /* ted.s:7639 */
struct IntuiText defaultreqin12 = {1, 0, 1, 20, 130, NULL, (UBYTE *)defaultreqla33, (struct IntuiText *)&defaultreqin13};  /* ted.s:7641 */
UBYTE defaultreqla33[7] = "Green:";  /* ted.s:7645 */
struct IntuiText defaultreqin13 = {1, 0, 1, 20, 142, NULL, (UBYTE *)defaultreqla34, NULL};  /* ted.s:7647 */
UBYTE defaultreqla34[6] = "Blue:";  /* ted.s:7651 */
struct IntuiText defaultreqin14 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla53, NULL};  /* ted.s:7653 */
UBYTE defaultreqla53[3] = "  ";  /* ted.s:7657 */
struct IntuiText defaultreqin15 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla57, NULL};  /* ted.s:7659 */
UBYTE defaultreqla57[3] = "  ";  /* ted.s:7663 */
struct IntuiText defaultreqin16 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla61, NULL};  /* ted.s:7665 */
UBYTE defaultreqla61[3] = "  ";  /* ted.s:7669 */
struct IntuiText defaultreqin17 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla65, NULL};  /* ted.s:7671 */
UBYTE defaultreqla65[2] = " ";  /* ted.s:7675 */
struct IntuiText defaultreqin18 = {1, 1, 1, 4, 2, NULL, (UBYTE *)defaultreqla69, NULL};  /* ted.s:7677 */
UBYTE defaultreqla69[2] = " ";  /* ted.s:7681 */
struct IntuiText defaultreqin19 = {1, 2, 1, 4, 2, NULL, (UBYTE *)defaultreqla73, NULL};  /* ted.s:7683 */
UBYTE defaultreqla73[2] = " ";  /* ted.s:7687 */
struct IntuiText defaultreqin20 = {1, 3, 1, 4, 2, NULL, (UBYTE *)defaultreqla77, NULL};  /* ted.s:7689 */
UBYTE defaultreqla77[2] = " ";  /* ted.s:7693 */
struct IntuiText defaultreqin21 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla81, NULL};  /* ted.s:7695 */
UBYTE defaultreqla81[2] = " ";  /* ted.s:7699 */
struct IntuiText defaultreqin22 = {1, 1, 1, 4, 2, NULL, (UBYTE *)defaultreqla85, NULL};  /* ted.s:7701 */
UBYTE defaultreqla85[2] = " ";  /* ted.s:7705 */
struct IntuiText defaultreqin23 = {1, 2, 1, 4, 2, NULL, (UBYTE *)defaultreqla89, NULL};  /* ted.s:7707 */
UBYTE defaultreqla89[2] = " ";  /* ted.s:7711 */
struct IntuiText defaultreqin24 = {1, 3, 1, 4, 2, NULL, (UBYTE *)defaultreqla93, NULL};  /* ted.s:7713 */
UBYTE defaultreqla93[2] = " ";  /* ted.s:7717 */
struct IntuiText defaultreqin25 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla97, NULL};  /* ted.s:7719 */
UBYTE defaultreqla97[2] = " ";  /* ted.s:7723 */
struct IntuiText defaultreqin26 = {1, 1, 1, 4, 2, NULL, (UBYTE *)defaultreqla101, NULL};  /* ted.s:7725 */
UBYTE defaultreqla101[2] = " ";  /* ted.s:7729 */
struct IntuiText defaultreqin27 = {1, 2, 1, 4, 2, NULL, (UBYTE *)defaultreqla105, NULL};  /* ted.s:7731 */
UBYTE defaultreqla105[2] = " ";  /* ted.s:7735 */
struct IntuiText defaultreqin28 = {1, 3, 1, 4, 2, NULL, (UBYTE *)defaultreqla109, NULL};  /* ted.s:7737 */
UBYTE defaultreqla109[2] = " ";  /* ted.s:7741 */
struct IntuiText defaultreqin29 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla113, NULL};  /* ted.s:7743 */
UBYTE defaultreqla113[9] = "        ";  /* ted.s:7747 */
struct IntuiText defaultreqin30 = {1, 1, 1, 4, 2, NULL, (UBYTE *)defaultreqla117, NULL};  /* ted.s:7749 */
UBYTE defaultreqla117[9] = "        ";  /* ted.s:7753 */
struct IntuiText defaultreqin31 = {1, 2, 1, 4, 2, NULL, (UBYTE *)defaultreqla121, NULL};  /* ted.s:7755 */
UBYTE defaultreqla121[9] = "        ";  /* ted.s:7759 */
struct IntuiText defaultreqin32 = {1, 3, 1, 4, 2, NULL, (UBYTE *)defaultreqla125, NULL};  /* ted.s:7761 */
UBYTE defaultreqla125[9] = "        ";  /* ted.s:7765 */
struct IntuiText defaultreqin33 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla129, NULL};  /* ted.s:7767 */
UBYTE defaultreqla129[5] = " OK ";  /* ted.s:7771 */
struct IntuiText defaultreqin34 = {1, 0, 1, 4, 2, NULL, (UBYTE *)defaultreqla133, NULL};  /* ted.s:7773 */
UBYTE defaultreqla133[9] = " CANCEL ";  /* ted.s:7777 */
struct Gadget defaultreqga1 = {(struct Gadget *)&defaultreqga2, 75, 120, 306, 4, 0, 3, 3, (APTR)&defaultreqla35, NULL, NULL, 0, (APTR)&defaultreqhp1, 1, NULL};  /* ted.s:7781 */
struct Image defaultreqla35 = {0};  /* ted.s:7786 */
struct PropInfo defaultreqhp1 = {11, (6<<16)/15, (UWORD)-1, 4096, (UWORD)-1, 0, 0, 0, 0, 0, 0};  /* ted.s:7787 */
struct Gadget defaultreqga2 = {(struct Gadget *)&defaultreqga3, 75, 132, 306, 4, 0, 3, 3, (APTR)&defaultreqla39, NULL, NULL, 0, (APTR)&defaultreqhp2, 2, NULL};  /* ted.s:7789 */
struct Image defaultreqla39 = {0};  /* ted.s:7794 */
struct PropInfo defaultreqhp2 = {11, (6<<16)/15, (UWORD)-1, 4096, (UWORD)-1, 0, 0, 0, 0, 0, 0};  /* ted.s:7795 */
struct Gadget defaultreqga3 = {(struct Gadget *)&defaultreqga4, 75, 144, 306, 4, 0, 3, 3, (APTR)&defaultreqla43, NULL, NULL, 0, (APTR)&defaultreqhp3, 3, NULL};  /* ted.s:7797 */
struct Image defaultreqla43 = {0};  /* ted.s:7802 */
struct PropInfo defaultreqhp3 = {11, (6<<16)/15, (UWORD)-1, 4096, (UWORD)-1, 0, 0, 0, 0, 0, 0};  /* ted.s:7803 */
struct Gadget defaultreqga4 = {(struct Gadget *)&defaultreqga5, 20, 54, 24, 8, 0, 2049, 4, NULL, NULL, NULL, 0, (APTR)&defaultreqsg1, 4, NULL};  /* ted.s:7805 */
struct StringInfo defaultreqsg1 = {(UBYTE *)defaultreqla47, (UBYTE *)undobuffer, 0, 3, 0, 0, 0, 0, 0, 0, NULL, 12, NULL};  /* ted.s:7810 */
UBYTE defaultreqla47[3] = "12";  /* ted.s:7814 */
struct Gadget defaultreqga5 = {(struct Gadget *)&defaultreqga6, 20, 66, 24, 8, 0, 2049, 4, NULL, NULL, NULL, 0, (APTR)&defaultreqsg2, 5, NULL};  /* ted.s:7817 */
struct StringInfo defaultreqsg2 = {(UBYTE *)defaultreqla48, (UBYTE *)undobuffer, 0, 3, 0, 0, 0, 0, 0, 0, NULL, 6, NULL};  /* ted.s:7822 */
UBYTE defaultreqla48[3] = "6";  /* ted.s:7826 */
struct Gadget defaultreqga6 = {(struct Gadget *)&defaultreqga7, 20, 78, 24, 8, 0, 2049, 4, NULL, NULL, NULL, 0, (APTR)&defaultreqsg3, 6, NULL};  /* ted.s:7829 */
struct StringInfo defaultreqsg3 = {(UBYTE *)defaultreqla49, (UBYTE *)undobuffer, 0, 3, 0, 0, 0, 0, 0, 0, NULL, 4, NULL};  /* ted.s:7834 */
UBYTE defaultreqla49[3] = "2";  /* ted.s:7838 */
struct Gadget defaultreqga7 = {(struct Gadget *)&defaultreqga8, 16, 16, 32, 12, 0, 257, 1, (APTR)&defaultreqbo11, NULL, (struct IntuiText *)&defaultreqin14, 0, NULL, 7, NULL};  /* ted.s:7842 */
struct Gadget defaultreqga8 = {(struct Gadget *)&defaultreqga9, 16, 28, 32, 12, 0, 257, 1, (APTR)&defaultreqbo12, NULL, (struct IntuiText *)&defaultreqin15, 0, NULL, 8, NULL};  /* ted.s:7847 */
struct Gadget defaultreqga9 = {(struct Gadget *)&defaultreqga10, 16, 40, 32, 12, 128, 257, 1, (APTR)&defaultreqbo13, NULL, (struct IntuiText *)&defaultreqin16, 0, NULL, 9, NULL};  /* ted.s:7852 */
struct Gadget defaultreqga10 = {(struct Gadget *)&defaultreqga11, 280, 28+8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo14, (APTR)&defaultreqbo14, (struct IntuiText *)&defaultreqin17, 0, NULL, 10, NULL};  /* ted.s:7857 */
struct Gadget defaultreqga11 = {(struct Gadget *)&defaultreqga12, 304, 28+8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo15, (APTR)&defaultreqbo15, (struct IntuiText *)&defaultreqin18, 0, NULL, 11, NULL};  /* ted.s:7862 */
struct Gadget defaultreqga12 = {(struct Gadget *)&defaultreqga13, 328, 28+8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo16, (APTR)&defaultreqbo16, (struct IntuiText *)&defaultreqin19, 0, NULL, 12, NULL};  /* ted.s:7867 */
struct Gadget defaultreqga13 = {(struct Gadget *)&defaultreqga18, 352, 28+8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo17, (APTR)&defaultreqbo17, (struct IntuiText *)&defaultreqin20, 0, NULL, 13, NULL};  /* ted.s:7872 */
struct Gadget defaultreqga14 = {(struct Gadget *)&defaultreqga15, 280, 52, 16, 12, 2, 2, 1, (APTR)&defaultreqbo18, (APTR)&defaultreqbo18, (struct IntuiText *)&defaultreqin21, 0, NULL, 14, NULL};  /* ted.s:7877 */
struct Gadget defaultreqga15 = {(struct Gadget *)&defaultreqga16, 304, 52, 16, 12, 2, 2, 1, (APTR)&defaultreqbo19, (APTR)&defaultreqbo19, (struct IntuiText *)&defaultreqin22, 0, NULL, 15, NULL};  /* ted.s:7882 */
struct Gadget defaultreqga16 = {(struct Gadget *)&defaultreqga17, 328, 52, 16, 12, 2, 2, 1, (APTR)&defaultreqbo20, (APTR)&defaultreqbo20, (struct IntuiText *)&defaultreqin23, 0, NULL, 16, NULL};  /* ted.s:7887 */
struct Gadget defaultreqga17 = {(struct Gadget *)&defaultreqga18, 352, 52, 16, 12, 2, 2, 1, (APTR)&defaultreqbo21, (APTR)&defaultreqbo21, (struct IntuiText *)&defaultreqin24, 0, NULL, 17, NULL};  /* ted.s:7892 */
struct Gadget defaultreqga18 = {(struct Gadget *)&defaultreqga19, 280, 76-8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo22, (APTR)&defaultreqbo22, (struct IntuiText *)&defaultreqin25, 0, NULL, 18, NULL};  /* ted.s:7897 */
struct Gadget defaultreqga19 = {(struct Gadget *)&defaultreqga20, 304, 76-8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo23, (APTR)&defaultreqbo23, (struct IntuiText *)&defaultreqin26, 0, NULL, 19, NULL};  /* ted.s:7902 */
struct Gadget defaultreqga20 = {(struct Gadget *)&defaultreqga21, 328, 76-8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo24, (APTR)&defaultreqbo24, (struct IntuiText *)&defaultreqin27, 0, NULL, 20, NULL};  /* ted.s:7907 */
struct Gadget defaultreqga21 = {(struct Gadget *)&defaultreqga22, 352, 76-8, 16, 12, 2, 2, 1, (APTR)&defaultreqbo25, (APTR)&defaultreqbo25, (struct IntuiText *)&defaultreqin28, 0, NULL, 21, NULL};  /* ted.s:7912 */
struct Gadget defaultreqga22 = {(struct Gadget *)&defaultreqga23, 16, 100, 72, 12, 2, 2, 1, (APTR)&defaultreqbo26, (APTR)&defaultreqbo26, (struct IntuiText *)&defaultreqin29, 0, NULL, 22, NULL};  /* ted.s:7917 */
struct Gadget defaultreqga23 = {(struct Gadget *)&defaultreqga24, 112, 100, 72, 12, 2, 2, 1, (APTR)&defaultreqbo27, (APTR)&defaultreqbo27, (struct IntuiText *)&defaultreqin30, 0, NULL, 23, NULL};  /* ted.s:7922 */
struct Gadget defaultreqga24 = {(struct Gadget *)&defaultreqga25, 216, 100, 72, 12, 2, 2, 1, (APTR)&defaultreqbo28, (APTR)&defaultreqbo28, (struct IntuiText *)&defaultreqin31, 0, NULL, 24, NULL};  /* ted.s:7927 */
struct Gadget defaultreqga25 = {(struct Gadget *)&defaultreqga26, 312, 100, 72, 12, 2, 2, 1, (APTR)&defaultreqbo29, (APTR)&defaultreqbo29, (struct IntuiText *)&defaultreqin32, 0, NULL, 25, NULL};  /* ted.s:7932 */
struct Gadget defaultreqga26 = {(struct Gadget *)&defaultreqga27, 8, 156, 40, 12, 0, 5, 1, (APTR)&defaultreqbo30, NULL, (struct IntuiText *)&defaultreqin33, 0, NULL, 26, NULL};  /* ted.s:7937 */
struct Gadget defaultreqga27 = {NULL, 320, 156, 72, 12, 0, 5, 1, (APTR)&defaultreqbo31, NULL, (struct IntuiText *)&defaultreqin34, 0, NULL, 27, NULL};  /* ted.s:7942 */
struct Requester prtreq = {NULL, 60, 46, 360, 68, 0, 0, (struct Gadget *)&prtreqga1, (struct Border *)&prtreqbo1, (struct IntuiText *)&prtreqin1};  /* ted.s:7971 */
struct Border prtreqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)prtreqla1, (struct Border *)&prtreqla2};  /* ted.s:7981 */
WORD prtreqla1[6] = {343, 28, 144, 28, 144, 39};  /* ted.s:7984 */
struct Border prtreqla2 = {0, 0, 1, 2, 1, 3, (WORD *)prtreqla3, (struct Border *)&prtreqbo2};  /* ted.s:7985 */
WORD prtreqla3[6] = {343, 29, 343, 39, 145, 39};  /* ted.s:7988 */
struct Border prtreqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)prtreqla4, (struct Border *)&prtreqla5};  /* ted.s:7989 */
WORD prtreqla4[6] = {351, 24, 8, 24, 8, 43};  /* ted.s:7992 */
struct Border prtreqla5 = {0, 0, 2, 1, 1, 3, (WORD *)prtreqla6, (struct Border *)&prtreqbo3};  /* ted.s:7993 */
WORD prtreqla6[6] = {351, 25, 351, 43, 9, 43};  /* ted.s:7996 */
struct Border prtreqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)prtreqla7, (struct Border *)&prtreqla8};  /* ted.s:7997 */
WORD prtreqla7[6] = {359, 0, 0, 0, 0, 67};  /* ted.s:8000 */
struct Border prtreqla8 = {0, 0, 2, 1, 1, 3, (WORD *)prtreqla9, NULL};  /* ted.s:8001 */
WORD prtreqla9[6] = {359, 1, 359, 67, 1, 67};  /* ted.s:8004 */
struct Border prtreqbo4 = {0, 0, 1, 2, 1, 3, (WORD *)prtreqla13, (struct Border *)&prtreqla14};  /* ted.s:8005 */
WORD prtreqla13[6] = {63, 0, 0, 0, 0, 11};  /* ted.s:8008 */
struct Border prtreqla14 = {0, 0, 2, 1, 1, 3, (WORD *)prtreqla15, NULL};  /* ted.s:8009 */
WORD prtreqla15[6] = {63, 1, 63, 11, 1, 11};  /* ted.s:8012 */
struct Border prtreqbo5 = {0, 0, 1, 2, 1, 3, (WORD *)prtreqla17, (struct Border *)&prtreqla18};  /* ted.s:8013 */
WORD prtreqla17[6] = {71, 0, 0, 0, 0, 11};  /* ted.s:8016 */
struct Border prtreqla18 = {0, 0, 2, 1, 1, 3, (WORD *)prtreqla19, NULL};  /* ted.s:8017 */
WORD prtreqla19[6] = {71, 1, 71, 11, 1, 11};  /* ted.s:8020 */
struct IntuiText prtreqin1 = {1, 0, 1, 20, 30, NULL, (UBYTE *)prtreqla10, (struct IntuiText *)&prtreqin2};  /* ted.s:8022 */
UBYTE prtreqla10[16] = "PRINTER DEVICE:";  /* ted.s:8026 */
struct IntuiText prtreqin2 = {1, 0, 1, 12, 6, NULL, (UBYTE *)prtreqla11, NULL};  /* ted.s:8028 */
UBYTE prtreqla11[11] = "Print Text";  /* ted.s:8032 */
struct IntuiText prtreqin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)prtreqla16, NULL};  /* ted.s:8034 */
UBYTE prtreqla16[8] = " PRINT ";  /* ted.s:8038 */
struct IntuiText prtreqin4 = {1, 0, 1, 4, 2, NULL, (UBYTE *)prtreqla20, NULL};  /* ted.s:8040 */
UBYTE prtreqla20[9] = " CANCEL ";  /* ted.s:8044 */
struct Gadget prtreqga1 = {(struct Gadget *)&prtreqga2, 148, 30, 192, 8, 0, 1, 4, NULL, NULL, NULL, 0, (APTR)&prtreqsg1, 1, NULL};  /* ted.s:8047 */
struct StringInfo prtreqsg1 = {(UBYTE *)prtname, (UBYTE *)undobuffer, 0, 64, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};  /* ted.s:8052 */
char prtname[64] = "PRT:";  /* ted.s:8056 */
struct Gadget prtreqga2 = {(struct Gadget *)&prtreqga3, 8, 52, 64, 12, 0, 5, 1, (APTR)&prtreqbo4, NULL, (struct IntuiText *)&prtreqin3, 0, NULL, 2, NULL};  /* ted.s:8059 */
struct Gadget prtreqga3 = {NULL, 280, 52, 72, 12, 0, 5, 1, (APTR)&prtreqbo5, NULL, (struct IntuiText *)&prtreqin4, 0, NULL, 3, NULL};  /* ted.s:8064 */
/* req2c:end data */

/* ted.s:7948 alertstuff: x = 56 (word), y = 22, text, 0, continuation 0 */
const char alertstuff[] = "\0\x38\x16Not enough memory for operation!"
                          " - Click Mouse Button to Continue.\0";

/* fixed strides of the asm as pointer tables (survey-07 G.1, DIFF) */
struct Gadget *const intgads[3] = {     /* ga4 + n*84 */
    &defaultreqga4, &defaultreqga5, &defaultreqga6
};
struct PropInfo *const hps[3] = {       /* hp1 + n*86 */
    &defaultreqhp1, &defaultreqhp2, &defaultreqhp3
};
struct Gadget *const colgads[16] = {    /* ga10 + n*44 (dispgadd0) */
    &defaultreqga10, &defaultreqga11, &defaultreqga12, &defaultreqga13,
    &defaultreqga14, &defaultreqga15, &defaultreqga16, &defaultreqga17,
    &defaultreqga18, &defaultreqga19, &defaultreqga20, &defaultreqga21,
    &defaultreqga22, &defaultreqga23, &defaultreqga24, &defaultreqga25
};
/* bo14 + n*28 (swapbords): each colour button has two borders, the
   second one is the NextBorder of the first (swapd0bord: bo14 + n*56 and
   that + la63-bo14) */
struct Border *const colbords[32] = {
    &defaultreqbo14, &defaultreqla63,
    &defaultreqbo15, &defaultreqla67,
    &defaultreqbo16, &defaultreqla71,
    &defaultreqbo17, &defaultreqla75,
    &defaultreqbo18, &defaultreqla79,
    &defaultreqbo19, &defaultreqla83,
    &defaultreqbo20, &defaultreqla87,
    &defaultreqbo21, &defaultreqla91,
    &defaultreqbo22, &defaultreqla95,
    &defaultreqbo23, &defaultreqla99,
    &defaultreqbo24, &defaultreqla103,
    &defaultreqbo25, &defaultreqla107,
    &defaultreqbo26, &defaultreqla111,
    &defaultreqbo27, &defaultreqla115,
    &defaultreqbo28, &defaultreqla119,
    &defaultreqbo29, &defaultreqla123
};

/* NEW (hooks.h): realsave's icon switch = SELECTED of defaultreqga9
   (asm: btst #7,defaultreqga9+13, ted.s:805) */
BOOL icongad(void)
{
    return (defaultreqga9.Flags & GFLG_SELECTED) != 0;
}

/* NEW (hooks.h): isithere2's case switch = SELECTED of findreqga8
   (asm: btst #7,findreqga8+13, ted.s:3543/3550) */
BOOL findcase(void)
{
    return (findreqga8.Flags & GFLG_SELECTED) != 0;
}

