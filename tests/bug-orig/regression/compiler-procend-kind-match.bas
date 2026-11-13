; compiler-procend-kind-match (#940) - the check of End Statement / End
; Function and Statement Return / Function Return against the kind of
; the open procedure refuses only the other kind: each kind ended and
; left with its own keywords compiles and runs.
; A regression test of the fix of #940: the original already gives this (correct) result.
Statement s{a}
  If a=1 Then Statement Return
  NPrint "s ",a
End Statement
Function$ f{a}
  If a=1 Then Function Return "one"
  Function Return "other"
End Function
Function g{a}
  If a=1 Then Function Return 10
End Function
s{1}
s{2}
NPrint f{1}," ",f{2}
NPrint g{1}," ",g{2}
End
