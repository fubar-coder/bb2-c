; passes: a case that passes (the self-test of the bundles, a member stored
; and served)
!TestCase{one}
  a.l=1
  !CheckEq{a+1,2,"sum"}
!EndTestCase
