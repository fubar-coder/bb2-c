; rievallib-power-exponents (#641) - PFEvaluate's "^" returns 1 for the
; base 0, the base itself for the exponents 1, 0 and every negative one,
; and multiplies (exponent - 1) times with DBRA on the low word of
; exponent - 2: "3 0 ^" is 3, "0 5 ^" 1, "2 -1 ^" 2, "-1 -2 ^" -1,
; "0 -1 ^" 1, "3 65538 ^" 9.
; Library 121 is the acidlibs' polylib on the CD: ref and port get the
; acidlibs of the variant rwe-1999-eval too (C build, without polylib).
; Correct (an integer library: the truncated value, user decision W2
; Q2): x^0 = 1, 0^n = 0 for n > 0; a negative exponent gives 0, except
; for the base 1 (1) and -1 (1 for an even, -1 for an odd exponent), and
; 0^-n is error 3 "Divide by 0 error!" with the result 0; the exponent
; counts with all 32 bits (the low 32 bits of the power: 3^65538 is
; 815005705).  "-1 100001 ^", "2 10 ^", "-1 -3 ^", "1 -5 ^" and
; "0 0 ^" stay -1, 1024, -1, 1 and 1.
; @fixed #641
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
Dim e$(12)
e$(1)="3 0 ^" : e$(2)="0 5 ^" : e$(3)="2 -1 ^" : e$(4)="-1 -2 ^"
e$(5)="0 -1 ^" : e$(6)="3 65538 ^" : e$(7)="-1 100001 ^"
e$(8)="2 10 ^" : e$(9)="-1 -3 ^" : e$(10)="1 -5 ^" : e$(11)="0 0 ^"
e$(12)="7 -2 ^"
For i=1 To 12
  r=PFEvaluate(e$(i))
  NPrint e$(i)," ",r," ",PFErrorType
Next
End
