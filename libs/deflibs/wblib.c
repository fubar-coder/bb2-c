/*
 * wblib - deflibs library 220: AmigaOS calls of workbench.library
 * (7 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/workbench.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("workbench.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_wblib,#la1)
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

BLITZ_OSLIBHEADER(LIB_wblib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_wblib, -48, "AddAppWindowA_", "(id,userdata,window,msgport,taglist)", d0, d1, a0, a1, a2);
BLITZ_OSCALL(LIB_wblib, -54, "RemoveAppWindow_", "(appWindow)", a0);
BLITZ_OSCALL(LIB_wblib, -60, "AddAppIconA_", "(id,userdata,text,msgport,lock,diskobj,taglist)", d0, d1, a0, a1, a2, a3, a4);
BLITZ_OSCALL(LIB_wblib, -66, "RemoveAppIcon_", "(appIcon)", a0);
BLITZ_OSCALL(LIB_wblib, -72, "AddAppMenuItemA_", "(id,userdata,text,msgport,taglist)", d0, d1, a0, a1, a2);
BLITZ_OSCALL(LIB_wblib, -78, "RemoveAppMenuItem_", "(appMenuItem)", a0);
BLITZ_OSCALL(LIB_wblib, -90, "WBInfo_", "(lock,name,screen)", a0, a1, a2);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_wblib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "workbench.library";

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
