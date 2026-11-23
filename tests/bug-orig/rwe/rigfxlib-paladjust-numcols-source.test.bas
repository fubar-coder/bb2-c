; rigfxlib-paladjust-numcols-source (#685) - PalAdjust palette#,ratio
; adjusts as many colours as PaletteInfo's palette (the source) has,
; into the target palette, without a look at the target's number of
; colours: a target smaller than the source is written behind its
; colours.  The manual leaves that to the programmer ("should be AT LEAST
; as big [as] the source palette"); the original has no error check
; area.  Here with runtime error checking: a source of 32 colours, a
; target that says 8.  Made safe: the target is palette 1 of 32 colours
; with the number in its colour data set to 8, so the 24 colours behind
; are its own memory (the object keeps 32, with which it is freed).
; Correct: the error check refuses the target: "PalAdjust colours outside
; the target palette".
DEFTYPE .l

!TestCase{numcols}
  InitPalette 0,32
  InitPalette 1,32
  b=Peek.l(Addr Palette(1))
  !Require{b<>0,"memory"}
  Poke.w b,8
  PaletteInfo 0
  !ExpectError{"PalAdjust colours outside the target palette","refused"}
  PalAdjust 1,0.5
  !Check{0,"not_refused"}
  Poke.w b,32
!EndTestCase
