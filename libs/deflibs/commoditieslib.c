/*
 * commoditieslib - deflibs library 240: AmigaOS calls of commodities.library
 * (26 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/commodities.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("commodities.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_commoditieslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_commoditieslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_commoditieslib, -30, "CreateCxObj_", "(type,arg1,arg2)", d0, a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -36, "CxBroker_", "(nb,error)", a0, d0);
BLITZ_OSCALL(LIB_commoditieslib, -42, "ActivateCxObj_", "(co,true)", a0, d0);
BLITZ_OSCALL(LIB_commoditieslib, -48, "DeleteCxObj_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -54, "DeleteCxObjAll_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -60, "CxObjType_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -66, "CxObjError_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -72, "ClearCxObjError_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -78, "SetCxObjPri_", "(co,pri)", a0, d0);
BLITZ_OSCALL(LIB_commoditieslib, -84, "AttachCxObj_", "(headObj,co)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -90, "EnqueueCxObj_", "(headObj,co)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -96, "InsertCxObj_", "(headObj,co,pred)", a0, a1, a2);
BLITZ_OSCALL(LIB_commoditieslib, -102, "RemoveCxObj_", "(co)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -114, "SetTranslate_", "(translator,events)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -120, "SetFilter_", "(filter,text)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -126, "SetFilterIX_", "(filter,ix)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -132, "ParseIX_", "(description,ix)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -138, "CxMsgType_", "(cxm)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -144, "CxMsgData_", "(cxm)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -150, "CxMsgID_", "(cxm)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -156, "DivertCxMsg_", "(cxm,headObj,returnObj)", a0, a1, a2);
BLITZ_OSCALL(LIB_commoditieslib, -162, "RouteCxMsg_", "(cxm,co)", a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -168, "DisposeCxMsg_", "(cxm)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -174, "InvertKeyMap_", "(ansiCode,event,km)", d0, a0, a1);
BLITZ_OSCALL(LIB_commoditieslib, -180, "AddIEvents_", "(events)", a0);
BLITZ_OSCALL(LIB_commoditieslib, -204, "MatchIX_", "(event,ix)", a0, a1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_commoditieslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "commodities.library";

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
