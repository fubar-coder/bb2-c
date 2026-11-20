; riwbspritelib-getwbsprite-fail-returns-channel (#920) - GetWBSprite does
; not look at whether this call got a sprite: at its end it returns the
; channel whenever the library's table holds a sprite on it.  A second
; GetWBSprite on channel 2, which the library already holds, fails in
; GetExtSpriteA but returns 2 as if it had got one.  A shape narrower
; than 16 pixels gives 0, which cannot be told from channel 0.
; Correct: the first call gets channel 2; the second call and the narrow
; shape fail with -1.
; @rwe riwbspritelib
BitMap 0,320,64,2
Boxf 0,0,15,15,3
GetaShape 0,0,0,16,16
GetaShape 1,0,0,8,8
s.l=GetWBSprite(10,20,0,2)
NPrint s
t.l=GetWBSprite(50,60,0,2)
NPrint t
u.l=GetWBSprite(10,20,1,5)
NPrint u
End
