; rievallib-lessthan-unsigned-or-equal (#638) - PFEvaluate's "<" is
; "unsigned lower or same" (SLS), while ">" is signed "greater": "2 2 <"
; is true, "-1 1 <" false, "1 -1 <" true.  RIEvalLib's manual: "< - Test
; for less than".  Library 121 is the acidlibs' polylib on the CD: the
; cells with the CD's libraries get the acidlibs of the variant
; rwe-1999-eval too (C build, without polylib).
; Correct: a signed, strict "less than": 0, -1, 0, -1.
DEFTYPE .l

!TestCase{less_than}
  a=PFEvaluate("2 2 <")
  b=PFEvaluate("-1 1 <")
  c=PFEvaluate("1 -1 <")
  d=PFEvaluate("1 2 <")
  !CheckEq{a,0,"equal"}
  !CheckEq{b,-1,"negative_first"}
  !CheckEq{c,0,"negative_second"}
  !CheckEq{d,-1,"smaller"}
!EndTestCase
