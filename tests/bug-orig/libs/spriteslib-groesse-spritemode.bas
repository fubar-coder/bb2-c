; spriteslib-groesse-spritemode (#280) - freeasprite, SaveSprites and
; LoadSprites compute the size of a sprite's data with the CURRENT
; widthshift (SpriteMode), not with the one the sprite was made with.
; Here: a 16x8 4 colour sprite made in SpriteMode 0 (9 rows of 4 bytes =
; 36 bytes of data), saved after SpriteMode 1: SaveSprites writes 9 rows
; of 8 bytes (72 bytes, the second half from behind the data).
; Correct: the file holds the 16 byte block and the 36 bytes of the
; sprite (52 bytes), whatever the SpriteMode at the time of saving.
; @configs clib
DEFTYPE .l
BitMap 0,16,8,2
Use BitMap 0
GetaShape 0,0,0,16,8
SpriteMode 0
GetaSprite 0,0
NPrint "bytes per image ",Peek.w(Addr Sprite(0)+10)
SpriteMode 1
SaveSprites 0,0,"RAM:bo280.spr"
SpriteMode 0
If ReadFile(0,"RAM:bo280.spr")
  NPrint "file ",Lof(0)
  CloseFile 0
EndIf
KillFile "RAM:bo280.spr"
End
