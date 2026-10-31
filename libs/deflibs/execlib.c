/*
 * execlib - deflibs library 253: AmigaOS calls of exec.library
 * (117 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/exec.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("exec.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6), 0 if the
 *          library cannot be opened.  Fixed (intended deviation, #44):
 *          the original retries forever (deflib.h)
 *   finit: CloseLibrary(base) if base != 0, a1 = base (!libs LIB_execlib,#la1)
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

BLITZ_OSLIBHEADER(LIB_execlib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_execlib, -30, "Supervisor_", "(userFunction)", a5);
BLITZ_OSCALL(LIB_execlib, -72, "InitCode_", "(startClass,version)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -78, "InitStruct_", "(initTable,memory,size)", a1, a2, d0);
BLITZ_OSCALL(LIB_execlib, -84, "MakeLibrary_", "(funcInit,structInit,libInit,dataSize,segList)", a0, a1, a2, d0, d1);
BLITZ_OSCALL(LIB_execlib, -90, "MakeFunctions_", "(target,functionArray,funcDispBase)", a0, a1, a2);
BLITZ_OSCALL(LIB_execlib, -96, "FindResident_", "(name)", a1);
BLITZ_OSCALL(LIB_execlib, -102, "InitResident_", "(resident,segList)", a1, d1);
BLITZ_OSCALL(LIB_execlib, -108, "Alert_", "(alertNum)", d7);
BLITZ_OSCALL(LIB_execlib, -114, "Debug_", "(flags)", d0);
BLITZ_OSCALL(LIB_execlib, -120, "Disable_", "");
BLITZ_OSCALL(LIB_execlib, -126, "Enable_", "");
BLITZ_OSCALL(LIB_execlib, -132, "Forbid_", "");
BLITZ_OSCALL(LIB_execlib, -138, "Permit_", "");
BLITZ_OSCALL(LIB_execlib, -144, "SetSR_", "(newSR,mask)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -150, "SuperState_", "");
BLITZ_OSCALL(LIB_execlib, -156, "UserState_", "(sysStack)", d0);
BLITZ_OSCALL(LIB_execlib, -162, "SetIntVector_", "(intNumber,interrupt)", d0, a1);
BLITZ_OSCALL(LIB_execlib, -168, "AddIntServer_", "(intNumber,interrupt)", d0, a1);
BLITZ_OSCALL(LIB_execlib, -174, "RemIntServer_", "(intNumber,interrupt)", d0, a1);
BLITZ_OSCALL(LIB_execlib, -180, "Cause_", "(interrupt)", a1);
BLITZ_OSCALL(LIB_execlib, -186, "Allocate_", "(freeList,byteSize)", a0, d0);
BLITZ_OSCALL(LIB_execlib, -192, "Deallocate_", "(freeList,memoryBlock,byteSize)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -198, "AllocMem_", "(byteSize,requirements)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -204, "AllocAbs_", "(byteSize,location)", d0, a1);
BLITZ_OSCALL(LIB_execlib, -210, "FreeMem_", "(memoryBlock,byteSize)", a1, d0);
BLITZ_OSCALL(LIB_execlib, -216, "AvailMem_", "(requirements)", d1);
BLITZ_OSCALL(LIB_execlib, -222, "AllocEntry_", "(entry)", a0);
BLITZ_OSCALL(LIB_execlib, -228, "FreeEntry_", "(entry)", a0);
BLITZ_OSCALL(LIB_execlib, -234, "Insert_", "(list,node,pred)", a0, a1, a2);
BLITZ_OSCALL(LIB_execlib, -240, "AddHead_", "(list,node)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -246, "AddTail_", "(list,node)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -252, "Remove_", "(node)", a1);
BLITZ_OSCALL(LIB_execlib, -258, "RemHead_", "(list)", a0);
BLITZ_OSCALL(LIB_execlib, -264, "RemTail_", "(list)", a0);
BLITZ_OSCALL(LIB_execlib, -270, "Enqueue_", "(list,node)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -276, "FindName_", "(list,name)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -282, "AddTask_", "(task,initPC,finalPC)", a1, a2, a3);
BLITZ_OSCALL(LIB_execlib, -288, "RemTask_", "(task)", a1);
BLITZ_OSCALL(LIB_execlib, -294, "FindTask_", "(name)", a1);
BLITZ_OSCALL(LIB_execlib, -300, "SetTaskPri_", "(task,priority)", a1, d0);
BLITZ_OSCALL(LIB_execlib, -306, "SetSignal_", "(newSignals,signalSet)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -312, "SetExcept_", "(newSignals,signalSet)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -318, "Wait_", "(signalSet)", d0);
BLITZ_OSCALL(LIB_execlib, -324, "Signal_", "(task,signalSet)", a1, d0);
BLITZ_OSCALL(LIB_execlib, -330, "AllocSignal_", "(signalNum)", d0);
BLITZ_OSCALL(LIB_execlib, -336, "FreeSignal_", "(signalNum)", d0);
BLITZ_OSCALL(LIB_execlib, -342, "AllocTrap_", "(trapNum)", d0);
BLITZ_OSCALL(LIB_execlib, -348, "FreeTrap_", "(trapNum)", d0);
BLITZ_OSCALL(LIB_execlib, -354, "AddPort_", "(port)", a1);
BLITZ_OSCALL(LIB_execlib, -360, "RemPort_", "(port)", a1);
BLITZ_OSCALL(LIB_execlib, -366, "PutMsg_", "(port,message)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -372, "GetMsg_", "(port)", a0);
BLITZ_OSCALL(LIB_execlib, -378, "ReplyMsg_", "(message)", a1);
BLITZ_OSCALL(LIB_execlib, -384, "WaitPort_", "(port)", a0);
BLITZ_OSCALL(LIB_execlib, -390, "FindPort_", "(name)", a1);
BLITZ_OSCALL(LIB_execlib, -396, "AddLibrary_", "(library)", a1);
BLITZ_OSCALL(LIB_execlib, -402, "RemLibrary_", "(library)", a1);
BLITZ_OSCALL(LIB_execlib, -408, "OldOpenLibrary_", "(libName)", a1);
BLITZ_OSCALL(LIB_execlib, -414, "CloseLibrary_", "(library)", a1);
BLITZ_OSCALL(LIB_execlib, -420, "SetFunction_", "(library,funcOffset,newFunction)", a1, a0, d0);
BLITZ_OSCALL(LIB_execlib, -426, "SumLibrary_", "(library)", a1);
BLITZ_OSCALL(LIB_execlib, -432, "AddDevice_", "(device)", a1);
BLITZ_OSCALL(LIB_execlib, -438, "RemDevice_", "(device)", a1);
BLITZ_OSCALL(LIB_execlib, -444, "OpenDevice_", "(devName,unit,ioRequest,flags)", a0, d0, a1, d1);
BLITZ_OSCALL(LIB_execlib, -450, "CloseDevice_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -456, "DoIO_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -462, "SendIO_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -468, "CheckIO_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -474, "WaitIO_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -480, "AbortIO_", "(ioRequest)", a1);
BLITZ_OSCALL(LIB_execlib, -486, "AddResource_", "(resource)", a1);
BLITZ_OSCALL(LIB_execlib, -492, "RemResource_", "(resource)", a1);
BLITZ_OSCALL(LIB_execlib, -498, "OpenResource_", "(resName)", a1);
BLITZ_OSCALL(LIB_execlib, -522, "RawDoFmt_", "(formatString,dataStream,putChProc,putChData)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_execlib, -528, "GetCC_", "");
BLITZ_OSCALL(LIB_execlib, -534, "TypeOfMem_", "(address)", a1);
BLITZ_OSCALL(LIB_execlib, -540, "Procure_", "(sigSem,bidMsg)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -546, "Vacate_", "(sigSem,bidMsg)", a0, a1);
BLITZ_OSCALL(LIB_execlib, -552, "OpenLibrary_", "(libName,version)", a1, d0);
BLITZ_OSCALL(LIB_execlib, -558, "InitSemaphore_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -564, "ObtainSemaphore_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -570, "ReleaseSemaphore_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -576, "AttemptSemaphore_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -582, "ObtainSemaphoreList_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -588, "ReleaseSemaphoreList_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -594, "FindSemaphore_", "(sigSem)", a1);
BLITZ_OSCALL(LIB_execlib, -600, "AddSemaphore_", "(sigSem)", a1);
BLITZ_OSCALL(LIB_execlib, -606, "RemSemaphore_", "(sigSem)", a1);
BLITZ_OSCALL(LIB_execlib, -612, "SumKickData_", "");
BLITZ_OSCALL(LIB_execlib, -618, "AddMemList_", "(size,attributes,pri,base,name)", d0, d1, d2, a0, a1);
BLITZ_OSCALL(LIB_execlib, -624, "CopyMem_", "(source,dest,size)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -630, "CopyMemQuick_", "(source,dest,size)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -636, "CacheClearU_", "");
BLITZ_OSCALL(LIB_execlib, -642, "CacheClearE_", "(address,length,caches)", a0, d0, d1);
BLITZ_OSCALL(LIB_execlib, -648, "CacheControl_", "(cacheBits,cacheMask)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -654, "CreateIORequest_", "(port,size)", a0, d0);
BLITZ_OSCALL(LIB_execlib, -660, "DeleteIORequest_", "(iorequest)", a0);
BLITZ_OSCALL(LIB_execlib, -666, "CreateMsgPort_", "");
BLITZ_OSCALL(LIB_execlib, -672, "DeleteMsgPort_", "(port)", a0);
BLITZ_OSCALL(LIB_execlib, -678, "ObtainSemaphoreShared_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -684, "AllocVec_", "(byteSize,requirements)", d0, d1);
BLITZ_OSCALL(LIB_execlib, -690, "FreeVec_", "(memoryBlock)", a1);
BLITZ_OSCALL(LIB_execlib, -696, "CreatePool_", "(requirements,puddleSize,threshSize)", d0, d1, d2);
BLITZ_OSCALL(LIB_execlib, -702, "DeletePool_", "(poolHeader)", a0);
BLITZ_OSCALL(LIB_execlib, -708, "AllocPooled_", "(poolHeader,memSize)", a0, d0);
BLITZ_OSCALL(LIB_execlib, -714, "FreePooled_", "(poolHeader,memory,memSize)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -720, "AttemptSemaphoreShared_", "(sigSem)", a0);
BLITZ_OSCALL(LIB_execlib, -726, "ColdReboot_", "");
BLITZ_OSCALL(LIB_execlib, -732, "StackSwap_", "(newStack)", a0);
BLITZ_OSCALL(LIB_execlib, -738, "ChildFree_", "(tid)", d0);
BLITZ_OSCALL(LIB_execlib, -744, "ChildOrphan_", "(tid)", d0);
BLITZ_OSCALL(LIB_execlib, -750, "ChildStatus_", "(tid)", d0);
BLITZ_OSCALL(LIB_execlib, -756, "ChildWait_", "(tid)", d0);
/* Fixed (intended deviation, #959): flags in d0 as exec takes it
   (exec_lib.fd); the original declares d1 */
BLITZ_OSCALL(LIB_execlib, -762, "CachePreDMA_", "(address,length,flags)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -768, "CachePostDMA_", "(address,length,flags)", a0, a1, d0);
BLITZ_OSCALL(LIB_execlib, -774, "AddMemHandler_", "(memhand)", a1);
BLITZ_OSCALL(LIB_execlib, -780, "RemMemHandler_", "(memhand)", a1);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_execlib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "exec.library";

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
