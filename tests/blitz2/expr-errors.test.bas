; expr-errors - the first of two errors in one expression: the data
; sources of expr-errors.test/ each have two errors in one expression (an
; operator a type has none of before an unknown name and before a missing
; bracket, a number under a String type so far before a missing bracket,
; an error in the parameter of a Function or a library function before
; one after it, a conversion that does not exist before an unknown name, a
; division of constants by zero before an unknown name); each compile
; ends with the first one, message and line (asserted in
; expr-errors.test.toml).  The case here compiles the forms without the
; errors.
; Correct: each compile reports the first error; the case gives its values.
DEFTYPE .l
Function.l f2{a.l,b.l}
  Function Return a+b
End Function

!TestCase{without_errors}
  a$ = "x"
  b.l = Len(a$)*2+f2{1,2}
  !CheckEq{b,5,"sum"}
  b = f2{Len(a$)*2,f2{3,4}}
  !CheckEq{b,9,"params"}
!EndTestCase
