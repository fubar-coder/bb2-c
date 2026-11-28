; expr-tsr-shift-float.test/float_left.bas: a Float left operand of LSR is
; refused also under a Float destination
f.f=1.5
rf.f=f LSR 1
NPrint rf
End
