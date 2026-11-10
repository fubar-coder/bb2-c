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
    struct Menu *compmenu;
    struct MenuItem *next, *item;

    if (!erase())
        return;                                 /* asm: addq #4,a7 in erase */
    compmenu = menu2.NextMenu;                  /* move.l menu2(pc),d0 */
    if (compmenu) {
        /* free up external menus.  survey-02 I.7 (no effect): the strip is
           still attached (no ClearMenuStrip), menu2.NextMenu is not
           cleared; a COMPILER menu without items still has FirstItem =
           &menu2item1 (copied by getstuff) - FreeMem on program data. */
        next = compmenu->FirstItem;
        FreeMem(compmenu, sizeof(struct Menu)); /* menu2name-menu2 = 30 */
        do {                                    /* .loop */
            item = next;
            next = item->NextItem;
            FreeMem(item, sizeof(struct mitem)); /* m2i1text-menu2item1 = 54 */
        } while (next);
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
void gadget(const struct tedevent *ev)
{
    UWORD pos, data = ev->data;
    LONG event;

    if (edrows >= numlines)                     /* bcs .skip */
        return;
    /* survey-02 G: bloktopx is the scratch word of the loop (harmless) */
    bloktopx = (WORD)(curstop + cursy);
    cursor();
    do {
        /* .loop */
        pos = vpos();
        if (pos != (UWORD)bloktopx) {
            bloktopx = (WORD)pos;
            cursor();
            gotolined02((UWORD)bloktopx);
            cursor();
            showrow();
        }
        /* .skip2: busy poll; the ending message is swallowed
           (survey-02 I.11) */
        event = getinput2(&data);
    } while (event == 0);
    cursor();
    gotolined02(vpos());                        /* bra gotolined02 */
}

/* ted.s:1542 extmenu - an external menu item has been found!  do the menu
   sub, and, at return, jump to line d0, unless d0 = -1; d0 = -2 for
   showall */
void extmenu(UWORD code)
{
    WORD line;

    if (!menurout)
        return;                                 /* .skip */
    /* lsr #4,d6; a5 = firstitem, d7 = numlines (glue.c).  blitz2 may call
       extalloc -> outmem: longjmp over its frames (BUG(orig) #369). */
    line = call_menurout((UWORD)(code >> 4));
    showme();
    /* move.l thisitem,a4: gotolined0 starts from thisitem */
    if (line == -2) {
        showall();
        return;
    }
    if (line != -1)
        gotolined0((UWORD)line);                /* I.13: no range check (a text line) */
}

/* ted.s:1565 menu - menu number in d6 */
void menu(const struct tedevent *ev)
{
    UWORD raw = ev->data;                       /* Intuition menu code */
    UWORD menunum = raw & 15;                   /* move.b d6,d0 / and #15,d0 */
    UWORD code = (UWORD)((raw >> 1) & 0xf0);
    const struct jtentry *entry;
    UBYTE flag;

    if ((UBYTE)menunum == 4) {
        extmenu(code);                          /* beq extmenu */
        return;
    }
    code |= menunum;
    /* convert menu number to byte (hi nyb = item, lo = menu); survey-02
       I.14: menus/items >= 16 alias */
    for (entry = menutable; entry->code != 255; entry++) {
        if (entry->code == (UBYTE)code) {
            /* .found: the flag byte on the stack */
            flag = entry->flag;
            if (entry->evroutine)
                entry->evroutine(ev);           /* d7/d6 of mainedit */
            else
                entry->routine();
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
    const char *src = deftitname;
    char *dst = newsctit;
    struct item *item;

    if (!erase())
        return;
    doclean();
    /* Fixed (#356): donew's empty line is allocated before the old text is
       freed; without memory the old text stays and outmem reports it (the
       original freed it and went on with the freed lines) */
    item = allocnull();
    if (!item)
        outmem();
    freeall();
    while ((*dst++ = *src++) != 0)              /* .loop (survey-02 I.15) */
        ;
    showme();
    donew_with(item);                           /* bra donew */
}
