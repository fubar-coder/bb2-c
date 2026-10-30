/*
 * lineedit.c - chunk 06 core (ted.s 5253-6285): see lineedit.h.
 *
 * Block edge rules (inbloks/inblokf/blokfix/blokfixb), the horizontal
 * cursor logic (horizpos/cursleft/cursrite) and the chmap parts of the
 * line editor getline (gl_*).  The getline loop itself, the bitmap shifts
 * and the printing are in src/input.c.
 *
 * Word arithmetic as in the asm: "abs. column" = cursx + cursoff as a
 * 16 bit value (cursoff may be negative, survey-06 I.5); bcs/bcc/bhi/bls
 * compare unsigned (UWORD casts), blt/bgt/bmi/bpl signed (WORD casts).
 * a2 = chmap + chloc is the cursor row of the character map.
 */
#include <stdlib.h>
#include "tedcore.h"
#include "hooks.h"
#include "state.h"
#include "fileio.h"
#include "lines.h"
#include "loadsave.h"
#include "services.h"
#include "tokit.h"
#include "lineedit.h"
#include "keymap.h"

/* the cursor row of chmap (asm: move.l chmap,a2 / add chloc,a2) */
static UBYTE *chrow(void)
{
    return chmap + (WORD)chloc;                 /* add chloc(pc),a2: word, sign-extended */
}

/* abs. column: move cursx,d0 / add cursoff,d0 (16 bit) */
static UWORD abscol(void)
{
    return (UWORD)(cursx + cursoff);
}

/* line number: move curstop,d2 / add cursy,d2 (16 bit) */
static UWORD lineno(void)
{
    return (UWORD)(curstop + cursy);
}

/* moveq #0,d1 / move d0,d1 / divu tabstop,d1 / swap d1: remainder of the
   32/16 division (the quotient always fits: d0 < 32768).  BUG(orig) (#385)
   (open point O3): tabstop = 0 from l:blitzeditor.opts is a division by
   zero - divu_w is the real DIVU.W on the Amiga (zero divide trap like the
   original); the host aborts. */
static UWORD tabrem(UWORD d0)
{
    return (UWORD)(divu_w(d0, tabstop) >> 16);
}

/* ted.s:5360 inbloks */
BOOL inbloks(void)
{
    /* return eq if cursor is <= start of first line of blok */
    if (lineno() != btopy)
        return FALSE;
    if (abscol() > (UWORD)btopx)        /* bhi */
        return FALSE;
    return TRUE;                        /* moveq #0,d3 -> Z=1 */
}

/* ted.s:5372 inblokf */
BOOL inblokf(void)
{
    /* return eq if cursor is =< last char of blok */
    if (lineno() != bboty)
        return FALSE;
    if (abscol() > (UWORD)bbotx)        /* bhi */
        return FALSE;
    return TRUE;
}

/* ted.s:5384 blokfix */
void blokfix(void)
{
    if ((UWORD)btopx < chcols) {        /* bcs blokfixb */
        blokfixb();
        return;
    }
    btopx = 0;
    btopy++;
    if (btopy <= bboty) {               /* bls blokfixb */
        blokfixb();
        return;
    }
    blokdis();                          /* .skip: bra blokdis */
}

/* ted.s:5393 blokfixb */
void blokfixb(void)
{
    UWORD d2;

    if (bbotx < 0) {                    /* bpl .ok2 not taken */
        bbotx = (WORD)(chcols - 1);
        bboty--;
        if (bboty >= btopy)             /* bcc .ok3 */
            return;
        blokdis();                      /* .zap */
        return;
    }
    /* .ok2 */
    d2 = btopy;
    if (btopy == bboty) {
        d2 = (UWORD)bbotx;
        if (bbotx < btopx) {            /* blt .zap */
            blokdis();
            return;
        }
    }
    /* .ok4  BUG(orig) (#349): for a multi-line block d2 is still btopy (not
       bbotx): bbotx >= chcols stays uncorrected, and a block starting at
       line >= chcols gets bbotx = chcols-1 (survey-06 I.1) */
    if (d2 < chcols)                    /* bcs .ok3 */
        return;
    bbotx = (WORD)(chcols - 1);
    /* .ok3 */
}

/* ted.s:5742 horizpos */
void horizpos(WORD d1)
{
    /* position cursor at d1 */
    WORD d4 = cursoff;
    UWORD d2 = (UWORD)(edcols - hscmarg);
    WORD d0, d3;

    if ((UWORD)d1 < d2) {               /* bcc .skip3 not taken */
        cursoff = 0;
        cursx = (UWORD)d1;
    } else {
        /* .skip3 */
        d2 = (UWORD)(edcols >> 1);      /* lsr #1 */
        d0 = (WORD)(d1 - d2);
        d3 = (WORD)(chcols - edcols);
        if (d0 > d3) {                  /* bgt .skip4 */
            /* .skip4: may give an odd cursoff (survey-06 I.5) */
            cursoff = d3;
            cursx = (UWORD)(d1 - d3);
        } else {
            if (d0 & 1) {
                d0++;
                d2--;
            }
            /* .skip5: d0 < 0 possible if hscmarg > edcols/2 (I.5) */
            cursoff = d0;
            cursx = d2;
        }
    }
    /* .moverite */
    if (cursoff == d4)
        return;                         /* .done */
    refresh();                          /* bra refresh */
}

/* ted.s:5775 cursleft (asm: d0 = cursx at every call) */
void cursleft(void)
{
    if (cursx >= hscmarg) {             /* bcs .skip not taken */
        cursx--;                        /* .skip3 */
        return;
    }
    /* .skip */
    if (cursoff == 0) {
        if (cursx != 0)
            cursx--;                    /* bne .skip3 */
        return;
    }
    /* .skip2 */
    cursoff -= 2;
    scrollrite();
    cursx++;
}

/* ted.s:5789 cursrite (asm: d0 = cursx at every call) */
void cursrite(void)
{
    UWORD d2 = edcols;

    if ((UWORD)(cursoff + d2) >= chcols) {      /* bcs .skip not taken */
        d2--;
        if (cursx < d2)
            cursx++;                    /* bcs .skip3 */
        return;
    }
    /* .skip */
    if (cursx >= hscmarg) {             /* bcs .skip3 not taken */
        d2 = (UWORD)(d2 - hscmarg);
        if (cursx >= d2) {              /* bcc .skip2 */
            cursoff += 2;
            scrollleft();
            cursx--;
            return;
        }
    }
    cursx++;                            /* .skip3 */
}

/* ------------------------------------------------- getline data parts */

/* ted.s:5444 (getline3 .isasc, core part) */
void gl_isasc(UBYTE c)
{
    UBYTE *a2 = chrow();
    UWORD d0, d1;

    anyalt = c;                         /* move d7,anyalt */
    /* adjust block */
    if (bboty != (UWORD)-1) {
        if (inbloks())
            btopx++;
        if (inblokf())
            bbotx++;                    /* .skipx */
        blokfix();                      /* .skipy */
    }
    /* .skip0: (bitmap shift: caller)  .skip: chmap row one to the right
       from the abs. column, the byte at chcols-1 is lost (survey-06 I.2) */
    d0 = abscol();
    d1 = chcols;
    for (;;) {
        d1--;                           /* .loop3 */
        if (d0 >= d1)                   /* bcc .skip2 */
            break;
        a2[(WORD)d1] = a2[(WORD)d1 - 1];
    }
    /* .skip2: print d7 (caller; print2 writes chmap[abs. column]) */
}

/* ted.s:5665 (char127, core part) */
void gl_char127(UBYTE d7)
{
    UBYTE *a2 = chrow();
    UWORD d0, d2;

    anyalt = d7;                        /* move d7,anyalt */
    /* adjust block */
    if (bboty != (UWORD)-1) {
        if (inbloks() && abscol() != (UWORD)btopx)
            btopx--;                    /* .skipz */
        if (inblokf()) {
            d2 = (UWORD)(bbotx + 1);
            if (d2 < chcols)            /* bcc .skipmo */
                bbotx--;
            blokfixb();                 /* .skipmo; harmless: only
                                           after inblokf (survey-06 I.14) */
        }
    }
    /* .loop: (bitmap shift: caller)  .skip: chmap row one to the left */
    d0 = abscol();
    for (;;) {
        d0++;                           /* .loop3 */
        if (d0 >= chcols)               /* bcc .skip2 */
            break;
        a2[(WORD)d0 - 1] = a2[(WORD)d0];
    }
    a2[(WORD)d0 - 1] = ' ';             /* .skip2 */
    /* last visible cell: print3 by the caller */
}

/* ted.s:5555 (charnot8, core part) without Shift (".loop", "insert tab") */
WORD gl_tab(void)
{
    UBYTE *a2 = chrow();
    UWORD d0, d2, d1, target;
    WORD d3;

    d0 = d2 = abscol();                 /* add cursoff,d0 / move d0,d2 */
    do {
        d0++;                           /* .loop */
        if (d0 >= chcols)
            return -1;                  /* bcc getline2 */
    } while (tabrem(d0) != 0);
    anyalt = 9;                         /* move d7,anyalt */
    target = d0;                        /* move d0,-(a7) */
    d0 = (UWORD)(d0 - d2);              /* d0=number of chars to insert */
    /* adjust for block */
    if (bboty != (UWORD)-1) {
        if (inbloks())
            btopx += (WORD)d0;
        if (inblokf())
            bbotx += (WORD)d0;          /* .zap2 */
        blokfix();                      /* .zap3 */
    }
    /* .zap: row from the column d0 to the right (overflow is lost) */
    d1 = chcols;
    for (;;) {
        d1--;                           /* .loop2 */
        d3 = (WORD)(d1 - d0);
        if (d3 < 0)                     /* bmi .done */
            break;
        if ((UWORD)d3 < d2)             /* bcs .done */
            break;
        a2[(WORD)d1] = a2[d3];
    }
    for (;;) {
        a2[(WORD)d1] = ' ';             /* .done */
        d1--;
        if ((WORD)d1 < 0)               /* bmi reline */
            break;
        if (d1 < d2)                    /* bcc .done */
            break;
    }
    return (WORD)target;                /* falls into reline */
}

/* ted.s:5555 (charnot8, core part) with Shift ("delete tab") */
WORD gl_untab(void)
{
    UBYTE *a2 = chrow();
    WORD d0, d2, d4;

    d0 = d2 = (WORD)abscol();
    anyalt = 9;                         /* move d7,anyalt (also when nothing
                                           happens below) */
    do {
        d0--;                           /* .loop4 */
        if (d0 < 0)
            return -1;                  /* bmi getline2 */
    } while (tabrem((UWORD)d0) != 0);
    /* adjust block (no blokfix, survey-06 I.3) */
    if (bboty != (UWORD)-1) {
        d4 = (WORD)(d2 - d0);
        if (inbloks())
            btopx -= d4;
        if (inblokf())
            bbotx -= d4;                /* .zip1 */
    }
    /* .zip: move d0,-(a7) = the target column; any characters between
       the target and the cursor are deleted (survey-06 I.3) */
    {
        WORD target = d0;

        do {
            a2[d0] = a2[d2];            /* .loop5 */
            d2++;
            d0++;
        } while ((UWORD)d2 < chcols);
        do {
            a2[d0] = ' ';               /* .loop6 */
            d0++;
        } while ((UWORD)d0 < chcols);
        return target;                  /* bra reline */
    }
}

/* ted.s:5726 (charnotgad, core part): $f8e1 = menu 1 item 7 "delete to
   end of line" */
WORD gl_delright(void)
{
    UBYTE *a2 = chrow();
    UWORD d0;
    WORD col;

    anyalt = 0xf8e1;                    /* move d6,anyalt */
    d0 = abscol();
    col = (WORD)d0;                     /* move d0,-(a7) */
    do {
        a2[(WORD)d0] = ' ';             /* .loop; no block adjust (I.4) */
        d0++;
    } while (d0 < chcols);
    return col;                         /* bra reline */
}

/* ted.s:5498 (charlow .tababove, core part): Shift + cursor left at column 0 */
WORD gl_tababove(void)
{
    /* tab to line aboves tab pos. */
    const UBYTE *a0;
    WORD d1;

    if (lineno() == 0)
        return -1;                      /* beq getline2 */
    a0 = thisitem->lastitem->chars;
    d1 = -1;
    for (;;) {
        if (*a0 == 0)
            return -1;                  /* .none -> getline2 */
        d1++;
        if (*a0++ != ' ')
            return d1;                  /* bra curspos */
    }
}

/* ted.s:5521 (charnot31, core part) with Shift (Shift + cursor right) */
WORD gl_lastcol(void)
{
    const UBYTE *a2 = chrow();
    WORD d1 = (WORD)chcols;

    for (;;) {
        d1--;                           /* .loop */
        if (d1 < 0)
            return 0;                   /* bpl .skip not taken */
        if (a2[d1] != ' ')              /* .skip; token colour spaces ($A0)
                                           count as characters */
            break;
    }
    d1++;
    if ((UWORD)d1 < chcols)             /* bcs curspos */
        return d1;
    return (WORD)(d1 - 1);
}
