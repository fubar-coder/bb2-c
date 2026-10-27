/*
 * ciablib - deflibs library 241: AmigaOS calls of ciab.resource
 * (4 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/ciab.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("ciab.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          the original opens "esource" (LEA points 6 bytes
 *          into the name), which never exists, and retries
 *          forever (deflib.h): the init hangs
 *   finit: nothing (resources are not closed; !libs
 *          LIB_ciablib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_ciablib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_ciablib, -6, "AddICRVector_b", "(resource,iCRBit,interrupt)", a6, d0, a1);
BLITZ_OSCALL(LIB_ciablib, -12, "RemICRVector_b", "(resource,iCRBit,interrupt)", a6, d0, a1);
BLITZ_OSCALL(LIB_ciablib, -18, "AbleICR_b", "(resource,mask)", a6, d0);
BLITZ_OSCALL(LIB_ciablib, -24, "SetICR_b", "(resource,mask)", a6, d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_ciablib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "ciab.resource";

/* init nullsub: d0 = resource base.  BUG(orig): LEA libname+6(pc),a1 */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openresource(libname + DEFLIB_RESOURCE_SKEW);
}

/* finit nullsub: a1 = resource base, nothing to do (RTS) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    (void)base;
}
