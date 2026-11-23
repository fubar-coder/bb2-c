; vallib-val-decimal-places (#304) - getdec computes the divisor of the
; fractional part (10^n) as a 32 bit integer: with 32 decimal places it is
; 0 mod 2^32 and the fractional part is dropped.
; Correct: Val of 1.5 written with 32 decimal places gives 1.5.
DEFTYPE .l

!TestCase{decimal_places}
  v.f=Val("1.50000000000000000000000000000000")
  !CheckFltNear{v,1.5,0.001,"value"}
!EndTestCase
