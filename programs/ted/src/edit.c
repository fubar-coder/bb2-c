/*
 * edit.c - chunk 02, Amiga part (ted.s 1016-1980): see edit.h.
 *
 * escape, beepem, the vertical prop gadget, the external COMPILER menu
 * (callback into blitz2), the menu dispatch through menutable, new.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "ted.h"

const char deftitname[] = "Blitz Basic 2 Editor V1.0";   /* ted.s:1604 */

/* ted.s:1016 escape */
void escape(void)
{
    struct Menu *a1;
    struct MenuItem *a2, *it;

    if (!erase())
        return;                                 /* asm: addq #4,a7 in erase */
    a1 = menu2.NextMenu;                        /* move.l menu2(pc),d0 */
    if (a1) {
        /* free up external menus.  survey-02 I.7 (no effect): the strip is
           still attached (no ClearMenuStrip), menu2.NextMenu is not
           cleared; a COMPILER menu without items still has FirstItem =
           &menu2item1 (copied by getstuff) - FreeMem on program data. */
        a2 = a1->FirstItem;
        FreeMem(a1, sizeof(struct Menu));       /* menu2name-menu2 = 30 */
        do {                                    /* .loop */
            it = a2;
            a2 = it->NextItem;
            FreeMem(it, sizeof(struct mitem));  /* m2i1text-menu2item1 = 54 */
        } while (a2);
    }
    /* .skip */
    doclean();
    freemice();
    allalt = 0;
    /* DIFF (docs/ted/deviations.md, escape): `lea 12(a7),a7` + `bra freeall`
       returns behind `bsr mainedit`; C: edquit, mainedit returns */
    freeall();
    edquit = TRUE;
}

/* ted.s:1054 beepem */
void beepem(void)
{
    pointeron();
    DisplayBeep(myscreen);                      /* jmp displaybeep(a6) */
}

/* the VertPot of the vertical prop: changed by Intuition while the loop
   polls (asm: move prop1sinfo+4(pc),d0 every round) */
static UWORD vpos(void)
{
    UWORD pot = *(volatile UWORD *)&prop1sinfo.VertPot;

    return (UWORD)(((ULONG)pot * numlines) >> 16);  /* mulu numlines / swap */
}

/* ted.s:1460 gadget - some one hit my vprop gadget! */
void gadget(void)
{
    UWORD d0, d6 = ev_d6;
    LONG d7;

    if (edrows >= numlines)                     /* bcs .skip */
        return;
    /* survey-02 G: bloktopx is the scratch word of the loop (harmless) */
    bloktopx = (WORD)(curstop + cursy);
    cursor();
    do {
        /* .loop */
        d0 = vpos();
        if (d0 != (UWORD)bloktopx) {
            bloktopx = (WORD)d0;
            cursor();
            gotolined02((UWORD)bloktopx);
            cursor();
            showrow();
        }
        /* .skip2: busy poll; the ending message is swallowed
           (survey-02 I.11) */
        d7 = getinput2(&d6);
    } while (d7 == 0);
    cursor();
    gotolined02(vpos());                        /* bra gotolined02 */
}

/* ted.s:1542 extmenu - an external menu item has been found!  do the menu
   sub, and, at return, jump to line d0, unless d0 = -1; d0 = -2 for
   showall */
void extmenu(UWORD d6)
{
    WORD d0;

    if (!menurout)
        return;                                 /* .skip */
    /* lsr #4,d6; a5 = firstitem, d7 = numlines (glue.c).  blitz2 may call
       extalloc -> outmem: longjmp over its frames (BUG(orig) #369). */
    d0 = call_menurout((UWORD)(d6 >> 4));
    showme();
    /* move.l thisitem,a4: gotolined0 starts from thisitem */
    if (d0 == -2) {
        showall();
        return;
    }
    if (d0 != -1)
        gotolined0((UWORD)d0);                  /* I.13: no range check (a text line) */
}

/* ted.s:1565 menu - menu number in d6 */
void menu(void)
{
    UWORD d6 = ev_d6, d0;
    const struct jtentry *a0;
    UBYTE flag;

    d0 = d6 & 15;                               /* move.b d6,d0 / and #15,d0 */
    d6 = (UWORD)((d6 >> 1) & 0xf0);
    if ((UBYTE)d0 == 4) {
        extmenu(d6);                            /* beq extmenu */
        return;
    }
    d6 |= d0;
    /* convert menu number to byte (hi nyb = item, lo = menu); survey-02
       I.14: menus/items >= 16 alias */
    for (a0 = menutable; a0->code != 255; a0++) {
        if (a0->code == (UBYTE)d6) {
            /* .found: the flag byte on the stack */
            flag = a0->flag;
            a0->routine();
            if (flag)
                showvprop();                    /* bne showvprop */
            return;
        }
    }
    /* .done */
}

/* ted.s:1593 new */
void new(void)
{
    const char *a0 = deftitname;
    char *a1 = newsctit;

    if (!erase())
        return;
    doclean();
    freeall();
    while ((*a1++ = *a0++) != 0)                /* .loop (survey-02 I.15) */
        ;
    showme();
    donew();                                    /* bra donew */
}
