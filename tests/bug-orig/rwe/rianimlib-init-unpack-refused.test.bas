; rianimlib-init-unpack-refused (#1106) - RIAnimInit unpacks the first
; frame with RIPackLib's UnpackIFF but drops its result: it reports
; success for a bitmap UnpackIFF refused (too narrow, too shallow, no
; plane 0), and RINextAnimFrame would apply the deltas to it.  The ANIM
; has 2 planes, the bitmap 1.
; Correct: RIAnimInit returns 0.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{init_unpack_refused}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  FreeMem_ a,4096
  !CheckEq{r,0,"init"}
!EndTestCase
