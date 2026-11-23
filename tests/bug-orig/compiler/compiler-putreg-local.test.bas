; compiler-putreg-local (#1202) - PutReg Rn,v inside a Statement or
; Function with a simple local variable v stores the register at the
; local's offset from a5, the base of the global variables, instead of a4,
; the procedure's locals (doputreg emits lea x(a5),a2 with a plain pokewd,
; not pokewda5s): the local keeps its value and 1, 2 or 4 bytes outside
; the local frame are overwritten.
; Correct: the function returns 42, the local holds what PutReg stored.
DEFTYPE .l

Function.l putlocal{}
  MOVEQ #42,d0
  PutReg d0,v
  Function Return v
End Function

!TestCase{putreg_local}
  !CheckEq{putlocal{},42,"local"}
!EndTestCase
