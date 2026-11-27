; proc-onlyopt7 - name{} of a Statement with seven optional parameters
; (spec ab3-compat/procedures; AmiBlitz3: a syntax error,
; proc-onlyopt7.test.toml)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement s{@a.l,@b.l,@c.l,@d.l,@e.l,@f.l,@g.l}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(f)+" "+Str$(g)
End Statement
!TestCase{proc_onlyopt7}
  s{1,2,3,4,5,6,77}
  !CheckStrEq{zo$,"1 6 77","l1"}
  s{}
  !CheckStrEq{zo$,"-1 -1 -1","l2"}
!EndTestCase
