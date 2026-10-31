/*
 * diskfontlib - deflibs library 251: AmigaOS calls of diskfont.library
 * (5 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/diskfont.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("diskfont.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_diskfontlib,#la1)
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

BLITZ_OSLIBHEADER(LIB_diskfontlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_diskfontlib, -30, "OpenDiskFont_", "(textAttr)", a0);
BLITZ_OSCALL(LIB_diskfontlib, -36, "AvailFonts_", "(buffer,bufBytes,flags)", a0, d0, d1);
BLITZ_OSCALL(LIB_diskfontlib, -42, "NewFontContents_", "(fontsLock,fontName)", a0, a1);
BLITZ_OSCALL(LIB_diskfontlib, -48, "DisposeFontContents_", "(fontContentsHeader)", a1);
BLITZ_OSCALL(LIB_diskfontlib, -54, "NewScaledDiskFont_", "(sourceFont,destTextAttr)", a0, a1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_diskfontlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "diskfont.library";

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
