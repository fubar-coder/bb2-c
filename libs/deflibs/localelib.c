/*
 * localelib - deflibs library 231: AmigaOS calls of locale.library
 * (24 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/locale.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("locale.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_localelib,#la1)
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

BLITZ_OSLIBHEADER(LIB_localelib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_localelib, -36, "CloseCatalog_", "(catalog)", a0);
BLITZ_OSCALL(LIB_localelib, -42, "CloseLocale_", "(locale)", a0);
BLITZ_OSCALL(LIB_localelib, -48, "ConvToLower_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -54, "ConvToUpper_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -60, "FormatDate_", "(locale,template,date,putCharFunc)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_localelib, -66, "FormatString_", "(locale,string,dataStream,putCharFunc)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_localelib, -72, "GetCatalogStr_", "(catalog,stringNum,defaultString)", a0, d0, a1);
BLITZ_OSCALL(LIB_localelib, -78, "GetLocaleStr_", "(locale,stringNum)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -84, "IsAlNum_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -90, "IsAlpha_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -96, "IsCntrl_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -102, "IsDigit_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -108, "IsGraph_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -114, "IsLower_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -120, "IsPrint_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -126, "IsPunct_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -132, "IsSpace_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -138, "IsUpper_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -144, "IsXDigit_", "(locale,character)", a0, d0);
BLITZ_OSCALL(LIB_localelib, -150, "OpenCatalogA_", "(locale,name,tags)", a0, a1, a2);
BLITZ_OSCALL(LIB_localelib, -156, "OpenLocale_", "(name)", a0);
BLITZ_OSCALL(LIB_localelib, -162, "ParseDate_", "(locale,date,template,getCharFunc)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_localelib, -174, "StrConvert_", "(locale,string,buffer,bufferSize,type)", a0, a1, a2, d0, d1);
BLITZ_OSCALL(LIB_localelib, -180, "StrnCmp_", "(locale,string1,string2,length,type)", a0, a1, a2, d0, d1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_localelib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "locale.library";

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
