/*
 * ctxwrite.h - text written to the program's output from the context of
 * a task that is inside a patched system function (the requester patches
 * of reqtrap, the alert patches of inject).  The caller of such a patch
 * must not make a DOS call itself:
 *
 *   - A task started the Workbench way (wbrun) whose WBStartup message
 *     nobody took has that message first in its process port, and every
 *     DOS packet wait of the process takes it for a late reply.  DOS then
 *     raises the "Software Failure" requester (error 87000004) - from
 *     inside the packet wait of the process, that is from inside the
 *     patched function.  A Write there sends a packet, its wait gets the
 *     reply of the previous packet (one behind, every time), DOS raises
 *     the requester again, the patch is entered again: every level costs
 *     about 360 bytes of stack, after about 120 the CPU halts.
 *   - A patch called under Forbid (exec's Alert) must not wait for a
 *     packet at all.
 *
 * The text is therefore collected in memory (struct cw_msg on the
 * caller's stack) and written by a process of this program, the writer:
 * the caller sends the message to the writer's port and waits for the
 * reply with exec calls only (its own signal bit, no DOS call).  The
 * caller is blocked until the text is written, so the order of the lines
 * against the output of the program is the same as with a Write in the
 * caller.  The writer has priority 1 (ahead of a program at priority 0
 * that goes on after the call).
 *
 * A task that is already inside a call of the patch (a re-entry) and the
 * writer itself record nothing (the answer is given at once); the number
 * of these is counted, cw_stop() reports it.  Without the writer
 * (Kickstart before 2.0 has no CreateNewProc: reqtrap there runs the
 * command in its own process and no WBStartup message exists) the text is
 * written by the caller as before.
 *
 * The program includes <exec/...>, <dos/dostags.h>, <proto/exec.h>,
 * <proto/dos.h> and <string.h> first.
 */
#ifndef CTXWRITE_H
#define CTXWRITE_H

#define CW_MAX 700              /* the text of one message; longer ends in "..." */
#define CW_TASKS 8              /* tasks inside a call at the same time */
#define CW_NAME "ctxwrite writer"

struct cw_msg {
    struct Message m;
    ULONG len;                  /* -1: stop the writer */
    char text[CW_MAX];
};

static struct MsgPort *volatile cw_wport;   /* the writer's port, 0 without writer */
static struct Task *volatile cw_wtask;
static struct Task *cw_in[CW_TASKS];        /* the tasks inside a call (Forbid) */
static volatile ULONG cw_skipped;           /* calls that recorded nothing */
static BPTR cw_out;
#ifdef CW_DIRECT
#define CW_DEPTH_CAP 2
static volatile LONG cw_depth;
#endif

static void cw_writer(void)
{
    struct MsgPort *mp = CreateMsgPort();
    struct cw_msg *m;
    int quit = 0;

    if (!mp) return;
    cw_wtask = FindTask(0);
    cw_wport = mp;
    while (!quit) {
        WaitPort(mp);
        while ((m = (struct cw_msg *)GetMsg(mp)) != 0) {
            if (m->len == (ULONG)-1)
                quit = 1;
            else if (m->len)
                Write(cw_out, m->text, m->len);
            ReplyMsg(&m->m);
        }
    }
    cw_wport = 0;
    DeleteMsgPort(mp);
}

/* start the writer for OUT (0: no writer, the callers write themselves) */
static int cw_start(BPTR out, int withwriter)
{
    int i;

    cw_out = out;
#ifdef CW_DIRECT
    withwriter = 0;
#endif
    if (!withwriter) return 1;
    if (!CreateNewProcTags(NP_Entry, (ULONG)cw_writer, NP_Name, (ULONG)CW_NAME,
                           NP_StackSize, 4096, NP_Priority, 1, TAG_END))
        return 0;
    for (i = 0; i < 100 && !cw_wport; i++) Delay(1);
    return cw_wport != 0;
}

/* a call of a patch begins: 1 = record the text, 0 = record nothing */
static int cw_begin(struct cw_msg *m)
{
    struct Task *me = FindTask(0);
    int i, dup = 0, fr = -1;

    m->len = 0;
#ifdef CW_DIRECT
    /* the broken twin of the tests: no writer, no guard, the caller writes
       (the old reqtrap); the depth is capped so that the stack lasts */
    return ++cw_depth <= CW_DEPTH_CAP ? 1 : (cw_depth--, cw_skipped++, 0);
#endif
    if (!cw_wport) return 1;    /* no writer: the caller writes (cw_end) */
    Forbid();
    if (me != cw_wtask) {
        for (i = 0; i < CW_TASKS; i++) {
            if (cw_in[i] == me) dup = 1;
            else if (!cw_in[i] && fr < 0) fr = i;
        }
        if (!dup && fr >= 0) cw_in[fr] = me;
        else fr = -1;
    }
    Permit();
    /* no entry: a re-entry, the writer itself, or too many tasks */
    if (fr < 0) { cw_skipped++; return 0; }
    return 1;
}

static void cw_add(struct cw_msg *m, const char *s)
{
    if (!s) s = "(null)";
    for (; *s && m->len < CW_MAX - 4; s++) m->text[m->len++] = *s;
    if (*s) {                   /* cut */
        m->text[m->len++] = '.'; m->text[m->len++] = '.'; m->text[m->len++] = '.';
    }
}

/* the text is complete: write it (the caller waits until it is written) */
static void cw_end(struct cw_msg *m)
{
    struct Task *me = FindTask(0);
    struct MsgPort rp;
    BYTE sig;
    int own;
    int i;

    if (!cw_wport) {
        if (m->len) Write(cw_out, m->text, m->len);
#ifdef CW_DIRECT
        cw_depth--;
#endif
        return;
    }
    /* a signal of its own; if the task has none left (inject's "signals"
       takes all) the one exec keeps for its own short waits, SIGB_SINGLE */
    sig = AllocSignal(-1);
    own = sig >= 0;
    if (!own) {
        sig = SIGB_SINGLE;
        SetSignal(0, SIGF_SINGLE);
    }
    memset(&rp, 0, sizeof rp);
    rp.mp_Node.ln_Type = NT_MSGPORT;
    rp.mp_Flags = PA_SIGNAL;
    rp.mp_SigBit = sig;
    rp.mp_SigTask = me;
    rp.mp_MsgList.lh_Head = (struct Node *)&rp.mp_MsgList.lh_Tail;   /* NewList */
    rp.mp_MsgList.lh_TailPred = (struct Node *)&rp.mp_MsgList.lh_Head;
    rp.mp_MsgList.lh_Type = NT_MESSAGE;
    m->m.mn_Node.ln_Type = NT_MESSAGE;
    m->m.mn_ReplyPort = &rp;
    m->m.mn_Length = sizeof *m;
    PutMsg(cw_wport, &m->m);
    WaitPort(&rp);
    GetMsg(&rp);
    if (own) FreeSignal(sig);
    Forbid();
    for (i = 0; i < CW_TASKS; i++)
        if (cw_in[i] == me) cw_in[i] = 0;
    Permit();
}

/* stop the writer (after the last patch is removed): returns when its
   process has gone, so that the program's code can be freed */
static void cw_stop(void)
{
    struct cw_msg m;
    struct MsgPort *rp;
    int i;

    if (!cw_wport) return;
    rp = CreateMsgPort();
    if (!rp) return;
    memset(&m, 0, sizeof m);
    m.len = (ULONG)-1;
    m.m.mn_Node.ln_Type = NT_MESSAGE;
    m.m.mn_ReplyPort = rp;
    m.m.mn_Length = sizeof m;
    PutMsg(cw_wport, &m.m);
    WaitPort(rp);
    GetMsg(rp);
    DeleteMsgPort(rp);
    for (i = 0; i < 200 && FindTask((STRPTR)CW_NAME); i++) Delay(1);
}
#endif
