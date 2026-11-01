/*
 * ab3inc.c - the includes of blitz2-ab3 (extension points of ext.h; spec
 * ab3-compat/command-line, docs/ab3/variant.md "The command line (G1)").
 *
 * In both modes a tokenised include of the other mode is refused.  In a
 * command line run the lines readinc takes of every open include are
 * counted (the place of an error), and an include of text is tokenised by
 * ab3cli first (readinc takes only tokenised lines); an editor session
 * reads includes as blitz2 does.
 */
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"
#include "ab3main.h"

/* a tokenised include (TED's or BB22) has the mode of its own (xtra_mode:
   by its companion .xtra, as a main source); if that is not the program's,
   its tokens belong to the other token list - "Include of another mode:
   NAME" (docs/ab3/variant.md, "Mixed includes").  An include as text is
   not judged: it gets the program's mode.  fh is the open include, buf
   INCLEN bytes; the file position is the start again afterwards.  Returns
   1 for an include of text. */
static int checkincmode(BPTR fh, UBYTE *buf)
{
    static char msg[25 + 128] = "Include of another mode: ";
    char compname[sizeof namebuff + 5];
    UBYTE *x = NULL;
    LONG got, xlen = 0, i;
    BPTR xf;
    int type, mode;

    got = Read(fh, buf, INCLEN);
    if (got < 0)
        readerr();
    Seek(fh, 0, OFFSET_BEGINNING);
    type = xtra_detect(buf, (size_t)got, (const char *)namebuff);
    if (type != XTRA_T_TED && type != XTRA_T_BB22)
        return 1;
    if (xtra_companion_name((const char *)namebuff, compname, sizeof compname) == XTRA_OK
        && (xf = Open((CONST_STRPTR)compname, MODE_OLDFILE))) {
        Seek(xf, 0, OFFSET_END);
        xlen = Seek(xf, 0, OFFSET_BEGINNING);
        if (xlen > 0 && (x = AllocMem((ULONG)xlen, MEMF_PUBLIC)) && Read(xf, x, xlen) != xlen) {
            FreeMem(x, (ULONG)xlen);
            x = NULL;
        }
        Close(xf);
    }
    mode = xtra_mode(XTRA_MODE_DETECT, buf, (size_t)got, (const char *)namebuff, x,
                     x ? (size_t)xlen : 0);
    if (x)
        FreeMem(x, (ULONG)xlen);
    if (mode == progmode)
        return 0;
    for (i = 0; i < 127 && namebuff[i]; i++)
        msg[25 + i] = (char)namebuff[i];
    msg[25 + i] = 0;
    err(msg);
    return 0;
}

/* ------------------------------------ the includes of a command line run */

#define MAXINC 16                       /* open at the same time (nested) */
static struct {
    struct include *inc;                /* NULL: a free slot */
    ULONG line;                         /* its lines read so far */
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

/* a text include in a command line run: ab3cli tokenises it first, as TED
   loads a source, with the program's token list (job tokfile); readinc
   then reads those lines (ext_readinc) instead of the file.  An error
   stops the compile. */
static void textinc(struct include *inc, const char *name)
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

/* up to 16 nested includes are counted; the errors of a deeper one are
   placed at the next outer one */
void ext_incopen(struct include *inc)
{
    int i;

    if (ab3job && (i = incslot(NULL)) < MAXINC) {
        memset(&incs[i], 0, sizeof incs[i]);
        incs[i].inc = inc;
    }
    if (checkincmode(inc->fh, inc->buf) && ab3job && ab3job->tokfile)
        textinc(inc, (const char *)namebuff);
}

/* the tokenised lines of a text include, else the file (not handled) */
BOOL ext_readinc(struct include *inc, UBYTE *dst, LONG len, LONG *got)
{
    ULONG n;
    int i = incslot(inc);

    if (i == MAXINC || !incs[i].mem)
        return FALSE;
    n = incs[i].len - incs[i].pos;
    if (n > (ULONG)len)
        n = (ULONG)len;
    memcpy(dst, incs[i].mem + incs[i].pos, n);
    incs[i].pos += n;
    *got = (LONG)n;
    return TRUE;
}

void ext_incline(struct include *inc)
{
    int i = incslot(inc);

    if (i < MAXINC)
        incs[i].line++;
}

/* its tokenised lines go */
void ext_incdone(struct include *inc)
{
    int i;

    if (!inc || (i = incslot(inc)) == MAXINC)
        return;
    if (incs[i].mem)
        FreeMem(incs[i].mem, incs[i].len + 1);
    incs[i].inc = NULL;
    incs[i].mem = NULL;
}

/* a compile error: its place for ab3cli in the job - the first error of a
   compile only.  Inside an include: its name and the line read last (the
   innermost include that has a line read), else line 0 (the compiler's
   line of the source, the editor's line number).  Nothing in an editor
   session. */
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
