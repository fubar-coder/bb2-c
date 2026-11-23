; rianimlib-init-xy-offset (#514) - RIAnimInit's x,y form turns the
; position into a byte offset into the planes (BytesPerRow * y with
; MULU.w, + x >> 3) without looking at the bitmap: a negative y or one of
; 65536 and above gives an offset of up to 65535 rows, and UnpackIFF and
; every RINextAnimFrame write there.  The original has no error check
; area.  Here with runtime error checking and x = 32 on a bitmap 32
; pixels wide (the animation is one row high, the offset of 4 bytes stays
; inside the plane in the original).
; Correct: the error check refuses the position: "Animation position
; outside the bitmap".
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{init_xy_offset}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,32,16,1
  !ExpectError{"Animation position outside the bitmap","refused"}
  r=RIAnimInit(a,0,0,32,0)
  !Check{0,"not_refused"}
  FreeMem_ a,4096
!EndTestCase
