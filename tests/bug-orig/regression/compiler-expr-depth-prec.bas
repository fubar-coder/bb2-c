; compiler-expr-depth-prec (#1080) - one parenthesis more than the
; precedence stack takes (64), compiled with the shell's default stack
; (4000 bytes) on Kickstart 1.3; the line is built with macros (a TED line
; holds 128 characters).
; Correct: the compile error "Precedence Stack Overflow".  Before the fix
; of #1080 the compile overran the stack before it got there.
; @machine a500
; @pass stack=4000
Macro o
((((((((
End Macro
Macro c
))))))))
End Macro
b.l = 3
b = !o!o!o!o!o!o!o!o b!c!c!c!c!c!c!c!c
NPrint b
End
