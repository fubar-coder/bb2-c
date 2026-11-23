; rifnslib-underline-ignores-xmin (#667) - FNSPrint clips the characters
; at xmin, but the underline only at x = 0: with FNSClip 32,... an
; underlined text that starts left of 32 gets its underline drawn left
; of the clip zone, where its characters are not drawn.  The font (built
; in memory, slot 1) is 16 pixels wide, 8 rows high, underline offset 6;
; "a" is 8 pixels wide, its glyph empty.  The text of eight "a" starts at
; x = 0: its underline (row 16) runs from 0 to 63.
; Correct: no underline pixel left of 32, the underline from 32 on
; ("left: 0 right: 1").
DEFTYPE .l

!TestCase{underline_xmin}
  f=AllocMem_(4096,$10001)
  !Require{f<>0,"memory"}
  Poke.l f,$464E532E
  Poke.w f+4,8 : Poke.w f+6,16 : Poke.w f+8,6 : Poke.w f+10,16
  Poke.b f+97,8
  BitMap 0,320,64,1
  r=InstallFNS(1,f)
  FNSOutput 0
  FNSClip 32,0,319,63
  FNSPrint 1,0,10,"aaaaaaaa",4,1
  s=0
  For x=0 To 31 : s=s+Point(x,16) : Next
  pr=Point(40,16)
  RemoveFNS 1
  FreeMem_ f,4096
  !CheckEq{s,0,"left_of_xmin"}
  !CheckEq{pr,1,"right_of_xmin"}
!EndTestCase
