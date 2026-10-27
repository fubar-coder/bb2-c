/*
 * mathieeesingbaslib - deflibs library 227: AmigaOS calls of mathieeesingbas.library
 * (12 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathieeesingbas.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathieeesingbas.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_mathieeesingbaslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathieeesingbaslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathieeesingbaslib, -30, "IEEESPFix_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -36, "IEEESPFlt_", "(integer)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -42, "IEEESPCmp_", "(leftParm,rightParm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -48, "IEEESPTst_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -54, "IEEESPAbs_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -60, "IEEESPNeg_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -66, "IEEESPAdd_", "(leftParm,rightParm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -72, "IEEESPSub_", "(leftParm,rightParm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -78, "IEEESPMul_", "(leftParm,rightParm)", d0, d1);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -84, "IEEESPDiv_", "(dividend,divisor)", d0, d1);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -90, "IEEESPFloor_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathieeesingbaslib, -96, "IEEESPCeil_", "(parm)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathieeesingbaslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathieeesingbas.library";

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
