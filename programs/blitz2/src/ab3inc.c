/*
 * ab3inc.c - the includes of blitz2-ab3 (extension points of ext.h; spec
 * ab3-compat/command-line and source-mode, docs/ab3/variant.md "The
 * command line (G1)" and "Includes of AB3 mode (G2)").
 *
 * In both modes a tokenised include of the other mode is refused.  In a
 * command line run the lines readinc takes of every open include are
 * counted (the place of an error), and an include of text is tokenised by
 * ab3cli first (readinc takes only tokenised lines); an editor session
 * reads includes as blitz2 does.  In AB3 mode (change ab3-mode-g2, design
 * D4) the name of an include is found and judged as AmiBlitz3 does it:
 * searched in its four places with its extension fallback, of text or
 * tokenised by its name, other names refused, an XINCLUDE skipped by its
 * file part.
 */
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"
#include "tokenizer.h"

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
    ULONG hdr;                          /* its first hdr lines are the
                                           empty lines of its embedded
                                           header (ab3cli, AB3 mode) */
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
    incs[i].hdr = r.d[2];
    incs[i].pos = 0;
}

/* up to 16 nested includes are counted; the errors of a deeper one are
   placed at the next outer one.  Of text or tokenised: in AB3 mode by the
   name (ext_incname), else by the contents. */
void ext_incopen(struct include *inc)
{
    int i, text;

    if (ab3job && (i = incslot(NULL)) < MAXINC) {
        memset(&incs[i], 0, sizeof incs[i]);
        incs[i].inc = inc;
    }
    if (progmode == XTRA_MODE_AB3) {
        text = tokenizer_ab3_inctype((const char *)namebuff, strlen((const char *)namebuff))
               == TOKENIZER_AB3_TEXT;
        if (!text)
            checkincmode(inc->fh, inc->buf);    /* read as it is */
    } else
        text = checkincmode(inc->fh, inc->buf);
    if (text && ab3job && ab3job->tokfile)
        textinc(inc, (const char *)namebuff);
}

/* ------------------------------------------ AmiBlitz3's include names */

/* AmiBlitz3's includes directory: config\path_includes, the tooltype
   INCLUDEDIR, by default Blitz3: + Sourcecodes/Includes (AmiBlitz3.ab3:351
   at 58a11658) */
#define AB3_INCLUDES "Blitz3:Sourcecodes/Includes"
/* the longest name an include node holds (structs.h include: UBYTE len =
   19 + name + 0) */
#define MAXNAME 235

static char incpath[300], inctry[300], incname[sizeof namebuff];

/* ab3_GetIncludeLock (AmiBlitz3.ab3:25796): the name, then the names of
   its extension fallback; the lock of the first that is there, 0 if none */
static BPTR inclock(const char *name)
{
    BPTR l = 0;
    int k;

    for (k = 0; k < TOKENIZER_AB3_INCNAMES && !l; k++)
        if (tokenizer_ab3_incname(name, strlen(name), k, inctry, sizeof inctry))
            l = Lock((CONST_STRPTR)inctry, ACCESS_READ);
    return l;
}

/* dir + name as AddPart joins them, then inclock; 0 if it does not fit */
static BPTR inclockin(const char *dir, const char *name)
{
    strncpy(incpath, dir, sizeof incpath - 1);
    incpath[sizeof incpath - 1] = 0;
    return AddPart((STRPTR)incpath, (CONST_STRPTR)name, sizeof incpath) ? inclock(incpath) : 0;
}

/* check_incdir (AmiBlitz3.ab3:25836): the include name in namebuff
   (INCDIR + the name, makeiname) searched in the includes directory, the
   main source's directory, the directory of the innermost open include and
   the includes directory with the name's file part; the found file's full
   name (NameFromLock) replaces it.  Returns its length (len if not found:
   Open fails later). */
static UWORD resolve(UWORD len)
{
    BPTR l, src;

    strcpy(incname, (const char *)namebuff);
    l = inclockin(AB3_INCLUDES, incname);
    if (!l && (src = Lock((CONST_STRPTR)ab3job->source, ACCESS_READ))) {
        if (NameFromLock(src, (STRPTR)incpath, sizeof incpath)) {
            *PathPart((CONST_STRPTR)incpath) = 0;
            l = inclockin(incpath, incname);
        }
        UnLock(src);
    }
    if (!l && firstinc) {               /* the innermost one is the first */
        strncpy(incpath, firstinc->name, sizeof incpath - 1);
        incpath[sizeof incpath - 1] = 0;
        *PathPart((CONST_STRPTR)incpath) = 0;
        l = inclockin(incpath, incname);
    }
    if (!l)
        l = inclockin(AB3_INCLUDES, (const char *)FilePart((CONST_STRPTR)incname));
    if (l) {
        if (NameFromLock(l, (STRPTR)incpath, sizeof incpath) && strlen(incpath) <= MAXNAME) {
            strcpy((char *)namebuff, incpath);
            len = (UWORD)strlen(incpath);
        }
        UnLock(l);
    }
    return len;
}

/* processInclude (AmiBlitz3.ab3:20754): only names ending in ab3, AB3,
   ab2 (text) or bb2, .bb (tokenised) are included */
static void inctype(UWORD len)
{
    if (tokenizer_ab3_inctype((const char *)namebuff, len) == TOKENIZER_AB3_REFUSED)
        err("You can only include tokenized files (.bb2 |.bb)");
}

/* AB3 mode keeps the INCDIR string in front of a name with ':' too (the
   spec's "INCDIR followed by the name"; whether AmiBlitz3 has blitz2's
   #341 is not measured); BB2 mode uses such a name as it is */
BOOL ext_incdevice(void)
{
    return progmode != XTRA_MODE_AB3;
}

/* INCLUDE: found and judged as AmiBlitz3 does it.  The type is judged
   before blitz2's "Already Included" (AmiBlitz3: after it) - only a
   recursive include of a refused name sees the difference. */
UWORD ext_incname(UWORD len)
{
    if (progmode != XTRA_MODE_AB3)
        return len;
    len = resolve(len);
    inctype(len);
    return len;
}

/* do_xinclude (AmiBlitz3.ab3:20615): the found name's file part decides
   whether it was included already; the file is included by its full name */
BOOL ext_xinclude(UWORD len)
{
    struct node *last;
    char *fp;
    UWORD fplen;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    len = resolve(len);
    strcpy(incname, (const char *)namebuff);
    for (fp = incname + len; fp > incname && fp[-1] != ':' && fp[-1] != '/'; fp--)
        ;
    fplen = (UWORD)strlen(fp);
    strcpy((char *)namebuff, fp);
    if (findxinc(fplen, &last)) {
        strcpy((char *)namebuff, incname);
        return TRUE;                    /* skipped */
    }
    addhere(last, NAMEOFF_XINC, fplen);
    strcpy((char *)namebuff, incname);
    inctype(len);
    doinc2(len);
}

UWORD ab3_incdepth(void)
{
    return (UWORD)numincs;
}

/* ---------------- the line being read, for the escapes (change ab3-mode-g3) */

BOOL ab3_incheaderline(void)
{
    int i;

    if (!numincs || !firstinc || (i = incslot(firstinc)) == MAXINC)
        return FALSE;
    return incs[i].line <= incs[i].hdr;
}

const char *ab3_incfile(void)
{
    if (!numincs || !firstinc)
        return "";
    return (const char *)FilePart((CONST_STRPTR)firstinc->name);
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
void ext_errpos(void)
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
