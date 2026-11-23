; rianimlib-nextframe-odd-offset-anim7-68000 (#1122) - RIAnimInit's
; xy_offset form adds the byte offset to the plane pointers; for an ANIM7
; frame an odd offset makes the decoder's word or long accesses to the
; planes odd: on a 68000 that RINextAnimFrame is an address error (the
; session ends in a timeout).  The machine is the A600 (68000).  Here
; with runtime error checking on a bitmap 64 pixels wide, an ANIM7 of
; 32 x 16, 1 plane, long data: first xy_offset 2 (even: row 0 gets
; $11223344 at byte 2), then xy_offset 1.
; Correct: the even offset plays; at the odd one RINextAnimFrame's error
; check refuses the odd plane address: "Animation position on an odd
; byte for ANIM7".
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{odd_offset}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,64,16,1
  p=Peek.l(Addr BitMap(0)+8)
  r=RIAnimInit(a,0,0,2)
  !CheckEq{r,-1,"init_2"}
  r=RINextAnimFrame(0)
  v=Peek.l(p+2)
  !CheckEq{r,-1,"frame_2_even"}
  !CheckEq{v,$11223344,"frame_2_row_0"}
  r=RIAnimInit(a,0,0,1)
  !CheckEq{r,-1,"init_1"}
  !ExpectError{"Animation position on an odd byte for ANIM7","refused"}
  r=RINextAnimFrame(0)
  !Check{0,"not_refused"}
  FreeMem_ a,4096
!EndTestCase
