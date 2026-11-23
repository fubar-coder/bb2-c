; rigfxlib-paladjust-end-before-start (#684) - PalAdjust computes the
; count end - start + 1 as a word, * 3 (MULU.w) - 1 as the DBRA counter:
; end = start - 1 gives 0 channels and the counter $FFFF, 65536 channels
; (one byte in every long, 256 KB) are written from colour `start` on.
; end < start is an argument the programmer controls; the original has
; no error check area.  Here with runtime error checking.
; Made safe: palette 1 (source through PaletteInfo and target) has 22000
; colours (264004 bytes of colour data), so the 65536 channels from
; colour 5 on stay inside it; a marker byte $FF lies on the 1000th
; channel behind colour 5, ratio 0.5 halves what is written there.
; Correct: the error check refuses the range: "PalAdjust end colour
; before the start colour".
DEFTYPE .l

!TestCase{end_before_start}
  InitPalette 1,22000
  b=Peek.l(Addr Palette(1))
  !Require{b<>0,"memory"}
  Poke.b b+64+4000,$FF
  PaletteInfo 1
  !ExpectError{"PalAdjust end colour before the start colour","refused"}
  PalAdjust 1,0.5,5,4
  v=Peek.b(b+64+4000) AND $FF
  !CheckEq{v=$FF,-1,"marker_kept"}
!EndTestCase
