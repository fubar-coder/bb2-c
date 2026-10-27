/*
 * doslib - deflibs library 252: AmigaOS calls of dos.library
 * (127 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/dos.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("dos.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_doslib,#la1)
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

BLITZ_OSLIBHEADER(LIB_doslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_doslib, -30, "Open_", "(name,accessMode)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -36, "Close_", "(file)", d1);
BLITZ_OSCALL(LIB_doslib, -42, "Read_", "(file,buffer,length)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -48, "Write_", "(file,buffer,length)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -54, "Input_", "");
BLITZ_OSCALL(LIB_doslib, -60, "Output_", "");
BLITZ_OSCALL(LIB_doslib, -66, "Seek_", "(file,position,offset)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -72, "DeleteFile_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -78, "Rename_", "(oldName,newName)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -84, "Lock_", "(name,type)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -90, "UnLock_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -96, "DupLock_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -102, "Examine_", "(lock,fileInfoBlock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -108, "ExNext_", "(lock,fileInfoBlock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -114, "Info_", "(lock,parameterBlock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -120, "CreateDir_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -126, "CurrentDir_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -132, "IoErr_", "");
BLITZ_OSCALL(LIB_doslib, -138, "CreateProc_", "(name,pri,segList,stackSize)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -144, "Exit_", "(returnCode)", d1);
BLITZ_OSCALL(LIB_doslib, -150, "LoadSeg_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -156, "UnLoadSeg_", "(seglist)", d1);
BLITZ_OSCALL(LIB_doslib, -174, "DeviceProc_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -180, "SetComment_", "(name,comment)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -186, "SetProtection_", "(name,protect)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -192, "DateStamp_", "(date)", d1);
BLITZ_OSCALL(LIB_doslib, -198, "Delay_", "(timeout)", d1);
BLITZ_OSCALL(LIB_doslib, -204, "WaitForChar_", "(file,timeout)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -210, "ParentDir_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -216, "IsInteractive_", "(file)", d1);
BLITZ_OSCALL(LIB_doslib, -222, "Execute_", "(string,file,file2)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -228, "AllocDosObject_", "(type,tags)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -234, "FreeDosObject_", "(type,ptr)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -240, "DoPkt_", "(port,action,arg1,arg2,arg3,arg4,arg5)", d1, d2, d3, d4, d5, d6, d7);
BLITZ_OSCALL(LIB_doslib, -246, "SendPkt_", "(dp,port,replyport)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -252, "WaitPkt_", "");
BLITZ_OSCALL(LIB_doslib, -258, "ReplyPkt_", "(dp,res1,res2)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -264, "AbortPkt_", "(port,pkt)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -270, "LockRecord_", "(fh,offset,length,mode,timeout)", d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_doslib, -276, "LockRecords_", "(recArray,timeout)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -282, "UnLockRecord_", "(fh,offset,length)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -288, "UnLockRecords_", "(recArray)", d1);
BLITZ_OSCALL(LIB_doslib, -294, "SelectInput_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -300, "SelectOutput_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -306, "FGetC_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -312, "FPutC_", "(fh,ch)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -318, "UnGetC_", "(fh,character)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -324, "FRead_", "(fh,block,blocklen,number)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -330, "FWrite_", "(fh,block,blocklen,number)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -336, "FGets_", "(fh,buf,buflen)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -342, "FPuts_", "(fh,str)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -348, "VFWritef_", "(fh,format,argarray)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -354, "VFPrintf_", "(fh,format,argarray)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -360, "Flush_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -366, "SetVBuf_", "(fh,buff,type,size)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -372, "DupLockFromFH_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -378, "OpenFromLock_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -384, "ParentOfFH_", "(fh)", d1);
BLITZ_OSCALL(LIB_doslib, -390, "ExamineFH_", "(fh,fib)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -396, "SetFileDate_", "(name,date)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -402, "NameFromLock_", "(lock,buffer,len)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -408, "NameFromFH_", "(fh,buffer,len)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -414, "SplitName_", "(name,seperator,buf,oldpos,size)", d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_doslib, -420, "SameLock_", "(lock1,lock2)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -426, "SetMode_", "(fh,mode)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -432, "ExAll_", "(lock,buffer,size,data,control)", d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_doslib, -438, "ReadLink_", "(port,lock,path,buffer,size)", d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_doslib, -444, "MakeLink_", "(name,dest,soft)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -450, "ChangeMode_", "(type,fh,newmode)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -456, "SetFileSize_", "(fh,pos,mode)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -462, "SetIoErr_", "(result)", d1);
BLITZ_OSCALL(LIB_doslib, -468, "Fault_", "(code,header,buffer,len)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -474, "PrintFault_", "(code,header)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -480, "ErrorReport_", "(code,type,arg1,device)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -492, "Cli_", "");
BLITZ_OSCALL(LIB_doslib, -498, "CreateNewProc_", "(tags)", d1);
BLITZ_OSCALL(LIB_doslib, -504, "RunCommand_", "(seg,stack,paramptr,paramlen)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -510, "GetConsoleTask_", "");
BLITZ_OSCALL(LIB_doslib, -516, "SetConsoleTask_", "(task)", d1);
BLITZ_OSCALL(LIB_doslib, -522, "GetFileSysTask_", "");
BLITZ_OSCALL(LIB_doslib, -528, "SetFileSysTask_", "(task)", d1);
BLITZ_OSCALL(LIB_doslib, -534, "GetArgStr_", "");
BLITZ_OSCALL(LIB_doslib, -540, "SetArgStr_", "(string)", d1);
BLITZ_OSCALL(LIB_doslib, -546, "FindCliProc_", "(num)", d1);
BLITZ_OSCALL(LIB_doslib, -552, "MaxCli_", "");
BLITZ_OSCALL(LIB_doslib, -558, "SetCurrentDirName_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -564, "GetCurrentDirName_", "(buf,len)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -570, "SetProgramName_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -576, "GetProgramName_", "(buf,len)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -582, "SetPrompt_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -588, "GetPrompt_", "(buf,len)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -594, "SetProgramDir_", "(lock)", d1);
BLITZ_OSCALL(LIB_doslib, -600, "GetProgramDir_", "");
BLITZ_OSCALL(LIB_doslib, -606, "SystemTagList_", "(command,tags)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -612, "AssignLock_", "(name,lock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -618, "AssignLate_", "(name,path)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -624, "AssignPath_", "(name,path)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -630, "AssignAdd_", "(name,lock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -636, "RemAssignList_", "(name,lock)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -642, "GetDeviceProc_", "(name,dp)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -648, "FreeDeviceProc_", "(dp)", d1);
BLITZ_OSCALL(LIB_doslib, -654, "LockDosList_", "(flags)", d1);
BLITZ_OSCALL(LIB_doslib, -660, "UnLockDosList_", "(flags)", d1);
BLITZ_OSCALL(LIB_doslib, -666, "AttemptLockDosList_", "(flags)", d1);
BLITZ_OSCALL(LIB_doslib, -672, "RemDosEntry_", "(dlist)", d1);
BLITZ_OSCALL(LIB_doslib, -678, "AddDosEntry_", "(dlist)", d1);
BLITZ_OSCALL(LIB_doslib, -684, "FindDosEntry_", "(dlist,name,flags)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -690, "NextDosEntry_", "(dlist,flags)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -696, "MakeDosEntry_", "(name,type)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -702, "FreeDosEntry_", "(dlist)", d1);
BLITZ_OSCALL(LIB_doslib, -708, "IsFileSystem_", "(name)", d1);
BLITZ_OSCALL(LIB_doslib, -714, "Format_", "(filesystem,volumename,dostype)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -720, "Relabel_", "(drive,newname)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -726, "Inhibit_", "(name,onoff)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -732, "AddBuffers_", "(name,number)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -738, "CompareDates_", "(date1,date2)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -744, "DateToStr_", "(datetime)", d1);
BLITZ_OSCALL(LIB_doslib, -750, "StrToDate_", "(datetime)", d1);
BLITZ_OSCALL(LIB_doslib, -756, "InternalLoadSeg_", "(fh,table,funcarray,stack)", d0, a0, a1, a2);
BLITZ_OSCALL(LIB_doslib, -762, "InternalUnLoadSeg_", "(seglist,freefunc)", d1, a1);
BLITZ_OSCALL(LIB_doslib, -768, "NewLoadSeg_", "(file,tags)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -774, "AddSegment_", "(name,seg,system)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -780, "FindSegment_", "(name,seg,system)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -786, "RemSegment_", "(seg)", d1);
BLITZ_OSCALL(LIB_doslib, -792, "CheckSignal_", "(mask)", d1);
BLITZ_OSCALL(LIB_doslib, -798, "ReadArgs_", "(template,array,args)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -804, "FindArg_", "(keyword,template)", d1, d2);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_doslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "dos.library";

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
