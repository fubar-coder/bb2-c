; rievallib-pferrortext-no-error-length (#640) - PFErrorText without an
; error sets only the low word of d0, the length of its string result:
; the upper word is what d0 held before the call.  Here d0 is $20000 at
; the call (GetReg), so the empty text gets a length of 131072.  Library
; 121 is the acidlibs' polylib on the CD: ref and port get the acidlibs
; of the variant rwe-1999-eval too (C build, without polylib).
; Correct: without an error the text is empty, length 0.
; @fixed #640
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
r=PFEvaluate("1 2 +")
NPrint r
GetReg d0,$20000
a$=PFErrorText
NPrint Len(a$)
End
