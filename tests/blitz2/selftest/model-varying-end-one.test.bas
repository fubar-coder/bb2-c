; model-varying-end-one: the source of model-varying-end.test.bas with only
; one of its alternative ends declared (ctest
; blitz2_selftest_model_varying_end_one, the check of task 3.3 with broken
; input): the cell whose program hangs (runargs hang) has no alternative
; that holds, so the case fails, naming the alternative and its failed
; assertions.
!TestCase{varies}
  !Check{-1,"before"}
  If Par$(1)="hang"
    s.l=Wait_($1000)
  EndIf
  !Check{-1,"after"}
!EndTestCase
