/*
 * intuitionlib - deflibs library 255: AmigaOS calls of intuition.library
 * (124 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/intuition.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("intuition.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_intuitionlib,#la1)
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

BLITZ_OSLIBHEADER(LIB_intuitionlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_intuitionlib, -30, "OpenIntuition_", "");
BLITZ_OSCALL(LIB_intuitionlib, -36, "Intuition_", "(iEvent)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -42, "AddGadget_", "(window,gadget,position)", a0, a1, d0);
BLITZ_OSCALL(LIB_intuitionlib, -48, "ClearDMRequest_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -54, "ClearMenuStrip_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -60, "ClearPointer_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -66, "CloseScreen_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -72, "CloseWindow_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -78, "CloseWorkBench_", "");
BLITZ_OSCALL(LIB_intuitionlib, -84, "CurrentTime_", "(seconds,micros)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -90, "DisplayAlert_", "(alertNumber,string,height)", d0, a0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -96, "DisplayBeep_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -102, "DoubleClick_", "(sSeconds,sMicros,cSeconds,cMicros)", d0, d1, d2, d3);
BLITZ_OSCALL(LIB_intuitionlib, -108, "DrawBorder_", "(rp,border,leftOffset,topOffset)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -114, "DrawImage_", "(rp,image,leftOffset,topOffset)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -120, "EndRequest_", "(requester,window)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -126, "GetDefPrefs_", "(preferences,size)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -132, "GetPrefs_", "(preferences,size)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -138, "InitRequester_", "(requester)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -144, "ItemAddress_", "(menuStrip,menuNumber)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -150, "ModifyIDCMP_", "(window,flags)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -156, "ModifyProp_", "(gadget,window,requester,flags,horizPot,vertPot,horizBody,vertBody)", a0, a1, a2, d0, d1, d2, d3, d4);
BLITZ_OSCALL(LIB_intuitionlib, -162, "MoveScreen_", "(screen,dx,dy)", a0, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -168, "MoveWindow_", "(window,dx,dy)", a0, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -174, "OffGadget_", "(gadget,window,requester)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -180, "OffMenu_", "(window,menuNumber)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -186, "OnGadget_", "(gadget,window,requester)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -192, "OnMenu_", "(window,menuNumber)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -198, "OpenScreen_", "(newScreen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -204, "OpenWindow_", "(newWindow)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -210, "OpenWorkBench_", "");
BLITZ_OSCALL(LIB_intuitionlib, -216, "PrintIText_", "(rp,iText,left,top)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -222, "RefreshGadgets_", "(gadgets,window,requester)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -228, "RemoveGadget_", "(window,gadget)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -234, "ReportMouse_", "(flag,window)", d0, a0);
BLITZ_OSCALL(LIB_intuitionlib, -240, "Request_", "(requester,window)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -246, "ScreenToBack_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -252, "ScreenToFront_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -258, "SetDMRequest_", "(window,requester)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -264, "SetMenuStrip_", "(window,menu)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -270, "SetPointer_", "(window,pointer,height,width,xOffset,yOffset)", a0, a1, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_intuitionlib, -276, "SetWindowTitles_", "(window,windowTitle,screenTitle)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -282, "ShowTitle_", "(screen,showIt)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -288, "SizeWindow_", "(window,dx,dy)", a0, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -294, "ViewAddress_", "");
BLITZ_OSCALL(LIB_intuitionlib, -300, "ViewPortAddress_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -306, "WindowToBack_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -312, "WindowToFront_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -318, "WindowLimits_", "(window,widthMin,heightMin,widthMax,heightMax)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_intuitionlib, -324, "SetPrefs_", "(preferences,size,inform)", a0, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -330, "IntuiTextLength_", "(iText)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -336, "WBenchToBack_", "");
BLITZ_OSCALL(LIB_intuitionlib, -342, "WBenchToFront_", "");
BLITZ_OSCALL(LIB_intuitionlib, -348, "AutoRequest_", "(window,body,posText,negText,pFlag,nFlag,width,height)", a0, a1, a2, a3, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_intuitionlib, -354, "BeginRefresh_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -360, "BuildSysRequest_", "(window,body,posText,negText,flags,width,height)", a0, a1, a2, a3, d0, d1, d2);
BLITZ_OSCALL(LIB_intuitionlib, -366, "EndRefresh_", "(window,complete)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -372, "FreeSysRequest_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -378, "MakeScreen_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -384, "RemakeDisplay_", "");
BLITZ_OSCALL(LIB_intuitionlib, -390, "RethinkDisplay_", "");
BLITZ_OSCALL(LIB_intuitionlib, -396, "AllocRemember_", "(rememberKey,size,flags)", a0, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -402, "AlohaWorkbench_", "(wbport)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -408, "FreeRemember_", "(rememberKey,reallyForget)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -414, "LockIBase_", "(dontknow)", d0);
BLITZ_OSCALL(LIB_intuitionlib, -420, "UnlockIBase_", "(ibLock)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -426, "GetScreenData_", "(buffer,size,type,screen)", a0, d0, d1, a1);
BLITZ_OSCALL(LIB_intuitionlib, -432, "RefreshGList_", "(gadgets,window,requester,numGad)", a0, a1, a2, d0);
BLITZ_OSCALL(LIB_intuitionlib, -438, "AddGList_", "(window,gadget,position,numGad,requester)", a0, a1, d0, d1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -444, "RemoveGList_", "(remPtr,gadget,numGad)", a0, a1, d0);
BLITZ_OSCALL(LIB_intuitionlib, -450, "ActivateWindow_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -456, "RefreshWindowFrame_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -462, "ActivateGadget_", "(gadgets,window,requester)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -468, "NewModifyProp_", "(gadget,window,requester,flags,horizPot,vertPot,horizBody,vertBody,numGad)", a0, a1, a2, d0, d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_intuitionlib, -474, "QueryOverscan_", "(displayID,rect,oScanType)", a0, a1, d0);
BLITZ_OSCALL(LIB_intuitionlib, -480, "MoveWindowInFrontOf_", "(window,behindWindow)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -486, "ChangeWindowBox_", "(window,left,top,width,height)", a0, d0, d1, d2, d3);
BLITZ_OSCALL(LIB_intuitionlib, -492, "SetEditHook_", "(hook)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -498, "SetMouseQueue_", "(window,queueLength)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -504, "ZipWindow_", "(window)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -510, "LockPubScreen_", "(name)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -516, "UnlockPubScreen_", "(name,screen)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -522, "LockPubScreenList_", "");
BLITZ_OSCALL(LIB_intuitionlib, -528, "UnlockPubScreenList_", "");
BLITZ_OSCALL(LIB_intuitionlib, -534, "NextPubScreen_", "(screen,namebuf)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -540, "SetDefaultPubScreen_", "(name)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -546, "SetPubScreenModes_", "(modes)", d0);
BLITZ_OSCALL(LIB_intuitionlib, -552, "PubScreenStatus_", "(screen,statusFlags)", a0, d0);
BLITZ_OSCALL(LIB_intuitionlib, -558, "ObtainGIRPort_", "(gInfo)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -564, "ReleaseGIRPort_", "(rp)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -570, "GadgetMouse_", "(gadget,gInfo,mousePoint)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -582, "GetDefaultPubScreen_", "(nameBuffer)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -588, "EasyRequestArgs_", "(window,easyStruct,idcmpPtr,args)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_intuitionlib, -594, "BuildEasyRequestArgs_", "(window,easyStruct,idcmp,args)", a0, a1, d0, a3);
BLITZ_OSCALL(LIB_intuitionlib, -600, "SysReqHandler_", "(window,idcmpPtr,waitInput)", a0, a1, d0);
BLITZ_OSCALL(LIB_intuitionlib, -606, "OpenWindowTagList_", "(newWindow,tagList)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -612, "OpenScreenTagList_", "(newScreen,tagList)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -618, "DrawImageState_", "(rp,image,leftOffset,topOffset,state,drawInfo)", a0, a1, d0, d1, d2, a2);
BLITZ_OSCALL(LIB_intuitionlib, -624, "PointInImage_", "(point,image)", d0, a0);
BLITZ_OSCALL(LIB_intuitionlib, -630, "EraseImage_", "(rp,image,leftOffset,topOffset)", a0, a1, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -636, "NewObjectA_", "(classPtr,classID,tagList)", a0, a1, a2);
BLITZ_OSCALL(LIB_intuitionlib, -642, "DisposeObject_", "(object)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -648, "SetAttrsA_", "(object,tagList)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -654, "GetAttr_", "(attrID,object,storagePtr)", d0, a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -660, "SetGadgetAttrsA_", "(gadget,window,requester,tagList)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_intuitionlib, -666, "NextObject_", "(objectPtrPtr)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -678, "MakeClass_", "(classID,superClassID,superClassPtr,instanceSize,flags)", a0, a1, a2, d0, d1);
BLITZ_OSCALL(LIB_intuitionlib, -684, "AddClass_", "(classPtr)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -690, "GetScreenDrawInfo_", "(screen)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -696, "FreeScreenDrawInfo_", "(screen,drawInfo)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -702, "ResetMenuStrip_", "(window,menu)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -708, "RemoveClass_", "(classPtr)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -714, "FreeClass_", "(classPtr)", a0);
BLITZ_OSCALL(LIB_intuitionlib, -768, "AllocScreenBuffer_", "(sc,bm,flags)", a0, a1, d0);
BLITZ_OSCALL(LIB_intuitionlib, -774, "FreeScreenBuffer_", "(sc,sb)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -780, "ChangeScreenBuffer_", "(sc,sb)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -786, "ScreenDepth_", "(screen,flags,reserved)", a0, d0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -792, "ScreenPosition_", "(screen,flags,x1,y1,x2,y2)", a0, d0, d1, d2, d3, d4);
BLITZ_OSCALL(LIB_intuitionlib, -798, "ScrollWindowRaster_", "(win,dx,dy,xMin,yMin,xMax,yMax)", a1, d0, d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_intuitionlib, -804, "LendMenus_", "(fromwindow,towindow)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -810, "DoGadgetMethodA_", "(gad,win,req,message)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_intuitionlib, -816, "SetWindowPointerA_", "(win,taglist)", a0, a1);
BLITZ_OSCALL(LIB_intuitionlib, -822, "TimedDisplayAlert_", "(alertNumber,string,height,time)", d0, a0, d1, a1);
BLITZ_OSCALL(LIB_intuitionlib, -828, "HelpControl_", "(win,flags)", a0, d0);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_intuitionlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "intuition.library";

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
