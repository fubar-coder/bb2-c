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
 * are the struct bc of fileio.h (bloksetup/getbchar work on it).
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
    UWORD d1 = 0x20 & 0xf0;

    if (fonthite == 6) {
        UWORD d0;

        d1 <<= 1;
        d0 = d1;
        d1 <<= 1;
        d1 += d0;
        return d1;
    }
    return (UWORD)(d1 << 3);
}

/* ted.s:1041 cursdown */
void cursdown(void)
{
    struct item *a4 = thisitem;
    UWORD d0;

    if (qualifier & 3) {
        /* Shift: page down */
        d0 = (UWORD)(curstop + cursy + edrows);
        if (d0 < numlines) {                    /* bcs gotolined02 */
            gotolined02(d0);
            return;
        }
        gotolined02((UWORD)(numlines - 1));
        return;
    }
    /* .skip */
    if (a4->nextitem) {
        cursdownok(a4->nextitem);               /* bne cursdownok */
        return;
    }
    beepem();                                   /* falls through into beepem */
}

/* ted.s:1058 cursdownok */
void cursdownok(struct item *d0)
{
    thisitem = d0;
    movecdown();                                /* falls through into movecdown */
}

/* ted.s:1061 movecdown */
void movecdown(void)
{
    UWORD d0;

    d0 = (UWORD)(curstop + edrows);
    if (d0 >= numlines) {                       /* bcc godown */
        godown();
        return;
    }
    d0 = (UWORD)(cursy + 1 + vscmarg);
    if (d0 < edrows) {                          /* bcs godown */
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
    struct item *a4 = thisitem;
    UWORD d0;

    if (qualifier & 3) {
        /* Shift: page up */
        d0 = curline();
        if (d0 >= edrows) {                     /* bcc .skip2 */
            gotolined02((UWORD)(d0 - edrows));  /* .skip2 */
            return;
        }
        gotolined02(0);
        return;
    }
    /* .skip */
    if (!a4->lastitem) {
        beepem();                               /* beq beepem */
        return;
    }
    thisitem = a4->lastitem;
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
    struct item *a4 = thisitem, *a1;
    const UBYTE *a0 = a4->chars;
    WORD d0, d3;
    UWORD d1, d2;

    if (*a0 == 0) {
        beepem();                               /* beq beepem */
        return;
    }
    d0 = -1;
    do                                          /* .loop */
        d0++;
    while (*a0++ == 32);
    /* .find - find above bits */
    a1 = a4;
    d1 = curline();
    d2 = d1;
    /* .uploop: beq tests Z of `move d1,d2` / `subq #1,d1` */
    while (d1 != 0) {
        a1 = a1->lastitem;
        a0 = a1->chars;
        d3 = -1;
        do                                      /* .loop2 */
            d3++;
        while (*a0++ == 32);
        if (a0[-1] != 0 && d3 < d0)             /* tst.b -(a0) / blt .dodown */
            break;
        /* .upline */
        d1--;
    }
    /* .dodown - find down bits */
    a1 = a4;
    for (;;) {                                  /* .doloop */
        if (!a1->nextitem)
            break;                              /* beq .downdone */
        a1 = a1->nextitem;
        a0 = a1->chars;
        d3 = -1;
        do                                      /* .loop3 */
            d3++;
        while (*a0++ == 32);
        if (a0[-1] != 0 && d3 < d0)             /* blt .downdone */
            break;
        /* .downline */
        d2++;
    }
    /* .downdone - d1 = top of block, d2 = bottom */
    btopy = d1;
    btopx = 0;
    bboty = d2;
    bbotx = (WORD)(chcols - 1);
    bloken();
    showall();                                  /* bra showall */
}

/* ted.s:1153 insertline */
void insertline(void)
{
    struct item *a4 = thisitem, *a0, *a1;

    a0 = allocnull();
    if (!a0)
        outmem();                               /* beq outmem */
    allalt = (UWORD)-1;
    blokadj(curline());
    numlines++;
    a1 = a4->lastitem;
    if (a1) {
        a1->nextitem = a0;
        a0->lastitem = a1;
    } else
        firstitem = a0;                         /* .skip */
    /* .skip2 */
    a0->nextitem = a4;
    a4->lastitem = a0;
    thisitem = a0;
    scrolldown(cursy);                          /* move cursy,d7 / bra scrolldown */
}

/* ted.s:1173 deleteline */
void deleteline(void)
{
    struct item *a4 = thisitem, *d7, *d6;
    UWORD d0;

    allalt = (UWORD)-1;
    if (bboty != (UWORD)-1) {
        /* adjust block (survey-02 H).  BUG(orig) (#353) survey-02 I.6: a one
           line block ends as bboty = -1 without blokdis. */
        d0 = curline();
        if (d0 < btopy)                         /* bcs .skip0z */
            goto skip0z;
        if (d0 > bboty)                         /* bhi .skip0 */
            goto skip0;
        bboty--;
        if (bboty >= btopy)                     /* bcc .skip0 */
            goto skip0;
        bboty = 0;
skip0z:
        btopy--;
        bboty--;
    }
skip0:
    d7 = a4->nextitem;
    if (!d7) {
        freelast();                             /* beq freelast */
        return;
    }
    numlines--;
    d6 = a4->lastitem;
    freethis();                                 /* frees thisitem = a4 */
    d7->lastitem = d6;
    if (d6)
        d6->nextitem = d7;                      /* .skip */
    else
        firstitem = d7;
    /* .skip2 */
    thisitem = d7;
    scrollup(cursy);                            /* move cursy,d7 */
    showbotitem();                              /* bra showbotitem */
}

/* ted.s:1208 freelast */
void freelast(void)
{
    struct item *a4 = thisitem, *d6;

    d6 = a4->lastitem;
    if (!d6) {
        freeonly();                             /* beq freeonly */
        return;
    }
    numlines--;
    freethis();
    thisitem = d6;
    enditem = d6;
    d6->nextitem = NULL;
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
    struct item *a4 = thisitem, *a5, *a1, *n;
    UBYTE *a2, *a0, *b1;
    WORD d0, d2, d3;
    UWORD d1, size;

    if (!a4->nextitem) {
        beepem();                               /* beq beepem */
        return;
    }
    allalt = (UWORD)-1;
    a5 = a4->nextitem;
    a2 = chmap + chloc;
    d0 = (WORD)chcols;
    for (;;) {                                  /* .loop */
        d0--;
        if (d0 < 0)                             /* bmi .skip */
            break;
        if (a2[d0] != 32)
            break;
    }
    /* .skip */
    d0++;                                       /* offset for join! */

    if (bboty != (UWORD)-1) {
        /* block adjust (survey-02 H) */
        d1 = curline();
        if (d1 >= bboty)                        /* bcc .sskip */
            goto sskip;
        d1++;
        if (d1 == btopy)
            goto nexttop;
        if (d1 < btopy)                         /* bcs .sskip2 */
            goto sskip2;
        if (d1 == bboty)
            goto nextbot;
        goto sskip3;
nexttop:
        if (d1 == bboty)
            goto nextboth;
        btopx = (WORD)(btopx + d0);
        goto sskip4;
nextbot:
        if (d1 == btopy)                        /* dead (survey-02 I.18) */
            goto nextboth;
        bbotx = (WORD)(bbotx + d0);
        blokfix();
        goto sskip3;
nextboth:
        d3 = (WORD)(bbotx - btopx);
        d2 = (WORD)(btopx + d0);
        btopx = d2;
        d2 = (WORD)(d2 + d3);
        bbotx = d2;
sskip4:
        blokfix();
sskip2:
        btopy--;
sskip3:
        bboty--;                                /* BUG(orig) (#346) survey-02 I.5: also after blokdis */
    }
sskip:
    a0 = charbuff2;
    b1 = a0 + d0;
    while (--d0 >= 0)                           /* .loop2: subq / bmi */
        a0[d0] = a2[d0];
    /* .skip2 */
    if (tokenise) {
        /* .skip2a.  BUG(orig) (#355) survey-02 I.3: no length check (detok can
           run past charbuff2/charbuff3) */
        b1 = detok(a5->chars, b1);
    } else {
        const UBYTE *s = a5->chars;

        while ((*b1++ = *s++) != 0)             /* .loop3 */
            ;
        b1--;
    }
    /* .skip3 */
    while (b1 < charbuff3)
        *b1++ = 32;
    /* .skip4 */
    a4 = makeline(a4, charstobuff(charbuff2));
    a1 = a4->nextitem;                          /* = a5 */
    n = a1->nextitem;
    if (n) {
        a4->nextitem = n;
        n->lastitem = a4;
    } else {
        /* .end */
        enditem = a4;
        a4->nextitem = NULL;
    }
    /* .done.  BUG(orig) (#364, #365) survey-02 I.1: FreeMem size numchars, not
       numchars + lenitem (`add.b numchars(a1),d0`, "was move.b") - 9
       bytes lost per join; the host reports the size mismatch through
       ORIG_HAZARD (stubs.c).  survey-02 I.2: no mforget2. */
    size = a1->numchars;                        /* moveq #0,d0 / add.b numchars(a1),d0 */
    memalt = size;
    ted_free(a1, size);
    numlines--;
    showall();                                  /* bra showall */
}

/* ted.s:1313 split */
void split(void)
{
    struct item *a4 = thisitem, *a0, *d1;
    UBYTE *b0, *a2;
    const UBYTE *s;
    UWORD d0, d2, d3, d5;

    allalt = (UWORD)-1;
    d0 = (UWORD)(cursoff + cursx);
    if (bboty != (UWORD)-1) {
        /* block adjust (survey-02 H) */
        d2 = curline();
        if (d2 > bboty)                         /* bhi .sskip */
            goto sskip;
        if (d2 == bboty)
            goto sonbot;
        if (d2 < btopy)                         /* bcs .sskip2 */
            goto sskip2;
        if (d2 != btopy)
            goto sinmid;
        /* .sontop */
        if (d2 == bboty)                        /* dead: d2 < bboty here */
            goto sonboth;
        if (d0 > (UWORD)btopx)                  /* bhi .sinmid */
            goto sinmid;
        btopx = (WORD)(btopx - d0);
        goto sskip2;
sonbot:
        if (d2 == btopy)
            goto sonboth;
        if (d0 > (UWORD)bbotx)                  /* bhi .sskip */
            goto sskip;
sskip3:
        bbotx = (WORD)(bbotx - d0);
        goto sinmid;
sonboth:
        if (d0 > (UWORD)bbotx)                  /* bhi .sskip */
            goto sskip;
        if (d0 > (UWORD)btopx)                  /* bhi .sskip3 */
            goto sskip3;
        d3 = (UWORD)(bbotx - btopx);
        d2 = (UWORD)(btopx - d0);
        btopx = (WORD)d2;
        d2 = (UWORD)(d2 + d3);
        bbotx = (WORD)d2;
        blokfix();
sskip2:
        btopy++;                                /* BUG(orig) (#346) survey-02 I.5: also after blokdis */
sinmid:
        bboty++;
    }
sskip:
    b0 = charbuff2;
    a2 = chmap + chloc;
    if (d0 >= chcols)
        ORIG_HAZARD("split: cursor column >= chcols (one byte behind the chmap row)");
    do {                                        /* .loop */
        *b0++ = a2[d0];
        a2[d0] = 32;
        d0++;
    } while (d0 < chcols);                      /* bcs .loop */
    while (b0 < charbuff3)                      /* .loop3 */
        *b0++ = 32;
    /* .skipit */
    a4 = makeline(a4, charstobuff(a2));
    d5 = charstobuff(charbuff2);
    a0 = alloc2((UWORD)(d5 + lenitem), TED_MEMF_PUBLIC);
    a0->numchars = (UBYTE)d5;
    b0 = a0->chars;
    s = charbuff;
    while ((*b0++ = *s++) != 0)                 /* .loop2 */
        ;
    /* survey-02 I.16: no remember for a label line */
    a0->lastitem = a4;
    d1 = a4->nextitem;
    if (d1) {
        d1->lastitem = a0;
        a0->nextitem = d1;
    } else {
        /* .end */
        enditem = a0;
        a0->nextitem = NULL;
    }
    /* .skip */
    a4->nextitem = a0;
    numlines++;
    showall();                                  /* bra showall */
}

/* ted.s:1402 return */
void return_(void)
{
    struct item *a4 = thisitem, *a0, *d1;
    UWORD d0, se, sp;

    if (qualifier & 3) {
        split();                                /* bne split */
        return;
    }
    a0 = allocnull();
    if (!a0)
        outmem();                               /* beq outmem */
    allalt = (UWORD)-1;
    blokadj((UWORD)(curline() + 1));
    numlines++;
    d1 = a4->nextitem;
    if (!d1) {
        a4->nextitem = a0;
        a0->lastitem = a4;
        enditem = a0;
    } else {
        /* .skip */
        d1->lastitem = a0;
        a4->nextitem = a0;
        a0->nextitem = d1;
        a0->lastitem = a4;
    }
    /* .skip2 */
    thisitem = a0;
    cursx = 0;
    if (cursoff != 0) {
        cursoff = 0;
        refresh();
    }
    /* .skip4 */
    showhprop();
    d0 = (UWORD)(cursy + 1 + vscmarg);
    if (d0 >= edrows) {                         /* bcs .skip3 */
        curstop++;
        /* scroll only the rows above the new line: edrows/pixrows
           changed temporarily (asm: saved on the stack) */
        se = edrows;
        d0 = (UWORD)(cursy + 1);
        edrows = d0;
        sp = pixrows;
        pixrows = (UWORD)((ULONG)d0 * fontmul); /* mulu fontmul,d0 */
        scrupall();
        pixrows = sp;
        edrows = se;
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
struct item *gotolined0(UWORD d0)
{
    /* move #-1,-(a7): change hprop.  `clr cursoff` is commented out in
       the asm (survey-02 I.12). */
    cursx = 0;
    return gd03(d0, -1);
}

/* ted.s:1495 gotolined02 */
struct item *gotolined02(UWORD d0)
{
    return gd03(d0, 0);                         /* clr -(a7): leave hprop, falls into gd03 */
}

/* ted.s:1496 gd03 */
struct item *gd03(UWORD d0, WORD flag)
{
    struct item *a4 = thisitem;                 /* asm: a4 (= thisitem at all callers) */
    UWORD d1, d2, d3, oldtop;

    d1 = curstop;
    oldtop = d1;                                /* move d1,-(a7) */
    d1 = (UWORD)(d1 + cursy);
    if (d0 != d1) {
        /* survey-02 I.13: no range check - a target >= numlines
           walks over the NULL at the end of the list (no caller passes one) */
        if (d0 >= d1) {                         /* bcc .skip2 */
            do {                                /* .skip2 */
                d1++;
#ifdef TED_HOST
                if (!a4->nextitem) {
                    /* host-only guard: the asm reads on from address 0 */
                    ORIG_HAZARD("gd03: target line behind the end of the list");
                    break;
                }
#endif
                a4 = a4->nextitem;
            } while (d1 != d0);
        } else {
            do {                                /* .loop */
                d1--;
#ifdef TED_HOST
                if (!a4->lastitem) {
                    ORIG_HAZARD("gd03: target line before the start of the list");
                    break;
                }
#endif
                a4 = a4->lastitem;
            } while (d1 != d0);
        }
    }
    /* .skip */
    thisitem = a4;
    d1 = vscmarg;
    d2 = d0;
    d0 = (UWORD)(d0 - d1);
    if ((WORD)d0 < 0)                           /* bpl .skip3 */
        goto skip5;
    /* .skip3 */
    d3 = numlines;
    if (d3 <= edrows)                           /* bls .skip5 */
        goto skip5;
    d3 = (UWORD)(d3 - edrows + vscmarg);
    if (d2 < d3) {                              /* bcs .skip4 */
        /* .skip4 */
        cursy = d1;
        curstop = d0;
        goto done;
    }
    d3 = (UWORD)(d3 - vscmarg);
    curstop = d3;
    cursy = (UWORD)(d2 - d3);
    goto done;
skip5:
    curstop = 0;
    cursy = d2;
done:
    calcpos();
    if (oldtop != curstop)                      /* move (a7)+,d1 / cmp curstop,d1 */
        showall();
    /* .noshowall */
    if (flag)                                   /* tst (a7)+ / bne showhprop */
        showhprop();
    return a4;
}

/* ------------------------------------------------------------- blocks */

/* ted.s:1607 blokkill - kill the block */
void blokkill(void)
{
    struct item *a4, *next;
    UBYTE *a0, *keep;
    WORD d0;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    blokop = 0;
    allalt = (UWORD)-1;
    bloksetup();
    bc.col = 0;                                 /* moveq #0,d5 */
    a0 = charbuff2;
    while ((UWORD)bc.col < (UWORD)btopx)        /* .loop: bcc .skip0 */
        *a0++ = (UBYTE)getbchar2();
    /* .skip0 */
    keep = a0;                                  /* move.l a0,-(a7) */
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
    a0 = keep;
    bc.col = bbotx;
    for (;;) {                                  /* .done3 */
        bc.col++;
        if ((UWORD)bc.col >= chcols)            /* bcc .done2 */
            break;
        d0 = getbchar2();
        bc.col--;
        *a0++ = (UBYTE)d0;
    }
    /* .done2 */
    while (a0 < charbuff3)
        *a0++ = 32;
    /* .done4 */
    if (btopx == 0 && (UWORD)(bbotx + 1) >= chcols) {
        freeupa5(bc.item);                      /* whole last line */
    } else {
        /* .doit */
        a4 = bc.item;
        thisitem = a4;
        makefirst(a4);
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
    WORD d0;
    UWORD d3;
    UBYTE *a0;

    /* cripple = 0: the full version */
    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    name = getbname(bloktext2);
    if (!name)
        return;                                 /* asm: addq #4,a7 in getbname */
    if (!doopen(name, TED_MODE_NEWFILE))
        return;                                 /* asm: addq #4,a7 + error */
    fh = rd.fh;                                 /* d7 */
    bloksetup();                                /* a4 = thisitem */
    blokop = (UWORD)-1;
    for (;;) {
        /* .loop */
        d0 = getbchar();
        if (d0 < 0)                             /* bmi .done */
            break;
        if (d0 != 0) {
            bsbyte(fh, (UBYTE)d0);
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
            a0 = charbuff3;
            d3 = a0[0];
            a0[d3] = 10;
            if (ted_write(fh, a0 + 1, d3) < 0) {    /* tst.l d0 / bpl .skip2 */
                error(error3);
                doclose();                      /* .done */
                return;
            }
            /* .skip2 */
            bc.item = bc.item->nextitem;
            bc.line++;
        }
    }
    /* .done */
    doclose();                                  /* bra doclose (the rts behind
                                                   is cripple only) */
}

/* ted.s:1738 blokload - load block from disk */
void blokload(void)
{
    struct item *a4, *a0;
    const UBYTE *name;
    UBYTE *a2, *a3, *endp, *keep;
    UWORD d1, d4, d5;
    ULONG l;
    int r;

    blokop = 0;
    name = getbname(bloktext);
    if (!name)
        return;                                 /* asm: addq #4,a7 in getbname */
    a4 = thisitem;
    if (!doopen(name, TED_MODE_OLDFILE))
        return;
    if (fillbuff() == 0) {
        doclose();                              /* beq doclose */
        return;
    }
    mousealt = (UWORD)-1;
    allalt = (UWORD)-1;
    a2 = chmap + chloc;
    endp = a2 + chcols;
    /* survey-02 G (harmless, see bsbyte): the end pointer is kept in bloktopx as a
       LONG (bloktopx/bloktopy get its halves) */
    l = (ULONG)(size_t)endp;
    bloktopx = (WORD)(l >> 16);
    bloktopy = (WORD)l;
    d1 = (UWORD)(cursoff + cursx);
    a3 = charbuff;
    d4 = 0;
    while (d4 < d1) {                           /* .loop: bcc .skip */
        *a3++ = *a2++ & 0x7F;                   /* bclr #7,d0 */
        d4++;
    }
    /* .skip */
    keep = a2;                                  /* move.l a2,-(a7) */
    r = readline(&d5);
    if (r == RL_NOEOL)                          /* beq done2 */
        goto done2;
    a3 = bloadadd(a3);
    a4 = makeline2(a3, a4);
    for (;;) {
        /* .big */
        r = readline(&d5);
        if (r == RL_EOF)                        /* bmi done3 */
            goto done3;
        if (r == RL_NOEOL)                      /* beq done4 */
            goto done4;
        a4 = linka0(allocline(d5), a4);
    }

done3:
    /* ted.s:1778 done3 */
    a0 = allocnull();
    if (!a0)
        outmem();
    a4 = linka0(a0, a4);
    a3 = charbuff;
    goto done;
done2:
    /* ted.s:1783 done2 */
    a3 = bloadadd(a3);
    goto done;
done4:
    /* ted.s:1785 done4: the line is replaced by makeline in blalldone
       right away (survey-02 I.10) */
    a4 = linka0(allocline(d5), a4);
    a3 = charbuff;
    a3 = bloadadd(a3);
    /* falls through into done */
done:
    /* ted.s:1791 done */
    doclose();
    a2 = keep;
    while (a2 < endp && a3 < charbuff2) {       /* cmp.l bloktopx,a2 / cmp.l charbuff2,a3 */
        *a3++ = *a2++ & 0x7F;
    }
    /* ted.s:1801 blalldone: charstobuff in place over charbuff
       (survey-02 I.10) */
    a4 = makeline(a4, charstobuff(charbuff));
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
    UWORD d7;

    if (bboty == (UWORD)-1) {
        beepem();
        return;
    }
    blokop = 0;
    allalt = (UWORD)-1;
    d7 = curline();
    if (d7 < btopy)                             /* bcs bcopyok */
        goto ok;
    if (d7 == btopy && inbloks())               /* same as first line before block */
        goto ok;
    /* .skip */
    if (d7 > bboty)                             /* bhi bcopyok */
        goto ok;
    if (d7 != bboty || inblokf()) {             /* bne beepem / beq beepem */
        beepem();
        return;
    }
ok:
    bcopyok();                                  /* falls through into bcopyok */
}

/* ted.s:1832 bcopyok */
void bcopyok(void)
{
    struct item *a4 = thisitem, *a0, *d1p;
    UBYTE *a1, *b0, *keepa1;
    const UBYTE *s;
    UWORD d1, d2, d5, d7, keepd1;
    WORD d0;

    bloksetup();
    /* a5 = btopy, d6 = btop line, d5 = btopx; a4 = thisy, d7 = this line */
    a1 = chmap + chloc;
    d2 = (UWORD)(cursoff + cursx);
    d1 = 0;
    b0 = charbuff2;
    while (d1 < d2) {                           /* .loop: bcc .skip2 */
        *b0++ = *a1++ & 127;
        d1++;
    }
    /* .skip2 */
    keepd1 = d1;                                /* movem.l d1/a1,-(a7) */
    keepa1 = a1;
    for (;;) {
        /* .skip */
        d0 = getbchar();
        if (d0 < 0)                             /* bmi .oneline */
            break;
        if (d0 != 0) {
            /* .proc */
            if (d1 < chcols) {
                *b0++ = (UBYTE)d0;
                d1++;
            }
            continue;
        }
        /* .newline */
        while (d1 < chcols) {
            *b0++ = 32;
            d1++;
        }
        /* .newline2 */
        d5 = charstobuff(charbuff2);
        a0 = alloc2((UWORD)(d5 + lenitem), TED_MEMF_PUBLIC);
        b0 = a0->chars;
        s = charbuff;
        while ((*b0++ = *s++) != 0)             /* .nloop */
            ;
        for (;;) {
            /* .insert - new line BEFORE a4 (survey-02 I.16: no remember) */
            numlines++;
            a0->numchars = (UBYTE)d5;
            d1p = a4->lastitem;
            if (!d1p) {
                firstitem = a0;
                a0->lastitem = NULL;
            } else {
                /* .skipz */
                d1p->nextitem = a0;
                a0->lastitem = d1p;
            }
            /* .skipy */
            a4->lastitem = a0;
            a0->nextitem = a4;
            if (bc.line >= bboty)               /* bcc .skipx */
                break;
            /* a whole line in the middle: raw copy (tokenised) */
            d5 = bc.item->numchars;
            a0 = alloc2((UWORD)(d5 + lenitem), TED_MEMF_PUBLIC);
            b0 = a0->chars;
            s = bc.item->chars;
            while ((*b0++ = *s++) != 0)         /* .loopz */
                ;
            bc.line++;
            bc.item = bc.item->nextitem;
        }
        /* .skipx */
        if (tokenise)
            detoka5(bc.item);
        /* .skipx2 */
        b0 = charbuff2;
        bc.col = 0;
        d1 = 0;
    }
    /* .oneline.  survey-02 I.4: d1 = the PREFIX length again,
       so chcols - prefix characters are appended behind the fragment
       (the line may run into charbuff3; charstobuff cuts it to the same line) */
    d1 = keepd1;
    a1 = keepa1;
    while (d1 < chcols) {                       /* .oneline3 */
        *b0++ = *a1++ & 127;
        d1++;
    }
    if (b0 > charbuff3)
        ORIG_HAZARD("blokcopy: line built past charbuff2 into charbuff3");
    while (b0 < charbuff3)                      /* .oneline2 */
        *b0++ = 32;
    /* .line */
    makefirst(a4);
    d0 = (WORD)(cursoff + cursx);
    d7 = curline();
    if (d7 > bboty)                             /* bhi .done2 */
        goto done2;
    if (d7 == bboty && (UWORD)d0 > (UWORD)bbotx)    /* bne .shit / bhi .done2 */
        goto done2;
    /* .shit - o.k., here we have to adjust the blocks position: first,
       calculate how far down the block has to go into d1 */
    d1 = (UWORD)(bboty - btopy);
    d2 = (UWORD)(bbotx + 1);
    if (d2 >= chcols) {                         /* bcs .shit2 */
        d1++;
        if (d7 < btopy)                         /* bcs .shit3 */
            goto shit3;
        btopx = (WORD)(btopx - d0);
        goto shit3;
    }
    /* .shit2 */
    if (d7 < btopy)
        goto shit3;
    if (d7 != bboty) {
        d2 = (UWORD)(bbotx - d0 + 1);
        btopx = (WORD)(btopx + d2);
        goto shit4;
    }
    /* .shit5 */
    d2 = (UWORD)(bbotx - btopx + 1);
    btopx = (WORD)(btopx + d2);
    bbotx = (WORD)(bbotx + d2);
    /* .shit4 */
shit4:
    if (blokfix_dis())
        d1 = blokdis_d1();                      /* BUG(orig) (#346): d1 clobbered by blokdis */
shit3:
    btopy = (UWORD)(btopy + d1);
    bboty = (UWORD)(bboty + d1);
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
