; displaylib-customsprites-ccoffset (#324) - CustomSprites is declared
; with the !libs of DisplaySprite ({#spriteslib,$1181,...}): the compiler
; loads a sprite object for the second argument, CCOffset, and shifts it
; by the object shift (x 16, checked against the sprite maximum); the
; routine's ASL #2 then makes the offset into the custom area CCOffset x
; 64 bytes instead of x 4.
; Amiga mode (the list is built in memory only, as in t164_display): a
; CopList with 80 custom cops, CustomSprites at CCOffset 2; where its
; sprite moves start (+92) relative to the custom area (+28): the wait
; for the line (2 longs) comes first.
; Correct: 2 x 4 + 8 = 16 bytes into the custom area.  The original:
; 2 x 64 + 8 = 136.
DEFTYPE .l

!TestCase{ccoffset}
  InitCopList 0,44,32,$10004,8,32,80
  c=Addr CopList(0)
  CustomSprites 0,2,14,2
  !CheckEq{Peek.l(c+92)-Peek.l(c+28),16,"sprite_moves_offset"}
!EndTestCase
