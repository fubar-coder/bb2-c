/*
 * mathieeedoubtranslib - deflibs library 228: AmigaOS calls of mathieeedoubtrans.library
 * (17 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathieeedoubtrans.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathieeedoubtrans.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_mathieeedoubtranslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathieeedoubtranslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathieeedoubtranslib, -30, "IEEEDPAtan_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -36, "IEEEDPSin_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -42, "IEEEDPCos_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -48, "IEEEDPTan_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -54, "IEEEDPSincos_", "(pf2,parm)", a0, d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -60, "IEEEDPSinh_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -66, "IEEEDPCosh_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -72, "IEEEDPTanh_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -78, "IEEEDPExp_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -84, "IEEEDPLog_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -90, "IEEEDPPow_", "(exp,arg)", d2, d3, d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -96, "IEEEDPSqrt_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -102, "IEEEDPTieee_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -108, "IEEEDPFieee_", "(single)", d0);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -114, "IEEEDPAsin_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -120, "IEEEDPAcos_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubtranslib, -126, "IEEEDPLog10_", "(parm)", d0, d1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathieeedoubtranslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathieeedoubtrans.library";

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
