/*
 * mathffplib - deflibs library 230: AmigaOS calls of mathffp.library
 * (12 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/mathffp.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("mathffp.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_mathffplib,#la1)
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

BLITZ_OSLIBHEADER(LIB_mathffplib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_mathffplib, -30, "SPFix_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathffplib, -36, "SPFlt_", "(integer)", d0);
BLITZ_OSCALL(LIB_mathffplib, -42, "SPCmp_", "(leftParm,rightParm)", d1, d0);
BLITZ_OSCALL(LIB_mathffplib, -48, "SPTst_", "(parm)", d1);
BLITZ_OSCALL(LIB_mathffplib, -54, "SPAbs_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathffplib, -60, "SPNeg_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathffplib, -66, "SPAdd_", "(leftParm,rightParm)", d1, d0);
BLITZ_OSCALL(LIB_mathffplib, -72, "SPSub_", "(leftParm,rightParm)", d1, d0);
BLITZ_OSCALL(LIB_mathffplib, -78, "SPMul_", "(leftParm,rightParm)", d1, d0);
BLITZ_OSCALL(LIB_mathffplib, -84, "SPDiv_", "(leftParm,rightParm)", d1, d0);
BLITZ_OSCALL(LIB_mathffplib, -90, "SPFloor_", "(parm)", d0);
BLITZ_OSCALL(LIB_mathffplib, -96, "SPCeil_", "(parm)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_mathffplib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "mathffp.library";

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
