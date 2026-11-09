/*
 * edstub.c - the frame of a headless editor, see edstub.h: init entry,
 * comdata (the services the compiler calls), main entry, log and small
 * libc replacements.
 */
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <exec/io.h>
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
void (*edstub_abort_hook)(const char *why);
void (*edstub_openio_hook)(void);
void (*edstub_closeio_hook)(void);
UWORD (*edstub_getinput2_hook)(void);
char edstub_logname[EDSTUB_PATHLEN];
int edstub_quiet;

void edstub_tomain(void);
ULONG edstub_abort_sp;

#define STUB(n) extern void n##_stub(void)
STUB(screenclose); STUB(screenopen2); STUB(openreq); STUB(closereq);
STUB(getinput); STUB(error); STUB(detokallz); STUB(tokall); STUB(ezfreq);
STUB(refgads); STUB(getinput2); STUB(tokone); STUB(openio); STUB(closeio);
STUB(actgad); STUB(pointeron); STUB(sleepon); STUB(extalloc);
STUB(progress);

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
    ULONG progmark;     /* 98 TED-C: 'TEDp' = the progress call is there */
    APTR progress;      /* 102 TED-C: d0 bar, d1 column (#980) */
};
#define PROGMARK 0x54454470UL           /* 'TEDp' */
#define CDOFF(m) (offsetof(struct comdata_s, m) - 8)
static_assert(CDOFF(openreq) == 0, "openreq");
static_assert(CDOFF(getinput2) == 32, "getinput2");
static_assert(CDOFF(combitmap) == 36, "combitmap");
static_assert(CDOFF(extint) == 44, "extint");
static_assert(CDOFF(tokone) == 56, "tokone");
static_assert(CDOFF(pointeron) == 72, "pointeron");
static_assert(CDOFF(mywindow2) == 80, "mywindow2");
static_assert(CDOFF(extalloc) == 88, "extalloc");
static_assert(CDOFF(bmapmul) == 92, "bmapmul");
static_assert(CDOFF(req_window) == 94, "req_window");
static_assert(CDOFF(progmark) == 98, "progmark");
static_assert(CDOFF(progress) == 102, "progress");
static_assert(offsetof(struct Window, UserPort) == 86, "UserPort");
static_assert(offsetof(struct Window, Width) == 8, "Width");
static_assert(offsetof(struct edstruct, colon) == 24, "edstruct");
static_assert(sizeof(struct edstruct) == 28, "edstruct: 14 words");

static struct comdata_s cd = {
    screenclose_stub, screenopen2_stub,
    openreq_stub, closereq_stub, getinput_stub, error_stub,
    detokallz_stub, tokall_stub, ezfreq_stub, refgads_stub, getinput2_stub,
    0, 0, 0, 0, 0,
    tokone_stub, openio_stub, closeio_stub, actgad_stub,
    pointeron_stub, sleepon_stub,
    0, 0, extalloc_stub, 0, 0,
    0, progress_stub                    /* not announced: edstub_set_progress */
};

static UWORD colors[32], halfcolors[32];
static UBYTE flag84;

/* fake editor window / bitmap */
#define WIN_WIDTH   640
#define BMAPMUL     80                  /* bytes per row of a 640 pixel plane */
#define BITMAP_ROWS 16                  /* stripe uses rows 0..9 */
static struct Window fakewin;
static struct MsgPort *winport;
static UBYTE *bitmap;                   /* = cd.combitmap, in bitmap_mem */
static UBYTE *bitmap_mem;               /* guard, bitmap, guard */
static ULONG bitmap_size, guard_size;   /* of bitmap_mem, of each guard */
static void stripe_check(void);         /* bitmap= checkpoint (#980) */
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
/* lines before the log file is known (the version requester comes before
   main); they go into the file when edstub_set_log opens it */
static char early[2048];
static struct sb earlyb = { early, 0, sizeof early };

/* every line is appended and the file closed again: the log survives a
   crash of the compiler */
void edstub_log(const char *s)
{
    ULONG n = edstub_strlen(s);
    BPTR out = Output(), fh;

    if (out && !edstub_quiet) {
        Write(out, (APTR)s, n);
        Write(out, "\n", 1);
    }
    if (!edstub_logname[0]) {
        sbs(&earlyb, s);
        sbc(&earlyb, '\n');
        return;
    }
    if (!(fh = Open(edstub_logname, MODE_READWRITE)))
        return;
    Seek(fh, 0, OFFSET_END);
    Write(fh, (APTR)s, n);
    Write(fh, "\n", 1);
    Close(fh);
}

int edstub_set_log(const char *path)
{
    BPTR fh;

    edstub_strcpy(edstub_logname, path, sizeof edstub_logname);
    if (!(fh = Open(edstub_logname, MODE_NEWFILE))) {
        edstub_logname[0] = 0;
        return 0;
    }
    Write(fh, early, earlyb.len);
    Close(fh);
    earlyb.len = 0;
    early[0] = 0;
    return 1;
}

static char lbuf[600];
#define LOG_BEGIN struct sb L = { lbuf, 0, sizeof lbuf }; lbuf[0] = 0
#define LOG_END   edstub_log(lbuf)

static void closeall(void)
{
    ((struct Process *)FindTask(0))->pr_WindowPtr = old_winptr;
    if (winport) {
        edstub_delete_port(winport);
        winport = 0;
    }
    if (bitmap_mem) {
        FreeMem(bitmap_mem, bitmap_size);
        bitmap_mem = bitmap = 0;
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
        if (edstub_abort_hook)
            edstub_abort_hook(why);
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

void edstub_reset_requests(void)
{
    nomem_answers = unexpected = 0;
}

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
void openio_c(struct regs *r)
{
    (void)r;
    if (edstub_openio_hook)
        edstub_openio_hook();
}

void closeio_c(struct regs *r)
{
    (void)r;
    if (edstub_closeio_hook)
        edstub_closeio_hook();
}
int edstub_busy;
void pointeron_c(struct regs *r) { (void)r; edstub_busy = 0; }
void sleepon_c(struct regs *r) { (void)r; edstub_busy = 1; }
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
    stripe_check();
}

void closereq_c(struct regs *r)
{
    (void)r;
    popreq();
    stripe_check();
}

/* getinput2: the gadget the program's hook reports as hit (d7 = -1, d6 =
   its id), else nothing (d7 = 0: STOP never hit) */
void getinput2_c(struct regs *r)
{
    UWORD id = edstub_getinput2_hook ? edstub_getinput2_hook() : 0;

    r->d[7] = id ? (ULONG)-1 : 0;
    if (id)
        r->d[6] = id;
}

int edstub_open_requests(void)
{
    return reqdepth;
}

struct MsgPort *edstub_req_port(void)
{
    return winport;
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
 * the racing stripe (#980, edstub.h): the bitmap with guards, the
 * progress call of TED-C.  No multiplication (no libgcc): row offsets by
 * addition.
 * ------------------------------------------------------------------- */
#define GUARD_BYTE  0xA5
#define PROGCOLS    256                 /* columns counted and pictured */
static int stripe_on;                   /* edstub_set_bitmap was called */
static UBYTE progcount[4][PROGCOLS];    /* calls per bar and column */
static ULONG progcalls, progbad;
/* the fullest picture of the checkpoints (columns 0 to PROGCOLS - 1),
   the most guard bytes changed at one checkpoint */
static UBYTE snap[BITMAP_ROWS][PROGCOLS];
static ULONG snap_set, guard_before, guard_after;

/* the offset of pixel row `row` of the bitmap */
static ULONG rowoff(UWORD row)
{
    ULONG off = 0;

    while (row--)
        off += cd.bmapmul;
    return off;
}

/* bitmap cleared, guards filled, counters and checkpoints 0 */
static void stripe_reset(void)
{
    ULONG i;

    for (i = 0; i < bitmap_size; i++)
        bitmap_mem[i] = i < guard_size || i >= bitmap_size - guard_size ? GUARD_BYTE : 0;
    memset(progcount, 0, sizeof progcount);
    memset(snap, 0, sizeof snap);
    progcalls = progbad = snap_set = guard_before = guard_after = 0;
}

/* a checkpoint (openreq, closereq, the end of a job): a compile runs one
   or two passes, and the XORs of the second undo those of the first, so
   the state at the end alone would hide a stripe (and writes into the
   guards); the requester of the second pass opens between them */
static void stripe_check(void)
{
    ULONG i, set = 0, before = 0, after = 0;
    UWORD row, n;

    if (!stripe_on)
        return;
    for (row = 0; row < BITMAP_ROWS; row++) {
        UBYTE *p = bitmap + rowoff(row);

        for (n = 0; n < PROGCOLS && n < cd.bmapmul; n++)
            if (p[n])
                set++;
    }
    if (set > snap_set) {
        snap_set = set;
        for (row = 0; row < BITMAP_ROWS; row++) {
            UBYTE *p = bitmap + rowoff(row);

            for (n = 0; n < PROGCOLS && n < cd.bmapmul; n++)
                snap[row][n] = p[n];
        }
    }
    for (i = 0; i < guard_size; i++) {
        if (bitmap_mem[i] != GUARD_BYTE)
            before++;
        if (bitmap_mem[bitmap_size - guard_size + i] != GUARD_BYTE)
            after++;
    }
    if (before > guard_before)
        guard_before = before;
    if (after > guard_after)
        guard_after = after;
}

int edstub_set_bitmap(UWORD bmapmul, ULONG guard)
{
    ULONG size = 2 * guard, i;
    UBYTE *mem;

    if (bmapmul < BMAPMUL)
        return 0;
    for (i = 0; i < BITMAP_ROWS; i++)
        size += bmapmul;
    if (!(mem = AllocMem(size, MEMF_PUBLIC)))
        return 0;
    if (bitmap_mem)
        FreeMem(bitmap_mem, bitmap_size);
    bitmap_mem = mem;
    bitmap_size = size;
    guard_size = guard;
    bitmap = mem + guard;
    cd.combitmap = bitmap;
    cd.bmapmul = bmapmul;
    stripe_on = 1;
    stripe_reset();
    return 1;
}

void edstub_set_progress(int on)
{
    cd.progmark = on ? PROGMARK : 0;
}

/* comdata+102: d0.w bar, d1.w column; toggled as the original XORs */
void progress_c(struct regs *r)
{
    UWORD bar = (UWORD)r->d[0], col = (UWORD)r->d[1];

    progcalls++;
    if (bar > 3 || col >= cd.bmapmul || col >= PROGCOLS) {
        progbad++;
        return;
    }
    if (progcount[bar][col] < 255)
        progcount[bar][col]++;
    bitmap[rowoff(bar) + col] ^= 0xff;
    bitmap[rowoff(9 - bar) + col] ^= 0xff;
}

void edstub_stripe_log(const char *id)
{
    UWORD bar, row, n, last;

    if (!stripe_on)
        return;
    stripe_check();
    if (!progcalls) {
        LOG_BEGIN;
        sbs(&L, "PROGRESS\t");
        sbs(&L, id);
        sbs(&L, "\tnone");
        LOG_END;
    }
    for (bar = 0; bar < 4; bar++) {
        LOG_BEGIN;

        for (last = 0, n = 0; n < PROGCOLS; n++)
            if (progcount[bar][n])
                last = n + 1;
        if (!last)
            continue;
        sbs(&L, "PROGRESS\t");
        sbs(&L, id);
        sbs(&L, "\tbar ");
        sbn(&L, bar);
        sbc(&L, '\t');
        for (n = 0; n < last; n++)
            sbc(&L, progcount[bar][n] > 9 ? '+' : '0' + progcount[bar][n]);
        LOG_END;
    }
    if (progbad) {
        LOG_BEGIN;
        sbs(&L, "PROGRESS\t");
        sbs(&L, id);
        sbs(&L, "\tbad ");
        sbn(&L, progbad);
        LOG_END;
    }
    for (row = 0; row < BITMAP_ROWS; row++) {
        LOG_BEGIN;

        for (last = 0, n = 0; n < PROGCOLS; n++)
            if (snap[row][n])
                last = n + 1;
        if (!last)
            continue;
        sbs(&L, "STRIPE\t");
        sbs(&L, id);
        sbs(&L, "\trow ");
        sbn(&L, row);
        sbc(&L, '\t');
        for (n = 0; n < last; n++)
            sbc(&L, snap[row][n] == 0xff ? '#' : snap[row][n] ? 'x' : '.');
        LOG_END;
    }
    if (!snap_set) {
        LOG_BEGIN;
        sbs(&L, "STRIPE\t");
        sbs(&L, id);
        sbs(&L, "\tnone");
        LOG_END;
    }
    {
        LOG_BEGIN;
        sbs(&L, "GUARD\t");
        sbs(&L, id);
        if (!guard_before && !guard_after)
            sbs(&L, "\tok");
        else {
            sbs(&L, "\tbefore ");
            sbn(&L, guard_before);
            sbs(&L, " after ");
            sbn(&L, guard_after);
        }
        LOG_END;
    }
    stripe_reset();
}

/* ---------------------------------------------------------------------
 * ports and IO requests with Kickstart 1.x calls (edstub.h)
 * ------------------------------------------------------------------- */
struct MsgPort *edstub_create_port(void)
{
    struct MsgPort *port = AllocMem(sizeof *port, MEMF_PUBLIC | MEMF_CLEAR);
    BYTE sig;

    if (!port)
        return 0;
    if ((sig = AllocSignal(-1)) < 0) {
        FreeMem(port, sizeof *port);
        return 0;
    }
    port->mp_Node.ln_Type = NT_MSGPORT;
    port->mp_Flags = PA_SIGNAL;
    port->mp_SigBit = sig;
    port->mp_SigTask = FindTask(0);
    port->mp_MsgList.lh_Head = (struct Node *)&port->mp_MsgList.lh_Tail;
    port->mp_MsgList.lh_TailPred = (struct Node *)&port->mp_MsgList.lh_Head;
    return port;
}

void edstub_delete_port(struct MsgPort *port)
{
    if (!port)
        return;
    FreeSignal(port->mp_SigBit);
    FreeMem(port, sizeof *port);
}

struct IORequest *edstub_create_ioreq(struct MsgPort *port, ULONG size)
{
    struct IORequest *io;

    if (!port || !(io = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR)))
        return 0;
    io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io->io_Message.mn_ReplyPort = port;
    io->io_Message.mn_Length = size;
    return io;
}

void edstub_delete_ioreq(struct IORequest *io)
{
    if (!io)
        return;
    io->io_Message.mn_Node.ln_Succ = (struct Node *)-1;     /* as DeleteIORequest */
    io->io_Device = (struct Device *)-1;
    FreeMem(io, io->io_Message.mn_Length);
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
    if (!edstub_ed || SysBase->LibNode.lib_Version < 33)
        return;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 33);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 33);
    bitmap_size = (ULONG)BMAPMUL * BITMAP_ROWS;
    bitmap = bitmap_mem = AllocMem(bitmap_size, MEMF_PUBLIC | MEMF_CLEAR);
    winport = edstub_create_port();     /* real port: blitz2 reads mp_SigBit */
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

#ifdef EDSTUB_INIT
    edstub_init();
#endif
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
