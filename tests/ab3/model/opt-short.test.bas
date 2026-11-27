; opt-short - OPTIMIZE 16 (short branches): the program's results are the
; same as without the line
OPTIMIZE 16
!TestCase{opt_short}
  Dim os$(3)
  For i = 1 To 3
    If i = 2 Then os$(i)="two" Else os$(i)=Str$(i)
  Next
  !CheckStrEq{os$(1),"1","l1"}
  !CheckStrEq{os$(2),"two","l2"}
  !CheckStrEq{os$(3),"3","l3"}
!EndTestCase
