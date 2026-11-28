; expr-tsr-bitop-float (#1235, type-system-rework) - a bit operator or NOT
; with whole operands compiles under a Float destination (spec compiler/
; expression-types, "Bit operators take whole operands", scenario "Whole
; operands under a Float destination"): its whole result converts to Float
; at the end (rf.f=b AND 1 with b.b=3 is 1.0); a Float operand stays
; refused (the data source expr-tsr-bitop.test/float_dest.bas).  The
; original refuses the operator under a Float destination ("Illegal
; Operator for Type"): its compile result is the declared deviation of its
; cells.
; Correct: the values of the spec's scenarios.
!TestCase{and_of_whole_under_float}
  b.b=3
  rf.f=b AND 1
  !CheckFltNear{rf,1.0,0.000001,"rf_and"}
!EndTestCase

!TestCase{not_of_whole_under_float}
  w.w=5
  rf.f=Not w
  !CheckFltNear{rf,-6.0,0.000001,"rf_not"}
!EndTestCase

!TestCase{or_beside_float_term}
  b.b=3
  rf.f=(b OR 4)+0.5
  !CheckFltNear{rf,7.5,0.000001,"rf_or"}
!EndTestCase
