; fails: a case whose check fails (the self-test of the bundles: not
; stored, its case test runs it alone)
!TestCase{one}
  a.l=1
  !CheckEq{a+1,3,"sum"}
!EndTestCase
