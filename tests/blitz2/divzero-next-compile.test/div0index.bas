; divzero-next-compile.test/div0index.bas: a division of two Word constants
; by zero in a later array index ("Division by Zero")
Dim m.w(9,9)
i.w=m(1,6/0)
NPrint i
End
