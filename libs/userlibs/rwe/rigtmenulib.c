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

/* the object GTMenuList (16 bytes) */
struct gtmenulist {
    struct Menu *strip;                 /* 0: made by CreateMenuStrip */
    struct gtentry *titles;             /* 4: the list of titles */
    UWORD count;                        /* 8: the number of entries added */
    UWORD rest[3];                      /* 10: unused (CLR.l 8(a3) clears 10) */
};
static_assert(sizeof(struct gtmenulist) == 16);

/* a title (TITLESIZE, the first three fields) or an item/sub item
   (ITEMSIZE, cleared); the strings are maxslib strings */
struct gtentry {
    struct gtentry *next;               /* 0 */
    ULONG text;                         /* 4: an item's -1 = NM_BARLABEL */
    struct gtentry *items;              /* 8: a title's first item; 0 */
    UWORD flags;                        /* 12 */
    ULONG shortcut;                     /* 14 */
    struct gtentry *subs;               /* 18: an item's first sub item */
    ULONG mutualexclude;                /* 22 */
    ULONG userdata;                     /* 26 */
};
#define TITLESIZE 12
#define ITEMSIZE 30
static_assert(__builtin_offsetof(struct gtentry, flags) == TITLESIZE);
static_assert(sizeof(struct gtentry) == ITEMSIZE);
static_assert(sizeof(struct NewMenu) == 20);

/* a list head (the object's titles, a title's items, an item's subs) as
   an entry whose next is the head, as the original walks the lists
   (MOVE.l (aN),aN from the head on) */
#define HEAD(head) ((struct gtentry *)&(head))

/* the bytes of Intuition's Menu/MenuItem Flags, as the original changes
   them (BSET/BCLR/BTST on a byte): the high byte (CHECKED) and the low
   byte (MENUENABLED, ITEMENABLED) */
#define FLAGS_HI(f) (((UBYTE *)&(f))[0])
#define FLAGS_LO(f) (((UBYTE *)&(f))[1])

static struct Library *gtbase, *intuibase;
static ULONG me, userdata;              /* mutual exclude, UserData */
static UWORD flags;
static ULONG marraysize;
static struct NewMenu *marray;
static APTR vi;
static const char gtname[] = "gadtools.library";
static const struct TagItem taglist[] = { { GTMN_FullMenu, (ULONG)-1 }, { 0, 0 } };
static const struct TagItem tl_layout[] = { { GTMN_NewLookMenus, (ULONG)-1 }, { 0, 0 } };

static ULONG galloc(ULONG size, ULONG fl)
{
    struct bl_regs r = { .d0 = size, .d1 = fl };
    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void gfree(APTR mem, ULONG size)
{
    struct bl_regs r = { .d1 = size, .a1 = (ULONG)mem };
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

/* n + 1 links on from the link at `slot` (MOVE.l (aN),aN:DBF n) of the
   menu strip: Menu and MenuItem have their link (NextMenu, NextItem, as
   the list heads FirstItem, SubItem) at offset 0 */
static APTR walk(APTR *slot, UWORD n)
{
    APTR p;

    do {
        p = *slot;
        slot = (APTR *)p;
    } while (n-- != 0);
    return p;
}

/* ------------------------------------------------------- init / finit */

/**
 * @brief init: open gadtools.library (any version).
 *
 * Without it the list commands and CreateMenuStrip do nothing.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    gtbase = OpenLibrary((CONST_STRPTR)gtname, 0);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/**
 * @brief finit: close gadtools.library.
 *
 * BUG(orig) (#690): it closes only if the library is NOT open (TST.l:BNE
 * 'ok), so never.
 */
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
static struct NewMenu *makearray(struct gtmenulist *obj)
{
    struct gtentry *title = obj->titles, *it, *sub;
    struct NewMenu *entry;

    marraysize = bl_mulu_w((UWORD)(obj->count + 1), sizeof(struct NewMenu));
    marray = entry = (struct NewMenu *)galloc(marraysize, MEMF_PUBLIC | MEMF_CLEAR);
    for (;;) {
        entry->nm_Type = NM_TITLE;
        entry->nm_Label = (STRPTR)title->text;
        entry++;
        for (it = title->items; it; it = it->next) {
            entry->nm_Type = NM_ITEM;
            entry->nm_Label = (STRPTR)it->text;
            entry->nm_CommKey = (STRPTR)it->shortcut;
            entry->nm_Flags = it->flags;
            entry->nm_MutualExclude = it->mutualexclude;
            entry->nm_UserData = (APTR)it->userdata;
            entry++;
            for (sub = it->subs; sub; sub = sub->next) {
                entry->nm_Type = NM_SUB;
                entry->nm_Label = (STRPTR)sub->text;
                entry->nm_CommKey = (STRPTR)sub->shortcut;
                entry->nm_Flags = sub->flags;
                entry->nm_MutualExclude = sub->mutualexclude;
                entry->nm_UserData = (APTR)sub->userdata;
                entry++;
            }
        }
        if (!(title = title->next))
            break;
    }
    entry->nm_Type = NM_END;
    return marray;
}

/* CreateMenuStrip: a3 = the object, a0 = the used screen object */
static BB_BOOL createmenustrip(struct gtmenulist *obj, struct Screen **scr)
{
    struct NewMenu *arr;

    if (!gtbase)
        return BB_FALSE;
    if (obj->strip) {
        FreeMenus(obj->strip);
        obj->strip = NULL;
    }
    if (!(arr = makearray(obj)))
        return BB_FALSE;
    obj->strip = CreateMenusA(arr, (struct TagItem *)taglist);
    vi = GetVisualInfoA(*scr, NULL);
    LayoutMenusA(obj->strip, vi, (struct TagItem *)tl_layout);
    FreeVisualInfo(vi);
    gfree(marray, marraysize);
    return BB_TRUE;
}

/**
 * @brief CreateMenuStrip GTMenuList#: make the menu strip of the object's
 * lists (gadtools CreateMenusA, LayoutMenusA with new look menus).
 *
 * An old strip is freed first.  The NewMenu array ((count + 1) * 20
 * bytes, memlib) is freed again afterwards.  BUG(orig) (#689): no title
 * reads the list at address 0, a failed allocation writes it to address
 * 0; CreateMenusA and GetVisualInfoA are not checked.  Error check
 * (createchk): "GTMenu has no data to build GTMenuList from!".
 * @param scr  a0: the used screen object (screenslib), its first long the
 *             Screen
 * @param obj  a3: the GTMenuList object (#ia3|#pd0 of list#)
 * @return d0: BB_TRUE; BB_FALSE without gadtools.library or if the
 *         array could not be allocated
 */
static BB_BOOL BLITZ_ROUTINE createmenustrip_c(struct Screen **scr BL_REG(a0),
                                               struct gtmenulist *obj BL_REG(a3))
{
    return createmenustrip(obj, scr);
}
BLITZ_THUNK(_CreateMenuStrip, createmenustrip_c, "d1/a0-a1");

/**
 * @brief GTSetMenu GTMenuList#: attach the object's menu strip to the used
 * window.
 *
 * Without a strip, CreateMenuStrip makes it first (nothing if that
 * fails); a strip already on the window is cleared first.
 * @param scr    a0: the used screen object
 * @param win    a1: the used window object, its first long the Window
 * @param obj    a3: the GTMenuList object
 * @param intui  a6: intuition.library
 */
static void BLITZ_ROUTINE setmenu_c(struct Screen **scr BL_REG(a0), struct Window **win BL_REG(a1),
                                    struct gtmenulist *obj BL_REG(a3),
                                    struct Library *intui BL_REG(a6))
{
    struct Window *w;

    intuibase = intui;
    if (!obj->strip) {
        createmenustrip(obj, scr);
        if (!obj->strip)
            return;
    }
    w = *win;
    if (w->MenuStrip)
        ClearMenuStrip(w);
    SetMenuStrip(w, obj->strip);
}
BLITZ_THUNK(_setmenu, setmenu_c, "d0-d1/a0-a1");

/**
 * @brief GTUnSetMenu: remove the menu strip from the used window (if it
 * has one).
 * @param win    a2: the used window object
 * @param intui  a6: intuition.library
 */
static void BLITZ_ROUTINE gtunsetmenu_c(struct Window **win BL_REG(a2),
                                        struct Library *intui BL_REG(a6))
{
    intuibase = intui;
    if ((*win)->MenuStrip)
        ClearMenuStrip(*win);
}
BLITZ_THUNK(_gtunsetmenu, gtunsetmenu_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- the lists */

/**
 * @brief GTMenuTitle GTMenuList#,menu,title$: insert a menu title as
 * title `menu` of the list.
 *
 * Nothing without gadtools.library or if title menu - 1 is missing.  The
 * title (12 bytes) is allocated uncleared (MEMF_PUBLIC; all its fields
 * are written, no defect), the text copied with maxslib AllocSt.
 * @param menu  d1.w: the title's number
 * @param text  d2: title$
 * @param args  a2: the argument stack, -4(a2) the length of title$
 * @param obj   a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenutitle_c(ULONG menu BL_REG(d1), ULONG text BL_REG(d2),
                                        const ULONG *args BL_REG(a2),
                                        struct gtmenulist *obj BL_REG(a3))
{
    struct gtentry *link = HEAD(obj->titles), *title;
    UWORD n;

    if (!gtbase)
        return;
    n = (UWORD)menu - 1;
    if ((WORD)n >= 0)
        do {
            if (!link->next)
                return;
            link = link->next;
        } while (n-- != 0);
    title = (struct gtentry *)galloc(TITLESIZE, MEMF_PUBLIC); /* uncleared, all written */
    if (!title)
        return;
    title->next = link->next;
    link->next = title;
    title->text = allocst(text, args[-1]);
    title->items = NULL;
    obj->count++;
}
BLITZ_THUNK(_gtmenutitle, gtmenutitle_c, "d0-d1/a0-a1");

/* a new item or sub item after entry n - 1 of the list at link (n = 0: at
   the head) of the object obj (+8 counts it); text < 0: NM_BARLABEL; the
   lengths popped from the a2 stack in order (`sp`); the shortcut: `sc`
   and its length, or for a sub item (`sub`) the length and the pushed
   address from the stack */
static void additem(struct gtmenulist *obj, struct gtentry *link, UWORD n, int sub, ULONG text,
                    ULONG sc, const ULONG *sp)
{
    struct gtentry *it;

    n--;
    if ((WORD)n >= 0)
        do {
            if (!link->next)
                return;
            link = link->next;
        } while (n-- != 0);
    it = (struct gtentry *)galloc(ITEMSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!it)
        return;
    it->next = link->next;
    link->next = it;
    if ((LONG)text < 0)
        it->text = (ULONG)NM_BARLABEL;
    else
        it->text = allocst(text, BL_POPL(sp));
    it->items = NULL;
    it->flags = flags;
    if ((UWORD)sc) {                    /* BUG(orig) (#694): TST.w */
        if (sub) {
            /* MOVE.l -(a2),d1:MOVE.l -(a2),d0: length and the pushed
               address from the stack */
            ULONG len = BL_POPL(sp);
            it->shortcut = allocst(BL_POPL(sp), len);
        } else
            it->shortcut = allocst(sc, BL_POPL(sp));
    }
    it->mutualexclude = me;
    it->userdata = userdata;
    obj->count++;
}

/* _do_gtitem: d1 flags, d2 menu, d3 item, d4 text, d5 shortcut */
static void do_gtitem(ULONG itemflags, ULONG menu, ULONG itemno, ULONG text, ULONG shortcut,
                      const ULONG *sp, struct gtmenulist *obj)
{
    struct gtentry *link = HEAD(obj->titles);
    UWORD n = (UWORD)menu;

    if (!gtbase)
        return;
    flags = (UWORD)itemflags;
    do {
        if (!link->next)
            return;
        link = link->next;
    } while (n-- != 0);
    additem(obj, HEAD(link->items), (UWORD)itemno, 0, text, shortcut, sp);
}
#define gtitem do_gtitem

#define ITEMARGS ULONG itemflags BL_REG(d1), ULONG menu BL_REG(d2), ULONG itemno BL_REG(d3), \
    ULONG text BL_REG(d4), ULONG shortcut BL_REG(d5), ULONG exclude BL_REG(d6),              \
    const ULONG *stack BL_REG(a2), struct gtmenulist *obj BL_REG(a3)

/**
 * @brief GTMenuItem GTMenuList#,flags,menu,item: insert a bar label
 * (NM_BARLABEL) as item `item` of title `menu`.
 *
 * Mutual exclude and UserData 0.  Nothing without gadtools.library or if
 * the title or item - 1 is missing.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item's number
 * @param stack      a2: the argument stack
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenuitem2_c(ITEMARGS)
{
    (void)text; (void)shortcut;
    me = 0;
    userdata = 0;
    gtitem(itemflags, menu, itemno, (ULONG)-1, 0, stack, obj);
}
BLITZ_THUNK(_gtmenuitem2, gtmenuitem2_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuItem GTMenuList#,flags,menu,item,itemtext$: insert an item
 * without a shortcut.
 *
 * As the form without text; the text is copied with AllocSt.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item's number
 * @param text       d4: itemtext$
 * @param stack      a2: the argument stack (the string lengths)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenuitem3_c(ITEMARGS)
{
    (void)shortcut;
    me = 0;
    userdata = 0;
    gtitem(itemflags, menu, itemno, text, 0, stack, obj);
}
BLITZ_THUNK(_gtmenuitem3, gtmenuitem3_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuItem GTMenuList#,flags,menu,item,itemtext$,shortcut$:
 * insert an item with a shortcut.
 *
 * BUG(orig) (#694): shortcut$ is tested by its low word, a string at an
 * address with low word 0 counts as none.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item's number
 * @param text       d4: itemtext$
 * @param shortcut   d5: shortcut$
 * @param stack      a2: the argument stack (the string lengths)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenuitem_c(ITEMARGS)
{
    me = 0;
    userdata = 0;
    gtitem(itemflags, menu, itemno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtmenuitem, gtmenuitem_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuItem ...,itemtext$,shortcut$,mutualexclude: insert an item
 * with a mutual exclude mask.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item's number
 * @param text       d4: itemtext$
 * @param shortcut   d5: shortcut$ (#694)
 * @param exclude    d6: the mutual exclude mask
 * @param stack      a2: the argument stack (the string lengths)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenuitem4_c(ITEMARGS)
{
    me = exclude;
    userdata = 0;
    gtitem(itemflags, menu, itemno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtmenuitem4, gtmenuitem4_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuItem ...,itemtext$,shortcut$,mutualexclude,UserData:
 * insert an item with UserData.
 *
 * BUG(orig) (#692): the mutual exclude is not taken (it jumps past MOVE.l
 * d6,me): it stays from the GTMenuItem/GTSubItem before.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item's number
 * @param text       d4: itemtext$
 * @param shortcut   d5: shortcut$ (#694)
 * @param exclude    d6: the mutual exclude mask (ignored, #692)
 * @param stack      a2: the argument stack, UserData at -16(a2)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtmenuitem5_c(ITEMARGS)
{
    (void)exclude;
    userdata = stack[-4];               /* -16(a2) */
    gtitem(itemflags, menu, itemno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtmenuitem5, gtmenuitem5_c, "d0-d1/a0-a1");

/* _do_gtsubitem: d1 flags, d2 menu, d3 item, d4 sub, d5 text, d6 shortcut */
static void do_gtsubitem(ULONG itemflags, ULONG menu, ULONG itemno, ULONG subno, ULONG text,
                         ULONG shortcut, const ULONG *sp, struct gtmenulist *obj)
{
    struct gtentry *link = HEAD(obj->titles);
    UWORD n = (UWORD)menu;

    if (!gtbase)
        return;
    flags = (UWORD)itemflags;
    do {
        if (!link->next)
            return;
        link = link->next;
    } while (n-- != 0);
    link = HEAD(link->items);
    n = (UWORD)itemno;
    do {
        if (!link->next)
            return;
        link = link->next;
    } while (n-- != 0);
    additem(obj, HEAD(link->subs), (UWORD)subno, 1, text, shortcut, sp);
}

#define SUBARGS ULONG itemflags BL_REG(d1), ULONG menu BL_REG(d2), ULONG itemno BL_REG(d3), \
    ULONG subno BL_REG(d4), ULONG text BL_REG(d5), ULONG shortcut BL_REG(d6),            \
    const ULONG *stack BL_REG(a2), struct gtmenulist *obj BL_REG(a3)

/**
 * @brief GTSubItem GTMenuList#,flags,menu,item,subitem: insert a bar label
 * as sub item `subitem` of item `item` of title `menu`.
 *
 * Mutual exclude and UserData 0.  Nothing without gadtools.library or if
 * the title, the item or subitem - 1 is missing.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item
 * @param subno      d4.w: the sub item's number
 * @param stack      a2: the argument stack
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtsubitem2_c(SUBARGS)
{
    (void)text; (void)shortcut;
    me = 0;
    userdata = 0;
    do_gtsubitem(itemflags, menu, itemno, subno, (ULONG)-1, 0, stack, obj);
}
BLITZ_THUNK(_gtsubitem2, gtsubitem2_c, "d0-d1/a0-a1");

/**
 * @brief GTSubItem GTMenuList#,flags,menu,item,subitem,itemtext$: insert a
 * sub item without a shortcut.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item
 * @param subno      d4.w: the sub item's number
 * @param text       d5: itemtext$
 * @param stack      a2: the argument stack (the string lengths)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtsubitem3_c(SUBARGS)
{
    (void)shortcut;
    me = 0;
    userdata = 0;
    do_gtsubitem(itemflags, menu, itemno, subno, text, 0, stack, obj);
}
BLITZ_THUNK(_gtsubitem3, gtsubitem3_c, "d0-d1/a0-a1");

/**
 * @brief GTSubItem ...,itemtext$,shortcut$: insert a sub item with a
 * shortcut.
 *
 * shortcut$ is a #push argument: its address and length are taken from
 * the a2 stack (the d6 value only decides whether there is one, by its
 * low word, BUG(orig) #694).
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item
 * @param subno      d4.w: the sub item's number
 * @param text       d5: itemtext$
 * @param shortcut   d6: tested for 0 (TST.w)
 * @param stack      a2: the argument stack (the string lengths, the
 *                   pushed shortcut$)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtsubitem_c(SUBARGS)
{
    me = 0;
    userdata = 0;
    do_gtsubitem(itemflags, menu, itemno, subno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtsubitem, gtsubitem_c, "d0-d1/a0-a1");

/**
 * @brief GTSubItem ...,itemtext$,shortcut$,mutualexclude: insert a sub
 * item with a mutual exclude mask.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item
 * @param subno      d4.w: the sub item's number
 * @param text       d5: itemtext$
 * @param shortcut   d6: shortcut$ (#694)
 * @param stack      a2: the argument stack, the mutual exclude at -16(a2)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtsubitem4_c(SUBARGS)
{
    userdata = 0;
    me = stack[-4];                     /* -16(a2) */
    do_gtsubitem(itemflags, menu, itemno, subno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtsubitem4, gtsubitem4_c, "d0-d1/a0-a1");

/**
 * @brief GTSubItem ...,itemtext$,shortcut$,mutualexclude,UserData: insert
 * a sub item with a mutual exclude mask and UserData.
 * @param itemflags  d1.w: the NewMenu flags
 * @param menu       d2.w: the title
 * @param itemno     d3.w: the item
 * @param subno      d4.w: the sub item's number
 * @param text       d5: itemtext$
 * @param shortcut   d6: shortcut$ (#694)
 * @param stack      a2: the argument stack, the mutual exclude at -16(a2),
 *                   UserData at -20(a2)
 * @param obj        a3: the GTMenuList object
 */
static void BLITZ_ROUTINE gtsubitem5_c(SUBARGS)
{
    userdata = stack[-5];               /* -20(a2) */
    me = stack[-4];
    do_gtsubitem(itemflags, menu, itemno, subno, text, shortcut, stack, obj);
}
BLITZ_THUNK(_gtsubitem5, gtsubitem5_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ freeing */

/* freeitemlist: up to n items (DBRA on the long n - 1) from `it` with
   their strings and sub items */
static void freeitemlist(struct gtentry *it, ULONG n)
{
    struct gtentry *entry, *sub;
    UWORD left = (UWORD)(n - 1);        /* d7 */

    for (;;) {
        if ((LONG)it->text > 0)
            freest(it->text);
        if (it->shortcut)
            freest(it->shortcut);
        for (sub = it->subs; sub; ) {
            if ((LONG)sub->text > 0)
                freest(sub->text);
            if (sub->shortcut)
                freest(sub->shortcut);
            entry = sub;
            sub = sub->next;
            gfree(entry, ITEMSIZE);
        }
        entry = it;
        it = it->next;
        gfree(entry, ITEMSIZE);
        if (!it || left-- == 0)
            return;
    }
}

/**
 * @brief The free routine of a GTMenuList object: the menu strip, the
 * titles and their items.
 *
 * FreeMenus with the gadtools base unchecked, like the original (a strip
 * exists only if it was open).  BUG(orig) (#695): the title strings are
 * never freed.  The object is cleared (+0, +4, the long at +8).
 * @param obj  a3: the object
 */
static void BLITZ_ROUTINE freemenu_c(struct gtmenulist *obj BL_REG(a3))
{
    struct gtentry *title, *next, *items;

    if (obj->strip)
        FreeMenus(obj->strip);
    for (title = obj->titles; title; title = next) {
        next = title->next;
        items = title->items;
        gfree(title, TITLESIZE);
        if (items)
            freeitemlist(items, 99999);
    }
    obj->strip = NULL;
    obj->titles = NULL;
    *(ULONG *)&obj->count = 0;          /* CLR.l 8(a3): count and +10 */
}
BLITZ_THUNK(freemenu, freemenu_c, "d0-d1/a0-a1");

/* finditem: title d1, item d2 (if d2 >= 0 as a long), sub item d3 (if
   d3 >= 0); *prev = the link that points to it */
static struct gtentry *finditem(struct gtmenulist *obj, ULONG menu, ULONG itemno, ULONG subno,
                                struct gtentry **prev)
{
    struct gtentry *entry, *link;
    UWORD n;

    if (!gtbase || !*(ULONG *)&obj->titles)     /* TST.l: no list (as a long) */
        return NULL;
    entry = HEAD(obj->titles);
    n = (UWORD)menu;
    do {
        link = entry;
        if (!entry->next)
            return NULL;
        entry = entry->next;
    } while (n-- != 0);
    if ((LONG)itemno >= 0) {
        entry = HEAD(entry->items);
        n = (UWORD)itemno;
        do {
            link = entry;
            if (!entry->next)
                return NULL;
            entry = entry->next;
        } while (n-- != 0);
        if ((LONG)subno >= 0) {
            entry = HEAD(entry->subs);
            n = (UWORD)subno;
            do {
                link = entry;
                if (!entry->next)
                    return NULL;
                entry = entry->next;
            } while (n-- != 0);
        }
    }
    *prev = link;
    return entry;
}

/**
 * @brief GTFreeMenu GTMenuList#,menu: remove a title with its items from
 * the list.
 *
 * Nothing without gadtools.library or if it is missing.  BUG(orig)
 * (#695): its string stays.  The menu strip is not changed (it is made
 * again by CreateMenuStrip).
 * @param menu  d1.w: the title
 * @param obj   a3: the GTMenuList object
 */
static void BLITZ_ROUTINE freetitle_c(ULONG menu BL_REG(d1), struct gtmenulist *obj BL_REG(a3))
{
    struct gtentry *title, *prev, *items;

    if (!(title = finditem(obj, menu, (ULONG)-1, (ULONG)-1, &prev)))
        return;
    prev->next = title->next;
    items = title->items;
    gfree(title, TITLESIZE);
    if (items)
        freeitemlist(items, 99999);
}
BLITZ_THUNK(_freetitle, freetitle_c, "d0-d1/a0-a1");

/**
 * @brief GTFreeMenu GTMenuList#,menu,item: remove an item with its sub
 * items and strings from the list.
 *
 * BUG(orig) (#691): the item number is tested as a long (TST.l): the
 * upper word of the word argument decides whether it is given.
 * @param menu    d1.w: the title
 * @param itemno  d2: the item
 * @param obj     a3: the GTMenuList object
 */
static void BLITZ_ROUTINE freeitem_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                     struct gtmenulist *obj BL_REG(a3))
{
    struct gtentry *it, *prev;

    if (!(it = finditem(obj, menu, itemno, (ULONG)-1, &prev)))
        return;
    prev->next = it->next;
    freeitemlist(it, 1);
}
BLITZ_THUNK(_freeitem, freeitem_c, "d0-d1/a0-a1");

/**
 * @brief GTFreeMenu GTMenuList#,menu,item,subitem: remove a sub item with
 * its strings from the list.
 *
 * BUG(orig) (#691): the numbers are tested as longs (TST.l).
 * @param menu    d1.w: the title
 * @param itemno  d2: the item
 * @param subno   d3: the sub item
 * @param obj     a3: the GTMenuList object
 */
static void BLITZ_ROUTINE freesubitem_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                        ULONG subno BL_REG(d3), struct gtmenulist *obj BL_REG(a3))
{
    struct gtentry *it, *prev;

    if (!(it = finditem(obj, menu, itemno, subno, &prev)))
        return;
    prev->next = it->next;
    freeitemlist(it, 1);
}
BLITZ_THUNK(_freesubitem, freesubitem_c, "d0-d1/a0-a1");

/* ------------------------------------------------ the menu strip's flags */

/* the menu `menu` of the strip, its item `itemno`, an item's sub item
   `subno` */
#define MENU(obj, menu) ((struct Menu *)walk((APTR *)&(obj)->strip, (menu)))
#define SUBITEM(mi, subno) ((struct MenuItem *)walk((APTR *)&(mi)->SubItem, (subno)))
static struct MenuItem *item(struct gtmenulist *obj, UWORD menu, UWORD itemno)
{
    return walk((APTR *)&MENU(obj, menu)->FirstItem, itemno);
}

/* the Flags of a Menu or MenuItem (both at 12) */
static_assert(__builtin_offsetof(struct Menu, Flags) == __builtin_offsetof(struct MenuItem, Flags));
static void changestate(struct Menu *m, UWORD on, UBYTE bit)
{
    if (on)
        FLAGS_LO(m->Flags) |= bit;
    else
        FLAGS_LO(m->Flags) &= (UBYTE)~bit;
}

/**
 * @brief GTMenuState GTMenuList#,On|Off: enable or disable every menu of
 * the strip (MENUENABLED).
 *
 * BUG(orig) (#693): the flags are changed directly, also while the strip
 * is attached to a window.
 * @param on   d1.w: 0 off, else on
 * @param obj  a3: the GTMenuList object (0x1380: #ia3)
 */
static void BLITZ_ROUTINE domenustate_c(ULONG on BL_REG(d1), struct gtmenulist *obj BL_REG(a3))
{
    APTR *p;

    for (p = (APTR *)&obj->strip; *p; ) {
        p = (APTR *)*p;
        changestate((struct Menu *)p, (UWORD)on, MENUENABLED);
    }
}
BLITZ_THUNK(domenustate, domenustate_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuState GTMenuList#,menu,On|Off: enable or disable a menu
 * (MENUENABLED).
 *
 * The menu is not checked (a missing one reads address 0; error check
 * statechk2: "Illegal Menu number").  BUG(orig) (#693) as above.
 * @param menu  d1.w: the menu
 * @param on    d2.w: 0 off, else on
 * @param obj   a3: the GTMenuList object
 */
static void BLITZ_ROUTINE domenustate2_c(ULONG menu BL_REG(d1), ULONG on BL_REG(d2),
                                         struct gtmenulist *obj BL_REG(a3))
{
    changestate(MENU(obj, (UWORD)menu), (UWORD)on, MENUENABLED);
}
BLITZ_THUNK(domenustate2, domenustate2_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuState GTMenuList#,menu,item,On|Off: enable or disable an
 * item (ITEMENABLED, bit 4 of the low byte of Flags).
 *
 * Not checked (error check statechk3); BUG(orig) (#693) as above.
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param on      d3.w: 0 off, else on
 * @param obj     a3: the GTMenuList object
 */
static void BLITZ_ROUTINE domenustate3_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                         ULONG on BL_REG(d3), struct gtmenulist *obj BL_REG(a3))
{
    changestate((struct Menu *)item(obj, (UWORD)menu, (UWORD)itemno), (UWORD)on, ITEMENABLED);
}
BLITZ_THUNK(domenustate3, domenustate3_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuState GTMenuList#,menu,item,subitem,On|Off: enable or
 * disable a sub item (ITEMENABLED).
 *
 * Not checked (error check statechk4); BUG(orig) (#693) as above.
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param subno   d3.w: the sub item
 * @param on      d4.w: 0 off, else on
 * @param obj     a3: the GTMenuList object
 */
static void BLITZ_ROUTINE domenustate4_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                         ULONG subno BL_REG(d3), ULONG on BL_REG(d4),
                                         struct gtmenulist *obj BL_REG(a3))
{
    changestate((struct Menu *)SUBITEM(item(obj, (UWORD)menu, (UWORD)itemno), (UWORD)subno),
                (UWORD)on, ITEMENABLED);
}
BLITZ_THUNK(domenustate4, domenustate4_c, "d0-d1/a0-a1");

/**
 * @brief GTMenuChecked(GTMenuList#,Menu,Item): whether an item of the
 * strip is checked (CHECKED, bit 0 of the high byte of Flags).
 *
 * Not checked (error check chkchk3).
 * @param result  d0: its upper word stays (SNE:EXT.w, a word result)
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param obj     a2: the GTMenuList object (0x1280: #ia2)
 * @return d0.w: -1 if checked, else 0
 */
static ULONG BLITZ_ROUTINE domenuchecked_c(ULONG result BL_REG(d0), ULONG menu BL_REG(d1),
                                           ULONG itemno BL_REG(d2),
                                           struct gtmenulist *obj BL_REG(a2))
{
    return BL_SETW(result,
                   (FLAGS_HI(item(obj, (UWORD)menu, (UWORD)itemno)->Flags) & (CHECKED >> 8))
                       ? 0xffff : 0);
}
BLITZ_THUNK(domenuchecked, domenuchecked_c, "d1/a0-a1");

/**
 * @brief GTMenuChecked(GTMenuList#,Menu,Item,Subitem): whether a sub item
 * is checked.
 *
 * Not checked (error check chkchk4).
 * @param result  d0: its upper word stays
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param subno   d3.w: the sub item
 * @param obj     a2: the GTMenuList object
 * @return d0.w: -1 if checked, else 0
 */
static ULONG BLITZ_ROUTINE domenuchecked2_c(ULONG result BL_REG(d0), ULONG menu BL_REG(d1),
                                            ULONG itemno BL_REG(d2), ULONG subno BL_REG(d3),
                                            struct gtmenulist *obj BL_REG(a2))
{
    struct MenuItem *mi = SUBITEM(item(obj, (UWORD)menu, (UWORD)itemno), (UWORD)subno);

    return BL_SETW(result, (FLAGS_HI(mi->Flags) & (CHECKED >> 8)) ? 0xffff : 0);
}
BLITZ_THUNK(domenuchecked2, domenuchecked2_c, "d1/a0-a1");

static void setchecked(struct MenuItem *mi, UWORD on)
{
    if (on)
        FLAGS_HI(mi->Flags) |= CHECKED >> 8;
    else
        FLAGS_HI(mi->Flags) &= (UBYTE)~(CHECKED >> 8);
}

/**
 * @brief GTSetMenuChecked GTMenuList#,Menu,Item,Off|On: check or uncheck
 * an item of the strip (CHECKED).
 *
 * Not checked (error check chkchk3).  BUG(orig) (#693): directly, also
 * while the strip is attached to a window.
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param on      d3.w: 0 off, else on
 * @param obj     a2: the GTMenuList object
 */
static void BLITZ_ROUTINE setmenuchecked_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                           ULONG on BL_REG(d3), struct gtmenulist *obj BL_REG(a2))
{
    setchecked(item(obj, (UWORD)menu, (UWORD)itemno), (UWORD)on);
}
BLITZ_THUNK(setmenuchecked, setmenuchecked_c, "d0-d1/a0-a1");

/**
 * @brief GTSetMenuChecked GTMenuList#,Menu,Item,Subitem,Off|On: check or
 * uncheck a sub item.
 *
 * Not checked (error check chkchk4).  BUG(orig) (#693) as above.
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param subno   d3.w: the sub item
 * @param on      d4.w: 0 off, else on
 * @param obj     a2: the GTMenuList object
 */
static void BLITZ_ROUTINE setmenuchecked2_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                            ULONG subno BL_REG(d3), ULONG on BL_REG(d4),
                                            struct gtmenulist *obj BL_REG(a2))
{
    setchecked(SUBITEM(item(obj, (UWORD)menu, (UWORD)itemno), (UWORD)subno), (UWORD)on);
}
BLITZ_THUNK(setmenuchecked2, setmenuchecked2_c, "d0-d1/a0-a1");

/* GTGetUserData: the long behind the MenuItem (GadTools' UserData) */
static ULONG getuserdata(struct gtmenulist *obj, ULONG menu, ULONG itemno, ULONG subno)
{
    struct MenuItem *mi = item(obj, (UWORD)menu, (UWORD)itemno);

    if ((WORD)subno >= 0)
        mi = SUBITEM(mi, (UWORD)subno);
    return (ULONG)GTMENUITEM_USERDATA(mi);
}

/**
 * @brief GTGetUserData(GTMenuList#,Menu,Item): the UserData of an item of
 * the strip (GadTools' long behind the MenuItem).
 *
 * Not checked (error check chkchk3).
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param obj     a2: the GTMenuList object
 * @return d0: the UserData
 */
static ULONG BLITZ_ROUTINE getuserdata_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                         struct gtmenulist *obj BL_REG(a2))
{
    return getuserdata(obj, menu, itemno, (ULONG)-1);
}
BLITZ_THUNK(_GetUserData, getuserdata_c, "d1/a0-a1");

/**
 * @brief GTGetUserData(GTMenuList#,Menu,Item,Subitem): the UserData of a
 * sub item (of the item if Subitem is negative as a word).
 *
 * Not checked (error check chkchk5: as chkchk3 for Subitem -1).
 * @param menu    d1.w: the menu
 * @param itemno  d2.w: the item
 * @param subno   d3.w: the sub item
 * @param obj     a2: the GTMenuList object
 * @return d0: the UserData
 */
static ULONG BLITZ_ROUTINE getuserdata2_c(ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2),
                                          ULONG subno BL_REG(d3), struct gtmenulist *obj BL_REG(a2))
{
    return getuserdata(obj, menu, itemno, subno);
}
BLITZ_THUNK(_GetUserData2, getuserdata2_c, "d1/a0-a1");

/* -------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Menu number");
BLITZ_ERRCHK_MESSAGE(mess2, "Illegal Menu Item number");
BLITZ_ERRCHK_MESSAGE(mess3, "Illegal Menu Sub Item number");
BLITZ_ERRCHK_MESSAGE(mess8, "GTMenu has no data to build GTMenuList from!");

/* createchk, setchk: the object has a list */
static void BLITZ_ERRCHK listchk_c(struct gtmenulist *obj BL_REG(a3))
{
    if (!obj->titles)
        BLITZ_RUNERR(mess8);
}

/* existschk: entries 0..n of the list at the link **slot exist (DBF);
   *slot moves to entry n, or stays at the last one there is.  0 if one
   is missing. */
static int BLITZ_ERRCHK existschk(APTR **slot, UWORD n)
{
    APTR *p = *slot;

    do {
        if (!*p) {
            *slot = p;
            return 0;
        }
        p = (APTR *)*p;
    } while (n-- != 0);
    *slot = p;
    return 1;
}

/* statechk2/3/4: menu d1, item d2 (FirstItem), sub item d3 (SubItem)
   exist in the strip of the object */
static void BLITZ_ERRCHK statechk(struct gtmenulist *obj, int depth, UWORD menu, UWORD itemno,
                                  UWORD subno)
{
    APTR *entry = (APTR *)&obj->strip;

    if (!existschk(&entry, menu)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    if (depth < 3)
        return;
    entry = (APTR *)&((struct Menu *)entry)->FirstItem;
    if (!existschk(&entry, itemno)) {
        BLITZ_RUNERR(mess2);
        return;
    }
    if (depth < 4)
        return;
    entry = (APTR *)&((struct MenuItem *)entry)->SubItem;
    if (!existschk(&entry, subno))
        BLITZ_RUNERR(mess3);
}

#define CHKARGS ULONG menu BL_REG(d1), ULONG itemno BL_REG(d2), ULONG subno BL_REG(d3)

static void BLITZ_ERRCHK statechk2_c(CHKARGS, struct gtmenulist *obj BL_REG(a3))
{
    statechk(obj, 2, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

static void BLITZ_ERRCHK statechk3_c(CHKARGS, struct gtmenulist *obj BL_REG(a3))
{
    statechk(obj, 3, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

static void BLITZ_ERRCHK statechk4_c(CHKARGS, struct gtmenulist *obj BL_REG(a3))
{
    statechk(obj, 4, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

/* chkchk3/4: the same with the object in a2 (EXG a2,a3); chkchk5: chkchk3
   if the sub item is -1 (CMP.w), else chkchk4 */
static void BLITZ_ERRCHK chkchk3_c(CHKARGS, struct gtmenulist *obj BL_REG(a2))
{
    statechk(obj, 3, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

static void BLITZ_ERRCHK chkchk4_c(CHKARGS, struct gtmenulist *obj BL_REG(a2))
{
    statechk(obj, 4, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

static void BLITZ_ERRCHK chkchk5_c(CHKARGS, struct gtmenulist *obj BL_REG(a2))
{
    statechk(obj, (UWORD)subno == 0xffff ? 3 : 4, (UWORD)menu, (UWORD)itemno, (UWORD)subno);
}

/* the thunks after all check functions: BLITZ_ERRCHK_THUNK ends with
   `.text` while GCC still assumes the error check section, so a check
   function behind a thunk would land in the normal code
   (docs/libs/deviations.md, section 6) */
BLITZ_ERRCHK_THUNK(createchk, listchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(setchk, listchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(statechk2, statechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(statechk3, statechk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(statechk4, statechk4_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(chkchk3, chkchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(chkchk4, chkchk4_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(chkchk5, chkchk5_c, "d0-d1/a0-a1");
