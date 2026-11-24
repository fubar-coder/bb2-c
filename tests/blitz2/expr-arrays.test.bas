; expr-arrays - every form of a variable that calcvar reads besides a
; plain one: arrays of one and of three dimensions (Byte to Float, String,
; NewType, pointers), an index that holds an array element, List arrays
; (the current item and an index), NewType fields at offsets 0, 1 to 8 and
; above 8, nested fields, field arrays ([n], at offset 0 and above, of
; pointers, and a [0] tail, which has no bound check), pointer variables
; and pointer fields (followed with \, at offset 0 and above, in a chain,
; and as a value), BCPL pointers (@), the address of a NewType variable
; as a value, UsePath, and arrays in procedures (a local one and a Shared
; global one).
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-3b-arrays): their executable is
; compared with the previous commit's (blitz2_expr_arrays_prev), the
; values here are their results.  An @ variable cannot be read in an
; expression (element: "Syntax Error"), only a field.
; Correct: every case gives the values of its checks.
DEFTYPE .l
NEWTYPE .in
  a.w
  b.l
End NEWTYPE
NEWTYPE .rec
  x.w
  y.l
  z.q
  i.in
  w.w[4]
  *p.rec
  *ps.in[3]
  @bp.in
  s$
End NEWTYPE
NEWTYPE .node
  *nxt.node
  v.l
End NEWTYPE
NEWTYPE .tl
  n.w
  d.b[0]
End NEWTYPE
NEWTYPE .fa
  c.b[4]
  e.w
End NEWTYPE
Function.l locarr{n.l}
  Dim la.l(4,3)
  la(1,2)=n : la(3,1)=n*2
  Function Return la(1,2)+la(3,1)
End Function
Function.l locrec{n.l}
  Dim lr.rec(3)
  lr(2)\i\b=n : lr(1)\w[2]=n+1
  Function Return lr(2)\i\b+lr(1)\w[2]
End Function
!TestBatchInit
  Dim ga.w(5)
!EndTestBatchInit

Function.l sharr{i.l}
  Shared ga()
  ga(i)=ga(i)+1
  Function Return ga(i)
End Function

!TestCase{arrays}
  Dim ab.b(5),aw.w(5),al.l(5),aq.q(5),af.f(5),as$(5)
  i=3
  ab(i)=-5 : aw(i)=300 : al(i)=70000 : aq(i)=1.5 : af(i)=2.5 : as$(i)="ab"
  !CheckEq{ab(3),-5,"byte"}
  !CheckEq{aw(i),300,"word"}
  !CheckEq{al(i)+aw(3),70300,"long"}
  !CheckQEq{aq(i),1.5,"quick"}
  !CheckFltNear{af(i)*2,5,0.0001,"float"}
  !CheckStrEq{as$(i)+"c","abc","string"}
  q.q=aw(i)+aq(3) : !CheckQEq{q,301.5,"word_quick"}
  aw(1)=3
  !CheckEq{al(aw(1)),70000,"index_element"}
  !CheckEq{&aw(4)-&aw(2),4,"step"}
!EndTestCase

!TestCase{multi}
  Dim m.w(3,4,5),n.l(2,6)
  i=1 : j=2 : k=3
  m(i,j,k)=77 : m(3,4,5)=-2 : n(2,6)=123456 : n(i,j)=9
  !CheckEq{m(1,2,3),77,"three"}
  !CheckEq{m(i+2,j*2,k+2),-2,"three_expr"}
  !CheckEq{n(2,6)+n(i,j),123465,"two"}
  !CheckEq{&m(1,0,0)-&m(0,0,0),2,"first_step"}
  !CheckEq{&m(0,1,0)-&m(0,0,0),8,"second_step"}
  !CheckEq{&m(0,0,1)-&m(0,0,0),40,"third_step"}
!EndTestCase

!TestCase{lists}
  Dim List li.w(10)
  s=0
  For i=1 To 3
    If AddItem(li()) Then li()=i*10
  Next
  ResetList li()
  While NextItem(li())
    s=s+li()
  Wend
  !CheckEq{s,60,"current"}
  Dim List lr.rec(4)
  If AddItem(lr()) Then lr()\y=5 : lr()\i\b=6
  !CheckEq{lr()\y+lr()\i\b,11,"current_field"}
  !CheckEq{&li(2)-&li(1),10,"index_step"}
  !CheckEq{&lr(1)\y-&lr(0)\y,SizeOf .rec+8,"index_field_step"}
!EndTestCase

!TestCase{newtype_fields}
  v.rec\x=1 : v\y=2 : v\z=0.25 : v\i\a=3 : v\i\b=4 : v\s="ok"
  !CheckEq{v\x+v\y,3,"offset_0_2"}
  !CheckQEq{v\z,0.25,"offset_6"}
  !CheckEq{v\i\a*10+v\i\b,34,"nested"}
  !CheckStrEq{v\s,"ok","string"}
  !CheckEq{&v\i\b-&v,12,"nested_offset"}
  a1=v : !CheckEq{a1,&v,"address"}
  v\w[0]=7 : j=3 : v\w[j]=9
  !CheckEq{v\w[0]+v\w[j],16,"field_array"}
  !CheckEq{&v\w[2]-&v,20,"field_array_offset"}
!EndTestCase

!TestCase{newtype_arrays}
  Dim pa.rec(5)
  i=2 : j=1
  pa(i)\x=3 : pa(2)\i\b=4 : pa(i)\w[j]=5 : pa(4)\s="pq"
  !CheckEq{pa(2)\x+pa(i)\i\b,7,"fields"}
  !CheckEq{pa(i)\w[j+0],5,"field_array"}
  !CheckStrEq{pa(4)\s,"pq","string"}
  a1=pa(3) : !CheckEq{a1,&pa(3)\x,"address"}
  !CheckEq{&pa(3)\x-&pa(2)\x,SizeOf .rec,"step"}
  u.rec\y=8
  pa(1)\p=u
  !CheckEq{pa(1)\p\y,8,"pointer_field"}
!EndTestCase

!TestCase{pointers}
  u.rec\x=11 : u\y=12 : u\i\b=13
  *p.rec=u
  !CheckEq{*p\x+*p\y,23,"field"}
  !CheckEq{*p\i\b,13,"nested"}
  !CheckEq{*p,&u,"value"}
  v.rec\p=u
  !CheckEq{v\p\y,12,"field_pointer"}
  !CheckEq{v\p,&u,"field_pointer_value"}
  u\p=v : v\y=21
  !CheckEq{*p\p\y,21,"chain"}
  !CheckEq{v\p\p\p\i\b,13,"chain_long"}
  n1.node\v=1 : n2.node\v=2 : n3.node\v=3
  n1\nxt=n2 : n2\nxt=n3
  !CheckEq{n1\nxt\v,2,"offset_0"}
  !CheckEq{n1\nxt\nxt\v,3,"offset_0_chain"}
  v\ps[1]=&u : j=2 : v\ps[j]=&u
  !CheckEq{v\ps[1]\b,12,"field_array_pointer"}
  !CheckEq{v\ps[j],&u,"field_array_pointer_value"}
  *pw.w=&u\x
  !CheckEq{*pw,&u\x,"simple_pointer"}
  Dim *pp.rec(3)
  *pp(2)=u
  !CheckEq{*pp(2)\y,12,"array_field"}
  !CheckEq{*pp(2),&u,"array_value"}
!EndTestCase

!TestCase{field_tail}
  Dim ta.tl(3)
  ta(1)\n=$1234 : j=1
  !CheckEq{ta(0)\d[j],$34,"tail"}
  !CheckEq{SizeOf .tl,2,"size"}
  f.fa\c[2]=9 : f\e=7
  !CheckEq{f\c[j+1]+f\e,16,"offset_0"}
!EndTestCase

!TestCase{bcpl}
  Dim bi.in(2)
  bi(0)\b=77
  v.rec\bp=&bi(0) LSR 2
  !CheckEq{v\bp\b,77,"field"}
  !CheckEq{v\bp,&bi(0) LSR 2,"value"}
!EndTestCase

!TestCase{path}
  Dim up.rec(3)
  k=2
  UsePath up(k)
  \x=4 : \i\b=6
  !CheckEq{up(2)\x,4,"write"}
  !CheckEq{\x+\i\b,10,"read"}
  UsePath u
  u.rec\y=40
  !CheckEq{\y,40,"var"}
!EndTestCase

!TestCase{procedures}
  !CheckEq{locarr{5},15,"local"}
  !CheckEq{locrec{2},5,"local_newtype"}
  ga(2)=4
  !CheckEq{sharr{2},5,"shared"}
  !CheckEq{ga(2),5,"shared_global"}
!EndTestCase
