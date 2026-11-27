; proc-nested - parameters from the 7th on in nested calls (spec
; ab3-compat/procedures): parameters 7 to 10 are on the stack, where
; AmiBlitz3 passes them in static longs that an inner call overwrites
; (proc-nested.test.toml)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
; proc_nested: a call with seven parameters in the 8th argument of another
; call (AmiBlitz3: p7=100)
Function.l pn_g{a.l,b.l,c.l,d.l,e.l,f.l,gg.l}
  Function Return gg
End Function
Function.l pn_f{a.l,b.l,c.l,d.l,e.l,f.l,p7.l,p8.l}
  SHARED zo$
  zo$="p7="+Str$(p7)+" p8="+Str$(p8)
  Function Return p7
End Function
; proc_stack: calls with parameters on the stack inside the arguments of a
; library command, inside a string expression and in a recursion
Function.l ps_f{a.l,b.l,c.l,d.l,e.l,f.l,g.l,h.w}
  Function Return a+g*10+h*100
End Function
Function.s ps_fs{a.l,b.l,c.l,d.l,e.l,f.l,g.l,t.s}
  Function Return t+Str$(g)
End Function
Function.l ps_r{n.l,b.l,c.l,d.l,e.l,f.l,g.l,h.l}
  If n=0 Then Function Return g+h
  Function Return ps_r{n-1,b,c,d,e,f,g+1,h*2}
End Function
!TestCase{proc_nested}
  r.l=pn_f{1,2,3,4,5,6,7,pn_g{1,2,3,4,5,6,100}}
  !CheckStrEq{zo$,"p7=7 p8=100","l1"}
  !CheckEq{r,7,"l2"}
!EndTestCase
!TestCase{proc_stack}
  zs$="x"+Str$(ps_f{1,0,0,0,0,0,2,3})+"y"+Str$(ps_f{4,0,0,0,0,0,5,6})
  !CheckStrEq{zs$,"x321y654","l1"}
  a$="<"+ps_fs{1,2,3,4,5,6,7,"s"+Str$(ps_f{1,0,0,0,0,0,2,3})}+">"
  !CheckStrEq{a$,"<s3217>","l2"}
  k.l=ps_r{3,0,0,0,0,0,1,1}
  !CheckEq{k,12,"l3"}
!EndTestCase
