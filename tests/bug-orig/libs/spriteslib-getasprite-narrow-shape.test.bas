; spriteslib-getasprite-narrow-shape (#282) - GetaSprite copies 2/4/8
; bytes per sprite and row (SpriteMode 0/1/2) whatever the width of the
; shape: in SpriteMode 1 or 2 the last sprite of a shape whose byte width
; is no multiple of the sprite width takes bytes of the next row (or from
; behind the shape) into its right edge.
; Here: a 16x2 4 colour shape (2 bytes per row), row 0 colour 0, row 1
; colour 3; SpriteMode 1 (32 pixels, 4 bytes per plane and row).  The
; original's sprite row 0 holds row 1 of the shape in its right half
; ($0000FFFF in both planes).
; Correct: the sprite's pixels right of the shape's 16 are 0: row 0 is
; 0 in both planes, row 1 has the shape's row in its left half
; ($FFFF0000).
DEFTYPE .l

!TestCase{narrow_shape}
  BitMap 0,16,2,2
  Use BitMap 0
  Cls 0
  Line 0,1,15,1,3
  GetaShape 0,0,0,16,2
  SpriteMode 1
  GetaSprite 0,0
  SpriteMode 0
  d=Peek.l(Addr Sprite(0))
  !CheckStrEq{Hex$(Peek.l(d)),"00000000","row_0_plane_0"}
  !CheckStrEq{Hex$(Peek.l(d+4)),"00000000","row_0_plane_1"}
  !CheckStrEq{Hex$(Peek.l(d+8)),"FFFF0000","row_1"}
!EndTestCase
