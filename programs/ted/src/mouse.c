/*
 * mouse.c - chunk 03, Amiga part (ted.s 1981-3062): see mouse.h.
 *
 * Mouse selection (mousedown, gomouse), the goto requester, Workbench
 * open/close, the console window for running programs (openio/closeio,
 * comdata+60/64), the gadget services for blitz2 (actgad, refgads), the
 * help key display, the COMPILER menu (getstuff), the label column
 * (showmice), printmem, outmem/extalloc/doalert, doclean.
 *
 * The asm keeps its state in registers across calls; the C locals carry
 * the same names (d0-d6, a4).  Where a called routine changes a register
 * that the asm uses afterwards, C passes it explicitly (getinput2 writes
 * d6 for gadget/menu messages, orderem returns the ordered d0-d3).
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
void gomouse(WORD d2, UWORD d3)
{
    struct micenode *a0;
    struct item *a1;
    UWORD d0, d1;

    d0 = (UWORD)(edcols + 2);
    if ((UWORD)d2 < d0)                         /* bcs .skip */
        return;
    if (d3 >= nummice)                          /* bcc .skip */
        return;
    a0 = mice;                                  /* lea mice,a0 / move.l (a0),a0 */
    while (d3--)                                /* dbf d3,.loop */
        a0 = a0->next;
    a1 = firstitem;
    d0 = 0;
    d1 = (UWORD)(numlines - 1);
    do {                                        /* .loop2 */
        if (a0->line == a1) {
            gotolined0(d0);                     /* beq gotolined0 */
            return;
        }
        a1 = a1->nextitem;
        d0++;
    } while (d1-- != 0);
    /* .skip */
}

/* ted.s:2001 mousedown - o.k. - now for the biggy - select text with the
   f***ing mouse. */
void mousedown(void)
{
    struct item *a4 = thisitem, *s_a4;
    struct quad q;
    WORD d0, d1, d2, d3, d5, t;
    UWORD d6, s_curstop;
    WORD s_btopx, s_bbotx, s_cursoff;
    UWORD s_btopy, s_bboty;
    LONG d7;
    BOOL in;

    if (!getmousexy(&q))
        return;                                 /* bmi .done */
    d2 = q.d2;
    d3 = q.d3;
    if ((UWORD)d2 >= edcols) {
        gomouse(d2, (UWORD)d3);                 /* bcc gomouse */
        return;
    }
    d1 = (WORD)(d3 + curstop);
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
    d1 = (WORD)(d3 + curstop);
    bloktopy = d1;
    blokboty = d1;
    d2 = (WORD)(d2 + cursoff);
    bloktopx = d2;
    blokbotx = d2;
    showprops2(d2, (UWORD)d3);
    d5 = d2;
    d6 = (UWORD)d3;
    d0 = d2;
    d1 = d3;
    invflag = 0;
    s_curstop = curstop;
    s_cursoff = cursoff;
    s_a4 = a4;
    /* .loop2 */
    invertarea(d0, d1, d2, d3);
loop:
    /* busy poll (survey-03 I.11); handled0 writes d6 for gadget and menu
       messages - the asm's d6 (end row) as well */
    d7 = getinput2(&d6);
    if ((WORD)d7 == -4)
        goto mouseup;
    in = getmousexy(&q);
    d0 = q.d0; d1 = q.d1; d2 = q.d2; d3 = q.d3;
    if (in)
        goto skip0;
    if ((UWORD)d1 < topskip)                    /* bcs .mup */
        goto mup;
    /* mouse drag down */
    d3 = (WORD)edrows;
    d0 = (WORD)(d3 + curstop);
    if ((UWORD)d0 >= numlines)
        goto loop;
    d3--;
    d2 = (WORD)chcols;
    d0 = (WORD)(curstop + edrows);
    if ((UWORD)d0 >= numlines)                  /* never (survey-03 I.11) */
        goto skip2;
    scrupall();                                 /* (d5/d6 saved) */
    a4 = a4->nextitem;
    thisitem = a4;
    curstop++;
    showbotitem();
    d6--;
    /* .ok (unused label) */
    d2 = (WORD)chcols;
    d3 = (WORD)(edrows - 1);
    d1 = (WORD)(d3 + curstop);
    if ((UWORD)d1 > (UWORD)bloktopy)            /* bhi .skip2 */
        goto skip2;
    t = d1;                                     /* move d1,-(a7) */
    invertarea((WORD)chcols, (WORD)(edrows - 2), d5, (WORD)d6);
    d5 = (WORD)chcols;
    d6 = (UWORD)(edrows - 1);
    d3 = (WORD)d6;
    d2 = -1;
    d1 = t;
    if (d1 != bloktopy)
        goto skip2;
    d5 = (WORD)(bloktopx + 1);
    goto skip2;

mup:
    /* mouse drag up! - currently FUCKED */
    d2 = 0;
    d3 = 0;
    if (curstop == 0)
        goto skip2;
    scrdownall();
    a4 = a4->lastitem;
    thisitem = a4;
    curstop--;
    showtopitem();
    d6++;
    /* .ok2 (unused label) */
    d3 = 0;
    d2 = 0;
    d1 = (WORD)curstop;
    if ((UWORD)d1 < (UWORD)bloktopy)            /* bcs .skip2 */
        goto skip2;
    invertarea(0, 1, d5, (WORD)d6);
    d2 = (WORD)chcols;
    d3 = 0;
    d5 = -1;
    d6 = 0;
    d1 = (WORD)curstop;
    if (d1 != bloktopy)
        goto skip2;
    d5 = (WORD)(bloktopx - 1);
    goto skip2;

skip0:
    d2 = (WORD)(d2 + cursoff);
    d1 = (WORD)(d3 + curstop);
    if ((UWORD)d1 >= numlines)                  /* bcs .skip0z */
        d3 = (WORD)(numlines - curstop - 1);
    /* .skip0z */
    if (d0 == 0) {
        /* go left */
        d2--;
        if (cursoff == 0)
            goto skip2;
        d2--;
        /* movem d2-d3/d5-d6 around: locals stay */
        blokbotx = d5;
        blokboty = (WORD)(d6 + curstop);
        btopx = bloktopx; btopy = (UWORD)bloktopy;  /* move.l bloktopx,btopx */
        bbotx = blokbotx; bboty = (UWORD)blokboty;  /* move.l blokbotx,bbotx */
        sortblox(NULL);
        cursoff -= 2;
        scrollrite();
        blokdis();
        goto skip2;
    }
    /* .skip0y - go rite */
    d0 = (WORD)(d2 - cursoff);
    if ((UWORD)d0 < edcols)                     /* bcs .skip2 */
        goto skip2;
    d2 = (WORD)(cursoff + edcols);
    if ((UWORD)d2 >= chcols)                    /* bcc .skip2 */
        goto skip2;
    d2++;
    blokbotx = d5;
    blokboty = (WORD)(d6 + curstop);
    btopx = bloktopx; btopy = (UWORD)bloktopy;
    bbotx = blokbotx; bboty = (UWORD)blokboty;
    sortblox(NULL);
    cursoff += 2;
    scrollleft();
    blokdis();
    goto skip2;

skip2:
    if (d5 == d2 && d6 == (UWORD)d3)
        goto loop;
    /* .skip */
    showprops2(d2, (UWORD)d3);
    blokbotx = d2;
    d1 = (WORD)(d3 + curstop);
    blokboty = d1;
    if ((UWORD)d1 < (UWORD)bloktopy)            /* bcs .above */
        goto above;
    if (d1 != bloktopy)                         /* bne .below */
        goto below;
    if (d2 < bloktopx)                          /* blt .above */
        goto above;
below:
    if (invflag)
        goto fix;
    q.d0 = d5; q.d1 = (WORD)d6; q.d2 = d2; q.d3 = d3;
    d5 = d2;
    d6 = (UWORD)d3;
    orderem(&q);
    invertarea2((WORD)(q.d0 + 1), q.d1, q.d2, q.d3);
    goto loop;
above:
    if (!invflag)
        goto fix;
    q.d0 = d5; q.d1 = (WORD)d6; q.d2 = d2; q.d3 = d3;
    d5 = d2;
    d6 = (UWORD)d3;
    orderem(&q);
    invertarea2(q.d0, q.d1, (WORD)(q.d2 - 1), q.d3);
    goto loop;
fix:
    invflag = (UWORD)~invflag;
    {
        WORD sd2 = d2, sd3 = d3;                /* movem d2-d3,-(a7) */

        d2 = d5;
        d3 = (WORD)d6;
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
        invertarea(d0, d1, d2, d3);
        d2 = sd2;
        d3 = sd3;
        d5 = d2;
        d6 = (UWORD)d3;
        invertarea(d0, d1, d2, d3);
    }
    goto loop;

mouseup:
    newctop = curstop;
    newcoff = (UWORD)cursoff;
    /* survey-03 I.11: the scroll position before the drag comes back */
    thisitem = s_a4;
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
    a4 = thisitem;
    for (;;) {
        /* ted.s:2290 oneloop */
        if ((UWORD)d0 == (UWORD)bloktopy)
            break;                              /* beq .skip */
        if ((UWORD)d0 < (UWORD)bloktopy) {      /* bcs .skip2 */
            a4 = a4->nextitem;
            d0++;
        } else {
            a4 = a4->lastitem;
            d0--;
        }
    }
    /* .skip */
    cursy = (UWORD)(d0 - curstop);
    thisitem = a4;
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

/* ted.s:2409 actgad - activate gadget in a0 */
ULONG actgad(struct Gadget *a0)
{
    Delay(8);
    return ActivateGadget(a0, req_window, NULL);
}

/* ted.s:2420 gotoline */
void gotoline(void)
{
    UWORD d6 = ev_d6;
    LONG d7;
    ULONG d0;

    gotoreqsg1.BufferPos = 0;                   /* clr gotoreqsg1+8 */
    gotoreqsg1.DispPos = 0;                     /* clr gotoreqsg1+12 */
    gotoint = 0;
    gotoreqla8[0] = 0;
    openreq(&gotoreq);
    actgad(&gotoreqga1);                        /* (lea gotoreq,a2: no effect) */
    for (;;) {
        /* .loop: getinput closes the requester on ENDGADGET */
        d7 = getinput(&d6);
        if ((WORD)d7 != -1)
            continue;
        if (d6 == 1 || d6 == 2)
            break;                              /* .ok */
        if (d6 == 3)
            return;
    }
    /* .ok */
    d0 = (ULONG)gotoint;
    if (d0 == 0) {
        beepem();
        return;
    }
    if ((UWORD)d0 > numlines) {                 /* word compare (survey-03 I.10) */
        beepem();
        return;
    }
    gotolined0((UWORD)(d0 - 1));                /* bra gotolined0 */
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
BOOL findio(const char *a0, UWORD d0)
{
    struct RootNode *rn = (struct RootNode *)DOSBase->dl_Root;
    struct DosInfo *di = (struct DosInfo *)BADDR(rn->rn_Info);
    BPTR d1 = di->di_DevInfo;
    struct DevInfo *a1;
    const UBYTE *a2, *a3;
    UWORD n;

    for (;;) {
        /* .loop */
        a1 = (struct DevInfo *)BADDR(d1);
        a2 = (const UBYTE *)BADDR(a1->dvi_Name);
        if (*a2++ == (UBYTE)d0) {
            a3 = (const UBYTE *)a0;
            n = (UWORD)(d0 - 1);
            for (;;) {                          /* .comp */
                if (*a3++ != *a2++)
                    goto next;
                if (n-- == 0)
                    return TRUE;                /* moveq #0,d0: Z=1 */
            }
        }
next:
        d1 = a1->dvi_Next;
        if (!d1)
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
    struct Process *a3;
    UWORD d0;

    hiteasc[0] = ' ';
    hiteasc[1] = ' ';
    hiteasc[2] = ' ';
    /* survey-03 I.8: the editor screen's height, also on the WB screen */
    d0 = (UWORD)newscreen.Height;               /* newscreen+6 */
    if (!screensopen)
        d0 = calciorows();
    /* .docon */
    numtoascii(d0, (UBYTE *)hiteasc);
    a3 = (struct Process *)FindTask(NULL);
    oldih = a3->pr_CIS;
    oldoh = a3->pr_COS;
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
    a3->pr_CIS = myhandle;
    a3->pr_COS = myhandle;
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
    struct Process *a3 = (struct Process *)FindTask(NULL);

    a3->pr_CIS = oldih;
    a3->pr_COS = oldoh;
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

/* ted.s:2629 refgads - refresh gadgets - a0 = gadget, d0 = number */
void refgads(struct Gadget *a0, LONG d0)
{
    RefreshGList(a0, req_window, NULL, d0);
}

/* ------------------------------------------------------------ help */

/* ted.s:2697 help - the help key stuff..... */
void help(void)
{
    struct item *a4 = thisitem;
    const char *d1, *a2, *a0;
    char *a1;
    UWORD n, d0, d2;
    int i;

    if (!tokenise) {
        beepem();
        return;
    }
    n = helptoke;
    d1 = help_core(a4, (UWORD)(cursoff + cursx), &n);
    helptoke = n;
    if (!d1) {
        showfname();                            /* beq showfname */
        return;
    }
    a2 = d1 + strlen(d1) + 1;                   /* help text behind the name */
    /* survey-03 I.13: newsctit (112 bytes) is not checked */
    a0 = d1;
    a1 = newsctit;
    while ((*a1++ = *a0++) != 0)                /* .loop3 */
        ;
    a1--;
    if (*a2 != '(')
        *a1++ = ' ';
    while ((*a1++ = *a2++) != 0)                /* .loop4 */
        ;
    ShowTitle(myscreen, -1);
    if (!(qualifier & 0x80))
        return;
    /* .am: right Amiga - "blitz2:help XXXX" */
    a1 = helphex;
    d0 = helptoke;
    for (i = 0; i < 4; i++) {                   /* .looph */
        d0 = (UWORD)((d0 << 4) | (d0 >> 12));   /* rol.w #4,d0 */
        d2 = (UWORD)((d0 & 15) + 48);
        if (d2 >= 58)
            d2 += 7;
        *a1++ = (char)d2;
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
    UBYTE *a5;
    struct Menu *m;
    struct mitem *a0;
    struct MenuItem **a4;
    UWORD d0, d7;
    UBYTE c;

    if (tokens) {
        tokenise = 1;
        defaultreqga8.Flags |= 0x80;            /* or #$80,defaultreqga8+12 */
    }
    /* .skip */
    a5 = menitems;
    if (!a5)
        return;                                 /* .skip2 */
    ClearMenuStrip(mywindow);
    /* survey-03 I.16: AllocMem unchecked, never freed (escape frees it) */
    m = AllocMem(sizeof(struct Menu), MEMF_PUBLIC);   /* menu2name-menu2 */
    menu2.NextMenu = m;                         /* move.l a0,(a1) - before the copy */
    memcpy(m, &menu2, sizeof(struct Menu));     /* .loop2 */
    m->NextMenu = NULL;
    m->MenuName = (STRPTR)a5;
    d0 = 0;
    do                                          /* .loop3 */
        d0++;
    while (*a5++);
    m->LeftEdge = 192 + 56;
    m->Width = (WORD)(d0 << 3);
    a4 = &m->FirstItem;                         /* lea 18(a0),a4 */
    d7 = 0;
    while (*a5) {
        /* .loop4 */
        a0 = AllocMem(sizeof(struct mitem), MEMF_PUBLIC);   /* m2i1text-menu2item1 */
        memcpy(a0, &menu2item1, sizeof(struct mitem));      /* .loop5 */
        a0->item.Width = 200;                   /* !*! */
        a0->item.NextItem = NULL;
        a0->item.TopEdge = (WORD)d7;
        d7 += 12;
        *a4 = &a0->item;
        a4 = &a0->item.NextItem;
        a0->item.ItemFill = &a0->it;
        a0->it.IText = a5;
        while (*a5++)                           /* .loop6 */
            ;
        a0->item.Command = 0;
        a0->item.Flags &= ~COMMSEQ;
        c = *a5++;
        if (c) {
            a0->item.Command = c;
            a0->item.Flags |= COMMSEQ;
        }
    }
    /* .skip3 */
    SetMenuStrip(mywindow, &menu1);
}

/* ted.s:2916 showmice - show all mouseable labels! */
void showmice(void)
{
    UBYTE *a3;
    const UBYTE *a2;
    struct micenode *a4;
    UWORD d3, d4;
    WORD d5;
    UBYTE d0;

    if (label == 255)
        return;
    freemice();
    makemice();
    d3 = nummice;
    if (!d3) {
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
        d3 = nummice;
    }
    /* .miceok1 */
    a3 = bitmaps[1] + (WORD)edcols + 2;
    d4 = 0;
    a4 = mice;
    do {
        /* .loop */
        d5 = (WORD)(mousecols - 1);
        if (d4 >= d3)
            goto loop3;
        a2 = a4->line->chars;
        if (tokenise) {
            detok(a2, charbuff);                /* survey-03 I.17: charbuff */
            a2 = charbuff;
        }
        while (*a2++ == 32)                     /* .loop12: skips the label char too */
            ;
        for (;;) {
            /* .loop2 */
            d0 = *a2++;
            if (d0 == 0 || d0 == colon || d0 == 32)
                goto loop3;
            qprint(d0, a3);
            a3++;
            if (d5-- == 0)                      /* dbf d5,.loop2 */
                goto next;
        }
loop3:
        do {
            qprint(32, a3);
            a3++;
        } while (d5-- != 0);                    /* dbf d5,.loop3 */
next:
        a3 -= (WORD)mousecols;
        a3 += (WORD)fontmul;
        if (a4) {
            /* BUG(orig) (#381) survey-03 I.4: behind the last node this reads the
               uninitialised `next` (and then follows it) */
            if (d4 + 1 >= nummice)
                ORIG_HAZARD("showmice: next of the last label node");
            a4 = a4->next;
        }
        /* .skip0 */
        d4++;
    } while (d4 < edrows);
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
APTR extalloc(ULONG d0, ULONG d1)
{
    APTR p = AllocMem(d0, d1);

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
