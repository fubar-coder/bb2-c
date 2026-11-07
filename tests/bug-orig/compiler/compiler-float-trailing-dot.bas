; compiler-float-trailing-dot (#10) - fetchpoint2 wants a digit after the
; decimal point: "1." is a syntax error.
; Correct: "1." is the float 1.
; @fixed #10
a.f=1.
NPrint a
b.f=2.+1
NPrint b
End
