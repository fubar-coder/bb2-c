/*
 * iffparselib - deflibs library 234: AmigaOS calls of iffparse.library
 * (40 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/iffparse.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("iffparse.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_iffparselib,#la1)
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

BLITZ_OSLIBHEADER(LIB_iffparselib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_iffparselib, -30, "AllocIFF_", "");
BLITZ_OSCALL(LIB_iffparselib, -36, "OpenIFF_", "(iff,rwMode)", a0, d0);
BLITZ_OSCALL(LIB_iffparselib, -42, "ParseIFF_", "(iff,control)", a0, d0);
BLITZ_OSCALL(LIB_iffparselib, -48, "CloseIFF_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -54, "FreeIFF_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -60, "ReadChunkBytes_", "(iff,buf,numBytes)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -66, "WriteChunkBytes_", "(iff,buf,numBytes)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -72, "ReadChunkRecords_", "(iff,buf,bytesPerRecord,numRecords)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -78, "WriteChunkRecords_", "(iff,buf,bytesPerRecord,numRecords)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -84, "PushChunk_", "(iff,type,id,size)", a0, d0, d1, d2);
BLITZ_OSCALL(LIB_iffparselib, -90, "PopChunk_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -102, "EntryHandler_", "(iff,type,id,position,handler,object)", a0, d0, d1, d2, a1, a2);
BLITZ_OSCALL(LIB_iffparselib, -108, "ExitHandler_", "(iff,type,id,position,handler,object)", a0, d0, d1, d2, a1, a2);
BLITZ_OSCALL(LIB_iffparselib, -114, "PropChunk_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -120, "PropChunks_", "(iff,propArray,numPairs)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -126, "StopChunk_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -132, "StopChunks_", "(iff,propArray,numPairs)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -138, "CollectionChunk_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -144, "CollectionChunks_", "(iff,propArray,numPairs)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -150, "StopOnExit_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -156, "FindProp_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -162, "FindCollection_", "(iff,type,id)", a0, d0, d1);
BLITZ_OSCALL(LIB_iffparselib, -168, "FindPropContext_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -174, "CurrentChunk_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -180, "ParentChunk_", "(contextNode)", a0);
BLITZ_OSCALL(LIB_iffparselib, -186, "AllocLocalItem_", "(type,id,ident,dataSize)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_iffparselib, -192, "LocalItemData_", "(localItem)", a0);
BLITZ_OSCALL(LIB_iffparselib, -198, "SetLocalItemPurge_", "(localItem,purgeHook)", a0, a1);
BLITZ_OSCALL(LIB_iffparselib, -204, "FreeLocalItem_", "(localItem)", a0);
BLITZ_OSCALL(LIB_iffparselib, -210, "FindLocalItem_", "(iff,type,id,ident)", a0, d0, d1, d2);
BLITZ_OSCALL(LIB_iffparselib, -216, "StoreLocalItem_", "(iff,localItem,position)", a0, a1, d0);
BLITZ_OSCALL(LIB_iffparselib, -222, "StoreItemInContext_", "(iff,localItem,contextNode)", a0, a1, a2);
BLITZ_OSCALL(LIB_iffparselib, -228, "InitIFF_", "(iff,flags,streamHook)", a0, d0, a1);
BLITZ_OSCALL(LIB_iffparselib, -234, "InitIFFasDOS_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -240, "InitIFFasClip_", "(iff)", a0);
BLITZ_OSCALL(LIB_iffparselib, -246, "OpenClipboard_", "(unitNumber)", d0);
BLITZ_OSCALL(LIB_iffparselib, -252, "CloseClipboard_", "(clipHandle)", a0);
BLITZ_OSCALL(LIB_iffparselib, -258, "GoodID_", "(id)", d0);
BLITZ_OSCALL(LIB_iffparselib, -264, "GoodType_", "(type)", d0);
BLITZ_OSCALL(LIB_iffparselib, -270, "IDtoStr_", "(id,buf)", d0, a0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_iffparselib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "iffparse.library";

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
