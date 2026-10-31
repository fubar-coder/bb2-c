/*
 * main.c - ab3cli, the headless editor of a command line run of
 * blitz2-ab3 (design D2 of the change ab3-mode-g1, docs/ab3/variant.md).
 *
 * blitz2-ab3 -s SOURCE -e EXE loads PROGDIR:ab3cli instead of blitz2:ted
 * (edstub frame, shared/edstub) and calls its main entry with a0 = the job
 * (shared/ab3job/ab3job.h).  ab3cli
 *   - loads SOURCE with TED's core as TED does (fileopen2: a text source
 *     is tokenised with the token list blitz2 handed over - the one of the
 *     program's mode, a tokenised source is taken as it is),
 *   - hands the compiler the options of the source (loadxtra): from
 *     AmiBlitz3's embedded header (shared/xtra, written as a Blitz 2
 *     .xtra image), else from the companion SOURCE.xtra, else the
 *     compiler's own; with -r without runtime error checking, debugger and
 *     debug information,
 *   - compiles with CREATE EXECUTABLE into EXE,
 *   - prints what went wrong on the console - a compile error as
 *     "FILE:LINE: MESSAGE" - and leaves the return code in the job: 0
 *     compiled, 10 compile error, 20 set-up error (design D6).
 * Relative names (source, executable, includes) are relative to the
 * shell's current directory.
 *
 * Built with TED's options (-fbaserel, the frame compiled with them and
 * EDSTUB_INIT): an editor-world program, not linked into blitz2.
 */
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "edstub.h"
#include "ab3cli.h"
#include "ab3job.h"
#include "xtra.h"
#include "fileio.h"
#include "hooks.h"
#include "loadsave.h"
#include "state.h"
#include "tokit.h"

extern struct DosLibrary *DOSBase;

static struct ab3job *job;
static const char *curfile;             /* the file messages are about */
static int in_menu, errpending, loaderr, setuperr;
static char errmsg[256];

/* ---------------------------------------------------------------------
 * the console
 * ------------------------------------------------------------------- */
static char obuf[600];
#define OUT_BEGIN struct sb L = { obuf, 0, sizeof obuf }; obuf[0] = 0
#define OUT_END   sbc(&L, '\n'); say(obuf)

static void say(const char *s)
{
    BPTR out = Output();

    if (out)
        Write(out, (APTR)s, (LONG)edstub_strlen(s));
}

/* "FILE: msg" (FILE: the file worked on, before the job blitz2-ab3) */
static void report(const char *msg)
{
    OUT_BEGIN;

    sbs(&L, curfile ? curfile : "blitz2-ab3");
    sbs(&L, ": ");
    sbs(&L, msg);
    OUT_END;
}

void ab3cli_fatal(const char *why)
{
    report(why);
    if (job)
        job->result = AB3JOB_RC_FAIL;
    in_menu = 0;
    edstub_abort();
}

void ab3cli_loaderror(const char *msg)
{
    loaderr = 1;
    report(msg);
}

/* ---------------------------------------------------------------------
 * the compiler's services
 * ------------------------------------------------------------------- */
/* the compiler's error requester: during the compile the first message
   is kept (the line is known when the menu call returns), else (loading
   libraries or residents) it is a set-up error */
static void error_hook(const char *m)
{
    if (in_menu) {
        if (!errpending) {
            edstub_strcpy(errmsg, m, sizeof errmsg);
            errpending = 1;
        }
        return;
    }
    setuperr++;
    report(m);
}

static const char *ezfreq_hook(const char *title)
{
    return job && edstub_streq(title, "Name of Executable to create") ? job->exe : 0;
}

/* the frame gives up (the compiler waits for input forever) */
static void abort_hook(const char *why)
{
    report(why);
    if (job)
        job->result = AB3JOB_RC_FAIL;
    in_menu = 0;
}

void edstub_init(void)
{
    edstub_quiet = 1;                   /* the frame's log is not for the user */
    edstub_error_hook = error_hook;
    edstub_ezfreq_hook = ezfreq_hook;
    edstub_abort_hook = abort_hook;
}

#define MENU_CREATE_EXECUTABLE 2        /* blitz2.s menuadds: menu0,menu10,menu1 */

/* the compiler's menu callback: d6 = item, a5 = first line, d7 = lines;
   returns d0 = the line of an error (menuret, 0-based) */
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

/* the compiler's loadxtra/savextra: d7 = file handle, a6 = DOSBase */
static void call_xtra(APTR fn, BPTR fh)
{
    struct regs cr;

    memset(&cr, 0, sizeof cr);
    cr.d[7] = (ULONG)fh;
    cr.a[6] = (ULONG)DOSBase;
    edstub_callregs(fn, &cr);
}

/* ---------------------------------------------------------------------
 * files
 * ------------------------------------------------------------------- */
/* a whole file (AllocMem, *len + 1 bytes); NULL if it cannot be opened
   (*len = 0) or read (*len = -1) */
static UBYTE *readfile(const char *name, LONG *len)
{
    BPTR fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    UBYTE *buf = 0;
    LONG size;

    *len = 0;
    if (!fh)
        return 0;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    *len = -1;
    if (size >= 0 && (buf = AllocMem((ULONG)size + 1, MEMF_PUBLIC))) {
        if (Read(fh, buf, size) == size)
            *len = size;
        else {
            FreeMem(buf, (ULONG)size + 1);
            buf = 0;
        }
    }
    Close(fh);
    return buf;
}

static void freefile(UBYTE *buf, LONG len)
{
    if (buf)
        FreeMem(buf, (ULONG)len + 1);
}

static int exists(const char *name)
{
    BPTR l = Lock((CONST_STRPTR)name, ACCESS_READ);

    if (l)
        UnLock(l);
    return l != 0;
}

/* a temporary file for an .xtra image: T:, else RAM: (T: may be on a
   read-only disk); NEWFILE handle, name in name[] */
static BPTR tempfile(char *name, ULONG cap)
{
    static const char *const dirs[] = { "T:", "RAM:" };
    BPTR fh = 0;
    int i;

    for (i = 0; i < 2 && !fh; i++) {
        struct sb n = { name, 0, cap };

        name[0] = 0;
        sbs(&n, dirs[i]);
        sbs(&n, "ab3cli-");
        sbx(&n, (ULONG)FindTask(0), 8);
        sbs(&n, ".xtra");
        fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    }
    if (!fh)
        report("cannot write a temporary file in T: or RAM:");
    return fh;
}

/* the compiler's options now, as a Blitz 2 .xtra (savextra); NULL if not
   written */
static UBYTE *saveoptions(LONG *len)
{
    char name[40];
    BPTR fh = tempfile(name, sizeof name);
    UBYTE *buf;

    *len = 0;
    if (!fh)
        return 0;
    call_xtra(edstub_ed->savextra, fh);
    Close(fh);
    buf = readfile(name, len);
    DeleteFile((CONST_STRPTR)name);
    if (!buf)
        report("cannot read the compiler's options back");
    return buf;
}

/* hand the compiler an .xtra image (loadxtra); 0 if not possible */
static int loadimage(const UBYTE *img, ULONG len)
{
    char name[40];
    BPTR fh = tempfile(name, sizeof name);
    int ok;

    if (!fh)
        return 0;
    ok = Write(fh, (APTR)img, (LONG)len) == (LONG)len;
    Close(fh);
    if (ok && (fh = Open((CONST_STRPTR)name, MODE_OLDFILE))) {
        call_xtra(edstub_ed->loadxtra, fh);
        Close(fh);
    } else {
        report("cannot write a temporary file");
        ok = 0;
    }
    DeleteFile((CONST_STRPTR)name);
    return ok;
}

static void xtraerror(const char *file, const struct xtra_error *e)
{
    OUT_BEGIN;

    sbs(&L, file);
    if (e->line) {
        sbc(&L, ':');
        sbn(&L, (LONG)e->line);
    }
    sbs(&L, ": ");
    sbs(&L, xtra_strerror(e->code));
    OUT_END;
}

/* ---------------------------------------------------------------------
 * the options of the source
 * ------------------------------------------------------------------- */
/* the options: AmiBlitz3's embedded header, else the companion .xtra
   (handed over as it is unless -r changes it), else the compiler's own
   (nothing to do unless -r).  1 = done, 0 = set-up error (printed) */
/* the big ones static: this runs on the shell's stack (4096 bytes by
   default) below the compiler's loadxtra and loadres */
static char xname[AB3JOB_PATHLEN + 8];
static struct xtra x;
static struct xtra_error e;
static struct xtra_base base;

static int load_options(void)
{
    UBYTE *src, *xb = 0, *img = 0;
    LONG slen, xlen = 0;
    ULONG imglen = 0;
    size_t written;
    int type, ok = 0, have = 0;

    src = readfile(job->source, &slen);
    if (!src) {
        report("cannot read the source");
        return 0;
    }
    type = xtra_detect(src, (size_t)slen, job->source);
    if (type == XTRA_T_AB3) {
        if (xtra_load(&x, type, src, (size_t)slen, 0, 0, &e)) {
            xtraerror(job->source, &e);
            goto out;
        }
        have = x.ab3.header;            /* without one: the compiler's own */
    } else if (xtra_has_companion(type)
               && xtra_companion_name(job->source, xname, sizeof xname) == XTRA_OK
               && (xb = readfile(xname, &xlen))) {
        if (!job->release_exe) {            /* as TED: the file itself */
            BPTR fh = Open((CONST_STRPTR)xname, MODE_OLDFILE);

            if (fh) {
                call_xtra(edstub_ed->loadxtra, fh);
                Close(fh);
            }
            ok = 1;
            goto out;
        }
        if (xtra_load(&x, type, src, (size_t)slen, xb, (size_t)xlen, &e)) {
            xtraerror(xname, &e);
            goto out;
        }
        have = 1;
    } else if (xlen < 0) {
        report("cannot read the companion .xtra");
        goto out;
    }
    if (!have) {
        if (!job->release_exe) {
            ok = 1;
            goto out;
        }
        freefile(xb, xlen);
        if (!(xb = saveoptions(&xlen)))
            goto out;
        if (xtra_load(&x, XTRA_T_TED, 0, 0, xb, (size_t)xlen, &e)) {
            xtraerror("the compiler's options", &e);
            goto out;
        }
    }
    if (job->release_exe) {
        x.sw[XTRA_SW_RUNTIME_ERRORS] = 0;
        x.sw[XTRA_SW_DEBUGGER] = 0;
        x.sw[XTRA_SW_DEBUG_INFO] = 0;
    }
    base.xtra = xb;
    base.header = src;
    imglen = (ULONG)xtra_blitz2_size(&x);
    if (!(img = AllocMem(imglen, MEMF_PUBLIC | MEMF_CLEAR))) {
        report("out of memory");
        goto out;
    }
    if (xtra_write_blitz2(&x, &base, img, imglen, &written, 0, &e)) {
        xtraerror(job->source, &e);
        goto out;
    }
    ok = loadimage(img, (ULONG)written);
out:
    if (img)
        FreeMem(img, imglen);
    freefile(xb, xlen);
    freefile(src, slen);
    return ok;
}

/* ---------------------------------------------------------------------
 * the job
 * ------------------------------------------------------------------- */
#define BUFFLEN (doslen + 3 * 128 + 6)  /* ted.s outside: dosbuff + charbuff1..3 */

static int run(void)
{
    UBYTE *buffers = AllocMem(BUFFLEN, MEMF_PUBLIC);
    WORD ret;
    int rc = AB3JOB_RC_FAIL;

    if (!buffers) {
        report("out of memory");
        return rc;
    }
    /* TED's outside (ted.s:24): the compiler's edstruct, the buffers */
    dosbuff = buffers;
    charbuff = dosbuff + doslen;
    charbuff2 = charbuff + chcols + 2;
    charbuff3 = charbuff2 + chcols + 2;
    charbuff4 = charbuff3 + chcols + 2;
    external = edstub_ed;
    tokens = edstub_ed->tokens;
    colon = edstub_ed->colon;
    quote = edstub_ed->quote;
    label = edstub_ed->label;
    remark = edstub_ed->remark;
    cleanup = edstub_ed->cleanup;
    loadxtra = 0;                       /* fileopen2 leaves the options to us */
    savextra = 0;
    tokenise = tokens ? 1 : 0;          /* getstuff */
    donew();                            /* the empty source of TED's start */

    fileopen2((UBYTE *)job->source);
    if (loaderr)
        goto out;
    if (!load_options() || setuperr)
        goto out;
    DeleteFile((CONST_STRPTR)job->exe); /* no stale executable */
    ret = call_menu(MENU_CREATE_EXECUTABLE);
    if (errpending) {
        OUT_BEGIN;

        sbs(&L, job->source);
        if (ret >= 0) {
            sbc(&L, ':');
            sbn(&L, (LONG)ret + 1);
        }
        sbs(&L, ": ");
        sbs(&L, errmsg);
        OUT_END;
        rc = AB3JOB_RC_ERROR;
    } else if (exists(job->exe))
        rc = AB3JOB_RC_OK;
    else
        report("no executable written");
out:
    freeall();
    firstitem = 0;
    numlines = 0;
    FreeMem(buffers, BUFFLEN);
    return rc;
}

void edstub_main(const char *arg)
{
    job = (struct ab3job *)arg;
    if (!job || !edstub_streq(job->magic, AB3JOB_MAGIC)) {
        job = 0;
        say("ab3cli: the editor of blitz2-ab3 -s SOURCE -e EXE, not to be started otherwise\n");
        return;
    }
    job->started = 1;
    job->result = AB3JOB_RC_FAIL;
    if (setuperr)                       /* the compiler's set-up (printed) */
        return;
    curfile = job->source;
    job->result = (UBYTE)run();
}
