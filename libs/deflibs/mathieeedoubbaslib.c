/*
 * mathieeedoubbaslib - deflibs library 229: AmigaOS calls of mathieeedoubbas.library
 * (12 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathieeedoubbas.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathieeedoubbas.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_mathieeedoubbaslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathieeedoubbaslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathieeedoubbaslib, -30, "IEEEDPFix_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -36, "IEEEDPFlt_", "(integer)", d0);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -42, "IEEEDPCmp_", "(leftParm,rightParm)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -48, "IEEEDPTst_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -54, "IEEEDPAbs_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -60, "IEEEDPNeg_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -66, "IEEEDPAdd_", "(leftParm,rightParm)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -72, "IEEEDPSub_", "(leftParm,rightParm)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -78, "IEEEDPMul_", "(factor1,factor2)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -84, "IEEEDPDiv_", "(dividend,divisor)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -90, "IEEEDPFloor_", "(parm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeedoubbaslib, -96, "IEEEDPCeil_", "(parm)", d0, d1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathieeedoubbaslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathieeedoubbas.library";

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
