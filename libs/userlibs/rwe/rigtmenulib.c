/*
 * rigtmenulib - RWE userlib 37 (Reflective Images): GadTools menus,
 * object library "GTMenuList" (16 bytes, default maximum 5).
 *   GTMenuTitle list#,menu,title$
 *   GTMenuItem list#,flags,menu,item[,text$[,shortcut$[,mx[,UserData]]]]
 *   GTSubItem list#,flags,menu,item,sub[,text$[,shortcut$[,mx[,UserData]]]]
 *   CreateMenuStrip list#   GTSetMenu list#   GTUnSetMenu
 *   GTMenuState list#[,menu[,item[,sub]]],on|off
 *   GTMenuChecked(list#,menu,item[,sub])   GTSetMenuChecked ...,on|off
 *   GTFreeMenu list#,menu[,item[,sub]]
 *   GTGetUserData(list#,menu,item[,sub])
 *
 * C translation of rwelibsrc/GTMenuLib/GTMenuLib3.bb (10/03/96);
 * reference: the reference build of the source (REFERENCE refbuild) -
 * BSS2 RIGTMENULIB.OBJ is the same but for GTSubItem's shortcut$
 * (docs/rwelibs/README.md 3.2).  Notes: docs/rwelibs/port/rigtmenulib.md.
 *
 * The object: +0.l the menu strip made by CreateMenuStrip (gadtools
 * CreateMenusA/LayoutMenusA), +4.l the list of titles, +8.w the number of
 * entries added.  Title (12 bytes, memlib): +0 next, +4 the text, +8 the
 * first item.  Item/sub item (30 bytes, cleared): +0 next, +4 the text
 * (-1 = NM_BARLABEL), +12.w flags, +14 the shortcut, +18 the first sub
 * item, +22 mutual exclude, +26 UserData.  Strings are copied with
 * maxslib AllocSt.  GTMenuState, GTMenuChecked, GTSetMenuChecked and
 * GTGetUserData work on the menu strip (Intuition's Menu/MenuItem).
 *
 * C behind thunks; the walks are DBF loops on words (n + 1 steps, 65536
 * for $FFFF; a missing entry reads address 0), the string lengths are
 * read from the compiler's a2 stack in the original's order without
 * moving a2 (DIFF: the original pops them, docs/libs/cmdlibs-port.md
 * "General").  No assembler.
 *
 * BUG(orig), kept:
 *   - finit closes gadtools.library only if it is NOT open (TST.l:BNE
 *     'ok): never (#690).
 *   - GTMenuItem with UserData (form 4) keeps the mutual exclude of the
 *     GTMenuItem/GTSubItem before (it jumps past MOVE.l d6,me) (#692).
 *   - The shortcut$ is tested by its low word (TST.w d5/d6): a string at
 *     an address with low word 0 counts as none (#694).
 *   - CreateMenuStrip with no title reads the list at address 0; a failed
 *     allocation writes it to address 0.  CreateMenuStrip does not check
 *     CreateMenusA/GetVisualInfoA (#689).
 *   - The free routines (the object's free, GTFreeMenu) never free the
 *     title strings (#695).
 *   - GTFreeMenu tests the item/sub item numbers as longs (TST.l: the
 *     upper word of a word argument decides whether they are given)
 *     (#691).
 *   - GTMenuState/GTSetMenuChecked change the flags of the menu strip
 *     directly, also while it is attached to a window (#693).
 * Like the original, no defects: GTMenuTitle allocates the title
 * uncleared (MEMF_PUBLIC; all three fields are written); +8 counts every
 * entry added and is never decreased (the array of CreateMenuStrip,
 * (count + 1) * 20 bytes, is only larger than needed); freemenu calls
 * FreeMenus with the gadtools base unchecked (a menu strip exists only
 * if it was open); GTMenuChecked returns -1/0 in the low word, the upper
 * word of d0 stays (a word result).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#define __NOLIBBASE__
#define GADTOOLS_BASE_NAME gtbase
#define INTUITION_BASE_NAME intuibase
#include <proto/gadtools.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void _CreateMenuStrip(void), _setmenu(void), _gtunsetmenu(void), _gtmenutitle(void);
void _gtmenuitem2(void), _gtmenuitem3(void), _gtmenuitem(void), _gtmenuitem4(void);
void _gtmenuitem5(void), domenustate(void), domenustate2(void), domenustate3(void);
void domenustate4(void), _gtsubitem2(void), _gtsubitem3(void), _gtsubitem(void);
void _gtsubitem4(void), _gtsubitem5(void), domenuchecked(void), domenuchecked2(void);
void setmenuchecked(void), setmenuchecked2(void), _freetitle(void), _freeitem(void);
void _freesubitem(void), _GetUserData(void), _GetUserData2(void);
void _libinit(void), _libfinit(void), freemenu(void);
void createchk(void), setchk(void), statechk2(void), statechk3(void), statechk4(void);
void chkchk3(void), chkchk4(void), chkchk5(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */
#define W BL_WORD
#define S BL_STRING
#define LG BL_LONG

/* !libheader {#rigtmenulib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_rigtmenulib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_DUMTOKE(_toke, "GTMenuList", "List of GT Menus");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0), LIB_screenslib, BL_LA(0) | BL_USED);
BLITZ_SUBS(_CreateMenuStrip, createchk, 0);
BLITZ_NAME("CreateMenuStrip", "GTMenuList#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0), LIB_windowslib, BL_LA(1) | BL_USED, LIB_intuitionlib,
           BL_LA(6), LIB_screenslib, BL_LA(0) | BL_USED);
BLITZ_SUBS(_setmenu, setchk, 0);
BLITZ_NAME("GTSetMenu", "GTMenuList#");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, BL_LA(2) | BL_USED, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(_gtunsetmenu, 0, 0);
BLITZ_NAME("GTUnSetMenu", "- removes the gtmenulist from the current window");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W, S);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenutitle, 0, 0);
BLITZ_NAME("GTMenuTitle", "GTMenuList#,menu,title$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenuitem2, 0, 0);
BLITZ_ARGS(W, W, W, W, S);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenuitem3, 0, 0);
BLITZ_ARGS(W, W, W, W, S, S);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenuitem, 0, 0);
BLITZ_ARGS(W, W, W, W, S, S, LG);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenuitem4, 0, 0);
BLITZ_ARGS(W, W, W, W, S, S, LG, LG);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtmenuitem5, 0, 0);
BLITZ_NAME("GTMenuItem",
           "GTMenuList#,flags,menu,item[,itemtext$[,shortcut$[,mutualexclude[,UserData]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1380);
BLITZ_SUBS(domenustate, 0, 0);
BLITZ_ARGS(W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1380);
BLITZ_SUBS(domenustate2, statechk2, 0);
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1380);
BLITZ_SUBS(domenustate3, statechk3, 0);
BLITZ_ARGS(W, W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1380);
BLITZ_SUBS(domenustate4, statechk4, 0);
BLITZ_NAME("GTMenuState", "GTMenuList#[,Menu[,Item[,Subitem]]],On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtsubitem2, 0, 0);
BLITZ_ARGS(W, W, W, W, W, S);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtsubitem3, 0, 0);
BLITZ_ARGS(W, W, W, W, W, S, S | BL_PUSH);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtsubitem, 0, 0);
BLITZ_ARGS(W, W, W, W, W, S, S, LG);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtsubitem4, 0, 0);
BLITZ_ARGS(W, W, W, W, W, S, S, LG, LG);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_gtsubitem5, 0, 0);
BLITZ_NAME("GTSubItem", "GTMenuList#,flags,menu,item,subitem[,itemtext$[,shortcut$"
           "[,mutualexclude[,UserData]]]]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(domenuchecked, chkchk3, 0);
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(domenuchecked2, chkchk4, 0);
BLITZ_NAME("GTMenuChecked", "(GTMenuList#,Menu,Item[,Subitem])");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(setmenuchecked, chkchk3, 0);
BLITZ_ARGS(W, W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(setmenuchecked2, chkchk4, 0);
BLITZ_NAME("GTSetMenuChecked", "GTMenuList#,Menu,Item[,Subitem],Off|On");

BLITZ_ASTATEMENT();
BLITZ_ARGS(W, W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_freetitle, 0, 0);
BLITZ_ARGS(W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_freeitem, 0, 0);
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, IA(3, 0));
BLITZ_SUBS(_freesubitem, 0, 0);
BLITZ_NAME("GTFreeMenu", "GTMenuList#,Menu[,Item[,Subitem]]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(_GetUserData, chkchk3, 0);
BLITZ_ARGS(W, W, W, W);
BLITZ_LIBS(LIB_rigtmenulib, 0x1280);
BLITZ_SUBS(_GetUserData2, chkchk5, 0);
BLITZ_NAME("GTGetUserData", "(GTMenuList#,Menu,Item[,Subitem])");

BLITZ_NULLSUB_AT(init, _libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freemenu, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 4);

/* ---------------------------------------------------------------- data */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define P(p, o) (*(UBYTE **)((UBYTE *)(p) + (o)))

#define TITLESIZE 12
#define ITEMSIZE 30
#define MI_SIZEOF 34                    /* GadTools' UserData behind a MenuItem */

static struct Library *gtbase, *intuibase;
static ULONG me, userdata;              /* mutual exclude, UserData */
static UWORD flags;
static ULONG marraysize;
static UBYTE *marray;
static APTR vi;
static const char gtname[] = "gadtools.library";
static const struct TagItem taglist[] = { { GTMN_FullMenu, (ULONG)-1 }, { 0, 0 } };
static const struct TagItem tl_layout[] = { { GTMN_NewLookMenus, (ULONG)-1 }, { 0, 0 } };

static ULONG galloc(ULONG size, ULONG fl)
{
    struct bl_regs r = { .d0 = size, .d1 = fl };
    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void gfree(APTR mem, ULONG d1)
{
    struct bl_regs r = { .d1 = d1, .a1 = (ULONG)mem };
    BLITZ_ALIBJSR(TOK_globfree, r);
}

static ULONG allocst(ULONG s, ULONG len)
{
    struct bl_regs r = { .d0 = s, .d1 = len };
    return BLITZ_ALIBJSR(TOK_allocst, r);
}

static void freest(ULONG s)
{
    struct bl_regs r = { .d0 = s };
    BLITZ_ALIBJSR(TOK_freest, r);
}

/* n + 1 links on from p (MOVE.l (aN),aN:DBF n) */
static UBYTE *walk(UBYTE *p, UWORD n)
{
    do
        p = P(p, 0);
    while (n-- != 0);
    return p;
}

/* ------------------------------------------------------- init / finit */

static void BLITZ_ROUTINE libinit_c(void)
{
    gtbase = OpenLibrary((CONST_STRPTR)gtname, 0);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/* BUG(orig) (#690): closes only if not open */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!gtbase) {
        CloseLibrary(gtbase);
        gtbase = NULL;
    }
}
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- the menu strip */

/* makearray: the NewMenu array of the lists of the object */
static UBYTE *makearray(UBYTE *obj)
{
    UBYTE *t = P(obj, 4), *a1, *it, *sub;

    marraysize = bl_mulu_w((UWORD)(SW(obj, 8) + 1), 20);
    marray = a1 = (UBYTE *)galloc(marraysize, MEMF_PUBLIC | MEMF_CLEAR);
    for (;;) {
        a1[0] = NM_TITLE;
        SL(a1, 2) = SL(t, 4);
        a1 += 20;
        for (it = P(t, 8); it; it = P(it, 0)) {
            a1[0] = NM_ITEM;
            SL(a1, 2) = SL(it, 4);
            SL(a1, 6) = SL(it, 14);
            SW(a1, 10) = SW(it, 12);
            SL(a1, 12) = SL(it, 22);
            SL(a1, 16) = SL(it, 26);
            a1 += 20;
            for (sub = P(it, 18); sub; sub = P(sub, 0)) {
                a1[0] = NM_SUB;
                SL(a1, 2) = SL(sub, 4);
                SL(a1, 6) = SL(sub, 14);
                SW(a1, 10) = SW(sub, 12);
                SL(a1, 12) = SL(sub, 22);
                SL(a1, 16) = SL(sub, 26);
                a1 += 20;
            }
        }
        if (!(t = P(t, 0)))
            break;
    }
    a1[0] = NM_END;
    return marray;
}

/* CreateMenuStrip: a3 = the object, a0 = the used screen object */
static ULONG createmenustrip(UBYTE *obj, struct Screen **scr)
{
    UBYTE *arr;

    if (!gtbase)
        return 0;
    if (P(obj, 0)) {
        FreeMenus((struct Menu *)P(obj, 0));
        SL(obj, 0) = 0;
    }
    if (!(arr = makearray(obj)))
        return 0;
    SL(obj, 0) = (ULONG)CreateMenusA((struct NewMenu *)arr, (struct TagItem *)taglist);
    vi = GetVisualInfoA(*scr, NULL);
    LayoutMenusA((struct Menu *)P(obj, 0), vi, (struct TagItem *)tl_layout);
    FreeVisualInfo(vi);
    gfree(marray, marraysize);
    return (ULONG)-1;
}

static ULONG BLITZ_ROUTINE createmenustrip_c(struct Screen **scr BL_REG(a0),
                                             UBYTE *obj BL_REG(a3))
{
    return createmenustrip(obj, scr);
}
BLITZ_THUNK(_CreateMenuStrip, createmenustrip_c, "d1/a0-a1");

/* GTSetMenu: a1 = the used window object, a6 = intuition */
static void BLITZ_ROUTINE setmenu_c(struct Screen **scr BL_REG(a0), struct Window **win BL_REG(a1),
                                    UBYTE *obj BL_REG(a3), struct Library *intui BL_REG(a6))
{
    struct Window *w;

    intuibase = intui;
    if (!P(obj, 0)) {
        createmenustrip(obj, scr);
        if (!P(obj, 0))
            return;
    }
    w = *win;
    if (w->MenuStrip)
        ClearMenuStrip(w);
    SetMenuStrip(w, (struct Menu *)P(obj, 0));
}
BLITZ_THUNK(_setmenu, setmenu_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtunsetmenu_c(struct Window **win BL_REG(a2),
                                        struct Library *intui BL_REG(a6))
{
    intuibase = intui;
    if ((*win)->MenuStrip)
        ClearMenuStrip(*win);
}
BLITZ_THUNK(_gtunsetmenu, gtunsetmenu_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- the lists */

/* GTMenuTitle list#,menu,title$: inserted as title `menu` */
static void BLITZ_ROUTINE gtmenutitle_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                        const ULONG *args BL_REG(a2), UBYTE *obj BL_REG(a3))
{
    UBYTE *a0 = obj + 4, *t;
    UWORD n;

    if (!gtbase)
        return;
    n = (UWORD)d1 - 1;
    if ((WORD)n >= 0)
        do {
            if (!P(a0, 0))
                return;
            a0 = P(a0, 0);
        } while (n-- != 0);
    t = (UBYTE *)galloc(TITLESIZE, MEMF_PUBLIC);        /* uncleared, all written */
    if (!t)
        return;
    SL(t, 0) = SL(a0, 0);
    SL(a0, 0) = (ULONG)t;
    SL(t, 4) = allocst(d2, args[-1]);
    SL(t, 8) = 0;
    SW(obj, 8)++;
}
BLITZ_THUNK(_gtmenutitle, gtmenutitle_c, "d0-d1/a0-a1");

/* a new item or sub item after entry n - 1 of the list at a0 (n = 0: at
   the head) of the object obj (+8 counts it); text < 0: NM_BARLABEL; the
   lengths popped from the a2 stack in order (`sp`); the shortcut: `sc`
   and its length, or for a sub item (`sub`) the length and the pushed
   address from the stack */
static void additem(UBYTE *obj, UBYTE *a0, UWORD n, int sub, ULONG text, ULONG sc,
                    const ULONG *sp)
{
    UBYTE *it;

    n--;
    if ((WORD)n >= 0)
        do {
            if (!P(a0, 0))
                return;
            a0 = P(a0, 0);
        } while (n-- != 0);
    it = (UBYTE *)galloc(ITEMSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!it)
        return;
    SL(it, 0) = SL(a0, 0);
    SL(a0, 0) = (ULONG)it;
    if ((LONG)text < 0)
        SL(it, 4) = (ULONG)NM_BARLABEL;
    else
        SL(it, 4) = allocst(text, BL_POPL(sp));
    SL(it, 8) = 0;
    SW(it, 12) = flags;
    if ((UWORD)sc) {                    /* BUG(orig) (#694): TST.w */
        if (sub) {
            /* MOVE.l -(a2),d1:MOVE.l -(a2),d0: length and the pushed
               address from the stack */
            ULONG len = BL_POPL(sp);
            SL(it, 14) = allocst(BL_POPL(sp), len);
        } else
            SL(it, 14) = allocst(sc, BL_POPL(sp));
    }
    SL(it, 22) = me;
    SL(it, 26) = userdata;
    SW(obj, 8)++;
}

/* _do_gtitem: d1 flags, d2 menu, d3 item, d4 text, d5 shortcut */
static void do_gtitem(ULONG d1, ULONG d2, ULONG d3, ULONG d4, ULONG d5, const ULONG *sp,
                      UBYTE *obj)
{
    UBYTE *a0 = obj + 4;
    UWORD n = (UWORD)d2;

    if (!gtbase)
        return;
    flags = (UWORD)d1;
    do {
        if (!P(a0, 0))
            return;
        a0 = P(a0, 0);
    } while (n-- != 0);
    additem(obj, a0 + 8, (UWORD)d3, 0, d4, d5, sp);
}
#define gtitem do_gtitem

#define ITEMARGS ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3), \
    ULONG d4 BL_REG(d4), ULONG d5 BL_REG(d5), ULONG d6 BL_REG(d6),               \
    const ULONG *a2 BL_REG(a2), UBYTE *obj BL_REG(a3)

static void BLITZ_ROUTINE gtmenuitem2_c(ITEMARGS)
{
    (void)d4; (void)d5;
    me = 0;
    userdata = 0;
    gtitem(d1, d2, d3, (ULONG)-1, 0, a2, obj);
}
BLITZ_THUNK(_gtmenuitem2, gtmenuitem2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtmenuitem3_c(ITEMARGS)
{
    (void)d5;
    me = 0;
    userdata = 0;
    gtitem(d1, d2, d3, d4, 0, a2, obj);
}
BLITZ_THUNK(_gtmenuitem3, gtmenuitem3_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtmenuitem_c(ITEMARGS)
{
    me = 0;
    userdata = 0;
    gtitem(d1, d2, d3, d4, d5, a2, obj);
}
BLITZ_THUNK(_gtmenuitem, gtmenuitem_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtmenuitem4_c(ITEMARGS)
{
    me = d6;
    userdata = 0;
    gtitem(d1, d2, d3, d4, d5, a2, obj);
}
BLITZ_THUNK(_gtmenuitem4, gtmenuitem4_c, "d0-d1/a0-a1");

/* BUG(orig) (#692): `me` stays from the call before */
static void BLITZ_ROUTINE gtmenuitem5_c(ITEMARGS)
{
    (void)d6;
    userdata = a2[-4];                  /* -16(a2) */
    gtitem(d1, d2, d3, d4, d5, a2, obj);
}
BLITZ_THUNK(_gtmenuitem5, gtmenuitem5_c, "d0-d1/a0-a1");

/* _do_gtsubitem: d1 flags, d2 menu, d3 item, d4 sub, d5 text, d6 shortcut */
static void do_gtsubitem(ULONG d1, ULONG d2, ULONG d3, ULONG d4, ULONG d5, ULONG d6,
                         const ULONG *sp, UBYTE *obj)
{
    UBYTE *a0 = obj + 4;
    UWORD n = (UWORD)d2;

    if (!gtbase)
        return;
    flags = (UWORD)d1;
    do {
        if (!P(a0, 0))
            return;
        a0 = P(a0, 0);
    } while (n-- != 0);
    a0 += 8;
    n = (UWORD)d3;
    do {
        if (!P(a0, 0))
            return;
        a0 = P(a0, 0);
    } while (n-- != 0);
    additem(obj, a0 + 18, (UWORD)d4, 1, d5, d6, sp);
}

#define SUBARGS ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3), \
    ULONG d4 BL_REG(d4), ULONG d5 BL_REG(d5), ULONG d6 BL_REG(d6),             \
    const ULONG *a2 BL_REG(a2), UBYTE *obj BL_REG(a3)

static void BLITZ_ROUTINE gtsubitem2_c(SUBARGS)
{
    (void)d5; (void)d6;
    me = 0;
    userdata = 0;
    do_gtsubitem(d1, d2, d3, d4, (ULONG)-1, 0, a2, obj);
}
BLITZ_THUNK(_gtsubitem2, gtsubitem2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtsubitem3_c(SUBARGS)
{
    (void)d6;
    me = 0;
    userdata = 0;
    do_gtsubitem(d1, d2, d3, d4, d5, 0, a2, obj);
}
BLITZ_THUNK(_gtsubitem3, gtsubitem3_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtsubitem_c(SUBARGS)
{
    me = 0;
    userdata = 0;
    do_gtsubitem(d1, d2, d3, d4, d5, d6, a2, obj);
}
BLITZ_THUNK(_gtsubitem, gtsubitem_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtsubitem4_c(SUBARGS)
{
    userdata = 0;
    me = a2[-4];                        /* -16(a2) */
    do_gtsubitem(d1, d2, d3, d4, d5, d6, a2, obj);
}
BLITZ_THUNK(_gtsubitem4, gtsubitem4_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE gtsubitem5_c(SUBARGS)
{
    userdata = a2[-5];                  /* -20(a2) */
    me = a2[-4];
    do_gtsubitem(d1, d2, d3, d4, d5, d6, a2, obj);
}
BLITZ_THUNK(_gtsubitem5, gtsubitem5_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ freeing */

/* freeitemlist: up to n items (DBRA on the long n - 1) from `it` with
   their strings and sub items */
static void freeitemlist(UBYTE *it, ULONG n)
{
    UBYTE *a1, *sub;
    UWORD d7 = (UWORD)(n - 1);

    for (;;) {
        if ((LONG)SL(it, 4) > 0)
            freest(SL(it, 4));
        if (SL(it, 14))
            freest(SL(it, 14));
        for (sub = P(it, 18); sub; ) {
            if ((LONG)SL(sub, 4) > 0)
                freest(SL(sub, 4));
            if (SL(sub, 14))
                freest(SL(sub, 14));
            a1 = sub;
            sub = P(sub, 0);
            gfree(a1, ITEMSIZE);
        }
        a1 = it;
        it = P(it, 0);
        gfree(a1, ITEMSIZE);
        if (!it || d7-- == 0)
            return;
    }
}

/* the object's free routine (FreeMenus with gtbase unchecked, like the
   original); BUG(orig) (#695): the title strings stay */
static void BLITZ_ROUTINE freemenu_c(UBYTE *obj BL_REG(a3))
{
    UBYTE *t, *next, *items;

    if (P(obj, 0))
        FreeMenus((struct Menu *)P(obj, 0));
    for (t = P(obj, 4); t; t = next) {
        next = P(t, 0);
        items = P(t, 8);
        gfree(t, TITLESIZE);
        if (items)
            freeitemlist(items, 99999);
    }
    SL(obj, 0) = 0;
    SL(obj, 4) = 0;
    SL(obj, 8) = 0;
}
BLITZ_THUNK(freemenu, freemenu_c, "d0-d1/a0-a1");

/* finditem: title d1, item d2 (if d2 >= 0 as a long), sub item d3 (if
   d3 >= 0); *prev = the link that points to it */
static UBYTE *finditem(UBYTE *obj, ULONG d1, ULONG d2, ULONG d3, UBYTE **prev)
{
    UBYTE *a0, *a1;
    UWORD n;

    if (!gtbase || !SL(obj, 4))
        return NULL;
    a0 = obj + 4;
    n = (UWORD)d1;
    do {
        a1 = a0;
        if (!P(a0, 0))
            return NULL;
        a0 = P(a0, 0);
    } while (n-- != 0);
    if ((LONG)d2 >= 0) {
        a0 += 8;
        n = (UWORD)d2;
        do {
            a1 = a0;
            if (!P(a0, 0))
                return NULL;
            a0 = P(a0, 0);
        } while (n-- != 0);
        if ((LONG)d3 >= 0) {
            a0 += 18;
            n = (UWORD)d3;
            do {
                a1 = a0;
                if (!P(a0, 0))
                    return NULL;
                a0 = P(a0, 0);
            } while (n-- != 0);
        }
    }
    *prev = a1;
    return a0;
}

static void BLITZ_ROUTINE freetitle_c(ULONG d1 BL_REG(d1), UBYTE *obj BL_REG(a3))
{
    UBYTE *t, *prev, *items;

    if (!(t = finditem(obj, d1, (ULONG)-1, (ULONG)-1, &prev)))
        return;
    SL(prev, 0) = SL(t, 0);
    items = P(t, 8);
    gfree(t, TITLESIZE);
    if (items)
        freeitemlist(items, 99999);
}
BLITZ_THUNK(_freetitle, freetitle_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE freeitem_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                     UBYTE *obj BL_REG(a3))
{
    UBYTE *it, *prev;

    if (!(it = finditem(obj, d1, d2, (ULONG)-1, &prev)))
        return;
    SL(prev, 0) = SL(it, 0);
    freeitemlist(it, 1);
}
BLITZ_THUNK(_freeitem, freeitem_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE freesubitem_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                        ULONG d3 BL_REG(d3), UBYTE *obj BL_REG(a3))
{
    UBYTE *it, *prev;

    if (!(it = finditem(obj, d1, d2, d3, &prev)))
        return;
    SL(prev, 0) = SL(it, 0);
    freeitemlist(it, 1);
}
BLITZ_THUNK(_freesubitem, freesubitem_c, "d0-d1/a0-a1");

/* ------------------------------------------------ the menu strip's flags */

/* menu d1, item d2 (+18: FirstItem), sub d3 (+28: SubItem) of the strip
   at *obj */
static UBYTE *item(UBYTE *obj, UWORD m, UWORD i)
{
    return walk(walk(obj, m) + 18, i);
}

static void changestate(UBYTE *p, UWORD on, UBYTE bit)
{
    if (on)
        p[13] |= bit;
    else
        p[13] &= (UBYTE)~bit;
}

/* GTMenuState list#,on|off: every menu (MENUENABLED) */
static void BLITZ_ROUTINE domenustate_c(ULONG d1 BL_REG(d1), UBYTE *obj BL_REG(a3))
{
    UBYTE *p;

    for (p = obj; P(p, 0); ) {
        p = P(p, 0);
        changestate(p, (UWORD)d1, 1);
    }
}
BLITZ_THUNK(domenustate, domenustate_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE domenustate2_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                         UBYTE *obj BL_REG(a3))
{
    changestate(walk(obj, (UWORD)d1), (UWORD)d2, 1);
}
BLITZ_THUNK(domenustate2, domenustate2_c, "d0-d1/a0-a1");

/* an item: ITEMENABLED (bit 4 of the low byte of Flags) */
static void BLITZ_ROUTINE domenustate3_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                         ULONG d3 BL_REG(d3), UBYTE *obj BL_REG(a3))
{
    changestate(item(obj, (UWORD)d1, (UWORD)d2), (UWORD)d3, 0x10);
}
BLITZ_THUNK(domenustate3, domenustate3_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE domenustate4_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                         ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4),
                                         UBYTE *obj BL_REG(a3))
{
    changestate(walk(item(obj, (UWORD)d1, (UWORD)d2) + 28, (UWORD)d3), (UWORD)d4, 0x10);
}
BLITZ_THUNK(domenustate4, domenustate4_c, "d0-d1/a0-a1");

/* GTMenuChecked: CHECKED (bit 0 of the high byte of Flags); SNE:EXT.w -
   the upper word of d0 stays */
static ULONG BLITZ_ROUTINE domenuchecked_c(ULONG d0 BL_REG(d0), ULONG d1 BL_REG(d1),
                                           ULONG d2 BL_REG(d2), UBYTE *obj BL_REG(a2))
{
    return BL_SETW(d0, (item(obj, (UWORD)d1, (UWORD)d2)[12] & 1) ? 0xffff : 0);
}
BLITZ_THUNK(domenuchecked, domenuchecked_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE domenuchecked2_c(ULONG d0 BL_REG(d0), ULONG d1 BL_REG(d1),
                                            ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3),
                                            UBYTE *obj BL_REG(a2))
{
    UBYTE *p = walk(item(obj, (UWORD)d1, (UWORD)d2) + 28, (UWORD)d3);

    return BL_SETW(d0, (p[12] & 1) ? 0xffff : 0);
}
BLITZ_THUNK(domenuchecked2, domenuchecked2_c, "d1/a0-a1");

static void setchecked(UBYTE *p, UWORD on)
{
    if (on)
        p[12] |= 1;
    else
        p[12] &= (UBYTE)~1;
}

static void BLITZ_ROUTINE setmenuchecked_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                           ULONG d3 BL_REG(d3), UBYTE *obj BL_REG(a2))
{
    setchecked(item(obj, (UWORD)d1, (UWORD)d2), (UWORD)d3);
}
BLITZ_THUNK(setmenuchecked, setmenuchecked_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE setmenuchecked2_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                            ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4),
                                            UBYTE *obj BL_REG(a2))
{
    setchecked(walk(item(obj, (UWORD)d1, (UWORD)d2) + 28, (UWORD)d3), (UWORD)d4);
}
BLITZ_THUNK(setmenuchecked2, setmenuchecked2_c, "d0-d1/a0-a1");

/* GTGetUserData: the long behind the MenuItem (GadTools' UserData) */
static ULONG getuserdata(UBYTE *obj, ULONG d1, ULONG d2, ULONG d3)
{
    UBYTE *p = item(obj, (UWORD)d1, (UWORD)d2);

    if ((WORD)d3 >= 0)
        p = walk(p + 28, (UWORD)d3);
    return SL(p, MI_SIZEOF);
}

static ULONG BLITZ_ROUTINE getuserdata_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                         UBYTE *obj BL_REG(a2))
{
    return getuserdata(obj, d1, d2, (ULONG)-1);
}
BLITZ_THUNK(_GetUserData, getuserdata_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE getuserdata2_c(ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2),
                                          ULONG d3 BL_REG(d3), UBYTE *obj BL_REG(a2))
{
    return getuserdata(obj, d1, d2, d3);
}
BLITZ_THUNK(_GetUserData2, getuserdata2_c, "d1/a0-a1");

/* -------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Menu number");
BLITZ_ERRCHK_MESSAGE(mess2, "Illegal Menu Item number");
BLITZ_ERRCHK_MESSAGE(mess3, "Illegal Menu Sub Item number");
BLITZ_ERRCHK_MESSAGE(mess8, "GTMenu has no data to build GTMenuList from!");

/* createchk, setchk: the object has a list */
static void BLITZ_ERRCHK listchk_c(UBYTE *obj BL_REG(a3))
{
    if (!SL(obj, 4))
        BLITZ_RUNERR(mess8);
}
BLITZ_ERRCHK_THUNK(createchk, listchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(setchk, listchk_c, "d0-d1/a0-a1");

/* existschk: entries 0..n of the list at *a0 exist (DBF); *a0 moves to
   entry n, or stays at the last one there is.  0 if one is missing. */
static int BLITZ_ERRCHK existschk(UBYTE **a0, UWORD n)
{
    UBYTE *p = *a0;

    do {
        if (!P(p, 0)) {
            *a0 = p;
            return 0;
        }
        p = P(p, 0);
    } while (n-- != 0);
    *a0 = p;
    return 1;
}

/* statechk2/3/4: menu d1, item d2 (+18), sub item d3 (+28) exist in the
   strip of the object */
static void BLITZ_ERRCHK statechk(UBYTE *obj, int depth, UWORD d1, UWORD d2, UWORD d3)
{
    UBYTE *a0 = obj;

    if (!existschk(&a0, d1)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    if (depth < 3)
        return;
    a0 += 18;
    if (!existschk(&a0, d2)) {
        BLITZ_RUNERR(mess2);
        return;
    }
    if (depth < 4)
        return;
    a0 += 28;
    if (!existschk(&a0, d3))
        BLITZ_RUNERR(mess3);
}

#define CHKARGS ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3)

static void BLITZ_ERRCHK statechk2_c(CHKARGS, UBYTE *obj BL_REG(a3))
{
    statechk(obj, 2, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(statechk2, statechk2_c, "d0-d1/a0-a1");

static void BLITZ_ERRCHK statechk3_c(CHKARGS, UBYTE *obj BL_REG(a3))
{
    statechk(obj, 3, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(statechk3, statechk3_c, "d0-d1/a0-a1");

static void BLITZ_ERRCHK statechk4_c(CHKARGS, UBYTE *obj BL_REG(a3))
{
    statechk(obj, 4, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(statechk4, statechk4_c, "d0-d1/a0-a1");

/* chkchk3/4: the same with the object in a2 (EXG a2,a3); chkchk5: chkchk3
   if the sub item is -1 (CMP.w), else chkchk4 */
static void BLITZ_ERRCHK chkchk3_c(CHKARGS, UBYTE *obj BL_REG(a2))
{
    statechk(obj, 3, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(chkchk3, chkchk3_c, "d0-d1/a0-a1");

static void BLITZ_ERRCHK chkchk4_c(CHKARGS, UBYTE *obj BL_REG(a2))
{
    statechk(obj, 4, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(chkchk4, chkchk4_c, "d0-d1/a0-a1");

static void BLITZ_ERRCHK chkchk5_c(CHKARGS, UBYTE *obj BL_REG(a2))
{
    statechk(obj, (UWORD)d3 == 0xffff ? 3 : 4, (UWORD)d1, (UWORD)d2, (UWORD)d3);
}
BLITZ_ERRCHK_THUNK(chkchk5, chkchk5_c, "d0-d1/a0-a1");
