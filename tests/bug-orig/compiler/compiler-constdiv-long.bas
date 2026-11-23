; compiler-constdiv-long (#1044) - a Long division by a constant 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A Long division calls ldivlib, which gives $7FFFFFFF for a divisor 0
; without a trap: the program compiles and prints 2147483647, a division
; by zero the source states and nobody is told of.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
; @fixed-orig #1044
NPrint "before"
d.l=7/0
NPrint d
End
