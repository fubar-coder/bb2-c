/*
 * reqtrap - run a command with intuition's AutoRequest() and
 * EasyRequestArgs() patched: the requester texts are written to stdout
 * (prefix "REQUESTER:") and the call returns at once with the rightmost
 * (negative) answer, so that nothing in a batch run waits for a click.
 *
 * usage: reqtrap COMMAND [ARGS]     (run through SystemTags, same stdout)
 * The patch stays only while the command runs; it is system-wide, so run
 * nothing else in parallel.
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

struct IntuitionBase *IntuitionBase;
static APTR old_auto, old_easy;
static BPTR out;

static void put(const char *s) { if (s) Write(out, (APTR)s, strlen(s)); }

#ifdef REQCLOSE
#include "reqclose.h"
#endif

static void puttexts(struct IntuiText *t)
{
    for (; t; t = t->NextText) { put(" \""); put((const char *)t->IText); put("\""); }
}

/* AutoRequest(a0 win, a1 body, a2 pos, a3 neg, d0, d1, d2 w, d3 h) */
__asm__(
"   .text\n"
"   .even\n"
"_auto_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a3,-(sp)\n"
"   move.l  a2,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _auto_c\n"
"   lea     12(sp),sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   moveq   #0,d0\n"                    /* FALSE = negative gadget */
"   rts\n"
/* EasyRequestArgs(a0 win, a1 easy, a2 idcmp, a3 args) */
"   .even\n"
"_easy_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _easy_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   moveq   #0,d0\n"                    /* 0 = rightmost gadget */
"   rts\n"
);
void auto_patch(void), easy_patch(void);

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

void auto_c(struct IntuiText *body, struct IntuiText *pos, struct IntuiText *neg)
{
    put("REQUESTER: AutoRequest body"); puttexts(body);
    put(" pos"); puttexts(pos);
    put(" neg"); puttexts(neg);
    put(" -> answered FALSE\n");
}

void easy_c(struct EasyStruct *es)
{
    put("REQUESTER: EasyRequest title \""); put((const char *)es->es_Title);
    put("\" text \""); put((const char *)es->es_TextFormat);
    put("\" gadgets \""); put((const char *)es->es_GadgetFormat);
    put("\" -> answered 0\n");
}

#ifdef REQCLOSE
static const char usage[] =
    "usage: reqclose COMMAND [ARGS]\n"
    "  as reqtrap, and modal requesters answered as ENV:REQCLOSE says\n";
#else
static const char usage[] =
    "usage: reqtrap COMMAND [ARGS]\n"
    "  run COMMAND with AutoRequest/EasyRequestArgs answered at once (texts\n"
    "  to stdout, prefix REQUESTER:)\n";
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
    LONG rc, n;

    if (args)
        *args++ = 0;
    else
        args = cmd + strlen(cmd);
    if (!(seg = LoadSeg(cmd)))
        return 20;
    n = strlen(args);
    args[n] = '\n';
    args[n + 1] = 0;
    if (name) {
        memcpy(oldname, name, name[0] + 1);
        n = strlen(cmd);
        name[0] = n > 255 ? 255 : n;
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
    Forbid();
    old_auto = SetFunction((struct Library *)IntuitionBase, -348, (APTR)auto_patch);
    if (v36) {
        old_easy = SetFunction((struct Library *)IntuitionBase, -588, (APTR)easy_patch);
        CacheClearU();
    }
    Permit();
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
    Forbid();
    SetFunction((struct Library *)IntuitionBase, -348, old_auto);
    if (v36) {
        SetFunction((struct Library *)IntuitionBase, -588, old_easy);
        CacheClearU();
    }
    Permit();
    CloseLibrary((struct Library *)IntuitionBase);
    return rc < 0 ? 20 : rc;
}
