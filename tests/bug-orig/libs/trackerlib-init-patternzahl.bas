; trackerlib-init-patternzahl (#289) - mt_init searches the highest
; pattern number with a counter that a new maximum decrements once more
; (SUBQ instead of DBF): a new maximum at position 126 turns the counter 0
; into $FFFF, the search reads on through the 64K behind the position
; table.  The module here (generated) uses pattern 1 first at position
; 126; the search then takes the "M" of M.K. (77) as the highest pattern,
; puts the samples behind "pattern 78" and clears their first long there
; (mt_lop3) instead of at the start of pattern 2.  PlayModule runs
; mt_init; the program reads both markers back.
; Correct: the highest pattern is 1 - the samples begin at pattern 2, the
; long there is cleared, the marker at pattern 78 stays.
; @configs clib
; @pass file=init.mod|@TESTDATA@/trackerlib-init-patternzahl.mod
DEFTYPE .l
LoadModule 0,"CORPUS:init.mod"
d=Peek.l(Addr Module(0))
PlayModule 0
StopModule
NPrint "pattern 2 ",Hex$(Peek.l(d+$43C+2*1024))
NPrint "pattern 78 ",Hex$(Peek.l(d+$43C+78*1024))
End
