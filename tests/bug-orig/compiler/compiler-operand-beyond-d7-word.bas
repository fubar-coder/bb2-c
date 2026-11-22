; compiler-operand-beyond-d7-word (#1185) - as compiler-operand-beyond-d7-arg7
; (a+b*c as the 7th argument: the right operand of the addition beyond
; d7), with Word operands: the multiplication muls.w gets "d8" as its
; source field, which reads as the mode "address register" (muls a0,d7),
; an illegal instruction: the compiled program crashes (Software
; Failure).
; Correct: the compile error "Expression too Complex" at the line, no
; wrong code.
; @fixed #1185
aw.w=1 : bw.w=5 : cw.w=6
NPrint "before"
NPrint "hit(aw+bw*cw) ",RectsHit(20,0,10,10,0,0,aw+bw*cw,10)
End
