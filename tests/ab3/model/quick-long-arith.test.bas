; quick-long-arith - a Quick under a Long type so far (#1178): the port
; computes with the fraction and rounds at the operator (+ - * down, /
; towards zero), in AB3 mode too; AmiBlitz3 v3.14.0 cuts the Quick first
; (quick-long-arith.test.toml).
!TestCase{quick_long_arith}
  q.q=1.5 : p.q=2.75 : n.q=-1.5 : l.l=10 : z.q=30000.5 : big.l=100000
  r.l=q*2
  !CheckEq{r,3,"l1"}
  r=l-q
  !CheckEq{r,8,"l2"}
  r=l*q
  !CheckEq{r,15,"l3"}
  r=l/n
  ; before: -6 (towards zero); type-system-rework (#1235, task 4.2): down
  !CheckEq{r,-7,"l4"}
  r=q+p
  !CheckEq{r,4,"l5"}
  r=q*p
  !CheckEq{r,4,"l6"}
  r=-q
  !CheckEq{r,-2,"l7"}
  r=big*z
  !CheckEq{r,-1294917296,"l8"}
!EndTestCase
