/*
 * reqtraptest - proves that reqtrap (and reqclose) cope with a requester
 * whose handling would re-enter: run it through the launcher,
 *
 *   reqtrap reqtraptest
 *
 * A helper process has an unreplied message first in its process port,
 * like a program started from the Workbench that did not take its
 * WBStartup message, and makes a DOS call.  DOS takes the message for a
 * late reply to its packet and raises the requester "Software Failure"
 * (error 87000004, gadgets "Suspend" and "Reboot") from inside the
 * packet wait of the helper, that is in the context of the helper, inside
 * reqtrap's patch of EasyRequestArgs.  A handler that makes a DOS call
 * there (a Write) gets the same requester again from the next packet wait,
 * and again: the stack runs out and the CPU halts.
 *
 * The right behaviour, checked by the test from the output: one line
 *   REQUESTER: EasyRequest title "Software Failure" ... -> answered 1
 * (1 = "Suspend": the helper is stopped, "Reboot" would halt the machine),
 * then this program's line "reqtraptest: the helper is stopped inside
 * Lock" (it did not come back from its call within 2 s), and the exit
 * status 0.  The broken twin (reqtrapdirect: reqtrap built with
 * -DCW_DIRECT, the handler writes in the caller's context as the old
 * reqtrap did, capped at 2 levels: deeper nesting wedges the machine)
 * writes the REQUESTER line a second time.
 *
 * With the argument `alert` the same for inject's alert patch: this task
 * (inject's own process: inject runs the program as its COMMAND) has the
 * message in its own port and calls DisplayAlert; inject's patch answers
 * at once and writes a line "inject: alert: ...", which it must do without
 * a DOS call in this task (ctxwrite.h), else DOS raises the requester here.
 * Prints "reqtraptest: DisplayAlert returned TRUE|FALSE" once the message
 * is taken from the port again.
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <string.h>

struct IntuitionBase *IntuitionBase;

static struct Message stray;
static volatile int state;              /* 1 before the call, 2 after it */

static void put(const char *s) { Write(Output(), (APTR)s, strlen(s)); }

static void helper(void)
{
    struct Process *me = (struct Process *)FindTask(0);
    BPTR l;

    memset(&stray, 0, sizeof stray);
    stray.mn_Node.ln_Type = NT_MESSAGE;
    stray.mn_Length = sizeof stray;
    PutMsg(&me->pr_MsgPort, &stray);
    state = 1;
    l = Lock("SYS:", SHARED_LOCK);
    state = 2;
    if (l) UnLock(l);
}

/* "alert": this task (inject runs it as COMMAND in its own process, whose
   alert patch answers at once and writes a line) has the unreplied message
   in its own port while it calls DisplayAlert.  The patch must not make a
   DOS call: a Write there would raise "Software Failure" in this task. */
static int alert_mode(void)
{
    static const UBYTE text[] = { 0x00, 0x40, 0x10, 'T', 'e', 's', 't', ' ', 'a', 'l', 'e', 'r', 't', 0, 0 };
    struct Process *me = (struct Process *)FindTask(0);
    struct IntuitionBase *ib = IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    LONG r;

    if (!ib) return 20;
    memset(&stray, 0, sizeof stray);
    stray.mn_Node.ln_Type = NT_MESSAGE;
    stray.mn_Length = sizeof stray;
    PutMsg(&me->pr_MsgPort, &stray);
    r = DisplayAlert(RECOVERY_ALERT, (UBYTE *)text, 40);
    while (GetMsg(&me->pr_MsgPort))     /* the stray message away: DOS works again */
        ;
    CloseLibrary((struct Library *)ib);
    put(r ? "reqtraptest: DisplayAlert returned TRUE\n" : "reqtraptest: DisplayAlert returned FALSE\n");
    return 0;
}

int main(int argc, char **argv)
{
    int i;

    if (argc > 1 && !strcmp(argv[1], "alert")) return alert_mode();

    if (!CreateNewProcTags(NP_Entry, (ULONG)helper, NP_Name, (ULONG)"reqtraptest helper",
                           NP_StackSize, 4000, NP_Priority, 0, TAG_END)) {
        put("reqtraptest: no process\n");
        return 20;
    }
    for (i = 0; i < 100 && state != 2; i++)
        Delay(2);
    put(state == 2 ? "reqtraptest: the helper came back from Lock\n"
        : state == 1 ? "reqtraptest: the helper is stopped inside Lock\n"
        : "reqtraptest: the helper did not start\n");
    return 0;
}
