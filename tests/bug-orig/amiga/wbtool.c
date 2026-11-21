/*
 * wbtool - a tool for the self test of wbrun's timeout= option
 * (tests/bug-orig/wbrun-timeout.cmake): it takes the WBStartup message from
 * its port and acts on the name of the first project (the second WBArg):
 *
 *   reply    replies the message at once and ends
 *   silent   ends without replying (as a Blitz program without WBStartup
 *            does, if nothing else happens to it)
 *   late     waits 2 s, replies, ends
 *   slow     waits 4 s, ends without replying (wbrun is gone by then)
 *
 * Without a project, or with another name, it ends at once without
 * replying.  No C startup (it would reply the message itself): __start is
 * the first code of the hunk.  Output is lost, as for every Workbench
 * start; wbrun's report is what the test looks at.  rc 0.
 */
__asm__("  .text\n  .globl __start\n__start:\n  jsr _run\n  rts\n");

#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

LONG run(void)
{
    struct Process *pr;
    struct WBStartup *msg;
    const char *n;

    SysBase = *(struct ExecBase **)4;
    pr = (struct Process *)FindTask(NULL);
    WaitPort(&pr->pr_MsgPort);
    msg = (struct WBStartup *)GetMsg(&pr->pr_MsgPort);
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 36))) {
        Forbid();
        ReplyMsg(&msg->sm_Message);
        return 20;
    }
    n = msg->sm_NumArgs > 1 ? (const char *)msg->sm_ArgList[1].wa_Name : "";
    if (n[0] == 'r') {                  /* reply */
        CloseLibrary((struct Library *)DOSBase);
        Forbid();
        ReplyMsg(&msg->sm_Message);
        return 0;
    }
    if (n[0] == 'l') {                  /* late */
        Delay(100);
        CloseLibrary((struct Library *)DOSBase);
        Forbid();
        ReplyMsg(&msg->sm_Message);
        return 0;
    }
    if (n[0] == 's' && n[1] == 'l')     /* slow */
        Delay(200);
    CloseLibrary((struct Library *)DOSBase);
    return 0;
}
