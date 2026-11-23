; ricopperfxlib-copperwait-x-carry (#591) - CopperWait builds the first
; word of the WAIT as (y << 8) | (((x >> 1) & $FE) + 6) | 1: the +6 is
; added after the mask, so for x from 500 on the sum carries into bit 8,
; the line.  CopperWait 500,$40 waits for line $41.
; Correct: the line byte is y ($40) for every x; x = 498 as the control.
DEFTYPE .l

!TestCase{lines}
  m=AllocMem_(64,$10000)
  !Require{m<>0,"memory"}
  CopperResetAdr m,0
  CopperWait 498,$40
  CopperWait 500,$40
  !CheckEq{Peek.b(m)&255,64,"x498"}
  !CheckEq{Peek.b(m+4)&255,64,"x500"}
  FreeMem_ m,64
!EndTestCase
