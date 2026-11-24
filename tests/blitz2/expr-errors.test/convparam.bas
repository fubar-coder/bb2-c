; expr-errors.test/convparam.bas: a String for a Word parameter (no
; conversion) before an unknown Function in the second parameter
Function.l f2w{a.w,b.w}
  Function Return a+b
End Function
a$ = "x"
b.l = f2w{a$,nofunc{}}
NPrint b
End
