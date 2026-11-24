; expr-errors.test/div0name.bas: a division of constants by zero before
; an unknown Function in the same expression
b.l = 1/0+nofunc{}
NPrint b
End
