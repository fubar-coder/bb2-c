; ribbmaplib-lowcol-highcol-sign (#550) - BBScenarioLowCol and
; BBScenarioHighCol read the low and high colour byte of the COLR chunk
; with its sign (MOVE.b, EXT.w): colours 128 to 255 come out negative
; (BBGetPalette reads the same bytes unsigned).  The scenario (built in
; memory) is FORM SCEN with a COLR chunk of the colours $81 to $83.
; Correct: 129 and 131.
; @fixed #550
; @rwe ribbmaplib
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,24 : Poke.l m+8,$5343454E
Poke.l m+12,$434F4C52 : Poke.l m+16,11 : Poke.b m+20,$81 : Poke.b m+21,$83
NPrint SetBBScenarioLoc(m)
NPrint "low ",BBScenarioLowCol
NPrint "high ",BBScenarioHighCol
FreeMem_ m,64
End
