/*
 * reqs.c - chunk 04, Amiga part (ted.s 3063-4197): see reqs.h.
 *
 * Options requester helpers, erase/error, requesters as windows, the file
 * requester bridge, mouse marking helpers, the line output hooks (the
 * chmap/walk logic is core: chmap_show* in fileio.c, decision O4).
 */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include "ted.h"

ULONG reqstruct[4];                     /* ted.s:3509 */
/* ted.s:3565 reqwindow: left/top/width/height set by openreq; pens 1,2;
   IDCMP GADGETDOWN|GADGETUP|CLOSEWINDOW|RAWKEY; WINDOWDRAG|WINDOWCLOSE|
   ACTIVATE; min/max -1; CUSTOMSCREEN */
struct NewWindow reqwindow = {
    0, 0, 0, 0, 1, 2,
    32 + 64 + 0x200 + 0x400,
    2 + 8 + 0x1000,
    NULL, NULL, NULL, NULL, NULL,
    -1, -1, -1, -1,
    15
};
struct Window *req_old, *req_old2;      /* ted.s:3577/3578 */

/* ---------------------------------------------- options requester */

/* ted.s:3092 setdefs - convert options at a5 to defaults */
void setdefs(struct defblock *a5)
{
    UWORD old[4], neu[4], d2;
    int i;

    if (!tokens)
        a5->d_tokenise = 0;                     /* clr 6(a5): the SOURCE block */
    /* .skip0 */
    defaultreqga7.Flags &= 0xFFFF - 0x80;
    if (a5->d_fonthite == 6)
        defaultreqga7.Flags |= 0x80;
    /* .skip - BUG(orig) (#383) survey-04 I.17: lsl #7 of the raw word (values
       other than 0/1 set foreign flag bits) */
    defaultreqga8.Flags &= 0xFFFF - 0x80;
    defaultreqga8.Flags |= (UWORD)(a5->d_tokenise << 7);
    defaultreqga9.Flags &= 0xFFFF - 0x80;
    defaultreqga9.Flags |= (UWORD)(a5->d_makeicons << 7);
    /* colour buttons: old one up, new one down (normcol, labcol, tokcol,
       propcol - consecutive words in both blocks) */
    old[0] = normcol; old[1] = labcol; old[2] = tokcol; old[3] = propcol;
    neu[0] = a5->d_normcol; neu[1] = a5->d_labcol;
    neu[2] = a5->d_tokcol; neu[3] = a5->d_propcol;
    for (i = 0, d2 = 0; d2 < 16; i++, d2 += 4) {   /* .loop */
        swapd0bord((UWORD)(old[i] | d2));
        swapd0bord((UWORD)(neu[i] | d2));
    }
    numtoascii2(a5->d_hscmarg, defaultreqla47);
    numtoascii2(a5->d_vscmarg, defaultreqla48);
    numtoascii2(a5->d_tabstop, defaultreqla49);
    defstart = *a5;                             /* .loop2: 25 words */
    /* move hscmarg,int1+2: low word of StringInfo.LongInt only */
    int1 = (LONG)(((ULONG)int1 & 0xFFFF0000) | hscmarg);
    int2 = (LONG)(((ULONG)int2 & 0xFFFF0000) | vscmarg);
    int3 = (LONG)(((ULONG)int3 & 0xFFFF0000) | tabstop);
    setprops(propcol);                          /* bra setprops */
}

/* ted.s:3167 setprops - set prop settings to colour reg d0 */
void setprops(UWORD d0)
{
    const UWORD *a0 = palit + (UWORD)(d0 * 3);  /* lsl #1 / *3 (bytes: *6) */
    int d1;

    for (d1 = 0; d1 < 3; d1++) {                /* .loop, dbf d1 = 2 */
        UWORD d2 = 0xFFFF;
        UWORD v = *a0++;

        if (v != 15) {
            ULONG l = (ULONG)v << 16;           /* swap d0 / clr d0 */

            /* divu #15: overflow (v > 15) leaves d0 = v<<16, low word 0 */
            d2 = (UWORD)divu_w(l, 15);
        }
        /* .skip */
        hps[d1]->HorizPot = d2;
    }
}

/* ted.s:3189 dispprops */
void dispprops(void)
{
    /* survey-04 I.16: defaultreq as requester argument although
       the gadgets live in a window (no misbehaviour known) */
    RefreshGList(&defaultreqga1, req_window, &defaultreq, 3);
}

/* ted.s:3196 propadj - do setrgb4 if necessary */
void propadj(UWORD *a5, UWORD d4)
{
    UWORD d1 = (UWORD)(hps[0]->HorizPot >> 12);
    UWORD d2 = (UWORD)(hps[1]->HorizPot >> 12);
    UWORD d3 = (UWORD)(hps[2]->HorizPot >> 12);

    if (a5[0] == d1 && a5[1] == d2 && a5[2] == d3)
        return;                                 /* .skip */
    /* .doit */
    a5[0] = d1;
    a5[1] = d2;
    a5[2] = d3;
    SetRGB4(&myscreen->ViewPort, d4, d1, d2, d3);
}

/* ted.s:3228 dispgadd0 - display gadget d0 */
void dispgadd0(UWORD d0)
{
    RefreshGList(colgads[d0], req_window, &defaultreq, 1);
}

/* the two borders of colour button d0 (asm: bo14 + d0*56, +28) */
static void swappens(struct Border *b)
{
    UBYTE d1 = b->FrontPen;                     /* 4(a0,d0) <-> 5(a0,d0) */

    b->FrontPen = b->BackPen;
    b->BackPen = d1;
}

/* ted.s:3238 swapd0bord (bsr .skip + fall through: both borders) */
void swapd0bord(UWORD d0)
{
    if (d0 >= 16) {
        /* BUG(orig) (#384): a value >= 16 from a damaged options file - the asm
           swaps the bytes 4/5 at bo14 + (d0 * 56).w and at that +
           (la63 - bo14) = +28, i.e. behind the button borders.  Done the
           same way here, with the original strides relative to
           defaultreqbo14.  DIFF (T): the C data is laid out differently
           (other variables lie behind the borders), so other bytes are
           swapped than in the original. */
        WORD off = (WORD)(UWORD)(d0 * 56);      /* mulu #56 / 4(a0,d0.w) */
        UBYTE *a0 = (UBYTE *)&defaultreqbo14;
        UBYTE d1;

        ORIG_HAZARD("swapd0bord: button index >= 16");
        d1 = a0[off + 4];
        a0[off + 4] = a0[off + 5];
        a0[off + 5] = d1;
        a0 += 28;                               /* lea la63-bo14(a0),a0 */
        d1 = a0[off + 4];
        a0[off + 4] = a0[off + 5];
        a0[off + 5] = d1;
        return;
    }
    swappens(colbords[2 * d0]);
    swappens(colbords[2 * d0 + 1]);
}

/* ted.s:3247 swapbords */
void swapbords(void)
{
    int d0;

    for (d0 = 0; d0 < 32; d0++)
        swappens(colbords[d0]);
}

/* ------------------------------------------------ erase / error */

/* ted.s:3257 erase */
BOOL erase(void)
{
    /* bne -> addq #4,a7: FALSE, the caller returns at once */
    return erase2();
}

/* ted.s:3262 erase2 - check erase request.  Return eq if OK */
BOOL erase2(void)
{
    if (allalt) {
        LONG d7;
        UWORD d6;

        openreq(&erasereq);
        do                                      /* .loop */
            d7 = getinput(&d6);
        while ((WORD)d7 != -1);
        if (d6 != 1)
            return FALSE;
    }
    /* .skip */
    mousealt = (UWORD)-1;
    return TRUE;                                /* cmp d6,d6 */
}

/* ted.s:3511 error (comdata+12): waits for gadget 1, always returns */
void error(const char *msg)
{
    LONG d7;
    UWORD d6;

    errorpnt = (UBYTE *)msg;
    openreq(&errorreq);
    for (;;) {                                  /* .loop */
        d7 = getinput(&d6);
        if ((WORD)d7 == -1 && d6 == 1)
            break;
    }
}

/* ----------------------------------------- file requester bridge */

/* ted.s:3485 getbname */
UBYTE *getbname(const char *title)
{
    return getfname3(blokpath, blokname, title);
}

/* ted.s:3489 getfname */
UBYTE *getfname(void)
{
    return getfname2(filetext);                 /* falls through into getfname2 */
}

/* ted.s:3490 getfname2 */
UBYTE *getfname2(const char *a2)
{
    return getfname3(filepath, filename, a2);   /* falls through into getfname3 */
}

/* ted.s:3493 getfname3 */
UBYTE *getfname3(char *a0, char *a1, const char *a2)
{
    UBYTE *d0 = ezfreq(a0, a1, a2);

    if (d0)
        return d0;                              /* d7 = d0 */
    /* .skip: cmp #1,d0 / beq outmem - dead (Z=1 only with d0 = 0,
       survey-07 I.3), kept */
    if ((ULONG)d0 == 1)
        outmem();
    return NULL;                                /* addq #4,a7 */
}

/* ted.s:3502 ezfreq (comdata+24) */
UBYTE *ezfreq(char *a0, char *a1, const char *a2)
{
    reqstruct[0] = (ULONG)a0;
    reqstruct[1] = (ULONG)a1;
    reqstruct[2] = (ULONG)a2;
    return filerequest(reqstruct);              /* bra filerequest+36 */
}

/* ----------------------------------------- requesters as windows */

/* ted.s:3580 openreq - open a requester.  The comment "return 0 if
   couldn't open it" is wrong: OpenWindow failing -> outmem (survey-04
   I.14).  The gadgets are moved by +6/+12 once, marked in the high word
   of UserData of the first gadget - permanently, also for blitz2's own
   requesters (decision O6, survey-04 I.15).  A requester
   without gadgets reads address 40 (I.14). */
void openreq(struct Requester *a0)
{
    struct Gadget *a1;
    struct RastPort *rp;
    struct Window *d0;

    req = a0;
    reqwindow.Width = (WORD)(a0->Width + 12);   /* move.l 8(a0),reqwindow+4 */
    reqwindow.Height = (WORD)(a0->Height + 15);
    /* lsr: unsigned - a requester wider/higher than the screen gives a
       huge position, OpenWindow fails -> outmem */
    reqwindow.LeftEdge = (WORD)((UWORD)((UWORD)(edcols << 3) - reqwindow.Width) >> 1);
    reqwindow.TopEdge = (WORD)((UWORD)((UWORD)newscreen.Height - reqwindow.Height) >> 1);
    a1 = a0->ReqGadget;
    req_gads = a1;
    if (!((ULONG)a1->UserData >> 16)) {         /* tst 40(a1) */
        a1->UserData = (APTR)((ULONG)a1->UserData | 0xFFFF0000);
        do {                                    /* .loop */
            a1->LeftEdge += 6;
            a1->TopEdge += 12;
            a1 = a1->NextGadget;
        } while (a1);
    }
    /* .skip */
    req_screen = myscreen;
    d0 = OpenWindow(&reqwindow);
    if (!d0)
        outmem();
    req_old2 = req_old;
    req_old = req_window;
    req_window = d0;
    rp = req_window->RPort;
    PrintIText(rp, ((struct Requester *)req)->ReqText, 6, 12);
    DrawBorder(rp, ((struct Requester *)req)->ReqBorder, 6, 12);
    pointeron();                                /* bra pointeron */
}

/* drain a message port (closereq/flushmw .loop) */
static void drainport(struct MsgPort *a2)
{
    struct Message *d0;

    while ((d0 = GetMsg(a2)) != NULL)
        ReplyMsg(d0);
}

/* ted.s:3643 closereq */
void closereq(void)
{
    if (req_window) {
        drainport(req_window->UserPort);
        /* .done */
        ClearPointer(req_window);
        CloseWindow(req_window);
        req_window = req_old;
        req_old = req_old2;
        req_old2 = NULL;
        flushmw();
    }
    /* .done2 */
    makemenus();
}

/* ted.s:3672 flushmw */
void flushmw(void)
{
    drainport(mywindow->UserPort);
}

/* ------------------------------------------------ mouse marking */

/* ted.s:3929 showprops2 */
void showprops2(WORD d2, UWORD d3)
{
    UWORD sx = cursx, sy = cursy;               /* move.l cursx,-(a7) */

    d2 = (WORD)(d2 - cursoff);
    if (d2 < 0)                                 /* bpl .hi */
        d2 = 0;
    else if ((UWORD)d2 >= chcols)               /* .hi: bcs .hi2 */
        d2 = (WORD)(chcols - 1);
    /* .hi2 */
    cursx = (UWORD)d2;
    cursy = d3;
    showhprop();
    showvprop();
    cursx = sx;                                 /* move.l (a7)+,cursx */
    cursy = sy;
}

/* ted.s:3974 invertarea - invert d0,d1 to d2,d3 (chars) */
void invertarea(WORD d0, WORD d1, WORD d2, WORD d3)
{
    struct quad q;

    q.d0 = d0;
    q.d1 = d1;
    q.d2 = d2;
    q.d3 = d3;
    orderem(&q);
    invertarea2(q.d0, q.d1, q.d2, q.d3);        /* falls through into invertarea2 */
}

/* ted.s:3977 invertarea2: not.b directly in both planes */
void invertarea2(WORD d0, WORD d1, WORD d2, WORD d3)
{
    UBYTE *a0 = bitmaps[0], *a1 = bitmaps[1];
    ULONG d4 = (ULONG)(UWORD)d1 * fontmul;      /* mulu */

    a0 += d4;
    a1 += d4;
    d2 = (WORD)(d2 - cursoff);
    d0 = (WORD)(d0 - cursoff);
more2:
    /* .more2 (flags of the sub/addq on d0) */
    if (d0 < 0)
        goto nextchar;
    if ((UWORD)d0 >= edcols)
        goto nextrow;
    {
        UWORD n = fonthite2;

        do {                                    /* .loop */
            a0[d0] = (UBYTE)~a0[d0];
            a1[d0] = (UBYTE)~a1[d0];
            a0 += (WORD)bmapmul;
            a1 += (WORD)bmapmul;
        } while (n-- != 0);
        a0 -= (WORD)fontmul;
        a1 -= (WORD)fontmul;
    }
nextchar:
    if (d0 < d2)                                /* blt (signed) */
        goto more;
    if ((UWORD)d1 < (UWORD)d3)                  /* bcs (unsigned) */
        goto more;
    return;                                     /* .done */
more:
    d0++;
    goto more2;
nextrow:
    if ((UWORD)d1 >= (UWORD)d3)
        return;
    d0 = (WORD)(-cursoff - 1);
    d1++;
    a0 += (WORD)fontmul;
    a1 += (WORD)fontmul;
    goto nextchar;
}

/* ted.s:4015 getmousexy - get mouse coords in d0,d1; d2,d3 = col,row;
   return mi if too high or low */
BOOL getmousexy(struct quad *r)
{
    struct Screen *a0 = myscreen;
    UWORD d1 = (UWORD)a0->MouseY;               /* 16(a0) */
    UWORD d2 = topskip;

    r->d1 = (WORD)d1;
    if (d1 < d2)
        goto done;
    d2 = (UWORD)(d2 + pixrows);
    if (d1 >= d2)
        goto done;
    r->d0 = a0->MouseX;                         /* 18(a0) */
    r->d2 = (WORD)((UWORD)r->d0 >> 3);
    /* divu fonthite: quotient (the remainder is in the high word) */
    r->d3 = (WORD)(UWORD)divu_w((UWORD)(d1 - topskip), fonthite);
    return TRUE;
done:
    r->d0 = a0->MouseX;
    r->d2 = (WORD)((UWORD)r->d0 >> 3);
    r->d3 = -1;
    return FALSE;
}

/* -------------------------------------------------- line output */

/* the drawing half of `print` for the core: printa0 with print4 (the
   chmap write of print2 is done by chmap_showa2item) */
static void drawchar(UBYTE d0)
{
    printa0(d0, print4);
}

/* ted.s:4050 showall */
void showall(void)
{
    chmap_showall(drawchar);
}

/* ted.s:4073 showtopitem */
void showtopitem(void)
{
    chmap_showtopitem(drawchar);
}

/* ted.s:4088 showbotitem */
void showbotitem(void)
{
    chmap_showbotitem(drawchar);
}

/* ted.s:4102 showthisitem */
void showthisitem(void)
{
    showa2item(thisitem);                       /* falls through into showa2item */
}

/* ted.s:4104 showa2item: chmap part in the core (chmap_showa2item, with
   endofline/normprint/normprint2/tokprint), drawing via printa0/print4 */
void showa2item(struct item *a2)
{
    chmap_showa2item(a2, drawchar);
}

/* ted.s:3899 blokprnt2 */
void blokprnt2(void)
{
    pstats(bltext, statsloc + 44);              /* bra pstats */
}

/* NEW: the menu part of bloken/blokdis (ted.s:3890-3912): ITEMENABLED */
void blokmenus(BOOL enable)
{
    if (enable) {
        menu4item1.item.Flags |= 16;
        menu4item2.item.Flags |= 16;
        menu4item4.item.Flags |= 16;
        menu4itema.item.Flags |= 16;
        menu4itemb.item.Flags |= 16;
    } else {
        menu4item1.item.Flags &= 0xFFFF - 16;
        menu4item2.item.Flags &= 0xFFFF - 16;
        menu4item4.item.Flags &= 0xFFFF - 16;
        menu4itema.item.Flags &= 0xFFFF - 16;
        menu4itemb.item.Flags &= 0xFFFF - 16;
    }
}
