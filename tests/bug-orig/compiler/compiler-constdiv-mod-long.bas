; compiler-constdiv-mod-long (#1044) - a Long Mod by a constant 0.
; When both operands of / or Mod are constants, blitz2 folds the operation
; by running the code it has just generated in its own process (eval5,
; blitz2.s:9007; evalconst2 for #constants), without looking at the
; divisor.
; A Long Mod is not folded (domodl, blitz2.s:9455): the program
; computes it, modlib gives the dividend for a divisor 0 and the program
; prints 7.
; Correct: the compile error "Division by Zero" at the line, for every
; type.
; @fixed-orig #1044
NPrint "before"
d.l=7 Mod 0
NPrint d
End
