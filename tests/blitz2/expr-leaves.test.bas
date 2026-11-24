; expr-leaves - the simple elements of an expression, each under the
; types so far it can be read under: decimal, $ and % literals (with a
; point, an exponent, a minus sign), #constants, SizeOf of a type, of a
; field and of a variable, Pi, On and Off, scalar variables of each type,
; a pointer, NewType fields, a string variable, &var, ?label (forward and
; back), Used, Maximum and Addr.  The cases pin the code of these elements
; for the restructuring of the evaluator (#1180, change
; expr-tree-3a-leaves): their executable is compared with the previous
; commit's (compiler_prev), the values here are their results.  ErrNum
; ("Ern") has no name in the token list: no source text reaches it.
; Correct: every case gives the values of its checks.
#k=5
#big=100000
#cthree=3
#cneg=-3
NEWTYPE .pt
  x.w
  y.l
  z.q
  f.f
End NEWTYPE

!TestCase{literals}
  b.b=100 : !CheckEq{b,100,"byte"}
  b=-$10 : !CheckEq{b,-16,"byte_hex"}
  b=2.0+1 : !CheckEq{b,3,"byte_point"}
  w.w=1000+$10+%101 : !CheckEq{w,1021,"word"}
  w=200 : !CheckEq{w,200,"word_200"}
  l.l=100000 : !CheckEq{l,100000,"long"}
  l=1e2 : !CheckEq{l,100,"long_exp"}
  l=-%11 : !CheckEq{l,-3,"long_bin"}
  q.q=1.5 : !CheckQEq{q,1.5,"quick"}
  q=-.25 : !CheckQEq{q,-0.25,"quick_frac"}
  q=30000 : !CheckQEq{q,30000,"quick_word"}
  f.f=.25*4 : !CheckFltNear{f,1,0.0001,"float"}
  f=3 : !CheckFltNear{f,3,0.0001,"float_int"}
  r.l=0
  If 1.5>1 Then r=1
  !CheckEq{r,1,"compare"}
!EndTestCase

!TestCase{constants}
  b.b=#k : !CheckEq{b,5,"byte"}
  w.w=#k*-#cthree : !CheckEq{w,-15,"word"}
  l.l=#big+#k : !CheckEq{l,100005,"long"}
  q.q=#k/2 : !CheckQEq{q,2.5,"quick"}
  f.f=#k : !CheckFltNear{f,5,0.0001,"float"}
  l=#cneg : !CheckEq{l,-3,"negative"}
  q=#k+0.5 : !CheckQEq{q,5.5,"fraction"}
  f=#cneg*0.5 : !CheckFltNear{f,-1.5,0.0001,"fraction_float"}
!EndTestCase

!TestCase{sizes}
  v.pt\x=1
  l.l=SizeOf .pt : !CheckEq{l,14,"type"}
  l=SizeOf .pt\y : !CheckEq{l,2,"field"}
  w.w=SizeOf .pt\f : !CheckEq{w,10,"field_word"}
  q.q=SizeOf .pt : !CheckQEq{q,14,"type_quick"}
  f.f=SizeOf .pt\z : !CheckFltNear{f,6,0.0001,"field_float"}
!EndTestCase

!TestCase{pi_on_off}
  f.f=Pi : !CheckFltNear{f,3.14159,0.0001,"pi"}
  q.q=Pi : !CheckFltNear{q,3.14159,0.0001,"pi_quick"}
  l.l=Pi : !CheckEq{l,3,"pi_long"}
  b.b=On : !CheckEq{b,-1,"on_byte"}
  w.w=Off : !CheckEq{w,0,"off_word"}
  l=On : !CheckEq{l,-1,"on_long"}
  q=Off : !CheckQEq{q,0,"off_quick"}
  f=Off : !CheckFltNear{f,0,0.0001,"off_float"}
!EndTestCase

!TestCase{variables}
  vb.b=-2 : vw.w=300 : vl.l=70000 : vq.q=1.25 : vf.f=2.5
  w.w=vb : !CheckEq{w,-2,"byte_word"}
  l.l=vb+vw : !CheckEq{l,298,"byte_long"}
  l=vw*vl : !CheckEq{l,21000000,"word_long"}
  q.q=vb+vq : !CheckQEq{q,-0.75,"byte_quick"}
  q=vw : !CheckQEq{q,300,"word_quick"}
  f.f=vq+vf : !CheckFltNear{f,3.75,0.0001,"quick_float"}
  f=vl : !CheckFltNear{f,70000,0.01,"long_float"}
  b.b=vb : !CheckEq{b,-2,"byte"}
  l=vq*2 : !CheckEq{l,2,"quick_long"}
  r.l=0
  If vq>1 Then r=1
  !CheckEq{r,1,"compare"}
  p.pt\x=7 : p\y=-9 : p\z=0.5 : p\f=1.5
  l=p\x+p\y : !CheckEq{l,-2,"fields"}
  q=p\z+p\x : !CheckQEq{q,7.5,"field_quick"}
  f=p\f*p\z : !CheckFltNear{f,0.75,0.0001,"field_float"}
  *pp.pt=p
  l=*pp\y : !CheckEq{l,-9,"pointer_field"}
  !CheckEq{*pp,&p,"pointer"}
  a$="abc"
  !CheckStrEq{a$+"d","abcd","string"}
!EndTestCase

!TestCase{addresses}
  aw.w=$1234
  !CheckEq{Peek.w(&aw),$1234,"var"}
  a$="xyz"
  !CheckEq{Peek.b(&a$),Asc("x"),"string"}
  t.pt\y=55
  !CheckEq{Peek.l(&t\y),55,"field"}
!EndTestCase

!TestCase{labels}
.back
  la.l=?back
  lb.l=?forward
  !Check{la<>0,"back"}
  !Check{lb>la,"forward"}
  Goto skip
.forward
  la=0
.skip
!EndTestCase

!TestCase{objects}
  BitMap 1,16,8,1
  Use BitMap 1
  !CheckEq{Used BitMap,1,"used"}
  w.w=Used BitMap+1 : !CheckEq{w,2,"used_word"}
  m.l=Maximum BitMap
  !Check{m>1,"maximum"}
  q.q=Maximum BitMap : !CheckQEq{q,m,"maximum_quick"}
  !Check{Addr BitMap(1)<>0,"addr"}
  !CheckEq{Addr BitMap(1)-Addr BitMap(0),Addr BitMap(2)-Addr BitMap(1),"addr_step"}
!EndTestCase
