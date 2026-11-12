/*
 * rwe rigtmenulib - rigtmenulib (userlib 37): the C build or the
 * reference build of its source (ref/rigtmenulib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), one set as LIBSET says, observed as
 * cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_rigtmenulib.txt).
 *
 *   rigtmenulib DIR     (DIR with rigtmenulib, memlib, maxslib .obj or .orig)
 *
 * The set has its memlib/maxslib, the GTMenuList objects and a window (on
 * the Workbench screen, also the "used screen").  The menus
 * are built with fixed calls; observed: the lists of the object
 * (titles, items, sub items: texts, flags, shortcuts, mutual exclude,
 * UserData, the count at +8), the menu strips gadtools makes from them
 * (every Menu and MenuItem field but the pointers, the texts, the
 * commands, GadTools' UserData), the window's MenuStrip, d0 of the
 * functions, the kept registers; memlib's memory back at the end.
 * Covered: every entry and form: GTMenuTitle (at the head, in the
 * middle, at the end, behind a missing one), GTMenuItem forms 0-4
 * (BUG(orig) (#692): form 4 keeps the mutual exclude of the call before),
 * GTSubItem forms 0-4 (shortcut$ from the stack), missing titles/items,
 * CreateMenuStrip (twice: the old strip freed), GTSetMenu (with and
 * without a strip, a window with a strip), GTUnSetMenu, GTMenuState all
 * forms, GTMenuChecked/GTSetMenuChecked both forms (the upper word of
 * d0), GTGetUserData both forms (sub item -1), GTFreeMenu all forms and
 * missing entries, the error checks with existing entries, the object's
 * free routine, init/finit (BUG(orig) (#690): gadtools stays open).
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

static UBYTE obj[2][16];                /* two GTMenuList objects */
static struct Screen *scr;
static ULONG scrobj[2];                 /* screenslib objects: the screen */
static struct Window *win;
static ULONG winobj[2];                 /* windowslib objects */
static ULONG stk[8];                    /* the compiler's a2 stack */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define P(p, o) (*(UBYTE **)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* ------------------------------------------------------------ observe */

/* a string pointer: the text, null, or -1 ($FFFFFFFF) */
static void obsstr(const char *what, const UBYTE *p)
{
    if (p == (const UBYTE *)-1)
        OBS(what, 0xffffffffUL);
    else
        OBSSTR(what, (const char *)p);
}

static void obsentry(const char *what, UBYTE *e, int item)
{
    char w[100], v[80];
    snprintf(w, sizeof w, "%s text", what);
    obsstr(w, P(e, 4));
    if (!item)
        return;
    snprintf(w, sizeof w, "%s flags/mx/ud", what);
    snprintf(v, sizeof v, "flags $%04X mx $%08lX ud $%08lX", SW(e, 12), SL(e, 22), SL(e, 26));
    OBSSTR(w, v);
    snprintf(w, sizeof w, "%s shortcut", what);
    obsstr(w, P(e, 14));
}

/* the lists of object n: every entry ("title N ...": there, then its
   fields), so that a missing or an added entry shows as a missing or an
   added case */
static void obslist(int n)
{
    UBYTE *t, *it, *sub;
    char w[100];
    int ti, ii, si;

    OBS("count", SW(obj[n], 8));
    for (ti = 0, t = P(obj[n], 4); t; ti++, t = P(t, 0)) {
        sprintf(w, "title %d", ti);
        OBS(w, 1);
        obsentry(w, t, 0);
        for (ii = 0, it = P(t, 8); it; ii++, it = P(it, 0)) {
            sprintf(w, "title %d item %d", ti, ii);
            OBS(w, 1);
            obsentry(w, it, 1);
            for (si = 0, sub = P(it, 18); sub; si++, sub = P(sub, 0)) {
                sprintf(w, "title %d item %d sub %d", ti, ii, si);
                OBS(w, 1);
                obsentry(w, sub, 1);
            }
        }
    }
}

static void obsitems(const char *what, struct MenuItem *o, int sub)
{
    char w[100], v[120];
    int i;

    for (i = 0; o; i++, o = o->NextItem) {
        snprintf(w, sizeof w, "%s %s %d", what, sub ? "sub" : "item", i);
        OBS(w, 1);
        snprintf(v, sizeof v, "L %d T %d W %d H %d flags $%04X mx $%08lX cmd $%02X ud $%08lX%s",
                 o->LeftEdge, o->TopEdge, o->Width, o->Height, o->Flags,
                 (ULONG)o->MutualExclude, (UBYTE)o->Command, (ULONG)GTMENUITEM_USERDATA(o),
                 o->NextSelect ? "" : " no NextSelect");
        snprintf(w + strlen(w), sizeof w - strlen(w), " fields");
        OBSSTR(w, v);
        w[strlen(w) - 7] = 0;
        if (!sub)
            obsitems(w, o->SubItem, 1);
    }
}

/* the menu strip of object n */
static void obsstrip(int n)
{
    struct Menu *m = (struct Menu *)P(obj[n], 0);
    char w[100], v[160];
    int i;

    OBS("strip made", m != 0);
    for (i = 0; m; i++, m = m->NextMenu) {
        sprintf(w, "menu %d", i);
        OBS(w, 1);
        snprintf(v, sizeof v, "L %d W %d H %d flags $%04X \"%.60s\"", m->LeftEdge, m->Width,
                 m->Height, m->Flags, m->MenuName ? (const char *)m->MenuName : "(null)");
        sprintf(w, "menu %d fields", i);
        OBSSTR(w, v);
        sprintf(w, "menu %d", i);
        obsitems(w, m->FirstItem, 0);
    }
}

/* ------------------------------------------------------------- calls */

static void title(int n, ULONG menu, const char *text)
{
    ROUT = "GTMenuTitle";
    sprintf(VEC, "%d \"%s\" at %ld", n, text, (LONG)(WORD)menu);
    DO1(ENT(LIB, TITLE, 0), R.d[0] = WA(n); R.d[1] = WA(menu);
        R.d[2] = (ULONG)text; stk[5] = strlen(text);
        R.a[2] = (ULONG)&stk[6]; R.a[3] = (ULONG)obj[n]);
    obslist(n);
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
    DO1(ENT(LIB, s ? SUBITEM : ITEM, form),
        R.d[0] = WA(n); R.d[1] = WA(fl); R.d[2] = WA(menu);
        R.d[3] = WA(it);
        memset(stk, 0x77, sizeof stk);
        stk[7] = text ? strlen(text) : 0x11111111;
        stk[6] = sc ? strlen(sc) : 0x22222222;
        if (s) {
            R.d[4] = WA(sub); R.d[5] = (ULONG)text; R.d[6] = (ULONG)sc;
            stk[5] = (ULONG)sc; stk[4] = mx; stk[3] = ud;
        } else {
            R.d[4] = (ULONG)text; R.d[5] = (ULONG)sc; R.d[6] = mx;
            stk[4] = ud;
        }
        R.a[2] = (ULONG)&stk[8]; R.a[3] = (ULONG)obj[n]);
    obslist(n);
}

static void create(int n)
{
    ROUT = "CreateMenuStrip";
    sprintf(VEC, "%d", n);
    DO1(ENT(LIB, CREATE, 0), R.d[0] = WA(n); R.a[0] = (ULONG)scrobj;
        R.a[3] = (ULONG)obj[n]);
    OBS("d0", R.d[0]);
    BOTH("strip made", R.d[0] == (ULONG)-1 && P(obj[n], 0));
    obsstrip(n);
}

static void setmenu(int n)
{
    ROUT = "GTSetMenu";
    sprintf(VEC, "%d", n);
    DO1(ENT(LIB, SETMENU, 0), R.d[0] = WA(n); R.a[0] = (ULONG)scrobj;
        R.a[1] = (ULONG)winobj; R.a[3] = (ULONG)obj[n];
        R.a[6] = (ULONG)IntuitionBase);
    BOTH("the window's strip", win->MenuStrip == (struct Menu *)P(obj[n], 0));
    obsstrip(n);
}

static void unsetmenu(void)
{
    ROUT = "GTUnSetMenu";
    strcpy(VEC, "");
    DO1(ENT(LIB, UNSETMENU, 0), R.a[2] = (ULONG)winobj;
        R.a[6] = (ULONG)IntuitionBase);
    BOTH("no strip", win->MenuStrip == NULL);
}

/* GTMenuState n[,m[,i[,s]]],on: form = the number of indexes */
static void state(int n, int form, ULONG m, ULONG i, ULONG s, ULONG on)
{
    ULONG a[4] = { m, i, s, 0 };

    a[form] = on;
    ROUT = "GTMenuState";
    sprintf(VEC, "%d form %d %lu/%lu/%lu on %lu", n, form, m, i, s, on);
    DO1(ENT(LIB, STATE, form), R.d[0] = WA(n); R.d[1] = WA(a[0]);
        R.d[2] = WA(a[1]); R.d[3] = WA(a[2]); R.d[4] = WA(a[3]);
        R.a[3] = (ULONG)obj[n]);
    if (form) {
        ROUT = "statechk";
        DO1(ERR(LIB, STATE, form), R.d[0] = WA(n); R.d[1] = WA(a[0]);
            R.d[2] = WA(a[1]); R.d[3] = WA(a[2]); R.d[4] = WA(a[3]);
            R.a[3] = (ULONG)obj[n]);
    }
    obsstrip(n);
}

/* GTMenuChecked / GTSetMenuChecked / GTGetUserData (object in a2); d0
   of the call in last */
static ULONG last;
static void check3(int idx, const char *name, int n, int form, ULONG m, ULONG i, ULONG s,
                   ULONG on)
{
    ROUT = name;
    sprintf(VEC, "%d form %d %lu/%lu/%ld on %lu", n, form, m, i, (LONG)(WORD)s, on);
    DO1(ENT(LIB, idx, form), R.d[0] = MARK(0); R.d[1] = WA(m); R.d[2] = WA(i);
        R.d[3] = form ? WA(s) : WA(on); R.d[4] = WA(on);
        R.a[2] = (ULONG)obj[n]);
    if (idx != E_SETCHECKED)
        OBS("d0", R.d[0]);
    last = R.d[0];
    ROUT = "chkchk";
    DO1(ERR(LIB, idx, form), R.d[0] = MARK(0); R.d[1] = WA(m);
        R.d[2] = WA(i); R.d[3] = form ? WA(s) : WA(on); R.d[4] = WA(on);
        R.a[2] = (ULONG)obj[n]);
    obsstrip(n);
}

static void freemenu(int n, int form, ULONG m, ULONG i, ULONG s)
{
    ROUT = "GTFreeMenu";
    sprintf(VEC, "%d form %d %lu/%lu/%lu", n, form, m, i, s);
    DO1(ENT(LIB, FREEMENU, form), R.d[0] = WA(n); R.d[1] = WA(m);
        R.d[2] = WA(i); R.d[3] = WA(s); R.a[3] = (ULONG)obj[n]);
    obslist(n);
}

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("finit", d0-d7|a0-a3|a6);
    KEEPS("GTMenuTitle", d2-d7|a3|a6);
    KEEPS("GTMenuItem", d5-d7|a3|a6);
    KEEPS("GTSubItem", d6|d7|a3|a6);
    KEEPS("CreateMenuStrip", d2-d7|a3);
    KEEPS("createchk", d0-d7|a0-a3|a6);
    KEEPS("GTSetMenu", d2-d7|a3|a6);
    KEEPS("setchk", d0-d7|a0-a3|a6);
    KEEPS("GTUnSetMenu", d2-d7|a2|a3|a6);
    KEEPS("GTMenuState", d4-d6|a0-a2|a6);
    KEEPS("statechk", d1-d6|a1-a3|a6);
    KEEPS("GTMenuChecked", d4-d7|a0|a1|a3|a6);
    KEEPS("GTSetMenuChecked", d4-d7|a0|a1|a3|a6);
    KEEPS("GTGetUserData", d4-d7|a0|a1|a3|a6);
    KEEPS("chkchk", d1-d6|a1-a3|a6);
    KEEPS("GTFreeMenu", d4-d7|a3|a6);
    KEEPS("free", d2-d7|a3);
    gt = OpenLibrary((STRPTR)"gadtools.library", 37);
    scr = LockPubScreen(NULL);
    if (!gt || !scr) {
        check("rigtmenulib: gadtools.library and the Workbench screen", FALSE);
        return 20;
    }
    win = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Left, 10, WA_Top, 20,
                         WA_Width, 110, WA_Height, 40, WA_Title, (ULONG)"rwe gtmenu",
                         WA_Flags, WFLG_DRAGBAR | WFLG_NEWLOOKMENUS, TAG_END);
    if (!win) {
        check("rigtmenulib: a window", FALSE);
        return 20;
    }
    scrobj[0] = (ULONG)scr;
    winobj[0] = (ULONG)win;
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    memlib(7);
    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, INIT, 0), (void)0);

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
    DO1(ERR(LIB, CREATE, 0), R.d[0] = WA(0); R.a[3] = (ULONG)obj[0]);
    create(0);                          /* again: the old one freed */
    setmenu(0);
    setmenu(0);                         /* the window has one: cleared first */
    ROUT = "setchk";
    DO1(ERR(LIB, SETMENU, 0), R.d[0] = WA(0); R.a[3] = (ULONG)obj[0]);
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
    BOTH("UserData of \"Data\"", last == 0x1234);
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
        DO1(ENT(LIB, FREE, 0), R.a[3] = (ULONG)obj[s]);
        BOTH("object cleared", !SL(obj[s], 0) && !SL(obj[s], 4)
                               && !SW(obj[s], 8));
    }
    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), (void)0);

    ROUT = "memlib finit";
    memlib(8);
    rwe_mem_check(LIB, avail, TRUE);
    CloseWindow(win);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(gt);
    return rwe_done(LIB);
}
