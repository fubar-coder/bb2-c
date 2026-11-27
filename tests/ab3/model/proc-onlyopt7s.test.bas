; proc-onlyopt7s - name{} of a Statement with seven optional parameters, a
; string as the 7th (spec ab3-compat/procedures; AmiBlitz3: a syntax
; error, proc-onlyopt7s.test.toml)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement s{@a.l,@b.l,@c.l,@d.l,@e.l,@f.l,@t.s}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(f)+" ["+t+"] "+Str$(Len(t))
End Statement
!TestCase{proc_onlyopt7s}
  s{1,2,3,4,5,6,"xy"}
  !CheckStrEq{zo$,"1 6 [xy] 2","l1"}
  s{}
  !CheckStrEq{zo$,"-1 -1 [] 0","l2"}
!EndTestCase
