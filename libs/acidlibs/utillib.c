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
 *   - TaskName$ leaves a3 BEHIND the terminating 0 (no SUBQ #1,a3; not a
 *     bug: no visible effect), d0 = length; a task without a name reads
 *     from address 0. (#297)
 *   - Description, not a bug (a port name of 64 KB does not occur):
 *     CreateMsgPort returns with a3 = the name argument ("Protect BB2
 *     String area from corruption") and copies (UWORD)len+1 bytes of the
 *     name (DBRA on the length's low word).  The length is -4(a2) of the
 *     caller (a2 = the compiler's argument pointer, see memacclib.c).
 *   - a failed AllocSignal (-1) is not noticed (TST.l d0:BEQ tests for
 *     signal 0, which AllocSignal(-1) never returns): mp_SigBit = $FF
 *     (#298).  The error path behind it would give globfree the port
 *     address (block+8) and $22 instead of the block and $2A, then runtime
 *     error "Failed to allocate signal for Port" (not a bug: never reached,
 *     and the program ends there).  On a failed allocation "Failed to
 *     allocate memory for Port" (the port block stays allocated if the
 *     name allocation fails).  After TRAP #0 the original runs into its
 *     message text; not reproduced (DIFF, K: the C version returns 0,
 *     #300).
 *   - DeleteMsgPort accepts anything: pending messages are not replied,
 *     a port that utillib did not create corrupts memory. (#299)
 *   - Not a bug (the intended semantics, any allocated signal): Wait waits
 *     for ALL allocated signals of the task (tc_SigAlloc).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, defined below) */
void _CreateMsgPort(void), _DeleteMsgPort(void);
ULONG _dowait(void);
void _taskname(void), _startup(void), CloseRoutine(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#utillib,init,#long,finit,0} - `return` = #long = 3 */
BLITZ_LIBHEADER(LIB_utillib, init, BL_LONG, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_CreateMsgPort, 0, 0);
BLITZ_NAME("CreateMsgPort", " *UserPort.port=CreateMsgPort(PortName$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DeleteMsgPort, 0, 0);
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
BLITZ_NULLSUB_AT(finit, CloseRoutine, 0, 0);

BLITZ_LIBFIN();

static ULONG signals;                                   /* Dc.l 0 */

/* PortListStart: Dc.l 0:Dc.l 0 - the head looks like a node */
struct portnode {
    struct portnode *next;                              /* +0 */
    struct portnode *prev;                              /* +4 */
    struct MsgPort port;                                /* +8, $22 bytes */
};
#define PORTBLOCK 0x2a
static struct portnode *PortListStart[2];

static const char NoMem[] = "Failed to allocate memory for Port";
static const char NoSig[] = "Failed to allocate signal for Port";

/* _taskname - d0 = task (0 = FindTask(0)); copy its name to a3 including
   the 0, a3 stays behind it */
static ULONG BLITZ_ROUTINE taskname_c(struct Task *task BL_REG(d0))
{
    const UBYTE *s;
    ULONG n = 0;
    UBYTE c;

    if (!task)
        task = FindTask(NULL);
    s = (const UBYTE *)task->tc_Node.ln_Name;
    do {
        n++;
        c = *s++;
        *blitz_string++ = c;
    } while (c);
    return n - 1;                                       /* SUBQ.l #1,d0 */
}
BLITZ_THUNK(_taskname, taskname_c, "a1");

/* _startup - CLR.l signals:MOVE.l #signals,d0 */
static ULONG BLITZ_ROUTINE startup_c(void)
{
    signals = 0;
    return (ULONG)&signals;
}
BLITZ_THUNK(_startup, startup_c, "d1/a0-a1");

/* _dowait - Wait(FindTask(0)->tc_SigAlloc), result also in `signals` */
ULONG BLITZ_ROUTINE _dowait(void)
{
    ULONG got = Wait(FindTask(NULL)->tc_SigAlloc);
    signals = got;
    return got;
}

/* FindListEnd - the last node (or the head) */
static struct portnode *findlistend(void)
{
    struct portnode *next = (struct portnode *)PortListStart, *node;

    do {
        node = next;
        next = node->next;
    } while (next);
    return node;
}

/* _CreateMsgPort - d0 = name, -4(a2) = its length */
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
        if (sig == 0) {                                 /* BUG(orig) (#298): TST.l d0 - never */
            /* error_NoSig: frees the port address, $22 bytes */
            r = (struct bl_regs){ .d0 = 0x22, .a1 = (ULONG)port };
            BLITZ_ALIBJSR(TOK_globfree, r);
            BLITZ_RUNERR(NoSig);
            /* DIFF (K): the original falls into Error_NoMemory (pops a0
               and the registers a second time, TRAP #0 NoMem, then the
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
BLITZ_THUNK(_CreateMsgPort, createmsgport_c, "d1/a1");

/* _DeleteMsgPort - d0 = port; keeps all registers */
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
BLITZ_THUNK(_DeleteMsgPort, deletemsgport_c, "d0-d1/a0-a1");

/* CloseRoutine - remove and free all ports that are still there */
static void BLITZ_ROUTINE closeroutine_c(void)
{
    struct portnode *n;

    while ((n = PortListStart[0]) != NULL)
        deletemsgport_c(&n->port);
}
BLITZ_THUNK(CloseRoutine, closeroutine_c, "d1/a1");
