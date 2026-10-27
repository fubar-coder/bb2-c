/*
 * rexxsyslib - deflibs library 222: AmigaOS calls of rexxsyslib.library
 * (10 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/rexxsyslib.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("rexxsyslib.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_rexxsyslib,#la1)
 *
 * Every call: BLITZ_OSCALL(base lib, LVO, name, help, argument
 * registers); the compiler generates the call (blitz2.s stamigalib).
 */
#include "blitzlib.h"
#include "deflib.h"

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
static ULONG libinit(void);
static void libfinit(struct Library *base BL_REG(a1));

BLITZ_OSLIBHEADER(LIB_rexxsyslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_rexxsyslib, -126, "CreateArgstring_", "(string,length)", a0, d0);
BLITZ_OSCALL(LIB_rexxsyslib, -132, "DeleteArgstring_", "(argstring)", a0);
BLITZ_OSCALL(LIB_rexxsyslib, -138, "LengthArgstring_", "(argstring)", a0);
BLITZ_OSCALL(LIB_rexxsyslib, -144, "CreateRexxMsg_", "(port,extension,host)", a0, a1, d0);
BLITZ_OSCALL(LIB_rexxsyslib, -150, "DeleteRexxMsg_", "(packet)", a0);
BLITZ_OSCALL(LIB_rexxsyslib, -156, "ClearRexxMsg_", "(msgptr,count)", a0, d0);
BLITZ_OSCALL(LIB_rexxsyslib, -162, "FillRexxMsg_", "(msgptr,count,mask)", a0, d0, d1);
BLITZ_OSCALL(LIB_rexxsyslib, -168, "IsRexxMsg_", "(msgptr)", a0);
BLITZ_OSCALL(LIB_rexxsyslib, -450, "LockRexxBase_", "(resource)", d0);
BLITZ_OSCALL(LIB_rexxsyslib, -456, "UnlockRexxBase_", "(resource)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_rexxsyslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "rexxsyslib.library";

/* init nullsub: d0 = library base */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openlibrary(libname);
}

/* finit nullsub: a1 = library base */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    CloseLibrary(base);                 /* MOVE.l 4,a6:JMP CloseLibrary(a6) */
}
