; lmullib-cross-swap (#199) - the Long and Quick multiplication of the
; 68000 (lmullib) adds the partial products SWAPped instead of shifted by
; 16 bits: when the product does not fit in 32 bits, the high word of a
; partial product also lands in the low word of the result, instead of
; the product being cut to 32 bits.
; Correct: the product modulo 2^32: 196608*30000 = 1603272704,
; -196608*30000 = -1603272704, the Quick 300*300 = $5F900000.
DEFTYPE .l

!TestCase{long_product}
  a=196608 : b=30000
  !CheckEq{a*b,1603272704,"positive"}
  a=-196608
  !CheckEq{a*b,-1603272704,"negative"}
!EndTestCase

!TestCase{quick_product}
  c.q=300 : d.q=300 : e.q=c*d
  !CheckEq{Peek.l(&e),$5F900000,"positive"}
  c=-300 : e=c*d
  !CheckEq{Peek.l(&e),$A0700000,"negative"}
!EndTestCase
