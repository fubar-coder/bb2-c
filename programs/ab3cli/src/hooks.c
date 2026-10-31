/*
 * hooks.c - TED's core (programs/ted/src/core, hooks.h) without a screen:
 * memory and files are exec and DOS, the drawing and pointer routines do
 * nothing, error() keeps the message for ab3cli (main.c), outmem() ends the
 * job.  The Amiga counterpart of the host stubs
 * (programs/ted/tests/host/stubs.c).
 */
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "edstub.h"
#include "ab3cli.h"
#include "fileio.h"
#include "hooks.h"
#include "state.h"
#include "tokit.h"

extern struct DosLibrary *DOSBase;

/* ------------------------------------------------------------ memory */

APTR ted_alloc(ULONG size, ULONG flags)
{
    return AllocMem(size, flags);
}

void ted_free(APTR p, ULONG size)
{
    FreeMem(p, size);
}

/* TED: an alert, then back to its editor loop; here the job ends */
NORETURN void outmem(void)
{
    if (filehand) {
        ted_close(filehand);
        filehand = 0;
    }
    ab3cli_fatal("out of memory");
}

/* -------------------------------------------------------------- files */

tedfh ted_open(const char *name, LONG mode)
{
    return (tedfh)Open((CONST_STRPTR)name, mode);
}

LONG ted_read(tedfh fh, APTR buf, LONG len)
{
    return Read((BPTR)fh, buf, len);
}

LONG ted_write(tedfh fh, const void *buf, LONG len)
{
    return Write((BPTR)fh, (APTR)buf, len);
}

void ted_close(tedfh fh)
{
    Close((BPTR)fh);
}

LONG ted_rename(const char *oldname, const char *newname)
{
    return Rename((CONST_STRPTR)oldname, (CONST_STRPTR)newname);
}

LONG ted_delete(const char *name)
{
    return DeleteFile((CONST_STRPTR)name);
}

tedfh ted_lock(const char *name, LONG mode)
{
    return (tedfh)Lock((CONST_STRPTR)name, mode);
}

void ted_unlock(tedfh lock)
{
    UnLock((BPTR)lock);
}

/* -------------------------------------------- messages / requesters */

/* TED's error requester: the message of loading a file */
void error(const char *msg)
{
    ab3cli_loaderror(msg);
}

UBYTE *getbname(const char *title)
{
    (void)title;
    return NULL;
}

/* ------------------------------------- pointer, title, drawing: nothing */

void pointeron(void) {}
void sleepon(void) {}
void showfname(void) {}
void beepem(void) {}
void showall(void) {}
void showtopitem(void) {}
void showbotitem(void) {}
void showthisitem(void) {}
void blokprnt2(void) {}
void blokmenus(BOOL enable) { (void)enable; }
void scrollup(UWORD row) { (void)row; }
void scrupall(void) {}
void scrolldown(UWORD row) { (void)row; }
void scrdownall(void) {}
void cls(void) {}
void scrollrite(void) {}
void scrollleft(void) {}
void refresh(void) {}
void showhprop(void) {}
void showvprop(void) {}

/* the option block as TED's setdefs takes it (without the gadgets) */
void setdefs(struct defblock *defs)
{
    if (!tokens)
        defs->d_tokenise = 0;
    defstart = *defs;
}

BOOL icongad(void) { return FALSE; }
BOOL findcase(void) { return FALSE; }

/* --------------------------------------------------- blitz2 callbacks */

static void call(APTR fn, tedfh fh)
{
    struct regs r;

    memset(&r, 0, sizeof r);
    r.d[7] = (ULONG)fh;
    r.a[6] = (ULONG)DOSBase;
    edstub_callregs(fn, &r);
}

/* ted.s:2771 doclean: the compiler's cleanup (all registers saved) */
void doclean(void)
{
    if (cleanup)
        call(cleanup, 0);
}

void call_loadxtra(tedfh fh) { call(loadxtra, fh); }
void call_savextra(tedfh fh) { call(savextra, fh); }

/* ------------------------------------------------------------- libc */

/* the core is compiled as TED is (with builtins): gcc may call strlen for
   a loop (tokone); no C library in this link - the frame's */
size_t strlen(const char *s)
{
    return edstub_strlen(s);
}

/* ------------------------------------------------------------- DIVU.W */

/* ULONG divu_w(ULONG d0, UWORD src) (core/fileio.h): the 68000's
   `divu.w src,d0`, as in TED (programs/ted/src/glue.c) - no libgcc */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _divu_w\n"
"_divu_w:\n"
"   move.l  4(sp),d0\n"
"   divu.w  10(sp),d0\n"            /* UWORD argument: low word of its long slot */
"   rts\n"
);
