/*
 * utillib - command library 194: low level exec stuff - CreateMsgPort,
 * DeleteMsgPort, Wait, TaskName$.
 *
 * C translation of bblibsrc/utillib (bum8; binary checked: same as the
 * source).
 *   init:   CLR.l signals; the init value is the ADDRESS of `signals`
 *           ("other libs access using #utillib,#la0 then btst sigbit,(a0)").
 *   finit:  CloseRoutine - DeleteMsgPort on every port still in the list.
 *
 * Ports: one block of $2A bytes (memlib globalloc, MEMF_PUBLIC|MEMF_CLEAR):
 *   +0 next, +4 prev (utillib's own port list, head PortListStart),
 *   +8 struct MsgPort ($22 bytes) - the address returned.
 * The name is copied into its own block {size.l, characters, 0}.
 *
 * Kept from the original (BUG(orig)):
 *   - Not a bug: TaskName$ leaves a3 BEHIND the terminating 0 (no
 *     SUBQ #1,a3; no visible effect), d0 = length.
 *   - Description, not a bug (a port name of 64 KB does not occur):
 *     CreateMsgPort returns with a3 = the name argument ("Protect BB2
 *     String area from corruption") and copies (UWORD)len+1 bytes of the
 *     name (DBRA on the length's low word).  The length is -4(a2) of the
 *     caller (a2 = the compiler's argument pointer, see memacclib.c).
 *   - On a failed allocation "Failed to allocate memory for Port" (the
 *     port block stays allocated if the name allocation fails).  After
 *     TRAP #0 the original runs into its message text; not reproduced
 *     (DIFF, K: the C version returns 0, #300).
 *   - Not a bug (the intended semantics, any allocated signal): Wait waits
 *     for ALL allocated signals of the task (tc_SigAlloc).
 *
 * Fixed (intended deviations):
 *   - #297: TaskName$ of a task without a name (ln_Name 0) copied from
 *     address 0 up to the first 0 byte; here it is the empty string.
 *   - #298: CreateMsgPort did not notice a failed AllocSignal (-1): TST.l
 *     d0:BEQ tests for signal 0, which AllocSignal(-1) never returns, and
 *     the port got mp_SigBit $FF; the error path behind it would have
 *     freed the port address (block+8) with $22 bytes.  Here -1 is the
 *     error: the block is freed with its $2A bytes, then the runtime error
 *     "Failed to allocate signal for Port"; if the trap returns,
 *     CreateMsgPort returns 0.
 *   - #299: DeleteMsgPort freed the port with the messages still queued
 *     on it, without replying them (a sender waiting at its reply port
 *     never got the message back).  Here they are replied first.
 *   - #1041: DeleteMsgPort accepted anything: a port that utillib did not
 *     create (not in its list) was unlinked through foreign pointers and
 *     freed as a utillib block, which corrupts memory.  Here an error
 *     check (delportchk, error check area only; utillib gets one for it)
 *     refuses such a port with "Port Not Created By CreateMsgPort";
 *     release code unchanged (an invalid argument).
 *   - #1060: TaskName$ of task 0 (FindTask(0)) and Wait changed a6 and did
 *     not restore it, although the compiler keeps the library base it
 *     loaded in a6 across a statement (lasta6): a call behind TaskName$ in
 *     the same expression went through exec's base.  Here both keep a6
 *     (the compiled C routines do; test: library test cases "a6 kept",
 *     bug-orig test utillib-taskname-a6).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (aliases / thunks, defined below) */
void _CreateMsgPort(void), _DeleteMsgPort(void);
ULONG _dowait(void);
void _taskname(void), _startup(void), utillib_CloseRoutine(void);
void delportchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#utillib,init,#long,finit,0} - `return` = #long = 3; the
   error field: the error check area of delportchk (fixed, #1041) */
BLITZ_LIBHEADER(LIB_utillib, init, BL_LONG, finit, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_CreateMsgPort, 0, 0);
BLITZ_NAME("CreateMsgPort", " *UserPort.port=CreateMsgPort(PortName$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DeleteMsgPort, delportchk, 0);
BLITZ_NAME("DeleteMsgPort", " *UserPort");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_dowait, 0, 0);
BLITZ_NAME("Wait", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_taskname, 0, 0);
BLITZ_NAME("TaskName$", "task");

BLITZ_NULLSUB_AT(init, _startup, 0, 0);
BLITZ_NULLSUB_AT(finit, utillib_CloseRoutine, 0, 0);

BLITZ_LIBFIN();

static ULONG signals;                                   /* Dc.l 0 */

/* PortListStart: Dc.l 0:Dc.l 0 - the head looks like a node */
struct portnode {
    struct portnode *next;                              /* +0 */
    struct portnode *prev;                              /* +4 */
    struct MsgPort port;                                /* +8, $22 bytes */
};
#define PORTBLOCK 0x2a
static_assert(sizeof(struct portnode) == PORTBLOCK);

/* the head: next and prev like a node (its prev is never read) */
static struct {
    struct portnode *next, *prev;
} PortListStart;

static const char NoMem[] = "Failed to allocate memory for Port";
static const char NoSig[] = "Failed to allocate signal for Port";

/**
 * @brief TaskName$(task) - the name of a task.
 *
 * Copies the name to a3 including the 0.
 *
 * Not a bug: a3 is left BEHIND the terminating 0 (no SUBQ #1,a3; no
 * visible effect).  Fixed (#297): a task without a name has the empty
 * name (the original copied from address 0).
 *
 * @param task d0: the task, 0 = this task (FindTask(0))
 * @return d0: the length of the name; a3 behind its 0
 */
static ULONG BLITZ_ROUTINE taskname_c(struct Task *task BL_REG(d0))
{
    const UBYTE *s;
    ULONG n = 0;
    UBYTE c;

    if (!task)
        task = FindTask(NULL);
    s = (const UBYTE *)task->tc_Node.ln_Name;
    if (!s)                                             /* fixed (#297) */
        s = (const UBYTE *)"";
    do {
        n++;
        c = *s++;
        *blitz_string++ = c;
    } while (c);
    return n - 1;                                       /* SUBQ.l #1,d0 */
}
BLITZ_ALIAS(_taskname, taskname_c);

/**
 * @brief init - CLR.l signals:MOVE.l #signals,d0.
 *
 * "other libs access using #utillib,#la0 then btst sigbit,(a0)".
 *
 * @return d0: the address of `signals`, the library's init value
 */
static ULONG BLITZ_ROUTINE startup_c(void)
{
    signals = 0;
    return (ULONG)&signals;
}
BLITZ_THUNK(_startup, startup_c, "d1/a0-a1");

/**
 * @brief Wait - wait for any allocated signal of the task.
 *
 * Wait(FindTask(0)->tc_SigAlloc) - all allocated signals (not a bug: the
 * intended semantics); the result also goes to `signals`.
 *
 * @return d0: the signals received
 */
ULONG BLITZ_ROUTINE _dowait(void)
{
    ULONG got = Wait(FindTask(NULL)->tc_SigAlloc);
    signals = got;
    return got;
}

/* FindListEnd - the last node (or the head) */
static struct portnode *findlistend(void)
{
    struct portnode *next = (struct portnode *)&PortListStart, *node;

    do {
        node = next;
        next = node->next;
    } while (next);
    return node;
}

/**
 * @brief CreateMsgPort(PortName$) - create a message port.
 *
 * Allocates the $2A byte block (memlib globalloc, MEMF_PUBLIC|MEMF_CLEAR),
 * allocates a signal, initialises the port (PA_SIGNAL, this task) and
 * links it into utillib's port list (freed by the finit).  A non-empty
 * name is copied into its own block {size.l, characters, 0} and the port
 * made public (AddPort).  Returns with a3 = the name argument ("Protect
 * BB2 String area from corruption") and copies (UWORD)len+1 bytes of the
 * name (DBRA on the length's low word; not a bug: a port name of 64 KB
 * does not occur).
 *
 * Errors: "Failed to allocate memory for Port" (runtime error; the port
 * block stays allocated if the name allocation fails).  DIFF (K, #300):
 * after a returning TRAP #0 the original runs into its message text; the
 * C version returns 0.
 *
 * Fixed (#298): a failed AllocSignal (-1) frees the block ($2A bytes) and
 * gives the runtime error "Failed to allocate signal for Port" (the
 * original tested for signal 0 and made a port with mp_SigBit $FF; its
 * error path would have freed the port address with $22 bytes).
 *
 * @param name d0: the port name ("" = a private port)
 * @param lens a2: the compiler's argument pointer: -4(a2) = the name's
 *             length
 * @return d0: the MsgPort (block + 8), 0 after an error (DIFF)
 */
static ULONG BLITZ_ROUTINE createmsgport_c(const UBYTE *name BL_REG(d0),
                                           const ULONG *lens BL_REG(a2))
{
    struct portnode *node, *end;
    struct MsgPort *port;
    struct bl_regs r;

    blitz_string = (UBYTE *)name;                       /* MOVE.l d0,a3 */
    r = (struct bl_regs){ .d0 = PORTBLOCK, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
    node = (struct portnode *)BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!node) {                                        /* Error_NoMemory */
        BLITZ_RUNERR(NoMem);
        /* DIFF (K): after a returning TRAP #0 the original runs into its
           message text; here CreateMsgPort returns 0 */
        return 0;
    }
    port = &node->port;                                 /* ADDA.l #8,a5 */
    {
        BYTE sig = AllocSignal(-1);
        if (sig == -1) {                                /* fixed (#298): -1, not 0 */
            /* error_NoSig: fixed (#298): the block and its $2A bytes */
            r = (struct bl_regs){ .d0 = PORTBLOCK, .a1 = (ULONG)node };
            BLITZ_ALIBJSR(TOK_globfree, r);
            BLITZ_RUNERR(NoSig);
            /* DIFF (K): the original would fall into Error_NoMemory (pops
               a0 and the registers a second time, TRAP #0 NoMem, then the
               message text); here it returns 0 */
            return 0;
        }
        port->mp_SigBit = (UBYTE)sig;                   /* MOVE.b d0,15(a5) */
    }
    port->mp_SigTask = FindTask(NULL);
    port->mp_Flags = PA_SIGNAL;
    port->mp_Node.ln_Pri = 0;
    port->mp_Node.ln_Type = NT_MSGPORT;
    {                                                   /* NewList */
        struct List *l = &port->mp_MsgList;
        l->lh_TailPred = (struct Node *)l;
        l->lh_Tail = NULL;
        l->lh_Head = (struct Node *)&l->lh_Tail;
    }
    if (*name) {                                        /* TST.b (a3) */
        ULONG len = lens[-1];                           /* MOVE.l -4(a4),d0 */
        ULONG size = len + 5;
        UBYTE *copy, *d;
        const UBYTE *s = name;
        UWORD n;

        r = (struct bl_regs){ .d0 = size, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
        copy = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
        if (!copy) {                                    /* Error_NoMemory1 */
            BLITZ_RUNERR(NoMem);
            /* DIFF (K): as above, returns 0 instead of running into the
               message text */
            return 0;
        }
        copy += 4;
        ((ULONG *)copy)[-1] = size;
        d = copy;
        n = (UWORD)len;                                 /* DBRA d0 */
        do
            *d++ = *s++;
        while (n-- != 0);
        port->mp_Node.ln_Name = (char *)copy;
        AddPort(port);
    }
    end = findlistend();                                /* Leave */
    end->next = node;
    node->prev = end;
    return (ULONG)port;
}
BLITZ_ALIAS(_CreateMsgPort, createmsgport_c);

/**
 * @brief DeleteMsgPort *UserPort - delete a port made by CreateMsgPort.
 *
 * RemPort and frees the name if there is one, replies the messages still
 * queued, frees the signal, unlinks the port from utillib's list and frees
 * its block.  Keeps all registers.
 *
 * Fixed (#299): the queued messages are replied (the original freed the
 * port with them); replies that came back to the port (NT_REPLYMSG) are
 * only removed.  Errors (debugger only, delportchk): "Port Not Created By
 * CreateMsgPort" (fixed, #1041: the original took any address; without
 * the debugger a port that utillib did not create still corrupts
 * memory, an invalid argument).
 *
 * @param port d0: the port
 */
static void BLITZ_ROUTINE deletemsgport_c(struct MsgPort *port BL_REG(d0))
{
    struct portnode *node, *prev, *next;
    struct bl_regs r;

    if (port->mp_Node.ln_Name) {
        ULONG *h;
        RemPort(port);
        h = (ULONG *)port->mp_Node.ln_Name - 1;         /* SUBA.l #4,a1 */
        r = (struct bl_regs){ .d0 = *h, .a1 = (ULONG)h };
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    {                                                   /* fixed (#299) */
        struct Message *m;

        /* only messages sent to the port: a reply that came back to it
           (NT_REPLYMSG; the port was the reply port) is not replied again,
           which would also put it back on this port */
        while ((m = GetMsg(port)) != NULL)
            if (m->mn_Node.ln_Type == NT_MESSAGE)
                ReplyMsg(m);
    }
    /* FreeSignal: MOVEQ #0,d0:MOVE.b 15(a5),d0 - zero extended */
    BL_FREESIGNAL(port->mp_SigBit);
    node = (struct portnode *)((UBYTE *)port - 8);
    prev = node->prev;                                  /* unlink */
    prev->next = node->next;
    next = node->next;
    if (next)
        next->prev = prev;
    r = (struct bl_regs){ .d0 = PORTBLOCK, .a1 = (ULONG)node };
    BLITZ_ALIBJSR(TOK_globfree, r);
}
BLITZ_ALIAS(_DeleteMsgPort, deletemsgport_c);

/* CloseRoutine - remove and free all ports that are still there */
static void BLITZ_ROUTINE closeroutine_c(void)
{
    struct portnode *n;

    while ((n = PortListStart.next) != NULL)
        deletemsgport_c(&n->port);
}
BLITZ_THUNK(utillib_CloseRoutine, closeroutine_c, "d1/a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(NotOurs, "Port Not Created By CreateMsgPort");

/**
 * @brief The error check of DeleteMsgPort (debugger only).
 *
 * Fixed (#1041), not in the original (DeleteMsgPort took any address):
 * the port must be one of utillib's list, made by CreateMsgPort and not
 * deleted yet.
 *
 * Errors: "Port Not Created By CreateMsgPort".
 *
 * @param port d0: the port
 */
static void BLITZ_ERRCHK delportchk_c(const struct MsgPort *port BL_REG(d0))
{
    const struct portnode *n;

    for (n = PortListStart.next; n; n = n->next)
        if (&n->port == port)
            return;
    BLITZ_RUNERR(NotOurs);
}
BLITZ_ERRCHK_THUNK(delportchk, delportchk_c, "d0-d1/a0-a1");
