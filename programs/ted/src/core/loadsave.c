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
    struct item *item;
    UBYTE *src, *dst;
    UWORD n, lineno;
    struct bcur bc;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    bloksetup2(&bc);
    /* a5 = btopy, d6 = btop line; a4 = thisy, d7 = this line */
    item = bc.item;
    lineno = bc.line;
    do {
        /* @335 .loop2 */
        dst = charbuff2;
        n = tabstop;
        while (n) {             /* subq #1 / dbf: tabstop spaces */
            *dst++ = 32;
            n--;
        }
        src = item->chars;
        if (tokenise) {
            dst = detok(src, dst);      /* dst (a1) ON the 0 byte */
        } else {
            /* BUG(orig) (#347): the copy leaves a1 BEHIND the 0 byte, so the 0
               stays in the chmap style line and charstobuff keeps it as
               the last "non space": every ASCII line of the block gets an
               extra 0 byte (numchars + 1, written as "text\0\n"). */
            while ((*dst++ = *src++) != 0)
                ;
        }
        /* @348 .loop3 */
        if (dst > charbuff3)
            ORIG_HAZARD("blocktab: tabstop + line longer than charbuff2 "
                        "(survey-01 I.10)");
        while (dst < charbuff3)
            *dst++ = 32;
        thisitem = item;
        item = makefirst(item);
        item = item->nextitem;
        lineno++;
    } while (lineno <= bboty);  /* bls: unsigned */
    calcitem();
    showall();
}

/* ted.s:361 blockuntab */
void blockuntab(void)
{
    struct item *item;
    UBYTE *src, *dst;
    UWORD n, lineno;
    struct bcur bc;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    bloksetup2(&bc);
    item = bc.item;
    lineno = bc.line;
    do {
        /* @369 .loop2 */
        dst = charbuff2;
        src = item->chars;
        if (tokenise) {
            detok(src, dst);
        } else {
            while ((*dst++ = *src++) != 0)
                ;
        }
        /* @377 .skip3 */
        dst = charbuff2;
        src = dst;
        n = tabstop;
        /* BUG(orig) (#348): removes the first tabstop characters WHATEVER they
           are; tabstop = 0 or a line shorter than tabstop leaves a1 at
           the start: the whole line becomes spaces (survey-01 I.11). */
        if (n == 0)
            goto loop3;
        do {
            if (*src++ == 0)    /* tst.b (a0)+ / beq .loop3 */
                goto loop3;
        } while (--n);          /* dbf d0 (tabstop iterations) */
        while ((*dst++ = *src++) != 0)  /* .loop5 */
            ;
        dst--;
loop3:  /* @388 .loop3 */
        while (dst < charbuff3)
            *dst++ = 32;
        thisitem = item;
        item = makefirst(item);
        item = item->nextitem;
        lineno++;
    } while (lineno <= bboty);
    calcitem();
    showall();
}

/* ------------------------------------------------------- printing */

/* ted.s:570 (prtfile, core part - NEW name): the .ploop body of prtfile
   (ted.s lines 595-624): copy line a4 to buf; a byte with bit 7 starts a
   token -> ESC[1m name ESC[22m; the 0 byte -> LF.  Returns the Write
   length.  BUG(orig) (#371, #372): bytes >= $80 are always tokens (also in ASCII mode
   and standalone, where tokens == 0 makes findtoke read at 0; survey-01
   I.14); no limit for buf (charbuff: chcols+2 bytes, survey-01 I.10). */
UWORD prtfile_line(struct item *line, UBYTE *buf)
{
    UBYTE *dst = buf;
    const UBYTE *src = line->chars;
    const UBYTE *name;
    UWORD tok;
    UBYTE c;

    for (;;) {
        /* @598 .dloop */
        c = *src++;
        *dst++ = c;
        if (c == 0)
            break;
        if (!(c & 0x80))
            continue;
        tok = (UWORD)(*--dst << 8);
        tok |= *src++;
        /* bold on */
        *dst++ = 27;
        *dst++ = '[';
        *dst++ = '1';
        *dst++ = 'm';
        if (!tokens)
            ORIG_HAZARD("prtfile: token without token list (survey-01 I.14)");
        name = (const UBYTE *)findtoke(tok);
        while ((*dst++ = *name++) != 0)
            ;
        /* bold off (over the 0 byte of the name) */
        dst[-1] = 27;
        *dst++ = '[';
        *dst++ = '2';
        *dst++ = '2';
        *dst++ = 'm';
    }
    /* @622 .ddone */
    dst[-1] = 10;
    if (buf == charbuff && dst > charbuff2)
        ORIG_HAZARD("prtfile: printer line longer than charbuff (survey-01 I.10)");
    return (UWORD)(dst - buf);
}

/* ----------------------------------------------------- .xtra / .info */

/* ted.s:641 openinfo */
tedfh openinfo(void)
{
    tedfh lock;
    UBYTE *name = openmore(infotxt);

    lock = ted_lock((const char *)name, TED_ACCESS_READ);
    if (lock) {
        /* the icon exists: never overwritten */
        ted_unlock(lock);
        return 0;
    }
    /* @650 .skip2 */
    return ted_open((const char *)xtraat, TED_MODE_NEWFILE);
}

/* ted.s:658 openxtra */
tedfh openxtra(LONG mode)
{
    UBYTE *name = openmore(xtratxt);

    return ted_open((const char *)name, mode);
}

/* ted.s:664 openmore */
UBYTE *openmore(const char *ext)
{
    UBYTE *p = xtraat;

    while (*p++ != 0)
        ;
    p--;
    xtranull = p;
    while ((*p++ = (UBYTE)*ext++) != 0)
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
void fileopen2(UBYTE *name)
{
    struct item *item, *prev;
    UWORD len = 0;
    tedfh fh;
    struct rdstate rd;

    xtraat = name;
    if (!doopen(&rd, name, TED_MODE_OLDFILE))
        return;                 /* asm: addq #4,a7 + bra error */
    allalt = 0;
    doclean();
    if (fillbuff(&rd) == 0) {
        /* BUG(orig) (#358): empty file (or read error): donew without freeall -
           the old lines are lost (not freed) */
        doclose(&rd);
        donew();
        return;
    }
    /* @689 .ok */
    sleepon();
    blokdis();
    freeall();
    numlines = 1;
    readline(&rd, &len);        /* result ignored: fillbuff had bytes */
    item = allocline(len);
    firstitem = item;
    thisitem = item;
    item->lastitem = NULL;
    enditem = item;
    for (;;) {
        /* @699 .loop */
        prev = item;
        if (readline(&rd, &len) == RL_EOF)
            break;
        item = allocline(len);
        enditem = item;
        prev->nextitem = item;
        numlines++;
        item->lastitem = prev;
    }
    /* @708 .done */
    doclose(&rd);
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
void realsave(UBYTE *name)
{
    struct item *item;
    UBYTE *chars;
    UWORD len, n;
    LONG written;
    tedfh fh;
    struct rdstate rd;

    if (!doopen(&rd, name, TED_MODE_NEWFILE))
        return;                 /* the file is then only there as .bak
                                   (survey-01 I.15) */
    sleepon();
    item = firstitem;
    n = numlines - 1;           /* dbf: numlines 0 = 65536 rounds */
    for (;;) {
        /* @775 .loop */
        len = item->numchars;
        chars = item->chars;
        if (!tokenise)
            chars[len - 1] = 10; /* ASCII: the 0 byte becomes LF */
        /* @781 .nocr */
        written = ted_write(rd.fh, chars, len);
        chars[len - 1] = 0;
        if (written < 0) {
            /* BUG(orig) (#380): allalt stays set, but .xtra/.info are still
               written (survey-01 I.15) */
            error(error3);
            goto skip3;
        }
        /* @791 .skip2 */
        item = item->nextitem;
        if (n-- == 0)
            break;
    }
    allalt = 0;
skip3:  /* @794 .skip3 */
    doclose(&rd);
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
    struct item *line = thisitem;

    if (!line->nextitem) {
        beepem();
        return;
    }
    dosearch(line->nextitem, 1); /* falls through into dosearch */
}

/* ted.s:931 dosearch */
void dosearch(struct item *line, WORD dir)
{
    WORD col, lineno;

    /* direction in d6 (1 or -1), line in a4 */
    if (findreqla16[0] == 0) {
        beepem();
        return;
    }
    sleepon();
    lineno = (WORD)(curstop + cursy);
    lineno += dir;
    for (;;) {
        /* @938 .loop */
        col = isithere(line, NULL);
        if (col >= 0)
            break;
        lineno += dir;
        if (lineno < 0) {       /* bmi (word) */
            beepem();
            return;
        }
        if ((UWORD)lineno >= numlines) { /* bcc */
            beepem();
            return;
        }
        if (dir < 0)
            line = line->lastitem;
        else
            line = line->nextitem; /* @948 .skip */
    }
    /* @950 .gotit */
    pointeron();
    gotolined0((UWORD)lineno);  /* a4 = thisitem */
    horizpos(col);
    showhprop();
}

/* ted.s:959 previous */
void previous(void)
{
    struct item *line = thisitem;

    if (!line->lastitem) {
        beepem();
        return;
    }
    dosearch(line->lastitem, -1);
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
BOOL replace2(struct item *line, struct item **lineout)
{
    /* do all replaces on a4 line; return eq if at least one was done! */
    UBYTE *dst, *src;
    const UBYTE *p;
    WORD col;
    UWORD pos;

    col = isithere(line, &src);
    if (col < 0) {
        /* @1013 .skip */
        if (lineout)
            *lineout = line;
        return FALSE;
    }
    allalt = (UWORD)-1;
    do {
        /* @977 .big */
        dst = charbuff2;
        pos = 0;
        while (pos < (UWORD)col) {      /* cmp d0,d1 / bcc */
            *dst++ = *src++;
            pos++;
        }
        /* @984 .skip2: skip the search text */
        for (p = findreqla16; *p++ != 0; )
            src++;
        /* @990 .skip2a: the replacement */
        p = findreqla17;
        do
            pos++;
        while ((*dst++ = *p++) != 0);
        dst--;
        pos--;                  /* column behind the replacement */
        /* @998 .loop3: rest of the line */
        while ((*dst++ = *src++) != 0)
            ;
        dst--;
        if (dst > charbuff3)
            ORIG_HAZARD("replace2: line longer than charbuff2 (survey-01 I.10)");
        while (dst < charbuff3) /* @1001 .loop4 */
            *dst++ = 32;
        /* @1005 .skip3 */
        line = makeline(line, charstobuff(charbuff2));
        /* move (a7)+,d5: searching again from behind the replacement
           prevents an endless loop when the replacement contains the
           search text */
        col = isithere2(line, pos, &src);
    } while (col >= 0);
    if (lineout)
        *lineout = line;
    return TRUE;
}
