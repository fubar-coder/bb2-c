; rianimlib-nextframe-anhd-length (#1117) - RINextAnimFrame steps over
; the ANHD (and a CMAP) of a frame by its length without comparing the
; step with the frame's end.  The ANHD of frame 2 (16 x 16, 1 plane)
; claims a length that ends at the DLTA of frame 3, behind frame 2: the
; first RINextAnimFrame plays frame 3's delta (row 1 of column 0 gets
; $0F) instead of frame 2's (row 0 gets $F0).
; Correct: the step leaves frame 2, so the frame is not played
; (RINextAnimFrame returns 0, rows 0 and 1 stay 0).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{anhd_length}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  v=Peek.w(p) AND $FFFF
  w=Peek.w(p+2) AND $FFFF
  FreeMem_ a,4096
  !CheckEq{r,0,"result"}
  !CheckEq{w,0,"frame_3_delta"}
  !CheckEq{v,0,"frame_2_delta"}
!EndTestCase
