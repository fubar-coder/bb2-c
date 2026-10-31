/*
 * iconlib - deflibs library 235: AmigaOS calls of icon.library
 * (14 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/icon.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("icon.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_iconlib,#la1)
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

BLITZ_OSLIBHEADER(LIB_iconlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_iconlib, -42, "GetIcon_", "(name,icon,freelist)", a0, a1, a2);
BLITZ_OSCALL(LIB_iconlib, -48, "PutIcon_", "(name,icon)", a0, a1);
BLITZ_OSCALL(LIB_iconlib, -54, "FreeFreeList_", "(freelist)", a0);
BLITZ_OSCALL(LIB_iconlib, -72, "AddFreeList_", "(freelist,mem,size)", a0, a1, a2);
BLITZ_OSCALL(LIB_iconlib, -78, "GetDiskObject_", "(name)", a0);
BLITZ_OSCALL(LIB_iconlib, -84, "PutDiskObject_", "(name,diskobj)", a0, a1);
BLITZ_OSCALL(LIB_iconlib, -90, "FreeDiskObject_", "(diskobj)", a0);
BLITZ_OSCALL(LIB_iconlib, -96, "FindToolType_", "(toolTypeArray,typeName)", a0, a1);
BLITZ_OSCALL(LIB_iconlib, -102, "MatchToolValue_", "(typeString,value)", a0, a1);
BLITZ_OSCALL(LIB_iconlib, -108, "BumpRevision_", "(newname,oldname)", a0, a1);
BLITZ_OSCALL(LIB_iconlib, -120, "GetDefDiskObject_", "(type)", d0);
BLITZ_OSCALL(LIB_iconlib, -126, "PutDefDiskObject_", "(diskObject)", a0);
BLITZ_OSCALL(LIB_iconlib, -132, "GetDiskObjectNew_", "(name)", a0);
BLITZ_OSCALL(LIB_iconlib, -138, "DeleteDiskObject_", "(name)", a0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_iconlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "icon.library";

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
