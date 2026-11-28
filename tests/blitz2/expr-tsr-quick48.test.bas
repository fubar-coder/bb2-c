; expr-tsr-quick48 (#1235, type-system-rework) - an operation with a Quick
; operand is computed in 48.16 (spec compiler/expression-types, "An
; operation with a Quick is computed in 48.16"): both operands are
; converted to 48.16 and the exact result computed there; a product or
; quotient is cut down to 1/65536; a 48.16 value wraps beyond 48 integer
; bits, so no intermediate leaves its format at the Quick range.
; The original computes in 16.16 and wraps an intermediate beyond the Quick
; range (q+20000 -25536, q*p*r 6116); its values are the declared
; deviations.
; Correct: the values of the spec's scenarios.
!TestCase{products_beyond_the_range}
  q=300 : p=300 : r=0.25
  xq.q=q*p*r
  !CheckQEq{xq,22500,"x_product"}
  yq.q=r*q*p
  !CheckQEq{yq,22500,"y_product"}
!EndTestCase

!TestCase{print_sum_beyond_the_range}
  q=20000
  NPrint q+20000
!EndTestCase

; the cut of a product under a Long: down, not towards zero
!TestCase{cut_under_long}
  q=-0.5 : p=1/65536
  rl.l=q*p
  !CheckEq{rl,-1,"r_long"}
!EndTestCase
