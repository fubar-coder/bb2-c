/*
 * potgolib - deflibs library 223: AmigaOS calls of potgo.resource
 * (3 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/potgo.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("potgo.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          the original opens "resource" (LEA points 6 bytes
 *          into the name), which never exists, and retries
 *          forever (deflib.h): the init hangs
 *   finit: nothing (resources are not closed; !libs
 *          LIB_potgolib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_potgolib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_potgolib, -6, "AllocPotBits_", "(bits)", d0);
BLITZ_OSCALL(LIB_potgolib, -12, "FreePotBits_", "(bits)", d0);
BLITZ_OSCALL(LIB_potgolib, -18, "WritePotgo_", "(word,mask)", d0, d1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_potgolib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "potgo.resource";

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
