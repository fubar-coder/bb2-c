; expr-errors.test/opparam.bas: an operator the type has none of in the
; first parameter of a Function before an unknown Function in the second
Function.l f2{a.l,b.l}
  Function Return a+b
End Function
a$ = "x"
b.l = f2{a$*a$,nofunc{}}
NPrint b
End
