; rigfxlib-fillpalette-end-upper-word (#683) - FillPalette
; palette#,r,g,b,start,end takes an end of 0 as "up to the last colour",
; but tests it with TST.l d5, the whole register, while end is a #word
; argument and the comparison with start and the loop use its word only.
; The compiled program leaves an upper word other than 0 in d5 for the
; constant 0 and for a word variable that holds 0 (a long variable gives
; a clear register): end is then colour 0, and with start 2 nothing is
; filled.  The palette has 8 colours; colours 2 to 7 are 1 before each
; form, colour 1 is 0 throughout.
; Correct: end 0 fills colours 2 to 7 in all three forms.
DEFTYPE .l

!TestCase{end_zero}
  InitPalette 0,8
  PaletteInfo 0
  FillPalette 0,1,1,1,2,7
  FillPalette 0,2,2,2,2,0
  c1=PalRed(1) : c2=PalRed(2) : c7=PalRed(7)
  !CheckEq{c1,0,"constant_1"}
  !CheckEq{c2,2,"constant_2"}
  !CheckEq{c7,2,"constant_7"}
  FillPalette 0,1,1,1,2,7
  e.w=0
  FillPalette 0,3,3,3,2,e
  c1=PalRed(1) : c2=PalRed(2) : c7=PalRed(7)
  !CheckEq{c1,0,"word_1"}
  !CheckEq{c2,3,"word_2"}
  !CheckEq{c7,3,"word_7"}
  FillPalette 0,1,1,1,2,7
  f.l=0
  FillPalette 0,4,4,4,2,f
  c1=PalRed(1) : c2=PalRed(2) : c7=PalRed(7)
  !CheckEq{c1,0,"long_1"}
  !CheckEq{c2,4,"long_2"}
  !CheckEq{c7,4,"long_7"}
!EndTestCase
