; rianimlib-anim7-odd-data-offset-68000 (#1121) - the ANIM7 decoder of
; RINextAnimFrame reads the data list of a plane (longs, or words in the
; short form) at the offset the DLTA gives; an odd offset makes every
; read odd: on a 68000 that is an address error (the session ends in a
; timeout).  The machine is the A600 (68000).  32 x 16, 1 plane, ANIM7
; long data: frame 2's data list of plane 0 is at the odd offset 67,
; frame 3's at 68 (row 0 gets $11223344).
; Correct: frame 2 is refused (RINextAnimFrame returns 0, row 0 stays 0);
; frame 3 plays (row 0 gets $11223344).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{odd_data_offset}
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
  h2=Peek.l(p)
  !CheckEq{r2,0,"frame_2_result"}
  !CheckEq{h2,0,"frame_2_row_0"}
  r3=RINextAnimFrame(0)
  h3=Peek.l(p)
  !CheckEq{r3,-1,"frame_3_result"}
  !CheckEq{h3,$11223344,"frame_3_row_0"}
  FreeMem_ a,4096
!EndTestCase
