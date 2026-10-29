/*
 * teddrive - drive TED (blitz2:ted) with a script, for the TED A/B tests
 * in the emulator (bb2 ted-c/notes/PHASE3-PLAN.md section 4, "Weg D").
 *
 *   teddrive SCRIPT PREFIX COMMAND [ARGS]
 *
 * Runs COMMAND (normally `blitz2:blitz2 FILE`) through SystemTags; a
 * second process plays SCRIPT against the windows of the program:
 * keys, menu picks, gadgets and requester answers are synthetic
 * IntuiMessages put into the UserPort of TED's current window (the
 * technique of tests/corpus/amiga/reqclose.h, whose helpers are
 * included here unchanged: log buffer, rc_valid, rc_label, the static
 * message buffers with their own reply port).  No input.device.
 *
 * While the command runs, intuition's CloseWindow is patched (a window
 * opened at the address of a closed one counts as new) and dos's Open
 * (did TED open the file argument as it is, #340).  The patches are
 * removed at the end; a call still inside CloseWindow then returns into
 * teddrive, which waits 0.2 s before it ends (not a guarantee, TODO.md).
 *
 * Run it under reqtrap (tests/corpus/amiga), which answers AutoRequest and
 * EasyRequestArgs at once (DOS requesters, alerts):
 *   reqtrap teddrive SCRIPT PREFIX blitz2:blitz2 FILE >STDOUT
 *
 * Files (PREFIX is a path prefix, e.g. TEST:out/t00.):
 *   PREFIX "log"       every step before it runs, its result and time, new
 *                      and closed windows with their gadgets (appended,
 *                      opened and closed per line: survives a hang)
 *   PREFIX "result"    one line: OK | FAIL step N reason
 *   PREFIX "ok"        only after OK (for `If EXISTS` in session scripts)
 *   PREFIX NAME ".scr" screen dumps (`dump NAME`, format below)
 *
 * The program's screen is the first screen opened after the start that has
 * a backdrop window (TED's mywindow); that window is the main window, every
 * later window on the screen a requester.  The target of all input is the
 * newest requester still open, else the main window - what TED's
 * pickwindow() reads (req_window, else mywindow).
 *
 * "idle" (TED waits for input): under Forbid + LockIBase the task of the
 * target's UserPort is in TS_WAIT, its tc_SigWait contains the port's
 * signal (TED sits in WaitPort of getinput, not in Delay or a DOS packet),
 * the port is empty and all our messages are replied - three times in a
 * row, 20 ms apart (Delay(1)).  The reply alone is not enough: handled0
 * replies before the routine runs.  Commands that send wait for the reply
 * and then for idle again (or the end of the program).
 *
 * Script (one command per line, ';' starts a comment, strings in "..."
 * with \n \t \\ \" \xHH):
 *   timeout SEC              limit of every following wait (default 15)
 *   wait screen              program screen open, target = main window, idle
 *   wait req "L1" ["L2"...]  newest requester has gadgets with these labels
 *                            (case-insensitive, blanks around ignored), idle
 *   wait freq "TITLE"        newest requester has this window title, idle
 *   wait gone                the requester of the last `wait req`/`wait freq`
 *                            is closed, idle
 *   wait idle
 *   wait exit                the command has returned
 *   key CODE [QUAL]          RAWKEY (hex), then idle (or exit)
 *   type "TEXT"              each character via MapANSI (default keymap) as
 *                            RAWKEY + qualifier, \n = Return ($44), \t = Tab
 *                            ($42); idle after every key
 *   menu M I [S]             MENUPICK FULLMENUNUM(M, I, S; S default none)
 *   gad ID                   GADGETUP on the gadget with this GadgetID in the
 *                            target window
 *   str ID "TEXT"            fill a string gadget of the target (Buffer,
 *                            NumChars, BufferPos/DispPos 0, LongInt if
 *                            GACT_LONGINT), RefreshGList; no message
 *   freq ok|cancel ["NAME" ["PATH"]]
 *                            file requester: NAME into its file gadget (the
 *                            lowest string gadget), PATH into its path gadget
 *                            (the highest), then GADGETUP on "OK" / "Cancel"
 *                            (no Cancel gadget: CLOSEWINDOW)
 *   dump NAME                wait idle, then the screen into PREFIX NAME.scr
 *   mem NAME                 AvailMem into the log
 *   log "TEXT"
 *   expectfail COMMAND...    COMMAND must fail (e.g. a wait that must time
 *                            out); its failure does not end the script
 *
 * A failing command writes "FAIL step N reason" into the result file at
 * once (with " (#340: ...)" behind it if TED did not open the file argument as
 * it is), then the rescue: as long as the program runs and its target window
 * becomes idle (at most 8 rounds, 15 s): main window -> menu QUIT, file
 * requester -> Cancel, TED requester -> gadget 1 if it is an ENDGADGET (yes
 * on the erase requester, OK on error/version requesters) else Esc.  If the
 * program does not end, the session time limit of the runner does.
 *
 * Dump format: text header "key value" lines (magic TEDSCR1, width,
 * height, depth, bytesperrow, rows, rowbytes, interleaved, modeid, mousex,
 * mousey, memtype, title, windows N + N lines "window ..."), a line "data",
 * then depth x rows x rowbytes bytes (plane 0 first, rows of rowbytes bytes
 * from stride bytesperrow).  Decoder: tests/ted-ab/runner (teddump).
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <string.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase;
static BPTR out;                /* reqclose.h rc_line() (not used here) */

/* the closer's helpers, unchanged (most of it is unused: rc_main and the
   answer logic; -Wno-unused-function for this target) */
#include "reqclose.h"
#include "tdscript.h"

#define TD_MAXSCR  16
#define TD_MAXWIN  16
#define TD_MAXGAD  48
#define TD_TICKS   50           /* DateStamp ticks per second */

/* ------------------------------------------------------------ shared */

static char td_prefix[200];
static char td_logpath[220];
static char td_cmd[512];
static char td_script[16384];
static struct Task *td_parent;
static BYTE td_donesig = -1;
static volatile int td_exited;  /* the command has returned */
static volatile LONG td_rc;

static struct Screen *td_oldscr[TD_MAXSCR];
static int td_noldscr;

/* ------------------------------------------------------------ log */

/* append the collected text (rc_lb) to the log; IBase must not be
   locked.  Opened and closed per call: the file survives a hang. */
static void td_flush(void)
{
    BPTR fh;
    if (!rc_lb[0]) return;
    fh = Open((CONST_STRPTR)td_logpath, MODE_READWRITE);
    if (fh) {
        Seek(fh, 0, OFFSET_END);
        Write(fh, rc_lb, strlen(rc_lb));
        Close(fh);
    }
    rc_lb[0] = 0;
}

static void td_log(const char *a, const char *b)
{
    rc_cat(a);
    if (b) rc_cat(b);
    rc_cat("\n");
    td_flush();
}

static void td_writefile(const char *suffix, const char *text)
{
    char p[240];
    BPTR fh;
    strcpy(p, td_prefix);
    strcat(p, suffix);
    fh = Open((CONST_STRPTR)p, MODE_NEWFILE);
    if (fh) {
        Write(fh, (APTR)text, strlen(text));
        Close(fh);
    }
}

/* time in ticks (1/50 s) since the first call */
static LONG td_day0 = -1;
static LONG td_now(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    if (td_day0 < 0) td_day0 = ds.ds_Days;
    return ((ds.ds_Days - td_day0) * 1440 + ds.ds_Minute) * 3000 + ds.ds_Tick;
}

static LONG td_t0;
static void td_time(void)       /* "t=SS.SS" since the start of the driver */
{
    LONG t = td_now() - td_t0;
    char b[8];
    rc_cat("t=");
    rc_dec(t / TD_TICKS);
    rc_cat(".");
    b[0] = '0' + (t % TD_TICKS) * 2 / 10;
    b[1] = '0' + (t % TD_TICKS) * 2 % 10;
    b[2] = 0;
    rc_cat(b);
}

/* ------------------------------------------------- windows of the program */

struct tdgad {
    UWORD id, type, flags, act;
    WORD l, t, w, h;
    char label[28];
    char buf[40];               /* string gadgets: the buffer contents */
};
struct tdwin {
    struct Window *w;
    UBYTE *title;               /* title pointer when first seen */
    int main;                   /* backdrop: TED's mywindow */
    int seq;                    /* order of appearance */
    ULONG since;                /* td_ncl when first seen */
    char text[64];              /* title text */
};
static struct Screen *td_scr;  /* the program's screen */
static struct tdwin td_w[TD_MAXWIN];
static int td_nw, td_seq;

/* CloseWindow is patched while the command runs: the closed windows go to
   a ring, so that a new window at the address of a closed one (with the
   same title pointer - blitz2's requesters all have "") is seen as new.
   td_ncl counts the closes; entry k is td_cl[k % TD_NCL]. */
#define TD_NCL 16
struct Window *td_cl[TD_NCL];
volatile ULONG td_ncl;
APTR td_oldclose;
static ULONG td_sawcl;          /* closes handled by td_scan */
__asm__(
"   .text\n"
"   .even\n"
"_td_closepatch:\n"            /* CloseWindow(a0 window): first closed, then */
"   move.l  a0,-(sp)\n"           /* entered (td_scan must not find it in */
"   move.l  _td_oldclose,a1\n"    /* the list any more) */
"   jsr     (a1)\n"
"   move.l  (sp)+,a0\n"
"   move.l  _td_ncl,d0\n"
"   and.w   #15,d0\n"
"   lsl.w   #2,d0\n"
"   lea     _td_cl,a1\n"
"   move.l  a0,0(a1,d0.w)\n"
"   addq.l  #1,_td_ncl\n"
"   rts\n"
);
void td_closepatch(void);

/* Open is patched too: did TED open the file argument as it is, or split
   wrongly by a0toname (#340: it compares the caller's d0, blitz2's LoadSeg
   result, instead of the characters; low byte '/' turns "dir/name.bb"
   into "dir/name.b/b", ':' into no file at all)? */
static const char *td_farg;     /* the last argument, NULL: none */
static char td_fsplit[256];     /* its wrong split */
volatile int td_fseen;          /* 1: opened as it is, 2: split wrongly */
APTR td_oldopen;
__asm__(
"   .text\n"
"   .even\n"
"_td_openpatch:\n"             /* Open(d1 name, d2 mode) */
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _td_openc\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _td_oldopen,-(sp)\n"
"   rts\n"
);
void td_openpatch(void);
void td_openc(const char *name)
{
    if (!td_farg || !name) return;
    if (!strcmp(name, td_farg)) td_fseen |= 1;
    else if (!strcmp(name, td_fsplit)) td_fseen |= 2;
}

/* new windows of this tick, logged after UnlockIBase */
static struct tdwin td_new[4];
static WORD td_newpos[4][4];
static ULONG td_newidcmp[4], td_newflags[4];
static struct tdgad td_newgad[4][TD_MAXGAD];
static int td_newngad[4], td_newmore[4], td_nnew;

static int td_oldscreen(struct Screen *s)
{
    int i;
    for (i = 0; i < td_noldscr; i++)
        if (td_oldscr[i] == s) return 1;
    return 0;
}

/* copy the gadgets of w (IBase locked); returns the count */
static int td_snapgads(struct Window *w, struct tdgad *g, int max, int *more)
{
    struct Gadget *x;
    int n = 0;
    *more = 0;
    for (x = w->FirstGadget; x; x = x->NextGadget) {
        if (x->GadgetType & GTYP_SYSGADGET) continue;
        if (n >= max) { (*more)++; continue; }
        g[n].id = x->GadgetID;
        g[n].type = x->GadgetType;
        g[n].flags = x->Flags;
        g[n].act = x->Activation;
        g[n].l = x->LeftEdge;
        g[n].t = x->TopEdge;
        g[n].w = x->Width;
        g[n].h = x->Height;
        rc_copytitle(g[n].label, rc_label(x), sizeof g[n].label);
        g[n].buf[0] = 0;
        if ((x->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET && x->SpecialInfo)
            rc_copytitle(g[n].buf, ((struct StringInfo *)x->SpecialInfo)->Buffer, sizeof g[n].buf);
        n++;
    }
    return n;
}

/* follow the windows of the program (IBase locked); window events go to
   the log buffer, new windows with their gadgets to td_new */
static void td_scan(void)
{
    struct Screen *s;
    struct Window *w;
    int i;

    td_nnew = 0;
    if (td_scr) {
        for (s = IntuitionBase->FirstScreen; s && s != td_scr; s = s->NextScreen) ;
        if (!s) {
            rc_cat("screen closed\n");
            td_scr = 0;
            td_nw = 0;
        }
    }
    if (!td_scr) {
        for (s = IntuitionBase->FirstScreen; s && !td_scr; s = s->NextScreen) {
            if (td_oldscreen(s)) continue;
            for (w = s->FirstWindow; w; w = w->NextWindow)
                if (w->Flags & WFLG_BACKDROP) break;
            if (w) {
                char t[64];
                td_scr = s;
                rc_copytitle(t, s->Title, sizeof t);
                rc_cat("screen ");
                rc_hex((ULONG)s);
                rc_cat(" \"");
                rc_cat(t);
                rc_cat("\" ");
                rc_dec(s->Width); rc_cat("x"); rc_dec(s->Height);
                rc_cat(" depth ");
                rc_dec(s->RastPort.BitMap->Depth);
                rc_cat("\n");
            }
        }
        if (!td_scr) return;
    }
    /* windows that are gone (or reused with another title, or closed and
       opened again at the same address) */
    {
        ULONG n = td_ncl, k;
        if (n - td_sawcl > TD_NCL) td_sawcl = n - TD_NCL;
        for (k = td_sawcl; k != n; k++)
            for (i = 0; i < td_nw; i++)
                if (td_w[i].w == td_cl[k % TD_NCL] && td_w[i].since <= k)
                    td_w[i].title = (UBYTE *)-1;    /* closed: below */
        td_sawcl = n;
    }
    for (i = 0; i < td_nw; i++) {
        for (w = td_scr->FirstWindow; w && w != td_w[i].w; w = w->NextWindow) ;
        if (!w || w->Title != td_w[i].title) {
            rc_cat("window closed #");
            rc_dec(td_w[i].seq);
            rc_cat(" \"");
            rc_cat(td_w[i].text);
            rc_cat("\"\n");
            td_w[i--] = td_w[--td_nw];
        }
    }
    /* new windows */
    for (w = td_scr->FirstWindow; w; w = w->NextWindow) {
        for (i = 0; i < td_nw && td_w[i].w != w; i++) ;
        if (i < td_nw || td_nw >= TD_MAXWIN) continue;
        td_w[td_nw].w = w;
        td_w[td_nw].title = w->Title;
        td_w[td_nw].main = (w->Flags & WFLG_BACKDROP) != 0;
        td_w[td_nw].seq = ++td_seq;
        td_w[td_nw].since = td_ncl;
        rc_copytitle(td_w[td_nw].text, w->Title, sizeof td_w[0].text);
        if (td_nnew < 4) {
            td_new[td_nnew] = td_w[td_nw];
            td_newpos[td_nnew][0] = w->LeftEdge;
            td_newpos[td_nnew][1] = w->TopEdge;
            td_newpos[td_nnew][2] = w->Width;
            td_newpos[td_nnew][3] = w->Height;
            td_newidcmp[td_nnew] = w->IDCMPFlags;
            td_newflags[td_nnew] = w->Flags;
            td_newngad[td_nnew] = td_snapgads(w, td_newgad[td_nnew], TD_MAXGAD,
                                              &td_newmore[td_nnew]);
            td_nnew++;
        }
        td_nw++;
    }
}

/* log the new windows of the last scan (IBase unlocked) */
static void td_lognew(void)
{
    int i, j;
    for (i = 0; i < td_nnew; i++) {
        rc_cat("window new #");
        rc_dec(td_new[i].seq);
        rc_cat(td_new[i].main ? " main" : " requester");
        rc_cat(" \"");
        rc_cat(td_new[i].text);
        rc_cat("\" pos ");
        rc_dec(td_newpos[i][0]); rc_cat(","); rc_dec(td_newpos[i][1]);
        rc_cat(" size ");
        rc_dec(td_newpos[i][2]); rc_cat(","); rc_dec(td_newpos[i][3]);
        rc_cat(" flags ");
        rc_hex(td_newflags[i]);
        rc_cat(" idcmp ");
        rc_hex(td_newidcmp[i]);
        rc_cat(" gadgets ");
        rc_dec(td_newngad[i] + td_newmore[i]);
        rc_cat("\n");
        td_flush();
        for (j = 0; j < td_newngad[i]; j++) {
            struct tdgad *g = &td_newgad[i][j];
            rc_cat("  gadget id ");
            rc_dec(g->id);
            rc_cat(" type ");
            rc_hex(g->type);
            rc_cat(" flags ");
            rc_hex(g->flags);
            rc_cat(" act ");
            rc_hex(g->act);
            rc_cat(" pos ");
            rc_dec(g->l); rc_cat(","); rc_dec(g->t);
            rc_cat(" size ");
            rc_dec(g->w); rc_cat(","); rc_dec(g->h);
            rc_cat(" \"");
            rc_cat(g->label);
            rc_cat("\"");
            if ((g->type & GTYP_GTYPEMASK) == GTYP_STRGADGET) {
                rc_cat(" buffer \"");
                rc_cat(g->buf);
                rc_cat("\"");
            }
            rc_cat("\n");
            if (j % 8 == 7) td_flush();
        }
        if (td_newmore[i]) {
            rc_cat("  ... ");
            rc_dec(td_newmore[i]);
            rc_cat(" more\n");
        }
        td_flush();
    }
}

/* the window TED reads (IBase locked) */
static struct tdwin *td_target(void)
{
    struct tdwin *t = 0;
    int i;
    for (i = 0; i < td_nw; i++)
        if (!td_w[i].main && (!t || td_w[i].seq > t->seq)) t = &td_w[i];
    if (!t)
        for (i = 0; i < td_nw; i++)
            if (td_w[i].main) t = &td_w[i];
    return t;
}

static struct tdwin *td_find(struct Window *w, UBYTE *title)
{
    int i;
    for (i = 0; i < td_nw; i++)
        if (td_w[i].w == w && td_w[i].title == title) return &td_w[i];
    return 0;
}

/* ------------------------------------------------------------- idle */

static int td_msgsout(void)
{
    int i;
    for (i = 0; i < RC_NMSG; i++)
        if (rc_busy[i]) return 1;
    return 0;
}

static int td_isidle(struct Window *w, UBYTE *title)
{
    int idle = 0;
    ULONG lock;

    if (td_msgsout()) return 0;
    Forbid();
    lock = LockIBase(0);
    if (rc_valid(w) && w->Title == title && w->UserPort) {
        struct MsgPort *p = w->UserPort;
        struct Task *t = p->mp_SigTask;
        idle = t && t->tc_State == TS_WAIT
            && (t->tc_SigWait & (1UL << p->mp_SigBit))
            && IsListEmpty(&p->mp_MsgList);
    }
    UnlockIBase(lock);
    Permit();
    return idle;
}

/* ------------------------------------------------------------- sending */

/* a synthetic IntuiMessage into w's port (rc_send of reqclose.h with a
   qualifier); 1 if sent */
static int td_send(struct Window *w, UBYTE *title, ULONG class, UWORD code, UWORD qual, APTR addr)
{
    struct ExtIntuiMessage *em = 0;
    struct MsgPort *port = 0;
    ULONG lock;
    int i;

    for (i = 0; i < RC_NMSG; i++)
        if (!rc_busy[i]) { em = &rc_msg[i]; break; }
    if (!em) return 0;
    Forbid();
    lock = LockIBase(0);
    if (rc_valid(w) && w->Title == title)
        port = w->UserPort;
    UnlockIBase(lock);
    if (port) {
        struct IntuiMessage *m = &em->eim_IntuiMessage;
        memset(em, 0, sizeof *em);
        m->ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
        m->ExecMessage.mn_ReplyPort = rc_port;
        m->ExecMessage.mn_Length = sizeof *em;
        m->Class = class;
        m->Code = code;
        m->Qualifier = qual;
        m->IAddress = addr;
        m->IDCMPWindow = w;
        if (class == IDCMP_GADGETUP) {
            struct Gadget *g = addr;
            m->MouseX = g->LeftEdge + g->Width / 2;
            m->MouseY = g->TopEdge + g->Height / 2;
        }
        CurrentTime(&m->Seconds, &m->Micros);
        rc_busy[i] = 1;
        PutMsg(port, &m->ExecMessage);
    }
    Permit();
    return port != 0;
}

/* ------------------------------------------------------------- waiting */

enum { W_SCREEN, W_REQ, W_FREQ, W_GONE, W_IDLE, W_EXIT, W_SETTLE };

static LONG td_timeout = 15 * TD_TICKS;
static struct Window *td_reqw;          /* of the last wait req/freq */
static UBYTE *td_reqtitle;
static const char *td_why;              /* reason of the last failure */
static int td_settled;                  /* target idle, nothing sent since */

/* has w gadgets with all labels? (IBase locked) */
static int td_haslabels(struct Window *w, char **labels, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        struct Gadget *g;
        for (g = w->FirstGadget; g; g = g->NextGadget)
            if (!(g->GadgetType & GTYP_SYSGADGET) && td_labelmatch(rc_label(g), labels[i]))
                break;
        if (!g) return 0;
    }
    return 1;
}

/* wait for a condition; 1 = reached, 0 = failed (td_why) */
static int td_wait(int what, char **args, int nargs)
{
    LONG end = td_now() + td_timeout;
    int idles = 0;

    /* idle was just confirmed and nothing was sent since: TED still waits */
    if (what == W_IDLE && td_settled && !td_exited)
        return 1;
    td_settled = 0;
    for (;;) {
        struct Window *w = 0;
        UBYTE *title = 0;
        int cond = 0, needidle = 1, ismain = 0;
        ULONG lock;

        Delay(1);
        rc_replies();
        lock = LockIBase(0);
        td_scan();
        {
            struct tdwin *t = td_target();
            if (t) {
                w = t->w;
                title = t->title;
                ismain = t->main;
            }
            switch (what) {
            case W_SCREEN: cond = t && ismain; break;
            case W_REQ:
                cond = t && !ismain && td_haslabels(w, args, nargs);
                break;
            case W_FREQ:
                cond = t && !ismain && nargs > 0 && !strcmp(t->text, args[0]);
                break;
            case W_GONE: cond = t && (!td_reqw || !td_find(td_reqw, td_reqtitle)); break;
            case W_IDLE: cond = t != 0; break;
            case W_EXIT: cond = td_exited; needidle = 0; break;
            case W_SETTLE:
                /* after an input: idle again, or the program ended */
                if (td_exited) { cond = 1; needidle = 0; w = 0; }
                else cond = t != 0;
                break;
            }
        }
        UnlockIBase(lock);
        td_flush();
        td_lognew();
        if (cond && (!needidle || td_isidle(w, title))) {
            if (!needidle || ++idles >= 3) {
                if (what == W_REQ || what == W_FREQ) {
                    td_reqw = w;
                    td_reqtitle = title;
                }
                td_settled = needidle;
                return 1;
            }
        } else
            idles = 0;
        if (td_exited && what != W_EXIT && what != W_SETTLE) {
            td_why = "program exited";
            return 0;
        }
        if (td_now() > end) {
            td_why = "timeout";
            return 0;
        }
    }
}

/* the target of the input (after an idle wait): window and title */
static int td_gettarget(struct Window **w, UBYTE **title, int *ismain)
{
    ULONG lock = LockIBase(0);
    struct tdwin *t;
    td_scan();
    t = td_target();
    if (t) {
        *w = t->w;
        *title = t->title;
        if (ismain) *ismain = t->main;
    }
    UnlockIBase(lock);
    td_flush();
    td_lognew();
    return t != 0;
}

/* wait for the replies of our messages (bounded) */
static int td_replied(LONG ticks)
{
    LONG end = td_now() + ticks;
    while (td_msgsout()) {
        if (td_now() > end) return 0;
        Delay(1);
        rc_replies();
    }
    return 1;
}

/* send to the current target, then wait: reply, idle or exit */
static int td_input(ULONG class, UWORD code, UWORD qual, APTR addr, struct Window *w, UBYTE *title)
{
    struct Window *tw = w;
    UBYTE *tt = title;
    if (!tw && !td_gettarget(&tw, &tt, 0)) {
        td_why = "no window";
        return 0;
    }
    td_settled = 0;
    if (!td_send(tw, tt, class, code, qual, addr)) {
        td_why = "window gone or no message buffer";
        return 0;
    }
    if (!td_replied(td_timeout)) {
        td_why = "message not replied";
        return 0;
    }
    td_flush();
    return td_wait(W_SETTLE, 0, 0);
}

/* ------------------------------------------------------------- commands */

/* gadget with this ID in the target window (IBase locked) */
static struct Gadget *td_gadget(struct Window *w, UWORD id)
{
    struct Gadget *g;
    for (g = w->FirstGadget; g; g = g->NextGadget)
        if (!(g->GadgetType & GTYP_SYSGADGET) && g->GadgetID == id)
            return g;
    return 0;
}

static struct Gadget *td_labelgad(struct Window *w, const char *label)
{
    struct Gadget *g;
    for (g = w->FirstGadget; g; g = g->NextGadget)
        if (!(g->GadgetType & GTYP_SYSGADGET) && td_labelmatch(rc_label(g), label))
            return g;
    return 0;
}

/* diagnostics: the buffers of all string gadgets of w (and which one is
   active, GFLG_SELECTED) into the log */
static void td_logstrings(struct Window *w, UBYTE *title)
{
    struct tdgad g[8];
    int n = 0, more, i;
    ULONG lock = LockIBase(0);
    if (rc_valid(w) && w->Title == title) {
        struct tdgad all[TD_MAXGAD];
        int k = td_snapgads(w, all, TD_MAXGAD, &more);
        for (i = 0; i < k && n < 8; i++)
            if ((all[i].type & GTYP_GTYPEMASK) == GTYP_STRGADGET) g[n++] = all[i];
    }
    UnlockIBase(lock);
    for (i = 0; i < n; i++) {
        rc_cat("  string gadget pos ");
        rc_dec(g[i].l); rc_cat(","); rc_dec(g[i].t);
        rc_cat(g[i].flags & GFLG_SELECTED ? " (active)" : "");
        rc_cat(" buffer \"");
        rc_cat(g[i].buf);
        rc_cat("\"\n");
    }
    td_flush();
}

/* fill a string gadget (Forbid + LockIBase), then RefreshGList */
static int td_str(struct Window *w, UBYTE *title, struct Gadget *g, const char *text)
{
    ULONG lock;
    int ok = 0;
    Forbid();
    lock = LockIBase(0);
    if (rc_valid(w) && w->Title == title
        && (g->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET && g->SpecialInfo) {
        struct StringInfo *si = g->SpecialInfo;
        int n = strlen(text);
        if (si->MaxChars > 0 && n > si->MaxChars - 1) n = si->MaxChars - 1;
        memcpy(si->Buffer, text, n);
        si->Buffer[n] = 0;
        si->NumChars = n;
        si->BufferPos = 0;
        si->DispPos = 0;
        if (g->Activation & GACT_LONGINT)
            si->LongInt = td_number(text, 10);
        ok = 1;
    }
    UnlockIBase(lock);
    Permit();
    if (ok)
        RefreshGList(g, w, NULL, 1);
    else
        td_why = "no string gadget";
    return ok;
}

/* the screen into PREFIX NAME.scr */
static int td_dump(const char *name)
{
    ULONG lock, size = 0;
    UBYTE *buf = 0;
    struct Screen *s;
    struct BitMap *bm;
    UWORD width = 0, height = 0, bpr = 0, rows = 0, rb = 0, depth = 0;
    ULONG modeid = 0, memtype = 0;
    WORD mx = 0, my = 0;
    int il = 0, nwin = 0, i, p, r;
    char title[80];
    static WORD wpos[TD_MAXWIN][4];
    static ULONG wflags[TD_MAXWIN];
    static char wtitle[TD_MAXWIN][40];
    char path[240];
    BPTR fh;

    if (!td_wait(W_IDLE, 0, 0))
        return 0;
    /* sizes */
    lock = LockIBase(0);
    td_scan();
    s = td_scr;
    if (s) {
        bm = s->RastPort.BitMap;
        depth = bm->Depth;
        bpr = bm->BytesPerRow;
        rows = bm->Rows;
        width = s->Width;
        rb = (width + 15) / 16 * 2;
        if (rb > bpr) rb = bpr;
        size = (ULONG)depth * rows * rb;
    }
    UnlockIBase(lock);
    if (!s || !size) {
        td_why = "no screen";
        return 0;
    }
    buf = AllocMem(size, MEMF_ANY);
    if (!buf) {
        td_why = "no memory for the dump";
        return 0;
    }
    /* copy: TED is idle, its task waits; nothing else draws there */
    Forbid();
    lock = LockIBase(0);
    td_scan();
    if (td_scr == s && s->RastPort.BitMap->Depth == depth && s->RastPort.BitMap->BytesPerRow == bpr
        && s->RastPort.BitMap->Rows == rows && s->Width == width) {
        struct Window *w;
        bm = s->RastPort.BitMap;
        height = s->Height;
        mx = s->MouseX;
        my = s->MouseY;
        modeid = GetVPModeID(&s->ViewPort);
        memtype = TypeOfMem(bm->Planes[0]);
        il = depth > 1 && (ULONG)bm->Planes[1] - (ULONG)bm->Planes[0] == rb;
        rc_copytitle(title, s->Title, sizeof title);
        for (w = s->FirstWindow; w && nwin < TD_MAXWIN; w = w->NextWindow, nwin++) {
            wpos[nwin][0] = w->LeftEdge;
            wpos[nwin][1] = w->TopEdge;
            wpos[nwin][2] = w->Width;
            wpos[nwin][3] = w->Height;
            wflags[nwin] = w->Flags;
            rc_copytitle(wtitle[nwin], w->Title, sizeof wtitle[0]);
        }
        for (p = 0; p < depth; p++)
            for (r = 0; r < rows; r++)
                memcpy(buf + ((ULONG)p * rows + r) * rb, bm->Planes[p] + (ULONG)r * bpr, rb);
    } else
        s = 0;
    UnlockIBase(lock);
    Permit();
    td_flush();
    if (!s) {
        FreeMem(buf, size);
        td_why = "screen changed while dumping";
        return 0;
    }
    /* header */
    rc_lb[0] = 0;
    strcpy(path, td_prefix);
    strcat(path, name);
    strcat(path, ".scr");
    fh = Open((CONST_STRPTR)path, MODE_NEWFILE);
    if (!fh) {
        FreeMem(buf, size);
        td_why = "cannot write the dump";
        return 0;
    }
#define HDR(k, v) do { rc_cat(k " "); rc_dec(v); rc_cat("\n"); } while (0)
#define HDRX(k, v) do { rc_cat(k " "); rc_hex(v); rc_cat("\n"); } while (0)
    rc_cat("magic TEDSCR1\n");
    HDR("width", width);
    HDR("height", height);
    HDR("depth", depth);
    HDR("bytesperrow", bpr);
    HDR("rows", rows);
    HDR("rowbytes", rb);
    HDR("interleaved", il);
    HDRX("modeid", modeid);
    HDR("mousex", mx);
    HDR("mousey", my);
    HDRX("memtype", memtype);
    rc_cat("title ");
    rc_cat(title);
    rc_cat("\n");
    HDR("windows", nwin);
    Write(fh, rc_lb, strlen(rc_lb));
    rc_lb[0] = 0;
    for (i = 0; i < nwin; i++) {
        rc_cat("window ");
        rc_dec(wpos[i][0]); rc_cat(" ");
        rc_dec(wpos[i][1]); rc_cat(" ");
        rc_dec(wpos[i][2]); rc_cat(" ");
        rc_dec(wpos[i][3]); rc_cat(" ");
        rc_hex(wflags[i]);
        rc_cat(" ");
        rc_cat(wtitle[i]);
        rc_cat("\n");
        Write(fh, rc_lb, strlen(rc_lb));
        rc_lb[0] = 0;
    }
#undef HDR
#undef HDRX
    Write(fh, "data\n", 5);
    r = Write(fh, buf, size) == (LONG)size;
    Close(fh);
    FreeMem(buf, size);
    rc_cat("  dump ");
    rc_cat(path);
    rc_cat(" ");
    rc_dec(width); rc_cat("x"); rc_dec(height); rc_cat("x"); rc_dec(depth);
    rc_cat("\n");
    td_flush();
    if (!r) td_why = "dump write error";
    return r;
}

/* ------------------------------------------------------------- script */

static int td_step;

/* one command; 1 ok, 0 failed (td_why) */
static int td_command(int ac, char **av)
{
    struct Window *w = 0;
    UBYTE *title = 0;
    int ismain = 0;

    td_why = "bad command";
    if (!strcmp(av[0], "timeout") && ac == 2) {
        td_timeout = td_number(av[1], 10) * TD_TICKS;
        return 1;
    }
    if (!strcmp(av[0], "log")) {
        return 1;               /* the step line is the log */
    }
    if (!strcmp(av[0], "wait") && ac >= 2) {
        if (!strcmp(av[1], "screen")) return td_wait(W_SCREEN, 0, 0);
        if (!strcmp(av[1], "req") && ac >= 3) return td_wait(W_REQ, av + 2, ac - 2);
        if (!strcmp(av[1], "freq") && ac == 3) return td_wait(W_FREQ, av + 2, 1);
        if (!strcmp(av[1], "gone")) return td_wait(W_GONE, 0, 0);
        if (!strcmp(av[1], "idle")) return td_wait(W_IDLE, 0, 0);
        if (!strcmp(av[1], "exit")) return td_wait(W_EXIT, 0, 0);
        return 0;
    }
    if (!strcmp(av[0], "key") && (ac == 2 || ac == 3)) {
        if (!td_wait(W_IDLE, 0, 0)) return 0;
        return td_input(IDCMP_RAWKEY, (UWORD)td_number(av[1], 16),
                        ac == 3 ? (UWORD)td_number(av[2], 16) : 0, &rc_prevkeys, 0, 0);
    }
    if (!strcmp(av[0], "type") && ac == 2) {
        const UBYTE *p;
        for (p = (const UBYTE *)av[1]; *p; p++) {
            UBYTE ev[8];
            UWORD code, qual;
            if (*p == '\n') code = 0x44, qual = 0;
            else if (*p == '\t') code = 0x42, qual = 0;
            else {
                LONG n = MapANSI((CONST_STRPTR)p, 1, (STRPTR)ev, 4, NULL);
                if (n != 1) {
                    td_why = "character needs more than one key (MapANSI)";
                    return 0;
                }
                code = ev[0];
                qual = ev[1];
            }
            if (!td_wait(W_IDLE, 0, 0)) return 0;
            if (!td_input(IDCMP_RAWKEY, code, qual, &rc_prevkeys, 0, 0)) return 0;
        }
        return 1;
    }
    if (!strcmp(av[0], "menu") && (ac == 3 || ac == 4)) {
        UWORD m = td_number(av[1], 10), i = td_number(av[2], 10);
        UWORD s = ac == 4 ? td_number(av[3], 10) : 31;
        if (!td_wait(W_IDLE, 0, 0)) return 0;
        return td_input(IDCMP_MENUPICK, (m & 31) | (i & 63) << 5 | (s & 31) << 11, 0, 0, 0, 0);
    }
    if ((!strcmp(av[0], "gad") && ac == 2) || (!strcmp(av[0], "str") && ac == 3)) {
        struct Gadget *g = 0;
        ULONG lock;
        if (!td_wait(W_IDLE, 0, 0)) return 0;
        lock = LockIBase(0);
        td_scan();
        {
            struct tdwin *t = td_target();
            if (t) {
                w = t->w;
                title = t->title;
                g = td_gadget(w, (UWORD)td_number(av[1], 10));
            }
        }
        UnlockIBase(lock);
        td_flush();
        td_lognew();
        if (!g) {
            td_why = "no such gadget in the target window";
            return 0;
        }
        if (av[0][0] == 's') {
            int r = td_str(w, title, g, av[2]);
            td_logstrings(w, title);
            return r;
        }
        return td_input(IDCMP_GADGETUP, 0, 0, g, w, title);
    }
    if (!strcmp(av[0], "freq") && ac >= 2 && ac <= 4) {
        struct Gadget *g = 0, *sg = 0, *pg = 0, *x;
        int ok = !strcmp(av[1], "ok");
        ULONG lock;
        if (!ok && strcmp(av[1], "cancel")) return 0;
        if (!td_wait(W_IDLE, 0, 0)) return 0;
        lock = LockIBase(0);
        td_scan();
        {
            struct tdwin *t = td_target();
            if (t && !t->main) {
                w = t->w;
                title = t->title;
                g = td_labelgad(w, ok ? "OK" : "Cancel");
                /* the file name gadget: the lowest string gadget; the
                   path gadget: the highest one */
                for (x = w->FirstGadget; x; x = x->NextGadget)
                    if ((x->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET) {
                        if (!sg || x->TopEdge > sg->TopEdge) sg = x;
                        if (!pg || x->TopEdge < pg->TopEdge) pg = x;
                    }
            }
        }
        UnlockIBase(lock);
        td_flush();
        td_lognew();
        if (!w) {
            td_why = "no requester";
            return 0;
        }
        if (ac >= 3) {
            if (!sg || (ac == 4 && pg == sg)) {
                td_why = "no file/path string gadgets in the file requester";
                return 0;
            }
            if (!td_str(w, title, sg, av[2])) return 0;
            if (ac == 4 && !td_str(w, title, pg, av[3])) return 0;
        }
        td_logstrings(w, title);
        if (g)
            return td_input(IDCMP_GADGETUP, 0, 0, g, w, title);
        if (ok) {
            td_why = "no OK gadget";
            return 0;
        }
        return td_input(IDCMP_CLOSEWINDOW, 0, 0, w, w, title);
    }
    if (!strcmp(av[0], "dump") && ac == 2)
        return td_dump(av[1]);
    if (!strcmp(av[0], "mem") && ac == 2) {
        rc_cat("  mem ");
        rc_cat(av[1]);
        rc_cat(" any ");
        rc_dec(AvailMem(MEMF_ANY));
        rc_cat(" chip ");
        rc_dec(AvailMem(MEMF_CHIP));
        rc_cat(" largest ");
        rc_dec(AvailMem(MEMF_ANY | MEMF_LARGEST));
        rc_cat("\n");
        td_flush();
        return 1;
    }
    (void)ismain;
    return 0;
}

/* end the program after a failure (see the head comment) */
static void td_rescue(void)
{
    LONG end = td_now() + 15 * TD_TICKS;
    LONG save = td_timeout;
    int round;

    td_timeout = 3 * TD_TICKS;
    for (round = 0; round < 8 && !td_exited && td_now() < end; round++) {
        struct Window *w = 0;
        UBYTE *title = 0;
        struct Gadget *g = 0;
        int ismain = 0, ok;
        ULONG lock;

        if (!td_wait(W_IDLE, 0, 0)) {
            td_log("rescue: no idle window: ", td_why);
            break;
        }
        lock = LockIBase(0);
        td_scan();
        {
            struct tdwin *t = td_target();
            if (t) {
                w = t->w;
                title = t->title;
                ismain = t->main;
                if (!ismain && title)
                    g = td_labelgad(w, "Cancel");
                else if (!ismain) {
                    g = td_gadget(w, 1);
                    if (g && !(g->Activation & GACT_ENDGADGET)) g = 0;
                }
            }
        }
        UnlockIBase(lock);
        td_flush();
        td_lognew();
        if (!w) break;
        if (ismain) {
            td_log("rescue: menu QUIT", 0);
            ok = td_input(IDCMP_MENUPICK, 0 | 9 << 5 | 31 << 11, 0, 0, w, title);
        } else if (title) {
            td_log("rescue: file requester Cancel", 0);
            ok = g ? td_input(IDCMP_GADGETUP, 0, 0, g, w, title)
                   : td_input(IDCMP_CLOSEWINDOW, 0, 0, w, w, title);
        } else if (g) {
            td_log("rescue: gadget 1", 0);
            ok = td_input(IDCMP_GADGETUP, 0, 0, g, w, title);
        } else {
            td_log("rescue: Esc", 0);
            ok = td_input(IDCMP_RAWKEY, 0x45, 0, &rc_prevkeys, w, title);
        }
        if (!ok) td_log("rescue: ", td_why);
    }
    if (!td_exited) {
        td_timeout = 3 * TD_TICKS;
        td_wait(W_EXIT, 0, 0);
    }
    td_log(td_exited ? "rescue: program ended" : "rescue: program still running", 0);
    td_timeout = save;
}

/* the driver process */
static void td_main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    char *line, *next;
    int failed = 0;

    me->pr_WindowPtr = (APTR)-1;    /* no DOS requesters from here */
    rc_port = CreateMsgPort();
    td_t0 = td_now();
    if (!rc_port) {
        td_writefile("result", "FAIL step 0 no message port\n");
        goto end;
    }
    for (line = td_script; line && !failed; line = next) {
        char *av[24], copy[300];
        int ac, ok, expectfail = 0;
        next = strchr(line, '\n');
        if (next) *next++ = 0;
        strncpy(copy, line, sizeof copy - 1);
        copy[sizeof copy - 1] = 0;
        ac = td_split(copy, av, 24);
        if (!ac) continue;
        td_step++;
        rc_cat("step ");
        rc_dec(td_step);
        rc_cat(" ");
        td_time();
        rc_cat(": ");
        rc_cat(line);
        rc_cat("\n");
        td_flush();
        if (!strcmp(av[0], "expectfail") && ac > 1) {
            expectfail = 1;
            ac--;
            memmove(av, av + 1, ac * sizeof av[0]);
        }
        ok = td_command(ac, av);
        rc_cat("  ");
        if (expectfail) {
            rc_cat(ok ? "succeeded, expected to fail " : "failed as expected: ");
            if (!ok) { rc_cat(td_why); rc_cat(" "); }
            ok = !ok;
            if (!ok) td_why = "expected failure did not happen";
        } else if (ok)
            rc_cat("ok ");
        else {
            rc_cat("FAILED: ");
            rc_cat(td_why);
            rc_cat(" ");
        }
        td_time();
        rc_cat("\n");
        td_flush();
        if (!ok) {
            char r[160];
            strcpy(r, "FAIL step ");
            {
                char *p = r + strlen(r);
                LONG v = td_step;
                char b[12], *q = b + 11;
                *q = 0;
                do { *--q = '0' + v % 10; v /= 10; } while (v);
                strcpy(p, q);
            }
            strcat(r, " ");
            strncat(r, td_why, 100);
            if (td_farg && (td_fseen & 2))
                strcat(r, " (#340: TED opened the file name split wrongly)");
            else if (td_farg && strpbrk(td_farg, ":/") && td_seq && !(td_fseen & 1))
                strcat(r, " (#340: TED did not open the file)");
            strcat(r, "\n");
            td_writefile("result", r);
            failed = 1;
        }
    }
    if (!failed) {
        if (!td_exited) {
            td_log("script ended while the program runs", 0);
            td_writefile("result", "FAIL end script ended while the program runs\n");
            failed = 1;
        } else {
            td_writefile("result", "OK\n");
            td_writefile("ok", "OK\n");    /* marker for the session script (If EXISTS) */
        }
    }
    if (failed && !td_exited)
        td_rescue();
    if (td_exited) {
        rc_cat("program returned rc=");
        rc_dec(td_rc);
        rc_cat(" ");
        td_time();
        rc_cat("\n");
        td_flush();
    }
    /* replies of messages still out (bounded); a port that may still get
       a reply is never deleted (leaked: a ReplyMsg to a freed port would
       crash the system) */
    if (td_replied(2 * TD_TICKS)) {
        DeleteMsgPort(rc_port);
        rc_port = 0;
    } else
        td_log("messages still unreplied at the end: reply port leaked", 0);
end:
    Forbid();                       /* exits before the parent runs */
    Signal(td_parent, 1UL << td_donesig);
}

static int td_readscript(const char *path)
{
    BPTR fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
    LONG n;
    if (!fh) return 0;
    n = Read(fh, td_script, sizeof td_script - 1);
    Close(fh);
    if (n < 0) return 0;
    td_script[n] = 0;
    return 1;
}

static const char usage[] =
    "usage: teddrive SCRIPT PREFIX COMMAND [ARGS]\n"
    "  run COMMAND and play SCRIPT against its windows (TED A/B tests)\n";

int main(int argc, char **argv)
{
    struct CommandLineInterface *cli = Cli();
    struct Screen *s;
    ULONG lock;
    LONG rc;
    int i;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    if (argc < 4) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 20;
    }
    strncpy(td_prefix, argv[2], sizeof td_prefix - 1);
    strcpy(td_logpath, td_prefix);
    strcat(td_logpath, "log");
    td_cmd[0] = 0;
    for (i = 3; i < argc; i++) {
        if (strlen(td_cmd) + strlen(argv[i]) + 2 >= sizeof td_cmd) return 20;
        if (i > 3) strcat(td_cmd, " ");
        strcat(td_cmd, argv[i]);
    }
    out = Output();
    td_log("teddrive start: ", td_cmd);
    if (argc > 4 && argv[argc - 1][0] != '"' && strlen(argv[argc - 1]) >= 2
        && strlen(argv[argc - 1]) < sizeof td_fsplit - 1) {
        size_t n = strlen(argv[argc - 1]);
        td_farg = argv[argc - 1];
        memcpy(td_fsplit, td_farg, n - 1);
        td_fsplit[n - 1] = '/';
        td_fsplit[n] = td_farg[n - 1];
        td_fsplit[n + 1] = 0;
    }
    td_log("script: ", argv[1]);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    KeymapBase = OpenLibrary((CONST_STRPTR)"keymap.library", 37);
    if (!IntuitionBase || !GfxBase || !KeymapBase || !td_readscript(argv[1])) {
        td_log("FATAL: libraries or script missing", 0);
        td_writefile("result", "FAIL step 0 libraries or script missing\n");
        return 20;
    }
    /* the program's stack: Stack of the calling shell (SystemTags) */
    if (cli) {
        if (cli->cli_DefaultStack < 40000 / 4) cli->cli_DefaultStack = 40000 / 4;
        rc_cat("stack ");
        rc_dec(cli->cli_DefaultStack * 4);
        rc_cat("\n");
        td_flush();
    }
    /* screens of the start (the program's screen is a new one) */
    lock = LockIBase(0);
    for (s = IntuitionBase->FirstScreen; s && td_noldscr < TD_MAXSCR; s = s->NextScreen)
        td_oldscr[td_noldscr++] = s;
    UnlockIBase(lock);
    td_parent = FindTask(NULL);
    td_donesig = AllocSignal(-1);
    if (td_donesig < 0
        || !CreateNewProcTags(NP_Entry, (ULONG)td_main, NP_Name, (ULONG)"teddrive",
                              NP_Priority, 1, NP_StackSize, 16384, TAG_END)) {
        td_log("FATAL: no driver process", 0);
        td_writefile("result", "FAIL step 0 no driver process\n");
        return 20;
    }
    Forbid();
    td_oldclose = SetFunction((struct Library *)IntuitionBase, -72, (APTR)td_closepatch);
    td_oldopen = SetFunction((struct Library *)DOSBase, -30, (APTR)td_openpatch);
    CacheClearU();
    Permit();
    rc = SystemTags((CONST_STRPTR)td_cmd, SYS_Input, Input(), SYS_Output, out, SYS_Asynch, FALSE,
                    NP_CloseInput, FALSE, NP_CloseOutput, FALSE, TAG_END);
    td_rc = rc;
    td_exited = 1;
    Wait(1UL << td_donesig);        /* the driver ends (all its waits are bounded) */
    Forbid();
    SetFunction((struct Library *)IntuitionBase, -72, td_oldclose);
    SetFunction((struct Library *)DOSBase, -30, td_oldopen);
    CacheClearU();
    Permit();
    Delay(10);                      /* a CloseWindow still inside returns to us */
    FreeSignal(td_donesig);
    td_log("teddrive end", 0);
    CloseLibrary(KeymapBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return rc < 0 ? 20 : rc;
}
