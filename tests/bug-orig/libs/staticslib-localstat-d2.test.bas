; staticslib-localstat-d2 (#1063) - a procedure with a local NEWTYPE
; variable calls localstat in its prologue, while the parameters are still
; in d0-d5.  The original localstat changes d2 (the third parameter) and d4
; (the fifth, and the DBF count that a call with three or four parameters
; does not save).
; Correct: every parameter keeps its value.
DEFTYPE .l
NEWTYPE .nt
  a.l
  b.l
End NEWTYPE
Statement s3{a.l,b.l,c.l}
  DEFTYPE .nt v
  !CheckEq{a,1,"a"}
  !CheckEq{b,2,"b"}
  !CheckEq{c,3,"c"}
End Statement
Statement s4{a.l,b.l,c.l,d.l}
  DEFTYPE .nt v
  !CheckEq{a,11,"a"}
  !CheckEq{b,12,"b"}
  !CheckEq{c,13,"c"}
  !CheckEq{d,14,"d"}
End Statement
Statement s5{a.l,b.l,c.l,d.l,e.l}
  DEFTYPE .nt v
  !CheckEq{a,21,"a"}
  !CheckEq{b,22,"b"}
  !CheckEq{c,23,"c"}
  !CheckEq{d,24,"d"}
  !CheckEq{e,25,"e"}
End Statement
Function.l f3{a.l,b.l,c.l}
  DEFTYPE .nt v
  Function Return a+b*10+c*100
End Function

!TestCase{parameters}
  s3{1,2,3}
  s4{11,12,13,14}
  s5{21,22,23,24,25}
  !CheckEq{f3{4,5,6},654,"f3"}
!EndTestCase
