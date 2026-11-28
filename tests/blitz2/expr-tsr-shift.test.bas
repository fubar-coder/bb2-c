; expr-tsr-shift (#1235, type-system-rework) - a shift keeps the type of its
; left operand (spec compiler/expression-types, "A shift keeps the type of
; its left operand"): a Quick left operand keeps its fraction bits (LSL,
; ASL, ASR in 48.16, LSR on the 16.16 bits); a whole left operand is
; shifted as a whole number (LSR, ASR in its own width, LSL, ASL as a
; Long); the count is whole (a Quick rounded down) and does not make the
; left operand fractional.  The shift exception under a Quick destination
; is expr-tsr-shift-top, the count forms expr-tsr-shift-count.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{a_quick_under_a_long}
  q=1.5
  rl.l=q LSL 2
  !CheckEq{rl,6,"r_long"}
!EndTestCase

!TestCase{print_a_quick_right}
  q=3
  NPrint q LSR 1
!EndTestCase

!TestCase{print_whole_beside_a_quick}
  qa=1.25 : ba.b=3 : bb.b=3
  NPrint qa+3 LSR 3
  NPrint qa+ba LSR bb
!EndTestCase

!TestCase{whole_shift_under_a_long}
  qa=1.25
  xl.l=qa+3 LSR 3
  !CheckEq{xl,1,"x_long"}
!EndTestCase

!TestCase{print_a_quick_count}
  x.w=3 : h=2.5
  NPrint x LSR h
!EndTestCase

!TestCase{whole_widths}
  w.w=-3
  xw.w=w LSR 3
  !CheckEq{xw,8191,"x_word_lsr"}
  rl.l=1 LSL 17
  !CheckEq{rl,131072,"r_long_lsl"}
!EndTestCase

!TestCase{print_whole_widths}
  b.b=-8 : w2.w=$1000
  NPrint b LSR 1
  NPrint w2 LSL 4
!EndTestCase

!TestCase{print_quick_in_48_16}
  q=20000
  NPrint q LSL 1
!EndTestCase
