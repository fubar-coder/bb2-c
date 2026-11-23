; rifnslib-clip-glyph-shift-48-64 (#656) - a glyph row of a 48 pixel
; font has 3 words; FNSPrint reads the first two, tests the row against
; the clip zone and reads the third only for a row it draws.  After a
; row above ymin the glyph pointer is one word behind, and every row
; drawn after it takes the wrong words.  The font (built in memory,
; slot 1) is 48 pixels wide and 10 rows high; of the glyph of "!" only
; row 5 is set, its first word $8000 (pixel 0).  The clip zone starts at
; row 20, the print at row 15: rows 0-4 of the glyph are clipped, row 5
; is the first drawn, at row 20.
; Correct: pixel 0,20 is set, pixel 32,21 is not ("20: 1 21: 0").
DEFTYPE .l

!TestCase{glyph_shift}
  f=AllocMem_(1024,$10001)
  !Require{f<>0,"memory"}
  Poke.l f,$464E532E
  Poke.w f+4,10 : Poke.w f+6,48 : Poke.w f+8,0 : Poke.w f+10,60
  Poke.b f+33,48
  Poke.w f+256+60+5*6,$8000
  BitMap 0,320,64,1
  r=InstallFNS(1,f)
  FNSOutput 0
  FNSClip 0,20,319,30
  FNSInk 1
  FNSPrint 1,0,15,"!"
  p20=Point(0,20) : p21=Point(32,21)
  RemoveFNS 1
  FreeMem_ f,1024
  !CheckEq{p20,1,"pixel_0_20"}
  !CheckEq{p21,0,"pixel_32_21"}
!EndTestCase
