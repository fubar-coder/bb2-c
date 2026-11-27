; chr-float - character literals with a float target (spec
; ab3-compat/strings): the long of the characters converted to float
; (AmiBlitz3 writes the long unconverted: chr-float.test.toml). The case
; prints the Float (asserted in chr-float.test.toml).
OPTIMIZE 4
!TestCase{chr_float}
  f.f = @"AB"
  NPrint f
  f = @"A"
  NPrint f
!EndTestCase
