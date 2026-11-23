; compiler-procend-kind-match (#940) - the check of End Statement / End
; Function and Statement Return / Function Return against the kind of
; the open procedure refuses only the other kind: each kind ended and
; left with its own keywords compiles and runs.
; A regression test of the fix of #940: the original already gives this
; (correct) result.

DEFTYPE .l

Statement s{a,p}
  If a=1 Then Statement Return
  Poke.l p,a
End Statement

Function$ f{a}
  If a=1 Then Function Return "one"
  Function Return "other"
End Function

Function g{a}
  If a=1 Then Function Return 10
End Function

!TestCase{procend_kind_match}
  Dim r.l(1)
  r(0)=0
  s{1,&r(0)}
  !CheckEq{r(0),0,"statement_return"}
  s{2,&r(0)}
  !CheckEq{r(0),2,"statement_end"}
  !CheckStrEq{f{1},"one","function_string_return"}
  !CheckStrEq{f{2},"other","function_string_end"}
  !CheckEq{g{1},10,"function_return"}
  !CheckEq{g{2},0,"function_end_without_value"}
!EndTestCase
