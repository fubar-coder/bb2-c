; ribbmaplib-getpalette-past-colr (#1113) - BBGetPalette reads the
; colours low..high of the COLR chunk without comparing high with the
; chunk's length.  The scenario (built in memory) has a COLR chunk with
; low 0 and high 3 but only the bytes of two colours ($11,$22,$33 and
; $44,$55,$66); the chunk "NEXT" follows.  The original takes colour 2
; from the next chunk's id and colour 3 from its id and length.
; Correct: only the two colours inside the chunk are read; colours 2 and
; 3 of the palette are kept.
DEFTYPE .l

!TestCase{two_colours_in_chunk}
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,32 : Poke.l m+8,$5343454E
  Poke.l m+12,$434F4C52 : Poke.l m+16,8 : Poke.b m+20,0 : Poke.b m+21,3
  For i=0 To 5 : Poke.b m+22+i,$11*(i+1) : Next
  Poke.l m+28,$4E455854 : Poke.l m+32,4 : Poke.l m+36,$778899AA
  InitPalette 0,16
  o=Addr Palette(0) : d=Peek.l(o)
  c2=Peek.l(d+28) : c3=Peek.l(d+40)
  !CheckEq{SetBBScenarioLoc(m),-1,"scenario_set"}
  BBGetPalette 0
  d=Peek.l(o)
  !CheckEq{Peek.w(o+4),16,"count"}
  !CheckEq{Peek.l(d+4),$11111111,"colour_0_red"}
  !CheckEq{Peek.l(d+8),$22222222,"colour_0_green"}
  !CheckEq{Peek.l(d+12),$33333333,"colour_0_blue"}
  !CheckEq{Peek.l(d+16),$44444444,"colour_1_red"}
  !CheckEq{Peek.l(d+20),$55555555,"colour_1_green"}
  !CheckEq{Peek.l(d+24),$66666666,"colour_1_blue"}
  !CheckEq{Peek.l(d+28),c2,"colour_2_kept"}
  !CheckEq{Peek.l(d+40),c3,"colour_3_kept"}
  FreeMem_ m,64
!EndTestCase
