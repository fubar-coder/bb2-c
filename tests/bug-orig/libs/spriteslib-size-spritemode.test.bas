; spriteslib-size-spritemode (#280) - freeasprite, SaveSprites and
; LoadSprites compute the size of a sprite's data with the CURRENT
; widthshift (SpriteMode), not with the one the sprite was made with.
; Here: a 16x8 4 colour sprite made in SpriteMode 0 (9 rows of 4 bytes =
; 36 bytes of data), saved after SpriteMode 1: SaveSprites writes 9 rows
; of 8 bytes (72 bytes, the second half from behind the data).
; Correct: the file holds the 16 byte block and the 36 bytes of the
; sprite (52 bytes), whatever the SpriteMode at the time of saving.
DEFTYPE .l

!TestCase{size_in_other_mode}
  BitMap 0,16,8,2
  Use BitMap 0
  GetaShape 0,0,0,16,8
  SpriteMode 0
  GetaSprite 0,0
  !CheckEq{Peek.w(Addr Sprite(0)+10),36,"bytes_per_image"}
  SpriteMode 1
  SaveSprites 0,0,"CORPUS:bo280.spr"
  SpriteMode 0
  ok=ReadFile(0,"CORPUS:bo280.spr")
  !Require{ok,"sprites_file_read"}
  !CheckEq{Lof(0),52,"file_size"}
  CloseFile 0
  KillFile "CORPUS:bo280.spr"
!EndTestCase
