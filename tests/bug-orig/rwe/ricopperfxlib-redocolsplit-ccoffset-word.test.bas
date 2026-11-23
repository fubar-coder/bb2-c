; ricopperfxlib-redocolsplit-ccoffset-word (#594) - RedoColSplit takes
; ccoffset as a long but turns it into the byte offset with ASL.w #2:
; only the low word is shifted, so instruction $4000 (byte offset
; $10000) becomes byte offset 0.  The custom space is the program's own
; memory (CopperResetAdr), 64 KB and a bit, cleared; RedoColSplit of one
; line at instruction $4000 writes its two colour words at +10 and +18
; of the split.
; Correct: the colour words are written at $10000 + 10 (and + 18), the
; start of the space stays 0.
DEFTYPE .l

!TestCase{offset_4000}
  Dim c.l(5)
  c(0)=$12 : c(1)=$34 : c(2)=$56 : c(3)=$12 : c(4)=$34 : c(5)=$56
  s=$10100
  m=AllocMem_(s,$10000)
  !Require{m<>0,"memory"}
  CopperResetAdr m,0
  RedoColSplit &c(0),1,$4000
  !CheckEq{Peek.w(m+10)&$FFFF,0,"at_10"}
  !CheckEq{Peek.w(m+$1000A)&$FFFF,309,"at_1000A"}
  FreeMem_ m,s
!EndTestCase
