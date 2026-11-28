; expr-tsr-shift-float (#1235, type-system-rework) - a shift with a whole
; or Quick left operand compiles under a Float destination (spec compiler/
; expression-types, "A shift keeps the type of its left operand", scenario
; "A shift under a Float destination"): the shift keeps the type of its
; left operand and converts to Float at the end (rf.f=b LSR 1 with b.b=3
; is 1.0, rf.f=q ASL 1 with q=1.25 is 2.5, the Quick shifted in 48.16); a
; Float left operand stays refused (the data source
; expr-tsr-shift-float.test/float_left.bas).  The original refuses the
; operator under a Float destination ("Illegal Operator for Type"): its
; compile result is the declared deviation of its cells.
; Correct: the values of the spec's scenarios.
!TestCase{lsr_of_whole_under_float}
  b.b=3
  rf.f=b LSR 1
  !CheckFltNear{rf,1.0,0.000001,"rf_lsr"}
!EndTestCase

!TestCase{asl_of_quick_under_float}
  q=1.25
  rf.f=q ASL 1
  !CheckFltNear{rf,2.5,0.000001,"rf_asl"}
!EndTestCase

!TestCase{shifts_beside_float_term}
  b.b=3 : q=20000
  rf.f=(b LSL 2)+0.5
  !CheckFltNear{rf,12.5,0.000001,"rf_whole_sum"}
  rf=(q LSL 1)+0.5
  !CheckFltNear{rf,40000.5,0.01,"rf_quick_sum"}
!EndTestCase
