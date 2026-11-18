; rievallib-digits-unchecked (#634) - PFEvaluate's number loops take every
; character up to the next space as a digit: decimal adds c-"0", hex
; c-"0" or c-"a"+10, binary ORs c-"0".  "12x" is 192, "%2" 2, "$g" 16,
; "1.5" 2645, all without an error.
; Library 121 is the acidlibs' polylib on the CD: ref and port get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct: a character that is no digit of the number's base is error 4
; "No result value!" with the result 0; "$ff", "%101" and "-12" stay
; 255, 5 and -12.
; @fixed #634
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
Dim e$(7)
e$(1)="12x" : e$(2)="%2" : e$(3)="$g" : e$(4)="1.5"
e$(5)="$ff" : e$(6)="%101" : e$(7)="-12"
For i=1 To 7
  r=PFEvaluate(e$(i))
  NPrint e$(i)," ",r," ",PFErrorType
Next
End
