; expr-tsr-data (#1235, type-system-rework) - a Data.b value with a point
; is cut towards zero as a Float (spec compiler/expression-types, "The
; conversion to a whole destination rounds down and wraps", scenario
; "Data.b with a point"): Data.b -1.5,1.0 reads as -1 and 1 into Byte
; variables (a Quick constant rounded down would give -2).  The Data
; stands behind a label, outside the cases.
; The original and the port agree (these values already hold), so the test
; has no declared deviation.
; Correct: the values of the spec's scenario.
!TestCase{data_with_a_point}
  Restore data_point
  Read ab.b
  Read bb.b
  !CheckEq{ab,-1,"a_negative"}
  !CheckEq{bb,1,"b_whole"}
!EndTestCase
.data_point
  Data.b -1.5,1.0
