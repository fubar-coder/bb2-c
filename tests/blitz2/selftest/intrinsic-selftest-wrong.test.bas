; intrinsic-selftest-wrong - wrong on purpose: the check of the compiled
; code of the intrinsic tests (tests/blitz2/intrinsic-*.test.bas). Int of a
; Float is always a library call, the check says it is not; the runner must
; report FAILED and the report must name the check (int_f_calls), with the
; expected 0 and the actual 1 (ctest blitz2_selftest_intrinsic_selftest_wrong).
; jsrs{a,b}: the JSR instructions ($4EB9, every library call) of the code
; from label a to label b
Function.l jsrs{a.l,b.l}
  n.l=0
  While a<b
    If Peek.w(a)=$4EB9
      n=n+1 : a=a+4
    EndIf
    a=a+2
  Wend
  Function Return n
End Function
!TestCase{int_float}
  f.f=-1.75 : c.l=0
  .f1
  c=Int(f)
  .f2
  !CheckEq{jsrs{?f1,?f2},0,"int_f_calls"}
!EndTestCase
