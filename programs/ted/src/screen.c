/*
 * screen.c - chunk 05, Amiga part (ted.s 4198-5252): see screen.h.
 *
 * Blitter scrolling of the text area, screen/window open and close, user
 * copper list, colours, cls, horizontal scrolling and the own text output.
 * The chmap halves of the blits are chmap_* in the core (tokit.c, DIFF:
 * CPU instead of blitter, docs/ted/deviations.md); the bitplane blits stay
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

/* ted.s:4407 scup2 - one plane (asm a0): rows up (if the height is not
   0), then the last row filled with 0, or $FFFF if that screen row is in
   the block (whole width, survey-05 I.9) */
static void scup2(UBYTE *plane, ULONG rowoff, UWORD size, ULONG lastoff, UWORD rowsize)
{
    if (size >= 64) {                           /* cmp #64,d4 / bcs .skip0 */
        UBYTE *dst = plane + rowoff;

        hw.bltdpt = dst;
        hw.bltapt = dst + (WORD)fontmul;
        hw.bltsize = size;
        WaitBlit();
    }
    /* .skip0 */
    hw.bltdpt = plane + lastoff;                /* a1 */
    hw.bltadat = 0;
    if (bboty != (UWORD)-1) {
        UWORD lastline = (UWORD)(curstop + edrows - 1);

        if (lastline >= btopy && lastline <= bboty)
            hw.bltadat = 0xFFFF;
    }
    /* .skip */
    hw.bltcon0 = 0x1F0;
    hw.bltsize = rowsize;
    WaitBlit();                                 /* jmp waitblit(a6) */
}

/* ted.s:4346 scrollup - screen up from d7 to bottom */
void scrollup(UWORD row)
{
    WORD rows = (WORD)(edrows - row - 1);       /* rows to scroll */
    ULONG rowoff, lastoff;
    UWORD size, rowsize, words;

    getblitter();
    hw.bltcon0 = 0x9F0;
    /* the chmap blits (rows up, last row = $2020): CPU, DIFF */
    chmap_scrollup(row);
    /* .skip */
    rowoff = (ULONG)row * fontmul;              /* mulu */
    hw.bltamod = blitmod;
    hw.bltdmod = blitmod;
    size = (UWORD)((UWORD)((ULONG)(UWORD)rows * fonthite) << 6);
    words = (UWORD)(edcols >> 1);               /* blit width in words */
    size |= words;
    rowsize = (UWORD)((UWORD)(fonthite << 6) | words);
    lastoff = (ULONG)(UWORD)(edrows - 1) * fontmul;
    scup2(bitmaps[0], rowoff, size, lastoff, rowsize);
    hw.bltcon0 = 0x9F0;
    scup2(bitmaps[1], rowoff, size, lastoff, rowsize);
    putblitter();
}

/* ted.s:4345 scrupall */
void scrupall(void)
{
    scrollup(0);                                /* moveq #0,d7, falls through */
}

/* ted.s:4523 scdown2 - one plane (asm a0) down (descending) */
static void scdown2(UBYTE *plane, ULONG off, UWORD size)
{
    UBYTE *dst = plane + off;

    hw.bltdpt = dst;
    hw.bltapt = dst - (WORD)fontmul;
    hw.bltsize = size;
    WaitBlit();
}

/* ted.s:4434 scrolldown - scroll screen down from d7 to bottom */
void scrolldown(UWORD row)
{
    WORD rows;
    UWORD size;
    ULONG off;

    getblitter();
    rows = (WORD)(edrows - row - 1);            /* rows to scroll */
    if (rows != 0) {
        hw.bltcon0 = 0x9F0;
        hw.bltcon1 = 2;
        /* the chmap blit (descending, one row too many for even rows,
           survey-05 I.4): CPU, DIFF.  chmap_scrolldown also erases chmap
           row `row`, which eraseline below does again (same bytes). */
        chmap_scrolldown(row);
        hw.bltamod = blitmod;
        hw.bltdmod = blitmod;
        size = (UWORD)((UWORD)((ULONG)(UWORD)rows * fonthite) << 6);
        size |= (UWORD)(edcols >> 1);           /* bltsize */
        off = pixrows22 - (ULONG)(UWORD)(blitmod + 2);  /* moveq #0 / move / addq.w #2 / sub.l */
        scdown2(bitmaps[0], off, size);
        scdown2(bitmaps[1], off, size);
    }
    eraseline(row);                             /* falls through into eraseline */
}

/* ted.s:4433 scrdownall */
void scrdownall(void)
{
    scrolldown(0);
}

/* ted.s:4479 eraseline */
void eraseline(UWORD row)
{
    ULONG rowoff;
    UWORD size;

    hw.bltamod = blitmod;
    hw.bltdmod = blitmod;
    hw.bltcon0 = 0x1F0;                         /* move.l #$1f00000,$040 */
    hw.bltcon1 = 0;
    rowoff = (ULONG)row * fontmul;
    size = (UWORD)((UWORD)(fonthite << 6) | (edcols >> 1)); /* bltsize */
    hw.bltadat = 0;
    if (bboty != (UWORD)-1) {
        UWORD line = (UWORD)(row + curstop);

        if (line >= btopy && line <= bboty)
            hw.bltadat = 0xFFFF;
    }
    /* .skip0 */
    hw.bltdpt = bitmaps[0] + rowoff;
    hw.bltsize = size;
    WaitBlit();
    hw.bltdpt = bitmaps[1] + rowoff;
    hw.bltsize = size;
    WaitBlit();
    /* chmap row d7 = $2020: CPU, DIFF */
    chmap_eraseline(row);
    putblitter();
}

/* ------------------------------------------------------------ screen */

/* ted.s:4619 calcscreen */
void calcscreen(void)
{
    UWORD width, columns, height, winheight;
    ULONG q;

    GetScreenData(&scdata, 84, WBENCHSCREEN, NULL);
    scdata.Width &= 0xFFF0;                     /* "multiple of 8" (really 16) */
    topskip = 12;
    botskip = 8;
    newwindow.TopEdge = topskip;
    viewmode = scdata.ViewPort.Modes;           /* scdata+44+32 */
    width = (UWORD)scdata.Width;
    newscreen.Width = width;                    /* use width for screen! */
    newwindow.Width = width;                    /* and window! */
    columns = (UWORD)(width >> 3);
    totsccols = columns;
    edcols = (UWORD)(columns - mousecols - 2);
    height = (UWORD)((UWORD)scdata.Height - topskip - botskip);
    q = (UWORD)divu_w(height, fonthite);        /* divu (quotient word) */
    edrows = (UWORD)(q - 1);                    /* subq.w #1 */
    pixrows = (UWORD)((ULONG)edrows * fonthite); /* mulu (low word) */
    /* BUG(orig) (#351): subtracted were topskip + botskip (20), added back are
       fonthite + botskip + 1 + topskip (edrows = quotient - 1): if
       Height - 20 is a multiple of fonthite, the screen is one row taller
       than the Workbench.  OS 3.2 opens it with the Workbench height, the
       main window (topskip + newwindow.Height) no longer fits, OpenWindow
       fails and screenopen's error path crashes (cfail, BUG(orig) #351, #382,
       survey-05 I.1).  Seen with fonthite 6 on a 512 row Workbench: 513
       rows (TED A/B 23-font6, original and TED-C, confirmed in Amiberry
       28.10.2026). */
    winheight = (UWORD)(pixrows + fonthite + botskip + 1);
    newwindow.Height = winheight;
    newscreen.Height = (UWORD)(winheight + topskip);
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
    UBYTE *plane;
    UWORD bpr, textwidth, bottom, right;
    ULONG size, topoff;

    calcscreen();
    myscreen = OpenScreen((struct NewScreen *)&newscreen);
    if (!myscreen)
        goto sfail;
    winscreen = myscreen;
    reqstruct[3] = (ULONG)myscreen;
    bm = myscreen->RastPort.BitMap;             /* 88(a0) */
    bpr = bm->BytesPerRow;
    bmapmul = bpr;
    blitmod = (UWORD)(bpr - edcols);
    size = (ULONG)bpr * pixrows;
    pixrows2 = (UWORD)size;
    pixrows22 = size;
    fontmul = (UWORD)((ULONG)bpr * fonthite);
    topoff = (ULONG)bpr * topskip;
    plane = bm->Planes[0];
    combitmap = plane;
    bitmaps[0] = plane + topoff;
    plane = bm->Planes[1] + topoff;
    bitmaps[1] = plane;
    statsloc = plane + (ULONG)(UWORD)(pixrows + botskip + 1) * bmapmul;

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
    textwidth = (UWORD)(edcols << 3);
    newgads.LeftEdge = textwidth;
    newgads2.Width = textwidth;
    newgads.Height = pixrows;
    newgads2.TopEdge = pixrows;
    /* divu chcols: overflow (edcols >= chcols) leaves d0, low word 0 */
    prop2sinfo.HorizBody = (UWORD)divu_w((ULONG)edcols << 16, chcols);
    bottom = (UWORD)(newgads.Height - 1);
    ngc1[5] = bottom;
    ngc2[5] = bottom;
    ngc2[3] = bottom;
    newgadbord.LeftEdge = newgads.LeftEdge;
    nextngad.LeftEdge = newgads.LeftEdge;
    newgads.LeftEdge += 3;
    newgads.TopEdge += 2;
    newgads.Height -= 4;
    right = (UWORD)(newgads2.Width - 1);
    ngc3[0] = right;
    ngc4[0] = right;
    ngc4[2] = right;
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
        /* BUG(orig) (#382) (survey-05 I.1, decision O2: reproduced): a6 is
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
   in the title bar).  Fixed (#366): without memory for the UCopList the
   screen has no gradient (the original passed the unchecked AllocMem to
   UCopperListInit, survey-05 I.3).  The end CWait has no CBump (harmless,
   MrgCop ends the list). */
void makecop(void)
{
    struct UCopList *ucl;
    UWORD rgb;
    LONG line;

    if (copok)
        return;
    /* .doit: col2 = RGB4 of colour 2 (byte operations as in the asm) */
    rgb = (UWORD)(palit[6] << 8);
    rgb = (UWORD)((rgb & 0xFF00) | (UBYTE)palit[7]);
    rgb = (UWORD)((rgb & 0xFF00) | (UBYTE)(((UBYTE)rgb) << 4));
    rgb = (UWORD)(rgb | (UBYTE)palit[8]);
    col2 = rgb;
    ucl = AllocMem(12, MEMF_PUBLIC | MEMF_CLEAR);
    coplist = ucl;
    if (!ucl)
        return;
    UCopperListInit(ucl, 24);
    for (line = 0; line < 12; line++) {
        CWait(ucl, line, 0);
        CBump(ucl);
        CMove(ucl, (APTR)0x184, cols[line]);    /* COLOR02 */
        CBump(ucl);
    }
    CWait(ucl, 10000, 255);
    ucl->Next = myscreen->ViewPort.UCopIns;     /* 44+20 */
    myscreen->ViewPort.UCopIns = ucl;
    RemakeDisplay();
}

/* ted.s:4880 pstats */
void pstats(const char *text, UBYTE *at)
{
    UBYTE c;

    while ((c = (UBYTE)*text++) != 0) {
        qprint(c, at);
        at++;
    }
}

/* ted.s:4888 freechmap */
void freechmap(void)
{
    ULONG size = (ULONG)chcols * edrows;        /* d0 */

    memalt = (UWORD)size;
    FreeMem(chmap, size);
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
    struct ViewPort *vp = &myscreen->ViewPort;
    const UWORD *rgb = palit;
    UWORD pen;

    for (pen = 0; pen < 4; pen++) {
        UWORD r = (UWORD)(rgb[0] >> 1), g = (UWORD)(rgb[1] >> 1), b = (UWORD)(rgb[2] >> 1);

        rgb += 3;
        SetRGB4(vp, pen, r, g, b);
    }
}

/* ted.s:4940 colors: 0-3 from palit, 17-19 (pointer) from mpalit */
void colors(void)
{
    struct ViewPort *vp = &myscreen->ViewPort;
    const UWORD *rgb = palit;
    UWORD pen = 0;
    WORD n;

    for (n = 6; n >= 0; n--) {                  /* dbf d6 */
        UWORD r = rgb[0], g = rgb[1], b = rgb[2];

        rgb += 3;
        SetRGB4(vp, pen, r, g, b);
        pen++;
        if (pen == 4) {
            rgb = mpalit;
            pen = 17;
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
static void cls2(UBYTE *plane)
{
    hw.bltdpt = plane;
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
    UWORD mod = (UWORD)(blitmod + 2);

    hw.bltamod = mod;
    hw.bltdmod = mod;
    hw.bltsize = (UWORD)((UWORD)(pixrows << 6) | ((UWORD)(edcols - 2) >> 1));
}

/* ted.s:5064 scrite2 */
static void scrite2(UBYTE *plane)
{
    hw.bltdpt = plane;
    hw.bltapt = plane - 2;
    hscmods();
}

/* ted.s:5108 scleft2 */
static void scleft2(UBYTE *plane)
{
    hw.bltdpt = plane;
    hw.bltapt = plane + 2;
    hscmods();                                  /* falls through into hscmods */
}

/* ted.s:5019 scrollrite - scroll the screen rite a word, then redraw the
   two left columns from chmap (cursoff already adjusted by the caller) */
void scrollrite(void)
{
    const UBYTE *src;
    ULONG endoff;
    UWORD n;

    getblitter();
    hw.bltcon0 = 0x9F0;                         /* move.l #$9f00002,$040 */
    hw.bltcon1 = 2;
    endoff = pixrows22 - (ULONG)(UWORD)(blitmod + 2); /* moveq #0 / move / addq.w #2 / sub.l */
    scrite2(bitmaps[0] + endoff);
    WaitBlit();
    scrite2(bitmaps[1] + endoff);
    putblitter();

    storepos();
    cursx = 0;                                  /* clr.l cursx */
    cursy = 0;
    scloc = 0;
    chloc = 0;
    src = chmap + cursoff;
    n = (UWORD)(edrows - 1);
    do {                                        /* .loop ... dbf d2 */
        print3(*src++);
        cursx = 1;
        print3(*src);
        cursx = 0;
        scloc += fontmul;
        src += (WORD)chcols;
        src--;
        chloc = (UWORD)(chloc + chcols);
        cursy++;
    } while (n-- != 0);
    color = normcol;
    restorepos();
}

/* ted.s:5069 scrollleft - scroll the screen a word left, redraw the two
   right columns */
void scrollleft(void)
{
    const UBYTE *src;
    UWORD col, n;

    getblitter();
    hw.bltcon0 = 0x9F0;
    scleft2(bitmaps[0]);
    WaitBlit();
    scleft2(bitmaps[1]);
    putblitter();

    storepos();
    col = (UWORD)(edcols - 2);
    cursx = col;
    cursy = 0;
    scloc = 0;
    chloc = 0;
    src = chmap + cursoff + (WORD)col;
    n = (UWORD)(edrows - 1);
    do {
        print3(src[0]);
        cursx++;
        print3(src[1]);
        cursx--;
        scloc += fontmul;
        src += (WORD)chcols;
        chloc = (UWORD)(chloc + chcols);
        cursy++;
    } while (n-- != 0);
    color = normcol;
    restorepos();
}

/* ------------------------------------------------------ text output */

/* ted.s:5125 print3 */
void print3(UBYTE c)
{
    color = normcol;
    if (c & 0x80) {
        c &= 0x7F;
        color = tokcol;
    }
    printa0(c, print4);
}

/* ted.s:5132 print - print d0 at cursx,cursy, in color (1,2,3) */
void print(UBYTE c)
{
    printa0(c, print2);                         /* falls through into printa0 */
}

/* ted.s:5135 printa0: inverse from the block bounds (rows unsigned,
   columns signed), then the computed call jsr (a0) */
void printa0(UBYTE c, void (*out)(UBYTE))
{
    UWORD line;
    WORD col;

    if (bboty == (UWORD)-1)
        goto skip0;
    line = (UWORD)(curstop + cursy);
    if (line < btopy)
        goto skip0;
    if (line == btopy) {
        col = (WORD)(cursoff + cursx);
        if (col < btopx)
            goto skip0;
        if (line < bboty)
            goto skipdo;
        if (col > bbotx)
            goto skip0;
        goto skipdo;
    }
    /* .skip01 */
    if (line > bboty)
        goto skip0;
    if (line != bboty)
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
    out(c);
}

/* ted.s:5167 print2 */
void print2(UBYTE c)
{
    UBYTE mapped = c;

    if (color != normcol)
        mapped |= 128;
    chmap[(WORD)chloc + cursoff + (WORD)cursx] = mapped;
    print4(c);                                  /* falls through into print4 */
}

/* ted.s:5178 print4 - glyph into both planes (colour bits, inverse).
   d0 >= 128 reads behind the font (survey-05 I.11). */
void print4(UBYTE c)
{
    UBYTE *p0, *p1, mask0, mask1, inv;
    const UBYTE *glyph;
    UWORD n;

    calcphys(&p0, &p1);
    glyph = fontcalc(c);
    mask0 = (color & 1) ? 0xFF : 0;
    mask1 = (color & 2) ? 0xFF : 0;
    inv = (UBYTE)inverse;
    n = fonthite2;
    do {                                        /* .loop ... dbf d3 */
        *p0 = (UBYTE)((*glyph & mask0) ^ inv);
        *p1 = (UBYTE)((*glyph & mask1) ^ inv);
        glyph += 16;
        p0 += (WORD)bmapmul;
        p1 += (WORD)bmapmul;
    } while (n-- != 0);
}

/* ted.s:5205 qprint - print d0 to a0, 1 bitplane only, quick */
void qprint(UBYTE c, UBYTE *at)
{
    const UBYTE *glyph = fontcalc(c);
    UWORD n;

    n = fonthite2;
    do {
        *at = *glyph;
        glyph += 16;
        at += (WORD)bmapmul;
    } while (n-- != 0);
}

/* ted.s:5244 calcphys - calculate physical coords of cursor */
void calcphys(UBYTE **p0, UBYTE **p1)
{
    *p0 = bitmaps[0] + scloc + (WORD)cursx;
    *p1 = bitmaps[1] + scloc + (WORD)cursx;
}

/* Not in ted.s: the progress call of blitz2-c (comdata+102, #980).  The
   original compiler XORs the byte `column` of plane 0 in pixel row `bar`
   and in row 9 - bar itself (main bar 0: rows 0 and 9; include levels 1
   to 3: the rows inside); TED-C does the same through the screen's
   RastPort, and nothing while its screen is closed.  A cell outside the
   bitmap is ignored. */
void progress(UWORD bar, UWORD column)
{
    struct RastPort *rp;
    struct BitMap *bm;
    UBYTE mode, mask;
    WORD x;

    if (!screensopen || !myscreen || bar > 3)
        return;
    rp = &myscreen->RastPort;
    bm = rp->BitMap;
    if (column >= bm->BytesPerRow || bm->Rows < 10)
        return;
    mode = rp->DrawMode;
    mask = rp->Mask;
    SetDrMd(rp, COMPLEMENT);
    SetWrMsk(rp, 1);
    x = (WORD)(column * 8);
    RectFill(rp, x, bar, x + 7, bar);
    RectFill(rp, x, 9 - bar, x + 7, 9 - bar);
    SetDrMd(rp, mode);
    SetWrMsk(rp, mask);
}
