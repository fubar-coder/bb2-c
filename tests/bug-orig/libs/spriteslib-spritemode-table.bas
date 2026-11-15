; spriteslib-spritemode-table (#281) - SpriteMode takes the sprite width
; from a table of 3 words (shlup: 2, 4, 8) and sets the width shift to
; mode + 2 without limiting the mode: SpriteMode 3 reads _spritemode
; behind the table as the width (3 bytes) and sets the shift 5.
; Without the debugger: GetaSprite of a 16x8 4 colour shape after
; SpriteMode 3 makes a sprite with 9 rows of 32 bytes (bytes per image
; +10 = 288).
; An invalid argument: the error check catches it (BB2 style); the
; command itself is unchanged. Runtime error checking on (the debugger).
; Correct: the error check stops SpriteMode 3 with "Illegal SpriteMode".
; The original has no check and makes rows of 32 bytes.
; @fixed #281
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,16,8,2
Use BitMap 0
GetaShape 0,0,0,16,8
SpriteMode 3
GetaSprite 0,0
SpriteMode 0
a=Addr Sprite(0)
r=Peek.w(a+10)/Peek.w(a+4)
If r=4 OR r=8 OR r=16
  NPrint "row bytes valid"
Else
  NPrint "row bytes ",r
EndIf
End
