; vallib-sgn-float (#301) - Sgn of a float calls mathffp's SPTst, which
; tests d1, not d0 - and d1 holds the type word (5) of the argument: Sgn
; of every float is 1.
; Correct: -1, 0 and 1 for a negative float, 0 and a positive float.
DEFTYPE .l

!TestCase{sgn_of_float}
  f.f=-2.5 : a=Sgn(f)
  f=0 : b=Sgn(f)
  f=2.5 : c=Sgn(f)
  f=-0.001 : d=Sgn(f)
  !CheckEq{a,-1,"negative"}
  !CheckEq{b,0,"zero"}
  !CheckEq{c,1,"positive"}
  !CheckEq{d,-1,"small_negative"}
!EndTestCase
