; rievallib-lmult-overflow (#639) - PFEvaluate's "*" adds the high words
; of the partial products into the low word of the result: a product that
; does not fit in 32 bits is not its low 32 bits ("1000000 1000000 *" is
; -727379962); "^" multiplies the same way.  Library 121 is the acidlibs'
; polylib on the CD: the cells with the CD's libraries get the acidlibs of
; the variant rwe-1999-eval too (C build, without polylib).
; Correct: the low 32 bits of the product: -727379968, 605032704 and
; 3^21 = 10460353203 -> 1870418611.
DEFTYPE .l

!TestCase{products}
  a=PFEvaluate("1000000 1000000 *")
  b=PFEvaluate("70000 70000 *")
  c=PFEvaluate("3 21 ^")
  !CheckEq{a,-727379968,"million_squared"}
  !CheckEq{b,605032704,"seventy_thousand_squared"}
  !CheckEq{c,1870418611,"three_to_21"}
!EndTestCase
