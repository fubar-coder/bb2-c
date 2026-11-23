; model-driver-broken: a source whose driver cannot be written (an unknown
; placeholder), as the build step basic_test_driver would fail on it
; (ctest blitz2_selftest_model_driver_broken)
!TestCase{one}
  a$="@NOSUCH@"
  !CheckStrEq{a$,"x","one"}
!EndTestCase
