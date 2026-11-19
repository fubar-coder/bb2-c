; rianimlib-init-bmhd-range (#511) - RIAnimInit searches the BMHD from the
; first frame's chunks up to the end of the ANIM, not of the first frame:
; a first frame without BMHD is accepted if a BMHD chunk stands in the
; ANIM behind it, and its size is taken for the animation, while
; RIAnimDepth (which searches the frame only) finds none.
; Correct: RIAnimInit fails (0), as RIAnimDepth (0).
; @fixed #511
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-init-bmhd-range.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
NPrint r
d.w=RIAnimDepth(a)
NPrint d
FreeMem_ a,4096
End
