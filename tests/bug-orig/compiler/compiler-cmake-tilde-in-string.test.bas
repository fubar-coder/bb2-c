; compiler-cmake-tilde-in-string (#966) - getchar takes every '~' outside
; a false CNIF block for the start of a constant maker ~expr~ (replaced
; by the decimal value of expr), without testing quoteflag: a '~' inside
; a string literal starts one too. In "J~" of Dc.b, "x~y" and the Data$
; string "d~1" no closing '~' follows, "Illegal Constant Expression";
; "a~1+2~b" compiles with the value put in, a3b. Correct: a '~' inside
; a string literal is an ordinary character, nothing is computed inside
; strings; ~1+2~ outside a string still gives 3.
; The expected strings are built with Chr$(126), so that the macro
; arguments of the checks hold no '~'.
.a1
  Dc.b "J~",0
  Even
.d1
  Data$ "d~1"

!TestCase{tilde_in_string}
  t.l=126
  Restore d1
  Read s$
  a$=Peek$(?a1)
  !CheckStrEq{a$,"J"+Chr$(t),"dc_string"}
  a$="x~y"
  !CheckStrEq{a$,"x"+Chr$(t)+"y","literal"}
  a$="a~1+2~b"
  !CheckStrEq{a$,"a"+Chr$(t)+"1+2"+Chr$(t)+"b","not_computed"}
  !CheckStrEq{s$,"d"+Chr$(t)+"1","data_string"}
  r.l=~1+2~
  !CheckEq{r,3,"constant_maker"}
!EndTestCase
