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
 * and a second one with the same name (fails, the first broker gets
 * CXCMD_UNIQUE: NBU_NOTIFY); the six other Exchange commands through
 * commodities' BrokerCommand, each fetched with CommodityEvent and looked
 * at with HotKeyHit, ExchangeMessage and all Exchange...; hotkeys on the
 * input event class "timer" (input.device sends timer events all the
 * time, so messages come without input): two slots, one replaced, one
 * with a description ParseIX does not understand; WaitCommodity with two
 * messages queued; SetCxStatus off/on/other.  The messages the library
 * takes from its port are replied here (the library never does), and the
 * port is drained before finit.
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

/* the call of fn as routine `name` with the registers r: its kept case */
static void call_kept(const char *name, APTR fn, struct regs *r)
{
    struct regs in = *r;

    callregs(fn, r, 0, 0);
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
static struct Message *removed;   /* taken from the port by the library */

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

/* reply what the library took from the port (after it is done with it) */
static void settle(void)
{
    if (removed)
        ReplyMsg(removed);
    removed = NULL;
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
    struct Message *before = head();

    REC0(E_EVENT);
    if (before && !queued(port, before))
        removed = before;
    rec("event took the head", removed == before);
    look();
    settle();
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
    struct Message *before;
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
    rec("unique message", waitmsgs(port, 1));
    all_events();
    for (i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        rec("BrokerCommand", brokercommand(bname, cmds[i]));
        rec("command message", waitmsgs(port, 1));
        all_events();
    }
    /* nothing queued: msg = 0, the ids stay */
    REC0(E_EVENT);
    look();

    /* hotkeys */
    rec("SetHotKey 0", cx(E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec("timer message", waitmsgs(port, 1));
    rec("SetHotKey 3", cx(E_HOTKEY, 2, 3, (ULONG)timer, 0));
    drain(port);
    rec("two timer messages", waitmsgs(port, 2));
    event_round();                     /* id 0 */
    event_round();                     /* id 3 */
    rec("SetHotKey 0 again", cx(E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec("SetHotKey 5 nonsense", cx(E_HOTKEY, 2, 5, (ULONG)nonsense, 0));
    drain(port);
    rec("two timer messages again", waitmsgs(port, 2));
    event_round();                     /* id 3: slot 0 is behind it now */
    event_round();                     /* id 0 */

    /* WaitCommodity with messages queued */
    drain(port);
    if (waitmsgs(port, 2)) {
        before = head();
        REC0(E_WAIT);
        rec("WaitCommodity took the head", !queued(port, before));
        if (!queued(port, before))
            removed = before;
        look();
        settle();
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
out:
    CX0(E_FINIT);
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
    scenario();
    CloseLibrary(CxBase);

    OBS("results", nv);
    OBS("calls", nc);
    printf("ricommoditieslib: %d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
