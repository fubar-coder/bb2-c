; rilesdebuglib-customreg-noop (#702) - GetCustomReg and GetCustomRegAdr
; end their range at $1FC (CMPI.w #$1fc): the last register of the name
; table, $1FE NOOP, is never named by GetCustomReg and never found by
; GetCustomRegAdr (both give what they give for an unknown register).
; Correct: GetCustomReg($1FE) is "NOOP", GetCustomRegAdr("NOOP") 510.
DEFTYPE .l

!TestCase{noop}
  a$=GetCustomReg($1FE)
  n$="NOOP"
  r=GetCustomRegAdr(n$)
  !CheckStrEq{a$,"NOOP","name"}
  !CheckEq{r,510,"address"}
!EndTestCase
