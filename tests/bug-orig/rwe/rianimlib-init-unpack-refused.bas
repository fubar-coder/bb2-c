; rianimlib-init-unpack-refused (#1106) - RIAnimInit unpacks the first
; frame with RIPackLib's UnpackIFF but drops its result: it reports
; success for a bitmap UnpackIFF refused (too narrow, too shallow, no
; plane 0), and RINextAnimFrame would apply the deltas to it.  The ANIM
; has 2 planes, the bitmap 1.
; Correct: RIAnimInit returns 0.
; @fixed #1106
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-init-unpack-refused.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
NPrint r
FreeMem_ a,4096
End
