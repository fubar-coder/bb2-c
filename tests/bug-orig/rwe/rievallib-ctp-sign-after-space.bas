; rievallib-ctp-sign-after-space (#633) - ConvToPostFix takes a "-" as a
; sign only if the character directly before it is an operator or "(" (or
; it starts the string): behind a space it is a binary minus without an
; operand, "4* -2" becomes "4 * 2 - " and PFEvaluate fails with error 1.
; Library 121 is the acidlibs' polylib on the CD: ref and port get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct: spaces in front of the "-" are skipped: "4 -2 * " (-8),
; "3 -4 - " (7), "-5 " (-5); "3 -4" stays 3-4 (-1).
; @fixed #633
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
buf=AllocMem_(256,$10001)
If buf=0 Then NPrint "no memory" : End
Dim e$(4)
e$(1)="4* -2" : e$(2)="3- -4" : e$(3)=" -5" : e$(4)="3 -4"
For i=1 To 4
  ConvToPostFix e$(i),buf
  p$=Peek$(buf)
  r=PFEvaluate(p$)
  NPrint "<",p$,"> ",r," ",PFErrorType
Next
FreeMem_ buf,256
End
