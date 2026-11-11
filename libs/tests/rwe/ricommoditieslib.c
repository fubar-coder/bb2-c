/*
 * rwe ricommoditieslib - the C build of RWE userlib 89 against the CD
 * binary (BSS2 RICOMMODITIESLIB.OBJ).
 *
 *   ricommoditieslib DIR      (DIR: ricommoditieslib.orig / .obj)
 *
 * The same scenario runs on the original, then on the C build (each with
 * init ... finit, one after the other: the broker name is unique); every
 * result is recorded and the two records are compared, and every call
 * must keep the registers the original keeps.  Recorded are only values
 * that do not depend on addresses (results, "port != 0", message counts).
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

#define MAXV 400
static ULONG val[2][MAXV];
static const char *vname[2][MAXV];
static int nv[2];

#define MAXC 300
static struct call calls[2][MAXC];
static const char *cname[2][MAXC];
static int nc[2];

static void rec(int s, const char *what, ULONG v)
{
    if (nv[s] < MAXV) {
        val[s][nv[s]] = v;
        vname[s][nv[s]] = what;
    }
    nv[s]++;
}

/* call entry `e` of set s; d0-d2 = in (n of them), the rest markers */
static ULONG cx(int s, int e, int n, ULONG d0, ULONG d1, ULONG d2)
{
    struct lib *l = getlib(&sets[s], LIBNAME);
    UBYTE *fn = (UBYTE *)((ULONG)entry_form(l, e, 0) & ~1UL);
    struct regs r;

    markregs(&r);
    if (n > 0) r.d[0] = d0;
    if (n > 1) r.d[1] = d1;
    if (n > 2) r.d[2] = d2;
    if (nc[s] < MAXC) {
        calls[s][nc[s]].in = r;
        cname[s][nc[s]] = ename[e];
    }
    callregs(fn, &r, 0, 0);
    if (nc[s] < MAXC)
        calls[s][nc[s]].out = r;
    nc[s]++;
    return r.d[0];
}
#define CX0(s, e) cx(s, e, 0, 0, 0, 0)
#define REC0(s, e) rec(s, ename[e], CX0(s, e))

/* an inline entry ($A000,len): the code copied with an RTS, as the
   compiler copies it into the program */
static UWORD tramp[2][16];
static void inline_call(int s, int e)
{
    struct lib *l = getlib(&sets[s], LIBNAME);
    UBYTE *code = (UBYTE *)((ULONG)entry_form(l, e, 0) & ~1UL);
    UWORD len = W(code + 2);
    struct regs r;

    rec(s, "inline $A000", W(code) == 0xa000 && len > 0 && len < 24);
    if (W(code) != 0xa000 || len == 0 || len >= 24)
        return;
    memcpy(tramp[s], code + 4, len);
    tramp[s][len / 2] = 0x4e75;
    CacheClearU();
    markregs(&r);
    if (nc[s] < MAXC) {
        calls[s][nc[s]].in = r;
        cname[s][nc[s]] = ename[e];
    }
    callregs(tramp[s], &r, 0, 0);
    if (nc[s] < MAXC)
        calls[s][nc[s]].out = r;
    nc[s]++;
    rec(s, ename[e], r.d[0]);
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
static void look(int s)
{
    int e;

    REC0(s, E_HIT);
    REC0(s, E_XMSG);
    for (e = E_XDISABLE; e <= E_XUNIQUE; e++)
        REC0(s, e);
}

/* CommodityEvent and a look at the message */
static void event_round(int s)
{
    struct Message *before = head();

    REC0(s, E_EVENT);
    if (before && !queued(port, before))
        removed = before;
    rec(s, "event took the head", removed == before);
    look(s);
    settle();
}

/* every message that is queued, one by one */
static void all_events(int s)
{
    int i;

    rec(s, "queued", msgcount(port));
    for (i = 0; i < 4 && msgcount(port); i++)
        event_round(s);
}

/* ------------------------------------------------------------ scenario */

/* shorter than CBD_NAMELEN (24): commodities keeps 23 characters and
   compares those with the full name (a longer name is never "unique") */
static const char bname[] = "RWE commodity test";
static const char btitle[] = "ricommoditieslib";
static const char bdescr[] = "libs_tests: C build against the original";
static const char timer[] = "timer";
static const char nonsense[] = "rawkey nonsense_qualifier q";

static void scenario(int s)
{
    static const LONG cmds[] = { CXCMD_DISABLE, CXCMD_ENABLE, CXCMD_APPEAR,
                                 CXCMD_DISAPPEAR, CXCMD_KILL, CXCMD_LIST_CHG };
    struct Message *before;
    unsigned i;
    int e;

    CX0(s, E_INIT);

    /* no broker yet */
    port = NULL;
    REC0(s, E_HIT);
    REC0(s, E_WAIT);
    REC0(s, E_EVENT);
    REC0(s, E_XMSG);
    for (e = E_XDISABLE; e <= E_XUNIQUE; e++)
        REC0(s, e);
    rec(s, "SetHotKey (no broker)", cx(s, E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec(s, "SetCxStatus -1 (no port)", cx(s, E_STATUS, 1, (ULONG)-1, 0, 0));
    rec(s, "SetCxStatus 0 (no port)", cx(s, E_STATUS, 1, 0, 0, 0));
    REC0(s, E_PORT);
    for (e = E_CXDISABLE; e <= E_CXUNIQUE; e++)
        inline_call(s, e);

    /* the broker */
    rec(s, "MakeCommodity", cx(s, E_MAKE, 3, (ULONG)bname, (ULONG)btitle, (ULONG)bdescr));
    port = (struct MsgPort *)CX0(s, E_PORT);
    rec(s, "CommodityMsgPort != 0", port != NULL);
    if (!port)
        goto out;
    rec(s, "broker there", brokercommand(bname, CXCMD_ENABLE) == 0);
    waitmsgs(port, 1);
    all_events(s);
    /* the same name again: fails, the first broker gets CXCMD_UNIQUE */
    rec(s, "MakeCommodity again", cx(s, E_MAKE, 3, (ULONG)bname, (ULONG)"t2", (ULONG)"d2"));
    rec(s, "unique message", waitmsgs(port, 1));
    all_events(s);
    for (i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        rec(s, "BrokerCommand", brokercommand(bname, cmds[i]));
        rec(s, "command message", waitmsgs(port, 1));
        all_events(s);
    }
    /* nothing queued: msg = 0, the ids stay */
    REC0(s, E_EVENT);
    look(s);

    /* hotkeys */
    rec(s, "SetHotKey 0", cx(s, E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec(s, "timer message", waitmsgs(port, 1));
    rec(s, "SetHotKey 3", cx(s, E_HOTKEY, 2, 3, (ULONG)timer, 0));
    drain(port);
    rec(s, "two timer messages", waitmsgs(port, 2));
    event_round(s);                     /* id 0 */
    event_round(s);                     /* id 3 */
    rec(s, "SetHotKey 0 again", cx(s, E_HOTKEY, 2, 0, (ULONG)timer, 0));
    rec(s, "SetHotKey 5 nonsense", cx(s, E_HOTKEY, 2, 5, (ULONG)nonsense, 0));
    drain(port);
    rec(s, "two timer messages again", waitmsgs(port, 2));
    event_round(s);                     /* id 3: slot 0 is behind it now */
    event_round(s);                     /* id 0 */

    /* WaitCommodity with messages queued */
    drain(port);
    if (waitmsgs(port, 2)) {
        before = head();
        REC0(s, E_WAIT);
        rec(s, "WaitCommodity took the head", !queued(port, before));
        if (!queued(port, before))
            removed = before;
        look(s);
        settle();
    } else {
        rec(s, "WaitCommodity: no messages", 0);
    }

    /* SetCxStatus */
    rec(s, "SetCxStatus 0", cx(s, E_STATUS, 1, 0, 0, 0));
    Delay(5);
    drain(port);
    Delay(25);
    rec(s, "off: no messages", msgcount(port));
    rec(s, "SetCxStatus 5", cx(s, E_STATUS, 1, 5, 0, 0));
    Delay(10);
    rec(s, "still off", msgcount(port));
    rec(s, "SetCxStatus -1", cx(s, E_STATUS, 1, (ULONG)-1, 0, 0));
    rec(s, "on: messages", waitmsgs(port, 1));
    rec(s, "SetCxStatus 0 again", cx(s, E_STATUS, 1, 0, 0, 0));
    Delay(5);
    drain(port);
out:
    CX0(s, E_FINIT);
    rec(s, "broker gone", brokercommand(bname, CXCMD_ENABLE) != 0);
}

/* ------------------------------------------------------------ main */

static const char *names[] = { LIBNAME, NULL };

int main(int argc, char **argv)
{
    char buf[200], cbuf[80];
    int s, i, n;

    if (argc < 2) {
        printf("usage: ricommoditieslib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    if (!CxBase) {
        check("commodities.library V37", FALSE);
        return 20;
    }
    for (s = 0; s < 2; s++)
        scenario(s);
    CloseLibrary(CxBase);

    check("same number of results", tape_same("results", nv[0], nv[1]) && nv[1] <= MAXV);
    n = nv[0] < nv[1] ? nv[0] : nv[1];
    for (i = 0; i < n && i < MAXV; i++) {
        snprintf(buf, sizeof buf, "#%d %s: orig $%08lX, C $%08lX", i, vname[0][i],
                 val[0][i], val[1][i]);
        check(buf, tape_same(buf, val[0][i], val[1][i])
                   & tape_strsame(buf, vname[0][i], vname[1][i]));
    }
    check("same number of calls", tape_same("calls", nc[0], nc[1]) && nc[1] <= MAXC);
    n = nc[0] < nc[1] ? nc[0] : nc[1];
    for (i = 0; i < n && i < MAXC; i++) {
        snprintf(cbuf, sizeof cbuf, "call #%d %s", i, cname[0][i]);
        cmp_preserve(cbuf, &calls[0][i], &calls[1][i]);
    }
    printf("ricommoditieslib: %d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
