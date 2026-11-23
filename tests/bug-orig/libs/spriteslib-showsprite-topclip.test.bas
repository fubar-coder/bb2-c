; spriteslib-showsprite-topclip (#283) - ShowSprite of a sprite that
; sticks out of the slice at the top advances the data pointer by 4
; bytes per clipped line (EXG d2,d4:NEG d4:ASL #2,d4), also in
; SpriteMode 1/2 where a line has 8 or 16 bytes, and adds the offset as
; a long whose upper word is that of the y argument's register.
; Here: a 32x20 sprite in SpriteMode 1 (8 bytes per line), shown 5 lines
; above the slice; then the same with y in a long variable that holds
; $1FFFB (the word -5, the upper word 1).  The offset of the data pointer
; in the slice's copper list (sprite channel 0, +14 of the slice: pth at
; +10, ptl at +14) from the sprite's data.
; Correct: both offsets are 5 lines of 8 bytes, 40.  The original gives
; 20 and 65556.
DEFTYPE .l

!TestCase{topclip}
  SpriteMode 1
  BitMap 0,320,200,2
  BitMap 1,32,20,2
  Use BitMap 1 : Boxf 0,0,31,19,3
  GetaShape 0,0,0,32,20
  GetaSprite 0,0
  d=Peek.l(Addr Sprite(0))
  BLITZ
  Slice 0,44,320,200,$fff8,2,8,8,320,320
  Show 0
  ShowSprite 0,100,-5,0
  m=Peek.l(Addr Slice(0)+14)
  p1=(Peek.w(m+10) LSL 16) OR (Peek.w(m+14) AND $FFFF)
  y=$1FFFB
  ShowSprite 0,100,y,0
  p2=(Peek.w(m+10) LSL 16) OR (Peek.w(m+14) AND $FFFF)
  AMIGA
  SpriteMode 0
  !CheckEq{p1-d,40,"offset_word_y"}
  !CheckEq{p2-d,40,"offset_long_y"}
!EndTestCase
