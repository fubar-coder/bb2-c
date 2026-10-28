/*
 * edstub.c - the frame of a headless editor, see edstub.h: init entry,
 * comdata (the services the compiler calls), main entry, log and small
 * libc replacements.
 */
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "edstub.h"

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;

struct edstruct *edstub_ed;
const char *(*edstub_ezfreq_hook)(const char *title);
void (*edstub_error_hook)(const char *msg);
void (*edstub_tokone_hook)(UBYTE *line);
char edstub_logname[EDSTUB_PATHLEN];

void edstub_tomain(void);
ULONG edstub_abort_sp;

#define STUB(n) extern void n##_stub(void)
STUB(screenclose); STUB(screenopen2); STUB(openreq); STUB(closereq);
STUB(getinput); STUB(error); STUB(detokallz); STUB(tokall); STUB(ezfreq);
STUB(refgads); STUB(getinput2); STUB(tokone); STUB(openio); STUB(closeio);
STUB(actgad); STUB(pointeron); STUB(sleepon); STUB(extalloc);

/* ---------------------------------------------------------------------
 * The table.  comdata (what blitz2 gets in a1) is &cd.openreq; the two
 * longwords before it are screenclose (-8) and screenopen2 (-4).
 * Alignment of longs on m68k is 2, so 92 (word) / 94 (long) need no
 * packing; the asserts check every offset against ted.s.
 * ------------------------------------------------------------------- */
struct comdata_s {
    APTR screenclose;   /* -8 */
    APTR screenopen2;   /* -4 */
    APTR openreq;       /*  0 a0 = requester template */
    APTR closereq;      /*  4 */
    APTR getinput;      /*  8 out d7 = -1, d6 = gadget id */
    APTR error;         /* 12 a0 = message */
    APTR detokallz;     /* 16 */
    APTR tokall;        /* 20 */
    APTR ezfreq;        /* 24 a0 path, a1 name, a2 title -> d0, Z */
    APTR refgads;       /* 28 */
    APTR getinput2;     /* 32 poll: d7 = 0 nothing */
    APTR combitmap;     /* 36 plane memory for the racing stripe */
    APTR myscreen;      /* 40 */
    APTR extint;        /* 44 IntuitionBase */
    APTR colors;        /* 48 */
    APTR halfcolors;    /* 52 */
    APTR tokone;        /* 56 a0 = line */
    APTR openio;        /* 60 */
    APTR closeio;       /* 64 */
    APTR actgad;        /* 68 */
    APTR pointeron;     /* 72 */
    APTR sleepon;       /* 76 */
    APTR mywindow2;     /* 80 editor window (Width at +8) */
    APTR flag84;        /* 84 ted: &defaultreqga9+13 */
    APTR extalloc;      /* 88 d0 size, d1 flags, a6 exec -> d0 */
    UWORD bmapmul;      /* 92 bytes per plane row */
    APTR req_window;    /* 94 window, +86 = UserPort */
};
#define CDOFF(m) (offsetof(struct comdata_s, m) - 8)
_Static_assert(CDOFF(openreq) == 0, "openreq");
_Static_assert(CDOFF(getinput2) == 32, "getinput2");
_Static_assert(CDOFF(combitmap) == 36, "combitmap");
_Static_assert(CDOFF(extint) == 44, "extint");
_Static_assert(CDOFF(tokone) == 56, "tokone");
_Static_assert(CDOFF(pointeron) == 72, "pointeron");
_Static_assert(CDOFF(mywindow2) == 80, "mywindow2");
_Static_assert(CDOFF(extalloc) == 88, "extalloc");
_Static_assert(CDOFF(bmapmul) == 92, "bmapmul");
_Static_assert(CDOFF(req_window) == 94, "req_window");
_Static_assert(offsetof(struct Window, UserPort) == 86, "UserPort");
_Static_assert(offsetof(struct Window, Width) == 8, "Width");
_Static_assert(offsetof(struct edstruct, colon) == 24, "edstruct");
_Static_assert(sizeof(struct edstruct) == 28, "edstruct: 14 words");

static struct comdata_s cd = {
    screenclose_stub, screenopen2_stub,
    openreq_stub, closereq_stub, getinput_stub, error_stub,
    detokallz_stub, tokall_stub, ezfreq_stub, refgads_stub, getinput2_stub,
    0, 0, 0, 0, 0,
    tokone_stub, openio_stub, closeio_stub, actgad_stub,
    pointeron_stub, sleepon_stub,
    0, 0, extalloc_stub, 0, 0
};

static UWORD colors[32], halfcolors[32];
static UBYTE flag84;

/* fake editor window / bitmap */
#define WIN_WIDTH   640
#define BMAPMUL     80                  /* bytes per row of a 640 pixel plane */
#define BITMAP_SIZE (BMAPMUL * 16)      /* stripe uses rows 0..9 */
static struct Window fakewin;
static struct MsgPort *winport;
static UBYTE *bitmap;
static APTR old_winptr;
static int in_main;

/* ---------------------------------------------------------------------
 * small libc replacements
 * ------------------------------------------------------------------- */
void *memset(void *d, int c, size_t n)
{
    UBYTE *p = d;

    while (n--)
        *p++ = c;
    return d;
}

void *memcpy(void *d, const void *s, size_t n)
{
    UBYTE *p = d;
    const UBYTE *q = s;

    while (n--)
        *p++ = *q++;
    return d;
}

ULONG edstub_strlen(const char *s)
{
    ULONG n = 0;

    while (s[n])
        n++;
    return n;
}

int edstub_prefix(const char *s, const char *pre)
{
    while (*pre)
        if (*s++ != *pre++)
            return 0;
    return 1;
}

int edstub_streq(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return *a == *b;
}

void edstub_strcpy(char *d, const char *s, ULONG max)
{
    ULONG i = 0;

    while (s[i] && i + 1 < max) {
        d[i] = s[i];
        i++;
    }
    if (max)
        d[i] = 0;
}

void edstub_strcat(char *d, const char *s, ULONG max)
{
    ULONG n = edstub_strlen(d);

    edstub_strcpy(d + n, s, max > n ? max - n : 0);
}

/* u / 10 without libgcc (no __udivsi3 in a -nostdlib build) */
ULONG edstub_udiv10(ULONG u, ULONG *rem)
{
    ULONG q = 0, r = 0;
    int i;

    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((u >> i) & 1);
        if (r >= 10) {
            r -= 10;
            q |= 1UL << i;
        }
    }
    *rem = r;
    return q;
}

void sbc(struct sb *b, char c)
{
    if (b->len + 1 < b->max)
        b->buf[b->len++] = c;
    b->buf[b->len] = 0;
}

void sbs(struct sb *b, const char *s)
{
    if (!s)
        s = "(null)";
    while (*s)
        sbc(b, *s++);
}

void sbn(struct sb *b, LONG v)
{
    char t[12];
    int i = 0;
    ULONG u, rem;

    if (v < 0) {
        sbc(b, '-');
        u = -(ULONG)v;
    } else
        u = v;
    do {
        u = edstub_udiv10(u, &rem);
        t[i++] = '0' + rem;
    } while (u);
    while (i)
        sbc(b, t[--i]);
}

void sbx(struct sb *b, ULONG v, int digits)
{
    while (digits--)
        sbc(b, "0123456789ABCDEF"[(v >> (4 * digits)) & 15]);
}

void sbq(struct sb *b, const char *s)
{
    sbc(b, '"');
    if (s)
        for (; *s; s++) {
            UBYTE c = *s;

            if (c < 32 || c == 127) {
                sbs(b, "\\x");
                sbx(b, c, 2);
            } else
                sbc(b, c);
        }
    sbc(b, '"');
}

/* ---------------------------------------------------------------------
 * log
 * ------------------------------------------------------------------- */
void edstub_log(const char *s)
{
    ULONG n = edstub_strlen(s);
    BPTR out = Output(), fh;

    if (out) {
        Write(out, (APTR)s, n);
        Write(out, "\n", 1);
    }
    if (!edstub_logname[0] || !(fh = Open(edstub_logname, MODE_READWRITE)))
        return;
    Seek(fh, 0, OFFSET_END);
    Write(fh, (APTR)s, n);
    Write(fh, "\n", 1);
    Close(fh);
}

static char lbuf[600];
#define LOG_BEGIN struct sb L = { lbuf, 0, sizeof lbuf }; lbuf[0] = 0
#define LOG_END   edstub_log(lbuf)

static void closeall(void)
{
    ((struct Process *)FindTask(0))->pr_WindowPtr = old_winptr;
    if (winport) {
        DeleteMsgPort(winport);
        winport = 0;
    }
    if (bitmap) {
        FreeMem(bitmap, BITMAP_SIZE);
        bitmap = 0;
    }
    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = 0;
    }
    if (DOSBase) {
        CloseLibrary((struct Library *)DOSBase);
        DOSBase = 0;
    }
}

/* abort: inside main return to the compiler as if the editor had quit
   (ted's outmem went back to the editor loop); before main there is no
   way back, so the process exits. */
static void abort_job(const char *why) __attribute__((noreturn));
static int reqdepth;

static void abort_job(const char *why)
{
    struct Task *me = FindTask(0);
    LOG_BEGIN;

    me->tc_SigExcept = 0;               /* stop requester's exception, like endstop */
    me->tc_ExceptCode = 0;
    reqdepth = 0;
    sbs(&L, "ABORT: ");
    sbs(&L, why);
    LOG_END;
    if (in_main) {
        in_main = 0;
        closeall();
        edstub_abort();
    }
    Exit(20);
    for (;;)
        ;
}

/* ---------------------------------------------------------------------
 * requesters: opened (remembered) but never shown
 * ------------------------------------------------------------------- */
#define MAXREQ 8
static struct Requester *reqstack[MAXREQ];
static int nomem_answers, unexpected;

static const char *reqtext(struct Requester *r)
{
    if (r && r->ReqText && r->ReqText->IText)
        return (const char *)r->ReqText->IText;
    return "";
}

static const char *gadtext(struct Gadget *g)
{
    if (g && g->GadgetText && g->GadgetText->IText)
        return (const char *)g->GadgetText->IText;
    return "";
}

static void popreq(void)
{
    if (reqdepth)
        reqstack[--reqdepth] = 0;
}

void screenclose_c(struct regs *r) { (void)r; }
void screenopen2_c(struct regs *r) { (void)r; }
void detokallz_c(struct regs *r) { (void)r; }
void tokall_c(struct regs *r) { (void)r; }
void refgads_c(struct regs *r) { (void)r; }
void openio_c(struct regs *r) { (void)r; }
void closeio_c(struct regs *r) { (void)r; }
void pointeron_c(struct regs *r) { (void)r; }
void sleepon_c(struct regs *r) { (void)r; }
void actgad_c(struct regs *r) { r->d[0] = 1; }      /* ActivateGadget() TRUE */

void tokone_c(struct regs *r)
{
    LOG_BEGIN;

    if (edstub_tokone_hook) {
        edstub_tokone_hook((UBYTE *)r->a[0]);
        return;
    }
    sbs(&L, "tokone (not implemented, line left as is): ");
    sbq(&L, (const char *)r->a[0]);
    LOG_END;
}

void openreq_c(struct regs *r)
{
    struct Requester *q = (struct Requester *)r->a[0];
    LOG_BEGIN;

    if (reqdepth < MAXREQ)
        reqstack[reqdepth++] = q;
    sbs(&L, "openreq ");
    sbq(&L, reqtext(q));
    LOG_END;
    r->d[0] = (ULONG)cd.req_window;     /* ted: window opened */
}

void closereq_c(struct regs *r)
{
    (void)r;
    popreq();
}

void getinput2_c(struct regs *r)
{
    r->d[7] = 0;                        /* nothing there; STOP never hit */
}

/* getinput: answer the topmost requester.  An ENDGADGET closes it, like
   ted's handled0 does. */
void getinput_c(struct regs *r)
{
    struct Requester *q = reqdepth ? reqstack[reqdepth - 1] : 0;
    struct Gadget *g, *pick = 0, *lastend = 0;
    const char *t = reqtext(q);
    const char *why;
    LOG_BEGIN;

    if (q) {
        UWORD want = 0;

        if (edstub_prefix(t, "Blitz Basic 2 Version")) {
            want = 1;
            why = "version requester";
        } else if (edstub_prefix(t, "Not Enough Object Memory")) {
            if (++nomem_answers <= 4) {
                want = 1;
                why = "object memory: increase 150%";
            } else {
                want = 3;
                why = "object memory: giving up, cancel";
            }
        } else {
            why = "unexpected requester, last end gadget";
            unexpected++;
        }
        for (g = q->ReqGadget; g; g = g->NextGadget) {
            if (want && g->GadgetID == want)
                pick = g;
            if (g->Activation & GACT_ENDGADGET)
                lastend = g;
        }
        if (!pick)
            pick = lastend ? lastend : q->ReqGadget;
    } else {
        why = "no requester open";
        unexpected++;
    }

    sbs(&L, "getinput ");
    sbq(&L, t);
    if (pick) {
        sbs(&L, " -> gadget ");
        sbn(&L, pick->GadgetID);
        sbc(&L, ' ');
        sbq(&L, gadtext(pick));
        r->d[6] = pick->GadgetID;
    } else {
        sbs(&L, " -> gadget 1 (none there)");
        r->d[6] = 1;
    }
    r->d[7] = (ULONG)-1;
    sbs(&L, " (");
    sbs(&L, why);
    sbc(&L, ')');
    LOG_END;
    if (pick && (pick->Activation & GACT_ENDGADGET))
        popreq();

    if (unexpected > 20)
        abort_job("compiler keeps waiting for input");
}

/* ezfreq: a file name from the program's hook, else cancelled.
   out: d0 = name, Z=0 (the stub does tst.l d0). */
void ezfreq_c(struct regs *r)
{
    const char *title = (const char *)r->a[2];
    const char *name = edstub_ezfreq_hook ? edstub_ezfreq_hook(title) : 0;
    LOG_BEGIN;

    sbs(&L, "ezfreq ");
    sbq(&L, title);
    r->d[0] = (ULONG)name;
    if (name) {
        sbs(&L, " -> ");
        sbs(&L, name);
    } else
        sbs(&L, " -> cancelled");
    LOG_END;
}

/* extalloc: AllocMem; on failure log and abort (ted: outmem) */
void extalloc_c(struct regs *r)
{
    ULONG size = r->d[0], flags = r->d[1];
    APTR m = AllocMem(size, flags);

    r->d[0] = (ULONG)m;
    if (!m) {
        LOG_BEGIN;
        sbs(&L, "extalloc AllocMem(");
        sbn(&L, size);
        sbs(&L, ", $");
        sbx(&L, flags, 8);
        sbs(&L, ") failed");
        LOG_END;
        abort_job("extalloc failed");
    }
}

void error_c(struct regs *r)
{
    const char *m = (const char *)r->a[0];
    LOG_BEGIN;

    if (edstub_error_hook) {
        edstub_error_hook(m ? m : "");
        return;
    }
    sbs(&L, "ERROR ");
    sbq(&L, m);
    LOG_END;
}

/* ---------------------------------------------------------------------
 * entries
 * ------------------------------------------------------------------- */
void edstub_init_c(struct regs *r)
{
    struct Process *me;

    SysBase = *(struct ExecBase **)4;
    edstub_ed = (struct edstruct *)r->a[1];
    r->d[0] = 1;                        /* fail unless everything is there */
    if (!edstub_ed || SysBase->LibNode.lib_Version < 36)
        return;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    bitmap = AllocMem(BITMAP_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    winport = CreateMsgPort();          /* real port: blitz2 reads mp_SigBit */
    if (!DOSBase || !IntuitionBase || !bitmap || !winport) {
        closeall();
        return;
    }

    fakewin.Width = WIN_WIDTH;
    fakewin.Height = 256;
    fakewin.UserPort = winport;
    cd.combitmap = bitmap;
    cd.myscreen = 0;
    cd.extint = IntuitionBase;
    cd.colors = colors;
    cd.halfcolors = halfcolors;
    cd.mywindow2 = &fakewin;
    cd.flag84 = &flag84;
    cd.bmapmul = BMAPMUL;
    cd.req_window = &fakewin;

    /* no DOS requesters ("Please insert volume ...") - they would wait */
    me = (struct Process *)FindTask(0);
    old_winptr = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;

    r->a[0] = (ULONG)edstub_tomain;
    r->a[1] = (ULONG)&cd.openreq;
    r->d[0] = 0;
}

void edstub_main_c(struct regs *r)
{
    in_main = 1;
    edstub_main((const char *)r->a[0]);
    in_main = 0;
    closeall();
}
