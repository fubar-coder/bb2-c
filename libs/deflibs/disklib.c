/*
 * disklib - deflibs library 238: AmigaOS calls of disk.resource
 * (6 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/disk.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("disk.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  Fixed (intended
 *          deviation, #45): the original opens "esource" (LEA
 *          points 6 bytes into the name), which never exists; here
 *          the name.  BUG(orig) (#44): retries forever if the resource
 *          cannot be opened (deflib.h)
 *   finit: nothing (resources are not closed; !libs
 *          LIB_disklib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_disklib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_disklib, -6, "AllocUnit_", "(unitNum)", d0);
BLITZ_OSCALL(LIB_disklib, -12, "FreeUnit_", "(unitNum)", d0);
BLITZ_OSCALL(LIB_disklib, -18, "GetUnit_", "(unitPointer)", a1);
BLITZ_OSCALL(LIB_disklib, -24, "GiveUnit_", "");
BLITZ_OSCALL(LIB_disklib, -30, "GetUnitID_", "(unitNum)", d0);
BLITZ_OSCALL(LIB_disklib, -36, "ReadUnitID_", "(unitNum)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_disklib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "disk.resource";

/* init nullsub: d0 = resource base (fixed, #45: the original has
   LEA libname+6(pc),a1) */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openresource(libname);
}

/* finit nullsub: a1 = resource base, nothing to do (RTS) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    (void)base;
}
