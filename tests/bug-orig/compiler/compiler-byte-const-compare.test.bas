; compiler-byte-const-compare (#978) - a byte variable is fetched with
; ext.w ($90 becomes $FF90), a constant from 128 to 255 is taken as a
; positive word ($0090), and cmp.w compares the two: the comparison with
; the constant never matches, on either side of the operator.
; Correct: a.b=$90 equals $90 (as it equals -112), $90 equals a, a is not
; different from 144; the assignment of 200 to a byte stays -56; the
; ordering comparisons stay mathematical: a<200 is true for a=$90 (-112)
; and for a=0.
!TestCase{byte_const}
  a.b=$90
  r.l=(a=$90)
  !CheckEq{r,-1,"eq_hex"}
  r=(a=-112)
  !CheckEq{r,-1,"eq_negative"}
  r=($90=a)
  !CheckEq{r,-1,"const_left"}
  r=(a<>144)
  !CheckEq{r,0,"not_different"}
  r=(a<200)
  !CheckEq{r,-1,"less_negative"}
  a=0
  r=(a<200)
  !CheckEq{r,-1,"less_zero"}
  c.b=200
  r=c
  !CheckEq{r,-56,"assign_200"}
!EndTestCase
