/*
 * tedstub.c - headless TED for batch runs of blitz2 (from bb2
 * compilerlauf/tedstub): blitz2 loads it as blitz2:ted (edstub frame) and
 * calls main with the name of a job list; the stub compiles every job in
 * this one compiler session, like TED loading files one after the other.
 *
 * Job list (the argument blitz2 got), lines KEY=VALUE:
 *   log=PATH        log (tab separated lines, see below)
 *   defaults=PATH   the compiler's start options are saved there with
 *                   savextra before the first job and loaded for every job
 *                   without its own .xtra (no options of the previous job)
 *   runerr=off      in those defaults runtime error checking and the
 *                   debugger off (bytes 2-5 of the .xtra zeroed; a source
 *                   with its own .xtra keeps its options)
 *   strbuf=N        in those defaults the size of the string buffer N
 *                   bytes (bytes 50-53 of the .xtra; default 10240)
 *   macrobuf=N      in those defaults the size of the macro buffer N
 *                   bytes (bytes 46-49 of the .xtra; default 8192)
 *   job=ID|SOURCE|OUTPUT[|DIR]
 *   menu=ID|ITEM[|ANSWER]  the compiler's menu item ITEM (blitz2.s
 *                   menuadds, e.g. 9 = load debug module, 8 = reload libs)
 *                   with no source; a file requester gets ANSWER (none:
 *                   cancelled).  Jobs and menu items run in the order of
 *                   the list.
 * Every job: CurrentDir(DIR, default: the source's directory - relative
 * INCLUDE/INCBIN), load the source like TED's readline/allocline,
 * loadxtra (SOURCE.xtra or defaults), menu CREATE EXECUTABLE, result.
 *
 * Log lines (each written at once, so the log survives a crash):
 *   BATCH start|end
 *   JOB ID SOURCE
 *   XTRA ID own|defaults|none
 *   XTRAERR ID MESSAGE                error outside the compile (residents)
 *   RESULT ID ok SIZE
 *   RESULT ID error LINE MESSAGE TEXT  the compiler's error, TEXT = the line
 *   RESULT ID failed REASON
 *   MENU ID ITEM RET [MESSAGE]        a menu item: what it returned
 *                                     (menuret), the compiler's error
 *   POINTER ID busy|normal            behind a menu item: the busy pointer
 *                                     (sleepon without pointeron)
 * plus the frame's requester lines.  Only a compiler that keeps waiting
 * for input or a failed extalloc end the session (RESULT ID failed abort:
 * ...); the runner goes on with the remaining jobs in a new session.
 */
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "edstub.h"

extern struct DosLibrary *DOSBase;

#define PATHLEN EDSTUB_PATHLEN
static char srcname[PATHLEN], outname[PATHLEN], xtraname[PATHLEN], defname[PATHLEN];
static char cur_id[16];
static int in_menu, runerr_off;
static ULONG strbuf, macrobuf;

static char lbuf[600];
#define LOG_BEGIN struct sb L = { lbuf, 0, sizeof lbuf }; lbuf[0] = 0
#define LOG_END   edstub_log(lbuf)

/* TED line item (ted.s rsreset) */
struct item {
    struct item *next, *prev;
    UBYTE numchars;
    UBYTE chars[];
};

static struct item *firstitem;
static UWORD numlines;

/* ---------------------------------------------------------------------
 * error: compile errors end the pass (err longjmps out), so there is at
 * most one per menu call.  Inside the menu call the line number is only
 * known after it returns (d0 = menuret = linenumat), so the message is
 * kept until then; outside of it the message is logged at once.
 * ------------------------------------------------------------------- */
static char errmsg[256];
static int errpending;

static void error_hook(const char *m)
{
    LOG_BEGIN;

    if (in_menu) {
        if (!errpending) {
            edstub_strcpy(errmsg, m, sizeof errmsg);
            errpending = 1;
        }
        return;
    }
    sbs(&L, "XTRAERR\t");
    sbs(&L, cur_id);
    sbc(&L, '\t');
    sbs(&L, m);
    LOG_END;
}

/* menu=: the answer for every file requester of the menu item */
static const char *menu_answer;
static int in_menu_job;

/* the only file requester of the batch flow: CREATE EXECUTABLE */
static const char *ezfreq_hook(const char *title)
{
    if (in_menu_job)
        return menu_answer;
    return edstub_streq(title, "Name of Executable to create") ? outname : 0;
}

/* ---------------------------------------------------------------------
 * source: TED's readline/allocline
 * ------------------------------------------------------------------- */
#define TABSTOP 2                       /* ted.s tabstop default */
#define LINEBUF 130                     /* chcols + 2 */

static struct item *allocline(const UBYTE *buf, ULONG d5)
{
    struct item *it = AllocMem(sizeof(struct item) + d5, MEMF_PUBLIC | MEMF_CLEAR);
    ULONG i;

    if (!it)
        return 0;
    it->numchars = d5;
    for (i = 0; i < d5; i++) {          /* ted copies up to the first 0 */
        it->chars[i] = buf[i];
        if (!buf[i])
            break;
    }
    return it;
}

static void freelines(void)
{
    struct item *it = firstitem, *n;

    while (it) {
        n = it->next;
        FreeMem(it, sizeof(struct item) + it->numchars);
        it = n;
    }
    firstitem = 0;
    numlines = 0;
}

static int load_source(void)
{
    BPTR fh = Open(srcname, MODE_OLDFILE);
    LONG size, pos = 0;
    UBYTE *data, buf[LINEBUF + 8];
    struct item *last = 0, *it;

    if (!fh)
        return 0;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    data = size > 0 ? AllocMem(size, MEMF_PUBLIC) : 0;
    if (size > 0 && (!data || Read(fh, data, size) != size)) {
        Close(fh);
        if (data)
            FreeMem(data, size);
        return 0;
    }
    Close(fh);

    while (pos < size) {                /* readline */
        ULONG a = 0;
        UBYTE c = data[pos++];

        for (;;) {
            if (c & 0x80) {             /* token: next byte unchecked */
                buf[a++] = c;
                if (pos >= size)
                    break;              /* .oops */
                buf[a++] = data[pos++];
            } else {
                if (c == 10)
                    c = 0;              /* ascii line */
                buf[a++] = c;
                if (!c)
                    goto eol;
                if (c == 9) {
                    ULONG col;

                    a--;
                    col = a;
                    do {                /* TABSTOP: a power of 2 */
                        buf[a++] = ' ';
                        col++;
                    } while (col & (TABSTOP - 1));
                }
            }
            if (a >= LINEBUF) {         /* .oops */
                a = LINEBUF - 3;
                break;
            }
            if (pos >= size)
                break;
            c = data[pos++];
        }
        buf[a++] = 0;                   /* .oops: no eol */
eol:
        if (!(it = allocline(buf, a))) {
            freelines();
            FreeMem(data, size);
            edstub_log("FATAL: out of memory loading source");
            return 0;
        }
        if (last) {
            last->next = it;
            it->prev = last;
        } else
            firstitem = it;
        last = it;
        numlines++;
    }
    if (data)
        FreeMem(data, size);
    if (!firstitem) {                   /* ted: donew, one empty line */
        buf[0] = 0;
        if (!(firstitem = allocline(buf, 1)))
            return 0;
        numlines = 1;
    }
    return 1;
}

/* one line with the compiler's token list (ted.s detok), for the log;
   tabs and newlines become spaces (TSV field) */
static void detok(struct sb *b, const UBYTE *s)
{
    while (*s) {
        UBYTE c = *s++;

        if (c & 0x80) {
            UWORD n = ((c << 8) | *s) & 0x7fff;
            UBYTE *t = edstub_ed->tokens;

            if (*s)
                s++;
            while (t && *(UWORD *)(t + 4) != n)
                t = *(UBYTE **)t;
            if (t)
                sbs(b, (const char *)t + 6);
            else {
                sbs(b, "{$");
                sbx(b, n | 0x8000, 4);
                sbc(b, '}');
            }
        } else if (c < 32) {
            sbs(b, "\\x");
            sbx(b, c, 2);
        } else
            sbc(b, c);
    }
}

static void tsv(struct sb *b, const char *s)    /* field without tabs/newlines */
{
    for (; *s; s++)
        sbc(b, (*s == '\t' || *s == '\n' || *s == '\r') ? ' ' : *s);
}

/* ---------------------------------------------------------------------
 * the compiler's callbacks
 * ------------------------------------------------------------------- */
/* ted: d7 = file handle, a6 = dos */
static void call_xtra(APTR fn, BPTR fh)
{
    struct regs cr;

    memset(&cr, 0, sizeof cr);
    cr.d[7] = fh;
    cr.a[6] = (ULONG)DOSBase;
    edstub_callregs(fn, &cr);
}

static void call_loadxtra(void)
{
    BPTR fh;

    if (edstub_streq(xtraname, "none") || !edstub_ed->loadxtra)
        return;
    if ((fh = Open(xtraname, MODE_OLDFILE))) {
        call_xtra(edstub_ed->loadxtra, fh);
        Close(fh);
    }
}

static void save_defaults(void)
{
    BPTR fh;

    if (!defname[0] || !edstub_ed->savextra)
        return;
    if (!(fh = Open(defname, MODE_NEWFILE))) {
        defname[0] = 0;
        return;
    }
    call_xtra(edstub_ed->savextra, fh);
    if (runerr_off) {
        static const UBYTE off[4];

        Seek(fh, 2, OFFSET_BEGINNING);  /* options runtime errors, debugger */
        Write(fh, (APTR)off, 4);
    }
    if (macrobuf) {
        Seek(fh, 46, OFFSET_BEGINNING); /* buffer sizes: the macro buffer */
        Write(fh, &macrobuf, 4);
    }
    if (strbuf) {
        Seek(fh, 50, OFFSET_BEGINNING); /* buffer sizes: the string buffer */
        Write(fh, &strbuf, 4);
    }
    Close(fh);
}

#define MENU_CREATE_EXECUTABLE 2        /* blitz2.s menuadds: menu0,menu10,menu1 */

/* the compiler's menu callback: d6 = item, a5 = first line, d7 = lines;
   returns d0 = the line of the error (menuret) */
static WORD call_menu(UWORD item)
{
    struct regs cr;

    errpending = 0;
    memset(&cr, 0, sizeof cr);
    cr.d[6] = item;
    cr.d[7] = numlines;
    cr.a[5] = (ULONG)firstitem;
    in_menu = 1;
    edstub_callregs(edstub_ed->menu, &cr);
    in_menu = 0;
    return (WORD)cr.d[0];
}

static LONG filesize(const char *name)
{
    BPTR l = Lock(name, ACCESS_READ);
    LONG size = -1;

    if (l) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, 0);

        if (fib) {
            if (Examine(l, fib))
                size = fib->fib_Size;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(l);
    }
    return size;
}

/* the frame aborts the session: this job failed */
static void abort_hook(const char *why)
{
    LOG_BEGIN;

    sbs(&L, "RESULT\t");
    sbs(&L, cur_id);
    sbs(&L, "\tfailed\tabort: ");
    sbs(&L, why);
    LOG_END;
    freelines();
    in_menu = 0;
}

/* ---------------------------------------------------------------------
 * the jobs
 * ------------------------------------------------------------------- */
static void batch_job(char *id, char *src, char *out, char *dirname)
{
    BPTR dir, old = 0;
    WORD ret;
    LONG size;

    edstub_strcpy(cur_id, id, sizeof cur_id);
    edstub_strcpy(srcname, src, PATHLEN);
    edstub_strcpy(outname, out, PATHLEN);
    edstub_strcpy(xtraname, src, PATHLEN);
    edstub_strcat(xtraname, ".xtra", PATHLEN);
    edstub_reset_requests();
    {
        LOG_BEGIN;
        sbs(&L, "JOB\t");
        sbs(&L, id);
        sbc(&L, '\t');
        sbs(&L, src);
        LOG_END;
    }
    if (dirname && *dirname)
        dir = Lock(dirname, ACCESS_READ);
    else {
        BPTR l = Lock(src, ACCESS_READ);

        dir = l ? ParentDir(l) : 0;
        if (l)
            UnLock(l);
    }
    if (dir)
        old = CurrentDir(dir);
    if (!load_source()) {
        LOG_BEGIN;
        sbs(&L, "RESULT\t");
        sbs(&L, id);
        sbs(&L, "\tfailed\tsource not loaded");
        LOG_END;
        goto out;
    }
    {
        BPTR fh = Open(xtraname, MODE_OLDFILE);
        LOG_BEGIN;

        sbs(&L, "XTRA\t");
        sbs(&L, id);
        if (fh) {
            Close(fh);
            sbs(&L, "\town");
        } else if (defname[0]) {
            edstub_strcpy(xtraname, defname, PATHLEN);
            sbs(&L, "\tdefaults");
        } else {
            edstub_strcpy(xtraname, "none", PATHLEN);
            sbs(&L, "\tnone");
        }
        LOG_END;
        call_loadxtra();
    }
    DeleteFile(outname);                /* no stale executable */
    ret = call_menu(MENU_CREATE_EXECUTABLE);
    size = filesize(outname);
    {
        LOG_BEGIN;

        sbs(&L, "RESULT\t");
        sbs(&L, id);
        if (errpending) {
            struct item *it = firstitem;
            WORD i;

            for (i = 0; it && i < ret; i++)
                it = it->next;
            sbs(&L, "\terror\t");
            sbn(&L, ret + 1);
            sbc(&L, '\t');
            tsv(&L, errmsg);
            sbc(&L, '\t');
            if (ret >= 0 && it) {
                char tb[300];
                struct sb t = { tb, 0, sizeof tb };

                tb[0] = 0;
                detok(&t, it->chars);
                tsv(&L, tb);
            }
        } else if (size >= 0) {
            sbs(&L, "\tok\t");
            sbn(&L, size);
        } else {
            sbs(&L, "\tfailed\tno error message, no executable, menu returned ");
            sbn(&L, ret);
        }
        LOG_END;
    }
out:
    freelines();
    if (dir) {
        CurrentDir(old);
        UnLock(dir);
    }
    cur_id[0] = 0;
}

/* menu=ID|ITEM|ANSWER: a menu item without a source */
static void menu_job(char *id, char *item, char *answer)
{
    UWORD n = 0;
    WORD ret;

    while (*item >= '0' && *item <= '9')
        n = n * 10 + (UWORD)(*item++ - '0');
    edstub_strcpy(cur_id, id, sizeof cur_id);
    edstub_reset_requests();
    menu_answer = answer && *answer ? answer : 0;
    in_menu_job = 1;
    ret = call_menu(n);
    in_menu_job = 0;
    {
        LOG_BEGIN;

        sbs(&L, "MENU\t");
        sbs(&L, id);
        sbc(&L, '\t');
        sbn(&L, n);
        sbc(&L, '\t');
        sbn(&L, ret);
        if (errpending) {
            sbc(&L, '\t');
            tsv(&L, errmsg);
        }
        LOG_END;
    }
    {
        LOG_BEGIN;

        sbs(&L, "POINTER\t");
        sbs(&L, id);
        sbs(&L, edstub_busy ? "\tbusy" : "\tnormal");
        LOG_END;
    }
    cur_id[0] = 0;
}

/* VALUE of "KEY=VALUE" up to the end of the line (trailing white space
   removed), or 0 */
static char *value(char *line, const char *key)
{
    ULONG n;

    if (!edstub_prefix(line, key))
        return 0;
    line += edstub_strlen(key);
    n = edstub_strlen(line);
    while (n && (line[n - 1] == ' ' || line[n - 1] == '\t' || line[n - 1] == '\r'))
        line[--n] = 0;
    return line;
}

/* the next field up to '|' (0-terminated), p behind it */
static char *field(char **p)
{
    char *f = *p;

    while (**p && **p != '|')
        (*p)++;
    if (**p)
        *(*p)++ = 0;
    return f;
}

void edstub_main(const char *arg)
{
    BPTR fh = Open(arg, MODE_OLDFILE);
    LONG size;
    char *buf, *p, *v;

    edstub_error_hook = error_hook;
    edstub_ezfreq_hook = ezfreq_hook;
    edstub_abort_hook = abort_hook;
    if (!fh) {
        edstub_log("FATAL: cannot open job list");
        return;
    }
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    buf = size > 0 ? AllocMem(size + 1, MEMF_PUBLIC) : 0;
    if (!buf || Read(fh, buf, size) != size) {
        Close(fh);
        if (buf)
            FreeMem(buf, size + 1);
        edstub_log("FATAL: cannot read job list");
        return;
    }
    Close(fh);
    buf[size] = 0;
    /* one line per entry */
    for (p = buf; *p; p++)
        if (*p == '\n')
            *p = 0;

    /* pass 1: settings */
    for (p = buf; p < buf + size; p += edstub_strlen(p) + 1)
        if ((v = value(p, "log=")))
            edstub_set_log(v);
        else if ((v = value(p, "defaults=")))
            edstub_strcpy(defname, v, PATHLEN);
        else if ((v = value(p, "runerr=")))
            runerr_off = edstub_streq(v, "off");
        else if ((v = value(p, "strbuf=")))
            while (*v >= '0' && *v <= '9')
                strbuf = strbuf * 10 + (ULONG)(*v++ - '0');
        else if ((v = value(p, "macrobuf=")))
            while (*v >= '0' && *v <= '9')
                macrobuf = macrobuf * 10 + (ULONG)(*v++ - '0');
    edstub_log("BATCH\tstart");
    save_defaults();
    /* pass 2: jobs */
    for (p = buf; p < buf + size; p += edstub_strlen(p) + 1)
        if ((v = value(p, "job="))) {
            char *id = field(&v), *src = field(&v), *out = field(&v), *dir = field(&v);

            batch_job(id, src, out, dir);
        } else if ((v = value(p, "menu="))) {
            char *id = field(&v), *item = field(&v), *answer = field(&v);

            menu_job(id, item, answer);
        }
    edstub_log("BATCH\tend");
    FreeMem(buf, size + 1);
}
