/*
 * layerslib - deflibs library 232: AmigaOS calls of layers.library
 * (32 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/layers.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("layers.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig) (#44):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_layerslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_layerslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_layerslib, -30, "InitLayers_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -36, "CreateUpfrontLayer_", "(li,bm,x0,y0,x1,y1,flags,bm2)", a0, a1, d0, d1, d2, d3, d4, a2);
BLITZ_OSCALL(LIB_layerslib, -42, "CreateBehindLayer_", "(li,bm,x0,y0,x1,y1,flags,bm2)", a0, a1, d0, d1, d2, d3, d4, a2);
BLITZ_OSCALL(LIB_layerslib, -48, "UpfrontLayer_", "(dummy,layer)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -54, "BehindLayer_", "(dummy,layer)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -60, "MoveLayer_", "(dummy,layer,dx,dy)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_layerslib, -66, "SizeLayer_", "(dummy,layer,dx,dy)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_layerslib, -72, "ScrollLayer_", "(dummy,layer,dx,dy)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_layerslib, -78, "BeginUpdate_", "(l)", a0);
BLITZ_OSCALL(LIB_layerslib, -84, "EndUpdate_", "(layer,flag)", a0, d0);
BLITZ_OSCALL(LIB_layerslib, -90, "DeleteLayer_", "(dummy,layer)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -96, "LockLayer_", "(dummy,layer)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -102, "UnlockLayer_", "(layer)", a0);
BLITZ_OSCALL(LIB_layerslib, -108, "LockLayers_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -114, "UnlockLayers_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -120, "LockLayerInfo_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -126, "SwapBitsRastPortClipRect_", "(rp,cr)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -132, "WhichLayer_", "(li,x,y)", a0, d0, d1);
BLITZ_OSCALL(LIB_layerslib, -138, "UnlockLayerInfo_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -144, "NewLayerInfo_", "");
BLITZ_OSCALL(LIB_layerslib, -150, "DisposeLayerInfo_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -156, "FattenLayerInfo_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -162, "ThinLayerInfo_", "(li)", a0);
BLITZ_OSCALL(LIB_layerslib, -168, "MoveLayerInFrontOf_", "(layer_to_move,other_layer)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -174, "InstallClipRegion_", "(layer,region)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -180, "MoveSizeLayer_", "(layer,dx,dy,dw,dh)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_layerslib, -186, "CreateUpfrontHookLayer_", "(li,bm,x0,y0,x1,y1,flags,hook,bm2)", a0, a1, d0, d1, d2, d3, d4, a3, a2);
BLITZ_OSCALL(LIB_layerslib, -192, "CreateBehindHookLayer_", "(li,bm,x0,y0,x1,y1,flags,hook,bm2)", a0, a1, d0, d1, d2, d3, d4, a3, a2);
BLITZ_OSCALL(LIB_layerslib, -198, "InstallLayerHook_", "(layer,hook)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -204, "InstallLayerInfoHook_", "(li,hook)", a0, a1);
BLITZ_OSCALL(LIB_layerslib, -210, "SortLayerCR_", "(layer,dx,dy)", a0, d0, d1);
BLITZ_OSCALL(LIB_layerslib, -216, "DoHookClipRects_", "(hook,rport,rect)", a0, a1, a2);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_layerslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "layers.library";

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
