; rievallib-sign-before-hex-binary (#643) - PFEvaluate tests for "$" and
; "%" before the sign: behind a "-" they are read as decimal digits, so
; "-$10" is -24410 and "-%1" -2451.  Library 121 is the acidlibs' polylib
; on the CD: the cells with the CD's libraries get the acidlibs of the
; variant rwe-1999-eval too (C build, without polylib).
; Correct: -16 and -1.
DEFTYPE .l

!TestCase{signed_numbers}
  a=PFEvaluate("-$10")
  b=PFEvaluate("-%1")
  !CheckEq{a,-16,"negative_hex"}
  !CheckEq{b,-1,"negative_binary"}
!EndTestCase
