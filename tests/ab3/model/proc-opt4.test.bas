; proc-opt4 - AB3 procedures under OPTIMIZE 4 (spec ab3-compat/procedures):
; optional parameters, parameters from the 7th on, FAST, a NewType return
; type. The procedures of a case have its prefix; a value a procedure
; printed is passed in zo$ (a string) or zn (a Long, from a FAST one).
OPTIMIZE 4
!TestBatchInit
  zo$="" : zn.l=0
!EndTestBatchInit
; proc_defstr: an optional string parameter left out is the empty string
Statement pds_s{x.l,@t.s}
  SHARED zo$
  zo$=Str$(x)+" ["+t+"] "+Str$(Len(t))
End Statement
; proc_onlyopt: name{} of a Statement and a Function whose parameters are
; all optional
Statement po_s{@a.l,@t.s}
  SHARED zo$
  zo$="s "+Str$(a)+" ["+t+"] "+Str$(Len(t))
End Statement
Function.l po_f{@a.l,@t.s}
  SHARED zo$
  zo$="f "+Str$(a)+" ["+t+"] "+Str$(Len(t))
  Function Return a
End Function
; proc_optfirst: a parameter without @ behind one with @ is optional too
Statement pof_s{@a.l,b.l}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(b)
End Statement
; proc_optnt: an optional NewType pointer left out is -1
NEWTYPE .pont
x.l
End NEWTYPE
Statement pon_s{a.l,@*p.pont}
  SHARED zo$
  If *p = -1 Then zo$="minus one" Else zo$="other"
End Statement
; proc_ten: ten parameters, of a Statement and of a Function with a word
; and a byte among 7 to 10
Statement p10_s{a.l,b.l,c.l,d.l,e.l,f.l,g.l,h.l,i.l,j.l}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(b)+" "+Str$(c)+" "+Str$(d)+" "+Str$(e)
  zo$=zo$+" "+Str$(f)+" "+Str$(g)+" "+Str$(h)+" "+Str$(i)+" "+Str$(j)
End Statement
Function.l p10_f{a.l,b.l,c.l,d.l,e.l,f.l,g.w,h.b,i.l,j.l}
  Function Return a+b+c+d+e+f+g+h+i+j
End Function
; proc_opt7: optional and string parameters behind the 6th
Statement po7_s{a.l,b.l,c.l,d.l,e.l,f.l,@g.l,@h.w,@t.s}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(f)+" "+Str$(g)+" "+Str$(h)+" ["+t+"] "+Str$(Len(t))
End Statement
; proc_types7: parameters 7 to 10 of every type, given and left out, and
; strings among them (one added from two)
Statement pt_sb{a.l,b.l,c.l,d.l,e.l,f.l,@g.b,@h.w,@i.q,@j.f}
  SHARED zo$
  zo$=Str$(g)+" "+Str$(h)+" "+Str$(i)+" "+Str$(j)
End Statement
Statement pt_ss{a.l,b.l,c.l,d.l,e.l,f.l,@g.s,@h.l,@t.s,@*p.l}
  SHARED zo$
  zo$="["+g+"] "+Str$(h)+" ["+t+"] "+Str$(*p)
End Statement
; proc_expr: AB3 procedure calls in expressions: left-out optional
; parameters and parameters on the stack (7 and 8) in a call that is the
; right operand of an operator (regat above 0), and a string Function with
; a default in a string expression
Function.l px_o{a.l,@b.l,@c.w}
  Function Return a*100+b*10+c
End Function
Function.l px_s{a.l,b.l,c.l,d.l,e.l,f.l,g.l,@h.l}
  Function Return a+g*10+h*100
End Function
Function.s px_t{a.s,@n.l}
  Function Return a+Str$(n)
End Function
; proc_fast: a FAST Function and a FAST Statement
Function.l FAST pf_ff{a.l,b.l}
  c.l = a+b
  Function Return c
End Function
Statement FAST pf_sf{a.l}
  SHARED zn
  zn=a
End Statement
; proc_fastkeep: FAST: the locals are not cleared, c keeps 42 from the
; call before
Statement FAST pfk_sf{a.l}
  SHARED zn
  If a = 1 Then c.l = 42
  zn=c
End Statement
; proc_ntret: a NewType return type: the pointer, assigned to a pointer of
; the NewType
NEWTYPE .pnrt
x.l
End NEWTYPE
DEFTYPE .pnrt one
Function.pnrt pnr_mk{}
  SHARED one
  Function Return &one
End Function
!TestCase{proc_defstr}
  pds_s{1}
  !CheckStrEq{zo$,"1 [] 0","l1"}
  pds_s{2,"hi"}
  !CheckStrEq{zo$,"2 [hi] 2","l2"}
  !CheckStrEq{"end","end","l3"}
!EndTestCase
!TestCase{proc_onlyopt}
  po_s{}
  !CheckStrEq{zo$,"s -1 [] 0","l1"}
  po_s{5}
  !CheckStrEq{zo$,"s 5 [] 0","l2"}
  po_s{6,"x"}
  !CheckStrEq{zo$,"s 6 [x] 1","l3"}
  r.l=po_f{}
  !CheckStrEq{zo$,"f -1 [] 0","l4"}
  !CheckEq{r,-1,"l5"}
  r=po_f{7}
  !CheckStrEq{zo$,"f 7 [] 0","l6"}
  !CheckEq{r,7,"l7"}
  !CheckStrEq{"end","end","l8"}
!EndTestCase
!TestCase{proc_optfirst}
  pof_s{}
  !CheckStrEq{zo$,"-1 -1","l1"}
  pof_s{1}
  !CheckStrEq{zo$,"1 -1","l2"}
  pof_s{1,2}
  !CheckStrEq{zo$,"1 2","l3"}
!EndTestCase
!TestCase{proc_optnt}
  pon_s{1}
  !CheckStrEq{zo$,"minus one","l1"}
!EndTestCase
!TestCase{proc_ten}
  p10_s{1,2,3,4,5,6,7,8,9,10}
  !CheckStrEq{zo$,"1 2 3 4 5 6 7 8 9 10","l1"}
  r=p10_f{1,2,3,4,5,6,7,8,9,10}
  !CheckEq{r,55,"l2"}
!EndTestCase
!TestCase{proc_opt7}
  po7_s{1,2,3,4,5,6}
  !CheckStrEq{zo$,"1 6 -1 -1 [] 0","l1"}
  po7_s{1,2,3,4,5,6,7}
  !CheckStrEq{zo$,"1 6 7 -1 [] 0","l2"}
  po7_s{1,2,3,4,5,6,7,8,"str"}
  !CheckStrEq{zo$,"1 6 7 8 [str] 3","l3"}
!EndTestCase
!TestCase{proc_types7}
  pt_sb{1,2,3,4,5,6}
  !CheckStrEq{zo$,"-1 -1 -1 -1","l1"}
  pt_sb{1,2,3,4,5,6,-7,8,9.5,10.25}
  !CheckStrEq{zo$,"-7 8 9.5 10.25","l2"}
  pt_sb{1,2,3,4,5,6,7,-8}
  !CheckStrEq{zo$,"7 -8 -1 -1","l3"}
  pt_ss{1,2,3,4,5,6}
  !CheckStrEq{zo$,"[] -1 [] -1","l4"}
  pt_ss{1,2,3,4,5,6,"gg",8,"t"+"t",12}
  !CheckStrEq{zo$,"[gg] 8 [tt] 12","l5"}
  pt_x$="x"
  pt_ss{1,2,3,4,5,6,pt_x$+pt_x$,-8,"t"+pt_x$+"t",-12}
  !CheckStrEq{zo$,"[xx] -8 [txt] -12","l6"}
!EndTestCase
!TestCase{proc_expr}
  x.l=2
  r=1+(2+px_o{x})
  !CheckEq{r,192,"l1"}
  r=1+(2+px_o{x,3}+px_o{x,3,4})
  !CheckEq{r,466,"l2"}
  r=5+(6+px_s{1,0,0,0,0,0,x})
  !CheckEq{r,-68,"l3"}
  r=5+(6+px_s{1,0,0,0,0,0,x,3})
  !CheckEq{r,332,"l4"}
  a$="<"+px_t{"a"}+px_t{"b",x}+">"
  !CheckStrEq{a$,"<a-1b2>","l5"}
!EndTestCase
!TestCase{proc_fast}
  r=pf_ff{2,3}
  !CheckEq{r,5,"l1"}
  pf_sf{7}
  !CheckEq{zn,7,"l2"}
!EndTestCase
!TestCase{proc_fastkeep}
  ; the two calls with no other call between them (a check would use the
  ; stack the local c is in)
  pfk_sf{1} : zm.l=zn : zn=0 : pfk_sf{2}
  !CheckEq{zm,42,"l1"}
  !CheckEq{zn,42,"l2"}
!EndTestCase
!TestCase{proc_ntret}
  *p.pnrt = pnr_mk{} : *p\x = 5
  !CheckEq{one\x,5,"l1"}
!EndTestCase
