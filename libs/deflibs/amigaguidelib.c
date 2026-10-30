/*
 * amigaguidelib - deflibs library 248: AmigaOS calls of amigaguide.library
 * (17 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/amigaguide.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("amigaguide.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig) (#44):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_amigaguidelib,#la1)
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

BLITZ_OSLIBHEADER(LIB_amigaguidelib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_amigaguidelib, -30, "AGARexxHost_", "(rmsg,result)", a0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -36, "LockAmigaGuideBase_", "(handle)", a0);
BLITZ_OSCALL(LIB_amigaguidelib, -42, "UnlockAmigaGuideBase_", "(key)", d0);
BLITZ_OSCALL(LIB_amigaguidelib, -54, "OpenAmigaGuideA_", "(nag,*)", a0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -60, "OpenAmigaGuideAsyncA_", "(nag,attrs)", a0, d0);
BLITZ_OSCALL(LIB_amigaguidelib, -66, "CloseAmigaGuide_", "(cl)", a0);
BLITZ_OSCALL(LIB_amigaguidelib, -72, "AmigaGuideSignal_", "(cl)", a0);
BLITZ_OSCALL(LIB_amigaguidelib, -78, "GetAmigaGuideMsg_", "(cl)", a0);
BLITZ_OSCALL(LIB_amigaguidelib, -84, "ReplyAmigaGuideMsg_", "(amsg)", a0);
BLITZ_OSCALL(LIB_amigaguidelib, -90, "SetAmigaGuideContextA_", "(cl,id,attrs)", a0, d0, d1);
BLITZ_OSCALL(LIB_amigaguidelib, -96, "SendAmigaGuideContextA_", "(cl,attrs)", a0, d0);
BLITZ_OSCALL(LIB_amigaguidelib, -102, "SendAmigaGuideCmdA_", "(cl,cmd,attrs)", a0, d0, d1);
BLITZ_OSCALL(LIB_amigaguidelib, -108, "SetAmigaGuideAttrsA_", "(cl,attrs)", a0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -114, "GetAmigaGuideAttr_", "(tag,cl,storage)", d0, a0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -138, "AddAmigaGuideHostA_", "(h,name,attrs)", a0, d0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -144, "RemoveAmigaGuideHostA_", "(hh,attrs)", a0, a1);
BLITZ_OSCALL(LIB_amigaguidelib, -210, "GetAmigaGuideString_", "(id)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_amigaguidelib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "amigaguide.library";

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
