/*
 * ab3main.c - the command line run of blitz2-ab3 (ab3main.h; spec
 * ab3-compat/command-line, design D2/D4 of the change ab3-mode-g1).  Only
 * in blitz2-ab3: blitz2 has no command line but the file to edit.
 *
 *   blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2]
 *
 * The arguments are parsed with shared/ab3job; the mode of the program is
 * decided before the compiler loads anything (progmode, shared/xtra
 * xtra_mode: -mode, AmiBlitz3's embedded header or a .ab3 name, the
 * companion .xtra of a tokenised source); then begin() runs as for the
 * editor, but with PROGDIR:ab3cli as the editor and the job as its
 * argument (main.c).
 */
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3main.h"

struct ab3job *ab3job;
const char ab3cliname[] = "PROGDIR:ab3cli";
static struct ab3job job;

static void say(const char *s)
{
    BPTR out = Output();

    if (out)
        Write(out, (APTR)s, (LONG)strlen(s));
}

static void sayline(const char *a, const char *b)
{
    say("blitz2-ab3: ");
    say(a);
    if (b)
        say(b);
    say("\n");
}

/* a whole file into memory (AllocMem, *len bytes, at least 1 allocated);
   NULL if it cannot be opened or read */
static UBYTE *readall(const char *name, LONG *len)
{
    BPTR fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    UBYTE *buf = NULL;
    LONG size;

    *len = 0;
    if (!fh)
        return NULL;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size >= 0 && (buf = AllocMem((ULONG)size + 1, MEMF_PUBLIC))
        && Read(fh, buf, size) != size) {
        FreeMem(buf, (ULONG)size + 1);
        buf = NULL;
    }
    Close(fh);
    if (buf)
        *len = size;
    return buf;
}

int ab3_cmdline(int argc, char **argv)
{
    char msg[AB3JOB_PATHLEN + 64], compname[AB3JOB_PATHLEN + 8];
    UBYTE *src, *x;
    LONG len, xlen = 0;
    int r = ab3args_parse(argc, argv, &job, msg, sizeof msg);

    switch (r) {
    case AB3ARGS_EDITOR:
        return -1;
    case AB3ARGS_HELP:
        say(ab3args_usage);
        return ab3args_rc(r);
    case AB3ARGS_UNSUPPORTED:
        sayline(msg, NULL);
        return ab3args_rc(r);
    case AB3ARGS_FAIL:
        sayline(msg, NULL);
        say(ab3args_usage);
        return ab3args_rc(r);
    }
    /* the mode: by the source and its companion .xtra (D4) */
    if (!(src = readall(job.source, &len))) {
        sayline("cannot read the source ", job.source);
        return AB3JOB_RC_FAIL;
    }
    x = NULL;
    if (xtra_companion_name(job.source, compname, sizeof compname) == XTRA_OK)
        x = readall(compname, &xlen);
    job.mode = (uint8_t)xtra_mode(job.force, src, (size_t)len, job.source, x,
                                  x ? (size_t)xlen : 0);
    if (x)
        FreeMem(x, (ULONG)xlen + 1);
    FreeMem(src, (ULONG)len + 1);
    progmode = job.mode;
    ab3job = &job;
    return -1;
}

/* -------------------------------- includes of a command line run (3.5, 3.6) */

#define MAXINC 16                       /* open at the same time (nested) */
static struct {
    struct include *inc;                /* NULL: a free slot */
    ULONG line;                         /* its lines read so far (3.6) */
    UBYTE *mem;                         /* a text include: its lines,
                                           AllocMem(len + 1) by ab3cli */
    ULONG len, pos;
} incs[MAXINC];

/* the slot of inc (NULL: a free one); MAXINC if there is none */
static int incslot(const struct include *inc)
{
    int i;

    for (i = 0; i < MAXINC && incs[i].inc != inc; i++)
        ;
    return i;
}

void ab3_incopen(struct include *inc)
{
    int i;

    if (!ab3job || (i = incslot(NULL)) == MAXINC)
        return;                         /* full: its errors at the outer place */
    memset(&incs[i], 0, sizeof incs[i]);
    incs[i].inc = inc;
}

void ab3_textinc(struct include *inc, const char *name)
{
    static char msg[] = "Can't read include";
    struct m68kregs r;
    int i = incslot(inc);

    if (i == MAXINC)
        err("Too many includes of text");
    memset(&r, 0, sizeof r);
    r.a[0] = (ULONG)name;
    callregs(ab3job->tokfile, &r);
    if (!r.d[0])
        err(msg);                       /* ab3cli printed why */
    incs[i].mem = (UBYTE *)r.d[0];
    incs[i].len = r.d[1];
    incs[i].pos = 0;
}

LONG ab3_incread(struct include *inc, UBYTE *dst, LONG len)
{
    ULONG n;
    int i = incslot(inc);

    if (i == MAXINC || !incs[i].mem)
        return Read(inc->fh, dst, len);
    n = incs[i].len - incs[i].pos;
    if (n > (ULONG)len)
        n = (ULONG)len;
    memcpy(dst, incs[i].mem + incs[i].pos, n);
    incs[i].pos += n;
    return (LONG)n;
}

void ab3_incline(struct include *inc)
{
    int i = incslot(inc);

    if (i < MAXINC)
        incs[i].line++;
}

void ab3_incdone(struct include *inc)
{
    int i;

    if (!inc || (i = incslot(inc)) == MAXINC)
        return;
    if (incs[i].mem)
        FreeMem(incs[i].mem, incs[i].len + 1);
    incs[i].inc = NULL;
    incs[i].mem = NULL;
}

void ab3_errpos(void)
{
    struct include *inc;
    int i;

    if (!ab3job || ab3job->errset)
        return;
    ab3job->errset = 1;
    ab3job->errline = 0;
    /* the innermost include with a line read; one just opened (an error
       in opening it, or of its kind) belongs to the INCLUDE line outside */
    for (inc = firstinc; inc; inc = inc->next)
        if ((i = incslot(inc)) < MAXINC && incs[i].line) {
            strncpy(ab3job->errfile, inc->name, AB3JOB_PATHLEN - 1);
            ab3job->errfile[AB3JOB_PATHLEN - 1] = 0;
            ab3job->errline = incs[i].line;
            return;
        }
}

int ab3_result(void)
{
    if (!job.started) {
        sayline("the compiler did not start ", ab3cliname);
        return AB3JOB_RC_FAIL;
    }
    return job.result;
}
