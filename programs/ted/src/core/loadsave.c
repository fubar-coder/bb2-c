/*
 * loadsave.c - chunk 01 core (ted.s 1-1015): block tab/untab, printer
 * line format, .xtra/.info names, the load loop fileopen2, the write loop
 * realsave, search and replace.  See loadsave.h.  UI part: src/main.c.
 */
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

UBYTE *xtraat;                  /* ted.s:636 */
UBYTE *xtranull;                /* ted.s:637 */
const char xtratxt[] = ".xtra"; /* ted.s:638 */
const char infotxt[] = ".info"; /* ted.s:639 */
const char bakname[] = ".bak";  /* ted.s:742 */

/* ------------------------------------------------- block tab / untab */

/* ted.s:327 blocktab */
void blocktab(void)
{
    struct item *a4;
    UBYTE *a0, *a1;
    UWORD d0, d6;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    bloksetup2();
    /* a5 = btopy, d6 = btop line; a4 = thisy, d7 = this line */
    a4 = bc.a5;
    d6 = bc.d6;
    do {
        /* @335 .loop2 */
        a1 = charbuff2;
        d0 = tabstop;
        while (d0) {            /* subq #1 / dbf: tabstop spaces */
            *a1++ = 32;
            d0--;
        }
        a0 = a4->chars;
        if (tokenise) {
            a1 = detok(a0, a1);         /* a1 ON the 0 byte */
        } else {
            /* BUG(orig): the copy leaves a1 BEHIND the 0 byte, so the 0
               stays in the chmap style line and charstobuff keeps it as
               the last "non space": every ASCII line of the block gets an
               extra 0 byte (numchars + 1, written as "text\0\n"). */
            while ((*a1++ = *a0++) != 0)
                ;
        }
        /* @348 .loop3 */
        if (a1 > charbuff3)
            ORIG_HAZARD("blocktab: tabstop + line longer than charbuff2 "
                        "(survey-01 I.10)");
        while (a1 < charbuff3)
            *a1++ = 32;
        thisitem = a4;
        a4 = makefirst(a4);
        a4 = a4->nextitem;
        d6++;
    } while (d6 <= bboty);      /* bls: unsigned */
    calcitem();
    showall();
}

/* ted.s:361 blockuntab */
void blockuntab(void)
{
    struct item *a4;
    UBYTE *a0, *a1;
    UWORD d0, d6;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    bloksetup2();
    a4 = bc.a5;
    d6 = bc.d6;
    do {
        /* @369 .loop2 */
        a1 = charbuff2;
        a0 = a4->chars;
        if (tokenise) {
            detok(a0, a1);
        } else {
            while ((*a1++ = *a0++) != 0)
                ;
        }
        /* @377 .skip3 */
        a1 = charbuff2;
        a0 = a1;
        d0 = tabstop;
        /* BUG(orig): removes the first tabstop characters WHATEVER they
           are; tabstop = 0 or a line shorter than tabstop leaves a1 at
           the start: the whole line becomes spaces (survey-01 I.11). */
        if (d0 == 0)
            goto loop3;
        do {
            if (*a0++ == 0)     /* tst.b (a0)+ / beq .loop3 */
                goto loop3;
        } while (--d0);         /* dbf d0 (tabstop iterations) */
        while ((*a1++ = *a0++) != 0)    /* .loop5 */
            ;
        a1--;
loop3:  /* @388 .loop3 */
        while (a1 < charbuff3)
            *a1++ = 32;
        thisitem = a4;
        a4 = makefirst(a4);
        a4 = a4->nextitem;
        d6++;
    } while (d6 <= bboty);
    calcitem();
    showall();
}

/* ------------------------------------------------------- printing */

/* ted.s:570 (prtfile, core part - NEW name): the .ploop body of prtfile
   (ted.s lines 595-624): copy line a4 to buf; a byte with bit 7 starts a
   token -> ESC[1m name ESC[22m; the 0 byte -> LF.  Returns the Write
   length.  BUG(orig): bytes >= $80 are always tokens (also in ASCII mode
   and standalone, where tokens == 0 makes findtoke read at 0; survey-01
   I.14); no limit for buf (charbuff: chcols+2 bytes, survey-01 I.10). */
UWORD prtfile_line(struct item *a4, UBYTE *buf)
{
    UBYTE *a0 = buf;
    const UBYTE *a1 = a4->chars;
    const UBYTE *a3;
    UWORD d0;
    UBYTE c;

    for (;;) {
        /* @598 .dloop */
        c = *a1++;
        *a0++ = c;
        if (c == 0)
            break;
        if (!(c & 0x80))
            continue;
        d0 = (UWORD)(*--a0 << 8);
        d0 |= *a1++;
        /* bold on */
        *a0++ = 27;
        *a0++ = '[';
        *a0++ = '1';
        *a0++ = 'm';
        if (!tokens)
            ORIG_HAZARD("prtfile: token without token list (survey-01 I.14)");
        a3 = (const UBYTE *)findtoke(d0);
        while ((*a0++ = *a3++) != 0)
            ;
        /* bold off (over the 0 byte of the name) */
        a0[-1] = 27;
        *a0++ = '[';
        *a0++ = '2';
        *a0++ = '2';
        *a0++ = 'm';
    }
    /* @622 .ddone */
    a0[-1] = 10;
    if (buf == charbuff && a0 > charbuff2)
        ORIG_HAZARD("prtfile: printer line longer than charbuff (survey-01 I.10)");
    return (UWORD)(a0 - buf);
}

/* ----------------------------------------------------- .xtra / .info */

/* ted.s:641 openinfo */
tedfh openinfo(void)
{
    tedfh lock;
    UBYTE *d1 = openmore(infotxt);

    lock = ted_lock((const char *)d1, TED_ACCESS_READ);
    if (lock) {
        /* the icon exists: never overwritten */
        ted_unlock(lock);
        return 0;
    }
    /* @650 .skip2 */
    return ted_open((const char *)xtraat, TED_MODE_NEWFILE);
}

/* ted.s:658 openxtra */
tedfh openxtra(LONG d2)
{
    UBYTE *d1 = openmore(xtratxt);

    return ted_open((const char *)d1, d2);
}

/* ted.s:664 openmore */
UBYTE *openmore(const char *a1)
{
    UBYTE *a0 = xtraat;

    while (*a0++ != 0)
        ;
    a0--;
    xtranull = a0;
    while ((*a0++ = (UBYTE)*a1++) != 0)
        ;
    return xtraat;
}

/* ted.s:674 closextra */
void closextra(void)
{
    *xtranull = 0;
}

/* ------------------------------------------------------- load / save */

/* ted.s:680 fileopen2 */
void fileopen2(UBYTE *d7)
{
    struct item *a0, *a4;
    UWORD d5 = 0;
    tedfh fh;

    xtraat = d7;
    if (!doopen(d7, TED_MODE_OLDFILE))
        return;                 /* asm: addq #4,a7 + bra error */
    allalt = 0;
    doclean();
    if (fillbuff() == 0) {
        /* BUG(orig): empty file (or read error): donew without freeall -
           the old lines are lost (not freed) */
        doclose();
        donew();
        return;
    }
    /* @689 .ok */
    sleepon();
    blokdis();
    freeall();
    numlines = 1;
    readline(&d5);              /* result ignored: fillbuff had bytes */
    a0 = allocline(d5);
    firstitem = a0;
    thisitem = a0;
    a0->lastitem = NULL;
    enditem = a0;
    for (;;) {
        /* @699 .loop */
        a4 = a0;
        if (readline(&d5) == RL_EOF)
            break;
        a0 = allocline(d5);
        enditem = a0;
        a4->nextitem = a0;
        numlines++;
        a0->lastitem = a4;
    }
    /* @708 .done */
    doclose();
    if (loadxtra) {
        fh = openxtra(TED_MODE_OLDFILE);
        if (fh) {
            /* asm: jsr (a5) with d7 = handle, a6 = DOSBase; then
               Close(d7) with the a6 the callback left */
            call_loadxtra(fh);
            ted_close(fh);
        }
        /* @720 .noxtra2 */
        closextra();
    }
    /* @721 .noxtra */
    scloc = 0;
    chloc = 0;
    cursx = 0;                  /* clr.l cursx = cursx + cursy */
    cursy = 0;
    cursoff = 0;                /* clr.l cursoff = cursoff + curstop */
    curstop = 0;
    cursy = 0;
    showhprop();
    showall();
    pointeron();
    showfname();                /* falls through into showfname */
}

/* ted.s:769 realsave */
void realsave(UBYTE *d7)
{
    struct item *a2;
    UBYTE *a3;
    UWORD d4, d5;
    LONG d0;
    tedfh fh;

    if (!doopen(d7, TED_MODE_NEWFILE))
        return;                 /* the file is then only there as .bak
                                   (survey-01 I.15) */
    sleepon();
    a2 = firstitem;
    d5 = numlines - 1;          /* dbf: numlines 0 = 65536 rounds */
    for (;;) {
        /* @775 .loop */
        d4 = a2->numchars;
        a3 = a2->chars;
        if (!tokenise)
            a3[d4 - 1] = 10;    /* ASCII: the 0 byte becomes LF */
        /* @781 .nocr */
        d0 = ted_write(rd.fh, a3, d4);
        a3[d4 - 1] = 0;
        if (d0 < 0) {
            /* BUG(orig): allalt stays set, but .xtra/.info are still
               written (survey-01 I.15) */
            error(error3);
            goto skip3;
        }
        /* @791 .skip2 */
        a2 = a2->nextitem;
        if (d5-- == 0)
            break;
    }
    allalt = 0;
skip3:  /* @794 .skip3 */
    doclose();
    if (savextra) {
        fh = openxtra(TED_MODE_NEWFILE);
        if (fh) {
            call_savextra(fh);
            ted_close(fh);
        }
        /* @804 .skipex */
        closextra();
    }
    /* @805 .skip - the icon switch is the GADGET (btst #7,
       defaultreqga9+13), not makeicons (survey-01 I.15) */
    if (icongad()) {
        /* OK, we're gonna save a .info file with the source. */
        fh = openinfo();
        if (fh) {
            ted_write(fh, srcinfo, sizeof(srcinfo));
            ted_close(fh);
        }
        /* @818 .noicons2 */
        closextra();
    }
    /* @820 .noicons */
    pointeron();
    showfname();                /* bra showfname */
}

/* --------------------------------------------------- search / replace */

/* ted.s:926 next */
void next(void)
{
    struct item *a4 = thisitem;

    if (!a4->nextitem) {
        beepem();
        return;
    }
    dosearch(a4->nextitem, 1);  /* falls through into dosearch */
}

/* ted.s:931 dosearch */
void dosearch(struct item *a4, WORD d6)
{
    WORD d0, d7;

    /* direction in d6 (1 or -1), line in a4 */
    if (findreqla16[0] == 0) {
        beepem();
        return;
    }
    sleepon();
    d7 = (WORD)(curstop + cursy);
    d7 += d6;
    for (;;) {
        /* @938 .loop */
        d0 = isithere(a4, NULL);
        if (d0 >= 0)
            break;
        d7 += d6;
        if (d7 < 0) {           /* bmi (word) */
            beepem();
            return;
        }
        if ((UWORD)d7 >= numlines) {    /* bcc */
            beepem();
            return;
        }
        if (d6 < 0)
            a4 = a4->lastitem;
        else
            a4 = a4->nextitem;  /* @948 .skip */
    }
    /* @950 .gotit */
    pointeron();
    gotolined0((UWORD)d7);      /* a4 = thisitem */
    horizpos(d0);
    showhprop();
}

/* ted.s:959 previous */
void previous(void)
{
    struct item *a4 = thisitem;

    if (!a4->lastitem) {
        beepem();
        return;
    }
    dosearch(a4->lastitem, -1);
}

/* ted.s:965 replace */
void replace(void)
{
    if (findreqla16[0] == 0) {
        beepem();
        return;
    }
    if (!replace2(thisitem, NULL)) {
        beepem();
        return;
    }
    showall();
}

/* ted.s:971 replace2 */
BOOL replace2(struct item *a4, struct item **a4out)
{
    /* do all replaces on a4 line; return eq if at least one was done! */
    UBYTE *a0, *a1;
    const UBYTE *a2;
    WORD d0;
    UWORD d1, d5;

    d0 = isithere(a4, &a1);
    if (d0 < 0) {
        /* @1013 .skip */
        if (a4out)
            *a4out = a4;
        return FALSE;
    }
    allalt = (UWORD)-1;
    do {
        /* @977 .big */
        a0 = charbuff2;
        d1 = 0;
        while (d1 < (UWORD)d0) {        /* cmp d0,d1 / bcc */
            *a0++ = *a1++;
            d1++;
        }
        /* @984 .skip2: skip the search text */
        for (a2 = findreqla16; *a2++ != 0; )
            a1++;
        /* @990 .skip2a: the replacement */
        a2 = findreqla17;
        do
            d1++;
        while ((*a0++ = *a2++) != 0);
        a0--;
        d1--;                   /* column behind the replacement */
        /* @998 .loop3: rest of the line */
        while ((*a0++ = *a1++) != 0)
            ;
        a0--;
        if (a0 > charbuff3)
            ORIG_HAZARD("replace2: line longer than charbuff2 (survey-01 I.10)");
        while (a0 < charbuff3)  /* @1001 .loop4 */
            *a0++ = 32;
        /* @1005 .skip3 */
        d5 = charstobuff(charbuff2);
        a4 = makeline(a4, d5);
        d5 = d1;                /* move (a7)+,d5 */
        /* searching again from behind the replacement prevents an
           endless loop when the replacement contains the search text */
        d0 = isithere2(a4, d5, &a1);
    } while (d0 >= 0);
    if (a4out)
        *a4out = a4;
    return TRUE;
}
