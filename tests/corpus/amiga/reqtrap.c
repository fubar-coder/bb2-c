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
 * Built a second time with -DREQCLOSE as `reqclose` (same usage): while
 * the command runs, a second process answers modal requesters as
 * ENV:REQCLOSE says (reqclose.h).  reqtrap itself is unchanged.
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <dos/dostags.h>
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

int main(int argc, char **argv)
{
    char cmd[512];
    LONG rc;
    int i;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    if (argc < 2) return 20;
    cmd[0] = 0;
    for (i = 1; i < argc; i++) {
        if (strlen(cmd) + strlen(argv[i]) + 2 >= sizeof cmd) return 20;
        if (i > 1) strcat(cmd, " ");
        strcat(cmd, argv[i]);
    }
    out = Output();
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase) return 20;
    Forbid();
    old_auto = SetFunction((struct Library *)IntuitionBase, -348, (APTR)auto_patch);
    old_easy = SetFunction((struct Library *)IntuitionBase, -588, (APTR)easy_patch);
    CacheClearU();
    Permit();
#ifdef REQCLOSE
    rc_start(cmd);
#endif
    rc = SystemTags(cmd, SYS_Input, Input(), SYS_Output, out, SYS_Asynch, FALSE,
                    NP_CloseInput, FALSE, NP_CloseOutput, FALSE, TAG_END);
#ifdef REQCLOSE
    rc_end();
#endif
    Forbid();
    SetFunction((struct Library *)IntuitionBase, -348, old_auto);
    SetFunction((struct Library *)IntuitionBase, -588, old_easy);
    CacheClearU();
    Permit();
    CloseLibrary((struct Library *)IntuitionBase);
    return rc < 0 ? 20 : rc;
}
