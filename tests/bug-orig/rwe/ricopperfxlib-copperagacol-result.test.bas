; ricopperfxlib-copperagacol-result (#590) - CopperAGACol is declared
; with a long result but returns the constant 1234 without a custom
; copper space and its col# argument with one.  The custom space is the
; program's own memory (CopperResetAdr).
; Correct (decision W6 Q3): -1 when the command worked, 0 when not (no
; custom space).
DEFTYPE .l

!TestCase{results}
  m=AllocMem_(256,$10000)
  !Require{m<>0,"memory"}
  r0=CopperAGACol(37,2,4,6)
  !CheckEq{r0,0,"no_space"}
  CopperResetAdr m,0
  r1=CopperAGACol(37,2,4,6)
  !CheckEq{r1,-1,"space"}
  FreeMem_ m,256
!EndTestCase
