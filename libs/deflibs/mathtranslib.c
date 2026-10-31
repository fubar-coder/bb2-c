/*
 * mathtranslib - deflibs library 225: AmigaOS calls of mathtrans.library
 * (17 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathtrans.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathtrans.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_mathtransoslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathtransoslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathtransoslib, -30, "SPAtan_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -36, "SPSin_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -42, "SPCos_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -48, "SPTan_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -54, "SPSincos_", "(cosResult,parm)", d1, d0);
BLITZ_OSCALL(LIB_mathtransoslib, -60, "SPSinh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -66, "SPCosh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -72, "SPTanh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -78, "SPExp_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -84, "SPLog_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -90, "SPPow_", "(power,arg)", d1, d0);
BLITZ_OSCALL(LIB_mathtransoslib, -96, "SPSqrt_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -102, "SPTieee_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -108, "SPFieee_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -114, "SPAsin_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -120, "SPAcos_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathtransoslib, -126, "SPLog10_", "(parm)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathtransoslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathtrans.library";

/* init nullsub: d0 = library base, 0 if it cannot be opened (fixed,
   #44: the original retries forever) */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openlibrary(libname);
}

/* finit nullsub: a1 = library base (0 if it was not opened) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    deflib_closelibrary(base);
}
