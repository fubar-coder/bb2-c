; rievallib-sign-before-hex-binary (#643) - PFEvaluate tests for "$" and
; "%" before the sign: behind a "-" they are read as decimal digits, so
; "-$10" is -24410 and "-%1" -2451.  Library 121 is the acidlibs' polylib
; on the CD: ref and port get the acidlibs of the variant rwe-1999-eval
; too (C build, without polylib).
; Correct: -16 and -1.
; @fixed #643
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
NPrint PFEvaluate("-$10")
NPrint PFEvaluate("-%1")
End
