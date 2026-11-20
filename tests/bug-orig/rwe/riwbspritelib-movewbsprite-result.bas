; riwbspritelib-movewbsprite-result (#921) - MoveWBSprite is declared with
; a byte result; without a sprite on the channel it returns 0, with one it
; ends with MoveSprite, a void function, and returns the d0 graphics.library
; leaves there.  The Workbench screen's ViewPort (FindScreen 0) for both
; calls; a sprite on channel 2, none on channel 6.
; Correct: -1 when the sprite was moved, 0 without a sprite.
; @fixed #921
; @rwe riwbspritelib
BitMap 0,320,64,2
Boxf 0,0,15,15,3
GetaShape 0,0,0,16,16
FindScreen 0
vp.l=ViewPort(0)
s.l=GetWBSprite(10,20,0,2)
NPrint s
r.w=MoveWBSprite(vp,100,50,2)
NPrint r
r=MoveWBSprite(vp,100,50,6)
NPrint r
End
