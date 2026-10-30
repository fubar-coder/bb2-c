; rianimlib-nextframe-no-dlta (#515) - RINextAnimFrame saves d1-d7/a0-a6
; and expects ANHD, an optional CMAP, then DLTA; for another chunk it
; returns with MOVEQ #0,d0 : RTS without restoring them: the RTS takes
; the saved d1 as the return address - a crash (the session ends in a
; timeout).  Frame 2 has a DPAN chunk between ANHD and DLTA.
; Correct: RINextAnimFrame returns, the program goes on.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-nextframe-no-dlta.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
NPrint "before"
r=RINextAnimFrame(0)
NPrint "after"
FreeMem_ a,4096
End
