; rievallib-value-count-byte (#645) - PFEvaluate counts the values on its
; stack in a byte and compares it signed: with 128 or more values every
; operator fails with error 1 "Need 2 arguments for expression!" and the
; result is 0, although enough values are there.  Library 121 is the
; acidlibs' polylib on the CD: ref and port get the acidlibs of the
; variant rwe-1999-eval too (C build, without polylib).
; Correct: 130 values and 129 "+" add up to 130, no error.
; @fixed #645
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
a$=""
For i=1 To 130 : a$=a$+"1 " : Next
For i=1 To 129 : a$=a$+"+ " : Next
r=PFEvaluate(a$)
NPrint r
NPrint PFErrorType
End
