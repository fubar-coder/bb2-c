; compiler-mod-quick-below-one (#1216) - a Quick Mod by a Quick variable
; below 1, and by a constant with a point beside whole numbers.
; BUG(orig) (#1216): the original takes the whole part of a Quick divisor
; for the DIVS.W of Mod: with p=0.5 that is 0, the CPU's division trap at
; run time; b Mod 1.5 with a Byte b divides by the whole part 1 (the
; fraction of the constant is lost): 0 instead of 1.
; Correct (spec compiler/expression-types, "A Quick divisor below 1", "A
; constant with a point beside whole numbers"): the remainder is exact,
; 7.5 Mod 0.5 is 0, 7.75 Mod 0.5 is 0.25, 4 Mod 1.5 is 1.
!TestCase{mod_quick_below_one}
  q.q=7.75 : p.q=0.5 : x.q=1
  x=q Mod p
  !CheckQEq{x,0.25,"quarter"}
!EndTestCase

!TestCase{mod_quick_below_one_zero}
  q.q=7.5 : p.q=0.5 : x.q=1
  x=q Mod p
  !CheckQEq{x,0,"half"}
!EndTestCase

!TestCase{mod_byte_const_fraction}
  b.b=4 : x.q=0
  x=b Mod 1.5
  !CheckQEq{x,1,"quick_dest"}
!EndTestCase

!TestCase{mod_byte_const_fraction_print}
  b.b=4
  NPrint b Mod 1.5
!EndTestCase
