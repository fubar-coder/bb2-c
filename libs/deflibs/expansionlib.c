/*
 * expansionlib - deflibs library 237: AmigaOS calls of expansion.library
 * (21 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/expansion.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("expansion.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_expansionlib,#la1)
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

BLITZ_OSLIBHEADER(LIB_expansionlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_expansionlib, -30, "AddConfigDev_", "(configDev)", a0);
BLITZ_OSCALL(LIB_expansionlib, -36, "AddBootNode_", "(bootPri,flags,deviceNode,configDev)", d0, d1, a0, a1);
BLITZ_OSCALL(LIB_expansionlib, -42, "AllocBoardMem_", "(slotSpec)", d0);
BLITZ_OSCALL(LIB_expansionlib, -48, "AllocConfigDev_", "");
BLITZ_OSCALL(LIB_expansionlib, -54, "AllocExpansionMem_", "(numSlots,slotAlign)", d0, d1);
BLITZ_OSCALL(LIB_expansionlib, -60, "ConfigBoard_", "(board,configDev)", a0, a1);
BLITZ_OSCALL(LIB_expansionlib, -66, "ConfigChain_", "(baseAddr)", a0);
BLITZ_OSCALL(LIB_expansionlib, -72, "FindConfigDev_", "(oldConfigDev,manufacturer,product)", a0, d0, d1);
BLITZ_OSCALL(LIB_expansionlib, -78, "FreeBoardMem_", "(startSlot,slotSpec)", d0, d1);
BLITZ_OSCALL(LIB_expansionlib, -84, "FreeConfigDev_", "(configDev)", a0);
BLITZ_OSCALL(LIB_expansionlib, -90, "FreeExpansionMem_", "(startSlot,numSlots)", d0, d1);
BLITZ_OSCALL(LIB_expansionlib, -96, "ReadExpansionByte_", "(board,offset)", a0, d0);
BLITZ_OSCALL(LIB_expansionlib, -102, "ReadExpansionRom_", "(board,configDev)", a0, a1);
BLITZ_OSCALL(LIB_expansionlib, -108, "RemConfigDev_", "(configDev)", a0);
BLITZ_OSCALL(LIB_expansionlib, -114, "WriteExpansionByte_", "(board,offset,byte)", a0, d0, d1);
BLITZ_OSCALL(LIB_expansionlib, -120, "ObtainConfigBinding_", "");
BLITZ_OSCALL(LIB_expansionlib, -126, "ReleaseConfigBinding_", "");
BLITZ_OSCALL(LIB_expansionlib, -132, "SetCurrentBinding_", "(currentBinding,bindingSize)", a0, d0);
BLITZ_OSCALL(LIB_expansionlib, -138, "GetCurrentBinding_", "(currentBinding,bindingSize)", a0, d0);
BLITZ_OSCALL(LIB_expansionlib, -144, "MakeDosNode_", "(parmPacket)", a0);
BLITZ_OSCALL(LIB_expansionlib, -150, "AddDosNode_", "(bootPri,flags,deviceNode)", d0, d1, a0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_expansionlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "expansion.library";

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
