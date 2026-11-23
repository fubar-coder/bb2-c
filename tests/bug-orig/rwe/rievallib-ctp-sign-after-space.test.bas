; rievallib-ctp-sign-after-space (#633) - ConvToPostFix takes a "-" as a
; sign only if the character directly before it is an operator or "(" (or
; it starts the string): behind a space it is a binary minus without an
; operand, "4* -2" becomes "4 * 2 - " and PFEvaluate fails with error 1.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: spaces in front of the "-" are skipped: "4 -2 * " (-8),
; "3 -4 - " (7), "-5 " (-5); "3 -4" stays 3-4 (-1).
DEFTYPE .l

!TestCase{sign_after_space}
  buf=AllocMem_(256,$10001)
  !Require{buf<>0,"memory"}
  Dim e$(4),p$(4),v(4),t(4)
  e$(1)="4* -2" : e$(2)="3- -4" : e$(3)=" -5" : e$(4)="3 -4"
  For i=1 To 4
    ConvToPostFix e$(i),buf
    p$(i)=Peek$(buf)
    v(i)=PFEvaluate(p$(i))
    t(i)=PFErrorType
  Next
  FreeMem_ buf,256
  !CheckStrEq{p$(1),"4 -2 * ","times_postfix"}
  !CheckEq{v(1),-8,"times_result"}
  !CheckEq{t(1),0,"times_error"}
  !CheckStrEq{p$(2),"3 -4 - ","minus_postfix"}
  !CheckEq{v(2),7,"minus_result"}
  !CheckEq{t(2),0,"minus_error"}
  !CheckStrEq{p$(3),"-5 ","leading_postfix"}
  !CheckEq{v(3),-5,"leading_result"}
  !CheckEq{t(3),0,"leading_error"}
  !CheckStrEq{p$(4),"3 4 - ","binary_postfix"}
  !CheckEq{v(4),-1,"binary_result"}
  !CheckEq{t(4),0,"binary_error"}
!EndTestCase
