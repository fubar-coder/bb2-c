/*
 * ricommoditieslib - RWE userlib 89: a commodity (commodities.library
 * V37) for a Blitz program: one broker with a message port, up to 16
 * hotkeys (a filter with a sender each), and the Exchange commands.
 *
 * C translation of rwelibsrc/Commoditieslib/CommoditiesLib2.bb; reference
 * BSS2 RICOMMODITIESLIB.OBJ (the source builds to it byte for byte with
 * blitz2 2.10, docs/rwelibs/README.md 3.1).  Notes:
 * docs/rwelibs/port/ricommoditieslib.md.
 *
 *   MakeCommodity name$,title$,description$   message port (once) and a
 *       broker (NBU_UNIQUE|NBU_NOTIFY, COF_SHOW_HIDE), activated; -1/0
 *   SetHotKey n,hotkey$   filter + sender (id n) attached to the broker
 *   CommodityEvent / WaitCommodity   fetch a message into `msg`
 *   HotKeyHit       id of a CXM_IEVENT message, else -1
 *   ExchangeMessage command of a CXM_COMMAND message, else 0
 *   ExchangeDisable ... ExchangeUnique   -1 if the last id is that
 *       CXCMD_ code; CxDisable ... CxUnique: the codes (inline)
 *   SetCxStatus on|off, CommodityMsgPort
 *
 * Every routine is C behind a thunk that keeps d1/a0/a1 (the original
 * keeps at least those in some of its paths; GCC keeps d2-d7/a2-a6).
 * Assembler: the inline constants (BLITZ_INLINE, copied by the compiler)
 * and the GetMsg of WaitCommodity, whose result the original takes from
 * the condition codes GetMsg leaves (C cannot see them, getmsg_cc).
 *
 * Kept from the original (BUG(orig)):
 *   - the help text of MakeCommodity lacks the closing parenthesis;
 *   - init does not check OpenLibrary: without commodities.library V37
 *     every command calls through a base of 0;
 *   - finit closes commodities.library first and then calls DeleteCxObj
 *     through the closed base; DeleteCxObj (not DeleteCxObjAll) leaves
 *     the filters and senders of the broker allocated; the message port
 *     is deleted with whatever messages are still queued;
 *   - WaitCommodity branches on the CCR after GetMsg (not on its result)
 *     and the logic is inverted: "ne" -> WaitPort (waits for and returns
 *     the NEXT message, which stays queued; the one GetMsg removed is
 *     lost), "eq" -> msg = GetMsg's result; -1 either way.  Since exec's
 *     GetMsg leaves "ne" in practice, WaitCommodity waits when the port
 *     is empty and skips a message when it is not;
 *   - messages are never replied (replymessage starts with RTS and is
 *     not called; kept as the dead code it is: not translated);
 *   - SetHotKey does not check the number: the slots are cxfilter[n] /
 *     cxsends[n] in the variables' original layout (st below), so n >= 16
 *     or < 0 hits the variables around them; a replaced filter/sender is
 *     removed but not deleted; the sender is only removed if the filter
 *     existed; objects made before a failure stay; AttachCxObj (void) is
 *     tested through the d0 it leaves; CxObjError is not looked at (the
 *     unreachable 'oops2 tail is not translated);
 *   - MakeCommodity makes a new broker on every call without deleting the
 *     previous one (on failure the old one stays in use; on success the
 *     old one stays active with the same port, which finit deletes);
 *   - SetCxStatus tests the message port, not the broker; any value other
 *     than -1 and 0 does nothing and returns 0;
 *   - the Exchange* functions look at the id of the last message whatever
 *     its type (a hotkey id 15 counts as ExchangeDisable).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <libraries/commodities.h>
#define __NOLIBBASE__
#define COMMODITIES_BASE_NAME st_cbase
#include <proto/commodities.h>
#undef __NOLIBBASE__

/* the CXCMD_ codes of the Exchange commands (#CXCMD_... in the source) */
#define CMD_DISABLE   15
#define CMD_ENABLE    17
#define CMD_APPEAR    19
#define CMD_DISAPPEAR 21
#define CMD_KILL      23
#define CMD_LIST_CHG  27
#define CMD_UNIQUE    25

/* entry points (thunks and inline code, defined below) */
void makecommodity(void), addhotkey(void), hotkeyhit(void), waitevent(void);
void event(void), exchangemessage(void), disabled(void), enable(void);
void appear(void), disappear(void), kill(void), list(void), unqiue(void);
void cxdisable(void), cxenable(void), cxappear(void), cxdisappear(void);
void cxkill(void), cxlist(void), cxunique(void), status(void), port(void);
void libinit(void), libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* a function without arguments, Amiga mode only (`_code+1`) */
#define RI_FUNC(code, name, help)                                       \
    BLITZ_AFUNCTION(BL_LONG); BLITZ_ARGS0(); BLITZ_LIBS0();             \
    BLITZ_SUBS(BLITZ_AMIGAONLY(code), 0, 0); BLITZ_NAME(name, help)

/* !libheader {#ricommoditieslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ricommoditieslib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(makecommodity), 0, 0);
/* BUG(orig): the help text lacks the closing parenthesis */
BLITZ_NAME("MakeCommodity", "(name$,title$,description$");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(addhotkey), 0, 0);
BLITZ_NAME("SetHotKey", "(hotkeynum,hotkey$)");

RI_FUNC(hotkeyhit, "HotKeyHit", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(waitevent), 0, 0);
BLITZ_NAME("WaitCommodity", "");

RI_FUNC(event, "CommodityEvent", "");
RI_FUNC(exchangemessage, "ExchangeMessage", "");
RI_FUNC(disabled, "ExchangeDisable", "");
RI_FUNC(enable, "ExchangeEnable", "");
RI_FUNC(appear, "ExchangeAppear", "");
RI_FUNC(disappear, "ExchangeDisAppear", "");
RI_FUNC(kill, "ExchangeKill", "");
RI_FUNC(list, "ExchangeListChange", "");
RI_FUNC(unqiue, "ExchangeUnique", "");
RI_FUNC(cxdisable, "CxDisable", "");
RI_FUNC(cxenable, "CxEnable", "");
RI_FUNC(cxappear, "CxAppear", "");
RI_FUNC(cxdisappear, "CxDisAppear", "");
RI_FUNC(cxkill, "CxKill", "");
RI_FUNC(cxlist, "CxListChange", "");
RI_FUNC(cxunique, "CxUnique", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(status), 0, 0);
BLITZ_NAME("SetCxStatus", "on|off");

/* the only entry without +1: callable in Blitz mode too */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(port, 0, 0);
BLITZ_NAME("CommodityMsgPort", "  -- returns the message port address");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

/* the variables in the order of the original's data block (newbroker,
   inputevent, msgport ... currentfilter), packed: SetHotKey indexes
   cxfilter/cxsends without a range check, so an out-of-range number hits
   the same neighbours as in the original */
static struct BL_PACKED {
    struct BL_PACKED {
        UBYTE version, pad0;
        STRPTR name, title, descr;
        WORD unique, flags;
        BYTE pri, pad1;
        struct MsgPort *port;
        WORD reservedchannel;
    } newbroker;                        /* = struct NewBroker */
    UBYTE inputevent[18];               /* unused (struct InputEvent head) */
    struct MsgPort *msgport;
    CxObj *cobject;
    ULONG cxfilter[16];
    ULONG cxsends[16];
    LONG hotkey;
    ULONG cxmsgid;
    ULONG cxmsgtype;
    ULONG cxmsgevent;
    CxMsg *msg;
    STRPTR hotkeystring;
    CxObj *currentfilter;
} st;
static struct Library *st_cbase;        /* cbase */
static const char cname[] = "commodities.library";

/* slot n of an array of the variables: address arithmetic as in the
   original (no range check, see above) */
#define SLOT(arr, n) ((ULONG *)((ULONG)(arr) + ((ULONG)(n) << 2)))

/* ------------------------------------------------------------- routines */

/* init: OpenLibrary("commodities.library",37).  BUG(orig): not checked. */
static void BLITZ_ROUTINE libinit_c(void)
{
    st_cbase = OpenLibrary((CONST_STRPTR)cname, 37);
}
BLITZ_THUNK(libinit, libinit_c, "d1/a0-a1");

/* delmsgport */
static void delmsgport(struct MsgPort *p)
{
    if (p)
        DeleteMsgPort(p);
}

/* finit.  BUG(orig): CloseLibrary first, then DeleteCxObj through the
   closed base; only the broker is deleted. */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!st_cbase)
        return;
    CloseLibrary(st_cbase);
    if (!st.msgport)
        return;
    DeleteCxObj(st.cobject);
    st.cobject = 0;
    delmsgport(st.msgport);
    st.msgport = 0;
}
BLITZ_THUNK(libfinit, libfinit_c, "d1/a0-a1");

/* GetMsg(port) and its condition codes: *ne = 0xff if exec returns with
   Z clear.  Assembler because the original branches on these flags (BNE
   right after JSR _LVOGetMsg), which C cannot see; exec keeps a6. */
static struct Message *getmsg_cc(struct MsgPort *port, UBYTE *ne)
{
    register struct MsgPort *a0 __asm("a0") = port;
    register struct ExecBase *a6 __asm("a6") = SysBase;
    register struct Message *d0 __asm("d0");
    UBYTE cc;

    __asm volatile("jsr -372(a6)\n\tsne %2"
                   : "=d"(d0), "+a"(a0), "=dm"(cc)
                   : "a"(a6)
                   : "d1", "a1", "cc", "memory");
    *ne = cc;
    return d0;
}

/* WaitCommodity.  BUG(orig): see the head of the file - the CCR of
   GetMsg decides, and inverted: "ne" waits for the next message. */
static LONG BLITZ_ROUTINE waitevent_c(void)
{
    struct Message *m;
    UBYTE ne;

    if (!st.cobject)
        return 0;
    m = getmsg_cc(st.msgport, &ne);
    if (ne)
        m = WaitPort(st.msgport);
    st.msg = (CxMsg *)m;
    return -1;
}
BLITZ_THUNK(waitevent, waitevent_c, "d1/a0-a1");

/* CommodityEvent: GetMsg -> msg, -1 if there was one */
static LONG BLITZ_ROUTINE event_c(void)
{
    if (!st.cobject)
        return 0;
    st.msg = (CxMsg *)GetMsg(st.msgport);
    return st.msg ? -1 : 0;
}
BLITZ_THUNK(event, event_c, "d1/a0-a1");

/* getmessage: id, type and data of msg */
static void getmessage(void)
{
    st.cxmsgid = CxMsgID(st.msg);
    st.cxmsgtype = CxMsgType(st.msg);
    st.cxmsgevent = (ULONG)CxMsgData(st.msg);
}

/* HotKeyHit: the id of a CXM_IEVENT message, else -1 */
static LONG BLITZ_ROUTINE hotkeyhit_c(void)
{
    if (!st.cobject || !st.msg)
        return -1;
    getmessage();
    if (st.cxmsgtype != CXM_IEVENT)
        return -1;
    return st.cxmsgid;                  /* BUG(orig): no ReplyMsg */
}
BLITZ_THUNK(hotkeyhit, hotkeyhit_c, "d1/a0-a1");

/* ExchangeMessage: the command of a CXM_COMMAND message, else 0 */
static LONG BLITZ_ROUTINE exchangemessage_c(void)
{
    if (!st.cobject || !st.msg)
        return 0;
    getmessage();
    if (st.cxmsgtype != CXM_COMMAND)
        return 0;
    return st.cxmsgid;
}
BLITZ_THUNK(exchangemessage, exchangemessage_c, "d1/a0-a1");

/* the inline constants: MOVE.l #CXCMD_...,d0 */
BLITZ_INLINE(cxdisable, "move.l #15,d0");
BLITZ_INLINE(cxenable, "move.l #17,d0");
BLITZ_INLINE(cxappear, "move.l #19,d0");
BLITZ_INLINE(cxdisappear, "move.l #21,d0");
BLITZ_INLINE(cxkill, "move.l #23,d0");
BLITZ_INLINE(cxlist, "move.l #27,d0");
BLITZ_INLINE(cxunique, "move.l #25,d0");

/* Exchange...: -1 if the id of the last message is the command */
static LONG is_cmd(ULONG cmd)
{
    if (!st.cxmsgid || st.cxmsgid != cmd)
        return 0;
    return -1;
}
static LONG BLITZ_ROUTINE disabled_c(void) { return is_cmd(CMD_DISABLE); }
static LONG BLITZ_ROUTINE enable_c(void) { return is_cmd(CMD_ENABLE); }
static LONG BLITZ_ROUTINE appear_c(void) { return is_cmd(CMD_APPEAR); }
static LONG BLITZ_ROUTINE disappear_c(void) { return is_cmd(CMD_DISAPPEAR); }
static LONG BLITZ_ROUTINE kill_c(void) { return is_cmd(CMD_KILL); }
static LONG BLITZ_ROUTINE list_c(void) { return is_cmd(CMD_LIST_CHG); }
static LONG BLITZ_ROUTINE unqiue_c(void) { return is_cmd(CMD_UNIQUE); }
BLITZ_THUNK(disabled, disabled_c, "d1/a0-a1");
BLITZ_THUNK(enable, enable_c, "d1/a0-a1");
BLITZ_THUNK(appear, appear_c, "d1/a0-a1");
BLITZ_THUNK(disappear, disappear_c, "d1/a0-a1");
BLITZ_THUNK(kill, kill_c, "d1/a0-a1");
BLITZ_THUNK(list, list_c, "d1/a0-a1");
BLITZ_THUNK(unqiue, unqiue_c, "d1/a0-a1");

/* AttachCxObj is void; the original tests the d0 it leaves, so it is
   called through the jump table with d0 as the result */
static ULONG attach(CxObj *head, CxObj *co)
{
    struct bl_regs r = { .a0 = (ULONG)head, .a1 = (ULONG)co };
    return BLITZ_BASEJSR(st_cbase, -84, r);
}

/* SetHotKey n,hotkey$.  BUG(orig): see the head of the file. */
static LONG BLITZ_ROUTINE addhotkey_c(LONG n BL_REG(d0), STRPTR desc BL_REG(d1))
{
    ULONG *slot, o;
    CxObj *obj;

    if (!st.cobject)
        return 0;
    st.hotkey = n;
    st.hotkeystring = desc;
    slot = SLOT(st.cxfilter, st.hotkey);
    if (*slot) {
        o = *slot;
        *slot = 0;
        RemoveCxObj((CxObj *)o);
        slot = SLOT(st.cxsends, st.hotkey);
        if (*slot) {
            o = *slot;
            *slot = 0;
            RemoveCxObj((CxObj *)o);
        }
    }
    obj = CreateCxObj(CX_FILTER, (LONG)st.hotkeystring, 0);
    if (!obj)
        return 0;
    st.currentfilter = obj;
    *SLOT(st.cxfilter, st.hotkey) = (ULONG)obj;
    if (!attach(st.cobject, st.currentfilter))
        return 0;
    obj = CreateCxObj(CX_SEND, (LONG)st.msgport, st.hotkey);
    if (!obj)
        return 0;
    *SLOT(st.cxsends, st.hotkey) = (ULONG)obj;
    if (!attach(st.currentfilter, obj))
        return 0;
    ActivateCxObj(st.cobject, 1);
    return -1;
}
BLITZ_THUNK(addhotkey, addhotkey_c, "d1/a0-a1");

/* CommodityMsgPort */
static ULONG BLITZ_ROUTINE port_c(void)
{
    return (ULONG)st.msgport;
}
BLITZ_THUNK(port, port_c, "d1/a0-a1");

/* SetCxStatus on|off: -1 activates, 0 deactivates the broker.
   BUG(orig): tests the message port, not the broker. */
static LONG BLITZ_ROUTINE status_c(LONG on BL_REG(d0))
{
    if (!st.msgport)
        return 0;
    if (on == -1)
        on = 1;
    else if (on != 0)
        return 0;
    ActivateCxObj(st.cobject, on);
    return -1;
}
BLITZ_THUNK(status, status_c, "d1/a0-a1");

/* MakeCommodity name$,title$,description$.  BUG(orig): a new broker on
   every call, the previous one is not deleted. */
static LONG BLITZ_ROUTINE makecommodity_c(STRPTR name BL_REG(d0), STRPTR title BL_REG(d1),
                                          STRPTR descr BL_REG(d2))
{
    CxObj *co;

    if (!st.msgport) {
        struct MsgPort *p = CreateMsgPort();    /* makemsgport */
        if (!p)
            return 0;
        st.msgport = p;
    }
    st.newbroker.name = name;
    st.newbroker.title = title;
    st.newbroker.descr = descr;
    st.newbroker.unique = NBU_UNIQUE | NBU_NOTIFY;
    st.newbroker.version = NB_VERSION;
    st.newbroker.flags = COF_SHOW_HIDE;
    st.newbroker.pri = 0;
    st.newbroker.port = st.msgport;
    co = CxBroker((struct NewBroker *)&st.newbroker, NULL);
    if (!co)
        return 0;
    st.cobject = co;
    ActivateCxObj(st.cobject, 1);
    return -1;
}
BLITZ_THUNK(makecommodity, makecommodity_c, "d1/a0-a1");
