; expr-tsr-destination (#1235, type-system-rework) - the destination does
; not change the computation (spec compiler/expression-types, "The
; destination does not change the computation" and "Only the named
; exceptions depend on the destination"): an operation is typed and
; computed from its operator and its operands only; a destination, a
; parameter or an index converts the finished value.  The named exceptions
; (the whole division under a Quick or Float destination, the shift, the
; top operation under a Float) are the subject of their own tests.
; A Quick or Float parameter takes the exceptions like a destination of its
; type (fq{1/16}).  The original's values are the declared deviations.
; Correct: the values of the spec's scenarios.
Function.q fq{x.q}
  Function Return x
End Function
Function.f ff{x.f}
  Function Return x
End Function

!TestCase{same_quotient}
  zl.l=(7/2)*2
  !CheckEq{zl,6,"z_long"}
  rw.w=7/2
  !CheckEq{rw,3,"r_word"}
!EndTestCase

; no destination: the printed value is that of the same rules
!TestCase{print_same_quotient}
  NPrint 7/2*2
!EndTestCase

!TestCase{whole_quick_long}
  i=300
  rl.l=i*300
  !CheckEq{rl,90000,"r_long"}
!EndTestCase

!TestCase{print_whole_quick}
  i=300
  NPrint i*300
!EndTestCase

!TestCase{exception_and_none}
  w.w=200
  xq.q=w/9
  !CheckEq{Peek.l(&xq),$001638E3,"x_quick"}
  rl.l=w/9
  !CheckEq{rl,22,"r_long"}
!EndTestCase

!TestCase{typed_parameters}
  q=20.875
  rq.q=fq{1/16}
  !CheckQEq{rq,0.0625,"pq_one_sixteenth"}
  rq=fq{1 LSL 16}
  !CheckQEq{rq,0,"pq_shift_wrap"}
  rf.f=ff{q/100}
  !CheckFltNear{rf,0.20875,0.000001,"pf_quotient"}
!EndTestCase
