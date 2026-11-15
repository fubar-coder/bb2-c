; compiler-constdiv-word (#1044) - a Word division by a constant 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A Word division is DIVS.W: the CPU's division trap in the compiler
; (exec's alert "Software Failure", 8000 0005), no result.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
; @fixed #1044
NPrint "before"
a.w=5/0
NPrint a
End
