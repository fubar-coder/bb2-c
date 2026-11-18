; rievallib-spaces-only-return-address (#644) - PFEvaluate of a string of
; spaces only sets error 4 "No result value!" but returns the long above
; its empty stack, the return address into the program, instead of 0.
; Library 121 is the acidlibs' polylib on the CD: ref and port get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct: the result is 0, the error 4.
; @fixed #644
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
r=PFEvaluate("   ")
If r=0 Then NPrint "result 0" Else NPrint "result not 0"
NPrint PFErrorType
End
