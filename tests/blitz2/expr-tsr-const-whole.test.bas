; expr-tsr-const-whole (#1235, type-system-rework) - a whole constant takes
; its smallest type (spec compiler/expression-types, "A whole constant
; takes its smallest type"): a Byte from -128 to 127, a Word from -32768 to
; 32767, else a Long; a minus before a constant is the unary minus, one
; step wider (-8 a Word, so -8 LSR 1 is the Word shift 32764; the spec says
; "as the original", but blitz2 1.95 prints 2147483644, a declared
; deviation).  DEFTYPE has no influence (expr-tsr-deftype).
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_constants_in_a_sum}
  NPrint 100+100
  NPrint 20000+20000
!EndTestCase

!TestCase{print_negated_constant}
  NPrint -8 LSR 1
!EndTestCase

; the same constants under a destination
!TestCase{sums_under_destinations}
  rl.l=100+100
  !CheckEq{rl,200,"r_byte_sum"}
  rl=20000+20000
  !CheckEq{rl,40000,"r_word_sum"}
  rw.w=-8 LSR 1
  !CheckEq{rw,32764,"r_negated_word"}
!EndTestCase
