/*
 * battclocklib - deflibs library 246: AmigaOS calls of battclock.resource
 * (3 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/battclock.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("battclock.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          the original opens "ock.resource" (LEA points 6 bytes
 *          into the name), which never exists, and retries
 *          forever (deflib.h): the init hangs
 *   finit: nothing (resources are not closed; !libs
 *          LIB_battclocklib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_battclocklib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_battclocklib, -6, "ResetBattClock_", "");
BLITZ_OSCALL(LIB_battclocklib, -12, "ReadBattClock_", "");
BLITZ_OSCALL(LIB_battclocklib, -18, "WriteBattClock_", "(time)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_battclocklib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "battclock.resource";

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
