; rievallib-indirect-odd-long (#636) - the indirection {address}.b of
; PFEvaluate always reads a long at the address and takes its top byte: an
; odd address is an address error on a 68000 (the session ends in a
; timeout).  The machine is the A600 (68000); the byte $5A is put at an
; odd address of a block of its own (the operand in lower case: PFEvaluate
; takes the postfix form, whose hex digits are a-f).  Library 121 is the
; acidlibs' polylib on the CD: the cells with the CD's libraries get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct: {address}.b reads the byte, 90.
DEFTYPE .l

!TestCase{odd_address}
  m=AllocMem_(16,$10001)
  !Require{m<>0,"memory"}
  Poke.b m+5,$5A
  v=PFEvaluate(LCase$("{$"+Hex$(m+5)+"}.b"))
  FreeMem_ m,16
  !CheckEq{v,90,"byte"}
!EndTestCase
