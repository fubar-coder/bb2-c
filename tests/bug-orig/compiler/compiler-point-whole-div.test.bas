; compiler-point-whole-div (#1231) - a constant with a point that has no
; fraction (2.0) is read as a whole number in a division: NPrint 3/2.0
; prints 1 and NPrint 7/2.0+0.5 prints 3, the division of two whole
; numbers cut towards zero.  The compiler's constant fetch (const_fetch)
; keeps "a point, but no fraction" as the whole number 2.
; Correct (spec compiler/expression-types, "A constant with a point is
; read by its operands"): a constant with a point is never whole; 2.0 is
; exact in 16.16, so a Quick, and 3/2.0 is the Quick quotient 1.5 and
; 7/2.0+0.5 is 4.  The original's values are the declared deviations.
!TestCase{prints}
  NPrint 3/2.0
  NPrint 7/2.0+0.5
!EndTestCase
