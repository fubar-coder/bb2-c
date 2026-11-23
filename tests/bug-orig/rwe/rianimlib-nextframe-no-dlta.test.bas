; rianimlib-nextframe-no-dlta (#515) - RINextAnimFrame saves d1-d7/a0-a6
; and expects ANHD, an optional CMAP, then DLTA; for another chunk it
; returns with MOVEQ #0,d0 : RTS without restoring them: the RTS takes
; the saved d1 as the return address - a crash (the session ends in a
; timeout).  Frame 2 has a DPAN chunk between ANHD and DLTA.
; Correct: RINextAnimFrame returns, the program goes on.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{no_dlta}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  r=RINextAnimFrame(0)
  !Check{1,"returned"}
  FreeMem_ a,4096
!EndTestCase
