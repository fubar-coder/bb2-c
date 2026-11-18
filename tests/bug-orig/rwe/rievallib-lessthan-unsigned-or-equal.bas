; rievallib-lessthan-unsigned-or-equal (#638) - PFEvaluate's "<" is
; "unsigned lower or same" (SLS), while ">" is signed "greater": "2 2 <"
; is true, "-1 1 <" false, "1 -1 <" true.  RIEvalLib's manual: "< - Test
; for less than".  Library 121 is the acidlibs' polylib on the CD: ref
; and port get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: a signed, strict "less than": 0, -1, 0, -1.
; @fixed #638
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
NPrint PFEvaluate("2 2 <")
NPrint PFEvaluate("-1 1 <")
NPrint PFEvaluate("1 -1 <")
NPrint PFEvaluate("1 2 <")
End
