; model-no-deviation: a bug-orig test of the BASIC test model that declares
; no deviation of the original (it has no model-no-deviation.test.toml), so
; it would not show its bug: the runner reports it BROKEN although its check
; passes (ctest bug_orig_selftest_model_no_deviation; bug-orig-basic-tests
; task 1.2)
!TestCase{a}
  !CheckEq{1,1,"one"}
!EndTestCase
