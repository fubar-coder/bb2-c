; ldivlib-div0-check-const (#200, #1044) - a #constant defined by a
; division by 0, with runtime error checking.  The compiler evaluates the
; constant by running the generated code, error checks included; were
; the divisor not refused before (#1044), ldivlib's divchk would raise
; the division trap inside the compiler.
; Correct: the compile error "Division by Zero" at the line.
; A regression test of the ldivlib error check (#200).
; @pass runtime_errors=on
#c=5/0
NPrint #c
End
