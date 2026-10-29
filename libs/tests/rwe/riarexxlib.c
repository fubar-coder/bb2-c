/*
 * rwe riarexxlib - the C build of riarexxlib (userlib 133) against the
 * reference build of its source (ref/riarexxlib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), method of rwecmp.h.
 *
 *   riarexxlib DIR     (DIR with riarexxlib, maxslib, memlib .obj/.orig)
 *
 * No RexxMast is needed: the messages go to the library's own port, to a
 * port of the test (RWETEST: messages the test sends, the replies come
 * back there) and to a responder process (port RWERESP, and REXX if no
 * such port exists): it logs every message (type, priority, length,
 * action, ln_Name, reply port, the argument strings) and replies with a
 * result code and string the test sets, so that RXSendCommand, which
 * waits, gets its reply.  The two sets have their own ports
 * (RWERX_O/RWERX_C), their own memlib/maxslib and string areas.
 * Compared: d0, a3 and the strings of the string functions, the messages
 * the responder got (by set, from ln_Name), the replies the test gets
 * (Result1, the Result2 string), the kept registers.
 * Covered: every entry; functions without a port and without a current
 * message (0); the error check portchk with a port; RXCreatePort twice;
 * the tokenizer (spaces, quotes, doubled quotes, the character after a
 * closing quote skipped: BUG(orig), RXQuoteChar), RXTokenise off and
 * BUG(orig) "off" with a word whose upper word is set; RXGetArg 0..17,
 * RXNextArg to the end, RXNumArgs, RXSendersName, RXReplyMsg both forms
 * to our own port, to the test (the Result2 string) and to a sender
 * whose port is gone (BUG(orig): no reply); replies to our messages
 * (freed); RXRunCommand both forms, to a missing port; RXSendCommand
 * both forms (the result over the arguments), with result code 10 and no
 * string, to a missing port, with a message arriving while it waits;
 * RXReturnCode (BUG(orig): 0 without a current message); RXDefaultPort;
 * RXExecuteScript/RXRunScript (BUG(orig): both RXCOMM|RXFF_STRING; to
 * the responder if there is no REXX port, else to RexxMast with the
 * string program "exit", its reply awaited with RXWaitPort); RXDeletePort with messages queued (replied with
 * 5); finit with a port; init after RXDefaultPort (BUG(orig): the byte
 * at +4 stays: "REXXE").
 * Not covered: the error check without a port (a runtime error, TRAP
 * #0), a failing AllocVec or AddPort (the exec flags/d0 the original
 * tests), more than 16 or 32 messages/arguments (overflows), messages of
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

static const char *own[2] = { "RWERX_O", "RWERX_C" };
static UBYTE sbuf[2][1024];             /* string area (a3) */
static ULONG stk[2][4];                 /* the compiler's a2 stack */
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
static struct logent rlog[2][24];
static volatile int nrlog[2], resp_done;
static volatile LONG resp_rc;
static const char *volatile resp_res;
static struct Task *maintask;
static volatile int resp_ready;

static void handle(struct RexxMsg *m, int port)
{
    const char *n = m->rm_Node.mn_Node.ln_Name;
    int s = n && n[6] == 'C', i;
    struct logent *e;

    if (nrlog[s] < 24) {
        e = &rlog[s][nrlog[s]];
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
        nrlog[s]++;
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

/* the last message the responder got, the same in both sets */
static void samelog(const char *what, int port)
{
    char w[80];
    int k;

    BOTH("responder got it", nrlog[cur] > 0);
    if (!nrlog[0] || !nrlog[1])
        return;
    {
        const struct logent *e[2] = { &rlog[0][nrlog[0] - 1], &rlog[1][nrlog[1] - 1] };
        sprintf(w, "%s: message", what);
        SAME(w, nrlog[cur]);
        SAME("type", e[cur]->type);
        SAME("pri", e[cur]->pri);
        SAME("length", e[cur]->len);
        SAME("action", e[cur]->action);
        SAME("port", e[cur]->port);
        BOTH("port served", e[cur]->port == port);
        SAME("args", e[cur]->nargs);
        BOTH("ln_Name", !strcmp(e[cur]->name, own[cur]));
        BOTH("reply port", e[cur]->replyport == (ULONG)FindPort((STRPTR)own[cur]));
        for (k = 0; k < 16; k++) {
            const UBYTE *a[2] = { (UBYTE *)e[0]->args[k], (UBYTE *)e[1]->args[k] };
            SAMEBYTES("argument", a, 48);
        }
    }
}

/* ------------------------------------------------------------ helpers */

/* a call; a3 = the string area at `off`, the string lengths `l1`, `l2`
   on the a2 stack */
static void call(int idx, int form, ULONG d0, ULONG d1, ULONG l1, ULONG l2, ULONG off)
{
    DO2(ENT(LIB, idx, form), R[cur].d[0] = d0; R[cur].d[1] = d1;
        stk[cur][0] = l2; stk[cur][1] = l1; R[cur].a[2] = (ULONG)&stk[cur][2];
        R[cur].a[3] = (ULONG)sbuf[cur] + off);
}

/* the string result: d0, a3, the bytes */
static void samestr(void)
{
    const UBYTE *s[2] = { sbuf[0], sbuf[1] };
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)sbuf[cur]);
    SAMEBYTES("string area", s, sizeof sbuf[0]);
}

static void clearstr(void)
{
    memset(sbuf, 0x5a, sizeof sbuf);
}

static void longfn(int idx, const char *name)
{
    ROUT = name;
    call(idx, 0, MARK(0), MARK(1), 0, 0, 0);
    SAME("d0", R[cur].d[0]);
}

/* RXReadPort / RXWaitPort: a message current (`expect`) or 0 */
static void readport(int idx, int expect, const char *what)
{
    ROUT = idx == READPORT ? "RXReadPort" : "RXWaitPort";
    strcpy(VEC, what);
    call(idx, 0, MARK(0), MARK(1), 0, 0, 0);
    SAME("message", R[cur].d[0] != 0);
    BOTH("message as expected", (R[cur].d[0] != 0) == expect);
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
        if (!R[0].d[0])
            break;
    }
    strcpy(VEC, what);
    clearstr();
    ROUT = "RXSendersName";
    call(SENDERSNAME, 0, 0, 0, 0, 0, 16);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)sbuf[cur]);
    BOTH("sender", R[cur].d[0] == 0 || !strcmp((char *)sbuf[cur] + 16, own[cur])
                   || !strncmp((char *)sbuf[cur] + 16, "RWE", 3));
    longfn(RETURNCODE, "RXReturnCode");
}

/* RXRunCommand to the own port (port = NULL) or `port`; form 0: the
   default port */
static void run(int form, const char *port, const char *cmd)
{
    ROUT = "RXRunCommand";
    sprintf(VEC, "%s \"%s\"", port ? port : (form ? "own" : "default"), cmd);
    if (form)
        DO2(ENT(LIB, RUNCOMMAND, 1), R[cur].d[0] = (ULONG)(port ? port : own[cur]);
            R[cur].d[1] = (ULONG)cmd; R[cur].a[3] = (ULONG)sbuf[cur]);
    else
        DO2(ENT(LIB, RUNCOMMAND, 0), R[cur].d[0] = (ULONG)cmd; R[cur].a[3] = (ULONG)sbuf[cur]);
    /* d0 is PutMsg's (BUG(orig)); 0 if the port is missing */
    SAME("sent", R[cur].d[0] != 0 || port == NULL || FindPort((STRPTR)port) != NULL);
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
    for (cur = 0; cur < 2; cur++) {
        strcpy((char *)sbuf[cur], port);
        strcpy((char *)sbuf[cur] + l1 + 1, cmd);
    }
    if (form)
        DO2(ENT(LIB, SENDCOMMAND, 1), R[cur].d[0] = (ULONG)sbuf[cur];
            R[cur].d[1] = (ULONG)sbuf[cur] + l1 + 1; stk[cur][0] = l1; stk[cur][1] = l2;
            R[cur].a[2] = (ULONG)&stk[cur][2]; R[cur].a[3] = (ULONG)sbuf[cur] + l1 + l2 + 2);
    else
        DO2(ENT(LIB, SENDCOMMAND, 0), R[cur].d[0] = (ULONG)sbuf[cur] + l1 + 1;
            R[cur].a[3] = (ULONG)sbuf[cur] + l1 + l2 + 2);
    samestr();
}

static void reply(int form, ULONG code, const char *res, const char *what)
{
    ROUT = "RXReplyMsg";
    sprintf(VEC, "%s %lu %s", what, code, res ? res : "-");
    call(REPLYMSG, form, code, (ULONG)res, res ? strlen(res) : 0, 0, 0);
}

static void strarg(int idx, const char *name, const char *s)
{
    ROUT = name;
    sprintf(VEC, "\"%s\"", s);
    call(idx, 0, (ULONG)s, MARK(1), strlen(s), 0, 0);
}

/* a message of the test to the library's port, ln_Name = `from` */
static struct RexxMsg *tmsg[2][4];
static void testmsg(int n, const char *from, const char *a1, const char *a2)
{
    for (cur = 0; cur < 2; cur++) {
        struct RexxMsg *m = AllocVec(128, MEMF_PUBLIC | MEMF_CLEAR);
        m->rm_Node.mn_Node.ln_Name = (char *)from;
        m->rm_Node.mn_Length = 128;
        m->rm_Node.mn_ReplyPort = testport;
        m->rm_Action = RXCOMM;
        m->rm_Args[0] = CreateArgstring((CONST_STRPTR)a1, strlen(a1));
        if (a2)
            m->rm_Args[1] = CreateArgstring((CONST_STRPTR)a2, strlen(a2));
        tmsg[cur][n] = m;
        PutMsg(FindPort((STRPTR)own[cur]), &m->rm_Node);
    }
}

/* the replies at the test's port, per set; free the messages */
static void testreplies(int n, LONG rc, const char *res)
{
    struct RexxMsg *m;
    int got[2] = { 0, 0 }, i;

    while ((m = (struct RexxMsg *)GetMsg(testport)) != NULL) {
        int s = -1;
        for (i = 0; i < 4; i++) {
            if (m == tmsg[0][i])
                s = 0;
            if (m == tmsg[1][i])
                s = 1;
        }
        if (s < 0)
            continue;
        got[s]++;
        snprintf(VEC, sizeof VEC, "%s: reply Result1 %ld", sets[s].what, m->rm_Result1);
        check(VEC, m->rm_Result1 == rc);
        if (res) {
            check("reply Result2", m->rm_Result2 && !strcmp((char *)m->rm_Result2, res));
            if (m->rm_Result2)
                DeleteArgstring((UBYTE *)m->rm_Result2);
        } else
            check("reply without Result2", !m->rm_Result2);
        for (i = 0; i < 2; i++)
            if (m->rm_Args[i])
                DeleteArgstring(m->rm_Args[i]);
        FreeVec(m);
    }
    BOTH("replies", got[cur] == n);
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
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
    if (!load_sets(argv[1], names))
        return 20;
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
    DO2(ENT(LIB, INIT, 0), (void)0);

    /* no port, no message */
    strcpy(VEC, "no port");
    longfn(MESSAGELIST, "RXMessageList");
    current("no port");

    /* the port */
    for (i = 0; i < 2; i++) {
        ROUT = "RXCreatePort";
        strcpy(VEC, "");
        DO2(ENT(LIB, CREATEPORT, 0), R[cur].d[0] = (ULONG)own[cur];
            stk[cur][1] = strlen(own[cur]); R[cur].a[2] = (ULONG)&stk[cur][2]);
        if (i == 0)
            BOTH("the port", R[cur].d[0] && R[cur].d[0] == (ULONG)FindPort((STRPTR)own[cur]));
        else
            SAME("second port: 0", R[cur].d[0]);
    }
    ROUT = "portchk";
    strcpy(VEC, "with a port");
    DO2(ERR(LIB, READPORT, 0), (void)0);
    ROUT = "RXMessageList";
    call(MESSAGELIST, 0, 0, 0, 0, 0, 0);
    BOTH("list", R[cur].d[0] != 0);

    /* the tokenizer, own port */
    run(1, NULL, "load  \"a b\" c\"\"d \"e\"\"f\" \"g\"h  i");
    readport(READPORT, 1, "own message");
    current("tokenised");
    reply(1, 0, "the result", "own");
    readport(READPORT, 0, "the reply");
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
        /* the default port = the own port of each set */
        ROUT = "RXDefaultPort";
        strcpy(VEC, "own");
        DO2(ENT(LIB, DEFAULTPORT, 0), R[cur].d[0] = (ULONG)own[cur]);
    }
    run(0, NULL, "hello default");
    readport(WAITPORT, 1, "RXWaitPort");
    current("default");
    reply(0, 0, NULL, "default");
    readport(WAITPORT, 0, "RXWaitPort: the reply");     /* waits for it */

    /* the responder */
    n = 0;
    run(1, "RWERESP", "cmd a b");
    waitresp(n += 2);
    samelog("RXRunCommand", 0);
    readport(READPORT, 0, "responder's reply");
    run(1, "RWENOWHERE", "cmd");
    send(1, "RWERESP", "get \"the\" thing", 0, "the result");
    n += 2;
    samelog("RXSendCommand", 0);
    longfn(RETURNCODE, "RXReturnCode");         /* no current message: 0 */
    run(1, NULL, "pending");
    readport(READPORT, 1, "current");
    send(1, "RWERESP", "fail", 10, NULL);
    n += 2;
    longfn(RETURNCODE, "RXReturnCode");
    run(1, NULL, "arrives while waiting");
    send(1, "RWERESP", "again", 0, "x");
    n += 2;
    readport(READPORT, 1, "arrived while waiting");
    current("arrived while waiting");
    longfn(RETURNCODE, "RXReturnCode");
    send(1, "RWENOWHERE", "cmd", 0, "x");
    strarg(DEFAULTPORT, "RXDefaultPort", "RWERESP");
    send(0, "", "default send", 0, "from the default port");
    n += 2;
    samelog("RXSendCommand default", 0);
    send(0, "", "empty result", 0, "");
    n += 2;
    if (have_rexx) {
        ROUT = "RXExecuteScript";
        strcpy(VEC, "");
        call(EXECUTESCRIPT, 0, (ULONG)"RAM:script.rexx", 0, 0, 0, 0);
        waitresp(n += 2);
        samelog("RXExecuteScript", 1);
        BOTH("RXCOMM|RXFF_STRING", rlog[cur][nrlog[cur] - 1].action == (RXCOMM | RXFF_STRING));
        ROUT = "RXRunScript";
        call(RUNSCRIPT, 0, (ULONG)"RAM:script.rexx", 0, 0, 0, 0);
        waitresp(n += 2);
        samelog("RXRunScript", 1);
        readport(READPORT, 0, "the script replies");
    } else {
        /* the real RexxMast: the "script name" is run as a string
           program (BUG(orig)); tokenised, its first word "exit" */
        ROUT = "RXExecuteScript";
        strcpy(VEC, "RexxMast");
        call(EXECUTESCRIPT, 0, (ULONG)"exit 0", 0, 0, 0, 0);
        readport(WAITPORT, 0, "RexxMast's reply");
        ROUT = "RXRunScript";
        call(RUNSCRIPT, 0, (ULONG)"exit 0", 0, 0, 0, 0);
        readport(WAITPORT, 0, "RexxMast's reply");
    }

    /* messages of the test */
    testmsg(0, "RWETEST", "from", "the test");
    readport(READPORT, 1, "test message");
    current("test message");
    reply(1, 0, "answer", "to the test");
    testreplies(1, 0, "answer");
    testmsg(1, "RWEGONE", "gone", NULL);
    readport(READPORT, 1, "sender gone");
    reply(1, 0, "lost", "sender gone");
    testreplies(0, 0, NULL);
    for (cur = 0; cur < 2; cur++)       /* not replied: ours again */
        FreeVec(tmsg[cur][1]);
    tmsg[0][1] = tmsg[1][1] = NULL;
    testmsg(2, "RWETEST", "one", NULL);
    testmsg(3, "RWETEST", "two", NULL);
    ROUT = "RXDeletePort";
    strcpy(VEC, "2 queued");
    call(DELETEPORT, 0, 0, 0, 0, 0, 0);
    testreplies(2, 5, NULL);
    BOTH("port gone", FindPort((STRPTR)own[cur]) == NULL);
    ROUT = "RXDeletePort";
    strcpy(VEC, "no port");
    call(DELETEPORT, 0, 0, 0, 0, 0, 0);

    /* finit with a port */
    ROUT = "RXCreatePort";
    strcpy(VEC, "for finit");
    DO2(ENT(LIB, CREATEPORT, 0), R[cur].d[0] = (ULONG)own[cur];
        stk[cur][1] = strlen(own[cur]); R[cur].a[2] = (ULONG)&stk[cur][2]);
    BOTH("the port", R[cur].d[0] != 0);
    ROUT = "finit";
    strcpy(VEC, "with a port");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    BOTH("port gone", FindPort((STRPTR)own[cur]) == NULL);

    /* init after RXDefaultPort "RWERESP": "REXXE" */
    ROUT = "init";
    strcpy(VEC, "again");
    DO2(ENT(LIB, INIT, 0), (void)0);
    run(0, NULL, "to REXXE");
    SAME("REXXE missing: 0", R[cur].d[0]);
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);

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
