; compiler-expr-depth-d7 (#1185) - expressions that reach d7 and stay
; there compile and give the right values (the fix of #1185 refuses only
; an operand beyond d7): an argument from the 7th on is evaluated in d6,
; its right operand in d7; inside six brackets the innermost left operand
; is in d6 as well.  A chain of one precedence, the tighter operator
; first, brackets around the right part, a comparison and a 6th argument
; (in d5).
; Correct: the program compiles and gives the values of the checks.

!TestCase{d7_expressions}
  a.l=1 : b.l=5 : c.l=6 : d.l=31 : aw.w=1 : bw.w=5 : cw.w=6
  k.l=-1 : m.l=3 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6
  !CheckEq{RectsHit(20,0,10,10,0,0,b*c+a,10),-1,"b_mul_c_plus_a"}
  !CheckEq{RectsHit(20,0,10,10,0,0,a+(b*c),10),-1,"a_plus_bracket"}
  !CheckEq{RectsHit(20,0,10,10,0,0,a+b+c+d,10),-1,"a_plus_b_plus_c_plus_d"}
  !CheckEq{RectsHit(20,0,10,10,0,0,d-a+a,10),-1,"d_minus_a_plus_a"}
  !CheckEq{RectsHit(20,0,10,10,0,0,bw*cw+aw,10),-1,"word_mul_plus"}
  !CheckEq{RectsHit(20,0,10,10,0,0,5*6+1,10),-1,"constants"}
  !CheckEq{RectsHit(0,20,10,10,0,0,10,b*c+a),-1,"eighth_argument"}
  !CheckEq{RectsHit(20,31,10,10,0,a+b*c,40,10),-1,"sixth_argument"}
  r.l=c1+(c2+(e+(h+(i+(j+(m>l))))))
  !CheckEq{r,20,"brackets_m_gt_l"}
  r=c1+(c2+(e+(h+(i+(j+(k+m+l))))))
  !CheckEq{r,25,"brackets_k_plus_m_plus_l"}
  r=c1+(c2+(e+(h+(i+(j+(m*l+k))))))
  !CheckEq{r,26,"brackets_m_mul_l_plus_k"}
  r=c1+(c2+(e+(h+(i+(j+(k+(m AND l)))))))
  !CheckEq{r,22,"brackets_k_plus_and"}
  r=c1+(c2+(e+(h+(i+(j+(1+2+3))))))
  !CheckEq{r,27,"brackets_constants"}
  r=c1+(c2+(e+(h+(i+(j+(k-(m-l)))))))
  !CheckEq{r,19,"brackets_k_minus_bracket"}
!EndTestCase
