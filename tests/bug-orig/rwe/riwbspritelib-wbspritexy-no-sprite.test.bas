; riwbspritelib-wbspritexy-no-sprite (#922) - WBSpriteX and WBSpriteY
; load the channel's ExtSprite pointer from the library's table and
; return the word at offset 6 or 8 of it, without testing the pointer:
; for a channel without a sprite of the library they return the words at
; the addresses 6 and 8.  A query of a sprite that does not exist is a
; programming error; the original has no error check area.  Here with
; runtime error checking.
; Correct: the error check refuses WBSpriteX(5) without a sprite on
; channel 5: "No sprite on this channel".
DEFTYPE .l

!TestCase{no_sprite}
  !ExpectError{"No sprite on this channel","refused"}
  x.w=WBSpriteX(5)
  !CheckEq{x,0,"no_value"}
!EndTestCase
