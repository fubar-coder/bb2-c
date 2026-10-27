/*
 * screen.c - chunk 05, Amiga part (ted.s 4198-5252): see screen.h.
 *
 * Blitter scrolling of the text area, screen/window open and close, user
 * copper list, colours, cls, horizontal scrolling and the own text output.
 * The chmap halves of the blits are chmap_* in the core (tokit.c, DIFF:
 * CPU instead of blitter, notes/abweichungen.md); the bitplane blits stay
 * on the blitter with the same register writes as the asm.
 */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <hardware/custom.h>
#include <graphics/gfxmacros.h>
#include <dos/dosextens.h>
#include <string.h>
#include "ted.h"

struct Screen scdata;                   /* ted.s:4617 ds.b 84 (GetScreenData) */
UWORD blitmod;                          /* ted.s:4666 */
APTR oldmywindow;                       /* ted.s:4811 */
UWORD screensopen;                      /* ted.s:4812 */
struct UCopList *coplist;               /* ted.s:4814 */
UWORD cols[12] = {                      /* ted.s:4816 + col2 (4819) */
    0x357, 0x469, 0x58b, 0x59d, 0x6af,
    0x59d, 0x58c, 0x47a, 0x469, 0x357,
    0x246, 0
};
UWORD copok;                            /* ted.s:4821 */
ULONG pixrows22;                        /* ted.s:5017 */

/* asm a5 = $DFF000 between getblitter and putblitter */
#define hw (*(volatile struct Custom *)0xDFF000)

/* ----------------------------------------------- vertical scrolling */

/* ted.s:4407 scup2 - one plane a0: rows up (if the height is not 0),
   then the last row filled with 0, or $FFFF if that screen row is in the
   block (whole width, survey-05 I.9) */
static void scup2(UBYTE *a0, ULONG d5, UWORD d4, ULONG d3, UWORD d2)
{
    UBYTE *a1 = a0;

    if (d4 >= 64) {                             /* cmp #64,d4 / bcs .skip0 */
        a0 += d5;
        hw.bltdpt = a0;
        a0 += (WORD)fontmul;
        hw.bltapt = a0;
        hw.bltsize = d4;
        WaitBlit();
    }
    /* .skip0 */
    a1 += d3;
    hw.bltdpt = a1;
    hw.bltadat = 0;
    if (bboty != (UWORD)-1) {
        UWORD d0 = (UWORD)(curstop + edrows - 1);

        if (d0 >= btopy && d0 <= bboty)
            hw.bltadat = 0xFFFF;
    }
    /* .skip */
    hw.bltcon0 = 0x1F0;
    hw.bltsize = d2;
    WaitBlit();                                 /* jmp waitblit(a6) */
}

/* ted.s:4346 scrollup - screen up from d7 to bottom */
void scrollup(UWORD d7)
{
    WORD d6 = (WORD)(edrows - d7 - 1);          /* rows to scroll */
    ULONG d5, d3;
    UWORD d4, d2, d0;

    getblitter();
    hw.bltcon0 = 0x9F0;
    /* the chmap blits (rows up, last row = $2020): CPU, DIFF */
    chmap_scrollup(d7);
    /* .skip */
    d5 = (ULONG)d7 * fontmul;                   /* mulu */
    d0 = blitmod;
    hw.bltamod = d0;
    hw.bltdmod = d0;
    d4 = (UWORD)((UWORD)((ULONG)(UWORD)d6 * fonthite) << 6);
    d0 = (UWORD)(edcols >> 1);
    d4 |= d0;
    d2 = (UWORD)((UWORD)(fonthite << 6) | d0);
    d3 = (ULONG)(UWORD)(edrows - 1) * fontmul;
    scup2(bitmaps[0], d5, d4, d3, d2);
    hw.bltcon0 = 0x9F0;
    scup2(bitmaps[1], d5, d4, d3, d2);
    putblitter();
}

/* ted.s:4345 scrupall */
void scrupall(void)
{
    scrollup(0);                                /* moveq #0,d7, falls through */
}

/* ted.s:4523 scdown2 - one plane down (descending) */
static void scdown2(UBYTE *a0, ULONG d5, UWORD d6)
{
    a0 += d5;
    hw.bltdpt = a0;
    a0 -= (WORD)fontmul;
    hw.bltapt = a0;
    hw.bltsize = d6;
    WaitBlit();
}

/* ted.s:4434 scrolldown - scroll screen down from d7 to bottom */
void scrolldown(UWORD d7)
{
    WORD d6;
    UWORD size;
    ULONG d5;

    getblitter();
    d6 = (WORD)(edrows - d7 - 1);               /* rows to scroll */
    if (d6 != 0) {
        hw.bltcon0 = 0x9F0;
        hw.bltcon1 = 2;
        /* the chmap blit (descending, one row too many for even d6,
           survey-05 I.4): CPU, DIFF.  chmap_scrolldown also erases chmap
           row d7, which eraseline below does again (same bytes). */
        chmap_scrolldown(d7);
        hw.bltamod = blitmod;
        hw.bltdmod = blitmod;
        size = (UWORD)((UWORD)((ULONG)(UWORD)d6 * fonthite) << 6);
        size |= (UWORD)(edcols >> 1);           /* bltsize */
        d5 = pixrows22 - (ULONG)(UWORD)(blitmod + 2);   /* moveq #0 / move / addq.w #2 / sub.l */
        scdown2(bitmaps[0], d5, size);
        scdown2(bitmaps[1], d5, size);
    }
    eraseline(d7);                              /* falls through into eraseline */
}

/* ted.s:4433 scrdownall */
void scrdownall(void)
{
    scrolldown(0);
}

/* ted.s:4479 eraseline */
void eraseline(UWORD d7)
{
    ULONG d6;
    UWORD d5;

    hw.bltamod = blitmod;
    hw.bltdmod = blitmod;
    hw.bltcon0 = 0x1F0;                         /* move.l #$1f00000,$040 */
    hw.bltcon1 = 0;
    d6 = (ULONG)d7 * fontmul;
    d5 = (UWORD)((UWORD)(fonthite << 6) | (edcols >> 1));  /* bltsize */
    hw.bltadat = 0;
    if (bboty != (UWORD)-1) {
        UWORD d0 = (UWORD)(d7 + curstop);

        if (d0 >= btopy && d0 <= bboty)
            hw.bltadat = 0xFFFF;
    }
    /* .skip0 */
    hw.bltdpt = bitmaps[0] + d6;
    hw.bltsize = d5;
    WaitBlit();
    hw.bltdpt = bitmaps[1] + d6;
    hw.bltsize = d5;
    WaitBlit();
    /* chmap row d7 = $2020: CPU, DIFF */
    chmap_eraseline(d7);
    putblitter();
}

/* ------------------------------------------------------------ screen */

/* ted.s:4619 calcscreen */
void calcscreen(void)
{
    UWORD d0;
    ULONG q;

    GetScreenData(&scdata, 84, WBENCHSCREEN, NULL);
    scdata.Width &= 0xFFF0;                     /* "multiple of 8" (really 16) */
    topskip = 12;
    botskip = 8;
    newwindow.TopEdge = topskip;
    viewmode = scdata.ViewPort.Modes;           /* scdata+44+32 */
    d0 = (UWORD)scdata.Width;
    newscreen.Width = d0;                       /* use width for screen! */
    newwindow.Width = d0;                       /* and window! */
    d0 >>= 3;
    totsccols = d0;
    d0 = (UWORD)(d0 - mousecols - 2);
    edcols = d0;
    d0 = (UWORD)((UWORD)scdata.Height - topskip - botskip);
    q = (UWORD)divu_w(d0, fonthite);            /* divu (quotient word) */
    d0 = (UWORD)(q - 1);                        /* subq.w #1 */
    edrows = d0;
    d0 = (UWORD)((ULONG)d0 * fonthite);         /* mulu (low word) */
    pixrows = d0;
    d0 = (UWORD)(d0 + fonthite + botskip + 1);
    newwindow.Height = d0;
    d0 = (UWORD)(d0 + topskip);
    newscreen.Height = d0;
}

/* ted.s:4661 screenopen2 - error of screenopen ignored (survey-05 I.2) */
void screenopen2(void)
{
    screenopen();
    showvprop();
    showhprop();
    showall();                                  /* bra showall */
}

/* ted.s:4668 screenopen */
BOOL screenopen(void)
{
    struct BitMap *bm;
    struct Process *pr;
    UBYTE *a1;
    UWORD d1, d0;
    ULONG l;

    calcscreen();
    myscreen = OpenScreen((struct NewScreen *)&newscreen);
    if (!myscreen)
        goto sfail;
    winscreen = myscreen;
    reqstruct[3] = (ULONG)myscreen;
    bm = myscreen->RastPort.BitMap;             /* 88(a0) */
    d1 = bm->BytesPerRow;
    bmapmul = d1;
    blitmod = (UWORD)(d1 - edcols);
    l = (ULONG)d1 * pixrows;
    pixrows2 = (UWORD)l;
    pixrows22 = l;
    fontmul = (UWORD)((ULONG)d1 * fonthite);
    l = (ULONG)d1 * topskip;
    a1 = bm->Planes[0];
    combitmap = a1;
    bitmaps[0] = a1 + l;
    a1 = bm->Planes[1] + l;
    bitmaps[1] = a1;
    a1 += (ULONG)(UWORD)(pixrows + botskip + 1) * bmapmul;
    statsloc = a1;

    memalt = 2;
    chmap = AllocMem((ULONG)chcols * edrows, MEMF_CHIP);
    if (!chmap)
        goto cfail;

    newgads.LeftEdge = 0;                       /* clr.l newgads+4 */
    newgads.TopEdge = 0;
    newgads.Width = 10;                         /* move.l #$a0000,newgads+8 */
    newgads.Height = 0;
    newgads2.LeftEdge = 0;
    newgads2.TopEdge = 0;
    newgads2.Width = 0;                         /* move.l #4,newgads2+8 */
    newgads2.Height = 4;
    ngc3[5] = 7;
    ngc4[3] = 7;
    ngc4[5] = 7;
    newgads2.Height = 4;
    d0 = (UWORD)(edcols << 3);
    newgads.LeftEdge = d0;
    newgads2.Width = d0;
    d0 = pixrows;
    newgads.Height = d0;
    newgads2.TopEdge = d0;
    l = (ULONG)edcols << 16;                    /* divu chcols: overflow (edcols
                                                   >= chcols) leaves d0, low word 0 */
    prop2sinfo.HorizBody = (UWORD)divu_w(l, chcols);
    d0 = (UWORD)(newgads.Height - 1);
    ngc1[5] = d0;
    ngc2[5] = d0;
    ngc2[3] = d0;
    newgadbord.LeftEdge = newgads.LeftEdge;
    nextngad.LeftEdge = newgads.LeftEdge;
    newgads.LeftEdge += 3;
    newgads.TopEdge += 2;
    newgads.Height -= 4;
    d0 = (UWORD)(newgads2.Width - 1);
    ngc3[0] = d0;
    ngc4[0] = d0;
    ngc4[2] = d0;
    newgadbord2.TopEdge = newgads2.TopEdge;
    nextngad2.TopEdge = newgads2.TopEdge;
    newgads2.LeftEdge += 3;
    newgads2.TopEdge += 2;
    newgads2.Width -= 6;

    mywindow = OpenWindow(&newwindow);
    if (!mywindow)
        goto wfail;
    mywindow2 = mywindow;
    SetMenuStrip(mywindow, &menu1);
    myrast = mywindow->RPort;
    DrawBorder(myrast, &newgadbord, 0, 0);
    DrawBorder(myrast, &newgadbord2, 0, 0);
    colors();
    pstats(statstext, statsloc);
    mousealt = (UWORD)-1;
    pr = (struct Process *)FindTask(NULL);
    oldmywindow = pr->pr_WindowPtr;
    pr->pr_WindowPtr = mywindow;
    pointeron();
    screensopen = (UWORD)-1;
    makecop();
    return TRUE;                                /* d0 = 0, Z=1 */

wfail:
    freechmap();
cfail:
    {
        /* BUG(orig) (survey-05 I.1, decision O2: reproduced): a6 is
           ExecBase here (AllocMem / freechmap), so `jsr closescreen(a6)`
           calls exec's LVO -66 (Exception) with a0 = myscreen instead of
           CloseScreen. */
        struct m68kregs r;

        memset(&r, 0, sizeof(r));
        r.a[0] = (ULONG)myscreen;
        r.a[6] = (ULONG)SysBase;
        callregs((APTR)((UBYTE *)SysBase - 66), &r);
    }
sfail:
    return FALSE;                               /* d0 = -1 */
}

/* ted.s:4823 makecop - make fancy copper list (colour gradient of pen 2
   in the title bar).  BUG(orig) survey-05 I.3: AllocMem unchecked, the
   end CWait has no CBump. */
void makecop(void)
{
    struct UCopList *d7;
    UWORD d0;
    LONG d6;

    if (copok)
        return;
    /* .doit: col2 = RGB4 of colour 2 (byte operations as in the asm) */
    d0 = (UWORD)(palit[6] << 8);
    d0 = (UWORD)((d0 & 0xFF00) | (UBYTE)palit[7]);
    d0 = (UWORD)((d0 & 0xFF00) | (UBYTE)(((UBYTE)d0) << 4));
    d0 = (UWORD)(d0 | (UBYTE)palit[8]);
    col2 = d0;
    d7 = AllocMem(12, MEMF_PUBLIC | MEMF_CLEAR);
    coplist = d7;
    UCopperListInit(d7, 24);
    for (d6 = 0; d6 < 12; d6++) {
        CWait(d7, d6, 0);
        CBump(d7);
        CMove(d7, (APTR)0x184, cols[d6]);       /* COLOR02 */
        CBump(d7);
    }
    CWait(d7, 10000, 255);
    d7->Next = myscreen->ViewPort.UCopIns;      /* 44+20 */
    myscreen->ViewPort.UCopIns = d7;
    RemakeDisplay();
}

/* ted.s:4880 pstats */
void pstats(const char *a3, UBYTE *a1)
{
    UBYTE d0;

    while ((d0 = (UBYTE)*a3++) != 0) {
        qprint(d0, a1);
        a1++;
    }
}

/* ted.s:4888 freechmap */
void freechmap(void)
{
    ULONG d0 = (ULONG)chcols * edrows;

    memalt = (UWORD)d0;
    FreeMem(chmap, d0);
}

/* ted.s:4895 screenclose.  The UCopList goes with CloseScreen; `coplist`
   stays dangling (unused). */
void screenclose(void)
{
    struct Process *pr;

    screensopen = 0;
    pr = (struct Process *)FindTask(NULL);
    pr->pr_WindowPtr = oldmywindow;
    freechmap();
    flushmw();
    ClearPointer(mywindow);
    ClearMenuStrip(mywindow);
    CloseWindow(mywindow);
    CloseScreen(myscreen);
}

/* ted.s:4913 mwait - dead code (CIA-A PRA bit 7 = fire button port 2) */
void mwait(void)
{
    while (*(volatile UBYTE *)0xBFE001 & 0x80)
        ;
}

/* ted.s:4917 halfcolors.  (moveq #0,d2 twice instead of d3: harmless,
   survey-05 I.7) */
void halfcolors(void)
{
    struct ViewPort *a5 = &myscreen->ViewPort;
    const UWORD *a4 = palit;
    UWORD d7;

    for (d7 = 0; d7 < 4; d7++) {
        UWORD r = (UWORD)(a4[0] >> 1), g = (UWORD)(a4[1] >> 1), b = (UWORD)(a4[2] >> 1);

        a4 += 3;
        SetRGB4(a5, d7, r, g, b);
    }
}

/* ted.s:4940 colors: 0-3 from palit, 17-19 (pointer) from mpalit */
void colors(void)
{
    struct ViewPort *a5 = &myscreen->ViewPort;
    const UWORD *a4 = palit;
    UWORD d7 = 0;
    WORD d6;

    for (d6 = 6; d6 >= 0; d6--) {               /* dbf d6 */
        UWORD r = a4[0], g = a4[1], b = a4[2];

        a4 += 3;
        SetRGB4(a5, d7, r, g, b);
        d7++;
        if (d7 == 4) {
            a4 = mpalit;
            d7 = 17;
        }
    }
}

/* ------------------------------------------------ blitter / cls */

/* ted.s:4964 getblitter */
void getblitter(void)
{
    OwnBlitter();
    WaitBlit();
    hw.bltafwm = 0xFFFF;                        /* move.l #-1,$044(a5) */
    hw.bltalwm = 0xFFFF;
    hw.bltcon1 = 0;
}

/* ted.s:4973 putblitter */
void putblitter(void)
{
    WaitBlit();
    DisownBlitter();
}

/* ted.s:5006 cls2 - starts the clear blit of one plane (no wait) */
static void cls2(UBYTE *a0)
{
    hw.bltdpt = a0;
    hw.bltdmod = blitmod;
    hw.bltsize = (UWORD)((UWORD)(pixrows << 6) | (edcols >> 1));
}

/* ted.s:4986 cls */
void cls(void)
{
    getblitter();
    hw.bltcon0 = 0x1F0;
    /* chmap = $2020 (blitter fill in the asm): CPU, DIFF */
    chmap_cls();
    hw.bltadat = 0;
    cls2(bitmaps[0]);
    WaitBlit();
    cls2(bitmaps[1]);
    putblitter();
}

/* ted.s:5112 hscmods */
static void hscmods(void)
{
    UWORD d0 = (UWORD)(blitmod + 2);

    hw.bltamod = d0;
    hw.bltdmod = d0;
    hw.bltsize = (UWORD)((UWORD)(pixrows << 6) | ((UWORD)(edcols - 2) >> 1));
}

/* ted.s:5064 scrite2 */
static void scrite2(UBYTE *a0)
{
    hw.bltdpt = a0;
    hw.bltapt = a0 - 2;
    hscmods();
}

/* ted.s:5108 scleft2 */
static void scleft2(UBYTE *a0)
{
    hw.bltdpt = a0;
    hw.bltapt = a0 + 2;
    hscmods();                                  /* falls through into hscmods */
}

/* ted.s:5019 scrollrite - scroll the screen rite a word, then redraw the
   two left columns from chmap (cursoff already adjusted by the caller) */
void scrollrite(void)
{
    const UBYTE *a5;
    ULONG d7;
    UWORD d2;

    getblitter();
    hw.bltcon0 = 0x9F0;                         /* move.l #$9f00002,$040 */
    hw.bltcon1 = 2;
    d7 = pixrows22 - (ULONG)(UWORD)(blitmod + 2);   /* moveq #0 / move / addq.w #2 / sub.l */
    scrite2(bitmaps[0] + d7);
    WaitBlit();
    scrite2(bitmaps[1] + d7);
    putblitter();

    storepos();
    cursx = 0;                                  /* clr.l cursx */
    cursy = 0;
    scloc = 0;
    chloc = 0;
    a5 = chmap + cursoff;
    d2 = (UWORD)(edrows - 1);
    do {                                        /* .loop ... dbf d2 */
        print3(*a5++);
        cursx = 1;
        print3(*a5);
        cursx = 0;
        scloc += fontmul;
        a5 += (WORD)chcols;
        a5--;
        chloc = (UWORD)(chloc + chcols);
        cursy++;
    } while (d2-- != 0);
    color = normcol;
    restorepos();
}

/* ted.s:5069 scrollleft - scroll the screen a word left, redraw the two
   right columns */
void scrollleft(void)
{
    const UBYTE *a5;
    UWORD d0, d2;

    getblitter();
    hw.bltcon0 = 0x9F0;
    scleft2(bitmaps[0]);
    WaitBlit();
    scleft2(bitmaps[1]);
    putblitter();

    storepos();
    d0 = (UWORD)(edcols - 2);
    cursx = d0;
    cursy = 0;
    scloc = 0;
    chloc = 0;
    a5 = chmap + cursoff + (WORD)d0;
    d2 = (UWORD)(edrows - 1);
    do {
        print3(a5[0]);
        cursx++;
        print3(a5[1]);
        cursx--;
        scloc += fontmul;
        a5 += (WORD)chcols;
        chloc = (UWORD)(chloc + chcols);
        cursy++;
    } while (d2-- != 0);
    color = normcol;
    restorepos();
}

/* ------------------------------------------------------ text output */

/* ted.s:5125 print3 */
void print3(UBYTE d0)
{
    color = normcol;
    if (d0 & 0x80) {
        d0 &= 0x7F;
        color = tokcol;
    }
    printa0(d0, print4);
}

/* ted.s:5132 print - print d0 at cursx,cursy, in color (1,2,3) */
void print(UBYTE d0)
{
    printa0(d0, print2);                        /* falls through into printa0 */
}

/* ted.s:5135 printa0: inverse from the block bounds (rows unsigned,
   columns signed), then the computed call jsr (a0) */
void printa0(UBYTE d0, void (*a0)(UBYTE))
{
    UWORD d2;
    WORD d3;

    if (bboty == (UWORD)-1)
        goto skip0;
    d2 = (UWORD)(curstop + cursy);
    if (d2 < btopy)
        goto skip0;
    if (d2 == btopy) {
        d3 = (WORD)(cursoff + cursx);
        if (d3 < btopx)
            goto skip0;
        if (d2 < bboty)
            goto skipdo;
        if (d3 > bbotx)
            goto skip0;
        goto skipdo;
    }
    /* .skip01 */
    if (d2 > bboty)
        goto skip0;
    if (d2 != bboty)
        goto skipdo;
    if ((WORD)(cursoff + cursx) <= bbotx)
        goto skipdo;
skip0:
    inverse = 0;
    goto prinv;
skipdo:
    inverse = (UWORD)-1;
prinv:
    /* ted.s:5163 prinv */
    a0(d0);
}

/* ted.s:5167 print2 */
void print2(UBYTE d0)
{
    UBYTE d2 = d0;

    if (color != normcol)
        d2 |= 128;
    chmap[(WORD)chloc + cursoff + (WORD)cursx] = d2;
    print4(d0);                                 /* falls through into print4 */
}

/* ted.s:5178 print4 - glyph into both planes (colour bits, inverse).
   d0 >= 128 reads behind the font (survey-05 I.11). */
void print4(UBYTE d0)
{
    UBYTE *a0, *a1, d1, d2, d4;
    const UBYTE *a2;
    UWORD d3;

    calcphys(&a0, &a1);
    a2 = fontcalc(d0);
    d1 = (color & 1) ? 0xFF : 0;
    d2 = (color & 2) ? 0xFF : 0;
    d4 = (UBYTE)inverse;
    d3 = fonthite2;
    do {                                        /* .loop ... dbf d3 */
        *a0 = (UBYTE)((*a2 & d1) ^ d4);
        *a1 = (UBYTE)((*a2 & d2) ^ d4);
        a2 += 16;
        a0 += (WORD)bmapmul;
        a1 += (WORD)bmapmul;
    } while (d3-- != 0);
}

/* ted.s:5205 qprint - print d0 to a0, 1 bitplane only, quick */
void qprint(UBYTE d0, UBYTE *a0)
{
    const UBYTE *a2 = fontcalc(d0);
    UWORD d1;

    d1 = fonthite2;
    do {
        *a0 = *a2;
        a2 += 16;
        a0 += (WORD)bmapmul;
    } while (d1-- != 0);
}

/* ted.s:5244 calcphys - calculate physical coords of cursor */
void calcphys(UBYTE **a0, UBYTE **a1)
{
    *a0 = bitmaps[0] + scloc + (WORD)cursx;
    *a1 = bitmaps[1] + scloc + (WORD)cursx;
}
