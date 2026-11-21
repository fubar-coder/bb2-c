/*
 * reqtrap - run a command with intuition's AutoRequest() and
 * EasyRequestArgs() patched: the requester texts are written to stdout
 * (prefix "REQUESTER:") and the call returns at once with the rightmost
 * (negative) answer, so that nothing in a batch run waits for a click.
 *
 * usage: reqtrap [-harden] COMMAND [ARGS]  (run through SystemTags, same stdout)
 * The patch stays only while the command runs; it is system-wide, so run
 * nothing else in parallel.
 *
 * The handlers run in the context of the task that raises the requester,
 * which can be inside DOS (a program started the Workbench way that did
 * not take its WBStartup message gets "Software Failure" from DOS).
 * By default every handler writes its line in that context and answers
 * 0 (the rightmost gadget; for AutoRequest FALSE), nothing is allocated
 * before the command runs.  With the option -harden (first argument) the
 * handlers make no DOS call: the text goes to a writer process of reqtrap
 * (ctxwrite.h: a task already inside a handler records nothing, counted),
 * and the requester "Software Failure" (a dead end, gadgets "Suspend" and
 * "Reboot") is answered with "Suspend" (1), not with "Reboot", which
 * halts the machine.  The writer takes about 4.5 KB from the memory of the
 * run, which is why the option is for the tests that need it
 * (tests/README.md, "The handlers of reqtrap").
 *
 * Kickstart 1.x (dos.library before V36: no SystemTags, no
 * EasyRequestArgs, no CacheClearU): only AutoRequest is patched, and
 * COMMAND (a path, no search path) is loaded with LoadSeg and called in
 * this process like the 1.3 CLI calls a command (d0/a0 = the arguments
 * with a newline, the stack size at 4(sp), cli_CommandName and cli_Module
 * set while it runs); its rc is reqtrap's.  Only machine code programs
 * (Blitz, C): the BCPL commands of the 1.3 C: directory need the BCPL
 * environment (checked: C:Version crashes after its output).  A command
 * that leaves with dos Exit() would take reqtrap with it (the Blitz
 * programs return).
 * Linked with libnix13 (-mcrt=nix13), which starts on 1.x as well.
 *
 * Built a second time with -DREQCLOSE as `reqclose` (same usage): while
 * the command runs, a second process answers modal requesters as
 * ENV:REQCLOSE says (reqclose.h).  reqtrap itself is unchanged.
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>

/* the patches: stubs in a block that is never freed (patchblock.h) */
#define PB_AUTO
#define PB_EASY
#include "patchblock.h"
#include "ctxwrite.h"

struct IntuitionBase *IntuitionBase;
static struct pbhead *pb_auto, *pb_easy;
static BPTR out;
static int harden;                      /* -harden: see the header */

static void put(const char *s) { if (s) Write(out, (APTR)s, strlen(s)); }

#ifdef REQCLOSE
#include "reqclose.h"
#endif

static void puttexts(struct cw_msg *m, struct IntuiText *t)
{
    for (; t; t = t->NextText) { cw_add(m, " \""); cw_add(m, (const char *)t->IText); cw_add(m, "\""); }
}

/* LONG call_seg(APTR entry, const char *args, LONG len, ULONG stack): a
   command's entry as the 1.3 CLI calls it - d0 = len, a0 = args, the
   stack size at 4(sp); every register but d0 kept, the stack pointer
   restored even if the command leaves it wrong */
__asm__(
"   .even\n"
"_call_seg:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"         /* 44 bytes; entry at 48(sp) */
"   move.l  sp,_call_seg_sp\n"
"   move.l  48(sp),a1\n"
"   move.l  52(sp),a0\n"
"   move.l  56(sp),d0\n"
"   move.l  60(sp),-(sp)\n"
"   jsr     (a1)\n"
"   move.l  _call_seg_sp,sp\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);
LONG call_seg(APTR entry, const char *args, LONG len, ULONG stack);
ULONG call_seg_sp;

/* The handlers run in the context of the task that raises the requester:
   no DOS call there (ctxwrite.h), the text goes to the writer process. */
void auto_c(struct IntuiText *body, struct IntuiText *pos, struct IntuiText *neg)
{
    CW_MSG(m);

    if (!cw_begin(m)) return;
    cw_add(m, "REQUESTER: AutoRequest body"); puttexts(m, body);
    cw_add(m, " pos"); puttexts(m, pos);
    cw_add(m, " neg"); puttexts(m, neg);
    cw_add(m, " -> answered FALSE\n");
    cw_end(m);
}

/* The answer: 0 (the rightmost gadget, the negative one); the requester
   "Software Failure" of DOS (error 87000004, a dead end, gadgets "Suspend"
   and "Reboot") is answered with 1 = "Suspend": "Reboot" halts the
   emulated machine, "Suspend" stops only the process. */
LONG easy_c(struct EasyStruct *es)
{
    CW_MSG(m);
    LONG answer = harden && es->es_Title && !strcmp((const char *)es->es_Title, "Software Failure") ? 1 : 0;

    if (!cw_begin(m)) return answer;
    cw_add(m, "REQUESTER: EasyRequest title \""); cw_add(m, (const char *)es->es_Title);
    cw_add(m, "\" text \""); cw_add(m, (const char *)es->es_TextFormat);
    cw_add(m, "\" gadgets \""); cw_add(m, (const char *)es->es_GadgetFormat);
    cw_add(m, answer ? "\" -> answered 1\n" : "\" -> answered 0\n");
    cw_end(m);
    return answer;
}

#ifdef REQCLOSE
static const char usage[] =
    "usage: reqclose [-harden] COMMAND [ARGS]\n"
    "  as reqtrap, and modal requesters answered as ENV:REQCLOSE says\n";
#else
static const char usage[] =
    "usage: reqtrap [-harden] COMMAND [ARGS]\n"
    "  run COMMAND with AutoRequest/EasyRequestArgs answered at once (texts\n"
    "  to stdout, prefix REQUESTER:); -harden: the texts are written by a\n"
    "  writer process, \"Software Failure\" is answered with Suspend\n";
#endif

/* Kickstart 1.x: the command cmd (args behind the first blank, a newline
   at the end) loaded and called in this process; its rc, 20 if it cannot
   be loaded */
static LONG run13(char *cmd)
{
    struct Process *me = (struct Process *)FindTask(0);
    struct CommandLineInterface *cli = BADDR(me->pr_CLI);
    UBYTE *name = cli ? BADDR(cli->cli_CommandName) : 0, oldname[256];
    BPTR oldmod = cli ? cli->cli_Module : 0, seg;
    char *args = strchr(cmd, ' ');
    LONG rc, n, cmdlen;

    if (args)
        *args++ = 0;
    else
        args = cmd + strlen(cmd);
    if (!(seg = LoadSeg(cmd)))
        return 20;
    /* the name's length before the newline is added: without arguments
       args is the end of cmd, and the newline would become part of the
       command name */
    cmdlen = strlen(cmd);
    n = strlen(args);
    args[n] = '\n';
    args[n + 1] = 0;
    if (name) {
        memcpy(oldname, name, name[0] + 1);
        name[0] = cmdlen > 255 ? 255 : cmdlen;
        memcpy(name + 1, cmd, name[0]);
        cli->cli_Module = seg;
    }
    rc = call_seg((APTR)((ULONG)BADDR(seg) + 4), args, strlen(args),
                  cli ? (ULONG)cli->cli_DefaultStack << 2 : 4000);
    if (name) {
        memcpy(name, oldname, oldname[0] + 1);
        cli->cli_Module = oldmod;
    }
    UnLoadSeg(seg);
    return rc;
}

int main(int argc, char **argv)
{
    static char cmd[512];               /* reqclose keeps a pointer (rc_cmd) */
    LONG rc;
    int i, v36;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "-harden")) {
        harden = 1;
        argc--;
        argv++;
    }
    if (argc < 2) return 20;
    cmd[0] = 0;
    for (i = 1; i < argc; i++) {
        if (strlen(cmd) + strlen(argv[i]) + 3 >= sizeof cmd) return 20;
        if (i > 1) strcat(cmd, " ");
        strcat(cmd, argv[i]);
    }
    out = Output();
    v36 = DOSBase->dl_lib.lib_Version >= 36;
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", v36 ? 36 : 33);
    if (!IntuitionBase) return 20;
    v36 = v36 && IntuitionBase->LibNode.lib_Version >= 36;
    if (!cw_start(out, v36 && harden)) {
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    pb_auto = PB_INSTALL((struct Library *)IntuitionBase, -348, auto, sizeof(struct pbhead), auto_c);
    if (v36)
        pb_easy = PB_INSTALL((struct Library *)IntuitionBase, -588, easy, sizeof(struct pbhead), easy_c);
    if (!pb_auto || (v36 && !pb_easy)) {
        if (pb_easy) PB_REMOVE(pb_easy, (struct Library *)IntuitionBase, -588, easy, 1);
        if (pb_auto) PB_REMOVE(pb_auto, (struct Library *)IntuitionBase, -348, auto, 1);
        cw_stop();
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
#ifdef REQCLOSE
    rc_start(cmd);
#endif
    if (v36)
        rc = SystemTags(cmd, SYS_Input, Input(), SYS_Output, out, SYS_Asynch, FALSE,
                        NP_CloseInput, FALSE, NP_CloseOutput, FALSE, TAG_END);
    else
        rc = run13(cmd);
#ifdef REQCLOSE
    rc_end();
#endif
    /* a call still inside a stub is waited for (patchblock.h) */
    PB_REMOVE(pb_auto, (struct Library *)IntuitionBase, -348, auto, 1);
    if (v36) PB_REMOVE(pb_easy, (struct Library *)IntuitionBase, -588, easy, 1);
    cw_stop();
    if (cw_skipped) {
        char n[12], *p = n + 11;
        ULONG u = cw_skipped;

        *p = 0;
        do { *--p = '0' + u % 10; u /= 10; } while (u);
        put("reqtrap: ");
        put(p);
        put(" requesters not recorded (a task already inside one)\n");
    }
    CloseLibrary((struct Library *)IntuitionBase);
    return rc < 0 ? 20 : rc;
}
