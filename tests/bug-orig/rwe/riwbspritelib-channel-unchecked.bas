; riwbspritelib-channel-unchecked (#916) - the sprite table (8 entries) is
; indexed with the channel without a check; entry 8 is the first long of
; the library name "graphics.library" behind it.  GetWBSprite with
; channel 8: GetExtSpriteA fails (there is no sprite 8), but the result
; check finds "grap" in entry 8 and returns 8, as if it had got a sprite.
; Correct: no sprite was allocated, the result is 0.
; @rwe riwbspritelib
BitMap 0,320,64,2
Boxf 0,0,15,15,3
GetaShape 0,0,0,16,16
s.l=GetWBSprite(10,10,0,8)
NPrint s
End
