; stringfunclib-instr-end-overflow (#1030) - the end test of
; Instr(s$,f$,Startpos) adds the length of f$ to the search position and
; compares the sum with the length of s$ signed, after the addition
; (CMP.l d2,d5:BGT 'done).  A Startpos near $7FFFFFFF makes the sum
; overflow to a negative number, "within the string", and the search
; compares memory far from s$ until it finds f$ somewhere or hangs.
; f$ needs 2 characters or more for the overflow.
; Correct: a start position beyond the end of s$ finds nothing: 0.
DEFTYPE .l

!TestCase{far_start}
  v=Instr("abc","xy",$7FFFFFFF)
  !CheckEq{v,0,"max"}
  v=Instr("abc","bc",$7FFFFFFE)
  !CheckEq{v,0,"max_minus_one"}
  v=Instr("abcxy","xy",4)
  !CheckEq{v,4,"inside"}
!EndTestCase
