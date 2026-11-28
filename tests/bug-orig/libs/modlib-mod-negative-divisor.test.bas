; modlib-mod-negative-divisor (#1197, #1198) - Mod of Byte, Word and Long
; variables (modlib's domodbyte, domodword, domodlong; the Long by its DIVU
; path and by its ldivide/lmultiply path) gives the remainder of the
; division towards zero plus the divisor when that remainder is negative:
; with a negative divisor the result leaves the divisor's range (-7 Mod -3
; is -4, -700000 Mod -300000 is -400000) and a positive remainder keeps
; its sign (7 Mod -3 is 1).
; Correct (type-system-rework Q2 "Floor"): the floor Mod a-b*floor(a/b),
; the remainder has the divisor's sign: -7 Mod -3 is -1, 7 Mod -3 is -2,
; -700000 Mod -300000 is -100000; with a positive divisor the values stay
; (-7 Mod 3 is 2, #1198).
!TestCase{negative_divisor}
  b1.b=-7 : b2.b=-3 : rb.b=0
  rb=b1 Mod b2
  !CheckEq{rb,-1,"byte_both_negative"}
  w1.w=7 : w2.w=-3 : rw.w=0
  rw=w1 Mod w2
  !CheckEq{rw,-2,"word_positive_dividend"}
  w1=-7
  rw=w1 Mod w2
  !CheckEq{rw,-1,"word_both_negative"}
  w1=-6
  rw=w1 Mod w2
  !CheckEq{rw,0,"word_no_remainder"}
  l1.l=7 : l2.l=-3 : rl.l=0
  rl=l1 Mod l2
  !CheckEq{rl,-2,"long_positive_dividend"}
  l1=-700000 : l2=-300000
  rl=l1 Mod l2
  !CheckEq{rl,-100000,"long_both_negative"}
  l1=700000
  rl=l1 Mod l2
  !CheckEq{rl,-200000,"long_wide_divisor"}
!EndTestCase

!TestCase{positive_divisor}
  w1.w=-7 : w2.w=3 : rw.w=0
  rw=w1 Mod w2
  !CheckEq{rw,2,"word_negative_dividend"}
  w1=7
  rw=w1 Mod w2
  !CheckEq{rw,1,"word_positive"}
  l1.l=-700000 : l2.l=300000 : rl.l=0
  rl=l1 Mod l2
  !CheckEq{rl,200000,"long_negative_dividend"}
!EndTestCase
