; ribbmaplib-getpalette-count-high (#548) - BBGetPalette on a palette
; with fewer colours than the COLR chunk's high colour + 1 allocates a
; block of high + 1 colours but sets the count (in the palette object
; and in the block) to high: the last colour lies outside the palette,
; and every further BBGetPalette allocates a new block again.  The
; scenario (built in memory) has a COLR chunk of the colours 0 to 31;
; palette 0 has 16 colours.
; Correct: the count is 32 in the object and in the block, and a second
; BBGetPalette keeps the block.
DEFTYPE .l

!TestCase{high_colour_32}
  m=AllocMem_(128,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,110 : Poke.l m+8,$5343454E
  Poke.l m+12,$434F4C52 : Poke.l m+16,98 : Poke.b m+20,0 : Poke.b m+21,31
  For i=0 To 95 : Poke.b m+22+i,i : Next
  InitPalette 0,16
  !CheckEq{SetBBScenarioLoc(m),-1,"scenario_set"}
  BBGetPalette 0
  o=Addr Palette(0) : d=Peek.l(o)
  !CheckEq{Peek.w(o+4),32,"object_count"}
  !CheckEq{Peek.w(d),32,"block_count"}
  BBGetPalette 0
  same=(Peek.l(o)=d)
  !CheckEq{same,-1,"same_block"}
  !CheckEq{Peek.w(o+4),32,"second_count"}
  FreeMem_ m,128
!EndTestCase
