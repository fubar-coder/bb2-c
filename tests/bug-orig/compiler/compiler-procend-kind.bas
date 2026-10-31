; compiler-procend-kind (#940) - doend handles End Statement and End
; Function by the token only and does not check the kind of the open
; procedure.  A string Function ended with End Statement gets the exit of
; a string function (endstate, getstlen: move.l (a7)+,d0) without the
; string length End Function would have pushed: it pops its return address
; as the length and returns to a wrong address.
; Correct: a compile error (End Statement inside a Function).
; @pass timeout=30
Function$ f{}
  NPrint "in f"
End Statement
NPrint f{}
End
