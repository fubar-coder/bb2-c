; label-proc - new syntax: a forward reference to a label of another
; procedure context (the main program's zlab from inside a statement) is
; accepted (spec ab3-compat/syntax-rules)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement zst{}
  SHARED zo$
  zp.l = ?zlab
  zo$="in "+Str$(zp <> 0)
End Statement
!TestCase{label_proc}
  zst{}
  !CheckStrEq{zo$,"in -1","l1"}
zlab:
  !CheckStrEq{"end","end","l2"}
!EndTestCase
