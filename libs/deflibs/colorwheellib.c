/*
 * colorwheellib - deflibs library 219: AmigaOS calls of gadgets/colorwheel.gadget
 * (2 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/colorwheel.gadget1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("gadgets/colorwheel.gadget", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig) (#44):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_colorwheellib,#la1)
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

BLITZ_OSLIBHEADER(LIB_colorwheellib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_colorwheellib, -30, "ConvertHSBToRGB_", "(hsb,rgb)", a0, a1);
BLITZ_OSCALL(LIB_colorwheellib, -36, "ConvertRGBToHSB_", "(rgb,hsb)", a0, a1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_colorwheellib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "gadgets/colorwheel.gadget";

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
