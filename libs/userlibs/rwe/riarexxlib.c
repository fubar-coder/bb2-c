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
 * docs/libs/cmdlibs-port.md "General").  Assembler: the rexxsyslib calls.
 * The variables from rexxbase on are one packed struct in the original's
 * order.
 *
 * Fixed (docs/libs/deviations.md, kind F):
 *   - init makes the default port "REXX" again (the original wrote "REXX"
 *     and a 0 at +5 over the name: the byte at +4 of an earlier
 *     RXDefaultPort stayed) (#532).
 *   - RXRunCommand (and RXExecuteScript, RXRunScript) return -1 when the
 *     message was sent (the original returned the d0 PutMsg leaves)
 *     (#543).
 *   - RXTokenise stores and tests the word of its argument (the original
 *     stored the whole register: "off" with a set upper word stayed on)
 *     (#547).
 *   - RXNextArg advances past every argument it returns, an empty one too
 *     (the original tested the length and returned "" for ever) (#534).
 *   - The character after a closing quote starts the next word; the
 *     original skipped it: `"g"h` gave g alone, and after a quoted last
 *     word it skipped the 0 and read the rest of an earlier, longer
 *     command as further arguments (#538, user decision W4 Q2 (a)).
 *   - In a quoted word two quotes are one quote character and any other
 *     quote closes the word; the original counted the quotes, so after a
 *     doubled quote the next quote did not close and the character behind
 *     it was taken into the word (#535).
 *   - An empty quoted word ("") is an empty argument string; the original
 *     took its length 0 for the end and dropped it and every word behind
 *     it (#536).
 *   - The tokenizer reads the command where it is and builds each word in
 *     a buffer of the command's length (allocated per message; out of
 *     memory: nothing sent, 0); the original copied the command and every
 *     word into buffers of 256 bytes without a limit (in the original over
 *     its own code) (#537).
 *   - A command makes at most 16 arguments (rm_Args); with more words the
 *     16th holds the rest of the command from its 16th word on, as it is
 *     (user decision W4 Q3 (b)); the original had no limit and wrote the
 *     17th and further words over rm_PassPort, rm_CommAddr ... (#527).
 *   - _makemsg tests the result of AllocVec: without memory nothing is
 *     sent (0); the original tested the flags AllocVec leaves (MOVE.l
 *     d0,a1 is a MOVEA) and went on with the message before (#533).
 *   - RXDeletePort frees the block of the lists (FreeVec) and the name
 *     (maxslib FreeSt; at finit the name is left to maxslib, which frees
 *     its strings at the end of the program); the original freed neither
 *     (#530).
 *   - A reply to a message sent without waiting is looked up in the sent
 *     list and freed with its argument strings and its entry; the list
 *     holds 31 messages, RXRunCommand returns 0 when it is full.  The
 *     original's _finditem always returned 0: such replies were freed
 *     with FreeVec only and stayed in the list, which overflowed the
 *     block after 32 messages (#531).
 *   - RXCreatePort allocates the lists before it makes the port public
 *     and does not test AddPort (which returns nothing); on failure it
 *     frees what it has, 0 (D-018).  The original added the port first,
 *     tested the d0 AddPort leaves and on 0, or if the lists could not be
 *     allocated, deleted the port without RemPort: it stayed in exec's
 *     list of public ports (#528).
 *   - RXDefaultPort keeps a copy of the name of its own length (maxslib
 *     AllocSt; the one before freed); the original copied it without a
 *     limit into the 65 bytes of rexxportname, over rexxbase and the
 *     variables behind it (#529).
 *   - RXReplyMsg code,result$ gives a result string only to a message
 *     that asked for one (RXFF_RESULT) and only with the code 0; the
 *     original always put one into rm_Result2 (#539).
 *   - RXReplyMsg looks for the sender's port (the message's ln_Name)
 *     first and makes the result string only when it replies; the
 *     original made it first and, with the port gone, did not reply but
 *     decided on the Result2 at the a0 FindPort leaves whether to delete
 *     it: a string lost, or DeleteArgString(0) in the form without one
 *     (#540).
 *   - RXReplyMsg leaves no current message, also when the sender's port
 *     is gone; the original kept the replied message current, so the
 *     argument functions read a message that belonged to its sender
 *     again (#541).
 *   - The reply of RXSendCommand has a result string only with Result1
 *     0; with an error Result2 is a secondary code, the result "" (the
 *     original read it as the address of a string) (#545).
 *   - RXSendCommand copies the reply's result string to a3 and deletes
 *     it (DeleteArgString); the original copied it with AllocSt, never
 *     freed the copy and never deleted the reply's string (#546).
 *
 * BUG(orig), kept:
 *   - RXExecuteScript/RXRunScript set their message type, but
 *     _rxruncommand overwrites it: both send RXCOMM|RXFF_STRING (the
 *     script name as a string program), without a result (#544).
 *   - RXReturnCode is 0 while no message is current (#542).
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
void _replymsg(void), _replymsg2(void), _rxruncommand2(void);
void _rxruncommand(void), _rxsendersname(void), _rxsendcommand2(void);
void _rxsendcommand(void), _rxnumargs(void), _rxgetarg(void);
void _rxexecutescript(void), _rxrunscript(void), _rxdefportname(void);
void _rxnextarg(void), _rxreturncode(void), _rxtokenise(void);
void _rxmessagelist(void), _rxquotechar(void), riarexxlib_libinit(void);
void riarexxlib_libfinit(void), portchk(void);

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

BLITZ_NULLSUB_AT(init, riarexxlib_libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, riarexxlib_libfinit, 0, 0);

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
static UWORD tokenise;                  /* RXTokenise: the word (fixed #547) */
static UWORD oktodelete;
static const UBYTE *curadr;             /* the parser (parseadr unused) */
static const char rexxname[] = "rexxsyslib.library";
static const char realrexxportname[] = "REXX";
static const char emptyname[] = "";

/* the default port of RXRunCommand/RXSendCommand without a port name:
   NULL "REXX", else RXDefaultPort's copy (AllocSt) - fixed (#529): the
   original's 65 bytes of rexxportname, filled without a limit */
static UBYTE *defportname;

/* rexxbase ... currentarg, as in the original (behind rexxportname) */
static struct BL_PACKED {
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
} v;

_Static_assert(__builtin_offsetof(__typeof__(v), currentarg) == 0xeca - 0xe62 - 66, "layout");

/* RXCreatePort's block: the received messages not yet current
   (messagelist) and the messages sent without waiting (sentlist), each
   a 0 ended list of 32 longs */
struct msglists {
    ULONG messages[32];
    ULONG sent[32];
};
static_assert(sizeof(struct msglists) == 256);

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

/* maxslib FreeSt: d0 = a copy of AllocSt (or 0) */
static void freest(ULONG s)
{
    struct bl_regs r = { .d0 = s };
    BLITZ_ALIBJSR(TOK_freest, r);
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

/**
 * @brief init: open rexxsyslib.library V34; the defaults (quote '"',
 * RXTokenise on, the default port "REXX").
 *
 * Fixed (#532): the default port is "REXX" again; the original wrote
 * "REXX" and a 0 at +5 over the name, so the byte at +4 of an earlier
 * RXDefaultPort stayed.  A copy of an earlier run is maxslib's, freed at
 * that run's end (#529).
 * @return d0: 0
 */
static ULONG BLITZ_ROUTINE libinit_c(void)
{
    quote = 34;
    tokenise = (UWORD)BB_TRUE;
    defportname = NULL;                 /* "REXX": fixed (#532), (#529) */
    msgport = NULL;
    v.rexxbase = NULL;
    v.rexxbase = OpenLibrary((CONST_STRPTR)rexxname, 34);
    return 0;
}
BLITZ_THUNK(riarexxlib_libinit, libinit_c, "d1/a0-a1");

static void deleteport(BB_BOOL freename);

/**
 * @brief finit: RXTokenise on again, delete the port if there is one (as
 * RXDeletePort; the name is left to maxslib, which frees its strings at
 * the end of the program).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    tokenise = (UWORD)BB_TRUE;
    if (msgport)
        deleteport(BB_FALSE);
}
BLITZ_THUNK(riarexxlib_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- small commands */

/**
 * @brief RXQuoteChar char$: the quote character of the parser (RXTokenise
 * on; default '"').
 * @param s  d0: the string, its first character
 */
static void BLITZ_ROUTINE rxquotechar_c(const UBYTE *s BL_REG(d0))
{
    quote = *s;
}
BLITZ_THUNK(_rxquotechar, rxquotechar_c, "d0-d1/a0-a1");

/**
 * @brief RXMessageList: the list of received messages not yet current.
 * @return d0: the list (32 longs, 0 ended; 0 before RXCreatePort)
 */
static ULONG BLITZ_ROUTINE rxmessagelist_c(void)
{
    return (ULONG)v.messagelist;
}
BLITZ_THUNK(_rxmessagelist, rxmessagelist_c, "d1/a0-a1");

/**
 * @brief RXTokenise on|off: whether commands are split into words (with
 * RXQuoteChar quotes) or sent as one argument string.
 *
 * Fixed (#547): only the word is stored; the original's MOVE.l d0 stored
 * the upper word of the register too, so "off" with a set upper word
 * stayed on.
 * @param on  d0.w: 0 off, else on (the upper word is not part of it)
 */
static void BLITZ_ROUTINE rxtokenise_c(ULONG on BL_REG(d0))
{
    tokenise = (UWORD)on;
}
BLITZ_THUNK(_rxtokenise, rxtokenise_c, "d0-d1/a0-a1");

/**
 * @brief RXReturnCode: the Result1 of the last reply to RXSendCommand.
 * @return d0: the code; BUG(orig) (#542): 0 while no message is current
 */
static ULONG BLITZ_ROUTINE rxreturncode_c(void)
{
    if (!v.currentmessage)
        return 0;
    return v.returncode;
}
BLITZ_THUNK(_rxreturncode, rxreturncode_c, "d1/a0-a1");

/**
 * @brief RXDefaultPort portname$: the port of the forms of RXRunCommand
 * and RXSendCommand without a port name.
 *
 * Fixed (#529): the name is kept as a copy of its own length (maxslib
 * AllocSt), the copy before freed; the original copied it without a
 * limit into the 65 bytes of rexxportname, over rexxbase and the
 * variables behind it.
 * @param s  d0: portname$
 */
static void BLITZ_ROUTINE rxdefportname_c(const UBYTE *s BL_REG(d0))
{
    ULONG n;

    for (n = 0; s[n]; n++)
        ;
    if (defportname != (UBYTE *)emptyname)
        freest((ULONG)defportname);
    defportname = n ? (UBYTE *)allocst((ULONG)s, n) : (UBYTE *)emptyname;
}

/* the default port's name */
static const UBYTE *defport(void)
{
    return defportname ? defportname : (const UBYTE *)realrexxportname;
}
BLITZ_THUNK(_rxdefportname, rxdefportname_c, "d0-d1/a0-a1");

/* --------------------------------------------------------- the lists */

/* _addtolist: d0 into the first 0 long of the list */
static void addtolist(ULONG *l, ULONG item)
{
    while (*l++)
        ;
    l[-1] = item;
}

/* the sent list: d0 into its first 0 long, the last one stays the end;
   BB_FALSE if it is full or there is none - fixed (#531): the original
   added without a limit (_addtolist) */
static BB_BOOL addsent(ULONG item)
{
    int n;

    if (!v.sentlist)
        return BB_FALSE;
    for (n = 0; n < 31; n++)
        if (!v.sentlist[n]) {
            v.sentlist[n] = item;
            return BB_TRUE;
        }
    return BB_FALSE;
}

/* _finditem: the slot of `item` in the sent list, or NULL - fixed
   (#531): the original fell from the found case into 'notfound */
static ULONG *findsent(ULONG item)
{
    ULONG *l;

    if (!v.sentlist)
        return NULL;
    for (l = v.sentlist; *l; l++)
        if (*l == item)
            return l;
    return NULL;
}

/* _deleteitem: remove the long at a0, the rest moves up */
static void deleteitem(ULONG *item)
{
    ULONG *next;

    if (!item)
        return;
    next = item + 1;
    *item = 0;
    while ((*item++ = *next++) != 0)
        ;
}

/* ------------------------------------------------------------ the parser */

/* fixed (#537): the command is read where it is; the original copied it
   into tempstorage (256 bytes) without a limit */
static void parseinit(const UBYTE *s)
{
    curadr = s;
}

/* nextparsearg's result when there is no word left */
#define NOWORD ((ULONG)-1)

/* _nextparsearg: the next word to `out`, its length (0: an empty word
   ""; NOWORD: none left - fixed (#536): the original's 0 for both) */
static ULONG nextparsearg(UBYTE *out)
{
    const UBYTE *p;
    UBYTE *dst = out, c;

    if (!curadr)
        return NOWORD;
    p = curadr;
    for (;;) {
        if (!*p) {                      /* 'lastarg */
            curadr = NULL;
            return NOWORD;
        }
        if (*p != ' ')
            break;
        p++;
    }
    for (;;) {                          /* 'cmploop */
        c = *p++;
        if (!c)
            goto endthisfirst;
        if (c == quote)
            break;
        if (c == ' ')
            goto endthis;
        *dst++ = c;
    }
    for (;;) {                          /* 'quoteloop */
        c = *p++;
        if (!c)
            goto endthisfirst;
        if (c != quote) {
            *dst++ = c;
            continue;
        }
        /* fixed (#535): two quotes are one quote, any other quote closes
           the word (the original counted the quotes); fixed (#538): the
           character behind the closing quote is not skipped */
        if (*p != quote)
            goto endthis;
        *dst++ = *p++;
    }
endthisfirst:
    p--;
endthis:
    curadr = p;
    return dst - out;
}

/* ----------------------------------------------------------- messages */

static struct RexxMsg *makemsg(void)
{
    struct RexxMsg *m;

    m = AllocVec(128, MEMF_CLEAR);
    if (!m) {                           /* fixed (#533): the original tested the flags */
        v.thismsg = NULL;
        return NULL;
    }
    m->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
    m->rm_Node.mn_Node.ln_Pri = 1;
    m->rm_Node.mn_Node.ln_Name = (char *)msgportname;
    m->rm_Node.mn_Length = 128;
    m->rm_Node.mn_ReplyPort = msgport;
    v.thismsg = m;
    m->rm_Action = msgtype;
    if (v.replyreq)
        v.waitreply = (ULONG)v.thismsg;
    else if (!addsent((ULONG)m)) {      /* fixed (#531): the list is full */
        FreeVec(m);
        v.thismsg = NULL;
        return NULL;
    }
    return m;
}

/* the arguments of thismsg; `word`: a buffer of the command's length + 1
   for the words (RXTokenise on) */
static void makemsgargs(UBYTE *word)
{
    ULONG *arg = (ULONG *)v.thismsg->rm_Args;
    const UBYTE *s = (const UBYTE *)msgstring, *start, *rest;
    ULONG n, argstr;
    int k;

    if (!tokenise) {
        for (n = 0; s[n]; n++)
            ;
        *arg = createargstring(s, n);
        return;
    }
    parseinit(s);
    for (k = 0; k < 16; k++) {          /* fixed (#527): the original had no limit */
        while (curadr && *curadr == ' ')
            curadr++;
        start = curadr;
        if ((n = nextparsearg(word)) == NOWORD)     /* fixed (#536) */
            break;
        if (k == 15 && curadr) {
            /* more words behind the 16th: the 16th argument is the rest of
               the command from the 16th word on, as it is (W4 Q3 (b)) */
            for (rest = curadr; *rest == ' '; rest++)
                ;
            if (*rest) {
                for (n = 0; start[n]; n++)
                    ;
                argstr = createargstring(start, n);
                if (argstr)
                    *arg = argstr;
                break;
            }
        }
        if (!(argstr = createargstring(word, n)))
            break;
        *arg++ = argstr;
    }
}

static ULONG waitport_c(ULONG result);
static ULONG readport_c(void);

/* _sendmsg: PutMsg, with replyreq the wait for the reply and its result
   string at a3 */
static ULONG sendmsg(void)
{
    ULONG result;

    if (!v.thismsg)
        return 0;
    PutMsg(v.destport, &v.thismsg->rm_Node);
    if (!v.replyreq)
        return BB_TRUE;                 /* fixed (#543): the original's PutMsg d0 */
    result = 0;
    do {
        oktodelete = 0xffff;
        result = waitport_c(result);
    } while (v.waitreply);
    oktodelete = 0;
    if (!v.returnstring)
        return 0;
    /* an empty string as none, as the original's AllocSt of length 0 */
    result = *v.returnstring ? tostring(v.returnstring) : 0;
    deleteargstring((ULONG)v.returnstring);     /* fixed (#546) */
    v.returnstring = NULL;
    return result;
}

static ULONG sendmessage(void)
{
    UBYTE *word = NULL;
    ULONG n;

    if (tokenise) {
        /* fixed (#537): a word buffer of the command's length; the
           original's stringbuffer had 256 bytes and no limit.  Out of
           memory: nothing sent, 0 (D-018) */
        for (n = 0; ((const UBYTE *)msgstring)[n]; n++)
            ;
        if (!(word = AllocVec(n + 1, MEMF_ANY)))
            return 0;
    }
    if (!makemsg()) {                   /* fixed (#533): out of memory, 0 (D-018) */
        FreeVec(word);
        return 0;
    }
    makemsgargs(word);
    FreeVec(word);
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

/**
 * @brief RXRunCommand portname$,command$: send a command to a port, no
 * reply awaited.
 *
 * The message (RXCOMM|RXFF_STRING) goes into the sent list (31 at most:
 * when it is full nothing is sent, 0); its reply is freed with its
 * arguments and its entry (fixed #531).
 * @param port  d0: portname$
 * @param cmd   d1: command$
 * @return d0: 0 if the port is not found or there is no message; else
 *         -1 (fixed #543: the original returned the d0 PutMsg leaves)
 */
static ULONG BLITZ_ROUTINE rxruncommand_c(const UBYTE *port BL_REG(d0), ULONG cmd BL_REG(d1))
{
    return runcommand(port, cmd);
}
BLITZ_THUNK(_rxruncommand, rxruncommand_c, "d1/a0-a1");

/**
 * @brief RXRunCommand command$: as RXRunCommand to the RXDefaultPort port.
 * @param cmd  d0: command$
 * @return d0: as above
 */
static ULONG BLITZ_ROUTINE rxruncommand2_c(ULONG cmd BL_REG(d0))
{
    return runcommand(defport(), cmd);
}
BLITZ_THUNK(_rxruncommand2, rxruncommand2_c, "d1/a0-a1");

/**
 * @brief RXExecuteScript scriptname$: run an ARexx script through the port
 * "REXX".
 *
 * BUG(orig) (#544): its message type (RXCOMM|RXFF_RESULT) is overwritten
 * by _rxruncommand: it sends RXCOMM|RXFF_STRING (the script name as a
 * string program), without a result.
 * @param name  d0: scriptname$
 * @return d0: as RXRunCommand
 */
static ULONG BLITZ_ROUTINE rxexecutescript_c(ULONG name BL_REG(d0))
{
    msgtype = RXCOMM | RXFF_RESULT;
    return runcommand((const UBYTE *)realrexxportname, name);
}
BLITZ_THUNK(_rxexecutescript, rxexecutescript_c, "d1/a0-a1");

/**
 * @brief RXRunScript scriptname$: run an ARexx script through the port
 * "REXX".
 *
 * BUG(orig) (#544): its type RXCOMM is overwritten as in RXExecuteScript.
 * @param name  d0: scriptname$
 * @return d0: as RXRunCommand
 */
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

/**
 * @brief RXSendCommand(command$): as RXSendCommand to the RXDefaultPort
 * port.
 * @param cmd  d0: command$
 * @return d0: the length of the result string (at a3); 0 without
 */
static ULONG BLITZ_ROUTINE rxsendcommand2_c(ULONG cmd BL_REG(d0))
{
    return sendcommand(defport(), cmd);
}
BLITZ_THUNK(_rxsendcommand2, rxsendcommand2_c, "d1/a0-a1");

/**
 * @brief RXSendCommand(portname$,command$): send a command and wait for
 * the reply; its result string.
 *
 * RXCOMM|RXFF_STRING|RXFF_RESULT.  The result goes over the two
 * arguments: a3 back by both lengths (on the a2 stack) + 2.  While it
 * waits, received messages go to the message list.  The reply's Result1
 * is RXReturnCode; its Result2 is the result string only with Result1 0
 * (fixed #545: the original read it as a string also with an error).
 * The result string is copied to a3 and the reply's string deleted
 * (fixed #546: the original copied it with AllocSt, never freed the copy
 * and never deleted the string).
 * @param port  d0: portname$
 * @param cmd   d1: command$
 * @param args  a2: the argument stack, -4/-8(a2) the two lengths
 * @return d0: the length of the result string; 0 without one or if the
 *         port is not found
 */
static ULONG BLITZ_ROUTINE rxsendcommand_c(const UBYTE *port BL_REG(d0), ULONG cmd BL_REG(d1),
                                           const ULONG *args BL_REG(a2))
{
    blitz_string -= args[-1] + args[-2];
    blitz_string -= 2;
    return sendcommand(port, cmd);
}
BLITZ_THUNK(_rxsendcommand, rxsendcommand_c, "d1/a0-a1");

/* -------------------------------------------------- receiving messages */

static void freemsg(ULONG *item)
{
    ULONG *args = (ULONG *)v.thismsg->rm_Args;
    int n;

    for (n = 0; n < 16 && args[n]; n++)         /* DBRA d3 = 15 */
        deleteargstring(args[n]);
    FreeVec(v.thismsg);
    deleteitem(item);
}

static void storeresult(struct RexxMsg *m)
{
    ULONG s;

    v.returncode = m->rm_Result1;
    /* fixed (#545): with Result1 not 0, Result2 is a secondary error
       code, not a string; the original read it as one */
    s = m->rm_Result1 ? 0 : m->rm_Result2;
    /* fixed (#546): the reply's string itself, copied to a3 and deleted
       by sendmsg; the original copied it with AllocSt, never freed the
       copy and never deleted the string */
    v.returnstring = (UBYTE *)s;
}

static void handlereply(void)
{
    struct RexxMsg *m = v.thismsg;

    ULONG *item;

    /* fixed (#531): a reply found in the sent list is freed with its
       arguments and its entry; the original's _finditem always returned 0 */
    if ((item = findsent((ULONG)m)) != NULL) {
        freemsg(item);
        return;
    }
    if (v.waitreply && v.waitreply == (ULONG)m) {
        v.waitreply = 0;
        storeresult(m);
        freemsg(NULL);
        return;
    }
    FreeVec(m);                         /* 'skip: in neither (as the original) */
}

/* donereadport: the first message of the list becomes current */
static ULONG donereadport(ULONG passed)
{
    ULONG first;                        /* d0: the first message */

    if (oktodelete)
        return passed;
    first = v.messagelist[0];
    v.currentmessage = (struct RexxMsg *)first;
    v.currentarg = 1;
    deleteitem(v.messagelist);
    return first;
}

/**
 * @brief RXReadPort: take every message from the port; the first of the
 * message list becomes the current message.
 *
 * Replies to our messages are freed (to RXSendCommand's: its result
 * kept); messages go to the message list; anything else is replied and
 * ends the reading.  Error check portchk: "No message port opened!".
 * @return d0: the new current message (0 if the list is empty); 0 while
 *         RXSendCommand waits (oktodelete: no message becomes current)
 */
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

static ULONG waitport_c(ULONG result)
{
    if (!v.waitreply && v.messagelist[0])
        return donereadport(result);
    WaitPort(msgport);
    return readport_c();
}

/**
 * @brief RXWaitPort: wait for a message (if none is in the list), then
 * RXReadPort.
 * @param result  d0: returned instead of a message while RXSendCommand
 *                waits (oktodelete)
 * @return d0: as RXReadPort
 */
static ULONG BLITZ_ROUTINE rxwaitport_c(ULONG result BL_REG(d0))
{
    return waitport_c(result);
}
BLITZ_THUNK(waitport, rxwaitport_c, "d1/a0-a1");

/* ------------------------------------------------- the current message */

/* RXReplyMsg errcode[,result$] (the length of result$ on the a2 stack):
   BB_TRUE if replied */
static BB_BOOL replymsg(ULONG code, const UBYTE *res, const ULONG *args)
{
    struct RexxMsg *m = v.currentmessage;

    if (!m)
        return BB_FALSE;
    m->rm_Result1 = code;
    m->rm_Result2 = 0;
    /* fixed (#540): the sender's port first, the result string only when
       the message is replied; the original made the string first and,
       with the port gone, tested the Result2 at the a0 FindPort leaves */
    if (!FindPort((CONST_STRPTR)m->rm_Node.mn_Node.ln_Name)) {
        v.currentmessage = NULL;        /* fixed (#541) */
        return BB_FALSE;
    }
    /* fixed (#539): a result string only if one was asked for and only
       with the code 0 (else rm_Result2 is a secondary code); the
       original always made one */
    if (res && (m->rm_Action & RXFF_RESULT) && !code)
        m->rm_Result2 = createargstring(res, args[-1]);
    m->rm_Node.mn_Node.ln_Type = NT_REPLYMSG;
    ReplyMsg(&m->rm_Node);
    v.currentmessage = NULL;            /* fixed (#541): the original kept it */
    return BB_TRUE;
}

/**
 * @brief RXReplyMsg errcode: reply the current message.
 *
 * Nothing without a current message.  If the port of the message's
 * ln_Name no longer exists, it is not replied.  Afterwards there is no
 * current message (fixed #541: the original kept the replied one).
 * @param code  d0: Result1
 */
static void BLITZ_ROUTINE replymsg_c(ULONG code BL_REG(d0))
{
    replymsg(code, NULL, NULL);
}
BLITZ_THUNK(_replymsg, replymsg_c, "d0-d1/a0-a1");

/**
 * @brief RXReplyMsg errcode,result$: reply the current message with a
 * result string (CreateArgString).
 *
 * The string only if the message asked for a result (RXFF_RESULT) and
 * errcode is 0 (fixed #539: the original made it always), and only when
 * the sender's port exists (fixed #540: the original made it first and,
 * with the port gone, tested the Result2 at the a0 FindPort leaves).  As
 * the form without.
 * @param code  d0: Result1
 * @param res   d1: result$, Result2
 * @param args  a2: the argument stack, -4(a2) the length of result$
 */
static void BLITZ_ROUTINE replymsg2_c(ULONG code BL_REG(d0), const UBYTE *res BL_REG(d1),
                                      const ULONG *args BL_REG(a2))
{
    replymsg(code, res, args);
}
BLITZ_THUNK(_replymsg2, replymsg2_c, "d0-d1/a0-a1");

/**
 * @brief RXSendersName: the name of the current message (ln_Name: the
 * sender's port).
 * @return d0: the length of the name (at a3); 0 without a current
 *         message
 */
static ULONG BLITZ_ROUTINE rxsendersname_c(void)
{
    if (!v.currentmessage)
        return 0;
    return tostring((UBYTE *)v.currentmessage->rm_Node.mn_Node.ln_Name);
}
BLITZ_THUNK(_rxsendersname, rxsendersname_c, "d1/a0-a1");

/* RXGetArg(n): argument n (1..16) of the current message */
static ULONG getarg(ULONG n)
{
    ULONG argstr;

    BL_SETW(n, n - 1);
    if ((WORD)n < 0 || (WORD)n > 15 || !v.currentmessage)
        return 0;
    argstr = (ULONG)v.currentmessage->rm_Args[(WORD)n];
    if (!argstr)
        return 0;
    return tostring((UBYTE *)argstr);
}

/**
 * @brief RXGetArg(argnum): argument argnum (1..16) of the current message.
 * @param n  d0.w: the argument's number
 * @return d0: the length of the argument (at a3); 0 if out of range,
 *         without a current message or argument
 */
static ULONG BLITZ_ROUTINE rxgetarg_c(ULONG n BL_REG(d0))
{
    return getarg(n);
}
BLITZ_THUNK(_rxgetarg, rxgetarg_c, "d1/a0-a1");

/**
 * @brief RXNextArg: the next argument of the current message (from 1 on,
 * as RXGetArg).
 *
 * Fixed (#534): it advances when the argument slot is not 0; the
 * original tested the length, so at an empty argument it stayed.
 * @param argnum  d0: no argument; MOVE.w currentarg,d0 keeps its upper
 *                word
 * @return d0: the length of the argument (at a3); 0 at the end
 */
static ULONG BLITZ_ROUTINE rxnextarg_c(ULONG argnum BL_REG(d0))
{
    ULONG arg;                          /* d0: the argument string */

    BL_SETW(argnum, v.currentarg);
    arg = getarg(argnum);
    if (v.currentmessage && v.currentarg >= 1 && v.currentarg <= 16
        && v.currentmessage->rm_Args[v.currentarg - 1])
        v.currentarg++;                 /* fixed (#534): the slot, not the length */
    return arg;
}
BLITZ_THUNK(_rxnextarg, rxnextarg_c, "d1/a0-a1");

/**
 * @brief RXNumArgs: the number of arguments of the current message (up to
 * the first 0 of rm_Args).
 * @return d0: the count; 0 without a current message
 */
static ULONG BLITZ_ROUTINE rxnumargs_c(void)
{
    ULONG *arg, n = (ULONG)-1;

    if (!v.currentmessage)
        return 0;
    arg = (ULONG *)v.currentmessage->rm_Args;
    do
        n++;
    while (*arg++);
    return n;
}
BLITZ_THUNK(_rxnumargs, rxnumargs_c, "d1/a0-a1");

/* ------------------------------------------------------------ the port */

/* RXDeletePort, finit: reply what is waiting, delete the port, free the
   lists and (freename) the name */
static void deleteport(BB_BOOL freename)
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
    /* fixed (#530): the original freed neither the lists nor the name */
    FreeVec(v.messagelist);             /* the block: struct msglists */
    v.messagelist = v.sentlist = NULL;
    if (freename)
        freest((ULONG)msgportname);
    msgportname = NULL;
}

/**
 * @brief RXDeletePort: delete the port.
 *
 * Messages still waiting are replied (RXReplyMsg 5 for each current
 * message, then ReplyMsg for the rest of the message list), then
 * RemPort, DeleteMsgPort; the block of the lists and the name are freed
 * (fixed #530: the original freed neither).  Error check portchk.
 */
static void BLITZ_ROUTINE rxdeleteport_c(void)
{
    if (msgport)
        deleteport(BB_TRUE);
}
BLITZ_THUNK(_rxdeleteport, rxdeleteport_c, "d0-d1/a0-a1");

/**
 * @brief RXCreatePort(name$): make the public port name$ and the message
 * lists.
 *
 * The name is copied with AllocSt.  Fixed (#528): the lists are
 * allocated before the port is made public, AddPort is not tested, and on
 * failure what was allocated is freed (0, D-018); the original added the
 * port first, tested the d0 AddPort leaves and on failure deleted the port
 * without RemPort, so it stayed in the public list.
 * @param name  d0: name$
 * @param args  a2: the argument stack, -4(a2) the length of name$
 * @return d0: the MsgPort; 0 if there is one already or on failure
 */
static ULONG BLITZ_ROUTINE rxcreateport_c(ULONG name BL_REG(d0), const ULONG *args BL_REG(a2))
{
    struct msglists *lists;

    if (msgport)
        return 0;
    /* fixed (#528): the lists first, the port public last, AddPort not
       tested; the original deleted a public port without RemPort */
    lists = AllocVec(sizeof(struct msglists), MEMF_PUBLIC | MEMF_CLEAR);
    if (!lists)
        return 0;
    msgportname = (UBYTE *)allocst(name, args[-1]);
    msgport = CreateMsgPort();
    if (!msgport) {
        FreeVec(lists);
        freest((ULONG)msgportname);
        msgportname = NULL;
        return 0;
    }
    msgport->mp_Node.ln_Name = (char *)msgportname;
    v.messagelist = lists->messages;
    v.sentlist = lists->sent;
    AddPort(msgport);
    return (ULONG)msgport;
}
BLITZ_THUNK(_rxcreateport, rxcreateport_c, "d1/a0-a1");
