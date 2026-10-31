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

/* ------------------------------------------------- text includes (3.5) */

#define MAXTEXTINC 16                   /* open at the same time (nested) */
static struct {
    struct include *inc;
    UBYTE *mem;                         /* AllocMem(len + 1) by ab3cli */
    ULONG len, pos;
} textinc[MAXTEXTINC];

void ab3_textinc(struct include *inc, const char *name)
{
    static char msg[] = "Can't read include";
    struct m68kregs r;
    int i;

    for (i = 0; i < MAXTEXTINC && textinc[i].inc; i++)
        ;
    if (i == MAXTEXTINC)
        err("Too many includes of text");
    memset(&r, 0, sizeof r);
    r.a[0] = (ULONG)name;
    callregs(ab3job->tokfile, &r);
    if (!r.d[0])
        err(msg);                       /* ab3cli printed why */
    textinc[i].inc = inc;
    textinc[i].mem = (UBYTE *)r.d[0];
    textinc[i].len = r.d[1];
    textinc[i].pos = 0;
}

LONG ab3_incread(struct include *inc, UBYTE *dst, LONG len)
{
    ULONG n;
    int i;

    for (i = 0; i < MAXTEXTINC && textinc[i].inc != inc; i++)
        ;
    if (i == MAXTEXTINC)
        return Read(inc->fh, dst, len);
    n = textinc[i].len - textinc[i].pos;
    if (n > (ULONG)len)
        n = (ULONG)len;
    memcpy(dst, textinc[i].mem + textinc[i].pos, n);
    textinc[i].pos += n;
    return (LONG)n;
}

void ab3_incdone(struct include *inc)
{
    int i;

    for (i = 0; i < MAXTEXTINC; i++)
        if (inc && textinc[i].inc == inc) {
            FreeMem(textinc[i].mem, textinc[i].len + 1);
            textinc[i].inc = NULL;
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
