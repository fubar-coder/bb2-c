; rianimlib-framecount-all-chunks (#510) - RIAnimFrameCount walks the
; chunks of the FORM ANIM and counts every one of them, not only the
; frames.  The ANIM has three frames and an ANNO chunk between the first
; and the second.
; Correct: RIAnimFrameCount returns 3.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{framecount_all_chunks}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  c.w=RIAnimFrameCount
  FreeMem_ a,4096
  !CheckEq{c,3,"count"}
!EndTestCase
