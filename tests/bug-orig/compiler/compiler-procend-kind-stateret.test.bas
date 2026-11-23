; compiler-procend-kind-stateret (#940) - Statement Return inside a string
; Function jumps to the exit of the Function without the string length
; End Function or Function Return pushes: the exit pops the return
; address as the length.
; Correct: a compile error (Statement Return inside a Function).
Function$ f{}
  NPrint "in f"
  Statement Return
  Function Return "x"
End Function

!TestCase{stateret_in_function}
  a$=f{}
  !CheckEq{1,1,"after_f"}
!EndTestCase
