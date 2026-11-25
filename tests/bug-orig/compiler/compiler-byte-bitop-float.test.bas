; compiler-byte-bitop-float (#1217) - a Byte AND or OR in the right operand
; of a comparison under a Float destination.  The original evaluates the
; right operand in the destination's type, a Float, and refuses AND and OR
; there ("Illegal Operator for Type"); the port types the right operand by
; itself since #1177 (a Byte operation, converted up), so it compiles the
; line.
; Correct: -2 AND 0 is 0 and 3 OR -2 is -1, so 0=(-2 AND 0) is true (-1)
; and 0=(3 OR -2) is false (0).
!TestCase{float_destination}
  rf.f=0
  rf=(0=(-2 AND 0))
  !CheckFltNear{rf,-1,0.001,"and"}
  rf=(0=(3 OR -2))
  !CheckFltNear{rf,0,0.001,"or"}
!EndTestCase
