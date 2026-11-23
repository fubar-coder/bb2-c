; ricopperfxlib-docolsplit-result (#592) - DoColSplit is declared with a
; word result but never sets one: without a custom copper space d0 is
; still the cols_adr argument (an address, printed as "other" here), with
; one it is the red step of the gradient ((255 - 0) * (16384 / 10), its
; low word).  The custom space is the program's own memory
; (CopperResetAdr).
; Correct (decision W6 Q3): -1 when the command worked, 0 when not (no
; custom space).
DEFTYPE .l

!TestCase{results}
  Dim c.l(5)
  c(0)=0 : c(1)=0 : c(2)=0 : c(3)=255 : c(4)=128 : c(5)=64
  m=AllocMem_(1024,$10000)
  !Require{m<>0,"memory"}
  r0=DoColSplit(&c(0),10,1)
  !CheckEq{r0,0,"no_space"}
  CopperResetAdr m,0
  r1=DoColSplit(&c(0),10,1)
  !CheckEq{r1,-1,"space"}
  FreeMem_ m,1024
!EndTestCase
