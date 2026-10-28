/*
 * stubs.c - host implementations of src/core/hooks.h and the test
 * environment (hostenv.h).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hostenv.h"
#include "fileio.h"
#include "tokit.h"

int host_calls[HC_COUNT];
int host_trace;
jmp_buf *host_outmem;
int host_hazards;
const char *host_lasterror;
BOOL host_icongad, host_findcase;
UBYTE *host_bname;

static const char *const callnames[HC_COUNT] = {
    "error", "getbname", "pointeron", "sleepon", "showfname", "beepem",
    "showall", "showtopitem", "showbotitem", "showthisitem",
    "blokprnt2", "blokmenus", "scrollup", "scrupall", "scrolldown",
    "scrdownall", "cls", "scrollrite", "scrollleft", "refresh",
    "showhprop", "showvprop", "setdefs", "doclean", "loadxtra",
    "savextra", "outmem"
};

static void called(enum hostcall c)
{
    host_calls[c]++;
    if (host_trace)
        fprintf(stderr, "hook %s\n", callnames[c]);
}

void host_resetcalls(void)
{
    memset(host_calls, 0, sizeof(host_calls));
    host_hazards = 0;
    host_lasterror = NULL;
}

void ted_hazard(const char *what, const char *file, int line)
{
    host_hazards++;
    if (host_trace)
        fprintf(stderr, "ORIG_HAZARD %s:%d: %s\n", file, line, what);
}

/* ------------------------------------------------------------ memory */

/* header in front of every block: the size AllocMem was asked for.
   SLACK: struct item has 8 byte pointers on the host (header 17 bytes,
   not lenitem = 9) */
#define HDR   16
#define SLACK 16

APTR ted_alloc(ULONG size, ULONG flags)
{
    UBYTE *p = calloc(1, HDR + size + SLACK);

    (void)flags;
    if (!p)
        return NULL;
    memcpy(p, &size, sizeof(size));
    return p + HDR;
}

void ted_free(APTR p, ULONG size)
{
    ULONG want;

    if (!p) {
        ORIG_HAZARD("FreeMem(NULL)");
        return;
    }
    memcpy(&want, (UBYTE *)p - HDR, sizeof(want));
    if (want != size)
        ORIG_HAZARD("FreeMem with a size different from AllocMem");
    free((UBYTE *)p - HDR);
}

NORETURN void outmem(void)
{
    called(HC_OUTMEM);
    if (!host_outmem) {
        fprintf(stderr, "outmem without target\n");
        abort();
    }
    longjmp(*host_outmem, 1);
}

/* -------------------------------------------------------------- files */

#define MAXFH 16
static FILE *fhs[MAXFH];

tedfh ted_open(const char *name, LONG mode)
{
    int i;

    for (i = 1; i < MAXFH; i++)
        if (!fhs[i]) {
            fhs[i] = fopen(name, mode == TED_MODE_NEWFILE ? "wb" : "rb");
            return fhs[i] ? i : 0;
        }
    return 0;
}

LONG ted_read(tedfh fh, APTR buf, LONG len)
{
    size_t n;

    if (fh <= 0 || fh >= MAXFH || !fhs[fh])
        return -1;
    n = fread(buf, 1, len, fhs[fh]);
    return ferror(fhs[fh]) ? -1 : (LONG)n;
}

LONG ted_write(tedfh fh, const void *buf, LONG len)
{
    if (fh <= 0 || fh >= MAXFH || !fhs[fh])
        return -1;
    return fwrite(buf, 1, len, fhs[fh]) == (size_t)len ? len : -1;
}

void ted_close(tedfh fh)
{
    if (fh > 0 && fh < MAXFH && fhs[fh]) {
        fclose(fhs[fh]);
        fhs[fh] = NULL;
    }
}

LONG ted_rename(const char *o, const char *n) { return rename(o, n) == 0 ? -1 : 0; }
LONG ted_delete(const char *name) { return remove(name) == 0 ? -1 : 0; }
tedfh ted_lock(const char *name, LONG mode) { (void)mode; return access(name, F_OK) == 0 ? 1000 : 0; }
void ted_unlock(tedfh lock) { (void)lock; }

/* ---------------------------------------------------------- UI hooks */

void error(const char *msg) { called(HC_ERROR); host_lasterror = msg; }
UBYTE *getbname(const char *title) { (void)title; called(HC_GETBNAME); return host_bname; }
void pointeron(void) { called(HC_POINTERON); }
void sleepon(void) { called(HC_SLEEPON); }
void showfname(void) { called(HC_SHOWFNAME); }
void beepem(void) { called(HC_BEEPEM); }

/* the screen walks of chunk 04 without drawing (core: chmap_show*, ted.s:4050ff) */
void showall(void)     { called(HC_SHOWALL);     chmap_showall(NULL); }
void showtopitem(void) { called(HC_SHOWTOPITEM); chmap_showtopitem(NULL); }
void showbotitem(void) { called(HC_SHOWBOTITEM); chmap_showbotitem(NULL); }
void showthisitem(void) { called(HC_SHOWTHISITEM); chmap_showa2item(thisitem, NULL); }
void blokprnt2(void) { called(HC_BLOKPRNT2); }
void blokmenus(BOOL enable) { (void)enable; called(HC_BLOKMENUS); }
void scrollup(UWORD row) { called(HC_SCROLLUP); chmap_scrollup(row); }
void scrupall(void) { called(HC_SCRUPALL); chmap_scrollup(0); }
void scrolldown(UWORD row) { called(HC_SCROLLDOWN); chmap_scrolldown(row); }
void scrdownall(void) { called(HC_SCRDOWNALL); chmap_scrolldown(0); }
void cls(void) { called(HC_CLS); chmap_cls(); }
void scrollrite(void) { called(HC_SCROLLRITE); }
void scrollleft(void) { called(HC_SCROLLLEFT); }
void refresh(void) { called(HC_REFRESH); }
void showhprop(void) { called(HC_SHOWHPROP); }
void showvprop(void) { called(HC_SHOWVPROP); }

void setdefs(struct defblock *a5)
{
    called(HC_SETDEFS);
    if (!tokens)
        a5->d_tokenise = 0;
    defstart = *a5;
}

BOOL icongad(void) { return host_icongad; }
BOOL findcase(void) { return host_findcase; }
void doclean(void) { called(HC_DOCLEAN); }
void call_loadxtra(tedfh fh) { (void)fh; called(HC_LOADXTRA); }
void call_savextra(tedfh fh) { (void)fh; called(HC_SAVEXTRA); }

/* ------------------------------------------------------- environment */

void hostenv_init(const char *tokfile, UWORD rows)
{
    static UBYTE *block, *map;
    ULONG size = doslen + 3 * (chcols + 2);

    free(block);
    free(map);
    block = calloc(1, size);
    dosbuff = block;                    /* ted.s:41-63 */
    charbuff = dosbuff + doslen;
    charbuff2 = charbuff + chcols + 2;
    charbuff3 = charbuff2 + chcols + 2;
    charbuff4 = charbuff3 + chcols + 2;
    edrows = rows;
    edcols = 80;
    map = malloc((size_t)rows * chcols + chcols);  /* + 1 row: tokit reads past the end */
    memset(map, ' ', (size_t)rows * chcols + chcols);
    chmap = map;
    colon = ':';
    quote = '"';
    label = '.';
    remark = ';';
    if (tokfile)
        tokens = host_loadtokens(tokfile);
    tokenise = tokens ? 1 : 0;
    firstitem = enditem = thisitem = NULL;
    numlines = 0;
    cursx = cursy = curstop = 0;
    cursoff = 0;
    bboty = (UWORD)-1;
    host_resetcalls();
}

void hostenv_droplist(void)
{
    struct item *it = firstitem, *n;
    UWORD i;

    for (i = 0; it && i < numlines; i++) {
        n = it->nextitem;
        free((UBYTE *)it - HDR);
        it = n;
    }
    firstitem = enditem = thisitem = NULL;
    numlines = 0;
}

UBYTE *host_readfile(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    UBYTE *b;

    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = malloc(*len + 1);
    if (fread(b, 1, *len, f) != (size_t)*len) {
        free(b);
        b = NULL;
    } else
        b[*len] = 0;
    fclose(f);
    return b;
}
