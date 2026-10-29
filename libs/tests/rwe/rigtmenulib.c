/*
 * rwe rigtmenulib - the C build of rigtmenulib (userlib 37) against the
 * reference build of its source (ref/rigtmenulib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), method of rwecmp.h.
 *
 *   rigtmenulib DIR     (DIR with rigtmenulib, memlib, maxslib .obj/.orig)
 *
 * Each set has its own memlib/maxslib, GTMenuList object and window (on
 * the Workbench screen, which both use as the "used screen").  The menus
 * are built with the same calls; compared: the lists of the object
 * (titles, items, sub items: texts, flags, shortcuts, mutual exclude,
 * UserData, the count at +8), the menu strips gadtools makes from them
 * (every Menu and MenuItem field but the pointers, the texts, the
 * commands, GadTools' UserData), the windows' MenuStrip, d0 of the
 * functions, the kept registers; memlib's memory back at the end.
 * Covered: every entry and form: GTMenuTitle (at the head, in the
 * middle, at the end, behind a missing one), GTMenuItem forms 0-4
 * (BUG(orig): form 4 keeps the mutual exclude of the call before),
 * GTSubItem forms 0-4 (shortcut$ from the stack), missing titles/items,
 * CreateMenuStrip (twice: the old strip freed), GTSetMenu (with and
 * without a strip, a window with a strip), GTUnSetMenu, GTMenuState all
 * forms, GTMenuChecked/GTSetMenuChecked both forms (the upper word of
 * d0), GTGetUserData both forms (sub item -1), GTFreeMenu all forms and
 * missing entries, the error checks with existing entries, the object's
 * free routine, init/finit (BUG(orig): gadtools stays open).
 * Not covered: the error checks of missing entries (runtime errors,
 * TRAP #0), a shortcut$ at an address with low word 0, CreateMenuStrip
 * without titles (reads address 0), the walks beyond the lists (write
 * near address 0).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/intuition.h>

#define LIB "rigtmenulib"

enum { TOKE, CREATE, SETMENU, UNSETMENU, TITLE, ITEM, STATE, SUBITEM, E_CHECKED, E_SETCHECKED,
       FREEMENU, USERDATA, INIT, FINIT, LOAD, SAVE, USE, FREE };

static UBYTE obj[2][2][16];             /* two GTMenuList objects per set */
static struct Screen *scr;
static ULONG scrobj[2][2];              /* screenslib objects: the screen */
static struct Window *win[2];
static ULONG winobj[2][2];              /* windowslib objects */
static ULONG stk[2][8];                 /* the compiler's a2 stack */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define P(p, o) (*(UBYTE **)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* ------------------------------------------------------------ compare */

static void samestr(const char *what, const UBYTE *o, const UBYTE *c)
{
    char w[240];
    snprintf(w, sizeof w, "%s %s", what, (o == c || (o && c && !strcmp((char *)o, (char *)c)))
                                         ? "" : "differs");
    if (o == (UBYTE *)-1 || c == (UBYTE *)-1 || !o || !c)
        check(w, o == c);
    else
        check(w, !strcmp((const char *)o, (const char *)c));
}

static void sameentry(const char *what, UBYTE *o, UBYTE *c, int item)
{
    char w[240];
    snprintf(w, sizeof w, "%s text", what);
    samestr(w, P(o, 4), P(c, 4));
    if (!item)
        return;
    snprintf(w, sizeof w, "%s flags/mx/ud", what);
    check(w, SW(o, 12) == SW(c, 12) && SL(o, 22) == SL(c, 22) && SL(o, 26) == SL(c, 26));
    snprintf(w, sizeof w, "%s shortcut", what);
    samestr(w, P(o, 14), P(c, 14));
}

/* the lists of object n of both sets */
static void samelist(int n)
{
    UBYTE *t[2], *it[2], *sub[2];
    char w[200];                        /* VEC (100) and the indexes */
    int ti, ii, si;

    SAME("count", SW(obj[cur][n], 8));
    t[0] = P(obj[0][n], 4);
    t[1] = P(obj[1][n], 4);
    for (ti = 0; t[0] || t[1]; ti++) {
        sprintf(w, "%s title %d", VEC, ti);
        check(w, t[0] && t[1]);
        if (!t[0] || !t[1])
            return;
        sameentry(w, t[0], t[1], 0);
        it[0] = P(t[0], 8);
        it[1] = P(t[1], 8);
        for (ii = 0; it[0] || it[1]; ii++) {
            sprintf(w, "%s title %d item %d", VEC, ti, ii);
            check(w, it[0] && it[1]);
            if (!it[0] || !it[1])
                return;
            sameentry(w, it[0], it[1], 1);
            sub[0] = P(it[0], 18);
            sub[1] = P(it[1], 18);
            for (si = 0; sub[0] || sub[1]; si++) {
                sprintf(w, "%s title %d item %d sub %d", VEC, ti, ii, si);
                check(w, sub[0] && sub[1]);
                if (!sub[0] || !sub[1])
                    return;
                sameentry(w, sub[0], sub[1], 1);
                sub[0] = P(sub[0], 0);
                sub[1] = P(sub[1], 0);
            }
            it[0] = P(it[0], 0);
            it[1] = P(it[1], 0);
        }
        t[0] = P(t[0], 0);
        t[1] = P(t[1], 0);
    }
}

static void sameitems(const char *what, struct MenuItem *o, struct MenuItem *c, int sub)
{
    char w[240];
    int i;

    for (i = 0; o || c; i++) {
        snprintf(w, sizeof w, "%s %s %d", what, sub ? "sub" : "item", i);
        check(w, o && c);
        if (!o || !c)
            return;
        check(w, o->LeftEdge == c->LeftEdge && o->TopEdge == c->TopEdge
                 && o->Width == c->Width && o->Height == c->Height && o->Flags == c->Flags
                 && o->MutualExclude == c->MutualExclude && o->Command == c->Command
                 && GTMENUITEM_USERDATA(o) == GTMENUITEM_USERDATA(c)
                 && !o->NextSelect == !c->NextSelect);
        if (!sub)
            sameitems(w, o->SubItem, c->SubItem, 1);
        o = o->NextItem;
        c = c->NextItem;
    }
}

/* the menu strips of object n of both sets */
static void samestrip(int n)
{
    struct Menu *m[2] = { (struct Menu *)P(obj[0][n], 0), (struct Menu *)P(obj[1][n], 0) };
    char w[200];                        /* VEC (100) and the indexes */
    int i;

    SAME("strip made", P(obj[cur][n], 0) != 0);
    for (i = 0; m[0] || m[1]; i++) {
        sprintf(w, "%s menu %d", VEC, i);
        check(w, m[0] && m[1]);
        if (!m[0] || !m[1])
            return;
        check(w, m[0]->LeftEdge == m[1]->LeftEdge && m[0]->Width == m[1]->Width
                 && m[0]->Height == m[1]->Height && m[0]->Flags == m[1]->Flags
                 && !strcmp((const char *)m[0]->MenuName, (const char *)m[1]->MenuName));
        sameitems(w, m[0]->FirstItem, m[1]->FirstItem, 0);
        m[0] = m[0]->NextMenu;
        m[1] = m[1]->NextMenu;
    }
}

/* ------------------------------------------------------------- calls */

static void title(int n, ULONG menu, const char *text)
{
    ROUT = "GTMenuTitle";
    sprintf(VEC, "%d \"%s\" at %ld", n, text, (LONG)(WORD)menu);
    DO2(ENT(LIB, TITLE, 0), R[cur].d[0] = WA(n); R[cur].d[1] = WA(menu);
        R[cur].d[2] = (ULONG)text; stk[cur][5] = strlen(text);
        R[cur].a[2] = (ULONG)&stk[cur][6]; R[cur].a[3] = (ULONG)obj[cur][n]);
    samelist(n);
}

/* GTMenuItem (sub < 0) or GTSubItem, form 0-4; the stack: -4 text's
   length, -8 shortcut's, -12 (GTSubItem) shortcut$, -16 mutual exclude,
   -20 UserData (GTMenuItem form 4: UserData at -16) */
static void item(int n, int form, ULONG fl, ULONG menu, ULONG it, LONG sub, const char *text,
                 const char *sc, ULONG mx, ULONG ud)
{
    int s = sub >= 0;

    ROUT = s ? "GTSubItem" : "GTMenuItem";
    sprintf(VEC, "%d form %d %ld/%ld/%ld \"%s\" \"%s\" mx %lu ud %lu", n, form, menu, it, sub,
            text ? text : "", sc ? sc : "", mx, ud);
    DO2(ENT(LIB, s ? SUBITEM : ITEM, form),
        R[cur].d[0] = WA(n); R[cur].d[1] = WA(fl); R[cur].d[2] = WA(menu);
        R[cur].d[3] = WA(it);
        memset(stk[cur], 0x77, sizeof stk[cur]);
        stk[cur][7] = text ? strlen(text) : 0x11111111;
        stk[cur][6] = sc ? strlen(sc) : 0x22222222;
        if (s) {
            R[cur].d[4] = WA(sub); R[cur].d[5] = (ULONG)text; R[cur].d[6] = (ULONG)sc;
            stk[cur][5] = (ULONG)sc; stk[cur][4] = mx; stk[cur][3] = ud;
        } else {
            R[cur].d[4] = (ULONG)text; R[cur].d[5] = (ULONG)sc; R[cur].d[6] = mx;
            stk[cur][4] = ud;
        }
        R[cur].a[2] = (ULONG)&stk[cur][8]; R[cur].a[3] = (ULONG)obj[cur][n]);
    samelist(n);
}

static void create(int n)
{
    ROUT = "CreateMenuStrip";
    sprintf(VEC, "%d", n);
    DO2(ENT(LIB, CREATE, 0), R[cur].d[0] = WA(n); R[cur].a[0] = (ULONG)scrobj[cur];
        R[cur].a[3] = (ULONG)obj[cur][n]);
    SAME("d0", R[cur].d[0]);
    BOTH("strip made", R[cur].d[0] == (ULONG)-1 && P(obj[cur][n], 0));
    samestrip(n);
}

static void setmenu(int n)
{
    ROUT = "GTSetMenu";
    sprintf(VEC, "%d", n);
    DO2(ENT(LIB, SETMENU, 0), R[cur].d[0] = WA(n); R[cur].a[0] = (ULONG)scrobj[cur];
        R[cur].a[1] = (ULONG)winobj[cur]; R[cur].a[3] = (ULONG)obj[cur][n];
        R[cur].a[6] = (ULONG)IntuitionBase);
    BOTH("the window's strip", win[cur]->MenuStrip == (struct Menu *)P(obj[cur][n], 0));
    samestrip(n);
}

static void unsetmenu(void)
{
    ROUT = "GTUnSetMenu";
    strcpy(VEC, "");
    DO2(ENT(LIB, UNSETMENU, 0), R[cur].a[2] = (ULONG)winobj[cur];
        R[cur].a[6] = (ULONG)IntuitionBase);
    BOTH("no strip", win[cur]->MenuStrip == NULL);
}

/* GTMenuState n[,m[,i[,s]]],on: form = the number of indexes */
static void state(int n, int form, ULONG m, ULONG i, ULONG s, ULONG on)
{
    ULONG a[4] = { m, i, s, 0 };

    a[form] = on;
    ROUT = "GTMenuState";
    sprintf(VEC, "%d form %d %lu/%lu/%lu on %lu", n, form, m, i, s, on);
    DO2(ENT(LIB, STATE, form), R[cur].d[0] = WA(n); R[cur].d[1] = WA(a[0]);
        R[cur].d[2] = WA(a[1]); R[cur].d[3] = WA(a[2]); R[cur].d[4] = WA(a[3]);
        R[cur].a[3] = (ULONG)obj[cur][n]);
    if (form) {
        ROUT = "statechk";
        DO2(ERR(LIB, STATE, form), R[cur].d[0] = WA(n); R[cur].d[1] = WA(a[0]);
            R[cur].d[2] = WA(a[1]); R[cur].d[3] = WA(a[2]); R[cur].d[4] = WA(a[3]);
            R[cur].a[3] = (ULONG)obj[cur][n]);
    }
    samestrip(n);
}

/* GTMenuChecked / GTSetMenuChecked / GTGetUserData (object in a2); d0
   of the call in last[] */
static ULONG last[2];
static void check3(int idx, const char *name, int n, int form, ULONG m, ULONG i, ULONG s,
                   ULONG on)
{
    ROUT = name;
    sprintf(VEC, "%d form %d %lu/%lu/%ld on %lu", n, form, m, i, (LONG)(WORD)s, on);
    DO2(ENT(LIB, idx, form), R[cur].d[0] = MARK(0); R[cur].d[1] = WA(m); R[cur].d[2] = WA(i);
        R[cur].d[3] = form ? WA(s) : WA(on); R[cur].d[4] = WA(on);
        R[cur].a[2] = (ULONG)obj[cur][n]);
    if (idx != E_SETCHECKED)
        SAME("d0", R[cur].d[0]);
    for (cur = 0; cur < 2; cur++)
        last[cur] = R[cur].d[0];
    ROUT = "chkchk";
    DO2(ERR(LIB, idx, form), R[cur].d[0] = MARK(0); R[cur].d[1] = WA(m);
        R[cur].d[2] = WA(i); R[cur].d[3] = form ? WA(s) : WA(on); R[cur].d[4] = WA(on);
        R[cur].a[2] = (ULONG)obj[cur][n]);
    samestrip(n);
}

static void freemenu(int n, int form, ULONG m, ULONG i, ULONG s)
{
    ROUT = "GTFreeMenu";
    sprintf(VEC, "%d form %d %lu/%lu/%lu", n, form, m, i, s);
    DO2(ENT(LIB, FREEMENU, form), R[cur].d[0] = WA(n); R[cur].d[1] = WA(m);
        R[cur].d[2] = WA(i); R[cur].d[3] = WA(s); R[cur].a[3] = (ULONG)obj[cur][n]);
    samelist(n);
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "memlib", "maxslib", NULL };
    struct Library *gt;
    ULONG avail;
    int s;

    if (argc < 2) {
        printf("usage: rigtmenulib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    gt = OpenLibrary((STRPTR)"gadtools.library", 37);
    scr = LockPubScreen(NULL);
    if (!gt || !scr) {
        check("rigtmenulib: gadtools.library and the Workbench screen", FALSE);
        return 20;
    }
    for (s = 0; s < 2; s++) {
        win[s] = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Left, 10 + 120 * s,
                                WA_Top, 20, WA_Width, 110, WA_Height, 40,
                                WA_Title, (ULONG)(s ? "rwe gtmenu C" : "rwe gtmenu orig"),
                                WA_Flags, WFLG_DRAGBAR | WFLG_NEWLOOKMENUS, TAG_END);
        if (!win[s]) {
            check("rigtmenulib: a window", FALSE);
            return 20;
        }
        scrobj[s][0] = (ULONG)scr;
        winobj[s][0] = (ULONG)win[s];
    }
    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib init";
    memlib(7);
    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);

    /* the lists */
    title(0, 0, "Project");
    title(0, 1, "Edit");
    title(0, 1, "Middle");              /* between */
    title(0, 3, "Last");
    title(0, 9, "Nowhere");             /* behind a missing title */
    title(0, 0, "First");               /* at the head */
    item(0, 2, 0, 1, 0, -1, "Open", "O", 0, 0);
    item(0, 0, 0, 1, 1, -1, NULL, NULL, 0, 0);          /* bar label */
    item(0, 1, CHECKIT | MENUTOGGLE, 1, 2, -1, "Toggle", NULL, 0, 0);
    item(0, 3, CHECKIT, 1, 0, -1, "Mx", "M", 6, 0);     /* at the head */
    item(0, 4, CHECKIT | CHECKED, 2, 0, -1, "Data", "D", 3, 0x1234);    /* mx stays 6 */
    item(0, 2, 0, 3, 0, -1, "Cut", "X", 0, 0);
    item(0, 2, 0, 3, 1, -1, "Copy", "C", 0, 0);
    item(0, 2, 0, 9, 0, -1, "None", "N", 0, 0);         /* no title 9 */
    item(0, 2, 0, 3, 7, -1, "None", "N", 0, 0);         /* no item 6 */
    item(0, 2, 0, 1, 1, 0, "Sub A", "A", 0, 0);
    item(0, 0, 0, 1, 1, 1, NULL, NULL, 0, 0);
    item(0, 1, CHECKIT, 1, 1, 2, "Sub C", NULL, 0, 0);
    item(0, 3, CHECKIT, 1, 1, 0, "Sub D", "Q", 2, 0);
    item(0, 4, CHECKIT | CHECKED, 1, 1, 4, "Sub E", "E", 1, 0x5678);
    item(0, 2, 0, 1, 9, 0, "None", "N", 0, 0);          /* no item 9 */
    item(0, 2, 0, 1, 1, 9, "None", "N", 0, 0);          /* no sub 8 */
    title(1, 0, "Other");
    item(1, 2, 0, 0, 0, -1, "One", "1", 0, 0);

    /* the strips */
    create(0);
    ROUT = "createchk";
    DO2(ERR(LIB, CREATE, 0), R[cur].d[0] = WA(0); R[cur].a[3] = (ULONG)obj[cur][0]);
    create(0);                          /* again: the old one freed */
    setmenu(0);
    setmenu(0);                         /* the window has one: cleared first */
    ROUT = "setchk";
    DO2(ERR(LIB, SETMENU, 0), R[cur].d[0] = WA(0); R[cur].a[3] = (ULONG)obj[cur][0]);
    unsetmenu();
    unsetmenu();                        /* no strip */
    setmenu(1);                         /* no strip yet: made */
    unsetmenu();

    state(0, 0, 0, 0, 0, 0);
    state(0, 0, 0, 0, 0, 1);
    state(0, 1, 2, 0, 0, 0);
    state(0, 1, 0, 0, 0, 0);
    state(0, 2, 1, 2, 0, 0);
    state(0, 2, 1, 2, 0, 1);
    state(0, 3, 1, 1, 1, 0);
    state(0, 3, 1, 1, 3, 0);
    check3(E_CHECKED, "GTMenuChecked", 0, 0, 2, 0, 0, 0);
    check3(E_CHECKED, "GTMenuChecked", 0, 0, 1, 0, 0, 0);
    check3(E_CHECKED, "GTMenuChecked", 0, 1, 1, 1, 4, 0);
    check3(E_SETCHECKED, "GTSetMenuChecked", 0, 0, 1, 0, 0, 1);
    check3(E_CHECKED, "GTMenuChecked", 0, 0, 1, 0, 0, 0);
    check3(E_SETCHECKED, "GTSetMenuChecked", 0, 0, 2, 0, 0, 0);
    check3(E_SETCHECKED, "GTSetMenuChecked", 0, 1, 1, 1, 4, 0);
    check3(E_SETCHECKED, "GTSetMenuChecked", 0, 1, 1, 1, 3, 1);
    check3(E_CHECKED, "GTMenuChecked", 0, 1, 1, 1, 3, 0);
    check3(USERDATA, "GTGetUserData", 0, 0, 2, 0, 0, 0);
    BOTH("UserData of \"Data\"", last[cur] == 0x1234);
    check3(USERDATA, "GTGetUserData", 0, 1, 1, 1, 4, 0);
    check3(USERDATA, "GTGetUserData", 0, 1, 2, 0, 0xffff, 0);   /* no sub item */

    /* GTFreeMenu */
    freemenu(0, 2, 1, 1, 2);
    freemenu(0, 2, 1, 1, 7);            /* missing */
    freemenu(0, 2, 1, 2, 0);            /* the bar label has no sub items */
    freemenu(0, 1, 1, 0, 0);
    freemenu(0, 1, 9, 0, 0);
    freemenu(0, 0, 3, 0, 0);
    freemenu(0, 0, 8, 0, 0);
    freemenu(0, 1, 2, 0, 0);
    create(0);

    /* the objects' free routine, finit */
    for (s = 0; s < 2; s++) {
        ROUT = "free";
        sprintf(VEC, "object %d", s);
        DO2(ENT(LIB, FREE, 0), R[cur].a[3] = (ULONG)obj[cur][s]);
        BOTH("object cleared", !SL(obj[cur][s], 0) && !SL(obj[cur][s], 4)
                               && !SW(obj[cur][s], 8));
    }
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);

    ROUT = "memlib finit";
    memlib(8);
    if (AvailMem(MEMF_ANY) != avail)
        printf("rigtmenulib: %ld bytes missing\n", (LONG)(avail - AvailMem(MEMF_ANY)));
    check("rigtmenulib: all memory back", AvailMem(MEMF_ANY) == avail);
    for (s = 0; s < 2; s++)
        CloseWindow(win[s]);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(gt);
    return rwe_done(LIB);
}
