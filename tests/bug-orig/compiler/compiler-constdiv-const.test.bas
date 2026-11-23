; compiler-constdiv-const (#1044) - a constant defined by a division by 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A #constant is evaluated as a Long (evalconst2); ldivlib gives
; $7FFFFFFF for a divisor 0 without a trap: the program compiles and
; prints 2147483647.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
#c=5/0

!TestCase{constdiv_const}
  v.l=#c
  !CheckEq{v,2147483647,"const_value"}
!EndTestCase
