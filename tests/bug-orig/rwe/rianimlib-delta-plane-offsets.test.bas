; rianimlib-delta-plane-offsets (#1120) - RINextAnimFrame takes the 16
; plane offsets at the start of a DLTA (the op lists, and for ANIM7 the
; data lists) without comparing them with the chunk's length.  32 x 16,
; 1 plane: frame 2's ANIM5 op list offset and frame 3's ANIM7 data list
; offset point behind their DLTA, at the ops of frame 4: frame 2 plays
; frame 4's ops (row 3 gets $AA), frame 3 writes those ops as its long
; (row 0 gets $02030001).
; Correct: frames 2 and 3 are refused (RINextAnimFrame returns 0, rows 0
; and 3 stay 0); frame 4 plays (row 3 gets $AA).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{plane_offsets}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,32,16,1
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r2=RINextAnimFrame(0)
  v2=Peek.b(p+12) AND $FF
  !CheckEq{r2,0,"frame_2_result"}
  !CheckEq{v2,0,"frame_2_row_3"}
  r3=RINextAnimFrame(0)
  v3=Peek.l(p)
  !CheckEq{r3,0,"frame_3_result"}
  !CheckEq{v3,0,"frame_3_row_0"}
  r4=RINextAnimFrame(0)
  v4=Peek.b(p+12) AND $FF
  !CheckEq{r4,-1,"frame_4_result"}
  !CheckEq{v4,$AA,"frame_4_row_3"}
  FreeMem_ a,4096
!EndTestCase
