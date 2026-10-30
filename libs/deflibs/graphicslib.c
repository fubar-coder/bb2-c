/*
 * graphicslib - deflibs library 254: AmigaOS calls of graphics.library
 * (127 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/graphics.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("graphics.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig) (#44):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_graphicslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_graphicslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_graphicslib, -30, "BltBitMap_", "(srcBitMap,xSrc,ySrc,destBitMap,xDest,yDest,xSize,ySize,minterm,mask,tempA)", a0, d0, d1, a1, d2, d3, d4, d5, d6, d7, a2);
BLITZ_OSCALL(LIB_graphicslib, -36, "BltTemplate_", "(source,xSrc,srcMod,destRP,xDest,yDest,xSize,ySize)", a0, d0, d1, a1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_graphicslib, -42, "ClearEOL_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -48, "ClearScreen_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -54, "TextLength_", "(rp,string,count)", a1, a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -60, "Text_", "(rp,string,count)", a1, a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -66, "SetFont_", "(rp,textFont)", a1, a0);
BLITZ_OSCALL(LIB_graphicslib, -72, "OpenFont_", "(textAttr)", a0);
BLITZ_OSCALL(LIB_graphicslib, -78, "CloseFont_", "(textFont)", a1);
BLITZ_OSCALL(LIB_graphicslib, -84, "AskSoftStyle_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -90, "SetSoftStyle_", "(rp,style,enable)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -96, "AddBob_", "(bob,rp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -102, "AddVSprite_", "(vSprite,rp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -108, "DoCollision_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -114, "DrawGList_", "(rp,vp)", a1, a0);
BLITZ_OSCALL(LIB_graphicslib, -120, "InitGels_", "(head,tail,gelsInfo)", a0, a1, a2);
BLITZ_OSCALL(LIB_graphicslib, -126, "InitMasks_", "(vSprite)", a0);
BLITZ_OSCALL(LIB_graphicslib, -132, "RemIBob_", "(bob,rp,vp)", a0, a1, a2);
BLITZ_OSCALL(LIB_graphicslib, -138, "RemVSprite_", "(vSprite)", a0);
BLITZ_OSCALL(LIB_graphicslib, -144, "SetCollision_", "(num,routine,gelsInfo)", d0, a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -150, "SortGList_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -156, "AddAnimOb_", "(anOb,anKey,rp)", a0, a1, a2);
BLITZ_OSCALL(LIB_graphicslib, -162, "Animate_", "(anKey,rp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -168, "GetGBuffers_", "(anOb,rp,flag)", a0, a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -174, "InitGMasks_", "(anOb)", a0);
BLITZ_OSCALL(LIB_graphicslib, -180, "DrawEllipse_", "(rp,xCenter,yCenter,a,b)", a1, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -186, "AreaEllipse_", "(rp,xCenter,yCenter,a,b)", a1, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -192, "LoadRGB4_", "(vp,colors,count)", a0, a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -198, "InitRastPort_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -204, "InitVPort_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -210, "MrgCop_", "(view)", a1);
BLITZ_OSCALL(LIB_graphicslib, -216, "MakeVPort_", "(view,vp)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -222, "LoadView_", "(view)", a1);
BLITZ_OSCALL(LIB_graphicslib, -228, "WaitBlit_", "");
BLITZ_OSCALL(LIB_graphicslib, -234, "SetRast_", "(rp,pen)", a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -240, "Move_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -246, "Draw_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -252, "AreaMove_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -258, "AreaDraw_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -264, "AreaEnd_", "(rp)", a1);
BLITZ_OSCALL(LIB_graphicslib, -270, "WaitTOF_", "");
BLITZ_OSCALL(LIB_graphicslib, -276, "QBlit_", "(blit)", a1);
BLITZ_OSCALL(LIB_graphicslib, -282, "InitArea_", "(areaInfo,vectorBuffer,maxVectors)", a0, a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -288, "SetRGB4_", "(vp,index,red,green,blue)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -294, "QBSBlit_", "(blit)", a1);
BLITZ_OSCALL(LIB_graphicslib, -300, "BltClear_", "(memBlock,byteCount,flags)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -306, "RectFill_", "(rp,xMin,yMin,xMax,yMax)", a1, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -312, "BltPattern_", "(rp,mask,xMin,yMin,xMax,yMax,maskBPR)", a1, a0, d0, d1, d2, d3, d4);
BLITZ_OSCALL(LIB_graphicslib, -318, "ReadPixel_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -324, "WritePixel_", "(rp,x,y)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -330, "Flood_", "(rp,mode,x,y)", a1, d2, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -336, "PolyDraw_", "(rp,count,polyTable)", a1, d0, a0);
BLITZ_OSCALL(LIB_graphicslib, -342, "SetAPen_", "(rp,pen)", a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -348, "SetBPen_", "(rp,pen)", a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -354, "SetDrMd_", "(rp,drawMode)", a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -360, "InitView_", "(view)", a1);
BLITZ_OSCALL(LIB_graphicslib, -366, "CBump_", "(copList)", a1);
BLITZ_OSCALL(LIB_graphicslib, -372, "CMove_", "(copList,destination,data)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -378, "CWait_", "(copList,v,h)", a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -384, "VBeamPos_", "");
BLITZ_OSCALL(LIB_graphicslib, -390, "InitBitMap_", "(bitMap,depth,width,height)", a0, d0, d1, d2);
BLITZ_OSCALL(LIB_graphicslib, -396, "ScrollRaster_", "(rp,dx,dy,xMin,yMin,xMax,yMax)", a1, d0, d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_graphicslib, -402, "WaitBOVP_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -408, "GetSprite_", "(sprite,num)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -414, "FreeSprite_", "(num)", d0);
BLITZ_OSCALL(LIB_graphicslib, -420, "ChangeSprite_", "(vp,sprite,newData)", a0, a1, a2);
BLITZ_OSCALL(LIB_graphicslib, -426, "MoveSprite_", "(vp,sprite,x,y)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -432, "LockLayerRom_", "(layer)", a5);
BLITZ_OSCALL(LIB_graphicslib, -438, "UnlockLayerRom_", "(layer)", a5);
BLITZ_OSCALL(LIB_graphicslib, -444, "SyncSBitMap_", "(layer)", a0);
BLITZ_OSCALL(LIB_graphicslib, -450, "CopySBitMap_", "(layer)", a0);
BLITZ_OSCALL(LIB_graphicslib, -456, "OwnBlitter_", "");
BLITZ_OSCALL(LIB_graphicslib, -462, "DisownBlitter_", "");
BLITZ_OSCALL(LIB_graphicslib, -468, "InitTmpRas_", "(tmpRas,buffer,size)", a0, a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -474, "AskFont_", "(rp,textAttr)", a1, a0);
BLITZ_OSCALL(LIB_graphicslib, -480, "AddFont_", "(textFont)", a1);
BLITZ_OSCALL(LIB_graphicslib, -486, "RemFont_", "(textFont)", a1);
BLITZ_OSCALL(LIB_graphicslib, -492, "AllocRaster_", "(width,height)", d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -498, "FreeRaster_", "(p,width,height)", a0, d0, d1);
BLITZ_OSCALL(LIB_graphicslib, -504, "AndRectRegion_", "(region,rectangle)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -510, "OrRectRegion_", "(region,rectangle)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -516, "NewRegion_", "");
BLITZ_OSCALL(LIB_graphicslib, -522, "ClearRectRegion_", "(region,rectangle)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -528, "ClearRegion_", "(region)", a0);
BLITZ_OSCALL(LIB_graphicslib, -534, "DisposeRegion_", "(region)", a0);
BLITZ_OSCALL(LIB_graphicslib, -540, "FreeVPortCopLists_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -546, "FreeCopList_", "(copList)", a0);
BLITZ_OSCALL(LIB_graphicslib, -552, "ClipBlit_", "(srcRP,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm)", a0, d0, d1, a1, d2, d3, d4, d5, d6);
BLITZ_OSCALL(LIB_graphicslib, -558, "XorRectRegion_", "(region,rectangle)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -564, "FreeCprList_", "(cprList)", a0);
BLITZ_OSCALL(LIB_graphicslib, -570, "GetColorMap_", "(entries)", d0);
BLITZ_OSCALL(LIB_graphicslib, -576, "FreeColorMap_", "(colorMap)", a0);
BLITZ_OSCALL(LIB_graphicslib, -582, "GetRGB4_", "(colorMap,entry)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -588, "ScrollVPort_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -594, "UCopperListInit_", "(uCopList,n)", a0, d0);
BLITZ_OSCALL(LIB_graphicslib, -600, "FreeGBuffers_", "(anOb,rp,flag)", a0, a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -606, "BltBitMapRastPort_", "(srcBitMap,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm)", a0, d0, d1, a1, d2, d3, d4, d5, d6);
BLITZ_OSCALL(LIB_graphicslib, -612, "OrRegionRegion_", "(srcRegion,destRegion)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -618, "XorRegionRegion_", "(srcRegion,destRegion)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -624, "AndRegionRegion_", "(srcRegion,destRegion)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -630, "SetRGB4CM_", "(colorMap,index,red,green,blue)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -636, "BltMaskBitMapRastPort_", "(srcBitMap,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm,bltMask)", a0, d0, d1, a1, d2, d3, d4, d5, d6, a2);
BLITZ_OSCALL(LIB_graphicslib, -642, "GfxInternal1_", "");
BLITZ_OSCALL(LIB_graphicslib, -648, "GfxInternal2_", "");
BLITZ_OSCALL(LIB_graphicslib, -654, "AttemptLockLayerRom_", "(layer)", a5);
BLITZ_OSCALL(LIB_graphicslib, -660, "GfxNew_", "(gfxNodeType)", d0);
BLITZ_OSCALL(LIB_graphicslib, -666, "GfxFree_", "(gfxNodePtr)", a0);
BLITZ_OSCALL(LIB_graphicslib, -672, "GfxAssociate_", "(associateNode,gfxNodePtr)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -678, "BitMapScale_", "(bitScaleArgs)", a0);
BLITZ_OSCALL(LIB_graphicslib, -684, "ScalerDiv_", "(factor,numerator,denominator)", d0, d1, d2);
BLITZ_OSCALL(LIB_graphicslib, -690, "TextExtent_", "(rp,string,count,textExtent)", a1, a0, d0, a2);
BLITZ_OSCALL(LIB_graphicslib, -696, "TextFit_", "(rp,string,strLen,textExtent,constrainingExtent,strDirection,constrainingBitWidth,constrainingBitHeight)", a1, a0, d0, a2, a3, d1, d2, d3);
BLITZ_OSCALL(LIB_graphicslib, -702, "GfxLookUp_", "(associateNode)", a0);
BLITZ_OSCALL(LIB_graphicslib, -708, "VideoControl_", "(colorMap,tagarray)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -714, "OpenMonitor_", "(monitorName,displayID)", a1, d0);
BLITZ_OSCALL(LIB_graphicslib, -720, "CloseMonitor_", "(monitorSpec)", a0);
BLITZ_OSCALL(LIB_graphicslib, -726, "FindDisplayInfo_", "(displayID)", d0);
BLITZ_OSCALL(LIB_graphicslib, -732, "NextDisplayInfo_", "(displayID)", d0);
BLITZ_OSCALL(LIB_graphicslib, -756, "GetDisplayInfoData_", "(handle,buf,size,tagID,displayID)", a0, a1, d0, d1, d2);
BLITZ_OSCALL(LIB_graphicslib, -762, "FontExtent_", "(font,fontExtent)", a0, a1);
BLITZ_OSCALL(LIB_graphicslib, -768, "ReadPixelLine8_", "(rp,xstart,ystart,width,array,tempRP)", a0, d0, d1, d2, a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -774, "WritePixelLine8_", "(rp,xstart,ystart,width,array,tempRP)", a0, d0, d1, d2, a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -780, "ReadPixelArray8_", "(rp,xstart,ystart,xstop,ystop,array,temprp)", a0, d0, d1, d2, d3, a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -786, "WritePixelArray8_", "(rp,xstart,ystart,xstop,ystop,array,temprp)", a0, d0, d1, d2, d3, a2, a1);
BLITZ_OSCALL(LIB_graphicslib, -792, "GetVPModeID_", "(vp)", a0);
BLITZ_OSCALL(LIB_graphicslib, -798, "ModeNotAvailable_", "(modeID)", d0);
BLITZ_OSCALL(LIB_graphicslib, -804, "WeighTAMatch_", "(reqTextAttr,targetTextAttr,targetTags)", a0, a1, a2);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_graphicslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "graphics.library";

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
