; compiler-procend-kind-funcret (#940) - Function Return inside a
; Statement evaluates its expression with the type byte of the Statement
; ($FF) and compiles.
; Correct: a compile error (Function Return inside a Statement).
; @fixed #940
Statement s{}
  NPrint "in s"
  Function Return 5
End Statement
s{}
End
