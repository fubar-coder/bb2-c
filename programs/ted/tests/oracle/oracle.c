/*
 * oracle.c - the TED oracle: the original TED (reference build of ted.s,
 * programs/ted ref/ted) as a library.  Runs as blitz2:ted under the
 * original blitz2 1.95 (edstub frame), so TED gets the compiler's token
 * list exactly as in the editor.
 *
 * Setup like ted.s `outside` (ted.s:24) without screen and windows:
 *   - the 14 words of edstruct into TED (tokens .. remark), but loadxtra,
 *     savextra and cleanup stay 0: no .xtra files, no compiler state
 *   - external, returncode; dosopen; IntuitionBase into `int`
 *   - the buffers dosbuff/charbuff..charbuff4 as outside allocates them
 *   - tokenise = 1 (getstuff: there is a token list)
 *   - bit 7 of defaultreqga9+13 cleared: realsave writes no .info
 *   - not called: rawed+32, loaddefs (defaults of ted.s, no
 *     l:blitzeditor.opts), screenopen
 * The display routines on the paths below are patched to rts in the loaded
 * copy, `error` to a jump to the frame's error service (logged, counted).
 * Labels: ted_labels.h, generated from ref/ted.labels.
 *
 * Argument: the job file, lines KEY=VALUE:
 *   ted=PATH   the reference TED          out=DIR   output directory
 *   log=PATH   log file                   src=PATH  one source (any number)
 * Outputs (NNNN = index of the src line, from 0):
 *   DIR/tokens.txt       the token list: "XXXX name" (word with bit 15)
 *   DIR/detok/NNNN.txt   tokenised source: per line (0-terminated chunk
 *                        p, p += strlen(p) + 1) detok + LF
 *   DIR/tokall/NNNN.bb2  every LF line of the detok text through tokit2,
 *                        its d5 bytes (with the 0) one after the other
 *   DIR/save/NNNN.bb2    every source: fileopen2, then realsave
 *   DIR/index.txt        "NNNN\tkind\tresult\terrors" per source
 *                        (kind tok/text; result ok or why not)
 */
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "edstub.h"
#include "ted_labels.h"

extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern void error_stub(void);

#define BUFLEN      32768               /* detok/tokit2 line buffers */
#define MAXCHUNK    1000                /* tokenised line: 500 tokens */
#define MAXNAME     31                  /* longest token name for BUFLEN */
#define DOSLEN      2048                /* ted.s:13 doslen (equ) */

static UBYTE *ted;                      /* first byte of the code hunk */
#define TED(off)    (ted + (off))
#define TEDW(off)   (*(UWORD *)TED(off))
#define TEDL(off)   (*(ULONG *)TED(off))

static char tedname[EDSTUB_PATHLEN], outdir[EDSTUB_PATHLEN];
static UBYTE *dosbuff;
static ULONG dosbufflen;
static UBYTE *inbuf, *outbuf, *textbuf;
static ULONG inclear;                   /* bytes of inbuf that may be used */
static ULONG errors;

/* display routines on the paths of fileopen2/realsave/donew */
static const ULONG rts_patches[] = {
    TED_pointeron, TED_sleepon, TED_showfname, TED_beepem, TED_showall,
    TED_showtopitem, TED_showbotitem, TED_showthisitem, TED_blokprnt2,
    TED_scrollup, TED_scrupall, TED_scrolldown, TED_scrdownall, TED_cls,
    TED_scrollrite, TED_scrollleft, TED_refresh, TED_showhprop, TED_showvprop,
};

static char lbuf[600];
#define LOG_BEGIN struct sb L = { lbuf, 0, sizeof lbuf }; lbuf[0] = 0
#define LOG_END   edstub_log(lbuf)

static void error_hook(const char *msg)
{
    LOG_BEGIN;

    errors++;
    sbs(&L, "TED error ");
    sbq(&L, msg);
    LOG_END;
}

/* ---------------------------------------------------------------------
 * buffered output file
 * ------------------------------------------------------------------- */
struct wfile {
    BPTR fh;
    ULONG n;
    int bad;
    UBYTE buf[4096];
};

static struct wfile wf;

static int wopen(struct wfile *w, const char *name)
{
    w->n = 0;
    w->bad = 0;
    w->fh = Open(name, MODE_NEWFILE);
    return w->fh != 0;
}

static void wflush(struct wfile *w)
{
    if (w->n && Write(w->fh, w->buf, w->n) != (LONG)w->n)
        w->bad = 1;
    w->n = 0;
}

static void wput(struct wfile *w, const UBYTE *p, ULONG n)
{
    while (n--) {
        if (w->n == sizeof w->buf)
            wflush(w);
        w->buf[w->n++] = *p++;
    }
}

static int wclose(struct wfile *w)
{
    wflush(w);
    Close(w->fh);
    w->fh = 0;
    return !w->bad;
}

/* NNNN: the index with 4 digits */
static void sbnum4(struct sb *b, ULONG index)
{
    ULONG digit[4], i;

    for (i = 0; i < 4; i++)
        index = edstub_udiv10(index, &digit[i]);
    for (i = 4; i--;)
        sbc(b, '0' + digit[i]);
}

/* DIR/sub/NNNN.ext */
static void outpath(char *d, const char *sub, ULONG index, const char *ext)
{
    struct sb b = { d, 0, EDSTUB_PATHLEN };

    d[0] = 0;
    sbs(&b, outdir);
    sbs(&b, sub);
    sbc(&b, '/');
    sbnum4(&b, index);
    sbs(&b, ext);
}

/* ---------------------------------------------------------------------
 * TED
 * ------------------------------------------------------------------- */
static ULONG call(ULONG off, struct regs *r)
{
    return edstub_callregs(TED(off), r);
}

static int ted_setup(void)
{
    struct regs r;
    BPTR seg = LoadSeg(tedname);
    UWORD chcols;
    ULONG i;
    LOG_BEGIN;

    if (!seg) {
        sbs(&L, "FATAL: cannot load ");
        sbs(&L, tedname);
        LOG_END;
        return 0;
    }
    ted = (UBYTE *)BADDR(seg) + 4;

    /* outside: the 14 words of edstruct, the hooks off */
    memcpy(TED(TED_tokens), edstub_ed, sizeof *edstub_ed);
    TEDL(TED_loadxtra) = 0;
    TEDL(TED_savextra) = 0;
    TEDL(TED_cleanup) = 0;
    TEDW(TED_returncode) = (UWORD)-1;
    TEDL(TED_external) = (ULONG)edstub_ed;
    memset(&r, 0, sizeof r);
    call(TED_dosopen, &r);
    TEDL(TED_int) = (ULONG)IntuitionBase;
    if (!TEDL(TED_dos)) {
        sbs(&L, "FATAL: dosopen failed");
        LOG_END;
        return 0;
    }

    /* outside: doslen + 3 * (chcols + 2) bytes */
    chcols = TEDW(TED_chcols);
    dosbufflen = DOSLEN + chcols + chcols + chcols + 6;
    if (!(dosbuff = AllocMem(dosbufflen, MEMF_PUBLIC)))
        return 0;
    TEDL(TED_dosbuff) = (ULONG)dosbuff;
    TEDL(TED_charbuff) = (ULONG)dosbuff + DOSLEN;
    TEDL(TED_charbuff2) = TEDL(TED_charbuff) + chcols + 2;
    TEDL(TED_charbuff3) = TEDL(TED_charbuff2) + chcols + 2;
    TEDL(TED_charbuff4) = TEDL(TED_charbuff3) + chcols + 2;

    TEDW(TED_tokenise) = 1;                     /* getstuff */
    *TED(TED_defaultreqga9 + 13) &= ~0x80;      /* no .info */

    for (i = 0; i < sizeof rts_patches / sizeof rts_patches[0]; i++)
        *(UWORD *)TED(rts_patches[i]) = 0x4E75;                 /* rts */
    TEDW(TED_error) = 0x4EF9;                                   /* jmp error_stub */
    *(ULONG *)TED(TED_error + 2) = (ULONG)error_stub;
    CacheClearU();

    memset(&r, 0, sizeof r);
    call(TED_donew, &r);
    return 1;
}

/* the token list as the compiler built it */
static int write_tokens(void)
{
    char name[EDSTUB_PATHLEN];
    UBYTE *node;
    ULONG n = 0;
    LOG_BEGIN;

    edstub_strcpy(name, outdir, sizeof name);
    edstub_strcat(name, "tokens.txt", sizeof name);
    if (!wopen(&wf, name))
        return 0;
    for (node = edstub_ed->tokens; node; node = *(UBYTE **)node) {
        char line[8];
        struct sb b = { line, 0, sizeof line };
        ULONG len = edstub_strlen((char *)node + 6);

        if (len > MAXNAME) {
            sbs(&L, "FATAL: token name longer than ");
            sbn(&L, MAXNAME);
            LOG_END;
            wclose(&wf);
            return 0;
        }
        sbx(&b, *(UWORD *)(node + 4) | 0x8000, 4);
        sbc(&b, ' ');
        wput(&wf, (UBYTE *)line, b.len);
        wput(&wf, node + 6, len);
        wput(&wf, (UBYTE *)"\n", 1);
        n++;
    }
    sbs(&L, "tokens: ");
    sbn(&L, n);
    LOG_END;
    return wclose(&wf);
}

/* the file into memory, 16 zero bytes behind it */
static UBYTE *readfile(const char *name, ULONG *size)
{
    BPTR fh = Open(name, MODE_OLDFILE);
    UBYTE *data;
    LONG n;

    if (!fh)
        return 0;
    Seek(fh, 0, OFFSET_END);
    n = Seek(fh, 0, OFFSET_BEGINNING);
    if (n < 0 || !(data = AllocMem(n + 16, MEMF_PUBLIC | MEMF_CLEAR))) {
        Close(fh);
        return 0;
    }
    if (Read(fh, data, n) != n) {
        FreeMem(data, n + 16);
        Close(fh);
        return 0;
    }
    Close(fh);
    *size = n;
    return data;
}

/* tokit2 on one detok line */
static void tokline(const UBYTE *t, ULONG tl)
{
    struct regs r;

    /* inbuf is 0 behind the line, as in a fresh buffer */
    memcpy(inbuf, t, tl);
    if (inclear > tl)
        memset(inbuf + tl, 0, inclear - tl);
    inclear = tl + 16;
    memset(&r, 0, sizeof r);
    r.a[1] = (ULONG)outbuf;
    r.a[2] = (ULONG)inbuf;
    r.d[1] = tl;
    call(TED_tokit2, &r);
    wput(&wf, outbuf, r.d[5]);
}

/* detok/NNNN.txt and tokall/NNNN.bb2 of a tokenised source */
static const char *detok_tokall(ULONG index, const UBYTE *data, ULONG size)
{
    char name[EDSTUB_PATHLEN];
    const UBYTE *p;
    struct wfile *w = &wf;
    int pass;

    for (p = data; p < data + size; p += edstub_strlen((const char *)p) + 1)
        if (edstub_strlen((const char *)p) > MAXCHUNK)
            return "long line";
    /* pass 0: detok text; pass 1: the same text line by line through tokit2 */
    for (pass = 0; pass < 2; pass++) {
        outpath(name, pass ? "tokall" : "detok", index, pass ? ".bb2" : ".txt");
        if (!wopen(w, name))
            return "cannot write";
        for (p = data; p < data + size; p += edstub_strlen((const char *)p) + 1) {
            struct regs r;
            UBYTE *t, *q, *end;

            memset(&r, 0, sizeof r);
            r.a[0] = (ULONG)p;
            r.a[1] = (ULONG)textbuf;
            call(TED_detok, &r);
            end = (UBYTE *)r.a[1];              /* at the 0 */
            if (!pass) {
                wput(w, textbuf, end - textbuf);
                wput(w, (UBYTE *)"\n", 1);
                continue;
            }
            /* the LF lines of "text + LF" (a byte 10 inside splits) */
            *end = '\n';
            for (t = q = textbuf; q <= end; q++)
                if (*q == '\n') {
                    tokline(t, q - t);
                    t = q + 1;
                }
        }
        if (!wclose(w))
            return "cannot write";
    }
    return 0;
}

/* save/NNNN.bb2: fileopen2 + realsave */
static const char *loadsave(ULONG index, char *src)
{
    char name[EDSTUB_PATHLEN];
    struct regs r;

    memset(&r, 0, sizeof r);
    r.d[7] = (ULONG)src;
    call(TED_fileopen2, &r);
    outpath(name, "save", index, ".bb2");
    memset(&r, 0, sizeof r);
    r.d[7] = (ULONG)name;
    call(TED_realsave, &r);
    return 0;
}

static void job(ULONG index, char *src, BPTR idx)
{
    ULONG size = 0, before = errors, i;
    UBYTE *data = readfile(src, &size);
    const char *result = 0;
    int tok = 0;
    char line[64];
    struct sb b = { line, 0, sizeof line };

    if (!data)
        result = "cannot read";
    else {
        for (i = 0; i < size && !tok; i++)
            tok = !data[i];
        if (tok)
            result = detok_tokall(index, data, size);
        if (!result)
            result = loadsave(index, src);
        FreeMem(data, size + 16);
    }
    sbnum4(&b, index);
    sbc(&b, '\t');
    sbs(&b, tok ? "tok" : "text");
    sbc(&b, '\t');
    sbs(&b, result ? result : "ok");
    sbc(&b, '\t');
    sbn(&b, errors - before);
    sbc(&b, '\n');
    Write(idx, line, b.len);
    if (result) {
        LOG_BEGIN;
        sbs(&L, src);
        sbs(&L, ": ");
        sbs(&L, result);
        LOG_END;
    }
}

void edstub_main(const char *arg)
{
    ULONG size, index = 0;
    UBYTE *jobs = readfile(arg, &size);
    char *p, name[EDSTUB_PATHLEN];
    BPTR idx = 0;
    LOG_BEGIN;

    edstub_error_hook = error_hook;
    if (!jobs) {
        sbs(&L, "FATAL: cannot read job file ");
        sbs(&L, arg);
        LOG_END;
        return;
    }
    inbuf = AllocMem(BUFLEN, MEMF_PUBLIC | MEMF_CLEAR);
    outbuf = AllocMem(BUFLEN, MEMF_PUBLIC | MEMF_CLEAR);
    textbuf = AllocMem(BUFLEN, MEMF_PUBLIC | MEMF_CLEAR);
    if (!inbuf || !outbuf || !textbuf)
        goto done;
    for (p = (char *)jobs; *p; ) {
        char *e = p, *v;

        while (*e && *e != '\n')
            e++;
        if (*e)
            *e++ = 0;
        for (v = p; *v && *v != '='; v++)
            ;
        if (*v == '=') {
            *v++ = 0;
            if (edstub_streq(p, "ted"))
                edstub_strcpy(tedname, v, sizeof tedname);
            else if (edstub_streq(p, "out"))
                edstub_strcpy(outdir, v, sizeof outdir);
            else if (edstub_streq(p, "log"))
                edstub_set_log(v);
            else if (edstub_streq(p, "src")) {
                if (!ted) {
                    if (!ted_setup() || !write_tokens())
                        goto done;
                    edstub_strcpy(name, outdir, sizeof name);
                    edstub_strcat(name, "index.txt", sizeof name);
                    if (!(idx = Open(name, MODE_NEWFILE)))
                        goto done;
                }
                job(index++, v, idx);
            }
        }
        p = e;
    }
    sbs(&L, "done: ");
    sbn(&L, index);
    sbs(&L, " sources, ");
    sbn(&L, errors);
    sbs(&L, " TED errors");
    LOG_END;
done:
    if (idx)
        Close(idx);
    if (inbuf)
        FreeMem(inbuf, BUFLEN);
    if (outbuf)
        FreeMem(outbuf, BUFLEN);
    if (textbuf)
        FreeMem(textbuf, BUFLEN);
    if (dosbuff)
        FreeMem(dosbuff, dosbufflen);
    FreeMem(jobs, size + 16);
}
