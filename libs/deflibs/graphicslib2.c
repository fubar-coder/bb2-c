/*
 * graphicslib2 - deflibs library 250: more AmigaOS calls
 * of library 254 (graphicslib)
 * (40 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/graphics.library2);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   no init/finit: the calls use the base of 254
 *   (LIB_graphicslib), this library only adds declarations
 *
 * Every call: BLITZ_OSCALL(base lib, LVO, name, help, argument
 * registers); the compiler generates the call (blitz2.s stamigalib).
 */
#include "blitzlib.h"

BLITZ_OSLIBHEADER(LIB_graphicslib2, 0, BL_NORET, 0);

BLITZ_OSCALL(LIB_graphicslib, -810, "EraseRect_", "(rp,xMin,yMin,xMax,yMax)", a1, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -816, "ExtendFont_", "(font,fontTags)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -822, "StripFont_", "(font)", a0);
BLITZ_OSCALL(LIB_graphicslib, -828, "CalcIVG_", "(v,vp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -834, "AttachPalExtra_", "(cm,vp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -840, "ObtainBestPenA_", "(cm,r,g,b,tags)", a0, d1, d2, d3, a1);
BLITZ_OSCALL(LIB_graphicslib, -846, "GfxInternal3_", "");
BLITZ_OSCALL(LIB_graphicslib, -852, "SetRGB32_", "(vp,n,r,g,b)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -858, "GetAPen_", "(rp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -864, "GetBPen_", "(rp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -870, "GetDrMd_", "(rp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -876, "GetOutlinePen_", "(rp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -882, "LoadRGB32_", "(vp,table)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -888, "SetChipRev_", "(want)", d0);
BLITZ_OSCALL(LIB_graphicslib, -894, "SetABPenDrMd_", "(rp,apen,bpen,drawmode)", a1, d0, d1, d2);
BLITZ_OSCALL(LIB_graphicslib, -900, "GetRGB32_", "(cm,firstcolor,ncolors,table)", a0, d0, d1, a1);
BLITZ_OSCALL(LIB_graphicslib, -906, "GfxSpare1_", "");
BLITZ_OSCALL(LIB_graphicslib, -918, "AllocBitMap_", "(sizex,sizey,depth,flags,friend_bitmap)", d0, d1, d2, d3, a0);
BLITZ_OSCALL(LIB_graphicslib, -924, "FreeBitMap_", "(bm)", a0);
BLITZ_OSCALL(LIB_graphicslib, -930, "GetExtSpriteA_", "(ss,tags)", a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -936, "CoerceMode_", "(vp,monitorid,flags)", a0, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -942, "ChangeVPBitMap_", "(vp,bm,db)", a0, a1, a2);
BLITZ_OSCALL(LIB_graphicslib, -948, "ReleasePen_", "(cm,n)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -954, "ObtainPen_", "(cm,n,r,g,b,f)", a0, d0, d1, d2, d3, d4);
BLITZ_OSCALL(LIB_graphicslib, -960, "GetBitMapAttr_", "(bm,attrnum)", a0, d1);
BLITZ_OSCALL(LIB_graphicslib, -966, "AllocDBufInfo_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -972, "FreeDBufInfo_", "(dbi)", a1);
BLITZ_OSCALL(LIB_graphicslib, -978, "SetOutlinePen_", "(rp,pen)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -984, "SetWriteMask_", "(rp,msk)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -990, "SetMaxPen_", "(rp,maxpen)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -996, "SetRGB32CM_", "(cm,n,r,g,b)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -1002, "ScrollRasterBF_", "(rp,dx,dy,xMin,yMin,xMax,yMax)", a1, d0, d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_graphicslib, -1008, "FindColor_", "(cm,r,g,b,maxcolor)", a3, d1, d2, d3, d4);
BLITZ_OSCALL(LIB_graphicslib, -1014, "GfxSpare2_", "");
BLITZ_OSCALL(LIB_graphicslib, -1020, "AllocSpriteDataA_", "(bm,tags)", a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -1026, "ChangeExtSpriteA_", "(vp,oldsprite,newsprite,tags)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_graphicslib, -1032, "FreeSpriteData_", "(sp)", a2);
BLITZ_OSCALL(LIB_graphicslib, -1038, "SetRPAttrsA_", "(rp,tags)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -1044, "GetRPAttrsA_", "(rp,tags)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -1050, "BestModeIDA_", "(tags)", a0);

BLITZ_LIBFIN();
