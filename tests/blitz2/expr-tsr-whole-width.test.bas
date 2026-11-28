; expr-tsr-whole-width (#1235, type-system-rework) - a whole-number
; operation is one step wider than its operands (spec compiler/expression-
; types, "A whole-number operation is one step wider than its operands"),
; and the optimizations keep that value ("Optimizations keep the value"):
; Byte to Word, Word to Long, Long stays Long; the unary minus and NOT of
; a whole operand are one step wider.  A narrow destination wraps the
; exact value; a Long result wraps modulo 2^32.
; The original computes a Byte sum in a Byte and a Word sum in a Word
; (b+c -56, w+v -25536 beside no destination); its values are the
; declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{narrow_destination}
  b.b=100 : c.b=100 : w.w=20000 : v.w=20000
  xb.b=b+c
  !CheckEq{xb,-56,"x_byte"}
  yw.w=w+v
  !CheckEq{yw,-25536,"y_word"}
!EndTestCase

; no destination: the exact sums
!TestCase{print_sums}
  b.b=100 : c.b=100 : w.w=20000 : v.w=20000
  NPrint b+c
  NPrint w+v
!EndTestCase

; a Long destination: the exact sum
!TestCase{long_destination}
  w.w=20000 : v.w=20000
  rl.l=w+v
  !CheckEq{rl,40000,"r_long"}
!EndTestCase

!TestCase{unary_minus}
  q=-32768
  xq.q=-q
  !CheckEq{Peek.l(&xq),$7FFFFFFF,"x_quick"}
!EndTestCase

!TestCase{print_unary_minus}
  q=-32768
  NPrint -q
!EndTestCase

!TestCase{not_wide}
  w.w=0
  xw.w=(Not w) LSR 1
  !CheckEq{xw,-1,"x_word"}
!EndTestCase

!TestCase{print_not}
  b.b=0 : w.w=0
  NPrint (Not b) LSR 1
  NPrint (Not w) LSR 1
!EndTestCase

!TestCase{long_wraps}
  l.l=100000
  rl.l=l*l
  !CheckEq{rl,1410065408,"r_long"}
!EndTestCase

!TestCase{byte_sum_destination}
  b1.b=100 : b2.b=100 : b3.b=100
  xb.b=b1+b2+b3
  !CheckEq{xb,44,"x_byte"}
!EndTestCase

!TestCase{print_byte_sum}
  b1.b=100 : b2.b=100 : b3.b=100
  NPrint b1+b2+b3
!EndTestCase
