; ribbmaplib-lowcol-highcol-sign (#550) - BBScenarioLowCol and
; BBScenarioHighCol read the low and high colour byte of the COLR chunk
; with its sign (MOVE.b, EXT.w): colours 128 to 255 come out negative
; (BBGetPalette reads the same bytes unsigned).  The scenario (built in
; memory) is FORM SCEN with a COLR chunk of the colours $81 to $83.
; Correct: 129 and 131.
DEFTYPE .l

!TestCase{colours_81_to_83}
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,24 : Poke.l m+8,$5343454E
  Poke.l m+12,$434F4C52 : Poke.l m+16,11 : Poke.b m+20,$81 : Poke.b m+21,$83
  !CheckEq{SetBBScenarioLoc(m),-1,"scenario_set"}
  !CheckEq{BBScenarioLowCol,129,"low"}
  !CheckEq{BBScenarioHighCol,131,"high"}
  FreeMem_ m,64
!EndTestCase
