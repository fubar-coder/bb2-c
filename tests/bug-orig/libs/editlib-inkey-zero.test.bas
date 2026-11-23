; editlib-inkey-zero (#128) - Inkey$(n) with n = 0 jumps to its end
; before MOVEQ #0,d6 and returns the caller's d6 as the length of the
; string: a string of a random length.  Here d6 is set to 5 and 3 right
; before the call (inline assembler).
; Correct: Inkey$(0) reads nothing and is the empty string.
DEFTYPE .l

!TestCase{zero_count}
  MOVEQ #5,d6 : l1=Len(Inkey$(0))
  MOVEQ #3,d6 : a$=Inkey$(0) : l2=Len(a$)
  !CheckEq{l1,0,"length_d6_5"}
  !CheckEq{l2,0,"length_d6_3"}
!EndTestCase
