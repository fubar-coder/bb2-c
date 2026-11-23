; stringfunclib-instr-startpos-check (#287) - Instr(s$,f$,Startpos)
; starts the search at Startpos-1 without checking Startpos.  The fix of
; #287 (release code) searches from the first character for a start
; position below 1; such a start position is a programming error, and the
; original has no error check for it, not even with runtime error
; checking.
; Here with runtime error checking: Instr with the start positions 1 and
; 3 (no error), then 0; the original compares the byte before the string
; and gives 0 for the empty find string.
; Correct: the error check refuses the start position: "Instr Startpos
; MUST be greater than zero".
DEFTYPE .l

!TestCase{start_zero}
  a$="abc"
  v=Instr(a$,"b",1)
  !CheckEq{v,2,"b_1"}
  v=Instr(a$,"c",3)
  !CheckEq{v,3,"c_3"}
  !ExpectError{"Instr Startpos MUST be greater than zero","refused"}
  v=Instr(a$,"",0)
  !Check{0,"not_refused"}
!EndTestCase
