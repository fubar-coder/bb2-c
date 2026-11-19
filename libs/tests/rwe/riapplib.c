/*
 * rwe riapplib - riapplib (userlib 61): the C build or the CD binary
 * (BSS2 RIAPPLIB.OBJ), one set as LIBSET says, observed as cases
 * (sysload.h; the expected data
 * libs/tests/expected/rwe_riapplib.txt), in the emulator (snitch test
 * case "rwe riapplib", argv[1] = the directory with riapplib.obj/.orig).
 *
 * The library is loaded like the compiler does (sysload.h) and every
 * routine is called (other registers = markers); observed are d0, a3/a4
 * where the routine sets them, the string results, the calls into
 * workbench.library and their arguments (pointers into the test program,
 * the window and the texts, as prog), the replies to the messages and
 * the kept registers (KEEPS).
 *
 * workbench.library: AddAppWindowA ... RemoveAppMenuItem are replaced
 * with SetFunction by stubs that log the call and return a fake object
 * (or 0, to test the failure paths) - no Workbench is needed, and the
 * objects never reach Workbench.  icon.library and dos.library are real:
 * the App icon comes from an icon written to RAM:, the WBArgs of the
 * App messages are locks on RAM:.  The App messages are put to
 * AppMsgPort by the test.  icon.library's FreeDiskObject is patched to
 * count its calls, exec's OpenLibrary for one init to fail icon.library.
 * The third run starts as a second run from the editor with the library
 * still loaded: the message routines and the first AppEvent before any
 * new message, after the run before ended with a current message.
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

static struct regs R, IN;
static ULONG POP;

/* after a call: its kept case (KEEPS of ROUT) and the bytes it popped */
static void preserve(void)
{
    obs_kept(&IN, &R);
    OBS("popped", POP);
}

#define DO1(fn, setup) do {                                                 \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callstk(fn, &R, NULL, 0, &POP);                                     \
        preserve();                                                         \
    } while (0)

/* code of form `form` of entry idx; Amiga-only code addresses are odd */
static UBYTE *ent(int idx, int form)
{
    UBYTE *p = entry_form(getlib(&loaded, "riapplib"), idx, form);
    return (UBYTE *)((ULONG)p & ~1UL);
}

enum { ADDWINDOW, DELWINDOW, ADDMENU, DELMENU, ADDICON, DELICON, APPEVENT,
       APPEVENTTYPE, APPEVENTID, NEXTAPPFILE, APPFILE, APPNUMFILES, APPLOCK,
       APPMESSAGE, APPMSGPORT, APPEVENTIDCMP, FINIT, INIT };

/* ------------------------------------------------- workbench.library stubs */

enum { F_ADDWIN = 1, F_REMWIN, F_ADDICON, F_REMICON, F_ADDMENU, F_REMMENU };

struct wbcall { int fn; ULONG d0, d1, a0, a1, a2, a3, a4; LONG cx, cy; };

#define MAXLOG 64
static struct wbcall wblog[MAXLOG];
static int nlog;
static ULONG wbfail;                    /* the next Add... returns 0 */
static ULONG nextobj;

static ULONG logcall(int fn, ULONG d0, ULONG d1, ULONG a0, ULONG a1, ULONG a2,
                     ULONG a3, ULONG a4)
{
    struct wbcall *c;
    if (nlog == MAXLOG)
        return 0;
    c = &wblog[nlog++];
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
        return nextobj += 0x10;
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

/* icon.library's FreeDiskObject (-90) counts its calls and goes on to
   the system's routine; exec's OpenLibrary (-552), for the test task
   only, returns 0 for its ap_olfail-th call once ap_olfail is set (init
   opens workbench, dos and icon.library in this order: 3 fails
   icon.library) */
struct Task *ap_task;
ULONG ap_nfree, ap_olfail;
APTR ap_old_free, ap_old_open;
void ap_free(void), ap_open(void);
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _ap_free\n"
"_ap_free:\n"
"   addq.l  #1,_ap_nfree\n"
"   move.l  _ap_old_free,-(sp)\n"
"   rts\n"
"   .globl  _ap_open\n"
"_ap_open:\n"
"   tst.l   _ap_olfail\n"
"   beq.s   1f\n"
"   move.l  a0,-(sp)\n"
"   move.l  4.w,a0\n"
"   move.l  276(a0),a0\n"               /* ThisTask */
"   cmp.l   _ap_task,a0\n"
"   move.l  (sp)+,a0\n"                 /* movea: the flags stay */
"   bne.s   1f\n"
"   subq.l  #1,_ap_olfail\n"
"   bne.s   1f\n"
"   moveq   #0,d0\n"
"   rts\n"
"1: move.l  _ap_old_open,-(sp)\n"
"   rts\n"
);

/* observe the calls logged since wbmark() */
static ULONG port;
static int mark;

static void wbmark(void)
{
    mark = nlog;
}

static void wbcompare(void)
{
    int i;
    OBS("wb calls", nlog - mark);
    for (i = 0; i < nlog - mark; i++) {
#define C (&wblog[mark + i])
        OBS("wb fn", C->fn);
        OBSPTR("wb d0", C->d0, NULL, 0);
        OBSPTR("wb d1", C->d1, NULL, 0);
        OBSPTR("wb a0", C->a0, NULL, 0);
        OBS("wb a1 = AppMsgPort", C->fn & 1 ? C->a1 == port : 1);
        OBSPTR("wb a2", C->a2, NULL, 0);
        OBS("wb a3 = DiskObject", C->a3 != 0);
        OBSPTR("wb a4", C->a4, NULL, 0);
        OBS("wb CurrentX", C->cx);
        OBS("wb CurrentY", C->cy);
#undef C
    }
}

/* ------------------------------------------------------------- the test */

static UBYTE out[300];
static ULONG frame[4];
static struct Window *fakewin = (struct Window *)0x00a0b0c0;
static struct MsgPort *reply;
static struct AppMessage am[6];
static struct WBArg args[3];
static BPTR rootlock, dirlock;

static int replies(void)
{
    int n = 0;
    while (GetMsg(reply))
        n++;
    return n;
}

static void putmsg(int i, UWORD type, ULONG id, LONG nargs)
{
    struct AppMessage *m = &am[i];
    memset(m, 0, sizeof *m);
    m->am_Message.mn_Node.ln_Type = NT_MESSAGE;
    m->am_Message.mn_ReplyPort = reply;
    m->am_Message.mn_Length = sizeof *m;
    m->am_Type = type;
    m->am_ID = id;
    m->am_NumArgs = nargs;
    m->am_ArgList = args;
    PutMsg((struct MsgPort *)port, &m->am_Message);
}

static void call_noargs(const char *name, int idx)
{
    ROUT = name;
    DO1(ent(idx, 0), (void)0);
    OBS("d0", R.d[0]);
}

/* AppMsgPort after init: the library's port */
static void getport(void)
{
    ROUT = "AppMsgPort";
    DO1(ent(APPMSGPORT, 0), (void)0);
    port = R.d[0];
    OBS("port", port != 0);
}

static void call_id(const char *name, int idx, LONG id)
{
    ROUT = name;
    sprintf(VEC, "id %ld", id);
    wbmark();
    DO1(ent(idx, 0), R.d[0] = id);
    OBS("d0", R.d[0]);
    wbcompare();
}

static void addmenu(LONG id, const char *text, int failit)
{
    ROUT = "AddAppMenu";
    sprintf(VEC, "id %ld%s", id, failit ? " (fails)" : "");
    wbmark();
    DO1(ent(ADDMENU, 0), wbfail = failit; R.d[0] = id; R.d[1] = (ULONG)text);
    OBS("d0 = item (BUG(orig) #518)", R.d[0] ? R.d[0] - nextobj : 1);
    wbcompare();
}

static void addwindow(LONG n, int failit)
{
    ROUT = "AddAppWindow";
    sprintf(VEC, "window %ld%s", n, failit ? " (fails)" : "");
    wbmark();
    DO1(ent(ADDWINDOW, 0), wbfail = failit; R.d[0] = n; R.a[0] = (ULONG)&fakewin;
        frame[0] = n; R.a[2] = (ULONG)&frame[1]);
    OBS("d0", R.d[0]);
    wbcompare();
}

static void addicon(int form, LONG id, const char *name, LONG x, LONG y, int failit)
{
    ROUT = "AddAppIcon";
    sprintf(VEC, "form %d id %ld %s%s", form, id, name, failit ? " (fails)" : "");
    wbmark();
    DONTCARE = 1UL << 12;               /* a4: observed below (#516) */
    DO1(ent(ADDICON, form), wbfail = failit; R.d[0] = id; R.d[1] = (ULONG)"App icon";
        R.d[2] = (ULONG)name; R.d[3] = x; R.d[4] = y);
    OBS("d0", R.d[0]);
    wbcompare();
    /* the original (#516): a3 = the DiskObject, a4 = 0 once AddAppIconA
       was called; fixed: a3 = text$, a4 kept */
    OBS("a3 = DiskObject", nlog > mark ? R.a[3] == wblog[nlog - 1].a3
        : R.a[3] == IN.a[3]);
    OBS("a4", R.a[4] == IN.a[4] ? 1 : R.a[4]);
    DONTCARE = 0;
    wbfail = 0;
}

static void stringfn(const char *name, int idx, LONG num)
{
    ROUT = name;
    sprintf(VEC, "%ld", num);
    memset(out, 0xEE, sizeof out);
    DO1(ent(idx, 0), R.d[0] = num; R.a[3] = (ULONG)out + 4);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)out);
    OBSBYTES("out", out, sizeof out);
    OBS("wa_Lock[0]", args[0].wa_Lock == rootlock);
    OBS("wa_Lock[1]", args[1].wa_Lock == dirlock);
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

    rootlock = Lock((STRPTR)"RAM:", SHARED_LOCK);
    dirlock = Lock((STRPTR)"RAM:rweappdir", SHARED_LOCK);
    args[0].wa_Lock = rootlock;
    args[0].wa_Name = (STRPTR)"file1";
    args[1].wa_Lock = dirlock;
    args[1].wa_Name = (STRPTR)"x.txt";
    check("locks", rootlock && dirlock);

    strcpy(VEC, "no message");
    call_noargs("AppEvent", APPEVENT);
    call_noargs("AppMessage", APPMESSAGE);
    call_noargs("AppEventType", APPEVENTTYPE);
    call_noargs("AppEventID", APPEVENTID);

    /* window, icon, menu, other type */
    for (i = 0; i < 4; i++) {
        static const UWORD types[] = { AMTYPE_APPWINDOW, AMTYPE_APPICON, AMTYPE_APPMENUITEM, 3 };
        putmsg(i, types[i], 0x100 + i, i == 0 ? 2 : i);
    }
    for (i = 0; i < 4; i++) {
        sprintf(VEC, "message %d", i);
        ROUT = "AppEvent";
        DO1(ent(APPEVENT, 0), (void)0);
        OBS("d0", R.d[0]);
        OBS("replies", replies());
        call_noargs("AppEventType", APPEVENTTYPE);
        call_noargs("AppEventID", APPEVENTID);
        call_noargs("AppNumFiles", APPNUMFILES);
        ROUT = "AppMessage";
        DO1(ent(APPMESSAGE, 0), (void)0);
        OBS("d0 = message", R.d[0] == (ULONG)&am[i]);
        ROUT = "AppLock";
        DO1(ent(APPLOCK, 0), (void)0);
        OBS("d0 = lock", R.d[0] == (ULONG)args[0].wa_Lock);
        if (i == 0) {
            /* the first argument: "Ram Disk:" ends with ':' - the lock
               stays; the second is unlocked (BUG(orig), #521) */
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
    DO1(ent(APPEVENT, 0), (void)0);
    OBS("d0", R.d[0]);
    OBS("replies", replies());
    call_noargs("AppMessage", APPMESSAGE);
    call_noargs("AppEventType", APPEVENTTYPE);
    call_noargs("AppEventID", APPEVENTID);
    UnLock(rootlock);
    if (args[1].wa_Lock == dirlock)     /* not unlocked by NextAppFile */
        UnLock(dirlock);
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

/* AddAppIcon(id,"App icon","RAM:rweapp") in the third run: d0, the
   workbench calls and the FreeDiskObject calls (a3/a4: the cases of
   addicon) */
static void addicon_freed(LONG id, int failit)
{
    ULONG n = ap_nfree;

    ROUT = "AddAppIcon";
    sprintf(VEC, "third run id %ld%s", id, failit ? " (fails)" : "");
    wbmark();
    DONTCARE = 1UL << 12;               /* a4: the cases of addicon */
    DO1(ent(ADDICON, 0), wbfail = failit; R.d[0] = id; R.d[1] = (ULONG)"App icon";
        R.d[2] = (ULONG)"RAM:rweapp");
    OBS("d0", R.d[0]);
    wbcompare();
    OBS("FreeDiskObject calls", ap_nfree - n);
    DONTCARE = 0;
    wbfail = 0;
}

/* third run: AddAppIconA fails for a used id and for a free one; the App
   icon removed before and the new DiskObject (#517) */
static void test_icon_fails(void)
{
    ULONG n;

    nlog = 0;                           /* the log is full after two runs */
    ROUT = "init";
    strcpy(VEC, "third run");
    DO1(ent(INIT, 0), (void)0);
    getport();
    /* the second run's finit replied to its current message am[4]; this
       run starts as from the editor with the library still loaded: what
       the message routines see before the first AppEvent, and whether
       that AppEvent replies to am[4] again */
    ROUT = "AppMessage";
    DO1(ent(APPMESSAGE, 0), (void)0);
    OBS("d0 = message of the run before", R.d[0] == (ULONG)&am[4]);
    call_noargs("AppEventType", APPEVENTTYPE);
    call_noargs("AppEventID", APPEVENTID);
    ROUT = "AppEvent";
    DO1(ent(APPEVENT, 0), (void)0);
    OBS("d0", R.d[0]);
    OBS("replies", replies());
    addicon_freed(1, 0);
    addicon_freed(1, 1);
    n = ap_nfree;
    call_id("DelAppIcon (after a failed replace)", DELICON, 1);
    OBS("FreeDiskObject calls", ap_nfree - n);
    addicon_freed(2, 1);
    n = ap_nfree;
    ROUT = "finit";
    strcpy(VEC, "third run");
    wbmark();
    DO1(ent(FINIT, 0), (void)0);
    wbcompare();
    OBS("FreeDiskObject calls", ap_nfree - n);
}

/* init when icon.library does not open: AppMsgPort and the opens of
   workbench and dos.library left behind (#526); finit afterwards */
static void test_init_fails(void)
{
    struct Library *wb = OpenLibrary((STRPTR)"workbench.library", 37);
    struct Library *dos = (struct Library *)DOSBase;
    UWORD wbcnt, doscnt;
    LONG wbleft, dosleft;
    ULONG failed;

    /* DO1 with the call under Forbid: no other task opens or closes the
       two libraries in between (the observations are written after) */
    strcpy(VEC, "icon.library fails");
    ROUT = "init";
    ap_task = FindTask(NULL);
    markregs(&R);
    IN = R;
    Forbid();
    ap_old_open = SetFunction((struct Library *)SysBase, -552, (APTR)ap_open);
    wbcnt = wb->lib_OpenCnt;
    doscnt = dos->lib_OpenCnt;
    ap_olfail = 3;
    callstk(ent(INIT, 0), &R, NULL, 0, &POP);
    failed = ap_olfail == 0;
    ap_olfail = 0;
    wbleft = wb->lib_OpenCnt - wbcnt;
    dosleft = dos->lib_OpenCnt - doscnt;
    while (wb->lib_OpenCnt > wbcnt)
        CloseLibrary(wb);
    while (dos->lib_OpenCnt > doscnt)
        CloseLibrary(dos);
    SetFunction((struct Library *)SysBase, -552, ap_old_open);
    Permit();
    preserve();
    OBS("OpenLibrary failed", failed);
    OBS("workbench.library opens left", wbleft);
    OBS("dos.library opens left", dosleft);
    ROUT = "AppMsgPort";
    DO1(ent(APPMSGPORT, 0), (void)0);
    OBS("port", R.d[0] != 0);
    ROUT = "finit";
    wbmark();
    DO1(ent(FINIT, 0), (void)0);
    wbcompare();
    CloseLibrary(wb);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("no init", d1-d7|a0-a3|a6);
    KEEPS("DelAppWindow", d2-d6|a2|a3);
    KEEPS("DelAppMenu", d2-d6|a2|a3);
    KEEPS("DelAppIcon", d2-d5|a2|a3);
    KEEPS("AppFile", d4|a2);
    KEEPS("AddAppMenu", d2-d6|a3);
    KEEPS("AddAppWindow", d2-d5|a3);
    KEEPS("AddAppIcon", d4|d5);
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("AppMsgPort", d1-d7|a0-a3|a6);
    KEEPS("DelAppWindow (empty: NULL)", d2-d6|a2|a3);
    KEEPS("DelAppIcon (failed add)", d1-d5|d7|a0|a2|a3);
    KEEPS("DelAppIcon (after a failed replace)", d2-d5|a2|a3);
    KEEPS("AppEvent", d2-d7|a2|a3);
    KEEPS("AppMessage", d1-d7|a0-a3|a6);
    KEEPS("AppEventType", d1-d7|a0-a3|a6);
    KEEPS("AppEventID", d1-d7|a0-a3|a6);
    KEEPS("AppNumFiles", d1-d7|a0-a3|a6);
    KEEPS("AppLock", d1-d7|a1-a3|a6);
    KEEPS("NextAppFile", d4|a2);
    KEEPS("NextAppFile (none left)", d0-d7|a1-a3);
    KEEPS("AppFile (lock 0 now)", d4|a2);
    KEEPS("NextAppFile (argnum kept)", d0-d7|a1-a3);
    KEEPS("finit", d0-d7|a0-a3|a6);
    wb = OpenLibrary((STRPTR)"workbench.library", 37);
    icon = OpenLibrary((STRPTR)"icon.library", 37);
    reply = CreateMsgPort();
    if (!wb || !icon || !reply) {
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
    ap_old_free = SetFunction(icon, -90, (APTR)ap_free);
    Permit();

    /* inline AppEventIDCMP: prefix, length and code */
    ROUT = "AppEventIDCMP";
    strcpy(VEC, "inline");
    OBS("$a000,len", L(ent(APPEVENTIDCMP, 0)));
    OBS("code", L(ent(APPEVENTIDCMP, 0) + 4));
    OBS("code+4", W(ent(APPEVENTIDCMP, 0) + 8));

    before_init();

    ROUT = "init";
    strcpy(VEC, "");
    DO1(ent(INIT, 0), (void)0);
    getport();

    test_objects();
    test_messages();

    /* finit with no current message: BUG(orig) (#525) - the queue stays */
    putmsg(4, AMTYPE_APPICON, 1, 0);
    addmenu(0, "m0", 0);
    addmenu(1, "m1", 0);
    addwindow(3, 0);
    addicon(0, 0, "RAM:rweapp", 0, 0, 0);
    ROUT = "finit";
    strcpy(VEC, "no current message");
    wbmark();
    DO1(ent(FINIT, 0), (void)0);
    wbcompare();
    OBS("replies (none)", replies());

    /* second run: finit with a current message replies to all */
    ROUT = "init";
    strcpy(VEC, "again");
    DO1(ent(INIT, 0), (void)0);
    getport();
    putmsg(4, AMTYPE_APPWINDOW, 7, 0);
    putmsg(5, AMTYPE_APPMENUITEM, 8, 0);
    strcpy(VEC, "current message");
    call_noargs("AppEvent", APPEVENT);
    OBS("replies", replies());
    ROUT = "finit";
    wbmark();
    DO1(ent(FINIT, 0), (void)0);
    wbcompare();
    OBS("replies (both)", replies());

    test_icon_fails();
    test_init_fails();

    Forbid();
    for (i = 0; i < NPATCH; i++)
        SetFunction(wb, patches[i].lvo, oldvec[i]);
    SetFunction(icon, -90, ap_old_free);
    Permit();
    DeleteFile((STRPTR)"RAM:rweapp.info");
    DeleteFile((STRPTR)"RAM:rweappdir");
    DeleteMsgPort(reply);
    CloseLibrary(icon);
    CloseLibrary(wb);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
