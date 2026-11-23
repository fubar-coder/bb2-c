; compiler-operand-beyond-d7-const (#1185) - as compiler-operand-beyond-d7-arg7
; (a+b*c as the 7th argument: the right operand of the addition beyond
; d7), with constants: eval5 (blitz2.s:9007) folds an operation of two
; constants by running the code it has just generated in its own process,
; and that code holds the register field "d8": the compiler crashes while
; compiling (guru 8000 0004), no result (hence a session of its own).
; Correct: the compile error "Expression too Complex" at the line, no
; code run.
; @fixed-orig #1185
; @pass timeout=30
NPrint "before"
NPrint "hit(1+5*6) ",RectsHit(20,0,10,10,0,0,1+5*6,10)
End
