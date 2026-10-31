/*
 * mathieeesingtranslib - deflibs library 226: AmigaOS calls of mathieeesingtrans.library
 * (17 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathieeesingtrans.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathieeesingtrans.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_mathieeesingtranslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathieeesingtranslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathieeesingtranslib, -30, "IEEESPAtan_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -36, "IEEESPSin_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -42, "IEEESPCos_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -48, "IEEESPTan_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -54, "IEEESPSincos_", "(cosptr,parm)", a0, d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -60, "IEEESPSinh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -66, "IEEESPCosh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -72, "IEEESPTanh_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -78, "IEEESPExp_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -84, "IEEESPLog_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -90, "IEEESPPow_", "(exp,arg)", d1, d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -96, "IEEESPSqrt_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -102, "IEEESPTieee_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -108, "IEEESPFieee_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -114, "IEEESPAsin_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -120, "IEEESPAcos_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingtranslib, -126, "IEEESPLog10_", "(parm)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathieeesingtranslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathieeesingtrans.library";

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
