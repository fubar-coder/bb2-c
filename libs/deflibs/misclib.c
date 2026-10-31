/*
 * misclib - deflibs library 224: AmigaOS calls of misc.resource
 * (2 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/misc.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("misc.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  Fixed (intended
 *          deviation, #45): the original opens "esource" (LEA
 *          points 6 bytes into the name), which never exists; here
 *          the name.  Fixed (intended deviation, #44): 0 if the
 *          resource cannot be opened; the original retries forever
 *          (deflib.h)
 *   finit: nothing (resources are not closed; !libs
 *          LIB_misclib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_misclib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_misclib, -6, "AllocMiscResource_", "(unitNum,name)", d0, a1);
BLITZ_OSCALL(LIB_misclib, -12, "FreeMiscResource_", "(unitNum)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_misclib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "misc.resource";

/* init nullsub: d0 = resource base, 0 if it cannot be opened (fixed,
   #44 and #45: the original has LEA libname+6(pc),a1 and retries
   forever) */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openresource(libname);
}

/* finit nullsub: a1 = resource base, nothing to do (RTS) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    (void)base;
}
