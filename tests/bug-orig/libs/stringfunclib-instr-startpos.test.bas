; stringfunclib-instr-startpos (#287) - Instr(s$,f$,Startpos) starts the
; search at Startpos-1 without checking Startpos: a start position of 0 or
; below compares the bytes before the string, and an empty find string is
; "found" there: Instr(s$,"",0) is 0, Instr(s$,"",-5) is -5.
; Correct: a start position below 1 searches from the first character, as
; Instr(s$,f$) does: 1 for the empty find string, 2 for "b".
DEFTYPE .l

!TestCase{start_below_one}
  a$="abc"
  v=Instr(a$,"",0)
  !CheckEq{v,1,"empty_0"}
  v=Instr(a$,"",-5)
  !CheckEq{v,1,"empty_minus5"}
  v=Instr(a$,"b",0)
  !CheckEq{v,2,"b_0"}
  v=Instr(a$,"",1)
  !CheckEq{v,1,"empty_1"}
!EndTestCase
