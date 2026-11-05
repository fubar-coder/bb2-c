/*
 * cov.c - function coverage of an m68k program (cov.h, the file layout in
 * covfmt.h).  Only exec and dos calls that Kickstart 1.3 has.
 */
#include "cov.h"
#include "covfmt.h"

#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stabs.h>

#ifndef COV_PROGRAM
#error "COV_PROGRAM (the name of the coverage file) is not defined"
#endif

#define NOINST __attribute__((no_instrument_function))

/* the coverage directory; the file is COV_DIR/COV_PROGRAM.cov */
#define COV_DIR "TEST:cov"

/* the code hunk: the linker's symbols around .text (__stext at its start,
   offset 0 of the hunk, before the startup code; __etext at its end), so
   a bit's offset is the function's address in the link map; declared as
   functions so that their addresses are taken as code addresses */
extern void _stext(void), _etext(void);

static UBYTE *cov_map;                  /* the bitmap, NULL before cov_start */
static ULONG cov_size;                  /* bytes of the code hunk */
static ULONG cov_len;                   /* bytes of the bitmap */

NOINST void __cyg_profile_func_enter(void *fn, void *site)
{
    ULONG n = (ULONG)fn - (ULONG)_stext;

    (void)site;
    if (cov_map && n < cov_size) {
        n >>= 1;
        cov_map[n >> 3] |= (UBYTE)(1 << (n & 7));
    }
}

NOINST void __cyg_profile_func_exit(void *fn, void *site)
{
    (void)fn;
    (void)site;
}

NOINST void cov_start(void)
{
    if (cov_map)
        return;
    cov_size = (ULONG)_etext - (ULONG)_stext;
    cov_len = COVFMT_BITMAP_LEN(cov_size);
    cov_map = AllocMem(cov_len, MEMF_PUBLIC | MEMF_CLEAR);
}

NOINST static void put32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

/* the header of this program's coverage file */
NOINST static void header(UBYTE *h)
{
    put32(h, COVFMT_MAGIC);
    put32(h + 4, COVFMT_VERSION);
    put32(h + 8, cov_size);
    put32(h + 12, cov_len);
}

/* OR the bitmap of an existing file into cov_map if its header is ours */
NOINST static void merge(CONST_STRPTR name, const UBYTE *ours)
{
    UBYTE buf[256];
    ULONG at = 0;
    LONG got;
    BPTR fh;
    int i;

    fh = Open(name, MODE_OLDFILE);
    if (!fh)
        return;
    if (Read(fh, buf, COVFMT_HEADER_SIZE) == COVFMT_HEADER_SIZE) {
        for (i = 0; i < COVFMT_HEADER_SIZE && buf[i] == ours[i]; i++)
            ;
        if (i == COVFMT_HEADER_SIZE) {
            while (at < cov_len) {
                got = Read(fh, buf, cov_len - at < sizeof(buf) ? cov_len - at : sizeof(buf));
                if (got <= 0)
                    break;
                for (i = 0; i < got; i++)
                    cov_map[at + i] |= buf[i];
                at += got;
            }
        }
    }
    Close(fh);
}

NOINST void cov_write(void)
{
    static const char name[] = COV_DIR "/" COV_PROGRAM ".cov";
    struct Process *pr = (struct Process *)FindTask(NULL);
    UBYTE h[COVFMT_HEADER_SIZE];
    UBYTE *map;
    APTR win = NULL;
    BPTR dir, fh;

    if (!cov_map)
        return;
    /* no requester for a missing TEST: */
    if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS) {
        win = pr->pr_WindowPtr;
        pr->pr_WindowPtr = (APTR)-1;
    }
    dir = Lock((CONST_STRPTR)COV_DIR, ACCESS_READ);
    if (dir) {
        header(h);
        merge((CONST_STRPTR)name, h);
        fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
        if (fh) {
            Write(fh, h, COVFMT_HEADER_SIZE);
            Write(fh, cov_map, cov_len);
            Close(fh);
        }
        UnLock(dir);
    }
    if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS)
        pr->pr_WindowPtr = win;
    map = cov_map;
    cov_map = NULL;
    FreeMem(map, cov_len);
}

/* libnix runs its constructor list before main and its destructor list
   when main returns and on exit(), the destructors before it closes the
   libraries (__exitlibraries, priority -79).  atexit would work as well,
   but needs malloc. */
ADD2INIT(cov_start, 0);
ADD2EXIT(cov_write, 0);
