; compiler-procend-kind (#940) - doend handles End Statement and End
; Function by the token only and does not check the kind of the open
; procedure.  A string Function ended with End Statement gets the exit of
; a string function (endstate, getstlen: move.l (a7)+,d0) without the
; string length End Function would have pushed: it pops its return address
; as the length and returns to a wrong address: the string buffer
; pointer the caller saved, so the CPU runs the text the Function put
; there.  The text "Nu" is $4E75, an RTS: the original returns to the
; Gosub of the case and ends the program, the same way on every run.
; Correct: a compile error (End Statement inside a Function).
Function$ f{}
  NPrint "Nu"
End Statement

!TestCase{endstatement_in_function}
  a$=f{}
  !CheckEq{1,1,"after_f"}
!EndTestCase
