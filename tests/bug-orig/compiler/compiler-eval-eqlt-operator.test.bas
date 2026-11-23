; compiler-eval-eqlt-operator (#9) - eval2 reads "=" and then "<" and falls
; into .isne (blitz2.s:9133): "=<" is compiled as "<>" instead of
; less-or-equal. "<=", ">=", "=>", "<>" and "><" are right.
; Correct: "=<" is less-or-equal, as "<=".
!TestCase{eqlt}
  a=1:b=2:r.l=(a=<b)
  !CheckEq{r,-1,"less"}
  a=2:b=2:r=(a=<b)
  !CheckEq{r,-1,"equal"}
  a=3:b=2:r=(a=<b)
  !CheckEq{r,0,"greater"}
  x.f=1.5:y.f=1.5:r=(x=<y)
  !CheckEq{r,-1,"float_equal"}
  r=(3=<2)
  !CheckEq{r,0,"constants"}
  For i=1 To 3
    If i=<2 Then r=-1 Else r=0
    If i=1 Then r1.l=r
    If i=2 Then r2.l=r
    If i=3 Then r3.l=r
  Next
  !CheckEq{r1,-1,"if_1"}
  !CheckEq{r2,-1,"if_2"}
  !CheckEq{r3,0,"if_3"}
!EndTestCase
