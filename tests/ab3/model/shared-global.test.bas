; shared-global - SHARED in the main program shares the variable with every
; procedure after it: zinc increments the main program's zcnt (spec
; ab3-compat/syntax-rules). The SHARED stands in the batch init, the main
; program's code before the Statement.
!TestBatchInit
  SHARED zcnt.l
!EndTestBatchInit
Statement zinc{}
  zcnt + 1
End Statement
!TestCase{shared_global}
  zcnt = 5 : zinc{} : zinc{}
  !CheckEq{zcnt,7,"l1"}
!EndTestCase
