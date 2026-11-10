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
static UWORD tabrem(UWORD col)
{
    return (UWORD)(divu_w(col, tabstop) >> 16);
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
    UWORD endcol;

    if (bbotx < 0) {                    /* bpl .ok2 not taken */
        bbotx = (WORD)(chcols - 1);
        bboty--;
        if (bboty >= btopy)             /* bcc .ok3 */
            return;
        blokdis();                      /* .zap */
        return;
    }
    /* .ok2 */
    if (btopy == bboty && bbotx < btopx) {      /* blt .zap */
        blokdis();
        return;
    }
    /* .ok4  Fixed (#349): the original tests d2 = btopy (not bbotx) for a
       multi-line block: bbotx >= chcols stayed uncorrected, and a block
       starting at line >= chcols got bbotx = chcols-1 (survey-06 I.1) */
    endcol = (UWORD)bbotx;
    if (endcol < chcols)                /* bcs .ok3 */
        return;
    bbotx = (WORD)(chcols - 1);
    /* .ok3 */
}

/* ted.s:5742 horizpos */
void horizpos(WORD col)
{
    /* position cursor at col (d1) */
    WORD oldoff = cursoff;              /* d4 */

    if ((UWORD)col < (UWORD)(edcols - hscmarg)) {   /* bcc .skip3 not taken */
        cursoff = 0;
        cursx = (UWORD)col;
    } else {
        /* .skip3 */
        UWORD x = (UWORD)(edcols >> 1); /* d2: lsr #1, the new cursx */
        WORD newoff = (WORD)(col - x);  /* d0 */
        WORD maxoff = (WORD)(chcols - edcols);      /* d3 */

        if (newoff > maxoff) {          /* bgt .skip4 */
            /* .skip4: may give an odd cursoff (survey-06 I.5) */
            cursoff = maxoff;
            cursx = (UWORD)(col - maxoff);
        } else {
            if (newoff & 1) {
                newoff++;
                x--;
            }
            /* .skip5: newoff < 0 possible if hscmarg > edcols/2 (I.5) */
            cursoff = newoff;
            cursx = x;
        }
    }
    /* .moverite */
    if (cursoff == oldoff)
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
    if ((UWORD)(cursoff + edcols) >= chcols) {  /* bcs .skip not taken */
        if (cursx < (UWORD)(edcols - 1))        /* d2 = edcols - 1 */
            cursx++;                    /* bcs .skip3 */
        return;
    }
    /* .skip */
    if (cursx >= hscmarg) {             /* bcs .skip3 not taken */
        if (cursx >= (UWORD)(edcols - hscmarg)) {   /* bcc .skip2 */
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
    UBYTE *row = chrow();
    UWORD col, i;

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
    col = abscol();
    i = chcols;
    for (;;) {
        i--;                            /* .loop3 */
        if (col >= i)                   /* bcc .skip2 */
            break;
        row[(WORD)i] = row[(WORD)i - 1];
    }
    /* .skip2: print d7 (caller; print2 writes chmap[abs. column]) */
}

/* ted.s:5665 (char127, core part) */
void gl_char127(UBYTE c)
{
    UBYTE *row = chrow();
    UWORD i, next;

    anyalt = c;                         /* move d7,anyalt */
    /* adjust block */
    if (bboty != (UWORD)-1) {
        if (inbloks() && abscol() != (UWORD)btopx)
            btopx--;                    /* .skipz */
        if (inblokf()) {
            next = (UWORD)(bbotx + 1);
            if (next < chcols)          /* bcc .skipmo */
                bbotx--;
            blokfixb();                 /* .skipmo; harmless: only
                                           after inblokf (survey-06 I.14) */
        }
    }
    /* .loop: (bitmap shift: caller)  .skip: chmap row one to the left */
    i = abscol();
    for (;;) {
        i++;                            /* .loop3 */
        if (i >= chcols)                /* bcc .skip2 */
            break;
        row[(WORD)i - 1] = row[(WORD)i];
    }
    row[(WORD)i - 1] = ' ';             /* .skip2 */
    /* last visible cell: print3 by the caller */
}

/* ted.s:5555 (charnot8, core part) without Shift (".loop", "insert tab") */
WORD gl_tab(void)
{
    UBYTE *row = chrow();
    UWORD col, dst, target, n;
    WORD src;

    target = col = abscol();            /* add cursoff,d0 / move d0,d2 */
    do {
        target++;                       /* .loop (d0) */
        if (target >= chcols)
            return -1;                  /* bcc getline2 */
    } while (tabrem(target) != 0);
    anyalt = 9;                         /* move d7,anyalt */
    /* move d0,-(a7): the target column */
    n = (UWORD)(target - col);          /* d0=number of chars to insert */
    /* adjust for block */
    if (bboty != (UWORD)-1) {
        if (inbloks())
            btopx += (WORD)n;
        if (inblokf())
            bbotx += (WORD)n;           /* .zap2 */
        blokfix();                      /* .zap3 */
    }
    /* .zap: row n columns to the right (overflow is lost) */
    dst = chcols;
    for (;;) {
        dst--;                          /* .loop2 */
        src = (WORD)(dst - n);
        if (src < 0)                    /* bmi .done */
            break;
        if ((UWORD)src < col)           /* bcs .done */
            break;
        row[(WORD)dst] = row[src];
    }
    for (;;) {
        row[(WORD)dst] = ' ';           /* .done */
        dst--;
        if ((WORD)dst < 0)              /* bmi reline */
            break;
        if (dst < col)                  /* bcc .done */
            break;
    }
    return (WORD)target;                /* falls into reline */
}

/* ted.s:5555 (charnot8, core part) with Shift ("delete tab") */
WORD gl_untab(void)
{
    UBYTE *row = chrow();
    WORD dst, src, n;

    dst = src = (WORD)abscol();
    anyalt = 9;                         /* move d7,anyalt (also when nothing
                                           happens below) */
    do {
        dst--;                          /* .loop4 */
        if (dst < 0)
            return -1;                  /* bmi getline2 */
    } while (tabrem((UWORD)dst) != 0);
    /* adjust block (no blokfix, survey-06 I.3) */
    if (bboty != (UWORD)-1) {
        n = (WORD)(src - dst);
        if (inbloks())
            btopx -= n;
        if (inblokf())
            bbotx -= n;                 /* .zip1 */
    }
    /* .zip: move d0,-(a7) = the target column; any characters between
       the target and the cursor are deleted (survey-06 I.3) */
    {
        WORD target = dst;

        do {
            row[dst] = row[src];        /* .loop5 */
            src++;
            dst++;
        } while ((UWORD)src < chcols);
        do {
            row[dst] = ' ';             /* .loop6 */
            dst++;
        } while ((UWORD)dst < chcols);
        return target;                  /* bra reline */
    }
}

/* ted.s:5726 (charnotgad, core part): $f8e1 = menu 1 item 7 "delete to
   end of line" */
WORD gl_delright(void)
{
    UBYTE *row = chrow();
    UWORD i;
    WORD col;

    anyalt = 0xf8e1;                    /* move d6,anyalt */
    i = abscol();
    col = (WORD)i;                      /* move d0,-(a7) */
    do {
        row[(WORD)i] = ' ';             /* .loop; no block adjust (I.4) */
        i++;
    } while (i < chcols);
    return col;                         /* bra reline */
}

/* ted.s:5498 (charlow .tababove, core part): Shift + cursor left at column 0 */
WORD gl_tababove(void)
{
    /* tab to line aboves tab pos. */
    const UBYTE *p;
    WORD col;

    if (lineno() == 0)
        return -1;                      /* beq getline2 */
    p = thisitem->lastitem->chars;
    col = -1;
    for (;;) {
        if (*p == 0)
            return -1;                  /* .none -> getline2 */
        col++;
        if (*p++ != ' ')
            return col;                 /* bra curspos */
    }
}

/* ted.s:5521 (charnot31, core part) with Shift (Shift + cursor right) */
WORD gl_lastcol(void)
{
    const UBYTE *row = chrow();
    WORD col = (WORD)chcols;

    for (;;) {
        col--;                          /* .loop */
        if (col < 0)
            return 0;                   /* bpl .skip not taken */
        if (row[col] != ' ')            /* .skip; token colour spaces ($A0)
                                           count as characters */
            break;
    }
    col++;
    if ((UWORD)col < chcols)            /* bcs curspos */
        return col;
    return (WORD)(col - 1);
}
