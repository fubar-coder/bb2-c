/*
 * crmlib - deflibs library 200: AmigaOS calls of CrM.library
 * (8 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (not on the CD as a single file);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("CrM.library", 0) -> d0 = base, 0 if missing
 *          (no retry loop, unlike all other deflibs)
 *   finit: CloseLibrary(base) if base != 0, a1 = base
 *          (!libs LIB_crmlib,#la1).  The original has two dead
 *          instructions (MOVEQ #0,d0:RTS) behind it.
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

BLITZ_OSLIBHEADER(LIB_crmlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_crmlib, -42, "cmCheckCrunched_", "(DataHeader)", a0);
BLITZ_OSCALL(LIB_crmlib, -48, "cmDecrunch_", "(Source,Destination,DataHeader)", a0, a1, a2);
BLITZ_OSCALL(LIB_crmlib, -54, "cmProcessPW_", "(Password,Action)", a0, d0);
BLITZ_OSCALL(LIB_crmlib, -60, "cmCryptData_", "(DataHeader,Data,Password,Action)", a0, a1, a2, d0);
BLITZ_OSCALL(LIB_crmlib, -66, "cmProcessCrunchStructA_", "(CrunchStruct,Action,Tags)", a1, d0, a0);
BLITZ_OSCALL(LIB_crmlib, -72, "cmCrunchData_", "(CrunchStruct)", a0);
BLITZ_OSCALL(LIB_crmlib, -78, "cmAllocCrunchStructA_", "(Tags)", a0);
BLITZ_OSCALL(LIB_crmlib, -84, "cmFreeCrunchStruct_", "(CrunchStruct)", a0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_crmlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "CrM.library";

/* init nullsub: d0 = library base or 0 (no retry) */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return (ULONG)OpenLibrary(libname, 0);
}

/* finit nullsub: a1 = library base (0 if it was not opened) */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    if (base)                           /* MOVE.l a1,d0:TST.l d0:BEQ */
        CloseLibrary(base);
}
