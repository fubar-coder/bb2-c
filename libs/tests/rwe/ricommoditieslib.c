/*
 * rwe ricommoditieslib - RWE userlib 89: the C build or the CD binary
 * (BSS2 RICOMMODITIESLIB.OBJ), one set as LIBSET says, observed as cases
 * (sysload.h; the expected data
 * libs/tests/expected/rwe_ricommoditieslib.txt).
 *
 *   ricommoditieslib DIR      (DIR: ricommoditieslib.orig / .obj)
 *
 * The scenario runs once with init ... finit; every result is observed
 * (the check text names it, the value is the case), and every call writes
 * the kept case of its routine (KEEPS).  Observed are only values that do
 * not depend on addresses (results, "port != 0", message counts).
 *
 * The scenario: every command before MakeCommodity (no broker); the
 * inline constants Cx... (copied like the compiler does); MakeCommodity
 * while a broker of the test has the name (fails at CxBroker, the port is
 * made: SetCxStatus with a port and no broker); MakeCommodity
 * and a second one with the same name (fails, the first broker gets
 * CXCMD_UNIQUE: NBU_NOTIFY); the six other Exchange commands through
 * commodities' BrokerCommand, each fetched with CommodityEvent and looked
 * at with HotKeyHit, ExchangeMessage and all Exchange...; hotkeys on the
 * input event class "timer" (input.device sends timer events all the
 * time, so messages come without input): two slots, one replaced, one
 * with a description ParseIX does not understand, one whose sender cannot
 * be made (CreateCxObj(CX_SEND) made to fail); WaitCommodity with two
 * messages queued; SetCxStatus off/on/other; finit with a message of the
 * test queued.  The commodities calls of SetHotKey and finit, finit's
 * CloseLibrary and the library's ReplyMsg calls are noted through
 * patches (below).  The test does not reply the messages the library
 * takes from its port (whether the library replies them is observed); it
 * drains (and replies) what stays queued.
 *
 * Not covered: WaitCommodity on an empty port (with the CCR exec leaves
 * it waits for the next message - with no hotkey forever), real keys
 * (would reach the console), SetHotKey numbers outside 0-15, two
 * successful MakeCommodity (the first broker keeps the port finit
 * deletes), a missing commodities.library.
 */
#include "sysload.h"
#include <libraries/commodities.h>
#include <proto/commodities.h>

struct Library *CxBase;

#define LIBNAME "ricommoditieslib"

/* entries of the token table */
enum {
    E_MAKE, E_HOTKEY, E_HIT, E_WAIT, E_EVENT, E_XMSG,
    E_XDISABLE, E_XENABLE, E_XAPPEAR, E_XDISAPPEAR, E_XKILL, E_XLIST, E_XUNIQUE,
    E_CXDISABLE, E_CXENABLE, E_CXAPPEAR, E_CXDISAPPEAR, E_CXKILL, E_CXLIST, E_CXUNIQUE,
    E_STATUS, E_PORT, E_FINIT, E_INIT
};
static const char *ename[] = {
    "MakeCommodity", "SetHotKey", "HotKeyHit", "WaitCommodity", "CommodityEvent",
    "ExchangeMessage", "ExchangeDisable", "ExchangeEnable", "ExchangeAppear",
    "ExchangeDisAppear", "ExchangeKill", "ExchangeListChange", "ExchangeUnique",
    "CxDisable", "CxEnable", "CxAppear", "CxDisAppear", "CxKill", "CxListChange",
    "CxUnique", "SetCxStatus", "CommodityMsgPort", "finit", "init"
};

/* ------------------------------------------------------------ records */

/* the results and the calls, counted (observed at the end) */
static int nv, nc;

/* a result: the check text names it, the value is the case (routine
   part empty) */
static void rec(const char *what, ULONG v)
{
    OBS(what, v);
    nv++;
}

/* ------------------------------------------------------------ patches */

/* While the scenario runs, commodities' CreateCxObj, DeleteCxObj,
   DeleteCxObjAll, RemoveCxObj and exec's ReplyMsg and CloseLibrary are
   patched (SetFunction).  A stub saves d0-d1/a0-a1, calls hk_note(kind, regs)
   and then the old function with the registers it was called with, so
   the library sees what the real function leaves (a stub of commodities
   also returns through itself, to count nesting: a call that a patched
   call makes is not noted).  hk_note notes a call only while a routine
   of the library runs (in_lib), from this task in user mode (its stack;
   an interrupt runs on the supervisor stack).  A nonzero result of
   hk_note makes the call fail (d0 = 0): CreateCxObj(CX_SEND) while
   fail_send is set. */
enum { HK_CREATE, HK_DELETE, HK_DELETEALL, HK_REMOVE, HK_REPLY, HK_CLOSE, HK_N };
static const WORD hk_lvo[HK_N] = { -30, -48, -54, -102, -378, -414 };
APTR hk_old[HK_N];
ULONG hk_depth;
extern char hk_create[], hk_delete[], hk_deleteall[], hk_remove[], hk_reply[], hk_close[];
static char *const hk_stub[HK_N] = { hk_create, hk_delete, hk_deleteall, hk_remove, hk_reply,
                                     hk_close };

#define HK_CX(name, n)                                                      \
    "   .globl  _" #name "\n_" #name ":\n"                                  \
    "   movem.l d0-d1/a0-a1,-(sp)\n"                                        \
    "   move.l  sp,-(sp)\n"                                                 \
    "   move.l  #" #n ",-(sp)\n"                                            \
    "   jsr     _hk_note\n"                                                 \
    "   addq.l  #8,sp\n"                                                    \
    "   tst.l   d0\n"                                                       \
    "   movem.l (sp)+,d0-d1/a0-a1\n"    /* keeps the CCR */                 \
    "   bne.s   1f\n"                                                       \
    "   addq.l  #1,_hk_depth\n"                                             \
    "   pea     2f\n"                                                       \
    "   move.l  _hk_old+4*" #n ",-(sp)\n"                                   \
    "   rts\n"                                                              \
    "2: subq.l  #1,_hk_depth\n"                                             \
    "   rts\n"                                                              \
    "1: moveq   #0,d0\n"                                                    \
    "   rts\n"
/* an exec function: noted, then a jump to the old one */
#define HK_EXEC(name, n)                                                    \
    "   .globl  _" #name "\n_" #name ":\n"                                  \
    "   movem.l d0-d1/a0-a1,-(sp)\n"                                        \
    "   move.l  sp,-(sp)\n"                                                 \
    "   move.l  #" #n ",-(sp)\n"                                            \
    "   jsr     _hk_note\n"                                                 \
    "   addq.l  #8,sp\n"                                                    \
    "   movem.l (sp)+,d0-d1/a0-a1\n"                                        \
    "   move.l  _hk_old+4*" #n ",-(sp)\n"                                   \
    "   rts\n"
__asm__("   .text\n   .even\n"
        HK_CX(hk_create, 0) HK_CX(hk_delete, 1) HK_CX(hk_deleteall, 2) HK_CX(hk_remove, 3)
        HK_EXEC(hk_reply, 4) HK_EXEC(hk_close, 5));

static volatile int in_lib, fail_send;
static struct Task *me;
static ULONG hk_count[HK_N];            /* the calls noted, per kind */
static ULONG hk_closed;                 /* CloseLibrary(commodities) calls
                                           before the last DeleteCxObj or
                                           DeleteCxObjAll */
#define MAXREPLIED 32
static struct Message *replied[MAXREPLIED];
static int nreplied;

int hk_note(int kind, ULONG *regs);
int hk_note(int kind, ULONG *regs)
{
    if (!in_lib || SysBase->ThisTask != me
        || (APTR)regs < me->tc_SPLower || (APTR)regs >= me->tc_SPUpper)
        return 0;
    if (kind < HK_REPLY && hk_depth)
        return 0;
    if (kind == HK_CLOSE && regs[3] != (ULONG)CxBase)
        return 0;
    hk_count[kind]++;
    switch (kind) {
    case HK_CREATE:                     /* d0 = the type */
        return fail_send && regs[0] == CX_SEND;
    case HK_DELETE:
    case HK_DELETEALL:
        hk_closed = hk_count[HK_CLOSE];
        break;
    case HK_REPLY:                      /* a1 = the message */
        if (nreplied < MAXREPLIED)
            replied[nreplied++] = (struct Message *)regs[3];
        break;
    }
    return 0;
}

static void patch(int on)
{
    int i;

    Forbid();
    for (i = 0; i < HK_N; i++) {
        struct Library *base = i >= HK_REPLY ? (struct Library *)SysBase : CxBase;
        if (on)
            hk_old[i] = SetFunction(base, hk_lvo[i], (APTR)hk_stub[i]);
        else
            SetFunction(base, hk_lvo[i], hk_old[i]);
    }
    CacheClearU();
    Permit();
}

/* forget the calls noted so far */
static void hk_clear(void)
{
    memset(hk_count, 0, sizeof hk_count);
    hk_closed = 0;
    nreplied = 0;
}

/* whether the library replied m since hk_clear */
static ULONG was_replied(struct Message *m)
{
    int i;

    if (!m)
        return 0;
    for (i = 0; i < nreplied; i++)
        if (replied[i] == m)
            return 1;
    return 0;
}

/* the call of fn as routine `name` with the registers r: its kept case */
static void call_kept(const char *name, APTR fn, struct regs *r)
{
    struct regs in = *r;

    in_lib = 1;
    callregs(fn, r, 0, 0);
    in_lib = 0;
    ROUT = name;
    obs_kept(&in, r);
    ROUT = "";
    nc++;
}

/* call entry `e`; d0-d2 = in (n of them), the rest markers */
static ULONG cx(int e, int n, ULONG d0, ULONG d1, ULONG d2)
{
    struct lib *l = getlib(&loaded, LIBNAME);
    UBYTE *fn = (UBYTE *)((ULONG)entry_form(l, e, 0) & ~1UL);
    struct regs r;

    markregs(&r);
    if (n > 0) r.d[0] = d0;
    if (n > 1) r.d[1] = d1;
    if (n > 2) r.d[2] = d2;
    call_kept(ename[e], fn, &r);
    return r.d[0];
}
#define CX0(e) cx(e, 0, 0, 0, 0)
#define REC0(e) rec(ename[e], CX0(e))

/* an inline entry ($A000,len): the code copied with an RTS, as the
   compiler copies it into the program */
static UWORD tramp[16];
static void inline_call(int e)
{
    struct lib *l = getlib(&loaded, LIBNAME);
    UBYTE *code = (UBYTE *)((ULONG)entry_form(l, e, 0) & ~1UL);
    UWORD len = W(code + 2);
    struct regs r;

    rec("inline $A000", W(code) == 0xa000 && len > 0 && len < 24);
    if (W(code) != 0xa000 || len == 0 || len >= 24)
        return;
    memcpy(tramp, code + 4, len);
    tramp[len / 2] = 0x4e75;
    CacheClearU();
    markregs(&r);
    call_kept(ename[e], tramp, &r);
    rec(ename[e], r.d[0]);
}

/* ------------------------------------------------------------ messages */

static LONG brokercommand(const char *name, LONG id)
{
    /* commodities' private BrokerCommand(name,id)(a0,d0), LVO -198, what
       Exchange uses to send the CXCMD_ commands */
    register struct Library *a6 __asm("a6") = CxBase;
    register const char *a0 __asm("a0") = name;
    register LONG d0 __asm("d0") = id;

    __asm volatile("jsr -198(a6)" : "+d"(d0), "+a"(a0) : "a"(a6) : "d1", "a1", "cc", "memory");
    return d0;
}

static ULONG msgcount(struct MsgPort *p)
{
    struct Node *n;
    ULONG c = 0;

    Forbid();
    for (n = p->mp_MsgList.lh_Head; n->ln_Succ; n = n->ln_Succ)
        c++;
    Permit();
    return c;
}

static BOOL queued(struct MsgPort *p, struct Message *m)
{
    struct Node *n;
    BOOL in = FALSE;

    Forbid();
    for (n = p->mp_MsgList.lh_Head; n->ln_Succ; n = n->ln_Succ)
        if (n == &m->mn_Node)
            in = TRUE;
    Permit();
    return in;
}

/* wait (at most 2 s) until n messages are queued */
static ULONG waitmsgs(struct MsgPort *p, ULONG n)
{
    int t;

    for (t = 0; t < 100 && msgcount(p) < n; t++)
        Delay(1);
    return msgcount(p) >= n;
}

static void drain(struct MsgPort *p)
{
    struct Message *m;

    while ((m = GetMsg(p)))
        ReplyMsg(m);
}

static struct MsgPort *port;
static struct Message *held;      /* the message the library took last */

static struct Message *head(void)
{
    struct Message *m;

    Forbid();
    m = (struct Message *)port->mp_MsgList.lh_Head;
    if (!m->mn_Node.ln_Succ)
        m = NULL;
    Permit();
    return m;
}

/* after a call that fetches a message: whether the library replied the
   one it took before (the test does not reply what the library takes) */
static void replied_before(const char *what)
{
    rec(what, was_replied(held));
}

/* all evaluating functions on the current message */
static void look(void)
{
    int e;

    REC0(E_HIT);
    REC0(E_XMSG);
    for (e = E_XDISABLE; e <= E_XUNIQUE; e++)
        REC0(e);
}

/* CommodityEvent and a look at the message */
static void event_round(void)
{
    struct Message *before = head(), *removed = NULL;

    hk_clear();
    REC0(E_EVENT);
    replied_before("CommodityEvent replied the message taken before");
    if (before && !queued(port, before))
        removed = before;
    rec("event took the head", removed == before);
    held = removed;
    look();
}

/* SetHotKey n,desc and the commodities calls it made */
static void hotkey(const char *what, LONG n, const char *desc)
{
    static const char *kname[] = { "CreateCxObj", "DeleteCxObj", "DeleteCxObjAll",
                                   "RemoveCxObj" };
    char t[80];
    int k;

    hk_clear();
    rec(what, cx(E_HOTKEY, 2, (ULONG)n, (ULONG)desc, 0));
    for (k = HK_CREATE; k <= HK_REMOVE; k++) {
        strcpy(t, what);
        strcat(t, ": ");
        strcat(t, kname[k]);
        rec(t, hk_count[k]);
    }
}

/* every message that is queued, one by one */
static void all_events(void)
{
    int i;

    rec("queued", msgcount(port));
    for (i = 0; i < 4 && msgcount(port); i++)
        event_round();
}

/* ------------------------------------------------------------ scenario */

/* shorter than CBD_NAMELEN (24): commodities keeps 23 characters and
   compares those with the full name (a longer name is never "unique") */
static const char bname[] = "RWE commodity test";
static const char btitle[] = "ricommoditieslib";
static const char bdescr[] = "libs_tests: C build against the original";
static const char timer[] = "timer";
static const char nonsense[] = "rawkey nonsense_qualifier q";

static void scenario(void)
{
    static const LONG cmds[] = { CXCMD_DISABLE, CXCMD_ENABLE, CXCMD_APPEAR,
                                 CXCMD_DISAPPEAR, CXCMD_KILL, CXCMD_LIST_CHG };
    struct Message *before, own;
    struct MsgPort *rp = NULL;
    unsigned i;
    int e;

    CX0(E_INIT);

    /* no broker yet */
    port = NULL;
    REC0(E_HIT);
    REC0(E_WAIT);
    REC0(E_EVENT);
    REC0(E_XMSG);
    for (e = E_XDISABLE; e <= E_XUNIQUE; e++)
        REC0(e);
    rec("SetHotKey (no broker)", cx(E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec("SetCxStatus -1 (no port)", cx(E_STATUS, 1, (ULONG)-1, 0, 0));
    rec("SetCxStatus 0 (no port)", cx(E_STATUS, 1, 0, 0, 0));
    REC0(E_PORT);
    for (e = E_CXDISABLE; e <= E_CXUNIQUE; e++)
        inline_call(e);

    /* the name taken by a broker of the test: MakeCommodity makes its
       port and fails at CxBroker (NBU_UNIQUE, no notification), so there
       is a port without a broker; the next MakeCommodity keeps the port */
    {
        struct NewBroker nb = { .nb_Version = NB_VERSION, .nb_Name = (STRPTR)bname,
                                .nb_Title = (STRPTR)"test", .nb_Descr = (STRPTR)"name taken",
                                .nb_Unique = NBU_UNIQUE };
        CxObj *taker = CxBroker(&nb, NULL);

        rec("own broker", taker != NULL);
        rec("MakeCommodity (name taken)",
            cx(E_MAKE, 3, (ULONG)bname, (ULONG)btitle, (ULONG)bdescr));
        rec("CommodityMsgPort != 0 (name taken)", CX0(E_PORT) != 0);
        rec("SetCxStatus -1 (port, no broker)", cx(E_STATUS, 1, (ULONG)-1, 0, 0));
        rec("SetCxStatus 0 (port, no broker)", cx(E_STATUS, 1, 0, 0, 0));
        DeleteCxObj(taker);
    }

    /* the broker */
    rec("MakeCommodity", cx(E_MAKE, 3, (ULONG)bname, (ULONG)btitle, (ULONG)bdescr));
    port = (struct MsgPort *)CX0(E_PORT);
    rec("CommodityMsgPort != 0", port != NULL);
    if (!port)
        goto out;
    rec("broker there", brokercommand(bname, CXCMD_ENABLE) == 0);
    waitmsgs(port, 1);
    all_events();
    /* the same name again: fails, the first broker gets CXCMD_UNIQUE */
    rec("MakeCommodity again", cx(E_MAKE, 3, (ULONG)bname, (ULONG)"t2", (ULONG)"d2"));
    /* the cases of this message have their own vector, so the occurrence
       numbers of the later rounds do not depend on it */
    strcpy(VEC, "unique message");
    rec("unique message", waitmsgs(port, 1));
    all_events();
    strcpy(VEC, "");
    for (i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        rec("BrokerCommand", brokercommand(bname, cmds[i]));
        rec("command message", waitmsgs(port, 1));
        all_events();
    }
    /* nothing queued: msg = 0, the ids stay */
    hk_clear();
    REC0(E_EVENT);
    replied_before("CommodityEvent (nothing queued) replied the message taken before");
    held = NULL;
    look();

    /* hotkeys */
    hotkey("SetHotKey 0", 0, timer);
    rec("timer message", waitmsgs(port, 1));
    hotkey("SetHotKey 3", 3, timer);
    drain(port);
    rec("two timer messages", waitmsgs(port, 2));
    event_round();                     /* id 0 */
    event_round();                     /* id 3 */
    hotkey("SetHotKey 0 again", 0, timer);
    hotkey("SetHotKey 5 nonsense", 5, nonsense);
    drain(port);
    rec("two timer messages again", waitmsgs(port, 2));
    event_round();                     /* id 3: slot 0 is behind it now */
    event_round();                     /* id 0 */

    /* WaitCommodity with messages queued */
    drain(port);
    if (waitmsgs(port, 2)) {
        before = head();
        hk_clear();
        REC0(E_WAIT);
        replied_before("WaitCommodity replied the message taken before");
        rec("WaitCommodity took the head", !queued(port, before));
        held = !queued(port, before) ? before : NULL;
        look();
    } else {
        rec("WaitCommodity: no messages", 0);
    }

    /* SetCxStatus */
    rec("SetCxStatus 0", cx(E_STATUS, 1, 0, 0, 0));
    Delay(5);
    drain(port);
    Delay(25);
    rec("off: no messages", msgcount(port));
    rec("SetCxStatus 5", cx(E_STATUS, 1, 5, 0, 0));
    Delay(10);
    rec("still off", msgcount(port));
    rec("SetCxStatus -1", cx(E_STATUS, 1, (ULONG)-1, 0, 0));
    rec("on: messages", waitmsgs(port, 1));
    rec("SetCxStatus 0 again", cx(E_STATUS, 1, 0, 0, 0));
    Delay(5);
    drain(port);

    /* the sender of a hotkey cannot be made (CreateCxObj(CX_SEND) fails;
       the broker stays off: SetHotKey activates it only on success) */
    fail_send = 1;
    hotkey("SetHotKey 6 (no sender)", 6, timer);
    fail_send = 0;

    /* a message of the test queued at finit (the broker is off, nothing
       else comes): it comes back to the test's reply port if finit
       replies it */
    rp = CreateMsgPort();
    if (rp) {
        memset(&own, 0, sizeof own);
        own.mn_ReplyPort = rp;
        own.mn_Length = sizeof own;
        PutMsg(port, &own);
    }
out:
    hk_clear();
    CX0(E_FINIT);
    rec("finit: DeleteCxObj", hk_count[HK_DELETE]);
    rec("finit: DeleteCxObjAll", hk_count[HK_DELETEALL]);
    rec("finit: CloseLibrary", hk_count[HK_CLOSE]);
    rec("finit: closed before the delete", hk_closed);
    rec("finit replied the message taken last", was_replied(held));
    if (rp) {
        rec("finit replied a queued message", GetMsg(rp) == &own);
        DeleteMsgPort(rp);
    }
    rec("broker gone", brokercommand(bname, CXCMD_ENABLE) != 0);
}

/* ------------------------------------------------------------ main */

static const char *names[] = { LIBNAME, NULL };

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: ricommoditieslib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("MakeCommodity", d4-d7|a2|a3);
    KEEPS("SetHotKey", d2-d7|a2|a3);
    KEEPS("HotKeyHit", d1-d7|a1-a3);
    KEEPS("WaitCommodity", d1-d7|a2|a3);
    KEEPS("CommodityEvent", d1-d7|a2|a3);
    KEEPS("ExchangeMessage", d1-d7|a1-a3);
    KEEPS("ExchangeDisable", d1-d7|a0-a3|a6);
    KEEPS("ExchangeEnable", d1-d7|a0-a3|a6);
    KEEPS("ExchangeAppear", d1-d7|a0-a3|a6);
    KEEPS("ExchangeDisAppear", d1-d7|a0-a3|a6);
    KEEPS("ExchangeKill", d1-d7|a0-a3|a6);
    KEEPS("ExchangeListChange", d1-d7|a0-a3|a6);
    KEEPS("ExchangeUnique", d1-d7|a0-a3|a6);
    KEEPS("CxDisable", d1-d7|a0-a3|a6);
    KEEPS("CxEnable", d1-d7|a0-a3|a6);
    KEEPS("CxAppear", d1-d7|a0-a3|a6);
    KEEPS("CxDisAppear", d1-d7|a0-a3|a6);
    KEEPS("CxKill", d1-d7|a0-a3|a6);
    KEEPS("CxListChange", d1-d7|a0-a3|a6);
    KEEPS("CxUnique", d1-d7|a0-a3|a6);
    KEEPS("SetCxStatus", d2-d7|a2|a3);
    KEEPS("CommodityMsgPort", d1-d7|a0-a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("init", d2-d7|a2|a3);
    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    if (!CxBase) {
        check("commodities.library V37", FALSE);
        return 20;
    }
    me = FindTask(NULL);
    patch(1);
    scenario();
    patch(0);
    Delay(2);                           /* no task left in a stub */
    CloseLibrary(CxBase);

    OBS("results", nv);
    OBS("calls", nc);
    printf("ricommoditieslib: %d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
