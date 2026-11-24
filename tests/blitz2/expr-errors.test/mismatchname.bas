; expr-errors.test/mismatchname.bas: a number under the type so far
; String (Type Mismatch) before an unknown Function in the same expression
a$ = "x"
b.l = a$*2+nofunc{}
NPrint b
End
