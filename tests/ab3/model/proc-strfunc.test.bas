; proc-strfunc - string Functions with parameters on the stack, string
; parameters from the 7th on, string arguments with calls of string
; Functions in them (spec ab3-compat/procedures; AmiBlitz3 returns a wrong
; result from a Function with a string parameter from the 7th on: f3 and
; f5, proc-strfunc.test.toml)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Function.s f1{a.s,b.w}
  Function Return a+"/"+Str$(b)
End Function
Function.s f2{a.l,b.l,c.l,d.l,e.l,f.l,g.l}
  Function Return "g"+Str$(g)
End Function
Function.s f3{a.l,b.l,c.l,d.l,e.l,f.l,g.l,h.s}
  Function Return "h"+h
End Function
Function.s f4{a.s,b.l,c.l,d.l,e.l,f.l,g.l}
  Function Return a+Str$(g)
End Function
Function.l f5{a.s,b.l,c.l,d.l,e.l,f.l,g.l,h.s}
  SHARED zo$
  zo$=a+Str$(g)+h
  Function Return g
End Function
Function.s f6{a.s,b.s}
  Function Return a+"|"+b
End Function
!TestCase{proc_strfunc}
  zs$=f1{"a",2}
  !CheckStrEq{zs$,"a/2","l1"}
  zs$=f6{"<"+f1{"a",2}+">","["+f4{"y",2,3,4,5,6,7}+"]"}
  !CheckStrEq{zs$,"<a/2>|[y7]","l2"}
  zs$=f2{1,2,3,4,5,6,7}
  !CheckStrEq{zs$,"g7","l3"}
  zs$=f3{1,2,3,4,5,6,7,"x"}
  !CheckStrEq{zs$,"hx","l4"}
  zs$=f4{"y",2,3,4,5,6,7}
  !CheckStrEq{zs$,"y7","l5"}
  r.l=f5{"y",2,3,4,5,6,7,"z"}
  !CheckStrEq{zo$,"y7z","l6"}
  !CheckEq{r,7,"l7"}
!EndTestCase
