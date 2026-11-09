/*
 * input.c - chunk 06, Amiga part (ted.s 5253-6285): see input.h.
 *
 * The chmap parts of the line editor are core (src/core/lineedit.c, gl_*);
 * getline here does the dispatch, the bitmap shifts and the printing in
 * the order of the asm.  Word compares as in the asm: d7 is a long, but
 * `cmp #n,d7` compares its low word (Latin-1 $FF is not -1 here,
 * survey-06 I.6).
 */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <hardware/custom.h>
#include <stddef.h>
#include <string.h>
#include "ted.h"

/* ted.s:6027 mydata / 6030 comdata: the code entries are the register
   stubs of glue.c; 84 is set by outside (&low byte of defaultreqga9.Flags) */
struct mydata mydata = {
    screenclose_stub, screenopen2_stub,
    openreq_stub, closereq_stub, getinput_stub, error_stub,
    detokallz_stub, tokall_stub, ezfreq_stub, refgads_stub, getinput2_stub,
    NULL, NULL, NULL,                   /* combitmap, myscreen, extint */
    colors_stub, halfcolors_stub, tokone_stub, openio_stub, closeio_stub,
    actgad_stub, pointeron_stub, sleepon_stub,
    NULL, NULL,                         /* mywindow2, 84 */
    extalloc_stub,
    0, NULL,                            /* bmapmul, req_window */
    TED_PROGMARK, progress_stub         /* TED-C: the progress call (#980) */
};
struct Window *mywindow;                /* ted.s:6154 */
struct RastPort *myrast;                /* ted.s:6155 */

/* ted.s:6171 jumptable: key code, showvprop flag, routine; end: code 0 */
const struct jtentry jumptable[] = {
    { 28, 1, cursup },
    { 29, 1, cursdown },
    { 13, 1, return_ },
    { 255, 1, NULL, gadget },
    { 254, 0, NULL, menu },
    { 253, 1, mousedown },
    { 139, 0, help },
    { 129, 0, blockstart },
    { 130, 0, NULL, blockend },
    { 131, 0, blocksave },
    { 132, 0, blokcopy2 },
    { 0, 0, NULL }
};

/* ted.s:6199 menutable: item<<4 | menu, flag, routine; end: 255.  Linear
   search in this order. */
const struct jtentry menutable[] = {
    { 0x00, 0, new },
    { 0x02, 1, top },
    { 0x12, 1, bottom },
    { 0x90, 0, escape },
    { 0x51, 1, insertline },
    { 0x61, 1, deleteline },
    { 0x41, 0, blokforget },
    { 0x01, 1, blokcopy },
    { 0x11, 1, blokkill },
    { 0x21, 0, bloksave },
    { 0x31, 0, blokload },
    { 0x10, 1, fileopen },
    { 0x20, 0, filesave },
    { 0x30, 0, defaults },
    { 0x81, 1, join },
    { 0x91, 1, blocktab },
    { 0xa1, 1, blockuntab },
    { 0xb1, 0, markindent },
    { 0x03, 1, find },
    { 0x13, 1, next },
    { 0x23, 1, previous },
    { 0x33, 1, replace },
    { 0x43, 1, replaceall },
    { 0x50, 0, prtfile },
    { 0x60, 0, mycli },
    { 0x22, 1, NULL, gotoline },
    { 0x70, 0, closewb },
    { 0x80, 0, openwb },
    { 0x40, 0, doabout },
    { 255, 0, NULL }
};

/* dbf loop counter: body runs n+1 times (n = $FFFF: 65536 times) */
#define DBF(n, body) do { UWORD dbf_ = (n); do { body; } while (dbf_-- != 0); } while (0)

/* ted.s:5253 cursor */
void cursor(void)
{
    cursor2();                          /* same address */
}

/* ted.s:5254 cursor2 */
void cursor2(void)
{
    UBYTE *plane0, *plane1;

    calcphys(&plane0, &plane1);
    DBF(fonthite2, {                    /* .loop */
        *plane0 = (UBYTE)~*plane0;
        *plane1 = (UBYTE)~*plane1;
        plane0 += (WORD)bmapmul;
        plane1 += (WORD)bmapmul;
    });
}

/* ted.s:5263 showrow */
void showrow(void)
{
    memcpy(numtext, "     ", 6);        /* move.l #$20202020 / move #$2000 */
    numtoascii((UWORD)(curstop + cursy + 1), (UBYTE *)numtext);
    pstats(numtext, statsloc + 5);      /* bra pstats */
}

/* ted.s:5275 showvprop */
void showvprop(void)
{
    ULONG vpot, vbody;
    UWORD line, lastline;

    showrow();
    vbody = (ULONG)-1;
    if (edrows < numlines)              /* bcc .skip */
        vbody = 0xFFFF0000UL | (UWORD)divu_w((ULONG)edrows << 16, numlines); /* swap/clr/divu */
    /* .skip */
    vpot = (ULONG)-1;
    line = (UWORD)(curstop + cursy);
    lastline = (UWORD)(numlines - 1);
    if (line < lastline)                /* bcc .skip2 */
        vpot = 0xFFFF0000UL | (UWORD)divu_w((ULONG)line << 16, lastline);
    /* .skip2: flags 13 = AUTOKNOB|FREEVERT|PROPBORDERLESS; jmp
       newmodifyprop(a6) */
    NewModifyProp(&newgads, mywindow, NULL, 13, (ULONG)-1, vpot, (ULONG)-1, vbody, 1);
}

/* ted.s:5305 showhprop */
void showhprop(void)
{
    ULONG hpot;
    UWORD col, lastcol;

    memcpy(numtext, "   ", 4);          /* move.l #$20202000 */
    numtoascii((UWORD)(cursoff + cursx + 1), (UBYTE *)numtext);
    pstats(numtext, statsloc + 18);
    hpot = (ULONG)-1;
    col = (UWORD)(cursx + cursoff);
    lastcol = (UWORD)(chcols - 1);
    if (col < lastcol) {                /* bcc .skip */
        /* move d0,d1 / swap / clr / divu d2,d1: remainder in the upper
           word (passed on as in the asm) */
        hpot = divu_w((ULONG)col << 16, lastcol);
    }
    /* .skip: flags 11 = AUTOKNOB|FREEHORIZ|PROPBORDERLESS, HorizBody
       unchanged (asm: the upper word of d3 is undefined, intuition uses
       the low word) */
    NewModifyProp(&newgads2, mywindow, NULL, 11, hpot, (ULONG)-1,
                  prop2sinfo.HorizBody, (ULONG)-1, 1);
}

/* ted.s:5337 refresh */
void refresh(void)
{
    /* redraw entire screen using cursoff */
    UBYTE *src;

    storepos();
    scloc = 0;
    chloc = 0;
    cursy = 0;
    DBF(edrows - 1, {                   /* .loop */
        cursx = 0;
        src = chmap + (WORD)chloc + cursoff;
        DBF(edcols - 1, {               /* .loop2 */
            print3(*src++);
            cursx++;
        });
        godown();
    });
    color = normcol;
    restorepos();                       /* bra restorepos */
}

/* bitmap part of .isasc: cells cursx..edcols-2 of the cursor row one to
   the right (both planes, fonthite rows) */
static void shiftright(UBYTE *plane0, UBYTE *plane1, UWORD from)
{
    UWORD col = edcols;

    for (;;) {
        col--;                          /* .loop */
        if (from >= col)                /* bcc .skip */
            return;
        DBF(fonthite2, {                /* .loop2 */
            plane0[(WORD)col] = plane0[(WORD)col - 1];
            plane1[(WORD)col] = plane1[(WORD)col - 1];
            plane0 += (WORD)bmapmul;
            plane1 += (WORD)bmapmul;
        });
        plane0 -= (WORD)fontmul;
        plane1 -= (WORD)fontmul;
    }
}

/* bitmap part of char127: cells cursx+1..edcols-1 one to the left */
static void shiftleft(UBYTE *plane0, UBYTE *plane1, UWORD col)
{
    for (;;) {
        col++;                          /* .loop */
        if (col >= edcols)              /* bcc .skip */
            return;
        DBF(fonthite2, {                /* .loop2 */
            plane0[(WORD)col - 1] = plane0[(WORD)col];
            plane1[(WORD)col - 1] = plane1[(WORD)col];
            plane0 += (WORD)bmapmul;
            plane1 += (WORD)bmapmul;
        });
        plane0 -= (WORD)fontmul;
        plane1 -= (WORD)fontmul;
    }
}

/* ted.s:5416 getline */
LONG getline(UWORD *id)
{
    /* get a line of input from user */
    LONG event;
    UBYTE *plane0, *plane1, *row;
    UWORD col;
    WORD newcol;

    anyalt = 0;
    cursor();
    goto getline3;
    /* ted.s:5420 getline2 */
getline2:
    cursor();
    showhprop();
    /* ted.s:5422 getline3 */
getline3:
    event = getinput(id);
    cursor();
    plane0 = bitmaps[0] + scloc;
    plane1 = bitmaps[1] + scloc;
    row = chmap + (WORD)chloc;
    col = cursx;
    /* ***** HELP ***** */
    if (event < 0)
        goto charhi;
    if (event & (1L << 30)) {           /* bclr #30,d7 / bne .isasc */
        event &= ~(1L << 30);
        goto isasc;
    }
    if ((UWORD)event < 32)              /* cmp #32,d7 / bcs charlow */
        goto charlow;
    if ((UWORD)event >= 127)            /* bcc charhi */
        goto charhi;
isasc:
    /* .isasc: anyalt, adjust block, chmap (core) */
    gl_isasc((UBYTE)event);
    shiftright(plane0, plane1, col);    /* .skip0 */
    print((UBYTE)event);                /* .skip2 */
    cursrite();
    goto getline2;

    /* ted.s:5484 charlow */
charlow:
    if ((UWORD)event != 31)             /* curs left */
        goto charnot31;
    if ((qualifier & 3) == 0) {
        cursleft();                     /* .skip */
        goto getline2;
    }
    /* shift cursor left. */
    if ((UWORD)(col + cursoff) != 0) {
        newcol = 0;
        goto curspos;
    }
    newcol = gl_tababove();             /* .tababove */
    if (newcol < 0)
        goto getline2;
    goto curspos;

    /* ted.s:5521 charnot31 */
charnot31:
    if ((UWORD)event != 30)
        goto charnot30;
    if ((qualifier & 3) == 0)
        goto cis30skip;
    newcol = gl_lastcol();
    /* falls through into curspos */
    /* ted.s:5537 curspos */
curspos:
    /* d1 holds column to go 4 */
    horizpos(newcol);
    goto getline2;
    /* ted.s:5540 cis30skip */
cis30skip:
    cursrite();
    goto getline2;

    /* ted.s:5543 charnot30 */
charnot30:
    if ((UWORD)event != 8)
        goto charnot8;
    if ((col | (UWORD)cursoff) == 0)
        goto getline2;
    cursleft();                         /* a0-a2 saved: they do not depend
                                           on cursx/cursoff */
    col = cursx;
    event = 8;
    goto char127;

    /* ted.s:5555 charnot8 */
charnot8:
    if ((UWORD)event != 9)
        goto charnot9;
    if (qualifier & 3)
        newcol = gl_untab();            /* delete tab */
    else
        newcol = gl_tab();              /* .loop: insert tab */
    if (newcol < 0)
        goto getline2;
    reline(newcol);                     /* bra reline / falls into reline */
    goto getline2;

    /* ted.s:5658 charnot9 */
charnot9:
    goto glinedone;

    /* ted.s:5662 charhi */
charhi:
    if ((UWORD)event != 127)
        goto charnot127;
    /* falls through into char127 */
    /* ted.s:5665 char127 */
char127:
    /* anyalt, adjust block, chmap (core) */
    gl_char127((UBYTE)event);
    shiftleft(plane0, plane1, col);     /* .loop */
    {
        /* .skip2: redraw the last visible cell from chmap */
        UWORD sx = cursx;

        cursx = (UWORD)(edcols - 1);
        print3(row[(WORD)(edcols + cursoff - 1)]);
        cursx = sx;
    }
    goto getline2;

    /* ted.s:5718 charnot127 */
charnot127:
    if ((UWORD)event != 0xFFFF)         /* cmp #-1,d7 */
        goto charnotgad;
    if (*id != 1)
        goto charnotgad;
    /* horizontal prop: (HorizPot * chcols) >> 16 */
    newcol = (WORD)(((ULONG)prop2sinfo.HorizPot * chcols) >> 16);
    goto curspos;

    /* ted.s:5726 charnotgad */
charnotgad:
    if ((UWORD)event != 0xFFFE)         /* cmp #-2,d7 */
        goto charnotmenu;
    if (*id != 0xf8e1)
        goto charnotmenu;
    reline(gl_delright());              /* bra reline */
    goto getline2;

    /* ted.s:5738 charnotmenu */
charnotmenu:
    /* ted.s:5740 glinedone */
glinedone:
    return event;
}

/* ted.s:5645 reline */
void reline(WORD col)
{
    /* redisplay line */
    UWORD sx = cursx;                   /* move cursx,-(a7) */
    UBYTE *src = chmap + (WORD)chloc + cursoff;

    cursx = 0;
    DBF(edcols - 1, {                   /* .loop3 */
        print3(*src++);
        cursx++;
    });
    cursx = sx;
    horizpos(col);                      /* move (a7)+,d1 / bra curspos
                                           (getline2 follows in getline) */
}

/* ted.s:5810 flash (unused debug code) */
void flash(UWORD unused)
{
    volatile struct Custom *c = (volatile struct Custom *)0xdff000;
    UWORD n = 0xFFFF;                   /* move #-1,d0 */

    (void)unused;                       /* saved and restored */
    do
        c->color[0] = n;                /* .loop: the counter is written */
    while (n-- != 0);
}

/* ted.s:5827 pickwindow */
struct Window *pickwindow(void)
{
    if (req_window)
        return req_window;
    return mywindow;
}

/* ted.s:5833 getinput2 */
LONG getinput2(UWORD *id)
{
    /* get input - or 0 if non available */
    struct IntuiMessage *msg;

    msg = (struct IntuiMessage *)GetMsg(pickwindow()->UserPort);
    if (msg)
        return handled0(msg, id);       /* bne handled0 */
    return 0;
}

/* ted.s:5845 getinput */
LONG getinput(UWORD *id)
{
    /* receive user input into d7 */
    struct MsgPort *port;
    struct IntuiMessage *msg;
    LONG event;

    for (;;) {
        /* ted.s:5852 ginp1 */
        port = pickwindow()->UserPort;
        do {
            WaitPort(port);             /* .loop */
            msg = (struct IntuiMessage *)GetMsg(port);
        } while (!msg);
        event = handled0(msg, id);
        if (event)                      /* beq ginp1 */
            return event;
    }
}

/* ted.s:5866 handled0 */
LONG handled0(struct IntuiMessage *im, UWORD *id)
{
    /* handle message in d0 - return ne if message understood, else eq */
    struct IntuiMessage *msg = im;
    struct Gadget *gad;
    ULONG class = msg->Class;
    LONG event;
    UWORD code, c;

    if (class == IDCMP_GADGETDOWN || class == IDCMP_GADGETUP) {
        gad = (struct Gadget *)msg->IAddress;   /* .yesgad: gadget! */
yesgad2:
        *id = gad->GadgetID;
        event = -1;
        if ((UBYTE)gad->GadgetType == GTYP_STRGADGET    /* string gadget? */
            && msg->Code == 9)                  /* tabbed? */
            goto again;                         /* yes, ignore */
        /* .notst */
        if (!((UBYTE)gad->Activation & GACT_ENDGADGET))
            goto done;
        if ((UWORD)class == IDCMP_GADGETDOWN)
            goto done;
        /* survey-06 I.8 (ENDGADGET semantics): the requester is closed before
           the caller sees the event */
        ReplyMsg(&msg->ExecMessage);
        closereq();
        return event;                   /* moveq #-1,d0 */
    }
    /* .notgadget */
    code = msg->Code;
    if (class == IDCMP_MENUVERIFY)
        goto again;                     /* verify */
    if (class == IDCMP_MENUPICK) {
        if (code == MENUNULL)
            goto again;
        *id = code;                     /* NextSelect ignored (I.16) */
        event = -2;
        goto done;
    }
    /* .notmenu */
    if (class == IDCMP_MOUSEBUTTONS) {
        event = -3;                     /* mouse down */
        if (code == SELECTDOWN)
            goto done;
        event = -4;                     /* mouse up */
        goto done;
    }
    /* .notmouse */
    if (class == IDCMP_RAWKEY) {
        /* get ascii (0-$bf) */
        if (code & IECODE_UP_PREFIX)
            goto again;                 /* key up! */
        qualifier = msg->Qualifier;
        c = rawed_convert(code, qualifier);     /* bsr rawed+32+4 */
        event = (UBYTE)c;               /* moveq #0,d7 / move.b d0,d7 */
        if (event == 0)
            goto again;
        if (!req_window)
            goto notareq;
        /* ok, flash checking for [return] and [esc] */
        if (c != 13)                    /* cmp #13,d0 (word) */
            goto notreturn;
        /* return! - find first closereq gadget! */
        for (gad = req_window->FirstGadget; gad; gad = gad->NextGadget) /* .retloop */
            if (((UBYTE)gad->Activation & GACT_ENDGADGET)
                && (UBYTE)gad->GadgetType == GTYP_BOOLGADGET)
                goto khere;
        goto again;
khere:
        refme(gad);
        class = IDCMP_GADGETUP;         /* move #$40,d1 */
        goto yesgad2;
notreturn:
        if (c != 27)                    /* escape? */
            goto again;
        goto doesc;
notareq:
        event = (LONG)germs_map((UBYTE)event);  /* .gloop / .gskip: bit 30 */
        goto done;
    }
    /* .notkey */
    if (class != IDCMP_CLOSEWINDOW)
        goto again;
    /* close gadget! - do an escape type thingy. */
doesc:
    {
        /* .doesc: last found...  survey-06 I.7: without a
           requester this reads FirstGadget at address 62 (window NULL);
           the main window has no CLOSEWINDOW IDCMP, so unreachable */
        struct Gadget *last = NULL;

        if (!req_window)
            ORIG_HAZARD("handled0 .doesc: 62(req_window = 0)");
        gad = *(struct Gadget * volatile *)((ULONG)req_window
                                            + offsetof(struct Window, FirstGadget));
        for (; gad; gad = gad->NextGadget)      /* .escloop */
            if (((UBYTE)gad->Activation & GACT_ENDGADGET)
                && (UBYTE)gad->GadgetType == GTYP_BOOLGADGET)
                last = gad;
        if (!last)                      /* .escdone */
            goto again;
        gad = last;
        goto khere;
    }
done:
    ReplyMsg(&msg->ExecMessage);
    return event;                       /* moveq #-1,d0 */
again:
    ReplyMsg(&msg->ExecMessage);
    return 0;                           /* d7 = 0, d0 = 0 */
}

/* ted.s:6003 refme */
void refme(struct Gadget *gad)
{
    /* a0=gadget to blink on!  (the "d0.b=pl if select else un-select" of
       the original comment is not used, survey-06 I.11) */
    gad->Flags |= GFLG_SELECTED;        /* or #$80,12(a0) */
    RefreshGList(gad, req_window, NULL, 1);
    Delay(4);
    gad->Flags &= 0xff7f;
    RefreshGList(gad, req_window, NULL, 1);
}
