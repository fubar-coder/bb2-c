/*
 * rwe riapplib - the C build of riapplib (userlib 61) against the CD
 * binary (BSS2 RIAPPLIB.OBJ), in the emulator (snitch test case
 * "rwe riapplib", argv[1] = the directory with riapplib.obj/.orig).
 *
 * Both libraries are loaded like the compiler does (sysload.h) and every
 * routine is called on both with the same inputs (other registers =
 * markers); compared are d0, a3/a4 where the routine sets them, the
 * string results, the calls into workbench.library and their arguments,
 * the replies to the messages; every register the original keeps must be
 * kept by the C build.
 *
 * workbench.library: AddAppWindowA ... RemoveAppMenuItem are replaced
 * with SetFunction by stubs that log the call and return a fake object
 * (or 0, to test the failure paths) - no Workbench is needed, and the
 * objects never reach Workbench.  icon.library and dos.library are real:
 * the App icon comes from an icon written to RAM:, the WBArgs of the
 * App messages are locks on RAM:.  The App messages are put to
 * AppMsgPort by the test.
 */
#include "sysload.h"
#include <exec/ports.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
static struct Library *iconbase;
#define ICON_BASE_NAME iconbase
#include <proto/icon.h>

extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static int cur;
static struct regs R[2], IN[2];
static ULONG POP[2];
static const char *ROUT;
static char VEC[100];
static int prints;

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

/* the same bytes in both sets (a SAME of a memcmp would compare the
   result with itself) */
static void samebytes(const char *what, const void *o, const void *c, ULONG n)
{
    const UBYTE *a = o, *b = c;
    ULONG i;
    for (i = 0; i < n && a[i] == b[i]; i++)
        ;
    if (i < n) {
        char buf[60];
        snprintf(buf, sizeof buf, "%s (offset %lu)", what, i);
        fail(buf, a[i], b[i]);
    } else
        checks++;
}

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
    SAME("popped", POP[cur]);
}

#define DO2(fn, setup) do {                                                 \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callstk(fn, &R[cur], NULL, 0, &POP[cur]);                       \
        }                                                                   \
        preserve();                                                         \
    } while (0)

/* code of form `form` of entry idx; Amiga-only code addresses are odd */
static UBYTE *ent(int idx, int form)
{
    UBYTE *p = entry_form(getlib(&sets[cur], "riapplib"), idx, form);
    return (UBYTE *)((ULONG)p & ~1UL);
}

enum { ADDWINDOW, DELWINDOW, ADDMENU, DELMENU, ADDICON, DELICON, APPEVENT,
       APPEVENTTYPE, APPEVENTID, NEXTAPPFILE, APPFILE, APPNUMFILES, APPLOCK,
       APPMESSAGE, APPMSGPORT, APPEVENTIDCMP, FINIT, INIT };

/* ------------------------------------------------- workbench.library stubs */

enum { F_ADDWIN = 1, F_REMWIN, F_ADDICON, F_REMICON, F_ADDMENU, F_REMMENU };

struct wbcall { int fn; ULONG d0, d1, a0, a1, a2, a3, a4; LONG cx, cy; };

#define MAXLOG 64
static struct wbcall wblog[2][MAXLOG];
static int nlog[2];
static ULONG wbfail;                    /* the next Add... returns 0 */
static ULONG nextobj[2];

static ULONG logcall(int fn, ULONG d0, ULONG d1, ULONG a0, ULONG a1, ULONG a2,
                     ULONG a3, ULONG a4)
{
    struct wbcall *c;
    if (nlog[cur] == MAXLOG)
        return 0;
    c = &wblog[cur][nlog[cur]++];
    c->fn = fn;
    c->d0 = d0; c->d1 = d1; c->a0 = a0; c->a1 = a1; c->a2 = a2; c->a3 = a3; c->a4 = a4;
    c->cx = c->cy = 0;
    if (fn == F_ADDICON && a3) {
        c->cx = ((struct DiskObject *)a3)->do_CurrentX;
        c->cy = ((struct DiskObject *)a3)->do_CurrentY;
    }
    if (fn == F_ADDWIN || fn == F_ADDICON || fn == F_ADDMENU) {
        if (wbfail) {
            wbfail = 0;
            return 0;
        }
        return nextobj[cur] += 0x10;
    }
    return 0;
}

static ULONG __attribute__((used)) st_addwin(ULONG id __asm("d0"), ULONG ud __asm("d1"),
    ULONG win __asm("a0"), ULONG port __asm("a1"), ULONG tags __asm("a2"))
{
    return logcall(F_ADDWIN, id, ud, win, port, tags, 0, 0);
}

static ULONG __attribute__((used)) st_remwin(ULONG aw __asm("a0"))
{
    return logcall(F_REMWIN, 0, 0, aw, 0, 0, 0, 0);
}

static ULONG __attribute__((used)) st_addicon(ULONG id __asm("d0"), ULONG ud __asm("d1"),
    ULONG text __asm("a0"), ULONG port __asm("a1"), ULONG lock __asm("a2"),
    ULONG dobj __asm("a3"), ULONG tags __asm("a4"))
{
    return logcall(F_ADDICON, id, ud, text, port, lock, dobj, tags);
}

static ULONG __attribute__((used)) st_remicon(ULONG ai __asm("a0"))
{
    return logcall(F_REMICON, 0, 0, ai, 0, 0, 0, 0);
}

static ULONG __attribute__((used)) st_addmenu(ULONG id __asm("d0"), ULONG ud __asm("d1"),
    ULONG text __asm("a0"), ULONG port __asm("a1"), ULONG tags __asm("a2"))
{
    return logcall(F_ADDMENU, id, ud, text, port, tags, 0, 0);
}

static ULONG __attribute__((used)) st_remmenu(ULONG am __asm("a0"))
{
    return logcall(F_REMMENU, 0, 0, am, 0, 0, 0, 0);
}

static const struct { WORD lvo; APTR fn; } patches[] = {
    { -48, (APTR)st_addwin }, { -54, (APTR)st_remwin }, { -60, (APTR)st_addicon },
    { -66, (APTR)st_remicon }, { -72, (APTR)st_addmenu }, { -78, (APTR)st_remmenu },
};
#define NPATCH (sizeof patches / sizeof patches[0])
static APTR oldvec[NPATCH];

/* compare the calls logged since `from` (per set) */
static ULONG port[2];
static int mark[2];

static void wbmark(void)
{
    mark[0] = nlog[0];
    mark[1] = nlog[1];
}

static void wbcompare(void)
{
    int i;
    SAME("wb calls", nlog[cur] - mark[cur]);
    if (nlog[0] - mark[0] != nlog[1] - mark[1])
        return;
    for (i = 0; i < nlog[0] - mark[0]; i++) {
#define C (&wblog[cur][mark[cur] + i])
        SAME("wb fn", C->fn);
        SAME("wb d0", C->d0);
        SAME("wb d1", C->d1);
        SAME("wb a0", C->a0);
        SAME("wb a1 = AppMsgPort", C->fn & 1 ? C->a1 == port[cur] : 1);
        SAME("wb a2", C->a2);
        SAME("wb a3 = DiskObject", C->a3 != 0);
        SAME("wb a4", C->a4);
        SAME("wb CurrentX", C->cx);
        SAME("wb CurrentY", C->cy);
#undef C
    }
}

/* ------------------------------------------------------------- the test */

static UBYTE out[2][300];
static ULONG frame[2][4];
static struct Window *fakewin[2] = { (struct Window *)0x00a0b0c0, (struct Window *)0x00a0b0c0 };
static struct MsgPort *reply[2];      /* one per set */
static struct AppMessage am[2][6];
static struct WBArg args[2][3];
static BPTR rootlock[2], dirlock[2];

static int replies(void)
{
    int n = 0;
    while (GetMsg(reply[cur]))
        n++;
    return n;
}

static void putmsg(int i, UWORD type, ULONG id, LONG nargs)
{
    struct AppMessage *m = &am[cur][i];
    memset(m, 0, sizeof *m);
    m->am_Message.mn_Node.ln_Type = NT_MESSAGE;
    m->am_Message.mn_ReplyPort = reply[cur];
    m->am_Message.mn_Length = sizeof *m;
    m->am_Type = type;
    m->am_ID = id;
    m->am_NumArgs = nargs;
    m->am_ArgList = args[cur];
    PutMsg((struct MsgPort *)port[cur], &m->am_Message);
}

static void call_noargs(const char *name, int idx)
{
    ROUT = name;
    DO2(ent(idx, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

/* AppMsgPort after init: a port of each set */
static void getport(void)
{
    ROUT = "AppMsgPort";
    DO2(ent(APPMSGPORT, 0), (void)0);
    for (cur = 0; cur < 2; cur++)
        port[cur] = R[cur].d[0];
    SAME("port", port[cur] != 0);
}

static void call_id(const char *name, int idx, LONG id)
{
    ROUT = name;
    sprintf(VEC, "id %ld", id);
    wbmark();
    DO2(ent(idx, 0), R[cur].d[0] = id);
    SAME("d0", R[cur].d[0]);
    wbcompare();
}

static void addmenu(LONG id, const char *text, int failit)
{
    ROUT = "AddAppMenu";
    sprintf(VEC, "id %ld%s", id, failit ? " (fails)" : "");
    wbmark();
    DO2(ent(ADDMENU, 0), wbfail = failit; R[cur].d[0] = id; R[cur].d[1] = (ULONG)text);
    SAME("d0 = item (BUG(orig))", R[cur].d[0] ? R[cur].d[0] - nextobj[cur] : 1);
    wbcompare();
}

static void addwindow(LONG n, int failit)
{
    ROUT = "AddAppWindow";
    sprintf(VEC, "window %ld%s", n, failit ? " (fails)" : "");
    wbmark();
    DO2(ent(ADDWINDOW, 0), wbfail = failit; R[cur].d[0] = n; R[cur].a[0] = (ULONG)&fakewin[cur];
        frame[cur][0] = n; R[cur].a[2] = (ULONG)&frame[cur][1]);
    SAME("d0", R[cur].d[0]);
    wbcompare();
}

static void addicon(int form, LONG id, const char *name, LONG x, LONG y, int failit)
{
    ROUT = "AddAppIcon";
    sprintf(VEC, "form %d id %ld %s%s", form, id, name, failit ? " (fails)" : "");
    wbmark();
    DO2(ent(ADDICON, form), wbfail = failit; R[cur].d[0] = id; R[cur].d[1] = (ULONG)"App icon";
        R[cur].d[2] = (ULONG)name; R[cur].d[3] = x; R[cur].d[4] = y);
    SAME("d0", R[cur].d[0]);
    wbcompare();
    /* BUG(orig): a3 = the DiskObject, a4 = 0 once AddAppIconA was called */
    SAME("a3 = DiskObject", nlog[cur] > mark[cur] ? R[cur].a[3] == wblog[cur][nlog[cur] - 1].a3
         : R[cur].a[3] == IN[cur].a[3]);
    SAME("a4", R[cur].a[4] == IN[cur].a[4] ? 1 : R[cur].a[4]);
    wbfail = 0;
}

static void stringfn(const char *name, int idx, LONG num)
{
    ROUT = name;
    sprintf(VEC, "%ld", num);
    for (cur = 0; cur < 2; cur++)
        memset(out[cur], 0xEE, sizeof out[cur]);
    DO2(ent(idx, 0), R[cur].d[0] = num; R[cur].a[3] = (ULONG)out[cur] + 4);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
    samebytes("out", out[0], out[1], sizeof out[0]);
    SAME("wa_Lock[0]", args[cur][0].wa_Lock == rootlock[cur]);
    SAME("wa_Lock[1]", args[cur][1].wa_Lock == dirlock[cur]);
}

static void before_init(void)
{
    static const int fns[] = { APPEVENT, APPEVENTTYPE, APPEVENTID, NEXTAPPFILE, APPNUMFILES,
                               APPLOCK, APPMESSAGE, APPMSGPORT };
    unsigned i;

    strcpy(VEC, "before init");
    for (i = 0; i < sizeof fns / sizeof fns[0]; i++)
        call_noargs("no init", fns[i]);
    call_id("DelAppWindow", DELWINDOW, 1);
    call_id("DelAppMenu", DELMENU, 1);
    call_id("DelAppIcon", DELICON, 1);
    call_id("AppFile", APPFILE, 1);
    addmenu(1, "x", 0);
    addwindow(1, 0);
    addicon(0, 1, "RAM:rweapp", 0, 0, 0);
    addicon(1, 1, "RAM:rweapp", 5, 6, 0);
}

static void test_messages(void)
{
    int i;

    for (cur = 0; cur < 2; cur++) {
        rootlock[cur] = Lock((STRPTR)"RAM:", SHARED_LOCK);
        dirlock[cur] = Lock((STRPTR)"RAM:rweappdir", SHARED_LOCK);
        args[cur][0].wa_Lock = rootlock[cur];
        args[cur][0].wa_Name = (STRPTR)"file1";
        args[cur][1].wa_Lock = dirlock[cur];
        args[cur][1].wa_Name = (STRPTR)"x.txt";
    }
    check("locks", rootlock[0] && rootlock[1] && dirlock[0] && dirlock[1]);

    strcpy(VEC, "no message");
    call_noargs("AppEvent", APPEVENT);
    call_noargs("AppMessage", APPMESSAGE);
    call_noargs("AppEventType", APPEVENTTYPE);

    /* window, icon, menu, other type */
    for (i = 0; i < 4; i++) {
        static const UWORD types[] = { AMTYPE_APPWINDOW, AMTYPE_APPICON, AMTYPE_APPMENUITEM, 3 };
        for (cur = 0; cur < 2; cur++)
            putmsg(i, types[i], 0x100 + i, i == 0 ? 2 : i);
    }
    for (i = 0; i < 4; i++) {
        sprintf(VEC, "message %d", i);
        ROUT = "AppEvent";
        DO2(ent(APPEVENT, 0), (void)0);
        SAME("d0", R[cur].d[0]);
        SAME("replies", replies());
        call_noargs("AppEventType", APPEVENTTYPE);
        call_noargs("AppEventID", APPEVENTID);
        call_noargs("AppNumFiles", APPNUMFILES);
        ROUT = "AppMessage";
        DO2(ent(APPMESSAGE, 0), (void)0);
        SAME("d0 = message", R[cur].d[0] == (ULONG)&am[cur][i]);
        ROUT = "AppLock";
        DO2(ent(APPLOCK, 0), (void)0);
        SAME("d0 = lock", R[cur].d[0] == (ULONG)args[cur][0].wa_Lock);
        if (i == 0) {
            /* the first argument: "Ram Disk:" ends with ':' - the lock
               stays; the second is unlocked (BUG(orig)) */
            stringfn("NextAppFile", NEXTAPPFILE, 0);
            stringfn("NextAppFile", NEXTAPPFILE, 0);
            stringfn("NextAppFile (none left)", NEXTAPPFILE, 0);
            stringfn("AppFile", APPFILE, 1);
            stringfn("AppFile (lock 0 now)", APPFILE, 2);
            stringfn("AppFile", APPFILE, 3);
            stringfn("AppFile", APPFILE, 100);
            stringfn("NextAppFile (argnum kept)", NEXTAPPFILE, 0);
        }
    }
    strcpy(VEC, "last");
    ROUT = "AppEvent";
    DO2(ent(APPEVENT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    SAME("replies", replies());
    call_noargs("AppMessage", APPMESSAGE);
    call_noargs("AppEventType", APPEVENTTYPE);
    for (cur = 0; cur < 2; cur++)
        UnLock(rootlock[cur]);
}

static void test_objects(void)
{
    addmenu(3, "Menu", 0);
    addmenu(3, "Menu 2", 0);            /* replaces: RemoveAppMenuItem first */
    addmenu(5, "Menu 5", 1);
    call_id("DelAppMenu", DELMENU, 3);
    call_id("DelAppMenu", DELMENU, 3);
    call_id("DelAppMenu", DELMENU, 5);

    addwindow(2, 0);
    addwindow(2, 0);
    addwindow(4, 1);
    call_id("DelAppWindow", DELWINDOW, 2);
    call_id("DelAppWindow (empty: NULL)", DELWINDOW, 7);

    addicon(0, 1, "RAM:rweapp", 0, 0, 0);
    addicon(1, 1, "RAM:rweapp", 10, 20, 0);     /* replaces */
    addicon(1, 2, "RAM:rweapp", -3, 0x7fff, 0);
    addicon(0, 3, "RAM:rwe-no-such-icon", 0, 0, 0);
    addicon(0, 4, "RAM:rweapp", 0, 0, 1);
    call_id("DelAppIcon", DELICON, 1);
    call_id("DelAppIcon", DELICON, 1);
    call_id("DelAppIcon (failed add)", DELICON, 4);
}

static const char *names[] = { "riapplib", NULL };

int main(int argc, char **argv)
{
    struct Library *wb, *icon;
    struct DiskObject *d;
    BPTR l;
    unsigned i;

    if (argc < 2) {
        printf("usage: rwe_riapplib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    wb = OpenLibrary((STRPTR)"workbench.library", 37);
    icon = OpenLibrary((STRPTR)"icon.library", 37);
    reply[0] = CreateMsgPort();
    reply[1] = CreateMsgPort();
    if (!wb || !icon || !reply[0] || !reply[1]) {
        check("workbench/icon.library, port", FALSE);
        return 20;
    }
    iconbase = icon;
    d = GetDefDiskObject(WBPROJECT);
    check("icon RAM:rweapp", d && PutDiskObject((STRPTR)"RAM:rweapp", d));
    if (d)
        FreeDiskObject(d);
    l = CreateDir((STRPTR)"RAM:rweappdir");
    if (l)
        UnLock(l);

    Forbid();
    for (i = 0; i < NPATCH; i++)
        oldvec[i] = SetFunction(wb, patches[i].lvo, (APTR)patches[i].fn);
    Permit();

    /* inline AppEventIDCMP: prefix, length and code */
    ROUT = "AppEventIDCMP";
    strcpy(VEC, "inline");
    SAME("$a000,len", L(ent(APPEVENTIDCMP, 0)));
    SAME("code", L(ent(APPEVENTIDCMP, 0) + 4));
    SAME("code+4", W(ent(APPEVENTIDCMP, 0) + 8));

    before_init();

    ROUT = "init";
    strcpy(VEC, "");
    DO2(ent(INIT, 0), (void)0);
    getport();

    test_objects();
    test_messages();

    /* finit with no current message: BUG(orig) - the queue stays */
    for (cur = 0; cur < 2; cur++) {
        putmsg(4, AMTYPE_APPICON, 1, 0);
    }
    addmenu(0, "m0", 0);
    addmenu(1, "m1", 0);
    addwindow(3, 0);
    addicon(0, 0, "RAM:rweapp", 0, 0, 0);
    ROUT = "finit";
    strcpy(VEC, "no current message");
    wbmark();
    DO2(ent(FINIT, 0), (void)0);
    wbcompare();
    SAME("replies (none)", replies());

    /* second run: finit with a current message replies to all */
    ROUT = "init";
    strcpy(VEC, "again");
    DO2(ent(INIT, 0), (void)0);
    getport();
    for (cur = 0; cur < 2; cur++) {
        putmsg(4, AMTYPE_APPWINDOW, 7, 0);
        putmsg(5, AMTYPE_APPMENUITEM, 8, 0);
    }
    strcpy(VEC, "current message");
    call_noargs("AppEvent", APPEVENT);
    SAME("replies", replies());
    ROUT = "finit";
    wbmark();
    DO2(ent(FINIT, 0), (void)0);
    wbcompare();
    SAME("replies (both)", replies());

    Forbid();
    for (i = 0; i < NPATCH; i++)
        SetFunction(wb, patches[i].lvo, oldvec[i]);
    Permit();
    DeleteFile((STRPTR)"RAM:rweapp.info");
    DeleteFile((STRPTR)"RAM:rweappdir");
    DeleteMsgPort(reply[0]);
    DeleteMsgPort(reply[1]);
    CloseLibrary(icon);
    CloseLibrary(wb);
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
