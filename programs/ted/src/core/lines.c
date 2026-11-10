/*
 * lines.c - chunk 02 core (ted.s 1016-1980): see lines.h.
 *
 * Cursor up/down, insert/delete/join/split/return, top/bottom,
 * gotolined0/gd03 and the block operations kill/save/load/copy/forget.
 * The UI part of the chunk (escape, beepem, gadget, extmenu, menu, new) is
 * in src/edit.c.
 *
 * Handlers are `void name(void)` with asm a4 = thisitem at the entry.
 * Word arithmetic as in the asm: bcs/bcc/bhi/bls compare unsigned (UWORD
 * casts), blt/bgt/bmi/bpl signed (WORD casts).  Line numbers are
 * curstop + cursy (16 bit).  The block walk registers a5/d6/d5 of the asm
 * are a local struct bcur (fileio.h) of each block operation, passed to
 * bloksetup/getbchar.
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

/* line number of the cursor: move curstop,dN / add cursy,dN (16 bit) */
static UWORD curline(void)
{
    return (UWORD)(curstop + cursy);
}

/* bsr blokfix, plus: did it end in blokdis?  blokdis prints the status
   text with pstats -> qprint -> fontcalc, which leaves d1 = (char & $f0)
   << 3 (font 8) or the font 6 variant of the last character printed
   (a space): blokcopy uses d1 right after blokfix (ted.s:1970-1972), so
   the clobbered d1 is observable there (BUG(orig) #346).  blokfix itself is
   chunk 06 (lineedit.c); whether it reached blokdis is seen at bstx,
   which blokdis always sets to -1 (bstx is not read by blokfix). */
static BOOL blokfix_dis(void)
{
    WORD save = bstx;
    BOOL dis;

    bstx = 0;
    blokfix();
    dis = bstx == -1;
    if (!dis)
        bstx = save;
    return dis;
}

/* d1 after blokdis (fontcalc of the last ' ' of bltext): font 8:
   ($20 & $f0) << 3 = 256; font 6 (ted.s:5228): d1 = ($20 & $f0) << 1,
   d0 = d1, d1 <<= 1, d1 += d0 = 192 */
static UWORD blokdis_d1(void)
{
    UWORD high = 0x20 & 0xf0;

    if (fonthite == 6) {
        UWORD t;

        high <<= 1;
        t = high;
        high <<= 1;
        high += t;
        return high;
    }
    return (UWORD)(high << 3);
}

/* ted.s:1041 cursdown */
void cursdown(void)
{
    struct item *cur = thisitem;
    UWORD line;

    if (qualifier & 3) {
        /* Shift: page down */
        line = (UWORD)(curstop + cursy + edrows);
        if (line < numlines) {                  /* bcs gotolined02 */
            gotolined02(line);
            return;
        }
        gotolined02((UWORD)(numlines - 1));
        return;
    }
    /* .skip */
    if (cur->nextitem) {
        cursdownok(cur->nextitem);              /* bne cursdownok */
        return;
    }
    beepem();                                   /* falls through into beepem */
}

/* ted.s:1058 cursdownok */
void cursdownok(struct item *next)
{
    thisitem = next;
    movecdown();                                /* falls through into movecdown */
}

/* ted.s:1061 movecdown */
void movecdown(void)
{
    if ((UWORD)(curstop + edrows) >= numlines) {    /* bcc godown */
        godown();
        return;
    }
    if ((UWORD)(cursy + 1 + vscmarg) < edrows) {    /* bcs godown */
        godown();
        return;
    }
    curstop++;
    scrupall();
    showbotitem();                              /* bra showbotitem */
}

/* ted.s:1074 cursup */
void cursup(void)
{
    struct item *cur = thisitem;
    UWORD line;

    if (qualifier & 3) {
        /* Shift: page up */
        line = curline();
        if (line >= edrows) {                   /* bcc .skip2 */
            gotolined02((UWORD)(line - edrows)); /* .skip2 */
            return;
        }
        gotolined02(0);
        return;
    }
    /* .skip */
    if (!cur->lastitem) {
        beepem();                               /* beq beepem */
        return;
    }
    thisitem = cur->lastitem;
    if (curstop == 0 || cursy > vscmarg) {      /* beq goup / bhi goup */
        goup();
        return;
    }
    curstop--;
    scrdownall();
    showtopitem();                              /* bra showtopitem */
}

/* ted.s:1098 markindent - calulate indented block */
void markindent(void)
{
    struct item *cur = thisitem, *it;
    const UBYTE *p = cur->chars;
    WORD indent, ind;
    UWORD top, bot;

    if (*p == 0) {
        beepem();                               /* beq beepem */
        return;
    }
    indent = -1;
    do                                          /* .loop */
        indent++;
    while (*p++ == 32);
    /* .find - find above bits */
    it = cur;
    top = curline();
    bot = top;
    /* .uploop: beq tests Z of `move d1,d2` / `subq #1,d1` */
    while (top != 0) {
        it = it->lastitem;
        p = it->chars;
        ind = -1;
        do                                      /* .loop2 */
            ind++;
        while (*p++ == 32);
        if (p[-1] != 0 && ind < indent)         /* tst.b -(a0) / blt .dodown */
            break;
        /* .upline */
        top--;
    }
    /* .dodown - find down bits */
    it = cur;
    for (;;) {                                  /* .doloop */
        if (!it->nextitem)
            break;                              /* beq .downdone */
        it = it->nextitem;
        p = it->chars;
        ind = -1;
        do                                      /* .loop3 */
            ind++;
        while (*p++ == 32);
        if (p[-1] != 0 && ind < indent)         /* blt .downdone */
            break;
        /* .downline */
        bot++;
    }
    /* .downdone - d1 = top of block, d2 = bottom */
    btopy = top;
    btopx = 0;
    bboty = bot;
    bbotx = (WORD)(chcols - 1);
    bloken();
    showall();                                  /* bra showall */
}

/* ted.s:1153 insertline */
void insertline(void)
{
    struct item *cur = thisitem, *newl, *prev;

    newl = allocnull();
    if (!newl)
        outmem();                               /* beq outmem */
    allalt = (UWORD)-1;
    blokadj(curline());
    numlines++;
    prev = cur->lastitem;
    if (prev) {
        prev->nextitem = newl;
        newl->lastitem = prev;
    } else
        firstitem = newl;                       /* .skip */
    /* .skip2 */
    newl->nextitem = cur;
    cur->lastitem = newl;
    thisitem = newl;
    scrolldown(cursy);                          /* move cursy,d7 / bra scrolldown */
}

/* ted.s:1173 deleteline */
void deleteline(void)
{
    struct item *cur = thisitem, *next, *prev;
    UWORD line;

    allalt = (UWORD)-1;
    if (bboty != (UWORD)-1) {
        /* adjust block (survey-02 H).  Fixed (#353) survey-02 I.6: the
           original ends a one line block as bboty = -1 (clr bboty, then
           both -1) without blokdis: the block menu items, the "Block"
           status and bstx stayed.  The deleted line was the whole block:
           blokdis. */
        line = curline();
        if (line < btopy)                       /* bcs .skip0z */
            goto skip0z;
        if (line > bboty)                       /* bhi .skip0 */
            goto skip0;
        if (btopy == bboty) {
            blokdis();
            goto skip0;
        }
        bboty--;                                /* now >= btopy: .skip0 */
        goto skip0;
skip0z:
        btopy--;
        bboty--;
    }
skip0:
    next = cur->nextitem;
    if (!next) {
        freelast();                             /* beq freelast */
        return;
    }
    numlines--;
    prev = cur->lastitem;
    freethis();                                 /* frees thisitem = a4 */
    next->lastitem = prev;
    if (prev)
        prev->nextitem = next;                  /* .skip */
    else
        firstitem = next;
    /* .skip2 */
    thisitem = next;
    scrollup(cursy);                            /* move cursy,d7 */
    showbotitem();                              /* bra showbotitem */
}

/* ted.s:1208 freelast */
void freelast(void)
{
    struct item *cur = thisitem, *prev;

    prev = cur->lastitem;
    if (!prev) {
        freeonly();                             /* beq freeonly */
        return;
    }
    numlines--;
    freethis();
    thisitem = prev;
    enditem = prev;
    prev->nextitem = NULL;
    if (curstop != 0) {
        curstop--;
        scrollup(cursy);
        scrdownall();                           /* survey-02 I.17: both blits */
        showtopitem();                          /* bra showtopitem */
        return;
    }
    /* .skip */
    scrollup(cursy);
    goup();                                     /* bra goup */
}

/* ted.s:1226 freeonly */
void freeonly(void)
{
    freethis();
    allocfirst();                               /* bra allocfirst */
}

/* ted.s:1229 join */
void join(void)
{
    struct item *cur = thisitem, *next, *old, *after;
    UBYTE *row, *buf, *p;
    WORD len, width;
    UWORD line, size;

    if (!cur->nextitem) {
        beepem();                               /* beq beepem */
        return;
    }
    allalt = (UWORD)-1;
    next = cur->nextitem;
    row = chmap + chloc;
    len = (WORD)chcols;
    for (;;) {                                  /* .loop */
        len--;
        if (len < 0)                            /* bmi .skip */
            break;
        if (row[len] != 32)
            break;
    }
    /* .skip */
    len++;                                      /* offset for join! */

    if (bboty != (UWORD)-1) {
        /* block adjust (survey-02 H) */
        line = curline();
        if (line >= bboty)                      /* bcc .sskip */
            goto sskip;
        line++;
        if (line == btopy)
            goto nexttop;
        if (line < btopy)                       /* bcs .sskip2 */
            goto sskip2;
        if (line == bboty)
            goto nextbot;
        goto sskip3;
nexttop:
        if (line == bboty)
            goto nextboth;
        btopx = (WORD)(btopx + len);
        goto sskip4;
nextbot:
        if (line == btopy)                      /* dead (survey-02 I.18) */
            goto nextboth;
        bbotx = (WORD)(bbotx + len);
        blokfix();
        goto sskip3;
nextboth:
        width = (WORD)(bbotx - btopx);
        btopx = (WORD)(btopx + len);
        bbotx = (WORD)(btopx + width);
sskip4:
        blokfix();
sskip2:
        btopy--;
sskip3:
        bboty--;                                /* BUG(orig) (#346) survey-02 I.5: also after blokdis */
    }
sskip:
    buf = charbuff2;
    p = buf + len;
    while (--len >= 0)                          /* .loop2: subq / bmi */
        buf[len] = row[len];
    /* .skip2 */
    if (tokenise) {
        /* .skip2a.  BUG(orig) (#355) survey-02 I.3: no length check (detok can
           run past charbuff2/charbuff3) */
        p = detok(next->chars, p);
    } else {
        const UBYTE *s = next->chars;

        while ((*p++ = *s++) != 0)              /* .loop3 */
            ;
        p--;
    }
    /* .skip3 */
    while (p < charbuff3)
        *p++ = 32;
    /* .skip4 */
    cur = makeline(cur, charstobuff(charbuff2));
    old = cur->nextitem;                        /* = a5 */
    after = old->nextitem;
    if (after) {
        cur->nextitem = after;
        after->lastitem = cur;
    } else {
        /* .end */
        enditem = cur;
        cur->nextitem = NULL;
    }
    /* .done.  Fixed (#364) survey-02 I.1: the original frees with size
       numchars, not numchars + lenitem (`moveq #0,d0 / add.b
       numchars(a1),d0`, "was move.b") - 8-16 bytes lost per join; here
       the size freethis uses (as a word).  BUG(orig) (#365) survey-02
       I.2: no mforget2. */
    size = (UWORD)(lenitem + old->numchars);
    memalt = size;
    ted_free(old, size);
    numlines--;
    showall();                                  /* bra showall */
}

/* ted.s:1313 split */
void split(void)
{
    struct item *cur = thisitem, *newl, *next;
    UBYTE *p, *row;
    const UBYTE *s;
    UWORD col, line, width, len;

    allalt = (UWORD)-1;
    col = (UWORD)(cursoff + cursx);
    if (bboty != (UWORD)-1) {
        /* block adjust (survey-02 H) */
        line = curline();
        if (line > bboty)                       /* bhi .sskip */
            goto sskip;
        if (line == bboty)
            goto sonbot;
        if (line < btopy)                       /* bcs .sskip2 */
            goto sskip2;
        if (line != btopy)
            goto sinmid;
        /* .sontop */
        if (line == bboty)                      /* dead: line < bboty here */
            goto sonboth;
        if (col > (UWORD)btopx)                 /* bhi .sinmid */
            goto sinmid;
        btopx = (WORD)(btopx - col);
        goto sskip2;
sonbot:
        if (line == btopy)
            goto sonboth;
        if (col > (UWORD)bbotx)                 /* bhi .sskip */
            goto sskip;
sskip3:
        bbotx = (WORD)(bbotx - col);
        goto sinmid;
sonboth:
        if (col > (UWORD)bbotx)                 /* bhi .sskip */
            goto sskip;
        if (col > (UWORD)btopx)                 /* bhi .sskip3 */
            goto sskip3;
        width = (UWORD)(bbotx - btopx);
        btopx = (WORD)(btopx - col);
        bbotx = (WORD)(btopx + width);
        blokfix();
sskip2:
        btopy++;                                /* BUG(orig) (#346) survey-02 I.5: also after blokdis */
sinmid:
        bboty++;
    }
sskip:
    p = charbuff2;
    row = chmap + chloc;
    if (col >= chcols)
        ORIG_HAZARD("split: cursor column >= chcols (one byte behind the chmap row)");
    do {                                        /* .loop */
        *p++ = row[col];
        row[col] = 32;
        col++;
    } while (col < chcols);                     /* bcs .loop */
    while (p < charbuff3)                       /* .loop3 */
        *p++ = 32;
    /* .skipit */
    cur = makeline(cur, charstobuff(row));
    len = charstobuff(charbuff2);
    newl = alloc2((UWORD)(len + lenitem), TED_MEMF_PUBLIC);
    newl->numchars = (UBYTE)len;
    p = newl->chars;
    s = charbuff;
    while ((*p++ = *s++) != 0)                  /* .loop2 */
        ;
    /* survey-02 I.16: no remember for a label line */
    newl->lastitem = cur;
    next = cur->nextitem;
    if (next) {
        next->lastitem = newl;
        newl->nextitem = next;
    } else {
        /* .end */
        enditem = newl;
        newl->nextitem = NULL;
    }
    /* .skip */
    cur->nextitem = newl;
    numlines++;
    showall();                                  /* bra showall */
}

/* ted.s:1402 return */
void return_(void)
{
    struct item *cur = thisitem, *newl, *next;
    UWORD oldrows, oldpix, rows;

    if (qualifier & 3) {
        split();                                /* bne split */
        return;
    }
    newl = allocnull();
    if (!newl)
        outmem();                               /* beq outmem */
    allalt = (UWORD)-1;
    blokadj((UWORD)(curline() + 1));
    numlines++;
    next = cur->nextitem;
    if (!next) {
        cur->nextitem = newl;
        newl->lastitem = cur;
        enditem = newl;
    } else {
        /* .skip */
        next->lastitem = newl;
        cur->nextitem = newl;
        newl->nextitem = next;
        newl->lastitem = cur;
    }
    /* .skip2 */
    thisitem = newl;
    cursx = 0;
    if (cursoff != 0) {
        cursoff = 0;
        refresh();
    }
    /* .skip4 */
    showhprop();
    if ((UWORD)(cursy + 1 + vscmarg) >= edrows) {   /* bcs .skip3 */
        curstop++;
        /* scroll only the rows above the new line: edrows/pixrows
           changed temporarily (asm: saved on the stack) */
        oldrows = edrows;
        rows = (UWORD)(cursy + 1);              /* d0 */
        edrows = rows;
        oldpix = pixrows;
        pixrows = (UWORD)((ULONG)rows * fontmul);   /* mulu fontmul,d0 */
        scrupall();
        pixrows = oldpix;
        edrows = oldrows;
        return;
    }
    /* .skip3 */
    scrolldown((UWORD)(cursy + 1));
    movecdown();                                /* bra movecdown */
}

/* ted.s:1453 top */
void top(void)
{
    gotolined0(0);
}

/* ted.s:1456 bottom */
void bottom(void)
{
    gotolined0((UWORD)(numlines - 1));
}

/* ted.s:1490 gotolined0 - move to line pointed at by d0 */
struct item *gotolined0(UWORD line)
{
    /* move #-1,-(a7): change hprop.  `clr cursoff` is commented out in
       the asm (survey-02 I.12). */
    cursx = 0;
    return gd03(line, -1);
}

/* ted.s:1495 gotolined02 */
struct item *gotolined02(UWORD line)
{
    return gd03(line, 0);                       /* clr -(a7): leave hprop, falls into gd03 */
}

/* ted.s:1496 gd03 */
struct item *gd03(UWORD line, WORD flag)
{
    struct item *it = thisitem;                 /* asm: a4 (= thisitem at all callers) */
    UWORD oldtop = curstop;                     /* move d1,-(a7) */
    UWORD at = (UWORD)(curstop + cursy);        /* d1: line of `it` */
    UWORD newtop, limit, lasttop;

    if (line != at) {
        /* survey-02 I.13: no range check - a target >= numlines
           walks over the NULL at the end of the list (no caller passes one) */
        if (line >= at) {                       /* bcc .skip2 */
            do {                                /* .skip2 */
                at++;
#ifdef TED_HOST
                if (!it->nextitem) {
                    /* host-only guard: the asm reads on from address 0 */
                    ORIG_HAZARD("gd03: target line behind the end of the list");
                    break;
                }
#endif
                it = it->nextitem;
            } while (at != line);
        } else {
            do {                                /* .loop */
                at--;
#ifdef TED_HOST
                if (!it->lastitem) {
                    ORIG_HAZARD("gd03: target line before the start of the list");
                    break;
                }
#endif
                it = it->lastitem;
            } while (at != line);
        }
    }
    /* .skip */
    thisitem = it;
    newtop = (UWORD)(line - vscmarg);           /* d0 (d2 = line, d1 = vscmarg) */
    if ((WORD)newtop < 0)                       /* bpl .skip3 */
        goto skip5;
    /* .skip3 */
    if (numlines <= edrows)                     /* bls .skip5 */
        goto skip5;
    limit = (UWORD)(numlines - edrows + vscmarg);   /* d3 */
    if (line < limit) {                         /* bcs .skip4 */
        /* .skip4 */
        cursy = vscmarg;
        curstop = newtop;
        goto done;
    }
    lasttop = (UWORD)(limit - vscmarg);         /* d3 */
    curstop = lasttop;
    cursy = (UWORD)(line - lasttop);
    goto done;
skip5:
    curstop = 0;
    cursy = line;
done:
    calcpos();
    if (oldtop != curstop)                      /* move (a7)+,d1 / cmp curstop,d1 */
        showall();
    /* .noshowall */
    if (flag)                                   /* tst (a7)+ / bne showhprop */
        showhprop();
    return it;
}

/* ------------------------------------------------------------- blocks */

/* ted.s:1607 blokkill - kill the block */
void blokkill(void)
{
    struct item *cur, *next;
    UBYTE *p, *keep;
    WORD c;
    struct bcur bc;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    blokop = 0;
    allalt = (UWORD)-1;
    bloksetup(&bc);
    bc.col = 0;                                 /* moveq #0,d5 */
    p = charbuff2;
    while ((UWORD)bc.col < (UWORD)btopx)        /* .loop: bcc .skip0 */
        *p++ = (UBYTE)getbchar2(&bc);
    /* .skip0 */
    keep = p;                                   /* move.l a0,-(a7) */
    while (bc.line < bboty) {                   /* .skip: bcc .done0 */
        next = bc.item->nextitem;
        freeupa5(bc.item);
        bc.item = next;
        bc.line++;
    }
    /* .done0 */
    if (bc.line != btopy) {
        /* BUG(orig) (#354) survey-02 I.8: detoka5 also without tokenise (bytes
           >= $80 then go to findtoke with tokens possibly 0) */
        if (!tokenise)
            ORIG_HAZARD("blokkill: detoka5 without tokenise");
        detoka5(bc.item);
    }
    /* .done */
    p = keep;
    bc.col = bbotx;
    for (;;) {                                  /* .done3 */
        bc.col++;
        if ((UWORD)bc.col >= chcols)            /* bcc .done2 */
            break;
        c = getbchar2(&bc);
        bc.col--;
        *p++ = (UBYTE)c;
    }
    /* .done2 */
    while (p < charbuff3)
        *p++ = 32;
    /* .done4 */
    if (btopx == 0 && (UWORD)(bbotx + 1) >= chcols) {
        freeupa5(bc.item);                      /* whole last line */
    } else {
        /* .doit */
        cur = bc.item;
        thisitem = cur;
        makefirst(cur);
    }
    /* .rest */
    blokdis();
    if (curline() < numlines) {                 /* bcs .done5 */
        /* .done5 */
        calcitem();
        showall();                              /* bra showall */
        return;
    }
    curstop = (UWORD)(numlines - 1);
    cursy = 0;
    calcitem();
    showall();
    gotolined02(curstop);                       /* bra gotolined02 */
}

/* ted.s:1671 blocksave */
void blocksave(void)
{
    if (qualifier & 3) {
        blokkill();
        showvprop();                            /* bra showvprop */
        return;
    }
    bloksave();                                 /* beq.s bloksave (falls through) */
}

/* write one byte through the asm's buffer: `move.b d0,bloktopx` writes the
   HIGH byte of the word bloktopx (harmless: the mouse anchor is the write
   buffer, survey-02 G); C writes from a local and keeps the side effect */
static void bsbyte(tedfh fh, UBYTE c)
{
    bloktopx = (WORD)((bloktopx & 0x00FF) | (c << 8));
    ted_write(fh, &c, 1);                       /* result unchecked (survey-02 I.9) */
}

/* ted.s:1677 bloksave */
void bloksave(void)
{
    const UBYTE *name;
    tedfh fh;
    WORD c;
    UWORD len;
    UBYTE *buf;
    struct rdstate rd;
    struct bcur bc;

    /* cripple = 0: the full version */
    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    name = getbname(bloktext2);
    if (!name)
        return;                                 /* asm: addq #4,a7 in getbname */
    if (!doopen(&rd, name, TED_MODE_NEWFILE))
        return;                                 /* asm: addq #4,a7 + error */
    fh = rd.fh;                                 /* d7 */
    bloksetup(&bc);                             /* a4 = thisitem */
    blokop = (UWORD)-1;
    for (;;) {
        /* .loop */
        c = getbchar(&bc);
        if (c < 0)                              /* bmi .done */
            break;
        if (c != 0) {
            bsbyte(fh, (UBYTE)c);
            continue;
        }
        /* .newline */
        bsbyte(fh, 10);
        bc.col = 0;
        for (;;) {
            /* .loop3 */
            if (bc.line >= bboty) {             /* bcs .bok */
                if (tokenise) {
                    /* BUG(orig) (#350) survey-02 G: a block ending behind the
                       last text line arrives here with a5 = 0 */
                    if (!bc.item) {
                        ORIG_HAZARD("bloksave: detoka5 on a5 = 0");
#ifdef TED_HOST
                        /* host-only guard: the asm reads chars(0) */
                        bc.col = 0;
                        break;
#endif
                    }
                    detoka5(bc.item);
                    bc.col = 0;
                }
                break;                          /* bra .loop */
            }
            /* .bok: a whole line at once */
            detoka5(bc.item);
            buf = charbuff3;
            len = buf[0];
            buf[len] = 10;
            if (ted_write(fh, buf + 1, len) < 0) {  /* tst.l d0 / bpl .skip2 */
                error(error3);
                doclose(&rd);                   /* .done */
                return;
            }
            /* .skip2 */
            bc.item = bc.item->nextitem;
            bc.line++;
        }
    }
    /* .done */
    doclose(&rd);                               /* bra doclose (the rts behind
                                                   is cripple only) */
}

/* ted.s:1738 blokload - load block from disk */
void blokload(void)
{
    struct item *cur, *newl;
    const UBYTE *name;
    UBYTE *src, *dst, *endp, *keep;
    UWORD col, i, len;
    ULONG l;
    int r;
    struct rdstate rd;

    blokop = 0;
    name = getbname(bloktext);
    if (!name)
        return;                                 /* asm: addq #4,a7 in getbname */
    cur = thisitem;
    if (!doopen(&rd, name, TED_MODE_OLDFILE))
        return;
    if (fillbuff(&rd) == 0) {
        doclose(&rd);                           /* beq doclose */
        return;
    }
    mousealt = (UWORD)-1;
    allalt = (UWORD)-1;
    src = chmap + chloc;
    endp = src + chcols;
    /* survey-02 G (harmless, see bsbyte): the end pointer is kept in bloktopx as a
       LONG (bloktopx/bloktopy get its halves) */
    l = (ULONG)(size_t)endp;
    bloktopx = (WORD)(l >> 16);
    bloktopy = (WORD)l;
    col = (UWORD)(cursoff + cursx);
    dst = charbuff;
    i = 0;
    while (i < col) {                           /* .loop: bcc .skip */
        *dst++ = *src++ & 0x7F;                 /* bclr #7,d0 */
        i++;
    }
    /* .skip */
    keep = src;                                 /* move.l a2,-(a7) */
    r = readline(&rd, &len);
    if (r == RL_NOEOL)                          /* beq done2 */
        goto done2;
    dst = bloadadd(dst);
    cur = makeline2(dst, cur);
    for (;;) {
        /* .big */
        r = readline(&rd, &len);
        if (r == RL_EOF)                        /* bmi done3 */
            goto done3;
        if (r == RL_NOEOL)                      /* beq done4 */
            goto done4;
        cur = linka0(allocline(len), cur);
    }

done3:
    /* ted.s:1778 done3 */
    newl = allocnull();
    if (!newl)
        outmem();
    cur = linka0(newl, cur);
    dst = charbuff;
    goto done;
done2:
    /* ted.s:1783 done2 */
    dst = bloadadd(dst);
    goto done;
done4:
    /* ted.s:1785 done4: the line is replaced by makeline in blalldone
       right away (survey-02 I.10) */
    cur = linka0(allocline(len), cur);
    dst = charbuff;
    dst = bloadadd(dst);
    /* falls through into done */
done:
    /* ted.s:1791 done */
    doclose(&rd);
    src = keep;
    while (src < endp && dst < charbuff2) {     /* cmp.l bloktopx,a2 / cmp.l charbuff2,a3 */
        *dst++ = *src++ & 0x7F;
    }
    /* ted.s:1801 blalldone: charstobuff in place over charbuff
       (survey-02 I.10) */
    cur = makeline(cur, charstobuff(charbuff));
    calcitem();
    blokdis();
    showall();                                  /* bra showall */
}

/* ted.s:1808 blokcopy2 */
void blokcopy2(void)
{
    blokcopy();
    showvprop();                                /* bra showvprop */
}

/* ted.s:1811 blokcopy - copy blok */
void blokcopy(void)
{
    UWORD line;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    blokop = 0;
    allalt = (UWORD)-1;
    line = curline();
    if (line < btopy)                           /* bcs bcopyok */
        goto ok;
    if (line == btopy && inbloks())             /* same as first line before block */
        goto ok;
    /* .skip */
    if (line > bboty)                           /* bhi bcopyok */
        goto ok;
    if (line != bboty || inblokf()) {           /* bne beepem / beq beepem */
        beepem();
        return;
    }
ok:
    bcopyok();                                  /* falls through into bcopyok */
}

/* ted.s:1832 bcopyok */
void bcopyok(void)
{
    struct item *cur = thisitem, *newl, *prev;
    UBYTE *row, *dst, *keeprow;
    const UBYTE *s;
    UWORD col, curcol, len, line, keepcol, rows, shift;
    WORD c, x;
    struct bcur bc;

    bloksetup(&bc);
    /* a5 = btopy, d6 = btop line, d5 = btopx; a4 = thisy, d7 = this line */
    row = chmap + chloc;
    curcol = (UWORD)(cursoff + cursx);          /* d2 */
    col = 0;                                    /* d1 */
    dst = charbuff2;
    while (col < curcol) {                      /* .loop: bcc .skip2 */
        *dst++ = *row++ & 127;
        col++;
    }
    /* .skip2 */
    keepcol = col;                              /* movem.l d1/a1,-(a7) */
    keeprow = row;
    for (;;) {
        /* .skip */
        c = getbchar(&bc);                      /* d0 */
        if (c < 0)                              /* bmi .oneline */
            break;
        if (c != 0) {
            /* .proc */
            if (col < chcols) {
                *dst++ = (UBYTE)c;
                col++;
            }
            continue;
        }
        /* .newline */
        while (col < chcols) {
            *dst++ = 32;
            col++;
        }
        /* .newline2 */
        len = charstobuff(charbuff2);
        newl = alloc2((UWORD)(len + lenitem), TED_MEMF_PUBLIC);
        dst = newl->chars;
        s = charbuff;
        while ((*dst++ = *s++) != 0)            /* .nloop */
            ;
        for (;;) {
            /* .insert - new line BEFORE a4 (survey-02 I.16: no remember) */
            numlines++;
            newl->numchars = (UBYTE)len;
            prev = cur->lastitem;
            if (!prev) {
                firstitem = newl;
                newl->lastitem = NULL;
            } else {
                /* .skipz */
                prev->nextitem = newl;
                newl->lastitem = prev;
            }
            /* .skipy */
            cur->lastitem = newl;
            newl->nextitem = cur;
            if (bc.line >= bboty)               /* bcc .skipx */
                break;
            /* a whole line in the middle: raw copy (tokenised) */
            len = bc.item->numchars;
            newl = alloc2((UWORD)(len + lenitem), TED_MEMF_PUBLIC);
            dst = newl->chars;
            s = bc.item->chars;
            while ((*dst++ = *s++) != 0)        /* .loopz */
                ;
            bc.line++;
            bc.item = bc.item->nextitem;
        }
        /* .skipx */
        if (tokenise)
            detoka5(bc.item);
        /* .skipx2 */
        dst = charbuff2;
        bc.col = 0;
        col = 0;
    }
    /* .oneline.  survey-02 I.4: col = the PREFIX length again,
       so chcols - prefix characters are appended behind the fragment
       (the line may run into charbuff3; charstobuff cuts it to the same line) */
    col = keepcol;
    row = keeprow;
    while (col < chcols) {                      /* .oneline3 */
        *dst++ = *row++ & 127;
        col++;
    }
    if (dst > charbuff3)
        ORIG_HAZARD("blokcopy: line built past charbuff2 into charbuff3");
    while (dst < charbuff3)                     /* .oneline2 */
        *dst++ = 32;
    /* .line */
    makefirst(cur);
    x = (WORD)(cursoff + cursx);                /* d0 */
    line = curline();
    if (line > bboty)                           /* bhi .done2 */
        goto done2;
    if (line == bboty && (UWORD)x > (UWORD)bbotx)   /* bne .shit / bhi .done2 */
        goto done2;
    /* .shit - o.k., here we have to adjust the blocks position: first,
       calculate how far down the block has to go into d1 */
    rows = (UWORD)(bboty - btopy);              /* d1 */
    if ((UWORD)(bbotx + 1) >= chcols) {         /* bcs .shit2 */
        rows++;
        if (line < btopy)                       /* bcs .shit3 */
            goto shit3;
        btopx = (WORD)(btopx - x);
        goto shit3;
    }
    /* .shit2 */
    if (line < btopy)
        goto shit3;
    if (line != bboty) {
        shift = (UWORD)(bbotx - x + 1);         /* d2 */
        btopx = (WORD)(btopx + shift);
        goto shit4;
    }
    /* .shit5 */
    shift = (UWORD)(bbotx - btopx + 1);
    btopx = (WORD)(btopx + shift);
    bbotx = (WORD)(bbotx + shift);
    /* .shit4 */
shit4:
    if (blokfix_dis())
        rows = blokdis_d1();                    /* BUG(orig) (#346): d1 clobbered by blokdis */
shit3:
    btopy = (UWORD)(btopy + rows);
    bboty = (UWORD)(bboty + rows);
done2:
    calcitem();
    showall();                                  /* bra showall */
}

/* ted.s:1976 blokforget */
void blokforget(void)
{
    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    blokdis();
    showall();                                  /* bra showall */
}
