; rievallib-register-index-unchecked (#642) - PFEvaluate takes the
; character behind "a" or "d" minus "0" (plus 8 for "a") as the register
; index, unchecked, and skips it: "d9" reads the long at index 9 of the
; PFRegisters block, "dx" the one at 72, "d" at the end of the string
; takes its 0 (index 208) and goes on behind it (here "5 +" behind a 0
; inside the Blitz string); a digit behind the register ("d01") is read
; as a number of its own.  The block here is a Dim of 300 longs holding
; their index, so every read has a known value.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: only 0-7 behind "a"/"d", and the register is followed by a
; space or the end: the others are error 4 "No result value!" with the
; result 0; "d7" and "a7" stay 7 and 15.
DEFTYPE .l

!TestCase{register_index}
  Dim rg.l(300)
  For i=0 To 300 : rg(i)=i : Next
  PFRegisters &rg(0)
  Dim e$(6),v(6),t(6)
  e$(1)="d9" : e$(2)="dx" : e$(3)="1 d"+Chr$(0)+"5 +"
  e$(4)="d01 +" : e$(5)="d7" : e$(6)="a7"
  For i=1 To 6
    v(i)=PFEvaluate(e$(i))
    t(i)=PFErrorType
  Next
  !CheckEq{v(1),0,"d9_result"}
  !CheckEq{t(1),4,"d9_error"}
  !CheckEq{v(2),0,"dx_result"}
  !CheckEq{t(2),4,"dx_error"}
  !CheckEq{v(3),0,"d_at_end_result"}
  !CheckEq{t(3),4,"d_at_end_error"}
  !CheckEq{v(4),0,"d01_result"}
  !CheckEq{t(4),4,"d01_error"}
  !CheckEq{v(5),7,"d7_result"}
  !CheckEq{t(5),0,"d7_error"}
  !CheckEq{v(6),15,"a7_result"}
  !CheckEq{t(6),0,"a7_error"}
!EndTestCase
