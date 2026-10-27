/*
 * xprzmodemlib - deflibs library 2: AmigaOS calls of xprzmodem.library
 * (6 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/xprzmodem.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("xprzmodem.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_xprzmodemlib,#la1)
 *
 * Every call: BLITZ_OSCALL(base lib, LVO, name, help, argument
 * registers); the compiler generates the call (blitz2.s stamigalib).
 */
#include "blitzlib.h"
#include "deflib.h"

/* not in libnums.bb (#xprzlib=218 is reserved, unused) */
#define LIB_xprzmodemlib 2

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
static ULONG libinit(void);
static void libfinit(struct Library *base BL_REG(a1));

BLITZ_OSLIBHEADER(LIB_xprzmodemlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_xprzmodemlib, -30, "XProtocolCleanup_", "(io)", a0);
BLITZ_OSCALL(LIB_xprzmodemlib, -36, "XProtocolSetup_", "(io)", a0);
BLITZ_OSCALL(LIB_xprzmodemlib, -42, "XProtocolSend_", "(io)", a0);
BLITZ_OSCALL(LIB_xprzmodemlib, -48, "XProtocolReceive_", "(io)", a0);
BLITZ_OSCALL(LIB_xprzmodemlib, -54, "XProtocolHostMon_", "(io,serbuff,actual,maxsize)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_xprzmodemlib, -60, "XProtocolUserMon_", "(io,serbuff,actual,maxsize)", a0, a1, d0, d1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_xprzmodemlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "xprzmodem.library";

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
