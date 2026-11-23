; compiler-procend-kind-endfunc (#940) - End Function closes a Statement
; without a compile error (doend does not check the kind of the open
; procedure; it only adds a moveq #0,d0).
; Correct: a compile error (End Function inside a Statement).
Statement s{}
  NPrint "in s"
End Function

!TestCase{endfunc_in_statement}
  s{}
  !CheckEq{1,1,"after_s"}
!EndTestCase
