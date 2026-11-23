; rievallib-extra-values-no-error (#635) - at the end of the string
; PFEvaluate returns the value on the top of its stack without checking
; that it is the only one: "1 2" is 2 and "1 2 + 3" 3, without an error.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: more than one value left is error 4 "No result value!" with
; the result 0; "1 2 +" stays 3.
DEFTYPE .l

!TestCase{extra_values}
  Dim e$(3),v(3),t(3)
  e$(1)="1 2" : e$(2)="1 2 + 3" : e$(3)="1 2 +"
  For i=1 To 3
    v(i)=PFEvaluate(e$(i))
    t(i)=PFErrorType
  Next
  !CheckEq{v(1),0,"two_values_result"}
  !CheckEq{t(1),4,"two_values_error"}
  !CheckEq{v(2),0,"sum_and_value_result"}
  !CheckEq{t(2),4,"sum_and_value_error"}
  !CheckEq{v(3),3,"sum_result"}
  !CheckEq{t(3),0,"sum_error"}
!EndTestCase
