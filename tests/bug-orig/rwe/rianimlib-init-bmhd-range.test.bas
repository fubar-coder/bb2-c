; rianimlib-init-bmhd-range (#511) - RIAnimInit searches the BMHD from the
; first frame's chunks up to the end of the ANIM, not of the first frame:
; a first frame without BMHD is accepted if a BMHD chunk stands in the
; ANIM behind it, and its size is taken for the animation, while
; RIAnimDepth (which searches the frame only) finds none.
; Correct: RIAnimInit fails (0), as RIAnimDepth (0).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{init_bmhd_range}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  d.w=RIAnimDepth(a)
  FreeMem_ a,4096
  !CheckEq{r,0,"init"}
  !CheckEq{d,0,"depth"}
!EndTestCase
