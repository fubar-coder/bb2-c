/*
 * mouse.c - chunk 03, Amiga part (ted.s 1981-3062): see mouse.h.
 *
 * Mouse selection (mousedown, gomouse), the goto requester, Workbench
 * open/close, the console window for running programs (openio/closeio,
 * comdata+60/64), the gadget services for blitz2 (actgad, refgads), the
 * help key display, the COMPILER menu (getstuff), the label column
 * (showmice), printmem, outmem/extalloc/doalert, doclean.
 *
 * The asm keeps its state in registers across calls; the C locals are
 * named after what they hold (the register in the comments).  Where a
 * called routine changes a register that the asm uses afterwards, C passes
 * it explicitly (getinput2 writes d6 = lastrow for gadget/menu messages,
 * orderem returns the ordered d0-d3).
 */
#include <string.h>
#include <setjmp.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include "ted.h"

UWORD newctop;                          /* ted.s:2318 */
UWORD newcoff;                          /* ted.s:2319 */

/* ------------------------------------------- ted.s:2324-2407 goto requester */

static WORD gotoreqla1[6] = { 175, 4, 120, 4, 120, 15 };       /* ted.s:2337 */
static WORD gotoreqla3[6] = { 175, 5, 175, 15, 121, 15 };      /* ted.s:2341 */
static WORD gotoreqla4[6] = { 183, 0, 0, 0, 0, 39 };           /* ted.s:2345 */
static WORD gotoreqla6[6] = { 183, 1, 183, 39, 1, 39 };        /* ted.s:2349 */
static WORD gotoreqla9[6] = { 39, 0, 0, 0, 0, 11 };            /* ted.s:2353 */
static WORD gotoreqla11[6] = { 39, 1, 39, 11, 1, 11 };         /* ted.s:2357 */
static WORD gotoreqla13[6] = { 71, 0, 0, 0, 0, 11 };           /* ted.s:2361 */
static WORD gotoreqla15[6] = { 71, 1, 71, 11, 1, 11 };         /* ted.s:2365 */

/* the asm interleaves Borders and coordinate blocks: gotoreqla2/la5/la10/
   la14 are Borders themselves */
static struct Border gotoreqla5 =                               /* ted.s:2346 */
    { 0, 0, 2, 1, 1, 3, gotoreqla6, NULL };
static struct Border gotoreqbo2 =                               /* ted.s:2342 */
    { 0, 0, 1, 2, 1, 3, gotoreqla4, &gotoreqla5 };
static struct Border gotoreqla2 =                               /* ted.s:2338 */
    { 0, 0, 1, 2, 1, 3, gotoreqla3, &gotoreqbo2 };
static struct Border gotoreqbo1 =                               /* ted.s:2334 */
    { 0, 0, 2, 1, 1, 3, gotoreqla1, &gotoreqla2 };
static struct Border gotoreqla10 =                              /* ted.s:2354 */
    { 0, 0, 2, 1, 1, 3, gotoreqla11, NULL };
static struct Border gotoreqbo3 =                               /* ted.s:2350 */
    { 0, 0, 1, 2, 1, 3, gotoreqla9, &gotoreqla10 };
static struct Border gotoreqla14 =                              /* ted.s:2362 */
    { 0, 0, 2, 1, 1, 3, gotoreqla15, NULL };
static struct Border gotoreqbo4 =                               /* ted.s:2358 */
    { 0, 0, 1, 2, 1, 3, gotoreqla13, &gotoreqla14 };

static struct IntuiText gotoreqin1 =                            /* ted.s:2367 */
    { 1, 0, 1, 12, 6, NULL, (UBYTE *)"LINE TO GOTO:", NULL };   /* la7 */
static struct IntuiText gotoreqin2 =                            /* ted.s:2373 */
    { 1, 0, 1, 4, 2, NULL, (UBYTE *)" OK ", NULL };             /* la12 */
static struct IntuiText gotoreqin3 =                            /* ted.s:2379 */
    { 1, 0, 1, 4, 2, NULL, (UBYTE *)" CANCEL ", NULL };         /* la16 */

UBYTE gotoreqla8[6];                    /* ted.s:2395 string buffer */

struct StringInfo gotoreqsg1 = {        /* ted.s:2391 */
    gotoreqla8, undobuffer,             /* Buffer, UndoBuffer */
    0, 6, 0,                            /* BufferPos, MaxChars, DispPos */
    0, 0, 0, 0, 0, NULL,                /* UndoPos .. CTop, Extension */
    0, NULL                             /* gotoint (2394): LongInt, AltKeyMap */
};
struct Gadget gotoreqga3 = {            /* ted.s:2402 */
    NULL, 104, 24, 72, 12, 0, 5, 1,
    &gotoreqbo4, NULL, &gotoreqin3, 0, NULL, 3, NULL
};
struct Gadget gotoreqga2 = {            /* ted.s:2397 */
    &gotoreqga3, 8, 24, 40, 12, 0, 5, 1,
    &gotoreqbo3, NULL, &gotoreqin2, 0, NULL, 2, NULL
};
struct Gadget gotoreqga1 = {            /* ted.s:2386 */
    &gotoreqga2, 124, 6, 48, 8, 0, 2049 + 4, 4,     /* LONGINT|ENDGADGET|RELVERIFY, STRGADGET */
    NULL, NULL, NULL, 0, &gotoreqsg1, 1, NULL
};
struct Requester gotoreq = {            /* ted.s:2324 */
    .LeftEdge = 140, .TopEdge = 58, .Width = 184, .Height = 40,
    .ReqGadget = &gotoreqga1, .ReqBorder = &gotoreqbo1, .ReqText = &gotoreqin1,
};

char ioname[] = "NEWCON:0/0/640/   /Blitz 2 CLI Window";   /* ted.s:2446-2448 */
BPTR oldih, oldoh;                      /* ted.s:2451/2452 */
BPTR myhandle;                          /* ted.s:2453 */
ULONG nullcom;                          /* ted.s:2614 */
UWORD helptoke;                         /* ted.s:2767 */
char helpex[] = "blitz2:help 1234";     /* ted.s:2768/2769 */

/* ------------------------------------------------------------- mouse */

/* ted.s:1981 gomouse */
void gomouse(WORD col, UWORD row)
{
    struct micenode *node;
    struct item *item;
    UWORD d0, n;

    d0 = (UWORD)(edcols + 2);
    if ((UWORD)col < d0)                        /* bcs .skip */
        return;
    if (row >= nummice)                         /* bcc .skip */
        return;
    node = mice;                                /* lea mice,a0 / move.l (a0),a0 */
    while (row--)                               /* dbf d3,.loop */
        node = node->next;
    item = firstitem;
    d0 = 0;
    n = (UWORD)(numlines - 1);
    do {                                        /* .loop2 */
        if (node->line == item) {
            gotolined0(d0);                     /* beq gotolined0 */
            return;
        }
        item = item->nextitem;
        d0++;
    } while (n-- != 0);
    /* .skip */
}

/* ted.s:2001 mousedown - o.k. - now for the biggy - select text with the
   f***ing mouse. */
void mousedown(void)
{
    struct item *item = thisitem, *s_item;
    struct mousexy m;
    struct quad q;
    WORD d0, d1, col, row, lastcol, savedline;
    UWORD lastrow, s_curstop;
    WORD s_btopx, s_bbotx, s_cursoff;
    UWORD s_btopy, s_bboty;
    LONG ev;
    BOOL inside;

    if (!getmousexy(&m))
        return;                                 /* bmi .done */
    col = m.col;
    row = m.row;
    if ((UWORD)col >= edcols) {
        gomouse(col, (UWORD)row);               /* bcc gomouse */
        return;
    }
    d1 = (WORD)(row + curstop);
    if ((UWORD)d1 >= numlines)
        return;
    /* the stack frame: btop/bbot (as longs) */
    s_btopx = btopx; s_btopy = btopy;
    s_bbotx = bbotx; s_bboty = bboty;
    if (bboty != (UWORD)-1) {
        blokdis();                              /* survey-03 I.12: bstx lost */
        showall();
    }
    /* .skipsal */
    d1 = (WORD)(row + curstop);
    bloktopy = d1;
    blokboty = d1;
    col = (WORD)(col + cursoff);
    bloktopx = col;
    blokbotx = col;
    showprops2(col, (UWORD)row);
    lastcol = col;
    lastrow = (UWORD)row;
    d0 = col;
    d1 = row;
    invflag = 0;
    s_curstop = curstop;
    s_cursoff = cursoff;
    s_item = item;
    /* .loop2 */
    invertarea(d0, d1, col, row);
loop:
    /* busy poll (survey-03 I.11); handled0 writes d6 for gadget and menu
       messages - the asm's d6 (end row, lastrow) as well */
    ev = getinput2(&lastrow);
    if ((WORD)ev == -4)
        goto mouseup;
    inside = getmousexy(&m);
    d0 = m.x; d1 = m.y; col = m.col; row = m.row;
    if (inside)
        goto skip0;
    if ((UWORD)d1 < topskip)                    /* bcs .mup */
        goto mup;
    /* mouse drag down */
    row = (WORD)edrows;
    d0 = (WORD)(row + curstop);
    if ((UWORD)d0 >= numlines)
        goto loop;
    row--;
    col = (WORD)chcols;
    d0 = (WORD)(curstop + edrows);
    if ((UWORD)d0 >= numlines)                  /* never (survey-03 I.11) */
        goto skip2;
    scrupall();                                 /* (d5/d6 saved) */
    item = item->nextitem;
    thisitem = item;
    curstop++;
    showbotitem();
    lastrow--;
    /* .ok (unused label) */
    col = (WORD)chcols;
    row = (WORD)(edrows - 1);
    d1 = (WORD)(row + curstop);
    if ((UWORD)d1 > (UWORD)bloktopy)            /* bhi .skip2 */
        goto skip2;
    savedline = d1;                             /* move d1,-(a7) */
    invertarea((WORD)chcols, (WORD)(edrows - 2), lastcol, (WORD)lastrow);
    lastcol = (WORD)chcols;
    lastrow = (UWORD)(edrows - 1);
    row = (WORD)lastrow;
    col = -1;
    d1 = savedline;
    if (d1 != bloktopy)
        goto skip2;
    lastcol = (WORD)(bloktopx + 1);
    goto skip2;

mup:
    /* mouse drag up! - currently FUCKED */
    col = 0;
    row = 0;
    if (curstop == 0)
        goto skip2;
    scrdownall();
    item = item->lastitem;
    thisitem = item;
    curstop--;
    showtopitem();
    lastrow++;
    /* .ok2 (unused label) */
    row = 0;
    col = 0;
    d1 = (WORD)curstop;
    if ((UWORD)d1 < (UWORD)bloktopy)            /* bcs .skip2 */
        goto skip2;
    invertarea(0, 1, lastcol, (WORD)lastrow);
    col = (WORD)chcols;
    row = 0;
    lastcol = -1;
    lastrow = 0;
    d1 = (WORD)curstop;
    if (d1 != bloktopy)
        goto skip2;
    lastcol = (WORD)(bloktopx - 1);
    goto skip2;

skip0:
    col = (WORD)(col + cursoff);
    d1 = (WORD)(row + curstop);
    if ((UWORD)d1 >= numlines)                  /* bcs .skip0z */
        row = (WORD)(numlines - curstop - 1);
    /* .skip0z */
    if (d0 == 0) {
        /* go left */
        col--;
        if (cursoff == 0)
            goto skip2;
        col--;
        /* movem d2-d3/d5-d6 around: locals stay */
        blokbotx = lastcol;
        blokboty = (WORD)(lastrow + curstop);
        btopx = bloktopx; btopy = (UWORD)bloktopy;  /* move.l bloktopx,btopx */
        bbotx = blokbotx; bboty = (UWORD)blokboty;  /* move.l blokbotx,bbotx */
        sortblox(NULL);
        cursoff -= 2;
        scrollrite();
        blokdis();
        goto skip2;
    }
    /* .skip0y - go rite */
    d0 = (WORD)(col - cursoff);
    if ((UWORD)d0 < edcols)                     /* bcs .skip2 */
        goto skip2;
    col = (WORD)(cursoff + edcols);
    if ((UWORD)col >= chcols)                   /* bcc .skip2 */
        goto skip2;
    col++;
    blokbotx = lastcol;
    blokboty = (WORD)(lastrow + curstop);
    btopx = bloktopx; btopy = (UWORD)bloktopy;
    bbotx = blokbotx; bboty = (UWORD)blokboty;
    sortblox(NULL);
    cursoff += 2;
    scrollleft();
    blokdis();
    goto skip2;

skip2:
    if (lastcol == col && lastrow == (UWORD)row)
        goto loop;
    /* .skip */
    showprops2(col, (UWORD)row);
    blokbotx = col;
    d1 = (WORD)(row + curstop);
    blokboty = d1;
    if ((UWORD)d1 < (UWORD)bloktopy)            /* bcs .above */
        goto above;
    if (d1 != bloktopy)                         /* bne .below */
        goto below;
    if (col < bloktopx)                         /* blt .above */
        goto above;
below:
    if (invflag)
        goto fix;
    q.topx = lastcol; q.topy = (WORD)lastrow; q.botx = col; q.boty = row;
    lastcol = col;
    lastrow = (UWORD)row;
    orderem(&q);
    invertarea2((WORD)(q.topx + 1), q.topy, q.botx, q.boty);
    goto loop;
above:
    if (!invflag)
        goto fix;
    q.topx = lastcol; q.topy = (WORD)lastrow; q.botx = col; q.boty = row;
    lastcol = col;
    lastrow = (UWORD)row;
    orderem(&q);
    invertarea2(q.topx, q.topy, (WORD)(q.botx - 1), q.boty);
    goto loop;
fix:
    invflag = (UWORD)~invflag;
    {
        WORD scol = col, srow = row;            /* movem d2-d3,-(a7) */

        col = lastcol;
        row = (WORD)lastrow;
        d0 = bloktopx;
        d1 = (WORD)(bloktopy - curstop);
        if (d1 < 0) {                           /* bpl .fskip */
            d0 = 0;
            d1 = 0;
        } else if ((UWORD)d1 >= edrows) {       /* .fskip: bcs .fskip2 */
            d1 = (WORD)(edrows - 1);
            d0 = (WORD)(chcols - 1);
        }
        /* .fskip2 */
        invertarea(d0, d1, col, row);
        col = scol;
        row = srow;
        lastcol = col;
        lastrow = (UWORD)row;
        invertarea(d0, d1, col, row);
    }
    goto loop;

mouseup:
    newctop = curstop;
    newcoff = (UWORD)cursoff;
    /* survey-03 I.11: the scroll position before the drag comes back */
    thisitem = s_item;
    cursoff = s_cursoff;
    curstop = s_curstop;
    d0 = blokbotx;
    if (d0 >= 0) {                              /* bpl .shit */
        if ((UWORD)d0 < chcols)                 /* .shit: bcs .shit2 */
            goto shit2;
        goto shit4;
    }
    d1 = blokboty;
    if ((UWORD)d1 <= (UWORD)bloktopy) {         /* bls .shit3 */
        d0 = 0;                                 /* .shit3 */
        goto shit2;
    }
    blokboty--;
shit4:
    d0 = (WORD)(chcols - 1);
shit2:
    blokbotx = d0;
    if (bloktopx == blokbotx && bloktopy == blokboty)   /* cmp.l d1,d0 */
        goto one;
    bloken();
    /* addq #8,a7: the saved btop/bbot are dropped */
    btopx = bloktopx; btopy = (UWORD)bloktopy;
    bbotx = blokbotx; bboty = (UWORD)blokboty;
    sortblox(NULL);
    showhprop();
    showall();                                  /* bra showall */
    return;

one:
    /* they've just selected a one block block - move cursor there. */
    bbotx = s_bbotx; bboty = s_bboty;
    btopx = s_btopx; btopy = s_btopy;
    cursx = (UWORD)(bloktopx - cursoff);
    d0 = (WORD)(curstop + cursy);
    item = thisitem;
    for (;;) {
        /* ted.s:2290 oneloop */
        if ((UWORD)d0 == (UWORD)bloktopy)
            break;                              /* beq .skip */
        if ((UWORD)d0 < (UWORD)bloktopy) {      /* bcs .skip2 */
            item = item->nextitem;
            d0++;
        } else {
            item = item->lastitem;
            d0--;
        }
    }
    /* .skip */
    cursy = (UWORD)(d0 - curstop);
    thisitem = item;
    calcpos();
    if (bboty != (UWORD)-1) {
        bloken();
        showall();
        return;
    }
    /* .nben - no block marked */
    if (curstop != newctop || (UWORD)cursoff != newcoff) {
        showall();
        return;
    }
    cursor2();                                  /* bra cursor2 */
}

/* ------------------------------------------- goto / block marks / WB */

/* ted.s:2409 actgad - activate gadget in a0 (gad) */
ULONG actgad(struct Gadget *gad)
{
    Delay(8);
    return ActivateGadget(gad, req_window, NULL);
}

/* ted.s:2420 gotoline */
void gotoline(void)
{
    UWORD id = ev_data;
    LONG ev;
    ULONG n;

    gotoreqsg1.BufferPos = 0;                   /* clr gotoreqsg1+8 */
    gotoreqsg1.DispPos = 0;                     /* clr gotoreqsg1+12 */
    gotoint = 0;
    gotoreqla8[0] = 0;
    openreq(&gotoreq);
    actgad(&gotoreqga1);                        /* (lea gotoreq,a2: no effect) */
    for (;;) {
        /* .loop: getinput closes the requester on ENDGADGET */
        ev = getinput(&id);
        if ((WORD)ev != -1)
            continue;
        if (id == 1 || id == 2)
            break;                              /* .ok */
        if (id == 3)
            return;
    }
    /* .ok */
    n = (ULONG)gotoint;
    if (n == 0) {
        beepem();
        return;
    }
    if ((UWORD)n > numlines) {                  /* word compare (survey-03 I.10) */
        beepem();
        return;
    }
    gotolined0((UWORD)(n - 1));                 /* bra gotolined0 */
}

/* ted.s:2501 closewb */
void closewb(void)
{
    CloseWorkBench();
}

/* ted.s:2507 openwb */
void openwb(void)
{
    OpenWorkBench();
    ScreenToFront(myscreen);
    ActivateWindow(mywindow);                   /* jmp activatewindow(a6) */
}

/* --------------------------------------------- console window / CLI */

/* ted.s:2513 findio (no Forbid/LockDosList, survey-03 I.9) */
BOOL findio(const char *name, UWORD len)
{
    struct RootNode *rn = (struct RootNode *)DOSBase->dl_Root;
    struct DosInfo *di = (struct DosInfo *)BADDR(rn->rn_Info);
    BPTR dev = di->di_DevInfo;
    struct DevInfo *dvi;
    const UBYTE *dname, *p;
    UWORD n;

    for (;;) {
        /* .loop */
        dvi = (struct DevInfo *)BADDR(dev);
        dname = (const UBYTE *)BADDR(dvi->dvi_Name);
        if (*dname++ == (UBYTE)len) {
            p = (const UBYTE *)name;
            n = (UWORD)(len - 1);
            for (;;) {                          /* .comp */
                if (*p++ != *dname++)
                    goto next;
                if (n-- == 0)
                    return TRUE;                /* moveq #0,d0: Z=1 */
            }
        }
next:
        dev = dvi->dvi_Next;
        if (!dev)
            return FALSE;                       /* moveq #-1,d0 */
    }
}

/* ted.s:2541 calciorows */
UWORD calciorows(void)
{
    return (GfxBase->DisplayFlags & NTSC) ? 200 : 256;     /* btst #0,207(a1) */
}

/* ted.s:2548 openio - open a default io handle */
ULONG openio(UWORD *ccr)
{
    struct Process *pr;
    UWORD rows;

    hiteasc[0] = ' ';
    hiteasc[1] = ' ';
    hiteasc[2] = ' ';
    /* survey-03 I.8: the editor screen's height, also on the WB screen */
    rows = (UWORD)newscreen.Height;             /* newscreen+6 */
    if (!screensopen)
        rows = calciorows();
    /* .docon */
    numtoascii(rows, (UBYTE *)hiteasc);
    pr = (struct Process *)FindTask(NULL);
    oldih = pr->pr_CIS;
    oldoh = pr->pr_COS;
    /* see if NEWCON is there... */
    if (findio(ioname, 6))
        myhandle = Open((STRPTR)ioname, MODE_NEWFILE);  /* .usenew */
    else
        myhandle = Open((STRPTR)ioname2, MODE_NEWFILE); /* .useold */
    if (!myhandle) {
        *ccr = CCR_Z;                           /* .fuct: move.l d0 = 0 / beq */
        return 0;
    }
    /* .isok */
    pr->pr_CIS = myhandle;
    pr->pr_COS = myhandle;
    /* callint wbenchtofront: the asm returns with d0 and the CCR that
       WBenchToFront left (mycli tests Z, survey-03 I.8) - called through
       callregs (glue.c) to get them: LVO -342, a6 = IntuitionBase */
    {
        struct m68kregs r = {{0}};

        r.a[6] = (ULONG)IntuitionBase;
        *ccr = callregs((UBYTE *)IntuitionBase - 342, &r);
        return r.d[0];
    }
}

/* ted.s:2597 closeio */
void closeio(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);

    pr->pr_CIS = oldih;
    pr->pr_COS = oldoh;
    Close(myhandle);                            /* also 0 (survey-03 I.9) */
    if (screensopen) {
        ScreenToFront(myscreen);
        ActivateWindow(mywindow);               /* jmp activatewindow(a6) */
    }
    /* .nosc */
}

/* ted.s:2616 mycli */
void mycli(void)
{
    UWORD ccr;

    openio(&ccr);
    if (ccr & CCR_Z)
        return;                                 /* beq .skip */
    Execute((STRPTR)&nullcom, myhandle, 0);
    closeio();                                  /* bra closeio */
}

/* ted.s:2629 refgads - refresh gadgets - a0 = gadget, d0 = number (count) */
void refgads(struct Gadget *gad, LONG count)
{
    RefreshGList(gad, req_window, NULL, count);
}

/* ------------------------------------------------------------ help */

/* ted.s:2697 help - the help key stuff..... */
void help(void)
{
    struct item *item = thisitem;
    const char *name, *text, *src;
    char *dst;
    UWORD n, hex, digit;
    int i;

    if (!tokenise) {
        beepem();
        return;
    }
    n = helptoke;
    name = help_core(item, (UWORD)(cursoff + cursx), &n);
    helptoke = n;
    if (!name) {
        showfname();                            /* beq showfname */
        return;
    }
    text = name + strlen(name) + 1;             /* help text behind the name */
    /* survey-03 I.13: newsctit (112 bytes) is not checked */
    src = name;
    dst = newsctit;
    while ((*dst++ = *src++) != 0)              /* .loop3 */
        ;
    dst--;
    if (*text != '(')
        *dst++ = ' ';
    while ((*dst++ = *text++) != 0)             /* .loop4 */
        ;
    ShowTitle(myscreen, -1);
    if (!(qualifier & 0x80))
        return;
    /* .am: right Amiga - "blitz2:help XXXX" */
    dst = helphex;
    hex = helptoke;
    for (i = 0; i < 4; i++) {                   /* .looph */
        hex = (UWORD)((hex << 4) | (hex >> 12)); /* rol.w #4,d0 */
        digit = (UWORD)((hex & 15) + 48);
        if (digit >= 58)
            digit += 7;
        *dst++ = (char)digit;
    }
    Execute((STRPTR)helpex, 0, 0);              /* jmp execute(a6) */
}

/* ted.s:2771 doclean */
void doclean(void)
{
    if (cleanup)
        call_cleanup();                         /* all registers saved */
}

/* ted.s:2781 getstuff */
void getstuff(void)
{
    UBYTE *text;
    struct Menu *m;
    struct mitem *mi;
    struct MenuItem **link;
    UWORD len, top;
    UBYTE c;

    if (tokens) {
        tokenise = 1;
        defaultreqga8.Flags |= 0x80;            /* or #$80,defaultreqga8+12 */
    }
    /* .skip */
    text = menitems;
    if (!text)
        return;                                 /* .skip2 */
    ClearMenuStrip(mywindow);
    /* survey-03 I.16: AllocMem unchecked, never freed (escape frees it) */
    m = AllocMem(sizeof(struct Menu), MEMF_PUBLIC);   /* menu2name-menu2 */
    menu2.NextMenu = m;                         /* move.l a0,(a1) - before the copy */
    memcpy(m, &menu2, sizeof(struct Menu));     /* .loop2 */
    m->NextMenu = NULL;
    m->MenuName = (STRPTR)text;
    len = 0;
    do                                          /* .loop3 */
        len++;
    while (*text++);
    m->LeftEdge = 192 + 56;
    m->Width = (WORD)(len << 3);
    link = &m->FirstItem;                       /* lea 18(a0),a4 */
    top = 0;
    while (*text) {
        /* .loop4 */
        mi = AllocMem(sizeof(struct mitem), MEMF_PUBLIC);   /* m2i1text-menu2item1 */
        memcpy(mi, &menu2item1, sizeof(struct mitem));      /* .loop5 */
        mi->item.Width = 200;                   /* !*! */
        mi->item.NextItem = NULL;
        mi->item.TopEdge = (WORD)top;
        top += 12;
        *link = &mi->item;
        link = &mi->item.NextItem;
        mi->item.ItemFill = &mi->it;
        mi->it.IText = text;
        while (*text++)                         /* .loop6 */
            ;
        mi->item.Command = 0;
        mi->item.Flags &= ~COMMSEQ;
        c = *text++;
        if (c) {
            mi->item.Command = c;
            mi->item.Flags |= COMMSEQ;
        }
    }
    /* .skip3 */
    SetMenuStrip(mywindow, &menu1);
}

/* ted.s:2916 showmice - show all mouseable labels! */
void showmice(void)
{
    UBYTE *at;
    const UBYTE *src;
    struct micenode *node;
    UWORD count, row;
    WORD n;
    UBYTE c;

    if (label == 255)
        return;
    freemice();
    makemice();
    count = nummice;
    if (!count) {
        if (mousecols) {
            mousecols = 0;
            screenclose();
            screenopen2();                      /* bra screenopen2 */
        }
        return;                                 /* .miceok2 */
    }
    /* .ismice (survey-03 I.17: results unchecked) */
    if (!mousecols) {
        mousecols = 12;
        screenclose();
        screenopen2();
        count = nummice;
    }
    /* .miceok1 */
    at = bitmaps[1] + (WORD)edcols + 2;
    row = 0;
    node = mice;
    do {
        /* .loop */
        n = (WORD)(mousecols - 1);
        if (row >= count)
            goto loop3;
        src = node->line->chars;
        if (tokenise) {
            detok(src, charbuff);               /* survey-03 I.17: charbuff */
            src = charbuff;
        }
        while (*src++ == 32)                    /* .loop12: skips the label char too */
            ;
        for (;;) {
            /* .loop2 */
            c = *src++;
            if (c == 0 || c == colon || c == 32)
                goto loop3;
            qprint(c, at);
            at++;
            if (n-- == 0)                       /* dbf d5,.loop2 */
                goto next;
        }
loop3:
        do {
            qprint(32, at);
            at++;
        } while (n-- != 0);                     /* dbf d5,.loop3 */
next:
        at -= (WORD)mousecols;
        at += (WORD)fontmul;
        if (node) {
            /* BUG(orig) (#381) survey-03 I.4: behind the last node this reads the
               uninitialised `next` (and then follows it) */
            if (row + 1 >= nummice)
                ORIG_HAZARD("showmice: next of the last label node");
            node = node->next;
        }
        /* .skip0 */
        row++;
    } while (row < edrows);
}

/* ted.s:3019 printmem */
void printmem(void)
{
    ULONG d0 = AvailMem(MEMF_LARGEST | MEMF_PUBLIC);    /* #1<<17+1 */

    d0 >>= 10;
    memcpy(numtext, "     ", 6);                 /* $20202020 / $2000 */
    numtoascii((UWORD)d0, (UBYTE *)numtext);    /* survey-03 I.18: low word */
    pstats(numtext, statsloc + 38);             /* bra pstats */
}

/* ----------------------------------------------- memory errors */

/* ted.s:3049 extalloc - external allocmem */
APTR extalloc(ULONG size, ULONG flags)
{
    APTR p = AllocMem(size, flags);

    if (p)
        return p;
    /* survey-03 I.7: leaves the compiler in the middle of its run */
    closereq();
    outmem();
}

/* ted.s:3057 doalert */
void doalert(void)
{
    DisplayAlert(RECOVERY_ALERT, (STRPTR)alertstuff, 40);
}

/* ted.s:3038 outmem */
NORETURN void outmem(void)
{
    doalert();
    if (filehand)
        Close((BPTR)filehand);                  /* not cleared (survey-03 I.5) */
    /* .skip */
    curstop = 0;
    cursy = 0;
    thisitem = firstitem;
    showall();
    /* move.l memstack,a7 / bra memcont: longjmp to the setjmp in
       mainedit (DIFF in docs/ted/deviations.md: outmem/memcont).
       BUG(orig) (#370) (O1, survey-03 I.6): before the first dispatch memstack
       is 0 - the asm continues memcont with a7 = 0 and crashes; the glue
       does exactly that (memcont_nostack, glue.c). */
    if (!memstack)
        memcont_nostack();
    longjmp(*memstack, 1);
}
