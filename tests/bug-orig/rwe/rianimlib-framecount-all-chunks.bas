; rianimlib-framecount-all-chunks (#510) - RIAnimFrameCount walks the
; chunks of the FORM ANIM and counts every one of them, not only the
; frames.  The ANIM has three frames and an ANNO chunk between the first
; and the second.
; Correct: RIAnimFrameCount returns 3.
; @fixed #510
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-framecount-all-chunks.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
c.w=RIAnimFrameCount
NPrint c
FreeMem_ a,4096
End
