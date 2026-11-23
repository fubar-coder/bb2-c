; riamosfunclib-reserve-negative-length (#500) - Reserve(length) returns
; the number of the bank it reserved, or -1 for failure (RIAmosFuncLib's
; manual).  For a negative length it returns 0 (BMI oops), the number of
; the first bank: the program takes bank 0 as reserved.
; Correct: Reserve(-5) returns -1 and reserves nothing.
DEFTYPE .l

!TestCase{reserve_negative_length}
  r=Reserve(-5)
  !CheckEq{r,-1,"result"}
  !CheckEq{Start(0),0,"bank_0_free"}
!EndTestCase
