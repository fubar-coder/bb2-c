/*
 * datatypeslib - deflibs library 239: AmigaOS calls of datatypes.library
 * (16 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/datatypes.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("datatypes.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_datatypeslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_datatypeslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_datatypeslib, -30, "RLDispatch_", "(rmsg,result)", a0, a1);
BLITZ_OSCALL(LIB_datatypeslib, -36, "ObtainDataTypeA_", "(type,handle,attrs)", d0, a0, a1);
BLITZ_OSCALL(LIB_datatypeslib, -42, "ReleaseDataType_", "(dt)", a0);
BLITZ_OSCALL(LIB_datatypeslib, -48, "NewDTObjectA_", "(name,attrs)", d0, a0);
BLITZ_OSCALL(LIB_datatypeslib, -54, "DisposeDTObject_", "(o)", a0);
BLITZ_OSCALL(LIB_datatypeslib, -60, "SetDTAttrsA_", "(o,win,req,attrs)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_datatypeslib, -66, "GetDTAttrsA_", "(o,attrs)", a0, a2);
BLITZ_OSCALL(LIB_datatypeslib, -72, "AddDTObject_", "(win,req,o,pos)", a0, a1, a2, d0);
BLITZ_OSCALL(LIB_datatypeslib, -78, "RefreshDTObjectA_", "(o,win,req,attrs)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_datatypeslib, -84, "DoAsyncLayout_", "(o,gpl)", a0, a1);
BLITZ_OSCALL(LIB_datatypeslib, -90, "DoDTMethodA_", "(o,win,req,msg)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_datatypeslib, -96, "RemoveDTObject_", "(win,o)", a0, a1);
BLITZ_OSCALL(LIB_datatypeslib, -102, "GetDTMethods_", "(object)", a0);
BLITZ_OSCALL(LIB_datatypeslib, -108, "GetDTTriggerMethods_", "(object)", a0);
BLITZ_OSCALL(LIB_datatypeslib, -114, "PrintDTObjectA_", "(o,w,r,msg)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_datatypeslib, -138, "GetDTString_", "(id)", d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_datatypeslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "datatypes.library";

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
