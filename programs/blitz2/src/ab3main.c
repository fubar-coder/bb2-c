/*
 * ab3main.c - the command line run of blitz2-ab3 (extension points of
 * ext.h; spec ab3-compat/command-line, design D2/D4 of the change
 * ab3-mode-g1).  Only in blitz2-ab3: blitz2 has no command line but the
 * file to edit.
 *
 *   blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2]
 *
 * The arguments are parsed with shared/ab3job; the mode of the program is
 * decided before the compiler loads anything (progmode, shared/xtra
 * xtra_mode: -mode, AmiBlitz3's embedded header or a .ab3 name, the
 * companion .xtra of a tokenised source); then begin() runs as for the
 * editor, but with PROGDIR:ab3cli as the editor and the job as its
 * argument (main.c begin, ext_editor/ext_editorarg).
 */
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"

struct ab3job *ab3job;                  /* ab3.h */
/* the editor of a command line run (instead of blitz2:ted) */
static const char ab3cliname[] = "PROGDIR:ab3cli";
static struct ab3job job;
UBYTE ab3version[3];                    /* ab3.h */
ULONG ab3builds;

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

/* the Version and NumberOfBuilds of AmiBlitz3's embedded header of the
   source src (len bytes) for the escapes \\__VER_* (ab3str.c); 0 without
   a header or a value.  A header xtra_load refuses is reported by ab3cli,
   which loads it again for the options. */
static void headerversion(const UBYTE *src, LONG len)
{
    static struct xtra x;               /* not on the shell's small stack */

    if (xtra_detect(src, (size_t)len, job.source) != XTRA_T_AB3
        || xtra_load(&x, XTRA_T_AB3, src, (size_t)len, 0, 0, 0) != XTRA_OK)
        return;
    memcpy(ab3version, x.ab3.version, sizeof ab3version);
    ab3builds = x.ab3.builds;
}

/* the command line (-s SOURCE -e EXE ...), before anything is loaded (the
   libraries depend on the mode): for a compile decide the program's mode
   and set ab3job.  Prints the usage and the messages of argument errors
   itself.  Returns -1: go on (editor session, or the compile of ab3job);
   else the return code of the run (usage, argument or set-up error) */
static int cmdline(int argc, char **argv)
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
    if (job.mode == XTRA_MODE_AB3)
        headerversion(src, len);
    if (x)
        FreeMem(x, (ULONG)xlen + 1);
    FreeMem(src, (ULONG)len + 1);
    ab3_setmode(job.mode);
    ab3job = &job;
    return -1;
}

BOOL ext_cmdline(int argc, char **argv, int *rc)
{
    int r;

    /* from the Workbench: the editor session */
    if (!argc || (r = cmdline(argc, argv)) < 0)
        return FALSE;
    *rc = r;
    return TRUE;
}

/* a command line run: ab3cli instead of TED, with the job */
const char *ext_editor(const char *name)
{
    return ab3job ? ab3cliname : name;
}

APTR ext_editorarg(APTR arg)
{
    return ab3job ? (APTR)ab3job : arg;
}

/* a command line run: ab3cli's return code (it left it in the job) */
BOOL ext_result(int *rc)
{
    if (!ab3job)
        return FALSE;
    if (!job.started) {
        sayline("the compiler did not start ", ab3cliname);
        *rc = AB3JOB_RC_FAIL;
    } else
        *rc = job.result;
    return TRUE;
}
