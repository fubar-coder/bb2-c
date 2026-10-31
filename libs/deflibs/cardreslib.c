/*
 * cardreslib - deflibs library 243: AmigaOS calls of card.resource
 * (17 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/card.resource1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenResource("card.resource") -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  Fixed (intended
 *          deviation, #45): the original opens "esource" (LEA
 *          points 6 bytes into the name), which never exists; here
 *          the name.  Fixed (intended deviation, #44): 0 if the
 *          resource cannot be opened; the original retries forever
 *          (deflib.h)
 *   finit: nothing (resources are not closed; !libs
 *          LIB_cardreslib,#la1 as with the libraries)
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

BLITZ_OSLIBHEADER(LIB_cardreslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_cardreslib, -6, "OwnCard_", "(handle)", a1);
BLITZ_OSCALL(LIB_cardreslib, -12, "ReleaseCard_", "(handle,flags)", a1, d0);
BLITZ_OSCALL(LIB_cardreslib, -18, "GetCardMap_", "");
BLITZ_OSCALL(LIB_cardreslib, -24, "BeginCardAccess_", "(handle)", a1);
BLITZ_OSCALL(LIB_cardreslib, -30, "EndCardAccess_", "(handle)", a1);
BLITZ_OSCALL(LIB_cardreslib, -36, "ReadCardStatus_", "");
BLITZ_OSCALL(LIB_cardreslib, -42, "CardResetRemove_", "(handle,flag)", a1, d0);
BLITZ_OSCALL(LIB_cardreslib, -48, "CardMiscControl_", "(handle,control_bits)", a1, d1);
BLITZ_OSCALL(LIB_cardreslib, -54, "CardAccessSpeed_", "(handle,nanoseconds)", a1, d0);
BLITZ_OSCALL(LIB_cardreslib, -60, "CardProgramVoltage_", "(handle,voltage)", a1, d0);
BLITZ_OSCALL(LIB_cardreslib, -66, "CardResetCard_", "(handle)", a1);
BLITZ_OSCALL(LIB_cardreslib, -72, "CopyTuple_", "(handle,buffer,tuplecode,size)", a1, a0, d1, d0);
BLITZ_OSCALL(LIB_cardreslib, -78, "DeviceTuple_", "(tuple_data,storage)", a0, a1);
BLITZ_OSCALL(LIB_cardreslib, -84, "IfAmigaXIP_", "(handle)", a2);
BLITZ_OSCALL(LIB_cardreslib, -90, "CardForceChange_", "");
BLITZ_OSCALL(LIB_cardreslib, -96, "CardChangeCount_", "");
BLITZ_OSCALL(LIB_cardreslib, -102, "CardInterface_", "");

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_cardreslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "card.resource";

/* init nullsub: d0 = resource base, 0 if it cannot be opened (fixed,
   #44 and #45: the original has LEA libname+6(pc),a1 and retries
   forever) */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openresource(libname);
}

/* finit nullsub: a1 = resource base, nothing to do (RTS) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    (void)base;
}
