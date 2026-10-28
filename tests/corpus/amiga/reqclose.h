/*
 * reqclose.h - the requester closer of `reqclose` (reqtrap.c compiled with
 * -DREQCLOSE).  A second process watches the windows of all screens and
 * answers modal requesters (ASL, the Blitz file requester ...) so that a
 * batch test can call blocking requester functions:
 *
 *   ENV:REQCLOSE   lines "ANSWER PATTERN" (or separated by ';'):
 *                    ANSWER  = ok | cancel | ignore, ok/cancel optionally
 *                              with "#ID": GADGETUP on the gadget with
 *                              that GadgetID (e.g. "ok#2 my window")
 *                    PATTERN = window title; "abc*" = prefix; "*" = any
 *                              window that appeared after the start
 *                  first matching line wins.  Without the variable, or for
 *                  windows no line matches, nothing is done.
 *
 * Candidates: windows that were not there when the closer started
 * (windows of the start are recognised by their pointer only, so a shell
 * window that changes its title is never touched).  Every later window is
 * looked at once (pointer + title pointer); an answered window that closed
 * is forgotten, so a reused window structure counts as new.  Every 40 ms
 * (Delay(2)) the closer locks IntuitionBase (LockIBase), looks the window
 * up again in the screens' window lists and reads what it needs while
 * locked; a window that is gone is never touched.  The variable is read
 * when a candidate appears.
 *
 * The answer: synthetic IntuiMessages put into the window's UserPort, as
 * if Intuition had sent them (no input.device: injected input events froze
 * the input task on the RTG Workbench of the test emulator).  Sending:
 * Forbid, LockIBase, re-validate the window, take its UserPort,
 * UnlockIBase, PutMsg, Permit - the owner cannot close the window in
 * between.  The messages are static buffers with the closer's own reply
 * port (IDCMPWindow = the window, so CloseWindow returns them if they are
 * still pending); a buffer is used again only after its reply came back.
 * Steps (each 2 s after the previous, while the window is still there; the
 * first 0.4 s after it appeared):
 *   ok:     GADGETUP on the gadget "OK"/"Open"/"Save"/"Use", then the
 *           cancel steps
 *   cancel: GADGETUP on "Cancel", CLOSEWINDOW, RAWKEY Esc, VANILLAKEY Esc
 * Gadgets are found by their IntuiText (GadgetText->IText, case-
 * insensitive).  GadTools buttons (GadgetType $0101) have no GadgetText
 * (GadTools draws the label into its frame image), so without a label
 * the "button row" rule applies: of the enabled GadTools buttons in the
 * lowest row (largest TopEdge, GFLG_RELBOTTOM resolved) ok takes the
 * leftmost, cancel the rightmost.  With "#ID" only the GadgetID counts.
 * A step without a gadget is skipped.  The choice is logged.
 * Output (to reqclose's stdout, the program's .run file), one line each,
 * only while the program waits in its requester (so the order relative to
 * the program's output is fixed; "window gone" is in the log only):
 *   REQCLOSE: "title" -> ok|cancel      decision for a new window
 *   REQCLOSE: "title" step NAME         a message sent
 *   REQCLOSE: "title" gave up           after the last step
 * Compile with -DREQCLOSE_INPUT for the old method (clicks and keys via
 * input.device IND_WRITEEVENT) - not used by default.
 */
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#ifdef REQCLOSE_INPUT
#include <devices/input.h>
#endif

#define RC_MAXOLD 64
#define RC_STEP0  10            /* ticks (40 ms) before the first step */
#define RC_STEPN  50            /* ticks between steps */
#define RC_NMSG   6

enum { ST_OK, ST_CANCEL, ST_CLOSE, ST_RAWESC, ST_VANESC, ST_END };
static const char *const rc_stepname[] = {
    "step gadget OK", "step gadget Cancel", "step CLOSEWINDOW",
    "step RAWKEY Esc", "step VANILLAKEY Esc"
};

static struct Task *rc_parent;
static const char *rc_cmd = "";   /* the command (log) */
static volatile int rc_stop;
static BYTE rc_donesig = -1;

struct rc_old {
    struct Window *w;
    UBYTE *title;               /* title pointer when first seen */
    char text[40];              /* and its text */
    int snap;                   /* there at the start: pointer only */
    int seen;                   /* still in the lists (this tick) */
};
static struct rc_old rc_olds[RC_MAXOLD];
static LONG rc_gid = -1;        /* "#ID" of the answer, -1 none */
static int rc_gstep;             /* the step the #ID is for */
static const char *rc_why = "";  /* how the gadget was found (log) */

#define RC_GADTOOLS 0x0100      /* GadgetType bit of GadTools gadgets */
static int rc_nold;

/* diagnostics: $REQCLOSELOG or T:reqclose.log (closer process only) */
static BPTR rc_logfh;
static char rc_lb[1600];        /* collected while IBase is locked */

static void rc_cat(const char *t) { strncat(rc_lb, t ? t : "(null)", sizeof rc_lb - strlen(rc_lb) - 2); }

static void rc_hex(ULONG v)
{
    char b[10];
    int i;
    for (i = 7; i >= 0; i--, v >>= 4) b[i] = "0123456789ABCDEF"[v & 15];
    b[8] = 0;
    rc_cat("$");
    rc_cat(b);
}

static void rc_dec(LONG v)
{
    char b[12], *p = b + 11;
    int neg = v < 0;
    ULONG u = neg ? -v : v;
    *p = 0;
    do { *--p = '0' + u % 10; u /= 10; } while (u);
    if (neg) *--p = '-';
    rc_cat(p);
}

/* write the collected text to the log (IBase must NOT be locked) */
static void rc_flush(void)
{
    if (rc_lb[0] && rc_logfh)
        Write(rc_logfh, rc_lb, strlen(rc_lb));
    rc_lb[0] = 0;
}

static void rc_copytitle(char *d, const UBYTE *t, int n)
{
    d[0] = 0;
    if (t) {
        strncpy(d, (const char *)t, n - 1);
        d[n - 1] = 0;
    }
}

/* is w a window seen before?  windows of the start by pointer, later
   ones by pointer, title pointer and title text */
static int rc_isold(struct Window *w)
{
    char t[40];
    int i;
    rc_copytitle(t, w->Title, sizeof t);
    for (i = 0; i < rc_nold; i++)
        if (rc_olds[i].w == w &&
            (rc_olds[i].snap || (rc_olds[i].title == w->Title && !strcmp(rc_olds[i].text, t))))
            return 1;
    return 0;
}

static void rc_remember(struct Window *w, int snap)
{
    if (rc_nold >= RC_MAXOLD) return;
    rc_olds[rc_nold].w = w;
    rc_olds[rc_nold].title = w->Title;
    rc_copytitle(rc_olds[rc_nold].text, w->Title, sizeof rc_olds[0].text);
    rc_olds[rc_nold].snap = snap;
    rc_olds[rc_nold].seen = 1;
    rc_nold++;
}

/* forget later windows that are no longer in any window list, so a
   reused window structure counts as new (IBase locked) */
static void rc_prune(void)
{
    struct Screen *s;
    struct Window *w;
    int i;
    for (i = 0; i < rc_nold; i++) rc_olds[i].seen = 0;
    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        for (w = s->FirstWindow; w; w = w->NextWindow)
            for (i = 0; i < rc_nold; i++)
                if (rc_olds[i].w == w) rc_olds[i].seen = 1;
    for (i = 0; i < rc_nold; i++)
        if (!rc_olds[i].seen && !rc_olds[i].snap) {
            rc_cat("forget ");
            rc_hex((ULONG)rc_olds[i].w);
            rc_cat(" \"");
            rc_cat(rc_olds[i].text);
            rc_cat("\"\n");
            rc_olds[i--] = rc_olds[--rc_nold];
        }
}

static struct MsgPort *rc_port;
static struct ExtIntuiMessage rc_msg[RC_NMSG];
static int rc_busy[RC_NMSG];
static ULONG rc_prevkeys;       /* RAWKEY IAddress: no previous keys */

static int rc_lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

/* is w (still) a window of some screen?  call with IBase locked */
static int rc_valid(struct Window *w)
{
    struct Screen *s;
    struct Window *x;
    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        for (x = s->FirstWindow; x; x = x->NextWindow)
            if (x == w) return 1;
    return 0;
}

/* the label of a gadget: IntuiText or plain string (GadgetText) */
static const UBYTE *rc_label(struct Gadget *g)
{
    if (!g->GadgetText) return 0;
    switch (g->Flags & GFLG_LABELMASK) {
    case GFLG_LABELITEXT:  return g->GadgetText->IText;
    case GFLG_LABELSTRING: return (const UBYTE *)g->GadgetText;
    }
    return 0;
}

/* find a gadget: labels for ST_OK/ST_CANCEL (IBase locked); the label
   may contain "_" (GadTools underscore) - it is skipped */
static int rc_labeleq(const UBYTE *a, const char *b)
{
    if (!a) return 0;
    for (;;) {
        if (*a == '_') { a++; continue; }
        if (!*a || !*b) return *a == 0 && *b == 0;
        if (rc_lower(*a++) != rc_lower(*b++)) return 0;
    }
}

/* an enabled GadTools button (GadgetType $0101, no system gadget) */
static int rc_button(struct Gadget *g)
{
    return !(g->GadgetType & GTYP_SYSGADGET)
        && (g->GadgetType & (RC_GADTOOLS | GTYP_GTYPEMASK)) == (RC_GADTOOLS | GTYP_BOOLGADGET)
        && !(g->Flags & GFLG_DISABLED);
}

static struct Gadget *rc_gadget(struct Window *w, int step)
{
    static const char *const oks[] = { "OK", "Open", "Save", "Use", 0 };
    static const char *const cancels[] = { "Cancel", 0 };
    const char *const *names = step == ST_OK ? oks : cancels;
    struct Gadget *g, *best = 0;
    WORD bt = 0, bl = 0;
    int i, any = 0;

    if (rc_gid >= 0 && step == rc_gstep) {
        rc_why = "id";
        for (g = w->FirstGadget; g; g = g->NextGadget)
            if (!(g->GadgetType & GTYP_SYSGADGET) && g->GadgetID == rc_gid
                && !(g->Flags & GFLG_DISABLED))
                return g;
        return 0;
    }
    rc_why = "label";
    for (g = w->FirstGadget; g; g = g->NextGadget) {
        if (g->Flags & GFLG_DISABLED) continue;
        for (i = 0; names[i]; i++)
            if (rc_labeleq(rc_label(g), names[i])) return g;
    }
    /* button row: lowest row of GadTools buttons, leftmost / rightmost */
    rc_why = "row";
    for (g = w->FirstGadget; g; g = g->NextGadget)
        if (rc_button(g)) {
            WORD t = g->Flags & GFLG_RELBOTTOM ? w->Height + g->TopEdge : g->TopEdge;
            if (!any || t > bt) bt = t;          /* the lowest row */
            any = 1;
        }
    for (g = w->FirstGadget; g; g = g->NextGadget)
        if (rc_button(g)) {
            WORD t = g->Flags & GFLG_RELBOTTOM ? w->Height + g->TopEdge : g->TopEdge;
            WORD l = g->Flags & GFLG_RELRIGHT ? w->Width + g->LeftEdge : g->LeftEdge;
            if (t >= bt - 2 && (!best || (step == ST_OK ? l < bl : l > bl)))
                best = g, bl = l;
        }
    return best;
}

/* gadget dump into the log text (IBase locked) */
static void rc_dumpgadgets(struct Window *w)
{
    struct Gadget *g;
    int n = 0;
    for (g = w->FirstGadget; g && n < 40; g = g->NextGadget, n++) {
        const UBYTE *l = rc_label(g);
        char t[24];
        rc_cat("  gadget ");
        rc_hex((ULONG)g);
        rc_cat(" id ");
        rc_dec(g->GadgetID);
        rc_cat(" type ");
        rc_hex(g->GadgetType);
        rc_cat(" flags ");
        rc_hex(g->Flags);
        rc_cat(" pos ");
        rc_dec(g->LeftEdge); rc_cat(","); rc_dec(g->TopEdge);
        rc_cat(" size ");
        rc_dec(g->Width); rc_cat(","); rc_dec(g->Height);
        rc_cat(" text ");
        rc_hex((ULONG)g->GadgetText);
        rc_cat(" label \"");
        rc_copytitle(t, l, sizeof t);
        rc_cat(t);
        rc_cat("\"\n");
    }
}

/* the answer for a title from ENV:REQCLOSE: 0 none, 1 ok, 2 cancel */
static int rc_answer(const UBYTE *title)
{
    static char buf[512];
    char *p, *e;
    /* GVF_BINARY_VAR: without it GetVar stops at the first newline */
    int n = GetVar("REQCLOSE", buf, sizeof buf, GVF_GLOBAL_ONLY | GVF_BINARY_VAR);

    rc_gid = -1;
    if (n <= 0) return 0;
    for (p = buf; *p; p = e) {
        char *pat;
        int ans, len, m;
        for (e = p; *e && *e != '\n' && *e != ';'; e++) ;
        if (*e) *e++ = 0;
        while (*p == ' ') p++;
        LONG gid = -1;
        if (!strncmp(p, "ignore ", 7)) ans = 0, pat = p + 7;
        else if (!strncmp(p, "ok", 2)) ans = 1, pat = p + 2;
        else if (!strncmp(p, "cancel", 6)) ans = 2, pat = p + 6;
        else continue;
        if (ans && *pat == '#') {
            for (gid = 0, pat++; *pat >= '0' && *pat <= '9'; pat++)
                gid = gid * 10 + *pat - '0';
        }
        if (ans && *pat++ != ' ') continue;
        len = strlen(pat);
        while (len && (pat[len - 1] == ' ' || pat[len - 1] == '\r')) pat[--len] = 0;
        if (!len) continue;
        if (pat[len - 1] == '*')
            m = !title ? len == 1 : !strncmp((const char *)title, pat, len - 1);
        else
            m = title && !strcmp((const char *)title, pat);
        if (m) {
            rc_gid = gid;
            rc_gstep = ans == 1 ? ST_OK : ST_CANCEL;
            return ans;
        }
    }
    return 0;
}

static void rc_line(const UBYTE *title, const char *what)
{
    char b[180];
    strcpy(b, "REQCLOSE: \"");
    strncat(b, title ? (const char *)title : "", 100);
    strcat(b, "\" ");
    strcat(b, what);
    strcat(b, "\n");
    Write(out, b, strlen(b));
}

/* replies of our messages: buffers free again */
static void rc_replies(void)
{
    struct Message *m;
    int i;
    while ((m = GetMsg(rc_port)) != NULL)
        for (i = 0; i < RC_NMSG; i++)
            if (m == &rc_msg[i].eim_IntuiMessage.ExecMessage) {
                rc_busy[i] = 0;
                rc_cat("  reply ");
                rc_dec(i);
                rc_cat("\n");
            }
}

#ifndef REQCLOSE_INPUT
/* put a synthetic IntuiMessage into the window's port; 1 if sent.  The
   window is re-validated under Forbid + LockIBase. */
static int rc_send(struct Window *w, UBYTE *title, ULONG class, UWORD code, APTR addr)
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
#else
static struct IOStdReq *rc_io;

static void rc_event(struct InputEvent *ev)
{
    rc_io->io_Command = IND_WRITEEVENT;
    rc_io->io_Data = ev;
    rc_io->io_Length = sizeof(*ev);
    DoIO((struct IORequest *)rc_io);
}

static void rc_key(UWORD code)
{
    struct InputEvent ev;
    memset(&ev, 0, sizeof ev);
    ev.ie_Class = IECLASS_RAWKEY;
    ev.ie_Code = code;
    rc_event(&ev);
    Delay(2);
    ev.ie_Code = code | IECODE_UP_PREFIX;
    rc_event(&ev);
}

static void rc_click(struct Screen *s, WORD x, WORD y)
{
    struct InputEvent ev;
    struct IEPointerPixel pp;
    memset(&ev, 0, sizeof ev);
    pp.iepp_Screen = s;
    pp.iepp_Position.X = x;
    pp.iepp_Position.Y = y;
    ev.ie_Class = IECLASS_NEWPOINTERPOS;
    ev.ie_SubClass = IESUBCLASS_PIXEL;
    ev.ie_EventAddress = &pp;
    rc_event(&ev);
    Delay(2);
    memset(&ev, 0, sizeof ev);
    ev.ie_Class = IECLASS_RAWMOUSE;
    ev.ie_Code = IECODE_LBUTTON;
    ev.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    rc_event(&ev);
    Delay(2);
    ev.ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
    rc_event(&ev);
}

/* the input.device variant: click on the gadget / the window's close
   gadget area, Esc as key (old method, -DREQCLOSE_INPUT) */
static int rc_send(struct Window *w, UBYTE *title, ULONG class, UWORD code, APTR addr)
{
    struct Screen *s = 0;
    WORD x = 0, y = 0;
    ULONG lock = LockIBase(0);
    int ok = rc_valid(w) && w->Title == title;

    if (ok && class == IDCMP_GADGETUP) {
        struct Gadget *g = addr;
        s = w->WScreen;
        x = w->LeftEdge + g->LeftEdge + g->Width / 2;
        y = w->TopEdge + g->TopEdge + g->Height / 2;
    }
    UnlockIBase(lock);
    if (!ok) return 0;
    if (s)
        rc_click(s, x, y);
    else if (class == IDCMP_RAWKEY)
        rc_key(code);
    return 1;
}
#endif

static void rc_main(void)
{
    struct Window *cur = 0;
    UBYTE *curtitle = 0, titlecopy[100];
    int ans = 0, step = 0, wait = 0, tick = 0;
    ULONG lock;
    struct Screen *s;
    struct Window *w;

    {
        /* the log: ENV REQCLOSELOG (appended, so one file can collect a
           whole pass and survives a hang) or T:reqclose.log (new) */
        static char lp[256];
        if (GetVar("REQCLOSELOG", lp, sizeof lp, GVF_GLOBAL_ONLY) > 0
            && (rc_logfh = Open(lp, MODE_READWRITE)) != 0)
            Seek(rc_logfh, 0, OFFSET_END);
        else
            rc_logfh = Open("T:reqclose.log", MODE_NEWFILE);
    }
    rc_port = CreateMsgPort();
#ifdef REQCLOSE_INPUT
    rc_io = rc_port ? (struct IOStdReq *)CreateIORequest(rc_port, sizeof(struct IOStdReq)) : 0;
    if (rc_io && OpenDevice("input.device", 0, (struct IORequest *)rc_io, 0)) {
        DeleteIORequest((struct IORequest *)rc_io);
        rc_io = 0;
    }
    if (!rc_io) rc_stop = 1;
#endif
    rc_cat("start \"");
    rc_cat(rc_cmd);
    rc_cat("\", ");
    rc_dec(rc_nold);
    rc_cat(" windows\n");
    rc_flush();
    while (!rc_stop && rc_port) {
        struct Gadget *g = 0;
        int send = -1;

        Delay(2);
        tick++;
        rc_replies();
        lock = LockIBase(0);
        rc_prune();
        if (cur) {
            char t[40];
            int same = rc_valid(cur) && cur->Title == curtitle;
            if (same) {
                rc_copytitle(t, cur->Title, sizeof t);
                same = !strncmp(t, (const char *)titlecopy, sizeof t - 1);
            }
            if (!same) {
                cur = 0;
                rc_cat("tick ");
                rc_dec(tick);
                rc_cat(" window gone or replaced\n");
                /* log only: on stdout the line would race with the
                   program's own output (it runs again once the window
                   is closed) and make .run files differ */
            }
        }
        if (!cur) {
            /* a new candidate: the first window not seen before */
            for (s = IntuitionBase->FirstScreen; s && !cur; s = s->NextScreen)
                for (w = s->FirstWindow; w && !cur; w = w->NextWindow) {
                    if (rc_isold(w)) continue;
                    rc_remember(w, 0);  /* looked at once */
                    rc_copytitle((char *)titlecopy, w->Title, sizeof titlecopy);
                    cur = w;
                    curtitle = w->Title;
                    rc_cat("tick ");
                    rc_dec(tick);
                    rc_cat(" new window ");
                    rc_hex((ULONG)w);
                    rc_cat(" title ");
                    rc_hex((ULONG)w->Title);
                    rc_cat(" \"");
                    rc_cat((const char *)titlecopy);
                    rc_cat("\" pos ");
                    rc_dec(w->LeftEdge); rc_cat(","); rc_dec(w->TopEdge);
                    rc_cat(" size ");
                    rc_dec(w->Width); rc_cat(","); rc_dec(w->Height);
                    rc_cat(" port ");
                    rc_hex((ULONG)w->UserPort);
                    rc_cat(" idcmp ");
                    rc_hex(w->IDCMPFlags);
                    rc_cat("\n");
                    rc_dumpgadgets(w);
                }
            if (cur) {
                UnlockIBase(lock);
                rc_flush();
                ans = rc_answer(curtitle ? titlecopy : 0);   /* dos: unlocked */
                rc_cat("  answer ");
                rc_dec(ans);
                rc_cat(" id ");
                rc_dec(rc_gid);
                rc_cat("\n");
                rc_flush();
                if (ans) {
                    step = ans == 1 ? ST_OK : ST_CANCEL;
                    wait = RC_STEP0;
                    rc_line(titlecopy, ans == 1 ? "-> ok" : "-> cancel");
                }
                lock = LockIBase(0);
                if (!ans || !(rc_valid(cur) && cur->Title == curtitle))
                    cur = 0;
            }
        } else if (--wait <= 0 && step >= ST_END) {
            UnlockIBase(lock);
            rc_line(titlecopy, "gave up");
            lock = LockIBase(0);
            cur = 0;                        /* stays seen: never again */
        } else if (wait <= 0) {
            /* the next step for the current window (still valid) */
            send = step;
            if (step == ST_OK || step == ST_CANCEL) {
                g = rc_gadget(cur, step);
                if (!g) send = -2;          /* no such gadget: next step */
                rc_cat("tick ");
                rc_dec(tick);
                rc_cat(" gadget by ");
                rc_cat(rc_why);
                rc_cat(": ");
                rc_hex((ULONG)g);
                if (g) { rc_cat(" id "); rc_dec(g->GadgetID); }
                rc_cat("\n");
            }
            rc_cat("tick ");
            rc_dec(tick);
            rc_cat(send == -2 ? " skip " : " do ");
            rc_cat(rc_stepname[step]);
            rc_cat("\n");
            step++;
            wait = send == -2 ? 1 : RC_STEPN;
        }
        UnlockIBase(lock);
        rc_flush();
        if (send >= 0) {
            int sent = 0;
            switch (send) {
            case ST_OK:
            case ST_CANCEL: sent = rc_send(cur, curtitle, IDCMP_GADGETUP, 0, g); break;
            case ST_CLOSE:  sent = rc_send(cur, curtitle, IDCMP_CLOSEWINDOW, 0, cur); break;
            case ST_RAWESC: sent = rc_send(cur, curtitle, IDCMP_RAWKEY, 0x45, &rc_prevkeys); break;
            case ST_VANESC: sent = rc_send(cur, curtitle, IDCMP_VANILLAKEY, 27, &rc_prevkeys); break;
            }
            rc_cat(sent ? "  sent\n" : "  not sent (window gone or no buffer)\n");
            rc_flush();
            if (sent)
                rc_line(titlecopy, rc_stepname[send]);
            else
                wait = 1;                   /* next tick */
        }
    }
    /* wait (bounded) for the replies of messages still out */
    {
        int i, n;
        for (n = 0; n < 100; n++) {
            int busy = 0;
            rc_replies();
            for (i = 0; i < RC_NMSG; i++) busy |= rc_busy[i];
            if (!busy) break;
            Delay(1);
        }
    }
    rc_cat("end\n");
    rc_flush();
    if (rc_logfh) Close(rc_logfh);
#ifdef REQCLOSE_INPUT
    if (rc_io) {
        CloseDevice((struct IORequest *)rc_io);
        DeleteIORequest((struct IORequest *)rc_io);
    }
#endif
    if (rc_port) DeleteMsgPort(rc_port);
    Forbid();                               /* exits before the parent runs */
    Signal(rc_parent, 1UL << rc_donesig);
}

/* start: snapshot of the existing windows, then the process */
static int rc_start(const char *cmd)
{
    ULONG lock = LockIBase(0);
    struct Screen *s;
    struct Window *w;

    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        for (w = s->FirstWindow; w; w = w->NextWindow)
            rc_remember(w, 1);
    UnlockIBase(lock);
    rc_cmd = cmd;
    rc_parent = FindTask(NULL);
    rc_donesig = AllocSignal(-1);
    if (rc_donesig < 0) return 0;
    if (!CreateNewProcTags(NP_Entry, (ULONG)rc_main, NP_Name, (ULONG)"reqclose",
                           NP_Priority, 1, NP_StackSize, 8192, TAG_END)) {
        FreeSignal(rc_donesig);
        rc_donesig = -1;
        return 0;
    }
    return 1;
}

static void rc_end(void)
{
    if (rc_donesig < 0) return;
    rc_stop = 1;
    Wait(1UL << rc_donesig);
    FreeSignal(rc_donesig);
}
