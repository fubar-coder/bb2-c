; proc-onlyopt8 - name{} of a Statement with eight optional parameters
; (spec ab3-compat/procedures; AmiBlitz3: a syntax error from the 7th,
; proc-onlyopt8.test.toml)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement s{@a.l,@b.l,@c.l,@d.l,@e.l,@f.l,@g.l,@h.l}
  SHARED zo$
  zo$=Str$(a)+" "+Str$(h)
End Statement
!TestCase{proc_onlyopt8}
  s{}
  !CheckStrEq{zo$,"-1 -1","l1"}
  s{1,2,3,4,5,6,7}
  !CheckStrEq{zo$,"1 -1","l2"}
!EndTestCase
