; open-bug: the form of an open bug, a check that fails as a declared
; deviation shows (the self-test of the bundles, run with
; --require-deviation: REPRODUCED, stored and served with exit status 1)
!TestCase{one}
  a.l=1
  !CheckEq{a+1,3,"sum"}
!EndTestCase
