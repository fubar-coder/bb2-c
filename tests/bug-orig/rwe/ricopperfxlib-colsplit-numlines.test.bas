; ricopperfxlib-colsplit-numlines (#589) - DoColSplit does not check
; numlines: the DBRA counter is numlines - 1 as a word, so -1 writes
; 65536 lines (20 bytes each, 24 with the line 256 wait: 1.3 MB) into the
; custom copper space; 0 divides by zero (DIVS: the divide-by-zero
; exception; without runtime error checking the session ends in a
; timeout, with it the debugger stops at "GURU! - Division by Zero").
; The custom space is the
; program's own memory (CopperResetAdr), 1.4 MB, with a marker long at
; 1000000 (line 50000 or so) that shows the run-away loop safely; then
; the program calls DoColSplit with 0 lines.  The original has no error
; check area.  Here with runtime error checking.
; Correct: the error check refuses numlines -1: "Colour split needs at
; least one line", nothing is written.
DEFTYPE .l

!TestCase{numlines}
  Dim c.l(5)
  For i=0 To 5 : c(i)=i*40 : Next
  s=1400000
  m=AllocMem_(s,$10000)
  !Require{m<>0,"memory"}
  Poke.l m+1000000,$12345678
  CopperResetAdr m,0
  !ExpectError{"Colour split needs at least one line","refused_minus1"}
  DoColSplit &c(0),-1,1
  !CheckEq{Peek.l(m+1000000),$12345678,"marker_kept"}
  CopperResetAdr m,0
  !ExpectError{"Colour split needs at least one line","refused_zero"}
  DoColSplit &c(0),0,1
  FreeMem_ m,s
!EndTestCase
