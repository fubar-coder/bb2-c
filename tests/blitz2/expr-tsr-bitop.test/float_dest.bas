; expr-tsr-bitop.test/float_dest.bas: a Float operand of AND is refused
; also under a Float destination
l.l=5
f.f=1.5
rf.f=l AND f
NPrint rf
End
