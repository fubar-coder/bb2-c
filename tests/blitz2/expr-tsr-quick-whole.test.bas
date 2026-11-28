; expr-tsr-quick-whole (#1235, type-system-rework) - a Quick used as a
; whole number keeps working (spec compiler/expression-types, "A Quick
; used as a whole number keeps working", modified): booleans in Quicks,
; bit operators and Not on Quicks give today's values (a Quick operand of
; a bit operator or Not is rounded down once where the operator takes it;
; a comparison's boolean in arithmetic is -1 or 0); a shift of a Quick
; keeps its fraction bits.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{flags_and_masks}
  h=-1
  h=Not h
  !CheckQEq{h,0,"h_not"}
  q=3
  rl.l=0
  If (q AND 1)=1 Then rl=1
  !CheckEq{rl,1,"and_whole"}
  q=1.5
  r2l.l=0
  If (q AND 1)=1 Then r2l=1
  !CheckEq{r2l,1,"and_fraction"}
  flag=-1
  xw.w=(flag=-1)*5
  !CheckEq{xw,-5,"x_boolean_times"}
  xv=10 : av=2 : bv=1
  r3l.l=xv-(av>bv)
  !CheckEq{r3l,11,"r3_boolean_minus"}
  q=7
  r4l.l=0
  If q Mod 2=1 Then r4l=1
  !CheckEq{r4l,1,"mod_true"}
!EndTestCase

!TestCase{shifts_of_a_quick_under_a_long}
  q=1.5
  rl.l=q LSL 2
  !CheckEq{rl,6,"r_lsl"}
  r2l.l=q ASL 4
  !CheckEq{r2l,24,"r2_asl"}
!EndTestCase

!TestCase{print_a_boolean_times_a_long}
  l.l=100000 : av=2 : bv=1
  NPrint l*(av>bv)
  NPrint (av>bv)*100000
!EndTestCase

!TestCase{a_48_16_operand_of_a_bit_operator_or_shift}
  q=1.5 : b.b=3 : l.l=100000
  rl.l=(q+40000) AND b
  !CheckEq{rl,1,"r_and"}
  r2l.l=(l+q) LSL 1
  !CheckEq{r2l,200003,"r2_lsl"}
!EndTestCase

!TestCase{a_long_and_a_quick_under_and}
  l.l=65536 : q=-1
  rl.l=0
  If q AND l Then rl=1
  !CheckEq{rl,1,"quick_first"}
  r2l.l=0
  If l AND q Then r2l=1
  !CheckEq{r2l,1,"long_first"}
!EndTestCase
