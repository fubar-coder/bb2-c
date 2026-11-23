; floatquicklib-min-positive (#147) - the conversion of a Quick to a
; Float tests the sign on the negated value (NEG.l d0:BMI); NEG of
; $80000000 overflows and stays negative, so the smallest Quick, -32768.0,
; is converted as the positive magnitude: +32768.0.
; Correct: -32768.0 gives the Float -32768.0 ($800000D0).
DEFTYPE .l

!TestCase{quick_to_float}
  q.q=-32768 : f.f=q
  !CheckEq{Peek.l(&f),$800000D0,"minimum"}
  q=-32767 : f=q
  !CheckEq{Peek.l(&f),$FFFE00CF,"next"}
!EndTestCase
