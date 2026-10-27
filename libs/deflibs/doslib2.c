/*
 * doslib2 - deflibs library 249: more AmigaOS calls
 * of library 252 (doslib)
 * (27 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/dos.library2);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   no init/finit: the calls use the base of 252
 *   (LIB_doslib), this library only adds declarations
 *
 * Every call: BLITZ_OSCALL(base lib, LVO, name, help, argument
 * registers); the compiler generates the call (blitz2.s stamigalib).
 */
#include "blitzlib.h"

BLITZ_OSLIBHEADER(LIB_doslib2, 0, BL_NORET, 0);

BLITZ_OSCALL(LIB_doslib, -810, "ReadItem_", "(name,maxchars,cSource)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -816, "StrToLong_", "(string,value)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -822, "MatchFirst_", "(pat,anchor)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -828, "MatchNext_", "(anchor)", d1);
BLITZ_OSCALL(LIB_doslib, -834, "MatchEnd_", "(anchor)", d1);
BLITZ_OSCALL(LIB_doslib, -840, "ParsePattern_", "(pat,buf,buflen)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -846, "MatchPattern_", "(pat,str)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -858, "FreeArgs_", "(args)", d1);
BLITZ_OSCALL(LIB_doslib, -870, "FilePart_", "(path)", d1);
BLITZ_OSCALL(LIB_doslib, -876, "PathPart_", "(path)", d1);
BLITZ_OSCALL(LIB_doslib, -882, "AddPart_", "(dirname,filename,size)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -888, "StartNotify_", "(notify)", d1);
BLITZ_OSCALL(LIB_doslib, -894, "EndNotify_", "(notify)", d1);
BLITZ_OSCALL(LIB_doslib, -900, "SetVar_", "(name,buffer,size,flags)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -906, "GetVar_", "(name,buffer,size,flags)", d1, d2, d3, d4);
BLITZ_OSCALL(LIB_doslib, -912, "DeleteVar_", "(name,flags)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -918, "FindVar_", "(name,type)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -930, "CliInitNewcli_", "(dp)", a0);
BLITZ_OSCALL(LIB_doslib, -936, "CliInitRun_", "(dp)", a0);
BLITZ_OSCALL(LIB_doslib, -942, "WriteChars_", "(buf,buflen)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -948, "PutStr_", "(str)", d1);
BLITZ_OSCALL(LIB_doslib, -954, "VPrintf_", "(format,argarray)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -966, "ParsePatternNoCase_", "(pat,buf,buflen)", d1, d2, d3);
BLITZ_OSCALL(LIB_doslib, -972, "MatchPatternNoCase_", "(pat,str)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -984, "SameDevice_", "(lock1,lock2)", d1, d2);
BLITZ_OSCALL(LIB_doslib, -990, "ExAllEnd_", "(lock,buffer,size,data,control)", d1, d2, d3, d4, d5);
BLITZ_OSCALL(LIB_doslib, -996, "SetOwner_", "(name,owner_info)", d1, d2);

BLITZ_LIBFIN();
