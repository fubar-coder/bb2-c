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
void setdefs(struct defblock *opts)
{
    UWORD old[4], neu[4], group;
    int i;

    if (!tokens)
        opts->d_tokenise = 0;                   /* clr 6(a5): the SOURCE block */
    /* .skip0 */
    defaultreqga7.Flags &= 0xFFFF - 0x80;
    if (opts->d_fonthite == 6)
        defaultreqga7.Flags |= 0x80;
    /* .skip - Fixed (#383): the original shifts the raw words (lsl #7) into
       the gadget flags and takes them over as tokenise/makeicons, so that
       values other than 0/1 from the options file set foreign flag bits
       (2: GFLG_DISABLED, survey-04 I.17).  Only bit 0 of each word counts. */
    opts->d_tokenise &= 1;
    opts->d_makeicons &= 1;
    defaultreqga8.Flags &= 0xFFFF - 0x80;
    defaultreqga8.Flags |= (UWORD)(opts->d_tokenise << 7);
    defaultreqga9.Flags &= 0xFFFF - 0x80;
    defaultreqga9.Flags |= (UWORD)(opts->d_makeicons << 7);
    /* colour buttons: old one up, new one down (normcol, labcol, tokcol,
       propcol - consecutive words in both blocks) */
    old[0] = normcol; old[1] = labcol; old[2] = tokcol; old[3] = propcol;
    neu[0] = opts->d_normcol; neu[1] = opts->d_labcol;
    neu[2] = opts->d_tokcol; neu[3] = opts->d_propcol;
    for (i = 0, group = 0; group < 16; i++, group += 4) { /* .loop */
        swapd0bord((UWORD)(old[i] | group));
        swapd0bord((UWORD)(neu[i] | group));
    }
    numtoascii2(opts->d_hscmarg, defaultreqla47);
    numtoascii2(opts->d_vscmarg, defaultreqla48);
    numtoascii2(opts->d_tabstop, defaultreqla49);
    defstart = *opts;                           /* .loop2: 25 words */
    /* move hscmarg,int1+2: low word of StringInfo.LongInt only */
    int1 = (LONG)(((ULONG)int1 & 0xFFFF0000) | hscmarg);
    int2 = (LONG)(((ULONG)int2 & 0xFFFF0000) | vscmarg);
    int3 = (LONG)(((ULONG)int3 & 0xFFFF0000) | tabstop);
    setprops(propcol);                          /* bra setprops */
}

/* ted.s:3167 setprops - set prop settings to colour reg d0 */
void setprops(UWORD pen)
{
    const UWORD *rgb = palit + (UWORD)(pen * 3); /* lsl #1 / *3 (bytes: *6) */
    int i;

    for (i = 0; i < 3; i++) {                   /* .loop, dbf d1 = 2 */
        UWORD pot = 0xFFFF;
        UWORD v = *rgb++;

        if (v != 15) {
            ULONG l = (ULONG)v << 16;           /* swap d0 / clr d0 */

            /* divu #15: overflow (v > 15) leaves d0 = v<<16, low word 0 */
            pot = (UWORD)divu_w(l, 15);
        }
        /* .skip */
        hps[i]->HorizPot = pot;
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
void propadj(UWORD *rgb, UWORD pen)
{
    UWORD red = (UWORD)(hps[0]->HorizPot >> 12);
    UWORD green = (UWORD)(hps[1]->HorizPot >> 12);
    UWORD blue = (UWORD)(hps[2]->HorizPot >> 12);

    if (rgb[0] == red && rgb[1] == green && rgb[2] == blue)
        return;                                 /* .skip */
    /* .doit */
    rgb[0] = red;
    rgb[1] = green;
    rgb[2] = blue;
    SetRGB4(&myscreen->ViewPort, pen, red, green, blue);
}

/* ted.s:3228 dispgadd0 - display gadget d0 */
void dispgadd0(UWORD button)
{
    RefreshGList(colgads[button], req_window, &defaultreq, 1);
}

/* the two borders of colour button d0 (asm: bo14 + d0*56, +28) */
static void swappens(struct Border *b)
{
    UBYTE t = b->FrontPen;                      /* 4(a0,d0) <-> 5(a0,d0) */

    b->FrontPen = b->BackPen;
    b->BackPen = t;
}

/* ted.s:3238 swapd0bord (bsr .skip + fall through: both borders) */
void swapd0bord(UWORD button)
{
    if (button >= 16) {
        /* BUG(orig) (#384): a value >= 16 from a damaged options file - the asm
           swaps the bytes 4/5 at bo14 + (d0 * 56).w and at that +
           (la63 - bo14) = +28, i.e. behind the button borders.  Done the
           same way here, with the original strides relative to
           defaultreqbo14.  DIFF (T): the C data is laid out differently
           (other variables lie behind the borders), so other bytes are
           swapped than in the original. */
        WORD off = (WORD)(UWORD)(button * 56);  /* mulu #56 / 4(a0,d0.w) */
        UBYTE *bords = (UBYTE *)&defaultreqbo14;
        UBYTE t;

        ORIG_HAZARD("swapd0bord: button index >= 16");
        t = bords[off + 4];
        bords[off + 4] = bords[off + 5];
        bords[off + 5] = t;
        bords += 28;                            /* lea la63-bo14(a0),a0 */
        t = bords[off + 4];
        bords[off + 4] = bords[off + 5];
        bords[off + 5] = t;
        return;
    }
    swappens(colbords[2 * button]);
    swappens(colbords[2 * button + 1]);
}

/* ted.s:3247 swapbords */
void swapbords(void)
{
    int i;

    for (i = 0; i < 32; i++)
        swappens(colbords[i]);
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
        LONG event;
        UWORD id;

        openreq(&erasereq);
        do                                      /* .loop */
            event = getinput(&id);
        while ((WORD)event != -1);
        if (id != 1)
            return FALSE;
    }
    /* .skip */
    mousealt = (UWORD)-1;
    return TRUE;                                /* cmp d6,d6 */
}

/* ted.s:3511 error (comdata+12): waits for gadget 1, always returns */
void error(const char *msg)
{
    LONG event;
    UWORD id;

    errorpnt = (UBYTE *)msg;
    openreq(&errorreq);
    for (;;) {                                  /* .loop */
        event = getinput(&id);
        if ((WORD)event == -1 && id == 1)
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
UBYTE *getfname2(const char *title)
{
    return getfname3(filepath, filename, title); /* falls through into getfname3 */
}

/* ted.s:3493 getfname3 */
UBYTE *getfname3(char *path, char *name, const char *title)
{
    UBYTE *full = ezfreq(path, name, title);

    if (full)
        return full;                            /* d7 = d0 */
    /* .skip: cmp #1,d0 / beq outmem - dead (Z=1 only with d0 = 0,
       survey-07 I.3), kept */
    if ((ULONG)full == 1)
        outmem();
    return NULL;                                /* addq #4,a7 */
}

/* ted.s:3502 ezfreq (comdata+24) */
UBYTE *ezfreq(char *path, char *name, const char *title)
{
    reqstruct[0] = (ULONG)path;
    reqstruct[1] = (ULONG)name;
    reqstruct[2] = (ULONG)title;
    return filerequest(reqstruct);              /* bra filerequest+36 */
}

/* ----------------------------------------- requesters as windows */

/* ted.s:3580 openreq - open a requester.  The comment "return 0 if
   couldn't open it" is wrong: OpenWindow failing -> outmem (survey-04
   I.14).  The gadgets are moved by +6/+12 once, marked in the high word
   of UserData of the first gadget - permanently, also for blitz2's own
   requesters (decision O6, survey-04 I.15).  A requester
   without gadgets reads address 40 (I.14). */
void openreq(struct Requester *rq)
{
    struct Gadget *gad;
    struct RastPort *rp;
    struct Window *win;

    req = rq;
    reqwindow.Width = (WORD)(rq->Width + 12);   /* move.l 8(a0),reqwindow+4 */
    reqwindow.Height = (WORD)(rq->Height + 15);
    /* lsr: unsigned - a requester wider/higher than the screen gives a
       huge position, OpenWindow fails -> outmem */
    reqwindow.LeftEdge = (WORD)((UWORD)((UWORD)(edcols << 3) - reqwindow.Width) >> 1);
    reqwindow.TopEdge = (WORD)((UWORD)((UWORD)newscreen.Height - reqwindow.Height) >> 1);
    gad = rq->ReqGadget;
    req_gads = gad;
    if (!((ULONG)gad->UserData >> 16)) {        /* tst 40(a1) */
        gad->UserData = (APTR)((ULONG)gad->UserData | 0xFFFF0000);
        do {                                    /* .loop */
            gad->LeftEdge += 6;
            gad->TopEdge += 12;
            gad = gad->NextGadget;
        } while (gad);
    }
    /* .skip */
    req_screen = myscreen;
    win = OpenWindow(&reqwindow);
    if (!win)
        outmem();
    req_old2 = req_old;
    req_old = req_window;
    req_window = win;
    rp = req_window->RPort;
    PrintIText(rp, ((struct Requester *)req)->ReqText, 6, 12);
    DrawBorder(rp, ((struct Requester *)req)->ReqBorder, 6, 12);
    pointeron();                                /* bra pointeron */
}

/* drain a message port (closereq/flushmw .loop) */
static void drainport(struct MsgPort *port)
{
    struct Message *msg;

    while ((msg = GetMsg(port)) != NULL)
        ReplyMsg(msg);
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
void showprops2(WORD col, UWORD row)
{
    UWORD sx = cursx, sy = cursy;               /* move.l cursx,-(a7) */

    col = (WORD)(col - cursoff);
    if (col < 0)                                /* bpl .hi */
        col = 0;
    else if ((UWORD)col >= chcols)              /* .hi: bcs .hi2 */
        col = (WORD)(chcols - 1);
    /* .hi2 */
    cursx = (UWORD)col;
    cursy = row;
    showhprop();
    showvprop();
    cursx = sx;                                 /* move.l (a7)+,cursx */
    cursy = sy;
}

/* ted.s:3974 invertarea - invert d0,d1 to d2,d3 (chars) */
void invertarea(WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct quad q;

    q.topx = x1;
    q.topy = y1;
    q.botx = x2;
    q.boty = y2;
    orderem(&q);
    invertarea2(q.topx, q.topy, q.botx, q.boty); /* falls through into invertarea2 */
}

/* ted.s:3977 invertarea2: not.b directly in both planes */
void invertarea2(WORD col, WORD row, WORD endcol, WORD endrow)
{
    UBYTE *plane0 = bitmaps[0], *plane1 = bitmaps[1];
    ULONG off = (ULONG)(UWORD)row * fontmul;    /* mulu */

    plane0 += off;
    plane1 += off;
    endcol = (WORD)(endcol - cursoff);
    col = (WORD)(col - cursoff);
more2:
    /* .more2 (flags of the sub/addq on d0) */
    if (col < 0)
        goto nextchar;
    if ((UWORD)col >= edcols)
        goto nextrow;
    {
        UWORD n = fonthite2;

        do {                                    /* .loop */
            plane0[col] = (UBYTE)~plane0[col];
            plane1[col] = (UBYTE)~plane1[col];
            plane0 += (WORD)bmapmul;
            plane1 += (WORD)bmapmul;
        } while (n-- != 0);
        plane0 -= (WORD)fontmul;
        plane1 -= (WORD)fontmul;
    }
nextchar:
    if (col < endcol)                           /* blt (signed) */
        goto more;
    if ((UWORD)row < (UWORD)endrow)             /* bcs (unsigned) */
        goto more;
    return;                                     /* .done */
more:
    col++;
    goto more2;
nextrow:
    if ((UWORD)row >= (UWORD)endrow)
        return;
    col = (WORD)(-cursoff - 1);
    row++;
    plane0 += (WORD)fontmul;
    plane1 += (WORD)fontmul;
    goto nextchar;
}

/* ted.s:4015 getmousexy - get mouse coords in d0,d1; d2,d3 = col,row;
   return mi if too high or low */
BOOL getmousexy(struct mousexy *r)
{
    struct Screen *scr = myscreen;
    UWORD my = (UWORD)scr->MouseY;              /* 16(a0) */

    r->y = (WORD)my;
    if (my < topskip)
        goto done;
    if (my >= (UWORD)(topskip + pixrows))
        goto done;
    r->x = scr->MouseX;                         /* 18(a0) */
    r->col = (WORD)((UWORD)r->x >> 3);
    /* divu fonthite: quotient (the remainder is in the high word) */
    r->row = (WORD)(UWORD)divu_w((UWORD)(my - topskip), fonthite);
    return TRUE;
done:
    r->x = scr->MouseX;
    r->col = (WORD)((UWORD)r->x >> 3);
    r->row = -1;
    return FALSE;
}

/* -------------------------------------------------- line output */

/* the drawing half of `print` for the core: printa0 with print4 (the
   chmap write of print2 is done by chmap_showa2item) */
static void drawchar(UBYTE c)
{
    printa0(c, print4);
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
void showa2item(struct item *item)
{
    chmap_showa2item(item, drawchar);
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
