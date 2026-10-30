/*
 * utilitylib - deflibs library 221: AmigaOS calls of utility.library
 * (38 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/utility.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("utility.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig) (#44):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_utilitylib,#la1)
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

BLITZ_OSLIBHEADER(LIB_utilitylib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_utilitylib, -30, "FindTagItem_", "(tagVal,tagList)", d0, a0);
BLITZ_OSCALL(LIB_utilitylib, -36, "GetTagData_", "(tagValue,defaultVal,tagList)", d0, d1, a0);
BLITZ_OSCALL(LIB_utilitylib, -42, "PackBoolTags_", "(initialFlags,tagList,boolMap)", d0, a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -48, "NextTagItem_", "(tagListPtr)", a0);
BLITZ_OSCALL(LIB_utilitylib, -54, "FilterTagChanges_", "(changeList,originalList,apply)", a0, a1, d0);
BLITZ_OSCALL(LIB_utilitylib, -60, "MapTags_", "(tagList,mapList,mapType)", a0, a1, d0);
BLITZ_OSCALL(LIB_utilitylib, -66, "AllocateTagItems_", "(numTags)", d0);
BLITZ_OSCALL(LIB_utilitylib, -72, "CloneTagItems_", "(tagList)", a0);
BLITZ_OSCALL(LIB_utilitylib, -78, "FreeTagItems_", "(tagList)", a0);
BLITZ_OSCALL(LIB_utilitylib, -84, "RefreshTagItemClones_", "(clone,original)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -90, "TagInArray_", "(tagValue,tagArray)", d0, a0);
BLITZ_OSCALL(LIB_utilitylib, -96, "FilterTagItems_", "(tagList,filterArray,logic)", a0, a1, d0);
BLITZ_OSCALL(LIB_utilitylib, -102, "CallHookPkt_", "(hook,object,paramPacket)", a0, a2, a1);
BLITZ_OSCALL(LIB_utilitylib, -120, "Amiga2Date_", "(seconds,result)", d0, a0);
BLITZ_OSCALL(LIB_utilitylib, -126, "Date2Amiga_", "(date)", a0);
BLITZ_OSCALL(LIB_utilitylib, -132, "CheckDate_", "(date)", a0);
BLITZ_OSCALL(LIB_utilitylib, -138, "SMult32_", "(arg1,arg2)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -144, "UMult32_", "(arg1,arg2)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -150, "SDivMod32_", "(dividend,divisor)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -156, "UDivMod32_", "(dividend,divisor)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -162, "Stricmp_", "(string1,string2)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -168, "Strnicmp_", "(string1,string2,length)", a0, a1, d0);
BLITZ_OSCALL(LIB_utilitylib, -174, "ToUpper_", "(character)", d0);
BLITZ_OSCALL(LIB_utilitylib, -180, "ToLower_", "(character)", d0);
BLITZ_OSCALL(LIB_utilitylib, -186, "ApplyTagChanges_", "(list,changeList)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -198, "SMult64_", "(arg1,arg2)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -204, "UMult64_", "(arg1,arg2)", d0, d1);
BLITZ_OSCALL(LIB_utilitylib, -210, "PackStructureTags_", "(pack,packtable,tagList)", a0, a1, a2);
BLITZ_OSCALL(LIB_utilitylib, -216, "UnpackStructureTags_", "(pack,packtable,tagList)", a0, a1, a2);
BLITZ_OSCALL(LIB_utilitylib, -222, "AddNamedObject_", "(namespace,object)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -228, "AllocNamedObjectA_", "(name,tags)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -234, "AttemptRemNamedObject_", "(object)", a0);
BLITZ_OSCALL(LIB_utilitylib, -240, "FindNamedObject_", "(namespace,name,lastobject)", a0, a1, a2);
BLITZ_OSCALL(LIB_utilitylib, -246, "FreeNamedObject_", "(object)", a0);
BLITZ_OSCALL(LIB_utilitylib, -252, "NamedObjectName_", "(object)", a0);
BLITZ_OSCALL(LIB_utilitylib, -258, "ReleaseNamedObject_", "(object)", a0);
BLITZ_OSCALL(LIB_utilitylib, -264, "RemNamedObject_", "(object,message)", a0, a1);
BLITZ_OSCALL(LIB_utilitylib, -270, "GetUniqueID_", "");

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_utilitylib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "utility.library";

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
