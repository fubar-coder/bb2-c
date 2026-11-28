; expr-tsr-conv-whole (#1235, type-system-rework) - the conversion to a
; whole destination rounds down and wraps (spec compiler/expression-types,
; "The conversion to a whole destination rounds down and wraps"): a Quick
; or 48.16 value converts to a Byte, Word or Long rounded down, a Float cut
; towards zero (as the original's SPFix); the result keeps the low 8, 16 or
; 32 bits; a Float beyond the Long range is cut towards zero and wraps to
; its low 32 bits, not saturated as SPFix does.  (Data.b with a point:
; expr-tsr-data.)
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{negative_fractions}
  f.f=-2.5 : q=-0.5 : v.f=-2.5 : n.w=3
  rl.l=-1.5
  !CheckEq{rl,-2,"r_exact_constant"}
  r1l.l=-1.4
  !CheckEq{r1l,-1,"r1_float_constant"}
  r2l.l=f
  !CheckEq{r2l,-2,"r2_float_variable"}
  w2.w=q
  !CheckEq{w2,-1,"w2_quick_variable"}
  r3l.l=v^n
  !CheckEq{r3l,-15,"r3_power"}
!EndTestCase

!TestCase{a_quotient_at_the_top}
  q=-3 : k.l=-10 : q2=1.5
  rl.l=q/2
  !CheckEq{rl,-2,"r_long"}
  xw.w=q/2
  !CheckEq{xw,-2,"x_word"}
  r2l.l=k/q2
  !CheckEq{r2l,-7,"r2_long_by_quick"}
!EndTestCase

!TestCase{beyond_the_range}
  l.l=100000 : f.f=1e10
  xw.w=l
  !CheckEq{xw,-31072,"x_word_wrap"}
  rl.l=f
  !CheckEq{rl,1410065408,"r_float_wrap"}
!EndTestCase
