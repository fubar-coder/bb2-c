; proc-fastname - FAST without a space behind it is the name of the
; procedure (spec ab3-compat/procedures)
OPTIMIZE 4
!TestBatchInit
  zo$=""
!EndTestBatchInit
Statement FAST{a.l}
  SHARED zo$
  zo$="fast "+Str$(a)
End Statement
!TestCase{proc_fastname}
  FAST{3}
  !CheckStrEq{zo$,"fast 3","l1"}
!EndTestCase
