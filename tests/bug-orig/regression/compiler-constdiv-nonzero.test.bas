; compiler-constdiv-nonzero (#1044) - constant divisions and Mods that the
; fix of #1044 lets through: a divisor that is not 0, of each type (also
; a Long divisor whose low word is 0), nested folds, a dividend 0, a
; #constant, a Long Mod (not folded), and a division by a variable that
; holds 0 in a branch that is not run (only a constant divisor is checked
; at compile time).
; Correct: the program compiles and gives the quotients and remainders.
; A regression test of the fix of #1044: the original already gives this
; (correct) result.

#c=10/3

!TestCase{constdiv_nonzero}
  a.w=7/2
  !CheckEq{a,3,"word_7_2"}
  a.w=-7/2
  !CheckEq{a,-3,"word_minus_7_2"}
  a.w=0/5
  !CheckEq{a,0,"dividend_zero"}
  a.w=(8/2)/(4/2)
  !CheckEq{a,2,"nested_fold"}
  b.b=100/7
  !CheckEq{b,14,"byte_100_7"}
  d.l=7/65536
  !CheckEq{d,0,"long_low_word_zero"}
  d.l=100000/-3
  !CheckEq{d,-33333,"long_negative_divisor"}
  d.l=70001 Mod 7
  !CheckEq{d,1,"long_mod"}
  e.q=7/2
  !CheckQEq{e,3.5,"quick_7_2"}
  f.f=1/4
  !CheckFltNear{f,0.25,0.0001,"float_1_4"}
  f.f=7/0.5
  !CheckFltNear{f,14,0.0001,"float_7_half"}

  !CheckEq{#c,3,"constant"}
  z.w=0
  a=2
  If z<>0 Then a=5/z
  !CheckEq{a,2,"variable_zero_not_run"}
!EndTestCase
