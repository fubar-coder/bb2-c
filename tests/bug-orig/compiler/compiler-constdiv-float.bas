; compiler-constdiv-float (#1044) - a Float division by a constant 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A Float division calls mathffp's SPDiv, which raises the CPU's
; division trap for a divisor 0: in the compiler (exec's alert "Software
; Failure", 8000 0005), no result.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
; @fixed #1044
NPrint "before"
f.f=7/0
NPrint f
End
