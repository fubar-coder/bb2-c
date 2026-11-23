; rievallib-digits-unchecked (#634) - PFEvaluate's number loops take every
; character up to the next space as a digit: decimal adds c-"0", hex
; c-"0" or c-"a"+10, binary ORs c-"0".  "12x" is 192, "%2" 2, "$g" 16,
; "1.5" 2645, all without an error.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: a character that is no digit of the number's base is error 4
; "No result value!" with the result 0; "$ff", "%101" and "-12" stay
; 255, 5 and -12.
DEFTYPE .l

!TestCase{non_digits}
  Dim e$(7),v(7),t(7)
  e$(1)="12x" : e$(2)="%2" : e$(3)="$g" : e$(4)="1.5"
  e$(5)="$ff" : e$(6)="%101" : e$(7)="-12"
  For i=1 To 7
    v(i)=PFEvaluate(e$(i))
    t(i)=PFErrorType
  Next
  !CheckEq{v(1),0,"decimal_letter_result"}
  !CheckEq{t(1),4,"decimal_letter_error"}
  !CheckEq{v(2),0,"binary_two_result"}
  !CheckEq{t(2),4,"binary_two_error"}
  !CheckEq{v(3),0,"hex_g_result"}
  !CheckEq{t(3),4,"hex_g_error"}
  !CheckEq{v(4),0,"dot_result"}
  !CheckEq{t(4),4,"dot_error"}
  !CheckEq{v(5),255,"hex_ff_result"}
  !CheckEq{t(5),0,"hex_ff_error"}
  !CheckEq{v(6),5,"binary_101_result"}
  !CheckEq{t(6),0,"binary_101_error"}
  !CheckEq{v(7),-12,"negative_result"}
  !CheckEq{t(7),0,"negative_error"}
!EndTestCase
