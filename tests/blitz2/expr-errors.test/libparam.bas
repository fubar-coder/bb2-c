; expr-errors.test/libparam.bas: an operator the type has none of in the
; parameter of a library function before an unknown Function after it
a$ = "x"
b.l = Abs(a$*a$)+nofunc{}
NPrint b
End
