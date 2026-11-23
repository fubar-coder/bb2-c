; touched: a case that passes, behind the hanging one (the self-test of the
; bundles: it runs again in the next session and is stored; the test then
; changes this source, so its case test finds the stored verdict stale and
; runs the case alone)
!TestCase{one}
  a.l=2
  !CheckEq{a*2,4,"product"}
!EndTestCase
