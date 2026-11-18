; rievallib-extra-values-no-error (#635) - at the end of the string
; PFEvaluate returns the value on the top of its stack without checking
; that it is the only one: "1 2" is 2 and "1 2 + 3" 3, without an error.
; Library 121 is the acidlibs' polylib on the CD: ref and port get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct: more than one value left is error 4 "No result value!" with
; the result 0; "1 2 +" stays 3.
; @fixed #635
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
Dim e$(3)
e$(1)="1 2" : e$(2)="1 2 + 3" : e$(3)="1 2 +"
For i=1 To 3
  r=PFEvaluate(e$(i))
  NPrint e$(i)," ",r," ",PFErrorType
Next
End
