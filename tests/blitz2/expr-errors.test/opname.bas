; expr-errors.test/opname.bas: an operator the type has none of (String *)
; before an unknown Function in the same expression
a$ = "x"
b.l = a$*a$+nofunc{}
NPrint b
End
