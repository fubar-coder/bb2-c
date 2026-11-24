; rianimlib-nextframe-no-dlta (#515) - RINextAnimFrame saves d1-d7/a0-a6
; and expects ANHD, an optional CMAP, then DLTA; for another chunk it
; returns with MOVEQ #0,d0 : RTS without restoring them: the RTS takes
; the saved d1 as the return address.  Frame 2 has a DPAN chunk between
; ANHD and DLTA.
; To give the wrong return a defined end the program loads d1 with the
; address of rec before the call: the original returns to rec, which
; pops the registers the routine saved behind the d1 the RTS took
; (d2-d7/a0-a6), drops the real return address and sets w=1.  This
; relies on the routine's register save
; layout and on d1 being untouched up to that branch: details of the
; library code, not of the bug.
; Correct: RINextAnimFrame returns, the program goes on (w stays 0).
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
  w=0
  LEA rec(pc),a0 : MOVE.l a0,d1
  r=RINextAnimFrame(0)
  Goto ok
rec:
  MOVEM.l (a7)+,d2-d7/a0-a6 : ADDQ.l #4,a7
  w=1
ok:
  !CheckEq{w,0,"return_address"}
  !Check{1,"returned"}
  FreeMem_ a,4096
!EndTestCase
