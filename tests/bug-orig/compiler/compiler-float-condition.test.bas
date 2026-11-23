; compiler-float-condition (#1187) - If, While and Until on a Float value
; test the value with mathffp SPTst (jsr -48(a6), blitz2.s:12966
; thetst5), which takes its argument in d1; the compiler has the value in
; d0 and does not copy it (doif2, blitz2.s:13299), so the branch follows
; what the code before left in d1.  The program sets d1 itself before
; every condition (x=a+b leaves b=0 in d1, x=a+c leaves c=7), each time
; to the value that makes the original take the wrong branch, so the
; result does not depend on the library set.  Correct: a Float condition
; is true when the value is not 0, whatever is in d1, like the other
; types.  Comparisons and Quick conditions are tested with tst and stay
; right (q-0.5 is a Quick condition, in the original and in the port).
!TestCase{float_condition}
  r.l=0
  a.l=5 : b.l=0 : c.l=7 : e.l=0 : x.l=0 : n.l=0
  f.f=1.5 : z.f=0 : m.f=-2.5 : q.q=0.5 : qz.q=0
  x=a+b : If f Then r=-1 Else r=0
  !CheckEq{r,-1,"c01"}
  x=a+c : If z Then r=-1 Else r=0
  !CheckEq{r,0,"c02"}
  x=a+b : If m Then r=-1 Else r=0
  !CheckEq{r,-1,"c03"}
  x=a+b : If 0.5 Then r=-1 Else r=0
  !CheckEq{r,-1,"c04"}
  x=a+c : If 0.0 Then r=-1 Else r=0
  !CheckEq{r,0,"c05"}
  x=a+c : If 1.5-1.5 Then r=-1 Else r=0
  !CheckEq{r,0,"c06"}
  x=a+b : If 1.5+1.5 Then r=-1 Else r=0
  !CheckEq{r,-1,"c07"}
  x=a+b : If f+z Then r=-1 Else r=0
  !CheckEq{r,-1,"c08"}
  x=a+c : If f-f Then r=-1 Else r=0
  !CheckEq{r,0,"c09"}
  x=a+c : If z*f Then r=-1 Else r=0
  !CheckEq{r,0,"c10"}
  x=a+c : If Abs(z) Then r=-1 Else r=0
  !CheckEq{r,0,"c11"}
  x=a+b : If Abs(m) Then r=-1 Else r=0
  !CheckEq{r,-1,"c12"}
  x=a+b : If -f Then r=-1 Else r=0
  !CheckEq{r,-1,"c13"}
  x=a+b
  If f
    r=-1
  Else
    r=0
  EndIf
  !CheckEq{r,-1,"c14"}
  x=a+c
  If z
    r=-1
  Else
    r=0
  EndIf
  !CheckEq{r,0,"c15"}
  x=a+c : If f<>0 Then r=-1 Else r=0
  !CheckEq{r,-1,"c16"}
  x=a+c : If z<>0 Then r=-1 Else r=0
  !CheckEq{r,0,"c17"}
  x=a+b : If z=0 Then r=-1 Else r=0
  !CheckEq{r,-1,"c18"}
  x=a+b : If f=z Then r=-1 Else r=0
  !CheckEq{r,0,"c19"}
  x=a+b : If q Then r=-1 Else r=0
  !CheckEq{r,-1,"c20"}
  x=a+c : If qz Then r=-1 Else r=0
  !CheckEq{r,0,"c21"}
  x=a+c : If q-0.5 Then r=-1 Else r=0
  !CheckEq{r,0,"c22"}
  ; While f: 3 rounds (the original: 0, d1=0)
  n=0 : f=1.5
  x=a+b : While f : n+1 : If n=3 Then f=0
  x=a+b : Wend
  !CheckEq{n,3,"c23"}
  ; While z: 0 rounds (the original: until c is 0 at round 3)
  n=0 : z=0 : c=7
  x=a+c : While z : n+1 : If n=3 Then c=0
  x=a+c : Wend
  !CheckEq{n,0,"c24"}
  c=7
  ; Until f with f 0, 0, 1.5: 3 rounds (the original: until e is 7 at round 5)
  n=0 : f=0 : e=0
  Repeat : n+1 : If n=3 Then f=1.5
  If n=5 Then e=7
  x=a+e : Until f
  !CheckEq{n,3,"c25"}
  ; Until z with z 0, 0, 0, 1: 4 rounds (the original: 1, d1=7)
  n=0 : z=0
  Repeat : n+1 : If n=4 Then z=1
  x=a+c : Until z
  !CheckEq{n,4,"c26"}
!EndTestCase
