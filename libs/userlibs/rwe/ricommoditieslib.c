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
 * Assembler: the inline constants (BLITZ_INLINE, copied by the compiler).
 *
 * Fixed: MakeCommodity returns 0 without commodities.library V37
 * (init's OpenLibrary failed), before it makes the port; the original
 * called CxBroker through a base of 0 (#559).  SetCxStatus tests the
 * broker; the original tested the message port, which a MakeCommodity
 * that fails at CxBroker leaves, and called ActivateCxObj(0) (#562).
 * SetCxStatus with a value other than -1 and 0 does nothing and returns
 * 0, as in the original.  The Exchange* functions count only a
 * CXM_COMMAND message; the original compared the id whatever the type, so
 * a hotkey with id 15 counted as ExchangeDisable (#555).  WaitCommodity
 * takes the first message with GetMsg and, if the port is empty, waits
 * for one (WaitPort) and takes it; the original branched on the
 * condition codes GetMsg leaves, inverted: with messages queued the first
 * was lost and the second returned but left queued (a following
 * CommodityEvent returned it again) (#566).  Messages are replied:
 * CommodityEvent and WaitCommodity reply the message fetched before, finit
 * the last one and those still queued; the original never replied one
 * (its replymessage starts with RTS and is never called: not translated),
 * so commodities could never free them (#561).  finit closes
 * commodities.library last; the original closed it first and then called
 * DeleteCxObj through the closed base (#556).  finit deletes the broker
 * with DeleteCxObjAll, its hotkey filters and senders with it, and
 * clears the slots; the original's DeleteCxObj left them allocated (#557).
 * SetHotKey's number indexes cxfilter[n] / cxsends[n] (in the variables'
 * original layout, st below) without a check, so n >= 16 or < 0 hits the
 * variables around them; the original has no error check area, here the
 * library has one: with runtime error checking a number outside 0..15 is
 * the runtime error "Hotkey number must be 0 to 15" (hotkeynumchk);
 * release code unchanged (an invalid argument) (#564).  SetHotKey
 * deletes a replaced hotkey's filter with its sender (DeleteCxObjAll),
 * and on a failure the objects it made, and clears the slots; the
 * original only removed the replaced objects (RemoveCxObj) and left what
 * it had made before a failure (#563).
 *
 * Kept from the original (BUG(orig)):
 *   - the help text of MakeCommodity lacks the closing parenthesis
 *     (#558);
 *   - SetHotKey: AttachCxObj (void) is tested through the d0 it leaves;
 *     CxObjError is not looked at (the unreachable 'oops2 tail is not
 *     translated) (#565);
 *   - MakeCommodity makes a new broker on every call without deleting the
 *     previous one (on failure the old one stays in use; on success the
 *     old one stays active with the same port, which finit deletes)
 *     (#560).
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
void ricommoditieslib_libinit(void), ricommoditieslib_libfinit(void);
void hotkeynumchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* a function without arguments, Amiga mode only (`_code+1`) */
#define RI_FUNC(code, name, help)                                       \
    BLITZ_AFUNCTION(BL_LONG); BLITZ_ARGS0(); BLITZ_LIBS0();             \
    BLITZ_SUBS(BLITZ_AMIGAONLY(code), 0, 0); BLITZ_NAME(name, help)

/* !libheader {#ricommoditieslib,init,0,finit,0}; error field: the error
   check area of hotkeynumchk (fixed, #564) */
BLITZ_LIBHEADER(LIB_ricommoditieslib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(makecommodity), 0, 0);
/* BUG(orig): the help text lacks the closing parenthesis (#558) */
BLITZ_NAME("MakeCommodity", "(name$,title$,description$");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(addhotkey), hotkeynumchk, 0);    /* check: fixed #564 */
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

BLITZ_NULLSUB_AT(finit, ricommoditieslib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ricommoditieslib_libinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

/* the variables in the order of the original's data block (newbroker,
   inputevent, msgport ... currentfilter), packed: SetHotKey indexes
   cxfilter/cxsends without a range check, so an out-of-range number hits
   the same neighbours as in the original */
static struct BL_PACKED {
    struct NewBroker newbroker;         /* 26 bytes */
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
static_assert(sizeof(struct NewBroker) == 26);
static struct Library *st_cbase;        /* cbase */
static const char cname[] = "commodities.library";

/* slot n of an array of the variables: address arithmetic as in the
   original (no range check, see above) */
#define SLOT(arr, n) ((ULONG *)((ULONG)(arr) + ((ULONG)(n) << 2)))

/* ------------------------------------------------------------- routines */

/**
 * @brief init: OpenLibrary("commodities.library",37).
 *
 * Without the library the base stays 0; MakeCommodity checks it (Fixed
 * (#559)), every other command needs MakeCommodity's broker or port.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    st_cbase = OpenLibrary((CONST_STRPTR)cname, 37);
}
BLITZ_THUNK(ricommoditieslib_libinit, libinit_c, "d1/a0-a1");

/* delmsgport */
static void delmsgport(struct MsgPort *p)
{
    if (p)
        DeleteMsgPort(p);
}

/* replymsg: replies the message the program fetched last (Fixed (#561):
   commodities frees a message when it comes back) */
static void replymsg(void)
{
    if (st.msg) {
        ReplyMsg((struct Message *)st.msg);
        st.msg = 0;
    }
}

/**
 * @brief finit: deletes the broker, replies the messages, deletes the
 * message port and closes commodities.library.
 *
 * Fixed (#561): the message fetched last and those still queued are
 * replied (after the broker is deleted, so no more come) before the port
 * is deleted; the original deleted the port with them.  Fixed (#556):
 * commodities.library is closed last; the original closed it first and
 * then called DeleteCxObj through the closed base.  Fixed (#557): the
 * broker is deleted with DeleteCxObjAll, its hotkey filters and senders
 * with it, and the slots are cleared; the original's DeleteCxObj left
 * them allocated.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    struct Message *m;
    int i;

    if (!st_cbase)
        return;
    if (st.msgport) {
        DeleteCxObjAll(st.cobject);
        st.cobject = 0;
        for (i = 0; i < 16; i++)
            st.cxfilter[i] = st.cxsends[i] = 0;
        replymsg();
        while ((m = GetMsg(st.msgport)))
            ReplyMsg(m);
        delmsgport(st.msgport);
        st.msgport = 0;
    }
    CloseLibrary(st_cbase);
}
BLITZ_THUNK(ricommoditieslib_libfinit, libfinit_c, "d1/a0-a1");

/**
 * @brief WaitCommodity: fetches the next message into msg, waiting for
 * one.
 *
 * Amiga mode only.  Fixed (#566): GetMsg, and on an empty port WaitPort
 * and GetMsg again; the original branched on the condition codes GetMsg
 * leaves, inverted ("ne", what exec leaves: WaitPort, msg = the head,
 * left queued; a message GetMsg had removed was lost).  Fixed (#561):
 * the message fetched before is replied first.
 * @return d0: BB_TRUE, BB_FALSE without a broker
 */
static BB_BOOL BLITZ_ROUTINE waitevent_c(void)
{
    struct Message *m;

    if (!st.cobject)
        return BB_FALSE;
    replymsg();
    while (!(m = GetMsg(st.msgport)))
        WaitPort(st.msgport);
    st.msg = (CxMsg *)m;
    return BB_TRUE;
}
BLITZ_THUNK(waitevent, waitevent_c, "d1/a0-a1");

/**
 * @brief CommodityEvent: fetches the next message (GetMsg) into msg.
 *
 * Amiga mode only.  Fixed (#561): the message fetched before is replied
 * first (HotKeyHit and the others read the current one until the next
 * fetch); the original never replied a message.
 * @return d0: BB_TRUE if there was one, BB_FALSE if not or without a
 *         broker
 */
static BB_BOOL BLITZ_ROUTINE event_c(void)
{
    if (!st.cobject)
        return BB_FALSE;
    replymsg();
    st.msg = (CxMsg *)GetMsg(st.msgport);
    return st.msg ? BB_TRUE : BB_FALSE;
}
BLITZ_THUNK(event, event_c, "d1/a0-a1");

/* getmessage: id, type and data of msg */
static void getmessage(void)
{
    st.cxmsgid = CxMsgID(st.msg);
    st.cxmsgtype = CxMsgType(st.msg);
    st.cxmsgevent = (ULONG)CxMsgData(st.msg);
}

/**
 * @brief HotKeyHit: the hotkey id of the fetched message.
 *
 * Amiga mode only.  Reads id, type and data of msg (they stay for the
 * Exchange* functions); the message is replied by the next fetch or by
 * finit (#561).
 * @return d0: the id of a CXM_IEVENT message, else -1 (also without a
 *         broker or message)
 */
static LONG BLITZ_ROUTINE hotkeyhit_c(void)
{
    if (!st.cobject || !st.msg)
        return -1;
    getmessage();
    if (st.cxmsgtype != CXM_IEVENT)
        return -1;
    return st.cxmsgid;
}
BLITZ_THUNK(hotkeyhit, hotkeyhit_c, "d1/a0-a1");

/**
 * @brief ExchangeMessage: the Exchange command of the fetched message.
 *
 * Amiga mode only.  Reads id, type and data of msg as HotKeyHit.
 * @return d0: the CXCMD_ code of a CXM_COMMAND message, else 0 (also
 *         without a broker or message)
 */
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

/**
 * @brief CxDisable, CxEnable, CxAppear, CxDisAppear, CxKill,
 * CxListChange, CxUnique: the CXCMD_ codes.
 *
 * Inline constants (MOVE.l #CXCMD_...,d0, copied by the compiler).
 * Result: d0 = the code (15, 17, 19, 21, 23, 27, 25).
 */
BLITZ_INLINE(cxdisable, "move.l #15,d0");
BLITZ_INLINE(cxenable, "move.l #17,d0");
BLITZ_INLINE(cxappear, "move.l #19,d0");
BLITZ_INLINE(cxdisappear, "move.l #21,d0");
BLITZ_INLINE(cxkill, "move.l #23,d0");
BLITZ_INLINE(cxlist, "move.l #27,d0");
BLITZ_INLINE(cxunique, "move.l #25,d0");

/* Exchange...: -1 if the last message is the command */
static BB_BOOL is_cmd(ULONG cmd)
{
    if (st.cxmsgtype != CXM_COMMAND || st.cxmsgid != cmd)
        return BB_FALSE;
    return BB_TRUE;
}
/**
 * @brief ExchangeDisable, ExchangeEnable, ExchangeAppear,
 * ExchangeDisAppear, ExchangeKill, ExchangeListChange, ExchangeUnique:
 * whether the last message was that Exchange command.
 *
 * Amiga mode only.  Fixed (#555): only a CXM_COMMAND message counts; the
 * original compared the id whatever the type (a hotkey id 15 counted as
 * ExchangeDisable).
 * @return d0: BB_TRUE if the last message is a CXM_COMMAND with the
 *         CXCMD_ code, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE disabled_c(void) { return is_cmd(CMD_DISABLE); }
static BB_BOOL BLITZ_ROUTINE enable_c(void) { return is_cmd(CMD_ENABLE); }
static BB_BOOL BLITZ_ROUTINE appear_c(void) { return is_cmd(CMD_APPEAR); }
static BB_BOOL BLITZ_ROUTINE disappear_c(void) { return is_cmd(CMD_DISAPPEAR); }
static BB_BOOL BLITZ_ROUTINE kill_c(void) { return is_cmd(CMD_KILL); }
static BB_BOOL BLITZ_ROUTINE list_c(void) { return is_cmd(CMD_LIST_CHG); }
static BB_BOOL BLITZ_ROUTINE unqiue_c(void) { return is_cmd(CMD_UNIQUE); }
BLITZ_THUNK(disabled, disabled_c, "d1/a0-a1");
BLITZ_THUNK(enable, enable_c, "d1/a0-a1");
BLITZ_THUNK(appear, appear_c, "d1/a0-a1");
BLITZ_THUNK(disappear, disappear_c, "d1/a0-a1");
BLITZ_THUNK(kill, kill_c, "d1/a0-a1");
BLITZ_THUNK(list, list_c, "d1/a0-a1");
BLITZ_THUNK(unqiue, unqiue_c, "d1/a0-a1");

/* AttachCxObj is void; the original tests the d0 it leaves, so it is
   called through the jump table with d0 as the result */
static LONG attach(CxObj *head, CxObj *co)
{
    struct bl_regs r = { .a0 = (ULONG)head, .a1 = (ULONG)co };
    return BLITZ_BASEJSR(st_cbase, -84, r);
}

/* delhotkey: deletes the filter of slot n with the sender attached to it
   (DeleteCxObjAll detaches it from the broker first) and clears both
   slots (Fixed (#563)) */
static void delhotkey(LONG n)
{
    ULONG *f = SLOT(st.cxfilter, n);
    ULONG o = *f;

    *f = 0;
    *SLOT(st.cxsends, n) = 0;
    if (o)
        DeleteCxObjAll((CxObj *)o);
}

/**
 * @brief SetHotKey(hotkeynum,hotkey$): a hotkey filter with a sender (id
 * hotkeynum) attached to the broker.
 *
 * Amiga mode only; the broker is activated afterwards.  A number outside
 * 0..15 hits the variables around cxfilter/cxsends: an invalid argument,
 * refused by hotkeynumchk with runtime error checking (#564).
 * Fixed (#563): a replaced hotkey's filter is deleted with its sender
 * (DeleteCxObjAll, which also detaches it), and when a step fails the
 * new filter (with a sender attached to it) is deleted and the slots are
 * cleared; the original only removed the replaced objects (RemoveCxObj,
 * the sender only if the filter existed) and left what it had made
 * before a failure.  BUG(orig): AttachCxObj (void) is tested through the
 * d0 it leaves, CxObjError is not looked at (#565).
 * @param n     d0: the hotkey number
 * @param desc  d1: the hotkey description (CX_FILTER)
 * @return d0: BB_TRUE, BB_FALSE without a broker or if a step fails
 */
static BB_BOOL BLITZ_ROUTINE addhotkey_c(LONG n BL_REG(d0), STRPTR desc BL_REG(d1))
{
    CxObj *obj;

    if (!st.cobject)
        return BB_FALSE;
    st.hotkey = n;
    st.hotkeystring = desc;
    delhotkey(st.hotkey);
    obj = CreateCxObj(CX_FILTER, (LONG)st.hotkeystring, 0);
    if (!obj)
        return BB_FALSE;
    st.currentfilter = obj;
    *SLOT(st.cxfilter, st.hotkey) = (ULONG)obj;
    if (!attach(st.cobject, st.currentfilter)) {
        delhotkey(st.hotkey);
        return BB_FALSE;
    }
    obj = CreateCxObj(CX_SEND, (LONG)st.msgport, st.hotkey);
    if (!obj) {
        delhotkey(st.hotkey);
        return BB_FALSE;
    }
    *SLOT(st.cxsends, st.hotkey) = (ULONG)obj;
    if (!attach(st.currentfilter, obj)) {
        delhotkey(st.hotkey);           /* the sender is attached: deleted too */
        return BB_FALSE;
    }
    ActivateCxObj(st.cobject, 1);
    return BB_TRUE;
}
BLITZ_THUNK(addhotkey, addhotkey_c, "d1/a0-a1");

/**
 * @brief CommodityMsgPort: the message port address.
 *
 * The only entry callable in Blitz mode too.
 * @return d0: the port, 0 before MakeCommodity
 */
static ULONG BLITZ_ROUTINE port_c(void)
{
    return (ULONG)st.msgport;
}
BLITZ_THUNK(port, port_c, "d1/a0-a1");

/**
 * @brief SetCxStatus on|off: -1 activates, 0 deactivates the broker.
 *
 * Amiga mode only; any other value does nothing.  Fixed (#562): tests
 * the broker; the original tested the message port, which a failed
 * MakeCommodity leaves without a broker, and called ActivateCxObj(0).
 * @param on  d0: -1 or 0 (other values are tested, so a LONG)
 * @return d0: BB_TRUE, BB_FALSE without a broker or for another value (a
 *         statement: the value is not used)
 */
static BB_BOOL BLITZ_ROUTINE status_c(LONG on BL_REG(d0))
{
    if (!st.cobject)
        return BB_FALSE;
    if (on == -1)
        on = 1;
    else if (on != 0)
        return BB_FALSE;
    ActivateCxObj(st.cobject, on);
    return BB_TRUE;
}
BLITZ_THUNK(status, status_c, "d1/a0-a1");

/**
 * @brief MakeCommodity(name$,title$,description$): the commodity's broker
 * (NBU_UNIQUE|NBU_NOTIFY, COF_SHOW_HIDE), activated.
 *
 * Amiga mode only.  The message port is created on the first call.
 * Fixed (#559): without commodities.library (init's OpenLibrary failed)
 * it returns 0 before it makes the port; the original called CxBroker
 * through a base of 0.  BUG(orig): a new broker on every call, the previous one is not deleted
 * (#560); the help text lacks the closing parenthesis (#558).
 * @param name   d0: the name
 * @param title  d1: the title
 * @param descr  d2: the description
 * @return d0: BB_TRUE, BB_FALSE without the library or if the port or
 *         the broker fails
 */
static BB_BOOL BLITZ_ROUTINE makecommodity_c(STRPTR name BL_REG(d0), STRPTR title BL_REG(d1),
                                             STRPTR descr BL_REG(d2))
{
    CxObj *co;

    if (!st_cbase)
        return BB_FALSE;
    if (!st.msgport) {
        struct MsgPort *p = CreateMsgPort();    /* makemsgport */
        if (!p)
            return BB_FALSE;
        st.msgport = p;
    }
    st.newbroker.nb_Name = name;
    st.newbroker.nb_Title = title;
    st.newbroker.nb_Descr = descr;
    st.newbroker.nb_Unique = NBU_UNIQUE | NBU_NOTIFY;
    st.newbroker.nb_Version = NB_VERSION;
    st.newbroker.nb_Flags = COF_SHOW_HIDE;
    st.newbroker.nb_Pri = 0;
    st.newbroker.nb_Port = st.msgport;
    co = CxBroker(&st.newbroker, NULL);
    if (!co)
        return BB_FALSE;
    st.cobject = co;
    ActivateCxObj(st.cobject, 1);
    return BB_TRUE;
}
BLITZ_THUNK(makecommodity, makecommodity_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badhotkey, "Hotkey number must be 0 to 15");  /* fixed, #564 */

/**
 * @brief The error check of SetHotKey (debugger only).
 *
 * Fixed (#564), not in the original (ricommoditieslib had no error check
 * area): the number indexes the tables of 16 filters and senders without
 * a check, so a number outside 0..15 made SetHotKey remove and overwrite
 * the variables around them (with -1 the broker).  The release code is
 * unchanged (an invalid argument).
 *
 * Errors: "Hotkey number must be 0 to 15".
 * @param n  d0: the hotkey number
 */
static void BLITZ_ERRCHK hotkeynumchk_c(ULONG n BL_REG(d0))
{
    if (n >= 16)
        BLITZ_RUNERR(badhotkey);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(hotkeynumchk, hotkeynumchk_c, "d0-d1/a0-a1");
