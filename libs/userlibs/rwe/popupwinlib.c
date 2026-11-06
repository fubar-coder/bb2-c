/*
 * popupwinlib - RWE userlib 16 (Reflective Images, 1994): popup menus in
 * borderless windows.
 *   PopUpInit                 a new message port for the windows
 *   PopUpWindow(screen#,*items)  the menu `items` at 0,0 of the screen;
 *                             waits until a mouse button is pressed
 *
 * C translation of rwelibsrc/popuplib/popupwinlib.bb (with #GADTOOLS = 0,
 * as the source builds); reference: the reference build of the source
 * (REFERENCE refbuild, ref/popupwinlib.obj; no binary exists).  Number
 * 16 (none known; RWE's RESERVEDLIB_10).  Notes:
 * docs/rwelibs/port/popupwinlib.md.
 *
 * An item: +0 next, +4 the text (0: a separator line), +12 the sub
 * menu's first item (0: none); +8/+10/+16/+20 are not used.  Each menu
 * level has a 64 byte structure (AllocVec, cleared) and a window
 * (WFLG_BORDERLESS|REPORTMOUSE|ACTIVATE on the screen of screenslib's
 * object; the windows share the library's message port, IDCMP
 * MOUSEBUTTONS, INACTIVEWINDOW, MOUSEMOVE and CLOSEWINDOW).  The window
 * is as wide as the widest text + 4 (+ 8 with sub menus), each item
 * fontheight + 6 lines high, a separator 4; the font is the screen's.
 * A mouse move over an item highlights it (pen 3); an item with a sub
 * menu opens it at once to the right of the item; moving to the parent
 * menu closes the sub menu, moving out of all windows clears the
 * highlight; a mouse button closes all levels.  The position comes from
 * the windows' MouseX/MouseY (not from the messages).
 *
 * init opens intuition.library and graphics.library (any version, not
 * checked) and creates the message port; finit replies what is still in
 * the port, deletes it and closes the libraries.
 *
 * Not ported: the routines of the source that nothing calls (WPrint,
 * WinInnerCls, BitMaptoWin, WSizeLimits, WinGadgetHit) and the GadTools
 * variant of WaitWinEvent (#GADTOOLS = 0).
 *
 * BUG(orig), kept:
 *   - PopUpWindow always returns 0: the item chosen is returned by the
 *     menu levels, but the result is `selecteditem`, which is only
 *     cleared (#471).
 *   - Sizing a menu with sub menus toggles bit 1 of CIA-A PRA ($BFE001:
 *     the power LED / audio filter), a debug leftover (#468).
 *   - PopUpInit creates a new message port without deleting the old one
 *     (#470).
 *   - A failed AllocVec is not checked: the menu structure is at address
 *     0; a failed OpenWindow returns 0 and closes nothing else (#467).
 *   - Closing a menu window replies every message in the shared port,
 *     also those for the other levels (#469).
 * Not a defect: the item number count at +8 of the structure is the
 * number of items - 1, but nothing uses that field.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfxbase
#define INTUITION_BASE_NAME intuibase
#include <proto/graphics.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void _popupinit(void), _dopopup(void), popupwinlib_libinit(void);
void popupwinlib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {#popupwinlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_popupwinlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_popupinit, 0, 0);
BLITZ_NAME("PopUpInit", "-- initialise the popup window system");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_screenslib, IA(0, 0));
BLITZ_SUBS(_dopopup, 0, 0);
BLITZ_NAME("PopUpWindow", "Screen#,*items");

BLITZ_NULLSUB_AT(finit, popupwinlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, popupwinlib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

/* an item of the program's (the fields the library reads) */
struct pitem {
    struct pitem *succ;         /* +0 the next item */
    const UBYTE *name;          /* +4 the text, 0: a separator line */
    UWORD unused8, unused10;    /* +8, +10 */
    struct pitem *subs;         /* +12 the sub menu's first item, 0: none */
};
static_assert(__builtin_offsetof(struct pitem, subs) == 12);

/* the structure of a menu level (STRUCT_SIZEOF 64) */
struct popup {
    struct popup *parent;       /* 0 */
    UWORD imagewidth;           /* 4 */
    UWORD itemheight;           /* 6 */
    UWORD numitems;             /* 8 */
    struct pitem *items;        /* 10 */
    UWORD textyoffset;          /* 14 */
    UWORD winwidth;             /* 16 */
    struct pitem *activeitem;   /* 18: the highlighted item */
    UWORD activeypos;           /* 22 */
    struct Window *windowptr;   /* 24 */
    UWORD winx, winy;           /* 28, 30 */
    UWORD popupdelay;           /* 32 */
    UWORD winheight;            /* 34 */
    ULONG exitval;              /* 36 */
    UBYTE pad[24];
} __attribute__((packed, aligned(2)));
#define STRUCT_SIZEOF 64

static struct Library *intuibase, *gfxbase;
static struct MsgPort *msgport;
static struct Screen *myscreen;
static struct RastPort myrast;
static struct TextFont *myfont;
static UWORD fontheight, baseline;
static ULONG selecteditem;
static UBYTE hassubs[2];        /* Dc.b 0,0: SF/ST the first, TST.w both */

/* the event of WaitWinEvent: the IntuiMessage from Class on, 8 longs */
static ULONG e_msg[8];

/* OpenWindow's tag list (static, filled for every window); `extra`:
   TAG_MORE (2) to extra tags, cleared (TAG_DONE) by openwindow */
static struct {
    struct TagItem left, top, width, height, title, screen, flags, detailpen, blockpen;
    struct TagItem minwidth, minheight, maxwidth, maxheight, newlookmenus, idcmp, gadgets;
    struct TagItem extra, end;
} wwtags = {
    { WA_Left, 0 }, { WA_Top, 0 }, { WA_Width, 0 }, { WA_Height, 0 }, { WA_Title, 0 },
    { WA_CustomScreen, 0 }, { WA_Flags, 0 }, { WA_DetailPen, (ULONG)-1 },
    { WA_BlockPen, (ULONG)-1 }, { WA_MinWidth, 32 }, { WA_MinHeight, 11 },
    { WA_MaxWidth, 640 }, { WA_MaxHeight, 512 }, { WA_NewLookMenus, (ULONG)-1 },
    { WA_IDCMP, 0 }, { WA_Gadgets, 0 }, { 2, 0 }, { 0, 0 } };
static_assert(sizeof wwtags == 18 * sizeof(struct TagItem));

static ULONG strlen0(const UBYTE *s)
{
    const UBYTE *p = s;
    while (*p)
        p++;
    return p - s;
}

/* ------------------------------------------------------------- windows */

/* WindowInit: the message port */
static void windowinit(void)
{
    msgport = CreateMsgPort();
}

/* FlushWinEvents: reply every message in the port (those without a
   reply port are only taken) */
static void flushwinevents(void)
{
    struct Message *m;

    if (!msgport)
        return;
    while ((m = GetMsg(msgport)) != NULL)
        if (m->mn_ReplyPort)
            ReplyMsg(m);
}

static void windowfinit(void)
{
    if (!msgport)
        return;
    Forbid();
    flushwinevents();
    DeleteMsgPort(msgport);
    Permit();
    msgport = NULL;
}

/* OpenWindow: on screen scr at x,y, w x h, with the flags and IDCMP;
   the window gets the shared port */
static struct Window *openwindow(struct Screen *scr, ULONG x, ULONG y, ULONG w, ULONG h,
                                 ULONG flags, ULONG idcmp)
{
    struct Window *win;

    wwtags.extra.ti_Tag = 0;                        /* no extra tags */
    wwtags.left.ti_Data = x;
    wwtags.top.ti_Data = y;
    wwtags.width.ti_Data = w;
    wwtags.height.ti_Data = h;
    wwtags.title.ti_Data = 0;                       /* no title */
    wwtags.screen.ti_Data = (ULONG)scr;
    wwtags.flags.ti_Data = flags;
    wwtags.gadgets.ti_Data = 0;                     /* no gadgets */
    win = OpenWindowTagList(NULL, &wwtags.left);
    if (win) {
        win->UserPort = msgport;
        ModifyIDCMP(win, idcmp | IDCMP_CLOSEWINDOW | IDCMP_MOUSEBUTTONS);
    }
    return win;
}

/* WaitWinEvent: the class of the next message (0 for one without a
   reply port, -1 without a port) */
static ULONG waitwinevent(void)
{
    struct Message *m;
    int i;

    e_msg[0] = 0;
    if (!msgport)
        return (ULONG)-1;
    m = GetMsg(msgport);
    if (!m) {
        WaitPort(msgport);
        m = GetMsg(msgport);
        if (!m)
            return (ULONG)-1;
    }
    if (m->mn_ReplyPort) {
        for (i = 0; i < 8; i++)
            e_msg[i] = ((ULONG *)((UBYTE *)m + 20))[i];
        ReplyMsg(m);
    }
    return e_msg[0];
}

/* CloseWin: BUG(orig): replies every message of the shared port (#469) */
static void closewin(struct Window *win)
{
    if (!win)
        return;
    Forbid();
    flushwinevents();
    win->UserPort = NULL;
    CloseWindow(win);
    Permit();
}

/* ------------------------------------------------------------- drawing */

/* BevelBox x1,y1 - x2,y2: pen 2 left and top, pen 1 right and bottom */
static void bevelbox(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2)
{
    SetAPen(rp, 2);
    Move(rp, x1, y2);
    Draw(rp, x1, y1);
    Draw(rp, x2, y1);
    SetAPen(rp, 1);
    Draw(rp, x2, y2);
    Draw(rp, x1 + 1, y2);
}

static void bevelwindow(struct Window *win)
{
    bevelbox(win->RPort, 0, 0, win->Width - 1, win->Height - 1);
}

/* DrawTextItem: the item's text at x, y + the text offset */
static void drawtextitem(struct popup *p, const struct pitem *item, struct RastPort *rp, WORD x,
                         WORD y)
{
    const UBYTE *s = item->name;

    Move(rp, x, (WORD)(y + p->textyoffset));
    Text(rp, (CONST_STRPTR)s, strlen0(s));
}

static void drawitems(struct popup *p, struct Window *win)
{
    struct RastPort *rp = win->RPort;
    struct pitem *it = p->items;
    WORD y = 2;

    for (;;) {
        if (!it->name) {
            bevelbox(rp, 2, y + 1, p->winwidth - 4, y + 2);
            y += 4;
        } else {
            drawtextitem(p, it, rp, 2, y);
            y += p->itemheight;
        }
        if (!it->succ)
            break;
        it = it->succ;
    }
}

/* the highlight of the active item: background pen `pen` */
static void drawactiveitem(struct popup *p, ULONG pen)
{
    struct RastPort *rp = p->windowptr->RPort;

    SetAPen(rp, pen);
    RectFill(rp, 1, p->activeypos, (WORD)(p->winwidth - 2),
             (WORD)(p->activeypos + p->itemheight - 1));
    SetAPen(rp, 1);
    drawtextitem(p, p->activeitem, rp, 2, p->activeypos);
}

/* ----------------------------------------------------------- the mouse */

/* the menu level whose window has the mouse: p or one of its parents */
static struct popup *findmousewindow(struct popup *p)
{
    for (;;) {
        struct Window *w = p->windowptr;
        WORD mx = w->MouseX, my = w->MouseY;

        if (mx >= 0 && my >= 0 && my >= 2 && mx < (WORD)p->winwidth
            && my < (WORD)p->winheight)
            return p;
        if (!p->parent)
            return NULL;
        p = p->parent;
    }
}

/* the item under the mouse (NULL: none or a separator), *ypos its y */
static struct pitem *findmouseitem(struct popup *p, UWORD *ypos)
{
    WORD my = p->windowptr->MouseY;
    struct pitem *it = p->items;
    WORD y = 2, bottom;

    for (;;) {
        bottom = y + (it->name ? (WORD)p->itemheight : 4);
        if (my <= bottom) {
            if (!it->name)
                return NULL;
            *ypos = y;
            return it;
        }
        if (!it->succ)
            return NULL;
        y = bottom;
        it = it->succ;
    }
}

static void clearactive(struct popup *p)
{
    if (!p->activeitem)
        return;
    drawactiveitem(p, 0);
    p->activeitem = NULL;
}

/* MouseMove: highlight, or exitval = the level the mouse went to */
static void mousemove(struct popup *p)
{
    struct popup *m = findmousewindow(p);
    struct pitem *it;
    UWORD y = 0;

    if (!m) {
        clearactive(p);
        return;
    }
    if (m != p) {
        if (m != p->parent) {
            p->exitval = (ULONG)m;
            return;
        }
        /* the parent: back to it unless the mouse is on its active item */
        if (findmouseitem(m, &y) == m->activeitem) {
            clearactive(p);
            return;
        }
        p->exitval = (ULONG)m;
        return;
    }
    it = findmouseitem(p, &y);
    if (!it) {
        clearactive(p);
        return;
    }
    if (it == p->activeitem)
        return;
    if (p->activeitem)
        drawactiveitem(p, 0);
    if (it->subs)
        p->popupdelay = 0;
    p->activeitem = it;
    p->activeypos = y;
    drawactiveitem(p, 3);
}

/* ------------------------------------------------------------ a level */

/* AllocStruct - BUG(orig): not checked (#467) */
static struct popup *allocstruct(struct pitem *items, struct popup *parent)
{
    struct popup *p = AllocVec(STRUCT_SIZEOF, MEMF_PUBLIC | MEMF_CLEAR);
    UWORD n;

    if (!p)
        return p;
    p->itemheight = fontheight + 6;
    p->items = items;
    p->parent = parent;
    n = (UWORD)-1;
    for (;;) {
        n++;
        if (!items->succ)
            break;
        items = items->succ;
    }
    p->numitems = n;
    p->popupdelay = 1;
    p->exitval = (ULONG)-1;
    return p;
}

/* CalcWinSize: width (result) and height (*h) of the window */
static UWORD calcwinsize(struct popup *p, UWORD *h)
{
    struct pitem *it = p->items;
    WORD w = 8, y = 2;

    hassubs[0] = 0;
    while (it) {
        const UBYTE *s = it->name;
        if (!s) {
            y += 4;
        } else {
            WORD l = TextLength(&myrast, (CONST_STRPTR)s, strlen0(s));
            if (w < l)
                w = l;
            y += p->itemheight;
            if (it->subs)
                hassubs[0] = 0xff;
        }
        it = it->succ;
    }
    p->winheight = y;
    w += 4;
    *h = y + 2;
    p->textyoffset = ((WORD)(p->itemheight - fontheight) >> 1) + baseline;    /* ASR.w */
    if (*(UWORD *)hassubs) {
        w += 8;
        *(volatile UBYTE *)0xbfe001 ^= 2;           /* BUG(orig): BCHG #1,$bfe001 (#468) */
    }
    p->winwidth = w;
    return w;
}

/* newpopupwin: the window with the screen's font, JAM1 */
static struct Window *newpopupwin(WORD x, WORD y, UWORD w, UWORD h)
{
    struct Window *win = openwindow(myscreen, (UWORD)x, (UWORD)y, w, h,
                                    WFLG_BORDERLESS | WFLG_REPORTMOUSE | WFLG_ACTIVATE,
                                    IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW
                                    | IDCMP_MOUSEMOVE);
    if (win) {
        SetFont(win->RPort, myfont);
        SetDrMd(win->RPort, JAM1);
    }
    return win;
}

/* DoPop: the menu `items` at x,y; the result is the level that should be
   active now, an item (a mouse button) or 0 (close everything) */
static ULONG dopop(struct pitem *items, struct popup *parent, UWORD x, UWORD y)
{
    struct popup *p = allocstruct(items, parent);
    struct Window *win;
    UWORD w, h;
    ULONG event;                        /* d0: an event class or a sub level's result */
    ULONG result;                       /* d0 */

    w = calcwinsize(p, &h);
    p->winx = x;
    p->winy = y;
    win = newpopupwin(x, y, w, h);
    if (!win) {
        result = 0;
        goto theend;
    }
    p->windowptr = win;
    bevelwindow(win);
    drawitems(p, win);
    for (;;) {
        if (p->popupdelay)
            event = waitwinevent();
        else {
            /* the active item's sub menu, to the right of it */
            event = dopop(p->activeitem->subs, p,
                          p->winx + p->winwidth, p->winy + p->activeypos - 2);
            p->popupdelay = 1;
            if (!event || (event != (ULONG)p && event != (ULONG)p->parent)) {
                result = event;
                break;
            }
            event = IDCMP_MOUSEMOVE;
        }
        if (event == IDCMP_MOUSEMOVE) {
            mousemove(p);
            result = p->exitval;
            if ((LONG)result < 0)
                continue;
            break;
        }
        if (event == IDCMP_MOUSEBUTTONS) {
            result = (ULONG)p->activeitem;
            break;
        }
    }
    closewin(p->windowptr);
theend:
    FreeVec(p);
    return result;
}

/* inittemprast: a RastPort with the screen's font, for TextLength */
static void inittemprast(void)
{
    struct TextFont *f;

    InitRastPort(&myrast);
    f = myscreen->RastPort.Font;
    myfont = f;
    fontheight = f->tf_YSize;
    baseline = f->tf_Baseline;
    SetFont(&myrast, f);
}

/* ------------------------------------------------------------ commands */

/**
 * @brief init: opens intuition.library and graphics.library (any version,
 * not checked) and creates the message port.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    intuibase = OpenLibrary((CONST_STRPTR)"intuition.library", 0);
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    windowinit();
}

/**
 * @brief finit: replies what is still in the port, deletes it and closes
 * the libraries.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    windowfinit();
    CloseLibrary(intuibase);
    CloseLibrary(gfxbase);
}

/**
 * @brief PopUpInit: a new message port for the menu windows.
 *
 * BUG(orig) (#470): the old port is not deleted.  (The word argument is
 * not used.)
 */
static void BLITZ_ROUTINE popupinit_c(void)
{
    windowinit();
}

/**
 * @brief PopUpWindow Screen#,*items: shows the menu `items` at 0,0 of the
 * screen and waits until a mouse button is pressed.
 *
 * Each level is a borderless window with the screen's font; moving over
 * an item highlights it, an item with a sub menu opens it to the right.
 * BUG(orig) (#471): the result is selecteditem, which is only cleared:
 * always 0.  BUG(orig) (#467): a failed AllocVec is not checked;
 * BUG(orig) (#468): sizing a menu with sub menus toggles bit 1 of CIA-A
 * PRA; BUG(orig) (#469): closing a level replies every message of the
 * shared port.
 * @param scr    a0: the screen object (screenslib; first long = the
 *               Screen)
 * @param items  d1: the first item of the menu
 * @return d0: 0
 */
static ULONG BLITZ_ROUTINE dopopup_c(struct Screen **scr BL_REG(a0), struct pitem *items BL_REG(d1))
{
    myscreen = *scr;
    inittemprast();
    selecteditem = 0;
    dopop(items, NULL, 0, 0);
    return selecteditem;
}

/* Registers: the original keeps a3-a6 in PopUpWindow and changes d0-d7/
   a0-a2 there, everything the system calls change in init, finit and
   PopUpInit.  The thunks keep d0 (the statements), d1/a0/a1, the C code
   d2-d7/a2-a6. */
BLITZ_THUNK(popupwinlib_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(popupwinlib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_popupinit, popupinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_dopopup, dopopup_c, "d1/a0-a1");
