; rievallib-ctp-drops-characters (#632) - ConvToPostFix ends an operand at
; a character that is no digit, a-f, register, "$ % . { }" and drops it:
; "1,2" becomes "1 2 " and "12x+3" "12 3 + ", which PFEvaluate takes as
; 15 without an error.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: the character stays in the operand ("1,2 ", "12x 3 + "), and
; PFEvaluate rejects it with error 4 "No result value!" and the result 0;
; "12+3" stays "12 3 + " (15).
DEFTYPE .l

!TestCase{dropped_characters}
  buf=AllocMem_(256,$10001)
  !Require{buf<>0,"memory"}
  Dim e$(3),p$(3),v(3),t(3)
  e$(1)="1,2" : e$(2)="12x+3" : e$(3)="12+3"
  For i=1 To 3
    ConvToPostFix e$(i),buf
    p$(i)=Peek$(buf)
    v(i)=PFEvaluate(p$(i))
    t(i)=PFErrorType
  Next
  FreeMem_ buf,256
  !CheckStrEq{p$(1),"1,2 ","comma_postfix"}
  !CheckEq{v(1),0,"comma_result"}
  !CheckEq{t(1),4,"comma_error"}
  !CheckStrEq{p$(2),"12x 3 + ","letter_postfix"}
  !CheckEq{v(2),0,"letter_result"}
  !CheckEq{t(2),4,"letter_error"}
  !CheckStrEq{p$(3),"12 3 + ","plain_postfix"}
  !CheckEq{v(3),15,"plain_result"}
  !CheckEq{t(3),0,"plain_error"}
!EndTestCase
