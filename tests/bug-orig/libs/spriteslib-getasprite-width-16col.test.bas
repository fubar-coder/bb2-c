; spriteslib-getasprite-width-16col (#988) - the other direction of the
; width limits of GetaSprite (see spriteslib-getasprite-width-limits): a
; 16 colour shape of 128 pixels is 8 attached pairs = 16 hardware
; sprites, but ShowSprite needs channel + count <= 8 ("Sprite Channel
; Unavailable"), so such a sprite can never be shown.  gspchk allows
; 128 pixels for 16 colours, so GetaSprite accepts it.  Runtime error
; checking on (the debugger).
; Correct: "Sprite too Wide" at GetaSprite.  The original accepts it
; ("accepted" is printed) and ShowSprite fails.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l

!TestCase{width_128_16col}
  BitMap 0,128,16,4
  Use BitMap 0
  GetaShape 0,0,0,128,16
  !ExpectError{"Sprite too Wide","refused"}
  GetaSprite 0,0
  !Check{0,"after"}
!EndTestCase
