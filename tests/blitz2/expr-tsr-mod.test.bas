; expr-tsr-mod (#1235, type-system-rework; #1197, #1198) - Mod takes the
; sign of the divisor (spec compiler/expression-types, "Mod takes the sign
; of the divisor"): Mod gives a-b*floor(a/b), the remainder has the
; divisor's sign, computed exactly with the fractions of its operands, also
; for two constants.  (A divisor 0: expr-tsr-div0-trap and
; expr-tsr-div0-checked.)
; The original's values (-7 Mod -3 is -4, #1197), where they differ, are
; the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{signs}
  q=-7
  r1l.l=-7 Mod 3
  !CheckEq{r1l,2,"r1_negative_dividend"}
  r2l.l=7 Mod -3
  !CheckEq{r2l,-2,"r2_negative_divisor"}
  r3l.l=-7 Mod -3
  !CheckEq{r3l,-1,"r3_both_negative"}
  r4l.l=q Mod 2
  !CheckEq{r4l,1,"r4_quick_dividend"}
!EndTestCase

!TestCase{fractions}
  q=7.5 : p=0.5 : b.b=4
  xq.q=q Mod 2
  !CheckQEq{xq,1.5,"x_quick_whole"}
  yq.q=q Mod p
  !CheckQEq{yq,0,"y_quick_quick"}
  zq.q=b Mod 1.5
  !CheckQEq{zq,1,"z_byte_point"}
!EndTestCase

!TestCase{two_constants}
  xq.q=5.25 Mod 2
  !CheckQEq{xq,1.25,"x_quick"}
  yw.w=5.25 Mod 2
  !CheckEq{yw,1,"y_word"}
  zq.q=-5.25 Mod 2
  !CheckQEq{zq,0.75,"z_negative_dividend"}
  rl.l=-5.25 Mod -2
  !CheckEq{rl,-2,"r_negative_both"}
!EndTestCase
