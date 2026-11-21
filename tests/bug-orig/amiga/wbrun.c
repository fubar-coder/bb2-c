/*
 * wbrun - start a program like the Workbench does, for the bug-orig tests
 * (pass file keys wrap= for the compiler, runwrap= for the programs): a
 * new process without a CLI, a WBStartup message on its port with the
 * tool and the projects as WBArgs (lock of the directory + name), wait
 * for the reply.
 *
 * usage: wbrun [noprojects] [freeseg] [cd] [dosopen] [racehold[=nowait]]
 *              [timeout=SEC] [inuse=PATH]... TOOL [PROJECT...]
 *   noprojects   start the tool alone, without the PROJECTs (for a
 *                wrapper whose caller always passes one)
 *   freeseg      like the Workbench (priority 1, the tool 0) with a memory
 *                checker like MungWall: right after the reply the
 *                seglist is overwritten with ILLEGAL ($4AFC) and freed,
 *                before the process has ended; wbrun then waits up to 2 s
 *                for the end (a tool that still runs its code after the
 *                reply crashes; without dosopen)
 *   cd           report the current directory at the end
 *   dosopen      report how often the process opened dos.library minus
 *                how often it closed it (exec's OpenLibrary,
 *                OldOpenLibrary, CloseLibrary patched while it runs;
 *                other tasks do not count - lib_OpenCnt would, e.g. a
 *                background tool of the Workbench closing its own).  The
 *                patches run from a block of memory that is never freed
 *                (patchblock.h) and the removal waits until no task is
 *                inside them any more
 *   racehold[=nowait]  self test of that removal (wbrun_race), with dosopen:
 *                when the tool has ended, a helper process is held inside a
 *                patch while the patches are removed; wbrun reports
 *                "racehold: ..." lines: a task was inside, the calls still
 *                inside after the removal, whether the task returned.  With
 *                =nowait the removal does not wait: the broken input of the
 *                test
 *   timeout=SEC  wait at most SEC seconds (1 to 60) for the reply, for a tool
 *                that may not answer the message: when it has not come, wbrun
 *                says whether the process ended or still runs, waits for its
 *                end (a process that crashed or hangs keeps wbrun here) and
 *                stops there; without timeout= it waits for ever.  A reply
 *                that comes in time goes on as usual, with no extra line
 *                (built with -DWBRUN_DEFAULT_TIMEOUT=N, as wbrunt, N is the
 *                default: wbrunt as the launcher of a corpus test starts every
 *                program the Workbench way and does not wait for ever)
 *   inuse=PATH   after the end: is PATH still in use (an exclusive Lock
 *                fails with ERROR_OBJECT_IN_USE, e.g. a file left open)
 * The tool has to answer the WBStartup message (a Blitz program only
 * does with the command WBStartup; without it, the manual says, it
 * crashes when run from the Workbench), otherwise wbrun waits for ever
 * (or for timeout=).  A tool that leaves the message in its port is
 * stopped by DOS with the requester "Software Failure" (error 87000004):
 * at its first DOS call that waits for a packet, or, if it makes none, at
 * its exit (DOS frees a lock then).  It is no hang of wbrun: the requester
 * waits for a click (a launcher answers it: reqtrap, "Suspend").
 *
 * Behind the end it writes what the options ask for (lines "wbrun: ..."):
 *   current dir at end: start | tool dir | project dir N | other
 *                the current directory of the process when it ended
 *                (NP_ExitCode, in the process): start = the one it was
 *                started with, tool dir / project dir N = the lock of
 *                that WBArg
 *   dos.library open count +N   opened minus closed by the process
 *   seglist overwritten and freed after the reply   with freeseg
 *   the process did not end     with freeseg (then nothing else)
 *   no reply, the process ended          with timeout=: the process's code
 *                ended (its exit hook ran) without a reply (then nothing
 *                else).  That says nothing about what DOS does with such a
 *                process afterwards: it frees a lock at the exit and raises
 *                the requester "Software Failure" (error 87000004) there
 *                (a launcher such as reqtrap records it, answers "Suspend",
 *                and the process stays suspended).  wbrun waits 0.2 s before
 *                it writes the line, so that a requester comes first
 *   no reply within N s, the process did not end   with timeout=: it neither
 *                replied nor ended, then one more line: the process ended
 *                later, after replying | without a reply (then nothing else)
 *   in use: PATH | not in use: PATH | missing: PATH   for every inuse=
 *
 * Like the Workbench the process has no input and output handles (0):
 * startup code for the Workbench may set its own and close them, DOS must
 * not close them again (AN_FileReclosed); what the program prints is
 * lost.  Unlike the Workbench: the exit hook sets the current
 * directory back to the start lock, so that DOS frees that one and not a
 * WBArg lock, which wbrun frees itself (like the Workbench after the
 * reply).
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#define PB_THRU
#include "../../corpus/amiga/patchblock.h"

#ifndef WBRUN_DEFAULT_TIMEOUT
#define WBRUN_DEFAULT_TIMEOUT 0
#endif
#define MAXARGS 8
#define MAXINUSE 8

extern struct DosLibrary *DOSBase;

static BPTR out;
static BPTR startdir;
static struct WBArg args[MAXARGS];
static int nargs;
static int cd_end = -2;                 /* -1 start, 0.. WBArg, -3 other */
static struct Task *parent;

static void put(const char *s) { Write(out, (APTR)s, strlen(s)); }

/* in the process, when it ends: which lock is its current directory */
void exit_c(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    BPTR cd = pr->pr_CurrentDir;
    int i;

    cd_end = -3;
    if (cd == startdir)
        cd_end = -1;
    for (i = 0; i < nargs; i++)
        if (cd == args[i].wa_Lock)
            cd_end = i;
    if (cd != startdir)
        CurrentDir(startdir);
    Signal(parent, SIGBREAKF_CTRL_F);
}

void exit_stub(void);

/* the process started by wbrun: its exit code is wbrun's hook */
static int ours(void)
{
    struct Task *t = FindTask(NULL);

    return t->tc_Node.ln_Type == NT_PROCESS
           && ((struct Process *)t)->pr_ExitCode == (APTR)exit_stub;
}

/* dosopen: the calls of the process, counted in the process */
static volatile LONG dos_count;
static volatile int in_old;             /* OldOpenLibrary calls OpenLibrary */
APTR old_open, old_oldopen, old_close;

void enter_old_c(void)
{
    if (ours())
        in_old = 1;
}

/* racehold: a helper process that is held inside a patch (opened_c) */
static struct Task *race_task;
static volatile int race_in, race_done;

void opened_c(ULONG base, ULONG old)
{
    if (race_task && SysBase->ThisTask == race_task) {
        if (!race_in) {
            race_in = 1;                /* stay inside the stub */
            Delay(50);
        }
        return;
    }
    if (!ours())
        return;
    if (old)
        in_old = 0;
    else if (in_old)
        return;
    if (base && base == (ULONG)DOSBase)
        dos_count++;
}

void close_c(ULONG base)
{
    if (ours() && base == (ULONG)DOSBase)
        dos_count--;
}

void open_patch(void), oldopen_patch(void), close_patch(void);
__asm__(
"   .text\n"
"   .even\n"
"_open_patch:\n"
"   pea     _open_ret\n"
"   move.l  _old_open,-(sp)\n"
"   rts\n"
"_open_ret:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   clr.l   -(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _opened_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
"   .even\n"
"_oldopen_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _enter_old_c\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   pea     _oldopen_ret\n"
"   move.l  _old_oldopen,-(sp)\n"
"   rts\n"
"_oldopen_ret:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  #1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _opened_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
"   .even\n"
"_close_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _close_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _old_close,-(sp)\n"
"   rts\n");

/* The patches run from a block of memory that is never freed (patchblock.h,
   PB_THRU): the block counts the calls inside the stubs above, the removal
   restores the vectors and then waits until no task is inside any more (a
   task of another process may be inside OpenLibrary or CloseLibrary, which
   are patched system-wide, when wbrun ends and its code is freed).  After
   the removal the block passes every call on to the old function without
   touching the program. */
#define MAXPB 3
static struct pbhead *pb_blk[MAXPB];
static struct Library *pb_lib[MAXPB];
static WORD pb_lvo[MAXPB];
static int pb_n;
static int pb_failed;

/* patch vector lvo of exec with stub; the old function (0 if there is no
   memory for the block: that patch is then missing, reported at the end) */
static APTR pset(WORD lvo, APTR stub)
{
    struct Library *e = (struct Library *)SysBase;
    struct pbhead *b;

    if (pb_n >= MAXPB || !(b = PB_INSTALL(e, lvo, thru, 16, stub))) {
        pb_failed = 1;
        return NULL;
    }
    pb_blk[pb_n] = b;
    pb_lib[pb_n] = e;
    pb_lvo[pb_n++] = lvo;
    return b->old;
}

/* remove all patches (newest first) and wait for the calls inside: up to
   10 s; wait = 0 only for the self test racehold=nowait.  Returns the
   number of calls still inside. */
static LONG punset(int wait)
{
    LONG left;
    int i, n;

    Forbid();
    for (i = pb_n - 1; i >= 0; i--)
        PB_REMOVE(pb_blk[i], pb_lib[i], pb_lvo[i], thru, 0);
    Permit();
    for (n = 0;; n++) {
        for (left = 0, i = 0; i < pb_n; i++)
            left += pb_blk[i]->count;
        if (!left || !wait || n >= 500)
            break;
        Delay(1);
    }
    pb_n = 0;
    return left;
}

static void patch(void)
{
    old_open = pset(-552, (APTR)open_patch);
    old_oldopen = pset(-408, (APTR)oldopen_patch);
    old_close = pset(-414, (APTR)close_patch);
}

/* racehold: a helper process (priority 1: it runs at once) opens
   dos.library and is held inside the patch (opened_c) while the patches
   are removed */
static void race_helper(void)
{
    struct Library *l;

    race_task = FindTask(NULL);         /* before its first call of a patch */
    l = OpenLibrary("dos.library", 0);
    if (l)
        CloseLibrary(l);
    race_done = 1;
}

void exit_stub(void);
__asm__(
"   .text\n"
"   .even\n"
"_exit_stub:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   jsr     _exit_c\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rts\n");

/* PATH -> lock of its directory + name; 0 = cannot be locked */
static int split(const char *path, struct WBArg *a)
{
    static char dir[256];
    const char *p = strrchr(path, '/'), *c = strchr(path, ':');
    size_t n;

    if (!p || (c && c > p))
        p = c;
    if (!p) {
        a->wa_Lock = DupLock(((struct Process *)FindTask(NULL))->pr_CurrentDir);
        a->wa_Name = (BYTE *)path;
        return a->wa_Lock != 0;
    }
    n = (size_t)(p - path) + (*p == ':');
    if (n >= sizeof dir)
        return 0;
    memcpy(dir, path, n);
    dir[n] = 0;
    a->wa_Lock = Lock(dir, SHARED_LOCK);
    a->wa_Name = (BYTE *)p + 1;
    return a->wa_Lock != 0;
}

static const char usage[] =
    "usage: wbrun [noprojects] [freeseg] [cd] [dosopen] [racehold[=nowait]] [timeout=SEC]\n"
    "             [inuse=PATH]...\n"
    "             TOOL [PROJECT...]\n"
    "  start TOOL like the Workbench does (no CLI, a WBStartup message)\n";

int main(int argc, char **argv)
{
    static char line[300];
    static const char *inuse[MAXINUSE];
    struct CommandLineInterface *cli = Cli();
    ULONG stack = cli ? cli->cli_DefaultStack * 4 : 4096;
    struct MsgPort *port;
    struct WBStartup msg;
    struct Process *pr;
    struct Process *racep = NULL;
    BPTR seg;
    int i, k, ninuse = 0, noprojects = 0, o_cd = 0, o_dos = 0, o_free = 0, o_race = 0, o_time = WBRUN_DEFAULT_TIMEOUT;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    SetSignal(0, SIGBREAKF_CTRL_F);     /* a command runs in the Shell's process: no stale one */
    out = Output();
    parent = FindTask(NULL);
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "noprojects"))
            noprojects = 1;
        else if (!strcmp(argv[i], "freeseg"))
            o_free = 1;
        else if (!strcmp(argv[i], "cd"))
            o_cd = 1;
        else if (!strcmp(argv[i], "dosopen"))
            o_dos = 1;
        else if (!strcmp(argv[i], "racehold") || !strcmp(argv[i], "racehold=nowait"))
            o_dos = o_race = argv[i][8] ? 2 : 1;
        else if (!strncmp(argv[i], "timeout=", 8)) {
            const char *t = argv[i] + 8;

            o_time = 0;
            while (*t >= '0' && *t <= '9' && o_time <= 60)
                o_time = o_time * 10 + *t++ - '0';
            if (*t || o_time < 1 || o_time > 60) {
                put("wbrun: timeout=SEC needs 1 to 60\n");
                return 20;
            }
        } else if (!strncmp(argv[i], "inuse=", 6) && ninuse < MAXINUSE)
            inuse[ninuse++] = argv[i] + 6;
        else
            break;
    if (i < argc && noprojects)
        argc = i + 1;
    if (i >= argc || argc - i > MAXARGS || (o_free && o_dos)) {
        put("usage: wbrun [noprojects] [freeseg] [cd] [dosopen] [racehold[=nowait]] [timeout=SEC]\n"
            "             [inuse=PATH]... TOOL [PROJECT...]   (freeseg not with dosopen)\n");
        return 20;
    }
    if (!(seg = LoadSeg(argv[i]))) {
        sprintf(line, "wbrun: cannot load %s\n", argv[i]);
        put(line);
        return 20;
    }
    for (k = i; k < argc; k++, nargs++)
        if (!split(argv[k], &args[nargs])) {
            sprintf(line, "wbrun: cannot lock the directory of %s\n", argv[k]);
            put(line);
            return 20;
        }
    if (!(port = CreateMsgPort()))
        return 20;
    startdir = DupLock(((struct Process *)FindTask(NULL))->pr_CurrentDir);
    if (o_dos) {
        patch();
    }
    pr = CreateNewProcTags(NP_Seglist, seg, NP_FreeSeglist, FALSE,
                           NP_Name, args[0].wa_Name, NP_StackSize, stack, NP_Priority, 0,
                           NP_CurrentDir, startdir, NP_HomeDir, DupLock(args[0].wa_Lock),
                           NP_Input, 0, NP_CloseInput, FALSE,
                           NP_Output, 0, NP_CloseOutput, FALSE,
                           NP_ExitCode, exit_stub, TAG_DONE);
    if (!pr) {
        put("wbrun: CreateNewProc failed\n");
        return 20;
    }
    memset(&msg, 0, sizeof msg);
    msg.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
    msg.sm_Message.mn_ReplyPort = port;
    msg.sm_Message.mn_Length = sizeof msg;
    msg.sm_Process = &pr->pr_MsgPort;
    msg.sm_Segment = seg;
    msg.sm_NumArgs = nargs;
    msg.sm_ArgList = args;
    if (o_free)                         /* the Workbench runs at 1 */
        SetTaskPri(parent, 1);
    PutMsg(&pr->pr_MsgPort, &msg.sm_Message);
    /* the reply comes under Forbid (the startup code); the process has
       ended when this task runs again - unless the startup code calls
       Permit before it returns */
    if (o_time) {
        int replied = 0;

        for (k = 0; k < o_time * 10; k++) {
            if (GetMsg(port)) {
                replied = 1;
                break;
            }
            if (SetSignal(0, 0) & SIGBREAKF_CTRL_F)
                break;
            Delay(5);
        }
        if (!replied && !GetMsg(port)) {
            /* the exit hook of the process lives in this program: wbrun
               stays until the process has ended, however late (a process
               that crashed or hangs keeps it here: the session's time
               limit ends it) */
            if (SetSignal(0, 0) & SIGBREAKF_CTRL_F) {
                /* the process goes on after its exit hook: DOS frees its
                   lock, and a requester from that comes before this line */
                Delay(10);
                put("wbrun: no reply, the process ended\n");
                SetSignal(0, SIGBREAKF_CTRL_F);
            } else {
                sprintf(line, "wbrun: no reply within %d s, the process did not end\n", o_time);
                put(line);
                Wait(SIGBREAKF_CTRL_F);
                put(GetMsg(port) ? "wbrun: the process ended later, after replying\n"
                                 : "wbrun: the process ended later, without a reply\n");
            }
            Delay(5);                   /* DOS has removed the process */
            for (k = 0; k < nargs; k++)
                UnLock(args[k].wa_Lock);
            UnLoadSeg(seg);
            DeleteMsgPort(port);
            return 0;
        }
    } else {
        WaitPort(port);
        GetMsg(port);
    }
    if (o_free) {
        ULONG s, a, end;

        for (s = (ULONG)BADDR(seg); s; s = (ULONG)BADDR(*(BPTR *)s))
            for (a = s + 4, end = s - 4 + *(ULONG *)(s - 4); a + 2 <= end; a += 2)
                *(UWORD *)a = 0x4afc;
        UnLoadSeg(seg);
        seg = 0;
        put("wbrun: seglist overwritten and freed after the reply\n");
        SetTaskPri(parent, 0);
        for (k = 0; k < 100 && !(SetSignal(0, 0) & SIGBREAKF_CTRL_F); k++)
            Delay(1);
        if (!(SetSignal(0, SIGBREAKF_CTRL_F) & SIGBREAKF_CTRL_F)) {
            put("wbrun: the process did not end\n");
            return 0;
        }
    } else
        Wait(SIGBREAKF_CTRL_F);         /* the exit hook ran */
    Delay(5);                           /* DOS has removed the process */
    if (o_dos) {
        LONG left;

        if (o_race) {
            racep = CreateNewProcTags(NP_Entry, (ULONG)race_helper, NP_Name,
                                      (ULONG)"wbrun race helper", NP_Priority, 1,
                                      NP_StackSize, 4096, TAG_DONE);
            for (k = 0; racep && !race_in && k < 100; k++)
                Delay(1);
        }
        left = punset(o_race != 2);
        if (left && !o_race) {
            sprintf(line, "wbrun: %ld calls still inside a patch after 10 s\n", (long)left);
            put(line);
        }
        if (o_race) {
            for (k = 0; racep && !race_done && k < 500; k++)
                Delay(1);
            put(race_in ? "wbrun: racehold: a task was inside a patch\n"
                : "wbrun: racehold: no task inside a patch\n");
            sprintf(line, "wbrun: racehold: %ld calls inside after the removal\n", (long)left);
            put(line);
            put(race_done ? "wbrun: racehold: the task returned\n"
                : "wbrun: racehold: the task did not return\n");
        }
        if (pb_failed)
            put("wbrun: no memory for a patch\n");
    }

    if (!o_cd)
        ;
    else if (cd_end == -1)
        put("wbrun: current dir at end: start\n");
    else if (cd_end == 0)
        put("wbrun: current dir at end: tool dir\n");
    else if (cd_end > 0) {
        sprintf(line, "wbrun: current dir at end: project dir %d\n", cd_end);
        put(line);
    } else
        put("wbrun: current dir at end: other\n");
    if (o_dos) {
        sprintf(line, "wbrun: dos.library open count %+ld\n", (long)dos_count);
        put(line);
    }
    for (k = 0; k < ninuse; k++) {
        BPTR l = Lock(inuse[k], EXCLUSIVE_LOCK);

        if (l) {
            UnLock(l);
            sprintf(line, "wbrun: not in use: %s\n", inuse[k]);
        } else
            sprintf(line, IoErr() == ERROR_OBJECT_IN_USE ? "wbrun: in use: %s\n"
                    : "wbrun: missing: %s\n", inuse[k]);
        put(line);
    }
    for (k = 0; k < nargs; k++)
        UnLock(args[k].wa_Lock);
    if (seg)
        UnLoadSeg(seg);
    DeleteMsgPort(port);
    return 0;
}
