; riwbspritelib-getwbsprite-fail-returns-channel (#920) - GetWBSprite does
; not look at whether this call got a sprite: at its end it returns the
; channel whenever the library's table holds a sprite on it.  A second
; GetWBSprite on channel 2, which the library already holds, fails in
; GetExtSpriteA but returns 2 as if it had got one.  A shape narrower
; than 16 pixels gives 0, which cannot be told from channel 0.
; Correct: the first call gets channel 2; the second call and the narrow
; shape fail with -1.
DEFTYPE .l

!TestCase{failed_calls}
  BitMap 0,320,64,2
  Boxf 0,0,15,15,3
  GetaShape 0,0,0,16,16
  GetaShape 1,0,0,8,8
  s=GetWBSprite(10,20,0,2)
  t=GetWBSprite(50,60,0,2)
  u=GetWBSprite(10,20,1,5)
  !CheckEq{s,2,"first_call"}
  !CheckEq{t,-1,"second_call"}
  !CheckEq{u,-1,"narrow_shape"}
!EndTestCase
