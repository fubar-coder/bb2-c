/*
 * rwe riarexxlib - riarexxlib (userlib 133): the C build or the
 * reference build of its source (ref/riarexxlib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), one set as LIBSET says, observed as
 * cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_riarexxlib.txt).
 *
 *   riarexxlib DIR     (DIR with riarexxlib, maxslib, memlib .obj/.orig)
 *
 * No RexxMast is needed: the messages go to the library's own port, to a
 * port of the test (RWETEST: messages the test sends, the replies come
 * back there) and to a responder process (port RWERESP, and REXX if no
 * such port exists): it logs every message (type, priority, length,
 * action, ln_Name, reply port, the argument strings) and replies with a
 * result code and string the test sets, so that RXSendCommand, which
 * waits, gets its reply.  The library's own port is RWERX_P.  Observed:
 * d0, a3 and the strings of the string functions, the messages the
 * responder got, the replies the test gets (Result1, the Result2
 * string), the kept registers.
 * Covered: every entry; functions without a port and without a current
 * message (0); the error check portchk with a port; RXCreatePort twice;
 * the tokenizer, one command per case (spaces, quotes, empty quotes in a
 * word, a doubled quote: fixed #535, the character after a closing
 * quote: #538, an empty word: #536, a closing quote at the end of a
 * command after a longer one: #538, RXQuoteChar), RXTokenise off and
 * "off" with a word whose upper word is set (#547, fixed); RXGetArg 0..17,
 * RXNextArg to the end, RXNumArgs, RXSendersName, RXReplyMsg both forms
 * to our own port, to the test (the Result2 string) and to a sender
 * whose port is gone (no reply; its result string: #540); replies to our messages
 * (freed); RXRunCommand both forms, to a missing port; RXSendCommand
 * both forms (the result over the arguments), with result code 10 and no
 * string, to a missing port, with a message arriving while it waits;
 * RXReturnCode (also without a current message: #542, fixed); RXDefaultPort;
 * RXExecuteScript/RXRunScript (to the responder if there is no REXX port,
 * else to RexxMast; the original sends both as RXCOMM|RXFF_STRING, fixed
 * #544: RXCOMM|RXFF_RESULT, waiting, and RXCOMM, the name one argument);
 * RXDeletePort with messages queued (replied with
 * 5), with a reply outstanding (#1095: for 2 s, and for 8 s, past a time
 * limit of 5 s) and with 33 messages received
 * (#1100; one of them current: #1101); 40 messages and the reply of an
 * RXSendCommand behind them, then read and replied one by one (#1100:
 * the order, the sent list);
 * finit with a port; init after
 * RXDefaultPort (the original: the byte at +4 stays, "REXXE"; fixed
 * #532: "REXX", the responder gets it).
 * Not covered: the error check without a port (a runtime error, TRAP
 * #0), a failing AllocVec (the bug-orig tests of #528 and #533 with
 * inject), more than 40 messages, more than 16 arguments, messages of
 * other types (replied at once).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <dos/dostags.h>
#include <rexx/storage.h>
#define __NOLIBBASE__
#define REXXSYSLIB_BASE_NAME TRexxBase
#include <proto/rexxsyslib.h>
#undef __NOLIBBASE__

static struct Library *TRexxBase;

#define LIB "riarexxlib"

enum { CREATEPORT, DELETEPORT, READPORT, WAITPORT, REPLYMSG, RUNCOMMAND, SENDERSNAME,
       SENDCOMMAND = 9, NUMARGS, GETARG = 13, EXECUTESCRIPT = 16, RUNSCRIPT,
       DEFAULTPORT = 22, NEXTARG = 24, RETURNCODE, TOKENISE, MESSAGELIST, QUOTECHAR,
       INIT, FINIT };

static const char *own = "RWERX_P";     /* the library's port */
static UBYTE sbuf[1024];                /* string area (a3) */
static ULONG stk[4];                    /* the compiler's a2 stack */
static struct MsgPort *testport;
static int have_rexx;                   /* the responder serves REXX */

/* ------------------------------------------------------- the responder */

struct logent {
    char name[40];
    UBYTE type, pri;
    UWORD len;
    ULONG action, replyport;
    int nargs, port;
    char args[16][48];
};
static struct logent rlog[24];
static volatile int nrlog, resp_done;
static volatile LONG resp_rc;
static const char *volatile resp_res;
static struct Task *maintask;
static volatile int resp_ready;
static volatile int resp_hold, resp_release;    /* #1095: hold a message (ticks) */
static struct MsgPort *resp_redirect;           /* its reply port, if released */

static void handle(struct RexxMsg *m, int port)
{
    const char *n = m->rm_Node.mn_Node.ln_Name;
    int i;
    struct logent *e;

    if (nrlog < 24) {
        e = &rlog[nrlog];
        memset(e, 0, sizeof *e);
        if (n)
            strncpy(e->name, n, 39);
        e->type = m->rm_Node.mn_Node.ln_Type;
        e->pri = m->rm_Node.mn_Node.ln_Pri;
        e->len = m->rm_Node.mn_Length;
        e->action = m->rm_Action;
        e->replyport = (ULONG)m->rm_Node.mn_ReplyPort;
        e->port = port;
        for (i = 0; i < 16 && m->rm_Args[i]; i++)
            strncpy(e->args[i], (char *)m->rm_Args[i], 47);
        e->nargs = i;
        nrlog++;
    }
    if (resp_hold) {
        /* the message stays with the responder until the test releases it
           (it does after RXDeletePort returned: then the library's port
           is gone and the reply goes to the test's port instead of a freed
           one) or resp_hold ticks have passed (a library that waits: the
           reply goes to its port) */
        int t, ticks = resp_hold;

        resp_hold = 0;
        for (t = 0; t < ticks && !resp_release; t++)
            Delay(1);
        if (resp_release)
            m->rm_Node.mn_ReplyPort = resp_redirect;
    }
    m->rm_Result1 = resp_rc;
    m->rm_Result2 = 0;
    if (resp_rc == 0 && resp_res)
        m->rm_Result2 = (LONG)CreateArgstring((CONST_STRPTR)resp_res, strlen(resp_res));
    ReplyMsg(&m->rm_Node);
    resp_done++;
    Signal(maintask, SIGBREAKF_CTRL_F);
}

static void responder(void)
{
    struct MsgPort *p[2] = { NULL, NULL };
    struct Message *m;
    ULONG sigs;
    int i;

    p[0] = CreateMsgPort();
    p[0]->mp_Node.ln_Name = (char *)"RWERESP";
    p[0]->mp_Node.ln_Pri = 0;
    AddPort(p[0]);
    if (have_rexx) {
        p[1] = CreateMsgPort();
        p[1]->mp_Node.ln_Name = (char *)"REXX";
        AddPort(p[1]);
    }
    resp_ready = 1;
    Signal(maintask, SIGBREAKF_CTRL_F);
    for (;;) {
        sigs = Wait(SIGBREAKF_CTRL_C | 1UL << p[0]->mp_SigBit
                    | (p[1] ? 1UL << p[1]->mp_SigBit : 0));
        for (i = 0; i < 2; i++)
            while (p[i] && (m = GetMsg(p[i])) != NULL)
                handle((struct RexxMsg *)m, i);
        if (sigs & SIGBREAKF_CTRL_C)
            break;
    }
    for (i = 0; i < 2; i++)
        if (p[i]) {
            RemPort(p[i]);
            DeleteMsgPort(p[i]);
        }
    Forbid();
    resp_ready = 0;
    Signal(maintask, SIGBREAKF_CTRL_E);
}

/* wait until the responder has answered `n` messages in all */
static void waitresp(int n)
{
    while (resp_done < n)
        Wait(SIGBREAKF_CTRL_F);
}

/* the last message the responder got */
static void samelog(const char *what, int port)
{
    char w[80];
    int k;

    BOTH("responder got it", nrlog > 0);
    if (!nrlog)
        return;
    {
        const struct logent *e = &rlog[nrlog - 1];
        sprintf(w, "%s: message", what);
        OBS(w, nrlog);
        OBS("type", e->type);
        OBS("pri", e->pri);
        OBS("length", e->len);
        OBS("action", e->action);
        OBS("port", e->port);
        BOTH("port served", e->port == port);
        OBS("args", e->nargs);
        BOTH("ln_Name", !strcmp(e->name, own));
        BOTH("reply port", e->replyport == (ULONG)FindPort((STRPTR)own));
        for (k = 0; k < 16; k++)
            OBSBYTES("argument", e->args[k], 48);
    }
}

/* ------------------------------------------------------------ helpers */

/* a call; a3 = the string area at `off`, the string lengths `l1`, `l2`
   on the a2 stack */
static void call(int idx, int form, ULONG d0, ULONG d1, ULONG l1, ULONG l2, ULONG off)
{
    DO1(ENT(LIB, idx, form), R.d[0] = d0; R.d[1] = d1;
        stk[0] = l2; stk[1] = l1; R.a[2] = (ULONG)&stk[2];
        R.a[3] = (ULONG)sbuf + off);
}

/* the string result: d0, a3, the bytes */
static void samestr(void)
{
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)sbuf);
    OBSBYTES("string area", sbuf, sizeof sbuf);
}

static void clearstr(void)
{
    memset(sbuf, 0x5a, sizeof sbuf);
}

static void longfn(int idx, const char *name)
{
    ROUT = name;
    call(idx, 0, MARK(0), MARK(1), 0, 0, 0);
    OBS("d0", R.d[0]);
}

/* RXReadPort / RXWaitPort: a message current (`expect`) or 0 */
static void readport(int idx, int expect, const char *what)
{
    ROUT = idx == READPORT ? "RXReadPort" : "RXWaitPort";
    strcpy(VEC, what);
    call(idx, 0, MARK(0), MARK(1), 0, 0, 0);
    OBS("message", R.d[0] != 0);
    BOTH("message as expected", (R.d[0] != 0) == expect);
}

static void strfn(int idx, ULONG d0, const char *name)
{
    clearstr();
    ROUT = name;
    call(idx, 0, d0, MARK(1), 0, 0, 16);
    samestr();
}

/* the current message: numargs, every argument, RXNextArg to the end */
static void current(const char *what)
{
    ULONG i;

    strcpy(VEC, what);
    longfn(NUMARGS, "RXNumArgs");
    for (i = 0; i <= 17; i++) {
        sprintf(VEC, "%s arg %lu", what, i);
        strfn(GETARG, i, "RXGetArg");
    }
    for (i = 0; i < 18; i++) {
        sprintf(VEC, "%s next %lu", what, i);
        strfn(NEXTARG, MARK(0), "RXNextArg");
        if (!R.d[0])
            break;
    }
    strcpy(VEC, what);
    clearstr();
    ROUT = "RXSendersName";
    call(SENDERSNAME, 0, 0, 0, 0, 0, 16);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)sbuf);
    BOTH("sender", R.d[0] == 0 || !strcmp((char *)sbuf + 16, own)
                   || !strncmp((char *)sbuf + 16, "RWE", 3));
    longfn(RETURNCODE, "RXReturnCode");
}

/* RXRunCommand to the own port (port = NULL) or `port`; form 0: the
   default port */
static void run(int form, const char *port, const char *cmd)
{
    ROUT = "RXRunCommand";
    sprintf(VEC, "%s \"%s\"", port ? port : (form ? "own" : "default"), cmd);
    if (form)
        DO1(ENT(LIB, RUNCOMMAND, 1), R.d[0] = (ULONG)(port ? port : own);
            R.d[1] = (ULONG)cmd; R.a[3] = (ULONG)sbuf);
    else
        DO1(ENT(LIB, RUNCOMMAND, 0), R.d[0] = (ULONG)cmd; R.a[3] = (ULONG)sbuf);
    /* d0: the original's is PutMsg's (#543, fixed: -1); 0 if the port
       is missing */
    OBS("sent", R.d[0] != 0 || port == NULL || FindPort((STRPTR)port) != NULL);
}

/* RXSendCommand form 1: port$ and command$ in the string area, a3 behind
   them; form 0: command$ only */
static void send(int form, const char *port, const char *cmd, LONG rc, const char *res)
{
    ULONG l1 = strlen(port), l2 = strlen(cmd);

    resp_rc = rc;
    resp_res = res;
    clearstr();
    ROUT = "RXSendCommand";
    sprintf(VEC, "form %d %s \"%s\" rc %ld res %s", form, port, cmd, rc, res ? res : "-");
    strcpy((char *)sbuf, port);
    strcpy((char *)sbuf + l1 + 1, cmd);
    if (form)
        DO1(ENT(LIB, SENDCOMMAND, 1), R.d[0] = (ULONG)sbuf;
            R.d[1] = (ULONG)sbuf + l1 + 1; stk[0] = l1; stk[1] = l2;
            R.a[2] = (ULONG)&stk[2]; R.a[3] = (ULONG)sbuf + l1 + l2 + 2);
    else
        DO1(ENT(LIB, SENDCOMMAND, 0), R.d[0] = (ULONG)sbuf + l1 + 1;
            R.a[3] = (ULONG)sbuf + l1 + l2 + 2);
    samestr();
}

static void reply(int form, ULONG code, const char *res, const char *what)
{
    ROUT = "RXReplyMsg";
    sprintf(VEC, "%s %lu %s", what, code, res ? res : "-");
    call(REPLYMSG, form, code, (ULONG)res, res ? strlen(res) : 0, 0, 0);
}

/* a command to the own port, received as the current message, its
   arguments, replied (RXReplyMsg 0) and the reply read */
static void tokenised(const char *cmd, const char *what)
{
    run(1, NULL, cmd);
    readport(READPORT, 1, what);
    current(what);
    reply(0, 0, NULL, what);
    readport(READPORT, 0, "the reply");
}

static void strarg(int idx, const char *name, const char *s)
{
    ROUT = name;
    sprintf(VEC, "\"%s\"", s);
    call(idx, 0, (ULONG)s, MARK(1), strlen(s), 0, 0);
}

/* a message of the test to the library's port, ln_Name = `from` */
static struct RexxMsg *tmsg[4];
static void testmsg(int n, const char *from, const char *a1, const char *a2)
{
    struct RexxMsg *m = AllocVec(128, MEMF_PUBLIC | MEMF_CLEAR);
    m->rm_Node.mn_Node.ln_Name = (char *)from;
    m->rm_Node.mn_Length = 128;
    m->rm_Node.mn_ReplyPort = testport;
    m->rm_Action = RXCOMM;
    m->rm_Args[0] = CreateArgstring((CONST_STRPTR)a1, strlen(a1));
    if (a2)
        m->rm_Args[1] = CreateArgstring((CONST_STRPTR)a2, strlen(a2));
    tmsg[n] = m;
    PutMsg(FindPort((STRPTR)own), &m->rm_Node);
}

/* the replies at the test's port (`n` expected): Result1, whether there
   is a Result2 and its string; free the messages */
static void testreplies(int n)
{
    struct RexxMsg *m;
    int got = 0, i;

    while ((m = (struct RexxMsg *)GetMsg(testport)) != NULL) {
        int ours = 0;
        for (i = 0; i < 4; i++)
            if (m == tmsg[i])
                ours = 1;
        if (!ours)
            continue;
        got++;
        OBS("reply Result1", m->rm_Result1);
        OBS("reply Result2", m->rm_Result2 != 0);
        if (m->rm_Result2) {
            OBSSTR("reply Result2 string", (const char *)m->rm_Result2);
            DeleteArgstring((UBYTE *)m->rm_Result2);
        }
        for (i = 0; i < 2; i++)
            if (m->rm_Args[i])
                DeleteArgstring(m->rm_Args[i]);
        FreeVec(m);
    }
    BOTH("replies", got == n);
}

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "maxslib", "memlib", NULL };
    struct Process *resp;
    int i, n;

    if (argc < 2) {
        printf("usage: riarexxlib DIR\n");
        return 20;
    }
    TRexxBase = OpenLibrary((STRPTR)"rexxsyslib.library", 34);
    if (!TRexxBase) {
        check("riarexxlib: rexxsyslib.library", FALSE);
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("RXMessageList", d1-d7|a0-a3|a6);
    KEEPS("RXNumArgs", d1-d7|a1-a3|a6);
    KEEPS("RXGetArg", d2-d7|a1|a2|a6);
    KEEPS("RXNextArg", d2-d7|a1|a2|a6);
    KEEPS("RXSendersName", d1-d7|a1|a2|a6);
    KEEPS("RXReturnCode", d1-d7|a0-a3|a6);
    KEEPS("RXCreatePort", d2-d7|a3);
    KEEPS("portchk", d0-d7|a0-a3|a6);
    KEEPS("RXRunCommand", d4|a2|a3);
    KEEPS("RXReadPort", d2-d7|a2|a3);
    KEEPS("RXReplyMsg", d3-d7|a3);
    KEEPS("RXTokenise", d0-d7|a0-a3|a6);
    KEEPS("RXQuoteChar", d0-d7|a1-a3|a6);
    KEEPS("RXDefaultPort", d0-d7|a2|a3|a6);
    KEEPS("RXWaitPort", d2-d7|a2|a3);
    KEEPS("RXSendCommand", d4);
    KEEPS("RXExecuteScript", d1|d4|a2|a3);
    KEEPS("RXRunScript", d1|d4|a2|a3);
    KEEPS("RXDeletePort", d3-d7|a3);
    KEEPS("finit", d2-d7|a3);
    maintask = FindTask(NULL);
    testport = CreateMsgPort();
    testport->mp_Node.ln_Name = (char *)"RWETEST";
    AddPort(testport);
    have_rexx = FindPort((STRPTR)"REXX") == NULL;
    SetSignal(0, SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_E);
    resp = CreateNewProcTags(NP_Entry, (ULONG)responder, NP_Name, (ULONG)"rwe rexx responder",
                             NP_StackSize, 8192, TAG_END);
    if (!resp) {
        check("riarexxlib: responder process", FALSE);
        return 20;
    }
    while (!resp_ready)
        Wait(SIGBREAKF_CTRL_F);
    if (!have_rexx)
        printf("riarexxlib: a REXX port exists: RXExecuteScript/RXRunScript to it\n");

    ROUT = "memlib init";
    memlib(7);
    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, INIT, 0), (void)0);

    /* no port, no message */
    strcpy(VEC, "no port");
    longfn(MESSAGELIST, "RXMessageList");
    current("no port");

    /* the port */
    for (i = 0; i < 2; i++) {
        ROUT = "RXCreatePort";
        strcpy(VEC, "");
        DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
            stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
        if (i == 0)
            BOTH("the port", R.d[0] && R.d[0] == (ULONG)FindPort((STRPTR)own));
        else
            OBS("second port: 0", R.d[0]);
    }
    ROUT = "portchk";
    strcpy(VEC, "with a port");
    DO1(ERR(LIB, READPORT, 0), (void)0);
    ROUT = "RXMessageList";
    call(MESSAGELIST, 0, 0, 0, 0, 0, 0);
    BOTH("list", R.d[0] != 0);

    /* the tokenizer, own port */
    run(1, NULL, "load  \"a b\"  i");
    readport(READPORT, 1, "own message");
    current("tokenised");
    reply(1, 0, "the result", "own");
    readport(READPORT, 0, "the reply");
    /* one command per case of the quotes, so that each fix has its own */
    tokenised("c\"\"d e", "empty quotes in a word");
    tokenised("\"e\"\"f\" c d", "doubled quote");
    tokenised("\"g\"h", "after a closing quote");
    tokenised("a \"\" c", "empty word");
    tokenised("one two three four five six", "long command");
    tokenised("say \"hi\"", "closing quote at the end");
    ROUT = "RXTokenise";
    strcpy(VEC, "0");
    call(TOKENISE, 0, 0, 0, 0, 0, 0);
    run(1, NULL, "x y  \"z\"");
    readport(READPORT, 1, "untokenised");
    current("untokenised");
    reply(0, 3, NULL, "untokenised");
    readport(READPORT, 0, "the reply");
    ROUT = "RXTokenise";
    strcpy(VEC, "0 with an upper word");
    call(TOKENISE, 0, 0x5a010000UL, 0, 0, 0, 0);
    run(1, NULL, "x y  \"z\"");
    readport(READPORT, 1, "still tokenised");
    current("still tokenised");
    /* on again for the rest of the scenario, whatever the call before did */
    ROUT = "RXTokenise";
    strcpy(VEC, "1");
    call(TOKENISE, 0, 1, 0, 0, 0, 0);
    strarg(QUOTECHAR, "RXQuoteChar", "'");
    run(1, NULL, "a 'b c' \"d e\" ''");
    readport(READPORT, 1, "quote '");
    current("quote '");
    strarg(QUOTECHAR, "RXQuoteChar", "\"");
    run(1, NULL, "");                   /* no argument */
    readport(READPORT, 1, "empty");
    current("empty");
    strarg(DEFAULTPORT, "RXDefaultPort", "RWERX_X");
    run(0, NULL, "nowhere");            /* the default port is missing */
    {
        /* the default port = the own port */
        ROUT = "RXDefaultPort";
        strcpy(VEC, "own");
        DO1(ENT(LIB, DEFAULTPORT, 0), R.d[0] = (ULONG)own);
    }
    run(0, NULL, "hello default");
    readport(WAITPORT, 1, "RXWaitPort");
    current("default");
    reply(0, 0, NULL, "default");
    readport(WAITPORT, 0, "RXWaitPort: the reply");     /* waits for it */

    /* the responder */
    n = 0;
    run(1, "RWERESP", "cmd a b");
    waitresp(n += 1);
    samelog("RXRunCommand", 0);
    readport(READPORT, 0, "responder's reply");
    run(1, "RWENOWHERE", "cmd");
    send(1, "RWERESP", "get \"the\" thing", 0, "the result");
    n += 1;
    samelog("RXSendCommand", 0);
    longfn(RETURNCODE, "RXReturnCode");         /* no current message: 0 */
    run(1, NULL, "pending");
    readport(READPORT, 1, "current");
    send(1, "RWERESP", "fail", 10, NULL);
    n += 1;
    longfn(RETURNCODE, "RXReturnCode");
    run(1, NULL, "arrives while waiting");
    send(1, "RWERESP", "again", 0, "x");
    n += 1;
    readport(READPORT, 1, "arrived while waiting");
    current("arrived while waiting");
    longfn(RETURNCODE, "RXReturnCode");
    send(1, "RWENOWHERE", "cmd", 0, "x");
    strarg(DEFAULTPORT, "RXDefaultPort", "RWERESP");
    send(0, "", "default send", 0, "from the default port");
    n += 1;
    samelog("RXSendCommand default", 0);
    send(0, "", "empty result", 0, "");
    n += 1;
    if (have_rexx) {
        ROUT = "RXExecuteScript";
        strcpy(VEC, "");
        call(EXECUTESCRIPT, 0, (ULONG)"RAM:script.rexx", 0, 0, 0, 0);
        waitresp(n += 1);
        samelog("RXExecuteScript", 1);
        BOTH("RXCOMM|RXFF_STRING", rlog[nrlog - 1].action == (RXCOMM | RXFF_STRING));
        ROUT = "RXRunScript";
        call(RUNSCRIPT, 0, (ULONG)"RAM:script.rexx", 0, 0, 0, 0);
        waitresp(n += 1);
        samelog("RXRunScript", 1);
        readport(READPORT, 0, "the script replies");
    } else {
        /* the real RexxMast: the original runs the "script name" as a
           string program (#544), tokenised, its first word "exit"; fixed,
           RXExecuteScript waits for the reply itself and RexxMast finds
           no script of that name.  A second later the reply is read (the
           original's) or there is none (RXExecuteScript's, fixed) */
        ROUT = "RXExecuteScript";
        strcpy(VEC, "RexxMast");
        call(EXECUTESCRIPT, 0, (ULONG)"exit 0", 0, 0, 0, 0);
        Delay(50);
        readport(READPORT, 0, "RexxMast's reply");
        ROUT = "RXRunScript";
        call(RUNSCRIPT, 0, (ULONG)"exit 0", 0, 0, 0, 0);
        Delay(50);
        readport(READPORT, 0, "RexxMast's reply");
    }

    /* messages of the test */
    testmsg(0, "RWETEST", "from", "the test");
    readport(READPORT, 1, "test message");
    current("test message");
    reply(1, 0, "answer", "to the test");
    testreplies(1);
    testmsg(1, "RWEGONE", "gone", NULL);
    readport(READPORT, 1, "sender gone");
    reply(1, 0, "lost", "sender gone");
    testreplies(0);
    FreeVec(tmsg[1]);                   /* not replied: ours again */
    tmsg[1] = NULL;
    testmsg(2, "RWETEST", "one", NULL);
    testmsg(3, "RWETEST", "two", NULL);
    ROUT = "RXDeletePort";
    strcpy(VEC, "2 queued");
    call(DELETEPORT, 0, 0, 0, 0, 0, 0);
    testreplies(2);
    BOTH("port gone", FindPort((STRPTR)own) == NULL);
    ROUT = "RXDeletePort";
    strcpy(VEC, "no port");
    call(DELETEPORT, 0, 0, 0, 0, 0, 0);

    /* RXDeletePort with a sent message whose reply is outstanding (#1095):
       the responder holds the message; the original's RXDeletePort returns
       with the reply still outstanding (a library that waits for it, as
       the manual says, returns when the responder's 2 s are over, the
       reply received).  Safe: once RXDeletePort has returned the test
       releases the message and the responder replies to the test's port,
       not to the library's (deleted); the test frees the message and its
       arguments.  The held reply is the only one that can arrive late. */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for the outstanding reply");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    {
        struct RexxMsg *held;
        int before = resp_done;

        resp_release = 0;
        resp_redirect = testport;
        resp_hold = 100;                /* 2 s */
        run(1, "RWERESP", "held");
        while (resp_hold)               /* the responder has it */
            Delay(1);
        ROUT = "RXDeletePort";
        strcpy(VEC, "reply outstanding");
        call(DELETEPORT, 0, 0, 0, 0, 0, 0);
        OBS("replied before it returned", resp_done > before);
        resp_release = 1;
        waitresp(before + 1);
        BOTH("port gone", FindPort((STRPTR)own) == NULL);
        n = resp_done;
        /* the original: the reply is at the test's port; ours to free */
        while ((held = (struct RexxMsg *)GetMsg(testport)) != NULL) {
            for (i = 0; i < 16 && held->rm_Args[i]; i++)
                DeleteArgstring(held->rm_Args[i]);
            FreeVec(held);
        }
    }

    /* RXDeletePort with a reply outstanding past the time limit (#1095):
       the responder holds the message for 8 s.  The original returns at
       once; a library that waits up to 5 s for the replies returns when
       its time limit is over, before the reply, with the port off the
       public list.  Then the test releases the message: the reply goes to
       the test's port (in either case never to the library's port, whose
       memory the original has freed); the test frees the message and its
       arguments. */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for the late reply");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    {
        struct RexxMsg *held;
        struct DateStamp t0, t1;
        LONG ticks;
        int before = resp_done, late = 0;

        resp_release = 0;
        resp_redirect = testport;
        resp_hold = 400;                /* 8 s */
        run(1, "RWERESP", "held long");
        while (resp_hold)               /* the responder has it */
            Delay(1);
        ROUT = "RXDeletePort";
        strcpy(VEC, "reply late");
        DateStamp(&t0);
        call(DELETEPORT, 0, 0, 0, 0, 0, 0);
        DateStamp(&t1);
        ticks = ((t1.ds_Days - t0.ds_Days) * 1440 + t1.ds_Minute - t0.ds_Minute) * 3000
                + t1.ds_Tick - t0.ds_Tick;
        OBS("replied before it returned", resp_done > before);
        OBS("returned after 4.5 s", ticks >= 225);
        BOTH("port gone", FindPort((STRPTR)own) == NULL);
        resp_release = 1;
        waitresp(before + 1);
        n = resp_done;
        while ((held = (struct RexxMsg *)GetMsg(testport)) != NULL) {
            for (i = 0; i < 16 && held->rm_Args[i]; i++)
                DeleteArgstring(held->rm_Args[i]);
            FreeVec(held);
            late++;
        }
        OBS("replies at the test's port", late);
    }

    /* 33 messages received before one is taken (#1100): the message list
       has 32 longs, the original adds without a limit: the 32nd message
       overwrites the list's end mark, the 33rd runs into the sent list.
       33 stay inside the block (64 and more are not tried: what lies
       behind the block is not the test's).  After RXReadPort (the first
       message becomes current, the list moves down by one) the
       observations say whether the long at the list's 32nd place and the
       one behind it (the sent list's first) hold a message, and how many
       longs of the block are not 0.  RXDeletePort replies everything to
       the test's port, where the replies are counted and the messages
       freed (the original: 32, the message RXReadPort made current is
       not replied by RXDeletePort, whose loop makes the next one current
       first; fixed #1101: 33).  A fix that keeps all 33 differently
       changes these values (a pair). */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for 33 messages");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    {
        struct RexxMsg *many[33];
        ULONG *list;
        int nz, replies = 0, k;

        for (k = 0; k < 33; k++) {
            many[k] = AllocVec(128, MEMF_PUBLIC | MEMF_CLEAR);
            many[k]->rm_Node.mn_Node.ln_Name = (char *)"RWETEST";
            many[k]->rm_Node.mn_Length = 128;
            many[k]->rm_Node.mn_ReplyPort = testport;
            many[k]->rm_Action = RXCOMM;
            PutMsg(FindPort((STRPTR)own), &many[k]->rm_Node);
        }
        ROUT = "RXMessageList";
        strcpy(VEC, "33 messages");
        call(MESSAGELIST, 0, 0, 0, 0, 0, 0);
        list = (ULONG *)R.d[0];
        readport(READPORT, 1, "33 messages");
        for (nz = 0, k = 0; k < 64; k++)
            if (list[k])
                nz++;
        ROUT = "RXMessageList";
        strcpy(VEC, "33 messages");
        OBS("long 31 of the list", list[31] != 0);
        OBS("long 32, the sent list's first", list[32] != 0);
        OBS("longs of the block not 0", nz);
        ROUT = "RXDeletePort";
        strcpy(VEC, "33 messages");
        call(DELETEPORT, 0, 0, 0, 0, 0, 0);
        while (GetMsg(testport))
            replies++;
        OBS("replies at the test's port", replies);
        for (k = 0; k < 33; k++)
            FreeVec(many[k]);
    }

    /* 40 messages received before one is taken, then an RXSendCommand
       whose reply arrives behind them (#1100): RXSendCommand waits for its
       reply while the 40 are in the library's port.  Observed after it:
       its result, the long at the list's 32nd place, whether the sent
       list is all 0, how many longs of the block are not 0 and how many
       messages are still in the port.  Then each message is read and
       replied (RXReplyMsg 0, to the test's port); observed: how many came
       in the order they were put (the loop stops at the first that does
       not), the replies at the test's port.  The original appends all 40
       while it waits (the 32nd to the 40th run into the sent list; 40
       stay inside the block, as the 33 above), so the list is one run of
       40 and reads in order; a library that leaves the messages it has
       no room for in the port still has to take the reply out of it, and
       takes the rest later, in the same order. */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for 40 messages");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    {
        struct RexxMsg *many[40], *m;
        struct MsgPort *p = (struct MsgPort *)R.d[0];
        struct Node *nd;
        ULONG *list;
        int nz, left, replies = 0, inorder = 0, sentclear = 1, k;

        for (k = 0; k < 40; k++) {
            many[k] = AllocVec(128, MEMF_PUBLIC | MEMF_CLEAR);
            many[k]->rm_Node.mn_Node.ln_Name = (char *)"RWETEST";
            many[k]->rm_Node.mn_Length = 128;
            many[k]->rm_Node.mn_ReplyPort = testport;
            many[k]->rm_Action = RXCOMM;
            PutMsg(p, &many[k]->rm_Node);
        }
        send(1, "RWERESP", "behind 40", 0, "after 40");
        waitresp(n += 1);
        ROUT = "RXMessageList";
        strcpy(VEC, "40 messages");
        call(MESSAGELIST, 0, 0, 0, 0, 0, 0);
        list = (ULONG *)R.d[0];
        for (nz = 0, k = 0; k < 64; k++)
            if (list[k])
                nz++;
        for (k = 32; k < 64; k++)
            if (list[k])
                sentclear = 0;
        left = 0;
        Forbid();
        for (nd = p->mp_MsgList.lh_Head; nd->ln_Succ; nd = nd->ln_Succ)
            left++;
        Permit();
        OBS("long 31 of the list", list[31] != 0);
        OBS("the sent list all 0", sentclear);
        OBS("longs of the block not 0", nz);
        OBS("messages left in the port", left);
        readport(READPORT, 1, "40 messages");
        m = (struct RexxMsg *)R.d[0];
        for (k = 0; k < 40 && m == many[k]; k++) {
            inorder++;
            reply(0, 0, NULL, "40 messages");
            if (k < 39) {
                readport(READPORT, 1, "40 messages");
                m = (struct RexxMsg *)R.d[0];
            }
        }
        ROUT = "RXReadPort";
        strcpy(VEC, "40 messages");
        OBS("in the order put", inorder);
        ROUT = "RXDeletePort";
        strcpy(VEC, "40 messages");
        call(DELETEPORT, 0, 0, 0, 0, 0, 0);
        while (GetMsg(testport))
            replies++;
        OBS("replies at the test's port", replies);
        for (k = 0; k < 40; k++)
            FreeVec(many[k]);
    }

    /* finit with a port */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for finit");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    ROUT = "finit";
    strcpy(VEC, "with a port");
    DO1(ENT(LIB, FINIT, 0), (void)0);
    BOTH("port gone", FindPort((STRPTR)own) == NULL);

    /* init after RXDefaultPort "RWERESP": "REXXE" */
    ROUT = "init";
    strcpy(VEC, "again");
    DO1(ENT(LIB, INIT, 0), (void)0);
    /* with a port, so that a message to the default port can be sent and
       its reply received */
    ROUT = "RXCreatePort";
    strcpy(VEC, "after init");
    DO1(ENT(LIB, CREATEPORT, 0), R.d[0] = (ULONG)own;
        stk[1] = strlen(own); R.a[2] = (ULONG)&stk[2]);
    BOTH("the port", R.d[0] != 0);
    run(0, NULL, "to REXXE");
    /* d0 does not say whether it was sent (#543): the responder does,
       which serves the port REXX here (a REXX port of RexxMast is not
       there in the sessions) */
    Delay(25);
    OBS("sent to the default port", resp_done > n);
    if (resp_done > n) {
        waitresp(n += 1);
        samelog("default port after init", 1);
        readport(WAITPORT, 0, "the default port's reply");
    }
    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), (void)0);

    ROUT = "memlib finit";
    memlib(8);
    Signal(&resp->pr_Task, SIGBREAKF_CTRL_C);
    while (resp_ready)
        Wait(SIGBREAKF_CTRL_E);
    RemPort(testport);
    DeleteMsgPort(testport);
    CloseLibrary(TRexxBase);
    return rwe_done(LIB);
}
