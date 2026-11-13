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
#include "literal.h"

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
    litclear();                 /* NEW: literal characters (detok marks) */
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
            dst = detok(src, dst, charbuff4);   /* dst (a1) ON the 0 byte */
        } else {
            /* Fixed (#347): the original's copy leaves a1 BEHIND the 0
               byte, so the 0 stayed in the chmap style line and
               charstobuff kept it as the last "non space": every ASCII
               line of the block got an extra 0 byte (numchars + 1,
               written as "text\0\n").  dst back onto the 0, as
               blockuntab does. */
            while ((*dst++ = *src++) != 0)
                ;
            dst--;
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
    litclear();                 /* NEW: literal characters (detok marks) */
    item = bc.item;
    lineno = bc.line;
    do {
        /* @369 .loop2 */
        dst = charbuff2;
        src = item->chars;
        if (tokenise) {
            detok(src, dst, charbuff4);
        } else {
            while ((*dst++ = *src++) != 0)
                ;
        }
        /* @377 .skip3 */
        dst = charbuff2;
        src = dst;
        n = tabstop;
        /* Fixed (#348): the original removes the first tabstop characters
           WHATEVER they are, and tabstop = 0 or a line shorter than
           tabstop left a1 at the start: the whole line became spaces
           (survey-01 I.11).  Only leading spaces are skipped, at most
           tabstop of them; the rest of the line stays. */
        while (n && *src == 32) {
            src++;
            n--;
        }
        do {                            /* .loop5; NEW: with the marks */
            UBYTE l = litat(src);

            if (l)
                litput(dst, l);
            else
                *dst = *src;
            src++;
        } while (*dst++ != 0);
        dst--;
        /* @388 .loop3 */
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
   (ted.s lines 595-624): copy line a4 to buf (charbuff); a byte with bit
   7 starts a token -> ESC[1m name ESC[22m; the 0 byte -> LF.  Returns the
   Write length.  Fixed (#371) survey-01 I.14: the original takes bytes
   >= $80 for tokens also without tokenise (and standalone, where tokens
   == 0 made findtoke read at 0; a Latin-1 byte in front of the 0 took it
   as its second byte and ran on behind the line); without tokenise the
   bytes are copied as they are.  Fixed (#372) survey-01 I.10: the
   original has no limit and writes behind charbuff2/charbuff3, the end
   of TED's buffer block; the line now stops silently in front of the
   character or token (with its ESC sequences) that would not leave room
   for the LF before charbuff4 (charbuff2/charbuff3 are not in use while
   printing, as in the original). */
UWORD prtfile_line(struct item *line, UBYTE *buf)
{
    static const UBYTE bold[] = {27, '[', '1', 'm'};
    static const UBYTE normal[] = {27, '[', '2', '2', 'm'};
    UBYTE *dst = buf;
    UBYTE *const last = charbuff4 - 1;          /* room for the LF */
    const UBYTE *src = line->chars;
    const UBYTE *name, *e;
    UWORD tok, i;
    UBYTE c;

    for (;;) {
        /* @598 .dloop */
        c = *src++;
        if (c == 0)
            break;
        if (TED_ISLIT(c, *src)) {
            /* NEW (literal characters): the character's byte, no ESC
               sequence, with and without tokenise */
            if (dst >= last)
                break;
            *dst++ = *src++;
            continue;
        }
        if (!(c & 0x80) || !tokenise) {
            if (dst >= last)
                break;
            /* Fixed (#1017): the umlauts are stored as 26-31 (typed, and
               loaded with and without tokenise, germs) and shown as the
               umlauts wherever they are in the line; they are printed as
               the Latin-1 umlauts, as a text save writes them (design D6
               of literal-characters).  The original copied them as they
               are, with tokenise on: 27 (o umlaut) is ESC to the printer */
            *dst++ = unmapgerms(c);
            continue;
        }
        tok = (UWORD)(c << 8);
        tok |= *src++;
        name = (const UBYTE *)findtoke(tok);
        for (e = name; *e; e++)
            ;
        if (dst + sizeof(bold) + (e - name) + sizeof(normal) > last)
            break;
        for (i = 0; i < sizeof(bold); i++)      /* bold on */
            *dst++ = bold[i];
        while (name < e)
            *dst++ = *name++;
        for (i = 0; i < sizeof(normal); i++)    /* bold off */
            *dst++ = normal[i];
    }
    /* @622 .ddone */
    *dst++ = 10;
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

/* no line list (fileopen2 after freeall, fixed #1009): firstitem = 0
   tells outmem that there is no text to show */
static void nolines(void)
{
    firstitem = NULL;
    thisitem = NULL;
    enditem = NULL;
}

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
        /* empty file (or read error): a new text.  Fixed (#358): the
           original goes to donew without freeall - the lines of the old
           text stay allocated, nothing frees them later. */
        doclose(&rd);
        freeall();
        nolines();              /* fixed #1009, below */
        if (!donew())
            outmem();           /* Fixed (#356): the original went on */
        return;
    }
    /* @689 .ok */
    sleepon();
    blokdis();
    freeall();
    /* Fixed (#1009): until the first line is linked the text has no
       lines; the original left firstitem/thisitem/enditem on the freed
       text, and when the first allocation failed (here, or donew's
       above) outmem redrew from the freed lines and the editor went on
       with them.  outmem now leaves an empty text then (donew), or ends
       TED if even that fails. */
    nolines();
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
        if (!tokenise) {
            /* NEW (design D6): a text save writes only text - tokens as
               their names, a literal character as its byte, 26-31 as the
               umlauts (textline; dosbuff is free while writing); the 0
               byte becomes LF as in the original */
            UBYTE *e = textline(chars, dosbuff, dosbuff + doslen - 1);

            *e++ = 10;
            written = ted_write(rd.fh, dosbuff, (LONG)(e - dosbuff));
        } else {
            /* @781 .nocr */
            written = ted_write(rd.fh, chars, len);
        }
        if (written < 0) {
            /* allalt stays set.  Fixed (#380): the original goes on to
               .skip3 and still writes the .xtra and the .info of the
               truncated source (survey-01 I.15); here neither is
               written. */
            error(error3);
            doclose(&rd);
            goto noicons;
        }
        /* @791 .skip2 */
        item = item->nextitem;
        if (n-- == 0)
            break;
    }
    allalt = 0;
    /* @794 .skip3 */
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
noicons:  /* @820 .noicons */
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

/* NEW (literal characters): **dst = **src, a literal character with its
   mark (literal.h); both one further */
static void litmove(UBYTE **dst, UBYTE **src)
{
    UBYTE l = litat(*src);

    if (l)
        litput(*dst, l);
    else
        **dst = **src;
    (*dst)++;
    (*src)++;
}

/* ted.s:971 replace2 */
BOOL replace2(struct item *line, struct item **lineout)
{
    /* do all replaces on a4 line; return eq if at least one was done! */
    UBYTE *dst, *src;
    const UBYTE *p;
    WORD col;
    UWORD pos;

    litclear();                 /* NEW: literal characters */
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
            litmove(&dst, &src);        /* NEW: with the marks */
            pos++;
        }
        /* @984 .skip2: skip the search text */
        for (p = findreqla16; *p++ != 0; )
            src++;
        /* @990 .skip2a: the replacement */
        p = findreqla17;
        do {
            /* NEW (literal characters): an umlaut as typing stores it
               (26-31), another character >= $80 a literal character */
            UBYTE c = *p++;

            c = (UBYTE)germs_map(TED_LITIN(c));

            pos++;
            if (c & 0x80)
                litput(dst, c);
            else
                *dst = c;
        } while (*dst++ != 0);
        dst--;
        pos--;                  /* column behind the replacement */
        /* @998 .loop3: rest of the line */
        do                      /* NEW: with the marks */
            litmove(&dst, &src);
        while (dst[-1] != 0);
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
