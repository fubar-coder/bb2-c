/*
 * riarexxlib - RWE userlib 133: ARexx ports and messages ("RIARexxLib
 * 2", (C) 1997 Red When Excited, Steven Matty and Stephen McNamara).
 *   RXCreatePort(name$)  RXDeletePort  RXReadPort  RXWaitPort
 *   RXReplyMsg errcode[,result$]
 *   RXRunCommand [portname$,]command$       (no reply awaited)
 *   RXSendCommand([portname$,]command$)     (waits; the result string)
 *   RXSendersName  RXNumArgs  RXGetArg(n)  RXNextArg  RXReturnCode
 *   RXExecuteScript/RXRunScript scriptname$ ("REXX")
 *   RXDefaultPort portname$  RXTokenise on|off  RXMessageList
 *   RXQuoteChar char$
 *
 * C translation of rwelibsrc/rexxlib/NEWAREXXLIB.bb (BB22 format; the
 * source defines its number itself, #myconst = 133); reference: the
 * reference build of the source (REFERENCE refbuild) - the BSS2 binary
 * RIAREXXLIB.OBJ is the older generation newrexxlib7 (docs/rwelibs/
 * README.md 3.2).  Notes: docs/rwelibs/port/riarexxlib.md.
 *
 * init opens rexxsyslib.library V34.  RXCreatePort makes a public port
 * and one block of 256 bytes: the list of received messages (32 longs)
 * and the list of messages sent without waiting (32 longs).  Messages
 * are 128 byte RexxMsgs (AllocVec): ln_Name = our port's name, rm_Args =
 * the command, as one argument string or (RXTokenise on, the default)
 * split into words with RXQuoteChar quotes.  RXReadPort takes everything
 * from the port: replies to our messages are freed, messages go to the
 * list; the first of the list becomes the current message (RXGetArg,
 * RXReplyMsg ...).  Strings are copied with maxslib's AllocSt (ALibJsr
 * $CF01).  The commands have the error check portchk ("No message port
 * opened!").
 *
 * C behind thunks; the string results are written at a3 (the C build
 * moves the fixed register blitz_string itself), a2 stays unchanged
 * (DIFF: the original pops the string lengths it reads with -(a2), see
 * docs/libs/cmdlibs-port.md "General").  Assembler: exec calls whose
 * leftovers the original uses (the flags of AllocVec, d0 of AddPort and
 * PutMsg, a0 of FindPort).  The variables from rexxportname on are one
 * packed struct in the original's order: RXDefaultPort copies without a
 * limit into the 65 bytes of rexxportname and overwrites them.
 *
 * BUG(orig), kept:
 *   - init writes "REXX" and a 0 at +5 into the default port name: the
 *     byte at +4 of an earlier RXDefaultPort stays.
 *   - RXExecuteScript/RXRunScript set their message type, but
 *     _rxruncommand overwrites it: both send RXCOMM|RXFF_STRING (the
 *     script name as a string program), without a result.
 *   - _makemsg tests the flags AllocVec leaves (MOVE.l d0,a1 is a MOVEA):
 *     if they say 0, the message stays the one before (or 0).
 *   - RXRunCommand returns d0 of PutMsg (exec leaves something there).
 *   - Replies to messages sent without waiting are found in the sent
 *     list by _finditem, which always returns 0: they are freed with
 *     FreeVec only (their argument strings stay allocated) and stay in
 *     the sent list, which grows by one per message and overflows the
 *     block after 32.
 *   - The result string of a reply is copied (AllocSt, never freed); the
 *     reply's own string is never deleted; Result2 is taken as a string
 *     also when Result1 is not 0.
 *   - RXReturnCode is 0 while no message is current.
 *   - RXReplyMsg does not check that a result was asked for; the current
 *     message stays current after the reply.  If the port of the message's
 *     ln_Name no longer exists, it is not replied, but its Result2 (tested
 *     at the a0 FindPort leaves) is deleted.
 *   - RXCreatePort tests d0 of AddPort; on 0, or if the lists cannot be
 *     allocated, it deletes the port without RemPort (it stays in the
 *     public list).  RXDeletePort frees neither the lists nor the name.
 *   - The parser copies the command into 256 bytes and every word into
 *     256 bytes, both without a limit (in the original over its own
 *     code); a quote right after a closing quote is a quote character, any
 *     other character there is skipped; an empty word ("") ends the
 *     arguments; there is no limit of 16 arguments (rm_Args).
 *   - RXNextArg does not advance after an empty argument.
 *   - RXTokenise stores the whole register of its word argument.
 */
#include "blitzlib.h"
#include <exec/ports.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#define __NOLIBBASE__
#include <proto/exec.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void _rxcreateport(void), _rxdeleteport(void), readport(void), waitport(void);
void _replymsg(void), _replymsg2(void), _rxruncommand2(void), _rxruncommand(void);
void _rxsendersname(void), _rxsendcommand2(void), _rxsendcommand(void);
void _rxnumargs(void), _rxgetarg(void), _rxexecutescript(void), _rxrunscript(void);
void _rxdefportname(void), _rxnextarg(void), _rxreturncode(void), _rxtokenise(void);
void _rxmessagelist(void), _rxquotechar(void), _libinit(void), _libfinit(void);
void portchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define AO BLITZ_AMIGAONLY

/* !libheader {#myconst,init,0,finit,error} */
BLITZ_LIBHEADER(LIB_riarexxlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxcreateport), 0, 0);
BLITZ_NAME("RXCreatePort", "(name$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxdeleteport), portchk, 0);
BLITZ_NAME("RXDeletePort", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(readport), portchk, 0);
BLITZ_NAME("RXReadPort", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(waitport), portchk, 0);
BLITZ_NAME("RXWaitPort", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_replymsg), portchk, 0);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_replymsg2), portchk, 0);
BLITZ_NAME("RXReplyMsg", "errcode[,result$]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxruncommand2), portchk, 0);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxruncommand), portchk, 0);
BLITZ_NAME("RXRunCommand", "[portname$,]command$");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxsendersname), portchk, 0);
BLITZ_NAME("RXSendersName", "");

/* the slots of the commands of newrexxlib7 that RIARexxLib 2 dropped */
BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ACOMMAND(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxsendcommand2), portchk, 0);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxsendcommand), portchk, 0);
BLITZ_NAME("RXSendCommand", "[portname$,]command$");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxnumargs), portchk, 0);
BLITZ_NAME("RXNumArgs", "");

BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxgetarg), portchk, 0);
BLITZ_NAME("RXGetArg", "argnum");

BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxexecutescript), portchk, 0);
BLITZ_NAME("RXExecuteScript", "scriptname$");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxrunscript), portchk, 0);
BLITZ_NAME("RXRunScript", "scriptname$");

BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxdefportname), portchk, 0);
BLITZ_NAME("RXDefaultPort", "portname$");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxnextarg), portchk, 0);
BLITZ_NAME("RXNextArg", "argnum");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(AO(_rxreturncode), portchk, 0);
BLITZ_NAME("RXReturnCode", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_rxtokenise, 0, 0);
BLITZ_NAME("RXTokenise", "on|off");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rxmessagelist, 0, 0);
BLITZ_NAME("RXMessageList", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rxquotechar, 0, 0);
BLITZ_NAME("RXQuoteChar", "char$");

BLITZ_NULLSUB_AT(init, _libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);

BLITZ_LIBFIN();

/* -------------------------------------------------------------- errors */

/* error: portchk - TST.l msgport:BEQ msg0:RTS, msg0: the runtime error */
BLITZ_ERRCHK_MESSAGE(err0, "No message port opened!");

static struct MsgPort *msgport;

static void BLITZ_ERRCHK portchk_c(void)
{
    if (!msgport)
        BLITZ_RUNERR(err0);
}
BLITZ_ERRCHK_THUNK(portchk, portchk_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------------- data */

#define SysBase (*(struct ExecBase **)4)

static ULONG msgstring, msgtype;
static UBYTE *msgportname;
static UBYTE quote;
static ULONG tokenise;
static UWORD oktodelete;
static UBYTE *curadr;                   /* the parser (parseadr unused) */
static UBYTE tempstorage[256], stringbuffer[256];
static const char rexxname[] = "rexxsyslib.library";
static const char realrexxportname[] = "REXX";

/* rexxportname ... currentarg, as in the original */
static struct BL_PACKED {
    UBYTE rexxportname[66];             /* "REXX",0 + 60, Even */
    struct Library *rexxbase;
    struct MsgPort *destport;
    UBYTE *returnstring;
    ULONG returncode;
    ULONG *sentlist;                    /* sent, reply not awaited */
    ULONG *messagelist;                 /* received, not yet current */
    ULONG waitreply;
    UWORD replyreq;
    struct RexxMsg *thismsg;
    struct RexxMsg *currentmessage;
    UWORD currentarg;
} v = { .rexxportname = "REXX" };

_Static_assert(__builtin_offsetof(__typeof__(v), rexxbase) == 66, "layout");
_Static_assert(__builtin_offsetof(__typeof__(v), currentarg) == 0xeca - 0xe62, "layout");

/* rexxsyslib CreateArgString(a0 = string, d0 = length) / DeleteArgString */
static ULONG createargstring(const UBYTE *s, ULONG len)
{
    register ULONG d0 __asm("d0") = len;
    register const UBYTE *a0 __asm("a0") = s;
    register struct Library *a6 __asm("a6") = v.rexxbase;

    __asm volatile("jsr -126(a6)" : "+d"(d0), "+a"(a0), "+a"(a6)
                   : : "d1", "a1", "cc", "memory");
    return d0;
}

static void deleteargstring(ULONG s)
{
    register ULONG a0 __asm("a0") = s;
    register struct Library *a6 __asm("a6") = v.rexxbase;

    __asm volatile("jsr -132(a6)" : "+a"(a0), "+a"(a6)
                   : : "d0", "d1", "a1", "cc", "memory");
}

/* maxslib AllocSt: d0 = the string, d1 = its length -> a copy */
static ULONG allocst(ULONG s, ULONG len)
{
    struct bl_regs r = { .d0 = s, .d1 = len };
    return BLITZ_ALIBJSR(TOK_allocst, r);
}

/* exec calls with what they leave */
static APTR allocvec_z(ULONG size, ULONG flags, UBYTE *z)
{
    register ULONG d0 __asm("d0") = size;
    register ULONG d1 __asm("d1") = flags;
    register struct ExecBase *a6 __asm("a6") = SysBase;

    __asm volatile("jsr -684(a6)\n\tseq %2" : "+d"(d0), "+d"(d1), "=m"(*z), "+a"(a6)
                   : : "a0", "a1", "cc", "memory");
    return (APTR)d0;
}

static ULONG addport_d0(struct MsgPort *port)
{
    register ULONG d0 __asm("d0");
    register struct MsgPort *a1 __asm("a1") = port;
    register struct ExecBase *a6 __asm("a6") = SysBase;

    __asm volatile("jsr -354(a6)" : "=d"(d0), "+a"(a1), "+a"(a6)
                   : : "d1", "a0", "cc", "memory");
    return d0;
}

static ULONG putmsg_d0(struct MsgPort *port, struct Message *m)
{
    register ULONG d0 __asm("d0");
    register struct MsgPort *a0 __asm("a0") = port;
    register struct Message *a1 __asm("a1") = m;
    register struct ExecBase *a6 __asm("a6") = SysBase;

    __asm volatile("jsr -366(a6)" : "=d"(d0), "+a"(a0), "+a"(a1), "+a"(a6)
                   : : "d1", "cc", "memory");
    return d0;
}

static struct MsgPort *findport_a0(const UBYTE *name, UBYTE **a0out)
{
    register ULONG d0 __asm("d0");
    register UBYTE *a0 __asm("a0");
    register const UBYTE *a1 __asm("a1") = name;
    register struct ExecBase *a6 __asm("a6") = SysBase;

    __asm volatile("jsr -390(a6)" : "=d"(d0), "=a"(a0), "+a"(a1), "+a"(a6)
                   : : "d1", "cc", "memory");
    *a0out = a0;
    return (struct MsgPort *)d0;
}

/* the string s to a3 with its 0; a3 ends at the 0, d0 = the length */
static ULONG tostring(const UBYTE *s)
{
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;
    return n;
}

/* ------------------------------------------------------- init / finit */

static ULONG BLITZ_ROUTINE libinit_c(void)
{
    quote = 34;
    tokenise = (ULONG)-1;
    v.rexxportname[0] = 'R';            /* MOVE.l #"REXX" */
    v.rexxportname[1] = 'E';
    v.rexxportname[2] = 'X';
    v.rexxportname[3] = 'X';
    v.rexxportname[5] = 0;              /* BUG(orig): +4 stays */
    msgport = NULL;
    v.rexxbase = NULL;
    v.rexxbase = OpenLibrary((CONST_STRPTR)rexxname, 34);
    return 0;
}
BLITZ_THUNK(_libinit, libinit_c, "d1/a0-a1");

static void deleteport(void);

static void BLITZ_ROUTINE libfinit_c(void)
{
    tokenise = (ULONG)-1;
    if (msgport)
        deleteport();
}
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- small commands */

static void BLITZ_ROUTINE rxquotechar_c(const UBYTE *s BL_REG(d0))
{
    quote = *s;
}
BLITZ_THUNK(_rxquotechar, rxquotechar_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE rxmessagelist_c(void)
{
    return (ULONG)v.messagelist;
}
BLITZ_THUNK(_rxmessagelist, rxmessagelist_c, "d1/a0-a1");

/* RXTokenise: MOVE.l d0 - BUG(orig): the upper word of the word too */
static void BLITZ_ROUTINE rxtokenise_c(ULONG on BL_REG(d0))
{
    tokenise = on;
}
BLITZ_THUNK(_rxtokenise, rxtokenise_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE rxreturncode_c(void)
{
    if (!v.currentmessage)
        return 0;
    return v.returncode;
}
BLITZ_THUNK(_rxreturncode, rxreturncode_c, "d1/a0-a1");

/* RXDefaultPort: BUG(orig) without a limit (see the struct) */
static void BLITZ_ROUTINE rxdefportname_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *d = v.rexxportname;

    while ((*d++ = *s++) != 0)
        ;
}
BLITZ_THUNK(_rxdefportname, rxdefportname_c, "d0-d1/a0-a1");

/* --------------------------------------------------------- the lists */

/* _addtolist: d0 into the first 0 long of the list */
static void addtolist(ULONG *l, ULONG d0)
{
    while (*l++)
        ;
    l[-1] = d0;
}

/* _deleteitem: remove the long at a0, the rest moves up */
static void deleteitem(ULONG *a0)
{
    ULONG *a1;

    if (!a0)
        return;
    a1 = a0 + 1;
    *a0 = 0;
    while ((*a0++ = *a1++) != 0)
        ;
}

/* ------------------------------------------------------------ the parser */

static void parseinit(const UBYTE *s)
{
    UBYTE *d = tempstorage;

    while ((*d++ = *s++) != 0)
        ;
    curadr = tempstorage;
}

/* _nextparsearg: the next word to `out`, its length (0: none) */
static ULONG nextparsearg(UBYTE *out)
{
    UBYTE *a0, *a3 = out, c;
    ULONG d7 = 0;

    if (!curadr)
        return 0;
    a0 = curadr;
    for (;;) {
        if (!*a0) {                     /* 'lastarg */
            curadr = NULL;
            return 0;
        }
        if (*a0 != ' ')
            break;
        a0++;
    }
    for (;;) {                          /* 'cmploop */
        c = *a0++;
        if (!c)
            goto endthisfirst;
        if (c == quote)
            break;
        if (c == ' ')
            goto endthis;
        *a3++ = c;
    }
    d7++;                               /* 'insidequotedloop */
    for (;;) {                          /* 'quoteloop */
        c = *a0++;
        if (!c)
            goto endthisfirst;
        if (c != quote) {
            *a3++ = c;
            continue;
        }
        d7++;                           /* 'gotquote */
        if (!(d7 & 1) && *a0 != quote) {
            a0++;                       /* BUG(orig): skips any character */
            goto endthis;
        }
        if ((*a3++ = *a0++) == 0)       /* 'cantquityet */
            goto endthisfirst;
    }
endthisfirst:
    a0--;
endthis:
    curadr = a0;
    return a3 - out;
}

/* ----------------------------------------------------------- messages */

static struct RexxMsg *makemsg(void)
{
    struct RexxMsg *m;
    UBYTE z;

    m = allocvec_z(128, MEMF_CLEAR, &z);
    if (z)                              /* BUG(orig): AllocVec's flags */
        return v.thismsg;
    m->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
    m->rm_Node.mn_Node.ln_Pri = 1;
    m->rm_Node.mn_Node.ln_Name = (char *)msgportname;
    m->rm_Node.mn_Length = 128;
    m->rm_Node.mn_ReplyPort = msgport;
    v.thismsg = m;
    m->rm_Action = msgtype;
    if (v.replyreq)
        v.waitreply = (ULONG)v.thismsg;
    else
        addtolist(v.sentlist, (ULONG)m);
    return m;
}

static void makemsgargs(void)
{
    ULONG *a5 = (ULONG *)((UBYTE *)v.thismsg + 0x28);   /* rm_Args */
    const UBYTE *s = (const UBYTE *)msgstring;
    ULONG n, a;

    if (!tokenise) {
        for (n = 0; s[n]; n++)
            ;
        *a5 = createargstring(s, n);
        return;
    }
    parseinit(s);
    while ((n = nextparsearg(stringbuffer)) != 0) {
        if (!(a = createargstring(stringbuffer, n)))
            break;
        *a5++ = a;                      /* BUG(orig): no limit of 16 */
    }
}

static ULONG waitport_c(ULONG d0);
static ULONG readport_c(void);

/* _sendmsg: PutMsg, with replyreq the wait for the reply and its result
   string at a3 */
static ULONG sendmsg(void)
{
    ULONG d0;

    if (!v.thismsg)
        return 0;
    d0 = putmsg_d0(v.destport, &v.thismsg->rm_Node);
    if (!v.replyreq)
        return d0;                      /* BUG(orig): PutMsg's d0 */
    do {
        oktodelete = 0xffff;
        d0 = waitport_c(d0);
    } while (v.waitreply);
    oktodelete = 0;
    if (!v.returnstring)
        return 0;
    return tostring(v.returnstring);
}

static ULONG sendmessage(void)
{
    makemsg();
    makemsgargs();
    return sendmsg();
}

/* _rxruncommand: d0 = the port name, d1 = the command */
static ULONG runcommand(const UBYTE *port, ULONG cmd)
{
    struct MsgPort *p;

    msgtype = RXCOMM | RXFF_STRING;
    v.replyreq = 0;
    if (!(p = FindPort((CONST_STRPTR)port)))
        return 0;
    v.destport = p;
    msgstring = cmd;
    return sendmessage();
}

static ULONG BLITZ_ROUTINE rxruncommand_c(const UBYTE *port BL_REG(d0), ULONG cmd BL_REG(d1))
{
    return runcommand(port, cmd);
}
BLITZ_THUNK(_rxruncommand, rxruncommand_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE rxruncommand2_c(ULONG cmd BL_REG(d0))
{
    return runcommand(v.rexxportname, cmd);
}
BLITZ_THUNK(_rxruncommand2, rxruncommand2_c, "d1/a0-a1");

/* BUG(orig): the type of the scripts is overwritten by _rxruncommand */
static ULONG BLITZ_ROUTINE rxexecutescript_c(ULONG name BL_REG(d0))
{
    msgtype = RXCOMM | RXFF_RESULT;
    return runcommand((const UBYTE *)realrexxportname, name);
}
BLITZ_THUNK(_rxexecutescript, rxexecutescript_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE rxrunscript_c(ULONG name BL_REG(d0))
{
    msgtype = RXCOMM;
    return runcommand((const UBYTE *)realrexxportname, name);
}
BLITZ_THUNK(_rxrunscript, rxrunscript_c, "d1/a0-a1");

/* dorxsendcommand: d0 = the port name, d5 = the command */
static ULONG sendcommand(const UBYTE *port, ULONG cmd)
{
    struct MsgPort *p;

    msgtype = RXCOMM | RXFF_STRING | RXFF_RESULT;
    v.replyreq = 0xffff;
    if (!(p = FindPort((CONST_STRPTR)port)))
        return 0;
    v.destport = p;
    msgstring = cmd;
    return sendmessage();
}

static ULONG BLITZ_ROUTINE rxsendcommand2_c(ULONG cmd BL_REG(d0))
{
    return sendcommand(v.rexxportname, cmd);
}
BLITZ_THUNK(_rxsendcommand2, rxsendcommand2_c, "d1/a0-a1");

/* RXSendCommand(port$,command$): the result over the two arguments - a3
   back by both lengths (on the a2 stack) + 2 */
static ULONG BLITZ_ROUTINE rxsendcommand_c(const UBYTE *port BL_REG(d0), ULONG cmd BL_REG(d1),
                                           const ULONG *args BL_REG(a2))
{
    blitz_string -= args[-1] + args[-2];
    blitz_string -= 2;
    return sendcommand(port, cmd);
}
BLITZ_THUNK(_rxsendcommand, rxsendcommand_c, "d1/a0-a1");

/* -------------------------------------------------- receiving messages */

static void freemsg(ULONG *a0)
{
    ULONG *args = (ULONG *)((UBYTE *)v.thismsg + 0x28);
    int n;

    for (n = 0; n < 16 && args[n]; n++)         /* DBRA d3 = 15 */
        deleteargstring(args[n]);
    FreeVec(v.thismsg);
    deleteitem(a0);
}

static void storeresult(struct RexxMsg *m)
{
    ULONG s, n;

    v.returncode = m->rm_Result1;
    s = m->rm_Result2;
    if (s) {
        for (n = 0; ((UBYTE *)s)[n]; n++)
            ;
        s = allocst(s, n);
    }
    v.returnstring = (UBYTE *)s;
}

static void handlereply(void)
{
    struct RexxMsg *m = v.thismsg;

    /* _finditem in the sent list: BUG(orig) its result is always 0 (the
       found case falls into 'notfound), so nothing follows from it */
    if (v.waitreply && v.waitreply == (ULONG)m) {
        v.waitreply = 0;
        storeresult(m);
        freemsg(NULL);
        return;
    }
    FreeVec(m);                         /* 'skip: the arguments stay */
}

/* donereadport: the first message of the list becomes current */
static ULONG donereadport(ULONG d0)
{
    if (oktodelete)
        return d0;
    d0 = v.messagelist[0];
    v.currentmessage = (struct RexxMsg *)d0;
    v.currentarg = 1;
    deleteitem(v.messagelist);
    return d0;
}

static ULONG readport_c(void)
{
    struct Message *m;

    for (;;) {
        m = GetMsg(msgport);
        if (!m)
            return donereadport(0);
        v.thismsg = (struct RexxMsg *)m;
        if (m->mn_Node.ln_Type == NT_REPLYMSG)
            handlereply();
        else if (m->mn_Node.ln_Type == NT_MESSAGE)
            addtolist(v.messagelist, (ULONG)v.thismsg);
        else {
            ReplyMsg(m);
            return donereadport(0);
        }
    }
}
BLITZ_THUNK(readport, readport_c, "d1/a0-a1");

static ULONG waitport_c(ULONG d0)
{
    if (!v.waitreply && v.messagelist[0])
        return donereadport(d0);
    WaitPort(msgport);
    return readport_c();
}

static ULONG BLITZ_ROUTINE rxwaitport_c(ULONG d0 BL_REG(d0))
{
    return waitport_c(d0);
}
BLITZ_THUNK(waitport, rxwaitport_c, "d1/a0-a1");

/* ------------------------------------------------- the current message */

/* RXReplyMsg errcode[,result$] (the length of result$ on the a2 stack) */
static ULONG replymsg(ULONG code, const UBYTE *res, const ULONG *args)
{
    struct RexxMsg *m = v.currentmessage;
    UBYTE *a0;

    if (!m)
        return 0;
    m->rm_Result1 = code;
    if (!res)
        m->rm_Result2 = 0;
    else
        m->rm_Result2 = createargstring(res, args[-1]);
    if (findport_a0((UBYTE *)m->rm_Node.mn_Node.ln_Name, &a0)) {
        v.currentmessage->rm_Node.mn_Node.ln_Type = NT_REPLYMSG;
        ReplyMsg(&v.currentmessage->rm_Node);
        return (ULONG)-1;
    }
    /* 'shit: BUG(orig) the Result2 of the a0 FindPort left */
    if (!((struct RexxMsg *)a0)->rm_Result2)
        return 0;
    deleteargstring(v.currentmessage->rm_Result2);
    return 0;
}

static void BLITZ_ROUTINE replymsg_c(ULONG code BL_REG(d0))
{
    replymsg(code, NULL, NULL);
}
BLITZ_THUNK(_replymsg, replymsg_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE replymsg2_c(ULONG code BL_REG(d0), const UBYTE *res BL_REG(d1),
                                      const ULONG *args BL_REG(a2))
{
    replymsg(code, res, args);
}
BLITZ_THUNK(_replymsg2, replymsg2_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE rxsendersname_c(void)
{
    if (!v.currentmessage)
        return 0;
    return tostring((UBYTE *)v.currentmessage->rm_Node.mn_Node.ln_Name);
}
BLITZ_THUNK(_rxsendersname, rxsendersname_c, "d1/a0-a1");

/* RXGetArg(n): argument n (1..16) of the current message */
static ULONG getarg(ULONG d0)
{
    ULONG a;

    BL_SETW(d0, d0 - 1);
    if ((WORD)d0 < 0 || (WORD)d0 > 15 || !v.currentmessage)
        return 0;
    a = (ULONG)v.currentmessage->rm_Args[(WORD)d0];
    if (!a)
        return 0;
    return tostring((UBYTE *)a);
}

static ULONG BLITZ_ROUTINE rxgetarg_c(ULONG n BL_REG(d0))
{
    return getarg(n);
}
BLITZ_THUNK(_rxgetarg, rxgetarg_c, "d1/a0-a1");

/* RXNextArg: MOVE.w currentarg,d0 keeps the upper word of d0 */
static ULONG BLITZ_ROUTINE rxnextarg_c(ULONG d0 BL_REG(d0))
{
    BL_SETW(d0, v.currentarg);
    d0 = getarg(d0);
    if (d0)
        v.currentarg++;
    return d0;
}
BLITZ_THUNK(_rxnextarg, rxnextarg_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE rxnumargs_c(void)
{
    ULONG *a, n = (ULONG)-1;

    if (!v.currentmessage)
        return 0;
    a = (ULONG *)v.currentmessage->rm_Args;
    do
        n++;
    while (*a++);
    return n;
}
BLITZ_THUNK(_rxnumargs, rxnumargs_c, "d1/a0-a1");

/* ------------------------------------------------------------ the port */

static void deleteport(void)
{
    ULONG *l;
    struct Message *m;

    for (;;) {
        readport_c();
        if (!v.currentmessage)
            break;
        replymsg(5, NULL, NULL);
    }
    for (l = v.messagelist; *l; ) {
        m = (struct Message *)*l++;
        l[-1] = 0;
        ReplyMsg(m);
    }
    RemPort(msgport);
    DeleteMsgPort(msgport);
    msgport = NULL;
}

static void BLITZ_ROUTINE rxdeleteport_c(void)
{
    if (msgport)
        deleteport();
}
BLITZ_THUNK(_rxdeleteport, rxdeleteport_c, "d0-d1/a0-a1");

/* RXCreatePort(name$): the name's length on the a2 stack */
static ULONG BLITZ_ROUTINE rxcreateport_c(ULONG name BL_REG(d0), const ULONG *args BL_REG(a2))
{
    ULONG *lists;

    if (msgport)
        return 0;
    msgportname = (UBYTE *)allocst(name, args[-1]);
    msgport = CreateMsgPort();
    if (!msgport)
        return 0;
    msgport->mp_Node.ln_Name = (char *)msgportname;
    if (addport_d0(msgport)) {          /* BUG(orig): d0 of AddPort */
        lists = AllocVec(256, MEMF_PUBLIC | MEMF_CLEAR);
        if (lists) {
            v.messagelist = lists;
            v.sentlist = lists + 32;
            return (ULONG)msgport;
        }
    }
    /* 'oops: BUG(orig) the port stays in the public list */
    DeleteMsgPort(msgport);
    msgport = NULL;
    return 0;
}
BLITZ_THUNK(_rxcreateport, rxcreateport_c, "d1/a0-a1");
