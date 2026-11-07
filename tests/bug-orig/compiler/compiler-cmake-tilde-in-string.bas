; compiler-cmake-tilde-in-string (#966) - getchar takes every '~' outside
; a false CNIF block for the start of a constant maker ~expr~ (replaced
; by the decimal value of expr), without testing quoteflag: a '~' inside
; a string literal starts one too. In "J~" of Dc.b, "x~y" and the Data$
; string "d~1" no closing '~' follows, "Illegal Constant Expression";
; "a~1+2~b" compiles with the value put in, a3b. Correct: a '~' inside
; a string literal is an ordinary character, nothing is computed inside
; strings; ~1+2~ outside a string still gives 3.
; @fixed #966
Restore d1
Read s$
NPrint Peek$(?a1)
NPrint "x~y"
NPrint "a~1+2~b"
NPrint s$
NPrint ~1+2~
End
a1: Dc.b "J~",0
Even
d1: Data$ "d~1"
