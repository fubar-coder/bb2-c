; rievallib-power-exponents (#641) - PFEvaluate's "^" returns 1 for the
; base 0, the base itself for the exponents 1, 0 and every negative one,
; and multiplies (exponent - 1) times with DBRA on the low word of
; exponent - 2: "3 0 ^" is 3, "0 5 ^" 1, "2 -1 ^" 2, "-1 -2 ^" -1,
; "0 -1 ^" 1, "3 65538 ^" 9.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct (an integer library: the truncated value, user decision W2
; Q2): x^0 = 1, 0^n = 0 for n > 0; a negative exponent gives 0, except
; for the base 1 (1) and -1 (1 for an even, -1 for an odd exponent), and
; 0^-n is error 3 "Divide by 0 error!" with the result 0; the exponent
; counts with all 32 bits (the low 32 bits of the power: 3^65538 is
; 815005705).  "-1 100001 ^", "2 10 ^", "-1 -3 ^", "1 -5 ^" and
; "0 0 ^" stay -1, 1024, -1, 1 and 1.
DEFTYPE .l

!TestCase{exponents}
  Dim e$(12),v(12),t(12)
  e$(1)="3 0 ^" : e$(2)="0 5 ^" : e$(3)="2 -1 ^" : e$(4)="-1 -2 ^"
  e$(5)="0 -1 ^" : e$(6)="3 65538 ^" : e$(7)="-1 100001 ^"
  e$(8)="2 10 ^" : e$(9)="-1 -3 ^" : e$(10)="1 -5 ^" : e$(11)="0 0 ^"
  e$(12)="7 -2 ^"
  For i=1 To 12
    v(i)=PFEvaluate(e$(i))
    t(i)=PFErrorType
  Next
  !CheckEq{v(1),1,"base_3_exp_0_result"}
  !CheckEq{t(1),0,"base_3_exp_0_error"}
  !CheckEq{v(2),0,"base_0_exp_5_result"}
  !CheckEq{t(2),0,"base_0_exp_5_error"}
  !CheckEq{v(3),0,"base_2_exp_m1_result"}
  !CheckEq{t(3),0,"base_2_exp_m1_error"}
  !CheckEq{v(4),1,"base_m1_exp_m2_result"}
  !CheckEq{t(4),0,"base_m1_exp_m2_error"}
  !CheckEq{v(5),0,"base_0_exp_m1_result"}
  !CheckEq{t(5),3,"base_0_exp_m1_error"}
  !CheckEq{v(6),815005705,"base_3_exp_65538_result"}
  !CheckEq{t(6),0,"base_3_exp_65538_error"}
  !CheckEq{v(7),-1,"base_m1_exp_100001_result"}
  !CheckEq{t(7),0,"base_m1_exp_100001_error"}
  !CheckEq{v(8),1024,"base_2_exp_10_result"}
  !CheckEq{t(8),0,"base_2_exp_10_error"}
  !CheckEq{v(9),-1,"base_m1_exp_m3_result"}
  !CheckEq{t(9),0,"base_m1_exp_m3_error"}
  !CheckEq{v(10),1,"base_1_exp_m5_result"}
  !CheckEq{t(10),0,"base_1_exp_m5_error"}
  !CheckEq{v(11),1,"base_0_exp_0_result"}
  !CheckEq{t(11),0,"base_0_exp_0_error"}
  !CheckEq{v(12),0,"base_7_exp_m2_result"}
  !CheckEq{t(12),0,"base_7_exp_m2_error"}
!EndTestCase
