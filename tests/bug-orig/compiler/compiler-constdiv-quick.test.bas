; compiler-constdiv-quick (#1044) - a Quick division by a constant 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A Quick division calls ldivlib, which gives $7FFFFFFF for a divisor 0
; without a trap: the program compiles and prints the largest Quick, a
; division by zero the source states and nobody is told of.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
!TestCase{constdiv_quick}
  e.q=7/0
  !CheckEq{e,32767,"after_division"}
!EndTestCase
